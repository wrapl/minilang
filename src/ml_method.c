#include "ml_method.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>

#ifdef ML_THREADSAFE
#include <pthread.h>

typedef struct {
	inthash_t Cache[1];
	size_t Version;
} ml_thread_methods_t;
#endif

typedef struct ml_method_cached_t ml_method_cached_t;
typedef struct ml_method_definition_t ml_method_definition_t;

struct ml_methods_t {
	ml_type_t *Type;
	ml_methods_t *Parent;
	inthash_t Cache[1];
	inthash_t Definitions[1];
	inthash_t Methods[1];
#ifdef ML_THREADSAFE
	pthread_key_t ThreadKey;
	pthread_rwlock_t Lock[1];
	size_t Version;
#endif
};

static void ml_methods_call(ml_state_t *Caller, ml_methods_t *Methods, int Count, ml_value_t **Args) {
	ml_state_t *State = ml_state_new(Caller);
	State->Context->Values[ML_METHODS_INDEX] = Methods;
	ml_value_t *Function = Args[0];
	return ml_call(State, Function, Count - 1, Args + 1);
}

ML_TYPE(MLMethodsT, (), "methods",
	.call = (void *)ml_methods_call
);

static ml_methods_t MLRootMethods[1] = {{
	NULL, NULL,
	{INTHASH_INIT},
	{INTHASH_INIT},
	{INTHASH_INIT}
}};

#ifdef ML_THREADSAFE
static void ml_methods_clean(ml_methods_t *Methods, void *Data) {
	pthread_key_delete(Methods->ThreadKey);
}
#endif

ml_methods_t *ml_methods_context_new(ml_context_t *Context) {
	ml_methods_t *Methods = new(ml_methods_t);
	Methods->Type = MLMethodsT;
	Methods->Parent = Context->Values[ML_METHODS_INDEX];
#ifdef ML_THREADSAFE
	pthread_key_create(&Methods->ThreadKey, GC_free);
	pthread_rwlock_init(Methods->Lock, NULL);
	GC_register_finalizer(Methods, (GC_finalization_proc)ml_methods_clean, NULL, NULL, NULL);
#endif
	Context->Values[ML_METHODS_INDEX] = Methods;
	return Methods;
}

struct ml_method_definition_t {
	ml_method_definition_t *Next;
	ml_value_t *Callback;
	int Count, Variadic;
	ml_type_t *Types[];
};

static __attribute__ ((pure)) unsigned int ml_method_definition_score(ml_method_definition_t *Definition, int Count, ml_type_t **Types) {
	unsigned int Score = 1;
	if (Definition->Count > Count) return 0;
	if (Definition->Count < Count) {
		if (!Definition->Variadic) return 0;
		Count = Definition->Count;
	} else if (!Definition->Variadic) {
		Score = 2;
	}
	for (ssize_t I = Count; --I >= 0;) {
		ml_type_t *Type = Definition->Types[I];
		if (ml_is_subtype(Types[I], Type)) goto found;
		return 0;
	found:
		Score += 5 + Type->Rank;
	}
	return Score;
}

struct ml_method_cached_t {
	ml_method_cached_t *Next, *MethodNext;
	ml_method_t *Method;
	ml_method_definition_t *Definition;
	int Count, Score;
	ml_type_t *Types[];
};

static ml_method_cached_t *ml_method_search_entry(ml_methods_t *Methods, ml_method_t *Method, int Count, ml_type_t **Types, uint64_t Hash);

static __attribute__ ((noinline)) ml_method_cached_t *ml_method_search_entry2(ml_methods_t *Methods, ml_method_t *Method, int Count, ml_type_t **Types, uint64_t Hash, ml_method_cached_t *Cached) {
#ifdef ML_THREADSAFE
	pthread_rwlock_wrlock(Methods->Lock);
#endif
	unsigned int BestScore = 0;
	ml_method_definition_t *BestDefinition = NULL;
	ml_method_definition_t *Definition = inthash_search(Methods->Definitions, (uintptr_t)Method);
	while (Definition) {
		unsigned int Score = ml_method_definition_score(Definition, Count, Types);
		if (Score > BestScore) {
			BestScore = Score;
			BestDefinition = Definition;
		}
		Definition = Definition->Next;
	}
	if (Methods->Parent) {
		ml_method_cached_t *Cached2 = ml_method_search_entry(Methods->Parent, Method, Count, Types, Hash);
		if (Cached2 && Cached2->Score > BestScore) {
			BestScore = Cached2->Score;
			BestDefinition = Cached2->Definition;
		}
	}
	if (!BestDefinition) {
#ifdef ML_THREADSAFE
		pthread_rwlock_unlock(Methods->Lock);
#endif
		return NULL;
	}
	if (!Cached) {
		Cached = xnew(ml_method_cached_t, Count, ml_type_t *);
		Cached->Method = Method;
		Cached->Count = Count;
		for (int I = 0; I < Count; ++I) Cached->Types[I] = Types[I];
		Cached->Next = inthash_insert(Methods->Cache, Hash, Cached);
		Cached->MethodNext = inthash_insert(Methods->Methods, (uintptr_t)Method, Cached);
#ifdef ML_THREADSAFE
		++Methods->Version;
#endif
	}
	Cached->Definition = BestDefinition;
	Cached->Score = BestScore;
#ifdef ML_THREADSAFE
	pthread_rwlock_unlock(Methods->Lock);
#endif
	return Cached;
}

#ifdef ML_THREADSAFE

static __attribute__ ((noinline)) void ml_thread_methods_copy(ml_thread_methods_t *ThreadMethods, ml_methods_t *Methods) {
	pthread_rwlock_rdlock(Methods->Lock);
	inthash_t *Source = Methods->Cache;
	inthash_t *Target = ThreadMethods->Cache;
	int Size = Source->Size;
	if (Target->Size != Size) {
		Target->Size = Size;
		Target->Keys = anew(uintptr_t, Size);
		Target->Values = anew(void *, Size);
	}
	memcpy(Target->Keys, Source->Keys, Size * sizeof(uintptr_t));
	memcpy(Target->Values, Source->Values, Size * sizeof(void *));
	Target->Space = Source->Space;
	ThreadMethods->Version = Methods->Version;
	pthread_rwlock_unlock(Methods->Lock);
}

#endif

static inline ml_method_cached_t *ml_method_search_entry(ml_methods_t *Methods, ml_method_t *Method, int Count, ml_type_t **Types, uint64_t Hash) {
#ifdef ML_THREADSAFE
	ml_thread_methods_t *ThreadMethods = pthread_getspecific(Methods->ThreadKey);
	if (!ThreadMethods) {
		ThreadMethods = GC_malloc_uncollectable(sizeof(ml_thread_methods_t));
		pthread_setspecific(Methods->ThreadKey, ThreadMethods);
		ml_thread_methods_copy(ThreadMethods, Methods);
	} else if (ThreadMethods->Version != Methods->Version) {
		ml_thread_methods_copy(ThreadMethods, Methods);
	}
	inthash_t *Cache = ThreadMethods->Cache;
#else
	inthash_t *Cache = Methods->Cache;
#endif
	ml_method_cached_t *Cached = inthash_search(Cache, Hash);
	while (Cached) {
		if (Cached->Method != Method) goto next;
		if (Cached->Count != Count) goto next;
		for (int I = 0; I < Count; ++I) {
			if (Cached->Types[I] != Types[I]) goto next;
		}
		if (!Cached->Definition) break;
		return Cached;
	next:
		Cached = Cached->Next;
	}
	return ml_method_search_entry2(Methods, Method, Count, Types, Hash, Cached);
}

static inline uintptr_t rotl(uintptr_t X, unsigned int N) {
	const unsigned int Mask = (CHAR_BIT * sizeof(uintptr_t) - 1);
	return (X << (N & Mask)) | (X >> ((-N) & Mask ));
}

static __attribute__ ((noinline)) ml_value_t *ml_method_search2(ml_state_t *Caller, ml_method_t *Method, int Count, ml_value_t **Args) {
	// Use alloca here, VLA prevents TCO.
	ml_type_t **Types = alloca(Count * sizeof(ml_type_t *));
	uintptr_t Hash = (uintptr_t)Method;
	for (ssize_t I = Count; --I >= 0;) {
		ml_type_t *Type = Types[I] = ml_typeof_deref(Args[I]);
		Hash = rotl(Hash, 1) ^ (uintptr_t)Type;
	}
	ml_methods_t *Methods = Caller->Context->Values[ML_METHODS_INDEX];
	ml_method_cached_t *Cached = ml_method_search_entry(Methods, Method, Count, Types, Hash);
	if (Cached) return Cached->Definition->Callback;
	return NULL;
}

#define ML_SMALL_METHOD_COUNT 8

static
#ifdef ML_NANBOXING
	inline
#else
	__attribute__ ((noinline))
#endif
	ml_value_t *ml_method_search(ml_state_t *Caller, ml_method_t *Method, int Count, ml_value_t **Args) {
	// TODO: Use generation numbers to check Methods->Parent for invalidated definitions
	if (Count > ML_SMALL_METHOD_COUNT) return ml_method_search2(Caller, Method, Count, Args);
	ml_type_t *Types[ML_SMALL_METHOD_COUNT];
	uintptr_t Hash = (uintptr_t)Method;
	for (ssize_t I = Count; --I >= 0;) {
		ml_type_t *Type = Types[I] = ml_typeof_deref(Args[I]);
		Hash = rotl(Hash, 1) ^ (uintptr_t)Type;
	}
	ml_methods_t *Methods = Caller->Context->Values[ML_METHODS_INDEX];
	ml_method_cached_t *Cached = ml_method_search_entry(Methods, Method, Count, Types, Hash);
	if (Cached) return Cached->Definition->Callback;
	return NULL;
}

void ml_method_insert(ml_methods_t *Methods, ml_method_t *Method, ml_value_t *Callback, int Count, int Variadic, ml_type_t **Types) {
	ml_method_definition_t *Definition = xnew(ml_method_definition_t, Count, ml_type_t *);
	Definition->Callback = Callback;
	Definition->Count = Count;
	Definition->Variadic = Variadic;
	memcpy(Definition->Types, Types, Count * sizeof(ml_type_t *));
#ifdef ML_THREADSAFE
	pthread_rwlock_wrlock(Methods->Lock);
#endif
	Definition->Next = inthash_insert(Methods->Definitions, (uintptr_t)Method, Definition);
	ml_method_cached_t *Cached = inthash_search(Methods->Methods, (uintptr_t)Method);
#ifdef ML_THREADSAFE
	pthread_rwlock_unlock(Methods->Lock);
#endif
	while (Cached) {
		Cached->Definition = NULL;
		Cached = Cached->MethodNext;
	}
}

void ml_method_define(ml_value_t *Method, ml_value_t *Function, int Variadic, ...) {
	int Count = 0;
	va_list Args;
	va_start(Args, Variadic);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) ++Count;
	va_end(Args);
	ml_type_t *Types[Count], **T = Types;
	va_start(Args, Variadic);
	while ((Type = va_arg(Args, ml_type_t *))) *T++ = Type;
	va_end(Args);
	ml_method_insert(MLRootMethods, (ml_method_t *)Method, Function, Count, Variadic, Types);
}

static stringmap_t Methods[1] = {STRINGMAP_INIT};

const char *ml_method_name(const ml_value_t *Value) {
	return ((ml_method_t *)Value)->Name;
}

static long ml_method_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	ml_method_t *Method = (ml_method_t *)Value;
	long Hash = 5381;
	for (const char *P = Method->Name; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

static void __attribute__ ((noinline)) ml_method_not_found(ml_state_t *Caller, ml_method_t *Method, int Count, ml_value_t **Args) {
	int Length = 4;
	for (int I = 0; I < Count; ++I) Length += strlen(ml_typeof(Args[I])->Name) + 2;
	char *Types = snew(Length);
	Types[0] = 0;
	char *P = Types;
#ifdef __MINGW32__
	for (int I = 0; I < Count; ++I) {
		strcpy(P, Args[I]->Type->Path);
		P += strlen(Args[I]->Type->Path);
		strcpy(P, ", ");
		P += 2;
	}
#else
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = ml_deref(Args[I]);
		P = stpcpy(stpcpy(P, ml_typeof(Arg)->Name), ", ");
	}
#endif
	P[-2] = 0;
	ML_RETURN(ml_error("MethodError", "no method found for %s(%s)", Method->Name, Types));
}

static void ml_method_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_method_t *Method = (ml_method_t *)Value;
	ml_value_t *Callback = ml_method_search(Caller, Method, Count, Args);

	if (__builtin_expect(Callback != NULL, 1)) {
		return ml_call(Caller, Callback, Count, Args);
	} else {
		return ml_method_not_found(Caller, Method, Count, Args);
	}
}

ML_TYPE(MLMethodT, (MLFunctionT), "method",
//!method
	.hash = ml_method_hash,
	.call = ml_method_call
);

ml_value_t *ml_method(const char *Name) {
	if (!Name) {
		ml_method_t *Method = new(ml_method_t);
		Method->Type = MLMethodT;
		asprintf((char **)&Method->Name, "<anon:0x%lx>", (uintptr_t)Method);
		return (ml_value_t *)Method;
	}
	ML_RUNTIME_LOCK();
	ml_method_t **Slot = (ml_method_t **)stringmap_slot(Methods, Name);
	if (!Slot[0]) {
		ml_method_t *Method = new(ml_method_t);
		Method->Type = MLMethodT;
		Method->Name = Name;
		Slot[0] = Method;
	}
	ML_RUNTIME_UNLOCK();
	return (ml_value_t *)Slot[0];
}

ml_value_t *ml_method_anon(const char *Name) {
	ml_method_t *Method = new(ml_method_t);
	Method->Type = MLMethodT;
	Method->Name = Name;
	return (ml_value_t *)Method;
}

ML_METHOD(MLMethodT) {
//!method
//>method
	return ml_method(NULL);
}

ML_METHOD(MLMethodT, MLStringT) {
//!method
//<Name
//>method
	return ml_method(ml_string_value(Args[0]));
}

void ml_method_by_name(const char *Name, void *Data, ml_callback_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)ml_method(Name);
	int Count = 0;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) ++Count;
	va_end(Args);
	ml_type_t *Types[Count], **T = Types;
	va_start(Args, Callback);
	while ((Type = va_arg(Args, ml_type_t *))) *T++ = Type;
	va_end(Args);
	ml_method_insert(MLRootMethods, Method, ml_cfunction(Data, Callback), Count, 1, Types);
}

void ml_method_by_value(ml_value_t *Value, void *Data, ml_callback_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)Value;
	int Count = 0;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) ++Count;
	va_end(Args);
	ml_type_t *Types[Count], **T = Types;
	va_start(Args, Callback);
	while ((Type = va_arg(Args, ml_type_t *))) *T++ = Type;
	va_end(Args);
	ml_method_insert(MLRootMethods, Method, ml_cfunction(Data, Callback), Count, 1, Types);
}

void ml_methodx_by_name(const char *Name, void *Data, ml_callbackx_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)ml_method(Name);
	int Count = 0;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) ++Count;
	va_end(Args);
	ml_type_t *Types[Count], **T = Types;
	va_start(Args, Callback);
	while ((Type = va_arg(Args, ml_type_t *))) *T++ = Type;
	va_end(Args);
	ml_method_insert(MLRootMethods, Method, ml_cfunctionx(Data, Callback), Count, 1, Types);
}

void ml_methodx_by_value(ml_value_t *Value, void *Data, ml_callbackx_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)Value;
	int Count = 0;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) ++Count;
	va_end(Args);
	ml_type_t *Types[Count], **T = Types;
	va_start(Args, Callback);
	while ((Type = va_arg(Args, ml_type_t *))) *T++ = Type;
	va_end(Args);
	ml_method_insert(MLRootMethods, Method, ml_cfunctionx(Data, Callback), Count, 1, Types);
}

void ml_method_by_array(ml_value_t *Value, ml_value_t *Function, int Count, ml_type_t **Types) {
	ml_method_t *Method = (ml_method_t *)Value;
	ml_method_insert(MLRootMethods, Method, Function, Count, 1, Types);
}

ML_METHOD(MLStringT, MLMethodT) {
//!method
//>string
	ml_method_t *Method = (ml_method_t *)Args[0];
	return ml_string_format(":%s", Method->Name);
}

ML_METHOD("append", MLStringBufferT, MLMethodT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_method_t *Method = (ml_method_t *)Args[1];
	ml_stringbuffer_add(Buffer, ":", 1);
	ml_stringbuffer_add(Buffer, Method->Name, strlen(Method->Name));
	return MLSome;
}

/*ML_METHODVX("[]", MLMethodT) {
	ml_method_t *Method = (ml_method_t *)Args[0];
	for (int I = 1; I < Count; ++I) ML_CHECKX_ARG_TYPE(I, MLTypeT);
	ml_value_t *Matches = ml_list();
	ml_type_t **Types = (ml_type_t **)Args + 1;
	--Count;
	ml_methods_t *Methods = Caller->Context->Values[ML_METHODS_INDEX];
	do {
		ml_method_definition_t *Definition = inthash_search(Methods->Definitions, (uintptr_t)Method);
		while (Definition) {
			unsigned int Score = ml_method_definition_score(Definition, Count, Types);
			if (Score) {
				ml_value_t *Match = ml_tuple(2);
				ml_tuple_set(Match, 1, Definition->Callback);
				ml_tuple_set(Match, 2, ml_integer(Score));
				ml_list_put(Matches, Match);
			}
			Definition = Definition->Next;
		}
		Methods = Methods->Parent;
	} while (Methods);
	ML_RETURN(Matches);
}*/

static ML_METHOD_DECL(RangeMethod, "..");

static inline void ml_method_set(ml_context_t *Context, int NumTypes, int Variadic, ml_value_t **Args, ml_value_t *Function) {
	// Use alloca here, VLA prevents TCO.
	ml_type_t **Types = alloca(NumTypes * sizeof(ml_type_t *));
	for (int I = 1; I <= NumTypes; ++I) {
		if (Args[I] == MLNil) {
			Types[I - 1] = MLNilT;
		} else {
			Types[I - 1] = (ml_type_t *)Args[I];
		}
	}
	ml_method_t *Method = (ml_method_t *)Args[0];
	ml_methods_t *Methods = Context->Values[ML_METHODS_INDEX];
	ml_method_insert(Methods, Method, Function, NumTypes, Variadic, Types);
}

ML_FUNCTIONX(MLMethodSet) {
//@method::set
//<Method
//<Types...:type
//<Function:function
//>Function
	ML_CHECKX_ARG_COUNT(2);
	if (ml_is(Args[0], MLTypeT)) Args[0] = ((ml_type_t *)Args[0])->Constructor;
	ML_CHECKX_ARG_TYPE(0, MLMethodT);
	int NumTypes = Count - 2, Variadic = 0;
	if (Count >= 3 && Args[Count - 2] == RangeMethod) {
		Variadic = 1;
		--NumTypes;
	}
	for (int I = 1; I <= NumTypes; ++I) {
		if (Args[I] != MLNil) {
			ML_CHECKX_ARG_TYPE(I, MLTypeT);
		}
	}
	ML_CHECKX_ARG_TYPE(Count - 1, MLFunctionT);
	ml_value_t *Function = Args[Count - 1];
	ml_method_set(Caller->Context, NumTypes, Variadic, Args, Function);
	ML_RETURN(Function);
}

ML_FUNCTIONX(MLMethodContext) {
//@method::context
//>methods
	ml_methods_t *Methods = new(ml_methods_t);
	Methods->Type = MLMethodsT;
	Methods->Parent = Caller->Context->Values[ML_METHODS_INDEX];
	ML_RETURN(Methods);
}

void ml_method_init() {
#ifdef ML_THREADSAFE
	pthread_key_create(&MLRootMethods->ThreadKey, GC_free);
	pthread_rwlock_init(MLRootMethods->Lock, NULL);
#endif
	ml_context_set(&MLRootContext, ML_METHODS_INDEX, MLRootMethods);
#include "ml_method_init.c"
	MLRootMethods->Type = MLMethodsT;
	stringmap_insert(MLMethodT->Exports, "set", MLMethodSet);
	stringmap_insert(MLMethodT->Exports, "context", MLMethodContext);
	ml_method_by_value(MLMethodT->Constructor, NULL, ml_identity, MLMethodT, NULL);
}

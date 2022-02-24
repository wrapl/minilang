#include "ml_method.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>

#ifdef ML_THREADSAFE
#include <stdatomic.h>
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "method"

typedef struct ml_method_definition_t ml_method_definition_t;

struct ml_method_definition_t {
	ml_method_definition_t *Next;
	ml_value_t *Callback;
	int Count, Variadic;
	ml_type_t *Types[];
};

struct ml_methods_t {
	ml_type_t *Type;
	ml_methods_t *Parent;
	inthash_t Cache[1];
	inthash_t Definitions[1];
	inthash_t Methods[1];
#ifdef ML_THREADSAFE
	volatile atomic_flag Lock[1];
#endif
};

static void ml_methods_call(ml_state_t *Caller, ml_methods_t *Methods, int Count, ml_value_t **Args) {
	ml_state_t *State = ml_state(Caller);
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
#ifdef ML_THREADSAFE
	, {ATOMIC_FLAG_INIT}
#endif
}};

ml_methods_t *ml_methods_context(ml_context_t *Context) {
	ml_methods_t *Methods = new(ml_methods_t);
	Methods->Type = MLMethodsT;
	Methods->Parent = Context->Values[ML_METHODS_INDEX];
#ifdef ML_THREADSAFE
	Methods->Lock[0] = (atomic_flag)ATOMIC_FLAG_INIT;
#endif
	Context->Values[ML_METHODS_INDEX] = Methods;
	return Methods;
}

static inline void ml_methods_lock(ml_methods_t *Methods) {
#ifdef ML_THREADSAFE
	while (atomic_flag_test_and_set(Methods->Lock));
#endif
}

static inline void ml_methods_unlock(ml_methods_t *Methods) {
#ifdef ML_THREADSAFE
	atomic_flag_clear(Methods->Lock);
#endif
}

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

static ml_method_cached_t *ml_method_search_entry(ml_methods_t *Methods, ml_method_t *Method, int Count, ml_type_t **Types, uint64_t Hash);

static __attribute__ ((noinline)) ml_method_cached_t *ml_method_search_entry2(ml_methods_t *Methods, ml_method_t *Method, int Count, ml_type_t **Types, uint64_t Hash, ml_method_cached_t *Cached) {
	unsigned int BestScore = 0;
	ml_value_t *BestCallback = NULL;
	ml_method_definition_t *Definition = inthash_search(Methods->Definitions, (uintptr_t)Method);
	while (Definition) {
		unsigned int Score = ml_method_definition_score(Definition, Count, Types);
		if (Score > BestScore) {
			BestScore = Score;
			BestCallback = Definition->Callback;
		}
		Definition = Definition->Next;
	}
	if (Methods->Parent) {
		ml_method_cached_t *Cached2 = ml_method_search_entry(Methods->Parent, Method, Count, Types, Hash);
		if (Cached2 && Cached2->Score > BestScore) {
			BestScore = Cached2->Score;
			BestCallback = Cached2->Callback;
		}
	}
	if (!BestCallback) {
		ml_methods_unlock(Methods);
		return NULL;
	}
	if (!Cached) {
		Cached = xnew(ml_method_cached_t, Count, ml_type_t *);
		Cached->Method = Method;
		Cached->Count = Count;
		for (int I = 0; I < Count; ++I) Cached->Types[I] = Types[I];
		Cached->Next = inthash_insert(Methods->Cache, Hash, Cached);
		Cached->MethodNext = inthash_insert(Methods->Methods, (uintptr_t)Method, Cached);
	}
	Cached->Callback = BestCallback;
	Cached->Score = BestScore;
	ml_methods_unlock(Methods);
	return Cached;
}

static inline ml_method_cached_t *ml_method_search_entry(ml_methods_t *Methods, ml_method_t *Method, int Count, ml_type_t **Types, uint64_t Hash) {
	ml_methods_lock(Methods);
	inthash_t *Cache = Methods->Cache;
	ml_method_cached_t *Cached = inthash_search_inline(Cache, Hash);
	while (Cached) {
		if (Cached->Method != Method) goto next;
		if (Cached->Count != Count) goto next;
		for (int I = 0; I < Count; ++I) {
			if (Cached->Types[I] != Types[I]) goto next;
		}
		if (!Cached->Callback) break;
		ml_methods_unlock(Methods);
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
	ml_type_t *Types[Count];
	uintptr_t Hash = (uintptr_t)Method;
	for (ssize_t I = Count; --I >= 0;) {
		ml_type_t *Type = Types[I] = ml_typeof_deref(Args[I]);
		Hash = rotl(Hash, 1) ^ (uintptr_t)Type;
	}
	ml_methods_t *Methods = Caller->Context->Values[ML_METHODS_INDEX];
	ml_method_cached_t *Cached = ml_method_search_entry(Methods, Method, Count, Types, Hash);
	if (Cached) return Cached->Callback;
	return NULL;
}

#define ML_SMALL_METHOD_COUNT 8

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
	if (Cached) return Cached->Callback;
	return NULL;
}

static __attribute__ ((noinline)) ml_method_cached_t *ml_method_search_cached2(ml_methods_t *Methods, ml_method_t *Method, int Count, ml_value_t **Args) {
	ml_type_t *Types[Count];
	uintptr_t Hash = (uintptr_t)Method;
	for (ssize_t I = Count; --I >= 0;) {
		ml_type_t *Type = Types[I] = ml_typeof_deref(Args[I]);
		Hash = rotl(Hash, 1) ^ (uintptr_t)Type;
	}
	return ml_method_search_entry(Methods, Method, Count, Types, Hash);
}

ml_method_cached_t *ml_method_search_cached(ml_methods_t *Methods, ml_method_t *Method, int Count, ml_value_t **Args) {
	// TODO: Use generation numbers to check Methods->Parent for invalidated definitions
	Methods = Methods ?: MLRootMethods;
	if (Count > ML_SMALL_METHOD_COUNT) return ml_method_search_cached2(Methods, Method, Count, Args);
	ml_type_t *Types[ML_SMALL_METHOD_COUNT];
	uintptr_t Hash = (uintptr_t)Method;
	for (ssize_t I = Count; --I >= 0;) {
		ml_type_t *Type = Types[I] = ml_typeof_deref(Args[I]);
		Hash = rotl(Hash, 1) ^ (uintptr_t)Type;
	}
	return ml_method_search_entry(Methods, Method, Count, Types, Hash);
}

void ml_method_insert(ml_methods_t *Methods, ml_method_t *Method, ml_value_t *Callback, int Count, int Variadic, ml_type_t **Types) {
	if (!ml_is((ml_value_t *)Method, MLMethodT)) {
		fprintf(stderr, "Internal error: attempting to define method for non-method value\n");
		exit(-1);
	}
	ml_method_definition_t *Definition = xnew(ml_method_definition_t, Count, ml_type_t *);
	Definition->Callback = Callback;
	Definition->Count = Count;
	Definition->Variadic = Variadic;
	memcpy(Definition->Types, Types, Count * sizeof(ml_type_t *));
	ml_methods_lock(Methods);
	Definition->Next = inthash_insert(Methods->Definitions, (uintptr_t)Method, Definition);
	ml_method_cached_t *Cached = inthash_search(Methods->Methods, (uintptr_t)Method);
	ml_methods_unlock(Methods);
	while (Cached) {
		Cached->Callback = NULL;
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

__attribute__ ((noinline)) ml_value_t *ml_no_method_error(ml_method_t *Method, int Count, ml_value_t **Args) {
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
	return ml_error("MethodError", "no method found for %s(%s)", Method->Name, Types);
}

static void ml_method_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_method_t *Method = (ml_method_t *)Value;
	ml_value_t *Callback = ml_method_search(Caller, Method, Count, Args);

	if (__builtin_expect(Callback != NULL, 1)) {
		return ml_call(Caller, Callback, Count, Args);
	} else {
		ML_RETURN(ml_no_method_error(Method, Count, Args));
	}
}

ML_TYPE(MLMethodT, (MLFunctionT), "method",
//!method
	.hash = ml_method_hash,
	.call = ml_method_call
);

#ifdef ML_THREADSAFE

static volatile atomic_flag MLMethodsLock[1] = {ATOMIC_FLAG_INIT};

#define ML_METHODS_LOCK() while (atomic_flag_test_and_set(MLMethodsLock))

#define ML_METHODS_UNLOCK() atomic_flag_clear(MLMethodsLock)

#else

#define ML_METHODS_LOCK() {}
#define ML_METHODS_UNLOCK() {}

#endif

ml_value_t *ml_method(const char *Name) {
	if (!Name) {
		ml_method_t *Method = new(ml_method_t);
		Method->Type = MLMethodT;
		asprintf((char **)&Method->Name, "<anon:0x%lx>", (uintptr_t)Method);
		return (ml_value_t *)Method;
	}
	ML_METHODS_LOCK();
	ml_method_t **Slot = (ml_method_t **)stringmap_slot(Methods, Name);
	if (!Slot[0]) {
		ml_method_t *Method = new(ml_method_t);
		Method->Type = MLMethodT;
		Method->Name = Name;
		Slot[0] = Method;
	}
	ML_METHODS_UNLOCK();
	return (ml_value_t *)Slot[0];
}

ml_value_t *ml_method_anon(const char *Name) {
	ml_method_t *Method = new(ml_method_t);
	Method->Type = MLMethodT;
	Method->Name = Name;
	return (ml_value_t *)Method;
}

ML_METHOD(MLMethodT) {
//>method
// Returns a new anonymous method.
	return ml_method(NULL);
}

ML_METHOD(MLMethodT, MLStringT) {
//<Name
//>method
// Returns the method with name :mini:`Name`.
	return ml_method(ml_string_value(Args[0]));
}

ML_METHOD("name", MLMethodT) {
//<Method
//>string
// Returns the name of :mini:`Method`.
	ml_method_t *Method = (ml_method_t *)Args[0];
	return ml_string(Method->Name, -1);
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

void ml_method_by_value(void *Value, void *Data, ml_callback_t Callback, ...) {
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

void ml_methodx_by_value(void *Value, void *Data, ml_callbackx_t Callback, ...) {
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

ML_METHOD("append", MLStringBufferT, MLMethodT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_method_t *Method = (ml_method_t *)Args[1];
	ml_stringbuffer_put(Buffer, ':');
	ml_stringbuffer_write(Buffer, Method->Name, strlen(Method->Name));
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

typedef struct {
	ml_type_t *Type;
	ml_value_t *Default;
	inthash_t Cases[1];
} ml_method_switch_t;

static void ml_method_switch(ml_state_t *Caller, ml_method_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	ml_value_t *Index = inthash_search(Switch->Cases, (uintptr_t)Arg);
	ML_RETURN(Index ?: Switch->Default);
}

ML_TYPE(MLMethodSwitchT, (MLFunctionT), "method-switch",
//!internal
	.call = (void *)ml_method_switch
);

ML_FUNCTION(MLMethodSwitch) {
//!internal
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_method_switch_t *Switch = new(ml_method_switch_t);
	Switch->Type = MLMethodSwitchT;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, MLMethodT)) {
				inthash_insert(Switch->Cases, (uintptr_t)Iter->Value, ml_integer(I));
			} else {
				return ml_error("ValueError", "Unsupported value in method case");
			}
		}
	}
	Switch->Default = ml_integer(Count);
	return (ml_value_t *)Switch;
}

ML_METHOD_DECL(RangeMethod, "..");

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

ML_METHODX("list", MLMethodT) {
	ml_value_t *Results = ml_map();
	ml_methods_t *Methods = Caller->Context->Values[ML_METHODS_INDEX];
	do {
		for (ml_method_definition_t *Definition = (ml_method_definition_t *)inthash_search(Methods->Definitions, (uintptr_t)Args[0]); Definition; Definition = Definition->Next) {
			ml_value_t *Signature = ml_tuple(Definition->Count + Definition->Variadic);
			for (int I = 0; I < Definition->Count; ++I) {
				ml_tuple_set(Signature, I + 1, (ml_value_t *)Definition->Types[I]);
			}
			if (Definition->Variadic) ml_tuple_set(Signature, Definition->Count + 1, RangeMethod);
			ml_map_node_t *Node = ml_map_slot(Results, Signature);
			if (!Node->Value) {
				const char *Source;
				int Line;
				if (ml_function_source(Definition->Callback, &Source, &Line)) {
					Node->Value = ml_tuplev(2, ml_string(Source, -1), ml_integer(Line));
				} else {
					Node->Value = MLNil;
				}
			}
		}
		Methods = Methods->Parent;
	} while (Methods);
	ML_RETURN(Results);
}

ML_FUNCTIONX(MLMethodContext) {
//@method::context
//>methods
	ml_methods_t *Methods = new(ml_methods_t);
	Methods->Type = MLMethodsT;
	Methods->Parent = Caller->Context->Values[ML_METHODS_INDEX];
	ML_RETURN(Methods);
}

static int ml_method_list_fn(const char *Name, ml_value_t *Method, ml_value_t *Result) {
	ml_list_put(Result, Method);
	return 0;
}

ML_FUNCTION(MLMethodList) {
//@method::list
//>list[method]
	ml_value_t *Result = ml_list();
	stringmap_foreach(Methods, Result, (void *)ml_method_list_fn);
	return Result;

}

void ml_method_init() {
	ml_context_set(&MLRootContext, ML_METHODS_INDEX, MLRootMethods);
#include "ml_method_init.c"
	MLRootMethods->Type = MLMethodsT;
	stringmap_insert(MLMethodT->Exports, "switch", ml_inline_call_macro(MLMethodSwitch));
	stringmap_insert(MLMethodT->Exports, "set", MLMethodSet);
	stringmap_insert(MLMethodT->Exports, "context", MLMethodContext);
	stringmap_insert(MLMethodT->Exports, "list", MLMethodList);
	ml_method_by_value(MLMethodT->Constructor, NULL, ml_identity, MLMethodT, NULL);
}

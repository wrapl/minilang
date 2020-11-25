#include "ml_method.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>

ML_METHOD_DECL(MLMethodOf, "method::of");

typedef struct ml_method_cached_t ml_method_cached_t;
typedef struct ml_method_definition_t ml_method_definition_t;

struct ml_method_t {
	const ml_type_t *Type;
	const char *Name;
};

struct ml_methods_t {
	ml_methods_t *Parent;
	inthash_t Cache[1];
	inthash_t Definitions[1];
	inthash_t Methods[1];
};

static ml_methods_t MLRootMethods[1] = {{NULL, {INTHASH_INIT}, {INTHASH_INIT}}};

void ml_methods_context_new(ml_context_t *Context) {
	ml_methods_t *Methods = new(ml_methods_t);
	Methods->Parent = Context->Values[ML_METHODS_INDEX];
	Context->Values[ML_METHODS_INDEX] = Methods;
}

struct ml_method_cached_t {
	ml_method_cached_t *Next, *MethodNext;
	ml_method_t *Method;
	ml_method_definition_t *Definition;
	int Count, Score;
	const ml_type_t *Types[];
};

struct ml_method_definition_t {
	ml_method_definition_t *Next;
	ml_value_t *Callback;
	int Count, Variadic;
	const ml_type_t *Types[];
};

static __attribute__ ((pure)) unsigned int ml_method_definition_score(ml_method_definition_t *Definition, int Count, const ml_type_t **Types) {
	unsigned int Score = 1;
	if (Definition->Count > Count) return 0;
	if (Definition->Count < Count) {
		if (!Definition->Variadic) return 0;
		Count = Definition->Count;
	} else if (!Definition->Variadic) {
		Score = 2;
	}
	for (int I = 0; I < Count; ++I) {
		const ml_type_t *Type = Definition->Types[I];
		if (ml_is_subtype(Types[I], Type)) goto found;
		return 0;
	found:
		Score += 5 + Type->Rank;
	}
	return Score;
}

static ml_method_cached_t *ml_method_search_entry(ml_methods_t *Methods, ml_method_t *Method, int Count, const ml_type_t **Types, uint64_t Hash) {
	ml_method_cached_t *Cached = inthash_search(Methods->Cache, Hash);
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
	if (!BestDefinition) return NULL;
	if (!Cached) {
		Cached = xnew(ml_method_cached_t, Count, ml_type_t *);
		Cached->Method = Method;
		Cached->Next = inthash_insert(Methods->Cache, Hash, Cached);
		Cached->MethodNext = inthash_insert(Methods->Methods, (uintptr_t)Method, Cached);
		Cached->Count = Count;
		for (int I = 0; I < Count; ++I) Cached->Types[I] = Types[I];
	}
	Cached->Definition = BestDefinition;
	Cached->Score = BestScore;
	return Cached;
}

static inline uintptr_t rotl(uintptr_t X, unsigned int N) {
	const unsigned int Mask = (CHAR_BIT * sizeof(uintptr_t) - 1);
	return (X << (N & Mask)) | (X >> ((-N) & Mask ));
}

static ml_value_t *ml_method_search(ml_methods_t *Methods, ml_method_t *Method, int Count, ml_value_t **Args) {
	// TODO: Use generation numbers to check Methods->Parent for invalidated definitions
	// Use alloca here, VLA prevents TCO.
	const ml_type_t **Types = alloca(Count * sizeof(const ml_type_t *));
	uintptr_t Hash = (uintptr_t)Method;
	for (int I = Count; --I >= 0;) {
#ifdef USE_NANBOXING
		ml_value_t *Value = Args[I];
		const ml_type_t *Type;
		unsigned Tag = ml_tag(Value);
		if (__builtin_expect(Tag == 0, 1)) {
			Type = ml_typeof(Value->Type->deref(Value));
		} else if (Tag == 1) {
			Type = MLInt32T;
		} else if (Tag < 7) {
			Type = NULL;
		} else {
			Type = MLDoubleT;
		}
		Types[I] = Type;
#else
		const ml_type_t *Type = Types[I] = ml_typeof(ml_deref(Args[I]));
#endif
		Hash = rotl(Hash, 1) ^ (uintptr_t)Type;
	}
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
	Definition->Next = inthash_insert(Methods->Definitions, (uintptr_t)Method, Definition);
	for (ml_method_cached_t *Cached = inthash_search(Methods->Methods, (uintptr_t)Method); Cached; Cached = Cached->MethodNext) {
		Cached->Definition = NULL;
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

static void ml_method_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_method_t *Method = (ml_method_t *)Value;
	ml_methods_t *Methods = Caller->Context->Values[ML_METHODS_INDEX];
	ml_value_t *Callback = ml_method_search(Methods, Method, Count, Args);

	if (Callback) {
		return ml_call(Caller, Callback, Count, Args);
	} else {
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
		for (int I = 0; I < Count; ++I) P = stpcpy(stpcpy(P, ml_typeof(Args[I])->Name), ", ");
#endif
		P[-2] = 0;
		ML_RETURN(ml_error("MethodError", "no method found for %s(%s)", Method->Name, Types));
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
#ifdef USE_ML_THREADSAFE
	static pthread_mutex_t Lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&Lock);
#endif
	ml_method_t **Slot = (ml_method_t **)stringmap_slot(Methods, Name);
	if (!Slot[0]) {
		ml_method_t *Method = new(ml_method_t);
		Method->Type = MLMethodT;
		Method->Name = Name;
		Slot[0] = Method;
	}
#ifdef USE_ML_THREADSAFE
	pthread_mutex_unlock(&Lock);
#endif
	return (ml_value_t *)Slot[0];
}

ml_value_t *ml_method_anon(const char *Name) {
	ml_method_t *Method = new(ml_method_t);
	Method->Type = MLMethodT;
	Method->Name = Name;
	return (ml_value_t *)Method;
}

ML_METHOD(MLMethodOfMethod) {
//!method
//>method
	return ml_method(NULL);
}

ML_METHOD(MLMethodOfMethod, MLStringT) {
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

ML_METHOD(MLStringOfMethod, MLMethodT) {
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
	const ml_type_t **Types = (const ml_type_t **)Args + 1;
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

ML_METHOD_DECL(Range, "..");

ML_FUNCTIONX(MLMethodSet) {
//!method
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
	ml_type_t *Types[NumTypes];
	for (int I = 1; I <= NumTypes; ++I) {
		if (Args[I] == MLNil) {
			Types[I - 1] = MLNilT;
		} else {
			ML_CHECKX_ARG_TYPE(I, MLTypeT);
			Types[I - 1] = (ml_type_t *)Args[I];
		}
	}
	ML_CHECKX_ARG_TYPE(Count - 1, MLFunctionT);
	ml_method_t *Method = (ml_method_t *)Args[0];
	ml_value_t *Function = Args[Count - 1];
	ml_methods_t *Methods = Caller->Context->Values[ML_METHODS_INDEX];
	ml_method_insert(Methods, Method, Function, NumTypes, Variadic, Types);
	ML_RETURN(Function);
}

void ml_method_init() {
	ml_context_set(&MLRootContext, ML_METHODS_INDEX, MLRootMethods);
#include "ml_method_init.c"
	MLMethodT->Constructor = MLMethodOfMethod;
	stringmap_insert(MLMethodT->Exports, "of", MLMethodOfMethod);
	stringmap_insert(MLMethodT->Exports, "set", MLMethodSet);
	ml_method_by_value(MLMethodOfMethod, NULL, ml_identity, MLMethodT, NULL);
}

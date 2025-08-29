#include "minilang.h"
#include "ml_macros.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

#undef ML_CATEGORY
#define ML_CATEGORY "function"

extern ml_value_t *IndexMethod;
extern ml_value_t *CompareMethod;

ML_INTERFACE(MLFunctionT, (), "function");
// The base type of all functions.

int ml_function_source(ml_value_t *Value, const char **Source, int *Line) {
	typeof(ml_function_source) *function = ml_typed_fn_get(ml_typeof(Value), ml_function_source);
	if (function) return function(Value, Source, Line);
	return 0;
}

ML_METHOD("source", MLFunctionT) {
	const char *Source;
	int Line;
	if (ml_function_source(Args[0], &Source, &Line)) {
		return ml_tuplev(2, ml_string(Source, -1), ml_integer(Line));
	} else {
		return MLNil;
	}
}

ML_METHODX("!", MLFunctionT, MLTupleT) {
//<Function
//<Tuple
//>any
// Calls :mini:`Function` with the values in :mini:`Tuple` as positional arguments.
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[1];
	ml_value_t *Function = Args[0];
	return ml_call(Caller, Function, Tuple->Size, Tuple->Values);
}

ML_METHODX("!", MLFunctionT, MLListT) {
//<Function
//<List
//>any
// Calls :mini:`Function` with the values in :mini:`List` as positional arguments.
	ml_value_t *Function = Args[0];
	ml_value_t *List = Args[1];
	int Count2 = ml_list_length(List);
	ml_value_t **Args2 = ml_alloc_args(Count2);
	ml_list_to_array(List, Args2);
	return ml_call(Caller, Function, Count2, Args2);
}

ML_METHODX("!", MLFunctionT, MLMapT) {
//<Function
//<Map
//>any
// Calls :mini:`Function` with the keys and values in :mini:`Map` as named arguments.
// Returns an error if any of the keys in :mini:`Map` is not a string or method.
	ml_value_t *Function = Args[0];
	ml_value_t *Map = Args[1];
	int Count2 = ml_map_size(Map) + 1;
	ml_value_t **Args2 = ml_alloc_args(Count2);
	ml_value_t *Names = ml_names();
	ml_value_t **Arg = Args2;
	*(Arg++) = Names;
	ML_MAP_FOREACH(Map, Node) {
		ml_value_t *Name = Node->Key;
		if (ml_is(Name, MLStringT)) {
			ml_names_add(Names, Name);
		} else {
			ML_ERROR("TypeError", "Parameter names must be strings or methods");
		}
		*(Arg++) = Node->Value;
	}
	return ml_call(Caller, Function, Count2, Args2);
}

ML_METHODX("!", MLFunctionT, MLTupleT, MLMapT) {
//<Function
//<Tuple
//<Map
//>any
// Calls :mini:`Function` with the values in :mini:`Tuple` as positional arguments and the keys and values in :mini:`Map` as named arguments.
// Returns an error if any of the keys in :mini:`Map` is not a string or method.
	ml_value_t *Function = Args[0];
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[1];
	ml_value_t *Map = Args[2];
	int TupleCount = Tuple->Size;
	int MapCount = ml_map_size(Map);
	int Count2 = TupleCount + MapCount + 1;
	ml_value_t **Args2 = ml_alloc_args(Count2);
	memcpy(Args2, Tuple->Values, TupleCount * sizeof(ml_value_t *));
	ml_value_t *Names = ml_names();
	ml_value_t **Arg = Args2 + TupleCount;
	*(Arg++) = Names;
	ML_MAP_FOREACH(Map, Node) {
		ml_value_t *Name = Node->Key;
		if (ml_is(Name, MLStringT)) {
			ml_names_add(Names, Name);
		} else {
			ML_ERROR("TypeError", "Parameter names must be strings or methods");
		}
		*(Arg++) = Node->Value;
	}
	return ml_call(Caller, Function, Count2, Args2);
}

ML_METHODX("!", MLFunctionT, MLListT, MLMapT) {
//<Function
//<List
//<Map
//>any
// Calls :mini:`Function` with the values in :mini:`List` as positional arguments and the keys and values in :mini:`Map` as named arguments.
// Returns an error if any of the keys in :mini:`Map` is not a string or method.
	ml_value_t *Function = Args[0];
	int ListCount = ml_list_length(Args[1]);
	ml_value_t *Map = Args[2];
	int MapCount = ml_map_size(Map);
	int Count2 = ListCount + MapCount + 1;
	ml_value_t **Args2 = ml_alloc_args(Count2);
	ml_list_to_array(Args[1], Args2);
	ml_value_t *Names = ml_names();
	ml_value_t **Arg = Args2 + ListCount;
	*(Arg++) = Names;
	ML_MAP_FOREACH(Map, Node) {
		ml_value_t *Name = Node->Key;
		if (ml_is(Name, MLStringT)) {
			ml_names_add(Names, Name);
		} else {
			ML_ERROR("TypeError", "Parameter names must be strings or methods");
		}
		*(Arg++) = Node->Value;
	}
	return ml_call(Caller, Function, Count2, Args2);
}

static __attribute__ ((noinline)) void ml_cfunction_call_deref(ml_state_t *Caller, ml_cfunction_t *Function, int Count, ml_value_t **Args, int I) {
	Args[I] = Args[I]->Type->deref(Args[I]);
	while (--I >= 0) Args[I] = ml_deref(Args[I]);
	ML_RETURN((Function->Callback)(Function->Data, Count, Args));
}

static void ml_cfunction_call(ml_state_t *Caller, ml_cfunction_t *Function, int Count, ml_value_t **Args) {
	for (int I = Count; --I >= 0;) {
#ifdef ML_NANBOXING
		if (!ml_tag(Args[I]) && Args[I]->Type->deref != ml_default_deref) {
#else
		if (Args[I]->Type->deref != ml_default_deref) {
#endif
			return ml_cfunction_call_deref(Caller, Function, Count, Args, I);
		}
	}
	ML_RETURN((Function->Callback)(Function->Data, Count, Args));
}

static long ml_cfunction_hash(ml_cfunction_t *Function, ml_hash_chain_t *Chain) {
	if (Function->Source) {
		long Hash = 23879;
		for (const char *P = Function->Source; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
		return Hash + Function->Line;
	} else {
		return (long)(uintptr_t)Function->Callback;
	}
}

ML_TYPE(MLCFunctionT, (MLFunctionT), "c-function",
//!internal
	.hash = (void *)ml_cfunction_hash,
	.call = (void *)ml_cfunction_call
);

ml_value_t *ml_cfunction(void *Data, ml_callback_t Callback) {
	ml_cfunction_t *Function = new(ml_cfunction_t);
	Function->Type = MLCFunctionT;
	Function->Data = Data;
	Function->Callback = Callback;
	return (ml_value_t *)Function;
}

ml_value_t *ml_cfunction2(void *Data, ml_callback_t Callback, const char *Source, int Line) {
	ml_cfunction_t *Function = new(ml_cfunction_t);
	Function->Type = MLCFunctionT;
	Function->Data = Data;
	Function->Callback = Callback;
	Function->Source = Source;
	Function->Line = Line;
	return (ml_value_t *)Function;
}

static int ML_TYPED_FN(ml_function_source, MLCFunctionT, ml_cfunction_t *Function, const char **Source, int *Line) {
	if (Function->Source) {
		*Source = Function->Source;
		*Line = Function->Line;
		return 1;
	} else {
		return 0;
	}
}

static void ML_TYPED_FN(ml_iterate, MLCFunctionT, ml_state_t *Caller, ml_cfunction_t *Function) {
	ML_RETURN((Function->Callback)(Function->Data, 0, NULL));
}

static __attribute__ ((noinline)) void ml_cfunctionx_call_deref(ml_state_t *Caller, ml_cfunctionx_t *Function, int Count, ml_value_t **Args, int I) {
	Args[I] = Args[I]->Type->deref(Args[I]);
	while (--I >= 0) Args[I] = ml_deref(Args[I]);
	return (Function->Callback)(Caller, Function->Data, Count, Args);
}

static void ml_cfunctionx_call(ml_state_t *Caller, ml_cfunctionx_t *Function, int Count, ml_value_t **Args) {
	for (int I = Count; --I >= 0;) {
#ifdef ML_NANBOXING
		if (!ml_tag(Args[I]) && Args[I]->Type->deref != ml_default_deref) {
#else
		if (Args[I]->Type->deref != ml_default_deref) {
#endif
			return ml_cfunctionx_call_deref(Caller, Function, Count, Args, I);
		}
	}
	//for (int I = 0; I < Count; ++I) Args[I] = ml_deref(Args[I]);
	return (Function->Callback)(Caller, Function->Data, Count, Args);
}

static long ml_cfunctionx_hash(ml_cfunctionx_t *Function, ml_hash_chain_t *Chain) {
	if (Function->Source) {
		long Hash = 23879;
		for (const char *P = Function->Source; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
		return Hash + Function->Line;
	} else {
		return (long)(uintptr_t)Function->Callback;
	}
}

ML_TYPE(MLCFunctionXT, (MLFunctionT), "c-functionx",
//!internal
	.hash = (void *)ml_cfunctionx_hash,
	.call = (void *)ml_cfunctionx_call
);

ml_value_t *ml_cfunctionx(void *Data, ml_callbackx_t Callback) {
	ml_cfunctionx_t *Function = new(ml_cfunctionx_t);
	Function->Type = MLCFunctionXT;
	Function->Data = Data;
	Function->Callback = Callback;
	return (ml_value_t *)Function;
}

ml_value_t *ml_cfunctionx2(void *Data, ml_callbackx_t Callback, const char *Source, int Line) {
	ml_cfunctionx_t *Function = new(ml_cfunctionx_t);
	Function->Type = MLCFunctionXT;
	Function->Data = Data;
	Function->Callback = Callback;
	Function->Source = Source;
	Function->Line = Line;
	return (ml_value_t *)Function;
}

static int ML_TYPED_FN(ml_function_source, MLCFunctionXT, ml_cfunctionx_t *Function, const char **Source, int *Line) {
	if (Function->Source) {
		*Source = Function->Source;
		*Line = Function->Line;
		return 1;
	} else {
		return 0;
	}
}

static void ml_cfunctionz_call(ml_state_t *Caller, ml_cfunctionx_t *Function, int Count, ml_value_t **Args) {
	return (Function->Callback)(Caller, Function->Data, Count, Args);
}

ML_TYPE(MLCFunctionZT, (MLFunctionT), "c-functionx",
//!internal
	.hash = (void *)ml_cfunctionx_hash,
	.call = (void *)ml_cfunctionz_call
);

ml_value_t *ml_cfunctionz(void *Data, ml_callbackx_t Callback) {
	ml_cfunctionx_t *Function = new(ml_cfunctionx_t);
	Function->Type = MLCFunctionZT;
	Function->Data = Data;
	Function->Callback = Callback;
	return (ml_value_t *)Function;
}

ml_value_t *ml_cfunctionz2(void *Data, ml_callbackx_t Callback, const char *Source, int Line) {
	ml_cfunctionx_t *Function = new(ml_cfunctionx_t);
	Function->Type = MLCFunctionZT;
	Function->Data = Data;
	Function->Callback = Callback;
	Function->Source = Source;
	Function->Line = Line;
	return (ml_value_t *)Function;
}

typedef struct {
	const ml_type_t *Type;
	ml_value_t *Function;
	int Count, Set, Names;
	ml_value_t *Args[];
} ml_partial_function_t;

static long ml_partial_function_hash(ml_partial_function_t *Partial, ml_hash_chain_t *Chain) {
	long Hash = ml_hash_chain(Partial->Function, Chain);
	for (int I = 0; I < Partial->Count; ++I) {
		ml_value_t *Arg = Partial->Args[I];
		if (Arg) Hash ^= ml_hash(Arg) << (I + 1);
	}
	return Hash;
}

static void __attribute__ ((noinline)) ml_partial_function_copy_args(ml_partial_function_t *Partial, int CombinedCount, ml_value_t **CombinedArgs, int Count, ml_value_t **Args) {
	ml_value_t *Copy[Count];
	memcpy(Copy, Args, Count * sizeof(ml_value_t *));
	int I = 0, J = 0;
	for (; I < Partial->Count; ++I) {
		CombinedArgs[I] = Partial->Args[I] ?: (J < Count) ? Copy[J++] : MLNil;
	}
	for (; I < CombinedCount; ++I) {
		CombinedArgs[I] = (J < Count) ? Copy[J++] : MLNil;
	}
}

static void ml_partial_function_call(ml_state_t *Caller, ml_partial_function_t *Partial, int Count, ml_value_t **Args) {
	int CombinedCount = Count + Partial->Set;
	if (CombinedCount < Partial->Count) CombinedCount = Partial->Count;
	ml_value_t **CombinedArgs = ml_alloc_args(CombinedCount);
	if (CombinedArgs == Args) {
		ml_partial_function_copy_args(Partial, CombinedCount, CombinedArgs, Count, Args);
	} else {
		int I = 0, J = 0;
		for (; I < Partial->Count; ++I) {
			CombinedArgs[I] = Partial->Args[I] ?: (J < Count) ? Args[J++] : MLNil;
		}
		for (; I < CombinedCount; ++I) {
			CombinedArgs[I] = (J < Count) ? Args[J++] : MLNil;
		}
	}
	return ml_call(Caller, Partial->Function, CombinedCount, CombinedArgs);
}

ML_FUNCTION(MLFunctionPartial) {
//@function::partial
//<Function
//<Size
//>function::partial
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLFunctionT);
	ML_CHECK_ARG_TYPE(1, MLIntegerT);
	return ml_partial_function(Args[0], ml_integer_value(Args[1]));
}

ML_TYPE(MLFunctionPartialT, (MLFunctionT, MLSequenceT), "function::partial",
	.hash = (void *)ml_partial_function_hash,
	.call = (void *)ml_partial_function_call,
	.Constructor = (ml_value_t *)MLFunctionPartial
);

static void __attribute__ ((noinline)) ml_partial_function_named_copy_args(ml_partial_function_t *Partial, int CombinedCount, ml_value_t **CombinedArgs, int Count, ml_value_t **Args) {
	ml_value_t *Copy[Count];
	memcpy(Copy, Args, Count * sizeof(ml_value_t *));
	int I = 0, J = 0, Named = Count;
	for (int K = 0; K < Count; ++K) if (ml_typeof(Copy[K]) == MLNamesT) {
		Named = K;
		break;
	}
	for (; I < Partial->Names; ++I) {
		CombinedArgs[I] = Partial->Args[I] ?: (J < Named) ? Copy[J++] : MLNil;
	}
	int UnnamedCount = Named;
	if (J < Named) {
		int NamedBlanks = 0;
		for (int K = Partial->Names + 1; K < Partial->Count; ++K) if (!Partial->Args[K]) ++NamedBlanks;
		if (UnnamedCount > J + NamedBlanks) {
			for (int K = J + NamedBlanks; K < UnnamedCount; ++K) CombinedArgs[I++] = Copy[K];
			UnnamedCount = J + NamedBlanks;
		}
	}
	if (Named < Count) {
		int K = Partial->Names;
		ml_value_t *Names = ml_names();
		ML_NAMES_FOREACH(Partial->Args[K], Iter) ml_names_add(Names, Iter->Value);
		ML_NAMES_FOREACH(Copy[Named], Iter) ml_names_add(Names, Iter->Value);
		CombinedArgs[I++] = Names;
		for (++K; K < Partial->Count; ++I, ++K) {
			CombinedArgs[I] = Partial->Args[K] ?: (J < UnnamedCount) ? Copy[J++] : MLNil;
		}
		for (J = Named + 1; J < Count; ++J, ++I) CombinedArgs[I] = Copy[J];
	} else {
		int K = Partial->Names;
		CombinedArgs[I++] = Partial->Args[K++];
		for (; K < Partial->Count; ++I, ++K) {
			CombinedArgs[I] = Partial->Args[K] ?: (J < UnnamedCount) ? Copy[J++] : MLNil;
		}
	}
}

static void ml_partial_function_named_call(ml_state_t *Caller, ml_partial_function_t *Partial, int Count, ml_value_t **Args) {
	int CombinedCount = Count + Partial->Set;
	if (CombinedCount < Partial->Count) CombinedCount = Partial->Count;
	ml_value_t **CombinedArgs = ml_alloc_args(CombinedCount);
	//if (CombinedArgs == Args) {
		ml_partial_function_named_copy_args(Partial, CombinedCount, CombinedArgs, Count, Args);
	/*} else {
		int I = 0, J = 0;
		for (; I < Partial->Count; ++I) {
			CombinedArgs[I] = Partial->Args[I] ?: (J < Count) ? Args[J++] : MLNil;
		}
		for (; I < CombinedCount; ++I) {
			CombinedArgs[I] = (J < Count) ? Args[J++] : MLNil;
		}
	}*/
	return ml_call(Caller, Partial->Function, CombinedCount, CombinedArgs);
}

ML_TYPE(MLFunctionPartialNamedT, (MLFunctionPartialT), "function::partial::named",
//!internal
	.hash = (void *)ml_partial_function_hash,
	.call = (void *)ml_partial_function_named_call
);

ml_value_t *ml_partial_function(ml_value_t *Function, int Count) {
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, Count, ml_value_t *);
	Partial->Type = MLFunctionPartialT;
	Partial->Function = Function;
	Partial->Count = 0;
	Partial->Set = 0;
	return (ml_value_t *)Partial;
}

ml_value_t *ml_partial_function_set(ml_value_t *Partial0, size_t Index, ml_value_t *Value) {
	ml_partial_function_t *Partial = (ml_partial_function_t *)Partial0;
	++Partial->Set;
	if (Partial->Count < Index + 1) Partial->Count = Index + 1;
	if (ml_typeof(Value) == MLNamesT) {
		Partial->Type = MLFunctionPartialNamedT;
		Partial->Names = Index;
	}
	return Partial->Args[Index] = Value;
}

static void ML_TYPED_FN(ml_value_sha256, MLFunctionPartialT, ml_partial_function_t *Partial, ml_hash_chain_t *Chain, unsigned char Hash[SHA256_BLOCK_SIZE]) {
	ml_value_sha256(Partial->Function, Chain, Hash);
	for (int I = 0; I < Partial->Count; ++I) {
		ml_value_t *Arg = Partial->Args[I];
		if (Arg) *(long *)(Hash + (I % 16)) ^= ml_hash_chain(Arg, Chain);
	}
}

static ml_value_t *ML_TYPED_FN(ml_serialize, MLFunctionPartialT, ml_partial_function_t *Partial) {
	ml_value_t *Result = ml_list();
	ml_list_put(Result, ml_cstring("$!"));
	ml_list_put(Result, Partial->Function);
	for (int I = 0; I < Partial->Count; ++I) ml_list_put(Result, Partial->Args[I] ?: MLBlank);
	return Result;
}

ML_DESERIALIZER("$!") {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Partial = ml_partial_function(Args[0], Count - 1);
	for (int I = 1; I < Count; ++I) {
		if (Args[I] != MLBlank) ml_partial_function_set(Partial, I - 1, Args[I]);
	}
	return Partial;
}

ML_METHOD("arity", MLFunctionPartialT) {
	ml_partial_function_t *Partial = (ml_partial_function_t *)Args[0];
	return ml_integer(Partial->Count);
}

ML_METHOD("set", MLFunctionPartialT) {
	ml_partial_function_t *Partial = (ml_partial_function_t *)Args[0];
	return ml_integer(Partial->Set);
}

ML_METHODV("[]", MLFunctionPartialT) {
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, Count, ml_value_t *);
	Partial->Type = MLFunctionPartialT;
	Partial->Function = IndexMethod;
	Partial->Count = Count;
	Partial->Set = Count - 1;
	for (int I = 1; I < Count; ++I) Partial->Args[I] = Args[I];
	return ml_chainedv(2, Args[0], Partial);
}

static ml_value_t *ml_partial_function_compare(ml_partial_function_t *A, ml_partial_function_t *B) {
	// TODO: Replace this with a state to remove ml_simple_call
	ml_value_t *Args[2];
	ml_value_t *Result;
	int N;
	if (A->Count > B->Count) {
		N = B->Count;
		Result = (ml_value_t *)One;
	} else if (A->Count < B->Count) {
		N = A->Count;
		Result = (ml_value_t *)NegOne;
	} else {
		N = A->Count;
		Result = (ml_value_t *)Zero;
	}
	Args[0] = A->Function;
	Args[1] = B->Function;
	ml_value_t *C = ml_simple_call(CompareMethod, 2, Args);
	if (ml_is_error(C)) return C;
	if (ml_integer_value(C)) return C;
	for (int I = 0; I < N; ++I) {
		Args[0] = A->Args[I];
		Args[1] = B->Args[I];
		if (!Args[0]) {
			if (Args[1]) return (ml_value_t *)NegOne;
		} else if (!Args[1]) {
			return (ml_value_t *)One;
		} else {
			ml_value_t *C = ml_simple_call(CompareMethod, 2, Args);
			if (ml_is_error(C)) return C;
			if (ml_integer_value(C)) return C;
		}
	}
	return Result;
}

ML_METHOD("<>", MLFunctionPartialT, MLFunctionPartialT) {
	ml_partial_function_t *A = (ml_partial_function_t *)Args[0];
	ml_partial_function_t *B = (ml_partial_function_t *)Args[1];
	return ml_partial_function_compare(A, B);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value;
} ml_value_function_t;

static void ml_value_function_call(ml_state_t *Caller, ml_value_function_t *Function, int Count, ml_value_t **Args) {
	ML_RETURN(Function->Value);
}

ML_TYPE(MLFunctionValueT, (MLFunctionT), "function::value",
	.call = (void *)ml_value_function_call
);

ml_value_t *ml_value_function(ml_value_t *Value) {
	ml_value_function_t *Function = new(ml_value_function_t);
	Function->Type = MLFunctionValueT;
	Function->Value = Value;
	return (ml_value_t *)Function;
}

ML_FUNCTIONX(MLFunctionConstant) {
//@function::constant
//<Value
//>function::value
	ML_CHECKX_ARG_COUNT(1);
	ML_RETURN(ml_value_function(Args[0]));
}

ML_FUNCTIONZ(MLFunctionVariable) {
//@function::variable
//<Value
//>function::value
	ML_CHECKX_ARG_COUNT(1);
	ML_RETURN(ml_value_function(Args[0]));
}

static ml_value_t *ML_TYPED_FN(ml_serialize, MLFunctionValueT, ml_value_function_t *Function) {
	ml_value_t *Result = ml_list();
	ml_list_put(Result, ml_cstring("$="));
	ml_list_put(Result, Function->Value);
	return Result;
}

ML_DESERIALIZER("$=") {
	ML_CHECK_ARG_COUNT(1);
	return ml_value_function(Args[0]);
}

ML_METHOD("<>", MLFunctionValueT, MLFunctionValueT) {
	ml_value_function_t *A = (ml_value_function_t *)Args[0];
	ml_value_function_t *B = (ml_value_function_t *)Args[1];
	Args[0] = A->Value;
	Args[1] = B->Value;
	return ml_simple_call(CompareMethod, 2, Args);
}

ML_METHOD("$!", MLFunctionT, MLListT) {
//<Function
//<List
//>function::partial
// Returns a function equivalent to :mini:`fun(Args...) Function(List[1], List[2], ..., Args...)`.
	ml_list_t *ArgsList = (ml_list_t *)Args[1];
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, ArgsList->Length, ml_value_t *);
	Partial->Type = MLFunctionPartialT;
	Partial->Function = Args[0];
	Partial->Count = Partial->Set = ArgsList->Length;
	ml_value_t **Arg = Partial->Args;
	ML_LIST_FOREACH(ArgsList, Node) *Arg++ = Node->Value;
	return (ml_value_t *)Partial;
}

ML_METHOD("$!", MLFunctionT, MLMapT) {
//<Function
//<List
//>function::partial
// Returns a function equivalent to :mini:`fun(Args...) Function(List[1], List[2], ..., Args...)`.
	int Size = ml_map_size(Args[1]);
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, Size + 1, ml_value_t *);
	Partial->Type = MLFunctionPartialNamedT;
	Partial->Function = Args[0];
	Partial->Count = Partial->Set = Size + 1;
	Partial->Names = 0;
	ml_value_t **Arg = Partial->Args;
	ml_value_t *Names = ml_names();
	*Arg++ = Names;
	ML_MAP_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Key, MLStringT)) return ml_error("TypeError", "Names must be strings");
		ml_names_add(Names, Iter->Key);
		*Arg++ = Iter->Value;
	}
	return (ml_value_t *)Partial;
}

ML_METHOD("$!", MLFunctionT, MLListT, MLMapT) {
//<Function
//<List
//>function::partial
// Returns a function equivalent to :mini:`fun(Args...) Function(List[1], List[2], ..., Args...)`.
	int Size = ml_list_length(Args[1]) + ml_map_size(Args[2]);
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, Size + 1, ml_value_t *);
	Partial->Type = MLFunctionPartialNamedT;
	Partial->Function = Args[0];
	Partial->Count = Partial->Set = Size + 1;
	ml_value_t **Arg = Partial->Args;
	ML_LIST_FOREACH(Args[1], Iter) *Arg++ = Iter->Value;
	Partial->Names = Arg - Partial->Args;
	ml_value_t *Names = ml_names();
	*Arg++ = Names;
	ML_MAP_FOREACH(Args[2], Iter) {
		if (!ml_is(Iter->Key, MLStringT)) return ml_error("TypeError", "Names must be strings");
		ml_names_add(Names, Iter->Key);
		*Arg++ = Iter->Value;
	}
	return (ml_value_t *)Partial;
}

ML_METHOD("!!", MLFunctionT, MLListT) {
//<Function
//<List
//>function::partial
//
// .. deprecated:: 2.7.0
//
//    Use :mini:`$!` instead.
//
// Returns a function equivalent to :mini:`fun(Args...) Function(List[1], List[2], ..., Args...)`.
	ml_list_t *ArgsList = (ml_list_t *)Args[1];
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, ArgsList->Length, ml_value_t *);
	Partial->Type = MLFunctionPartialT;
	Partial->Function = Args[0];
	Partial->Count = Partial->Set = ArgsList->Length;
	ml_value_t **Arg = Partial->Args;
	ML_LIST_FOREACH(ArgsList, Node) *Arg++ = Node->Value;
	return (ml_value_t *)Partial;
}

ML_METHODV("$", MLFunctionT, MLAnyT) {
//<Function
//<Values...
//>function::partial
// Returns a function equivalent to :mini:`fun(Args...) Function(Values..., Args...)`.
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, Count - 1, ml_value_t *);
	Partial->Type = MLFunctionPartialT;
	Partial->Function = Args[0];
	Partial->Count = Partial->Set = Count - 1;
	for (int I = 1; I < Count; ++I) Partial->Args[I - 1] = Args[I];
	return (ml_value_t *)Partial;
}

static void ML_TYPED_FN(ml_iterate, MLFunctionPartialT, ml_state_t *Caller, ml_partial_function_t *Partial) {
	if (Partial->Set != Partial->Count) ML_ERROR("CallError", "Partial function used with missing arguments");
	return ml_call(Caller, Partial->Function, Partial->Count, Partial->Args);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Function;
} ml_argless_function_t;

static void ml_argless_function_call(ml_state_t *Caller, ml_argless_function_t *Argless, int Count, ml_value_t **Args) {
	return ml_call(Caller, Argless->Function, 0, NULL);
}

ML_TYPE(MLFunctionArglessT, (MLFunctionT, MLSequenceT), "function::argless",
//!internal
	.call = (void *)ml_argless_function_call
);

ML_METHOD("/", MLFunctionT) {
//<Function
//>function
// Returns a function equivalent to :mini:`fun(Args...) Function()`.
	ml_argless_function_t *Argless = new(ml_argless_function_t);
	Argless->Type = MLFunctionArglessT;
	Argless->Function = Args[0];
	return (ml_value_t *)Argless;
}

static void ML_TYPED_FN(ml_iterate, MLFunctionArglessT, ml_state_t *Caller, ml_argless_function_t *Argless) {
	return ml_call(Caller, Argless->Function, 0, NULL);
}

void ml_function_init() {
#include "ml_function_init.c"
	stringmap_insert(MLFunctionT->Exports, "partial", MLFunctionPartialT);
	stringmap_insert(MLFunctionT->Exports, "variable", MLFunctionVariable);
	stringmap_insert(MLFunctionT->Exports, "constant", MLFunctionConstant);
}

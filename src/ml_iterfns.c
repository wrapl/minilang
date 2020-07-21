#include <gc.h>
#include "ml_runtime.h"
#include <string.h>
#include "minilang.h"
#include "ml_macros.h"
#include "ml_iterfns.h"

typedef struct ml_chained_state_t {
	ml_state_t Base;
	ml_value_t *Value;
	ml_value_t **Current;
} ml_chained_state_t;

static void ml_chained_state_run(ml_chained_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t *Function = *State->Current++;
	if (!Function) ML_CONTINUE(State->Base.Caller, Value);
	State->Value = Value;
	return ml_typeof(Function)->call((ml_state_t *)State, Function, 1, &State->Value);
}

typedef struct ml_chained_function_t {
	const ml_type_t *Type;
	ml_value_t *Functions[];
} ml_chained_function_t;

static void ml_chained_function_call(ml_state_t *Caller, ml_chained_function_t *Chained, int Count, ml_value_t **Args) {
	ml_value_t **Functions = Chained->Functions;
	ml_value_t *Function = *Functions++;
	ml_chained_state_t *State = new(ml_chained_state_t);
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_chained_state_run;
	State->Base.Context = Caller->Context;
	State->Current = Functions;
	return ml_typeof(Function)->call((ml_state_t *)State, Function, Count, Args);
}

typedef struct ml_chained_iterator_t {
	ml_state_t Base;
	ml_value_t *Iterator;
	ml_value_t *Value;
	ml_value_t **Functions, **Current;
} ml_chained_iterator_t;

ML_TYPE(MLChainedIteratorT, (), "chained-iterator");

ML_TYPE(MLChainedFunctionT, (MLFunctionT), "chained-function",
	.call = (void *)ml_chained_function_call
);

static ml_value_t *ml_chained(int Count, ml_value_t **Functions) {
	if (Count == 1) return Functions[0];
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, Count + 1, ml_value_t *);
	Chained->Type = MLChainedFunctionT;
	for (int I = 0; I < Count; ++I) Chained->Functions[I] = Functions[I];
	return (ml_value_t *)Chained;
}

static void ML_TYPED_FN(ml_iter_key, MLChainedIteratorT, ml_state_t *Caller, ml_chained_iterator_t *Chained) {
	return ml_iter_key(Caller, Chained->Iterator);
}

static void ML_TYPED_FN(ml_iter_value, MLChainedIteratorT, ml_state_t *Caller, ml_chained_iterator_t *Chained) {
	ML_RETURN(Chained->Value);
}

static void ml_chained_iterator_value(ml_chained_iterator_t *Chained, ml_value_t *Iter);

static void ml_chained_iterator_apply(ml_chained_iterator_t *Chained, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(Chained->Base.Caller, Value);
	// TODO: Make the nil filtering optional
	if (Value == MLNil) {
		Chained->Base.run = (void *)ml_chained_iterator_value;
		Chained->Current = Chained->Functions;
		return ml_iter_next((ml_state_t *)Chained, Chained->Iterator);
	}
	ml_value_t *Current = Chained->Current[0];
	if (Current) {
		Chained->Value = ml_deref(Value);
		++Chained->Current;
		return ml_typeof(Current)->call((ml_state_t *)Chained, Current, 1, &Chained->Value);
	} else {
		Chained->Value = Value;
		ML_CONTINUE(Chained->Base.Caller, Chained);
	}
}

static void ml_chained_iterator_value(ml_chained_iterator_t *Chained, ml_value_t *Iter) {
	if (ml_is_error(Iter)) ML_CONTINUE(Chained->Base.Caller, Iter);
	if (Iter == MLNil) ML_CONTINUE(Chained->Base.Caller, Iter);
	Chained->Base.run = (void *)ml_chained_iterator_apply;
	return ml_iter_value((ml_state_t *)Chained, Chained->Iterator = Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLChainedIteratorT, ml_state_t *Caller, ml_chained_iterator_t *Chained) {
	Chained->Base.Caller = Caller;
	Chained->Base.Context = Caller->Context;
	Chained->Base.run = (void *)ml_chained_iterator_value;
	Chained->Current = Chained->Functions;
	return ml_iter_next((ml_state_t *)Chained, Chained->Iterator);
}

static void ML_TYPED_FN(ml_iterate, MLChainedFunctionT, ml_state_t *Caller, ml_chained_function_t *Chained) {
	ml_value_t **Functions = Chained->Functions;
	ml_value_t *Function = *Functions++;
	ml_chained_iterator_t *Iterator = new(ml_chained_iterator_t);
	Iterator->Base.Type =  MLChainedIteratorT;
	Iterator->Base.Caller = Caller;
	Iterator->Base.Context = Caller->Context;
	Iterator->Base.run = (void *)ml_chained_iterator_value;
	Iterator->Functions = Functions;
	Iterator->Current = Functions;
	return ml_iterate((ml_state_t *)Iterator, Function);
}

ML_METHOD(">>", MLIteratableT, MLFunctionT) {
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 3, ml_value_t *);
	Chained->Type = MLChainedFunctionT;
	Chained->Functions[0] = Args[0];
	Chained->Functions[1] = Args[1];
	return (ml_value_t *)Chained;
}

ML_METHOD(">>", MLChainedFunctionT, MLFunctionT) {
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Functions[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 2, ml_value_t *);
	Chained->Type = MLChainedFunctionT;
	for (int I = 0; I < N; ++I) Chained->Functions[I] = Base->Functions[I];
	Chained->Functions[N] = Args[1];
	return (ml_value_t *)Chained;
}

typedef struct ml_iter_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Values[];
} ml_iter_state_t;

static void first_iterate(ml_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Caller, Result);
	return ml_iter_value(State->Caller, Result);
}

ML_FUNCTIONX(First) {
	ML_CHECKX_ARG_COUNT(1);
	ml_state_t *State = new(ml_state_t);
	State->Caller = Caller;
	State->run = first_iterate;
	State->Context = Caller->Context;
	return ml_iterate(State, ml_chained(Count, Args));
}

static void first2_iter_value(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	ml_tuple_set(State->Values[0], 2, Result);
	ML_CONTINUE(State->Base.Caller, State->Values[0]);
}

static void first2_iter_key(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	ml_tuple_set(State->Values[0], 1, Result);
	State->Base.run = (ml_state_fn)first2_iter_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void first2_iterate(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Base.run = (ml_state_fn)first2_iter_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Result);
}

ML_FUNCTIONX(First2) {
	ML_CHECKX_ARG_COUNT(1);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (ml_state_fn)first2_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_tuple(2);
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

static void last_iterate(ml_iter_state_t *State, ml_value_t *Result);

static void last_value(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	State->Values[0] = Result;
	State->Base.run = (void *)last_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void last_iterate(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[0]);
	State->Base.run = (void *)last_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Result);
}

ML_FUNCTIONX(Last) {
	ML_CHECKX_ARG_COUNT(1);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)last_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = MLNil;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

static void last2_iterate(ml_iter_state_t *State, ml_value_t *Result);

static void last2_value(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	State->Values[1] = Result;
	State->Base.run = (void *)last2_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void last2_key(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	State->Values[0] = Result;
	State->Base.run = (void *)last2_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void last2_iterate(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) {
		if (State->Values[0]) {
			ml_value_t *Tuple = ml_tuple(2);
			ml_tuple_set(Tuple, 1, State->Values[0]);
			ml_tuple_set(Tuple, 2, State->Values[1]);
			ML_CONTINUE(State->Base.Caller, Tuple);
		} else {
			ML_CONTINUE(State->Base.Caller, MLNil);
		}
	}
	State->Base.run = (void *)last2_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Result);
}

ML_FUNCTIONX(Last2) {
	ML_CHECKX_ARG_COUNT(1);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)last2_iterate;
	State->Base.Context = Caller->Context;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

static void all_iterate(ml_iter_state_t *State, ml_value_t *Result);

static void all_iter_value(ml_iter_state_t *State, ml_value_t *Result) {
	Result = ml_deref(Result);
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	ml_list_append(State->Values[0], Result);
	State->Base.run = (void *)all_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void all_iterate(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	State->Base.run = (void *)all_iter_value;
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[0]);
	return ml_iter_value((ml_state_t *)State, State->Iter = Result);
}

ML_FUNCTIONX(All) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)all_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_list();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

extern ml_value_t *MLListOfMethod;

ML_METHODVX(MLListOfMethod, MLIteratableT) {
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)all_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_list();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

static void map_iterate(ml_iter_state_t *State, ml_value_t *Result);

static void map_iter_value(ml_iter_state_t *State, ml_value_t *Result) {
	Result = ml_deref(Result);
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	ml_map_insert(State->Values[0], State->Values[1], Result);
	State->Base.run = (void *)map_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void map_iter_key(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) Result = ml_integer(ml_map_size(State->Values[0]) + 1);
	State->Values[1] = Result;
	State->Base.run = (void *)map_iter_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void map_iterate(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[0]);
	State->Base.run = (void *)map_iter_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Result);
}

ML_FUNCTIONX(All2) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)map_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_map();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

extern ml_value_t *MLMapOfMethod;

ML_METHODVX(MLMapOfMethod, MLIteratableT) {
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)map_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_map();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

typedef struct ml_count_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	long Count;
} ml_count_state_t;

static void count_iterate(ml_count_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, ml_integer(State->Count));
	++State->Count;
	return ml_iter_next((ml_state_t *)State, State->Iter = Result);
}

ML_FUNCTIONX(Count) {
	ml_count_state_t *State = xnew(ml_count_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)count_iterate;
	State->Base.Context = Caller->Context;
	State->Count = 0;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

ML_METHOD_DECL(Less, "<");
ML_METHOD_DECL(Greater, ">");
ML_METHOD_DECL(Add, "+");
ML_METHOD_DECL(Mul, "*");

static void fold_iter_next(ml_iter_state_t *State, ml_value_t *Result);

static void fold_call(ml_iter_state_t *State, ml_value_t *Result) {
	Result = ml_deref(Result);
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result != MLNil) State->Values[1] = Result;
	State->Base.run = (void *)fold_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void fold_next_value(ml_iter_state_t *State, ml_value_t *Result) {
	Result = ml_deref(Result);
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	ml_value_t *Compare = State->Values[0];
	State->Values[2] = Result;
	State->Base.run = (void *)fold_call;
	return ml_typeof(Compare)->call((ml_state_t *)State, Compare, 2, State->Values + 1);
}

static void fold_iter_next(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[1] ?: MLNil);
	State->Base.run = (void *)fold_next_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Result);
}

static void fold_first_value(ml_iter_state_t *State, ml_value_t *Result) {
	Result = ml_deref(Result);
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	State->Values[1] = Result;
	State->Base.run = (void *)fold_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void fold_iterate(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	State->Base.run = (void *)fold_first_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Result);
}

ML_FUNCTIONX(Fold) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(1, MLFunctionT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)fold_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = Args[1];
	return ml_iterate((ml_state_t *)State, Args[0]);
}

ML_FUNCTIONX(Min) {
	ML_CHECKX_ARG_COUNT(1);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)fold_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = GreaterMethod;
	return ml_iterate((ml_state_t *)State, Args[0]);
}

ML_FUNCTIONX(Max) {
	ML_CHECKX_ARG_COUNT(1);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)fold_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = LessMethod;
	return ml_iterate((ml_state_t *)State, Args[0]);
}

ML_FUNCTIONX(Sum) {
	ML_CHECKX_ARG_COUNT(1);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)fold_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = AddMethod;
	return ml_iterate((ml_state_t *)State, Args[0]);
}

ML_FUNCTIONX(Prod) {
	ML_CHECKX_ARG_COUNT(1);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)fold_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = MulMethod;
	return ml_iterate((ml_state_t *)State, Args[0]);
}

static void fold2_iter_next(ml_iter_state_t *State, ml_value_t *Result);

static void fold2_next_key(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	State->Values[1] = Result;
	State->Base.run = (void *)fold2_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void fold2_call(ml_iter_state_t *State, ml_value_t *Result) {
	Result = ml_deref(Result);
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result != MLNil) {
		State->Values[2] = Result;
		State->Base.run = (void *)fold2_next_key;
		return ml_iter_key((ml_state_t *)State, State->Iter);
	} else {
		State->Base.run = (void *)fold2_iter_next;
		return ml_iter_next((ml_state_t *)State, State->Iter);
	}
}

static void fold2_next_value(ml_iter_state_t *State, ml_value_t *Result) {
	Result = ml_deref(Result);
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	ml_value_t *Compare = State->Values[0];
	State->Values[3] = Result;
	State->Base.run = (void *)fold2_call;
	return ml_typeof(Compare)->call((ml_state_t *)State, Compare, 2, State->Values + 2);
}

static void fold2_iter_next(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) {
		if (State->Values[1]) {
			ml_value_t *Tuple = ml_tuple(2);
			ml_tuple_set(Tuple, 1, State->Values[1]);
			ml_tuple_set(Tuple, 2, State->Values[2]);
			ML_CONTINUE(State->Base.Caller, Tuple);
		} else {
			ML_CONTINUE(State->Base.Caller, MLNil);
		}
	}
	State->Base.run = (void *)fold2_next_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Result);
}

static void fold2_first_value(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	State->Values[2] = Result;
	State->Base.run = (void *)fold2_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void fold2_first_key(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	State->Values[1] = Result;
	State->Base.run = (void *)fold2_first_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void fold2_iterate(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	State->Base.run = (void *)fold2_first_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Result);
}

ML_FUNCTIONX(Fold2) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(1, MLFunctionT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 4, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)fold2_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = Args[1];
	return ml_iterate((ml_state_t *)State, Args[0]);
}

ML_FUNCTIONX(Min2) {
	ML_CHECKX_ARG_COUNT(1);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)fold2_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = GreaterMethod;
	return ml_iterate((ml_state_t *)State, Args[0]);
}

ML_FUNCTIONX(Max2) {
	ML_CHECKX_ARG_COUNT(1);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)fold2_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = LessMethod;
	return ml_iterate((ml_state_t *)State, Args[0]);
}

typedef struct ml_folded_t {
	const ml_type_t *Type;
	ml_value_t *Value, *FoldFn;
} ml_folded_t;

ML_TYPE(MLFoldedT, (MLIteratableT), "folded");

ML_TYPE(MLFoldedStateT, (), "folded-state");

static void folded_iter_next(ml_iter_state_t *State, ml_value_t *Result);

static void folded_call(ml_iter_state_t *State, ml_value_t *Result) {
	Result = ml_deref(Result);
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	State->Values[1] = Result;
	State->Base.run = (void *)folded_iter_next;
	ML_CONTINUE(State->Base.Caller, State);
}

static void folded_next_value(ml_iter_state_t *State, ml_value_t *Result) {
	Result = ml_deref(Result);
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	ml_value_t *FoldFn = State->Values[0];
	State->Values[2] = Result;
	State->Base.run = (void *)folded_call;
	return ml_typeof(FoldFn)->call((ml_state_t *)State, FoldFn, 2, State->Values + 1);
}

static void folded_iter_next(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	State->Base.run = (void *)folded_next_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Result);
}

static void folded_first_value(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	State->Values[1] = Result;
	State->Base.run = (void *)folded_iter_next;
	ML_CONTINUE(State->Base.Caller, State);
}

static void folded_iterate(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	State->Base.run = (void *)folded_first_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Result);
}


static void ML_TYPED_FN(ml_iter_key, MLFoldedStateT, ml_state_t *Caller, ml_iter_state_t *State) {
	State->Base.Caller = Caller;
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLFoldedStateT, ml_state_t *Caller, ml_iter_state_t *State) {
	State->Base.Caller = Caller;
	ML_RETURN(State->Values[1]);
}

static void ML_TYPED_FN(ml_iter_next, MLFoldedStateT, ml_state_t *Caller, ml_iter_state_t *State) {
	State->Base.Caller = Caller;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ML_TYPED_FN(ml_iterate, MLFoldedT, ml_state_t *Caller, ml_folded_t *Folded) {
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Type = MLFoldedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)folded_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = Folded->FoldFn;
	return ml_iterate((ml_state_t *)State, Folded->Value);
}

ML_METHOD("//", MLIteratableT, MLFunctionT) {
	ml_folded_t *Folded = new(ml_folded_t);
	Folded->Type = MLFoldedT;
	Folded->Value = Args[0];
	Folded->FoldFn = Args[1];
	return (ml_value_t *)Folded;
}

typedef struct ml_limited_t {
	const ml_type_t *Type;
	ml_value_t *Value;
	int Remaining;
} ml_limited_t;

ML_TYPE(MLLimitedT, (MLIteratableT), "limited");

typedef struct ml_limited_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	int Remaining;
} ml_limited_state_t;

ML_TYPE(MLLimitedStateT, (), "limited-state");

static void limited_iterate(ml_limited_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Iter = Result;
	--State->Remaining;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iterate, MLLimitedT, ml_state_t *Caller, ml_limited_t *Limited) {
	if (Limited->Remaining) {
		ml_limited_state_t *State = new(ml_limited_state_t);
		State->Base.Type = MLLimitedStateT;
		State->Base.Caller = Caller;
		State->Base.run = (void *)limited_iterate;
		State->Base.Context = Caller->Context;
		State->Remaining = Limited->Remaining;
		return ml_iterate((ml_state_t *)State, Limited->Value);
	} else {
		ML_RETURN(MLNil);
	}
}

static void ML_TYPED_FN(ml_iter_key, MLLimitedStateT, ml_state_t *Caller, ml_limited_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLLimitedStateT, ml_state_t *Caller, ml_limited_state_t *State) {
	return ml_iter_value(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLLimitedStateT, ml_state_t *Caller, ml_limited_state_t *State) {
	if (State->Remaining) {
		State->Base.Caller = Caller;
		State->Base.run = (void *)limited_iterate;
		return ml_iter_next((ml_state_t *)State, State->Iter);
	} else {
		ML_RETURN(MLNil);
	}
}

ML_METHOD("limit", MLIteratableT, MLIntegerT) {
	ml_limited_t *Limited = new(ml_limited_t);
	Limited->Type = MLLimitedT;
	Limited->Value = Args[0];
	Limited->Remaining = ml_integer_value(Args[1]);
	return (ml_value_t *)Limited;
}

typedef struct ml_skipped_t {
	const ml_type_t *Type;
	ml_value_t *Value;
	long Remaining;
} ml_skipped_t;

ML_TYPE(MLSkippedT, (MLIteratableT), "skipped");

typedef struct ml_skipped_state_t {
	ml_state_t Base;
	long Remaining;
} ml_skipped_state_t;

static void skipped_iterate(ml_skipped_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	if (State->Remaining) {
		--State->Remaining;
		return ml_iter_next((ml_state_t *)State, Result);
	} else {
		ML_CONTINUE(State->Base.Caller, Result);
	}
}

static void ML_TYPED_FN(ml_iterate, MLSkippedT, ml_state_t *Caller, ml_skipped_t *Skipped) {
	if (Skipped->Remaining) {
		ml_skipped_state_t *State = new(ml_skipped_state_t);
		State->Base.Caller = Caller;
		State->Base.run = (void *)skipped_iterate;
		State->Base.Context = Caller->Context;
		State->Remaining = Skipped->Remaining;
		return ml_iterate((ml_state_t *)State, Skipped->Value);
	} else {
		return ml_iterate(Caller, Skipped->Value);
	}
}

ML_METHOD("skip", MLIteratableT, MLIntegerT) {
	ml_skipped_t *Skipped = new(ml_skipped_t);
	Skipped->Type = MLSkippedT;
	Skipped->Value = Args[0];
	Skipped->Remaining = ml_integer_value(Args[1]);
	return (ml_value_t *)Skipped;
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Result;
	size_t Waiting;
} ml_tasks_t;

static void ml_tasks_call(ml_state_t *Caller, ml_tasks_t *Tasks, int Count, ml_value_t **Args) {
	if (Tasks->Result != MLNil) ML_RETURN(Tasks->Result);
	ML_CHECKX_ARG_TYPE(Count - 1, MLFunctionT);
	ml_value_t *Function = Args[Count - 1];
	++Tasks->Waiting;
	ml_typeof(Function)->call((ml_state_t *)Tasks, Function, Count - 1, Args);
	ML_RETURN(Tasks);
}

ML_TYPE(MLTasksT, (MLFunctionT), "tasks",
	.call = (void *)ml_tasks_call
);

static void ml_tasks_continue(ml_tasks_t *Tasks, ml_value_t *Value) {
	if (ml_is_error(Value)) Tasks->Result = Value;
	if (--Tasks->Waiting == 0) ML_CONTINUE(Tasks->Base.Caller, Tasks->Result);
}

ML_FUNCTIONX(Tasks) {
	ml_tasks_t *Tasks = new(ml_tasks_t);
	Tasks->Base.Type = MLTasksT;
	Tasks->Base.run = (void *)ml_tasks_continue;
	Tasks->Result = MLNil;
	Tasks->Waiting = 1;
	ML_RETURN(Tasks);
}

ML_METHODX("wait", MLTasksT) {
	ml_tasks_t *Tasks = (ml_tasks_t *)Args[0];
	Tasks->Base.Caller = Caller;
	Tasks->Base.Context = Caller->Context;
	if (--Tasks->Waiting == 0) ML_CONTINUE(Tasks->Base.Caller, Tasks->Result);
}

typedef struct ml_parallel_iter_t ml_parallel_iter_t;

typedef struct {
	ml_state_t Base;
	ml_state_t NextState[1];
	ml_state_t KeyState[1];
	ml_state_t ValueState[1];
	ml_value_t *Iter;
	ml_value_t *Function;
	ml_value_t *Args[2];
	size_t Waiting, Limit, Burst;
} ml_parallel_t;

static void parallel_iter_next(ml_state_t *State, ml_value_t *Iter) {
	ml_parallel_t *Parallel = (ml_parallel_t *)((char *)State - offsetof(ml_parallel_t, NextState));
	if (Iter == MLNil) {
		Parallel->Iter = NULL;
		ML_CONTINUE(Parallel, MLNil);
	}
	if (ml_is_error(Iter)) ML_CONTINUE(Parallel->Base.Caller, Iter);
	return ml_iter_key(Parallel->KeyState, Parallel->Iter = Iter);
}

static void parallel_iter_key(ml_state_t *State, ml_value_t *Value) {
	ml_parallel_t *Parallel = (ml_parallel_t *)((char *)State - offsetof(ml_parallel_t, KeyState));
	Parallel->Args[0] = Value;
	return ml_iter_value(Parallel->ValueState, Parallel->Iter);
}

static void parallel_iter_value(ml_state_t *State, ml_value_t *Value) {
	ml_parallel_t *Parallel = (ml_parallel_t *)((char *)State - offsetof(ml_parallel_t, ValueState));
	Parallel->Waiting += 1;
	Parallel->Args[1] = Value;
	ml_typeof(Parallel->Function)->call((ml_state_t *)Parallel, Parallel->Function, 2, Parallel->Args);
	if (Parallel->Iter) {
		if (Parallel->Waiting > Parallel->Limit) return;
		return ml_iter_next(Parallel->NextState, Parallel->Iter);
	}
}

static void parallel_continue(ml_parallel_t *Parallel, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		Parallel->Waiting = 0xFFFFFFFF;
		ML_CONTINUE(Parallel->Base.Caller, Value);
	}
	--Parallel->Waiting;
	if (Parallel->Iter) {
		if (Parallel->Waiting > Parallel->Burst) return;
		return ml_iter_next(Parallel->NextState, Parallel->Iter);
	}
	if (Parallel->Waiting == 0) ML_CONTINUE(Parallel->Base.Caller, MLNil);
}

ML_FUNCTIONX(Parallel) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);

	ml_parallel_t *Parallel = new(ml_parallel_t);
	Parallel->Base.Caller = Caller;
	Parallel->Base.run = (void *)parallel_continue;
	Parallel->Base.Context = Caller->Context;
	Parallel->Waiting = 1;
	Parallel->NextState->run = parallel_iter_next;
	Parallel->NextState->Context = Caller->Context;
	Parallel->KeyState->run = parallel_iter_key;
	Parallel->KeyState->Context = Caller->Context;
	Parallel->ValueState->run = parallel_iter_value;
	Parallel->ValueState->Context = Caller->Context;

	if (Count > 3) {
		ML_CHECKX_ARG_TYPE(1, MLIntegerT);
		ML_CHECKX_ARG_TYPE(2, MLIntegerT);
		ML_CHECKX_ARG_TYPE(3, MLFunctionT);
		Parallel->Limit = ml_integer_value(Args[2]);
		Parallel->Burst = ml_integer_value(Args[1]) + 1;
		Parallel->Function = Args[3];
	} else if (Count > 2) {
		ML_CHECKX_ARG_TYPE(1, MLIntegerT);
		ML_CHECKX_ARG_TYPE(2, MLFunctionT);
		Parallel->Limit = ml_integer_value(Args[1]);
		Parallel->Burst = 0xFFFFFFFF;
		Parallel->Function = Args[2];
	} else {
		ML_CHECKX_ARG_TYPE(1, MLFunctionT);
		Parallel->Limit = 0xFFFFFFFF;
		Parallel->Burst = 0xFFFFFFFF;
		Parallel->Function = Args[1];
	}

	return ml_iterate(Parallel->NextState, Args[0]);
}

typedef struct ml_unique_t {
	const ml_type_t *Type;
	ml_value_t *Iter;
} ml_unique_t;

ML_TYPE(MLUniqueT, (MLIteratableT), "unique");

typedef struct ml_unique_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *History;
	ml_value_t *Value;
	int Iteration;
} ml_unique_state_t;

ML_TYPE(MLUniqueStateT, (), "unique-state");

static void ml_unique_fnx_iterate(ml_unique_state_t *State, ml_value_t *Result);

static void ml_unique_fnx_value(ml_unique_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (ml_map_insert(State->History, Result, MLSome) == MLNil) {
		State->Value = Result;
		++State->Iteration;
		ML_CONTINUE(State->Base.Caller, State);
	}
	State->Base.run = (void *)ml_unique_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ml_unique_fnx_iterate(ml_unique_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Base.run = (void *)ml_unique_fnx_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Result);
}

static void ML_TYPED_FN(ml_iterate, MLUniqueT, ml_state_t *Caller, ml_unique_t *Unique) {
	ml_unique_state_t *State = new(ml_unique_state_t);
	State->Base.Type = MLUniqueStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_unique_fnx_iterate;
	State->Base.Context = Caller->Context;
	State->History = ml_map();
	State->Iteration = 0;
	return ml_iterate((ml_state_t *)State, Unique->Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLUniqueStateT, ml_state_t *Caller, ml_unique_state_t *State) {
	ML_RETURN(ml_integer(State->Iteration));
}

static void ML_TYPED_FN(ml_iter_value, MLUniqueStateT, ml_state_t *Caller, ml_unique_state_t *State) {
	ML_RETURN(State->Value);
}

static void ML_TYPED_FN(ml_iter_next, MLUniqueStateT, ml_state_t *Caller, ml_unique_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_unique_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_FUNCTION(Unique) {
	ML_CHECK_ARG_COUNT(1);
	ml_unique_t *Unique = new(ml_unique_t);
	Unique->Type = MLUniqueT;
	Unique->Iter = Args[0];
	return (ml_value_t *)Unique;
}

typedef struct ml_grouped_t {
	const ml_type_t *Type;
	ml_value_t *Function;
	ml_value_t **Iters;
	int Count;
} ml_grouped_t;

ML_TYPE(MLGroupedT, (MLIteratableT), "grouped");

typedef struct ml_grouped_state_t {
	ml_state_t Base;
	ml_value_t *Function;
	ml_value_t **Iters;
	ml_value_t **Args;
	int Count, Index, Iteration;
} ml_grouped_state_t;

ML_TYPE(MLGroupedStateT, (), "grouped-state");

static void grouped_iterate(ml_grouped_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Iters[State->Index] = Result;
	if (++State->Index ==  State->Count) ML_CONTINUE(State->Base.Caller, State);
	return ml_iterate((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iterate, MLGroupedT, ml_state_t *Caller, ml_grouped_t *Grouped) {
	ml_grouped_state_t *State = new(ml_grouped_state_t);
	State->Base.Type = MLGroupedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)grouped_iterate;
	State->Base.Context = Caller->Context;
	State->Function = Grouped->Function;
	State->Iters = anew(ml_value_t *, Grouped->Count);
	State->Args = anew(ml_value_t *, Grouped->Count);
	for (int I = 0; I < Grouped->Count; ++I) State->Iters[I] = Grouped->Iters[I];
	State->Count = Grouped->Count;
	State->Iteration = 1;
	return ml_iterate((ml_state_t *)State, State->Iters[0]);
}

static void ML_TYPED_FN(ml_iter_key, MLGroupedStateT, ml_state_t *Caller, ml_grouped_state_t *State) {
	ML_RETURN(ml_integer(State->Iteration));
}

static void ml_grouped_fnx_value(ml_grouped_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	State->Args[State->Index] = Result;
	if (++State->Index ==  State->Count) {
		return ml_typeof(State->Function)->call(State->Base.Caller, State->Function, State->Count, State->Args);
	}
	return ml_iter_value((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iter_value, MLGroupedStateT, ml_state_t *Caller, ml_grouped_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_grouped_fnx_value;
	State->Index = 0;
	return ml_iter_value((ml_state_t *)State, State->Iters[0]);
}

static void grouped_iter_next(ml_grouped_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Iters[State->Index] = Result;
	if (++State->Index ==  State->Count) ML_CONTINUE(State->Base.Caller, State);
	return ml_iter_next((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iter_next, MLGroupedStateT, ml_state_t *Caller, ml_grouped_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)grouped_iter_next;
	++State->Iteration;
	State->Index = 0;
	return ml_iter_next((ml_state_t *)State, State->Iters[0]);
}

ML_FUNCTION(Group) {
	ml_grouped_t *Grouped = new(ml_grouped_t);
	Grouped->Type = MLGroupedT;
	Grouped->Count = Count - 1;
	Grouped->Function = Args[Count - 1];
	Grouped->Iters = anew(ml_value_t *, Count - 1);
	for (int I = 0; I < Count - 1; ++I) Grouped->Iters[I] = Args[I];
	return (ml_value_t *)Grouped;
}

typedef struct ml_repeated_t {
	const ml_type_t *Type;
	ml_value_t *Value, *Function;
} ml_repeated_t;

ML_TYPE(MLRepeatedT, (MLIteratableT), "repeated");

typedef struct ml_repeated_state_t {
	ml_state_t Base;
	ml_value_t *Value, *Function;
	int Iteration;
} ml_repeated_state_t;

ML_TYPE(MLRepeatedStateT, (), "repeated-state");

static void repeated_call(ml_repeated_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Value = Result;
	++State->Iteration;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iter_next, MLRepeatedStateT, ml_state_t *Caller, ml_repeated_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)repeated_call;
	return ml_typeof(State->Function)->call((ml_state_t *)State, State->Function, 1, &State->Value);
}

static void ML_TYPED_FN(ml_iter_key, MLRepeatedStateT, ml_state_t *Caller, ml_repeated_state_t *State) {
	ML_RETURN(ml_integer(State->Iteration));
}

static void ML_TYPED_FN(ml_iter_value, MLRepeatedStateT, ml_state_t *Caller, ml_repeated_state_t *State) {
	ML_RETURN(State->Value);
}

static void ML_TYPED_FN(ml_iterate, MLRepeatedT, ml_state_t *Caller, ml_repeated_t *Repeated) {
	ml_repeated_state_t *State = new(ml_repeated_state_t);
	State->Base.Type = MLRepeatedStateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)repeated_call;
	State->Value = Repeated->Value;
	State->Function = Repeated->Function;
	State->Iteration = 0;
	return ml_typeof(State->Function)->call((ml_state_t *)State, State->Function, 1, &State->Value);
}

ML_FUNCTION(Repeat) {
	ML_CHECK_ARG_COUNT(1);
	ml_repeated_t *Repeated = new(ml_repeated_t);
	Repeated->Type = MLRepeatedT;
	Repeated->Value = Args[0];
	Repeated->Function = Count > 1 ? Args[1] : ml_integer(1);
	return (ml_value_t *)Repeated;
}

typedef struct ml_sequenced_t {
	const ml_type_t *Type;
	ml_value_t *First, *Second;
} ml_sequenced_t;

ML_TYPE(MLSequencedT, (MLIteratableT), "sequenced");

typedef struct ml_sequenced_state_t {
	ml_state_t Base;
	ml_value_t *Iter, *Next;
} ml_sequenced_state_t;

ML_TYPE(MLSequencedStateT, (), "sequenced-state");

static void ml_sequenced_fnx_iterate(ml_sequenced_state_t *State, ml_value_t *Result);

static void ML_TYPED_FN(ml_iter_next, MLSequencedStateT, ml_state_t *Caller, ml_sequenced_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_sequenced_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLSequencedStateT, ml_state_t *Caller, ml_sequenced_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLSequencedStateT, ml_state_t *Caller, ml_sequenced_state_t *State) {
	return ml_iter_value(Caller, State->Iter);
}

static void ml_sequenced_fnx_iterate(ml_sequenced_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) {
		return ml_iterate(State->Base.Caller, State->Next);
	}
	State->Iter = Result;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iterate, MLSequencedT, ml_state_t *Caller, ml_sequenced_t *Sequenced) {
	ml_sequenced_state_t *State = new(ml_sequenced_state_t);
	State->Base.Type = MLSequencedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_sequenced_fnx_iterate;
	State->Next = Sequenced->Second;
	return ml_iterate((ml_state_t *)State, Sequenced->First);
}

ML_METHOD("||", MLIteratableT, MLIteratableT) {
	ml_sequenced_t *Sequenced = xnew(ml_sequenced_t, 3, ml_value_t *);
	Sequenced->Type = MLSequencedT;
	Sequenced->First = Args[0];
	Sequenced->Second = Args[1];
	return (ml_value_t *)Sequenced;
}

ML_METHOD("||", MLIteratableT) {
	ml_sequenced_t *Sequenced = xnew(ml_sequenced_t, 3, ml_value_t *);
	Sequenced->Type = MLSequencedT;
	Sequenced->First = Args[0];
	Sequenced->Second = (ml_value_t *)Sequenced;
	return (ml_value_t *)Sequenced;
}

typedef struct {
	const ml_type_t *Type;
	ml_value_t *Value;
} ml_swapped_t;

ML_TYPE(MLSwappedT, (MLIteratableT), "swapped");

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
} ml_swapped_state_t;

ML_TYPE(MLSwappedStateT, (), "swapped-state");

static void swapped_iterate(ml_swapped_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Iter = Result;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iterate, MLSwappedT, ml_state_t *Caller, ml_swapped_t *Swapped) {
	ml_swapped_state_t *State = new(ml_swapped_state_t);
	State->Base.Caller = Caller;
	State->Base.Type = MLSwappedStateT;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)swapped_iterate;
	return ml_iterate((ml_state_t *)State, Swapped->Value);
}

static void ML_TYPED_FN(ml_iter_key, MLSwappedStateT, ml_state_t *Caller, ml_swapped_state_t *State) {
	return ml_iter_value(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLSwappedStateT, ml_state_t *Caller, ml_swapped_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLSwappedStateT, ml_state_t *Caller, ml_swapped_state_t *State) {
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_METHOD("swap", MLIteratableT) {
	ml_swapped_t *Swapped = new(ml_swapped_t);
	Swapped->Type = MLSwappedT;
	Swapped->Value = Args[0];
	return (ml_value_t *)Swapped;
}

void ml_iterfns_init(stringmap_t *Globals) {
#include "ml_iterfns_init.c"
	stringmap_insert(Globals, "first", First);
	stringmap_insert(Globals, "first2", First2);
	stringmap_insert(Globals, "last", Last);
	stringmap_insert(Globals, "last2", Last2);
	stringmap_insert(Globals, "all", All);
	stringmap_insert(Globals, "all2", All2);
	stringmap_insert(Globals, "count", Count);
	stringmap_insert(Globals, "fold", Fold);
	stringmap_insert(Globals, "min", Min);
	stringmap_insert(Globals, "max", Max);
	stringmap_insert(Globals, "sum", Sum);
	stringmap_insert(Globals, "prod", Prod);
	stringmap_insert(Globals, "fold2", Fold2);
	stringmap_insert(Globals, "min2", Min2);
	stringmap_insert(Globals, "max2", Max2);
	stringmap_insert(Globals, "parallel", Parallel);
	stringmap_insert(Globals, "unique", Unique);
	stringmap_insert(Globals, "tasks", Tasks);
	stringmap_insert(Globals, "group", Group);
	stringmap_insert(Globals, "repeat", Repeat);
}

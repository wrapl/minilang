#include "ml_sequence.h"
#include "ml_macros.h"
#include "ml_runtime.h"
#include "minilang.h"
#include <string.h>

#undef ML_CATEGORY
#define ML_CATEGORY "sequence"

/****************************** Chained ******************************/

static ML_METHOD_DECL(SoloMethod, "->");
static ML_METHOD_DECL(DuoMethod, "=>");
static ML_METHOD_DECL(FilterSoloMethod, "->?");
static ML_METHOD_DECL(FilterDuoMethod, "=>?");
static ML_METHOD_DECL(SoloApplyMethod, "->!");
static ML_METHOD_DECL(FilterSoloApplyMethod, "->!?");
static ML_METHOD_DECL(ApplyMethod, "!");

typedef struct ml_chained_state_t {
	ml_state_t Base;
	ml_value_t *Value;
	ml_value_t **Current;
} ml_chained_state_t;

static void ml_chained_state_value(ml_chained_state_t *State, ml_value_t *Value);

static void ml_chained_state_filter(ml_chained_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t **Entry = State->Current;
	if (!Entry[0]) ML_CONTINUE(State->Base.Caller, State->Value);
	State->Current = Entry + 1;
	ml_value_t *Function = Entry[0];
	if (Function == FilterSoloMethod) {
		Function = Entry[1];
		if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
		State->Current = Entry + 2;
		State->Base.run = (void *)ml_chained_state_filter;
	}
	return ml_call(State, Function, 1, &State->Value);
}

static void ml_chained_state_value(ml_chained_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t **Entry = State->Current;
	if (!Entry[0]) ML_CONTINUE(State->Base.Caller, Value);
	State->Current = Entry + 1;
	State->Value = Value;
	ml_value_t *Function = Entry[0];
	if (Function == FilterSoloMethod) {
		Function = Entry[1];
		if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
		State->Current = Entry + 2;
		State->Base.run = (void *)ml_chained_state_filter;
	}
	return ml_call(State, Function, 1, &State->Value);
}

typedef struct ml_chained_function_t {
	ml_type_t *Type;
	ml_value_t *Entries[];
} ml_chained_function_t;

static void ml_chained_function_call(ml_state_t *Caller, ml_chained_function_t *Chained, int Count, ml_value_t **Args) {
	ml_chained_state_t *State = new(ml_chained_state_t);
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_chained_state_value;
	State->Base.Context = Caller->Context;
	State->Current = Chained->Entries + 1;
	return ml_call(State, Chained->Entries[0], Count, Args);
}

ml_value_t *ml_chained(int Count, ml_value_t **Functions);

ML_FUNCTION(MLChained) {
//@chained
//<Base
//<Fn/1,...,Fn/n:function
//>chained
// Returns a new chained function or sequence with base :mini:`Base` and additional functions or filters :mini:`Fn/1, ..., Fn/n`.
//$- let F := chained(fun(X) X + 1, fun(X) X ^ 2)
//$= F(10)
	ML_CHECK_ARG_COUNT(1);
	return ml_chained(Count, Args);
}

ML_TYPE(MLChainedT, (MLFunctionT, MLSequenceT), "chained",
// A chained function or sequence, consisting of a base function or sequence and any number of additional functions or filters.
//
// When used as a function or sequence, the base is used to produce an initial result, then the additional functions are applied in turn to the result.
//
// Filters do not affect the result but will shortcut a function call or skip an iteration if :mini:`nil` is returned. I.e. filters remove values from a sequence that fail a condition without affecting the values that pass.
	.call = (void *)ml_chained_function_call,
	.Constructor = (ml_value_t *)MLChained
);

ml_value_t *ml_chained(int Count, ml_value_t **Functions) {
	if (Count == 1) return Functions[0];
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, Count + 1, ml_value_t *);
	Chained->Type = MLChainedT;
	for (int I = 0; I < Count; ++I) Chained->Entries[I] = *Functions++;
	return (ml_value_t *)Chained;
}

ml_value_t *ml_chainedv(int Count, ...) {
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, Count + 1, ml_value_t *);
	Chained->Type = MLChainedT;
	va_list Args;
	va_start(Args, Count);
	for (int I = 0; I < Count; ++I) Chained->Entries[I] = va_arg(Args, ml_value_t *);
	va_end(Args);
	return (ml_value_t *)Chained;
}

typedef struct ml_chained_iterator_t {
	ml_state_t Base;
	ml_value_t *Iterator;
	ml_value_t **Current, **Entries;
	ml_value_t *Values[4];
} ml_chained_iterator_t;

ML_TYPE(MLChainedStateT, (MLStateT), "chained-state");
//!internal

static void ML_TYPED_FN(ml_iter_key, MLChainedStateT, ml_state_t *Caller, ml_chained_iterator_t *State) {
	ML_RETURN(State->Values[0]);
}

static void ML_TYPED_FN(ml_iter_value, MLChainedStateT, ml_state_t *Caller, ml_chained_iterator_t *State) {
	ML_RETURN(State->Values[1]);
}

static void ml_chained_iterator_next(ml_chained_iterator_t *State, ml_value_t *Iter);

static void ml_chained_iterator_continue(ml_chained_iterator_t *State);

static void ml_chained_iterator_filter(ml_chained_iterator_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		State->Base.run = (void *)ml_chained_iterator_next;
		State->Current = State->Entries;
		return ml_iter_next((ml_state_t *)State, State->Iterator);
	}
	return ml_chained_iterator_continue(State);
}

static void ml_chained_iterator_value(ml_chained_iterator_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Values[2] = NULL;
	return ml_chained_iterator_continue(State);
}

static void ml_chained_iterator_duo_key(ml_chained_iterator_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[2] = State->Values[1];
	State->Values[1] = State->Values[0];
	State->Values[0] = Value;
	ml_value_t **Entry = State->Current;
	ml_value_t *Function = Entry[0];
	if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
	State->Current = Entry + 1;
	State->Base.run = (void *)ml_chained_iterator_value;
	return ml_call(State, Function, 2, State->Values + 1);
}

static void ml_chained_iterator_continue(ml_chained_iterator_t *State) {
	ml_value_t **Entry = State->Current;
	ml_value_t *Function = Entry[0];
	if (!Function) ML_CONTINUE(State->Base.Caller, State);
	if (Function == SoloMethod) {
		Function = Entry[1];
		if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
		State->Current = Entry + 2;
		State->Base.run = (void *)ml_chained_iterator_value;
		return ml_call(State, Function, 1, State->Values + 1);
	} else if (Function == DuoMethod) {
		Function = Entry[1];
		if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
		State->Current = Entry + 2;
		State->Base.run = (void *)ml_chained_iterator_duo_key;
		return ml_call(State, Function, 2, State->Values);
	} else if (Function == FilterSoloMethod) {
		Function = Entry[1];
		if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
		State->Current = Entry + 2;
		State->Base.run = (void *)ml_chained_iterator_filter;
		return ml_call(State, Function, 1, State->Values + 1);
	} else if (Function == FilterDuoMethod) {
		Function = Entry[1];
		if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
		State->Current = Entry + 2;
		State->Base.run = (void *)ml_chained_iterator_filter;
		return ml_call(State, Function, 2, State->Values);
	} else if (Function == SoloApplyMethod) {
		Function = Entry[1];
		if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
		State->Current = Entry + 2;
		State->Base.run = (void *)ml_chained_iterator_value;
		State->Values[3] = State->Values[1];
		State->Values[2] = Function;
		return ml_call(State, ApplyMethod, 2, State->Values + 2);
	} else if (Function == FilterSoloApplyMethod) {
		Function = Entry[1];
		if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
		State->Current = Entry + 2;
		State->Base.run = (void *)ml_chained_iterator_filter;
		State->Values[3] = State->Values[1];
		State->Values[2] = Function;
		return ml_call(State, ApplyMethod, 2, State->Values + 2);
	} else {
		State->Current = Entry + 1;
		State->Base.run = (void *)ml_chained_iterator_value;
		return ml_call(State, Function, 1, State->Values + 1);
	}
}

static void ml_chained_iterator_key(ml_chained_iterator_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[0] = Value;
	State->Base.run = (void *)ml_chained_iterator_value;
	return ml_iter_value((ml_state_t *)State, State->Iterator);
}

static void ml_chained_iterator_next(ml_chained_iterator_t *State, ml_value_t *Iter) {
	if (ml_is_error(Iter)) ML_CONTINUE(State->Base.Caller, Iter);
	if (Iter == MLNil) ML_CONTINUE(State->Base.Caller, Iter);
	State->Base.run = (void *)ml_chained_iterator_key;
	State->Current = State->Entries;
	return ml_iter_key((ml_state_t *)State, State->Iterator = Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLChainedStateT, ml_state_t *Caller, ml_chained_iterator_t *State) {
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_chained_iterator_next;
	return ml_iter_next((ml_state_t *)State, State->Iterator);
}

static void ML_TYPED_FN(ml_iterate, MLChainedT, ml_state_t *Caller, ml_chained_function_t *Chained) {
	ml_chained_iterator_t *State = new(ml_chained_iterator_t);
	State->Base.Type =  MLChainedStateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_chained_iterator_next;
	State->Entries = Chained->Entries + 1;
	return ml_iterate((ml_state_t *)State, Chained->Entries[0]);
}

ML_METHOD("->", MLFunctionT, MLFunctionT) {
//<Base
//<Function
//>chained
// Returns a chained function equivalent to :mini:`Function(Base(...))`.
//$- let F := :upper -> (3 * _)
//$= F("hello")
//$= F("cake")
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 3, ml_value_t *);
	Chained->Type = MLChainedT;
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = Args[1];
	//Chained->Entries[2] = NULL;
	return (ml_value_t *)Chained;
}

ML_METHOD("->", MLSequenceT, MLFunctionT) {
//<Base
//<F
//>sequence
// Returns a chained sequence equivalent to :mini:`(K/1, F(V/1)), ..., (K/n, F(V/n))` where :mini:`K/i` and :mini:`V/i` are the keys and values produced by :mini:`Base`.
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 3, ml_value_t *);
	Chained->Type = MLChainedT;
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = Args[1];
	//Chained->Entries[2] = NULL;
	return (ml_value_t *)Chained;
}

ML_METHOD("->", MLChainedT, MLFunctionT) {
//!internal
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Entries[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 2, ml_value_t *);
	Chained->Type = MLChainedT;
	for (int I = 0; I < N; ++I) Chained->Entries[I] = Base->Entries[I];
	Chained->Entries[N] = Args[1];
	return (ml_value_t *)Chained;
}

ML_METHOD("=>", MLSequenceT, MLFunctionT) {
//<Base
//<F
//>sequence
// Returns a chained sequence equivalent to :mini:`(K/1, F(K/1, V/1)), ..., (K/n, F(K/n, V/n))` where :mini:`K/i` and :mini:`V/i` are the keys and values produced by :mini:`Base`.
//$= map("cake" => *)
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 5, ml_value_t *);
	Chained->Type = MLChainedT;
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = DuoMethod;
	Chained->Entries[2] = ml_integer(1);
	Chained->Entries[3] = Args[1];
	//Chained->Entries[4] = NULL;
	return (ml_value_t *)Chained;
}

ML_METHOD("=>", MLSequenceT, MLFunctionT, MLFunctionT) {
//<Base
//<F/1
//<F/2
//>sequence
// Returns a chained sequence equivalent to :mini:`(F/1(K/1, V/1), F/2(K/1, V/1)), ..., (F/1(K/n, V/n), F/2(K/n, V/n))` where :mini:`K/i` and :mini:`V/i` are the keys and values produced by :mini:`Base`.
//$= map("cake" => (tuple, *))
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 5, ml_value_t *);
	Chained->Type = MLChainedT;
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = DuoMethod;
	Chained->Entries[2] = Args[1];
	Chained->Entries[3] = Args[2];
	//Chained->Entries[4] = NULL;
	return (ml_value_t *)Chained;
}

ML_METHOD("=>", MLChainedT, MLFunctionT) {
//!internal
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Entries[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 4, ml_value_t *);
	Chained->Type = MLChainedT;
	for (int I = 0; I < N; ++I) Chained->Entries[I] = Base->Entries[I];
	Chained->Entries[N] = DuoMethod;
	Chained->Entries[N + 1] = ml_integer(1);
	Chained->Entries[N + 2] = Args[1];
	return (ml_value_t *)Chained;
}

ML_METHOD("=>", MLChainedT, MLFunctionT, MLFunctionT) {
//!internal
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Entries[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 4, ml_value_t *);
	Chained->Type = MLChainedT;
	for (int I = 0; I < N; ++I) Chained->Entries[I] = Base->Entries[I];
	Chained->Entries[N] = DuoMethod;
	Chained->Entries[N + 1] = Args[1];
	Chained->Entries[N + 2] = Args[2];
	return (ml_value_t *)Chained;
}

ML_METHOD("->!", MLSequenceT, MLFunctionT) {
//<Base
//<F
//>sequence
// Returns a chained sequence equivalent to :mini:`(K/1, F ! V/1), ..., (K/n, F ! V/n)` where :mini:`K/i` and :mini:`V/i` are the keys and values produced by :mini:`Base`.
//$= map({"A" is [1, 2], "B" is [3, 4], "C" is [5, 6]} ->! +)
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 4, ml_value_t *);
	Chained->Type = MLChainedT;
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = SoloApplyMethod;
	Chained->Entries[2] = Args[1];
	//Chained->Entries[3] = NULL;
	return (ml_value_t *)Chained;
}

ML_METHOD("->!", MLChainedT, MLFunctionT) {
//!internal
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Entries[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 3, ml_value_t *);
	Chained->Type = MLChainedT;
	for (int I = 0; I < N; ++I) Chained->Entries[I] = Base->Entries[I];
	Chained->Entries[N] = SoloApplyMethod;
	Chained->Entries[N + 1] = Args[1];
	return (ml_value_t *)Chained;
}

static inline ml_type_t *ml_generic_sequence(ml_type_t *Base, ml_value_t *Sequence) {
#ifdef ML_GENERICS
	ml_type_t *TArgs[3];
	if (ml_find_generic_parent(ml_typeof(Sequence), MLSequenceT, 3, TArgs) == 3) {
		if (TArgs[1] == MLAnyT && TArgs[2] == MLAnyT) return Base;
		TArgs[0] = Base;
		return ml_generic_type(3, TArgs);
	}
#endif
	return Base;
}

ML_METHOD("->?", MLSequenceT, MLFunctionT) {
//<Base
//<F
//>sequence
// Returns a chained sequence equivalent to :mini:`(K/j, V/j), ...` where :mini:`K/i` and :mini:`V/i` are the keys and values produced by :mini:`Base` and :mini:`F(V/j)` returns non-:mini:`nil`.
//$= list(1 .. 10 ->? (2 | _))
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 4, ml_value_t *);
	Chained->Type = ml_generic_sequence(MLChainedT, Args[0]);
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = FilterSoloMethod;
	Chained->Entries[2] = Args[1];
	//Chained->Entries[3] = NULL;
	return (ml_value_t *)Chained;
}

ML_METHOD("->?", MLChainedT, MLFunctionT) {
//!internal
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Entries[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 3, ml_value_t *);
	Chained->Type = MLChainedT;
	for (int I = 0; I < N; ++I) Chained->Entries[I] = Base->Entries[I];
	Chained->Entries[N] = FilterSoloMethod;
	Chained->Entries[N + 1] = Args[1];
	return (ml_value_t *)Chained;
}

ML_METHOD("=>?", MLSequenceT, MLFunctionT) {
//<Base
//<F
//>sequence
// Returns a chained sequence equivalent to :mini:`(K/j, V/j), ...` where :mini:`K/i` and :mini:`V/i` are the keys and values produced by :mini:`Base` and :mini:`F(K/j, V/j)` returns non-:mini:`nil`.
//$= let M := map(1 .. 10 -> fun(X) X ^ 2 % 10)
//$= map(M =>? !=)
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 4, ml_value_t *);
	Chained->Type = ml_generic_sequence(MLChainedT, Args[0]);
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = FilterDuoMethod;
	Chained->Entries[2] = Args[1];
	//Chained->Entries[3] = NULL;
	return (ml_value_t *)Chained;
}

ML_METHOD("=>?", MLChainedT, MLFunctionT) {
//!internal
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Entries[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 3, ml_value_t *);
	Chained->Type = ml_generic_sequence(MLChainedT, Args[0]);
	for (int I = 0; I < N; ++I) Chained->Entries[I] = Base->Entries[I];
	Chained->Entries[N] = FilterDuoMethod;
	Chained->Entries[N + 1] = Args[1];
	return (ml_value_t *)Chained;
}

ML_METHOD("->!?", MLSequenceT, MLFunctionT) {
//<Base
//<F
//>sequence
// Returns a chained sequence equivalent to :mini:`(K/j, V/j), ...` where :mini:`K/i` and :mini:`V/i` are the keys and values produced by :mini:`Base` and :mini:`F ! V/j` returns non-:mini:`nil`.
//$= map({"A" is [1, 2], "B" is [3, 3], "C" is [5, 6]} ->!? !=)
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 4, ml_value_t *);
	Chained->Type = ml_generic_sequence(MLChainedT, Args[0]);
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = FilterSoloApplyMethod;
	Chained->Entries[2] = Args[1];
	//Chained->Entries[3] = NULL;
	return (ml_value_t *)Chained;
}

ML_METHOD("->!?", MLChainedT, MLFunctionT) {
//!internal
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Entries[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 3, ml_value_t *);
	Chained->Type = ml_generic_sequence(MLChainedT, Args[0]);
	for (int I = 0; I < N; ++I) Chained->Entries[I] = Base->Entries[I];
	Chained->Entries[N] = FilterSoloApplyMethod;
	Chained->Entries[N + 1] = Args[1];
	return (ml_value_t *)Chained;
}

extern ml_value_t *IndexMethod;

ML_METHODV("[]", MLChainedT) {
	ml_value_t *Partial = ml_partial_function(IndexMethod, Count + 1);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Entries[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 2, ml_value_t *);
	Chained->Type = MLChainedT;
	for (int I = 0; I < N; ++I) Chained->Entries[I] = Base->Entries[I];
	Chained->Entries[N] = Partial;
	return (ml_value_t *)Chained;
}

ML_METHOD("::", MLChainedT, MLStringT) {
//!internal
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Entries[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 2, ml_value_t *);
	Chained->Type = MLChainedT;
	for (int I = 0; I < N; ++I) Chained->Entries[I] = Base->Entries[I];
	Chained->Entries[N] = ml_symbol(ml_string_value(Args[1]));
	return (ml_value_t *)Chained;
}

/****************************** Doubled ******************************/

typedef struct {
	ml_type_t *Type;
	ml_value_t *Sequence, *ValueFn;
} ml_doubled_t;

ML_TYPE(MLDoubledT, (MLSequenceT), "doubled");
//!internal

typedef struct ml_double_state_t {
	ml_state_t Base;
	ml_value_t *Iterator1;
	ml_value_t *Iterator2;
	ml_value_t *Function;
	ml_value_t *Args[];
} ml_double_state_t;

ML_TYPE(MLDoubledIteratorStateT, (MLStateT), "doubled-state");
//!internal

static void ml_double_iter0_next(ml_double_state_t *State, ml_value_t *Value);

static void ml_double_iter_next(ml_double_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		State->Base.run = (void *)ml_double_iter0_next;
		return ml_iter_next((ml_state_t *)State, State->Iterator1);
	}
	State->Iterator2 = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ml_double_function_call(ml_double_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)ml_double_iter_next;
	return ml_iterate((ml_state_t *)State, Value);
}

static void ml_double_value0(ml_double_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)ml_double_function_call;
	State->Args[0] = Value;
	ml_value_t *Function = State->Function;
	return ml_call(State, Function, 1, State->Args);
}

static void ml_double_iter0_next(ml_double_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)ml_double_value0;
	return ml_iter_value((ml_state_t *)State, State->Iterator1 = Value);
}

static void ML_TYPED_FN(ml_iterate, MLDoubledT, ml_state_t *Caller, ml_doubled_t *Doubled) {
	ml_double_state_t *State = xnew(ml_double_state_t, 1, ml_value_t *);
	State->Base.Type = MLDoubledIteratorStateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_double_iter0_next;
	State->Function = Doubled->ValueFn;
	return ml_iterate((ml_state_t *)State, Doubled->Sequence);
}

static void ML_TYPED_FN(ml_iter_key, MLDoubledIteratorStateT, ml_state_t *Caller, ml_double_state_t *State) {
	return ml_iter_key(Caller, State->Iterator2);
}

static void ML_TYPED_FN(ml_iter_value, MLDoubledIteratorStateT, ml_state_t *Caller, ml_double_state_t *State) {
	return ml_iter_value(Caller, State->Iterator2);
}

static void ML_TYPED_FN(ml_iter_next, MLDoubledIteratorStateT, ml_state_t *Caller, ml_double_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_double_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iterator2);
}

ml_value_t *ml_doubled(ml_value_t *Sequence, ml_value_t *Function) {
	ml_doubled_t *Doubled = new(ml_doubled_t);
	Doubled->Type = MLDoubledT;
	Doubled->Sequence = Sequence;
	Doubled->ValueFn = Function;
	return (ml_value_t *)Doubled;
}

ML_METHOD("->>", MLSequenceT, MLFunctionT) {
//<Sequence
//<Function
//>sequence
// Returns a new sequence that generates the keys and values from :mini:`Function(Value)` for each value generated by :mini:`Sequence`.
//$= list(1 .. 5 ->> (1 .. _))
	return ml_doubled(Args[0], Args[1]);
}

ML_METHOD("^", MLSequenceT, MLFunctionT) {
//<Sequence
//<Function
//>sequence
// Returns a new sequence that generates the keys and values from :mini:`Function(Value)` for each value generated by :mini:`Sequence`.
//
// .. deprecated:: 2.5.0
//
//    Use :mini:`->>` instead.
//$= list(1 .. 5 ^ (1 .. _))
	return ml_doubled(Args[0], Args[1]);
}

ML_TYPE(MLDoubled2T, (MLSequenceT), "doubled");
//!internal

ML_TYPE(MLDoubled2StateT, (MLStateT), "doubled-state");
//!internal

static void ml_double2_iter0_next(ml_double_state_t *State, ml_value_t *Value);

static void ml_double2_iter_next(ml_double_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		State->Base.run = (void *)ml_double2_iter0_next;
		return ml_iter_next((ml_state_t *)State, State->Iterator1);
	}
	State->Iterator2 = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ml_double2_function_call(ml_double_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)ml_double2_iter_next;
	return ml_iterate((ml_state_t *)State, Value);
}

static void ml_double2_value0(ml_double_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)ml_double2_function_call;
	State->Args[1] = Value;
	ml_value_t *Function = State->Function;
	return ml_call(State, Function, 2, State->Args);
}

static void ml_double2_key0(ml_double_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)ml_double2_value0;
	State->Args[0] = Value;
	return ml_iter_value((ml_state_t *)State, State->Iterator1);
}

static void ml_double2_iter0_next(ml_double_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)ml_double2_key0;
	return ml_iter_key((ml_state_t *)State, State->Iterator1 = Value);
}

static void ML_TYPED_FN(ml_iterate, MLDoubled2T, ml_state_t *Caller, ml_doubled_t *Doubled) {
	ml_double_state_t *State = xnew(ml_double_state_t, 2, ml_value_t *);
	State->Base.Type = MLDoubled2StateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_double2_iter0_next;
	State->Function = Doubled->ValueFn;
	return ml_iterate((ml_state_t *)State, Doubled->Sequence);
}

static void ML_TYPED_FN(ml_iter_key, MLDoubled2StateT, ml_state_t *Caller, ml_double_state_t *State) {
	return ml_iter_key(Caller, State->Iterator2);
}

static void ML_TYPED_FN(ml_iter_value, MLDoubled2StateT, ml_state_t *Caller, ml_double_state_t *State) {
	return ml_iter_value(Caller, State->Iterator2);
}

static void ML_TYPED_FN(ml_iter_next, MLDoubled2StateT, ml_state_t *Caller, ml_double_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_double2_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iterator2);
}

ML_METHOD("=>>", MLSequenceT, MLFunctionT) {
//<Sequence
//<Function
//>sequence
// Returns a new sequence that generates the keys and values from :mini:`Function(Key, Value)` for each key and value generated by :mini:`Sequence`.
//$= list("cake" =>> *)
	ml_doubled_t *Doubled = new(ml_doubled_t);
	Doubled->Type = MLDoubled2T;
	Doubled->Sequence = Args[0];
	Doubled->ValueFn = Args[1];
	return (ml_value_t *)Doubled;
}

/****************************** All ******************************/

static void all_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void all_iter_value(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)all_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void all_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, MLSome);
	State->Base.run = (void *)all_iter_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(All) {
//<Sequence
//>some | nil
// Returns :mini:`nil` if :mini:`nil` is produced by :mini:`Sequence`. Otherwise returns :mini:`some`. If :mini:`Sequence` is empty, then :mini:`some` is returned.
//$= all([1, 2, 3, 4])
//$= all([1, 2, nil, 4])
//$= all([])
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_iter_state_t *State = new(ml_iter_state_t);
	State->Base.Caller = Caller;
	State->Base.run = (void *)all_iterate;
	State->Base.Context = Caller->Context;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

/****************************** Some ******************************/

static void some_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void some_value(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value != MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)some_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void some_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)some_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void ml_some_call(ml_state_t *Caller, ml_value_t *Some, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	for (int I = 0; I < Count; ++I) Args[I] = ml_deref(Args[I]);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_iter_state_t *State = new(ml_iter_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)some_iterate;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

/*
ML_FUNCTIONX(Some) {
//<Sequence:sequence
//>any | nil
// Returns the first value produced by :mini:`Sequence` that is not :mini:`nil`.
//$= some([nil, nil, "X", nil])
//$= some([nil, nil, nil, nil])
}
*/

/****************************** First ******************************/

static void first_iterate(ml_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Caller, Value);
	return ml_iter_value(State->Caller, Value);
}

ML_METHODX("first", MLSequenceT) {
//<Sequence
//>any | nil
// Returns the first value produced by :mini:`Sequence`.
//$= first("cake")
//$= first([])
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_state_t *State = new(ml_state_t);
	State->Caller = Caller;
	State->Context = Caller->Context;
	State->run = first_iterate;
	return ml_iterate(State, ml_chained(Count, Args));
}

static void first2_iter_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t *Tuple = ml_tuplev(2, State->Values[0], Value);
	ML_CONTINUE(State->Base.Caller, Tuple);
}

static void first2_iter_key(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[0] = Value;
	State->Base.run = (ml_state_fn)first2_iter_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void first2_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)first2_iter_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Value);
}

ML_METHODX("first2", MLSequenceT) {
//<Sequence
//>tuple(any, any) | nil
// Returns the first key and value produced by :mini:`Sequence`.
//$= first2("cake")
//$= first2([])
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)first2_iterate;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

static void last_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void last_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[0] = Value;
	State->Base.run = (void *)last_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void last_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[0]);
	State->Base.run = (void *)last_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_METHODX("last", MLSequenceT) {
//<Sequence
//>any | nil
// Returns the last value produced by :mini:`Sequence`.
//$= last("cake")
//$= last([])
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)last_iterate;
	State->Values[0] = MLNil;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

static void last2_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void last2_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)last2_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void last2_key(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[0] = Value;
	State->Base.run = (void *)last2_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void last2_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		if (State->Values[0]) {
			ML_CONTINUE(State->Base.Caller, ml_tuplen(2, State->Values));
		} else {
			ML_CONTINUE(State->Base.Caller, MLNil);
		}
	}
	State->Base.run = (void *)last2_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Value);
}

ML_METHODX("last2", MLSequenceT) {
//<Sequence
//>tuple(any, any) | nil
// Returns the last key and value produced by :mini:`Sequence`.
//$= last2("cake")
//$= last2([])
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)last2_iterate;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

typedef struct ml_count_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	long Count;
} ml_count_state_t;

static void count_iterate(ml_count_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, ml_integer(State->Count));
	++State->Count;
	return ml_iter_next((ml_state_t *)State, State->Iter = Value);
}

/*
ML_FUNCTION(Count) {
//@count
//<Sequence
//>integer
// Returns the count of the values produced by :mini:`Sequence`. For some types of sequences (e.g. :mini:`list`, :mini:`map`, etc), the count is simply retrieved. For all other types, the sequence is iterated and the total number of values counted.
//$= count([1, 2, 3, 4])
//$= count(1 .. 10 ->? (2 | _))
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLSequenceT);
}
*/

ML_METHODVX("count", MLSequenceT) {
//!internal
	ml_count_state_t *State = new(ml_count_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)count_iterate;
	State->Count = 0;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

typedef struct ml_count2_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Counts;
} ml_count2_state_t;

static void count2_iterate(ml_count2_state_t *State, ml_value_t *Value);

static void count2_value(ml_count2_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value != MLNil) {
		ml_map_node_t *Node = ml_map_slot(State->Counts, Value);
		Node->Value = (ml_value_t *)((char *)Node->Value + 1);
	}
	State->Base.run = (void *)count2_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void count2_iterate(ml_count2_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		ML_MAP_FOREACH(State->Counts, Iter) Iter->Value = ml_integer((char *)Iter->Value - (char *)0);
		ML_CONTINUE(State->Base.Caller, State->Counts);
	}
	State->Base.run = (void *)count2_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(Count2) {
//<Sequence
//>map
// Returns a map of the values produced by :mini:`Sequence` with associated counts.
//$= count2("banana")
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_count2_state_t *State = new(ml_count2_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)count2_iterate;
	State->Counts = ml_map();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

/****************************** Find ******************************/

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Args[2];
} ml_find_state_t;

static void find_iterate(ml_find_state_t *State, ml_value_t *Value);

static void find_compare(ml_find_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value != MLNil) return ml_iter_key(State->Base.Caller, State->Iter);
	State->Base.run = (ml_state_fn)find_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static ML_METHOD_DECL(EqualMethod, "=");

static void find_value(ml_find_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)find_compare;
	State->Args[1] = Value;
	return ml_call(State, EqualMethod, 2, State->Args);
}

static void find_iterate(ml_find_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)find_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_METHODX("find", MLSequenceT, MLAnyT) {
//<Sequence
//<Value
//>any|nil
// Returns the first key generated by :mini:`Sequence` with correspding value :mini:`= Value`.
	ml_find_state_t *State = new(ml_find_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)find_iterate;
	State->Args[0] = Args[1];
	return ml_iterate((ml_state_t *)State, Args[0]);
}

/****************************** Random ******************************/

ML_METHODVX("random", MLTypeT) {
	ml_type_t *Type = (ml_type_t *)Args[0];
	ml_value_t *Random = stringmap_search(Type->Exports, "random");
	if (!Random) ML_ERROR("TypeError", "Type %s does not export random", Type->Name);
	return ml_call(Caller, Random, Count - 1, Args + 1);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Value;
	ml_value_t *Iter;
	int Index;
} ml_random_state_t;

static void random_iterate(ml_random_state_t *State, ml_value_t *Value);

static void random_value(ml_random_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Value = Value;
	State->Base.run = (void *)random_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void random_iterate(ml_random_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Value);
	int Index = ++State->Index;
	int Divisor = RAND_MAX / Index;
	int Random;
	do Random = random() / Divisor; while (Random >= Index);
	if (!Random) {
		State->Base.run = (void *)random_value;
		return ml_iter_value((ml_state_t *)State, State->Iter = Value);
	} else {
		return ml_iter_next((ml_state_t *)State, Value);
	}
}

ML_METHODVX("random", MLSequenceT) {
//<Sequence
//>any | nil
// Returns a random value produced by :mini:`Sequence`.
//$= random("cake")
//$= random([])
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_random_state_t *State = new(ml_random_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)random_iterate;
	State->Index = 0;
	State->Value = MLNil;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

extern ml_value_t *LessMethod;
extern ml_value_t *GreaterMethod;
extern ml_value_t *AddMethod;
extern ml_value_t *MulMethod;
extern ml_value_t *MinMethod;
extern ml_value_t *MaxMethod;

static void reduce_iter_next(ml_iter_state_t *State, ml_value_t *Value);

static void reduce_call(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)reduce_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void reduce_next_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t *Function = State->Values[0];
	State->Values[2] = Value;
	State->Base.run = (void *)reduce_call;
	return ml_call(State, Function, 2, State->Values + 1);
}

static void reduce_iter_next(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[1]);
	State->Base.run = (void *)reduce_next_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void reduce_first_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)reduce_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void reduce_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	State->Base.run = (void *)reduce_first_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(Reduce) {
//<Initial?:any
//<Sequence:sequence
//<Fn:function
//>any | nil
// Returns :mini:`Fn(Fn( ... Fn(Initial, V/1), V/2) ..., V/n)` where :mini:`V/i` are the values produced by :mini:`Sequence`.
// If :mini:`Initial` is omitted, the first value produced by :mini:`Sequence` is used.
//$= reduce(1 .. 10, +)
//$= reduce([], 1 .. 10, :put)
	ML_CHECKX_ARG_COUNT(2);
	if (Count == 2) {
		ML_CHECKX_ARG_TYPE(0, MLSequenceT);
		ML_CHECKX_ARG_TYPE(1, MLFunctionT);
		ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (void *)reduce_iterate;
		State->Values[0] = Args[1];
		return ml_iterate((ml_state_t *)State, Args[0]);
	} else {
		ML_CHECKX_ARG_TYPE(1, MLSequenceT);
		ML_CHECKX_ARG_TYPE(2, MLFunctionT);
		ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (void *)reduce_iter_next;
		State->Values[0] = Args[2];
		State->Values[1] = Args[0];
		return ml_iterate((ml_state_t *)State, Args[1]);
	}
}

ML_FUNCTIONX(Min) {
//<Sequence
//>any | nil
// Returns the smallest value (using :mini:`:min`) produced by :mini:`Sequence`.
//$= min([1, 5, 2, 10, 6])
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)reduce_iterate;
	State->Values[0] = MinMethod;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

ML_FUNCTIONX(Max) {
//<Sequence
//>any | nil
// Returns the largest value (using :mini:`:max`) produced by :mini:`Sequence`.
//$= max([1, 5, 2, 10, 6])
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)reduce_iterate;
	State->Values[0] = MaxMethod;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

ML_FUNCTIONX(Sum) {
//<Sequence
//>any | nil
// Returns the sum of the values (using :mini:`+`) produced by :mini:`Sequence`.
//$= sum([1, 5, 2, 10, 6])
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)reduce_iterate;
	State->Values[0] = AddMethod;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

ML_FUNCTIONX(Prod) {
//<Sequence
//>any | nil
// Returns the product of the values (using :mini:`*`) produced by :mini:`Sequence`.
//$= prod([1, 5, 2, 10, 6])
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)reduce_iterate;
	State->Values[0] = MulMethod;
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

static void reduce2_iter_next(ml_iter_state_t *State, ml_value_t *Value);

static void reduce2_call(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)reduce2_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void reduce2_next_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t *Function = State->Values[0];
	State->Values[3] = Value;
	State->Base.run = (void *)reduce2_call;
	return ml_call(State, Function, 3, State->Values + 1);
}

static void reduce2_next_key(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[2] = Value;
	State->Base.run = (void *)reduce2_next_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void reduce2_iter_next(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[1]);
	State->Base.run = (void *)reduce2_next_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Value);
}

static void reduce2_first_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = ml_tuplev(2, State->Values[1], Value);
	State->Base.run = (void *)reduce2_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void reduce2_first_key(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)reduce2_first_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void reduce2_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	State->Base.run = (void *)reduce2_first_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(Reduce2) {
//<Initial?:any
//<Sequence:sequence
//<Fn:function
//>any | nil
// Returns :mini:`Fn(Fn( ... Fn(Initial, K/1, V/1), K/2, V/2) ..., K/n, V/n)` where :mini:`K/i` and :mini:`V/i` are the keys and values produced by :mini:`Sequence`.
// If :mini:`Initial` is omitted, a tuple with the first key and value produced by :mini:`Sequence` is used.
//$= reduce2([], "cake", fun(L, K, V) L:put((K, V)))
	ML_CHECKX_ARG_COUNT(2);
	if (Count == 2) {
		ML_CHECKX_ARG_TYPE(0, MLSequenceT);
		ML_CHECKX_ARG_TYPE(1, MLFunctionT);
		ml_iter_state_t *State = xnew(ml_iter_state_t, 4, ml_value_t *);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (void *)reduce2_iterate;
		State->Values[0] = Args[1];
		return ml_iterate((ml_state_t *)State, Args[1]);
	} else {
		ML_CHECKX_ARG_TYPE(1, MLSequenceT);
		ML_CHECKX_ARG_TYPE(2, MLFunctionT);
		ml_iter_state_t *State = xnew(ml_iter_state_t, 4, ml_value_t *);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (void *)reduce2_iter_next;
		State->Values[0] = Args[2];
		State->Values[1] = Args[0];
		return ml_iterate((ml_state_t *)State, Args[1]);
	}
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Tuple, *Key, *Value;
	ml_value_t *Args[2];
} ml_extremum_state_t;

static void ml_extremum_run(ml_extremum_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (Value == MLNil) {
		ml_tuple_set(State->Tuple, 1, State->Key);
		ml_tuple_set(State->Tuple, 2, State->Value);
	}
	ML_RETURN(State->Tuple);
}

static void ml_minimum2_fn(ml_state_t *Caller, ml_extremum_state_t *State, int Count, ml_value_t **Args) {
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Tuple = Args[0];
	State->Key = Args[1];
	State->Value = Args[2];
	State->Args[0] = ml_tuple_get(State->Tuple, 2);
	State->Args[1] = State->Value;
	return ml_call(State, LessMethod, 2, State->Args);
}

ML_FUNCTIONX(Min2) {
//<Sequence
//>tuple | nil
// Returns a tuple with the key and value of the smallest value (using :mini:`<`) produced by :mini:`Sequence`.  Returns :mini:`nil` if :mini:`Sequence` is empty.
//$= min2([1, 5, 2, 10, 6])
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 4, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)reduce2_iterate;
	ml_extremum_state_t *Extremum = new(ml_extremum_state_t);
	Extremum->Base.run = (ml_state_fn)ml_extremum_run;
	State->Values[0] = ml_cfunctionx(Extremum, (ml_callbackx_t)ml_minimum2_fn);
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

static void ml_maximum2_fn(ml_state_t *Caller, ml_extremum_state_t *State, int Count, ml_value_t **Args) {
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Tuple = Args[0];
	State->Key = Args[1];
	State->Value = Args[2];
	State->Args[0] = ml_tuple_get(State->Tuple, 2);
	State->Args[1] = State->Value;
	return ml_call(State, GreaterMethod, 2, State->Args);
}

ML_FUNCTIONX(Max2) {
//<Sequence
//>tuple | nil
// Returns a tuple with the key and value of the largest value (using :mini:`>`) produced by :mini:`Sequence`.  Returns :mini:`nil` if :mini:`Sequence` is empty.
//$= max2([1, 5, 2, 10, 6])
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 4, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)reduce2_iterate;
	State->Base.Context = Caller->Context;
	ml_extremum_state_t *Extremum = new(ml_extremum_state_t);
	Extremum->Base.run = (ml_state_fn)ml_extremum_run;
	State->Values[0] = ml_cfunctionx(Extremum, (ml_callbackx_t)ml_maximum2_fn);
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

typedef struct ml_join_state_t {
	ml_state_t Base;
	const char *Separator;
	ml_value_t *Iter;
	ml_stringbuffer_t Buffer[1];
	size_t SeparatorLength;
} ml_join_state_t;

static void join_value(ml_join_state_t *State, ml_value_t *Value);

static void join_next(ml_join_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, ml_stringbuffer_get_value(State->Buffer));
	ml_stringbuffer_write(State->Buffer, State->Separator, State->SeparatorLength);
	State->Base.run = (void *)join_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void join_append(ml_join_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)join_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void join_value(ml_join_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)join_append;
	return ml_stringbuffer_append((ml_state_t *)State, State->Buffer, Value);
}

static void join_first(ml_join_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, ml_stringbuffer_get_value(State->Buffer));
	State->Base.run = (void *)join_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_METHODX("join", MLSequenceT, MLStringT) {
//<Sequence
//<Separator
//>string
// Joins the elements of :mini:`Sequence` into a string using :mini:`Separator` between elements.
//$= (1 .. 10):join
	ml_join_state_t *State = new(ml_join_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)join_first;
	State->Separator = ml_string_value(Args[1]);
	State->SeparatorLength = ml_string_length(Args[1]);
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	return ml_iterate((ml_state_t *)State, Args[0]);
}

ML_METHODX("join", MLSequenceT) {
//<Sequence
//>string
// Joins the elements of :mini:`Sequence` into a string.
//$= 1 .. 10 join ","
	ml_join_state_t *State = new(ml_join_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)join_first;
	State->Separator = "";
	State->SeparatorLength = 0;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	return ml_iterate((ml_state_t *)State, Args[0]);
}

typedef struct ml_stacked_t {
	ml_type_t *Type;
	ml_value_t *Initial, *Value, *ReduceFn;
} ml_stacked_t;

ML_TYPE(MLStackedT, (MLSequenceT), "stacked");
//!internal

ML_TYPE(MLStackedStateT, (MLStateT), "stacked-state");
//!internal

static void stacked_iter_next(ml_iter_state_t *State, ml_value_t *Value);

static void stacked_call(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)stacked_iter_next;
	ML_CONTINUE(State->Base.Caller, State);
}

static void stacked_next_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t *ReduceFn = State->Values[0];
	State->Values[2] = Value;
	State->Base.run = (void *)stacked_call;
	return ml_call(State, ReduceFn, 2, State->Values + 1);
}

static void stacked_iter_next(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	State->Base.run = (void *)stacked_next_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void stacked_first_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)stacked_iter_next;
	ML_CONTINUE(State->Base.Caller, State);
}

static void stacked_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	if (State->Values[1]) {
		State->Base.run = (void *)stacked_next_value;
	} else {
		State->Base.run = (void *)stacked_first_value;
	}
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}


static void ML_TYPED_FN(ml_iter_key, MLStackedStateT, ml_state_t *Caller, ml_iter_state_t *State) {
	State->Base.Caller = Caller;
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLStackedStateT, ml_state_t *Caller, ml_iter_state_t *State) {
	State->Base.Caller = Caller;
	ML_RETURN(State->Values[1]);
}

static void ML_TYPED_FN(ml_iter_next, MLStackedStateT, ml_state_t *Caller, ml_iter_state_t *State) {
	State->Base.Caller = Caller;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ML_TYPED_FN(ml_iterate, MLStackedT, ml_state_t *Caller, ml_stacked_t *Stacked) {
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Type = MLStackedStateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)stacked_iterate;
	State->Values[0] = Stacked->ReduceFn;
	State->Values[1] = Stacked->Initial;
	return ml_iterate((ml_state_t *)State, Stacked->Value);
}

ML_METHOD("//", MLSequenceT, MLFunctionT) {
//<Sequence
//<Fn
//>sequence
// Returns an sequence that produces :mini:`V/1`, :mini:`Fn(V/1, V/2)`, :mini:`Fn(Fn(V/1, V/2), V/3)`, ... .
//$= list(1 .. 10 // +)
	ml_stacked_t *Stacked = new(ml_stacked_t);
	Stacked->Type = MLStackedT;
	Stacked->Value = Args[0];
	Stacked->ReduceFn = Args[1];
	return (ml_value_t *)Stacked;
}

ML_METHOD("//", MLSequenceT, MLAnyT, MLFunctionT) {
//<Sequence
//<Initial
//<Fn
//>sequence
// Returns an sequence that produces :mini:`Initial`, :mini:`Fn(Initial, V/1)`, :mini:`Fn(Fn(Initial, V/1), V/2)`, ... .
//$= list(1 .. 10 // (10, +))
	ml_stacked_t *Stacked = new(ml_stacked_t);
	Stacked->Type = MLStackedT;
	Stacked->Value = Args[0];
	Stacked->Initial = Args[1];
	Stacked->ReduceFn = Args[2];
	return (ml_value_t *)Stacked;
}

typedef struct ml_repeated_t {
	ml_type_t *Type;
	ml_value_t *Value, *Update;
} ml_repeated_t;

ML_TYPE(MLRepeatedT, (MLSequenceT), "repeated");
//!internal

typedef struct ml_repeated_state_t {
	ml_state_t Base;
	ml_value_t *Value, *Update;
	int Iteration;
} ml_repeated_state_t;

ML_TYPE(MLRepeatedStateT, (MLStateT), "repeated-state");
//!internal

static void repeated_update(ml_repeated_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Value = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iter_next, MLRepeatedStateT, ml_state_t *Caller, ml_repeated_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	++State->Iteration;
	if (State->Update) {
		return ml_call(State, State->Update, 1, &State->Value);
	} else {
		ML_RETURN(State);
	}
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
	State->Base.run = (void *)repeated_update;
	State->Value = Repeated->Value;
	State->Update = Repeated->Update;
	State->Iteration = 1;
	ML_RETURN(State);
}

ML_METHOD("@", MLAnyT) {
//<Value
//>sequence
// Returns an infinite sequence that repeatedly produces :mini:`Value`. Should be used with :mini:`:limit` or paired with a finite sequence in :mini:`zip`, :mini:`weave`, etc.
//$= list(@1 limit 10)
	ML_CHECK_ARG_COUNT(1);
	ml_repeated_t *Repeated = new(ml_repeated_t);
#ifdef ML_GENERICS
	ml_type_t *TArgs[3] = {MLRepeatedT, MLIntegerT, ml_typeof(Args[0])};
	Repeated->Type = ml_generic_type(3, TArgs);
#else
	Repeated->Type = MLRepeatedT;
#endif
	Repeated->Value = Args[0];
	return (ml_value_t *)Repeated;
}

ML_METHOD("@", MLAnyT, MLFunctionT) {
//<Initial
//<Fn
//>sequence
// Returns a sequence that produces :mini:`Initial`, :mini:`Fn(Initial)`, :mini:`Fn(Fn(Initial))`, ... stopping when :mini:`Fn(Last)` returns :mini:`nil`.
//$= list(1 @ (_ + 1) limit 10)
	ML_CHECK_ARG_COUNT(1);
	ml_repeated_t *Repeated = new(ml_repeated_t);
	Repeated->Type = MLRepeatedT;
	Repeated->Value = Args[0];
	Repeated->Update = Args[1];
	return (ml_value_t *)Repeated;
}

typedef struct ml_sequenced_t {
	ml_type_t *Type;
	ml_value_t *First, *Second;
} ml_sequenced_t;

ML_TYPE(MLSequencedT, (MLSequenceT), "sequenced");
//!internal

typedef struct ml_sequenced_state_t {
	ml_state_t Base;
	ml_value_t *Iter, *Next;
} ml_sequenced_state_t;

ML_TYPE(MLSequencedStateT, (MLStateT), "sequenced-state");
//!internal

static void ml_sequenced_fnx_iterate(ml_sequenced_state_t *State, ml_value_t *Value);

static void ML_TYPED_FN(ml_iter_next, MLSequencedStateT, ml_state_t *Caller, ml_sequenced_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_sequenced_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLSequencedStateT, ml_state_t *Caller, ml_sequenced_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLSequencedStateT, ml_state_t *Caller, ml_sequenced_state_t *State) {
	return ml_iter_value(Caller, State->Iter);
}

static void ml_sequenced_fnx_iterate(ml_sequenced_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		return ml_iterate(State->Base.Caller, State->Next);
	}
	State->Iter = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iterate, MLSequencedT, ml_state_t *Caller, ml_sequenced_t *Sequenced) {
	ml_sequenced_state_t *State = new(ml_sequenced_state_t);
	State->Base.Type = MLSequencedStateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_sequenced_fnx_iterate;
	State->Next = Sequenced->Second;
	return ml_iterate((ml_state_t *)State, Sequenced->First);
}

static ml_value_t *ml_sequenced(ml_value_t *First, ml_value_t *Second) {
	ml_sequenced_t *Sequenced = new(ml_sequenced_t);
#ifdef ML_GENERICS
	ml_type_t *TArgs[3];
	if (ml_find_generic_parent(ml_typeof(First), MLSequenceT, 3, TArgs) == 3) {
		ml_type_t *KeyType = TArgs[1], *ValueType = TArgs[2];
		if (ml_find_generic_parent(ml_typeof(Second), MLSequenceT, 3, TArgs) == 3) {
			TArgs[0] = MLSequencedT;
			TArgs[1] = ml_type_max(KeyType, TArgs[1]);
			TArgs[2] = ml_type_max(ValueType, TArgs[2]);
			Sequenced->Type = ml_generic_type(3, TArgs);
		} else {
			Sequenced->Type = MLSequencedT;
		}
	} else {
		Sequenced->Type = MLSequencedT;
	}
#else
	Sequenced->Type = MLSequencedT;
#endif
	Sequenced->First = First;
	Sequenced->Second = Second;
	return (ml_value_t *)Sequenced;
}

ML_METHOD("&", MLSequenceT, MLSequenceT) {
//<Sequence/1
//<Sequence/2
//>Sequence
// Returns an sequence that produces the values from :mini:`Sequence/1` followed by those from :mini:`Sequence/2`.
//$= list(1 .. 3 & "cake")
	return ml_sequenced(Args[0], Args[1]);
}

ML_METHOD("&", MLIntegerRangeT, MLIntegerRangeT) {
	ml_integer_range_t *Range1 = (ml_integer_range_t *)Args[0];
	ml_integer_range_t *Range2 = (ml_integer_range_t *)Args[1];
	if ((Range1->Step == Range2->Step) && (Range1->Limit + Range1->Step == Range2->Start)) {
		ml_integer_range_t *Range = new(ml_integer_range_t);
		Range->Type = MLIntegerRangeT;
		Range->Start = Range1->Start;
		Range->Limit = Range2->Limit;
		Range->Step = Range1->Step;
		return (ml_value_t *)Range;
	}
	return ml_sequenced(Args[0], Args[1]);
}

ML_METHOD("&", MLSequenceT) {
//<Sequence
//>Sequence
// Returns an sequence that repeatedly produces the values from :mini:`Sequence` (for use with :mini:`limit`).
//$= list(&(1 .. 3) limit 10)
	ml_sequenced_t *Sequenced = xnew(ml_sequenced_t, 3, ml_value_t *);
	Sequenced->Type = ml_generic_sequence(MLSequencedT, Args[0]);
	Sequenced->First = Args[0];
	Sequenced->Second = (ml_value_t *)Sequenced;
	return (ml_value_t *)Sequenced;
}

typedef struct ml_limited_t {
	ml_type_t *Type;
	ml_value_t *Value;
	int Remaining;
} ml_limited_t;

ML_TYPE(MLLimitedT, (MLSequenceT), "limited");
//!internal

typedef struct ml_limited_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	int Remaining;
} ml_limited_state_t;

ML_TYPE(MLLimitedStateT, (MLStateT), "limited-state");
//!internal

static void limited_iterate(ml_limited_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iter = Value;
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

ML_METHOD("limit", MLSequenceT, MLIntegerT) {
//<Sequence
//<Limit
//>sequence
// Returns an sequence that produces at most :mini:`Limit` values from :mini:`Sequence`.
//$= list(1 .. 10)
//$= list(1 .. 10 limit 5)
	ml_limited_t *Limited = new(ml_limited_t);
	Limited->Type = ml_generic_sequence(MLLimitedT, Args[0]);
	Limited->Value = Args[0];
	Limited->Remaining = ml_integer_value_fast(Args[1]);
	return (ml_value_t *)Limited;
}

typedef struct ml_skipped_t {
	ml_type_t *Type;
	ml_value_t *Value;
	long Remaining;
} ml_skipped_t;

ML_TYPE(MLSkippedT, (MLSequenceT), "skipped");
//!internal

typedef struct ml_skipped_state_t {
	ml_state_t Base;
	long Remaining;
} ml_skipped_state_t;

static void skipped_iterate(ml_skipped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	if (State->Remaining) {
		--State->Remaining;
		return ml_iter_next((ml_state_t *)State, Value);
	} else {
		ML_CONTINUE(State->Base.Caller, Value);
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

ML_METHOD("skip", MLSequenceT, MLIntegerT) {
//<Sequence
//<Skip
//>sequence
// Returns an sequence that skips the first :mini:`Skip` values from :mini:`Sequence` and then produces the rest.
//$= list(1 .. 10)
//$= list(1 .. 10 skip 5)
	ml_skipped_t *Skipped = new(ml_skipped_t);
	Skipped->Type = ml_generic_sequence(MLSkippedT, Args[0]);
	Skipped->Value = Args[0];
	Skipped->Remaining = ml_integer_value_fast(Args[1]);
	return (ml_value_t *)Skipped;
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value, *Fn;
	int Remaining;
} ml_provided_t;

ML_TYPE(MLProvidedT, (MLSequenceT), "until");
//!internal

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter, *Fn;
	ml_value_t *Args[1];
} ml_provided_state_t;

ML_TYPE(MLProvidedStateT, (MLStateT), "until-state");
//!internal

static void provided_check(ml_provided_state_t *State, ml_value_t *Value);

static void provided_iterate(ml_provided_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	ML_CONTINUE(State->Base.Caller, State);
}

static void provided_value(ml_provided_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)provided_iterate;
	State->Args[0] = Value;
	return ml_call(State, State->Fn, 1, State->Args);
}

static void provided_check(ml_provided_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)provided_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void ML_TYPED_FN(ml_iterate, MLProvidedT, ml_state_t *Caller, ml_provided_t *Provided) {
	ml_provided_state_t *State = new(ml_provided_state_t);
	State->Base.Type = MLProvidedStateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)provided_check;
	State->Fn = Provided->Fn;
	return ml_iterate((ml_state_t *)State, Provided->Value);
}

static void ML_TYPED_FN(ml_iter_key, MLProvidedStateT, ml_state_t *Caller, ml_provided_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLProvidedStateT, ml_state_t *Caller, ml_provided_state_t *State) {
	ML_RETURN(State->Args[0]);
}

static void ML_TYPED_FN(ml_iter_next, MLProvidedStateT, ml_state_t *Caller, ml_provided_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)provided_check;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_METHOD("provided", MLSequenceT, MLFunctionT) {
//<Sequence
//<Fn
//>sequence
// Returns an sequence that stops when :mini:`Fn(Value)` is :mini:`nil`.
//$= list("banana")
//$= list("banana" provided (_ != "n"))
	ml_provided_t *Provided = new(ml_provided_t);
	Provided->Type = ml_generic_sequence(MLProvidedT, Args[0]);
	Provided->Value = Args[0];
	Provided->Fn = Args[1];
	return (ml_value_t *)Provided;
}

typedef struct ml_unique_t {
	ml_type_t *Type;
	ml_value_t *Iter;
} ml_unique_t;

ML_TYPE(MLUniqueT, (MLSequenceT), "unique");
//!internal

typedef struct ml_unique_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *History;
	ml_value_t *Value;
	int Iteration;
} ml_unique_state_t;

ML_TYPE(MLUniqueStateT, (MLStateT), "unique-state");
//!internal

static void ml_unique_fnx_iterate(ml_unique_state_t *State, ml_value_t *Value);

static void ml_unique_fnx_value(ml_unique_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (ml_map_insert(State->History, Value, MLSome) == MLNil) {
		State->Value = Value;
		++State->Iteration;
		ML_CONTINUE(State->Base.Caller, State);
	}
	State->Base.run = (void *)ml_unique_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ml_unique_fnx_iterate(ml_unique_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)ml_unique_fnx_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
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
//<Sequence:sequence
//>sequence
// Returns an sequence that returns the unique values produced by :mini:`Sequence`. Uniqueness is determined by using a :mini:`map`.
//$= list(unique("banana"))
	ML_CHECK_ARG_COUNT(1);
	ml_unique_t *Unique = new(ml_unique_t);
	ml_value_t *Iter = Unique->Iter = ml_chained(Count, Args);
	Unique->Type = ml_generic_sequence(MLUniqueT, Iter);
	return (ml_value_t *)Unique;
}

typedef struct ml_zipped_t {
	ml_type_t *Type;
	ml_value_t *Function;
	int Count;
	ml_value_t *Iters[];
} ml_zipped_t;

ML_TYPE(MLZippedT, (MLSequenceT), "zipped");
//!internal

typedef struct ml_zipped_state_t {
	ml_state_t Base;
	ml_value_t *Function;
	ml_value_t **Iters;
	int Count, Index, Iteration;
	ml_value_t *Args[];
} ml_zipped_state_t;

ML_TYPE(MLZippedStateT, (MLStateT), "zipped-state");
//!internal

static void zipped_iterate(ml_zipped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iters[State->Index] = Value;
	if (++State->Index ==  State->Count) ML_CONTINUE(State->Base.Caller, State);
	return ml_iterate((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iterate, MLZippedT, ml_state_t *Caller, ml_zipped_t *Zipped) {
	ml_zipped_state_t *State = xnew(ml_zipped_state_t, 2 * Zipped->Count, ml_value_t *);
	State->Base.Type = MLZippedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)zipped_iterate;
	State->Base.Context = Caller->Context;
	State->Function = Zipped->Function;
	State->Iters = State->Args + Zipped->Count;
	for (int I = 0; I < Zipped->Count; ++I) State->Iters[I] = Zipped->Iters[I];
	State->Count = Zipped->Count;
	State->Iteration = 1;
	return ml_iterate((ml_state_t *)State, State->Iters[0]);
}

static void ML_TYPED_FN(ml_iter_key, MLZippedStateT, ml_state_t *Caller, ml_zipped_state_t *State) {
	ML_RETURN(ml_integer(State->Iteration));
}

static void ml_zipped_fnx_value(ml_zipped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[State->Index] = Value;
	if (++State->Index ==  State->Count) {
		return ml_call(State->Base.Caller, State->Function, State->Count, State->Args);
	}
	return ml_iter_value((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iter_value, MLZippedStateT, ml_state_t *Caller, ml_zipped_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_zipped_fnx_value;
	State->Index = 0;
	return ml_iter_value((ml_state_t *)State, State->Iters[0]);
}

static void zipped_iter_next(ml_zipped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iters[State->Index] = Value;
	if (++State->Index ==  State->Count) ML_CONTINUE(State->Base.Caller, State);
	return ml_iter_next((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iter_next, MLZippedStateT, ml_state_t *Caller, ml_zipped_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)zipped_iter_next;
	++State->Iteration;
	State->Index = 0;
	return ml_iter_next((ml_state_t *)State, State->Iters[0]);
}

ML_FUNCTION(Zip) {
//@zip
//<Sequence/1,...,Sequence/n:sequence
//<Function
//>sequence
// Returns a new sequence that produces :mini:`Function(V/1/1, ..., V/n/1), Function(V/1/2, ..., V/n/2), ...` where :mini:`V/i/j` is the :mini:`j`-th value produced by :mini:`Sequence/i`.
// The sequence stops produces values when any of the :mini:`Sequence/i` stops.
//$= list(zip(1 .. 3, "cake", tuple))
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(Count - 1, MLFunctionT);
	ml_zipped_t *Zipped = xnew(ml_zipped_t, Count - 1, ml_value_t *);
	Zipped->Type = MLZippedT;
	Zipped->Count = Count - 1;
	Zipped->Function = Args[Count - 1];
	for (int I = 0; I < Count - 1; ++I) Zipped->Iters[I] = Args[I];
	return (ml_value_t *)Zipped;
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *ValueFn, *KeyFn;
	int Count;
	ml_value_t *Iters[];
} ml_zipped2_t;

ML_TYPE(MLZipped2T, (MLSequenceT), "zipped2");
//!internal

typedef struct {
	ml_state_t Base;
	ml_value_t *ValueFn, *KeyFn;
	ml_value_t **Iters;
	int Count, Index, Iteration;
	ml_value_t *Args[];
} ml_zipped2_state_t;

ML_TYPE(MLZipped2StateT, (MLStateT), "zipped2-state");
//!internal

static void zipped2_iterate(ml_zipped2_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iters[State->Index] = Value;
	if (++State->Index ==  State->Count) ML_CONTINUE(State->Base.Caller, State);
	return ml_iterate((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iterate, MLZipped2T, ml_state_t *Caller, ml_zipped2_t *Zipped) {
	ml_zipped2_state_t *State = xnew(ml_zipped2_state_t, 2 * Zipped->Count, ml_value_t *);
	State->Base.Type = MLZipped2StateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)zipped2_iterate;
	State->Base.Context = Caller->Context;
	State->ValueFn = Zipped->ValueFn;
	State->KeyFn = Zipped->KeyFn;
	State->Iters = State->Args + Zipped->Count;
	for (int I = 0; I < Zipped->Count; ++I) State->Iters[I] = Zipped->Iters[I];
	State->Count = Zipped->Count;
	State->Iteration = 1;
	return ml_iterate((ml_state_t *)State, State->Iters[0]);
}

static void ml_zipped2_fnx_key(ml_zipped2_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[State->Index] = Value;
	if (++State->Index ==  State->Count) {
		return ml_call(State->Base.Caller, State->KeyFn, State->Count, State->Args);
	}
	return ml_iter_key((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iter_key, MLZipped2StateT, ml_state_t *Caller, ml_zipped2_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_zipped2_fnx_key;
	State->Index = 0;
	return ml_iter_key((ml_state_t *)State, State->Iters[0]);
}

static void ml_zipped2_fnx_value(ml_zipped2_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[State->Index] = Value;
	if (++State->Index ==  State->Count) {
		return ml_call(State->Base.Caller, State->ValueFn, State->Count, State->Args);
	}
	return ml_iter_value((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iter_value, MLZipped2StateT, ml_state_t *Caller, ml_zipped2_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_zipped2_fnx_value;
	State->Index = 0;
	return ml_iter_value((ml_state_t *)State, State->Iters[0]);
}

static void zipped2_iter_next(ml_zipped2_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iters[State->Index] = Value;
	if (++State->Index ==  State->Count) ML_CONTINUE(State->Base.Caller, State);
	return ml_iter_next((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iter_next, MLZipped2StateT, ml_state_t *Caller, ml_zipped2_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)zipped2_iter_next;
	++State->Iteration;
	State->Index = 0;
	return ml_iter_next((ml_state_t *)State, State->Iters[0]);
}

ML_FUNCTION(Zip2) {
//@zip2
//<Sequence/1,...,Sequence/n:sequence
//<KeyFn
//<ValueFn
//>sequence
// Returns a new sequence that produces :mini:`KeyFn(K/1/1, ..., K/n/1) - ValueFn(V/1/1, ..., V/n/1), ...` where :mini:`K/i/j - V/i/j` are the :mini:`j`-th key and value produced by :mini:`Sequence/i`.
// The sequence stops produces values when any of the :mini:`Sequence/i` stops.
//$= map(zip2(1 .. 3, "cake", tuple, tuple))
	ML_CHECK_ARG_COUNT(3);
	ML_CHECK_ARG_TYPE(Count - 2, MLFunctionT);
	ML_CHECK_ARG_TYPE(Count - 1, MLFunctionT);
	ml_zipped2_t *Zipped = xnew(ml_zipped2_t, Count - 2, ml_value_t *);
	Zipped->Type = MLZipped2T;
	Zipped->Count = Count - 2;
	Zipped->ValueFn = Args[Count - 1];
	Zipped->KeyFn = Args[Count - 2];
	for (int I = 0; I < Count - 2; ++I) Zipped->Iters[I] = Args[I];
	return (ml_value_t *)Zipped;
}

typedef struct ml_grid_t {
	ml_type_t *Type;
	ml_value_t *Function;
	int Count;
	ml_value_t *Values[];
} ml_grid_t;

ML_TYPE(MLGridT, (MLSequenceT), "grid");
//!internal

typedef struct ml_grid_state_t {
	ml_state_t Base;
	ml_value_t *Function;
	ml_value_t **Values, **Iters;
	int Count, Index, Iteration;
	ml_value_t *Args[];
} ml_grid_state_t;

ML_TYPE(MLGridStateT, (MLStateT), "grid-state");
//!internal

static void grid_iterate(ml_grid_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	int Index = State->Index;
	if (Value == MLNil) {
		if (--Index < 0) ML_CONTINUE(State->Base.Caller, Value);
		State->Index = Index;
		return ml_iter_next((ml_state_t *)State, State->Iters[Index]);
	}
	State->Iters[Index] = Value;
	if (++Index ==  State->Count) ML_CONTINUE(State->Base.Caller, State);
	State->Index = Index;
	return ml_iterate((ml_state_t *)State, State->Values[Index]);
}

static void ML_TYPED_FN(ml_iterate, MLGridT, ml_state_t *Caller, ml_grid_t *Grid) {
	ml_grid_state_t *State = xnew(ml_grid_state_t, 2 * Grid->Count, ml_value_t *);
	State->Base.Type = MLGridStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)grid_iterate;
	State->Base.Context = Caller->Context;
	State->Function = Grid->Function;
	State->Iters = State->Args + Grid->Count;
	State->Values = Grid->Values;
	State->Count = Grid->Count;
	State->Index = 0;
	State->Iteration = 1;
	return ml_iterate((ml_state_t *)State, State->Values[0]);
}

static void ML_TYPED_FN(ml_iter_key, MLGridStateT, ml_state_t *Caller, ml_grid_state_t *State) {
	ML_RETURN(ml_integer(State->Iteration));
}

static void ml_grid_fnx_value(ml_grid_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[State->Index] = Value;
	if (++State->Index ==  State->Count) {
		return ml_call(State->Base.Caller, State->Function, State->Count, State->Args);
	}
	return ml_iter_value((ml_state_t *)State, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iter_value, MLGridStateT, ml_state_t *Caller, ml_grid_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_grid_fnx_value;
	State->Index = 0;
	return ml_iter_value((ml_state_t *)State, State->Iters[0]);
}

static void ML_TYPED_FN(ml_iter_next, MLGridStateT, ml_state_t *Caller, ml_grid_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)grid_iterate;
	++State->Iteration;
	int Index = State->Index = State->Count - 1;
	return ml_iter_next((ml_state_t *)State, State->Iters[Index]);
}

ML_FUNCTION(Grid) {
//@grid
//<Sequence/1,...,Sequence/n:sequence
//<Function
//>sequence
// Returns a new sequence that produces :mini:`Function(V/1, V/2, ..., V/n)` for all possible combinations of :mini:`V/1, ..., V/n`, where :mini:`V/i` are the values produced by :mini:`Sequence/i`.
//$= list(grid(1 .. 3, "cake", [true, false], tuple))
//$= list(grid(1 .. 3, "cake", *))
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(Count - 1, MLFunctionT);
	ml_grid_t *Grid = xnew(ml_grid_t, Count - 1, ml_value_t *);
	Grid->Type = MLGridT;
	Grid->Count = Count - 1;
	Grid->Function = Args[Count - 1];
	for (int I = 0; I < Count - 1; ++I) Grid->Values[I] = Args[I];
	return (ml_value_t *)Grid;
}

typedef struct ml_paired_t {
	ml_type_t *Type;
	ml_value_t *Keys, *Values;
} ml_paired_t;

ML_TYPE(MLPairedT, (MLSequenceT), "paired");
//!internal

typedef struct ml_paired_state_t {
	ml_state_t Base;
	ml_value_t *Keys, *Values;
} ml_paired_state_t;

ML_TYPE(MLPairedStateT, (MLStateT), "paired-state");
//!internal

static void paired_value_iterate(ml_paired_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Values = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void paired_key_iterate(ml_paired_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Keys = Value;
	State->Base.run = (void *)paired_value_iterate;
	return ml_iterate((ml_state_t *)State, State->Values);
}

static void ML_TYPED_FN(ml_iterate, MLPairedT, ml_state_t *Caller, ml_paired_t *Paired) {
	ml_paired_state_t *State = new(ml_paired_state_t);
	State->Base.Type = MLPairedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)paired_key_iterate;
	State->Base.Context = Caller->Context;
	State->Values = Paired->Values;
	return ml_iterate((ml_state_t *)State, Paired->Keys);
}

static void ML_TYPED_FN(ml_iter_key, MLPairedStateT, ml_state_t *Caller, ml_paired_state_t *State) {
	return ml_iter_value(Caller, State->Keys);
}

static void ML_TYPED_FN(ml_iter_value, MLPairedStateT, ml_state_t *Caller, ml_paired_state_t *State) {
	return ml_iter_value(Caller, State->Values);
}

static void paired_value_iter_next(ml_paired_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Values = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void paired_key_iter_next(ml_paired_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Keys = Value;
	State->Base.run = (void *)paired_value_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Values);
}

static void ML_TYPED_FN(ml_iter_next, MLPairedStateT, ml_state_t *Caller, ml_paired_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)paired_key_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Keys);
}

ML_FUNCTION(Pair) {
//@pair
//<Sequence/1:sequence
//<Sequence/2:sequence
//>sequence
// Returns a new sequence that produces the values from :mini:`Sequence/1` as keys and the values from :mini:`Sequence/2` as values.
	ML_CHECK_ARG_COUNT(2);
	ml_paired_t *Paired = new(ml_paired_t);
#ifdef ML_GENERICS
	ml_type_t *TArgs0[3];
	if (ml_find_generic_parent(ml_typeof(Args[0]), MLSequenceT, 3, TArgs0) == 3) {
		ml_type_t *TArgs1[3];
		if (ml_find_generic_parent(ml_typeof(Args[1]), MLSequenceT, 3, TArgs1) == 3) {
			TArgs1[0] = MLPairedT;
			TArgs1[1] = TArgs0[2];
			Paired->Type = ml_generic_type(3, TArgs1);
		} else {
			Paired->Type = MLPairedT;
		}
	} else {
		Paired->Type = MLPairedT;
	}
#else
	Paired->Type = MLPairedT;
#endif
	Paired->Keys = Args[0];
	Paired->Values = Args[1];
	return (ml_value_t *)Paired;
}

typedef struct ml_weaved_t {
	ml_type_t *Type;
	int Count;
	ml_value_t *Iters[];
} ml_weaved_t;

ML_TYPE(MLWeavedT, (MLSequenceT), "weaved");
//!internal

typedef struct ml_weaved_state_t {
	ml_state_t Base;
	int Count, Index, Iteration;
	ml_value_t *Iters[];
} ml_weaved_state_t;

ML_TYPE(MLWeavedStateT, (MLStateT), "weaved-state");
//!internal

static void weaved_iterate(ml_weaved_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iters[State->Index] = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iterate, MLWeavedT, ml_state_t *Caller, ml_weaved_t *Weaved) {
	ml_weaved_state_t *State = xnew(ml_weaved_state_t, Weaved->Count, ml_value_t *);
	State->Base.Type = MLWeavedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)weaved_iterate;
	State->Base.Context = Caller->Context;
	for (int I = 0; I < Weaved->Count; ++I) State->Iters[I] = Weaved->Iters[I];
	State->Count = Weaved->Count;
	State->Iteration = 1;
	return ml_iterate((ml_state_t *)State, State->Iters[0]);
}

static void ML_TYPED_FN(ml_iter_key, MLWeavedStateT, ml_state_t *Caller, ml_weaved_state_t *State) {
	ML_RETURN(ml_integer(State->Iteration));
}

static void ML_TYPED_FN(ml_iter_value, MLWeavedStateT, ml_state_t *Caller, ml_weaved_state_t *State) {
	return ml_iter_value(Caller, State->Iters[State->Index]);
}

static void ML_TYPED_FN(ml_iter_next, MLWeavedStateT, ml_state_t *Caller, ml_weaved_state_t *State) {
	State->Base.Caller = Caller;
	if (++State->Index == State->Count) State->Index = 0;
	if (++State->Iteration > State->Count) {
		return ml_iter_next((ml_state_t *)State, State->Iters[State->Index]);
	} else {
		return ml_iterate((ml_state_t *)State, State->Iters[State->Index]);
	}
}

ML_FUNCTION(Weave) {
//@weave
//<Sequence/1,...,Sequence/n:sequence
//>sequence
// Returns a new sequence that produces interleaved values :mini:`V/i` from each of :mini:`Sequence/i`.
// The sequence stops produces values when any of the :mini:`Sequence/i` stops.
//$= list(weave(1 .. 3, "cake"))
	ML_CHECK_ARG_COUNT(1);
	ml_weaved_t *Weaved = xnew(ml_weaved_t, Count, ml_value_t *);
	Weaved->Type = MLWeavedT;
	Weaved->Count = Count;
	for (int I = 0; I < Count; ++I) Weaved->Iters[I] = Args[I];
	return (ml_value_t *)Weaved;
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Iter;
} ml_unpacked_t;

ML_TYPE(MLUnpackedT, (MLSequenceT), "unpacked");
//!internal

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Value;
} ml_unpacked_state_t;

ML_TYPE(MLUnpackedStateT, (MLStateT), "unpacked-state");
//!internal

static void unpacked_value(ml_unpacked_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Value = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void unpacked_iterate(ml_unpacked_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)unpacked_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void ML_TYPED_FN(ml_iterate, MLUnpackedT, ml_state_t *Caller, ml_unpacked_t *Folded) {
	ml_unpacked_state_t *State = new(ml_unpacked_state_t);
	State->Base.Type = MLUnpackedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)unpacked_iterate;
	State->Base.Context = Caller->Context;
	return ml_iterate((ml_state_t *)State, Folded->Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLUnpackedStateT, ml_state_t *Caller, ml_unpacked_state_t *State) {
	ML_RETURN(ml_unpack(State->Value, 1));
}

static void ML_TYPED_FN(ml_iter_value, MLUnpackedStateT, ml_state_t *Caller, ml_unpacked_state_t *State) {
	ML_RETURN(ml_unpack(State->Value, 2));
}

static void ML_TYPED_FN(ml_iter_next, MLUnpackedStateT, ml_state_t *Caller, ml_unpacked_state_t *State) {
	State->Base.run = (void *)unpacked_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_FUNCTION(Unpack) {
//@unpack
//<Sequence:sequence
//>sequence
// Returns a new sequence unpacks each value generated by :mini:`Sequence` as keys and values respectively.
//$- let L := [("A", "a"), ("B", "b"), ("C", "c")]
//$= map(unpack(L))
	ML_CHECK_ARG_COUNT(1);
	ml_unpacked_t *Unpacked = new(ml_unpacked_t);
	Unpacked->Type = MLUnpackedT;
	Unpacked->Iter = ml_chained(Count, Args);
	return (ml_value_t *)Unpacked;
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Iter;
} ml_folded_t;

ML_TYPE(MLFoldedT, (MLSequenceT), "folded");
//!internal

typedef struct ml_folded_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Key;
	ml_value_t *Value;
} ml_folded_state_t;

ML_TYPE(MLFoldedStateT, (MLStateT), "folded-state");
//!internal

static void folded_value(ml_folded_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Value = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void folded_iterate_value(ml_folded_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)folded_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void folded_key(ml_folded_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Key = Value;
	State->Base.run = (void *)folded_iterate_value;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void folded_iterate_key(ml_folded_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)folded_key;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void ML_TYPED_FN(ml_iterate, MLFoldedT, ml_state_t *Caller, ml_folded_t *Folded) {
	ml_folded_state_t *State = new(ml_folded_state_t);
	State->Base.Type = MLFoldedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)folded_iterate_key;
	State->Base.Context = Caller->Context;
	return ml_iterate((ml_state_t *)State, Folded->Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLFoldedStateT, ml_state_t *Caller, ml_folded_state_t *State) {
	ML_RETURN(State->Key);
}

static void ML_TYPED_FN(ml_iter_value, MLFoldedStateT, ml_state_t *Caller, ml_folded_state_t *State) {
	ML_RETURN(State->Value);
}

static void ML_TYPED_FN(ml_iter_next, MLFoldedStateT, ml_state_t *Caller, ml_folded_state_t *State) {
	State->Base.run = (void *)folded_iterate_key;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_FUNCTION(Fold) {
//@fold
//<Sequence:sequence
//>sequence
// Returns a new sequence that treats alternating values produced by :mini:`Sequence` as keys and values respectively.
//$= map(fold(1 .. 10))
	ML_CHECK_ARG_COUNT(1);
	ml_folded_t *Folded = new(ml_folded_t);
	Folded->Type = MLFoldedT;
	Folded->Iter = ml_chained(Count, Args);
	return (ml_value_t *)Folded;
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Iter;
} ml_unfolded_t;

ML_TYPE(MLUnfoldedT, (MLSequenceT), "unfolded");
//!internal

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
	int Index;
} ml_unfolded_state_t;

ML_TYPE(MLUnfoldedStateT, (MLStateT), "unfolded-state");
//!internal

static void unfolded_iterate(ml_unfolded_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iter = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iterate, MLUnfoldedT, ml_state_t *Caller, ml_unfolded_t *Unfolded) {
	ml_unfolded_state_t *State = new(ml_unfolded_state_t);
	State->Base.Type = MLUnfoldedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)unfolded_iterate;
	State->Base.Context = Caller->Context;
	++State->Index;
	return ml_iterate((ml_state_t *)State, Unfolded->Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLUnfoldedStateT, ml_state_t *Caller, ml_unfolded_state_t *State) {
	ML_RETURN(ml_integer(State->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLUnfoldedStateT, ml_state_t *Caller, ml_unfolded_state_t *State) {
	if (State->Index % 2) {
		return ml_iter_key(Caller, State->Iter);
	} else {
		return ml_iter_value(Caller, State->Iter);
	}
}

static void ML_TYPED_FN(ml_iter_next, MLUnfoldedStateT, ml_state_t *Caller, ml_unfolded_state_t *State) {
	int Index = ++State->Index;
	if (Index % 2) {
		return ml_iter_next((ml_state_t *)State, State->Iter);
	} else {
		ML_RETURN(State);
	}
}

ML_FUNCTION(Unfold) {
//@unfold
//<Sequence:sequence
//>sequence
// Returns a new sequence that treats produces alternatively the keys and values produced by :mini:`Sequence`.
//$= list(unfold("cake"))
	ML_CHECK_ARG_COUNT(1);
	ml_unfolded_t *Unfolded = new(ml_unfolded_t);
	Unfolded->Type = MLUnfoldedT;
	Unfolded->Iter = ml_chained(Count, Args);
	return (ml_value_t *)Unfolded;
}


typedef struct {
	ml_type_t *Type;
	ml_value_t *Value;
} ml_swapped_t;

ML_TYPE(MLSwappedT, (MLSequenceT), "swapped");
//!internal

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
} ml_swapped_state_t;

ML_TYPE(MLSwappedStateT, (MLStateT), "swapped-state");
//!internal

static void swapped_iterate(ml_swapped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iter = Value;
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

ML_FUNCTION(Swap) {
//@swap
//<Sequence:sequence
// Returns a new sequence which swaps the keys and values produced by :mini:`Sequence`.
//$= map(swap("cake"))
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLSequenceT);
	ml_swapped_t *Swapped = new(ml_swapped_t);
#ifdef ML_GENERICS
	ml_type_t *TArgs[3];
	if (ml_find_generic_parent(ml_typeof(Args[0]), MLSequenceT, 3, TArgs) == 3) {
		TArgs[0] = MLSwappedT;
		ml_type_t *KeyType = TArgs[1];
		TArgs[1] = TArgs[2];
		TArgs[2] = KeyType;
		Swapped->Type = ml_generic_type(3, TArgs);
	} else {
		Swapped->Type = MLSwappedT;
	}
#else
	Swapped->Type = MLSwappedT;
#endif
	Swapped->Value = Args[0];
	return (ml_value_t *)Swapped;
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value;
} ml_key_t;

ML_TYPE(MLKeyT, (MLSequenceT), "key");
//!internal

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
	int Iteration;
} ml_key_state_t;

ML_TYPE(MLKeyStateT, (MLStateT), "keys-state");
//!internal

static void key_iterate(ml_key_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iter = Value;
	++State->Iteration;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iterate, MLKeyT, ml_state_t *Caller, ml_key_t *Key) {
	ml_key_state_t *State = new(ml_key_state_t);
	State->Base.Caller = Caller;
	State->Base.Type = MLKeyStateT;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)key_iterate;
	return ml_iterate((ml_state_t *)State, Key->Value);
}

static void ML_TYPED_FN(ml_iter_key, MLKeyStateT, ml_state_t *Caller, ml_key_state_t *State) {
	ML_RETURN(ml_integer(State->Iteration));
}

static void ML_TYPED_FN(ml_iter_value, MLKeyStateT, ml_state_t *Caller, ml_key_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLKeyStateT, ml_state_t *Caller, ml_key_state_t *State) {
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_FUNCTION(Key) {
//@key
//<Sequence:sequence
// Returns a new sequence which produces the keys of :mini:`Sequence`.
//$= list(key({"A" is 1, "B" is 2, "C" is 3}))
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLSequenceT);
	ml_key_t *Key = new(ml_key_t);
	Key->Type = MLKeyT;
	Key->Value = Args[0];
	return (ml_value_t *)Key;
}

typedef struct ml_batched_t {
	ml_type_t *Type;
	ml_value_t *Iter, *Function;
	int Size, Shift;
} ml_batched_t;

ML_TYPE(MLBatchedT, (MLSequenceT), "batched");
//!internal

typedef struct ml_batched_state_t {
	ml_state_t Base;
	ml_value_t *Iter, *Function, *Value;
	int Size, Shift, Index, Iteration;
	ml_value_t *Args[];
} ml_batched_state_t;

ML_TYPE(MLBatchedStateT, (MLStateT), "batched-state");
//!internal

static void batched_iterate(ml_batched_state_t *State, ml_value_t *Value);

static void batched_iter_value(ml_batched_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[State->Index] = Value;
	if (++State->Index == State->Size) {
		++State->Iteration;
		ml_state_t *Caller = State->Base.Caller;
		ML_RETURN(State);
	} else {
		State->Base.run = (void *)batched_iterate;
		return ml_iter_next((ml_state_t *)State, State->Iter);
	}
}

static void batched_iterate(ml_batched_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		State->Iter = NULL;
		if (State->Index > (State->Size - State->Shift)) {
			++State->Iteration;
			ML_CONTINUE(State->Base.Caller, State);
		}
		ML_CONTINUE(State->Base.Caller, MLNil);
	}
	State->Base.run = (void *)batched_iter_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void ML_TYPED_FN(ml_iterate, MLBatchedT, ml_state_t *Caller, ml_batched_t *Batched) {
	ml_batched_state_t *State = xnew(ml_batched_state_t, Batched->Size, ml_value_t *);
	State->Base.Type = MLBatchedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)batched_iterate;
	State->Base.Context = Caller->Context;
	State->Function = Batched->Function;
	State->Size = Batched->Size;
	State->Shift = Batched->Shift;
	State->Iteration = 0;
	return ml_iterate((ml_state_t *)State, Batched->Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLBatchedStateT, ml_state_t *Caller, ml_batched_state_t *State) {
	ML_RETURN(ml_integer(State->Iteration));
}

static void ML_TYPED_FN(ml_iter_value, MLBatchedStateT, ml_state_t *Caller, ml_batched_state_t *State) {
	return ml_call(Caller, State->Function, State->Index, State->Args);
}

static void ML_TYPED_FN(ml_iter_next, MLBatchedStateT, ml_state_t *Caller, ml_batched_state_t *State) {
	State->Base.Caller = Caller;
	if (!State->Iter) ML_RETURN(MLNil);
	int Size = State->Size, Shift = State->Shift;
	if (Shift < Size) {
		for (int I = Shift; I < Size; ++I) State->Args[I - Shift] = State->Args[I];
		State->Index = Size - Shift;
	} else {
		State->Index = 0;
	}
	State->Base.run = (void *)batched_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_FUNCTION(Batch) {
//@batch
//<Sequence:sequence
//<Size:integer
//<Shift?:integer
//<Function:function
//>sequence
// Returns a new sequence that calls :mini:`Function` with each batch of :mini:`Size` values produced by :mini:`Sequence` and produces the results. If a :mini:`Shift` is provided then :mini:`Size - Shift` values of each batch come from the previous batch.
//$= list(batch(1 .. 20, 4, tuple))
//$= list(batch(1 .. 20, 4, 2, tuple))
	ML_CHECK_ARG_COUNT(3);
	ml_batched_t *Batched = new(ml_batched_t);
	Batched->Type = MLBatchedT;
	Batched->Iter = Args[0];
	Batched->Size = ml_integer_value(Args[1]);
	if (Count > 3) {
		ML_CHECK_ARG_TYPE(2, MLIntegerT);
		Batched->Shift = ml_integer_value(Args[2]);
		if (Batched->Shift <= 0 || Batched->Shift > Batched->Size) {
			return ml_error("RangeError", "Invalid shift value");
		}
		Batched->Function = Args[3];
	} else {
		Batched->Shift = Batched->Size;
		Batched->Function = Args[2];
	}
	return (ml_value_t *)Batched;
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
} ml_iterator_t;

ML_TYPE(MLIteratorT, (), "iterator");
//@iterator
// An iterator.

static void ml_iterator_run(ml_iterator_t *Iterator, ml_value_t *Iter) {
	ml_state_t *Caller = Iterator->Base.Caller;
	Iterator->Iter = Iter;
	if (Iter == MLNil) ML_RETURN(Iter);
	if (ml_is_error(Iter)) ML_RETURN(Iter);
	ML_RETURN(Iterator);
}

ML_FUNCTIONX(MLIterate) {
//@iterate
//<Sequence:sequence
//>iterator|nil
// Create an iterator for :mini:`Sequence`. Returns :mini:`nil` is :mini:`Sequence` is empty.
	ML_CHECKX_ARG_COUNT(1);
	ml_iterator_t *Iterator = new(ml_iterator_t);
	Iterator->Base.Type = MLIteratorT;
	Iterator->Base.Caller = Caller;
	Iterator->Base.Context = Caller->Context;
	Iterator->Base.run = (ml_state_fn)ml_iterator_run;
	return ml_iterate((ml_state_t *)Iterator, Args[0]);
}

ML_METHODX("next", MLIteratorT) {
//<Iterator
//>iterator|nil
// Advances :mini:`Iterator`, returning :mini:`nil` if it is finished.
	ml_iterator_t *Iterator = (ml_iterator_t *)Args[0];
	Iterator->Base.Caller = Caller;
	Iterator->Base.Context = Caller->Context;
	return ml_iter_next((ml_state_t *)Iterator, Iterator->Iter);
}

ML_METHODX("key", MLIteratorT) {
//<Iterator
//>any
// Returns the current key produced by :mini:`Iterator`.
	ml_iterator_t *Iterator = (ml_iterator_t *)Args[0];
	return ml_iter_key(Caller, Iterator->Iter);
}

ML_METHODX("value", MLIteratorT) {
//<Iterator
//>any
// Returns the current value produced by :mini:`Iterator`.
	ml_iterator_t *Iterator = (ml_iterator_t *)Args[0];
	return ml_iter_value(Caller, Iterator->Iter);
}

void ml_sequence_init(stringmap_t *Globals) {
	MLSomeT->call = ml_some_call;
	MLFunctionT->Constructor = (ml_value_t *)MLChained;
	MLSequenceT->Constructor = (ml_value_t *)MLChained;
#include "ml_sequence_init.c"
#ifdef ML_GENERICS
	ml_type_add_rule(MLChainedT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
	ml_type_add_rule(MLDoubledT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
	ml_type_add_rule(MLRepeatedT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
	ml_type_add_rule(MLSequencedT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
	ml_type_add_rule(MLLimitedT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
	ml_type_add_rule(MLProvidedT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
	ml_type_add_rule(MLSkippedT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
	ml_type_add_rule(MLSwappedT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
	ml_type_add_rule(MLUniqueT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
	ml_type_add_rule(MLPairedT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
#endif
	if (Globals) {
		stringmap_insert(Globals, "chained", MLChainedT);
		stringmap_insert(Globals, "first", ml_method("first"));
		stringmap_insert(Globals, "first2", ml_method("first2"));
		stringmap_insert(Globals, "last", ml_method("last"));
		stringmap_insert(Globals, "last2", ml_method("last2"));
		stringmap_insert(Globals, "all", All);
		stringmap_insert(Globals, "iterate", MLIterate);
		stringmap_insert(Globals, "count", ml_method("count"));
		stringmap_insert(Globals, "count2", Count2);
		stringmap_insert(Globals, "random", ml_method("random"));
		stringmap_insert(Globals, "reduce", Reduce);
		stringmap_insert(Globals, "min", Min);
		stringmap_insert(Globals, "max", Max);
		stringmap_insert(Globals, "sum", Sum);
		stringmap_insert(Globals, "prod", Prod);
		stringmap_insert(Globals, "reduce2", Reduce2);
		stringmap_insert(Globals, "min2", Min2);
		stringmap_insert(Globals, "max2", Max2);
		stringmap_insert(Globals, "unique", Unique);
		stringmap_insert(Globals, "zip", Zip);
		stringmap_insert(Globals, "zip2", Zip2);
		stringmap_insert(Globals, "grid", Grid);
		stringmap_insert(Globals, "pair", Pair);
		stringmap_insert(Globals, "weave", Weave);
		stringmap_insert(Globals, "unpack", Unpack);
		stringmap_insert(Globals, "fold", Fold);
		stringmap_insert(Globals, "unfold", Unfold);
		stringmap_insert(Globals, "swap", Swap);
		stringmap_insert(Globals, "key", Key);
		stringmap_insert(Globals, "batch", Batch);
	}
}

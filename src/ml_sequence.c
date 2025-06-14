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
static ML_METHOD_DECL(WhileSoloMethod, "->|");
static ML_METHOD_DECL(WhileDuoMethod, "=>|");
static ML_METHOD_DECL(CountMethod, "count");
static ML_METHOD_DECL(PrecountMethod, "precount");

typedef struct {
	ml_state_t Base;
	ml_value_t **Current;
	int Count, Index;
	ml_value_t *Values[];
} ml_chained_state_t;

static void ml_chained_state_value(ml_chained_state_t *State, ml_value_t *Value);

static void ml_chained_state_filter(ml_chained_state_t *State, ml_value_t *Value) {
	ml_value_t *Deref = ml_deref(Value);
	if (ml_is_error(Deref)) ML_CONTINUE(State->Base.Caller, Deref);
	if (Deref == MLNil) ML_CONTINUE(State->Base.Caller, Deref);
	ml_value_t **Entry = State->Current;
	if (!Entry[0]) ML_CONTINUE(State->Base.Caller, State->Values[0]);
	State->Current = Entry + 1;
	ml_value_t *Function = Entry[0];
	if (Function == FilterSoloMethod) {
		Function = Entry[1];
		if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
		State->Current = Entry + 2;
		//State->Base.run = (void *)ml_chained_state_filter;
	} else {
		State->Base.run = (void *)ml_chained_state_value;
	}
	return ml_call(State, Function, 1, State->Values);
}

static void ml_chained_state_value(ml_chained_state_t *State, ml_value_t *Value) {
	ml_value_t *Deref = ml_deref(Value);
	if (ml_is_error(Deref)) ML_CONTINUE(State->Base.Caller, Deref);
	ml_value_t **Entry = State->Current;
	if (!Entry[0]) ML_CONTINUE(State->Base.Caller, Value);
	State->Current = Entry + 1;
	State->Values[0] = Value;
	ml_value_t *Function = Entry[0];
	if (Function == FilterSoloMethod) {
		Function = Entry[1];
		if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
		State->Current = Entry + 2;
		State->Base.run = (void *)ml_chained_state_filter;
	} else {
		//State->Base.run = (void *)ml_chained_state_value;
	}
	return ml_call(State, Function, 1, State->Values);
}

static void ml_chained_state_broadcast(ml_chained_state_t *State, ml_value_t *Value) {
	ml_value_t *Deref = ml_deref(Value);
	if (ml_is_error(Deref)) ML_CONTINUE(State->Base.Caller, Deref);
	State->Values[State->Index] = Value;
	if (++State->Index < State->Count) {
		return ml_call(State, State->Current[0], 1, State->Values + State->Index);
	}
	ml_value_t **Entry = State->Current;
	if (!Entry[2]) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
	ml_value_t *Function = Entry[3];
	if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
	State->Current = Entry + 4;
	State->Base.run = (void *)ml_chained_state_value;
	return ml_call(State, Function, State->Count, State->Values);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Entries[];
} ml_chained_function_t;

static void ml_chained_function_call(ml_state_t *Caller, ml_chained_function_t *Chained, int Count, ml_value_t **Args) {
	if (Chained->Entries[1] == DuoMethod) {
		ml_chained_state_t *State = xnew(ml_chained_state_t, Count, ml_value_t *);
		State->Base.Caller = Caller;
		State->Base.run = (void *)ml_chained_state_broadcast;
		State->Base.Context = Caller->Context;
		State->Current = Chained->Entries;
		State->Count = Count;
		State->Index = 0;
		memcpy(State->Values, Args, Count * sizeof(ml_value_t *));
		return ml_call(State, Chained->Entries[0], 1, State->Values);
	} else {
		ml_chained_state_t *State = xnew(ml_chained_state_t, 1, ml_value_t *);
		State->Base.Caller = Caller;
		State->Base.run = (void *)ml_chained_state_value;
		State->Base.Context = Caller->Context;
		State->Current = Chained->Entries + 1;
		return ml_call(State, Chained->Entries[0], Count, Args);
	}
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

static void ML_TYPED_FN(ml_value_sha256, MLChainedT, ml_chained_function_t *Chained, ml_hash_chain_t *Chain, unsigned char Hash[SHA256_BLOCK_SIZE]) {
	ml_value_sha256(Chained->Entries[0], Chain, Hash);
	ml_value_t **Entry = Chained->Entries;
	for (int I = 1; *Entry; ++I, ++Entry) {
		*(long *)(Hash + (I % 16)) ^= ml_hash_chain(*Entry, Chain);
	}
}

static ml_value_t *ML_TYPED_FN(ml_serialize, MLChainedT, ml_chained_function_t *Chained) {
	ml_value_t *Result = ml_list();
	ml_list_put(Result, ml_cstring("->"));
	for (ml_value_t **Entry = Chained->Entries; *Entry; ++Entry) {
		ml_list_put(Result, *Entry);
	}
	return Result;
}

ML_DESERIALIZER("->") {
	return ml_chained(Count, Args);
}

typedef struct {
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

static void ml_chained_iterator_while(ml_chained_iterator_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
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
	} else if (Function == WhileSoloMethod) {
		Function = Entry[1];
		if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
		State->Current = Entry + 2;
		State->Base.run = (void *)ml_chained_iterator_while;
		return ml_call(State, Function, 1, State->Values + 1);
	} else if (Function == WhileDuoMethod) {
		Function = Entry[1];
		if (!Function) ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Missing value function for chain"));
		State->Current = Entry + 2;
		State->Base.run = (void *)ml_chained_iterator_while;
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

ML_METHODX("precount", MLChainedT) {
//!internal
	ml_chained_function_t *Chained = (ml_chained_function_t *)Args[0];
	Args[0] = Chained->Entries[0];
	return ml_call(Caller, PrecountMethod, 1, Args);
}

ML_METHOD("->", MLFunctionT, MLFunctionT) {
//!function
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

ML_METHOD("=>", MLFunctionT, MLFunctionT) {
//!function
//<Base
//<Function
//>chained
// Returns a chained function equivalent to :mini:`Function(Base(Arg/1), Base(Arg/2), ...)`.
//$- let F := :upper => +
//$= F("h", "e", "l", "l", "o")
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 5, ml_value_t *);
	Chained->Type = MLChainedT;
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = DuoMethod;
	Chained->Entries[2] = ml_integer(1);
	Chained->Entries[3] = Args[1];
	//Chained->Entries[4] = NULL;
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

ML_METHOD("->!", MLFunctionT, MLFunctionT) {
//!function
//<Base
//<F
//>function
// Returns a chained function equivalent to :mini:`F ! Base(...)`.
//$= let F := list ->! 3
//$= F("cat")
	ml_value_t *Partial = ml_partial_function(ApplyMethod, 1);
	ml_partial_function_set(Partial, 0, Args[1]);
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 3, ml_value_t *);
	Chained->Type = MLChainedT;
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = Partial;
	//Chained->Entries[2] = NULL;
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

ML_METHOD("->?", MLFunctionT, MLFunctionT) {
//!function
//<Base
//<F
//>function
// Returns a chained function equivalent to :mini:`Base(...){F(it)}`.
//$= let F := 1 ->? (2 | _) -> (_ / 2)
//$= list(1 .. 10 -> F)
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 4, ml_value_t *);
	Chained->Type = ml_generic_sequence(MLChainedT, Args[0]);
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = FilterSoloMethod;
	Chained->Entries[2] = Args[1];
	//Chained->Entries[3] = NULL;
	return (ml_value_t *)Chained;
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



ML_METHOD("->|", MLSequenceT, MLFunctionT) {
//<Base
//<F
//>sequence
// Returns a chained sequence equivalent to :mini:`(K/j, V/j), ...` where :mini:`K/i` and :mini:`V/i` are the keys and values produced by :mini:`Base` while :mini:`F(V/j)` returns non-:mini:`nil`.
//$= list(1 .. 10 ->? (5 !| _))
//$= list(1 .. 10 ->| (5 !| _))
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 4, ml_value_t *);
	Chained->Type = ml_generic_sequence(MLChainedT, Args[0]);
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = WhileSoloMethod;
	Chained->Entries[2] = Args[1];
	//Chained->Entries[3] = NULL;
	return (ml_value_t *)Chained;
}

ML_METHOD("->|", MLChainedT, MLFunctionT) {
//!internal
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Entries[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 3, ml_value_t *);
	Chained->Type = MLChainedT;
	for (int I = 0; I < N; ++I) Chained->Entries[I] = Base->Entries[I];
	Chained->Entries[N] = WhileSoloMethod;
	Chained->Entries[N + 1] = Args[1];
	return (ml_value_t *)Chained;
}

ML_METHOD("=>|", MLSequenceT, MLFunctionT) {
//<Base
//<F
//>sequence
// Returns a chained sequence equivalent to :mini:`(K/j, V/j), ...` where :mini:`K/i` and :mini:`V/i` are the keys and values produced by :mini:`Base` while :mini:`F(K/j, V/j)` returns non-:mini:`nil`.
//$= let M := map(1 .. 10 -> fun(X) X ^ 2 % 10)
//$= map(M =>? fun(K, V) K + V < 15)
//$= map(M =>| fun(K, V) K + V < 15)
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 4, ml_value_t *);
	Chained->Type = ml_generic_sequence(MLChainedT, Args[0]);
	Chained->Entries[0] = Args[0];
	Chained->Entries[1] = WhileDuoMethod;
	Chained->Entries[2] = Args[1];
	//Chained->Entries[3] = NULL;
	return (ml_value_t *)Chained;
}

ML_METHOD("=>|", MLChainedT, MLFunctionT) {
//!internal
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Entries[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 3, ml_value_t *);
	Chained->Type = ml_generic_sequence(MLChainedT, Args[0]);
	for (int I = 0; I < N; ++I) Chained->Entries[I] = Base->Entries[I];
	Chained->Entries[N] = WhileDuoMethod;
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

typedef struct {
	ml_state_t Base;
	ml_value_t *ValueFn;
} ml_doubled_call_state_t;

ml_value_t *ml_doubled(ml_value_t *Sequence, ml_value_t *Function);

static void ml_doubled_run(ml_doubled_call_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_RETURN(Value);
	ML_RETURN(ml_doubled(Value, State->ValueFn));
}

static void ml_doubled_call(ml_state_t *Caller, ml_doubled_t *Doubled, int Count, ml_value_t **Args) {
	ml_doubled_call_state_t *State = new(ml_doubled_call_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_doubled_run;
	State->ValueFn = Doubled->ValueFn;
	return ml_call((ml_state_t *)State, Doubled->Sequence, Count, Args);
}

ML_TYPE(MLDoubledT, (MLSequenceT), "doubled",
//!internal
	.call = (void *)ml_doubled_call
);

typedef struct {
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
	Value = ml_deref(Value);
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

ML_METHOD("->>", MLFunctionT, MLFunctionT) {
//!internal
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

typedef struct {
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

ML_METHOD("precount", MLSequenceT) {
//!internal
	return MLNil;
}

typedef struct {
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

/****************************** Each ******************************/

typedef struct {
	ml_state_t Base;
	ml_value_t *Value, *Iter, *Fn;
	ml_value_t *Args[2];
} ml_apply_state_t;

static void apply_iterate(ml_apply_state_t *State, ml_value_t *Value);

static void apply_call(ml_apply_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)apply_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void apply_value(ml_apply_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)apply_call;
	State->Args[1] = Value;
	return ml_call(State, State->Fn, 2, State->Args);
}

static void apply_key(ml_apply_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)apply_value;
	State->Args[0] = Value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void apply_iterate(ml_apply_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Value);
	State->Base.run = (ml_state_fn)apply_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Value);
}

ML_METHODX("apply", MLSequenceT, MLFunctionT) {
//<Sequence
//<Fn
//>sequence
// Calls :mini:`Fn(Key, Value)` for each key and value produced by :mini:`Sequence`, then returns :mini:`Sequence`.
	ml_apply_state_t *State = new(ml_apply_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)apply_iterate;
	State->Value = Args[0];
	State->Fn = Args[1];
	return ml_iterate((ml_state_t *)State, Args[0]);
}

/****************************** Find ******************************/

static ML_METHOD_DECL(EqualMethod, "=");
static ML_METHOD_DECL(NotEqualMethod, "!=");

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Compare;
	ml_value_t *Args[2];
} ml_find_state_t;

static void find_iterate(ml_find_state_t *State, ml_value_t *Value);

static void find_compare(ml_find_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value != MLNil) return ml_iter_key(State->Base.Caller, State->Iter);
	State->Base.run = (ml_state_fn)find_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void find_value(ml_find_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)find_compare;
	State->Args[1] = Value;
	return ml_call(State, State->Compare, 2, State->Args);
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
// Returns the first key generated by :mini:`Sequence` with corresponding value :mini:`= Value`.
//$= map("a" .. "z"):find("j")
	ml_find_state_t *State = new(ml_find_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)find_iterate;
	State->Compare = EqualMethod;
	State->Args[0] = Args[1];
	return ml_iterate((ml_state_t *)State, Args[0]);
}

ML_METHODX("find", MLSequenceT, MLAnyT, MLFunctionT) {
//<Sequence
//<Value
//<Fn
//>any|nil
// Returns the first key generated by :mini:`Sequence` with corresponding value satisfying :mini:`Fn(_, Value)`.
//$= map("a" .. "z"):find("j", <)
	ml_find_state_t *State = new(ml_find_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)find_iterate;
	State->Compare = Args[2];
	State->Args[0] = Args[1];
	return ml_iterate((ml_state_t *)State, Args[0]);
}

/****************************** Random ******************************/

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

ML_METHODVX("random", MLTypeT) {
	ml_type_t *Type = (ml_type_t *)Args[0];
	ml_value_t *Random = stringmap_search(Type->Exports, "random");
	if (Random) {
		return ml_call(Caller, Random, Count - 1, Args + 1);
	} else  if (ml_is(Args[0], MLSequenceT)) {
		ml_random_state_t *State = new(ml_random_state_t);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (void *)random_iterate;
		State->Index = 0;
		State->Value = MLNil;
		return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
	} else {
		ML_ERROR("TypeError", "Type %s does not export random", Type->Name);
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

void ml_sum_optimized(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	typeof(ml_sum_optimized) *function = ml_typed_fn_get(ml_typeof(Value), ml_sum_optimized);
	if (function) return function(State, Value);
	State->Values[1] = Value;
	State->Base.run = (void *)reduce_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void reduce_iterate_sum(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	State->Base.run = (void *)ml_sum_optimized;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

void ml_sum_fallback(ml_iter_state_t *State, ml_value_t *Iter, ml_value_t *Total, ml_value_t *Value) {
	ml_value_t *Function = State->Values[0];
	State->Values[1] = Total;
	State->Values[2] = Value;
	State->Iter = Iter;
	State->Base.run = (void *)reduce_call;
	return ml_call(State, Function, 2, State->Values + 1);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_iter_state_t *State;
	union {
		int64_t Integer;
		double Real;
#ifdef ML_COMPLEX
		complex_double Complex;
#endif
	};
} ml_number_sum_t;

#ifdef ML_COMPLEX

static void complex_sum_iter_next(ml_number_sum_t *Sum, ml_value_t *Value);

static void complex_sum_next_value(ml_number_sum_t *Sum, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(Sum->Base.Caller, Value);
	if (!ml_is(Value, MLComplexT)) {
		return ml_sum_fallback(Sum->State, Sum->Iter, ml_complex(Sum->Complex), Value);
	}
	Sum->Complex += ml_complex_value(Value);
	Sum->Base.run = (void *)complex_sum_iter_next;
	return ml_iter_next((ml_state_t *)Sum, Sum->Iter);
}

static void complex_sum_iter_next(ml_number_sum_t *Sum, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(Sum->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(Sum->Base.Caller, ml_complex(Sum->Complex));
	Sum->Base.run = (void *)complex_sum_next_value;
	return ml_iter_value((ml_state_t *)Sum, Sum->Iter = Value);
}

static void ML_TYPED_FN(ml_sum_optimized, MLComplexT, ml_iter_state_t *State, ml_value_t *Value) {
	ml_number_sum_t *Sum = new(ml_number_sum_t);
	Sum->Base.Caller = State->Base.Caller;
	Sum->Base.Context = State->Base.Context;
	Sum->Base.run = (void *)complex_sum_iter_next;
	Sum->State = State;
	Sum->Iter = State->Iter;
	Sum->Complex = ml_complex_value(Value);
	return ml_iter_next((ml_state_t *)Sum, Sum->Iter);
}

#endif

static void real_sum_iter_next(ml_number_sum_t *Sum, ml_value_t *Value);

static void real_sum_next_value(ml_number_sum_t *Sum, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(Sum->Base.Caller, Value);
	if (ml_is(Value, MLRealT)) {
		Sum->Real += ml_real_value(Value);
		Sum->Base.run = (void *)real_sum_iter_next;
#ifdef ML_COMPLEX
	} else if (ml_is(Value, MLComplexT)) {
		Sum->Complex = Sum->Real + ml_complex_value(Value);
		Sum->Base.run = (void *)complex_sum_iter_next;
#endif
	} else {
		return ml_sum_fallback(Sum->State, Sum->Iter, ml_real(Sum->Integer), Value);
	}
	return ml_iter_next((ml_state_t *)Sum, Sum->Iter);
}

static void real_sum_iter_next(ml_number_sum_t *Sum, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(Sum->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(Sum->Base.Caller, ml_real(Sum->Real));
	Sum->Base.run = (void *)real_sum_next_value;
	return ml_iter_value((ml_state_t *)Sum, Sum->Iter = Value);
}

static void ML_TYPED_FN(ml_sum_optimized, MLRealT, ml_iter_state_t *State, ml_value_t *Value) {
	ml_number_sum_t *Sum = new(ml_number_sum_t);
	Sum->Base.Caller = State->Base.Caller;
	Sum->Base.Context = State->Base.Context;
	Sum->Base.run = (void *)real_sum_iter_next;
	Sum->State = State;
	Sum->Iter = State->Iter;
	Sum->Real = ml_real_value(Value);
	return ml_iter_next((ml_state_t *)Sum, Sum->Iter);
}

static void integer_sum_iter_next(ml_number_sum_t *Sum, ml_value_t *Value);

static void integer_sum_next_value(ml_number_sum_t *Sum, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(Sum->Base.Caller, Value);
	if (ml_is(Value, MLIntegerT)) {
		Sum->Integer += ml_integer_value(Value);
		Sum->Base.run = (void *)integer_sum_iter_next;
	} else if (ml_is(Value, MLRealT)) {
		Sum->Real = Sum->Integer + ml_real_value(Value);
		Sum->Base.run = (void *)real_sum_iter_next;
#ifdef ML_COMPLEX
	} else if (ml_is(Value, MLComplexT)) {
		Sum->Complex = Sum->Integer + ml_complex_value(Value);
		Sum->Base.run = (void *)complex_sum_iter_next;
#endif
	} else {
		return ml_sum_fallback(Sum->State, Sum->Iter, ml_integer(Sum->Integer), Value);
	}
	return ml_iter_next((ml_state_t *)Sum, Sum->Iter);
}

static void integer_sum_iter_next(ml_number_sum_t *Sum, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(Sum->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(Sum->Base.Caller, ml_integer(Sum->Integer));
	Sum->Base.run = (void *)integer_sum_next_value;
	return ml_iter_value((ml_state_t *)Sum, Sum->Iter = Value);
}

static void ML_TYPED_FN(ml_sum_optimized, MLIntegerT, ml_iter_state_t *State, ml_value_t *Value) {
	ml_number_sum_t *Sum = new(ml_number_sum_t);
	Sum->Base.Caller = State->Base.Caller;
	Sum->Base.Context = State->Base.Context;
	Sum->Base.run = (void *)integer_sum_iter_next;
	Sum->State = State;
	Sum->Iter = State->Iter;
	Sum->Integer = ml_integer_value(Value);
	return ml_iter_next((ml_state_t *)Sum, Sum->Iter);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_iter_state_t *State;
	ml_stringbuffer_t Buffer[1];
} ml_string_sum_t;

static void string_sum_iter_next(ml_string_sum_t *Sum, ml_value_t *Value);

static void string_sum_next_value(ml_string_sum_t *Sum, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(Sum->Base.Caller, Value);
	if (!ml_is(Value, MLStringT)) {
		return ml_sum_fallback(Sum->State, Sum->Iter, ml_stringbuffer_get_value(Sum->Buffer), Value);
	}
	ml_stringbuffer_write(Sum->Buffer, ml_string_value(Value), ml_string_length(Value));
	Sum->Base.run = (void *)string_sum_iter_next;
	return ml_iter_next((ml_state_t *)Sum, Sum->Iter);
}

static void string_sum_iter_next(ml_string_sum_t *Sum, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(Sum->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(Sum->Base.Caller, ml_stringbuffer_get_value(Sum->Buffer));
	Sum->Base.run = (void *)string_sum_next_value;
	return ml_iter_value((ml_state_t *)Sum, Sum->Iter = Value);
}

static void ML_TYPED_FN(ml_sum_optimized, MLStringT, ml_iter_state_t *State, ml_value_t *Value) {
	ml_string_sum_t *Sum = new(ml_string_sum_t);
	Sum->Base.Caller = State->Base.Caller;
	Sum->Base.Context = State->Base.Context;
	Sum->Base.run = (void *)string_sum_iter_next;
	Sum->State = State;
	Sum->Iter = State->Iter;
	ml_stringbuffer_write(Sum->Buffer, ml_string_value(Value), ml_string_length(Value));
	return ml_iter_next((ml_state_t *)Sum, Sum->Iter);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_iter_state_t *State;
	ml_value_t *Value;
} ml_container_sum_t;

#define ML_CONTAINER_SUM(TYPE, LNAME, UNAME, CREATE, INSERT) \
\
static void LNAME ## _sum_iter_next(ml_container_sum_t *Sum, ml_value_t *Value); \
\
static void LNAME ## _sum_next_value(ml_container_sum_t *Sum, ml_value_t *Value) { \
	Value = ml_deref(Value); \
	if (ml_is_error(Value)) ML_CONTINUE(Sum->Base.Caller, Value); \
	if (!ml_is(Value, TYPE)) { \
		return ml_sum_fallback(Sum->State, Sum->Iter, Sum->Value, Value); \
	} \
	ML_ ## UNAME ## _FOREACH(Value, Iter) INSERT; \
	Sum->Base.run = (void *)LNAME ## _sum_iter_next; \
	return ml_iter_next((ml_state_t *)Sum, Sum->Iter); \
} \
\
static void LNAME ## _sum_iter_next(ml_container_sum_t *Sum, ml_value_t *Value) { \
	if (ml_is_error(Value)) ML_CONTINUE(Sum->Base.Caller, Value); \
	if (Value == MLNil) ML_CONTINUE(Sum->Base.Caller, Sum->Value); \
	Sum->Base.run = (void *)LNAME ## _sum_next_value; \
	return ml_iter_value((ml_state_t *)Sum, Sum->Iter = Value); \
} \
\
static void ML_TYPED_FN(ml_sum_optimized, TYPE, ml_iter_state_t *State, ml_value_t *Value) { \
	ml_container_sum_t *Sum = new(ml_container_sum_t); \
	Sum->Base.Caller = State->Base.Caller; \
	Sum->Base.Context = State->Base.Context; \
	Sum->Base.run = (void *)LNAME ## _sum_iter_next; \
	Sum->State = State; \
	Sum->Iter = State->Iter; \
	Sum->Value = CREATE; \
	ML_ ## UNAME ## _FOREACH(Value, Iter) INSERT; \
	return ml_iter_next((ml_state_t *)Sum, Sum->Iter); \
}

ML_CONTAINER_SUM(MLListT, list, LIST, ml_list(), ml_list_put(Sum->Value, Iter->Value))
ML_CONTAINER_SUM(MLSliceT, slice, SLICE, ml_slice(0), ml_slice_put(Sum->Value, Iter->Value))
ML_CONTAINER_SUM(MLSetT, set, SET, ml_set(), ml_set_insert(Sum->Value, Iter->Key))
ML_CONTAINER_SUM(MLMapT, map, MAP, ml_map(), ml_map_insert(Sum->Value, Iter->Key, Iter->Value))

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
	State->Base.run = (void *)reduce_iterate_sum;
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

static void range_iter_next(ml_iter_state_t *State, ml_value_t *Value);

static void range_max(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value != MLNil) State->Values[2] = Value;
	State->Base.run = (ml_state_fn)range_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void range_min(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value != MLNil) State->Values[0] = Value;
	State->Base.run = (ml_state_fn)range_max;
	return ml_call(State, MaxMethod, 2, State->Values + 2);
}

static void range_value(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[1] = Value;
	State->Values[3] = Value;
	State->Base.run = (ml_state_fn)range_min;
	return ml_call(State, MinMethod, 2, State->Values + 0);
}

static void range_iter_next(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, ml_tuplev(2, State->Values[0], State->Values[2]));
	State->Base.run = (ml_state_fn)range_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void range_value_first(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Values[0] = Value;
	State->Values[1] = Value;
	State->Base.run = (ml_state_fn)range_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void range_iterate_first(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)range_value_first;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(Range) {
//<Sequence
//>tuple[any,any] | nil
// Returns the smallest and largest values (using :mini:`:min` and :mini:`:max`) produced by :mini:`Sequence`.
//$= range([1, 5, 2, 10, 6])
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLSequenceT);
	ml_iter_state_t *State = xnew(ml_iter_state_t, 4, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)range_iterate_first;
	State->Values[0] = MLNil;
	State->Values[2] = MLNil;
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

typedef struct {
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

typedef struct {
	ml_state_t Base;
	const char *Separator;
	ml_value_t *Iter;
	ml_stringbuffer_t *Buffer;
	size_t SeparatorLength;
} ml_append_state_t;

static void append_value(ml_append_state_t *State, ml_value_t *Value);

static void append_next(ml_append_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, MLSome);
	ml_stringbuffer_write(State->Buffer, State->Separator, State->SeparatorLength);
	State->Base.run = (void *)append_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void append_append(ml_append_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)append_next;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void append_value(ml_append_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)append_append;
	return ml_stringbuffer_append((ml_state_t *)State, State->Buffer, Value);
}

static void append_first(ml_append_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, ml_stringbuffer_get_value(State->Buffer));
	State->Base.run = (void *)append_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_METHODX("append", MLStringBufferT, MLSequenceT, MLStringT) {
//<Buffer
//<Sequence
//<Separator
//>some|nil
// Appends the values generated by :mini:`Sequence` to :mini:`Buffer` separated by :mini:`Separator`.
	ml_append_state_t *State = new(ml_append_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)append_first;
	State->Separator = ml_string_value(Args[2]);
	State->SeparatorLength = ml_string_length(Args[2]);
	State->Buffer = (ml_stringbuffer_t *)Args[0];
	return ml_iterate((ml_state_t *)State, Args[1]);
}

typedef struct {
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
// Returns a sequence that produces :mini:`V/1`, :mini:`Fn(V/1, V/2)`, :mini:`Fn(Fn(V/1, V/2), V/3)`, ... .
//
// .. deprecated:: 2.9.0
//
//    Use :mini:`distill` instead.
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
// Returns a sequence that produces :mini:`Initial`, :mini:`Fn(Initial, V/1)`, :mini:`Fn(Fn(Initial, V/1), V/2)`, ... .
//
// .. deprecated:: 2.9.0
//
//    Use :mini:`distill` instead.
//$= list(1 .. 10 // (10, +))
	ml_stacked_t *Stacked = new(ml_stacked_t);
	Stacked->Type = MLStackedT;
	Stacked->Value = Args[0];
	Stacked->Initial = Args[1];
	Stacked->ReduceFn = Args[2];
	return (ml_value_t *)Stacked;
}

ML_FUNCTION(Distill) {
//<Initial?:any
//<Sequence:sequence
//<Fn:function
//>any | nil
// Returns a sequence that produces :mini:`Fn(Initial, V/1)`, :mini:`Fn(Fn(Initial, V/1), V/2)`, ... if :mini:`Initial` is provided, otherwise returns a sequence that produces :mini:`V/1`, :mini:`Fn(V/1, V/2)`, :mini:`Fn(Fn(V/1, V/2), V/3)`, ... .
//
// The resulting sequence always has the same number of values as :mini:`Sequence`.
//$= list(distill(1 .. 10, +))
//$= list(distill(20, 1 .. 10, +))
	ML_CHECK_ARG_COUNT(2);
	if (Count == 2) {
		ML_CHECK_ARG_TYPE(0, MLSequenceT);
		ML_CHECK_ARG_TYPE(1, MLFunctionT);
		ml_stacked_t *Stacked = new(ml_stacked_t);
		Stacked->Type = MLStackedT;
		Stacked->Value = Args[0];
		Stacked->ReduceFn = Args[1];
		return (ml_value_t *)Stacked;
	} else {
		ML_CHECK_ARG_TYPE(1, MLSequenceT);
		ML_CHECK_ARG_TYPE(2, MLFunctionT);
		ml_stacked_t *Stacked = new(ml_stacked_t);
		Stacked->Type = MLStackedT;
		Stacked->Initial = Args[0];
		Stacked->Value = Args[1];
		Stacked->ReduceFn = Args[2];
		return (ml_value_t *)Stacked;
	}
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value, *Update;
} ml_repeated_t;

ML_TYPE(MLRepeatedT, (MLSequenceT), "repeated");
//!internal

typedef struct {
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

typedef struct {
	ml_type_t *Type;
	ml_value_t *Function;
} ml_function_sequence_t;

extern ml_type_t MLFunctionSequenceT[];

ML_FUNCTION(MLFunctionSequence) {
//!internal
	ML_CHECK_ARG_COUNT(1);
	ml_function_sequence_t *Sequence = new(ml_function_sequence_t);
	Sequence->Type = MLFunctionSequenceT;
	Sequence->Function = Args[0];
	return (ml_value_t *)Sequence;
}

ML_TYPE(MLFunctionSequenceT, (MLSequenceT), "function-sequence",
//!internal
	.Constructor = (ml_value_t *)MLFunctionSequence
);

typedef struct {
	ml_state_t Base;
	ml_value_t *Function;
	ml_value_t *Value;
	int Index;
} ml_function_state_t;

ML_TYPE(MLFunctionStateT, (MLStateT), "function-state");
//!internal

static void ML_TYPED_FN(ml_iter_next, MLFunctionStateT, ml_state_t *Caller, ml_function_state_t *State) {
	State->Base.Caller = Caller;
	return ml_call(State, State->Function, 0, NULL);
}

static void ML_TYPED_FN(ml_iter_key, MLFunctionStateT, ml_state_t *Caller, ml_function_state_t *State) {
	ML_RETURN(ml_integer(State->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLFunctionStateT, ml_state_t *Caller, ml_function_state_t *State) {
	ML_RETURN(State->Value);
}

static void ml_function_state_run(ml_function_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (Value == MLNil) ML_RETURN(Value);
	State->Value = Value;
	ML_RETURN(State);
}

static void ML_TYPED_FN(ml_iterate, MLFunctionSequenceT, ml_state_t *Caller, ml_function_sequence_t *Sequence) {
	ml_function_state_t *State = new(ml_function_state_t);
	State->Base.Type = MLFunctionStateT;
	State->Base.Context = Caller->Context;
	State->Base.Caller = Caller;
	State->Base.run = (ml_state_fn)ml_function_state_run;
	State->Function = Sequence->Function;
	return ml_call(State, State->Function, 0, NULL);
}

ML_METHOD("repeat", MLFunctionT) {
//<Function
//>sequence
// Returns a sequence that generates the result of calling :mini:`Function()` at each iteration until :mini:`nil` is returned.
//
// .. deprecated:: 2.9.0
//
//    Use :mini:`^` instead.
//$- let L := [1, 2, 3, 4]
//$= list(L:pull(_):repeat)
	ml_function_sequence_t *Sequence = new(ml_function_sequence_t);
	Sequence->Type = MLFunctionSequenceT;
	Sequence->Function = Args[0];
	return (ml_value_t *)Sequence;
}

ML_METHOD("^", MLFunctionT) {
//<Function
//>sequence
// Returns a sequence that generates the result of calling :mini:`Function()` at each iteration until :mini:`nil` is returned.
//$- let L := [1, 2, 3, 4]
//$= list(^fun L:pull)
	ml_function_sequence_t *Sequence = new(ml_function_sequence_t);
	Sequence->Type = MLFunctionSequenceT;
	Sequence->Function = Args[0];
	return (ml_value_t *)Sequence;
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *First, *Second;
} ml_sequenced_t;

ML_TYPE(MLSequencedT, (MLSequenceT), "sequenced");
//!internal

typedef struct {
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

ML_METHOD("&", MLIntegerIntervalT, MLIntegerIntervalT) {
	ml_integer_interval_t *Interval1 = (ml_integer_interval_t *)Args[0];
	ml_integer_interval_t *Interval2 = (ml_integer_interval_t *)Args[1];
	if (Interval1->Limit + 1 == Interval2->Start) {
		ml_integer_interval_t *Interval = new(ml_integer_interval_t);
		Interval->Type = MLIntegerIntervalT;
		Interval->Start = Interval1->Start;
		Interval->Limit = Interval2->Limit;
		return (ml_value_t *)Interval;
	}
	return ml_sequenced(Args[0], Args[1]);
}

ML_METHOD("&", MLIntegerRangeT, MLIntegerRangeT) {
	ml_integer_range_t *Range1 = (ml_integer_range_t *)Args[0];
	ml_integer_range_t *Range2 = (ml_integer_range_t *)Args[1];
	if ((Range1->Step == Range2->Step) && (Range1->Limit + Range1->Step == Range2->Start)) {
		ml_integer_range_t *Range = new(ml_integer_range_t);
		Range->Type = MLIntegerIntervalT;
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

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value;
	int Remaining;
} ml_limited_t;

ML_TYPE(MLLimitedT, (MLSequenceT), "limited");
//!internal

typedef struct {
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
	Limited->Remaining = ml_integer_value(Args[1]);
	return (ml_value_t *)Limited;
}

typedef struct {
	ml_state_t Base;
	int Value;
} integer_state_t;

static void limited_precount_run(integer_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (Value == MLNil) ML_RETURN(ml_integer(State->Value));
	int Precount = ml_integer_value(Value);
	if (Precount > State->Value) ML_RETURN(ml_integer(State->Value));
	ML_RETURN(Value);
}

ML_METHODX("precount", MLLimitedT) {
//!internal
	ml_limited_t *Limited = (ml_limited_t *)Args[0];
	integer_state_t *State = new(integer_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)limited_precount_run;
	State->Value = Limited->Remaining;
	Args[0] = Limited->Value;
	return ml_call(State, PrecountMethod, 1, Args);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value;
	long Remaining;
} ml_skipped_t;

ML_TYPE(MLSkippedT, (MLSequenceT), "skipped");
//!internal

typedef struct {
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
	Skipped->Remaining = ml_integer_value(Args[1]);
	return (ml_value_t *)Skipped;
}

static void skipped_precount_run(integer_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (Value == MLNil) ML_RETURN(Value);
	int Precount = ml_integer_value(Value);
	if (Precount <= State->Value) ML_RETURN(ml_integer(0));
	ML_RETURN(ml_integer(Precount - State->Value));
}

ML_METHODX("precount", MLSkippedT) {
//!internal
	ml_skipped_t *Skipped = (ml_skipped_t *)Args[0];
	integer_state_t *State = new(integer_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)skipped_precount_run;
	State->Value = Skipped->Remaining;
	Args[0] = Skipped->Value;
	return ml_call(State, PrecountMethod, 1, Args);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value, *Fn;
} ml_provided_t;

ML_TYPE(MLProvidedT, (MLSequenceT), "provided");
//!internal

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter, *Fn;
	ml_value_t *Args[1];
} ml_provided_state_t;

ML_TYPE(MLProvidedStateT, (MLStateT), "provided-state");
//!internal

static void provided_check(ml_provided_state_t *State, ml_value_t *Value);

static void provided_iterate(ml_provided_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (ml_deref(Value) == MLNil) ML_CONTINUE(State->Base.Caller, Value);
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

ML_METHOD("before", MLSequenceT, MLAnyT) {
	ml_value_t *Partial = ml_partial_function(NotEqualMethod, 2);
	ml_partial_function_set(Partial, 1, Args[1]);
	ml_provided_t *Provided = new(ml_provided_t);
	Provided->Type = ml_generic_sequence(MLProvidedT, Args[0]);
	Provided->Value = Args[0];
	Provided->Fn = Partial;
	return (ml_value_t *)Provided;
}

ML_TYPE(MLIgnoringT, (MLSequenceT), "ignoring-state");
//!internal

ML_TYPE(MLIgnoringStateT, (MLStateT), "ignoring-state");
//!internal

static void ignoring_check(ml_provided_state_t *State, ml_value_t *Value);

static void ignoring_iterate(ml_provided_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (ml_deref(Value) != MLNil) ML_CONTINUE(State->Base.Caller, State->Iter);
	State->Base.run = (ml_state_fn)ignoring_check;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ignoring_value(ml_provided_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)ignoring_iterate;
	State->Args[0] = Value;
	return ml_call(State, State->Fn, 1, State->Args);
}

static void ignoring_check(ml_provided_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)ignoring_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void ML_TYPED_FN(ml_iterate, MLIgnoringT, ml_state_t *Caller, ml_provided_t *Ignoring) {
	ml_provided_state_t *State = new(ml_provided_state_t);
	State->Base.Type = MLIgnoringStateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ignoring_check;
	State->Fn = Ignoring->Fn;
	return ml_iterate((ml_state_t *)State, Ignoring->Value);
}

ML_METHOD("skipuntil", MLSequenceT, MLFunctionT) {
//<Sequence
//<Fn
//>sequence
// Returns an sequence that skips initial values for which which :mini:`Fn(Value)` is :mini:`nil`.
//$= list("banana")
//$= list("banana" skipuntil (_ != "b"))
	ml_provided_t *Ignoring = new(ml_provided_t);
	Ignoring->Type = ml_generic_sequence(MLIgnoringT, Args[0]);
	Ignoring->Value = Args[0];
	Ignoring->Fn = Args[1];
	return (ml_value_t *)Ignoring;
}

ML_METHOD("from", MLSequenceT, MLAnyT) {
//<Sequence
//<Value
//>sequence
// Returns an sequence that skips initial values not equal to :mini:`Value`.
//$= list("banana")
//$= list("banana" from "n")
	ml_value_t *Partial = ml_partial_function(EqualMethod, 2);
	ml_partial_function_set(Partial, 1, Args[1]);
	ml_provided_t *Ignoring = new(ml_provided_t);
	Ignoring->Type = ml_generic_sequence(MLIgnoringT, Args[0]);
	Ignoring->Value = Args[0];
	Ignoring->Fn = Partial;
	return (ml_value_t *)Ignoring;
}

/*
ML_METHOD("after", MLSequenceT, MLAnyT) {
//<Sequence
//<Value
//>sequence
// Returns an sequence that skips initial values not equal to :mini:`Value` and skips :mini:`Value` itself once.
//$= list("banana")
//$= list("banana" after "n")
}
*/

typedef struct {
	ml_type_t *Type;
	ml_value_t *Iter;
} ml_unique_t;

ML_TYPE(MLUniqueT, (MLSequenceT), "unique");
//!internal

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *History;
	ml_value_t *Value;
} ml_unique_state_t;

ML_TYPE(MLUniqueStateT, (MLStateT), "unique-state");
//!internal

static void ml_unique_fnx_iterate(ml_unique_state_t *State, ml_value_t *Value);

static void ml_unique_fnx_value(ml_unique_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (ml_map_insert(State->History, Value, MLSome) == MLNil) {
		State->Value = Value;
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
	return ml_iterate((ml_state_t *)State, Unique->Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLUniqueStateT, ml_state_t *Caller, ml_unique_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
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

ML_TYPE(MLUniqueKeysT, (MLSequenceT), "unique");
//!internal

ML_TYPE(MLUniqueKeysStateT, (MLStateT), "unique-state");
//!internal

static void ml_unique2_fnx_iterate(ml_unique_state_t *State, ml_value_t *Value);

static void ml_unique2_fnx_value(ml_unique_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (ml_map_insert(State->History, Value, MLSome) == MLNil) {
		State->Value = Value;
		ML_CONTINUE(State->Base.Caller, State);
	}
	State->Base.run = (void *)ml_unique2_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ml_unique2_fnx_iterate(ml_unique_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)ml_unique2_fnx_value;
	return ml_iter_key((ml_state_t *)State, State->Iter = Value);
}

static void ML_TYPED_FN(ml_iterate, MLUniqueKeysT, ml_state_t *Caller, ml_unique_t *Unique) {
	ml_unique_state_t *State = new(ml_unique_state_t);
	State->Base.Type = MLUniqueKeysStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_unique2_fnx_iterate;
	State->Base.Context = Caller->Context;
	State->History = ml_map();
	return ml_iterate((ml_state_t *)State, Unique->Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLUniqueKeysStateT, ml_state_t *Caller, ml_unique_state_t *State) {
	ML_RETURN(State->Value);
}

static void ML_TYPED_FN(ml_iter_value, MLUniqueKeysStateT, ml_state_t *Caller, ml_unique_state_t *State) {
	return ml_iter_value(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLUniqueKeysStateT, ml_state_t *Caller, ml_unique_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_unique2_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_FUNCTION(UniqueKeys) {
//@unique1
//<Sequence:sequence
//>sequence
// Returns an sequence that returns the unique keys produced by :mini:`Sequence`. Uniqueness is determined by using a :mini:`map`.
//$= list(unique1("banana"))
	ML_CHECK_ARG_COUNT(1);
	ml_unique_t *Unique = new(ml_unique_t);
	ml_value_t *Iter = Unique->Iter = ml_chained(Count, Args);
	Unique->Type = ml_generic_sequence(MLUniqueKeysT, Iter);
	return (ml_value_t *)Unique;
}

typedef struct {
	ml_state_t Base;
	ml_value_t **Values;
	ml_value_t *Method;
	size_t Total;
	int Count, Index;
} ml_zip_count_state_t;

static void ml_zip_count_run(ml_zip_count_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	size_t Total = ml_integer_value(Value);
	if (Total > State->Total) Total = State->Total;
	int Index = State->Index + 1;
	if (Index == State->Count) ML_CONTINUE(State->Base.Caller, ml_integer(Total));
	State->Total = Total;
	State->Index = Index;
	return ml_call(State, State->Method, 1, State->Values + Index);
}

static void ml_zip_count(ml_state_t *Caller, ml_value_t *Method, int Count, ml_value_t **Values) {
	ml_zip_count_state_t *State = new(ml_zip_count_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_zip_count_run;
	State->Method = Method;
	State->Values = Values;
	State->Count = Count;
	State->Index = 0;
	State->Total = SIZE_MAX;
	return ml_call(State, State->Method, 1, State->Values);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Function;
	int Count;
	ml_value_t *Iters[];
} ml_zipped_t;

ML_TYPE(MLZippedT, (MLSequenceT), "zipped");
//!internal

typedef struct {
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

ML_METHODX("precount", MLZippedT) {
	ml_zipped_t *Zipped = (ml_zipped_t *)Args[0];
	return ml_zip_count(Caller, PrecountMethod, Zipped->Count, Zipped->Iters);
}

ML_METHODX("count", MLZippedT) {
	ml_zipped_t *Zipped = (ml_zipped_t *)Args[0];
	return ml_zip_count(Caller, CountMethod, Zipped->Count, Zipped->Iters);
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

ML_METHODX("precount", MLZipped2T) {
	ml_zipped2_t *Zipped = (ml_zipped2_t *)Args[0];
	return ml_zip_count(Caller, PrecountMethod, Zipped->Count, Zipped->Iters);
}

ML_METHODX("count", MLZipped2T) {
	ml_zipped2_t *Zipped = (ml_zipped2_t *)Args[0];
	return ml_zip_count(Caller, CountMethod, Zipped->Count, Zipped->Iters);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Function;
	int Count;
	ml_value_t *Values[];
} ml_grid_t;

ML_TYPE(MLGridT, (MLSequenceT), "grid");
//!internal

typedef struct {
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

typedef struct {
	ml_state_t Base;
	ml_grid_t *Grid;
	ml_value_t *Method;
	size_t Total;
	int Index;
} ml_grid_count_state_t;

static void ml_grid_count_run(ml_grid_count_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	size_t Total = State->Total * ml_integer_value(Value);
	int Index = State->Index + 1;
	if (Index == State->Grid->Count) ML_CONTINUE(State->Base.Caller, ml_integer(Total));
	State->Total = Total;
	State->Index = Index;
	return ml_call(State, State->Method, 1, State->Grid->Values + Index);
}

ML_METHODX("precount", MLGridT) {
	ml_grid_t *Grid = (ml_grid_t *)Args[0];
	if (!Grid->Count) ML_RETURN(ml_integer(0));
	ml_grid_count_state_t *State = new(ml_grid_count_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_grid_count_run;
	State->Grid = Grid;
	State->Method = PrecountMethod;
	State->Index = 0;
	State->Total = 1;
	return ml_call(State, State->Method, 1, Grid->Values);
}

ML_METHODX("count", MLGridT) {
	ml_grid_t *Grid = (ml_grid_t *)Args[0];
	if (!Grid->Count) ML_RETURN(ml_integer(0));
	ml_grid_count_state_t *State = new(ml_grid_count_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_grid_count_run;
	State->Grid = Grid;
	State->Method = CountMethod;
	State->Index = 0;
	State->Total = 1;
	return ml_call(State, State->Method, 1, Grid->Values);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Iters[2];
} ml_paired_t;

ML_TYPE(MLPairedT, (MLSequenceT), "paired");
//!internal

typedef struct {
	ml_state_t Base;
	ml_value_t *Iters[2];
} ml_paired_state_t;

ML_TYPE(MLPairedStateT, (MLStateT), "paired-state");
//!internal

static void paired_value_iterate(ml_paired_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iters[1] = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void paired_key_iterate(ml_paired_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iters[0] = Value;
	State->Base.run = (void *)paired_value_iterate;
	return ml_iterate((ml_state_t *)State, State->Iters[1]);
}

static void ML_TYPED_FN(ml_iterate, MLPairedT, ml_state_t *Caller, ml_paired_t *Paired) {
	ml_paired_state_t *State = new(ml_paired_state_t);
	State->Base.Type = MLPairedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)paired_key_iterate;
	State->Base.Context = Caller->Context;
	State->Iters[1] = Paired->Iters[1];
	return ml_iterate((ml_state_t *)State, Paired->Iters[0]);
}

static void ML_TYPED_FN(ml_iter_key, MLPairedStateT, ml_state_t *Caller, ml_paired_state_t *State) {
	return ml_iter_value(Caller, State->Iters[0]);
}

static void ML_TYPED_FN(ml_iter_value, MLPairedStateT, ml_state_t *Caller, ml_paired_state_t *State) {
	return ml_iter_value(Caller, State->Iters[1]);
}

static void paired_value_iter_next(ml_paired_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iters[1] = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void paired_key_iter_next(ml_paired_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Iters[0] = Value;
	State->Base.run = (void *)paired_value_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iters[1]);
}

static void ML_TYPED_FN(ml_iter_next, MLPairedStateT, ml_state_t *Caller, ml_paired_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)paired_key_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Iters[0]);
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
	Paired->Iters[0] = Args[0];
	Paired->Iters[1] = Args[1];
	return (ml_value_t *)Paired;
}

ML_METHODX("precount", MLPairedT) {
	ml_paired_t *Paired = (ml_paired_t *)Args[0];
	return ml_zip_count(Caller, PrecountMethod, 2, Paired->Iters);
}

ML_METHODX("count", MLPairedT) {
	ml_paired_t *Paired = (ml_paired_t *)Args[0];
	return ml_zip_count(Caller, CountMethod, 2, Paired->Iters);
}

typedef struct {
	ml_type_t *Type;
	int Count;
	ml_value_t *Iters[];
} ml_weaved_t;

ML_TYPE(MLWeavedT, (MLSequenceT), "weaved");
//!internal

typedef struct {
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

ML_METHODX("precount", MLUnpackedT) {
	ml_unpacked_t *Unpacked = (ml_unpacked_t *)Args[0];
	return ml_call(Caller, PrecountMethod, 1, &Unpacked->Iter);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Iter;
} ml_folded_t;

ML_TYPE(MLFoldedT, (MLSequenceT), "folded");
//!internal

typedef struct {
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

ml_value_t *ml_swap(ml_value_t *Value) {
	ml_swapped_t *Swapped = new(ml_swapped_t);
	Swapped->Value = Value;
#ifdef ML_GENERICS
	ml_type_t *TArgs[3];
	if (ml_find_generic_parent(ml_typeof(Swapped->Value), MLSequenceT, 3, TArgs) == 3) {
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
	return (ml_value_t *)Swapped;
}

ML_FUNCTION(Swap) {
//@swap
//<Sequence:sequence
// Returns a new sequence which swaps the keys and values produced by :mini:`Sequence`.
//$= map(swap("cake"))
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLSequenceT);
	ml_swapped_t *Swapped = new(ml_swapped_t);
	Swapped->Value = ml_chained(Count, Args);
#ifdef ML_GENERICS
	ml_type_t *TArgs[3];
	if (ml_find_generic_parent(ml_typeof(Swapped->Value), MLSequenceT, 3, TArgs) == 3) {
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
	State->Base.Caller = Caller;
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
	Key->Value = ml_chained(Count, Args);
	return (ml_value_t *)Key;
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value;
} ml_dup_t;

ML_TYPE(MLDupT, (MLSequenceT), "dup");
//!internal

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter, *Value;
} ml_dup_state_t;

ML_TYPE(MLDupStateT, (MLStateT), "dups-state");
//!internal

static void dup_value(ml_dup_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Value = Value;
	ML_CONTINUE(State->Base.Caller, State);
}

static void dup_iterate(ml_dup_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)dup_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void ML_TYPED_FN(ml_iterate, MLDupT, ml_state_t *Caller, ml_dup_t *Dup) {
	ml_dup_state_t *State = new(ml_dup_state_t);
	State->Base.Caller = Caller;
	State->Base.Type = MLDupStateT;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)dup_iterate;
	return ml_iterate((ml_state_t *)State, Dup->Value);
}

static void ML_TYPED_FN(ml_iter_key, MLDupStateT, ml_state_t *Caller, ml_dup_state_t *State) {
	ML_RETURN(State->Value);
}

static void ML_TYPED_FN(ml_iter_value, MLDupStateT, ml_state_t *Caller, ml_dup_state_t *State) {
	ML_RETURN(State->Value);
}

static void ML_TYPED_FN(ml_iter_next, MLDupStateT, ml_state_t *Caller, ml_dup_state_t *State) {
	State->Base.run = (ml_state_fn)dup_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ml_value_t *ml_dup(ml_value_t *Value) {
	ml_dup_t *Dup = new(ml_dup_t);
	Dup->Value = Value;
#ifdef ML_GENERICS
	ml_type_t *TArgs[3];
	if (ml_find_generic_parent(ml_typeof(Dup->Value), MLSequenceT, 3, TArgs) == 3) {
		TArgs[0] = MLDupT;
		TArgs[1] = TArgs[2];
		Dup->Type = ml_generic_type(3, TArgs);
	} else {
		Dup->Type = MLDupT;
	}
#else
	Dup->Type = MLDupT;
#endif
	return (ml_value_t *)Dup;
}

ML_FUNCTION(Dup) {
//@dup
//<Sequence:sequence
// Returns a new sequence which produces the values of :mini:`Sequence` as both keys and values.
//$= map(dup({"A" is 1, "B" is 2, "C" is 3}))
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLSequenceT);
	ml_dup_t *Dup = new(ml_dup_t);
	Dup->Value = ml_chained(Count, Args);
#ifdef ML_GENERICS
	ml_type_t *TArgs[3];
	if (ml_find_generic_parent(ml_typeof(Dup->Value), MLSequenceT, 3, TArgs) == 3) {
		TArgs[0] = MLDupT;
		TArgs[1] = TArgs[2];
		Dup->Type = ml_generic_type(3, TArgs);
	} else {
		Dup->Type = MLDupT;
	}
#else
	Dup->Type = MLDupT;
#endif
	return (ml_value_t *)Dup;
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Iter, *Function;
	int Size, Shift;
} ml_batched_t;

ML_TYPE(MLBatchedT, (MLSequenceT), "batched");
//!internal

typedef struct {
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
		if (!State->Iteration) {
			++State->Iteration;
			ML_CONTINUE(State->Base.Caller, State);
		} else if (State->Index > (State->Size - State->Shift)) {
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
			return ml_error("IntervalError", "Invalid shift value");
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

typedef struct {
	ml_type_t *Type;
	ml_value_t *Seq, *Fn;
} ml_split_t;

ML_TYPE(MLSplitT, (MLSequenceT), "split");

typedef struct {
	ml_state_t Base;
	ml_value_t Inner;
	ml_value_t *Fn;
	ml_value_t *Iter;
	ml_value_t *Args[1];
	int Skip, Index;
} ml_split_state_t;

ML_TYPE(MLSplitStateT, (MLStateT), "split-state");
//!internal

ML_TYPE(MLSplitInnerT, (MLSequenceT), "split-inner");
//!internal

static void ml_split_state_first_iter(ml_split_state_t *State, ml_value_t *Iter);

static void ml_split_state_first_split(ml_split_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (ml_deref(Value) == MLNil) {
		State->Base.run = (ml_state_fn)ml_split_state_first_iter;
		return ml_iter_next((ml_state_t *)State, State->Iter);
	}
	State->Skip = 1;
	++State->Index;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ml_split_state_first_value(ml_split_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[0] = Value;
	State->Base.run = (ml_state_fn)ml_split_state_first_split;
	return ml_call(State, State->Fn, 1, State->Args);
}

static void ml_split_state_first_iter(ml_split_state_t *State, ml_value_t *Iter) {
	if (ml_is_error(Iter)) ML_CONTINUE(State->Base.Caller, Iter);
	if (Iter == MLNil) ML_CONTINUE(State->Base.Caller, Iter);
	State->Base.run = (ml_state_fn)ml_split_state_first_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Iter);
}

static void ML_TYPED_FN(ml_iterate, MLSplitT, ml_state_t *Caller, ml_split_t *Split) {
	ml_split_state_t *State = new(ml_split_state_t);
	State->Base.Type = MLSplitStateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_split_state_first_iter;
	State->Fn = Split->Fn;
	State->Skip = 0;
	State->Index = 0;
	State->Inner.Type = MLSplitInnerT;
	return ml_iterate((ml_state_t *)State, Split->Seq);
}

static void ml_split_state_skip_iter(ml_split_state_t *State, ml_value_t *Iter);

static void ml_split_state_skip_split(ml_split_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (ml_deref(Value) != MLNil) {
		State->Base.run = (ml_state_fn)ml_split_state_skip_iter;
	} else {
		State->Base.run = (ml_state_fn)ml_split_state_first_iter;
	}
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ml_split_state_skip_value(ml_split_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[0] = Value;
	State->Base.run = (ml_state_fn)ml_split_state_skip_split;
	return ml_call(State, State->Fn, 1, State->Args);
}

static void ml_split_state_skip_iter(ml_split_state_t *State, ml_value_t *Iter) {
	if (ml_is_error(Iter)) ML_CONTINUE(State->Base.Caller, Iter);
	if (Iter == MLNil) ML_CONTINUE(State->Base.Caller, Iter);
	State->Base.run = (ml_state_fn)ml_split_state_skip_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLSplitStateT, ml_state_t *Caller, ml_split_state_t *State) {
	if (State->Iter == MLNil) ML_RETURN(MLNil);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	if (State->Skip) {
		State->Base.run = (ml_state_fn)ml_split_state_skip_iter;
	} else {
		State->Base.run = (ml_state_fn)ml_split_state_first_iter;
	}
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLSplitStateT, ml_state_t *Caller, ml_split_state_t *State) {
	ML_RETURN(ml_integer(State->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLSplitStateT, ml_state_t *Caller, ml_split_state_t *State) {
	ML_RETURN(&State->Inner);
}

static void ML_TYPED_FN(ml_iterate, MLSplitInnerT, ml_state_t *Caller, ml_value_t *Iter) {
	ml_split_state_t *State = (ml_split_state_t *)((void *)Iter - offsetof(ml_split_state_t, Inner));
	if (State->Iter == MLNil) ML_RETURN(MLNil);
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLSplitInnerT, ml_state_t *Caller, ml_value_t *Iter) {
	ml_split_state_t *State = (ml_split_state_t *)((void *)Iter - offsetof(ml_split_state_t, Inner));
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLSplitInnerT, ml_state_t *Caller, ml_value_t *Iter) {
	ml_split_state_t *State = (ml_split_state_t *)((void *)Iter - offsetof(ml_split_state_t, Inner));
	return ml_iter_value(Caller, State->Iter);
}

static void ml_split_state_inner_split(ml_split_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (ml_deref(Value) == MLNil) {
		State->Skip = 0;
		ML_CONTINUE(State->Base.Caller, MLNil);
	}
	ML_CONTINUE(State->Base.Caller, &State->Inner);
}

static void ml_split_state_inner_value(ml_split_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[0] = Value;
	State->Base.run = (ml_state_fn)ml_split_state_inner_split;
	return ml_call(State, State->Fn, 1, State->Args);
}

static void ml_split_state_inner_iter(ml_split_state_t *State, ml_value_t *Iter) {
	if (ml_is_error(Iter)) ML_CONTINUE(State->Base.Caller, Iter);
	State->Iter = Iter;
	if (Iter == MLNil) ML_CONTINUE(State->Base.Caller, Iter);
	State->Base.run = (ml_state_fn)ml_split_state_inner_value;
	return ml_iter_value((ml_state_t *)State, Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLSplitInnerT, ml_state_t *Caller, ml_value_t *Iter) {
	ml_split_state_t *State = (ml_split_state_t *)((void *)Iter - offsetof(ml_split_state_t, Inner));
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_split_state_inner_iter;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_METHOD("split", MLSequenceT, MLFunctionT) {
	ml_split_t *Split = new(ml_split_t);
	Split->Type = MLSplitT;
	Split->Seq = Args[0];
	Split->Fn = Args[1];
	return (ml_value_t *)Split;
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Seq;
	int Size;
} ml_chunk_t;

ML_TYPE(MLChunkT, (MLSequenceT), "chunk");

typedef struct {
	ml_state_t Base;
	ml_value_t Inner;
	ml_value_t *Iter;
	ml_value_t *Args[1];
	int Index, Size, Remain;
} ml_chunk_state_t;

ML_TYPE(MLChunkStateT, (MLStateT), "chunk-state");
//!internal

ML_TYPE(MLChunkInnerT, (MLSequenceT), "chunk-inner");
//!internal

static void ml_chunk_state_iterate(ml_chunk_state_t *State, ml_value_t *Iter) {
	if (ml_is_error(Iter)) ML_CONTINUE(State->Base.Caller, Iter);
	if (Iter == MLNil) ML_CONTINUE(State->Base.Caller, Iter);
	State->Iter = Iter;
	++State->Index;
	State->Remain = State->Size;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ML_TYPED_FN(ml_iterate, MLChunkT, ml_state_t *Caller, ml_chunk_t *Chunk) {
	ml_chunk_state_t *State = new(ml_chunk_state_t);
	State->Base.Type = MLChunkStateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_chunk_state_iterate;
	State->Size = Chunk->Size;
	State->Index = 0;
	State->Inner.Type = MLChunkInnerT;
	return ml_iterate((ml_state_t *)State, Chunk->Seq);
}

static void ML_TYPED_FN(ml_iter_next, MLChunkStateT, ml_state_t *Caller, ml_chunk_state_t *State) {
	if (State->Iter == MLNil) ML_RETURN(MLNil);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_chunk_state_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLChunkStateT, ml_state_t *Caller, ml_chunk_state_t *State) {
	ML_RETURN(ml_integer(State->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLChunkStateT, ml_state_t *Caller, ml_chunk_state_t *State) {
	ML_RETURN(&State->Inner);
}

static void ML_TYPED_FN(ml_iterate, MLChunkInnerT, ml_state_t *Caller, ml_value_t *Iter) {
	ml_chunk_state_t *State = (ml_chunk_state_t *)((void *)Iter - offsetof(ml_chunk_state_t, Inner));
	if (State->Iter == MLNil) ML_RETURN(MLNil);
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLChunkInnerT, ml_state_t *Caller, ml_value_t *Iter) {
	ml_chunk_state_t *State = (ml_chunk_state_t *)((void *)Iter - offsetof(ml_chunk_state_t, Inner));
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLChunkInnerT, ml_state_t *Caller, ml_value_t *Iter) {
	ml_chunk_state_t *State = (ml_chunk_state_t *)((void *)Iter - offsetof(ml_chunk_state_t, Inner));
	return ml_iter_value(Caller, State->Iter);
}

static void ml_chunk_state_inner_iter(ml_chunk_state_t *State, ml_value_t *Iter) {
	if (ml_is_error(Iter)) ML_CONTINUE(State->Base.Caller, Iter);
	State->Iter = Iter;
	if (Iter == MLNil) ML_CONTINUE(State->Base.Caller, Iter);
	ML_CONTINUE(State->Base.Caller, &State->Inner);
}

static void ML_TYPED_FN(ml_iter_next, MLChunkInnerT, ml_state_t *Caller, ml_value_t *Iter) {
	ml_chunk_state_t *State = (ml_chunk_state_t *)((void *)Iter - offsetof(ml_chunk_state_t, Inner));
	if (--State->Remain == 0) ML_RETURN(MLNil);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_chunk_state_inner_iter;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_METHOD("chunk", MLSequenceT, MLIntegerT) {
	ml_chunk_t *Chunk = new(ml_chunk_t);
	Chunk->Type = MLChunkT;
	Chunk->Seq = Args[0];
	Chunk->Size = ml_integer_value(Args[1]);
	return (ml_value_t *)Chunk;
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Seq, *Fn;
} ml_grouped_t;

ML_TYPE(MLGroupedT, (MLSequenceT), "grouped");

typedef struct {
	ml_state_t Base;
	ml_value_t Inner;
	ml_value_t *Current, *Next;
	ml_value_t *Fn;
	ml_value_t *Iter;
	ml_value_t *Args[2];
} ml_grouped_state_t;

ML_TYPE(MLGroupedStateT, (MLStateT), "grouped::state");
//!internal

ML_TYPE(MLGroupedInnerT, (MLSequenceT), "grouped::inner");
//!internal

static void ml_grouped_state_first_group(ml_grouped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Current = Value;
	State->Next = NULL;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ml_grouped_state_first_value(ml_grouped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[0] = Value;
	State->Base.run = (ml_state_fn)ml_grouped_state_first_group;
	return ml_call(State, State->Fn, 1, State->Args);
}

static void ml_grouped_state_first_iter(ml_grouped_state_t *State, ml_value_t *Iter) {
	if (ml_is_error(Iter)) ML_CONTINUE(State->Base.Caller, Iter);
	if (Iter == MLNil) ML_CONTINUE(State->Base.Caller, Iter);
	State->Base.run = (ml_state_fn)ml_grouped_state_first_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Iter);
}

static void ML_TYPED_FN(ml_iterate, MLGroupedT, ml_state_t *Caller, ml_grouped_t *Grouped) {
	ml_grouped_state_t *State = new(ml_grouped_state_t);
	State->Base.Type = MLGroupedStateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_grouped_state_first_iter;
	State->Fn = Grouped->Fn;
	State->Inner.Type = MLGroupedInnerT;
	return ml_iterate((ml_state_t *)State, Grouped->Seq);
}

static void ml_grouped_state_skip_iter(ml_grouped_state_t *State, ml_value_t *Iter);

static void ml_grouped_state_skip_check(ml_grouped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value != MLNil) {
		State->Base.run = (ml_state_fn)ml_grouped_state_skip_iter;
		return ml_iter_next((ml_state_t *)State, State->Iter);
	}
	State->Current = State->Next;
	State->Next = NULL;
	ML_CONTINUE(State->Base.Caller, State);
}

static void ml_grouped_state_skip_group(ml_grouped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[0] = State->Current;
	State->Args[1] = State->Next = Value;
	State->Base.run = (ml_state_fn)ml_grouped_state_skip_check;
	return ml_call(State, EqualMethod, 2, State->Args);
}

static void ml_grouped_state_skip_value(ml_grouped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[0] = Value;
	State->Base.run = (ml_state_fn)ml_grouped_state_skip_group;
	return ml_call(State, State->Fn, 1, State->Args);
}

static void ml_grouped_state_skip_iter(ml_grouped_state_t *State, ml_value_t *Iter) {
	if (ml_is_error(Iter)) ML_CONTINUE(State->Base.Caller, Iter);
	if (Iter == MLNil) ML_CONTINUE(State->Base.Caller, Iter);
	State->Base.run = (ml_state_fn)ml_grouped_state_skip_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLGroupedStateT, ml_state_t *Caller, ml_grouped_state_t *State) {
	if (State->Iter == MLNil) ML_RETURN(MLNil);
	if (State->Current) {
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (ml_state_fn)ml_grouped_state_skip_iter;
		return ml_iter_next((ml_state_t *)State, State->Iter);
	} else {
		State->Current = State->Next;
	}
	ML_RETURN(State);
}

static void ML_TYPED_FN(ml_iter_key, MLGroupedStateT, ml_state_t *Caller, ml_grouped_state_t *State) {
	ML_RETURN(State->Current);
}

static void ML_TYPED_FN(ml_iter_value, MLGroupedStateT, ml_state_t *Caller, ml_grouped_state_t *State) {
	ML_RETURN(&State->Inner);
}

static void ML_TYPED_FN(ml_iterate, MLGroupedInnerT, ml_state_t *Caller, ml_value_t *Iter) {
	ml_grouped_state_t *State = (ml_grouped_state_t *)((void *)Iter - offsetof(ml_grouped_state_t, Inner));
	if (State->Iter == MLNil) ML_RETURN(MLNil);
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLGroupedInnerT, ml_state_t *Caller, ml_value_t *Iter) {
	ml_grouped_state_t *State = (ml_grouped_state_t *)((void *)Iter - offsetof(ml_grouped_state_t, Inner));
	return ml_iter_key(Caller, State->Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLGroupedInnerT, ml_state_t *Caller, ml_value_t *Iter) {
	ml_grouped_state_t *State = (ml_grouped_state_t *)((void *)Iter - offsetof(ml_grouped_state_t, Inner));
	return ml_iter_value(Caller, State->Iter);
}

static void ml_grouped_state_inner_check(ml_grouped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		State->Current = NULL;
		ML_CONTINUE(State->Base.Caller, MLNil);
	} else {
		ML_CONTINUE(State->Base.Caller, &State->Inner);
	}
}

static void ml_grouped_state_inner_group(ml_grouped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[0] = State->Current;
	State->Args[1] = State->Next = Value;
	State->Base.run = (ml_state_fn)ml_grouped_state_inner_check;
	return ml_call(State, EqualMethod, 2, State->Args);
}

static void ml_grouped_state_inner_value(ml_grouped_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[0] = Value;
	State->Base.run = (ml_state_fn)ml_grouped_state_inner_group;
	return ml_call(State, State->Fn, 1, State->Args);
}

static void ml_grouped_state_inner_iter(ml_grouped_state_t *State, ml_value_t *Iter) {
	if (ml_is_error(Iter)) ML_CONTINUE(State->Base.Caller, Iter);
	State->Iter = Iter;
	if (Iter == MLNil) ML_CONTINUE(State->Base.Caller, Iter);
	State->Base.run = (ml_state_fn)ml_grouped_state_inner_value;
	return ml_iter_value((ml_state_t *)State, Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLGroupedInnerT, ml_state_t *Caller, ml_value_t *Iter) {
	ml_grouped_state_t *State = (ml_grouped_state_t *)((void *)Iter - offsetof(ml_grouped_state_t, Inner));
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_grouped_state_inner_iter;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

ML_METHOD("group", MLSequenceT, MLFunctionT) {
	ml_grouped_t *Grouped = new(ml_grouped_t);
	Grouped->Type = MLGroupedT;
	Grouped->Seq = Args[0];
	Grouped->Fn = Args[1];
	return (ml_value_t *)Grouped;
}

typedef struct {
	ml_state_t Base;
	union { ml_value_t *Iterator; ml_value_t *Sequence; };
	ml_value_t *Function;
	ml_value_t *Args[2];
} ml_generator_t;

static void ml_generator_value(ml_generator_t *Generator, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(Generator->Base.Caller, Value);
	Generator->Args[1] = Value;
	ml_state_t *Caller = Generator->Base.Caller;
	Generator->Base.Caller = NULL;
	return ml_call(Caller, Generator->Function, 2, Generator->Args);
}

static void ml_generator_key(ml_generator_t *Generator, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(Generator->Base.Caller, Value);
	Generator->Args[0] = Value;
	Generator->Base.run = (ml_state_fn)ml_generator_value;
	ml_iter_value((ml_state_t *)Generator, Generator->Iterator);
}

static void ml_generator_next(ml_generator_t *Generator, ml_value_t *Iterator) {
	if (ml_is_error(Iterator)) ML_CONTINUE(Generator->Base.Caller, Iterator);
	if (Iterator == MLNil) {
		Generator->Iterator = Iterator;
		ml_state_t *Caller = Generator->Base.Caller;
		Generator->Base.Caller = NULL;
		ML_RETURN(Iterator);
	}
	Generator->Base.run = (ml_state_fn)ml_generator_key;
	Generator->Iterator = Iterator;
	ml_iter_key((ml_state_t *)Generator, Iterator);
}

static void ml_generator2_call(ml_state_t *Caller, ml_generator_t *Generator, int Count, ml_value_t **Args) {
	if (Generator->Base.Caller) ML_ERROR("StateError", "Generator has not returned from previous call");
	if (Generator->Iterator == MLNil) ML_RETURN(MLNil);
	Generator->Base.Caller = Caller;
	Generator->Base.Context = Caller->Context;
	Generator->Base.run = (ml_state_fn)ml_generator_next;
	ml_iter_next((ml_state_t *)Generator, Generator->Iterator);
}

ML_TYPE(MLGenerator2T, (), "generator2",
//!internal
	.call = (void *)ml_generator2_call
);

static void ml_generator_call(ml_state_t *Caller, ml_generator_t *Generator, int Count, ml_value_t **Args) {
	if (Generator->Base.Caller) ML_ERROR("StateError", "Generator has not returned from previous call");
	Generator->Base.Type = MLGenerator2T;
	Generator->Base.Caller = Caller;
	Generator->Base.Context = Caller->Context;
	Generator->Base.run = (ml_state_fn)ml_generator_next;
	ml_iterate((ml_state_t *)Generator, Generator->Sequence);
}

ML_TYPE(MLGeneratorT, (), "generator",
//!internal
	.call = (void *)ml_generator_call
);

ML_METHOD("generate", MLSequenceT, MLFunctionT) {
//<Sequence
//<Function
//>function
// Returns a new function that returns :mini:`Function(Key, Value)` where :mini:`Key` and :mini:`Value` are the next key-value pair generated by :mini:`Sequence`. Once :mini:`Sequence` is exhausted, the function returns :mini:`nil`.
//$= let f := "cat" generate tuple
//$= f()
//$= f()
//$= f()
//$= f()
//$= f()
	ml_generator_t *Generator = new(ml_generator_t);
	Generator->Base.Type = MLGeneratorT;
	Generator->Sequence = Args[0];
	Generator->Function = Args[1];
	return (ml_value_t *)Generator;
}

void ml_sequence_init(stringmap_t *Globals) {
	MLSomeT->call = ml_some_call;
	MLFunctionT->Constructor = (ml_value_t *)MLChained;
	MLSequenceT->Constructor = (ml_value_t *)MLChained;
#include "ml_sequence_init.c"
	ml_value_t *Skip1 = ml_partial_function(ml_method("skip"), 2);
	ml_partial_function_set(Skip1, 1, ml_integer(1));
	ml_method_definev(ml_method("after"), ml_chainedv(2, ml_method("from"), Skip1), NULL, MLSequenceT, MLAnyT, NULL);
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
	ml_externals_default_add("min", Min);
	ml_externals_default_add("max", Max);
	ml_externals_default_add("unique", Unique);
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
		stringmap_insert(Globals, "distill", Distill);
		stringmap_insert(Globals, "min", Min);
		stringmap_insert(Globals, "max", Max);
		stringmap_insert(Globals, "sum", Sum);
		stringmap_insert(Globals, "prod", Prod);
		stringmap_insert(Globals, "range", Range);
		stringmap_insert(Globals, "reduce2", Reduce2);
		stringmap_insert(Globals, "min2", Min2);
		stringmap_insert(Globals, "max2", Max2);
		stringmap_insert(Globals, "unique", Unique);
		stringmap_insert(Globals, "unique1", UniqueKeys);
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
		stringmap_insert(Globals, "dup", Dup);
		stringmap_insert(Globals, "batch", Batch);
	}
}

#include <gc.h>
#include <string.h>
#include "minilang.h"
#include "ml_macros.h"
#include "ml_iterfns.h"
#include "ml_internal.h"

/*
typedef struct ml_each_iter_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t **Functions;
	int Index, Count;
} ml_each_iter_t;

static ml_value_t *ml_each_fnx_value(ml_each_iter_t *Frame, ml_value_t *Result);

static ml_value_t *ml_each_fnx_next(ml_each_iter_t *Frame, ml_value_t *Result) {
	Result = Result->Type->deref(Result);
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	if (Frame->Index < Frame->Count) {
		ml_value_t *Function = Frame->Functions[Frame->Index++];
		return Function->Type->call((ml_state_t *)Frame, Function, 1, &Result);
	} else {
		Frame->Base.run = (void *)ml_each_fnx_value;
		return ml_iter_next((ml_state_t *)Frame, Frame->Iter);
	}
}

static ml_value_t *ml_each_fnx_value(ml_each_iter_t *Frame, ml_value_t *Result) {
	Result = Result->Type->deref(Result);
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(Frame->Base.Caller, Result);
	Frame->Base.run = (void *)ml_each_fnx_next;
	Frame->Index = 0;
	return ml_iter_value((ml_state_t *)Frame, Frame->Iter = Result);
}

static ml_value_t *ml_each_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_each_iter_t *Frame = new(ml_each_iter_t);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_each_fnx_value;
	Frame->Count = Count - 1;
	Frame->Functions = Args + 1;
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}
*/

typedef struct ml_frame_iter_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Values[];
} ml_frame_iter_t;

static ml_value_t *ml_all_fnx_get_value(ml_frame_iter_t *Frame, ml_value_t *Result);

static ml_value_t *ml_all_fnx_append_value(ml_frame_iter_t *Frame, ml_value_t *Result) {
	Result = Result->Type->deref(Result);
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	ml_list_append(Frame->Values[0], Result);
	Frame->Base.run = (void *)ml_all_fnx_get_value;
	return ml_iter_next((ml_state_t *)Frame, Frame->Iter);
}

static ml_value_t *ml_all_fnx_get_value(ml_frame_iter_t *Frame, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	Frame->Base.run = (void *)ml_all_fnx_append_value;
	if (Result == MLNil) ML_CONTINUE(Frame->Base.Caller, Frame->Values[0]);
	return ml_iter_value((ml_state_t *)Frame, Frame->Iter = Result);
}

static ml_value_t *ml_all_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 1, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_all_fnx_get_value;
	Frame->Values[0] = ml_list();
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

static ml_value_t *ml_map_fnx_get_key(ml_frame_iter_t *Frame, ml_value_t *Result);

static ml_value_t *ml_map_fnx_get_value(ml_frame_iter_t *Frame, ml_value_t *Result);

static ml_value_t *ml_map_fnx_insert_key_value(ml_frame_iter_t *Frame, ml_value_t *Result);

static ml_value_t *ml_map_fnx_get_key(ml_frame_iter_t *Frame, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	Frame->Base.run = (void *)ml_map_fnx_get_value;
	if (Result == MLNil) ML_CONTINUE(Frame->Base.Caller, Frame->Values[0]);
	return ml_iter_key((ml_state_t *)Frame, Frame->Iter = Result);
}

static ml_value_t *ml_map_fnx_get_value(ml_frame_iter_t *Frame, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	Frame->Base.run = (void *)ml_map_fnx_insert_key_value;
	if (Result == MLNil) ML_CONTINUE(Frame->Base.Caller, Frame->Values[0]);
	Frame->Values[1] = Result;
	return ml_iter_value((ml_state_t *)Frame, Frame->Iter);
}

static ml_value_t *ml_map_fnx_insert_key_value(ml_frame_iter_t *Frame, ml_value_t *Result) {
	Result = Result->Type->deref(Result);
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	ml_map_insert(Frame->Values[0], Frame->Values[1], Result);
	Frame->Base.run = (void *)ml_map_fnx_get_key;
	return ml_iter_next((ml_state_t *)Frame, Frame->Iter);
}

static ml_value_t *ml_map_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 1, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_map_fnx_get_key;
	Frame->Values[0] = ml_map();
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

typedef struct ml_count_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	long Count;
} ml_count_state_t;

static ml_value_t *ml_count_fnx_increment(ml_count_state_t *Frame, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(Frame->Base.Caller, ml_integer(Frame->Count));
	++Frame->Count;
	return ml_iter_next((ml_state_t *)Frame, Frame->Iter = Result);
}

static ml_value_t *ml_count_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ml_count_state_t *Frame = xnew(ml_count_state_t, 1, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_count_fnx_increment;
	Frame->Count = 0;
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

static ml_value_t *LessMethod, *GreaterMethod, *AddMethod, *MulMethod;

static ml_value_t *ml_fold_fnx_get_next(ml_frame_iter_t *Frame, ml_value_t *Result);

static ml_value_t *ml_fold_fnx_result(ml_frame_iter_t *Frame, ml_value_t *Result) {
	Result = Result->Type->deref(Result);
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	if (Result != MLNil) Frame->Values[1] = Result;
	Frame->Base.run = (void *)ml_fold_fnx_get_next;
	return ml_iter_next((ml_state_t *)Frame, Frame->Iter);
}

static ml_value_t *ml_fold_fnx_fold(ml_frame_iter_t *Frame, ml_value_t *Result) {
	Result = Result->Type->deref(Result);
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	ml_value_t *Compare = Frame->Values[0];
	Frame->Values[2] = Result;
	Frame->Base.run = (void *)ml_fold_fnx_result;
	return Compare->Type->call((ml_state_t *)Frame, Compare, 2, Frame->Values + 1);
}

static ml_value_t *ml_fold_fnx_get_next(ml_frame_iter_t *Frame, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	Frame->Base.run = (void *)ml_fold_fnx_fold;
	if (Result == MLNil) ML_CONTINUE(Frame->Base.Caller, Frame->Values[1] ?: MLNil);
	return ml_iter_value((ml_state_t *)Frame, Frame->Iter = Result);
}

static ml_value_t *ml_fold_fnx_first(ml_frame_iter_t *Frame, ml_value_t *Result) {
	Result = Result->Type->deref(Result);
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	Frame->Values[1] = Result;
	Frame->Base.run = (void *)ml_fold_fnx_get_next;
	return ml_iter_next((ml_state_t *)Frame, Frame->Iter);
}

static ml_value_t *ml_fold_fnx_get_first(ml_frame_iter_t *Frame, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(Frame->Base.Caller, Result);
	Frame->Base.run = (void *)ml_fold_fnx_first;
	if (Result == MLNil) ML_CONTINUE(Frame->Base.Caller, Frame->Values[1] ?: MLNil);
	return ml_iter_value((ml_state_t *)Frame, Frame->Iter = Result);
}

static ml_value_t *ml_min_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 3, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_fold_fnx_get_first;
	Frame->Values[0] = GreaterMethod;
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

static ml_value_t *ml_max_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 3, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_fold_fnx_get_first;
	Frame->Values[0] = LessMethod;
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

static ml_value_t *ml_sum_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 3, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_fold_fnx_get_first;
	Frame->Values[0] = AddMethod;
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

static ml_value_t *ml_prod_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 3, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_fold_fnx_get_first;
	Frame->Values[0] = MulMethod;
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

static ml_value_t *ml_fold_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ML_CHECKX_ARG_TYPE(1, MLFunctionT);
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 3, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_fold_fnx_get_first;
	Frame->Values[0] = Args[1];
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

typedef struct ml_limited_t {
	const ml_type_t *Type;
	ml_value_t *Value;
	int Remaining;
} ml_limited_t;

static ml_type_t *MLLimitedT;

typedef struct ml_limited_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	int Remaining;
} ml_limited_state_t;

static ml_type_t *MLLimitedStateT;

static ml_value_t *ml_limited_fnx_iterate(ml_limited_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Iter = Result;
	--State->Remaining;
	ML_CONTINUE(State->Base.Caller, State);
}

static ml_value_t *ml_limited_iterate(ml_state_t *Caller, ml_limited_t *Limited) {
	if (Limited->Remaining) {
		ml_limited_state_t *State = new(ml_limited_state_t);
		State->Base.Type = MLLimitedStateT;
		State->Base.Caller = Caller;
		State->Base.run = (void *)ml_limited_fnx_iterate;
		State->Remaining = Limited->Remaining;
		return ml_iterate((ml_state_t *)State, Limited->Value);
	} else {
		ML_CONTINUE(Caller, MLNil);
	}
}

static ml_value_t *ml_limited_state_key(ml_state_t *Caller, ml_limited_state_t *State) {
	return ml_iter_key(Caller, State->Iter);
}

static ml_value_t *ml_limited_state_value(ml_state_t *Caller, ml_limited_state_t *State) {
	return ml_iter_value(Caller, State->Iter);
}

static ml_value_t *ml_limited_state_next(ml_state_t *Caller, ml_limited_state_t *State) {
	if (State->Remaining) {
		State->Base.Caller = Caller;
		State->Base.run = (void *)ml_limited_fnx_iterate;
		return ml_iter_next((ml_state_t *)State, State->Iter);
	} else {
		ML_CONTINUE(Caller, MLNil);
	}
}

ML_METHOD("of", MLIntegerT, MLIteratableT) {
	ml_limited_t *Limited = new(ml_limited_t);
	Limited->Type = MLLimitedT;
	Limited->Remaining = ml_integer_value(Args[0]);
	Limited->Value = Args[1];
	return (ml_value_t *)Limited;
}

typedef struct ml_skipped_t {
	const ml_type_t *Type;
	ml_value_t *Value;
	long Remaining;
} ml_skipped_t;

static ml_type_t *MLSkippedT;

typedef struct ml_skipped_state_t {
	ml_state_t Base;
	long Remaining;
} ml_skipped_state_t;

static ml_value_t *ml_skipped_fnx_iterate(ml_skipped_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	if (State->Remaining) {
		--State->Remaining;
		return ml_iter_next((ml_state_t *)State, Result);
	} else {
		ML_CONTINUE(State->Base.Caller, Result);
	}
}

static ml_value_t *ml_skipped_iterate(ml_state_t *Caller, ml_skipped_t *Skipped) {
	if (Skipped->Remaining) {
		ml_skipped_state_t *State = new(ml_skipped_state_t);
		State->Base.Caller = Caller;
		State->Base.run = (void *)ml_skipped_fnx_iterate;
		State->Remaining = Skipped->Remaining;
		return ml_iterate((ml_state_t *)State, Skipped->Value);
	} else {
		return ml_iterate(Caller, Skipped->Value);
	}
}

ML_METHOD("skip", MLIntegerT, MLIteratableT) {
	ml_skipped_t *Skipped = new(ml_skipped_t);
	Skipped->Type = MLSkippedT;
	Skipped->Remaining = ml_integer_value(Args[0]);
	Skipped->Value = Args[1];
	return (ml_value_t *)Skipped;
}

typedef struct {
	ml_state_t Base;
	size_t Waiting;
} ml_tasks_t;

static ml_type_t *MLTasksT;

static ml_value_t *ml_tasks_continue(ml_tasks_t *Tasks, ml_value_t *Value) {
	if (Value->Type == MLErrorT) {
		Tasks->Waiting = 0xFFFFFFFF;
		ML_CONTINUE(Tasks->Base.Caller, Value);
	}
	if (--Tasks->Waiting == 0) ML_CONTINUE(Tasks->Base.Caller, MLNil);
	return MLNil;
}

static ml_value_t *ml_tasks_fn(void *Data, int Count, ml_value_t **Args) {
	ml_tasks_t *Tasks = new(ml_tasks_t);
	Tasks->Base.Type = MLTasksT;
	Tasks->Base.run = (void *)ml_tasks_continue;
	Tasks->Waiting = 1;
	return (ml_value_t *)Tasks;
}

static ml_value_t *ml_tasks_call(ml_state_t *Caller, ml_tasks_t *Tasks, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_TYPE(Count - 1, MLFunctionT);
	ml_value_t *Function = Args[Count - 1];
	++Tasks->Waiting;
	Function->Type->call((ml_state_t *)Tasks, Function, Count - 1, Args);
	ML_CONTINUE(Caller, Tasks);
}

ML_METHODX("wait", MLTasksT) {
	ml_tasks_t *Tasks = (ml_tasks_t *)Args[0];
	Tasks->Base.Caller = Caller;
	if (--Tasks->Waiting == 0) ML_CONTINUE(Tasks->Base.Caller, MLNil);
	return MLNil;
}

typedef struct {
	ml_state_t Base;
	size_t Waiting;
} ml_parallel_t;

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Function;
} ml_parallel_iter_t;

static ml_value_t *ml_parallel_iterate(ml_parallel_iter_t *State, ml_value_t *Iter);

static ml_value_t *ml_parallel_iter_value(ml_parallel_iter_t *State, ml_value_t *Value) {
	ml_parallel_t *Parallel = (ml_parallel_t *)State->Base.Caller;
	Parallel->Waiting += 1;
	State->Function->Type->call(State->Base.Caller, State->Function, 1, &Value);
	State->Base.run = (void *)ml_parallel_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static ml_value_t *ml_parallel_iterate(ml_parallel_iter_t *State, ml_value_t *Iter) {
	if (Iter == MLNil) ML_CONTINUE(State->Base.Caller, MLNil);
	if (Iter->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Iter);
	State->Base.run = (void *)ml_parallel_iter_value;
	State->Iter = Iter;
	return ml_iter_value((ml_state_t *)State, Iter);
}

static ml_value_t *ml_parallel_continue(ml_parallel_t *State, ml_value_t *Value) {
	if (Value->Type == MLErrorT) {
		State->Waiting = 0xFFFFFFFF;
		ML_CONTINUE(State->Base.Caller, Value);
	}
	if (--State->Waiting == 0) ML_CONTINUE(State->Base.Caller, MLNil);
	return MLNil;
}

static ml_value_t *ml_parallel_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ML_CHECKX_ARG_TYPE(1, MLFunctionT);

	ml_parallel_t *S0 = new(ml_parallel_t);
	S0->Base.Caller = Caller;
	S0->Base.run = (void *)ml_parallel_continue;
	S0->Waiting = 1;

	ml_parallel_iter_t *S1 = new(ml_parallel_iter_t);
	S1->Base.Caller = (ml_state_t *)S0;
	S1->Base.run = (void *)ml_parallel_iterate;
	S1->Function = Args[1];

	return ml_iterate((ml_state_t *)S1, Args[0]);
}

typedef struct ml_unique_t {
	const ml_type_t *Type;
	ml_value_t *Iter;
} ml_unique_t;

static ml_type_t *MLUniqueT;

typedef struct ml_unique_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *History;
	ml_value_t *Value;
	int Iteration;
} ml_unique_state_t;

static ml_type_t *MLUniqueStateT;

static ml_value_t *ml_unique_fnx_iterate(ml_unique_state_t *State, ml_value_t *Result);

static ml_value_t *ml_unique_fnx_value(ml_unique_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	if (!ml_map_insert(State->History, Result, MLNil)) {
		State->Value = Result;
		++State->Iteration;
		ML_CONTINUE(State->Base.Caller, State);
	}
	State->Base.run = (void *)ml_unique_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static ml_value_t *ml_unique_fnx_iterate(ml_unique_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Base.run = (void *)ml_unique_fnx_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Result);
}

static ml_value_t *ml_unique_iterate(ml_state_t *Caller, ml_unique_t *Unique) {
	ml_unique_state_t *State = new(ml_unique_state_t);
	State->Base.Type = MLUniqueStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_unique_fnx_iterate;
	State->History = ml_map();
	State->Iteration = 0;
	return ml_iterate((ml_state_t *)State, Unique->Iter);
}

static ml_value_t *ml_unique_key(ml_state_t *Caller, ml_unique_state_t *State) {
	ML_CONTINUE(Caller, ml_integer(State->Iteration));
}

static ml_value_t *ml_unique_value(ml_state_t *Caller, ml_unique_state_t *State) {
	ML_CONTINUE(Caller, State->Value);
}

static ml_value_t *ml_unique_next(ml_state_t *Caller, ml_unique_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_unique_fnx_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static ml_value_t *ml_unique_fn(void *Data, int Count, ml_value_t **Args) {
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

static ml_type_t *MLGroupedT;

typedef struct ml_grouped_state_t {
	ml_state_t Base;
	ml_value_t *Function;
	ml_value_t **Iters;
	ml_value_t **Args;
	int Count, Index, Iteration;
} ml_grouped_state_t;

static ml_type_t *MLGroupedStateT;

static ml_value_t *ml_grouped_fnx_iterate(ml_grouped_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Iters[State->Index] = Result;
	if (++State->Index ==  State->Count) ML_CONTINUE(State->Base.Caller, State);
	return ml_iterate((ml_state_t *)State, State->Iters[State->Index]);
}

static ml_value_t *ml_grouped_iterate(ml_state_t *Caller, ml_grouped_t *Grouped) {
	ml_grouped_state_t *State = new(ml_grouped_state_t);
	State->Base.Type = MLGroupedStateT;
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_grouped_fnx_iterate;
	State->Function = Grouped->Function;
	State->Iters = anew(ml_value_t *, Grouped->Count);
	State->Args = anew(ml_value_t *, Grouped->Count);
	for (int I = 0; I < Grouped->Count; ++I) State->Iters[I] = Grouped->Iters[I];
	State->Count = Grouped->Count;
	State->Iteration = 1;
	return ml_iterate((ml_state_t *)State, State->Iters[0]);
}

static ml_value_t *ml_grouped_key(ml_state_t *Caller, ml_grouped_state_t *State) {
	ML_CONTINUE(Caller, ml_integer(State->Iteration));
}

static ml_value_t *ml_grouped_fnx_value(ml_grouped_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	State->Args[State->Index] = Result;
	if (++State->Index ==  State->Count) {
		return State->Function->Type->call(State->Base.Caller, State->Function, State->Count, State->Args);
	}
	return ml_iter_value((ml_state_t *)State, State->Iters[State->Index]);
}

static ml_value_t *ml_grouped_value(ml_state_t *Caller, ml_grouped_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_grouped_fnx_value;
	State->Index = 0;
	return ml_iter_value((ml_state_t *)State, State->Iters[0]);
}

static ml_value_t *ml_grouped_fnx_next(ml_grouped_state_t *State, ml_value_t *Result) {
	if (Result->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Iters[State->Index] = Result;
	if (++State->Index ==  State->Count) ML_CONTINUE(State->Base.Caller, State);
	return ml_iter_next((ml_state_t *)State, State->Iters[State->Index]);
}

static ml_value_t *ml_grouped_next(ml_state_t *Caller, ml_grouped_state_t *State) {
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_grouped_fnx_next;
	++State->Iteration;
	State->Index = 0;
	return ml_iter_next((ml_state_t *)State, State->Iters[0]);
}

static ml_value_t *ml_group_fn(void *Data, int Count, ml_value_t **Args) {
	ml_grouped_t *Grouped = new(ml_grouped_t);
	Grouped->Type = MLGroupedT;
	Grouped->Count = Count - 1;
	Grouped->Function = Args[Count - 1];
	Grouped->Iters = anew(ml_value_t *, Count - 1);
	for (int I = 0; I < Count - 1; ++I) Grouped->Iters[I] = Args[I];
	return (ml_value_t *)Grouped;
}

void ml_iterfns_init(stringmap_t *Globals) {
	LessMethod = ml_method("<");
	GreaterMethod = ml_method(">");
	AddMethod = ml_method("+");
	MulMethod = ml_method("*");
	//stringmap_insert(Globals, "each", ml_functionx(0, ml_each_fnx));
	stringmap_insert(Globals, "all", ml_functionx(0, ml_all_fnx));
	stringmap_insert(Globals, "map", ml_functionx(0, ml_map_fnx));
	stringmap_insert(Globals, "unique", ml_function(0, ml_unique_fn));
	stringmap_insert(Globals, "count", ml_functionx(0, ml_count_fnx));
	stringmap_insert(Globals, "min", ml_functionx(0, ml_min_fnx));
	stringmap_insert(Globals, "max", ml_functionx(0, ml_max_fnx));
	stringmap_insert(Globals, "sum", ml_functionx(0, ml_sum_fnx));
	stringmap_insert(Globals, "prod", ml_functionx(0, ml_prod_fnx));
	stringmap_insert(Globals, "fold", ml_functionx(0, ml_fold_fnx));
	stringmap_insert(Globals, "parallel", ml_functionx(0, ml_parallel_fnx));
	stringmap_insert(Globals, "tasks", ml_function(0, ml_tasks_fn));
	stringmap_insert(Globals, "group", ml_function(0, ml_group_fn));

	stringmap_insert(Globals, "tuple", ml_function(0, ml_tuple_new));
	stringmap_insert(Globals, "list", ml_function(0, ml_list_new));
	stringmap_insert(Globals, "map", ml_function(0, ml_map_new));

	MLLimitedT = ml_type(MLIteratableT, "limited");
	MLLimitedStateT = ml_type(MLAnyT, "limited-state");
	ml_typed_fn_set(MLLimitedT, ml_iterate, ml_limited_iterate);
	ml_typed_fn_set(MLLimitedStateT, ml_iter_next, ml_limited_state_next);
	ml_typed_fn_set(MLLimitedStateT, ml_iter_key, ml_limited_state_key);
	ml_typed_fn_set(MLLimitedStateT, ml_iter_value, ml_limited_state_value);

	MLSkippedT = ml_type(MLIteratableT, "skipped");
	ml_typed_fn_set(MLSkippedT, ml_iterate, ml_skipped_iterate);

	MLTasksT = ml_type(MLFunctionT, "tasks");
	MLTasksT->call = (void *)ml_tasks_call;

	MLUniqueT = ml_type(MLIteratableT, "unique");
	MLUniqueStateT = ml_type(MLAnyT, "unique-state");
	ml_typed_fn_set(MLUniqueT, ml_iterate, ml_unique_iterate);
	ml_typed_fn_set(MLUniqueStateT, ml_iter_next, ml_unique_next);
	ml_typed_fn_set(MLUniqueStateT, ml_iter_key, ml_unique_key);
	ml_typed_fn_set(MLUniqueStateT, ml_iter_value, ml_unique_value);

	MLGroupedT = ml_type(MLIteratableT, "grouped");
	MLGroupedStateT = ml_type(MLAnyT, "grouped-state");
	ml_typed_fn_set(MLGroupedT, ml_iterate, ml_grouped_iterate);
	ml_typed_fn_set(MLGroupedStateT, ml_iter_next, ml_grouped_next);
	ml_typed_fn_set(MLGroupedStateT, ml_iter_key, ml_grouped_key);
	ml_typed_fn_set(MLGroupedStateT, ml_iter_value, ml_grouped_value);


#include "ml_iterfns_init.c"
}

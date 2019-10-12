#include <gc.h>
#include <string.h>
#include "minilang.h"
#include "ml_macros.h"
#include "ml_iterfns.h"

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
	ml_frame_iter_t *Frame = xnew(ml_frame_iter_t, 1, ml_value_t *);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)ml_map_fnx_get_key;
	Frame->Values[0] = ml_map();
	return ml_iterate((ml_state_t *)Frame, Args[0]);
}

static ml_value_t *ml_uniq_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = ml_iterate(NULL, Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	ml_value_t *Map = ml_map();
	while (Iterator != MLNil) {
		ml_value_t *Value = ml_iter_value(NULL, Iterator);
		Value = Value->Type->deref(Value);
		if (Value->Type == MLErrorT) return Value;
		ml_map_insert(Map, Value, MLSome);
		Iterator = ml_iter_next(NULL, Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return Map;
}

static ml_value_t *ml_count_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = ml_iterate(NULL, Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	int Total = 0;
	while (Iterator != MLNil) {
		++Total;
		Iterator = ml_iter_next(NULL, Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return ml_integer(Total);
}

static ml_value_t *LessMethod, *GreaterMethod, *AddMethod, *MulMethod;

static ml_value_t *ml_min_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = ml_iterate(NULL, Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	if (Iterator == MLNil) return MLNil;
	ml_value_t *FoldArgs[2] = {ml_iter_value(NULL, Iterator), 0};
	FoldArgs[0] = FoldArgs[0]->Type->deref(FoldArgs[0]);
	if (FoldArgs[0]->Type == MLErrorT) return FoldArgs[0];
	Iterator = ml_iter_next(NULL, Iterator);
	if (Iterator->Type == MLErrorT) return Iterator;
	while (Iterator != MLNil) {
		FoldArgs[1] = ml_iter_value(NULL, Iterator);
		FoldArgs[1] = FoldArgs[1]->Type->deref(FoldArgs[1]);
		if (FoldArgs[1]->Type == MLErrorT) return FoldArgs[1];
		ml_value_t *Compare = ml_call(GreaterMethod, 2, FoldArgs);
		if (Compare->Type == MLErrorT) return Compare;
		if (Compare != MLNil) FoldArgs[0] = FoldArgs[1];
		Iterator = ml_iter_next(NULL, Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return FoldArgs[0];
}

static ml_value_t *ml_max_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = ml_iterate(NULL, Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	if (Iterator == MLNil) return MLNil;
	ml_value_t *FoldArgs[2] = {ml_iter_value(NULL, Iterator), 0};
	FoldArgs[0] = FoldArgs[0]->Type->deref(FoldArgs[0]);
	if (FoldArgs[0]->Type == MLErrorT) return FoldArgs[0];
	Iterator = ml_iter_next(NULL, Iterator);
	if (Iterator->Type == MLErrorT) return Iterator;
	while (Iterator != MLNil) {
		FoldArgs[1] = ml_iter_value(NULL, Iterator);
		FoldArgs[1] = FoldArgs[1]->Type->deref(FoldArgs[1]);
		if (FoldArgs[1]->Type == MLErrorT) return FoldArgs[1];
		ml_value_t *Compare = ml_call(LessMethod, 2, FoldArgs);
		if (Compare->Type == MLErrorT) return Compare;
		if (Compare != MLNil) FoldArgs[0] = FoldArgs[1];
		Iterator = ml_iter_next(NULL, Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return FoldArgs[0];
}

static ml_value_t *ml_sum_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = ml_iterate(NULL, Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	if (Iterator == MLNil) return MLNil;
	ml_value_t *FoldArgs[2] = {ml_iter_value(NULL, Iterator), 0};
	FoldArgs[0] = FoldArgs[0]->Type->deref(FoldArgs[0]);
	if (FoldArgs[0]->Type == MLErrorT) return FoldArgs[0];
	Iterator = ml_iter_next(NULL, Iterator);
	if (Iterator->Type == MLErrorT) return Iterator;
	while (Iterator != MLNil) {
		FoldArgs[1] = ml_iter_value(NULL, Iterator);
		FoldArgs[1] = FoldArgs[1]->Type->deref(FoldArgs[1]);
		if (FoldArgs[1]->Type == MLErrorT) return FoldArgs[1];
		FoldArgs[0] = ml_call(AddMethod, 2, FoldArgs);
		if (FoldArgs[0]->Type == MLErrorT) return FoldArgs[0];
		Iterator = ml_iter_next(NULL, Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return FoldArgs[0];
}

static ml_value_t *ml_prod_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Iterator = ml_iterate(NULL, Args[0]);
	if (Iterator->Type == MLErrorT) return Iterator;
	if (Iterator == MLNil) return MLNil;
	ml_value_t *FoldArgs[2] = {ml_iter_value(NULL, Iterator), 0};
	FoldArgs[0] = FoldArgs[0]->Type->deref(FoldArgs[0]);
	if (FoldArgs[0]->Type == MLErrorT) return FoldArgs[0];
	Iterator = ml_iter_next(NULL, Iterator);
	if (Iterator->Type == MLErrorT) return Iterator;
	while (Iterator != MLNil) {
		FoldArgs[1] = ml_iter_value(NULL, Iterator);
		FoldArgs[1] = FoldArgs[1]->Type->deref(FoldArgs[1]);
		if (FoldArgs[1]->Type == MLErrorT) return FoldArgs[1];
		FoldArgs[0] = ml_call(MulMethod, 2, FoldArgs);
		if (FoldArgs[0]->Type == MLErrorT) return FoldArgs[0];
		Iterator = ml_iter_next(NULL, Iterator);
		if (Iterator->Type == MLErrorT) return Iterator;
	}
	return FoldArgs[0];
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

static ml_value_t *ml_limited_fn(void *Data, int Count, ml_value_t **Args) {
	ml_limited_t *Limited = new(ml_limited_t);
	Limited->Type = MLLimitedT;
	Limited->Remaining = ml_integer_value(Args[0]);
	Limited->Value = Args[1];
	return (ml_value_t *)Limited;
}

typedef struct ml_skipped_t {
	const ml_type_t *Type;
	ml_value_t *Value;
	int Remaining;
} ml_skipped_t;

static ml_type_t *MLSkippedT;

typedef struct ml_skipped_state_t {
	ml_state_t Base;
	int Remaining;
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

static ml_value_t *ml_skipped_fn(void *Data, int Count, ml_value_t **Args) {
	ml_skipped_t *Skipped = new(ml_skipped_t);
	Skipped->Type = MLSkippedT;
	Skipped->Remaining = ml_integer_value(Args[0]);
	Skipped->Value = Args[1];
	return (ml_value_t *)Skipped;
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
	ML_CONTINUE(NULL, MLNil);
}

static ml_value_t *ml_parallel_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLIteratableT);
	ML_CHECKX_ARG_TYPE(1, MLFunctionT);

	ml_parallel_t *S0 = new(ml_parallel_t);
	S0->Base.Caller = Caller;
	S0->Base.run = (void *)ml_parallel_iterate;
	S0->Waiting = 1;

	ml_parallel_iter_t *S1 = new(ml_parallel_iter_t);
	S1->Base.Caller = (ml_state_t *)S0;
	S1->Base.run = (void *)ml_parallel_continue;
	S1->Function = Args[1];

	return ml_iterate((ml_state_t *)S1, Args[0]);
}

void ml_iterfns_init(stringmap_t *Globals) {
	LessMethod = ml_method("<");
	GreaterMethod = ml_method(">");
	AddMethod = ml_method("+");
	MulMethod = ml_method("*");
	stringmap_insert(Globals, "all", ml_functionx(0, ml_all_fnx));
	stringmap_insert(Globals, "map", ml_functionx(0, ml_map_fnx));
	stringmap_insert(Globals, "uniq", ml_function(0, ml_uniq_fn));
	stringmap_insert(Globals, "count", ml_function(0, ml_count_fn));
	stringmap_insert(Globals, "min", ml_function(0, ml_min_fn));
	stringmap_insert(Globals, "max", ml_function(0, ml_max_fn));
	stringmap_insert(Globals, "sum", ml_function(0, ml_sum_fn));
	stringmap_insert(Globals, "prod", ml_function(0, ml_prod_fn));
	stringmap_insert(Globals, "parallel", ml_functionx(0, ml_parallel_fnx));

	MLLimitedT = ml_type(MLIteratableT, "limited");
	MLLimitedStateT = ml_type(MLAnyT, "limited-state");
	ml_typed_fn_set(MLLimitedT, ml_iterate, ml_limited_iterate);
	ml_typed_fn_set(MLLimitedStateT, ml_iter_next, ml_limited_state_next);
	ml_typed_fn_set(MLLimitedStateT, ml_iter_key, ml_limited_state_key);
	ml_typed_fn_set(MLLimitedStateT, ml_iter_value, ml_limited_state_value);
	ml_method_by_name("of", NULL, ml_limited_fn, MLIntegerT, MLIteratableT, NULL);

	MLSkippedT = ml_type(MLIteratableT, "skipped");
	ml_typed_fn_set(MLSkippedT, ml_iterate, ml_skipped_iterate);
	ml_method_by_name("skip", NULL, ml_skipped_fn, MLIntegerT, MLIteratableT, NULL);
}

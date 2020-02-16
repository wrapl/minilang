#include "minilang.h"
#include "ml_macros.h"
#include "stringmap.h"
#include <gc.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include "ml_runtime.h"

static ml_value_t *ml_reference_deref(ml_value_t *Ref) {
	ml_reference_t *Reference = (ml_reference_t *)Ref;
	return Reference->Address[0];
}

static ml_value_t *ml_reference_assign(ml_value_t *Ref, ml_value_t *Value) {
	Value = Value->Type->deref(Value);
	if (Value->Type == MLErrorT) return Value;
	ml_reference_t *Reference = (ml_reference_t *)Ref;
	return Reference->Address[0] = Value;
}

ml_type_t MLReferenceT[1] = {{
	MLTypeT,
	MLAnyT, "reference",
	ml_default_hash,
	ml_default_call,
	ml_reference_deref,
	ml_reference_assign,
	NULL, 0, 0
}};

inline ml_value_t *ml_reference(ml_value_t **Address) {
	ml_reference_t *Reference;
	if (Address == 0) {
		Reference = xnew(ml_reference_t, 1, ml_value_t *);
		Reference->Address = Reference->Value;
		Reference->Value[0] = MLNil;
	} else {
		Reference = new(ml_reference_t);
		Reference->Address = Address;
	}
	Reference->Type = MLReferenceT;
	return (ml_value_t *)Reference;
}

static ml_value_t *ml_state_call(ml_state_t *Caller, ml_state_t *State, int Count, ml_value_t **Args) {
	return State->run(State, Count ? Args[0] : MLNil);
}

ml_type_t MLStateT[1] = {{
	MLTypeT,
	MLFunctionT, "state",
	ml_default_hash,
	(void *)ml_state_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

typedef struct ml_functionx_t {
	const ml_type_t *Type;
	ml_callbackx_t Callback;
	void *Data;
} ml_functionx_t;

static ml_value_t *ml_functionx_call(ml_state_t *Caller, ml_functionx_t *Function, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I] = Args[I]->Type->deref(Args[I]);
		if (Arg->Type == MLErrorT) ML_CONTINUE(Caller, Arg);
	}
	return (Function->Callback)(Caller, Function->Data, Count, Args);
}

ml_type_t MLFunctionXT[1] = {{
	MLTypeT,
	MLFunctionT, "functionx",
	ml_default_hash,
	(void *)ml_functionx_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_functionx(void *Data, ml_callbackx_t Callback) {
	ml_functionx_t *Function = fnew(ml_functionx_t);
	Function->Type = MLFunctionXT;
	Function->Data = Data;
	Function->Callback = Callback;
	GC_end_stubborn_change(Function);
	return (ml_value_t *)Function;
}

typedef struct ml_resumable_state_t {
	ml_state_t Base;
	ml_state_t *Last;
} ml_resumable_state_t;

static ml_value_t *ml_resumable_state_call(ml_state_t *Caller, ml_resumable_state_t *State, int Count, ml_value_t **Args) {
	State->Last->Caller = Caller;
	ML_CONTINUE(State->Base.Caller, Count ? Args[0] : MLNil);
}

ml_type_t MLResumableStateT[1] = {{
	MLTypeT,
	MLStateT, "resumable-state",
	ml_default_hash,
	(void *)ml_resumable_state_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

static ml_value_t *ml_resumable_state_run(ml_resumable_state_t *State, ml_value_t *Value) {
	ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Invalid use of resumable state"));
}

static ml_value_t *ml_callcc(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	if (Count > 1) {
		ML_CHECKX_ARG_TYPE(0, MLStateT);
		ml_state_t *State = (ml_state_t *)Args[0];
		ml_state_t *Last = Caller;
		while (Last && Last->Caller != State) Last = Last->Caller;
		if (!Last) ML_CONTINUE(Caller, ml_error("StateError", "State not in current call chain"));
		Last->Caller = NULL;
		ml_resumable_state_t *Resumable = new(ml_resumable_state_t);
		Resumable->Base.Type = MLResumableStateT;
		Resumable->Base.Caller = Caller;
		Resumable->Base.run = (void *)ml_resumable_state_run;
		Resumable->Last = Last;
		ml_value_t *Function = Args[1];
		ml_value_t **Args2 = anew(ml_value_t *, 1);
		Args2[0] = (ml_value_t *)Resumable;
		return Function->Type->call(State, Function, 1, Args2);
	} else {
		ML_CHECKX_ARG_COUNT(1);
		ml_value_t *Function = Args[0];
		ml_value_t **Args2 = anew(ml_value_t *, 1);
		Args2[0] = (ml_value_t *)Caller;
		return Function->Type->call(NULL, Function, 1, Args2);
	}
}

static ml_value_t *ml_spawn_state_fn(ml_state_t *State, ml_value_t *Value) {
	ML_CONTINUE(State->Caller, Value);
}

static ml_value_t *ml_spawn(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_state_t *State = new(ml_state_t);
	State->Type = MLStateT;
	State->Caller = Caller;
	State->run = ml_spawn_state_fn;
	ml_value_t *Func = Args[0];
	ml_value_t **Args2 = anew(ml_value_t *, 1);
	Args2[0] = (ml_value_t *)State;
	return Func->Type->call(State, Func, 1, Args2);
}

ml_functionx_t MLCallCC[1] = {{MLFunctionXT, ml_callcc, NULL}};
ml_functionx_t MLSpawn[1] = {{MLFunctionXT, ml_spawn, NULL}};

ml_type_t MLUninitializedT[] = {{
	MLTypeT,
	MLAnyT, "uninitialized",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_default_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	ML_CONTINUE(Caller, ml_error("TypeError", "<%s> is not callable", Value->Type->Name));
}

inline ml_value_t *ml_call(ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_value_t *Result = Value->Type->call(NULL, Value, Count, Args);
	return Result->Type->deref(Result);
}

ml_value_t *ml_inline(ml_value_t *Value, int Count, ...) {
	ml_value_t *Args[Count];
	va_list List;
	va_start(List, Count);
	for (int I = 0; I < Count; ++I) Args[I] = va_arg(List, ml_value_t *);
	va_end(List);
	return ml_call(Value, Count, Args);
}

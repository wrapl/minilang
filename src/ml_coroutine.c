#include "minilang.h"
#include "ml_coroutine.h"
#include "ml_macros.h"
#include "coro.h"

#define STACK_SIZE (1 << 20)

struct ml_coro_state_t {
	ml_state_t Base;
	ml_coro_state_t *Next;
	void *Data;
	void (*Callback)(ml_coro_state_t *, void *);
	coro_context Return[1];
	coro_context Context[1];
};

static
#ifdef ML_THREADS
__thread
#endif
ml_coro_state_t *CoroCache = NULL, *Current = NULL;

void *ml_coro_escape(void *Data, void (*Callback)(ml_coro_state_t *, void *)) {
	ml_coro_state_t *State = Current;
	if (!State) return ml_error("StateError", "Must be called from a coroutine");
	State->Callback = Callback;
	State->Data = Data;
	Current = NULL;
	coro_transfer(State->Context, State->Return);
	Current = State;
	State->Callback = NULL;
	return State->Data;
}

void ml_coro_resume(ml_coro_state_t *State, void *Data) {
	State->Data = Data;
	coro_transfer(State->Return, State->Context);
	Current = NULL;
	if (State->Callback) return State->Callback(State, State->Data);
	State->Next = CoroCache;
	CoroCache = State;
	ML_CONTINUE(State->Base.Caller, State->Data);
}

typedef struct {
	ml_value_t *Function, **Args;
	int Count;
} ml_coro_call_t;

static void ml_coro_escape_call(ml_coro_state_t *State, ml_coro_call_t *Call) {
	return ml_call(State, Call->Function, Call->Count, Call->Args);
}

ml_value_t *ml_coro_call(ml_value_t *Function, int Count, ml_value_t **Args) {
	ml_coro_call_t Call = {Function, Args, Count};
	return ml_coro_escape(&Call, (void *)ml_coro_escape_call);
}

typedef struct {
	ml_callback_t Function;
	ml_value_t **Args;
	int Count;
} ml_coro_entry_t;

static void ml_coro_start(ml_coro_state_t *State) {
	for (;;) {
		Current = State;
		ml_coro_entry_t *Entry = (ml_coro_entry_t *)State->Data;
		State->Data = Entry->Function(State, Entry->Count, Entry->Args);
		coro_transfer(State->Context, State->Return);
	}
}

void ml_coro_enter(ml_state_t *Caller, ml_callback_t Function, int Count, ml_value_t **Args) {
	ml_coro_state_t *State = CoroCache;
	if (!State) {
		State = new(ml_coro_state_t);
		State->Base.run = (ml_state_fn)ml_coro_resume;
		void *Stack = GC_memalign(STACK_SIZE, 16);
		coro_create(State->Context, (coro_func)ml_coro_start, State, Stack, STACK_SIZE);
	}
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	ml_coro_entry_t Entry = {Function, Args, Count};
	return ml_coro_resume(State, &Entry);
}

static void ml_cofunction_call(ml_state_t *Caller, ml_cfunction_t *Function, int Count, ml_value_t **Args) {
	for (int I = Count; --I >= 0;) Args[I] = ml_deref(Args[I]);
	return ml_coro_enter(Caller, Function->Callback, Count, Args);
}

ML_TYPE(MLCoFunctionT, (MLFunctionT), "co-function",
//!internal
	.call = (void *)ml_cofunction_call
);

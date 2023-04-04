#include "minilang.h"
#include "ml_coroutine.h"
#include "ml_macros.h"
#include "coro.h"

#define STACK_SIZE (1 << 20)

typedef struct ml_coro_state_t ml_coro_state_t;

struct ml_coro_state_t {
	ml_state_t Base;
	ml_coro_state_t *Next;
	ml_callback_t Function;
	ml_value_t *Value, **Args;
	int Count;
	coro_context Return[1];
	coro_context Context[1];
};

static
#ifdef ML_THREADS
__thread
#endif
ml_coro_state_t *CoroCache = NULL, *Current;

ml_value_t *ml_coro_call(ml_value_t *Function, int Count, ml_value_t **Args) {
	Current->Value = Function;
	Current->Count = Count;
	Current->Args = Args;
	coro_transfer(Current->Context, Current->Return);
	return Current->Value;
}

static void ml_coro_resume(ml_coro_state_t *State, ml_value_t *Value) {
	State->Value = Value;
	Current = State;
	coro_transfer(State->Return, State->Context);
}

static void ml_coro_start(ml_coro_state_t *State) {
	for (;;) {
		State->Value = State->Function(State, State->Count, State->Args);
		State->Count = -1;
		coro_transfer(State->Context, State->Return);
	}
}

void ml_coro_create(ml_state_t *Caller, ml_callback_t Function, int Count, ml_value_t **Args) {
	ml_coro_state_t *State = CoroCache;
	if (!State) {
		State = new(ml_coro_state_t);
		State->Base.run = (ml_state_fn)ml_coro_resume;
		void *Stack = GC_memalign(STACK_SIZE, 16);
		coro_create(State->Context, (coro_func)ml_coro_start, State, Stack, STACK_SIZE);
	}
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Function = Function;
	State->Count = Count;
	State->Args = Args;
	Current = State;
	coro_transfer(State->Return, State->Context);
	if (State->Count == -1) {
		State->Next = CoroCache;
		CoroCache = State;
		ML_RETURN(State->Value);
	}
	return ml_call(State, State->Value, State->Count, State->Args);
}

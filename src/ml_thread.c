#include "ml_thread.h"
#include "ml_macros.h"
#include "ml_compiler2.h"
#include <pthread.h>

typedef struct {
	ml_state_t Base;
	union {
		ml_value_t **Args;
		ml_value_t *Result;
	};
#ifdef ML_SCHEDULER
	ml_scheduler_queue_t Queue;
	uint64_t Counter;
#endif
	pthread_t Handle;
	int Count;
} ml_thread_t;

static int ML_THREAD_INDEX;

#ifdef ML_SCHEDULER

static void ml_thread_scheduler_queue_add(ml_state_t *State, ml_value_t *Value) {
	ml_thread_t *Thread = ml_context_get(State->Context, ML_THREAD_INDEX);
	ml_scheduler_queue_add(&Thread->Queue, State, Value);
}

static ml_schedule_t ml_thread_scheduler(ml_context_t *Context) {
	ml_thread_t *Thread = ml_context_get(Context, ML_THREAD_INDEX);
	return (ml_schedule_t){&Thread->Counter, (void *)ml_thread_scheduler_queue_add};
}

#endif

ml_value_t *ml_is_threadsafe(ml_value_t *Value) {
	typeof(ml_is_threadsafe) *function = ml_typed_fn_get(ml_typeof(Value), ml_is_threadsafe);
	if (function) return function(Value);
	return Value;
}

static ml_value_t *ML_TYPED_FN(ml_is_threadsafe, MLNilT, ml_value_t *Value) {
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_is_threadsafe, MLSomeT, ml_value_t *Value) {
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_is_threadsafe, MLNumberT, ml_value_t *Value) {
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_is_threadsafe, MLAddressT, ml_value_t *Value) {
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_is_threadsafe, MLMethodT, ml_value_t *Value) {
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_is_threadsafe, MLFunctionT, ml_value_t *Value) {
	return NULL;
}

static ml_value_t *ml_is_closure_threadsafe(ml_closure_info_t *Info) {
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Opcode == MLI_LINK) {
			Inst = Inst[1].Inst;
		} else {
			switch (MLInstTypes[Inst->Opcode]) {
			case MLIT_NONE: Inst += 1; break;
			case MLIT_INST: Inst += 2; break;
			case MLIT_INST_TYPES: Inst += 3; break;
			case MLIT_COUNT_COUNT: Inst += 3; break;
			case MLIT_COUNT: Inst += 2; break;
			case MLIT_VALUE: {
				ml_value_t *Value = ml_is_threadsafe(Inst[1].Value);
				if (Value) return Value;
				Inst += 2;
				break;
			}
			case MLIT_VALUE_DATA: {
				ml_value_t *Value = ml_is_threadsafe(Inst[1].Value);
				if (Value) return Value;
				Inst += 3;
				break;
			}
			case MLIT_VALUE_COUNT: {
				ml_value_t *Value = ml_is_threadsafe(Inst[1].Value);
				if (Value) return Value;
				Inst += 3;
				break;
			}
			case MLIT_VALUE_COUNT_DATA: {
				ml_value_t *Value = ml_is_threadsafe(Inst[1].Value);
				if (Value) return Value;
				Inst += 4;
				break;
			}
			case MLIT_COUNT_CHARS: Inst += 3; break;
			case MLIT_DECL: Inst += 2; break;
			case MLIT_COUNT_DECL: Inst += 3; break;
			case MLIT_COUNT_COUNT_DECL: Inst += 4; break;
			case MLIT_CLOSURE: {
				ml_value_t *Value = ml_is_closure_threadsafe(Inst[1].ClosureInfo);
				if (Value) return Value;
				Inst += 2 + Inst[1].ClosureInfo->NumUpValues;
				break;
			}
			case MLIT_SWITCH: Inst += 3; break;
			}
		}
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_is_threadsafe, MLClosureT, ml_closure_t *Closure) {
	ml_closure_info_t *Info = Closure->Info;
	for (int I = 0; I < Info->NumUpValues; ++I) {
		ml_value_t *Value = ml_is_threadsafe(Closure->UpValues[I]);
		if (Value) return Value;
	}
	return ml_is_closure_threadsafe(Info);
}

#ifdef ML_TIME

#include "ml_time.h"

static ml_value_t *ML_TYPED_FN(ml_is_threadsafe, MLTimeT, ml_value_t *Value) {
	return NULL;
}

#endif

#ifdef ML_UUID

#include "ml_uuid.h"

static ml_value_t *ML_TYPED_FN(ml_is_threadsafe, MLUUIDT, ml_value_t *Value) {
	return NULL;
}

#endif

static void *ml_thread_fn(ml_thread_t *Thread) {
	ml_value_t **Args = Thread->Args;
	int Count = Thread->Count;
	Thread->Args = NULL;
	Thread->Count = 0;
	ml_call((ml_state_t *)Thread, Args[Count - 1], Count - 1, Args);
	while (!Thread->Result) {
		ml_queued_state_t QueuedState = ml_scheduler_queue_next(&Thread->Queue);
		if (!QueuedState.State) break;
		Thread->Counter = 256;
		QueuedState.State->run(QueuedState.State, QueuedState.Value);
	}
	return Thread->Result;
}

static void ml_thread_run(ml_thread_t *Thread, ml_value_t *Result) {
	Thread->Result = Result;
}

ML_FUNCTIONX(MLThread) {
	ml_value_t **Args2 = anew(ml_value_t *, Count);
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Value = ml_is_threadsafe(Args[I]);
		if (Value) ML_ERROR("ThreadError", "%s is not thread safe", ml_typeof(Value)->Name);
		Args2[I] = Args[I];
	}
	ml_thread_t *Thread = new(ml_thread_t);
	Thread->Base.Type = MLThreadT;
	ml_context_t *Context = Thread->Base.Context = ml_context_new(Caller->Context);
	ml_context_set(Context, ML_THREAD_INDEX, Thread);
	Thread->Base.run = (ml_state_fn)ml_thread_run;
	Thread->Count = Count;
	Thread->Args = Args2;
#ifdef ML_SCHEDULER
	ml_scheduler_queue_init(&Thread->Queue, 8);
	Thread->Counter = 256;
	ml_context_set(Context, ML_SCHEDULER_INDEX, ml_thread_scheduler);
#endif
	pthread_create(&Thread->Handle, NULL, (void *)ml_thread_fn, Thread);
	ML_RETURN(Thread);
}

ML_TYPE(MLThreadT, (), "thread",
	.Constructor = (ml_value_t *)MLThread
);

ML_METHOD("join", MLThreadT) {
	ml_thread_t *Thread = (ml_thread_t *)Args[0];
	ml_value_t *Result;
	pthread_join(Thread->Handle, (void **)&Result);
	return Result ?: MLNil;
}

ML_FUNCTION(MLThreadSleep) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLNumberT);
	usleep(ml_real_value(Args[0]) * 1000000);
	return MLNil;
}

void ml_thread_init(stringmap_t *Globals) {
	ML_THREAD_INDEX = ml_context_index_new();
#include "ml_thread_init.c"
	stringmap_insert(MLThreadT->Exports, "sleep", MLThreadSleep);
	if (Globals) {
		stringmap_insert(Globals, "thread", MLThreadT);
	}
}

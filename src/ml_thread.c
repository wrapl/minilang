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
	return ml_error("ThreadError", "%s is not safe to pass to another thread", ml_typeof(Value)->Name);
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
				ml_value_t *Error = ml_is_threadsafe(Inst[1].Value);
				if (Error) {
					ml_error_trace_add(Error, (ml_source_t){Info->Source, Inst->Line});
					return Error;
				}
				Inst += 2;
				break;
			}
			case MLIT_VALUE_DATA: {
				ml_value_t *Error = ml_is_threadsafe(Inst[1].Value);
				if (Error) {
					ml_error_trace_add(Error, (ml_source_t){Info->Source, Inst->Line});
					return Error;
				}
				Inst += 3;
				break;
			}
			case MLIT_VALUE_COUNT: {
				ml_value_t *Error = ml_is_threadsafe(Inst[1].Value);
				if (Error) {
					ml_error_trace_add(Error, (ml_source_t){Info->Source, Inst->Line});
					return Error;
				}
				Inst += 3;
				break;
			}
			case MLIT_VALUE_COUNT_DATA: {
				ml_value_t *Error = ml_is_threadsafe(Inst[1].Value);
				if (Error) {
					ml_error_trace_add(Error, (ml_source_t){Info->Source, Inst->Line});
					return Error;
				}
				Inst += 4;
				break;
			}
			case MLIT_COUNT_CHARS: Inst += 3; break;
			case MLIT_DECL: Inst += 2; break;
			case MLIT_COUNT_DECL: Inst += 3; break;
			case MLIT_COUNT_COUNT_DECL: Inst += 4; break;
			case MLIT_CLOSURE: {
				ml_value_t *Error = ml_is_closure_threadsafe(Inst[1].ClosureInfo);
				if (Error) {
					ml_error_trace_add(Error, (ml_source_t){Info->Source, Inst->Line});
					return Error;
				}
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
		ml_value_t *Error = ml_is_threadsafe(Closure->UpValues[I]);
		if (Error) {
			int Index = ~I;
			for (ml_decl_t *Decl = Info->Decls; Decl; Decl = Decl->Next) {
				if (Decl->Index == Index) ml_error_trace_add(Error, Decl->Source);
			}
			return Error;
		}
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
//@thread
//<Args...:any
//<Fn:function
//>thread
// Creates a new thread and calls :mini:`Fn(Args...)` in the new thread.
// All arguments must be thread-safe.
	ml_value_t **Args2 = anew(ml_value_t *, Count);
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Error = ml_is_threadsafe(Args[I]);
		if (Error) ML_RETURN(Error);
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
// A thread.
	.Constructor = (ml_value_t *)MLThread
);

ML_METHOD("join", MLThreadT) {
//<Thread
//>any
// Waits until the thread :mini:`Thread` completes and returns its result.
	ml_thread_t *Thread = (ml_thread_t *)Args[0];
	ml_value_t *Result;
	pthread_join(Thread->Handle, (void **)&Result);
	return Result ?: MLNil;
}

ML_FUNCTION(MLThreadSleep) {
//@thread::sleep
//<Duration
//>nil
// Causes the current thread to sleep for :mini:`Duration` microseconds.
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLNumberT);
	usleep(ml_real_value(Args[0]) * 1000000);
	return MLNil;
}

typedef struct {
	ml_type_t *Type;
	int Size, Fill, Write, Read;
	ml_value_t *Messages[];
} ml_thread_channel_t;

extern ml_type_t MLThreadChannelT[];

ML_FUNCTION(MLThreadChannel) {
//@thread::channel
//<Capacity
//>thread::channel
// Creates a new channel with capacity :mini:`Capacity`.
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	int Size = ml_integer_value(Args[0]);
	ml_thread_channel_t *Channel = xnew(ml_thread_channel_t, Size, ml_value_t *);
	Channel->Type = MLThreadChannelT;
	Channel->Size = Size;
	return (ml_value_t *)Channel;
}

ML_TYPE(MLThreadChannelT, (), "thread-channel",
// A channel for thread communication.
	.Constructor = (ml_value_t *)MLThreadChannel
);

static ml_value_t *ML_TYPED_FN(ml_is_threadsafe, MLThreadChannelT, ml_value_t *Value) {
	return NULL;
}

static pthread_mutex_t MLThreadChannelLock[1] = {PTHREAD_MUTEX_INITIALIZER};
static pthread_cond_t MLThreadChannelWait[1] = {PTHREAD_COND_INITIALIZER};

ML_METHOD("send", MLThreadChannelT, MLAnyT) {
//<Channel
//<Message
//>thread::channel
// Adds :mini:`Message` to :mini:`Channel`. :mini:`Message` must be thread-safe.
// Blocks if :mini:`Channel` is currently full.
	ml_thread_channel_t *Channel = (ml_thread_channel_t *)Args[0];
	ml_value_t *Message = Args[1];
	ml_value_t *Error = ml_is_threadsafe(Message);
	if (Error) return Error;
	pthread_mutex_lock(MLThreadChannelLock);
	while (Channel->Fill == Channel->Size) pthread_cond_wait(MLThreadChannelWait, MLThreadChannelLock);
	++Channel->Fill;
	int Write = Channel->Write;
	Channel->Messages[Write] = Message;
	Channel->Write = (Write + 1) % Channel->Size;
	pthread_cond_broadcast(MLThreadChannelWait);
	pthread_mutex_unlock(MLThreadChannelLock);
	return (ml_value_t *)Channel;
}

ML_METHODV("recv", MLThreadChannelT) {
//<Channel/1
//>tuple[integer,any]
// Gets the next available message on any of :mini:`Channel/1, ..., Channel/n`, blocking if :mini:`Channel` is empty. Returns :mini:`(Index, Message)` where :mini:`Index = 1, ..., n`.
	for (int I = 1; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLThreadChannelT);
	ml_value_t *Result = ml_tuple(2);
	pthread_mutex_lock(MLThreadChannelLock);
	for (;;) {
		for (int I = 0; I < Count; ++I) {
			ml_thread_channel_t *Channel = (ml_thread_channel_t *)Args[I];
			if (Channel->Fill) {
				ml_value_t **Messages = Channel->Messages;
				int Read = Channel->Read;
				ml_value_t *Message = Messages[Read];
				Messages[Read] = NULL;
				--Channel->Fill;
				Channel->Read = (Read + 1) % Channel->Size;
				ml_tuple_set(Result, 1, ml_integer(I + 1));
				ml_tuple_set(Result, 2, Message);
				goto done;
			}
		}
		pthread_cond_wait(MLThreadChannelWait, MLThreadChannelLock);
	}
done:
	pthread_mutex_unlock(MLThreadChannelLock);
	return Result;
}

typedef struct {
	ml_type_t *Type;
	pthread_mutex_t Handle[1];
} ml_thread_mutex_t;

extern ml_type_t MLThreadMutexT[];

ML_FUNCTION(MLThreadMutex) {
//@thread::mutex
//>thread::mutex
// Creates a new mutex.
	ml_thread_mutex_t *Mutex = new(ml_thread_mutex_t);
	Mutex->Type = MLThreadMutexT;
	pthread_mutex_init(Mutex->Handle, NULL);
	return (ml_value_t *)Mutex;
}

ML_TYPE(MLThreadMutexT, (), "thread-mutex",
// A mutex.
	.Constructor = (ml_value_t *)MLThreadMutex
);

ML_METHOD("lock", MLThreadMutexT) {
//<Mutex
//>thread::mutex
// Locks :mini:`Mutex`.
	ml_thread_mutex_t *Mutex = (ml_thread_mutex_t *)Args[0];
	pthread_mutex_lock(Mutex->Handle);
	return (ml_value_t *)Mutex;
}

ML_METHOD("unlock", MLThreadMutexT) {
//<Mutex
//>thread::mutex
// Unlocks :mini:`Mutex`.
	ml_thread_mutex_t *Mutex = (ml_thread_mutex_t *)Args[0];
	pthread_mutex_unlock(Mutex->Handle);
	return (ml_value_t *)Mutex;
}

static ml_value_t *ML_TYPED_FN(ml_is_threadsafe, MLThreadMutexT, ml_value_t *Value) {
	return NULL;
}

typedef struct {
	ml_type_t *Type;
	pthread_cond_t Handle[1];
} ml_thread_condition_t;

extern ml_type_t MLThreadConditionT[];

ML_FUNCTION(MLThreadCondition) {
//@thread::condition
//>thread::condition
// Creates a new condition.
	ml_thread_condition_t *Condition = new(ml_thread_condition_t);
	Condition->Type = MLThreadConditionT;
	pthread_cond_init(Condition->Handle, NULL);
	return (ml_value_t *)Condition;
}

ML_TYPE(MLThreadConditionT, (), "thread-condition",
// A condition.
	.Constructor = (ml_value_t *)MLThreadCondition
);

ML_METHOD("wait", MLThreadConditionT, MLThreadMutexT) {
//<Condition
//<Mutex
//>thread::condition
// Waits for a signal on :mini:`Condition`, using :mini:`Mutex` for synchronization.
	ml_thread_condition_t *Condition = (ml_thread_condition_t *)Args[0];
	ml_thread_mutex_t *Mutex = (ml_thread_mutex_t *)Args[1];
	pthread_cond_wait(Condition->Handle, Mutex->Handle);
	return (ml_value_t *)Condition;
}

ML_METHOD("signal", MLThreadConditionT) {
//<Condition
//>thread::condition
// Signals a single thread waiting on :mini:`Condition`.
	ml_thread_condition_t *Condition = (ml_thread_condition_t *)Args[0];
	pthread_cond_signal(Condition->Handle);
	return (ml_value_t *)Condition;
}

ML_METHOD("broadcast", MLThreadConditionT) {
//<Condition
//>thread::condition
// Signals all threads waiting on :mini:`Condition`.
	ml_thread_condition_t *Condition = (ml_thread_condition_t *)Args[0];
	pthread_cond_broadcast(Condition->Handle);
	return (ml_value_t *)Condition;
}

static ml_value_t *ML_TYPED_FN(ml_is_threadsafe, MLThreadConditionT, ml_value_t *Value) {
	return NULL;
}

void ml_thread_init(stringmap_t *Globals) {
	ML_THREAD_INDEX = ml_context_index_new();
#include "ml_thread_init.c"
	stringmap_insert(MLThreadT->Exports, "sleep", MLThreadSleep);
	stringmap_insert(MLThreadT->Exports, "channel", MLThreadChannelT);
	stringmap_insert(MLThreadT->Exports, "mutex", MLThreadMutexT);
	stringmap_insert(MLThreadT->Exports, "condition", MLThreadConditionT);
	if (Globals) {
		stringmap_insert(Globals, "thread", MLThreadT);
	}
}

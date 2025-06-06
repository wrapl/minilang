#include "ml_tasks.h"
#include "minilang.h"
#include "ml_macros.h"
#include "ml_logging.h"

#undef ML_CATEGORY
#define ML_CATEGORY "tasks"

typedef struct ml_waiter_t ml_waiter_t;

struct ml_waiter_t {
	ml_waiter_t *Next;
	ml_state_t *State;
};

typedef struct {
	ml_state_t Base;
	ml_value_t *Value;
	ml_waiter_t *Waiters;
} ml_task_t;

static void ml_task_set(ml_task_t *Task, ml_value_t *Value);

static void ml_task_call(ml_state_t *Caller, ml_task_t *Task, int Count, ml_value_t **Args) {
	if (Task->Value) ML_RETURN(Task->Value);
	if (!Task->Base.Caller) {
		Task->Base.Caller = Caller;
	} else {
		ml_waiter_t *Waiter = new(ml_waiter_t);
		Waiter->Next = Task->Waiters;
		Waiter->State = Caller;
		Task->Waiters = Waiter;
	}
}

ML_TYPE(MLTaskT, (MLFunctionT), "task",
// A task representing a value that will eventually be completed.
	.call = (void *)ml_task_call
);

static void ml_task_set(ml_task_t *Task, ml_value_t *Value) {
	Task->Value = Value;
	ml_state_t *Caller = Task->Base.Caller;
	if (Caller) {
		for (ml_waiter_t *Waiter = Task->Waiters; Waiter; Waiter = Waiter->Next) {
			ml_state_schedule(Waiter->State, Value);
		}
		ml_state_schedule(Caller, Value);
	}
}

ML_METHOD(MLTaskT) {
//>task
// Returns a task. The task should eventually be completed with :mini:`Task:done()` or :mini:`Task:error()`.
	ml_task_t *Task = new(ml_task_t);
	Task->Base.Type = MLTaskT;
	return (ml_value_t *)Task;
}

static void ml_task_run(ml_task_t *Task, ml_value_t *Result) {
	if (!Task->Value) ml_task_set(Task, Result);
}

ML_METHODVZ(MLTaskT, MLAnyT) {
//<Arg/1...
//<Arg/n:any
//<Fn:function
//>task
// Returns a task which calls :mini:`Fn(Arg/1, ..., Arg/n)`.
	ml_value_t *Fn = ml_deref(Args[Count - 1]);
	if (!ml_is(Fn, MLFunctionT)) ML_ERROR("TypeError", "expected function for argument %d", Count);
	ml_task_t *Task = new(ml_task_t);
	Task->Base.Type = MLTaskT;
	Task->Base.Context = Caller->Context;
	Task->Base.run = (ml_state_fn)ml_task_run;
	// Register Caller as waiting for Task here
	ml_call(Task, Fn, Count - 1, Args);
	// Unregister Caller as waiting for Task here
	ML_RETURN(Task);
}

ML_METHODX("wait", MLTaskT) {
//<Task
//>any|error
// Waits until :mini:`Task` is completed and returns its result.
	ml_task_t *Task = (ml_task_t *)Args[0];
	return ml_task_call(Caller, Task, 0, NULL);
}

#ifdef ML_GENERICS

ML_GENERIC_TYPE(MLTaskListT, MLListT, MLTaskT);

typedef struct {
	ml_state_t Base;
	int Remaining;
} ml_task_list_t;

static void ml_task_list_run(ml_task_list_t *TaskList, ml_value_t *Value) {
	if (!TaskList->Remaining) return;
	if (ml_is_error(Value)) {
		TaskList->Remaining = 0;
		ML_CONTINUE(TaskList->Base.Caller, Value);
	} else if (--TaskList->Remaining == 0) {
		ML_CONTINUE(TaskList->Base.Caller, Value);
	}
}

ML_METHODX("wait", MLTaskListT) {
//<Tasks
//>any|error
// Waits until all the tasks in :mini:`Tasks` are completed or any task returns an error.
	ml_task_list_t *TaskList = new(ml_task_list_t);
	TaskList->Base.Caller = Caller;
	TaskList->Base.Context = Caller->Context;
	TaskList->Base.run = (ml_state_fn)ml_task_list_run;
	TaskList->Remaining = ml_list_length(Args[0]);
	ML_LIST_FOREACH(Args[0], Iter) {
		if (!ml_is(Iter->Value, MLTaskT)) ML_ERROR("TypeError", "Expected task not %s", ml_typeof(Iter->Value)->Name);
		ml_task_call((ml_state_t *)TaskList, (ml_task_t *)Iter->Value, 0, NULL);
	}
}

#endif

typedef struct {
	ml_task_t Base;
	int Remaining, Count;
	ml_task_t *Tasks[];
} ml_task_set_t;

static void ml_task_all_run(ml_task_set_t *TaskSet, ml_value_t *Value) {
	if (!TaskSet->Remaining) return;
	if (ml_is_error(Value)) {
		TaskSet->Remaining = 0;
		ml_task_set((ml_task_t *)TaskSet, Value);
	} else if (--TaskSet->Remaining) {
		ml_task_set((ml_task_t *)TaskSet, Value);
	}
}

ML_TYPE(MLTaskSetT, (MLTaskT), "task::set");
// A task combining a set of sub tasks.

ML_METHODX("*", MLTaskT, MLTaskT) {
//<Task/1
//<Task/2
//>task::set
// Returns a :mini:`task::set` that completes when all of its sub tasks complete, or any raises an error.
	ML_CHECKX_ARG_COUNT(1);
	for (int I = 0; I < Count; ++I) ML_CHECKX_ARG_TYPE(I, MLTaskT);
	ml_task_set_t *TaskSet = xnew(ml_task_set_t, Count, ml_task_t *);
	TaskSet->Base.Base.Type = MLTaskSetT;
	TaskSet->Base.Base.Context = Caller->Context;
	TaskSet->Base.Base.run = (ml_state_fn)ml_task_all_run;
	TaskSet->Remaining = TaskSet->Count = Count;
	for (int I = 0; I < Count; ++I) {
		ml_task_t *Task = (ml_task_t *)Args[I];
		ml_task_call((ml_state_t *)TaskSet, Task, 0, NULL);
		TaskSet->Tasks[I] = Task;
	}
	ML_RETURN(TaskSet);
}

static void ml_task_any_run(ml_task_set_t *TaskSet, ml_value_t *Value) {
	if (!TaskSet->Remaining) return;
	TaskSet->Remaining = 0;
	ml_task_set((ml_task_t *)TaskSet, Value);
}

ML_METHODX("+", MLTaskT, MLTaskT) {
//<Task/1
//<Task/2
//>task::set
// Returns a :mini:`task::set` that completes when any of its sub tasks complete, or any raises an error.
	ML_CHECKX_ARG_COUNT(1);
	for (int I = 0; I < Count; ++I) ML_CHECKX_ARG_TYPE(I, MLTaskT);
	ml_task_set_t *TaskSet = xnew(ml_task_set_t, Count, ml_task_t *);
	TaskSet->Base.Base.Type = MLTaskSetT;
	TaskSet->Base.Base.Context = Caller->Context;
	TaskSet->Base.Base.run = (ml_state_fn)ml_task_any_run;
	TaskSet->Remaining = TaskSet->Count = Count;
	for (int I = 0; I < Count; ++I) {
		ml_task_t *Task = (ml_task_t *)Args[I];
		ml_task_call((ml_state_t *)TaskSet, Task, 0, NULL);
		TaskSet->Tasks[I] = Task;
	}
	ML_RETURN(TaskSet);
}

ML_METHOD("done", MLTaskT, MLAnyT) {
//<Task
//<Result
//>any|error
// Completes :mini:`Task` with :mini:`Result`, resuming any waiting code. Raises an error if :mini:`Task` is already complete.
	ml_task_t *Task = (ml_task_t *)Args[0];
	if (Task->Value) return ml_error("TaskError", "Task value already set");
	ml_task_set(Task, Args[1]);
	return Args[1];
}

ML_METHOD("error", MLTaskT, MLStringT, MLStringT) {
//<Task
//<Type
//<Message
//>any|error
// Completes :mini:`Task` with an :mini:`error(Type, Message)`, resuming any waiting code. Raises an error if :mini:`Task` is already complete.
	ml_task_t *Task = (ml_task_t *)Args[0];
	if (Task->Value) return ml_error("TaskError", "Task value already set");
	ml_task_set(Task, ml_error(ml_string_value(Args[1]), "%s", ml_string_value(Args[2])));
	return MLNil;
}

typedef struct {
	ml_task_t Base;
	ml_value_t *Then, *Else, *On;
	ml_value_t *Args[1];
} ml_task_composed_t;

static void ml_task_composed_run(ml_task_composed_t *Composed, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		if (Composed->On) {
			Composed->Args[0] = ml_error_unwrap(Value);
			Composed->Base.Base.run = (ml_state_fn)ml_task_run;
			return ml_call((ml_state_t *)Composed, Composed->On, 1, Composed->Args);
		}
	} else if (Value == MLNil) {
		if (Composed->Else) {
			Composed->Args[0] = Value;
			Composed->Base.Base.run = (ml_state_fn)ml_task_run;
			return ml_call((ml_state_t *)Composed, Composed->Else, 1, Composed->Args);
		}
	} else if (!Composed->Else) {
		if (Composed->Then) {
			Composed->Args[0] = Value;
			Composed->Base.Base.run = (ml_state_fn)ml_task_run;
			return ml_call((ml_state_t *)Composed, Composed->Then, 1, Composed->Args);
		}
	}
	ml_task_set((ml_task_t *)Composed, Value);
}

ML_METHODX("then", MLFunctionT, MLFunctionT) {
//<Fn
//<Then
//>task
// Equivalent to :mini:`task(Fn, call -> Then)`.
	ml_task_composed_t *Composed = new(ml_task_composed_t);
	Composed->Base.Base.Type = MLTaskT;
	Composed->Base.Base.Context = Caller->Context;
	Composed->Base.Base.run = (ml_state_fn)ml_task_composed_run;
	Composed->Then = Args[1];
	// Register Caller as waiting for Composed here
	ml_call((ml_state_t *)Composed, Args[0], 0, NULL);
	// Unregister Caller as waiting for Composed here
	ML_RETURN(Composed);
}

ML_METHODX("then", MLFunctionT, MLFunctionT, MLFunctionT) {
//<Fn
//<Then
//<Else
//>task
	ml_task_composed_t *Composed = new(ml_task_composed_t);
	Composed->Base.Base.Type = MLTaskT;
	Composed->Base.Base.Context = Caller->Context;
	Composed->Base.Base.run = (ml_state_fn)ml_task_composed_run;
	Composed->Then = Args[1];
	Composed->Else = Args[2];
	// Register Caller as waiting for Composed here
	ml_call((ml_state_t *)Composed, Args[0], 0, NULL);
	// Unregister Caller as waiting for Composed here
	ML_RETURN(Composed);
}

ML_METHODX("else", MLFunctionT, MLFunctionT) {
//<Fn
//<Else
//>task
	ml_task_composed_t *Composed = new(ml_task_composed_t);
	Composed->Base.Base.Type = MLTaskT;
	Composed->Base.Base.Context = Caller->Context;
	Composed->Base.Base.run = (ml_state_fn)ml_task_composed_run;
	Composed->Else = Args[1];
	// Register Caller as waiting for Composed here
	ml_call((ml_state_t *)Composed, Args[0], 0, NULL);
	// Unregister Caller as waiting for Composed here
	ML_RETURN(Composed);
}

ML_METHODX("on", MLFunctionT, MLFunctionT) {
//<Fn
//<On
//>task
	ml_task_composed_t *Composed = new(ml_task_composed_t);
	Composed->Base.Base.Type = MLTaskT;
	Composed->Base.Base.Context = Caller->Context;
	Composed->Base.Base.run = (ml_state_fn)ml_task_composed_run;
	Composed->On = Args[1];
	// Register Caller as waiting for Composed here
	ml_call((ml_state_t *)Composed, Args[0], 0, NULL);
	// Unregister Caller as waiting for Composed here
	ML_RETURN(Composed);
}

typedef struct ml_task_pending_t ml_task_pending_t;

struct ml_task_pending_t {
	ml_task_pending_t *Next;
	ml_task_t *Task;
	ml_value_t *Fn;
	int Count;
	ml_value_t *Args[];
};

typedef struct {
	ml_state_t Base;
	ml_task_pending_t *Head, *Tail;
	ml_state_t Empty[1];
	ml_value_t *Callback;
	int NumRunning, MaxRunning;
	int NumPending, MaxPending;
} ml_task_queue_t;

static void ml_task_queue_call(ml_state_t *Caller, ml_task_queue_t *Queue, int Count, ml_value_t **Args) {
	ml_value_t *Fn = ml_deref(Args[Count - 1]);
	if (!ml_is(Fn, MLFunctionT)) ML_ERROR("TypeError", "expected function for argument %d", Count);
	ml_task_t *Task = new(ml_task_t);
	Task->Base.Type = MLTaskT;
	Task->Base.Caller = (ml_state_t *)Queue;
	Task->Base.Context = Caller->Context;
	Task->Base.run = (ml_state_fn)ml_task_run;
	if (Queue->NumRunning < Queue->MaxRunning) {
		++Queue->NumRunning;
		// Register Caller as waiting for Task here
		ml_call(Task, Fn, Count - 1, Args);
		// Unregister Caller as waiting for Task here
	} else {
		ml_task_pending_t *Pending = xnew(ml_task_pending_t, Count, ml_value_t *);
		Pending->Task = Task;
		Pending->Fn = Fn;
		Pending->Count = Count - 1;
		for (int I = 0; I < Count - 1; ++I) Pending->Args[I] = Args[I];
		if (Queue->Tail) Queue->Tail->Next = Pending; else Queue->Head = Pending;
		Queue->Tail = Pending;
	}
	ML_RETURN(Task);
}

ML_TYPE(MLTaskQueueT, (MLFunctionT), "task::queue",
// A queue of tasks that can run a limited number of tasks at once.
//
// :mini:`fun (Queue: task::queue)(Arg/1, ..., Arg/n, Fn): task`
//    Returns a new task that calls :mini:`Fn(Arg/1, ..., Arg/n)`. The task will be delayed if :mini:`Queue` has reached its limit.
	.call = (void *)ml_task_queue_call
);

static void ml_task_queue_run(ml_task_queue_t *Queue, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_LOG_ERROR(Value, "Task returned error");
	ml_task_pending_t *Pending = Queue->Head;
	if (Pending) {
		if (!(Queue->Head = Pending->Next)) Queue->Tail = NULL;
		return ml_call(Pending->Task, Pending->Fn, Pending->Count, Pending->Args);
	} else if (--Queue->NumRunning == 0) {
		if (Queue->Callback) {
			ml_value_t **Args = ml_alloc_args(1);
			Args[0] = (ml_value_t *)Queue;
			return ml_call(Queue->Empty, Queue->Callback, 1, Args);
		}
	}
}

static void ml_task_queue_empty(ml_task_queue_t *Queue, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_LOG_ERROR(Value, "Empty callback returned error");
}

ML_METHODX(MLTaskQueueT, MLIntegerT) {
//@task::queue
//<MaxRunning
//>task::queue
// Returns a new task queue which runs at most :mini:`MaxRunning` tasks at a time.
	ml_task_queue_t *Queue = new(ml_task_queue_t);
	Queue->Base.Type = MLTaskQueueT;
	Queue->Base.Context = Caller->Context;
	Queue->Base.run = (ml_state_fn)ml_task_queue_run;
	Queue->MaxRunning = ml_integer_value(Args[0]);
	Queue->NumRunning = 0;
	ML_RETURN(Queue);
}

ML_METHODX(MLTaskQueueT, MLIntegerT, MLFunctionT) {
//@task::queue
//<MaxRunning
//<Callback
//>task::queue
// Returns a new task queue which runs at most :mini:`MaxRunning` tasks at a time.
	ml_task_queue_t *Queue = new(ml_task_queue_t);
	Queue->Base.Type = MLTaskQueueT;
	Queue->Base.Context = Caller->Context;
	Queue->Base.run = (ml_state_fn)ml_task_queue_run;
	Queue->MaxRunning = ml_integer_value(Args[0]);
	Queue->NumRunning = 0;
	Queue->Empty->Context = Caller->Context;
	Queue->Empty->run = (ml_state_fn)ml_task_queue_empty;
	Queue->Callback = Args[1];
	ML_RETURN(Queue);
}

ML_METHOD("cancel", MLTaskQueueT) {
	ml_task_queue_t *Queue = (ml_task_queue_t *)Args[0];
	Queue->Head = Queue->Tail = NULL;
	return (ml_value_t *)Queue;
}

typedef struct ml_parallel_iter_t ml_parallel_iter_t;

typedef struct {
	ml_state_t Base;
	ml_state_t NextState[1];
	ml_state_t KeyState[1];
	ml_state_t ValueState[1];
	ml_value_t *Iter, *Fn, *Error;
	ml_value_t *Args[2];
	size_t NumRunning, MaxRunning, Burst;
} ml_parallel_t;

static void parallel_iter_next(ml_state_t *State, ml_value_t *Iter) {
	ml_parallel_t *Parallel = (ml_parallel_t *)((char *)State - offsetof(ml_parallel_t, NextState));
	if (Parallel->Error) return;
	if (Iter == MLNil) {
		Parallel->Iter = NULL;
		ML_CONTINUE(Parallel, MLNil);
	}
	if (ml_is_error(Iter)) {
		Parallel->Error = Iter;
		ML_CONTINUE(Parallel->Base.Caller, Iter);
	}
	return ml_iter_key(Parallel->KeyState, Parallel->Iter = Iter);
}

static void parallel_iter_key(ml_state_t *State, ml_value_t *Value) {
	ml_parallel_t *Parallel = (ml_parallel_t *)((char *)State - offsetof(ml_parallel_t, KeyState));
	if (Parallel->Error) return;
	Parallel->Args[0] = Value;
	return ml_iter_value(Parallel->ValueState, Parallel->Iter);
}

static void parallel_iter_value(ml_state_t *State, ml_value_t *Value) {
	ml_parallel_t *Parallel = (ml_parallel_t *)((char *)State - offsetof(ml_parallel_t, ValueState));
	if (Parallel->Error) return;
	Parallel->Args[1] = Value;
	ml_call(Parallel, Parallel->Fn, 2, Parallel->Args);
	if (Parallel->Iter) {
		if (Parallel->NumRunning > Parallel->MaxRunning) return;
		++Parallel->NumRunning;
		return ml_iter_next(Parallel->NextState, Parallel->Iter);
	}
}

static void parallel_continue(ml_parallel_t *Parallel, ml_value_t *Value) {
	if (Parallel->Error) return;
	if (ml_is_error(Value)) {
		Parallel->Error = Value;
		ML_CONTINUE(Parallel->Base.Caller, Value);
	}
	--Parallel->NumRunning;
	if (Parallel->Iter) {
		if (Parallel->NumRunning > Parallel->Burst) return;
		++Parallel->NumRunning;
		return ml_iter_next(Parallel->NextState, Parallel->Iter);
	}
	if (Parallel->NumRunning == 0) ML_CONTINUE(Parallel->Base.Caller, MLNil);
}

ML_FUNCTIONX(Parallel) {
//<Sequence
//<Max?:integer
//<Min?:integer
//<Fn:function
//>nil | error
// Iterates through :mini:`Sequence` and calls :mini:`Fn(Key, Value)` for each :mini:`Key, Value` pair produced **without** waiting for the call to return.
// The call to :mini:`parallel` returns when all calls to :mini:`Fn` return, or an error occurs.
// If :mini:`Max` is given, at most :mini:`Max` calls to :mini:`Fn` will run at a time by pausing iteration through :mini:`Sequence`.
// If :mini:`Min` is also given then iteration will be resumed only when the number of calls to :mini:`Fn` drops to :mini:`Min`.
	ML_CHECKX_ARG_COUNT(2);

	ml_parallel_t *Parallel = new(ml_parallel_t);
	Parallel->Base.Caller = Caller;
	Parallel->Base.run = (void *)parallel_continue;
	Parallel->Base.Context = Caller->Context;
	Parallel->NumRunning = 1;
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
		Parallel->MaxRunning = ml_integer_value(Args[2]);
		Parallel->Burst = ml_integer_value(Args[1]) + 1;
		Parallel->Fn = Args[3];
	} else if (Count > 2) {
		ML_CHECKX_ARG_TYPE(1, MLIntegerT);
		ML_CHECKX_ARG_TYPE(2, MLFunctionT);
		Parallel->MaxRunning = ml_integer_value(Args[1]);
		Parallel->Burst = SIZE_MAX;
		Parallel->Fn = Args[2];
	} else {
		ML_CHECKX_ARG_TYPE(1, MLFunctionT);
		Parallel->MaxRunning = SIZE_MAX;
		Parallel->Burst = SIZE_MAX;
		Parallel->Fn = Args[1];
	}

	return ml_iterate(Parallel->NextState, Args[0]);
}

static ML_METHOD_DECL(CountMethod, "count");

typedef struct {
	ml_type_t *Type;
	ml_value_t *Iter, *Fn;
	int Size;
} ml_buffered_t;

ML_TYPE(MLBufferedT, (MLSequenceT), "buffered");
//!internal

typedef struct {
	ml_state_t Base;
	ml_value_t *Key, *Value;
} ml_buffered_entry_t;

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter, *Fn;
	ml_value_t *Key, *Value;
	int Size, Use, Fetch, Head;
	ml_buffered_entry_t Entries[];
} ml_buffered_state_t;

ML_TYPE(MLBufferedStateT, (MLStateT), "buffered-state");
//!internal

static void ml_buffered_iterate(ml_buffered_state_t *State, ml_value_t *Value);

static void ml_buffered_call(ml_state_t *Caller, ml_buffered_state_t *State, ml_buffered_entry_t *Entry) {
	if (!Entry->Key) {
		ML_CONTINUE(Caller, Entry->Value);
	} else {
		State->Key = Entry->Key;
		State->Value = Entry->Value;
		Entry->Key = Entry->Value = NULL;
		++State->Use;
		State->Base.Caller = NULL;
		if (State->Head) {
			State->Head = 0;
			State->Base.run = (ml_state_fn)ml_buffered_iterate;
			ml_iter_next((ml_state_t *)State, State->Iter);
		}
		ML_CONTINUE(Caller, State);
	}
}

static void ML_TYPED_FN(ml_iter_next, MLBufferedStateT, ml_state_t *Caller, ml_buffered_state_t *State) {
	ml_buffered_entry_t *Entry = State->Entries + (State->Use % State->Size);
	if (Entry->Value) {
		return ml_buffered_call(Caller, State, Entry);
	} else {
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
	}
}

static void ML_TYPED_FN(ml_iter_key, MLBufferedStateT, ml_state_t *Caller, ml_buffered_state_t *State) {
	ML_RETURN(State->Key);
}

static void ML_TYPED_FN(ml_iter_value, MLBufferedStateT, ml_state_t *Caller, ml_buffered_state_t *State) {
	ML_RETURN(State->Value);
}

static void ml_buffered_entry_call(ml_buffered_entry_t *Entry, ml_value_t *Value) {
	ml_buffered_state_t *State = (ml_buffered_state_t *)Entry->Base.Caller;
	int Index = Entry - State->Entries;
	Entry->Value = Value;
	ml_state_t *Caller = State->Base.Caller;
	if (Caller && (Index == (State->Use % State->Size))) {
		return ml_buffered_call(Caller, State, Entry);
	}
}

static void ml_buffered_value(ml_buffered_state_t *State, ml_value_t *Value) {
	ml_buffered_entry_t *Entry = &State->Entries[State->Fetch % State->Size];
	if (ml_is_error(Value)) {
		++State->Fetch;
		Entry->Key = NULL;
		ml_buffered_entry_call(Entry, Value);
	} else {
		State->Head = 1;
		++State->Fetch;
		ml_value_t **Args = ml_alloc_args(2);
		Args[0] = Entry->Key;
		Args[1] = Value;
		ml_call((ml_state_t *)Entry, State->Fn, 2, Args);
		if (State->Fetch - State->Use < State->Size) {
			if (State->Head) {
				State->Head = 0;
				State->Base.run = (ml_state_fn)ml_buffered_iterate;
				ml_iter_next((ml_state_t *)State, State->Iter);
			}
		}
	}
}

static void ml_buffered_key(ml_buffered_state_t *State, ml_value_t *Value) {
	ml_buffered_entry_t *Entry = &State->Entries[State->Fetch % State->Size];
	if (ml_is_error(Value)) {
		++State->Fetch;
		Entry->Key = NULL;
		ml_buffered_entry_call(Entry, Value);
	} else {
		Entry->Key = Value;
		State->Base.run = (ml_state_fn)ml_buffered_value;
		ml_iter_value((ml_state_t *)State, State->Iter);
	}
}

static void ml_buffered_iterate(ml_buffered_state_t *State, ml_value_t *Value) {
	ml_buffered_entry_t *Entry = &State->Entries[State->Fetch % State->Size];
	if (ml_is_error(Value)) {
		++State->Fetch;
		Entry->Key = NULL;
		ml_buffered_entry_call(Entry, Value);
	} else if (Value == MLNil) {
		++State->Fetch;
		Entry->Key = NULL;
		ml_buffered_entry_call(Entry, Value);
	} else {
		State->Base.run = (ml_state_fn)ml_buffered_key;
		ml_iter_key((ml_state_t *)State, State->Iter = Value);
	}
}

static void ML_TYPED_FN(ml_iterate, MLBufferedT, ml_state_t *Caller, ml_buffered_t *Buffered) {
	ml_buffered_state_t *State = xnew(ml_buffered_state_t, Buffered->Size, ml_buffered_entry_t);
	State->Base.Type = MLBufferedStateT;
	State->Base.run = (void *)ml_buffered_iterate;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Fn = Buffered->Fn;
	State->Size = Buffered->Size;
	State->Use = State->Fetch = 0;
	for (int I = 0; I < State->Size; ++I) {
		State->Entries[I].Base.Caller = (ml_state_t *)State;
		State->Entries[I].Base.Context = Caller->Context;
		State->Entries[I].Base.run = (ml_state_fn)ml_buffered_entry_call;
	}
	return ml_iterate((ml_state_t *)State, Buffered->Iter);
}

ML_FUNCTION(Buffered) {
//<Sequence:sequence
//<Size:integer
//<Fn:function
//>sequence
// Returns the sequence :mini:`(K/i, Fn(K/i, V/i))` where :mini:`K/i, V/i` are the keys and values produced by :mini:`Sequence`. The calls to :mini:`Fn` are done in parallel, with at most :mini:`Size` calls at a time. The original sequence order is preserved (using an internal buffer).
//$= list(buffered(1 .. 10, 5, tuple))
	ML_CHECK_ARG_COUNT(3);
	ML_CHECK_ARG_TYPE(1, MLIntegerT);
	ML_CHECK_ARG_TYPE(2, MLFunctionT);
	int Size = ml_integer_value(Args[1]);
	if (Size <= 0 || Size > 1024) return ml_error("RangeError", "Buffered size out of range");
	ml_buffered_t *Buffered = new(ml_buffered_t);
	Buffered->Type = MLBufferedT;
	Buffered->Size = Size;
	Buffered->Iter = Args[0];
	Buffered->Fn = Args[2];
	return (ml_value_t *)Buffered;
}

ML_METHODX("count", MLBufferedT) {
//!internal
	ml_buffered_t *Buffered = (ml_buffered_t *)Args[0];
	Args[0] = Buffered->Iter;
	return ml_call(Caller, CountMethod, 1, Args);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Iter, *Fn;
	int Size;
} ml_diffused_t;

ML_TYPE(MLDiffusedT, (MLSequenceT), "diffused");
//!internal

typedef struct ml_diffused_task_t ml_diffused_task_t;

struct ml_diffused_task_t {
	ml_state_t Base;
	ml_diffused_task_t *Next;
	ml_value_t *Key, *Value;
};

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter, *Fn, *Final;
	ml_diffused_task_t *Head, *Tail, *Next;
	int Size, Waiting;
	ml_diffused_task_t Tasks[];
} ml_diffused_state_t;

ML_TYPE(MLDiffusedStateT, (MLStateT), "diffused-state");
//!internal

static void ml_diffused_iterate(ml_diffused_state_t *Task, ml_value_t *Value);

static void ML_TYPED_FN(ml_iter_next, MLDiffusedStateT, ml_state_t *Caller, ml_diffused_state_t *State) {
	if (State->Waiting <= 0) ML_RETURN(State->Final);
	if (!State->Head) ML_ERROR("StateError", "Invalid state");
	ml_diffused_task_t *Task = State->Head;
	--State->Waiting;
	State->Head = Task->Next;
	if (!State->Head) State->Tail = NULL;
	if (State->Final) {
		if (State->Waiting <= 0) ML_RETURN(State->Final);
	} else if (State->Next) {
		Task->Next = State->Next->Next;
		State->Next->Next = Task;
	} else {
		Task->Next = NULL;
		State->Next = Task;
		State->Base.run = (ml_state_fn)ml_diffused_iterate;
		ml_iter_next((ml_state_t *)State, State->Iter);
	}
	if (State->Head) {
		ML_RETURN(State);
	} else {
		State->Base.Caller = Caller;
	}
}

static void ML_TYPED_FN(ml_iter_key, MLDiffusedStateT, ml_state_t *Caller, ml_diffused_state_t *State) {
	if (!State->Head) ML_ERROR("StateError", "Invalid state");
	ML_RETURN(State->Head->Key);
}

static void ML_TYPED_FN(ml_iter_value, MLDiffusedStateT, ml_state_t *Caller, ml_diffused_state_t *State) {
	if (!State->Head) ML_ERROR("StateError", "Invalid state");
	ML_RETURN(State->Head->Value);
}

static void ml_diffused_task_call(ml_diffused_task_t *Task, ml_value_t *Value) {
	ml_diffused_state_t *State = (ml_diffused_state_t *)Task->Base.Caller;
	Task->Value = Value;
	if (ml_is_error(Value)) {
		State->Final = Value;
		State->Waiting = 0;
		ml_state_t *Caller = State->Base.Caller;
		if (Caller) {
			State->Base.Caller = NULL;
			ML_RETURN(Value);
		}
	} else if (State->Tail) {
		State->Tail->Next = Task;
		State->Tail = Task;
	} else {
		State->Head = State->Tail = Task;
		ml_state_t *Caller = State->Base.Caller;
		if (Caller) {
			State->Base.Caller = NULL;
			ML_RETURN(State);
		}
	}
}

static void ml_diffused_value(ml_diffused_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		State->Final = Value;
		State->Waiting = 0;
		ml_state_t *Caller = State->Base.Caller;
		if (Caller) {
			State->Base.Caller = NULL;
			ML_RETURN(Value);
		}
	} else {
		ml_diffused_task_t *Task = State->Next;
		State->Next = Task->Next;
		Task->Next = NULL;
		ml_value_t **Args = ml_alloc_args(2);
		Args[0] = Task->Key;
		Args[1] = Value;
		++State->Waiting;
		ml_call((ml_state_t *)Task, State->Fn, 2, Args);
		if (!State->Final && State->Next) {
			State->Base.run = (ml_state_fn)ml_diffused_iterate;
			ml_iter_next((ml_state_t *)State, State->Iter);
		}
	}
}

static void ml_diffused_key(ml_diffused_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		State->Final = Value;
		State->Waiting = 0;
		ml_state_t *Caller = State->Base.Caller;
		if (Caller) {
			State->Base.Caller = NULL;
			ML_RETURN(Value);
		}
	} else {
		State->Next->Key = Value;
		State->Base.run = (ml_state_fn)ml_diffused_value;
		ml_iter_value((ml_state_t *)State, State->Iter);
	}
}

static void ml_diffused_iterate(ml_diffused_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		State->Final = Value;
		State->Waiting = 0;
		ml_state_t *Caller = State->Base.Caller;
		if (Caller) {
			State->Base.Caller = NULL;
			ML_RETURN(Value);
		}
	} else if (Value == MLNil) {
		State->Final = Value;
		if (State->Waiting == 0) {
			ml_state_t *Caller = State->Base.Caller;
			if (Caller) {
				State->Base.Caller = NULL;
				ML_RETURN(Value);
			}
		}
	} else {
		State->Base.run = (ml_state_fn)ml_diffused_key;
		ml_iter_key((ml_state_t *)State, State->Iter = Value);
	}
}

static void ML_TYPED_FN(ml_iterate, MLDiffusedT, ml_state_t *Caller, ml_diffused_t *Diffused) {
	ml_diffused_state_t *State = xnew(ml_diffused_state_t, Diffused->Size, ml_diffused_task_t);
	State->Base.Type = MLDiffusedStateT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_diffused_iterate;
	State->Fn = Diffused->Fn;
	ml_diffused_task_t *Task = State->Tasks;
	Task->Base.Caller = (ml_state_t *)State;
	Task->Base.Context = Caller->Context;
	Task->Base.run = (ml_state_fn)ml_diffused_task_call;
	for (int I = Diffused->Size; --I > 0;) {
		++Task;
		Task->Next = Task - 1;
		Task->Base.Caller = (ml_state_t *)State;
		Task->Base.Context = Caller->Context;
		Task->Base.run = (ml_state_fn)ml_diffused_task_call;
	}
	State->Next = Task;
	return ml_iterate((ml_state_t *)State, Diffused->Iter);
}

ML_FUNCTION(Diffused) {
//<Sequence:sequence
//<Size:integer
//<Fn:function
//>sequence
// Returns the sequence :mini:`(K/i, Fn(K/i, V/i))` where :mini:`K/i, V/i` are the keys and values produced by :mini:`Sequence`. The calls to :mini:`Fn` are done in parallel, with at most :mini:`Size` calls at a time. The original sequence order is not preserved.
//$= list(diffused(1 .. 10, 5, tuple))
	ML_CHECK_ARG_COUNT(3);
	ML_CHECK_ARG_TYPE(1, MLIntegerT);
	ML_CHECK_ARG_TYPE(2, MLFunctionT);
	int Size = ml_integer_value(Args[1]);
	if (Size <= 0 || Size > 1024) return ml_error("RangeError", "Diffused size out of range");
	ml_diffused_t *Diffused = new(ml_diffused_t);
	Diffused->Type = MLDiffusedT;
	Diffused->Size = Size;
	Diffused->Iter = Args[0];
	Diffused->Fn = Args[2];
	return (ml_value_t *)Diffused;
}

ML_METHODX("count", MLDiffusedT) {
//!internal
	ml_diffused_t *Diffused = (ml_diffused_t *)Args[0];
	Args[0] = Diffused->Iter;
	return ml_call(Caller, CountMethod, 1, Args);
}

void ml_tasks_init(stringmap_t *Globals) {
#include "ml_tasks_init.c"
	stringmap_insert(MLTaskT->Exports, "queue", MLTaskQueueT);
	if (Globals) {
		stringmap_insert(Globals, "task", MLTaskT);
		stringmap_insert(Globals, "parallel", Parallel);
		stringmap_insert(Globals, "buffered", Buffered);
		stringmap_insert(Globals, "diffused", Diffused);
	}
}

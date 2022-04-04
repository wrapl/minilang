#include "ml_tasks.h"
#include "minilang.h"
#include "ml_macros.h"

#undef ML_CATEGORY
#define ML_CATEGORY "tasks"

typedef struct ml_waiter_t ml_waiter_t;

struct ml_waiter_t {
	ml_waiter_t *Next;
	ml_state_t *State;
};

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value;
	ml_state_t *Waiter;
	ml_waiter_t *Waiters;
} ml_task_t;

static void ml_task_set(ml_task_t *Task, ml_value_t *Value);

static void ml_task_call(ml_state_t *Caller, ml_task_t *Task, int Count, ml_value_t **Args) {
	if (Task->Value) ML_RETURN(Task->Value);
	if (!Task->Waiter) {
		Task->Waiter = Caller;
	} else {
		ml_waiter_t *Waiter = new(ml_waiter_t);
		Waiter->Next = Task->Waiters;
		Waiter->State = Caller;
		Task->Waiters = Waiter;
	}
}

ML_TYPE(MLTaskT, (), "task",
// A task representing a value that will eventually be completed.
	.call = (void *)ml_task_call
);

static void ml_task_set(ml_task_t *Task, ml_value_t *Value) {
	Task->Value = Value;
	if (Task->Waiter) {
		for (ml_waiter_t *Waiter = Task->Waiters; Waiter; Waiter = Waiter->Next) {
			Waiter->State->run(Waiter->State, Value);
		}
		Task->Waiter->run(Task->Waiter, Value);
	}
}

ML_METHOD(MLTaskT) {
//>task
// Returns a task. The task should eventually be completed with :mini:`Task:done()` or :mini:`Task:error()`.
	ml_task_t *Task = new(ml_task_t);
	Task->Type = MLTaskT;
	return (ml_value_t *)Task;
}

typedef struct {
	ml_state_t Base;
	ml_task_t Task[1];
} ml_task_state_t;

static void ml_task_run(ml_task_state_t *Task, ml_value_t *Result) {
	if (!Task->Task->Value) ml_task_set(Task->Task, Result);
}

ML_METHODVX(MLTaskT, MLFunctionT) {
//<Fn
//<Args
//>task
// Calls :mini:`Fn(Args)` and immediately returns a task that will eventually be completed with the result of the call.
	ml_task_state_t *State = new(ml_task_state_t);
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_task_run;
	State->Task->Type = MLTaskT;
	ml_call(State, Args[0], Count - 1, Args + 1);
	ML_RETURN(State->Task);
}

ML_METHODX("wait", MLTaskT) {
//<Task
//>any|error
// Waits until :mini:`Task` is completed and returns its result.
	ml_task_t *Task = (ml_task_t *)Args[0];
	return ml_task_call(Caller, Task, 0, NULL);
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
	ml_task_state_t Base;
	ml_value_t *Fn;
	ml_value_t *Args[1];
} ml_task_then_t;

static void ml_task_then_run(ml_task_then_t *State, ml_value_t *Value) {
	State->Args[0] = Value;
	State->Base.Base.run = (ml_state_fn)ml_task_run;
	return ml_call((ml_state_t *)State, State->Fn, 1, State->Args);
}

ML_METHODX("then", MLTaskT, MLFunctionT) {
//<Task
//<Fn
//>task
// Equivalent to :mini:`task(:wait -> Fn, Task)`.
	ml_task_t *Task = (ml_task_t *)Args[0];
	ml_task_then_t *Then = new(ml_task_then_t);
	Then->Base.Base.Context = Caller->Context;
	Then->Base.Base.run = (ml_state_fn)ml_task_then_run;
	Then->Base.Task->Type = MLTaskT;
	Then->Fn = Args[1];
	ml_task_call((ml_state_t *)Then, Task, 0, NULL);
	ML_RETURN(Then->Base.Task);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Value;
	ml_state_t *Limited;
	size_t Waiting, Limit, Burst;
} ml_tasks_t;

static void ml_tasks_call(ml_state_t *Caller, ml_tasks_t *Tasks, int Count, ml_value_t **Args) {
	if (!Tasks->Waiting) ML_ERROR("TasksError", "Tasks have already completed");
	if (Tasks->Value != MLNil) ML_RETURN(Tasks->Value);
	ML_CHECKX_ARG_TYPE(Count - 1, MLFunctionT);
	ml_value_t *Fn = Args[Count - 1];
	++Tasks->Waiting;
	ml_call(Tasks, Fn, Count - 1, Args);
	if (Tasks->Waiting > Tasks->Limit && !Tasks->Limited) {
		Tasks->Limited = Caller;
	} else {
		ML_RETURN(Tasks->Value);
	}
}

static void ml_tasks_continue(ml_tasks_t *Tasks, ml_value_t *Value) {
	if (ml_is_error(Value)) Tasks->Value = Value;
	--Tasks->Waiting;
	if (Tasks->Limited && Tasks->Waiting <= Tasks->Burst) {
		ml_state_t *Caller = Tasks->Limited;
		Tasks->Limited = NULL;
		ML_RETURN(Tasks->Value);
	}
	if (Tasks->Waiting == 0) ML_CONTINUE(Tasks->Base.Caller, Tasks->Value);
}

extern ml_type_t MLTasksT[];

ML_FUNCTIONX(Tasks) {
//<Max?:integer
//<Min?:integer
//>tasks
// Creates a new :mini:`tasks` set.
// If specified, at most :mini:`Max` functions will be called in parallel (the default is unlimited).
// If :mini:`Min` is also specified then the number of running tasks must drop below :mini:`Min` before more tasks are launched.
	ml_tasks_t *Tasks = new(ml_tasks_t);
	Tasks->Base.Type = MLTasksT;
	Tasks->Base.run = (void *)ml_tasks_continue;
	Tasks->Base.Caller = Caller;
	Tasks->Base.Context = Caller->Context;
	Tasks->Value = MLNil;
	Tasks->Waiting = 1;
	if (Count >= 2) {
		ML_CHECKX_ARG_TYPE(0, MLIntegerT);
		ML_CHECKX_ARG_TYPE(1, MLIntegerT);
		Tasks->Limit = ml_integer_value_fast(Args[1]);
		Tasks->Burst = ml_integer_value_fast(Args[0]) + 1;
	} else if (Count >= 1) {
		ML_CHECKX_ARG_TYPE(0, MLIntegerT);
		Tasks->Limit = ml_integer_value_fast(Args[0]);
		Tasks->Burst = SIZE_MAX;
	} else {
		Tasks->Limit = SIZE_MAX;
		Tasks->Burst = SIZE_MAX;
	}
	ML_RETURN(Tasks);
}

ML_TYPE(MLTasksT, (MLFunctionT), "tasks",
// A dynamic set of tasks (function calls). Multiple tasks can run in parallel (depending on the availability of a scheduler and/or asynchronous function calls).
	.call = (void *)ml_tasks_call,
	.Constructor = (ml_value_t *)Tasks
);

ML_METHODVX("add", MLTasksT, MLFunctionT) {
//<Tasks
//<Fn
//<Args
// Adds the function call :mini:`Fn(Args...)` to a set of tasks. Raises an error if :mini:`Tasks` is already complete.
	ml_tasks_t *Tasks = (ml_tasks_t *)Args[0];
	if (!Tasks->Waiting) ML_ERROR("TasksError", "Tasks have already completed");
	if (Tasks->Value != MLNil) ML_RETURN(Tasks->Value);
	ML_CHECKX_ARG_TYPE(Count - 1, MLFunctionT);
	ml_value_t *Fn = Args[Count - 1];
	++Tasks->Waiting;
	ml_call(Tasks, Fn, Count - 2, Args + 1);
	if (Tasks->Waiting > Tasks->Limit && !Tasks->Limited) {
		Tasks->Limited = Caller;
	} else {
		ML_RETURN(Tasks->Value);
	}
}

ML_METHODX("wait", MLTasksT) {
//<Tasks
//>nil | error
// Waits until all of the tasks in a tasks set have returned, or one of the tasks has returned an error (which is then returned from this call).
	ml_tasks_t *Tasks = (ml_tasks_t *)Args[0];
	Tasks->Base.Caller = Caller;
	Tasks->Base.Context = Caller->Context;
	ml_tasks_continue(Tasks, MLNil);
}

typedef struct ml_parallel_iter_t ml_parallel_iter_t;

typedef struct {
	ml_state_t Base;
	ml_state_t NextState[1];
	ml_state_t KeyState[1];
	ml_state_t ValueState[1];
	ml_value_t *Iter, *Fn, *Error;
	ml_value_t *Args[2];
	size_t Waiting, Limit, Burst;
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
		if (Parallel->Waiting > Parallel->Limit) return;
		++Parallel->Waiting;
		return ml_iter_next(Parallel->NextState, Parallel->Iter);
	}
}

static void parallel_continue(ml_parallel_t *Parallel, ml_value_t *Value) {
	if (Parallel->Error) return;
	if (ml_is_error(Value)) {
		Parallel->Error = Value;
		ML_CONTINUE(Parallel->Base.Caller, Value);
	}
	--Parallel->Waiting;
	if (Parallel->Iter) {
		if (Parallel->Waiting > Parallel->Burst) return;
		++Parallel->Waiting;
		return ml_iter_next(Parallel->NextState, Parallel->Iter);
	}
	if (Parallel->Waiting == 0) ML_CONTINUE(Parallel->Base.Caller, MLNil);
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
	Parallel->Waiting = 1;
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
		Parallel->Limit = ml_integer_value_fast(Args[2]);
		Parallel->Burst = ml_integer_value_fast(Args[1]) + 1;
		Parallel->Fn = Args[3];
	} else if (Count > 2) {
		ML_CHECKX_ARG_TYPE(1, MLIntegerT);
		ML_CHECKX_ARG_TYPE(2, MLFunctionT);
		Parallel->Limit = ml_integer_value_fast(Args[1]);
		Parallel->Burst = SIZE_MAX;
		Parallel->Fn = Args[2];
	} else {
		ML_CHECKX_ARG_TYPE(1, MLFunctionT);
		Parallel->Limit = SIZE_MAX;
		Parallel->Burst = SIZE_MAX;
		Parallel->Fn = Args[1];
	}

	return ml_iterate(Parallel->NextState, Args[0]);
}

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
	int Size, Use, Fetch, Ready;
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
		if (State->Ready) {
			State->Ready = 0;
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
		State->Ready = 1;
		++State->Fetch;
		ml_value_t **Args = ml_alloc_args(2);
		Args[0] = Entry->Key;
		Args[1] = Value;
		ml_call((ml_state_t *)Entry, State->Fn, 2, Args);
		if (State->Fetch - State->Use < State->Size) {
			if (State->Ready) {
				State->Ready = 0;
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

void ml_tasks_init(stringmap_t *Globals) {
#include "ml_tasks_init.c"
	if (Globals) {
		stringmap_insert(Globals, "task", MLTaskT);
		stringmap_insert(Globals, "tasks", MLTasksT);
		stringmap_insert(Globals, "parallel", Parallel);
		stringmap_insert(Globals, "buffered", Buffered);
	}
}

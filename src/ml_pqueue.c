#include "minilang.h"
#include "ml_macros.h"
#include <string.h>

#include "ml_pqueue.h"
#include "ml_sequence.h"

#pragma GCC optimize ("no-tree-loop-distribute-patterns")

#undef ML_CATEGORY
#define ML_CATEGORY "pqueue"

typedef struct ml_pqueue_t ml_pqueue_t;
typedef struct ml_pqueue_entry_t ml_pqueue_entry_t;

struct ml_pqueue_entry_t {
	ml_type_t *Type;
	ml_pqueue_t *Queue;
	ml_value_t *Value, *Priority;
	int Index;
};

ML_TYPE(MLPQueueEntryT, (), "pqueue::entry");
// A entry in a priority queue.

struct ml_pqueue_t {
	ml_state_t Base;
	ml_value_t *Compare;
	ml_pqueue_entry_t **Entries;

	ml_pqueue_entry_t *Entry, *Test, *Parent;
	ml_value_t *Args[2];
	ml_value_t *Result;

	int Count, Size;
};

ML_TYPE(MLPQueueT, (MLSequenceT), "pqueue");
// A priority queue with values and associated priorities.

ml_value_t *ml_pqueue(ml_value_t *Compare) {
	ml_pqueue_t *Queue = new(ml_pqueue_t);
	Queue->Base.Type = MLPQueueT;
	Queue->Compare = Compare;
	Queue->Size = 16;
	Queue->Entries = anew(ml_pqueue_entry_t *, Queue->Size);
	return (ml_value_t *)Queue;
}

extern ml_value_t *GreaterMethod;

ML_METHOD(MLPQueueT) {
//>pqueue
// Returns a new priority queue using :mini:`>` to compare priorities.
	return ml_pqueue(GreaterMethod);
}

ML_METHOD(MLPQueueT, MLFunctionT) {
//<Greater
//>pqueue
// Returns a new priority queue using :mini:`Greater` to compare priorities.
	return ml_pqueue(Args[0]);
}

static void ml_pqueue_finish(ml_pqueue_t *Queue) {
	ml_state_t *Caller = Queue->Base.Caller;
	ml_value_t *Result = Queue->Result;
	Queue->Base.Caller = NULL;
	Queue->Result = NULL;
	ML_RETURN(Result);
}

static void ml_pqueue_up2(ml_pqueue_t *Queue, ml_pqueue_entry_t *Entry);

static void ml_pqueue_up_run(ml_pqueue_t *Queue, ml_value_t *Result) {
	if (ml_is_error(Result)) return ml_pqueue_finish(Queue);
	ml_pqueue_entry_t *Entry = Queue->Entry;
	if (Result == MLNil) return ml_pqueue_finish(Queue);
	ml_pqueue_entry_t **Entries = Queue->Entries;
	ml_pqueue_entry_t *Parent = Queue->Parent;
	int Index = Entry->Index;
	Entries[Parent->Index] = Entry;
	Entries[Index] = Parent;
	Entry->Index = Parent->Index;
	Parent->Index = Index;
	return ml_pqueue_up2(Queue, Entry);
}

static void ml_pqueue_up2(ml_pqueue_t *Queue, ml_pqueue_entry_t *Entry) {
	if (Entry->Index == 0) return ml_pqueue_finish(Queue);
	ml_pqueue_entry_t **Entries = Queue->Entries;
	int ParentIndex = (Entry->Index - 1) / 2;
	ml_pqueue_entry_t *Parent = Queue->Parent = Entries[ParentIndex];
	Queue->Entry = Entry;
	Queue->Args[0] = Entry->Priority;
	Queue->Args[1] = Parent->Priority;
	return ml_call(Queue, Queue->Compare, 2, Queue->Args);
}

static void ml_pqueue_down1(ml_pqueue_t *Queue, ml_pqueue_entry_t *Entry);

static void ml_pqueue_down2(ml_pqueue_t *Queue, ml_value_t *Result) {
	ml_pqueue_entry_t *Entry = Queue->Entry;
	ml_pqueue_entry_t *Parent = Queue->Parent;
	if (Parent == Entry) return ml_pqueue_finish(Queue);
	ml_pqueue_entry_t **Entries = Queue->Entries;
	int Index = Entry->Index;
	Entries[Parent->Index] = Entry;
	Entries[Index] = Parent;
	Entry->Index = Parent->Index;
	Parent->Index = Index;
	return ml_pqueue_down1(Queue, Entry);
}

static void ml_pqueue_down_right(ml_pqueue_t *Queue, ml_value_t *Result) {
	if (ml_is_error(Result)) return ml_pqueue_finish(Queue);
	if (Result != MLNil) Queue->Parent = Queue->Test;
	ml_pqueue_down2(Queue, MLNil);
}

static void ml_pqueue_down_left(ml_pqueue_t *Queue, ml_value_t *Result) {
	if (ml_is_error(Result)) return ml_pqueue_finish(Queue);
	if (Result != MLNil) Queue->Parent = Queue->Test;
	ml_pqueue_entry_t **Entries = Queue->Entries;
	int Right = Queue->Test->Index + 1;
	if (Right < Queue->Count) {
		Queue->Base.run = (ml_state_fn)ml_pqueue_down_right;
		ml_pqueue_entry_t *Test = Queue->Test = Entries[Right];
		Queue->Args[0] = Test->Priority;
		Queue->Args[1] = Queue->Parent->Priority;
		return ml_call(Queue, Queue->Compare, 2, Queue->Args);
	}
	ml_pqueue_down2(Queue, MLNil);
}

static void ml_pqueue_down1(ml_pqueue_t *Queue, ml_pqueue_entry_t *Entry) {
	ml_pqueue_entry_t **Entries = Queue->Entries;
	Queue->Entry = Entry;
	Queue->Parent = Entry;
	int Left = 2 * Entry->Index + 1;
	if (Left < Queue->Count) {
		Queue->Base.run = (ml_state_fn)ml_pqueue_down_left;
		ml_pqueue_entry_t *Test = Queue->Test = Entries[Left];
		Queue->Args[0] = Test->Priority;
		Queue->Args[1] = Entry->Priority;
		return ml_call(Queue, Queue->Compare, 2, Queue->Args);
	}
	int Right = Left + 1;
	if (Right < Queue->Count) {
		Queue->Base.run = (ml_state_fn)ml_pqueue_down_right;
		ml_pqueue_entry_t *Test = Queue->Test = Entries[Right];
		Queue->Args[0] = Test->Priority;
		Queue->Args[1] = Entry->Priority;
		return ml_call(Queue, Queue->Compare, 2, Queue->Args);
	}
	return ml_pqueue_finish(Queue);
}

static void ml_pqueue_insert(ml_state_t *Caller, ml_pqueue_t *Queue, ml_pqueue_entry_t *Entry) {
	if (Queue->Count == Queue->Size) {
		Queue->Size *= 2;
		ml_pqueue_entry_t **Entries = anew(ml_pqueue_entry_t *, Queue->Size);
		memcpy(Entries, Queue->Entries, Queue->Count * sizeof(ml_pqueue_entry_t *));
		Queue->Entries = Entries;
	}
	Entry->Index = Queue->Count++;
	Queue->Entries[Entry->Index] = Entry;
	Queue->Base.Caller = Caller;
	Queue->Base.Context = Caller->Context;
	Queue->Result = (ml_value_t *)Entry;
	Queue->Base.run = (ml_state_fn)ml_pqueue_up_run;
	return ml_pqueue_up2(Queue, Entry);
}

ML_METHODX("insert", MLPQueueT, MLAnyT, MLAnyT) {
//<Queue
//<Value
//<Priority
//>pqueue::entry
// Creates and returns a new entry in :mini:`Queue` with value :mini:`Value` and priority :mini:`Priority`.
	ml_pqueue_t *Queue = (ml_pqueue_t *)Args[0];
	ml_pqueue_entry_t *Entry = new(ml_pqueue_entry_t);
	Entry->Type = MLPQueueEntryT;
	Entry->Queue = Queue;
	Entry->Value = Args[1];
	Entry->Priority = Args[2];
	return ml_pqueue_insert(Caller, Queue, Entry);
}

ML_METHOD("peek", MLPQueueT) {
//<Queue
//>pqueue::entry|nil
// Returns the highest priority entry in :mini:`Queue` without removing it, or :mini:`nil` if :mini:`Queue` is empty.
	ml_pqueue_t *Queue = (ml_pqueue_t *)Args[0];
	if (!Queue->Count) return MLNil;
	return (ml_value_t *)Queue->Entries[0];
}

ML_METHODX("next", MLPQueueT) {
//<Queue
//>pqueue::entry|nil
// Removes and returns the highest priority entry in :mini:`Queue`, or :mini:`nil` if :mini:`Queue` is empty.
	ml_pqueue_t *Queue = (ml_pqueue_t *)Args[0];
	if (!Queue->Count) ML_RETURN(MLNil);
	ml_pqueue_entry_t *Next = Queue->Entries[0];
	Next->Index = INT_MAX;
	ml_pqueue_entry_t *Entry = Queue->Entries[--Queue->Count];
	Queue->Entries[Queue->Count] = NULL;
	if (!Queue->Count) ML_RETURN(Next);
	Queue->Entries[0] = Entry;
	Entry->Index = 0;
	Queue->Base.Caller = Caller;
	Queue->Base.Context = Caller->Context;
	Queue->Result = (ml_value_t *)Next;
	return ml_pqueue_down1(Queue, Entry);
}

ML_METHOD("count", MLPQueueT) {
//<Queue
//>integer
// Returns the number of entries in :mini:`Queue`.
	ml_pqueue_t *Queue = (ml_pqueue_t *)Args[0];
	return ml_integer(Queue->Count);
}

ML_METHODX("requeue", MLPQueueEntryT) {
//<Entry
//>pqueue::entry
// Adds :mini:`Entry` back into its priority queue if it is not currently in the queue.
	ml_pqueue_entry_t *Entry = (ml_pqueue_entry_t *)Args[0];
	ml_pqueue_t *Queue = Entry->Queue;
	if (Entry->Index == INT_MAX) return ml_pqueue_insert(Caller, Queue, Entry);
	ML_RETURN(Entry);
}

static void ml_pqueue_adjust_run(ml_pqueue_t *Queue, ml_value_t *Result) {
	if (ml_is_error(Result)) return ml_pqueue_finish(Queue);
	if (Result != MLNil) {
		Queue->Base.run = (ml_state_fn)ml_pqueue_up_run;
		return ml_pqueue_up2(Queue, Queue->Entry);
	} else {
		return ml_pqueue_down1(Queue, Queue->Entry);
	}
}

ML_METHODX("adjust", MLPQueueEntryT, MLAnyT) {
//<Entry
//<Priority
//>pqueue::entry
// Changes the priority of :mini:`Entry` to :mini:`Priority`.
	ml_pqueue_entry_t *Entry = (ml_pqueue_entry_t *)Args[0];
	ml_value_t *Priority = Args[1];
	ml_pqueue_t *Queue = Entry->Queue;
	Queue->Base.Caller = Caller;
	Queue->Base.Context = Caller->Context;
	Queue->Base.run = (ml_state_fn)ml_pqueue_adjust_run;
	Queue->Entry = Entry;
	Queue->Result = (ml_value_t *)Queue->Entry;
	Queue->Args[0] = Priority;
	Queue->Args[1] = Entry->Priority;
	return ml_call(Queue, Queue->Compare, 2, Queue->Args);
}

static void ml_pqueue_raise_run(ml_pqueue_t *Queue, ml_value_t *Result) {
	if (ml_is_error(Result)) return ml_pqueue_finish(Queue);
	ml_value_t *Priority = Queue->Result;
	Queue->Result = (ml_value_t *)Queue->Entry;
	if (Result != MLNil) {
		ml_pqueue_entry_t *Entry = Queue->Entry;
		Entry->Priority = Priority;
		if (Entry->Index != INT_MAX) {
			Queue->Base.run = (ml_state_fn)ml_pqueue_up_run;
			return ml_pqueue_up2(Queue, Queue->Entry);
		}
	}
	return ml_pqueue_finish(Queue);
}

ML_METHODX("raise", MLPQueueEntryT, MLAnyT) {
//<Entry
//<Priority
//>pqueue::entry
// Changes the priority of :mini:`Entry` to :mini:`Priority` only if its current priority is less than :mini:`Priority`.
	ml_pqueue_entry_t *Entry = (ml_pqueue_entry_t *)Args[0];
	ml_value_t *Priority = Args[1];
	ml_pqueue_t *Queue = Entry->Queue;
	Queue->Base.Caller = Caller;
	Queue->Base.Context = Caller->Context;
	Queue->Base.run = (ml_state_fn)ml_pqueue_raise_run;
	Queue->Entry = Entry;
	Queue->Result = Priority;
	Queue->Args[0] = Priority;
	Queue->Args[1] = Entry->Priority;
	return ml_call(Queue, Queue->Compare, 2, Queue->Args);
}

static void ml_pqueue_lower_run(ml_pqueue_t *Queue, ml_value_t *Result) {
	if (ml_is_error(Result)) return ml_pqueue_finish(Queue);
	ml_value_t *Priority = Queue->Result;
	Queue->Result = (ml_value_t *)Queue->Entry;
	if (Result != MLNil) {
		ml_pqueue_entry_t *Entry = Queue->Entry;
		Entry->Priority = Priority;
		if (Entry->Index != INT_MAX) {
			Queue->Base.run = (ml_state_fn)ml_pqueue_up_run;
			return ml_pqueue_up2(Queue, Queue->Entry);
		}
	}
	return ml_pqueue_finish(Queue);
}

ML_METHODX("lower", MLPQueueEntryT, MLAnyT) {
//<Entry
//<Priority
//>pqueue::entry
// Changes the priority of :mini:`Entry` to :mini:`Priority` only if its current priority is greater than :mini:`Priority`.
	ml_pqueue_entry_t *Entry = (ml_pqueue_entry_t *)Args[0];
	ml_value_t *Priority = Args[1];
	ml_pqueue_t *Queue = Entry->Queue;
	Queue->Base.Caller = Caller;
	Queue->Base.Context = Caller->Context;
	Queue->Base.run = (ml_state_fn)ml_pqueue_lower_run;
	Queue->Entry = Entry;
	Queue->Result = Priority;
	Queue->Args[0] = Entry->Priority;
	Queue->Args[1] = Priority;
	return ml_call(Queue, Queue->Compare, 2, Queue->Args);
}

ML_METHODX("remove", MLPQueueEntryT) {
//<Entry
//>pqueue::entry
// Removes :mini:`Entry` from its priority queue.
	ml_pqueue_entry_t *Entry = (ml_pqueue_entry_t *)Args[0];
	if (Entry->Index == INT_MAX) ML_RETURN(Entry);
	ml_pqueue_t *Queue = Entry->Queue;
	ml_pqueue_entry_t *Next = Queue->Entries[--Queue->Count];
	Queue->Entries[Queue->Count] = NULL;
	int Index = Next->Index = Entry->Index;
	Queue->Entries[Index] = Next;
	Entry->Index = INT_MAX;
	Queue->Base.Caller = Caller;
	Queue->Base.Context = Caller->Context;
	Queue->Base.run = (ml_state_fn)ml_pqueue_adjust_run;
	Queue->Entry = Next;
	Queue->Result = (ml_value_t *)Entry;
	Queue->Args[0] = Next->Priority;
	Queue->Args[1] = Entry->Priority;
	return ml_call(Queue, Queue->Compare, 2, Queue->Args);
}

ML_METHOD("value", MLPQueueEntryT) {
//<Entry
//>any
// Returns the value associated with :mini:`Entry`.
	ml_pqueue_entry_t *Entry = (ml_pqueue_entry_t *)Args[0];
	return Entry->Value;
}

ML_METHOD("priority", MLPQueueEntryT) {
//<Entry
//>any
// Returns the priority associated with :mini:`Entry`.
	ml_pqueue_entry_t *Entry = (ml_pqueue_entry_t *)Args[0];
	return Entry->Priority;
}

static ml_value_t *ML_TYPED_FN(ml_unpack, MLPQueueEntryT, ml_pqueue_entry_t *Entry, int Index) {
	if (Index == 1) return Entry->Value;
	if (Index == 2) return Entry->Priority;
	return MLNil;
}

ML_METHOD("queued", MLPQueueEntryT) {
//<Entry
//>pqueue::entry|nil
// Returns :mini:`Entry` if it is currently in the priority queue, otherwise returns :mini:`nil`.
	ml_pqueue_entry_t *Entry = (ml_pqueue_entry_t *)Args[0];
	if (Entry->Index == INT_MAX) return MLNil;
	return (ml_value_t *)Entry;
}

ml_value_t *ML_TYPED_FN(ml_unpack, MLPQueueEntryT, ml_pqueue_entry_t *Entry, int Index) {
	if (Index == 1) return Entry->Value;
	if (Index == 2) return Entry->Priority;
	return MLNil;
}

typedef struct {
	ml_type_t *Type;
	ml_pqueue_entry_t **Entries;
	int Index, Count;
} ml_pqueue_iter_t;

ML_TYPE(MLPQueueIterT, (), "pqueue-iter");
//!internal

static void ML_TYPED_FN(ml_iterate, MLPQueueT, ml_state_t *Caller, ml_pqueue_t *Queue) {
	if (!Queue->Count) ML_RETURN(MLNil);
	ml_pqueue_iter_t *Iter = new(ml_pqueue_iter_t);
	Iter->Type = MLPQueueIterT;
	Iter->Entries = Queue->Entries;
	Iter->Count = Queue->Count;
	Iter->Index = 0;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLPQueueIterT, ml_state_t *Caller, ml_pqueue_iter_t *Iter) {
	if (++Iter->Index == Iter->Count) ML_RETURN(MLNil);
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLPQueueIterT, ml_state_t *Caller, ml_pqueue_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index + 1));
}

static void ML_TYPED_FN(ml_iter_value, MLPQueueIterT, ml_state_t *Caller, ml_pqueue_iter_t *Iter) {
	ML_RETURN(Iter->Entries[Iter->Index]);
}

void ml_pqueue_init(stringmap_t *Globals) {
#include "ml_pqueue_init.c"
	stringmap_insert(Globals, "pqueue", MLPQueueT);
}

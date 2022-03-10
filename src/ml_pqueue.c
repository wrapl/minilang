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
	ml_pqueue_t *PQueue;
	ml_value_t *Value, *Priority;
	int Index;
};

ML_TYPE(MLPQueueEntryT, (), "pqueue::entry");
// A entry in a priority queue.

struct ml_pqueue_t {
	ml_state_t Base;
	ml_value_t *Compare;
	ml_pqueue_entry_t **Entries;
	ml_pqueue_entry_t *Entry;
	ml_value_t *Args[2];
	int Count, Size, Index;
};

ML_TYPE(MLPQueueT, (MLSequenceT), "pqueue");
// A priority queue with values and associated priorities.

ml_value_t *ml_pqueue(ml_value_t *Compare) {
	ml_pqueue_t *PQueue = new(ml_pqueue_t);
	PQueue->Base.Type = MLPQueueT;
	PQueue->Compare = Compare;
	PQueue->Size = 16;
	PQueue->Entries = anew(ml_pqueue_entry_t *, PQueue->Size);
	return (ml_value_t *)PQueue;
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

static inline int priority_higher(ml_value_t *Compare, ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	ml_value_t *Result = ml_simple_call(Compare, 2, Args);
	return Result != MLNil;
}

static void ml_pqueue_up_run(ml_pqueue_t *PQueue, ml_value_t *Result) {
	if (ml_integer_value(Result) >= 0) {

	}
}

static void ml_pqueue_up(ml_pqueue_t *PQueue, ml_pqueue_entry_t *Entry) {
	ml_pqueue_entry_t **Entries = PQueue->Entries;
	int Index = Entry->Index;
	while (Index > 0) {
		int ParentIndex = (Index - 1) / 2;
		ml_pqueue_entry_t *Parent = Entries[ParentIndex];
		// if (Parent->Priority >= Entry->Priority) {
		if (!priority_higher(PQueue->Compare, Entry->Priority, Parent->Priority)) {
			Entry->Index = Index;
			return;
		}
		Parent->Index = Index;
		Entries[Index] = Parent;
		Entries[ParentIndex] = Entry;
		Index = ParentIndex;
	}
	Entry->Index = 0;
}

static void ml_pqueue_down(ml_pqueue_t *PQueue, ml_pqueue_entry_t *Entry) {
	ml_pqueue_entry_t **Entries = PQueue->Entries;
	int Count = PQueue->Count;
	int Index = Entry->Index;
	for (;;) {
		int Left = 2 * Index + 1;
		int Right = 2 * Index + 2;
		int Largest = Index;
		Entries[Index] = Entry;
		if (Left < Count && Entries[Left]) {
			if (priority_higher(PQueue->Compare, Entries[Left]->Priority, Entries[Largest]->Priority)) {
				Largest = Left;
			}
		}
		if (Right < Count && Entries[Right]) {
			if (priority_higher(PQueue->Compare, Entries[Right]->Priority, Entries[Largest]->Priority)) {
				Largest = Right;
			}
		}
		if (Largest != Index) {
			ml_pqueue_entry_t *Parent = Entries[Largest];
			Entries[Index] = Parent;
			Parent->Index = Index;
			Index = Largest;
		} else {
			Entry->Index = Index;
			return;
		}
	}
}

static void ml_pqueue_insert(ml_pqueue_t *PQueue, ml_pqueue_entry_t *Entry) {
	if (PQueue->Count == PQueue->Size) {
		PQueue->Size *= 2;
		ml_pqueue_entry_t **Entries = anew(ml_pqueue_entry_t *, PQueue->Size);
		memcpy(Entries, PQueue->Entries, PQueue->Count * sizeof(ml_pqueue_entry_t *));
		PQueue->Entries = Entries;
	}
	Entry->Index = PQueue->Count++;
	PQueue->Entries[Entry->Index] = Entry;
	ml_pqueue_up(PQueue, Entry);
}

ML_METHOD("insert", MLPQueueT, MLAnyT, MLAnyT) {
//<PQueue
//<Value
//<Priority
//>pqueue::entry
// Creates and returns a new entry in :mini:`PQueue` with value :mini:`Value` and priority :mini:`Priority`.
	ml_pqueue_t *PQueue = (ml_pqueue_t *)Args[0];
	ml_pqueue_entry_t *Entry = new(ml_pqueue_entry_t);
	Entry->Type = MLPQueueEntryT;
	Entry->PQueue = PQueue;
	Entry->Value = Args[1];
	Entry->Priority = Args[2];
	ml_pqueue_insert(PQueue, Entry);
	return (ml_value_t *)Entry;
}

ML_METHOD("peek", MLPQueueT) {
//<PQueue
//>pqueue::entry|nil
// Returns the highest priority entry in :mini:`PQueue` without removing it, or :mini:`nil` if :mini:`PQueue` is empty.
	ml_pqueue_t *PQueue = (ml_pqueue_t *)Args[0];
	if (!PQueue->Count) return MLNil;
	return (ml_value_t *)PQueue->Entries[0];
}

ML_METHOD("next", MLPQueueT) {
//<PQueue
//>pqueue::entry|nil
// Removes and returns the highest priority entry in :mini:`PQueue`, or :mini:`nil` if :mini:`PQueue` is empty.
	ml_pqueue_t *PQueue = (ml_pqueue_t *)Args[0];
	if (!PQueue->Count) return MLNil;
	ml_pqueue_entry_t *Next = PQueue->Entries[0];
	ml_pqueue_entry_t *Entry = PQueue->Entries[--PQueue->Count];
	PQueue->Entries[PQueue->Count] = NULL;
	PQueue->Entries[0] = Entry;
	Entry->Index = 0;
	ml_pqueue_down(PQueue, Entry);
	Next->Index = INT_MAX;
	return (ml_value_t *)Next;
}

ML_METHOD("count", MLPQueueT) {
//<PQueue
//>integer
// Returns the number of entries in :mini:`PQueue`.
	ml_pqueue_t *PQueue = (ml_pqueue_t *)Args[0];
	return ml_integer(PQueue->Count);
}

ML_METHOD("requeue", MLPQueueEntryT) {
//<Entry
//>pqueue::entry
// Adds :mini:`Entry` back into its priority queue if it is not currently in the queue.
	ml_pqueue_entry_t *Entry = (ml_pqueue_entry_t *)Args[0];
	ml_pqueue_t *PQueue = Entry->PQueue;
	if (Entry->Index == INT_MAX) ml_pqueue_insert(PQueue, Entry);
	return (ml_value_t *)Entry;
}

ML_METHOD("adjust", MLPQueueEntryT, MLAnyT) {
//<Entry
//<Priority
//>pqueue::entry
// Changes the priority of :mini:`Entry` to :mini:`Priority`.
	ml_pqueue_entry_t *Entry = (ml_pqueue_entry_t *)Args[0];
	ml_value_t *Priority = Args[1];
	ml_pqueue_t *PQueue = Entry->PQueue;
	int Greater = priority_higher(PQueue->Compare, Priority, Entry->Priority);
	Entry->Priority = Priority;
	if (Entry->Index == INT_MAX) {
		if (Greater) {
			ml_pqueue_up(PQueue, Entry);
		} else {
			ml_pqueue_down(PQueue, Entry);
		}
	}
	return (ml_value_t *)Entry;
}

ML_METHOD("raise", MLPQueueEntryT, MLAnyT) {
//<Entry
//<Priority
//>pqueue::entry
// Changes the priority of :mini:`Entry` to :mini:`Priority` only if its current priority is less than :mini:`Priority`.
	ml_pqueue_entry_t *Entry = (ml_pqueue_entry_t *)Args[0];
	ml_value_t *Priority = Args[1];
	ml_pqueue_t *PQueue = Entry->PQueue;
	if (priority_higher(PQueue->Compare, Priority, Entry->Priority)) {
		Entry->Priority = Priority;
		if (Entry->Index != INT_MAX) ml_pqueue_up(PQueue, Entry);
	}
	return (ml_value_t *)Entry;
}

ML_METHOD("lower", MLPQueueEntryT, MLAnyT) {
//<Entry
//<Priority
//>pqueue::entry
// Changes the priority of :mini:`Entry` to :mini:`Priority` only if its current priority is greater than :mini:`Priority`.
	ml_pqueue_entry_t *Entry = (ml_pqueue_entry_t *)Args[0];
	ml_value_t *Priority = Args[1];
	ml_pqueue_t *PQueue = Entry->PQueue;
	if (priority_higher(PQueue->Compare, Entry->Priority, Priority)) {
		Entry->Priority = Priority;
		if (Entry->Index != INT_MAX) ml_pqueue_down(PQueue, Entry);
	}
	return (ml_value_t *)Entry;
}

ML_METHOD("remove", MLPQueueEntryT) {
//<Entry
//>pqueue::entry
// Removes :mini:`Entry` from its priority queue.
	ml_pqueue_entry_t *Entry = (ml_pqueue_entry_t *)Args[0];
	if (Entry->Index == INT_MAX) return Args[0];
	ml_pqueue_t *PQueue = Entry->PQueue;
	ml_pqueue_entry_t *Next = PQueue->Entries[--PQueue->Count];
	PQueue->Entries[PQueue->Count] = NULL;
	int Index = Next->Index = Entry->Index;
	PQueue->Entries[Index] = Next;
	if (priority_higher(PQueue->Compare, Next->Priority, Entry->Priority)) {
		ml_pqueue_up(PQueue, Next);
	} else {
		ml_pqueue_down(PQueue, Next);
	}
	Entry->Index = INT_MAX;
	return (ml_value_t *)Entry;
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

static void ML_TYPED_FN(ml_iterate, MLPQueueT, ml_state_t *Caller, ml_pqueue_t *PQueue) {
	if (!PQueue->Count) ML_RETURN(MLNil);
	ml_pqueue_iter_t *Iter = new(ml_pqueue_iter_t);
	Iter->Type = MLPQueueIterT;
	Iter->Entries = PQueue->Entries;
	Iter->Count = PQueue->Count;
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

#include "ml_queue.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include "ml_sequence.h"

#pragma GCC optimize ("no-tree-loop-distribute-patterns")

#undef ML_CATEGORY
#define ML_CATEGORY "queue"

typedef struct ml_queue_t ml_queue_t;
typedef struct ml_queue_entry_t ml_queue_entry_t;

struct ml_queue_entry_t {
	ml_type_t *Type;
	ml_queue_t *Queue;
	ml_value_t *Value, *Priority;
	int Index;
};

ML_TYPE(MLQueueEntryT, (), "queue::entry");
// A entry in a priority queue.

struct ml_queue_t {
	ml_type_t *Type;
	ml_queue_entry_t **Entries;
	int Count, Size;
};

ML_TYPE(MLQueueT, (MLSequenceT), "queue");
// A priority queue with values and associated priorities.

ML_METHOD(MLQueueT) {
//>queue
// Returns a new queue using :mini:`<>` to compare priorities.
	ml_queue_t *Queue = new(ml_queue_t);
	Queue->Type = MLQueueT;
	Queue->Size = 16;
	Queue->Entries = anew(ml_queue_entry_t *, Queue->Size);
	return (ml_value_t *)Queue;
}

extern ml_value_t *CompareMethod;

static inline int ml_queue_compare(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	ml_value_t *Result = ml_simple_call(CompareMethod, 2, Args);
	return ml_integer_value(Result);
}

static void ml_queue_up(ml_queue_t *Queue, ml_queue_entry_t *Entry) {
	ml_queue_entry_t **Entries = Queue->Entries;
	int Index = Entry->Index;
	while (Index > 0) {
		int ParentIndex = (Index - 1) / 2;
		ml_queue_entry_t *Parent = Entries[ParentIndex];
		// if (Parent->Priority >= Entry->Priority) {
		if (ml_queue_compare(Parent->Priority, Entry->Priority) >= 0) {
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

static void ml_queue_down(ml_queue_t *Queue, ml_queue_entry_t *Entry) {
	ml_queue_entry_t **Entries = Queue->Entries;
	int Count = Queue->Count;
	int Index = Entry->Index;
	for (;;) {
		int Left = 2 * Index + 1;
		int Right = 2 * Index + 2;
		int Largest = Index;
		Entries[Index] = Entry;
		if (Left < Count && Entries[Left]) {
			int Compare = ml_queue_compare(Entries[Left]->Priority, Entries[Largest]->Priority);
			if (Compare > 0) Largest = Left;
		}
		if (Right < Count && Entries[Right]) {
			int Compare = ml_queue_compare(Entries[Right]->Priority, Entries[Largest]->Priority);
			if (Compare > 0) Largest = Right;
		}
		if (Largest != Index) {
			ml_queue_entry_t *Parent = Entries[Largest];
			Entries[Index] = Parent;
			Parent->Index = Index;
			Index = Largest;
		} else {
			Entry->Index = Index;
			return;
		}
	}
}

static void ml_queue_insert(ml_queue_t *Queue, ml_queue_entry_t *Entry) {
	if (Queue->Count == Queue->Size) {
		Queue->Size *= 2;
		ml_queue_entry_t **Entries = anew(ml_queue_entry_t *, Queue->Size);
		memcpy(Entries, Queue->Entries, Queue->Count * sizeof(ml_queue_entry_t *));
		Queue->Entries = Entries;
	}
	Entry->Index = Queue->Count++;
	Queue->Entries[Entry->Index] = Entry;
	ml_queue_up(Queue, Entry);
}

ML_METHOD("insert", MLQueueT, MLAnyT, MLAnyT) {
//<Queue
//<Value
//<Priority
//>queue::entry
// Creates and returns a new entry in :mini:`Queue` with value :mini:`Value` and priority :mini:`Priority`.
	ml_queue_t *Queue = (ml_queue_t *)Args[0];
	ml_queue_entry_t *Entry = new(ml_queue_entry_t);
	Entry->Type = MLQueueEntryT;
	Entry->Queue = Queue;
	Entry->Value = Args[1];
	Entry->Priority = Args[2];
	ml_queue_insert(Queue, Entry);
	return (ml_value_t *)Entry;
}

ML_METHOD("next", MLQueueT) {
//<Queue
//>queue::entry|nil
// Removes and returns the next entry in :mini:`Queue`, or :mini:`nil` if :mini:`Queue` is empty.
	ml_queue_t *Queue = (ml_queue_t *)Args[0];
	if (!Queue->Count) return MLNil;
	ml_queue_entry_t *Next = Queue->Entries[0];
	ml_queue_entry_t *Entry = Queue->Entries[--Queue->Count];
	Queue->Entries[Queue->Count] = NULL;
	Queue->Entries[0] = Entry;
	Entry->Index = 0;
	ml_queue_down(Queue, Entry);
	Next->Index = INT_MAX;
	return (ml_value_t *)Next;
}

ML_METHOD("count", MLQueueT) {
//<Queue
//>integer
// Returns the number of entries in :mini:`Queue`.
	ml_queue_t *Queue = (ml_queue_t *)Args[0];
	return ml_integer(Queue->Count);
}

ML_METHOD("update", MLQueueEntryT, MLAnyT) {
//<Entry
//<Priority
//>queue::entry
// Changes the priority of :mini:`Entry`.
	ml_queue_entry_t *Entry = (ml_queue_entry_t *)Args[0];
	ml_value_t *Priority = Args[1];
	ml_queue_t *Queue = Entry->Queue;
	if (Entry->Index == INT_MAX) {
		Entry->Priority = Priority;
		ml_queue_insert(Queue, Entry);
	} else {
		int Compare = ml_queue_compare(Priority, Entry->Priority);
		Entry->Priority = Priority;
		if (Compare < 0) {
			ml_queue_down(Queue, Entry);
		} else if (Compare > 0) {
			ml_queue_up(Queue, Entry);
		}
	}
	return Args[0];
}

ML_METHOD("remove", MLQueueEntryT) {
//<Entry
//>queue::entry
// Removes :mini:`Entry` from its queue.
	ml_queue_entry_t *Entry = (ml_queue_entry_t *)Args[0];
	if (Entry->Index == INT_MAX) return Args[0];
	ml_queue_t *Queue = Entry->Queue;
	ml_queue_entry_t *Next = Queue->Entries[--Queue->Count];
	Queue->Entries[Queue->Count] = NULL;
	int Index = Next->Index = Entry->Index;
	Queue->Entries[Index] = Next;
	int Compare = ml_queue_compare(Entry->Priority, Next->Priority);
	if (Compare < 0) {
		ml_queue_down(Queue, Next);
	} else if (Compare > 0) {
		ml_queue_up(Queue, Next);
	}
	return Args[0];
}

ML_METHOD("value", MLQueueEntryT) {
//<Entry
//>any
// Returns the value associated with :mini:`Entry`.
	ml_queue_entry_t *Entry = (ml_queue_entry_t *)Args[0];
	return Entry->Value;
}

ML_METHOD("priority", MLQueueEntryT) {
//<Entry
//>any
// Returns the priority associated with :mini:`Entry`.
	ml_queue_entry_t *Entry = (ml_queue_entry_t *)Args[0];
	return Entry->Priority;
}

ml_value_t *ML_TYPED_FN(ml_unpack, MLQueueEntryT, ml_queue_entry_t *Entry, int Index) {
	if (Index == 1) return Entry->Value;
	if (Index == 2) return Entry->Priority;
	return MLNil;
}

typedef struct {
	ml_type_t *Type;
	ml_queue_entry_t **Entries;
	int Index, Count;
} ml_queue_iter_t;

ML_TYPE(MLQueueIterT, (), "queue-iter");
//!internal

static void ML_TYPED_FN(ml_iterate, MLQueueT, ml_state_t *Caller, ml_queue_t *Queue) {
	if (!Queue->Count) ML_RETURN(MLNil);
	ml_queue_iter_t *Iter = new(ml_queue_iter_t);
	Iter->Type = MLQueueIterT;
	Iter->Entries = Queue->Entries;
	Iter->Count = Queue->Count;
	Iter->Index = 0;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLQueueIterT, ml_state_t *Caller, ml_queue_iter_t *Iter) {
	if (++Iter->Index == Iter->Count) ML_RETURN(MLNil);
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLQueueIterT, ml_state_t *Caller, ml_queue_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index + 1));
}

static void ML_TYPED_FN(ml_iter_value, MLQueueIterT, ml_state_t *Caller, ml_queue_iter_t *Iter) {
	ML_RETURN(Iter->Entries[Iter->Index]);
}

void ml_queue_init(stringmap_t *Globals) {
#include "ml_queue_init.c"
	stringmap_insert(Globals, "queue", MLQueueT);
}

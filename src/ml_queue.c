#include "ml_queue.h"
#include "minilang.h"
#include "ml_macros.h"
#include "ml_iterfns.h"
#include <string.h>

typedef struct ml_queue_t ml_queue_t;
typedef struct ml_queue_node_t ml_queue_node_t;

struct ml_queue_node_t {
	ml_type_t *Type;
	ml_queue_t *Queue;
	ml_value_t *Value;
	double Score;
	int Index;
};

ML_TYPE(MLQueueNodeT, (), "queue-node");

struct ml_queue_t {
	ml_type_t *Type;
	ml_queue_node_t **Nodes;
	int Count, Size;
};

ML_TYPE(MLQueueT, (MLIteratableT), "queue");
// A priority queue with values and associated scores.

ML_METHOD(MLQueueT) {
	ml_queue_t *Queue = new(ml_queue_t);
	Queue->Type = MLQueueT;
	Queue->Size = 16;
	Queue->Nodes = anew(ml_queue_node_t *, Queue->Size);
	return (ml_value_t *)Queue;
}

static void ml_queue_up(ml_queue_t *Queue, ml_queue_node_t *Node) {
	ml_queue_node_t **Nodes = Queue->Nodes;
	int Index = Node->Index;
	while (Index > 0) {
		int ParentIndex = (Index - 1) / 2;
		ml_queue_node_t *Parent = Nodes[ParentIndex];
		if (Parent->Score >= Node->Score) {
			Node->Index = Index;
			return;
		}
		Parent->Index = Index;
		Nodes[Index] = Parent;
		Nodes[ParentIndex] = Node;
		Index = ParentIndex;
	}
	Node->Index = 0;
}

static void ml_queue_down(ml_queue_t *Queue, ml_queue_node_t *Node) {
	ml_queue_node_t **Nodes = Queue->Nodes;
	int Count = Queue->Count;
	int Index = Node->Index;
	for (;;) {
		int Left = 2 * Index + 1;
		int Right = 2 * Index + 2;
		int Largest = Index;
		Nodes[Index] = Node;
		if (Left < Count && Nodes[Left] && Nodes[Left]->Score > Nodes[Largest]->Score) {
			Largest = Left;
		}
		if (Right < Count && Nodes[Right] && Nodes[Right]->Score > Nodes[Largest]->Score) {
			Largest = Right;
		}
		if (Largest != Index) {
			ml_queue_node_t *Parent = Nodes[Largest];
			Nodes[Index] = Parent;
			Parent->Index = Index;
			Index = Largest;
		} else {
			Node->Index = Index;
			return;
		}
	}
}

static void ml_queue_insert(ml_queue_t *Queue, ml_queue_node_t *Node) {
	if (Queue->Count == Queue->Size) {
		Queue->Size *= 2;
		ml_queue_node_t **Nodes = anew(ml_queue_node_t *, Queue->Size);
		memcpy(Nodes, Queue->Nodes, Queue->Count * sizeof(ml_queue_node_t *));
		Queue->Nodes = Nodes;
	}
	Node->Index = Queue->Count++;
	Queue->Nodes[Node->Index] = Node;
	ml_queue_up(Queue, Node);
}

ML_METHOD("insert", MLQueueT, MLAnyT, MLNumberT) {
	ml_queue_t *Queue = (ml_queue_t *)Args[0];
	ml_queue_node_t *Node = new(ml_queue_node_t);
	Node->Type = MLQueueNodeT;
	Node->Queue = Queue;
	Node->Value = Args[1];
	Node->Score = ml_real_value(Args[2]);
	ml_queue_insert(Queue, Node);
	return (ml_value_t *)Node;
}

ML_METHOD("next", MLQueueT) {
	ml_queue_t *Queue = (ml_queue_t *)Args[0];
	if (!Queue->Count) return MLNil;
	ml_queue_node_t *Next = Queue->Nodes[0];
	ml_queue_node_t *Node = Queue->Nodes[--Queue->Count];
	Queue->Nodes[Queue->Count] = NULL;
	Queue->Nodes[0] = Node;
	Node->Index = 0;
	ml_queue_down(Queue, Node);
	Next->Index = INT_MAX;
	return (ml_value_t *)Next;
}

ML_METHOD("count", MLQueueT) {
	ml_queue_t *Queue = (ml_queue_t *)Args[0];
	return ml_integer(Queue->Count);
}

ML_METHOD("size", MLQueueT) {
	ml_queue_t *Queue = (ml_queue_t *)Args[0];
	return ml_integer(Queue->Size);
}

ML_METHOD("update", MLQueueNodeT, MLNumberT) {
	ml_queue_node_t *Node = (ml_queue_node_t *)Args[0];
	double Score = ml_real_value(Args[1]);
	ml_queue_t *Queue = Node->Queue;
	if (Node->Index == INT_MAX) {
		Node->Score = Score;
		ml_queue_insert(Queue, Node);
	} else if (Score < Node->Score) {
		Node->Score = Score;
		ml_queue_down(Queue, Node);
	} else if (Score > Node->Score) {
		Node->Score = Score;
		ml_queue_up(Queue, Node);
	}
	return Args[0];
}

ML_METHOD("remove", MLQueueNodeT) {
	ml_queue_node_t *Node = (ml_queue_node_t *)Args[0];
	if (Node->Index == INT_MAX) return Args[0];
	ml_queue_t *Queue = Node->Queue;
	ml_queue_node_t *Next = Queue->Nodes[--Queue->Count];
	Queue->Nodes[Queue->Count] = NULL;
	int Index = Next->Index = Node->Index;
	Queue->Nodes[Index] = Next;
	if (Node->Score < Next->Score) {
		ml_queue_down(Queue, Next);
	} else if (Node->Score > Next->Score) {
		ml_queue_up(Queue, Next);
	}
	return Args[0];
}

ML_METHOD("value", MLQueueNodeT) {
	ml_queue_node_t *Node = (ml_queue_node_t *)Args[0];
	return Node->Value;
}

ML_METHOD("score", MLQueueNodeT) {
	ml_queue_node_t *Node = (ml_queue_node_t *)Args[0];
	return ml_real(Node->Score);
}

ml_value_t *ML_TYPED_FN(ml_unpack, MLQueueNodeT, ml_queue_node_t *Node, int Index) {
	if (Index == 1) return Node->Value;
	if (Index == 2) return ml_real(Node->Score);
	return MLNil;
}

typedef struct {
	ml_type_t *Type;
	ml_queue_node_t **Nodes;
	int Index, Count;
} ml_queue_iter_t;

ML_TYPE(MLQueueIterT, (), "queue-iter");

static void ML_TYPED_FN(ml_iterate, MLQueueT, ml_state_t *Caller, ml_queue_t *Queue) {
	if (!Queue->Count) ML_RETURN(MLNil);
	ml_queue_iter_t *Iter = new(ml_queue_iter_t);
	Iter->Type = MLQueueIterT;
	Iter->Nodes = Queue->Nodes;
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
	ML_RETURN(Iter->Nodes[Iter->Index]);
}

void ml_queue_init(stringmap_t *Globals) {
#include "ml_queue_init.c"
	stringmap_insert(Globals, "queue", MLQueueT);
}

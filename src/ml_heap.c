#include "ml_heap.h"
#include "minilang.h"
#include "ml_macros.h"
#include "ml_iterfns.h"
#include <string.h>

ML_METHOD_DECL(MLHeapOf, "heap::of");

typedef struct ml_heap_t ml_heap_t;
typedef struct ml_heap_node_t ml_heap_node_t;

struct ml_heap_node_t {
	const ml_type_t *Type;
	ml_heap_t *Heap;
	ml_value_t *Value;
	double Score;
	int Index;
};

ML_TYPE(MLHeapNodeT, (), "heap-node");

struct ml_heap_t {
	const ml_type_t *Type;
	ml_heap_node_t **Nodes;
	int Count, Size;
};

ML_TYPE(MLHeapT, (MLIteratableT), "heap");

ML_METHOD(MLHeapOfMethod) {
	ml_heap_t *Heap = new(ml_heap_t);
	Heap->Type = MLHeapT;
	Heap->Size = 16;
	Heap->Nodes = anew(ml_heap_node_t *, Heap->Size);
	return (ml_value_t *)Heap;
}

static void ml_heap_up(ml_heap_t *Heap, ml_heap_node_t *Node) {
	ml_heap_node_t **Nodes = Heap->Nodes;
	int Index = Node->Index;
	while (Index > 0) {
		int ParentIndex = (Index - 1) / 2;
		ml_heap_node_t *Parent = Nodes[ParentIndex];
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

static void ml_heap_down(ml_heap_t *Heap, ml_heap_node_t *Node) {
	ml_heap_node_t **Nodes = Heap->Nodes;
	int Count = Heap->Count;
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
			ml_heap_node_t *Parent = Nodes[Largest];
			Nodes[Index] = Parent;
			Parent->Index = Index;
			Index = Largest;
		} else {
			Node->Index = Index;
			return;
		}
	}
}

static void ml_heap_insert(ml_heap_t *Heap, ml_heap_node_t *Node) {
	if (Heap->Count == Heap->Size) {
		Heap->Size *= 2;
		ml_heap_node_t **Nodes = anew(ml_heap_node_t *, Heap->Size);
		memcpy(Nodes, Heap->Nodes, Heap->Count * sizeof(ml_heap_node_t *));
		Heap->Nodes = Nodes;
	}
	Node->Index = Heap->Count++;
	Heap->Nodes[Node->Index] = Node;
	ml_heap_up(Heap, Node);
}

ML_METHOD("insert", MLHeapT, MLAnyT, MLNumberT) {
	ml_heap_t *Heap = (ml_heap_t *)Args[0];
	ml_heap_node_t *Node = new(ml_heap_node_t);
	Node->Type = MLHeapNodeT;
	Node->Heap = Heap;
	Node->Value = Args[1];
	Node->Score = ml_real_value(Args[2]);
	ml_heap_insert(Heap, Node);
	return (ml_value_t *)Node;
}

ML_METHOD("next", MLHeapT) {
	ml_heap_t *Heap = (ml_heap_t *)Args[0];
	if (!Heap->Count) return MLNil;
	ml_heap_node_t *Next = Heap->Nodes[0];
	ml_heap_node_t *Node = Heap->Nodes[--Heap->Count];
	Heap->Nodes[Heap->Count] = NULL;
	Heap->Nodes[0] = Node;
	Node->Index = 0;
	ml_heap_down(Heap, Node);
	Next->Index = INT_MAX;
	return (ml_value_t *)Next;
}

ML_METHOD("count", MLHeapT) {
	ml_heap_t *Heap = (ml_heap_t *)Args[0];
	return ml_integer(Heap->Count);
}

ML_METHOD("size", MLHeapT) {
	ml_heap_t *Heap = (ml_heap_t *)Args[0];
	return ml_integer(Heap->Size);
}

ML_METHOD("update", MLHeapNodeT, MLNumberT) {
	ml_heap_node_t *Node = (ml_heap_node_t *)Args[0];
	double Score = ml_real_value(Args[1]);
	ml_heap_t *Heap = Node->Heap;
	if (Node->Index == INT_MAX) {
		Node->Score = Score;
		ml_heap_insert(Heap, Node);
	} else if (Score < Node->Score) {
		Node->Score = Score;
		ml_heap_down(Heap, Node);
	} else if (Score > Node->Score) {
		Node->Score = Score;
		ml_heap_up(Heap, Node);
	}
	return Args[0];
}

ML_METHOD("remove", MLHeapNodeT) {
	ml_heap_node_t *Node = (ml_heap_node_t *)Args[0];
	if (Node->Index == INT_MAX) return Args[0];
	ml_heap_t *Heap = Node->Heap;
	ml_heap_node_t *Next = Heap->Nodes[--Heap->Count];
	Heap->Nodes[Heap->Count] = NULL;
	int Index = Next->Index = Node->Index;
	Heap->Nodes[Index] = Next;
	if (Node->Score < Next->Score) {
		ml_heap_down(Heap, Next);
	} else if (Node->Score > Next->Score) {
		ml_heap_up(Heap, Next);
	}
	return Args[0];
}

ML_METHOD("value", MLHeapNodeT) {
	ml_heap_node_t *Node = (ml_heap_node_t *)Args[0];
	return Node->Value;
}

ML_METHOD("score", MLHeapNodeT) {
	ml_heap_node_t *Node = (ml_heap_node_t *)Args[0];
	return ml_real(Node->Score);
}

ml_value_t *ML_TYPED_FN(ml_unpack, MLHeapNodeT, ml_heap_node_t *Node, int Index) {
	if (Index == 1) return Node->Value;
	if (Index == 2) return ml_real(Node->Score);
	return MLNil;
}

typedef struct {
	const ml_type_t *Type;
	ml_heap_node_t **Nodes;
	int Index, Count;
} ml_heap_iter_t;

ML_TYPE(MLHeapIterT, (), "heap-iter");

static void ML_TYPED_FN(ml_iterate, MLHeapT, ml_state_t *Caller, ml_heap_t *Heap) {
	if (!Heap->Count) ML_RETURN(MLNil);
	ml_heap_iter_t *Iter = new(ml_heap_iter_t);
	Iter->Type = MLHeapIterT;
	Iter->Nodes = Heap->Nodes;
	Iter->Count = Heap->Count;
	Iter->Index = 0;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLHeapIterT, ml_state_t *Caller, ml_heap_iter_t *Iter) {
	if (++Iter->Index == Iter->Count) ML_RETURN(MLNil);
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLHeapIterT, ml_state_t *Caller, ml_heap_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index + 1));
}

static void ML_TYPED_FN(ml_iter_value, MLHeapIterT, ml_state_t *Caller, ml_heap_iter_t *Iter) {
	ML_RETURN(Iter->Nodes[Iter->Index]);
}

void ml_heap_init(stringmap_t *Globals) {
#include "ml_heap_init.c"
	MLHeapT->Constructor = MLHeapOfMethod;
	stringmap_insert(Globals, "heap", MLHeapT);
}

#include "inthash.h"
#include "ml_macros.h"
#include <gc/gc.h>

struct inthash_node_t {
	uintptr_t Key;
	void *Value;
};

inthash_t *inthash_new() {
	return new(inthash_t);
}

void *inthash_search(const inthash_t *Map, uintptr_t Key) {
	inthash_node_t *Nodes = Map->Nodes;
	if (Nodes) {
		size_t Mask = Map->Size - 1;
		size_t Index = (Key >> 5) & Mask;
		size_t Incr = (Key >> 9) | 1;
		for (;;) {
			if (Nodes[Index].Key == Key) return Nodes[Index].Value;
			if (Nodes[Index].Key < Key) break;
			Index = (Index + Incr) & Mask;
		}
	}
	return NULL;
}

static void inthash_nodes_sort(inthash_node_t *A, inthash_node_t *B) {
	inthash_node_t *A1 = A, *B1 = B;
	inthash_node_t Temp = *A;
	inthash_node_t Pivot = *B;
	while (A1 < B1) {
		if (Temp.Key > Pivot.Key) {
			*A1 = Temp;
			++A1;
			Temp = *A1;
		} else {
			*B1 = Temp;
			--B1;
			Temp = *B1;
		}
	}
	*A1 = Pivot;
	if (A1 - A > 1) inthash_nodes_sort(A, A1 - 1);
	if (B - B1 > 1) inthash_nodes_sort(B1 + 1, B);
}

void *inthash_insert(inthash_t *Map, uintptr_t Key, void *Value) {
	inthash_node_t *Nodes = Map->Nodes;
	if (!Nodes) {
		Nodes = Map->Nodes = anew(inthash_node_t, 4);
		Map->Size = 4;
		Map->Space = 3;
		size_t Index =  (Key >> 5) & 3;
		Nodes[Index].Key = Key;
		void *Old = Nodes[Index].Value;
		Nodes[Index].Value = Value;
		return Old;
	}
	size_t Mask = Map->Size - 1;
	size_t Index = (Key >> 5) & Mask;
	size_t Incr = (Key >> 9) | 1;
	for (;;) {
		if (Nodes[Index].Key == Key) {
			void *Old = Nodes[Index].Value;
			Nodes[Index].Value = Value;
			return Old;
		}
		if (Nodes[Index].Key < Key) break;
		Index = (Index + Incr) & Mask;
	}
	if (--Map->Space > 1) {
		uintptr_t Key1 = Nodes[Index].Key;
		void *Value1 = Nodes[Index].Value;
		Nodes[Index].Key = Key;
		Nodes[Index].Value = Value;
		while (Key1) {
			Incr = (Key1 >> 9) | 1;
			while (Nodes[Index].Key > Key1) Index = (Index + Incr) & Mask;
			uintptr_t Key2 = Nodes[Index].Key;
			void *Value2 = Nodes[Index].Value;
			Nodes[Index].Key = Key1;
			Nodes[Index].Value = Value1;
			Key1 = Key2;
			Value1 = Value2;
		}
	} else {
		while (Nodes[Index].Key) Index = (Index + 1) & Mask;
		Nodes[Index].Key = Key;
		Nodes[Index].Value = Value;
		inthash_nodes_sort(Nodes, Nodes + Mask);
		size_t Size2 = 2 * Map->Size;
		Mask = Size2 - 1;
		inthash_node_t *Nodes2 = anew(inthash_node_t, Size2);
		for (inthash_node_t *Node = Nodes; Node->Key; Node++) {
			uintptr_t Key2 = Node->Key;
			void *Value2 = Node->Value;
			size_t Index2 = (Key2 >> 5) & Mask;
			size_t Incr2 = (Key2 >> 9) | 1;
			while (Nodes2[Index2].Key) Index2 = (Index2 + Incr2) & Mask;
			Nodes2[Index2].Key = Key2;
			Nodes2[Index2].Value = Value2;
		}
		Map->Nodes = Nodes2;
		Map->Space += Map->Size;
		Map->Size = Size2;
	}
	return NULL;
}

int inthash_foreach(inthash_t *Map, void *Data, int (*callback)(uintptr_t, void *, void *)) {
	return 0;
}

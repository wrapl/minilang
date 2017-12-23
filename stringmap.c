#include "stringmap.h"
#include <gc.h>
#include <string.h>

#define new(T) ((T *)GC_MALLOC(sizeof(T)))

struct stringmap_node_t {
	stringmap_node_t *Left, *Right;
	const char *Key;
	void *Value;
	long Hash;
	int Depth;
};

static long stringmap_hash(const char *Key) {
	long Hash = 5381;
	for (const char *P = Key; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

static inline int compare(long Hash, const char *Key, stringmap_node_t *Node) {
	if (Hash < Node->Hash) return -1;
	if (Hash > Node->Hash) return 1;
	return strcmp(Key, Node->Key);
}

void *stringmap_search(stringmap_t *Tree, const char *Key) {
	stringmap_node_t *Node = Tree->Root;
	long Hash = stringmap_hash(Key);
	while (Node) {
		int Compare = compare(Hash, Key, Node);
		if (!Compare) {
			return Node->Value;
		} else {
			Node = Compare < 0 ? Node->Left : Node->Right;
		}
	}
	return 0;
}

static int stringmap_balance(stringmap_node_t *Node) {
	int Delta = 0;
	if (Node->Left) Delta = Node->Left->Depth;
	if (Node->Right) Delta -= Node->Right->Depth;
	return Delta;
}

static void stringmap_update_depth(stringmap_node_t *Node) {
	int Depth = 0;
	if (Node->Left) Depth = Node->Left->Depth;
	if (Node->Right && Depth < Node->Right->Depth) Depth = Node->Right->Depth;
	Node->Depth = Depth + 1;
}

static void stringmap_rotate_left(stringmap_node_t **Slot) {
	stringmap_node_t *Ch = Slot[0]->Right;
	Slot[0]->Right = Slot[0]->Right->Left;
	Ch->Left = Slot[0];
	stringmap_update_depth(Slot[0]);
	Slot[0] = Ch;
	stringmap_update_depth(Slot[0]);
}

static void stringmap_rotate_right(stringmap_node_t **Slot) {
	stringmap_node_t *Ch = Slot[0]->Left;
	Slot[0]->Left = Slot[0]->Left->Right;
	Ch->Right = Slot[0];
	stringmap_update_depth(Slot[0]);
	Slot[0] = Ch;
	stringmap_update_depth(Slot[0]);
}

static void stringmap_rebalance(stringmap_node_t **Slot) {
	int Delta = stringmap_balance(Slot[0]);
	if (Delta == 2) {
		if (stringmap_balance(Slot[0]->Left) < 0) stringmap_rotate_left(&Slot[0]->Left);
		stringmap_rotate_right(Slot);
	} else if (Delta == -2) {
		if (stringmap_balance(Slot[0]->Right) > 0) stringmap_rotate_right(&Slot[0]->Right);
		stringmap_rotate_left(Slot);
	}
}

static void *stringmap_insert_internal(stringmap_t *Tree, stringmap_node_t **Slot, long Hash, const char *Key, void *Value) {
	if (!Slot[0]) {
		stringmap_node_t *Node = Slot[0] = new(stringmap_node_t);
		Node->Depth = 1;
		Node->Hash = Hash;
		Node->Key = Key;
		Node->Value = Value;
		return 0;
	}
	int Compare = compare(Hash, Key, Slot[0]);
	if (!Compare) {
		void *Old = Slot[0]->Value;
		Slot[0]->Value = Value;
		return Old;
	} else {
		void *Old = stringmap_insert_internal(Tree, Compare < 0 ? &Slot[0]->Left : &Slot[0]->Right, Hash, Key, Value);
		stringmap_rebalance(Slot);
		stringmap_update_depth(Slot[0]);
		return Old;
	}
}

void *stringmap_insert(stringmap_t *Tree, const char *Key, void *Value) {
	return stringmap_insert_internal(Tree, &Tree->Root, stringmap_hash(Key), Key, Value);
}

static void stringmap_remove_depth_helper(stringmap_node_t *Node) {
	if (Node) {
		stringmap_remove_depth_helper(Node->Right);
		stringmap_update_depth(Node);
	}
}

void *stringmap_remove_internal(stringmap_t *Tree, stringmap_node_t **Slot, long Hash, const char *Key) {
	if (!Slot[0]) return 0;
	int Compare = compare(Hash, Key, Slot[0]);
	void *Removed = 0;
	if (!Compare) {
		Removed = Slot[0]->Value;
		if (Slot[0]->Left && Slot[0]->Right) {
			stringmap_node_t **Y = &Slot[0]->Left;
			while (Y[0]->Right) Y = &Y[0]->Right;
			Slot[0]->Key = Y[0]->Key;
			Slot[0]->Hash = Y[0]->Hash;
			Slot[0]->Value = Y[0]->Value;
			Y[0] = Y[0]->Left;
			stringmap_remove_depth_helper(Slot[0]->Left);
		} else if (Slot[0]->Left) {
			Slot[0] = Slot[0]->Left;
		} else if (Slot[0]->Right) {
			Slot[0] = Slot[0]->Right;
		} else {
			Slot[0] = 0;
		}
	} else {
		Removed = stringmap_remove_internal(Tree, Compare < 0 ? &Slot[0]->Left : &Slot[0]->Right, Hash, Key);
	}
	if (Slot[0]) {
		stringmap_update_depth(Slot[0]);
		stringmap_rebalance(Slot);
	}
	return Removed;
}

void *stringmap_remove(stringmap_t *Tree, const char *Key) {
	return stringmap_remove_internal(Tree, &Tree->Root, stringmap_hash(Key), Key);
}

static int stringmap_node_foreach(stringmap_node_t *Node, void *Data, int (*callback)(const char *, void *, void *)) {
	if (callback(Node->Key, Node->Value, Data)) return 1;
	if (Node->Left && stringmap_node_foreach(Node->Left, Data, callback)) return 1;
	if (Node->Right && stringmap_node_foreach(Node->Right, Data, callback)) return 1;
	return 0;
}

int stringmap_foreach(stringmap_t *Tree, void *Data, int (*callback)(const char *, void *, void *)) {
	return Tree->Root ? stringmap_node_foreach(Tree->Root, Data, callback) : 0;
}

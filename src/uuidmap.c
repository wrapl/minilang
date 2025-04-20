#include "uuidmap.h"
#include "ml_macros.h"
#include <string.h>

struct uuidmap_node_t {
	uuidmap_node_t *Left, *Right;
	uuid_t Key;
	void *Value;
	int Depth;
};

uuidmap_t *uuidmap_new() {
	return new(uuidmap_t);
}

static int uuidmap_copy_fn(const uuid_t Key, void *Value, void *Copy) {
	uuidmap_insert((uuidmap_t *)Copy, Key, Value);
	return 0;
}

uuidmap_t *uuidmap_copy(uuidmap_t *Map) {
	uuidmap_t *Copy = uuidmap_new();
	uuidmap_foreach(Map, Copy, uuidmap_copy_fn);
	return Copy;
}

static inline int compare(const uuid_t Key, uuidmap_node_t *Node) {
	return memcmp(Key, Node->Key, sizeof(uuid_t));
}

void *uuidmap_search(const uuidmap_t *Map, const uuid_t Key) {
	uuidmap_node_t *Node = Map->Root;
	while (Node) {
		int Compare = compare(Key, Node);
		if (!Compare) {
			return Node->Value;
		} else {
			Node = Compare < 0 ? Node->Left : Node->Right;
		}
	}
	return 0;
}

static int uuidmap_balance(uuidmap_node_t *Node) {
	int Delta = 0;
	if (Node->Left) Delta = Node->Left->Depth;
	if (Node->Right) Delta -= Node->Right->Depth;
	return Delta;
}

static void uuidmap_update_depth(uuidmap_node_t *Node) {
	int Depth = 0;
	if (Node->Left) Depth = Node->Left->Depth;
	if (Node->Right && Depth < Node->Right->Depth) Depth = Node->Right->Depth;
	Node->Depth = Depth + 1;
}

static void uuidmap_rotate_left(uuidmap_node_t **Slot) {
	uuidmap_node_t *Ch = Slot[0]->Right;
	Slot[0]->Right = Slot[0]->Right->Left;
	Ch->Left = Slot[0];
	uuidmap_update_depth(Slot[0]);
	Slot[0] = Ch;
	uuidmap_update_depth(Slot[0]);
}

static void uuidmap_rotate_right(uuidmap_node_t **Slot) {
	uuidmap_node_t *Ch = Slot[0]->Left;
	Slot[0]->Left = Slot[0]->Left->Right;
	Ch->Right = Slot[0];
	uuidmap_update_depth(Slot[0]);
	Slot[0] = Ch;
	uuidmap_update_depth(Slot[0]);
}

static void uuidmap_rebalance(uuidmap_node_t **Slot) {
	int Delta = uuidmap_balance(Slot[0]);
	if (Delta == 2) {
		if (uuidmap_balance(Slot[0]->Left) < 0) uuidmap_rotate_left(&Slot[0]->Left);
		uuidmap_rotate_right(Slot);
	} else if (Delta == -2) {
		if (uuidmap_balance(Slot[0]->Right) > 0) uuidmap_rotate_right(&Slot[0]->Right);
		uuidmap_rotate_left(Slot);
	}
}

static void **uuidmap_slot_internal(uuidmap_t *Map, uuidmap_node_t **Slot, const uuid_t Key) {
	if (!Slot[0]) {
		uuidmap_node_t *Node = Slot[0] = new(uuidmap_node_t);
		Node->Depth = 1;
		memcpy(Node->Key, Key, sizeof(uuid_t));
		++Map->Size;
		return &(Node->Value);
	}
	int Compare = compare(Key, Slot[0]);
	if (!Compare) {
		return &Slot[0]->Value;
	} else {
		void **Result = uuidmap_slot_internal(Map, Compare < 0 ? &Slot[0]->Left : &Slot[0]->Right, Key);
		uuidmap_rebalance(Slot);
		uuidmap_update_depth(Slot[0]);
		return Result;
	}
}

void **uuidmap_slot(uuidmap_t *Map, const uuid_t Key) {
	return uuidmap_slot_internal(Map, &Map->Root, Key);
}

void *uuidmap_insert(uuidmap_t *Map, const uuid_t Key, void *Value) {
	void **Slot = uuidmap_slot(Map, Key);
	void *Old = Slot[0];
	Slot[0] = Value;
	return Old;
}

static void uuidmap_remove_depth_helper(uuidmap_node_t *Node) {
	if (Node) {
		uuidmap_remove_depth_helper(Node->Right);
		uuidmap_update_depth(Node);
	}
}

static void *uuidmap_remove_internal(uuidmap_t *Map, uuidmap_node_t **Slot, const uuid_t Key) {
	if (!Slot[0]) return 0;
	int Compare = compare(Key, Slot[0]);
	void *Removed = 0;
	if (!Compare) {
		Removed = Slot[0]->Value;
		if (Slot[0]->Left && Slot[0]->Right) {
			uuidmap_node_t **Y = &Slot[0]->Left;
			while (Y[0]->Right) Y = &Y[0]->Right;
			memcpy(Slot[0]->Key, Y[0]->Key, sizeof(uuid_t));
			Slot[0]->Value = Y[0]->Value;
			Y[0] = Y[0]->Left;
			uuidmap_remove_depth_helper(Slot[0]->Left);
		} else if (Slot[0]->Left) {
			Slot[0] = Slot[0]->Left;
		} else if (Slot[0]->Right) {
			Slot[0] = Slot[0]->Right;
		} else {
			Slot[0] = 0;
		}
		--Map->Size;
	} else {
		Removed = uuidmap_remove_internal(Map, Compare < 0 ? &Slot[0]->Left : &Slot[0]->Right, Key);
	}
	if (Slot[0]) {
		uuidmap_update_depth(Slot[0]);
		uuidmap_rebalance(Slot);
	}
	return Removed;
}

void *uuidmap_remove(uuidmap_t *Map, const uuid_t Key) {
	return uuidmap_remove_internal(Map, &Map->Root, Key);
}

static int uuidmap_node_foreach(uuidmap_node_t *Node, void *Data, int (*callback)(const uuid_t, void *, void *)) {
	if (callback(Node->Key, Node->Value, Data)) return 1;
	if (Node->Left && uuidmap_node_foreach(Node->Left, Data, callback)) return 1;
	if (Node->Right && uuidmap_node_foreach(Node->Right, Data, callback)) return 1;
	return 0;
}

int uuidmap_foreach(uuidmap_t *Map, void *Data, int (*callback)(const uuid_t, void *, void *)) {
	return Map->Root ? uuidmap_node_foreach(Map->Root, Data, callback) : 0;
}

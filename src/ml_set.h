#ifndef ML_SET_H
#define ML_SET_H

#include "ml_types.h"

// Sets //

typedef struct ml_set_t ml_set_t;
typedef struct ml_set_node_t ml_set_node_t;

extern ml_type_t MLSetT[];

struct ml_set_t {
	ml_type_t *Type;
	ml_set_node_t *Head, *Tail, *Root;
	ml_method_cached_t *Cached;
	int Size, Order;
};

struct ml_set_node_t {
	ml_type_t *Type;
	ml_set_node_t *Next, *Prev;
	ml_value_t *Key;
	ml_set_node_t *Left, *Right;
	long Hash;
	int Depth;
};

ml_value_t *ml_set() __attribute__((malloc));
ml_value_t *ml_set_search(ml_value_t *Set, ml_value_t *Key);
ml_value_t *ml_set_search0(ml_value_t *Set, ml_value_t *Key);
ml_set_node_t *ml_set_slot(ml_value_t *Set, ml_value_t *Key);
ml_value_t *ml_set_insert(ml_value_t *Set, ml_value_t *Key);
ml_value_t *ml_set_delete(ml_value_t *Set, ml_value_t *Key);

static inline int ml_set_size(ml_value_t *Set) {
	return ((ml_set_t *)Set)->Size;
}

int ml_set_foreach(ml_value_t *Set, void *Data, int (*callback)(ml_value_t *, void *));

typedef struct {
	ml_set_node_t *Node;
	ml_value_t *Key;
} ml_set_iter_t;

static inline int ml_set_iter_forward(ml_value_t *Set0, ml_set_iter_t *Iter) {
	ml_set_t *Set = (ml_set_t *)Set0;
	ml_set_node_t *Node = Iter->Node = Set->Head;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		return 1;
	}
}

static inline int ml_set_iter_next(ml_set_iter_t *Iter) {
	ml_set_node_t *Node = Iter->Node = Iter->Node->Next;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		return 1;
	}
}

static inline int ml_set_iter_backward(ml_value_t *Set0, ml_set_iter_t *Iter) {
	ml_set_t *Set = (ml_set_t *)Set0;
	ml_set_node_t *Node = Iter->Node = Set->Tail;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		return 1;
	}
}

static inline int ml_set_iter_prev(ml_set_iter_t *Iter) {
	ml_set_node_t *Node = Iter->Node = Iter->Node->Prev;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		return 1;
	}
}

static inline int ml_set_iter_valid(ml_set_iter_t *Iter) {
	return Iter->Node != NULL;
}

#define ML_SET_FOREACH(SET, ITER) \
	for (ml_set_node_t *ITER = ((ml_set_t *)SET)->Head; ITER; ITER = ITER->Next)

void ml_set_init();

#endif

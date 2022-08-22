#include "ml_set.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include "ml_sequence.h"
#include "ml_method.h"
#include "ml_object.h"

#undef ML_CATEGORY
#define ML_CATEGORY "set"

ML_TYPE(MLSetT, (MLSequenceT), "set",
// A set of values.
// Values can be of any type supporting hashing and comparison.
// By default, iterating over a set generates the values in the order they were inserted, however this ordering can be changed.
);

ML_ENUM2(MLSetOrderT, "set::order",
// * :mini:`set::order::Insert` |harr| default ordering; inserted values are put at end, no reordering on access.
// * :mini:`set::order::Ascending` |harr| inserted values are kept in ascending order, no reordering on access.
// * :mini:`set::order::Ascending` |harr| inserted values are kept in descending order, no reordering on access.
// * :mini:`set::order::MRU` |harr| inserted values are put at start, accessed values are moved to start.
// * :mini:`set::order::LRU` |harr| inserted values are put at end, accessed values are moved to end.
	"Insert", SET_ORDER_INSERT,
	"LRU", SET_ORDER_LRU,
	"MRU", SET_ORDER_MRU,
	"Ascending", SET_ORDER_ASC,
	"Descending", SET_ORDER_DESC
);

static void ML_TYPED_FN(ml_value_find_refs, MLSetT, ml_value_t *Value, void *Data, ml_value_ref_fn RefFn, int RefsOnly) {
	if (!RefFn(Data, Value)) return;
	ML_SET_FOREACH(Value, Iter) ml_value_find_refs(Iter->Key, Data, RefFn, RefsOnly);
}

ML_TYPE(MLSetNodeT, (), "set-node");
//!internal

ml_value_t *ml_set() {
	ml_set_t *Set = new(ml_set_t);
	Set->Type = MLSetT;
	return (ml_value_t *)Set;
}

ML_METHOD(MLSetT) {
//>set
// Returns a new set.
//$= set()
	return ml_set();
}

static void set_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void set_iter_value(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_set_insert(State->Values[0], Value);
	State->Base.run = (void *)set_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void set_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[0]);
	State->Base.run = (void *)set_iter_value;
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_METHODVX(MLSetT, MLSequenceT) {
//<Sequence
//>set
// Returns a set of all the values produced by :mini:`Sequence`.
//$= set("cake")
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)set_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_set();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

ML_METHODVX("grow", MLSetT, MLSequenceT) {
//<Set
//<Sequence
//>set
// Adds of all the values produced by :mini:`Sequence` to :mini:`Set` and returns :mini:`Set`.
//$= set("cake"):grow("banana")
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)set_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = Args[0];
	return ml_iterate((ml_state_t *)State, ml_chained(Count - 1, Args + 1));
}

extern ml_value_t *CompareMethod;

static inline ml_value_t *ml_set_compare(ml_set_t *Set, ml_value_t **Args) {
	/*ml_method_cached_t *Cached = Set->Cached;
	if (Cached) {
		if (Cached->Types[0] != ml_typeof(Args[0])) Cached = NULL;
		if (Cached->Types[1] != ml_typeof(Args[1])) Cached = NULL;
	}
	if (!Cached || !Cached->Callback) {
		Cached = ml_method_search_cached(NULL, (ml_method_t *)CompareMethod, 2, Args);
		if (!Cached) return ml_no_method_error((ml_method_t *)CompareMethod, 2, Args);
		Set->Cached = Cached;
	}
	return ml_simple_call(Cached->Callback, 2, Args);*/
	return ml_simple_call(CompareMethod, 2, Args);
}

static ml_set_node_t *ml_set_find_node(ml_set_t *Set, ml_value_t *Key) {
	ml_set_node_t *Node = Set->Root;
	long Hash = ml_typeof(Key)->hash(Key, NULL);
	ml_method_cached_t *Cached = Set->Cached;
	if (Cached && Cached->Callback) {
		if (Cached->Types[0] != ml_typeof(Key)) Cached = NULL;
	}
	while (Node) {
		int Compare;
		if (Hash < Node->Hash) {
			Compare = -1;
		} else if (Hash > Node->Hash) {
			Compare = 1;
		} else {
			ml_value_t *Args[2] = {Key, Node->Key};
			ml_value_t *Result = ml_set_compare(Set, Args);
			if (ml_is_error(Result)) return NULL;
			Compare = ml_integer_value(Result);
		}
		if (!Compare) {
			return Node;
		} else {
			Node = Compare < 0 ? Node->Left : Node->Right;
		}
	}
	return NULL;
}

ml_value_t *ml_set_search(ml_value_t *Set0, ml_value_t *Key) {
	ml_set_node_t *Node = ml_set_find_node((ml_set_t *)Set0, Key);
	return Node ? MLSome : MLNil;
}

ml_value_t *ml_set_search0(ml_value_t *Set0, ml_value_t *Key) {
	ml_set_node_t *Node = ml_set_find_node((ml_set_t *)Set0, Key);
	return Node ? MLSome : NULL;
}

static int ml_set_balance(ml_set_node_t *Node) {
	int Delta = 0;
	if (Node->Left) Delta = Node->Left->Depth;
	if (Node->Right) Delta -= Node->Right->Depth;
	return Delta;
}

static void ml_set_update_depth(ml_set_node_t *Node) {
	int Depth = 0;
	if (Node->Left) Depth = Node->Left->Depth;
	if (Node->Right && Depth < Node->Right->Depth) Depth = Node->Right->Depth;
	Node->Depth = Depth + 1;
}

static void ml_set_rotate_left(ml_set_node_t **Slot) {
	ml_set_node_t *Ch = Slot[0]->Right;
	Slot[0]->Right = Slot[0]->Right->Left;
	Ch->Left = Slot[0];
	ml_set_update_depth(Slot[0]);
	Slot[0] = Ch;
	ml_set_update_depth(Slot[0]);
}

static void ml_set_rotate_right(ml_set_node_t **Slot) {
	ml_set_node_t *Ch = Slot[0]->Left;
	Slot[0]->Left = Slot[0]->Left->Right;
	Ch->Right = Slot[0];
	ml_set_update_depth(Slot[0]);
	Slot[0] = Ch;
	ml_set_update_depth(Slot[0]);
}

static void ml_set_rebalance(ml_set_node_t **Slot) {
	int Delta = ml_set_balance(Slot[0]);
	if (Delta == 2) {
		if (ml_set_balance(Slot[0]->Left) < 0) ml_set_rotate_left(&Slot[0]->Left);
		ml_set_rotate_right(Slot);
	} else if (Delta == -2) {
		if (ml_set_balance(Slot[0]->Right) > 0) ml_set_rotate_right(&Slot[0]->Right);
		ml_set_rotate_left(Slot);
	}
}

static void ml_set_insert_before(ml_set_t *Set, ml_set_node_t *Parent, ml_set_node_t *Node) {
	Node->Next = Parent;
	Node->Prev = Parent->Prev;
	if (Parent->Prev) {
		Parent->Prev->Next = Node;
	} else {
		Set->Head = Node;
	}
	Parent->Prev = Node;
}

static void ml_set_insert_after(ml_set_t *Set, ml_set_node_t *Parent, ml_set_node_t *Node) {
	Node->Prev = Parent;
	Node->Next = Parent->Next;
	if (Parent->Next) {
		Parent->Next->Prev = Node;
	} else {
		Set->Tail = Node;
	}
	Parent->Next = Node;
}

static ml_set_node_t *ml_set_node_child(ml_set_t *Set, ml_set_node_t *Parent, long Hash, ml_value_t *Key) {
	int Compare;
	if (Hash < Parent->Hash) {
		Compare = -1;
	} else if (Hash > Parent->Hash) {
		Compare = 1;
	} else {
		ml_value_t *Args[2] = {Key, Parent->Key};
		ml_value_t *Result = ml_set_compare(Set, Args);
		Compare = ml_integer_value(Result);
	}
	if (!Compare) return Parent;
	ml_set_node_t **Slot = Compare < 0 ? &Parent->Left : &Parent->Right;
	ml_set_node_t *Node;
	if (Slot[0]) {
		Node = ml_set_node_child(Set, Slot[0], Hash, Key);
	} else {
		++Set->Size;
		Node = Slot[0] = new(ml_set_node_t);
		Node->Type = MLSetNodeT;
		Node->Depth = 1;
		Node->Hash = Hash;
		Node->Key = Key;
		switch (Set->Order) {
		case SET_ORDER_INSERT:
		case SET_ORDER_LRU: {
			ml_set_node_t *Prev = Set->Tail;
			Prev->Next = Node;
			Node->Prev = Prev;
			Set->Tail = Node;
			break;
		}
		case SET_ORDER_MRU: {
			ml_set_node_t *Next = Set->Head;
			Next->Prev = Node;
			Node->Next = Next;
			Set->Head = Node;
			break;
		}
		case SET_ORDER_ASC: {
			if (Compare < 0) {
				ml_set_insert_before(Set, Parent, Node);
			} else {
				ml_set_insert_after(Set, Parent, Node);
			}
			break;
		}
		case SET_ORDER_DESC: {
			if (Compare > 0) {
				ml_set_insert_before(Set, Parent, Node);
			} else {
				ml_set_insert_after(Set, Parent, Node);
			}
			break;
		}
		}
	}
	ml_set_rebalance(Slot);
	ml_set_update_depth(Slot[0]);
	return Node;
}

static ml_set_node_t *ml_set_node(ml_set_t *Set, long Hash, ml_value_t *Key) {
	ml_set_node_t *Root = Set->Root;
	if (Root) return ml_set_node_child(Set, Root, Hash, Key);
	++Set->Size;
	ml_set_node_t *Node = Set->Root = new(ml_set_node_t);
	Node->Type = MLSetNodeT;
	Set->Head = Set->Tail = Node;
	Node->Depth = 1;
	Node->Hash = Hash;
	Node->Key = Key;
	return Node;
}

ml_set_node_t *ml_set_slot(ml_value_t *Set0, ml_value_t *Key) {
	ml_set_t *Set = (ml_set_t *)Set0;
	return ml_set_node(Set, ml_typeof(Key)->hash(Key, NULL), Key);
}

ml_value_t *ml_set_insert(ml_value_t *Set0, ml_value_t *Key) {
	ml_set_t *Set = (ml_set_t *)Set0;
	int Size = Set->Size;
	ml_set_node(Set, ml_typeof(Key)->hash(Key, NULL), Key);
#ifdef ML_GENERICS
	if (Set->Type->Type != MLTypeGenericT) {
		Set->Type = ml_generic_type(2, (ml_type_t *[]){Set->Type, ml_typeof(Key)});
	} else {
		ml_type_t *BaseType = ml_generic_type_args(Set->Type)[0];
		ml_type_t *KeyType = ml_generic_type_args(Set->Type)[1];
		if (KeyType != ml_typeof(Key)) {
			ml_type_t *KeyType2 = ml_type_max(KeyType, ml_typeof(Key));
			if (KeyType != KeyType2) {
				Set->Type = ml_generic_type(2, (ml_type_t *[]){BaseType, KeyType2});
			}
		}
	}
#endif
	return Set->Size == Size ? MLSome : MLNil;
}

static void ml_set_remove_depth_helper(ml_set_node_t *Node) {
	if (Node) {
		ml_set_remove_depth_helper(Node->Right);
		ml_set_update_depth(Node);
	}
}

static ml_value_t *ml_set_remove_internal(ml_set_t *Set, ml_set_node_t **Slot, long Hash, ml_value_t *Key) {
	if (!Slot[0]) return MLNil;
	ml_set_node_t *Node = Slot[0];
	int Compare;
	if (Hash < Node->Hash) {
		Compare = -1;
	} else if (Hash > Node->Hash) {
		Compare = 1;
	} else {
		ml_value_t *Args[2] = {Key, Node->Key};
		ml_value_t *Result = ml_set_compare(Set, Args);
		Compare = ml_integer_value(Result);
	}
	ml_value_t *Removed = MLNil;
	if (!Compare) {
		--Set->Size;
		Removed = MLSome;
		if (Node->Prev) Node->Prev->Next = Node->Next; else Set->Head = Node->Next;
		if (Node->Next) Node->Next->Prev = Node->Prev; else Set->Tail = Node->Prev;
		if (Node->Left && Node->Right) {
			ml_set_node_t **Y = &Node->Left;
			while (Y[0]->Right) Y = &Y[0]->Right;
			ml_set_node_t *Node2 = Y[0];
			Y[0] = Node2->Left;
			Node2->Left = Node->Left;
			Node2->Right = Node->Right;
			Slot[0] = Node2;
			ml_set_remove_depth_helper(Node2->Left);
		} else if (Node->Left) {
			Slot[0] = Node->Left;
		} else if (Node->Right) {
			Slot[0] = Node->Right;
		} else {
			Slot[0] = 0;
		}
	} else {
		Removed = ml_set_remove_internal(Set, Compare < 0 ? &Node->Left : &Node->Right, Hash, Key);
	}
	if (Slot[0]) {
		ml_set_update_depth(Slot[0]);
		ml_set_rebalance(Slot);
	}
	return Removed;
}

ml_value_t *ml_set_delete(ml_value_t *Set0, ml_value_t *Key) {
	ml_set_t *Set = (ml_set_t *)Set0;
	return ml_set_remove_internal(Set, &Set->Root, ml_typeof(Key)->hash(Key, NULL), Key);
}

int ml_set_foreach(ml_value_t *Value, void *Data, int (*callback)(ml_value_t *, void *)) {
	ml_set_t *Set = (ml_set_t *)Value;
	for (ml_set_node_t *Node = Set->Head; Node; Node = Node->Next) {
		if (callback(Node->Key, Data)) return 1;
	}
	return 0;
}

ML_METHOD("size", MLSetT) {
//<Set
//>integer
// Returns the number of values in :mini:`Set`.
//$= set(["A", "B", "C"]):size
	ml_set_t *Set = (ml_set_t *)Args[0];
	return ml_integer(Set->Size);
}

ML_METHOD("count", MLSetT) {
//<Set
//>integer
// Returns the number of values in :mini:`Set`.
//$= set(["A", "B", "C"]):count
	ml_set_t *Set = (ml_set_t *)Args[0];
	return ml_integer(Set->Size);
}

static ml_value_t *ml_set_index_deref(ml_set_node_t *Index) {
	return MLNil;
}


static ml_set_node_t *ml_set_insert_node(ml_set_t *Set, ml_set_node_t **Slot, long Hash, ml_set_node_t *Index) {
	if (!Slot[0]) {
		++Set->Size;
		ml_set_node_t *Node = Slot[0] = Index;
		Node->Type = MLSetNodeT;
		ml_set_node_t *Prev = Set->Tail;
		if (Prev) {
			Prev->Next = Node;
			Node->Prev = Prev;
		} else {
			Set->Head = Node;
		}
		Set->Tail = Node;
		Node->Depth = 1;
		Node->Hash = Hash;
		return Node;
	}
	int Compare;
	if (Hash < Slot[0]->Hash) {
		Compare = -1;
	} else if (Hash > Slot[0]->Hash) {
		Compare = 1;
	} else {
		ml_value_t *Args[2] = {Index->Key, Slot[0]->Key};
		ml_value_t *Result = ml_set_compare(Set, Args);
		Compare = ml_integer_value(Result);
	}
	if (!Compare) {
		return Slot[0];
	} else {
		ml_set_node_t *Node = ml_set_insert_node(Set, Compare < 0 ? &Slot[0]->Left : &Slot[0]->Right, Hash, Index);
		ml_set_rebalance(Slot);
		ml_set_update_depth(Slot[0]);
		return Node;
	}
}

ML_METHOD("order", MLSetT) {
//<Set
//>set::order
// Returns the current ordering of :mini:`Set`.
	ml_set_t *Set = (ml_set_t *)Args[0];
	return ml_enum_value(MLSetOrderT, Set->Order);
}

ML_METHOD("order", MLSetT, MLSetOrderT) {
//<Set
//<Order
//>set
// Sets the ordering
	ml_set_t *Set = (ml_set_t *)Args[0];
	Set->Order = ml_enum_value_value(Args[1]);
	return (ml_value_t *)Set;
}

static void ml_set_move_node_head(ml_set_t *Set, ml_set_node_t *Node) {
	ml_set_node_t *Prev = Node->Prev;
	if (Prev) {
		ml_set_node_t *Next = Node->Next;
		Prev->Next = Next;
		if (Next) {
			Next->Prev = Prev;
		} else {
			Set->Tail = Prev;
		}
		Node->Next = Set->Head;
		Node->Prev = NULL;
		Set->Head->Prev = Node;
		Set->Head = Node;
	}
}

static void ml_set_move_node_tail(ml_set_t *Set, ml_set_node_t *Node) {
	ml_set_node_t *Next = Node->Next;
	if (Next) {
		ml_set_node_t *Prev = Node->Prev;
		Next->Prev = Prev;
		if (Prev) {
			Prev->Next = Next;
		} else {
			Set->Head = Next;
		}
		Node->Prev = Set->Tail;
		Node->Next = NULL;
		Set->Tail->Next = Node;
		Set->Tail = Node;
	}
}

ML_METHOD("[]", MLSetT, MLAnyT) {
//<Set
//<Key
//>some|nil
// Returns the node corresponding to :mini:`Key` in :mini:`Set`. If :mini:`Key` is not in :mini:`Set` then a new floating node is returned with value :mini:`nil`. This node will insert :mini:`Key` into :mini:`Set` if assigned.
//$- let M := set(["A", "B", "C"])
//$= M["A"]
//$= M["D"]
//$= M
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_set_node_t *Node = ml_set_find_node(Set, Args[1]);
	if (!Node) return MLNil;
	if (Set->Order == SET_ORDER_LRU) {
		ml_set_move_node_tail(Set, Node);
	} else if (Set->Order == SET_ORDER_MRU) {
		ml_set_move_node_head(Set, Node);
	}
	return MLSome;
}

ML_METHOD("in", MLAnyT, MLSetT) {
//<Key
//<Set
//>any|nil
// Returns :mini:`Key` if it is in :mini:`Map`, otherwise return :mini:`nil`.
//$- let S := set(["A", "B", "C"])
//$= "A" in S
//$= "D" in S
	ml_set_t *Set = (ml_set_t *)Args[1];
	return ml_set_find_node(Set, Args[0]) ? Args[0] : MLNil;
}

ML_METHOD("empty", MLSetT) {
//<Set
//>set
// Deletes all values from :mini:`Set` and returns it.
//$= let M := set(["A", "B", "C"])
//$= M:empty
	ml_set_t *Set = (ml_set_t *)Args[0];
	Set->Root = Set->Head = Set->Tail = NULL;
	Set->Size = 0;
#ifdef ML_GENERICS
	Set->Type = MLSetT;
#endif
	return (ml_value_t *)Set;
}

ML_METHOD("pop", MLSetT) {
//<Set
//>any|nil
// Deletes the first value from :mini:`Set` according to its iteration order. Returns the deleted value, or :mini:`nil` if :mini:`Set` is empty.
//$- :> Insertion order (default)
//$= let M1 := set("cake")
//$= M1:pop
//$= M1
//$-
//$- :> LRU order
//$= let M2 := set("cake"):order(set::order::LRU)
//$- M2[2]; M2[4]; M2[1]; M2[3]
//$= M2:pop
//$= M2
//$-
//$- :> MRU order
//$= let M3 := set("cake"):order(set::order::MRU)
//$- M3[2]; M3[4]; M3[1]; M3[3]
//$= M3:pop
//$= M3
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_set_node_t *Node = Set->Head;
	if (!Node) return MLNil;
	ml_set_delete(Args[0], Node->Key);
	return Node->Key;
}

ML_METHOD("pull", MLSetT) {
//<Set
//>any|nil
// Deletes the last value from :mini:`Set` according to its iteration order. Returns the deleted value, or :mini:`nil` if :mini:`Set` is empty.
//$- :> Insertion order (default)
//$= let M1 := set("cake")
//$= M1:pull
//$= M1
//$-
//$- :> LRU order
//$= let M2 := set("cake"):order(set::order::LRU)
//$- M2[2]; M2[4]; M2[1]; M2[3]
//$= M2:pull
//$= M2
//$-
//$- :> MRU order
//$= let M3 := set("cake"):order(set::order::MRU)
//$- M3[2]; M3[4]; M3[1]; M3[3]
//$= M3:pull
//$= M3
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_set_node_t *Node = Set->Tail;
	if (!Node) return MLNil;
	ml_set_delete(Args[0], Node->Key);
	return Node->Key;
}

ML_METHOD("insert", MLSetT, MLAnyT) {
//<Set
//<Key
//<Value
//>some|nil
// Inserts :mini:`Key` into :mini:`Set` with corresponding value :mini:`Value`.
// Returns the previous value associated with :mini:`Key` if any, otherwise :mini:`nil`.
//$- let M := set(["A", "B", "C"])
//$= M:insert("A")
//$= M:insert("D")
//$= M
	ml_value_t *Set = (ml_value_t *)Args[0];
	ml_value_t *Key = Args[1];
	return ml_set_insert(Set, Key);
}

ML_METHOD("delete", MLSetT, MLAnyT) {
//<Set
//<Key
//>some|nil
// Removes :mini:`Key` from :mini:`Set` and returns the corresponding value if any, otherwise :mini:`nil`.
//$- let M := set(["A", "B", "C"])
//$= M:delete("A")
//$= M:delete("D")
//$= M
	ml_value_t *Set = (ml_value_t *)Args[0];
	ml_value_t *Key = Args[1];
	return ml_set_delete(Set, Key);
}

ML_METHOD("missing", MLSetT, MLAnyT) {
//<Set
//<Key
//>some|nil
// If :mini:`Key` is present in :mini:`Set` then returns :mini:`nil`. Otherwise inserts :mini:`Key` into :mini:`Set` with value :mini:`some` and returns :mini:`some`.
//$- let M := set(["A", "B", "C"])
//$= M:missing("A")
//$= M:missing("D")
//$= M
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_value_t *Key = Args[1];
	int Size = Set->Size;
	ml_set_node(Set, ml_typeof(Key)->hash(Key, NULL), Key);
	return Set->Size == Size ? MLNil : MLSome;
}

typedef struct {
	ml_type_t *Type;
	ml_set_node_t *Node;
} ml_set_from_t;

ML_TYPE(MLSetFromT, (MLSequenceT), "set::from");
//!internal

static void ML_TYPED_FN(ml_iterate, MLSetFromT, ml_state_t *Caller, ml_set_from_t *From) {
	ML_RETURN(From->Node);
}

ML_METHOD("from", MLSetT, MLAnyT) {
//<Set
//<Key
//>sequence|nil
// Returns the subset of :mini:`Set` after :mini:`Key` as a sequence.
//$- let M := set(["A", "B", "C", "D", "E"])
//$= set(M:from("C"))
//$= set(M:from("F"))
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_set_node_t *Node = ml_set_find_node(Set, Key);
	if (!Node) return MLNil;
	ml_set_from_t *From = new(ml_set_from_t);
	From->Type = MLSetFromT;
	From->Node = Node;
	return (ml_value_t *)From;
}

ML_METHOD("append", MLStringBufferT, MLSetT) {
//<Buffer
//<Set
// Appends a representation of :mini:`Set` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_put(Buffer, '{');
	ml_set_t *Set = (ml_set_t *)Args[1];
	ml_set_node_t *Node = Set->Head;
	if (Node) {
		ml_stringbuffer_simple_append(Buffer, Node->Key);
		while ((Node = Node->Next)) {
			ml_stringbuffer_write(Buffer, ", ", 2);
			ml_stringbuffer_simple_append(Buffer, Node->Key);
		}
	}
	ml_stringbuffer_put(Buffer, '}');
	return MLSome;
}

typedef struct ml_set_stringer_t {
	const char *Seperator;
	ml_stringbuffer_t *Buffer;
	int SeperatorLength, First;
	ml_value_t *Error;
} ml_set_stringer_t;

static int ml_set_stringer(ml_value_t *Key, ml_set_stringer_t *Stringer) {
	if (Stringer->First) {
		Stringer->First = 0;
	} else {
		ml_stringbuffer_write(Stringer->Buffer, Stringer->Seperator, Stringer->SeperatorLength);
	}
	Stringer->Error = ml_stringbuffer_simple_append(Stringer->Buffer, Key);
	if (ml_is_error(Stringer->Error)) return 1;
	return 0;
}

ML_METHOD("append", MLStringBufferT, MLSetT, MLStringT) {
//<Buffer
//<Set
//<Sep
// Appends the values of :mini:`Set` to :mini:`Buffer` with :mini:`Sep` between values.
	ml_set_stringer_t Stringer[1] = {{
		ml_string_value(Args[2]),
		(ml_stringbuffer_t *)Args[0],
		ml_string_length(Args[2]),
		1
	}};
	if (ml_set_foreach(Args[1], Stringer, (void *)ml_set_stringer)) return Stringer->Error;
	return MLSome;
}

static void ML_TYPED_FN(ml_iter_next, MLSetNodeT, ml_state_t *Caller, ml_set_node_t *Node) {
	ML_RETURN((ml_value_t *)Node->Next ?: MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLSetNodeT, ml_state_t *Caller, ml_set_node_t *Node) {
	ML_RETURN(Node->Key);
}

static void ML_TYPED_FN(ml_iter_value, MLSetNodeT, ml_state_t *Caller, ml_set_node_t *Node) {
	ML_RETURN(Node->Key);
}

static void ML_TYPED_FN(ml_iterate, MLSetT, ml_state_t *Caller, ml_set_t *Set) {
	ML_RETURN((ml_value_t *)Set->Head ?: MLNil);
}

ML_METHOD("+", MLSetT, MLSetT) {
//<Set/1
//<Set/2
//>set
// Returns a new set combining the values of :mini:`Set/1` and :mini:`Set/2`.
//$= let A := set("banana")
//$= let B := set("bread")
//$= A + B
	ml_value_t *Set = ml_set();
	ML_SET_FOREACH(Args[0], Node) ml_set_insert(Set, Node->Key);
	ML_SET_FOREACH(Args[1], Node) ml_set_insert(Set, Node->Key);
	return Set;
}

ML_METHOD("\\/", MLSetT, MLSetT) {
//<Set/1
//<Set/2
//>set
// Returns a new set combining the values of :mini:`Set/1` and :mini:`Set/2`.
//$= let A := set("banana")
//$= let B := set("bread")
//$= A \/ B
	ml_value_t *Set = ml_set();
	ML_SET_FOREACH(Args[0], Node) ml_set_insert(Set, Node->Key);
	ML_SET_FOREACH(Args[1], Node) ml_set_insert(Set, Node->Key);
	return Set;
}

ML_METHOD("*", MLSetT, MLSetT) {
//<Set/1
//<Set/2
//>set
// Returns a new set containing the values of :mini:`Set/1` which are also in :mini:`Set/2`.
//$= let A := set("banana")
//$= let B := set("bread")
//$= A * B
	ml_value_t *Set = ml_set();
	ML_SET_FOREACH(Args[1], Node) {
		if (ml_set_search0(Args[0], Node->Key)) ml_set_insert(Set, Node->Key);
	}
	return Set;
}

ML_METHOD("/\\", MLSetT, MLSetT) {
//<Set/1
//<Set/2
//>set
// Returns a new set containing the values of :mini:`Set/1` which are also in :mini:`Set/2`.
//$= let A := set("banana")
//$= let B := set("bread")
//$= A /\ B
	ml_value_t *Set = ml_set();
	ML_SET_FOREACH(Args[1], Node) {
		if (ml_set_search0(Args[0], Node->Key)) ml_set_insert(Set, Node->Key);
	}
	return Set;
}

ML_METHOD("/", MLSetT, MLSetT) {
//<Set/1
//<Set/2
//>set
// Returns a new set containing the values of :mini:`Set/1` which are not in :mini:`Set/2`.
//$= let A := set("banana")
//$= let B := set("bread")
//$= A / B
	ml_value_t *Set = ml_set();
	ML_SET_FOREACH(Args[0], Node) {
		if (!ml_set_search0(Args[1], Node->Key)) ml_set_insert(Set, Node->Key);
	}
	return Set;
}

ML_METHOD("><", MLSetT, MLSetT) {
//<Set/1
//<Set/2
//>set
// Returns a new set containing the values of :mini:`Set/1` and :mini:`Set/2` that are not in both.
//$= let A := set("banana")
//$= let B := set("bread")
//$= A >< B
	ml_value_t *Set = ml_set();
	ML_SET_FOREACH(Args[0], Node) {
		if (!ml_set_search0(Args[1], Node->Key)) ml_set_insert(Set, Node->Key);
	}
	ML_SET_FOREACH(Args[1], Node) {
		if (!ml_set_search0(Args[0], Node->Key)) ml_set_insert(Set, Node->Key);
	}
	return Set;
}

typedef struct {
	ml_state_t Base;
	ml_set_t *Set;
	ml_value_t *Compare;
	ml_value_t *Args[2];
	ml_set_node_t *Head, *Tail;
	ml_set_node_t *P, *Q;
	int Count, Size;
	int InSize, NMerges;
	int PSize, QSize;
} ml_set_sort_state_t;

static void ml_set_sort_state_run(ml_set_sort_state_t *State, ml_value_t *Result) {
	if (Result) goto resume;
	for (;;) {
		State->P = State->Head;
		State->Tail = State->Head = NULL;
		State->NMerges = 0;
		while (State->P) {
			State->NMerges++;
			State->Q = State->P;
			State->PSize = 0;
			for (int I = 0; I < State->InSize; I++) {
				State->PSize++;
				State->Q = State->Q->Next;
				if (!State->Q) break;
			}
			State->QSize = State->InSize;
			while (State->PSize > 0 || (State->QSize > 0 && State->Q)) {
				ml_set_node_t *E;
				if (State->PSize == 0) {
					E = State->Q; State->Q = State->Q->Next; State->QSize--;
				} else if (State->QSize == 0 || !State->Q) {
					E = State->P; State->P = State->P->Next; State->PSize--;
				} else {
					State->Args[0] = State->P->Key;
					State->Args[1] = State->Q->Key;
					return ml_call((ml_state_t *)State, State->Compare, State->Count, State->Args);
				resume:
					if (ml_is_error(Result)) {
						ml_set_node_t *Node = State->P, *Next;
						if (State->Tail) {
							State->Tail->Next = Node;
						} else {
							State->Head = Node;
						}
						Node->Prev = State->Tail;
						for (int Size = State->PSize; --Size > 0;) {
							Next = Node->Next; Next->Prev = Node; Node = Next;
						}
						Next = State->Q;
						Node->Next = Next;
						Next->Prev = Node;
						Node = Next;
						while (Node->Next) {
							Next = Node->Next; Next->Prev = Node; Node = Next;
						}
						Node->Next = NULL;
						State->Tail = Node;
						goto finished;
					} else if (Result == MLNil) {
						E = State->Q; State->Q = State->Q->Next; State->QSize--;
					} else {
						E = State->P; State->P = State->P->Next; State->PSize--;
					}
				}
				if (State->Tail) {
					State->Tail->Next = E;
				} else {
					State->Head = E;
				}
				E->Prev = State->Tail;
				State->Tail = E;
			}
			State->P = State->Q;
		}
		State->Tail->Next = 0;
		if (State->NMerges <= 1) {
			Result = (ml_value_t *)State->Set;
			goto finished;
		}
		State->InSize *= 2;
	}
finished:
	State->Set->Head = State->Head;
	State->Set->Tail = State->Tail;
	State->Set->Size = State->Size;
	ML_CONTINUE(State->Base.Caller, Result);
}

extern ml_value_t *LessMethod;

ML_METHODX("sort", MLSetT) {
//<Set
//>Set
// Sorts the values (changes the iteration order) of :mini:`Set` using :mini:`Value/i < Value/j` and returns :mini:`Set`.
//$= let M := set("cake")
//$= M:sort
	if (!ml_set_size(Args[0])) ML_RETURN(Args[0]);
	ml_set_sort_state_t *State = new(ml_set_sort_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_set_sort_state_run;
	ml_set_t *Set = (ml_set_t *)Args[0];
	State->Set = Set;
	State->Count = 2;
	State->Compare = LessMethod;
	State->Head = State->Set->Head;
	State->Size = Set->Size;
	State->InSize = 1;
	// TODO: Improve ml_set_sort_state_run so that List is still valid during sort
	Set->Head = Set->Tail = NULL;
	Set->Size = 0;
	return ml_set_sort_state_run(State, NULL);
}

ML_METHODX("sort", MLSetT, MLFunctionT) {
//<Set
//<Cmp
//>Set
// Sorts the values (changes the iteration order) of :mini:`Set` using :mini:`Cmp(Value/i, Value/j)` and returns :mini:`Set`
//$= let M := set("cake")
//$= M:sort(>)
	if (!ml_set_size(Args[0])) ML_RETURN(Args[0]);
	ml_set_sort_state_t *State = new(ml_set_sort_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_set_sort_state_run;
	ml_set_t *Set = (ml_set_t *)Args[0];
	State->Set = Set;
	State->Count = 2;
	State->Compare = Args[1];
	State->Head = State->Set->Head;
	State->Size = Set->Size;
	State->InSize = 1;
	// TODO: Improve ml_set_sort_state_run so that List is still valid during sort
	Set->Head = Set->Tail = NULL;
	Set->Size = 0;
	return ml_set_sort_state_run(State, NULL);
}

ML_METHOD("reverse", MLSetT) {
//<Set
//>set
// Reverses the iteration order of :mini:`Set` in-place and returns it.
//$= let M := set("cake")
//$= M:reverse
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_set_node_t *Prev = Set->Head;
	if (!Prev) return (ml_value_t *)Set;
	Set->Tail = Prev;
	ml_set_node_t *Node = Prev->Next;
	Prev->Next = NULL;
	while (Node) {
		ml_set_node_t *Next = Node->Next;
		Node->Next = Prev;
		Prev->Prev = Node;
		Prev = Node;
		Node = Next;
	}
	Prev->Prev = NULL;
	Set->Head = Prev;
	return (ml_value_t *)Set;
}

ML_METHOD("random", MLSetT) {
//<List
//>any
// Returns a random (assignable) node from :mini:`Set`.
//$= let M := set("cake")
//$= M:random
//$= M:random
	ml_set_t *Set = (ml_set_t *)Args[0];
	int Limit = Set->Size;
	if (Limit <= 0) return MLNil;
	int Divisor = RAND_MAX / Limit;
	int Random;
	do Random = random() / Divisor; while (Random >= Limit);
	ml_set_node_t *Node = Set->Head;
	while (--Random >= 0) Node = Node->Next;
	return Node->Key;
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Copy, *Dest;
	ml_set_node_t *Node;
	ml_value_t *Args[1];
} ml_set_copy_t;

static void ml_set_copy_run(ml_set_copy_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_set_insert(State->Dest, Value);
	ml_set_node_t *Node = State->Node->Next;
	if (!Node) ML_RETURN(State->Dest);
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, State->Copy, 1, State->Args);
}

ML_METHODX("copy", MLCopyT, MLSetT) {
//<Copy
//<Set
//>set
// Returns a new set contains copies of the elements of :mini:`Set` created using :mini:`Copy`.
	ml_copy_t *Copy = (ml_copy_t *)Args[0];
	ml_value_t *Dest = ml_set();
	inthash_insert(Copy->Cache, (uintptr_t)Args[1], Dest);
	ml_set_node_t *Node = ((ml_set_t *)Args[1])->Head;
	if (!Node) ML_RETURN(Dest);
	ml_set_copy_t *State = new(ml_set_copy_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_set_copy_run;
	State->Copy = (ml_value_t *)Copy;
	State->Dest = Dest;
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, (ml_value_t *)Copy, 1, State->Args);
}

#ifdef ML_CBOR

#include "ml_cbor.h"

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLSetT, ml_cbor_writer_t *Writer, ml_set_t *Set) {
	ml_cbor_write_tag(Writer, 258);
	ml_cbor_write_array(Writer, Set->Size);
	ML_SET_FOREACH(Set, Iter) ml_cbor_write(Writer, Iter->Key);
	return NULL;
}

static ml_value_t *ml_cbor_read_set(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (!ml_is(Value, MLListT)) return ml_error("TagError", "Set requires list");
	ml_value_t *Set = ml_set();
	ML_SET_FOREACH(Value, Iter) ml_set_insert(Set, Iter->Key);
	return Set;
}

#endif

void ml_set_init() {
#include "ml_set_init.c"
	stringmap_insert(MLSetT->Exports, "order", MLSetOrderT);
#ifdef ML_GENERICS
	ml_type_add_rule(MLSetT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(1), NULL);
#endif
#ifdef ML_CBOR
	ml_cbor_default_tag(258, ml_cbor_read_set);
#endif
}

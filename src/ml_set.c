#include "ml_set.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include "ml_sequence.h"
#include "ml_method.h"
#include "ml_object.h"

#undef ML_CATEGORY
#define ML_CATEGORY "set"

ML_TYPE(MLSetT, (MLSequenceT), "set"
// A set of values.
// Values can be of any type supporting hashing and comparison.
// By default, iterating over a set generates the values in the order they were inserted, however this ordering can be changed.
);

#ifdef ML_MUTABLES
ML_TYPE(MLSetMutableT, (MLSetT), "set::mutable");
#else
#define MLSetMutableT MLSetT
#endif

#ifdef ML_GENERICS

static void ml_set_update_generic(ml_set_t *Set, ml_value_t *Value) {
	if (Set->Type->Type != MLTypeGenericT) {
		Set->Type = ml_generic_type(2, (ml_type_t *[]){Set->Type, ml_typeof(Value)});
	} else {
		ml_type_t *ValueType0 = ml_typeof(Value);
		ml_type_t *BaseType = ml_generic_type_args(Set->Type)[0];
		ml_type_t *ValueType = ml_generic_type_args(Set->Type)[1];
		if (!ml_is_subtype(ValueType0, ValueType)) {
			ml_type_t *ValueType2 = ml_type_max(ValueType, ValueType0);
			if (ValueType != ValueType2) {
				Set->Type = ml_generic_type(2, (ml_type_t *[]){BaseType, ValueType2});
			}
		}
	}
}

#endif

ML_ENUM2(MLSetOrderT, "set::order",
	"Insert", SET_ORDER_INSERT, // default ordering; inserted values are put at end, no reordering on access.
	"LRU", SET_ORDER_LRU, // inserted values are put at start, accessed values are moved to start.
	"MRU", SET_ORDER_MRU, // inserted values are put at end, accessed values are moved to end.
	"Ascending", SET_ORDER_ASC, // inserted values are kept in ascending order, no reordering on access.
	"Descending", SET_ORDER_DESC // inserted values are kept in descending order, no reordering on access.
);

static void ML_TYPED_FN(ml_value_find_all, MLSetT, ml_value_t *Value, void *Data, ml_value_find_fn RefFn) {
	if (!RefFn(Data, Value, 1)) return;
	ML_SET_FOREACH(Value, Iter) ml_value_find_all(Iter->Key, Data, RefFn);
}

ML_TYPE(MLSetNodeT, (), "set::node");
//!internal

ml_value_t *ml_set() {
	ml_set_t *Set = new(ml_set_t);
	Set->Type = MLSetMutableT;
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

ML_METHOD(MLSetT, MLListT) {
//!internal
	ml_value_t *Set = ml_set();
	ML_LIST_FOREACH(Args[0], Iter) ml_set_insert(Set, Iter->Value);
	return Set;
}

ML_METHOD(MLSetT, MLSliceT) {
//!internal
	ml_value_t *Set = ml_set();
	ML_SLICE_FOREACH(Args[0], Iter) ml_set_insert(Set, Iter->Value);
	return Set;
}

ML_METHODVX("grow", MLSetMutableT, MLSequenceT) {
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

static void ml_set_node_order(ml_set_t *Set, ml_set_node_t *Parent, ml_set_node_t *Node, int Compare) {
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

static ml_set_node_t *ml_set_node_child(ml_set_t *Set, ml_set_node_t *Parent, ml_set_node_t *Node, long Hash, ml_value_t *Key) {
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
	if (!Slot[0]) {
		++Set->Size;
		if (Node) {
			Node->Next = Node->Prev = Node->Left = Node->Right = NULL;
		} else {
			Node = new(ml_set_node_t);
			Node->Key = Key;
		}
		Slot[0] = Node;
		Node->Type = MLSetNodeT;
		Node->Depth = 1;
		Node->Hash = Hash;
		ml_set_node_order(Set, Parent, Node, Compare);
		ml_set_rebalance(Slot);
		ml_set_update_depth(Slot[0]);
		return Node;
	}
	Node = ml_set_node_child(Set, Slot[0], Node, Hash, Key);
	ml_set_rebalance(Slot);
	ml_set_update_depth(Slot[0]);
	return Node;
}

static ml_set_node_t *ml_set_node(ml_set_t *Set, ml_set_node_t *Node, long Hash, ml_value_t *Key) {
	ml_set_node_t *Root = Set->Root;
	if (Root) return ml_set_node_child(Set, Root, Node, Hash, Key);
	++Set->Size;
	if (Node) {
		Node->Next = Node->Prev = Node->Left = Node->Right = NULL;
	} else {
		Node = new(ml_set_node_t);
		Node->Key = Key;
	}
	Set->Root = Node;
	Node->Type = MLSetNodeT;
	Set->Head = Set->Tail = Node;
	Node->Depth = 1;
	Node->Hash = Hash;
	return Node;
}

ml_set_node_t *ml_set_slot(ml_value_t *Set0, ml_value_t *Key) {
	ml_set_t *Set = (ml_set_t *)Set0;
	return ml_set_node(Set, NULL, ml_typeof(Key)->hash(Key, NULL), Key);
}

ml_value_t *ml_set_insert(ml_value_t *Set0, ml_value_t *Key) {
	ml_set_t *Set = (ml_set_t *)Set0;
	int Size = Set->Size;
	ml_set_node(Set, NULL, ml_typeof(Key)->hash(Key, NULL), Key);
#ifdef ML_GENERICS
	ml_set_update_generic(Set, Key);
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

ML_METHOD("precount", MLSetT) {
//<Set
//>integer
// Returns the number of values in :mini:`Set`.
//$= set(["A", "B", "C"]):count
	ml_set_t *Set = (ml_set_t *)Args[0];
	return ml_integer(Set->Size);
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

ML_METHOD("first", MLSetT) {
//<Set
// Returns the first value in :mini:`Set` or :mini:`nil` if :mini:`Set` is empty.
	ml_set_t *Set = (ml_set_t *)Args[0];
	return Set->Head ? Set->Head->Key : MLNil;
}

ML_METHOD("last", MLSetT) {
//<Set
// Returns the last value in :mini:`Set` or :mini:`nil` if :mini:`Set` is empty.
	ml_set_t *Set = (ml_set_t *)Args[0];
	return Set->Tail ? Set->Tail->Key : MLNil;
}

ML_METHOD("order", MLSetT) {
//<Set
//>set::order
// Returns the current ordering of :mini:`Set`.
	ml_set_t *Set = (ml_set_t *)Args[0];
	return ml_enum_value(MLSetOrderT, Set->Order);
}

ML_METHOD("order", MLSetMutableT, MLSetOrderT) {
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
//<Value
//>some|nil
// Returns :mini:`Value` if it is in :mini:`Set`, otherwise returns :mini:`nil`..
//$- let S := set(["A", "B", "C"])
//$= S["A"]
//$= S["D"]
//$= S
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_set_node_t *Node = ml_set_find_node(Set, Args[1]);
	if (!Node) return MLNil;
	if (Set->Order == SET_ORDER_LRU) {
		ml_set_move_node_tail(Set, Node);
	} else if (Set->Order == SET_ORDER_MRU) {
		ml_set_move_node_head(Set, Node);
	}
	return Args[1];
}

ML_METHOD("in", MLAnyT, MLSetT) {
//<Value
//<Set
//>any|nil
// Returns :mini:`Key` if it is in :mini:`Map`, otherwise return :mini:`nil`.
//$- let S := set(["A", "B", "C"])
//$= "A" in S
//$= "D" in S
	ml_set_t *Set = (ml_set_t *)Args[1];
	return ml_set_find_node(Set, Args[0]) ? Args[0] : MLNil;
}

ML_METHOD("empty", MLSetMutableT) {
//<Set
//>set
// Deletes all values from :mini:`Set` and returns it.
//$= let S := set(["A", "B", "C"])
//$= S:empty
	ml_set_t *Set = (ml_set_t *)Args[0];
	Set->Root = Set->Head = Set->Tail = NULL;
	Set->Size = 0;
#ifdef ML_GENERICS
	Set->Type = MLSetMutableT;
#endif
	return (ml_value_t *)Set;
}

ML_METHOD("pop", MLSetMutableT) {
//<Set
//>any|nil
// Deletes the first value from :mini:`Set` according to its iteration order. Returns the deleted value, or :mini:`nil` if :mini:`Set` is empty.
//$- :> Insertion order (default)
//$= let S1 := set("cake")
//$= S1:pop
//$= S1
//$-
//$- :> LRU order
//$= let S2 := set("cake"):order(set::order::LRU)
//$- S2["a"]; S2["e"]; S2["c"]; S2["k"]
//$= S2:pop
//$= S2
//$-
//$- :> MRU order
//$= let S3 := set("cake"):order(set::order::MRU)
//$- S3["a"]; S3["e"]; S3["c"]; S3["k"]
//$= S3:pop
//$= S3
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_set_node_t *Node = Set->Head;
	if (!Node) return MLNil;
	ml_set_delete(Args[0], Node->Key);
	return Node->Key;
}

ML_METHOD("pull", MLSetMutableT) {
//<Set
//>any|nil
// Deletes the last value from :mini:`Set` according to its iteration order. Returns the deleted value, or :mini:`nil` if :mini:`Set` is empty.
//$- :> Insertion order (default)
//$= let S1 := set("cake")
//$= S1:pull
//$= S1
//$-
//$- :> LRU order
//$= let S2 := set("cake"):order(set::order::LRU)
//$- S2["a"]; S2["e"]; S2["c"]; S2["k"]
//$= S2:pull
//$= S2
//$-
//$- :> MRU order
//$= let S3 := set("cake"):order(set::order::MRU)
//$- S3["a"]; S3["e"]; S3["c"]; S3["k"]
//$= S3:pull
//$= S3
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_set_node_t *Node = Set->Tail;
	if (!Node) return MLNil;
	ml_set_delete(Args[0], Node->Key);
	return Node->Key;
}

ML_METHOD("insert", MLSetMutableT, MLAnyT) {
//<Set
//<Value
//>some|nil
// Inserts :mini:`Value` into :mini:`Set`.
// Returns the previous value associated with :mini:`Key` if any, otherwise :mini:`nil`.
//$- let S := set(["A", "B", "C"])
//$= S:insert("A")
//$= S:insert("D")
//$= S
	return ml_set_insert(Args[0], Args[1]);
}

ML_METHOD("splice", MLSetMutableT, MLAnyT) {
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_set_t *Removed = (ml_set_t *)ml_set();
	ml_set_node_t *Node = ml_set_find_node(Set, Args[1]);
	if (!Node) return MLNil;
	do {
		ml_set_node_t *Next = Node->Next;
		ml_set_delete((ml_value_t *)Set, Node->Key);
		ml_set_node(Removed, Node, Node->Hash, Node->Key);
		Node = Next;
	} while (Node);
	return (ml_value_t *)Removed;
}

ML_METHOD("splice", MLSetMutableT, MLAnyT, MLIntegerT) {
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_set_t *Removed = (ml_set_t *)ml_set();
	ml_set_node_t *Node = ml_set_find_node(Set, Args[1]);
	if (!Node) return MLNil;
	int Remove = ml_integer_value(Args[2]);
	if (!Remove) return (ml_value_t *)Removed;
	ml_set_node_t *Last = Node;
	while (--Remove > 0) {
		Last = Last->Next;
		if (!Last) return MLNil;
	}
	Last = Last->Next;
	while (Node != Last) {
		ml_set_node_t *Next = Node->Next;
		ml_set_delete((ml_value_t *)Set, Node->Key);
		ml_set_node(Removed, Node, Node->Hash, Node->Key);
		Node = Next;
	}
	return (ml_value_t *)Removed;
}

ML_METHOD("splice", MLSetMutableT, MLAnyT, MLSetMutableT) {
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_set_t *Removed = (ml_set_t *)ml_set();
	ml_set_node_t *Node = ml_set_find_node(Set, Args[1]);
	if (!Node) return MLNil;
	ml_set_t *Source = (ml_set_t *)Args[2];
	ml_set_node_t *Last = Node;
	ml_set_node_t *Prev = Node->Prev, *Next = Last;
	while (Node != Last) {
		ml_set_node_t *Next = Node->Next;
		ml_set_delete((ml_value_t *)Set, Node->Key);
		ml_set_node(Removed, Node, Node->Hash, Node->Key);
		Node = Next;
	}
	ml_set_order_t Order = Set->Order;
	Set->Order = SET_ORDER_INSERT;
	for (ml_set_node_t *Node = Source->Head; Node;) {
		ml_set_node_t *Next = Node->Next;
		ml_set_node(Set, Node, ml_typeof(Node->Key)->hash(Node->Key, NULL), Node->Key);
		Node = Next;
	}
	Set->Order = Order;
	if (Next) {
		ml_set_node_t *Head = Source->Head, *Tail = Source->Tail;
		Set->Tail = Head->Prev;
		Set->Tail->Next = NULL;
		Head->Prev = Prev;
		if (Prev) {
			Prev->Next = Head;
		} else {
			Set->Head = Head;
		}
		Tail->Next = Next;
		Next->Prev = Tail;
	}
	Source->Root = Source->Head = Source->Tail = NULL;
	Source->Size = 0;
	return (ml_value_t *)Removed;
}

ML_METHOD("splice", MLSetMutableT, MLAnyT, MLIntegerT, MLSetMutableT) {
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_set_t *Removed = (ml_set_t *)ml_set();
	ml_set_node_t *Node = ml_set_find_node(Set, Args[1]);
	if (!Node) return MLNil;
	int Remove = ml_integer_value(Args[2]);
	ml_set_t *Source = (ml_set_t *)Args[3];
	ml_set_node_t *Last = Node;
	if (Remove) {
		while (--Remove > 0) {
			Last = Last->Next;
			if (!Last) return MLNil;
		}
		Last = Last->Next;
	}
	ml_set_node_t *Prev = Node->Prev, *Next = Last;
	while (Node != Last) {
		ml_set_node_t *Next = Node->Next;
		ml_set_delete((ml_value_t *)Set, Node->Key);
		ml_set_node(Removed, Node, Node->Hash, Node->Key);
		Node = Next;
	}
	ml_set_order_t Order = Set->Order;
	Set->Order = SET_ORDER_INSERT;
	for (ml_set_node_t *Node = Source->Head; Node;) {
		ml_set_node_t *Next = Node->Next;
		ml_set_node(Set, Node, ml_typeof(Node->Key)->hash(Node->Key, NULL), Node->Key);
		Node = Next;
	}
	Set->Order = Order;
	if (Next) {
		ml_set_node_t *Head = Source->Head, *Tail = Source->Tail;
		Set->Tail = Head->Prev;
		Set->Tail->Next = NULL;
		Head->Prev = Prev;
		if (Prev) {
			Prev->Next = Head;
		} else {
			Set->Head = Head;
		}
		Tail->Next = Next;
		Next->Prev = Tail;
	}
	Source->Root = Source->Head = Source->Tail = NULL;
	Source->Size = 0;
	return (ml_value_t *)Removed;
}

ML_METHODV("push", MLSetMutableT, MLAnyT) {
//<Set
//<Value
//>set
// Inserts each :mini:`Value` into :mini:`Set` at the start.
//$- let S := set(["A", "B", "C"])
//$= S:push("A")
//$= S:push("D")
//$= S:push("E", "B")
//$= S
	ml_set_t *Set = (ml_set_t *)Args[0];
	for (int I = 1; I < Count; ++I) {
		ml_value_t *Key = Args[I];
		ml_set_node_t *Node = ml_set_node(Set, NULL, ml_typeof(Key)->hash(Key, NULL), Key);
		ml_set_move_node_head(Set, Node);
#ifdef ML_GENERICS
		ml_set_update_generic(Set, Key);
#endif
	}
	return (ml_value_t *)Set;
}

ML_METHODV("put", MLSetMutableT, MLAnyT) {
//<Set
//<Value
//>set
// Inserts each :mini:`Value` into :mini:`Set` at the end.
//$- let S := set(["A", "B", "C"])
//$= S:put("A")
//$= S:put("D")
//$= S:put("E", "B")
//$= S
	ml_set_t *Set = (ml_set_t *)Args[0];
	for (int I = 1; I < Count; ++I) {
		ml_value_t *Key = Args[I];
		ml_set_node_t *Node = ml_set_node(Set, NULL, ml_typeof(Key)->hash(Key, NULL), Key);
		ml_set_move_node_tail(Set, Node);
#ifdef ML_GENERICS
		ml_set_update_generic(Set, Key);
#endif
	}
	return (ml_value_t *)Set;
}

ML_METHOD("delete", MLSetMutableT, MLAnyT) {
//<Set
//<Value
//>some|nil
// Removes :mini:`Value` from :mini:`Set` and returns it if found, otherwise :mini:`nil`.
//$- let S := set(["A", "B", "C"])
//$= S:delete("A")
//$= S:delete("D")
//$= S
	ml_value_t *Set = (ml_value_t *)Args[0];
	ml_value_t *Key = Args[1];
	return ml_set_delete(Set, Key);
}

ML_METHOD("missing", MLSetMutableT, MLAnyT) {
//<Set
//<Value
//>some|nil
// If :mini:`Value` is present in :mini:`Set` then returns :mini:`nil`. Otherwise inserts :mini:`Value` into :mini:`Set` and returns :mini:`some`.
//$- let S := set(["A", "B", "C"])
//$= S:missing("A")
//$= S:missing("D")
//$= S
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_value_t *Key = Args[1];
	int Size = Set->Size;
	ml_set_node(Set, NULL, ml_typeof(Key)->hash(Key, NULL), Key);
	return Set->Size == Size ? MLNil : MLSome;
}

ML_METHOD("take", MLSetMutableT, MLSetMutableT) {
//<Set
//<Source
//>set
// Inserts the values from :mini:`Source` into :mini:`Set`, leaving :mini:`Source` empty.
//$= let A := set("cat")
//$= let B := set("cake")
//$= A:take(B)
//$= A
//$= B
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_set_t *Source = (ml_set_t *)Args[1];
	for (ml_set_node_t *Node = Source->Head; Node;) {
		ml_set_node_t *Next = Node->Next;
		ml_set_node(Set, Node, ml_typeof(Node->Key)->hash(Node->Key, NULL), Node->Key);
		Node = Next;
	}
	Source->Root = Source->Head = Source->Tail = NULL;
	Source->Size = 0;
	return (ml_value_t *)Set;
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
//<Value
//>sequence|nil
// Returns the subset of :mini:`Set` after :mini:`Value` as a sequence.
//$- let S := set(["A", "B", "C", "D", "E"])
//$= set(S:from("C"))
//$= set(S:from("F"))
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_set_node_t *Node = ml_set_find_node(Set, Key);
	if (!Node) return MLNil;
	ml_set_from_t *From = new(ml_set_from_t);
	From->Type = MLSetFromT;
	From->Node = Node;
	return (ml_value_t *)From;
}

typedef struct {
	ml_state_t Base;
	ml_stringbuffer_t *Buffer;
	ml_set_node_t *Node;
	ml_value_t *Args[2];
	ml_hash_chain_t Chain[1];
	const char *Seperator;
	const char *Terminator;
	size_t SeperatorLength;
	size_t TerminatorLength;
} ml_set_append_state_t;

extern ml_value_t *AppendMethod;

static void ml_set_append_state_run(ml_set_append_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_set_node_t *Node = State->Node->Next;
	if (!Node) {
		ml_stringbuffer_write(State->Buffer, State->Terminator, State->TerminatorLength);
		if (State->Chain->Index) ml_stringbuffer_printf(State->Buffer, "<%d", State->Chain->Index);
		State->Buffer->Chain = State->Chain->Previous;
		ML_CONTINUE(State->Base.Caller, MLSome);
	}
	ml_stringbuffer_write(State->Buffer, State->Seperator, State->SeperatorLength);
	State->Node = Node;
	State->Args[1] = Node->Key;
	return ml_call(State, AppendMethod, 2, State->Args);
}

ML_METHODX("append", MLStringBufferT, MLSetT) {
//<Buffer
//<Set
// Appends a representation of :mini:`Set` to :mini:`Buffer` of the form :mini:`"[" + repr(V/1) + ", " + repr(V/2) + ", " + ... + repr(V/n) + "]"`, where :mini:`repr(V/i)` is a representation of the *i*-th element (using :mini:`:append`).
//$- let B := string::buffer()
//$- B:append(set(1 .. 4))
//$= B:rest
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_set_t *Set = (ml_set_t *)Args[1];
	for (ml_hash_chain_t *Link = Buffer->Chain; Link; Link = Link->Previous) {
		if (Link->Value == (ml_value_t *)Set) {
			int Index = Link->Index;
			if (!Index) Index = Link->Index = ++Buffer->Index;
			ml_stringbuffer_printf(Buffer, ">%d", Index);
			ML_RETURN(Buffer);
		}
	}
	ml_set_node_t *Node = Set->Head;
	if (!Node) {
		ml_stringbuffer_write(Buffer, "{}", 2);
		ML_RETURN(MLSome);
	}
	ml_stringbuffer_put(Buffer, '{');
	ml_set_append_state_t *State = new(ml_set_append_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_set_append_state_run;
	State->Chain->Previous = Buffer->Chain;
	State->Chain->Value = (ml_value_t *)Set;
	Buffer->Chain = State->Chain;
	State->Buffer = Buffer;
	State->Node = Node;
	State->Seperator = ", ";
	State->SeperatorLength = 2;
	State->Terminator = "}";
	State->TerminatorLength = 1;
	State->Args[0] = (ml_value_t *)Buffer;
	State->Args[1] = Node->Key;
	return ml_call(State, AppendMethod, 2, State->Args);
}

ML_METHODX("append", MLStringBufferT, MLSetT, MLStringT) {
//<Buffer
//<Set
//<Sep
// Appends a representation of :mini:`Set` to :mini:`Buffer` of the form :mini:`repr(V/1) + Sep + repr(V/2) + Sep + ... + repr(V/n)`, where :mini:`repr(V/i)` is a representation of the *i*-th element (using :mini:`:append`).
//$- let B := string::buffer()
//$- B:append(set(1 .. 4), " - ")
//$= B:rest
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_set_t *Set = (ml_set_t *)Args[1];
	for (ml_hash_chain_t *Link = Buffer->Chain; Link; Link = Link->Previous) {
		if (Link->Value == (ml_value_t *)Set) {
			int Index = Link->Index;
			if (!Index) Index = Link->Index = ++Buffer->Index;
			ml_stringbuffer_printf(Buffer, ">%d", Index);
			ML_RETURN(Buffer);
		}
	}
	ml_set_node_t *Node = Set->Head;
	if (!Node) ML_RETURN(MLNil);
	ml_set_append_state_t *State = new(ml_set_append_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_set_append_state_run;
	State->Chain->Previous = Buffer->Chain;
	State->Chain->Value = (ml_value_t *)Set;
	Buffer->Chain = State->Chain;
	State->Buffer = Buffer;
	State->Node = Node;
	State->Seperator = ml_string_value(Args[2]);
	State->SeperatorLength = ml_string_length(Args[2]);
	State->Args[0] = (ml_value_t *)Buffer;
	State->Args[1] = Node->Key;
	return ml_call(State, AppendMethod, 2, State->Args);
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

ML_METHOD("<=>", MLSetT, MLSetT) {
//<Set/1
//<Set/2
//>set
// Returns a tuple of :mini:`(Set/1 / Set/2, Set/1 * Set/2, Set/2 / Set/1)`.
//$= let A := set("banana")
//$= let B := set("bread")
//$= A <=> B
	ml_value_t *Set1 = ml_set(), *Set2 = ml_set(), *Set3 = ml_set();
	ML_SET_FOREACH(Args[0], Node) {
		if (!ml_set_search0(Args[1], Node->Key)) {
			ml_set_insert(Set1, Node->Key);
		} else {
			ml_set_insert(Set2, Node->Key);
		}
	}
	ML_SET_FOREACH(Args[1], Node) {
		if (!ml_set_search0(Args[0], Node->Key)) ml_set_insert(Set3, Node->Key);
	}
	return ml_tuplev(3, Set1, Set2, Set3);
}

ML_METHOD("<", MLSetT, MLSetT) {
//<Set/1
//<Set/2
//>set
// Returns a :mini:`Set/2` if :mini:`Set/1` is a strict subset of :mini:`Set/2`, otherwise returns :mini:`nil`.
//$= let A := set("bandana")
//$= let B := set("ban")
//$= let C := set("bread")
//$= let D := set("bandana")
//$= B < A
//$= C < A
//$= D < A
	ML_SET_FOREACH(Args[0], Node) {
		if (!ml_set_search0(Args[1], Node->Key)) return MLNil;
	}
	return ml_set_size(Args[1]) > ml_set_size(Args[0]) ? Args[1] : MLNil;
}

ML_METHOD("<=", MLSetT, MLSetT) {
//<Set/1
//<Set/2
//>set
// Returns a :mini:`Set/2` if :mini:`Set/1` is a subset of :mini:`Set/2`, otherwise returns :mini:`nil`.
//$= let A := set("bandana")
//$= let B := set("ban")
//$= let C := set("bread")
//$= let D := set("bandana")
//$= B <= A
//$= C <= A
//$= D <= A
	ML_SET_FOREACH(Args[0], Node) {
		if (!ml_set_search0(Args[1], Node->Key)) return MLNil;
	}
	return Args[1];
}

ML_METHOD(">", MLSetT, MLSetT) {
//<Set/1
//<Set/2
//>set
// Returns a :mini:`Set/2` if :mini:`Set/1` is a strict superset of :mini:`Set/2`, otherwise returns :mini:`nil`.
//$= let A := set("bandana")
//$= let B := set("ban")
//$= let C := set("bread")
//$= let D := set("bandana")
//$= A > B
//$= A > C
//$= A > D
	ML_SET_FOREACH(Args[1], Node) {
		if (!ml_set_search0(Args[0], Node->Key)) return MLNil;
	}
	return ml_set_size(Args[0]) > ml_set_size(Args[1]) ? Args[1] : MLNil;
}

ML_METHOD(">=", MLSetT, MLSetT) {
//<Set/1
//<Set/2
//>set
// Returns a :mini:`Set/2` if :mini:`Set/1` is a superset of :mini:`Set/2`, otherwise returns :mini:`nil`.
//$= let A := set("bandana")
//$= let B := set("ban")
//$= let C := set("bread")
//$= let D := set("bandana")
//$= A >= B
//$= A >= C
//$= A >= D
	ML_SET_FOREACH(Args[1], Node) {
		if (!ml_set_search0(Args[0], Node->Key)) return MLNil;
	}
	return Args[1];
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

ML_METHODX("sort", MLSetMutableT) {
//<Set
//>Set
// Sorts the values (changes the iteration order) of :mini:`Set` using :mini:`Value/i < Value/j` and returns :mini:`Set`.
//$= let S := set("cake")
//$= S:sort
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

ML_METHODX("sort", MLSetMutableT, MLFunctionT) {
//<Set
//<Cmp
//>Set
// Sorts the values (changes the iteration order) of :mini:`Set` using :mini:`Cmp(Value/i, Value/j)` and returns :mini:`Set`
//$= let S := set("cake")
//$= S:sort(>)
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

ML_METHOD("reverse", MLSetMutableT) {
//<Set
//>set
// Reverses the iteration order of :mini:`Set` in-place and returns it.
//$= let S := set("cake")
//$= S:reverse
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
//$= let S := set("cake")
//$= S:random
//$= S:random
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
	ml_type_t *Type;
	ml_value_t **Values;
	uint64_t Mask, Limit;
} ml_subset_iter_t;

ML_TYPE(MLSubsetIterT, (), "set::subset_iter");
//!internal

static void ML_TYPED_FN(ml_iter_next, MLSubsetIterT, ml_state_t *Caller, ml_subset_iter_t *Iter) {
	if (Iter->Mask == Iter->Limit) ML_RETURN(MLNil);
	++Iter->Mask;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLSubsetIterT, ml_state_t *Caller, ml_subset_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Mask + 1));
}

static void ML_TYPED_FN(ml_iter_value, MLSubsetIterT, ml_state_t *Caller, ml_subset_iter_t *Iter) {
	ml_value_t *Set = ml_set();
	uint64_t Mask = Iter->Mask;
	ml_value_t **Values = Iter->Values;
	while (Mask) {
		if (Mask & 1) ml_set_insert(Set, *Values);
		++Values;
		Mask >>= 1;
	}
	ML_RETURN(Set);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t **Values;
	int M, N, K;
	int Indices[];
} ml_subsetn_iter_t;

ML_TYPE(MLSubsetNIterT, (), "set::subset_iter");
//!internal

static void ML_TYPED_FN(ml_iter_next, MLSubsetNIterT, ml_state_t *Caller, ml_subsetn_iter_t *Iter) {
	int *Indices = Iter->Indices;
	int M = Iter->M, I = M - 1, N = Iter->N;
	while (I >= 0) {
		if (Indices[I] + (M - I) < N) {
			int J = Indices[I] + 1;
			do { Indices[I] = J; ++J; ++I; } while (I < M);
			++Iter->K;
			ML_RETURN(Iter);
		}
		--I;
	}
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLSubsetNIterT, ml_state_t *Caller, ml_subsetn_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->K));
}

static void ML_TYPED_FN(ml_iter_value, MLSubsetNIterT, ml_state_t *Caller, ml_subsetn_iter_t *Iter) {
	ml_value_t *Set = ml_set();
	for (int I = 0; I < Iter->M; ++I) {
		ml_set_insert(Set, Iter->Values[Iter->Indices[I]]);
	}
	ML_RETURN(Set);
}

typedef struct {
	ml_type_t *Type;
	ml_set_t *Set;
	int M;
} ml_subsets_t;

ML_TYPE(MLSubsetsT, (MLSequenceT), "set::subsets");
//!internal

static void ML_TYPED_FN(ml_iterate, MLSubsetsT, ml_state_t *Caller, ml_subsets_t *Subsets) {
	int M = Subsets->M;
	int N = Subsets->Set->Size;
	if (M == INT_MAX) {
		if (N > 64) ML_ERROR("RangeError", "Can only generate subsets of sets with size <= 64");
		if (N == 0) ML_RETURN(MLNil);
		ml_subset_iter_t *Iter = new(ml_subset_iter_t);
		Iter->Type = MLSubsetIterT;
		ml_value_t **Values = Iter->Values = anew(ml_value_t *, N);
		ML_SET_FOREACH(Subsets->Set, Iter) *Values++ = Iter->Key;
		Iter->Limit = N == 64 ? UINT64_MAX : (1 << N) - 1;
		ML_RETURN(Iter);
	}
	if (M > N) ML_RETURN(MLNil);
	ml_subsetn_iter_t *Iter = xnew(ml_subsetn_iter_t, M, int);
	Iter->Type = MLSubsetNIterT;
	ml_value_t **Values = Iter->Values = anew(ml_value_t *, N);
	ML_SET_FOREACH(Subsets->Set, Iter) *Values++ = Iter->Key;
	for (int I = 0; I < M; ++I) Iter->Indices[I] = I;
	Iter->M = M;
	Iter->N = N;
	Iter->K = 1;
	ML_RETURN(Iter);
}

ML_METHOD("subsets", MLSetT) {
	ml_set_t *Set = (ml_set_t *)Args[0];
	ml_subsets_t *Subsets = new(ml_subsets_t);
	Subsets->Type = MLSubsetsT;
	Subsets->Set = Set;
	Subsets->M = INT_MAX;
	return (ml_value_t *)Subsets;
}

ML_METHOD("subsets", MLSetT, MLIntegerT) {
	ml_set_t *Set = (ml_set_t *)Args[0];
	int M = ml_integer_value(Args[1]);
	if (M < 0) return ml_error("RangeError", "Subset size must be non-negative");
	ml_subsets_t *Subsets = new(ml_subsets_t);
	Subsets->Type = MLSubsetsT;
	Subsets->Set = Set;
	Subsets->M = M;
	return (ml_value_t *)Subsets;
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Visitor, *Dest;
	ml_set_node_t *Node;
	ml_value_t *Args[1];
} ml_set_visit_t;

static void ml_set_visit_run(ml_set_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_set_node_t *Node = State->Node->Next;
	if (!Node) ML_RETURN(MLNil);
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, State->Visitor, 1, State->Args);
}

ML_METHODX("visit", MLVisitorT, MLSetT) {
//<Visitor
//<Set
//>set
// Returns a new set contains copies of the elements of :mini:`Set` created using :mini:`Copy`.
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_set_node_t *Node = ((ml_set_t *)Args[1])->Head;
	if (!Node) ML_RETURN(MLNil);
	ml_set_visit_t *State = new(ml_set_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_set_visit_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

static void ml_set_copy_run(ml_set_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_set_insert(State->Dest, Value);
	ml_set_node_t *Node = State->Node->Next;
	if (!Node) ML_RETURN(State->Dest);
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, State->Visitor, 1, State->Args);
}

ML_METHODX("copy", MLVisitorT, MLSetT) {
//<Visitor
//<Set
//>set
// Returns a new set contains copies of the elements of :mini:`Set` created using :mini:`Copy`.
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_value_t *Dest = ml_set();
	inthash_insert(Visitor->Cache, (uintptr_t)Args[1], Dest);
	ml_set_node_t *Node = ((ml_set_t *)Args[1])->Head;
	if (!Node) ML_RETURN(Dest);
	ml_set_visit_t *State = new(ml_set_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_set_copy_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Dest = Dest;
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

static void ml_set_const_run(ml_set_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_set_insert(State->Dest, Value);
	ml_set_node_t *Node = State->Node->Next;
	if (!Node) {
#ifdef ML_GENERICS
		if (State->Dest->Type->Type == MLTypeGenericT) {
			ml_type_t *TArgs[2];
			ml_find_generic_parent(State->Dest->Type, MLSetMutableT, 2, TArgs);
			TArgs[0] = MLSetT;
			State->Dest->Type = ml_generic_type(2, TArgs);
		} else {
#endif
			State->Dest->Type = MLSetT;
#ifdef ML_GENERICS
		}
#endif
		ML_RETURN(State->Dest);
	}
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, State->Visitor, 1, State->Args);
}

ML_METHODX("const", MLVisitorT, MLSetT) {
//<Visitor
//<Set
//>set
// Returns a new set contains copies of the elements of :mini:`Set` created using :mini:`Visitor`.
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_value_t *Dest = ml_set();
	inthash_insert(Visitor->Cache, (uintptr_t)Args[1], Dest);
	ml_set_node_t *Node = ((ml_set_t *)Args[1])->Head;
	if (!Node) ML_RETURN(Dest);
	ml_set_visit_t *State = new(ml_set_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_set_const_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Dest = Dest;
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

#ifdef ML_CBOR

#include "ml_cbor.h"

static void ML_TYPED_FN(ml_cbor_write, MLSetT, ml_cbor_writer_t *Writer, ml_set_t *Set) {
	ml_cbor_write_tag(Writer, 258);
	ml_cbor_write_array(Writer, Set->Size);
	ML_SET_FOREACH(Set, Iter) ml_cbor_write(Writer, Iter->Key);
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
	stringmap_insert(MLSetT->Exports, "mutable", MLSetMutableT);
	MLSetMutableT->Constructor = MLSetT->Constructor;
#ifdef ML_GENERICS
	ml_type_add_rule(MLSetT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(1), NULL);
#ifdef ML_MUTABLES
	ml_type_add_rule(MLSetMutableT, MLSetT, ML_TYPE_ARG(1), NULL);
#endif
#endif
#ifdef ML_CBOR
	ml_cbor_default_tag(ML_CBOR_TAG_FINITE_SET, ml_cbor_read_set);
#endif
}

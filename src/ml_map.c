#include "ml_map.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include "ml_sequence.h"
#include "ml_method.h"
#include "ml_object.h"

#undef ML_CATEGORY
#define ML_CATEGORY "map"

ML_TYPE(MLMapT, (MLSequenceT), "map",
// A map of key-value pairs.
// Keys can be of any type supporting hashing and comparison.
// By default, iterating over a map generates the key-value pairs in the order they were inserted, however this ordering can be changed.
);

ML_ENUM2(MLMapOrderT, "map::order",
// * :mini:`map::order::Insert` |harr| default ordering; inserted pairs are put at end, no reordering on access.
// * :mini:`map::order::Ascending` |harr| inserted pairs are kept in ascending key order, no reordering on access.
// * :mini:`map::order::Ascending` |harr| inserted pairs are kept in descending key order, no reordering on access.
// * :mini:`map::order::MRU` |harr| inserted pairs are put at start, accessed pairs are moved to start.
// * :mini:`map::order::LRU` |harr| inserted pairs are put at end, accessed pairs are moved to end.
	"Insert", MAP_ORDER_INSERT,
	"LRU", MAP_ORDER_LRU,
	"MRU", MAP_ORDER_MRU,
	"Ascending", MAP_ORDER_ASC,
	"Descending", MAP_ORDER_DESC
);

static void ML_TYPED_FN(ml_value_find_refs, MLMapT, ml_value_t *Value, void *Data, ml_value_ref_fn RefFn) {
	if (!RefFn(Data, Value)) return;
	ML_MAP_FOREACH(Value, Iter) {
		ml_value_find_refs(Iter->Key, Data, RefFn);
		ml_value_find_refs(Iter->Value, Data, RefFn);
	}
}

static ml_value_t *ml_map_node_deref(ml_map_node_t *Node) {
	return Node->Value;
}

static void ml_map_node_assign(ml_state_t *Caller, ml_map_node_t *Node, ml_value_t *Value) {
	Node->Value = Value;
	ML_RETURN(Value);
}

static void ml_map_node_call(ml_state_t *Caller, ml_map_node_t *Node, int Count, ml_value_t **Args) {
	return ml_call(Caller, Node->Value, Count, Args);
}

ML_TYPE(MLMapNodeT, (), "map-node",
// A node in a :mini:`map`.
// Dereferencing a :mini:`map::node` returns the corresponding value from the :mini:`map`.
// Assigning to a :mini:`map::node` updates the corresponding value in the :mini:`map`.
	.deref = (void *)ml_map_node_deref,
	.assign = (void *)ml_map_node_assign,
	.call = (void *)ml_map_node_call
);

ml_value_t *ml_map() {
	ml_map_t *Map = new(ml_map_t);
	Map->Type = MLMapT;
	return (ml_value_t *)Map;
}

ML_METHOD(MLMapT) {
//>map
// Returns a new map.
//$= map()
	return ml_map();
}

ML_METHODV(MLMapT, MLNamesT) {
//<Key,Value
//>map
// Returns a new map with the specified keys and values.
//$= map(A is 1, B is 2, C is 3)
	ML_NAMES_CHECK_ARG_COUNT(0);
	ml_value_t *Map = ml_map();
	ml_value_t **Values = Args + 1;
	ML_NAMES_FOREACH(Args[0], Iter) ml_map_insert(Map, Iter->Value, *Values++);
	return Map;
}

static void map_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void map_iter_value(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_map_insert(State->Values[0], State->Values[1], Value);
	State->Base.run = (void *)map_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void map_iter_key(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) Value = ml_integer(ml_map_size(State->Values[0]) + 1);
	State->Values[1] = Value;
	State->Base.run = (void *)map_iter_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void map_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[0]);
	State->Base.run = (void *)map_iter_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Value);
}

ML_METHODVX(MLMapT, MLSequenceT) {
//<Sequence
//>map
// Returns a map of all the key and value pairs produced by :mini:`Sequence`.
//$= map("cake")
	ml_iter_state_t *State = xnew(ml_iter_state_t, 2, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)map_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_map();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

ML_METHODVX("grow", MLMapT, MLSequenceT) {
//<Map
//<Sequence
//>map
// Adds of all the key and value pairs produced by :mini:`Sequence` to :mini:`Map` and returns :mini:`Map`.
//$= map("cake"):grow("banana")
	ml_iter_state_t *State = xnew(ml_iter_state_t, 2, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)map_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = Args[0];
	return ml_iterate((ml_state_t *)State, ml_chained(Count - 1, Args + 1));
}

extern ml_value_t *CompareMethod;

static inline ml_value_t *ml_map_compare(ml_map_t *Map, ml_value_t **Args) {
	/*ml_method_cached_t *Cached = Map->Cached;
	if (Cached) {
		if (Cached->Types[0] != ml_typeof(Args[0])) Cached = NULL;
		if (Cached->Types[1] != ml_typeof(Args[1])) Cached = NULL;
	}
	if (!Cached || !Cached->Callback) {
		Cached = ml_method_search_cached(NULL, (ml_method_t *)CompareMethod, 2, Args);
		if (!Cached) return ml_no_method_error((ml_method_t *)CompareMethod, 2, Args);
		Map->Cached = Cached;
	}
	return ml_simple_call(Cached->Callback, 2, Args);*/
	return ml_simple_call(CompareMethod, 2, Args);
}

static ml_map_node_t *ml_map_find_node(ml_map_t *Map, ml_value_t *Key) {
	ml_map_node_t *Node = Map->Root;
	long Hash = ml_typeof(Key)->hash(Key, NULL);
	ml_method_cached_t *Cached = Map->Cached;
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
			ml_value_t *Result = ml_map_compare(Map, Args);
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

ml_value_t *ml_map_search(ml_value_t *Map0, ml_value_t *Key) {
	ml_map_node_t *Node = ml_map_find_node((ml_map_t *)Map0, Key);
	return Node ? Node->Value : MLNil;
}

ml_value_t *ml_map_search0(ml_value_t *Map0, ml_value_t *Key) {
	ml_map_node_t *Node = ml_map_find_node((ml_map_t *)Map0, Key);
	return Node ? Node->Value : NULL;
}

static int ml_map_balance(ml_map_node_t *Node) {
	int Delta = 0;
	if (Node->Left) Delta = Node->Left->Depth;
	if (Node->Right) Delta -= Node->Right->Depth;
	return Delta;
}

static void ml_map_update_depth(ml_map_node_t *Node) {
	int Depth = 0;
	if (Node->Left) Depth = Node->Left->Depth;
	if (Node->Right && Depth < Node->Right->Depth) Depth = Node->Right->Depth;
	Node->Depth = Depth + 1;
}

static void ml_map_rotate_left(ml_map_node_t **Slot) {
	ml_map_node_t *Ch = Slot[0]->Right;
	Slot[0]->Right = Slot[0]->Right->Left;
	Ch->Left = Slot[0];
	ml_map_update_depth(Slot[0]);
	Slot[0] = Ch;
	ml_map_update_depth(Slot[0]);
}

static void ml_map_rotate_right(ml_map_node_t **Slot) {
	ml_map_node_t *Ch = Slot[0]->Left;
	Slot[0]->Left = Slot[0]->Left->Right;
	Ch->Right = Slot[0];
	ml_map_update_depth(Slot[0]);
	Slot[0] = Ch;
	ml_map_update_depth(Slot[0]);
}

static void ml_map_rebalance(ml_map_node_t **Slot) {
	int Delta = ml_map_balance(Slot[0]);
	if (Delta == 2) {
		if (ml_map_balance(Slot[0]->Left) < 0) ml_map_rotate_left(&Slot[0]->Left);
		ml_map_rotate_right(Slot);
	} else if (Delta == -2) {
		if (ml_map_balance(Slot[0]->Right) > 0) ml_map_rotate_right(&Slot[0]->Right);
		ml_map_rotate_left(Slot);
	}
}

static void ml_map_insert_before(ml_map_t *Map, ml_map_node_t *Parent, ml_map_node_t *Node) {
	Node->Next = Parent;
	Node->Prev = Parent->Prev;
	if (Parent->Prev) {
		Parent->Prev->Next = Node;
	} else {
		Map->Head = Node;
	}
	Parent->Prev = Node;
}

static void ml_map_insert_after(ml_map_t *Map, ml_map_node_t *Parent, ml_map_node_t *Node) {
	Node->Prev = Parent;
	Node->Next = Parent->Next;
	if (Parent->Next) {
		Parent->Next->Prev = Node;
	} else {
		Map->Tail = Node;
	}
	Parent->Next = Node;
}

static ml_map_node_t *ml_map_node_child(ml_map_t *Map, ml_map_node_t *Parent, long Hash, ml_value_t *Key) {
	int Compare;
	if (Hash < Parent->Hash) {
		Compare = -1;
	} else if (Hash > Parent->Hash) {
		Compare = 1;
	} else {
		ml_value_t *Args[2] = {Key, Parent->Key};
		ml_value_t *Result = ml_map_compare(Map, Args);
		Compare = ml_integer_value(Result);
	}
	if (!Compare) return Parent;
	ml_map_node_t **Slot = Compare < 0 ? &Parent->Left : &Parent->Right;
	ml_map_node_t *Node;
	if (Slot[0]) {
		Node = ml_map_node_child(Map, Slot[0], Hash, Key);
	} else {
		++Map->Size;
		Node = Slot[0] = new(ml_map_node_t);
		Node->Type = MLMapNodeT;
		Node->Depth = 1;
		Node->Hash = Hash;
		Node->Key = Key;
		switch (Map->Order) {
		case MAP_ORDER_INSERT:
		case MAP_ORDER_LRU: {
			ml_map_node_t *Prev = Map->Tail;
			Prev->Next = Node;
			Node->Prev = Prev;
			Map->Tail = Node;
			break;
		}
		case MAP_ORDER_MRU: {
			ml_map_node_t *Next = Map->Head;
			Next->Prev = Node;
			Node->Next = Next;
			Map->Head = Node;
			break;
		}
		case MAP_ORDER_ASC: {
			if (Compare < 0) {
				ml_map_insert_before(Map, Parent, Node);
			} else {
				ml_map_insert_after(Map, Parent, Node);
			}
			break;
		}
		case MAP_ORDER_DESC: {
			if (Compare > 0) {
				ml_map_insert_before(Map, Parent, Node);
			} else {
				ml_map_insert_after(Map, Parent, Node);
			}
			break;
		}
		}
	}
	ml_map_rebalance(Slot);
	ml_map_update_depth(Slot[0]);
	return Node;
}

static ml_map_node_t *ml_map_node(ml_map_t *Map, long Hash, ml_value_t *Key) {
	ml_map_node_t *Root = Map->Root;
	if (Root) return ml_map_node_child(Map, Root, Hash, Key);
	++Map->Size;
	ml_map_node_t *Node = Map->Root = new(ml_map_node_t);
	Node->Type = MLMapNodeT;
	Map->Head = Map->Tail = Node;
	Node->Depth = 1;
	Node->Hash = Hash;
	Node->Key = Key;
	return Node;
}

ml_map_node_t *ml_map_slot(ml_value_t *Map0, ml_value_t *Key) {
	ml_map_t *Map = (ml_map_t *)Map0;
	return ml_map_node(Map, ml_typeof(Key)->hash(Key, NULL), Key);
}

ml_value_t *ml_map_insert(ml_value_t *Map0, ml_value_t *Key, ml_value_t *Value) {
	ml_map_t *Map = (ml_map_t *)Map0;
	ml_map_node_t *Node = ml_map_node(Map, ml_typeof(Key)->hash(Key, NULL), Key);
	ml_value_t *Old = Node->Value ?: MLNil;
	Node->Value = Value;
	ml_type_t *ValueType0 = ml_typeof(Value);
	if (ValueType0 == MLUninitializedT) {
		ml_uninitialized_use(Value, &Node->Value);
		ValueType0 = MLAnyT;
	}
#ifdef ML_GENERICS
	if (Map->Type->Type != MLTypeGenericT) {
		Map->Type = ml_generic_type(3, (ml_type_t *[]){Map->Type, ml_typeof(Key), ValueType0});
	} else {
		ml_type_t *BaseType = ml_generic_type_args(Map->Type)[0];
		ml_type_t *KeyType = ml_generic_type_args(Map->Type)[1];
		ml_type_t *ValueType = ml_generic_type_args(Map->Type)[2];
		if (KeyType != ml_typeof(Key) || ValueType != ValueType0) {
			ml_type_t *KeyType2 = ml_type_max(KeyType, ml_typeof(Key));
			ml_type_t *ValueType2 = ml_type_max(ValueType, ValueType0);
			if (KeyType != KeyType2 || ValueType != ValueType2) {
				Map->Type = ml_generic_type(3, (ml_type_t *[]){BaseType, KeyType2, ValueType2});
			}
		}
	}
#endif
	return Old;
}

static void ml_map_remove_depth_helper(ml_map_node_t *Node) {
	if (Node) {
		ml_map_remove_depth_helper(Node->Right);
		ml_map_update_depth(Node);
	}
}

static ml_value_t *ml_map_remove_internal(ml_map_t *Map, ml_map_node_t **Slot, long Hash, ml_value_t *Key) {
	if (!Slot[0]) return MLNil;
	ml_map_node_t *Node = Slot[0];
	int Compare;
	if (Hash < Node->Hash) {
		Compare = -1;
	} else if (Hash > Node->Hash) {
		Compare = 1;
	} else {
		ml_value_t *Args[2] = {Key, Node->Key};
		ml_value_t *Result = ml_map_compare(Map, Args);
		Compare = ml_integer_value(Result);
	}
	ml_value_t *Removed = MLNil;
	if (!Compare) {
		--Map->Size;
		Removed = Node->Value;
		if (Node->Prev) Node->Prev->Next = Node->Next; else Map->Head = Node->Next;
		if (Node->Next) Node->Next->Prev = Node->Prev; else Map->Tail = Node->Prev;
		if (Node->Left && Node->Right) {
			ml_map_node_t **Y = &Node->Left;
			while (Y[0]->Right) Y = &Y[0]->Right;
			ml_map_node_t *Node2 = Y[0];
			Y[0] = Node2->Left;
			Node2->Left = Node->Left;
			Node2->Right = Node->Right;
			Slot[0] = Node2;
			ml_map_remove_depth_helper(Node2->Left);
		} else if (Node->Left) {
			Slot[0] = Node->Left;
		} else if (Node->Right) {
			Slot[0] = Node->Right;
		} else {
			Slot[0] = 0;
		}
	} else {
		Removed = ml_map_remove_internal(Map, Compare < 0 ? &Node->Left : &Node->Right, Hash, Key);
	}
	if (Slot[0]) {
		ml_map_update_depth(Slot[0]);
		ml_map_rebalance(Slot);
	}
	return Removed;
}

ml_value_t *ml_map_delete(ml_value_t *Map0, ml_value_t *Key) {
	ml_map_t *Map = (ml_map_t *)Map0;
	return ml_map_remove_internal(Map, &Map->Root, ml_typeof(Key)->hash(Key, NULL), Key);
}

int ml_map_foreach(ml_value_t *Value, void *Data, int (*callback)(ml_value_t *, ml_value_t *, void *)) {
	ml_map_t *Map = (ml_map_t *)Value;
	for (ml_map_node_t *Node = Map->Head; Node; Node = Node->Next) {
		if (callback(Node->Key, Node->Value, Data)) return 1;
	}
	return 0;
}

ML_METHOD("size", MLMapT) {
//<Map
//>integer
// Returns the number of entries in :mini:`Map`.
//$= {"A" is 1, "B" is 2, "C" is 3}:size
	ml_map_t *Map = (ml_map_t *)Args[0];
	return ml_integer(Map->Size);
}

ML_METHOD("count", MLMapT) {
//<Map
//>integer
// Returns the number of entries in :mini:`Map`.
//$= {"A" is 1, "B" is 2, "C" is 3}:count
	ml_map_t *Map = (ml_map_t *)Args[0];
	return ml_integer(Map->Size);
}

static ml_value_t *ml_map_index_deref(ml_map_node_t *Index) {
	return MLNil;
}


static ml_map_node_t *ml_map_insert_node(ml_map_t *Map, ml_map_node_t **Slot, long Hash, ml_map_node_t *Index) {
	if (!Slot[0]) {
		++Map->Size;
		ml_map_node_t *Node = Slot[0] = Index;
		Node->Type = MLMapNodeT;
		ml_map_node_t *Prev = Map->Tail;
		if (Prev) {
			Prev->Next = Node;
			Node->Prev = Prev;
		} else {
			Map->Head = Node;
		}
		Map->Tail = Node;
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
		ml_value_t *Result = ml_map_compare(Map, Args);
		Compare = ml_integer_value(Result);
	}
	if (!Compare) {
		return Slot[0];
	} else {
		ml_map_node_t *Node = ml_map_insert_node(Map, Compare < 0 ? &Slot[0]->Left : &Slot[0]->Right, Hash, Index);
		ml_map_rebalance(Slot);
		ml_map_update_depth(Slot[0]);
		return Node;
	}
}

static void ml_map_index_assign(ml_state_t *Caller, ml_map_node_t *Index, ml_value_t *Value) {
	ml_map_t *Map = (ml_map_t *)Index->Value;
	ml_map_node_t *Node = ml_map_insert_node(Map, &Map->Root, ml_typeof(Index->Key)->hash(Index->Key, NULL), Index);
	Node->Value = Value;
	ML_RETURN(Value);
}

static void ml_map_index_call(ml_state_t *Caller, ml_map_node_t *Index, int Count, ml_value_t **Args) {
	return ml_call(Caller, MLNil, Count, Args);
}

ML_TYPE(MLMapIndexT, (), "map-index",
//!internal
	.deref = (void *)ml_map_index_deref,
	.assign = (void *)ml_map_index_assign,
	.call = (void *)ml_map_index_call
);

ML_METHOD("order", MLMapT) {
//<Map
//>map::order
// Returns the current ordering of :mini:`Map`.
	ml_map_t *Map = (ml_map_t *)Args[0];
	return ml_enum_value(MLMapOrderT, Map->Order);
}

ML_METHOD("order", MLMapT, MLMapOrderT) {
//<Map
//<Order
//>map
// Sets the ordering
	ml_map_t *Map = (ml_map_t *)Args[0];
	Map->Order = ml_enum_value_value(Args[1]);
	return (ml_value_t *)Map;
}

static void ml_map_move_node_head(ml_map_t *Map, ml_map_node_t *Node) {
	ml_map_node_t *Prev = Node->Prev;
	if (Prev) {
		ml_map_node_t *Next = Node->Next;
		Prev->Next = Next;
		if (Next) {
			Next->Prev = Prev;
		} else {
			Map->Tail = Prev;
		}
		Node->Next = Map->Head;
		Node->Prev = NULL;
		Map->Head->Prev = Node;
		Map->Head = Node;
	}
}

static void ml_map_move_node_tail(ml_map_t *Map, ml_map_node_t *Node) {
	ml_map_node_t *Next = Node->Next;
	if (Next) {
		ml_map_node_t *Prev = Node->Prev;
		Next->Prev = Prev;
		if (Prev) {
			Prev->Next = Next;
		} else {
			Map->Head = Next;
		}
		Node->Prev = Map->Tail;
		Node->Next = NULL;
		Map->Tail->Next = Node;
		Map->Tail = Node;
	}
}

ML_METHOD("[]", MLMapT, MLAnyT) {
//<Map
//<Key
//>mapnode
// Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then a new floating node is returned with value :mini:`nil`. This node will insert :mini:`Key` into :mini:`Map` if assigned.
//$- let M := {"A" is 1, "B" is 2, "C" is 3}
//$= M["A"]
//$= M["D"]
//$= M["A"] := 10
//$= M["D"] := 20
//$= M
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = ml_map_find_node(Map, Args[1]);
	if (!Node) {
		Node = new(ml_map_node_t);
		Node->Type = MLMapIndexT;
		Node->Value = Args[0];
		Node->Key = Args[1];
	} else if (Map->Order == MAP_ORDER_LRU) {
		ml_map_move_node_tail(Map, Node);
	} else if (Map->Order == MAP_ORDER_MRU) {
		ml_map_move_node_head(Map, Node);
	}
	return (ml_value_t *)Node;
}

ML_METHOD("in", MLAnyT, MLMapT) {
//<Key
//<Map
//>any|nil
// Returns :mini:`Key` if it is in :mini:`Map`, otherwise return :mini:`nil`.
//$- let M := {"A" is 1, "B" is 2, "C" is 3}
//$= "A" in M
//$= "D" in M
	ml_map_t *Map = (ml_map_t *)Args[1];
	return ml_map_find_node(Map, Args[0]) ? Args[0] : MLNil;
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Key;
	ml_map_node_t *Node;
} ml_ref_state_t;

static void ml_node_state_run(ml_ref_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		ML_CONTINUE(State->Base.Caller, Value);
	} else {
		State->Node->Value = Value;
		ML_CONTINUE(State->Base.Caller, State->Node);
	}
}

ML_METHODX("[]", MLMapT, MLAnyT, MLFunctionT) {
//<Map
//<Key
//<Fn
//>mapnode
// Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then :mini:`Fn(Key)` is called and the result inserted into :mini:`Map`.
//$- let M := {"A" is 1, "B" is 2, "C" is 3}
//$= M["A", fun(Key) Key:code]
//$= M["D", fun(Key) Key:code]
//$= M
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_map_node_t *Node = ml_map_node(Map, ml_typeof(Key)->hash(Key, NULL), Key);
	if (!Node->Value) {
		Node->Value = MLNil;
		ml_ref_state_t *State = new(ml_ref_state_t);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (void *)ml_node_state_run;
		State->Key = Key;
		State->Node = Node;
		ml_value_t *Function = Args[2];
		return ml_call(State, Function, 1, &State->Key);
	} else {
		ML_RETURN(Node);
	}
}

ML_METHOD("::", MLMapT, MLStringT) {
//<Map
//<Key
//>mapnode
// Same as :mini:`Map[Key]`. This method allows maps to be used as modules.
//$- let M := {"A" is 1, "B" is 2, "C" is 3}
//$= M::A
//$= M::D
//$= M::A := 10
//$= M::D := 20
//$= M
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = ml_map_find_node(Map, Args[1]);
	if (!Node) {
		Node = new(ml_map_node_t);
		Node->Type = MLMapIndexT;
		Node->Value = Args[0];
		Node->Key = Args[1];
	} else if (Map->Order == MAP_ORDER_LRU) {
		ml_map_move_node_tail(Map, Node);
	} else if (Map->Order == MAP_ORDER_MRU) {
		ml_map_move_node_head(Map, Node);
	}
	return (ml_value_t *)Node;
}

ML_METHOD("empty", MLMapT) {
//<Map
//>map
// Deletes all keys and values from :mini:`Map` and returns it.
//$= let M := {"A" is 1, "B" is 2, "C" is 3}
//$= M:empty
	ml_map_t *Map = (ml_map_t *)Args[0];
	Map->Root = Map->Head = Map->Tail = NULL;
	Map->Size = 0;
#ifdef ML_GENERICS
	Map->Type = MLMapT;
#endif
	return (ml_value_t *)Map;
}

ML_METHOD("pop", MLMapT) {
//<Map
//>any|nil
// Deletes the first key-value pair from :mini:`Map` according to its iteration order. Returns the deleted value, or :mini:`nil` if :mini:`Map` is empty.
//$- :> Insertion order (default)
//$= let M1 := map("cake")
//$= M1:pop
//$= M1
//$-
//$- :> LRU order
//$= let M2 := map("cake"):order(map::order::LRU)
//$- M2[2]; M2[4]; M2[1]; M2[3]
//$= M2:pop
//$= M2
//$-
//$- :> MRU order
//$= let M3 := map("cake"):order(map::order::MRU)
//$- M3[2]; M3[4]; M3[1]; M3[3]
//$= M3:pop
//$= M3
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = Map->Head;
	if (!Node) return MLNil;
	return ml_map_delete(Args[0], Node->Key);
}

ML_METHOD("pull", MLMapT) {
//<Map
//>any|nil
// Deletes the last key-value pair from :mini:`Map` according to its iteration order. Returns the deleted value, or :mini:`nil` if :mini:`Map` is empty.
//$- :> Insertion order (default)
//$= let M1 := map("cake")
//$= M1:pull
//$= M1
//$-
//$- :> LRU order
//$= let M2 := map("cake"):order(map::order::LRU)
//$- M2[2]; M2[4]; M2[1]; M2[3]
//$= M2:pull
//$= M2
//$-
//$- :> MRU order
//$= let M3 := map("cake"):order(map::order::MRU)
//$- M3[2]; M3[4]; M3[1]; M3[3]
//$= M3:pull
//$= M3
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = Map->Tail;
	if (!Node) return MLNil;
	return ml_map_delete(Args[0], Node->Key);
}

ML_METHOD("pop2", MLMapT) {
//<Map
//>tuple[any,any]|nil
// Deletes the first key-value pair from :mini:`Map` according to its iteration order. Returns the deleted key-value pair, or :mini:`nil` if :mini:`Map` is empty.
//$- :> Insertion order (default)
//$= let M1 := map("cake")
//$= M1:pop2
//$= M1
//$-
//$- :> LRU order
//$= let M2 := map("cake"):order(map::order::LRU)
//$- M2[2]; M2[4]; M2[1]; M2[3]
//$= M2:pop2
//$= M2
//$-
//$- :> MRU order
//$= let M3 := map("cake"):order(map::order::MRU)
//$- M3[2]; M3[4]; M3[1]; M3[3]
//$= M3:pop2
//$= M3
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = Map->Head;
	if (!Node) return MLNil;
	return ml_tuplev(2, Node->Key, ml_map_delete(Args[0], Node->Key));
}

ML_METHOD("pull2", MLMapT) {
//<Map
//>tuple[any,any]|nil
// Deletes the last key-value pair from :mini:`Map` according to its iteration order. Returns the deleted key-value pair, or :mini:`nil` if :mini:`Map` is empty.
//$- :> Insertion order (default)
//$= let M1 := map("cake")
//$= M1:pull2
//$= M1
//$-
//$- :> LRU order
//$= let M2 := map("cake"):order(map::order::LRU)
//$- M2[2]; M2[4]; M2[1]; M2[3]
//$= M2:pull2
//$= M2
//$-
//$- :> MRU order
//$= let M3 := map("cake"):order(map::order::MRU)
//$- M3[2]; M3[4]; M3[1]; M3[3]
//$= M3:pull2
//$= M3
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = Map->Tail;
	if (!Node) return MLNil;
	return ml_tuplev(2, Node->Key, ml_map_delete(Args[0], Node->Key));
}

ML_METHOD("insert", MLMapT, MLAnyT, MLAnyT) {
//<Map
//<Key
//<Value
//>any | nil
// Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.
// Returns the previous value associated with :mini:`Key` if any, otherwise :mini:`nil`.
//$- let M := {"A" is 1, "B" is 2, "C" is 3}
//$= M:insert("A", 10)
//$= M:insert("D", 20)
//$= M
	ml_value_t *Map = (ml_value_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_value_t *Value = Args[2];
	return ml_map_insert(Map, Key, Value);
}

ML_METHOD("delete", MLMapT, MLAnyT) {
//<Map
//<Key
//>any | nil
// Removes :mini:`Key` from :mini:`Map` and returns the corresponding value if any, otherwise :mini:`nil`.
//$- let M := {"A" is 1, "B" is 2, "C" is 3}
//$= M:delete("A")
//$= M:delete("D")
//$= M
	ml_value_t *Map = (ml_value_t *)Args[0];
	ml_value_t *Key = Args[1];
	return ml_map_delete(Map, Key);
}

ML_METHOD("missing", MLMapT, MLAnyT) {
//<Map
//<Key
//>some | nil
// If :mini:`Key` is present in :mini:`Map` then returns :mini:`nil`. Otherwise inserts :mini:`Key` into :mini:`Map` with value :mini:`some` and returns :mini:`some`.
//$- let M := {"A" is 1, "B" is 2, "C" is 3}
//$= M:missing("A")
//$= M:missing("D")
//$= M
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_map_node_t *Node = ml_map_node(Map, ml_typeof(Key)->hash(Key, NULL), Key);
	if (!Node->Value) return Node->Value = MLSome;
	return MLNil;
}

static void ml_missing_state_run(ml_ref_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		ML_CONTINUE(State->Base.Caller, Value);
	} else {
		State->Node->Value = Value;
		ML_CONTINUE(State->Base.Caller, MLSome);
	}
}

ML_METHODX("missing", MLMapT, MLAnyT, MLFunctionT) {
//<Map
//<Key
//<Fn
//>any | nil
// If :mini:`Key` is present in :mini:`Map` then returns :mini:`nil`. Otherwise inserts :mini:`Key` into :mini:`Map` with value :mini:`Fn(Key)` and returns :mini:`some`.
//$- let M := {"A" is 1, "B" is 2, "C" is 3}
//$= M:missing("A", fun(Key) Key:code)
//$= M:missing("D", fun(Key) Key:code)
//$= M
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_map_node_t *Node = ml_map_node(Map, ml_typeof(Key)->hash(Key, NULL), Key);
	if (!Node->Value) {
		Node->Value = MLNil;
		ml_ref_state_t *State = new(ml_ref_state_t);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (void *)ml_missing_state_run;
		State->Key = Key;
		State->Node = Node;
		ml_value_t *Function = Args[2];
		return ml_call(State, Function, 1, &State->Key);
	} else {
		ML_RETURN(MLNil);
	}
}

ML_METHOD("take", MLMapT, MLMapT) {
//<Map
//<Source
//>map
// Inserts the key-value pairs from :mini:`Source` into :mini:`Map`, leaving :mini:`Source` empty.
//$= let A := map(swap("cat"))
//$= let B := map(swap("cake"))
//$= A:take(B)
//$= A
//$= B
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_t *Source = (ml_map_t *)Args[1];
	for (ml_map_node_t *Node = Source->Head; Node;) {
		ml_map_node_t *Next = Node->Next;
		ml_value_t *Value = Node->Value;
		ml_map_node_t *New = ml_map_insert_node(Map, &Map->Root, ml_typeof(Node->Key)->hash(Node->Key, NULL), Node);
		New->Value = Value;
		Node = Next;
	}
	Source->Root = Source->Head = Source->Tail = NULL;
	Source->Size = 0;
	return (ml_value_t *)Map;
}

typedef struct {
	ml_type_t *Type;
	ml_map_node_t *Node;
} ml_map_from_t;

ML_TYPE(MLMapFromT, (MLSequenceT), "map::from");
//!internal

static void ML_TYPED_FN(ml_iterate, MLMapFromT, ml_state_t *Caller, ml_map_from_t *From) {
	ML_RETURN(From->Node);
}

ML_METHOD("from", MLMapT, MLAnyT) {
//<Map
//<Key
//>sequence|nil
// Returns the subset of :mini:`Map` after :mini:`Key` as a sequence.
//$- let M := {"A" is 1, "B" is 2, "C" is 3, "D" is 4, "E" is 5}
//$= map(M:from("C"))
//$= map(M:from("F"))
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_map_node_t *Node = ml_map_find_node(Map, Key);
	if (!Node) return MLNil;
	ml_map_from_t *From = new(ml_map_from_t);
	From->Type = MLMapFromT;
	From->Node = Node;
	return (ml_value_t *)From;
}

ML_METHOD("append", MLStringBufferT, MLMapT) {
//<Buffer
//<Map
// Appends a representation of :mini:`Map` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_put(Buffer, '{');
	ml_map_t *Map = (ml_map_t *)Args[1];
	ml_map_node_t *Node = Map->Head;
	if (Node) {
		ml_stringbuffer_simple_append(Buffer, Node->Key);
		if (Node->Value != MLSome) {
			ml_stringbuffer_write(Buffer, " is ", 4);
			ml_stringbuffer_simple_append(Buffer, Node->Value);
		}
		while ((Node = Node->Next)) {
			ml_stringbuffer_write(Buffer, ", ", 2);
			ml_stringbuffer_simple_append(Buffer, Node->Key);
			if (Node->Value != MLSome) {
				ml_stringbuffer_write(Buffer, " is ", 4);
				ml_stringbuffer_simple_append(Buffer, Node->Value);
			}
		}
	}
	ml_stringbuffer_put(Buffer, '}');
	return MLSome;
}

typedef struct ml_map_stringer_t {
	const char *Seperator, *Equals;
	ml_stringbuffer_t *Buffer;
	int SeperatorLength, EqualsLength, First;
	ml_value_t *Error;
} ml_map_stringer_t;

static int ml_map_stringer(ml_value_t *Key, ml_value_t *Value, ml_map_stringer_t *Stringer) {
	if (Stringer->First) {
		Stringer->First = 0;
	} else {
		ml_stringbuffer_write(Stringer->Buffer, Stringer->Seperator, Stringer->SeperatorLength);
	}
	Stringer->Error = ml_stringbuffer_simple_append(Stringer->Buffer, Key);
	if (ml_is_error(Stringer->Error)) return 1;
	ml_stringbuffer_write(Stringer->Buffer, Stringer->Equals, Stringer->EqualsLength);
	Stringer->Error = ml_stringbuffer_simple_append(Stringer->Buffer, Value);
	if (ml_is_error(Stringer->Error)) return 1;
	return 0;
}

ML_METHOD("append", MLStringBufferT, MLMapT, MLStringT, MLStringT) {
//<Buffer
//<Map
//<Sep
//<Conn
// Appends the entries of :mini:`Map` to :mini:`Buffer` with :mini:`Conn` between keys and values and :mini:`Sep` between entries.
	ml_map_stringer_t Stringer[1] = {{
		ml_string_value(Args[2]), ml_string_value(Args[3]),
		(ml_stringbuffer_t *)Args[0],
		ml_string_length(Args[2]), ml_string_length(Args[3]),
		1
	}};
	if (ml_map_foreach(Args[1], Stringer, (void *)ml_map_stringer)) return Stringer->Error;
	return MLSome;
}

static void ML_TYPED_FN(ml_iter_next, MLMapNodeT, ml_state_t *Caller, ml_map_node_t *Node) {
	ML_RETURN((ml_value_t *)Node->Next ?: MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLMapNodeT, ml_state_t *Caller, ml_map_node_t *Node) {
	ML_RETURN(Node->Key);
}

static void ML_TYPED_FN(ml_iter_value, MLMapNodeT, ml_state_t *Caller, ml_map_node_t *Node) {
	ML_RETURN(Node);
}

static void ML_TYPED_FN(ml_iterate, MLMapT, ml_state_t *Caller, ml_map_t *Map) {
	ML_RETURN((ml_value_t *)Map->Head ?: MLNil);
}

ML_METHOD("+", MLMapT, MLMapT) {
//<Map/1
//<Map/2
//>map
// Returns a new map combining the entries of :mini:`Map/1` and :mini:`Map/2`.
// If the same key is in both :mini:`Map/1` and :mini:`Map/2` then the corresponding value from :mini:`Map/2` is chosen.
//$= let A := map(swap("banana"))
//$= let B := map(swap("bread"))
//$= A + B
	ml_value_t *Map = ml_map();
	ML_MAP_FOREACH(Args[0], Node) ml_map_insert(Map, Node->Key, Node->Value);
	ML_MAP_FOREACH(Args[1], Node) ml_map_insert(Map, Node->Key, Node->Value);
	return Map;
}

ML_METHOD("\\/", MLMapT, MLMapT) {
//<Map/1
//<Map/2
//>map
// Returns a new map combining the entries of :mini:`Map/1` and :mini:`Map/2`.
// If the same key is in both :mini:`Map/1` and :mini:`Map/2` then the corresponding value from :mini:`Map/2` is chosen.
//$= let A := map(swap("banana"))
//$= let B := map(swap("bread"))
//$= A \/ B
	ml_value_t *Map = ml_map();
	ML_MAP_FOREACH(Args[0], Node) ml_map_insert(Map, Node->Key, Node->Value);
	ML_MAP_FOREACH(Args[1], Node) ml_map_insert(Map, Node->Key, Node->Value);
	return Map;
}

ML_METHOD("*", MLMapT, MLMapT) {
//<Map/1
//<Map/2
//>map
// Returns a new map containing the entries of :mini:`Map/1` which are also in :mini:`Map/2`. The values are chosen from :mini:`Map/2`.
//$= let A := map(swap("banana"))
//$= let B := map(swap("bread"))
//$= A * B
	ml_value_t *Map = ml_map();
	ML_MAP_FOREACH(Args[1], Node) {
		if (ml_map_search0(Args[0], Node->Key)) ml_map_insert(Map, Node->Key, Node->Value);
	}
	return Map;
}

ML_METHOD("/\\", MLMapT, MLMapT) {
//<Map/1
//<Map/2
//>map
// Returns a new map containing the entries of :mini:`Map/1` which are also in :mini:`Map/2`. The values are chosen from :mini:`Map/2`.
//$= let A := map(swap("banana"))
//$= let B := map(swap("bread"))
//$= A /\ B
	ml_value_t *Map = ml_map();
	ML_MAP_FOREACH(Args[1], Node) {
		if (ml_map_search0(Args[0], Node->Key)) ml_map_insert(Map, Node->Key, Node->Value);
	}
	return Map;
}

ML_METHOD("/", MLMapT, MLMapT) {
//<Map/1
//<Map/2
//>map
// Returns a new map containing the entries of :mini:`Map/1` which are not in :mini:`Map/2`.
//$= let A := map(swap("banana"))
//$= let B := map(swap("bread"))
//$= A / B
	ml_value_t *Map = ml_map();
	ML_MAP_FOREACH(Args[0], Node) {
		if (!ml_map_search0(Args[1], Node->Key)) ml_map_insert(Map, Node->Key, Node->Value);
	}
	return Map;
}

ML_METHOD("><", MLMapT, MLMapT) {
//<Map/1
//<Map/2
//>map
// Returns a new map containing the entries of :mini:`Map/1` and :mini:`Map/2` that are not in both.
//$= let A := map(swap("banana"))
//$= let B := map(swap("bread"))
//$= A >< B
	ml_value_t *Map = ml_map();
	ML_MAP_FOREACH(Args[0], Node) {
		if (!ml_map_search0(Args[1], Node->Key)) ml_map_insert(Map, Node->Key, Node->Value);
	}
	ML_MAP_FOREACH(Args[1], Node) {
		if (!ml_map_search0(Args[0], Node->Key)) ml_map_insert(Map, Node->Key, Node->Value);
	}
	return Map;
}

typedef struct {
	ml_state_t Base;
	ml_map_t *Map;
	ml_value_t *Compare;
	ml_value_t *Args[4];
	ml_map_node_t *Head, *Tail;
	ml_map_node_t *P, *Q;
	int Count, Size;
	int InSize, NMerges;
	int PSize, QSize;
} ml_map_sort_state_t;

static void ml_map_sort_state_run(ml_map_sort_state_t *State, ml_value_t *Result) {
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
				ml_map_node_t *E;
				if (State->PSize == 0) {
					E = State->Q; State->Q = State->Q->Next; State->QSize--;
				} else if (State->QSize == 0 || !State->Q) {
					E = State->P; State->P = State->P->Next; State->PSize--;
				} else {
					State->Args[0] = State->P->Key;
					State->Args[1] = State->Q->Key;
					State->Args[2] = State->P->Value;
					State->Args[3] = State->Q->Value;
					return ml_call((ml_state_t *)State, State->Compare, State->Count, State->Args);
				resume:
					if (ml_is_error(Result)) {
						ml_map_node_t *Node = State->P, *Next;
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
			Result = (ml_value_t *)State->Map;
			goto finished;
		}
		State->InSize *= 2;
	}
finished:
	State->Map->Head = State->Head;
	State->Map->Tail = State->Tail;
	State->Map->Size = State->Size;
	ML_CONTINUE(State->Base.Caller, Result);
}

extern ml_value_t *LessMethod;

ML_METHODX("sort", MLMapT) {
//<Map
//>Map
// Sorts the entries (changes the iteration order) of :mini:`Map` using :mini:`Key/i < Key/j` and returns :mini:`Map`.
//$= let M := map(swap("cake"))
//$= M:sort
	if (!ml_map_size(Args[0])) ML_RETURN(Args[0]);
	ml_map_sort_state_t *State = new(ml_map_sort_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_sort_state_run;
	ml_map_t *Map = (ml_map_t *)Args[0];
	State->Map = Map;
	State->Count = 2;
	State->Compare = LessMethod;
	State->Head = State->Map->Head;
	State->Size = Map->Size;
	State->InSize = 1;
	// TODO: Improve ml_map_sort_state_run so that List is still valid during sort
	Map->Head = Map->Tail = NULL;
	Map->Size = 0;
	return ml_map_sort_state_run(State, NULL);
}

ML_METHODX("sort", MLMapT, MLFunctionT) {
//<Map
//<Cmp
//>Map
// Sorts the entries (changes the iteration order) of :mini:`Map` using :mini:`Cmp(Key/i, Key/j)` and returns :mini:`Map`
//$= let M := map(swap("cake"))
//$= M:sort(>)
	if (!ml_map_size(Args[0])) ML_RETURN(Args[0]);
	ml_map_sort_state_t *State = new(ml_map_sort_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_sort_state_run;
	ml_map_t *Map = (ml_map_t *)Args[0];
	State->Map = Map;
	State->Count = 2;
	State->Compare = Args[1];
	State->Head = State->Map->Head;
	State->Size = Map->Size;
	State->InSize = 1;
	// TODO: Improve ml_map_sort_state_run so that List is still valid during sort
	Map->Head = Map->Tail = NULL;
	Map->Size = 0;
	return ml_map_sort_state_run(State, NULL);
}

ML_METHODX("sort2", MLMapT, MLFunctionT) {
//<Map
//<Cmp
//>Map
// Sorts the entries (changes the iteration order) of :mini:`Map` using :mini:`Cmp(Key/i, Key/j, Value/i, Value/j)` and returns :mini:`Map`
//$= let M := map(swap("cake"))
//$= M:sort(fun(K1, K2, V1, V2) V1 < V2)
	if (!ml_map_size(Args[0])) ML_RETURN(Args[0]);
	ml_map_sort_state_t *State = new(ml_map_sort_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_sort_state_run;
	ml_map_t *Map = (ml_map_t *)Args[0];
	State->Map = Map;
	State->Count = 4;
	State->Compare = Args[1];
	State->Head = State->Map->Head;
	State->Size = Map->Size;
	State->InSize = 1;
	// TODO: Improve ml_map_sort_state_run so that List is still valid during sort
	Map->Head = Map->Tail = NULL;
	Map->Size = 0;
	return ml_map_sort_state_run(State, NULL);
}

ML_METHOD("reverse", MLMapT) {
//<Map
//>map
// Reverses the iteration order of :mini:`Map` in-place and returns it.
//$= let M := map("cake")
//$= M:reverse
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Prev = Map->Head;
	if (!Prev) return (ml_value_t *)Map;
	Map->Tail = Prev;
	ml_map_node_t *Node = Prev->Next;
	Prev->Next = NULL;
	while (Node) {
		ml_map_node_t *Next = Node->Next;
		Node->Next = Prev;
		Prev->Prev = Node;
		Prev = Node;
		Node = Next;
	}
	Prev->Prev = NULL;
	Map->Head = Prev;
	return (ml_value_t *)Map;
}

ML_METHOD("random", MLMapT) {
//<List
//>any
// Returns a random (assignable) node from :mini:`Map`.
//$= let M := map("cake")
//$= M:random
//$= M:random
	ml_map_t *Map = (ml_map_t *)Args[0];
	int Limit = Map->Size;
	if (Limit <= 0) return MLNil;
	int Divisor = RAND_MAX / Limit;
	int Random;
	do Random = random() / Divisor; while (Random >= Limit);
	ml_map_node_t *Node = Map->Head;
	while (--Random >= 0) Node = Node->Next;
	return (ml_value_t *)Node;
}

void ml_map_init() {
#include "ml_map_init.c"
	stringmap_insert(MLMapT->Exports, "order", MLMapOrderT);
#ifdef ML_GENERICS
	ml_type_add_rule(MLMapT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
#endif
}

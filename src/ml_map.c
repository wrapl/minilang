#include "ml_map.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include "ml_sequence.h"
#include "ml_method.h"
#include "ml_object.h"

#undef ML_CATEGORY
#define ML_CATEGORY "map"

ML_TYPE(MLMapT, (MLSequenceT), "map"
// A map of key-value pairs.
// Keys can be of any type supporting hashing and comparison.
// By default, iterating over a map generates the key-value pairs in the order they were inserted, however this ordering can be changed.
);

#ifdef ML_MUTABLES
ML_TYPE(MLMapMutableT, (MLMapT), "map::mutable");
#else
#define MLMapMutableT MLMapT
#endif

ML_VALUE(MLAny, MLAnyT);

#ifdef ML_GENERICS

static void ml_map_update_generic(ml_map_t *Map, ml_value_t *Key, ml_value_t *Value) {
	if (Map->Type->Type != MLTypeGenericT) {
		Map->Type = ml_generic_type(3, (ml_type_t *[]){Map->Type, ml_typeof(Key), ml_typeof(Value)});
	} else {
		ml_type_t *KeyType0 = ml_typeof(Key);
		ml_type_t *ValueType0 = ml_typeof(Value);
		ml_type_t *BaseType = ml_generic_type_args(Map->Type)[0];
		ml_type_t *KeyType = ml_generic_type_args(Map->Type)[1];
		ml_type_t *ValueType = ml_generic_type_args(Map->Type)[2];
		if (!ml_is_subtype(KeyType0, KeyType) || !ml_is_subtype(ValueType0, ValueType)) {
			ml_type_t *KeyType2 = ml_type_max(KeyType, KeyType0);
			ml_type_t *ValueType2 = ml_type_max(ValueType, ValueType0);
			if (KeyType != KeyType2 || ValueType != ValueType2) {
				Map->Type = ml_generic_type(3, (ml_type_t *[]){BaseType, KeyType2, ValueType2});
			}
		}
	}
}

#endif

ML_ENUM2(MLMapOrderT, "map::order",
	"Insert", MAP_ORDER_INSERT, // default ordering; inserted pairs are put at end, no reordering on access.
	"LRU", MAP_ORDER_LRU, // inserted pairs are put at start, accessed pairs are moved to start.
	"MRU", MAP_ORDER_MRU, // inserted pairs are put at end, accessed pairs are moved to end.
	"Ascending", MAP_ORDER_ASC, // inserted pairs are kept in ascending key order, no reordering on access.
	"Descending", MAP_ORDER_DESC // inserted pairs are kept in descending key order, no reordering on access.
);

static void ML_TYPED_FN(ml_value_find_all, MLMapT, ml_value_t *Value, void *Data, ml_value_find_fn RefFn) {
	if (!RefFn(Data, Value, 1)) return;
	ML_MAP_FOREACH(Value, Iter) {
		ml_value_find_all(Iter->Key, Data, RefFn);
		ml_value_find_all(Iter->Value, Data, RefFn);
	}
}

static ml_value_t *ml_map_node_deref(ml_map_node_t *Node) {
	return Node->Value;
}

static void ml_map_node_assign(ml_state_t *Caller, ml_map_node_t *Node, ml_value_t *Value) {
	Node->Value = Value;
#ifdef ML_GENERICS
	ml_map_update_generic(Node->Map, Node->Key, Value);
#endif
	ML_RETURN(Value);
}

#ifdef ML_MUTABLES

ML_TYPE(MLMapNodeT, (), "map::node",
// A node in a :mini:`map`.
// Dereferencing a :mini:`map::node::const` returns the corresponding value from the :mini:`map`.
	.deref = (void *)ml_map_node_deref
);

ML_TYPE(MLMapNodeMutableT, (MLMapNodeT), "map::node::mutable",
// A node in a :mini:`map`.
// Dereferencing a :mini:`map::node` returns the corresponding value from the :mini:`map`.
// Assigning to a :mini:`map::node` updates the corresponding value in the :mini:`map`.
	.deref = (void *)ml_map_node_deref,
	.assign = (void *)ml_map_node_assign
);

#else

#define MLMapNodeMutableT MLMapNodeT

ML_TYPE(MLMapNodeMutableT, (), "map::node",
// A node in a :mini:`map`.
// Dereferencing a :mini:`map::node` returns the corresponding value from the :mini:`map`.
// Assigning to a :mini:`map::node` updates the corresponding value in the :mini:`map`.
	.deref = (void *)ml_map_node_deref,
	.assign = (void *)ml_map_node_assign
);

#endif

ml_value_t *ml_map() {
	ml_map_t *Map = new(ml_map_t);
	Map->Type = MLMapMutableT;
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

static ml_map_node_t *ml_map_template_node(ml_map_node_t *Template, ml_value_t **Args, ml_map_node_t **Nodes) {
	ml_map_node_t *Node = new(ml_map_node_t);
	Node->Type = MLMapNodeMutableT;
	Node->Key = Template->Key;
	int Index = ml_integer_value_fast(Template->Value);
	Node->Value = ml_deref(Args[Index]);
	Nodes[Index] = Node;
	Node->Hash = Template->Hash;
	Node->Depth = Template->Depth;
	if (Template->Left) Node->Left = ml_map_template_node(Template->Left, Args, Nodes);
	if (Template->Right) Node->Right = ml_map_template_node(Template->Right, Args, Nodes);
	return Node;

}

static void ml_map_template_call(ml_state_t *Caller, ml_map_t *Template, int Count, ml_value_t **Args) {
	if (Template->Size > Count) ML_ERROR("CallError", "Mismatched call to map template");
	ml_map_t *Map = (ml_map_t *)ml_map();
	Map->Cached = Template->Cached;
	Map->Size = Template->Size;
	Map->Order = Template->Order;
	if (Template->Root) {
		ml_map_node_t *Nodes[Count];
		memset(Nodes, 0, Map->Size * sizeof(ml_map_node_t *));
		Map->Root = ml_map_template_node(Template->Root, Args, Nodes);
		ml_map_node_t **Slot = &Map->Head, *Prev = NULL;
		for (int I = 0; I < Count; ++I) {
			ml_map_node_t *Node = Nodes[I];
			if (Node) {
				ml_value_t *Value = Node->Value;
				if (ml_typeof(Value) == MLUninitializedT) {
					ml_uninitialized_use(Value, &Node->Value);
					Value = MLAny;
				}
#ifdef ML_GENERICS
				ml_map_update_generic(Map, Node->Key, Value);
#endif
				Node->Map = Map;
				Node->Prev = Prev;
				*Slot = Node;
				Slot = &Node->Next;
				Prev = Node;
			}
		}
		Map->Tail = Prev;
	}
	ML_RETURN(Map);
}

ML_TYPE(MLMapTemplateT, (MLFunctionT), "map::template",
	.call = (void *)ml_map_template_call
);

ML_FUNCTION(MLMapTemplate) {
	ml_value_t *Template = ml_map();
	for (int I = 0; I < Count; ++I) ml_map_insert(Template, Args[I], ml_integer(I));
	Template->Type = MLMapTemplateT;
	return Template;
}

static ml_value_t *ML_TYPED_FN(ml_serialize, MLMapTemplateT, ml_value_t *Template) {
	ml_value_t *Result = ml_list();
	ml_list_put(Result, ml_cstring("map::template"));
	ML_MAP_FOREACH(Template, Node) ml_list_put(Result, Node->Key);
	return Result;
}

ML_DESERIALIZER("map::template") {
	ml_value_t *Template = ml_map();
	for (int I = 0; I < Count; ++I) ml_map_insert(Template, Args[I], ml_integer(I));
	Template->Type = MLMapTemplateT;
	return Template;
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter, *Map, *Reduce;
	ml_map_node_t *Slot;
	ml_value_t *Args[2];
} ml_map_reduce_t;

static void map_reduce_iterate(ml_map_reduce_t *State, ml_value_t *Value);

static void map_reduce_call(ml_map_reduce_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) {
		ml_map_delete(State->Map, State->Slot->Key);
	} else {
		State->Slot->Value = Value;
	}
	State->Base.run = (void *)map_reduce_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void map_reduce_value(ml_map_reduce_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Args[1] = Value;
	State->Base.run = (void *)map_reduce_call;
	return ml_call(State, State->Reduce, 2, State->Args);
}

static void map_reduce_key(ml_map_reduce_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Slot = ml_map_slot(State->Map, ml_deref(Value));
	State->Args[0] = State->Slot->Value ?: MLNil;
	State->Base.run = (void *)map_reduce_value;
	return ml_iter_value((ml_state_t *)State, State->Iter);
}

static void map_reduce_iterate(ml_map_reduce_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Map);
	State->Base.run = (void *)map_reduce_key;
	return ml_iter_key((ml_state_t *)State, State->Iter = Value);
}

ML_FUNCTIONX(MLMapReduce) {
//@map::reduce
//<Sequence:sequence
//<Reduce:function
//$= map::reduce(swap("banana"); L := [], I) L:put(I)
// Creates a new map, :mini:`Map`, then applies :mini:`Map[Key] := Reduce(old, Value)` for each :mini:`Key`, :mini:`Value` pair generated by :mini:`Sequence`, finally returning :mini:`Map`.
	ml_map_reduce_t *State = new(ml_map_reduce_t);
	State->Base.Caller = Caller;
	State->Base.run = (void *)map_reduce_iterate;
	State->Base.Context = Caller->Context;
	State->Map = ml_map();
	State->Reduce = Args[1];
	return ml_iterate((ml_state_t *)State, Args[0]);
}

ML_METHODVX("grow", MLMapMutableT, MLSequenceT) {
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

ML_METHODV("grow", MLMapMutableT, MLNamesT) {
	ML_NAMES_CHECK_ARG_COUNT(1);
	ml_value_t *Map = Args[0];
	int I = 1;
	ML_NAMES_FOREACH(Args[1], Iter) ml_map_insert(Map, Iter->Value, Args[++I]);
	return Map;
}

extern ml_value_t *CompareMethod;

static inline ml_value_t *ml_map_compare(ml_map_t *Map, ml_value_t **Args) {
	ml_method_cached_t *Cached = Map->Cached;
	if (Cached) {
		if (Cached->Types[0] != ml_typeof(Args[0])) {
			Cached = NULL;
		} else if (Cached->Types[1] != ml_typeof(Args[1])) {
			Cached = NULL;
		}
	}
	if (!Cached || !Cached->Callback) {
		Cached = ml_method_search_cached(NULL, (ml_method_t *)CompareMethod, 2, Args);
		if (!Cached) return ml_no_method_error((ml_method_t *)CompareMethod, 2, Args);
		Map->Cached = Cached;
	}
	return ml_simple_call(Cached->Callback, 2, Args);
	//return ml_simple_call(CompareMethod, 2, Args);
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

static void ml_map_node_order(ml_map_t *Map, ml_map_node_t *Parent, ml_map_node_t *Node, int Compare) {
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

static ml_map_node_t *ml_map_node_child(ml_map_t *Map, ml_map_node_t *Parent, ml_map_node_t *Node, long Hash, ml_value_t *Key) {
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
	if (!Compare) {
		if (Node) Parent->Value = Node->Value;
		return Parent;
	}
	ml_map_node_t **Slot = Compare < 0 ? &Parent->Left : &Parent->Right;
	if (!Slot[0]) {
		++Map->Size;
		if (Node) {
			Node->Next = Node->Prev = Node->Left = Node->Right = NULL;
		} else {
			Node = new(ml_map_node_t);
			Node->Key = Key;
		}
		Slot[0] = Node;
		Node->Type = MLMapNodeMutableT;
		Node->Map = Map;
		Node->Depth = 1;
		Node->Hash = Hash;
		ml_map_node_order(Map, Parent, Node, Compare);
		ml_map_rebalance(Slot);
		ml_map_update_depth(Slot[0]);
		return Node;
	}
	Node = ml_map_node_child(Map, Slot[0], Node, Hash, Key);
	ml_map_rebalance(Slot);
	ml_map_update_depth(Slot[0]);
	return Node;
}

static ml_map_node_t *ml_map_node(ml_map_t *Map, ml_map_node_t *Node, long Hash, ml_value_t *Key) {
	ml_map_node_t *Root = Map->Root;
	if (Root) return ml_map_node_child(Map, Root, Node, Hash, Key);
	++Map->Size;
	if (Node) {
		Node->Next = Node->Prev = Node->Left = Node->Right = NULL;
	} else {
		Node = new(ml_map_node_t);
		Node->Key = Key;
	}
	Map->Root = Node;
	Node->Type = MLMapNodeMutableT;
	Node->Map = Map;
	Map->Head = Map->Tail = Node;
	Node->Depth = 1;
	Node->Hash = Hash;
	return Node;
}

ml_map_node_t *ml_map_slot(ml_value_t *Map0, ml_value_t *Key) {
	ml_map_t *Map = (ml_map_t *)Map0;
	return ml_map_node(Map, NULL, ml_typeof(Key)->hash(Key, NULL), Key);
}

ml_value_t *ml_map_insert(ml_value_t *Map0, ml_value_t *Key, ml_value_t *Value) {
	ml_map_t *Map = (ml_map_t *)Map0;
	ml_map_node_t *Node = ml_map_node(Map, NULL, ml_typeof(Key)->hash(Key, NULL), Key);
	ml_value_t *Old = Node->Value ?: MLNil;
	Node->Value = Value;
	if (ml_typeof(Value) == MLUninitializedT) {
		ml_uninitialized_use(Value, &Node->Value);
		Value = MLAny;
	}
#ifdef ML_GENERICS
	ml_map_update_generic(Map, Key, Value);
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

ML_METHOD("precount", MLMapT) {
//<Map
//>integer
// Returns the number of entries in :mini:`Map`.
//$= {"A" is 1, "B" is 2, "C" is 3}:count
	ml_map_t *Map = (ml_map_t *)Args[0];
	return ml_integer(Map->Size);
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

ML_METHOD("first", MLMapT) {
//<Map
// Returns the first value in :mini:`Map` or :mini:`nil` if :mini:`Map` is empty.
	ml_map_t *Map = (ml_map_t *)Args[0];
	return (ml_value_t *)Map->Head ?: MLNil;
}

ML_METHOD("first2", MLMapT) {
//<Map
// Returns the first key and value in :mini:`Map` or :mini:`nil` if :mini:`Map` is empty.
	ml_map_t *Map = (ml_map_t *)Args[0];
	return Map->Head ? ml_tuplev(2, Map->Head->Key, Map->Head) : MLNil;
}

ML_METHOD("last", MLMapT) {
//<Map
// Returns the last value in :mini:`Map` or :mini:`nil` if :mini:`Map` is empty.
	ml_map_t *Map = (ml_map_t *)Args[0];
	return (ml_value_t *)Map->Tail ?: MLNil;
}

ML_METHOD("last2", MLMapT) {
//<Map
// Returns the last key and value in :mini:`Map` or :mini:`nil` if :mini:`Map` is empty.
	ml_map_t *Map = (ml_map_t *)Args[0];
	return Map->Tail ? ml_tuplev(2, Map->Tail->Key, Map->Tail) : MLNil;
}

static ml_value_t *ml_map_index_deref(ml_map_node_t *Index) {
	return MLNil;
}

static void ml_map_index_assign(ml_state_t *Caller, ml_map_node_t *Index, ml_value_t *Value) {
	ml_map_t *Map = (ml_map_t *)Index->Value;
	ml_map_node_t *Node = ml_map_node(Map, Index, ml_typeof(Index->Key)->hash(Index->Key, NULL), Index->Key);
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

ML_METHOD("order", MLMapMutableT, MLMapOrderT) {
//<Map
//<Order
//>map
// Sets the ordering
	ml_map_t *Map = (ml_map_t *)Args[0];
	Map->Order = ml_enum_value_value(Args[1]);
	return (ml_value_t *)Map;
}

void ml_map_move_node_head(ml_map_t *Map, ml_map_node_t *Node) {
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

void ml_map_move_node_tail(ml_map_t *Map, ml_map_node_t *Node) {
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

ML_METHOD("[]", MLMapMutableT, MLAnyT) {
//<Map
//<Key
//>map::node
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
		Node->Map = Map;
		Node->Value = Args[0];
		Node->Key = Args[1];
	} else if (Map->Order == MAP_ORDER_LRU) {
		ml_map_move_node_tail(Map, Node);
	} else if (Map->Order == MAP_ORDER_MRU) {
		ml_map_move_node_head(Map, Node);
	}
	return (ml_value_t *)Node;
}

ML_METHOD("::", MLMapMutableT, MLStringT) {
//<Map
//<Key
//>map::node
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
		Node->Map = Map;
		Node->Value = Args[0];
		Node->Key = Args[1];
	} else if (Map->Order == MAP_ORDER_LRU) {
		ml_map_move_node_tail(Map, Node);
	} else if (Map->Order == MAP_ORDER_MRU) {
		ml_map_move_node_head(Map, Node);
	}
	return (ml_value_t *)Node;
}

#ifdef ML_MUTABLES

ML_METHOD("[]", MLMapT, MLAnyT) {
//<Map
//<Key
//>any|nil
// Returns the value corresponding to :mini:`Key` in :mini:`Map`, or :mini:`nil` if :mini:`Key` is not in :mini:`Map`.
//$- let M := copy({"A" is 1, "B" is 2, "C" is 3}, :const)
//$= M["A"]
//$= M["D"]
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = ml_map_find_node(Map, Args[1]);
	return Node ? Node->Value : MLNil;
}

ML_METHOD("::", MLMapT, MLStringT) {
//<Map
//<Key
//>map::node
// Same as :mini:`Map[Key]`. This method allows maps to be used as modules.
//$- let M := copy({"A" is 1, "B" is 2, "C" is 3}, :const)
//$= M::A
//$= M::D
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = ml_map_find_node(Map, Args[1]);
	return Node ? Node->Value : MLNil;
}

#endif

#ifdef ML_GENERICS

static ml_map_node_t *ml_map_find_node_string(ml_map_t *Map, ml_value_t *Key) {
	long Hash = ml_hash(Key);
	int LengthA = ml_string_length(Key);
	const char *StringA = ml_string_value(Key);
	ml_map_node_t *Node = Map->Root;
	while (Node) {
		int Compare;
		if (Hash < Node->Hash) {
			Compare = -1;
		} else if (Hash > Node->Hash) {
			Compare = 1;
		} else {
			int LengthB = ml_string_length(Node->Key);
			const char *StringB = ml_string_value(Node->Key);
			if (LengthA < LengthB) {
				Compare = memcmp(StringA, StringB, LengthA) ?: -1;
			} else if (LengthA > LengthB) {
				Compare = memcmp(StringA, StringB, LengthB) ?: 1;
			} else {
				Compare = memcmp(StringA, StringB, LengthA);
			}
		}
		if (!Compare) {
			return Node;
		} else {
			Node = Compare < 0 ? Node->Left : Node->Right;
		}
	}
	return NULL;
}

ML_GENERIC_TYPE(MLMapMutableStringAnyT, MLMapMutableT, MLStringT, MLAnyT);

ML_METHOD("[]", MLMapMutableStringAnyT, MLStringT) {
//!internal
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = ml_map_find_node_string(Map, Args[1]);
	if (!Node) {
		Node = new(ml_map_node_t);
		Node->Type = MLMapIndexT;
		Node->Map = Map;
		Node->Value = Args[0];
		Node->Key = Args[1];
	} else if (Map->Order == MAP_ORDER_LRU) {
		ml_map_move_node_tail(Map, Node);
	} else if (Map->Order == MAP_ORDER_MRU) {
		ml_map_move_node_head(Map, Node);
	}
	return (ml_value_t *)Node;
}

ML_METHOD("::", MLMapMutableStringAnyT, MLStringT) {
//!internal
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = ml_map_find_node_string(Map, Args[1]);
	if (!Node) {
		Node = new(ml_map_node_t);
		Node->Type = MLMapIndexT;
		Node->Map = Map;
		Node->Value = Args[0];
		Node->Key = Args[1];
	} else if (Map->Order == MAP_ORDER_LRU) {
		ml_map_move_node_tail(Map, Node);
	} else if (Map->Order == MAP_ORDER_MRU) {
		ml_map_move_node_head(Map, Node);
	}
	return (ml_value_t *)Node;
}

#ifdef ML_MUTABLES

ML_GENERIC_TYPE(MLMapStringAnyT, MLMapT, MLStringT, MLAnyT);

ML_METHOD("[]", MLMapStringAnyT, MLStringT) {
//!internal
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = ml_map_find_node_string(Map, Args[1]);
	return Node ? Node->Value : MLNil;
}

ML_METHOD("::", MLMapStringAnyT, MLStringT) {
//!internal
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = ml_map_find_node_string(Map, Args[1]);
	return Node ? Node->Value : MLNil;
}

#endif

static ml_map_node_t *ml_map_find_node_integer(ml_map_t *Map, ml_value_t *Key) {
	long Hash = ml_hash(Key);
	int64_t ValueA = ml_integer_value(Key);
	ml_map_node_t *Node = Map->Root;
	while (Node) {
		int Compare;
		if (Hash < Node->Hash) {
			Compare = -1;
		} else if (Hash > Node->Hash) {
			Compare = 1;
		} else {
			int64_t ValueB = ml_integer_value(Node->Key);
			if (ValueA < ValueB) {
				Compare = -1;
			} else if (ValueA > ValueB) {
				Compare = 1;
			} else {
				Compare = 0;
			}
		}
		if (!Compare) {
			return Node;
		} else {
			Node = Compare < 0 ? Node->Left : Node->Right;
		}
	}
	return NULL;
}

ML_GENERIC_TYPE(MLMapMutableIntegerAnyT, MLMapMutableT, MLIntegerT, MLAnyT);

ML_METHOD("[]", MLMapMutableIntegerAnyT, MLIntegerT) {
//!internal
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = ml_map_find_node_integer(Map, Args[1]);
	if (!Node) {
		Node = new(ml_map_node_t);
		Node->Type = MLMapIndexT;
		Node->Map = Map;
		Node->Value = Args[0];
		Node->Key = Args[1];
	} else if (Map->Order == MAP_ORDER_LRU) {
		ml_map_move_node_tail(Map, Node);
	} else if (Map->Order == MAP_ORDER_MRU) {
		ml_map_move_node_head(Map, Node);
	}
	return (ml_value_t *)Node;
}

#ifdef ML_MUTABLES

ML_GENERIC_TYPE(MLMapIntegerAnyT, MLMapT, MLIntegerT, MLAnyT);

ML_METHOD("[]", MLMapIntegerAnyT, MLIntegerT) {
//!internal
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_node_t *Node = ml_map_find_node_integer(Map, Args[1]);
	return Node ? Node->Value : MLNil;
}

#endif

#endif

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

typedef struct ml_map_node_waiter_t ml_map_node_waiter_t;

struct ml_map_node_waiter_t {
	ml_map_node_waiter_t *Next;
	ml_state_t *Caller;
};

typedef struct {
	ml_state_t Base;
	ml_value_t *Key;
	ml_map_node_t *Node;
	ml_map_node_waiter_t *Waiters;
} ml_map_node_state_t;

ML_TYPE(MapNodeNodeStateT, (MLStateT), "map::node::state");
//!internal

static void ml_node_state_run(ml_map_node_state_t *State, ml_value_t *Value) {
	if (State->Node->Value != (ml_value_t *)State) {
		Value = State->Node->Value;
	} else if (!ml_is_error(Value)) {
		State->Node->Value = ml_deref(Value);
		Value = (ml_value_t *)State->Node;
	}
	for (ml_map_node_waiter_t *Waiter = State->Waiters; Waiter; Waiter = Waiter->Next) {
		ml_state_schedule(Waiter->Caller, Value);
	}
	ML_CONTINUE(State->Base.Caller, Value);
}

ML_METHODX("[]", MLMapMutableT, MLAnyT, MLFunctionT) {
//<Map
//<Key
//<Fn
//>map::node
// Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then :mini:`Fn(Key)` is called and the result inserted into :mini:`Map`.
//$- let M := {"A" is 1, "B" is 2, "C" is 3}
//$= M["A", fun(Key) Key:code]
//$= M["D", fun(Key) Key:code]
//$= M
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_map_node_t *Node = ml_map_node(Map, NULL, ml_typeof(Key)->hash(Key, NULL), Key);
	if (!Node->Value) {
		ml_map_node_state_t *State = new(ml_map_node_state_t);
		State->Base.Type = MapNodeNodeStateT;
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (void *)ml_node_state_run;
		State->Key = Key;
		State->Node = Node;
		Node->Value = (ml_value_t *)State;
		ml_value_t *Function = Args[2];
		return ml_call(State, Function, 1, &State->Key);
	} else if (ml_typeof(Node->Value) == MapNodeNodeStateT) {
		ml_map_node_state_t *State = (ml_map_node_state_t *)Node->Value;
		ml_map_node_waiter_t *Waiter = new(ml_map_node_waiter_t);
		Waiter->Caller = Caller;
		Waiter->Next = State->Waiters;
		State->Waiters = Waiter;
	} else {
		ML_RETURN(Node);
	}
}

ML_METHOD("empty", MLMapMutableT) {
//<Map
//>map
// Deletes all keys and values from :mini:`Map` and returns it.
//$= let M := {"A" is 1, "B" is 2, "C" is 3}
//$= M:empty
	ml_map_t *Map = (ml_map_t *)Args[0];
	Map->Root = Map->Head = Map->Tail = NULL;
	Map->Size = 0;
#ifdef ML_GENERICS
	Map->Type = MLMapMutableT;
#endif
	return (ml_value_t *)Map;
}

ML_METHOD("pop", MLMapMutableT) {
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

ML_METHOD("pull", MLMapMutableT) {
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

ML_METHOD("pop2", MLMapMutableT) {
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

ML_METHOD("pull2", MLMapMutableT) {
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

ML_METHOD("insert", MLMapMutableT, MLAnyT, MLAnyT) {
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
	return ml_map_insert(Args[0], Args[1], Args[2]);
}

ML_METHOD("splice", MLMapMutableT, MLAnyT) {
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_t *Removed = (ml_map_t *)ml_map();
	ml_map_node_t *Node = ml_map_find_node(Map, Args[1]);
	if (!Node) return MLNil;
	do {
		ml_map_node_t *Next = Node->Next;
		ml_map_delete((ml_value_t *)Map, Node->Key);
		ml_map_node(Removed, Node, Node->Hash, Node->Key);
		Node = Next;
	} while (Node);
	return (ml_value_t *)Removed;
}

ML_METHOD("splice", MLMapMutableT, MLAnyT, MLIntegerT) {
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_t *Removed = (ml_map_t *)ml_map();
	ml_map_node_t *Node = ml_map_find_node(Map, Args[1]);
	if (!Node) return MLNil;
	int Remove = ml_integer_value(Args[2]);
	if (!Remove) return (ml_value_t *)Removed;
	ml_map_node_t *Last = Node;
	while (--Remove > 0) {
		Last = Last->Next;
		if (!Last) return MLNil;
	}
	Last = Last->Next;
	while (Node != Last) {
		ml_map_node_t *Next = Node->Next;
		ml_map_delete((ml_value_t *)Map, Node->Key);
		ml_map_node(Removed, Node, Node->Hash, Node->Key);
		Node = Next;
	}
	return (ml_value_t *)Removed;
}

ML_METHOD("splice", MLMapMutableT, MLAnyT, MLMapMutableT) {
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_t *Removed = (ml_map_t *)ml_map();
	ml_map_node_t *Node = ml_map_find_node(Map, Args[1]);
	if (!Node) return MLNil;
	ml_map_t *Source = (ml_map_t *)Args[2];
	ml_map_node_t *Last = Node;
	ml_map_node_t *Prev = Node->Prev, *Next = Last;
	while (Node != Last) {
		ml_map_node_t *Next = Node->Next;
		ml_map_delete((ml_value_t *)Map, Node->Key);
		ml_map_node(Removed, Node, Node->Hash, Node->Key);
		Node = Next;
	}
	ml_map_order_t Order = Map->Order;
	Map->Order = MAP_ORDER_INSERT;
	for (ml_map_node_t *Node = Source->Head; Node;) {
		ml_map_node_t *Next = Node->Next;
		ml_map_node(Map, Node, ml_typeof(Node->Key)->hash(Node->Key, NULL), Node->Key);
		Node = Next;
	}
	Map->Order = Order;
	if (Next) {
		ml_map_node_t *Head = Source->Head, *Tail = Source->Tail;
		Map->Tail = Head->Prev;
		Map->Tail->Next = NULL;
		Head->Prev = Prev;
		if (Prev) {
			Prev->Next = Head;
		} else {
			Map->Head = Head;
		}
		Tail->Next = Next;
		Next->Prev = Tail;
	}
	Source->Root = Source->Head = Source->Tail = NULL;
	Source->Size = 0;
	return (ml_value_t *)Removed;
}

ML_METHOD("splice", MLMapMutableT, MLAnyT, MLIntegerT, MLMapMutableT) {
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_map_t *Removed = (ml_map_t *)ml_map();
	ml_map_node_t *Node = ml_map_find_node(Map, Args[1]);
	if (!Node) return MLNil;
	int Remove = ml_integer_value(Args[2]);
	ml_map_t *Source = (ml_map_t *)Args[3];
	ml_map_node_t *Last = Node;
	if (Remove) {
		while (--Remove > 0) {
			Last = Last->Next;
			if (!Last) return MLNil;
		}
		Last = Last->Next;
	}
	ml_map_node_t *Prev = Node->Prev, *Next = Last;
	while (Node != Last) {
		ml_map_node_t *Next = Node->Next;
		ml_map_delete((ml_value_t *)Map, Node->Key);
		ml_map_node(Removed, Node, Node->Hash, Node->Key);
		Node = Next;
	}
	ml_map_order_t Order = Map->Order;
	Map->Order = MAP_ORDER_INSERT;
	for (ml_map_node_t *Node = Source->Head; Node;) {
		ml_map_node_t *Next = Node->Next;
		ml_map_node(Map, Node, ml_typeof(Node->Key)->hash(Node->Key, NULL), Node->Key);
		Node = Next;
	}
	Map->Order = Order;
	if (Next) {
		ml_map_node_t *Head = Source->Head, *Tail = Source->Tail;
		Map->Tail = Head->Prev;
		Map->Tail->Next = NULL;
		Head->Prev = Prev;
		if (Prev) {
			Prev->Next = Head;
		} else {
			Map->Head = Head;
		}
		Tail->Next = Next;
		Next->Prev = Tail;
	}
	Source->Root = Source->Head = Source->Tail = NULL;
	Source->Size = 0;
	return (ml_value_t *)Removed;
}

ML_METHOD("push", MLMapMutableT, MLAnyT, MLAnyT) {
//<Map
//<Key
//<Value
//>map
// Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.
//$- let M := {"B" is 2, "C" is 3, "A" is 1}:order(map::order::Descending)
//$= M:push("A", 10)
//$= M:push("D", 20)
//$= M
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_value_t *Value = Args[2];
	ml_map_node_t *Node = ml_map_node(Map, NULL, ml_typeof(Key)->hash(Key, NULL), Key);
	Node->Value = Value;
	ml_type_t *ValueType0 = ml_typeof(Value);
	if (ValueType0 == MLUninitializedT) {
		ml_uninitialized_use(Value, &Node->Value);
		ValueType0 = MLAnyT;
	}
	ml_map_move_node_head(Map, Node);
#ifdef ML_GENERICS
	ml_map_update_generic(Map, Key, Value);
#endif
	return (ml_value_t *)Map;
}

ML_METHOD("put", MLMapMutableT, MLAnyT, MLAnyT) {
//<Map
//<Key
//<Value
//>map
// Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.
//$- let M := {"B" is 2, "C" is 3, "A" is 1}:order(map::order::Descending)
//$= M:put("A", 10)
//$= M:put("D", 20)
//$= M
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_value_t *Value = Args[2];
	ml_map_node_t *Node = ml_map_node(Map, NULL, ml_typeof(Key)->hash(Key, NULL), Key);
	Node->Value = Value;
	ml_type_t *ValueType0 = ml_typeof(Value);
	if (ValueType0 == MLUninitializedT) {
		ml_uninitialized_use(Value, &Node->Value);
		ValueType0 = MLAnyT;
	}
	ml_map_move_node_tail(Map, Node);
#ifdef ML_GENERICS
	ml_map_update_generic(Map, Key, Value);
#endif
	return (ml_value_t *)Map;
}

ML_METHOD("delete", MLMapMutableT, MLAnyT) {
//<Map
//<Key
//>any | nil
// Removes :mini:`Key` from :mini:`Map` and returns the corresponding value if any, otherwise :mini:`nil`.
//$- let M := {"A" is 1, "B" is 2, "C" is 3}
//$= M:delete("A")
//$= M:delete("D")
//$= M
	return ml_map_delete(Args[0], Args[1]);
}

ML_METHOD("missing", MLMapMutableT, MLAnyT) {
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
	ml_map_node_t *Node = ml_map_node(Map, NULL, ml_typeof(Key)->hash(Key, NULL), Key);
	if (!Node->Value) return Node->Value = MLSome;
	return MLNil;
}

static void ml_missing_state_run(ml_map_node_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		ML_CONTINUE(State->Base.Caller, Value);
	} else {
		State->Node->Value = ml_deref(Value);
		ML_CONTINUE(State->Base.Caller, State->Node->Value);
	}
}

ML_METHODX("missing", MLMapMutableT, MLAnyT, MLFunctionT) {
//<Map
//<Key
//<Fn
//>any | nil
// If :mini:`Key` is present in :mini:`Map` then returns :mini:`nil`. Otherwise inserts :mini:`Key` into :mini:`Map` with value :mini:`Fn(Key)` and returns the new value.
//$- let M := {"A" is 1, "B" is 2, "C" is 3}
//$= M:missing("A", fun(Key) Key:code)
//$= M:missing("D", fun(Key) Key:code)
//$= M
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_map_node_t *Node = ml_map_node(Map, NULL, ml_typeof(Key)->hash(Key, NULL), Key);
	if (!Node->Value) {
		Node->Value = MLNil;
		ml_map_node_state_t *State = new(ml_map_node_state_t);
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

static void ml_exists_state_run(ml_map_node_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		ML_CONTINUE(State->Base.Caller, Value);
	} else {
		State->Node->Value = ml_deref(Value);
		ML_CONTINUE(State->Base.Caller, MLNil);
	}
}

ML_METHODX("exists", MLMapMutableT, MLAnyT, MLFunctionT) {
//<Map
//<Key
//<Fn
//>any | nil
// If :mini:`Key` is present in :mini:`Map` then returns the corresponding value. Otherwise inserts :mini:`Key` into :mini:`Map` with value :mini:`Fn(Key)` and returns :mini:`nil`.
//$- let M := {"A" is 1, "B" is 2, "C" is 3}
//$= M:exists("A", fun(Key) Key:code)
//$= M:exists("D", fun(Key) Key:code)
//$= M
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_map_node_t *Node = ml_map_node(Map, NULL, ml_typeof(Key)->hash(Key, NULL), Key);
	if (!Node->Value) {
		Node->Value = MLNil;
		ml_map_node_state_t *State = new(ml_map_node_state_t);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (void *)ml_exists_state_run;
		State->Key = Key;
		State->Node = Node;
		ml_value_t *Function = Args[2];
		return ml_call(State, Function, 1, &State->Key);
	} else {
		ML_RETURN(Node->Value);
	}
}

ML_METHOD("take", MLMapMutableT, MLMapMutableT) {
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
		ml_map_node(Map, Node, ml_typeof(Node->Key)->hash(Node->Key, NULL), Node->Key);
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

typedef struct {
	ml_state_t Base;
	ml_stringbuffer_t *Buffer;
	ml_map_node_t *Node;
	ml_value_t *Args[2];
	ml_hash_chain_t Chain[1];
	const char *Seperator;
	const char *Equals;
	const char *Terminator;
	size_t SeperatorLength;
	size_t EqualsLength;
	size_t TerminatorLength;
} ml_map_append_state_t;

extern ml_value_t *AppendMethod;

static void ml_map_append_state_value(ml_map_append_state_t *State, ml_value_t *Value);

static void ml_map_append_state_key(ml_map_append_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_map_node_t *Node = State->Node;
	if (Node->Value == MLSome) return ml_map_append_state_value(State, Value);
	ml_stringbuffer_write(State->Buffer, State->Equals, State->EqualsLength);
	State->Base.run = (ml_state_fn)ml_map_append_state_value;
	State->Node = Node;
	State->Args[1] = Node->Value;
	return ml_call(State, AppendMethod, 2, State->Args);
}

static void ml_map_append_state_value(ml_map_append_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_map_node_t *Node = State->Node->Next;
	if (!Node) {
		ml_stringbuffer_write(State->Buffer, State->Terminator, State->TerminatorLength);
		if (State->Chain->Index) ml_stringbuffer_printf(State->Buffer, "<%d", State->Chain->Index);
		State->Buffer->Chain = State->Chain->Previous;
		ML_CONTINUE(State->Base.Caller, MLSome);
	}
	ml_stringbuffer_write(State->Buffer, State->Seperator, State->SeperatorLength);
	State->Base.run = (ml_state_fn)ml_map_append_state_key;
	State->Node = Node;
	State->Args[1] = Node->Key;
	return ml_call(State, AppendMethod, 2, State->Args);
}

ML_METHODX("append", MLStringBufferT, MLMapT) {
//<Buffer
//<Map
// Appends a representation of :mini:`Map` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_map_t *Map = (ml_map_t *)Args[1];
	for (ml_hash_chain_t *Link = Buffer->Chain; Link; Link = Link->Previous) {
		if (Link->Value == (ml_value_t *)Map) {
			int Index = Link->Index;
			if (!Index) Index = Link->Index = ++Buffer->Index;
			ml_stringbuffer_printf(Buffer, ">%d", Index);
			ML_RETURN(Buffer);
		}
	}
	ml_map_node_t *Node = Map->Head;
	if (!Node) {
		ml_stringbuffer_write(Buffer, "{}", 2);
		ML_RETURN(MLSome);
	}
	ml_stringbuffer_put(Buffer, '{');
	ml_map_append_state_t *State = new(ml_map_append_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_append_state_key;
	State->Chain->Previous = Buffer->Chain;
	State->Chain->Value = (ml_value_t *)Map;
	Buffer->Chain = State->Chain;
	State->Buffer = Buffer;
	State->Node = Node;
	State->Seperator = ", ";
	State->SeperatorLength = 2;
	State->Equals = " is ";
	State->EqualsLength = 4;
	State->Terminator = "}";
	State->TerminatorLength = 1;
	State->Args[0] = (ml_value_t *)Buffer;
	State->Args[1] = Node->Key;
	return ml_call(State, AppendMethod, 2, State->Args);
}

ML_METHODX("append", MLStringBufferT, MLMapT, MLStringT, MLStringT) {
//<Buffer
//<Map
//<Sep
//<Conn
// Appends the entries of :mini:`Map` to :mini:`Buffer` with :mini:`Conn` between keys and values and :mini:`Sep` between entries.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_map_t *Map = (ml_map_t *)Args[1];
	for (ml_hash_chain_t *Link = Buffer->Chain; Link; Link = Link->Previous) {
		if (Link->Value == (ml_value_t *)Map) {
			int Index = Link->Index;
			if (!Index) Index = Link->Index = ++Buffer->Index;
			ml_stringbuffer_printf(Buffer, ">%d", Index);
			ML_RETURN(Buffer);
		}
	}
	ml_map_node_t *Node = Map->Head;
	if (!Node) ML_RETURN(MLNil);
	ml_map_append_state_t *State = new(ml_map_append_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_append_state_key;
	State->Chain->Previous = Buffer->Chain;
	State->Chain->Value = (ml_value_t *)Map;
	Buffer->Chain = State->Chain;
	State->Buffer = Buffer;
	State->Node = Node;
	State->Seperator = ml_string_value(Args[2]);
	State->SeperatorLength = ml_string_length(Args[2]);
	State->Equals = ml_string_value(Args[3]);
	State->EqualsLength = ml_string_length(Args[3]);
	State->Args[0] = (ml_value_t *)Buffer;
	State->Args[1] = Node->Key;
	return ml_call(State, AppendMethod, 2, State->Args);
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

ML_METHOD("<=>", MLMapT, MLMapT) {
//<Map/1
//<Map/2
//>map
// Returns a tuple of :mini:`(Map/1 / Map/2, Map/1 * Map/2, Map/2 / Map/1)`.
//$= let A := map(swap("banana"))
//$= let B := map(swap("bread"))
//$= A <=> B
	ml_value_t *Map1 = ml_map(), *Map2 = ml_map(), *Map3 = ml_map();
	ML_MAP_FOREACH(Args[0], Node) {
		if (!ml_map_search0(Args[1], Node->Key)) {
			ml_map_insert(Map1, Node->Key, Node->Value);
		} else {
			ml_map_insert(Map2, Node->Key, Node->Value);
		}
	}
	ML_MAP_FOREACH(Args[1], Node) {
		if (!ml_map_search0(Args[0], Node->Key)) ml_map_insert(Map3, Node->Key, Node->Value);
	}
	return ml_tuplev(3, Map1, Map2, Map3);
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

ML_METHODX("sort", MLMapMutableT) {
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

ML_METHODX("sort", MLMapMutableT, MLFunctionT) {
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

ML_METHODX("sort2", MLMapMutableT, MLFunctionT) {
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

ML_METHOD("reverse", MLMapMutableT) {
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

typedef struct {
	ml_state_t Base;
	ml_value_t *Visitor, *Dest, *Key;
	ml_map_node_t *Node;
	ml_value_t *Args[1];
} ml_map_visit_t;

static void ml_map_visit_run(ml_map_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (!State->Key) {
		State->Key = Value;
		State->Args[0] = State->Node->Value;
		return ml_call(State, State->Visitor, 1, State->Args);
	}
	State->Key = NULL;
	ml_map_node_t *Node = State->Node->Next;
	if (!Node) ML_RETURN(MLNil);
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, State->Visitor, 1, State->Args);
}

ML_METHODX("visit", MLVisitorT, MLMapT) {
//<Copy
//<Map
//>map
// Returns a new map contains copies of the keys and values of :mini:`Map` created using :mini:`Copy`.
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_map_node_t *Node = ((ml_map_t *)Args[1])->Head;
	if (!Node) ML_RETURN(MLNil);
	ml_map_visit_t *State = new(ml_map_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_visit_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

static void ml_map_copy_run(ml_map_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (!State->Key) {
		State->Key = Value;
		State->Args[0] = State->Node->Value;
		return ml_call(State, State->Visitor, 1, State->Args);
	}
	ml_map_insert(State->Dest, State->Key, Value);
	State->Key = NULL;
	ml_map_node_t *Node = State->Node->Next;
	if (!Node) ML_RETURN(State->Dest);
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, State->Visitor, 1, State->Args);
}

ML_METHODX("copy", MLVisitorT, MLMapT) {
//<Copy
//<Map
//>map
// Returns a new map contains copies of the keys and values of :mini:`Map` created using :mini:`Copy`.
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_value_t *Dest = ml_map();
	((ml_map_t *)Dest)->Order = ((ml_map_t *)Args[1])->Order;
	inthash_insert(Visitor->Cache, (uintptr_t)Args[1], Dest);
	ml_map_node_t *Node = ((ml_map_t *)Args[1])->Head;
	if (!Node) ML_RETURN(Dest);
	ml_map_visit_t *State = new(ml_map_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_copy_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Dest = Dest;
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

static void ml_map_const_run(ml_map_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (!State->Key) {
		State->Key = Value;
		State->Args[0] = State->Node->Value;
		return ml_call(State, State->Visitor, 1, State->Args);
	}
	ml_map_insert(State->Dest, State->Key, Value);
	State->Key = NULL;
	ml_map_node_t *Node = State->Node->Next;
	if (!Node) {
#ifdef ML_GENERICS
		if (State->Dest->Type->Type == MLTypeGenericT) {
			ml_type_t *TArgs[3];
			ml_find_generic_parent(State->Dest->Type, MLMapMutableT, 3, TArgs);
			TArgs[0] = MLMapT;
			State->Dest->Type = ml_generic_type(3, TArgs);
		} else {
#endif
			State->Dest->Type = MLMapT;
#ifdef ML_GENERICS
		}
#endif
		ML_MAP_FOREACH(State->Dest, Iter) Iter->Type = MLMapNodeT;
		ML_RETURN(State->Dest);
	}
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, State->Visitor, 1, State->Args);
}

ML_METHODX("const", MLVisitorT, MLMapT) {
//<Copy
//<Map
//>map::const
// Returns a new constant map containing copies of the keys and values of :mini:`Map` created using :mini:`Copy`.
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_value_t *Dest = ml_map();
	((ml_map_t *)Dest)->Order = ((ml_map_t *)Args[1])->Order;
	inthash_insert(Visitor->Cache, (uintptr_t)Args[1], Dest);
	ml_map_node_t *Node = ((ml_map_t *)Args[1])->Head;
	if (!Node) ML_RETURN(Dest);
	ml_map_visit_t *State = new(ml_map_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_const_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Dest = Dest;
	State->Node = Node;
	State->Args[0] = Node->Key;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

static int ML_TYPED_FN(ml_value_is_constant, MLMapMutableT, ml_value_t *Map) {
	return 0;
}

static int ML_TYPED_FN(ml_value_is_constant, MLMapT, ml_value_t *Map) {
	ML_MAP_FOREACH(Map, Iter) {
		if (!ml_value_is_constant(Iter->Key)) return 0;
		if (!ml_value_is_constant(Iter->Value)) return 0;
	}
	return 1;
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Map, *Key, *Fn;
	int Count, Index;
	void *Nodes[];
} ml_map_join_state_t;

static void ml_map_join_state_run(ml_map_join_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_value_t *Map = State->Map;
	ml_map_insert(Map, State->Key, Value);
	int N = State->Count;
	int Index = State->Index;
	ml_map_node_t *Node = (ml_map_node_t *)State->Nodes[Index];
	while (Node) {
		if (!ml_map_search0(Map, Node->Key)) {
			ml_value_t *Key = State->Key = Node->Key;
			State->Nodes[Index] = Node->Next;
			ml_value_t **Args2 = ml_alloc_args(N);
			for (int I = 0; I < Index; ++I) Args2[I] = MLNil;
			Args2[Index] = Node->Value;
			for (int I = Index + 1; I < N; ++I) Args2[I] = ml_map_search(State->Nodes[I], Key);
			return ml_call(State, State->Fn, N, Args2);
		}
		Node = Node->Next;
	}
	while (++Index < N) {
		ml_map_node_t *Node = ((ml_map_t *)State->Nodes[Index])->Head;
		while (Node) {
			if (!ml_map_search0(Map, Node->Key)) {
				ml_value_t *Key = State->Key = Node->Key;
				State->Nodes[Index] = Node->Next;
				State->Index = Index;
				ml_value_t **Args2 = ml_alloc_args(N);
				for (int I = 0; I < Index; ++I) Args2[I] = MLNil;
				Args2[Index] = Node->Value;
				for (int I = Index + 1; I < N; ++I) Args2[I] = ml_map_search(State->Nodes[I], Key);
				return ml_call(State, State->Fn, N, Args2);
			}
			Node = Node->Next;
		}
	}
	ML_RETURN(State->Map);
}

ML_FUNCTIONX(MLMapJoin) {
//@map::join
//<Map/1,...:map
//<Fn:function
//>map
// Returns a new map containing the union of the keys of :mini:`Map/i`, and with values :mini:`Fn(V/1, ..., V/n)` where each :mini:`V/i` comes from :mini:`Map/i` (or :mini:`nil`).
//$= let A := map(swap("apple"))
//$= let B := map(swap("banana"))
//$= let C := map(swap("pear"))
//$= map::join(A, B, C, tuple)
	ML_CHECKX_ARG_COUNT(2);
	int N = Count - 1;
	for (int I = 0; I < N; ++I) ML_CHECKX_ARG_TYPE(I, MLMapT);
	ML_CHECKX_ARG_TYPE(N, MLFunctionT);
	ml_map_join_state_t *State = xnew(ml_map_join_state_t, N, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_join_state_run;
	State->Count = N;
	State->Fn = Args[N];
	State->Map = ml_map();
	for (int Index = 0; Index < N; ++Index) {
		ml_map_node_t *Node = ((ml_map_t *)Args[Index])->Head;
		if (Node) {
			ml_value_t *Key = State->Key = Node->Key;
			State->Nodes[Index] = Node->Next;
			State->Index = Index;
			for (int I = Index + 1; I < N; ++I) State->Nodes[I] = Args[I];
			ml_value_t **Args2 = ml_alloc_args(N);
			for (int I = 0; I < Index; ++I) Args2[I] = MLNil;
			Args2[Index] = Node->Value;
			for (int I = Index + 1; I < N; ++I) Args2[I] = ml_map_search(State->Nodes[I], Key);
			return ml_call(State, State->Fn, N, Args2);
		}
	}
	ML_RETURN(State->Map);
}

static void ml_map_join2_state_run(ml_map_join_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_value_t *Map = State->Map;
	ml_map_insert(Map, State->Key, Value);
	int N = State->Count;
	int Index = State->Index;
	ml_map_node_t *Node = (ml_map_node_t *)State->Nodes[Index];
	while (Node) {
		if (!ml_map_search0(Map, Node->Key)) {
			ml_value_t *Key = State->Key = Node->Key;
			State->Nodes[Index] = Node->Next;
			ml_value_t **Args2 = ml_alloc_args(N + 1);
			Args2[0] = Key;
			for (int I = 0; I < Index; ++I) Args2[I + 1] = MLNil;
			Args2[Index + 1] = Node->Value;
			for (int I = Index + 1; I < N; ++I) Args2[I + 1] = ml_map_search(State->Nodes[I], Key);
			return ml_call(State, State->Fn, N + 1, Args2);
		}
		Node = Node->Next;
	}
	while (++Index < N) {
		ml_map_node_t *Node = ((ml_map_t *)State->Nodes[Index])->Head;
		while (Node) {
			if (!ml_map_search0(Map, Node->Key)) {
				ml_value_t *Key = State->Key = Node->Key;
				State->Nodes[Index] = Node->Next;
				State->Index = Index;
				ml_value_t **Args2 = ml_alloc_args(N + 1);
				Args2[0] = Key;
				for (int I = 0; I < Index; ++I) Args2[I + 1] = MLNil;
				Args2[Index + 1] = Node->Value;
				for (int I = Index + 1; I < N; ++I) Args2[I + 1] = ml_map_search(State->Nodes[I], Key);
				return ml_call(State, State->Fn, N + 1, Args2);
			}
			Node = Node->Next;
		}
	}
	ML_RETURN(State->Map);
}

ML_FUNCTIONX(MLMapJoin2) {
//@map::join2
//<Map/1,...:map
//<Fn:function
//>map
// Returns a new map containing the union of the keys of :mini:`Map/i`, and with values :mini:`Fn(K, V/1, ..., V/n)` where each :mini:`V/i` comes from :mini:`Map/i` (or :mini:`nil`).
//$= let A := map(swap("apple"))
//$= let B := map(swap("banana"))
//$= let C := map(swap("pear"))
//$= map::join2(A, B, C, tuple)
	ML_CHECKX_ARG_COUNT(2);
	int N = Count - 1;
	for (int I = 0; I < N; ++I) ML_CHECKX_ARG_TYPE(I, MLMapT);
	ML_CHECKX_ARG_TYPE(N, MLFunctionT);
	ml_map_join_state_t *State = xnew(ml_map_join_state_t, N, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_join2_state_run;
	State->Count = N;
	State->Fn = Args[N];
	State->Map = ml_map();
	for (int Index = 0; Index < N; ++Index) {
		ml_map_node_t *Node = ((ml_map_t *)Args[Index])->Head;
		if (Node) {
			ml_value_t *Key = State->Key = Node->Key;
			State->Nodes[Index] = Node->Next;
			State->Index = Index;
			for (int I = Index + 1; I < N; ++I) State->Nodes[I] = Args[I];
			ml_value_t **Args2 = ml_alloc_args(N + 1);
			Args2[0] = Key;
			for (int I = 0; I < Index; ++I) Args2[I + 1] = MLNil;
			Args2[Index + 1] = Node->Value;
			for (int I = Index + 1; I < N; ++I) Args2[I + 1] = ml_map_search(State->Nodes[I], Key);
			return ml_call(State, State->Fn, N + 1, Args2);
		}
	}
	ML_RETURN(State->Map);
}

static void ml_map_labeller_call(ml_state_t *Caller, ml_value_t *Labeller, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Value = ml_deref(Args[0]);
	ml_map_node_t *Node = ml_map_slot(Labeller, Value);
	if (Node->Value) ML_RETURN(Node->Value);
	Node->Value = ml_integer(ml_map_size(Labeller));
	ML_RETURN(Node->Value);
}

ML_TYPE(MLMapLabellerT, (MLFunctionT, MLMapT), "labeller",
	.call = (void *)ml_map_labeller_call
);

ML_FUNCTION(MLMapLabeller) {
	ml_value_t *Labeller = ml_map();
	Labeller->Type = MLMapLabellerT;
	return Labeller;
}

void ml_map_init() {
#include "ml_map_init.c"
	stringmap_insert(MLMapT->Exports, "mutable", MLMapMutableT);
	MLMapMutableT->Constructor = MLMapT->Constructor;
	stringmap_insert(MLMapT->Exports, "order", MLMapOrderT);
	stringmap_insert(MLMapT->Exports, "join", MLMapJoin);
	stringmap_insert(MLMapT->Exports, "join2", MLMapJoin2);
	stringmap_insert(MLMapT->Exports, "reduce", MLMapReduce);
	stringmap_insert(MLMapT->Exports, "labeller", MLMapLabeller);
	stringmap_insert(MLMapT->Exports, "template", MLMapTemplate);
#ifdef ML_GENERICS
	ml_type_add_rule(MLMapT, MLSequenceT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
#ifdef ML_MUTABLES
	ml_type_add_rule(MLMapMutableT, MLMapT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
#endif
#endif
}

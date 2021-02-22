#include "ml_map.h"
#include "minilang.h"
#include "ml_macros.h"
#include "ml_iterfns.h"
#include <string.h>

ML_TYPE(MLMapT, (MLIteratableT), "map",
// A map of key-value pairs.
// Keys can be of any type supporting hashing and comparison.
// Insert order is preserved.
);

static ml_value_t *ml_map_node_deref(ml_map_node_t *Node) {
	return Node->Value;
}

static void ml_map_node_assign(ml_state_t *Caller, ml_map_node_t *Node, ml_value_t *Value) {
	ML_RETURN(Node->Value = Value);
}

ML_TYPE(MLMapNodeT, (), "map-node",
// A node in a :mini:`map`.
// Dereferencing a :mini:`mapnode` returns the corresponding value from the :mini:`map`.
// Assigning to a :mini:`mapnode` updates the corresponding value in the :mini:`map`.
	.deref = (void *)ml_map_node_deref,
	.assign = (void *)ml_map_node_assign
);

ml_value_t *ml_map() {
	ml_map_t *Map = new(ml_map_t);
	Map->Type = MLMapT;
	return (ml_value_t *)Map;
}

ML_METHOD(MLMapT) {
	return ml_map();
}

typedef struct {
	ml_state_t Base;
	ml_list_node_t *Node;
	ml_map_t *Map;
	ml_value_t **Value;
	ml_value_t *Values[];
} ml_map_names_state_t;

static void ml_map_names_state_run(ml_map_names_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	ml_list_node_t *Node = State->Node->Next;
	if (!Node) ML_CONTINUE(State->Base.Caller, State->Map);
	State->Node = Node;
	ml_value_t **Value = State->Value + 1;
	State->Value = Value;
	return ml_map_insert((ml_state_t *)State, (ml_value_t *)State->Map, Node->Value, *Value);
}

ML_METHODVX(MLMapT, MLNamesT) {
	ml_list_node_t *Node = ((ml_list_t *)Args[0])->Head;
	if (!Node) ML_RETURN(ml_map());
	ml_map_names_state_t *State = xnew(ml_map_names_state_t, Count - 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_names_state_run;
	State->Map = ml_map();
	State->Node = Node;
	for (int I = 1; I < Count; ++I) State->Values[I - 1] = Args[I];
	ml_value_t **Value = State->Value = State->Values;
	return ml_map_insert((ml_state_t *)State, State->Map, Node->Value, *Value);
}

static void map_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void map_iter_insert(ml_iter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	State->Base.run = (void *)map_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void map_iter_value(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)map_iter_insert;
	return ml_map_insert(State, State->Values[0], State->Values[1], Value);
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

ML_METHODVX(MLMapT, MLIteratableT) {
//<Iteratable
//>map
// Returns a map of all the key and value pairs produced by :mini:`Iteratable`.
	ml_iter_state_t *State = xnew(ml_iter_state_t, 2, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)map_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_map();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

extern ml_value_t *CompareMethod;

typedef struct {
	ml_state_t Base;
	ml_map_node_t *Node;
	void (*Complete)(ml_state_t *, ml_map_t *, ml_value_t *, ml_map_node_t *);
	ml_map_t *Map;
	ml_value_t *Args[2];
	long Hash;
} ml_map_find_node_state_t;

static void ml_map_find_node_state_run(ml_map_find_node_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	ml_map_node_t *Node = State->Node;
	int Compare = ml_integer_value(Result);
	if (!Compare) return State->Complete(State->Base.Caller, State->Map, State->Args[0], Node);
	Node = Compare < 0 ? Node->Left : Node->Right;
	long Hash = State->Hash;
	while (Node) {
		if (Hash < Node->Hash) {
			Node = Node->Left;
		} else if (Hash > Node->Hash) {
			Node = Node->Right;
		} else {
			State->Node = Node;
			State->Args[1] = Node->Key;
			return ml_call((ml_state_t *)State, CompareMethod, 2, State->Args);
		}
	}
	return State->Complete(State->Base.Caller, State->Map, State->Args[0], NULL);
}

static void ml_map_find_node(ml_state_t *Caller, ml_map_t *Map, ml_value_t *Key, void (*Complete)(ml_state_t *, ml_map_t *, ml_value_t *, ml_map_node_t *)) {
	ml_map_node_t *Node = Map->Root;
	long Hash = ml_hash(Key);
	while (Node) {
		if (Hash < Node->Hash) {
			Node = Node->Left;
		} else if (Hash > Node->Hash) {
			Node = Node->Right;
		} else {
			ml_map_find_node_state_t *State = new(ml_map_find_node_state_t);
			State->Base.Caller = Caller;
			State->Base.Context = Caller->Context;
			State->Base.run = (ml_state_fn)ml_map_find_node_state_run;
			State->Map = Map;
			State->Args[0] = Key;
			State->Hash = Hash;
			State->Node = Node;
			State->Args[1] = Node->Key;
			State->Complete = Complete;
			return ml_call((ml_state_t *)State, CompareMethod, 2, State->Args);
		}
	}
	Complete(Caller, Map, Key, NULL);
}

static void ml_map_search_nil(ml_state_t *Caller, ml_map_t *Map, ml_value_t *Key, ml_map_node_t *Node) {
	ML_RETURN(Node ? Node->Value : MLNil);
}

static void ml_map_search_null(ml_state_t *Caller, ml_map_t *Map, ml_value_t *Key, ml_map_node_t *Node) {
	ML_RETURN(Node);
}

void ml_map_search(ml_state_t *Caller, ml_value_t *Map, ml_value_t *Key) {
	return ml_map_find_node(Caller, (ml_map_t *)Map, Key, ml_map_search_nil);
}

ml_value_t *ml_map_search_string(ml_value_t *Map, const char *String) {
	static ml_result_state_t State = {{MLStateT, NULL, (void *)ml_result_state_run, &MLRootContext}, MLNil};
	ml_map_search((ml_state_t *)&State, Map, ml_cstring(String));
	ml_value_t *Value = State.Value;
	State.Value = MLNil;
	return Value;
}

/*ml_value_t *ml_map_search_(ml_value_t *Map0, ml_value_t *Key) {
	ml_map_node_t *Node = ml_map_find_node_((ml_map_t *)Map0, Key);
	return Node ? Node->Value : MLNil;
}

ml_value_t *ml_map_search0_(ml_value_t *Map0, ml_value_t *Key) {
	ml_map_node_t *Node = ml_map_find_node_((ml_map_t *)Map0, Key);
	return Node ? Node->Value : NULL;
}*/

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

#define MAX_DEPTH 64

typedef struct {
	ml_state_t Base;
	ml_map_node_t **Slot;
	void (*Complete)(ml_state_t *, ml_map_t *, ml_value_t *, ml_map_node_t *, ml_value_t *);
	ml_map_t *Map;
	ml_value_t *Value;
	ml_value_t *Args[2];
	ml_map_node_t **Balance[MAX_DEPTH];
	long Hash;
	int NumBalance;
} ml_map_update_node_state_t;

static void ml_map_update_finish(ml_map_node_t **Slots, int NumBalance) {
	for (int I = NumBalance; --I >= 0;) {
		ml_map_node_t *Slot = Slots[I];
		ml_map_rebalance(Slot);
		ml_map_update_depth(Slot[0]);
	}
}

void ml_map_create_node_state_run(ml_map_update_node_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	ml_map_node_t **Slot = State->Slot;
	int Compare = ml_integer_value(Result);
	if (!Compare) {
		ml_map_update_finish(State->Balance, State->NumBalance);
		return State->Complete(State->Base.Caller, State->Map, State->Args[0], Slot[0], State->Value);
	}
	State->Balance[State->NumBalance++] = Slot;
	Slot = Compare < 0 ? &Slot[0]->Left : &Slot[0]->Right;
	long Hash = State->Hash;
	while (Slot[0]) {
		if (Hash < Slot[0]->Hash) {
			State->Balance[State->NumBalance++] = Slot;
			Slot = &Slot[0]->Left;
		} else if (Hash > Slot[0]->Hash) {
			State->Balance[State->NumBalance++] = Slot;
			Slot = &Slot[0]->Right;
		} else {
			State->Slot = Slot;
			State->Args[1] = Slot[0]->Key;
			return ml_call((ml_state_t *)State, CompareMethod, 2, State->Args);
		}
	}
	ml_map_t *Map = State->Map;
	++Map->Size;
	ml_map_node_t *Node = Slot[0] = new(ml_map_node_t);
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
	Node->Key = State->Args[0];
	ml_map_update_finish(State->Balance, State->NumBalance);
	return State->Complete(State->Base.Caller, Map, Node->Key, Node, State->Value);
}

static void ml_map_create_node(ml_state_t *Caller, ml_map_t *Map, ml_value_t *Key, ml_value_t *Value, void (*Complete)(ml_state_t *, ml_map_t *, ml_value_t *, ml_map_node_t *, ml_value_t *)) {
	ml_map_node_t **Slot = &Map->Root;
	long Hash = ml_hash(Key);
	ml_map_update_node_state_t *State = new(ml_map_update_node_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_create_node_state_run;
	State->Map = Map;
	State->Value = Value;
	State->Args[0] = Key;
	State->Hash = Hash;
	State->Complete = Complete;
	while (Slot[0]) {
		if (Hash < Slot[0]->Hash) {
			State->Balance[State->NumBalance++] = Slot;
			Slot = &Slot[0]->Left;
		} else if (Hash > Slot[0]->Hash) {
			State->Balance[State->NumBalance++] = Slot;
			Slot = &Slot[0]->Right;
		} else {
			State->Slot = Slot;
			State->Args[1] = Slot[0]->Key;
			return ml_call((ml_state_t *)State, CompareMethod, 2, State->Args);
		}
	}
	++Map->Size;
	ml_map_node_t *Node = Slot[0] = new(ml_map_node_t);
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
	Node->Key = State->Args[0];
	ml_map_update_finish(State->Balance, State->NumBalance);
	Complete(Caller, Map, Key, Node, Value);
}

static void ml_map_create_slot(ml_state_t *Caller, ml_map_t *Map, ml_value_t *Key, ml_map_node_t *Node, ml_value_t *Value) {
	ML_RETURN(Node);
}

void ml_map_slot(ml_state_t *Caller, ml_value_t *Map0, ml_value_t *Key) {
	return ml_map_create_node(Caller, (ml_map_t *)Map0, Key, NULL, ml_map_create_slot);
}

static void ml_map_create_insert(ml_state_t *Caller, ml_map_t *Map, ml_value_t *Key, ml_map_node_t *Node, ml_value_t *Value) {
	ml_value_t *Old = Node->Value ?: MLNil;
	Node->Value = Value;
#ifdef ML_GENERICS
	if (Map->Size == 1 && Map->Type == MLMapT) {
		ml_type_t *Types[] = {MLMapT, ml_typeof(Key), ml_typeof(Value)};
		Map->Type = ml_generic_type(3, Types);
	} else if (Map->Type->Type == MLGenericTypeT) {
		ml_type_t *KeyType = ml_generic_type_args(Map->Type)[1];
		ml_type_t *ValueType = ml_generic_type_args(Map->Type)[2];
		if (KeyType != ml_typeof(Key) || ValueType != ml_typeof(Value)) {
			ml_type_t *KeyType2 = ml_type_max(KeyType, ml_typeof(Key));
			ml_type_t *ValueType2 = ml_type_max(ValueType, ml_typeof(Value));
			if (KeyType != KeyType2 || ValueType != ValueType2) {
				ml_type_t *Types[] = {MLMapT, KeyType2, ValueType2};
				Map->Type = ml_generic_type(3, Types);
			}
		}
	}
#endif
	ML_RETURN(Old);
}

void ml_map_insert(ml_state_t *Caller, ml_value_t *Map0, ml_value_t *Key, ml_value_t *Value) {
	return ml_map_create_node(Caller, (ml_map_t *)Map0, Key, Value, ml_map_create_insert);
}

void ml_map_insert_string(ml_value_t *Map, const char *String, ml_value_t *Value) {
	static ml_result_state_t State = {{MLStateT, NULL, (void *)ml_result_state_run, &MLRootContext}, MLNil};
	ml_map_insert((ml_state_t *)&State, Map, ml_cstring(String), Value);
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
		ml_value_t *Result = ml_simple_call(CompareMethod, 2, Args);
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
	ml_map_t *Map = (ml_map_t *)Args[0];
	return ml_integer(Map->Size);
}

ML_METHOD("count", MLMapT) {
//<Map
//>integer
// Returns the number of entries in :mini:`Map`.
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
		ml_value_t *Result = ml_simple_call(CompareMethod, 2, Args);
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
	ML_RETURN(Node->Value = Value);
}

ML_TYPE(MLMapIndexT, (), "map-index",
//!internal
	.deref = (void *)ml_map_index_deref,
	.assign = (void *)ml_map_index_assign
);

static void ml_map_search_index(ml_state_t *Caller, ml_map_t *Map, ml_value_t *Key, ml_map_node_t *Node) {
	if (!Node) {
		Node = new(ml_map_node_t);
		Node->Type = MLMapIndexT;
		Node->Value = (ml_value_t *)Map;
		Node->Key = Key;
	}
	ML_RETURN(Node);
}

ML_METHODX("[]", MLMapT, MLAnyT) {
//<Map
//<Key
//>mapnode
// Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then a new floating node is returned with value :mini:`nil`. This node will insert :mini:`Key` into :mini:`Map` if assigned.
	return ml_map_find_node(Caller, (ml_map_t *)Args[0], Args[1], ml_map_search_index);
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
//<Default
//>mapnode
// Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then :mini:`Default(Key)` is called and the result inserted into :mini:`Map`.
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_map_node_t *Node = ml_map_create_node(Map, &Map->Root, ml_typeof(Key)->hash(Key, NULL), Key);
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

ML_METHODX("::", MLMapT, MLStringT) {
//<Map
//<Key
//>mapnode
// Same as :mini:`Map[Key]`. This method allows maps to be used as modules.
	return ml_map_find_node(Caller, (ml_map_t *)Args[0], Args[1], ml_map_search_index);
}

ML_METHOD("insert", MLMapT, MLAnyT, MLAnyT) {
//<Map
//<Key
//<Value
//>any | nil
// Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.
// Returns the previous value associated with :mini:`Key` if any, otherwise :mini:`nil`.
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
	ml_value_t *Map = (ml_value_t *)Args[0];
	ml_value_t *Key = Args[1];
	return ml_map_delete(Map, Key);
}

ml_value_t *ml_map_fn(void *Data, int Count, ml_value_t **Args) {
	ml_map_t *Map = new(ml_map_t);
	Map->Type = MLMapT;
	for (int I = 0; I < Count; I += 2) ml_map_insert((ml_value_t *)Map, Args[I], Args[I + 1]);
	return (ml_value_t *)Map;
}

ML_METHOD("missing", MLMapT, MLAnyT) {
//<Map
//<Key
//>any | nil
// Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.
// Returns the previous value associated with :mini:`Key` if any, otherwise :mini:`nil`.
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_map_node_t *Node = ml_map_create_node(Map, &Map->Root, ml_typeof(Key)->hash(Key, NULL), Key);
	if (!Node->Value) return Node->Value = MLSome;
	return MLNil;
}

ML_METHOD("append", MLStringBufferT, MLMapT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_add(Buffer, "{", 1);
	ml_map_t *Map = (ml_map_t *)Args[1];
	ml_map_node_t *Node = Map->Head;
	if (Node) {
		ml_stringbuffer_append(Buffer, Node->Key);
		if (Node->Value != MLSome) {
			ml_stringbuffer_add(Buffer, " is ", 4);
			ml_stringbuffer_append(Buffer, Node->Value);
		}
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			ml_stringbuffer_append(Buffer, Node->Key);
			if (Node->Value != MLSome) {
				ml_stringbuffer_add(Buffer, " is ", 4);
				ml_stringbuffer_append(Buffer, Node->Value);
			}
		}
	}
	ml_stringbuffer_add(Buffer, "}", 1);
	return (ml_value_t *)Buffer;
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

typedef struct {
	ml_state_t Base;
	ml_map_node_t *Node;
	ml_value_t *Args[];
} ml_map_foreach_state_t;

static void ml_map_union_run(ml_map_foreach_state_t *State, ml_value_t *Result) {

}

ML_METHOD("+", MLMapT, MLMapT) {
//<Map/1
//<Map/2
//>map
// Returns a new map combining the entries of :mini:`Map/1` and :mini:`Map/2`.
// If the same key is in both :mini:`Map/1` and :mini:`Map/2` then the corresponding value from :mini:`Map/2` is chosen.
	ml_value_t *Map = ml_map();
	ML_MAP_FOREACH(Args[0], Node) ml_map_insert(Map, Node->Key, Node->Value);
	ML_MAP_FOREACH(Args[1], Node) ml_map_insert(Map, Node->Key, Node->Value);
	return Map;
}

// Args[0] = 2nd Map
// Args[1] = Result Map
static void ml_map_intersection_search(ml_map_foreach_state_t *State, ml_value_t *Result) {
	if (!Result) {
		ml_map_node_t *Node = State->Node->Next;
		if (!Node) ML_CONTINUE(State->Base.Caller, State->Args[1]);
		State->Node = Node;
	} else if (ml_is_error(Result)) {
		ML_CONTINUE(State->Base.Caller, Result);
	} else {
		ml_map_node_t *Node = State->Node;
		ml_map_insert(State->Args[1], Node->Key, Result);
		Node = Node->Next;
		if (!Node) ML_CONTINUE(State->Base.Caller, State->Args[1]);
		State->Node = Node;
	}
	ml_map_search((ml_state_t *)State, State->Args[0], State->Node->Key);
}

ML_METHODX("*", MLMapT, MLMapT) {
//<Map/1
//<Map/2
//>map
// Returns a new map containing the entries of :mini:`Map/1` which are also in :mini:`Map/2`. The values are chosen from :mini:`Map/1`.
	ml_map_t *Map1 = (ml_map_t *)Args[0];
	if (!Map1->Head) ML_RETURN(ml_map());
	ml_map_foreach_state_t *State = xnew(ml_map_foreach_state_t, 2, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_intersection_search;
	State->Node = Map1->Head;
	State->Args[0] = Args[1];
	State->Args[1] = ml_map();
	return ml_map_search((ml_state_t *)State, State->Args[0], State->Node->Key);
}

// Args[0] = 2nd Map
// Args[1] = Result Map
static void ml_map_difference_search(ml_map_foreach_state_t *State, ml_value_t *Result) {
	if (!Result) {
		ml_map_node_t *Node = State->Node;
		ml_map_insert(State->Args[1], Node->Key, Node->Value);
		Node = Node->Next;
		if (!Node) ML_CONTINUE(State->Base.Caller, State->Args[1]);
		State->Node = Node;
	} else if (ml_is_error(Result)) {
		ML_CONTINUE(State->Base.Caller, Result);
	} else {
		ml_map_node_t *Node = State->Node->Next;
		if (!Node) ML_CONTINUE(State->Base.Caller, State->Args[1]);
		State->Node = Node;
	}
	ml_map_search((ml_state_t *)State, State->Args[0], State->Node->Key);
}

ML_METHODX("/", MLMapT, MLMapT) {
//<Map/1
//<Map/2
//>map
// Returns a new map containing the entries of :mini:`Map/1` which are not in :mini:`Map/2`.
	ml_map_t *Map1 = (ml_map_t *)Args[0];
	if (!Map1->Head) ML_RETURN(ml_map());
	ml_map_foreach_state_t *State = xnew(ml_map_foreach_state_t, 2, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_map_intersection_search;
	State->Node = Map1->Head;
	State->Args[0] = Args[1];
	State->Args[1] = ml_map();
	return ml_map_search((ml_state_t *)State, State->Args[0], State->Node->Key);
}

typedef struct ml_map_stringer_t {
	const char *Seperator, *Equals;
	ml_stringbuffer_t Buffer[1];
	int SeperatorLength, EqualsLength, First;
	ml_value_t *Error;
} ml_map_stringer_t;

static int ml_map_stringer(ml_value_t *Key, ml_value_t *Value, ml_map_stringer_t *Stringer) {
	if (!Stringer->First) {
		ml_stringbuffer_add(Stringer->Buffer, Stringer->Seperator, Stringer->SeperatorLength);
	} else {
		Stringer->First = 0;
	}
	Stringer->Error = ml_stringbuffer_append(Stringer->Buffer, Key);
	if (ml_is_error(Stringer->Error)) return 1;
	ml_stringbuffer_add(Stringer->Buffer, Stringer->Equals, Stringer->EqualsLength);
	Stringer->Error = ml_stringbuffer_append(Stringer->Buffer, Value);
	if (ml_is_error(Stringer->Error)) return 1;
	return 0;
}

ML_METHOD(MLStringT, MLMapT) {
//<Map
//>string
// Returns a string containing the entries of :mini:`Map` surrounded by :mini:`"{"`, :mini:`"}"` with :mini:`" is "` between keys and values and :mini:`", "` between entries.
	ml_map_stringer_t Stringer[1] = {{
		", ", " is ",
		{ML_STRINGBUFFER_INIT},
		2, 4,
		1
	}};
	ml_stringbuffer_add(Stringer->Buffer, "{", 1);
	if (ml_map_foreach(Args[0], Stringer, (void *)ml_map_stringer)) {
		return Stringer->Error;
	}
	ml_stringbuffer_add(Stringer->Buffer, "}", 1);
	return ml_string(ml_stringbuffer_get(Stringer->Buffer), -1);
}

ML_METHOD(MLStringT, MLMapT, MLStringT, MLStringT) {
//<Map
//<Seperator
//<Connector
//>string
// Returns a string containing the entries of :mini:`Map` with :mini:`Connector` between keys and values and :mini:`Seperator` between entries.
	ml_map_stringer_t Stringer[1] = {{
		ml_string_value(Args[1]), ml_string_value(Args[2]),
		{ML_STRINGBUFFER_INIT},
		ml_string_length(Args[1]), ml_string_length(Args[2]),
		1
	}};
	if (ml_map_foreach(Args[0], Stringer, (void *)ml_map_stringer)) return Stringer->Error;
	return ml_stringbuffer_value(Stringer->Buffer);
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
	if (Result) {
		if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
		goto resume;
	}
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
					if (Result == MLNil) {
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
			State->Map->Head = State->Head;
			State->Map->Tail = State->Tail;
			State->Map->Size = State->Size;
			break;
		}
		State->InSize *= 2;
	}
	ML_CONTINUE(State->Base.Caller, State->Map);
}

extern ml_value_t *LessMethod;

ML_METHODX("sort", MLMapT) {
//<Map
//>Map
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
//<Compare
//>Map
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
//<Compare
//>Map
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

void ml_map_init() {
#include "ml_map_init.c"
#ifdef ML_GENERICS
	ml_type_add_rule(MLMapT, MLIteratableT, ML_TYPE_ARG(1), ML_TYPE_ARG(2), NULL);
#endif
}

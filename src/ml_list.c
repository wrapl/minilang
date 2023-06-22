#include "ml_list.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include "ml_sequence.h"

#undef ML_CATEGORY
#define ML_CATEGORY "list"

static ml_list_node_t *ml_list_index(ml_list_t *List, int Index) {
	int Length = List->Length;
	if (Index <= 0) Index += Length + 1;
	if (Index > Length) return NULL;
	if (Index == Length) return List->Tail;
	if (Index < 1) return NULL;
	if (Index == 1) return List->Head;
	int CachedIndex = List->CachedIndex;
	switch (Index - CachedIndex) {
	case -1: {
		List->CachedIndex = Index;
		return (List->CachedNode = List->CachedNode->Prev);
	}
	case 0: return List->CachedNode;
	case 1: {
		List->CachedIndex = Index;
		return (List->CachedNode = List->CachedNode->Next);
	}
	}
	List->CachedIndex = Index;
	ml_list_node_t *Node;
	if (2 * Index < CachedIndex) {
		Node = List->Head;
		int Steps = Index - 1;
		do Node = Node->Next; while (--Steps);
	} else if (Index < CachedIndex) {
		Node = List->CachedNode;
		int Steps = CachedIndex - Index;
		do Node = Node->Prev; while (--Steps);
	} else if (2 * Index < CachedIndex + Length) {
		Node = List->CachedNode;
		int Steps = Index - CachedIndex;
		do Node = Node->Next; while (--Steps);
	} else {
		Node = List->Tail;
		int Steps = Length - Index;
		do Node = Node->Prev; while (--Steps);
	}
	return (List->CachedNode = Node);
}

ML_TYPE(MLListT, (MLSequenceT), "list"
// A list of elements.
);

#ifdef ML_MUTABLES
ML_TYPE(MLListMutableT, (MLListT), "list::mutable");
#else
#define MLListMutableT MLListT
#endif

#ifdef ML_GENERICS

static void ml_list_update_generic(ml_list_t *List, ml_value_t *Value) {
	if (List->Type->Type != MLTypeGenericT) {
		List->Type = ml_generic_type(2, (ml_type_t *[]){List->Type, ml_typeof(Value)});
	} else {
		ml_type_t *ValueType0 = ml_typeof(Value);
		ml_type_t *BaseType = ml_generic_type_args(List->Type)[0];
		ml_type_t *ValueType = ml_generic_type_args(List->Type)[1];
		if (!ml_is_subtype(ValueType0, ValueType)) {
			ml_type_t *ValueType2 = ml_type_max(ValueType, ValueType0);
			if (ValueType != ValueType2) {
				List->Type = ml_generic_type(2, (ml_type_t *[]){BaseType, ValueType2});
			}
		}
	}
}

#endif


static void ML_TYPED_FN(ml_value_find_all, MLListT, ml_value_t *Value, void *Data, ml_value_find_fn RefFn) {
	if (!RefFn(Data, Value, 1)) return;
	ML_LIST_FOREACH(Value, Iter) ml_value_find_all(Iter->Value, Data, RefFn);
}

static ml_value_t *ml_list_node_deref(ml_list_node_t *Node) {
	return Node->Value;
}

static void ml_list_node_assign(ml_state_t *Caller, ml_list_node_t *Node, ml_value_t *Value) {
	Node->Value = Value;
	ML_RETURN(Value);
}

#ifdef ML_MUTABLES

ML_TYPE(MLListNodeT, (), "list::node",
// A node in a :mini:`list`.
// Dereferencing a :mini:`list::node::const` returns the corresponding value from the :mini:`list`.
	.deref = (void *)ml_list_node_deref
);

ML_TYPE(MLListNodeMutableT, (MLListNodeT), "list::node::mutable",
// A node in a :mini:`list`.
// Dereferencing a :mini:`list::node` returns the corresponding value from the :mini:`list`.
// Assigning to a :mini:`list::node` updates the corresponding value in the :mini:`list`.
	.deref = (void *)ml_list_node_deref,
	.assign = (void *)ml_list_node_assign
);

#else

#define MLListNodeMutableT MLListNodeT

ML_TYPE(MLListNodeMutableT, (), "list::node",
// A node in a :mini:`list`.
// Dereferencing a :mini:`list::node` returns the corresponding value from the :mini:`list`.
// Assigning to a :mini:`list::node` updates the corresponding value in the :mini:`list`.
	.deref = (void *)ml_list_node_deref,
	.assign = (void *)ml_list_node_assign
);

#endif

static void ML_TYPED_FN(ml_iter_next, MLListNodeT, ml_state_t *Caller, ml_list_node_t *Node) {
	ml_list_node_t *Next = Node->Next;
	if (!Next) ML_RETURN(MLNil);
	Next->Index = Node->Index + 1;
	ML_RETURN(Next);
}

static void ML_TYPED_FN(ml_iter_key, MLListNodeT, ml_state_t *Caller, ml_list_node_t *Node) {
	ML_RETURN(ml_integer(Node->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLListNodeT, ml_state_t *Caller, ml_list_node_t *Node) {
	ML_RETURN(Node);
}

ml_value_t *ml_list() {
	ml_list_t *List = new(ml_list_t);
	List->Type = MLListMutableT;
	List->Head = List->Tail = NULL;
	List->Length = 0;
	return (ml_value_t *)List;
}

ML_METHOD(MLListT) {
//>list
// Returns an empty list.
//$= list()
	return ml_list();
}

ML_METHOD(MLListT, MLTupleT) {
//<Tuple
//>list
// Returns a list containing the values in :mini:`Tuple`.
//$= list((1, 2, 3))
	ml_value_t *List = ml_list();
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	for (int I = 0; I < Tuple->Size; ++I) {
		ml_value_t *Value = Tuple->Values[I];
		ml_list_put(List, ml_deref(Value));
	}
	return List;
}

static void list_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void list_iter_value(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_list_put(State->Values[0], Value);
	State->Base.run = (void *)list_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void list_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)list_iter_value;
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[0]);
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_METHODVX(MLListT, MLSequenceT) {
//<Sequence
//>list
// Returns a list of all of the values produced by :mini:`Sequence`.
//$= list(1 .. 10)
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)list_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_list();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
}

ML_METHODVX("grow", MLListMutableT, MLSequenceT) {
//<List
//<Sequence
//>list
// Pushes of all of the values produced by :mini:`Sequence` onto :mini:`List` and returns :mini:`List`.
//$- let L := [1, 2, 3]
//$= L:grow(4 .. 6)
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)list_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = Args[0];
	return ml_iterate((ml_state_t *)State, ml_chained(Count - 1, Args + 1));
}

ml_value_t *ml_list_from_array(ml_value_t **Values, int Length) {
	ml_value_t *List = ml_list();
	for (int I = 0; I < Length; ++I) ml_list_put(List, Values[I]);
	return List;
}

void ml_list_to_array(ml_value_t *List, ml_value_t **Values) {
	int I = 0;
	for (ml_list_node_t *Node = ((ml_list_t *)List)->Head; Node; Node = Node->Next, ++I) {
		Values[I] = Node->Value;
	}
}

void ml_list_grow(ml_value_t *List0, int Count) {
	ml_list_t *List = (ml_list_t *)List0;
	for (int I = 0; I < Count; ++I) ml_list_put(List0, MLNil);
	List->CachedIndex = 1;
	List->CachedNode = List->Head;
}

void ml_list_push(ml_value_t *List0, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = new(ml_list_node_t);
	Node->Type = MLListNodeMutableT;
	Node->Value = Value;
	if ((Node->Next = List->Head)) {
		List->Head->Prev = Node;
	} else {
		List->Tail = Node;
	}
#ifdef ML_GENERICS
	ml_list_update_generic(List, Value);
#endif
	List->CachedNode = List->Head = Node;
	List->CachedIndex = 1;
	++List->Length;
}

void ml_list_put(ml_value_t *List0, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = new(ml_list_node_t);
	Node->Type = MLListNodeMutableT;
	Node->Value = Value;
	ml_type_t *Type0 = ml_typeof(Value);
	if (Type0 == MLUninitializedT) {
		ml_uninitialized_use(Value, &Node->Value);
		Type0 = MLAnyT;
	}
	if ((Node->Prev = List->Tail)) {
		List->Tail->Next = Node;
	} else {
		List->Head = Node;
	}
#ifdef ML_GENERICS
	ml_list_update_generic(List, Value);
#endif
	List->CachedNode = List->Tail = Node;
	List->CachedIndex = ++List->Length;
}

ml_value_t *ml_list_pop(ml_value_t *List0) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = List->Head;
	if (Node) {
		if ((List->Head = Node->Next)) {
			List->Head->Prev = NULL;
		} else {
			List->Tail = NULL;
		}
		List->CachedNode = List->Head;
		List->CachedIndex = 1;
		--List->Length;
		return Node->Value;
	} else {
		return MLNil;
	}
}

ml_value_t *ml_list_pull(ml_value_t *List0) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = List->Tail;
	if (Node) {
		if ((List->Tail = Node->Prev)) {
			List->Tail->Next = NULL;
		} else {
			List->Head = NULL;
		}
		List->CachedNode = List->Tail;
		List->CachedIndex = --List->Length;
		return Node->Value;
	} else {
		return MLNil;
	}
}

ml_value_t *ml_list_get(ml_value_t *List0, int Index) {
	ml_list_node_t *Node = ml_list_index((ml_list_t *)List0, Index);
	return Node ? Node->Value : NULL;
}

ml_value_t *ml_list_set(ml_value_t *List0, int Index, ml_value_t *Value) {
	ml_list_node_t *Node = ml_list_index((ml_list_t *)List0, Index);
	if (Node) {
		ml_value_t *Old = Node->Value;
		Node->Value = Value;
		return Old;
	} else {
		return NULL;
	}
}

int ml_list_foreach(ml_value_t *Value, void *Data, int (*callback)(ml_value_t *, void *)) {
	ML_LIST_FOREACH(Value, Node) if (callback(Node->Value, Data)) return 1;
	return 0;
}

ML_METHOD("precount", MLListT) {
//<List
//>integer
// Returns the length of :mini:`List`
//$= [1, 2, 3]:count
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_integer(List->Length);
}

ML_METHOD("count", MLListT) {
//<List
//>integer
// Returns the length of :mini:`List`
//$= [1, 2, 3]:count
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_integer(List->Length);
}

ML_METHOD("length", MLListT) {
//<List
//>integer
// Returns the length of :mini:`List`
//$= [1, 2, 3]:length
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_integer(List->Length);
}

ML_METHOD("first", MLListT) {
//<List
// Returns the first value in :mini:`List` or :mini:`nil` if :mini:`List` is empty.
	ml_list_t *List = (ml_list_t *)Args[0];
	return List->Head ? List->Head->Value : MLNil;
}

ML_METHOD("last", MLListT) {
//<List
// Returns the last value in :mini:`List` or :mini:`nil` if :mini:`List` is empty.
	ml_list_t *List = (ml_list_t *)Args[0];
	return List->Tail ? List->Tail->Value : MLNil;
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Filter;
	ml_list_t *List, *Drop;
	ml_list_node_t *Node;
	ml_list_node_t **KeepSlot;
	ml_list_node_t *KeepTail;
	ml_list_node_t **DropSlot;
	ml_list_node_t *DropTail;
	int Length;
} ml_list_filter_state_t;

static void ml_list_filter_state_run(ml_list_filter_state_t *State, ml_value_t *Result) {
	if (Result) {
		if (ml_is_error(Result)) {
			State->List->Head = State->List->Tail = NULL;
			State->List->Length = 0;
			ML_CONTINUE(State->Base.Caller, Result);
		}
		goto resume;
	}
	while (State->Node) {
		return ml_call((ml_state_t *)State, State->Filter, 1, &State->Node->Value);
	resume:
		if (Result == MLNil) {
			State->Node->Prev = State->DropSlot[0];
			State->DropSlot[0] = State->Node;
			State->DropSlot = &State->Node->Next;
			State->DropTail = State->Node;
		} else {
			State->Node->Prev = State->KeepSlot[0];
			State->KeepSlot[0] = State->Node;
			State->KeepSlot = &State->Node->Next;
			++State->Length;
			State->KeepTail = State->Node;
		}
		State->Node = State->Node->Next;
	}
	State->Drop->Tail = State->DropTail;
	if (State->DropTail) State->DropTail->Next = NULL;
	State->Drop->Length = State->List->Length - State->Length;
	State->Drop->CachedIndex = State->Drop->Length;
	State->Drop->CachedNode = State->DropTail;
	State->List->Tail = State->KeepTail;
	if (State->KeepTail) State->KeepTail->Next = NULL;
	State->List->Length = State->Length;
	State->List->CachedIndex = State->Length;
	State->List->CachedNode = State->KeepTail;
	ML_CONTINUE(State->Base.Caller, State->Drop);
}

ML_METHODX("filter", MLListMutableT, MLFunctionT) {
//<List
//<Filter
//>list
// Removes every :mini:`Value` from :mini:`List` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list.
//$- let L := [1, 2, 3, 4, 5, 6]
//$= L:filter(2 | _)
//$= L
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_filter_state_t *State = new(ml_list_filter_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_list_filter_state_run;
	State->List = List;
	ml_list_t *Drop = State->Drop = new(ml_list_t);
	Drop->Type = MLListMutableT;
	State->Filter = Args[1];
	State->Node = List->Head;
	State->KeepSlot = &List->Head;
	State->KeepTail = NULL;
	State->DropSlot = &Drop->Head;
	State->DropTail = NULL;
	List->Head = NULL;
	State->Length = 0;
	return ml_list_filter_state_run(State, NULL);
}

ML_METHOD("[]", MLListT, MLIntegerT) {
//<List
//<Index
//>list::node | nil
// Returns the :mini:`Index`-th node in :mini:`List` or :mini:`nil` if :mini:`Index` is outside the range of :mini:`List`.
// Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.
//$- let L := ["a", "b", "c", "d", "e", "f"]
//$= L[3]
//$= L[-2]
//$= L[8]
	ml_list_t *List = (ml_list_t *)Args[0];
	int Index = ml_integer_value_fast(Args[1]);
	return (ml_value_t *)ml_list_index(List, Index) ?: MLNil;
}

typedef struct {
	ml_type_t *Type;
	ml_list_node_t *Head;
	int Length;
} ml_list_slice_t;

static ml_value_t *ml_list_slice_deref(ml_list_slice_t *Slice) {
	ml_value_t *List = ml_list();
	ml_list_node_t *Node = Slice->Head;
	int Length = Slice->Length;
	while (Node && Length) {
		ml_list_put(List, Node->Value);
		Node = Node->Next;
		--Length;
	}
	return List;
}

static void ml_list_slice_assign(ml_state_t *Caller, ml_list_slice_t *Slice, ml_value_t *Packed) {
	ml_list_node_t *Node = Slice->Head;
	int Length = Slice->Length;
	int Index = 0;
	while (Node && Length) {
		ml_value_t *Value = ml_unpack(Packed, Index + 1);
		++Index;
		Node->Value = Value;
		Node = Node->Next;
		--Length;
	}
	ML_RETURN(Packed);
}

ML_TYPE(MLListSliceT, (), "list-slice",
// A slice of a list.
	.deref = (void *)ml_list_slice_deref,
	.assign = (void *)ml_list_slice_assign
);

ML_METHOD("[]", MLListMutableT, MLIntegerT, MLIntegerT) {
//<List
//<From
//<To
//>list::slice
// Returns a slice of :mini:`List` starting at :mini:`From` (inclusive) and ending at :mini:`To` (exclusive).
// Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.
	ml_list_t *List = (ml_list_t *)Args[0];
	int Start = ml_integer_value_fast(Args[1]);
	int End = ml_integer_value_fast(Args[2]);
	if (Start <= 0) Start += List->Length + 1;
	if (End <= 0) End += List->Length + 1;
	if (Start <= 0 || End < Start || End > List->Length + 1) return MLNil;
	ml_list_slice_t *Slice = new(ml_list_slice_t);
	Slice->Type = MLListSliceT;
	Slice->Head = ml_list_index(List, Start);
	Slice->Length = End - Start;
	return (ml_value_t *)Slice;
}

#ifdef ML_MUTABLES

ML_METHOD("[]", MLListT, MLIntegerT, MLIntegerT) {
//!internal
	ml_list_t *List = (ml_list_t *)Args[0];
	int Start = ml_integer_value_fast(Args[1]);
	int End = ml_integer_value_fast(Args[2]);
	if (Start <= 0) Start += List->Length + 1;
	if (End <= 0) End += List->Length + 1;
	if (Start <= 0 || End < Start || End > List->Length + 1) return MLNil;
	ml_list_slice_t *Slice = new(ml_list_slice_t);
	Slice->Type = MLListSliceT;
	Slice->Head = ml_list_index(List, Start);
	Slice->Length = End - Start;
	return ml_deref((ml_value_t *)Slice);
}

#endif

ML_METHOD("[]", MLListMutableT, MLIntegerRangeT) {
//<List
//<Range
//>list::slice
// Returns a slice of :mini:`List` starting at :mini:`Range:start` and ending at :mini:`Range:limit`, both inclusive.
// Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[1];
	int Start = Range->Start, End = Range->Limit + 1, Step = Range->Step;
	if (Step != 1) return ml_error("ValueError", "Invalid step size for list slice");
	if (Start <= 0) Start += List->Length + 1;
	if (End <= 0) End += List->Length + 1;
	if (Start <= 0 || End < Start || End > List->Length + 1) return MLNil;
	ml_list_slice_t *Slice = new(ml_list_slice_t);
	Slice->Type = MLListSliceT;
	Slice->Head = ml_list_index(List, Start);
	Slice->Length = End - Start;
	return (ml_value_t *)Slice;
}

#ifdef ML_MUTABLES

ML_METHOD("[]", MLListT, MLIntegerRangeT) {
//!internal
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[1];
	int Start = Range->Start, End = Range->Limit + 1, Step = Range->Step;
	if (Step != 1) return ml_error("ValueError", "Invalid step size for list slice");
	if (Start <= 0) Start += List->Length + 1;
	if (End <= 0) End += List->Length + 1;
	if (Start <= 0 || End < Start || End > List->Length + 1) return MLNil;
	ml_list_slice_t *Slice = new(ml_list_slice_t);
	Slice->Type = MLListSliceT;
	Slice->Head = ml_list_index(List, Start);
	Slice->Length = End - Start;
	return ml_deref((ml_value_t *)Slice);
}

#endif

ML_METHOD("[]", MLListT, MLListMutableT) {
//<List
//<Indices
//>list
// Returns a list containing the :mini:`List[Indices[1]]`, :mini:`List[Indices[2]]`, etc.
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_value_t *Result = ml_list();
	ML_LIST_FOREACH(Args[1], Iter) {
		int Index = ml_integer_value(Iter->Value);
		ml_list_node_t *Node = ml_list_index(List, Index);
		ml_list_put(Result, Node ? Node->Value : MLNil);
	}
	return Result;
}

ML_METHOD("append", MLStringBufferT, MLListT) {
//<Buffer
//<List
// Appends a representation of :mini:`List` to :mini:`Buffer` of the form :mini:`"[" + repr(V/1) + ", " + repr(V/2) + ", " + ... + repr(V/n) + "]"`, where :mini:`repr(V/i)` is a representation of the *i*-th element (using :mini:`:append`).
//$- let B := string::buffer()
//$- B:append([1, 2, 3, 4])
//$= B:rest
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_put(Buffer, '[');
	ml_list_t *List = (ml_list_t *)Args[1];
	ml_list_node_t *Node = List->Head;
	if (Node) {
		ml_stringbuffer_simple_append(Buffer, Node->Value);
		while ((Node = Node->Next)) {
			ml_stringbuffer_write(Buffer, ", ", 2);
			ml_stringbuffer_simple_append(Buffer, Node->Value);
		}
	}
	ml_stringbuffer_put(Buffer, ']');
	return (ml_value_t *)Buffer;
}

ML_METHOD("append", MLStringBufferT, MLListT, MLStringT) {
//<Buffer
//<List
//<Sep
// Appends a representation of :mini:`List` to :mini:`Buffer` of the form :mini:`repr(V/1) + Sep + repr(V/2) + Sep + ... + repr(V/n)`, where :mini:`repr(V/i)` is a representation of the *i*-th element (using :mini:`:append`).
//$- let B := string::buffer()
//$- B:append([1, 2, 3, 4], " - ")
//$= B:rest
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	const char *Seperator = ml_string_value(Args[2]);
	size_t SeperatorLength = ml_string_length(Args[2]);
	ml_list_t *List = (ml_list_t *)Args[1];
	ml_list_node_t *Node = List->Head;
	if (Node) {
		ml_stringbuffer_simple_append(Buffer, Node->Value);
		while ((Node = Node->Next)) {
			ml_stringbuffer_write(Buffer, Seperator, SeperatorLength);
			ml_stringbuffer_simple_append(Buffer, Node->Value);
		}
	}
	return (ml_value_t *)Buffer;
}

static ml_value_t *ML_TYPED_FN(ml_unpack, MLListT, ml_list_t *List, int Index) {
	return (ml_value_t *)ml_list_index(List, Index) ?: MLNil;
}

/*typedef struct ml_list_iterator_t {
	ml_type_t *Type;
	ml_list_node_t *Node;
	long Index;
} ml_list_iterator_t;

static void ML_TYPED_FN(ml_iter_value, MLListIterT, ml_state_t *Caller, ml_list_iterator_t *Iter) {
	ML_RETURN(Iter->Node);
}

static void ML_TYPED_FN(ml_iter_next, MLListIterT, ml_state_t *Caller, ml_list_iterator_t *Iter) {
	if ((Iter->Node = Iter->Node->Next)) {
		++Iter->Index;
		ML_RETURN(Iter);
	} else {
		ML_RETURN(MLNil);
	}
}

static void ML_TYPED_FN(ml_iter_key, MLListIterT, ml_state_t *Caller, ml_list_iterator_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

ML_TYPE(MLListIterT, (), "list-iterator");
//!internal
*/

static void ML_TYPED_FN(ml_iterate, MLListT, ml_state_t *Caller, ml_list_t *List) {
	if (List->Length) {
		List->Head->Index = 1;
		ML_RETURN(List->Head);
	} else {
		ML_RETURN(MLNil);
	}
}

ML_METHODV("push", MLListMutableT) {
//<List
//<Values...: any
//>list
// Pushes :mini:`Values` onto the start of :mini:`List` and returns :mini:`List`.
	ml_value_t *List = Args[0];
	for (int I = 1; I < Count; ++I) ml_list_push(List, Args[I]);
	return Args[0];
}

ML_METHODV("put", MLListMutableT) {
//<List
//<Values...: any
//>list
// Pushes :mini:`Values` onto the end of :mini:`List` and returns :mini:`List`.
	ml_value_t *List = Args[0];
	for (int I = 1; I < Count; ++I) ml_list_put(List, Args[I]);
	return Args[0];
}

ML_METHOD("pop", MLListMutableT) {
//<List
//>any | nil
// Removes and returns the first element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.
	return ml_list_pop(Args[0]) ?: MLNil;
}

ML_METHOD("pull", MLListMutableT) {
//<List
//>any | nil
// Removes and returns the last element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.
	return ml_list_pull(Args[0]) ?: MLNil;
}

ML_METHOD("empty", MLListMutableT) {
//<List
//>list
// Removes all elements from :mini:`List` and returns it.
	ml_list_t *List = (ml_list_t *)Args[0];
	List->Head = List->Tail = NULL;
	List->Length = 0;
#ifdef ML_GENERICS
	List->Type = MLListMutableT;
#endif
	return (ml_value_t *)List;
}

ML_METHOD("+", MLListT, MLListT) {
//<List/1
//<List/2
//>list
// Returns a new list with the elements of :mini:`List/1` followed by the elements of :mini:`List/2`.
	ml_value_t *List = ml_list();
	ML_LIST_FOREACH(Args[0], Iter) ml_list_put(List, Iter->Value);
	ML_LIST_FOREACH(Args[1], Iter) ml_list_put(List, Iter->Value);
	return List;
}

ML_METHOD("splice", MLListMutableT) {
//<List
//>list | nil
// Removes all elements from :mini:`List`. Returns the removed elements as a new list.
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_t *Removed = (ml_list_t *)ml_list();
	*Removed = *List;
	List->Head = List->Tail = NULL;
	List->Length = 0;
	return (ml_value_t *)Removed;
}

ML_METHOD("splice", MLListMutableT, MLIntegerT, MLIntegerT) {
//<List
//<Index
//<Count
//>list | nil
// Removes :mini:`Count` elements from :mini:`List` starting at :mini:`Index`. Returns the removed elements as a new list.
	ml_list_t *List = (ml_list_t *)Args[0];
	int Start = ml_integer_value_fast(Args[1]);
	if (Start <= 0) Start += List->Length + 1;
	if (Start <= 0) return MLNil;
	int Remove = ml_integer_value_fast(Args[2]);
	if (Remove < 0) return MLNil;
	ml_list_t *Removed = (ml_list_t *)ml_list();
	if (Remove == 0) {
		if (Start > List->Length + 1) return MLNil;
	} else {
		int End = Start + Remove - 1;
		if (End > List->Length) return MLNil;
		if (Start == 1) {
			if (End == List->Length) {
				*Removed = *List;
				List->Head = List->Tail = NULL;
				List->Length = 0;
			} else {
				ml_list_node_t *EndNode = ml_list_index(List, End);
				ml_list_node_t *NextNode = EndNode->Next;
				EndNode->Next = NULL;
				Removed->CachedNode = Removed->Head = List->Head;
				Removed->Tail = EndNode;
				Removed->Length = Remove;
				List->Head = NextNode;
				NextNode->Prev = NULL;
				List->CachedNode = List->Head;
				List->CachedIndex = 1;
				List->Length -= Remove;
			}
		} else {
			ml_list_node_t *StartNode = ml_list_index(List, Start);
			ml_list_node_t *PrevNode = StartNode->Prev;
			StartNode->Prev = NULL;
			if (End == List->Length) {
				Removed->CachedNode = Removed->Head = StartNode;
				Removed->Tail = List->Tail;
				Removed->Length = Remove;
				List->Tail = PrevNode;
				PrevNode->Next = NULL;
				List->CachedNode = List->Head;
				List->CachedIndex = 1;
				List->Length -= Remove;
			} else {
				ml_list_node_t *EndNode = ml_list_index(List, End);
				ml_list_node_t *NextNode = EndNode->Next;
				EndNode->Next = NULL;
				Removed->CachedNode = Removed->Head = StartNode;
				Removed->Tail = EndNode;
				Removed->Length = Remove;
				NextNode->Prev = PrevNode;
				PrevNode->Next = NextNode;
				List->CachedNode = List->Head;
				List->CachedIndex = 1;
				List->Length -= Remove;
			}
		}
	}
	return (ml_value_t *)Removed;
}

ML_METHOD("splice", MLListMutableT, MLIntegerT, MLIntegerT, MLListMutableT) {
//<List
//<Index
//<Count
//<Source
//>list | nil
// Removes :mini:`Count` elements from :mini:`List` starting at :mini:`Index`, then inserts the elements from :mini:`Source`, leaving :mini:`Source` empty. Returns the removed elements as a new list.
	ml_list_t *List = (ml_list_t *)Args[0];
	int Start = ml_integer_value_fast(Args[1]);
	if (Start <= 0) Start += List->Length + 1;
	if (Start <= 0) return MLNil;
	int Remove = ml_integer_value_fast(Args[2]);
	if (Remove < 0) return MLNil;
	ml_list_t *Source = (ml_list_t *)Args[3];
	ml_list_t *Removed = (ml_list_t *)ml_list();
	if (Remove == 0) {
		if (Start > List->Length + 1) return MLNil;
		if (!Source->Length) return (ml_value_t *)Removed;
		if (Start == 1) {
			Source->Tail->Next = List->Head;
			if (List->Head) {
				List->Head->Prev = Source->Tail;
			} else {
				List->Tail = Source->Tail;
			}
			List->Head = Source->Head;
		} else if (Start == List->Length + 1) {
			Source->Head->Prev = List->Tail;
			if (List->Tail) {
				List->Tail->Next = Source->Head;
			} else {
				List->Head = Source->Head;
			}
			List->Tail = Source->Tail;
		} else {
			ml_list_node_t *StartNode = ml_list_index(List, Start);
			ml_list_node_t *PrevNode = StartNode->Prev;
			PrevNode->Next = Source->Head;
			Source->Head->Prev = PrevNode;
			StartNode->Prev = Source->Tail;
			Source->Tail->Next = StartNode;
		}
		List->CachedNode = List->Head;
		List->CachedIndex = 1;
	} else {
		int End = Start + Remove - 1;
		if (End > List->Length) return MLNil;
		if (Start == 1) {
			if (End == List->Length) {
				*Removed = *List;
				*List = *Source;
			} else {
				ml_list_node_t *EndNode = ml_list_index(List, End);
				ml_list_node_t *NextNode = EndNode->Next;
				EndNode->Next = NULL;
				Removed->CachedNode = Removed->Head = List->Head;
				Removed->Tail = EndNode;
				Removed->Length = Remove;
				if (Source->Length) {
					List->Head = Source->Head;
					Source->Tail->Next = NextNode;
					NextNode->Prev = Source->Tail;
				} else {
					List->Head = NextNode;
					NextNode->Prev = NULL;
				}
				List->CachedNode = List->Head;
				List->CachedIndex = 1;
				List->Length -= Remove;
			}
		} else {
			ml_list_node_t *StartNode = ml_list_index(List, Start);
			ml_list_node_t *PrevNode = StartNode->Prev;
			StartNode->Prev = NULL;
			if (End == List->Length) {
				Removed->CachedNode = Removed->Head = StartNode;
				Removed->Tail = List->Tail;
				Removed->Length = Remove;
				if (Source->Length) {
					List->Tail = Source->Tail;
					Source->Head->Prev = PrevNode;
					PrevNode->Next = Source->Head;
				} else {
					List->Tail = PrevNode;
					PrevNode->Next = NULL;
				}
				List->CachedNode = List->Head;
				List->CachedIndex = 1;
				List->Length -= Remove;
			} else {
				ml_list_node_t *EndNode = ml_list_index(List, End);
				ml_list_node_t *NextNode = EndNode->Next;
				EndNode->Next = NULL;
				Removed->CachedNode = Removed->Head = StartNode;
				Removed->Tail = EndNode;
				Removed->Length = Remove;
				if (Source->Length) {
					Source->Tail->Next = NextNode;
					NextNode->Prev = Source->Tail;
					Source->Head->Prev = PrevNode;
					PrevNode->Next = Source->Head;
				} else {
					NextNode->Prev = PrevNode;
					PrevNode->Next = NextNode;
				}
				List->CachedNode = List->Head;
				List->CachedIndex = 1;
				List->Length -= Remove;
			}
		}
	}
	List->Length += Source->Length;
	Source->Head = Source->Tail = NULL;
	Source->Length = 0;
	return (ml_value_t *)Removed;
}

ML_METHOD("splice", MLListMutableT, MLIntegerT, MLListMutableT) {
//<List
//<Index
//<Source
//>nil
// Inserts the elements from :mini:`Source` into :mini:`List` starting at :mini:`Index`, leaving :mini:`Source` empty.
	ml_list_t *List = (ml_list_t *)Args[0];
	int Start = ml_integer_value_fast(Args[1]);
	if (Start <= 0) Start += List->Length + 1;
	if (Start <= 0) return MLNil;
	ml_list_t *Source = (ml_list_t *)Args[2];
	if (Start > List->Length + 1) return MLNil;
	if (!Source->Length) return MLNil;
	if (Start == 1) {
		Source->Tail->Next = List->Head;
		if (List->Head) {
			List->Head->Prev = Source->Tail;
		} else {
			List->Tail = Source->Tail;
		}
		List->Head = Source->Head;
	} else if (Start == List->Length + 1) {
		Source->Head->Prev = List->Tail;
		if (List->Tail) {
			List->Tail->Next = Source->Head;
		} else {
			List->Head = Source->Head;
		}
		List->Tail = Source->Tail;
	} else {
		ml_list_node_t *StartNode = ml_list_index(List, Start);
		ml_list_node_t *PrevNode = StartNode->Prev;
		PrevNode->Next = Source->Head;
		Source->Head->Prev = PrevNode;
		StartNode->Prev = Source->Tail;
		Source->Tail->Next = StartNode;
	}
	List->CachedNode = List->Head;
	List->CachedIndex = 1;
	List->Length += Source->Length;
	Source->Head = Source->Tail = NULL;
	Source->Length = 0;
	return MLNil;
}

ML_METHOD("reverse", MLListMutableT) {
//<List
//>list
// Reverses :mini:`List` in-place and returns it.
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_node_t *Prev = List->Head;
	if (!Prev) return (ml_value_t *)List;
	List->Tail = Prev;
	ml_list_node_t *Node = Prev->Next;
	Prev->Next = NULL;
	while (Node) {
		ml_list_node_t *Next = Node->Next;
		Node->Next = Prev;
		Prev->Prev = Node;
		Prev = Node;
		Node = Next;
	}
	Prev->Prev = NULL;
	List->Head = Prev;
	List->CachedIndex = 1;
	List->CachedNode = Prev;
	return (ml_value_t *)List;
}

typedef struct {
	ml_state_t Base;
	ml_list_t *List;
	ml_value_t *Compare;
	ml_value_t *Args[2];
	ml_list_node_t *Head, *Tail;
	ml_list_node_t *P, *Q;
	int Length;
	int InSize, NMerges;
	int PSize, QSize;
} ml_list_sort_state_t;

static void ml_list_sort_state_run(ml_list_sort_state_t *State, ml_value_t *Result) {
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
				ml_list_node_t *E;
				if (State->PSize == 0) {
					E = State->Q; State->Q = State->Q->Next; State->QSize--;
				} else if (State->QSize == 0 || !State->Q) {
					E = State->P; State->P = State->P->Next; State->PSize--;
				} else {
					State->Args[0] = State->P->Value;
					State->Args[1] = State->Q->Value;
					return ml_call((ml_state_t *)State, State->Compare, 2, State->Args);
				resume:
					if (ml_is_error(Result)) {
						ml_list_node_t *Node = State->P, *Next;
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
			Result = (ml_value_t *)State->List;
			goto finished;
		}
		State->InSize *= 2;
	}
finished:
	State->List->Head = State->Head;
	State->List->Tail = State->Tail;
	State->List->CachedIndex = 1;
	State->List->CachedNode = State->Head;
	State->List->Length = State->Length;
	ML_CONTINUE(State->Base.Caller, Result);
}

extern ml_value_t *LessMethod;

ML_METHODX("sort", MLListMutableT) {
//<List
//>List
// Sorts :mini:`List` in-place using :mini:`<` and returns it.
	if (!ml_list_length(Args[0])) ML_RETURN(Args[0]);
	ml_list_sort_state_t *State = new(ml_list_sort_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_list_sort_state_run;
	ml_list_t *List = (ml_list_t *)Args[0];
	State->List = List;
	State->Compare = LessMethod;
	State->Head = State->List->Head;
	State->Length = List->Length;
	State->InSize = 1;
	// TODO: Improve ml_list_sort_state_run so that List is still valid during sort
	List->CachedNode = NULL;
	List->Head = List->Tail = NULL;
	List->Length = 0;
	return ml_list_sort_state_run(State, NULL);
}

ML_METHODX("sort", MLListMutableT, MLFunctionT) {
//<List
//<Compare
//>List
// Sorts :mini:`List` in-place using :mini:`Compare` and returns it.
	if (!ml_list_length(Args[0])) ML_RETURN(Args[0]);
	ml_list_sort_state_t *State = new(ml_list_sort_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_list_sort_state_run;
	ml_list_t *List = (ml_list_t *)Args[0];
	State->List = List;
	State->Compare = Args[1];
	State->Head = List->Head;
	State->Length = List->Length;
	State->InSize = 1;
	// TODO: Improve ml_list_sort_state_run so that List is still valid during sort
	List->CachedNode = NULL;
	List->Head = List->Tail = NULL;
	List->Length = 0;
	return ml_list_sort_state_run(State, NULL);
}

typedef struct {
	ml_state_t Base;
	ml_list_node_t *Node;
	ml_value_t *Args[2];
	int Index;
} ml_list_find_state_t;

extern ml_value_t *EqualMethod;

static void ml_list_find_state_run(ml_list_find_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Caller);
	if (Value != MLNil) ML_RETURN(ml_integer(State->Index));
	ml_list_node_t *Node = State->Node->Next;
	if (!Node) ML_RETURN(MLNil);
	State->Node = Node;
	State->Args[1] = Node->Value;
	++State->Index;
	return ml_call(State, EqualMethod, 2, State->Args);
}

ML_METHODX("find", MLListT, MLAnyT) {
//<List
//<Value
//>integer|nil
// Returns the first position where :mini:`List[Position] = Value`.
	ml_list_node_t *Node = ((ml_list_t *)Args[0])->Head;
	if (!Node) ML_RETURN(MLNil);
	ml_list_find_state_t *State = new(ml_list_find_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_list_find_state_run;
	State->Args[0] = Args[1];
	State->Node = Node;
	State->Args[1] = Node->Value;
	State->Index = 1;
	return ml_call(State, EqualMethod, 2, State->Args);
}

static void ml_list_delete(ml_list_t *List, ml_list_node_t *Node) {
	ml_list_node_t *Prev = Node->Prev;
	ml_list_node_t *Next = Node->Next;
	if (Prev) Prev->Next = Next; else List->Head = Next;
	if (Next) Next->Prev = Prev; else List->Tail = Prev;
	List->CachedNode = List->Head;
	List->CachedIndex = 1;
	--List->Length;
}

ML_METHOD("delete", MLListMutableT, MLIntegerT) {
//<List
//<Index
//>any|nil
// Removes and returns the :mini:`Index`-th value from :mini:`List`.
//$= let L := list("cake")
//$= L:delete(2)
//$= L:delete(-1)
//$= L
	ml_list_t *List = (ml_list_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	ml_list_node_t *Node = ml_list_index(List, Index);
	if (Node) {
		ml_list_delete(List, Node);
		return Node->Value;
	} else {
		return MLNil;
	}
}

typedef struct {
	ml_state_t Base;
	ml_list_t *List;
	ml_list_node_t *Node;
	ml_value_t *Fn;
	ml_value_t *Args[1];
} ml_list_remove_state_t;

static void ml_list_pop_state_run(ml_list_remove_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_list_node_t *Node = State->Node;
	if (Value != MLNil) {
		ml_list_delete(State->List, Node);
		ML_CONTINUE(State->Base.Caller, Node->Value);
	}
	if ((Node = Node->Next)) {
		State->Node = Node;
		State->Args[0] = Node->Value;
		return ml_call(State, State->Fn, 1, State->Args);
	}
	ML_CONTINUE(State->Base.Caller, MLNil);
}

ML_METHODX("pop", MLListMutableT, MLFunctionT) {
//<List
//<Fn
//>any|nil
// Removes and returns the first value where :mini:`Fn(Value)` is not :mini:`nil`.
//$= let L := list(1 .. 10)
//$= L:pop(3 | _)
//$= L
	ml_list_node_t *Node = ((ml_list_t *)Args[0])->Head;
	if (!Node) ML_RETURN(MLNil);
	ml_list_remove_state_t *State = new(ml_list_remove_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_list_pop_state_run;
	State->Fn = Args[1];
	State->List = (ml_list_t *)Args[0];
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, State->Fn, 1, State->Args);
}

static void ml_list_pull_state_run(ml_list_remove_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_list_node_t *Node = State->Node;
	if (Value != MLNil) {
		ml_list_delete(State->List, Node);
		ML_CONTINUE(State->Base.Caller, Node->Value);
	}
	if ((Node = Node->Prev)) {
		State->Node = Node;
		State->Args[0] = Node->Value;
		return ml_call(State, State->Fn, 1, State->Args);
	}
	ML_CONTINUE(State->Base.Caller, MLNil);
}

ML_METHODX("pull", MLListMutableT, MLFunctionT) {
//<List
//<Fn
//>any|nil
// Removes and returns the last value where :mini:`Fn(Value)` is not :mini:`nil`.
//$= let L := list(1 .. 10)
//$= L:pull(3 | _)
//$= L
	ml_list_node_t *Node = ((ml_list_t *)Args[0])->Tail;
	if (!Node) ML_RETURN(MLNil);
	ml_list_remove_state_t *State = new(ml_list_remove_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_list_pull_state_run;
	State->Fn = Args[1];
	State->List = (ml_list_t *)Args[0];
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, State->Fn, 1, State->Args);
}

ML_METHOD("permute", MLListMutableT) {
//!list
//<List
//>list
// .. deprecated:: 2.7.0
//
//    Use :mini:`List:shuffle` instead.
//
// Permutes :mini:`List` in place.
	ml_list_t *List = (ml_list_t *)Args[0];
	int N = List->Length;
	if (N <= 1) return (ml_value_t *)List;
	ml_list_node_t *Nodes[N], *Node = List->Head;
	for (int I = 0; I < N; ++I, Node = Node->Next) Nodes[I] = Node;
	for (int I = N; --I > 0;) {
		int Divisor = RAND_MAX / (I + 1), J;
		do J = random() / Divisor; while (J > I);
		if (J != I) {
			ml_list_node_t *Old = Nodes[J];
			Nodes[J] = Nodes[I];
			Nodes[I] = Old;
		}
	}
	Node = List->Head = Nodes[0];
	List->CachedNode = Node;
	List->CachedIndex = 1;
	Node->Prev = NULL;
	for (int I = 1; I < N; ++I) {
		Node->Next = Nodes[I];
		Nodes[I]->Prev = Node;
		Node = Nodes[I];
	}
	Node->Next = NULL;
	List->Tail = Node;
	return (ml_value_t *)List;
}

ML_METHOD("shuffle", MLListMutableT) {
//!list
//<List
//>list
// Shuffles :mini:`List` in place.
	ml_list_t *List = (ml_list_t *)Args[0];
	int N = List->Length;
	if (N <= 1) return (ml_value_t *)List;
	ml_list_node_t *Nodes[N], *Node = List->Head;
	for (int I = 0; I < N; ++I, Node = Node->Next) Nodes[I] = Node;
	for (int I = N; --I > 0;) {
		int Divisor = RAND_MAX / (I + 1), J;
		do J = random() / Divisor; while (J > I);
		if (J != I) {
			ml_list_node_t *Old = Nodes[J];
			Nodes[J] = Nodes[I];
			Nodes[I] = Old;
		}
	}
	Node = List->Head = Nodes[0];
	List->CachedNode = Node;
	List->CachedIndex = 1;
	Node->Prev = NULL;
	for (int I = 1; I < N; ++I) {
		Node->Next = Nodes[I];
		Nodes[I]->Prev = Node;
		Node = Nodes[I];
	}
	Node->Next = NULL;
	List->Tail = Node;
	return (ml_value_t *)List;
}

ML_METHOD("cycle", MLListMutableT) {
//!list
//<List
//>list
// Permutes :mini:`List` in place with no sub-cycles.
	ml_list_t *List = (ml_list_t *)Args[0];
	int N = List->Length;
	if (N <= 1) return (ml_value_t *)List;
	ml_list_node_t *Nodes[N], *Node = List->Head;
	for (int I = 0; I < N; ++I, Node = Node->Next) Nodes[I] = Node;
	for (int I = N; --I > 0;) {
		int Divisor = RAND_MAX / I, J;
		do J = random() / Divisor; while (J >= I);
		ml_list_node_t *Old = Nodes[J];
		Nodes[J] = Nodes[I];
		Nodes[I] = Old;
	}
	Node = List->Head = Nodes[0];
	List->CachedNode = Node;
	List->CachedIndex = 1;
	Node->Prev = NULL;
	for (int I = 1; I < N; ++I) {
		Node->Next = Nodes[I];
		Nodes[I]->Prev = Node;
		Node = Nodes[I];
	}
	Node->Next = NULL;
	List->Tail = Node;
	return (ml_value_t *)List;
}

typedef struct {
	ml_type_t *Type;
	ml_list_t *List;
	ml_list_node_t **Nodes;
	int Index, Length;
	int P[];
} ml_permutations_t;

ML_TYPE(MLPermutationsT, (MLSequenceT), "list::permutations");
//!internal

static void ML_TYPED_FN(ml_iterate, MLPermutationsT, ml_state_t *Caller, ml_permutations_t *Permutations) {
	Permutations->Index = 1;
	ML_RETURN(Permutations);
}

static void ML_TYPED_FN(ml_iter_next, MLPermutationsT, ml_state_t *Caller, ml_permutations_t *Permutations) {
	int N = Permutations->Length;
	int *P = Permutations->P;
	int Index = 1;
	while (!P[Index]) {
		P[Index] = Index;
		++Index;
	}
	if (Index == N) ML_RETURN(MLNil);
	--P[Index];
	int J = Index % 2 ? P[Index] : 0;
	ml_list_node_t **Nodes = Permutations->Nodes;
	ml_list_node_t *Node = Nodes[Index];
	Nodes[Index] = Nodes[J];
	Nodes[J] = Node;
	ml_list_t *List = Permutations->List;
	Node = List->Head = Nodes[0];
	List->CachedNode = Node;
	List->CachedIndex = 1;
	Node->Prev = NULL;
	for (int I = 1; I < N; ++I) {
		Node->Next = Nodes[I];
		Nodes[I]->Prev = Node;
		Node = Nodes[I];
	}
	Node->Next = NULL;
	List->Tail = Node;
	List->Length = N;
	++Permutations->Index;
	ML_RETURN(Permutations);
}

static void ML_TYPED_FN(ml_iter_key, MLPermutationsT, ml_state_t *Caller, ml_permutations_t *Permutations) {
	ML_RETURN(ml_integer(Permutations->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLPermutationsT, ml_state_t *Caller, ml_permutations_t *Permutations) {
	ML_RETURN(Permutations->List);
}

ML_METHOD("permutations", MLListMutableT) {
//<List
//>sequence
// Returns a sequence of all permutations of :mini:`List`, performed in-place.
	ml_list_t *List = (ml_list_t *)Args[0];
	int N = List->Length;
	ml_permutations_t *Permutations = xnew(ml_permutations_t, N + 1, int);
	Permutations->Type = MLPermutationsT;
	Permutations->Length = N;
	ml_list_node_t **Nodes = Permutations->Nodes = anew(ml_list_node_t *, N);
	ml_list_node_t *Node = List->Head;
	for (int I = 0; I < N; ++I, Node = Node->Next) {
		Nodes[I] = Node;
		Permutations->P[I] = I;
	}
	Permutations->P[N] = N;
	Permutations->List = List;
	return (ml_value_t *)Permutations;
}

ML_METHOD("random", MLListT) {
//<List
//>any
// Returns a random (assignable) node from :mini:`List`.
//$= let L := list("cake")
//$= L:random
//$= L:random
	ml_list_t *List = (ml_list_t *)Args[0];
	int Limit = List->Length;
	if (Limit <= 0) return MLNil;
	int Divisor = RAND_MAX / Limit;
	int Random;
	do Random = random() / Divisor; while (Random >= Limit);
	return (ml_value_t *)ml_list_index(List, Random + 1);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Visitor, *Dest;
	ml_list_node_t *Node;
	ml_value_t *Args[1];
} ml_list_visit_t;

static void ml_list_visit_run(ml_list_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_list_node_t *Node = State->Node->Next;
	if (!Node) ML_RETURN(MLNil);
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, State->Visitor, 1, State->Args);
}

ML_METHODX("visit", MLVisitorT, MLListT) {
//<Visitor
//<List
//>list
// Returns a new list containing copies of the elements of :mini:`List` created using :mini:`Copy`.
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_list_node_t *Node = ((ml_list_t *)Args[1])->Head;
	if (!Node) ML_RETURN(MLNil);
	ml_list_visit_t *State = new(ml_list_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_list_visit_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

static void ml_list_copy_run(ml_list_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_list_put(State->Dest, Value);
	ml_list_node_t *Node = State->Node->Next;
	if (!Node) ML_RETURN(State->Dest);
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, State->Visitor, 1, State->Args);
}

ML_METHODX("copy", MLVisitorT, MLListT) {
//<Visitor
//<List
//>list
// Returns a new list containing copies of the elements of :mini:`List` created using :mini:`Copy`.
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_value_t *Dest = ml_list();
	inthash_insert(Visitor->Cache, (uintptr_t)Args[1], Dest);
	ml_list_node_t *Node = ((ml_list_t *)Args[1])->Head;
	if (!Node) ML_RETURN(Dest);
	ml_list_visit_t *State = new(ml_list_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_list_copy_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Dest = Dest;
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

static void ml_list_const_run(ml_list_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_list_put(State->Dest, Value);
	ml_list_node_t *Node = State->Node->Next;
	if (!Node) {
#ifdef ML_GENERICS
		if (State->Dest->Type->Type == MLTypeGenericT) {
			ml_type_t *TArgs[2];
			ml_find_generic_parent(State->Dest->Type, MLListMutableT, 2, TArgs);
			TArgs[0] = MLListT;
			State->Dest->Type = ml_generic_type(2, TArgs);
		} else {
#endif
			State->Dest->Type = MLListT;
#ifdef ML_GENERICS
		}
#endif
		ML_LIST_FOREACH(State->Dest, Iter) Iter->Type = MLListNodeT;
		ML_RETURN(State->Dest);
	}
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, State->Visitor, 1, State->Args);
}

ML_METHODX("const", MLVisitorT, MLListMutableT) {
//<Visitor
//<List
//>list::const
// Returns a new constant list containing copies of the elements of :mini:`List` created using :mini:`Copy`.
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_value_t *Dest = ml_list();
	inthash_insert(Visitor->Cache, (uintptr_t)Args[1], Dest);
	ml_list_node_t *Node = ((ml_list_t *)Args[1])->Head;
	if (!Node) ML_RETURN(Dest);
	ml_list_visit_t *State = new(ml_list_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_list_const_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Dest = Dest;
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

static int ML_TYPED_FN(ml_value_is_constant, MLListMutableT, ml_value_t *List) {
	return 0;
}

static int ML_TYPED_FN(ml_value_is_constant, MLListT, ml_value_t *List) {
	ML_LIST_FOREACH(List, Iter) {
		if (!ml_value_is_constant(Iter->Value)) return 0;
	}
	return 1;
}

ML_TYPE(MLNamesT, (), "names",
//!internal
);

ml_value_t *ml_names() {
	ml_value_t *Names = ml_list();
	Names->Type = MLNamesT;
	return Names;
}

void ml_names_add(ml_value_t *Names, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)Names;
	ml_list_node_t *Node = new(ml_list_node_t);
	Node->Type = MLListNodeMutableT;
	Node->Value = Value;
	if ((Node->Prev = List->Tail)) {
		List->Tail->Next = Node;
	} else {
		List->Head = Node;
	}
	List->CachedNode = List->Tail = Node;
	List->CachedIndex = ++List->Length;
}

ML_METHOD(MLListT, MLNamesT) {
	ml_value_t *List = ml_list();
	ML_NAMES_FOREACH(Args[0], Iter) ml_list_put(List, Iter->Value);
	return List;
}

void ml_list_init() {
#include "ml_list_init.c"
	stringmap_insert(MLListT->Exports, "mutable", MLListMutableT);
#ifdef ML_GENERICS
	ml_type_add_rule(MLListT, MLSequenceT, MLIntegerT, ML_TYPE_ARG(1), NULL);
#ifdef ML_MUTABLES
	ml_type_add_rule(MLListMutableT, MLListT, ML_TYPE_ARG(1), NULL);
#endif
#endif
}

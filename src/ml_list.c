#include "ml_list.h"
#include "minilang.h"
#include "ml_macros.h"
#include "ml_iterfns.h"
#include <string.h>

static ml_list_node_t *ml_list_index(ml_list_t *List, int Index) {
	int Length = List->Length;
	if (Index <= 0) Index += Length + 1;
	if (Index > Length) return NULL;
	if (Index == Length) return List->Tail;
	if (Index < 1) return NULL;
	if (Index == 1) return List->Head;
	int CachedIndex = List->CachedIndex;
	if (CachedIndex < 0) {
		CachedIndex = 0;
		List->CachedNode = List->Head;
	} else if (CachedIndex > Length) {

	}
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

ML_TYPE(MLListT, (MLIteratableT), "list",
// A list of elements.
);

static ml_value_t *ml_list_node_deref(ml_list_node_t *Node) {
	return Node->Value;
}

static ml_value_t *ml_list_node_assign(ml_list_node_t *Node, ml_value_t *Value) {
	return (Node->Value = Value);
}

ML_TYPE(MLListNodeT, (), "list-node",
// A node in a :mini:`list`.
// Dereferencing a :mini:`listnode` returns the corresponding value from the :mini:`list`.
// Assigning to a :mini:`listnode` updates the corresponding value in the :mini:`list`.
	.deref = (void *)ml_list_node_deref,
	.assign = (void *)ml_list_node_assign
);

static void ML_TYPED_FN(ml_iter_next, MLListNodeT, ml_state_t *Caller, ml_list_node_t *Node) {
	ML_RETURN((ml_value_t *)Node->Next ?: MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLListNodeT, ml_state_t *Caller, ml_list_node_t *Node) {
	ML_RETURN(ml_integer(Node->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLListNodeT, ml_state_t *Caller, ml_list_node_t *Node) {
	ML_RETURN(Node);
}

ml_value_t *ml_list() {
	ml_list_t *List = new(ml_list_t);
	List->Type = MLListT;
	List->Head = List->Tail = NULL;
	List->Length = 0;
	return (ml_value_t *)List;
}

ML_METHOD(MLListT) {
	return ml_list();
}

ML_METHOD(MLListT, MLTupleT) {
	ml_value_t *List = ml_list();
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	for (int I = 0; I < Tuple->Size; ++I) ml_list_put(List, Tuple->Values[I]);
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

ML_METHODVX(MLListT, MLIteratableT) {
//<Iteratable
//>list
// Returns a list of all of the values produced by :mini:`Iteratable`.
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)list_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_list();
	return ml_iterate((ml_state_t *)State, ml_chained(Count, Args));
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
	Node->Type = MLListNodeT;
	Node->Value = Value;
	List->ValidIndices = 0;
	if ((Node->Next = List->Head)) {
		List->Head->Prev = Node;
#ifdef ML_GENERICS
		if (List->Type->Type == MLGenericTypeT) {
			ml_type_t *Type = ml_generic_type_args(List->Type)[1];
			if (Type != ml_typeof(Value)) {
				ml_type_t *Type2 = ml_type_max(Type, ml_typeof(Value));
				if (Type != Type2) {
					ml_type_t *Types[] = {MLListT, Type2};
					List->Type = ml_generic_type(2, Types);
				}
			}
		}
#endif
	} else {
		List->Tail = Node;
#ifdef ML_GENERICS
		if (List->Type == MLListT) {
			ml_type_t *Types[] = {MLListT, ml_typeof(Value)};
			List->Type = ml_generic_type(2, Types);
		}
#endif
	}
	List->CachedNode = List->Head = Node;
	List->CachedIndex = 1;
	++List->Length;
}

void ml_list_put(ml_value_t *List0, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = new(ml_list_node_t);
	Node->Type = MLListNodeT;
	Node->Value = Value;
	List->ValidIndices = 0;
	if ((Node->Prev = List->Tail)) {
		List->Tail->Next = Node;
#ifdef ML_GENERICS
		if (List->Type->Type == MLGenericTypeT) {
			ml_type_t *Type = ml_generic_type_args(List->Type)[1];
			if (Type != ml_typeof(Value)) {
				ml_type_t *Type2 = ml_type_max(Type, ml_typeof(Value));
				if (Type != Type2) {
					ml_type_t *Types[] = {MLListT, Type2};
					List->Type = ml_generic_type(2, Types);
				}
			}
		}
#endif
	} else {
		List->Head = Node;
#ifdef ML_GENERICS
		if (List->Type == MLListT) {
			ml_type_t *Types[] = {MLListT, ml_typeof(Value)};
			List->Type = ml_generic_type(2, Types);
		}
#endif
	}
	List->CachedNode = List->Tail = Node;
	List->CachedIndex = ++List->Length;
}

ml_value_t *ml_list_pop(ml_value_t *List0) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = List->Head;
	List->ValidIndices = 0;
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
	List->ValidIndices = 0;
	if (Node) {
		if ((List->Tail = Node->Prev)) {
			List->Tail->Next = NULL;
		} else {
			List->Head = NULL;
		}
		List->CachedNode = List->Tail;
		List->CachedIndex = -List->Length;
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

ML_METHOD("count", MLListT) {
//<List
//>integer
// Returns the length of :mini:`List`
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_integer(List->Length);
}

ML_METHOD("length", MLListT) {
//<List
//>integer
// Returns the length of :mini:`List`
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_integer(List->Length);
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
	State->List->ValidIndices = 0;
	ML_CONTINUE(State->Base.Caller, State->Drop);
}

ML_METHODX("filter", MLListT, MLFunctionT) {
//<List
//<Filter
//>list
// Removes every :mini:`Value` from :mini:`List` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list.
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_filter_state_t *State = new(ml_list_filter_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_list_filter_state_run;
	State->List = List;
	ml_list_t *Drop = State->Drop = new(ml_list_t);
	Drop->Type = MLListT;
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
//>listnode | nil
// Returns the :mini:`Index`-th node in :mini:`List` or :mini:`nil` if :mini:`Index` is outside the range of :mini:`List`.
// Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.
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

static ml_value_t *ml_list_slice_assign(ml_list_slice_t *Slice, ml_value_t *Packed) {
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
	return Packed;
}

ML_TYPE(MLListSliceT, (), "list-slice",
// A slice of a list.
	.deref = (void *)ml_list_slice_deref,
	.assign = (void *)ml_list_slice_assign
);

ML_METHOD("[]", MLListT, MLIntegerT, MLIntegerT) {
//<List
//<From
//<To
//>listslice
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

ML_METHOD("append", MLStringBufferT, MLListT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_add(Buffer, "[", 1);
	ml_list_t *List = (ml_list_t *)Args[1];
	ml_list_node_t *Node = List->Head;
	if (Node) {
		ml_stringbuffer_append(Buffer, Node->Value);
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			ml_stringbuffer_append(Buffer, Node->Value);
		}
	}
	ml_stringbuffer_add(Buffer, "]", 1);
	return (ml_value_t *)Buffer;
}

ml_value_t *ML_TYPED_FN(ml_unpack, MLListT, ml_list_t *List, int Index) {
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
		if (!List->ValidIndices) {
			int I = 0;
			for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) {
				Node->Index = ++I;
			}
			List->ValidIndices = 1;
		}
		ML_RETURN(List->Head);
		/*ml_list_iterator_t *Iter = new(ml_list_iterator_t);
		Iter->Type = MLListIterT;
		Iter->Node = List->Head;
		Iter->Index = 1;
		ML_RETURN(Iter);*/
	} else {
		ML_RETURN(MLNil);
	}
}

ML_METHODV("push", MLListT) {
//<List
//<Values...: any
//>list
// Pushes :mini:`Values` onto the start of :mini:`List` and returns :mini:`List`.
	ml_value_t *List = Args[0];
	for (int I = 1; I < Count; ++I) ml_list_push(List, Args[I]);
	return Args[0];
}

ML_METHODV("put", MLListT) {
//<List
//<Values...: any
//>list
// Pushes :mini:`Values` onto the end of :mini:`List` and returns :mini:`List`.
	ml_value_t *List = Args[0];
	for (int I = 1; I < Count; ++I) ml_list_put(List, Args[I]);
	return Args[0];
}

ML_METHOD("pop", MLListT) {
//<List
//>any | nil
// Removes and returns the first element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.
	return ml_list_pop(Args[0]) ?: MLNil;
}

ML_METHOD("pull", MLListT) {
//<List
//>any | nil
// Removes and returns the last element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.
	return ml_list_pull(Args[0]) ?: MLNil;
}

ML_METHOD("copy", MLListT) {
//<List
//>list
// Returns a (shallow) copy of :mini:`List`.
	ml_value_t *List = ml_list();
	ML_LIST_FOREACH(Args[0], Iter) ml_list_put(List, Iter->Value);
	return List;
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

ML_METHOD(MLStringT, MLListT) {
//<List
//>string
// Returns a string containing the elements of :mini:`List` surrounded by :mini:`"["`, :mini:`"]"` and seperated by :mini:`", "`.
	ml_list_t *List = (ml_list_t *)Args[0];
	if (!List->Length) return ml_cstring("[]");
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Seperator = "[";
	int SeperatorLength = 1;
	ML_LIST_FOREACH(List, Node) {
		ml_stringbuffer_add(Buffer, Seperator, SeperatorLength);
		ml_value_t *Result = ml_stringbuffer_append(Buffer, Node->Value);
		if (ml_is_error(Result)) return Result;
		Seperator = ", ";
		SeperatorLength = 2;
	}
	ml_stringbuffer_add(Buffer, "]", 1);
	return ml_stringbuffer_value(Buffer);
}

ML_METHOD(MLStringT, MLListT, MLStringT) {
//<List
//<Seperator
//>string
// Returns a string containing the elements of :mini:`List` seperated by :mini:`Seperator`.
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Seperator = ml_string_value(Args[1]);
	size_t SeperatorLength = ml_string_length(Args[1]);
	ml_list_node_t *Node = List->Head;
	if (Node) {
		ml_value_t *Result = ml_stringbuffer_append(Buffer, Node->Value);
		if (ml_is_error(Result)) return Result;
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, Seperator, SeperatorLength);
			ml_value_t *Result = ml_stringbuffer_append(Buffer, Node->Value);
			if (ml_is_error(Result)) return Result;
		}
	}
	return ml_stringbuffer_value(Buffer);
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
			State->List->Head = State->Head;
			State->List->Tail = State->Tail;
			State->List->CachedIndex = 1;
			State->List->CachedNode = State->Head;
			State->List->Length = State->Length;
			break;
		}
		State->InSize *= 2;
	}
	ML_CONTINUE(State->Base.Caller, State->List);
}

extern ml_value_t *LessMethod;

ML_METHODX("sort", MLListT) {
//<List
//>List
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
	List->ValidIndices = 0;
	List->CachedNode = NULL;
	List->Head = List->Tail = NULL;
	List->Length = 0;
	return ml_list_sort_state_run(State, NULL);
}

ML_METHODX("sort", MLListT, MLFunctionT) {
//<List
//<Compare
//>List
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
	List->ValidIndices = 0;
	List->CachedNode = NULL;
	List->Head = List->Tail = NULL;
	List->Length = 0;
	return ml_list_sort_state_run(State, NULL);
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
	Node->Type = MLListNodeT;
	Node->Value = Value;
	List->ValidIndices = 0;
	if ((Node->Prev = List->Tail)) {
		List->Tail->Next = Node;
	} else {
		List->Head = Node;
	}
	List->CachedNode = List->Tail = Node;
	List->CachedIndex = ++List->Length;
}

void ml_list_init() {
#include "ml_list_init.c"
#ifdef ML_GENERICS
	ml_type_add_rule(MLListT, MLIteratableT, MLIntegerT, ML_TYPE_ARG(1), NULL);
#endif
}

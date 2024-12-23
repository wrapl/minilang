#include "ml_slice.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include "ml_sequence.h"

#undef ML_CATEGORY
#define ML_CATEGORY "slice"

ML_TYPE(MLSliceT, (MLSequenceT), "slice");
// A slice of elements.

#ifdef ML_MUTABLES
ML_TYPE(MLSliceMutableT, (MLSliceT), "slice::mutable");
#else
#define MLSliceMutableT MLSliceT
#endif

#ifdef ML_GENERICS

static void ml_slice_update_generic(ml_slice_t *Slice, ml_type_t *Type) {
	if (Slice->Type->Type != MLTypeGenericT) {
		Slice->Type = ml_generic_type(2, (ml_type_t *[]){Slice->Type, Type});
	} else {
		ml_type_t *BaseType = ml_generic_type_args(Slice->Type)[0];
		ml_type_t *ValueType = ml_generic_type_args(Slice->Type)[1];
		if (!ml_is_subtype(Type, ValueType)) {
			ml_type_t *ValueType2 = ml_type_max(ValueType, Type);
			if (ValueType != ValueType2) {
				Slice->Type = ml_generic_type(2, (ml_type_t *[]){BaseType, ValueType2});
			}
		}
	}
}

#endif

static void ML_TYPED_FN(ml_value_find_all, MLSliceT, ml_value_t *Value, void *Data, ml_value_find_fn RefFn) {
	if (!RefFn(Data, Value, 1)) return;
	ML_SLICE_FOREACH(Value, Iter) ml_value_find_all(Iter->Value, Data, RefFn);
}

typedef struct {
	ml_type_t *Type;
	ml_slice_t *Slice;
	size_t Index;
} ml_slice_index_t;

static ml_value_t *ml_slice_index_deref(ml_slice_index_t *Iter) {
	ml_slice_t *Slice = Iter->Slice;
	if (Iter->Index <= Slice->Length) {
		return Slice->Nodes[Slice->Offset + Iter->Index - 1].Value;
	} else {
		return MLNil;
	}
}

static void ml_slice_index_assign(ml_state_t *Caller, ml_slice_index_t *Iter, ml_value_t *Value) {
	ml_slice_t *Slice = Iter->Slice;
	if (Iter->Index <= Slice->Length) {
		Slice->Nodes[Slice->Offset + Iter->Index - 1].Value = Value;
		ML_RETURN(Value);
	} else {
		ML_RETURN(MLNil);
	}
}

ML_TYPE(MLSliceIndexT, (), "slice::index",
// An assignable reference to an index of a slice.
	.deref = (void *)ml_slice_index_deref,
	.assign = (void *)ml_slice_index_assign
);

static ml_slice_node_t Empty[1] = {{NULL}};

ml_value_t *ml_slice(size_t Capacity) {
	ml_slice_t *Slice = new(ml_slice_t);
	Slice->Type = MLSliceMutableT;
	if (Capacity) {
		Slice->Capacity = Capacity;
		Slice->Nodes = anew(ml_slice_node_t, Capacity + 1);
	} else {
		Slice->Nodes = Empty;
	}
	return (ml_value_t *)Slice;
}

ML_METHOD(MLSliceT) {
//>slice
// Returns an empty slice.
//$= slice()
	return ml_slice(0);
}

ML_METHOD(MLSliceT, MLTupleT) {
//<Tuple
//>slice
// Returns a slice containing the values in :mini:`Tuple`.
//$= slice((1, 2, 3))
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	ml_value_t *Slice = ml_slice(Tuple->Size);
	for (int I = 0; I < Tuple->Size; ++I) {
		ml_value_t *Value = Tuple->Values[I];
		ml_slice_put(Slice, ml_deref(Value));
	}
	return Slice;
}

static void slice_iterate(ml_iter_state_t *State, ml_value_t *Value);

static void slice_iter_value(ml_iter_state_t *State, ml_value_t *Value) {
	Value = ml_deref(Value);
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_slice_put(State->Values[0], Value);
	State->Base.run = (void *)slice_iterate;
	return ml_iter_next((ml_state_t *)State, State->Iter);
}

static void slice_iterate(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (void *)slice_iter_value;
	if (Value == MLNil) ML_CONTINUE(State->Base.Caller, State->Values[0]);
	return ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

static void slice_iterate_precount(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t *Sequence = State->Values[1];
	State->Values[0] = ml_slice(Value != MLNil ? ml_integer_value(Value) : 0);
	State->Base.run = (ml_state_fn)slice_iterate;
	return ml_iterate((ml_state_t *)State, Sequence);
}

static ML_METHOD_DECL(Precount, "precount");

ML_METHODVX(MLSliceT, MLSequenceT) {
//<Sequence
//>slice
// Returns a list of all of the values produced by :mini:`Sequence`.
//$= slice(1 .. 10)
	ml_iter_state_t *State = xnew(ml_iter_state_t, 2, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)slice_iterate_precount;
	State->Base.Context = Caller->Context;
	State->Values[0] = State->Values[1] = ml_chained(Count, Args);
	return ml_call(State, Precount, 1, State->Values);
}

void ml_slice_grow(ml_value_t *Slice0, int Count) {
	ml_slice_t *Slice = (ml_slice_t *)Slice0;
	size_t Offset = Slice->Offset;
	size_t Length = Slice->Length;
	size_t Capacity = Slice->Capacity + Count;
	ml_slice_node_t *Nodes = anew(ml_slice_node_t, Capacity + 1);
	memcpy(Nodes + Offset, Slice->Nodes + Offset, Length * sizeof(ml_slice_node_t));
	Slice->Nodes = Nodes;
	Slice->Capacity = Capacity;
}

static void slice_grow_precount(ml_iter_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t *Sequence = State->Values[2];
	size_t Precount = ml_integer_value(Value);
	if (Precount > 0) ml_slice_grow(State->Values[0], Precount);
	State->Base.run = (ml_state_fn)slice_iterate;
	return ml_iterate((ml_state_t *)State, Sequence);
}

ML_METHODVX("grow", MLSliceMutableT, MLSequenceT) {
//<Slice
//<Sequence
//>slice
// Pushes of all of the values produced by :mini:`Sequence` onto :mini:`List` and returns :mini:`List`.
//$- let L := slice([1, 2, 3])
//$= L:grow(4 .. 6)
	ml_iter_state_t *State = xnew(ml_iter_state_t, 3, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)slice_grow_precount;
	State->Base.Context = Caller->Context;
	State->Values[0] = Args[0];
	State->Values[1] = State->Values[2] = ml_chained(Count - 1, Args + 1);
	return ml_call(State, Precount, 1, State->Values + 1);
}

void ml_slice_put(ml_value_t *Slice0, ml_value_t *Value) {
	ml_slice_t *Slice = (ml_slice_t *)Slice0;
	size_t Offset = Slice->Offset;
	size_t Length = Slice->Length;
	size_t Capacity = Slice->Capacity;
	size_t Index = Offset + Length;
	if (Index < Capacity) {
		Slice->Nodes[Index].Value = Value;
	} else if (Offset) {
		memmove(Slice->Nodes, Slice->Nodes + Offset, Length * sizeof(ml_slice_node_t));
		Slice->Nodes[Length].Value = Value;
	} else {
		Capacity += (Capacity >> 2) + 4;
		ml_slice_node_t *Nodes = anew(ml_slice_node_t, Capacity + 1);
		memcpy(Nodes, Slice->Nodes, Length * sizeof(ml_slice_node_t));
		Slice->Nodes = Nodes;
		Slice->Capacity = Capacity;
		Slice->Nodes[Length].Value = Value;
	}
	Slice->Length = Length + 1;
#ifdef ML_GENERICS
	ml_slice_update_generic(Slice, ml_typeof(Value));
#endif
}

void ml_slice_push(ml_value_t *Slice0, ml_value_t *Value) {
	ml_slice_t *Slice = (ml_slice_t *)Slice0;
	size_t Offset = Slice->Offset;
	size_t Length = Slice->Length;
	size_t Capacity = Slice->Capacity;
	if (!Offset) {
		if (Length < Capacity) {
			Offset = Capacity - Length;
			memmove(Slice->Nodes + Offset, Slice->Nodes, Length * sizeof(ml_slice_node_t));
		} else {
			Capacity += (Capacity >> 2) + 4;
			Offset = Capacity - Length;
			ml_slice_node_t *Nodes = anew(ml_slice_node_t, Capacity + 1);
			memcpy(Nodes + Offset, Slice->Nodes, Length * sizeof(ml_slice_node_t));
			Slice->Nodes = Nodes;
			Slice->Capacity = Capacity;
		}
	}
	Slice->Offset = --Offset;
	Slice->Nodes[Offset].Value = Value;
	Slice->Length = Length + 1;
#ifdef ML_GENERICS
	ml_slice_update_generic(Slice, ml_typeof(Value));
#endif
}

void ml_slice_insert(ml_value_t *Slice0, int Index, ml_value_t *Value) {
	ml_slice_t *Slice = (ml_slice_t *)Slice0;
	size_t Offset = Slice->Offset;
	size_t Length = Slice->Length;
	size_t Capacity = Slice->Capacity;
	if (Length == Capacity) {
		Capacity += (Capacity >> 2) + 4;
		ml_slice_node_t *Nodes = anew(ml_slice_node_t, Capacity + 1);
		Offset = (Capacity - Length - 1) / 2;
		ml_slice_node_t *Next = mempcpy(Nodes + Offset, Slice->Nodes, (Index - 1) * sizeof(ml_slice_node_t));
		(Next++)->Value = Value;
		memcpy(Next, Slice->Nodes + (Index - 1), (Length - (Index - 1)) * sizeof(ml_slice_node_t));
		Slice->Nodes = Nodes;
		Slice->Capacity = Capacity;
		Slice->Offset = Offset;
	} else if (!Offset || (Index > Length / 2)) {
		ml_slice_node_t *Nodes = Slice->Nodes + Offset;
		memmove(Nodes + Index, Nodes + (Index - 1), (Length - (Index - 1)) * sizeof(ml_slice_node_t));
		Nodes[Index - 1].Value = Value;
	} else {
		Slice->Offset = --Offset;
		ml_slice_node_t *Nodes = Slice->Nodes + Offset;
		memmove(Nodes, Nodes + 1, (Index - 1) * sizeof(ml_slice_node_t));
		Nodes[Index - 1].Value = Value;
	}
	Slice->Length = Length + 1;
#ifdef ML_GENERICS
	ml_slice_update_generic(Slice, ml_typeof(Value));
#endif
}

ml_value_t *ml_slice_delete(ml_value_t *Slice0, int Index) {
	ml_slice_t *Slice = (ml_slice_t *)Slice0;
	size_t Offset = Slice->Offset;
	size_t Length = Slice->Length;
	if (Index <= 0 || Index > Length) return MLNil;
	ml_slice_node_t *Nodes = Slice->Nodes + Offset;
	ml_value_t *Value = Nodes[Index - 1].Value;
	if (Index > Length / 2) {
		memmove(Nodes + (Index - 1), Nodes + Index, (Length - Index) * sizeof(ml_slice_node_t));
		Nodes[Length - 1].Value = NULL;
	} else {
		Slice->Offset = Offset + 1;
		memmove(Nodes + 1, Nodes, (Index - 1) * sizeof(ml_slice_node_t));
		Nodes[0].Value = NULL;
	}
	Slice->Length = Length - 1;
	return Value;
}

ml_value_t *ml_slice_pop(ml_value_t *Slice0) {
	ml_slice_t *Slice = (ml_slice_t *)Slice0;
	size_t Length = Slice->Length;
	if (!Length) return MLNil;
	size_t Offset = Slice->Offset;
	ml_value_t *Value = Slice->Nodes[Offset].Value;
	Slice->Nodes[Offset].Value = NULL;
	Slice->Offset = Offset + 1;
	--Slice->Length;
	return Value;
}

ml_value_t *ml_slice_pull(ml_value_t *Slice0) {
	ml_slice_t *Slice = (ml_slice_t *)Slice0;
	size_t Length = Slice->Length;
	if (!Length) return MLNil;
	size_t Offset = Slice->Offset;
	size_t Index = Offset + Length - 1;
	ml_value_t *Value = Slice->Nodes[Index].Value;
	Slice->Nodes[Index].Value = NULL;
	--Slice->Length;
	return Value;
}

ml_value_t *ml_slice_get(ml_value_t *Slice0, int Index) {
	ml_slice_t *Slice = (ml_slice_t *)Slice0;
	if (Index <= 0) Index += Slice->Length + 1;
	if (Index <= 0 || Index > Slice->Length) return NULL;
	return Slice->Nodes[Slice->Offset + Index - 1].Value;
}

ml_value_t *ml_slice_set(ml_value_t *Slice0, int Index, ml_value_t *Value) {
	ml_slice_t *Slice = (ml_slice_t *)Slice0;
	if (Index <= 0) Index += Slice->Length + 1;
	if (Index <= 0 || Index > Slice->Length) return NULL;
	ml_value_t *Old = Slice->Nodes[Slice->Offset + Index].Value;
	Slice->Nodes[Slice->Offset + Index - 1].Value = Value;
	return Old;
}

ml_value_t *ml_slice_index(ml_value_t *Slice0, int Index) {
	ml_slice_t *Slice = (ml_slice_t *)Slice0;
	if (Index <= 0) Index += Slice->Length + 1;
	if (Index <= 0 || Index > Slice->Length) return MLNil;
	if (ml_is_subtype(Slice->Type, MLSliceMutableT)) {
		ml_slice_index_t *Iter = new(ml_slice_index_t);
		Iter->Type = MLSliceIndexT;
		Iter->Slice = Slice;
		Iter->Index = Index;
		return (ml_value_t *)Iter;
	} else {
		return Slice->Nodes[Slice->Offset + Index - 1].Value;
	}
}

int ml_slice_foreach(ml_value_t *Value, void *Data, int (*callback)(ml_value_t *, void *)) {
	ML_SLICE_FOREACH(Value, Node) if (callback(Node->Value, Data)) return 1;
	return 0;
}

ML_METHOD("precount", MLSliceT) {
//<Slice
//>integer
// Returns the length of :mini:`Slice`
//$= slice([1, 2, 3]):precount
	return ml_integer(((ml_slice_t *)Args[0])->Length);
}

ML_METHOD("count", MLSliceT) {
//<Slice
//>integer
// Returns the length of :mini:`Slice`
//$= slice([1, 2, 3]):count
	return ml_integer(((ml_slice_t *)Args[0])->Length);
}

ML_METHOD("length", MLSliceT) {
//<Slice
//>integer
// Returns the length of :mini:`Slice`
//$= slice([1, 2, 3]):length
	return ml_integer(((ml_slice_t *)Args[0])->Length);
}

ML_METHOD("capacity", MLSliceT) {
//<Slice
//>integer
// Returns the capacity of :mini:`Slice`
//$= slice([1, 2, 3]):capacity
	return ml_integer(((ml_slice_t *)Args[0])->Capacity);
}

ML_METHOD("offset", MLSliceT) {
//<Slice
//>integer
// Returns the offset of :mini:`Slice`
//$= slice([1, 2, 3]):offset
	return ml_integer(((ml_slice_t *)Args[0])->Offset);
}

ML_METHOD("first", MLSliceT) {
//<Slice
// Returns the first value in :mini:`Slice` or :mini:`nil` if :mini:`Slice` is empty.
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	if (!Slice->Length) return MLNil;
	ml_slice_index_t *Iter = new(ml_slice_index_t);
	Iter->Type = MLSliceIndexT;
	Iter->Slice = Slice;
	Iter->Index = 1;
	return (ml_value_t *)Iter;
}

ML_METHOD("first2", MLSliceT) {
//<Slice
// Returns the first index and value in :mini:`Slice` or :mini:`nil` if :mini:`Slice` is empty.
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	if (!Slice->Length) return MLNil;
	ml_slice_index_t *Iter = new(ml_slice_index_t);
	Iter->Type = MLSliceIndexT;
	Iter->Slice = Slice;
	Iter->Index = 1;
	return ml_tuplev(2, ml_integer(1), (ml_value_t *)Iter);
}

ML_METHOD("last", MLSliceT) {
//<Slice
// Returns the last value in :mini:`Slice` or :mini:`nil` if :mini:`Slice` is empty.
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	if (!Slice->Length) return MLNil;
	ml_slice_index_t *Iter = new(ml_slice_index_t);
	Iter->Type = MLSliceIndexT;
	Iter->Slice = Slice;
	Iter->Index = Slice->Length;
	return (ml_value_t *)Iter;
}

ML_METHOD("last2", MLSliceT) {
//<Slice
// Returns the last index and value in :mini:`Slice` or :mini:`nil` if :mini:`Slice` is empty.
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	if (!Slice->Length) return MLNil;
	ml_slice_index_t *Iter = new(ml_slice_index_t);
	Iter->Type = MLSliceIndexT;
	Iter->Slice = Slice;
	Iter->Index = Slice->Length;
	return ml_tuplev(2, ml_integer(Slice->Length), (ml_value_t *)Iter);
}

typedef struct {
	ml_state_t Base;
	ml_slice_t *Slice;
	ml_value_t *Keep, *Drop;
	ml_slice_node_t *Node;
	ml_value_t *Filter;
	ml_value_t *Args[1];
} ml_slice_filter_state_t;

static void ml_slice_filter_state_run(ml_slice_filter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	ml_slice_node_t *Node = State->Node;
	ml_slice_put(Result == MLNil ? State->Drop : State->Keep, Node->Value);
	++Node;
	if (!Node->Value) {
		*State->Slice = *(ml_slice_t *)State->Keep;
		ML_CONTINUE(State->Base.Caller, State->Drop);
	}
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, State->Filter, 1, State->Args);
}

ML_METHODX("filter", MLSliceMutableT, MLFunctionT) {
//<Slice
//<Filter
//>slice
// Removes every :mini:`Value` from :mini:`Slice` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list.
//$- let L := slice([1, 2, 3, 4, 5, 6])
//$= L:filter(2 | _)
//$= L
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	if (!Slice->Length) ML_RETURN(Slice);
	ml_slice_filter_state_t *State = new(ml_slice_filter_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_filter_state_run;
	State->Slice = Slice;
	State->Node = Slice->Nodes + Slice->Offset;
	State->Keep = ml_slice(0);
	State->Drop = ml_slice(0);
	State->Filter = Args[1];
	State->Args[0] = State->Node->Value;
	return ml_call(State, State->Filter, 1, State->Args);
}

static void ml_slice_remove_state_run(ml_slice_filter_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	ml_slice_node_t *Node = State->Node;
	ml_slice_put(Result == MLNil ? State->Keep : State->Drop, Node->Value);
	++Node;
	if (!Node->Value) {
		*State->Slice = *(ml_slice_t *)State->Keep;
		ML_CONTINUE(State->Base.Caller, State->Drop);
	}
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, State->Filter, 1, State->Args);
}

ML_METHODX("remove", MLSliceMutableT, MLFunctionT) {
//<Slice
//<Filter
//>slice
// Removes every :mini:`Value` from :mini:`Slice` for which :mini:`Function(Value)` returns non-:mini:`nil` and returns those values in a new list.
//$- let L := slice([1, 2, 3, 4, 5, 6])
//$= L:remove(2 | _)
//$= L
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	if (!Slice->Length) ML_RETURN(Slice);
	ml_slice_filter_state_t *State = new(ml_slice_filter_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_remove_state_run;
	State->Slice = Slice;
	State->Node = Slice->Nodes + Slice->Offset;
	State->Keep = ml_slice(0);
	State->Drop = ml_slice(0);
	State->Filter = Args[1];
	State->Args[0] = State->Node->Value;
	return ml_call(State, State->Filter, 1, State->Args);
}

#ifdef ML_MUTABLES

ML_METHOD("[]", MLSliceT, MLIntegerT) {
//<Slice
//<Index
//>slice::index | nil
// Returns the :mini:`Index`-th node in :mini:`Slice` or :mini:`nil` if :mini:`Index` is outside the interval of :mini:`List`.
// Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.
//$- let L := slice(["a", "b", "c", "d", "e", "f"])
//$= L[3]
//$= L[-2]
//$= L[8]
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index <= 0) Index += Slice->Length + 1;
	if (Index <= 0) return MLNil;
	if (Index > Slice->Length) return MLNil;
	return Slice->Nodes[Slice->Offset + Index - 1].Value;
}

static ml_value_t *ml_slice_slice_copy(ml_slice_t *Source, int Start, int End) {
	if (Start <= 0) Start += Source->Length + 1;
	if (End <= 0) End += Source->Length + 1;
	if (Start <= 0 || End < Start || End > Source->Length + 1) return MLNil;
	size_t Length = End - Start;
	ml_slice_t *Slice = (ml_slice_t *)ml_slice(Length);
	memcpy(Slice->Nodes, Source->Nodes + Source->Offset + Start - 1, Length * sizeof(ml_slice_node_t));
	Slice->Length = Length;
	return (ml_value_t *)Slice;
}

ML_METHOD("[]", MLSliceT, MLIntegerT, MLIntegerT) {
//!internal
	ml_slice_t *Source = (ml_slice_t *)Args[0];
	int Start = ml_integer_value_fast(Args[1]);
	int End = ml_integer_value_fast(Args[2]);
	return ml_slice_slice_copy(Source, Start, End);
}

ML_METHOD("[]", MLSliceT, MLIntegerRangeT) {
//!internal
	ml_slice_t *Source = (ml_slice_t *)Args[0];
	ml_integer_range_t *Sequence = (ml_integer_range_t *)Args[1];
	int Start = Sequence->Start, End = Sequence->Limit + 1, Step = Sequence->Step;
	if (Step != 1) return ml_error("ValueError", "Invalid step size for list slice");
	return ml_slice_slice_copy(Source, Start, End);
}

ML_METHOD("[]", MLSliceT, MLIntegerIntervalT) {
//!internal
	ml_slice_t *Source = (ml_slice_t *)Args[0];
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Args[1];
	int Start = Interval->Start, End = Interval->Limit + 1, Step = 1;
	if (Step != 1) return ml_error("ValueError", "Invalid step size for list slice");
	return ml_slice_slice_copy(Source, Start, End);
}

#endif

ML_METHOD("[]", MLSliceMutableT, MLIntegerT) {
//!internal
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index <= 0) Index += Slice->Length + 1;
	if (Index <= 0) return MLNil;
	if (Index > Slice->Length) return MLNil;
	ml_slice_index_t *Iter = new(ml_slice_index_t);
	Iter->Type = MLSliceIndexT;
	Iter->Slice = Slice;
	Iter->Index = Index;
	return (ml_value_t *)Iter;
}

typedef struct {
	ml_type_t *Type;
	ml_slice_t *Source;
	size_t Start, Length;
} ml_slice_slice_t;

static ml_value_t *ml_slice_slice_deref(ml_slice_slice_t *Ref) {
	ml_slice_t *Source = Ref->Source;
	size_t Start = Ref->Start - 1;
	size_t Length = Ref->Length;
	if (Start + Length > Source->Length) return MLNil;
	ml_slice_t *Slice = (ml_slice_t *)ml_slice(Length);
	memcpy(Slice->Nodes, Source->Nodes + Source->Offset + Start, Length * sizeof(ml_slice_node_t));
	Slice->Length = Length;
	return (ml_value_t *)Slice;
}

static void ml_slice_slice_assign(ml_state_t *Caller, ml_slice_slice_t *Ref, ml_value_t *Packed) {
	ml_slice_t *Source = Ref->Source;
	size_t Start = Ref->Start - 1;
	size_t Length = Ref->Length;
	if (Length > Source->Length - Start) {
		Length = Source->Length - Start;
	}
	ml_slice_node_t *Node = Source->Nodes + Start;
	for (int Index = 1; Index <= Length; ++Index) {
		Node->Value = ml_unpack(Packed, Index);
		++Node;
	}
	ML_RETURN(Packed);
}

ML_TYPE(MLSliceSliceT, (), "slice::slice",
// A sub-slice.
	.deref = (void *)ml_slice_slice_deref,
	.assign = (void *)ml_slice_slice_assign
);

static ml_value_t *ml_slice_slice(ml_slice_t *Source, int Start, int End) {
	if (Start <= 0) Start += Source->Length + 1;
	if (End <= 0) End += Source->Length + 1;
	if (Start <= 0 || End < Start || End > Source->Length + 1) return MLNil;
	ml_slice_slice_t *Slice = new(ml_slice_slice_t);
	Slice->Type = MLSliceSliceT;
	Slice->Source = Source;
	Slice->Start = Start;
	Slice->Length = End - Start;
	return (ml_value_t *)Slice;
}

ML_METHOD("[]", MLSliceMutableT, MLIntegerT, MLIntegerT) {
//<Slice
//<Indices
//>slice
// Returns a slice containing the :mini:`List[Indices[1]]`, :mini:`List[Indices[2]]`, etc.
	ml_slice_t *Source = (ml_slice_t *)Args[0];
	int Start = ml_integer_value_fast(Args[1]);
	int End = ml_integer_value_fast(Args[2]);
	return ml_slice_slice(Source, Start, End);
}

ML_METHOD("[]", MLSliceMutableT, MLIntegerRangeT) {
//<Slice
//<Interval
//>slice::slice
// Returns a slice of :mini:`Slice` starting at :mini:`Interval:start` and ending at :mini:`Interval:limit`, both inclusive.
// Indexing starts at :mini:`1`. Negative indices are counted from the end of the slice, with :mini:`-1` returning the last node.
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	ml_integer_range_t *Sequence = (ml_integer_range_t *)Args[1];
	int Start = Sequence->Start, End = Sequence->Limit + 1, Step = Sequence->Step;
	if (Step != 1) return ml_error("ValueError", "Invalid step size for slice slice");
	return ml_slice_slice(Slice, Start, End);
}

ML_METHOD("[]", MLSliceMutableT, MLIntegerIntervalT) {
//<Slice
//<Interval
//>slice::slice
// Returns a slice of :mini:`Slice` starting at :mini:`Interval:start` and ending at :mini:`Interval:limit`, both inclusive.
// Indexing starts at :mini:`1`. Negative indices are counted from the end of the slice, with :mini:`-1` returning the last node.
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Args[1];
	int Start = Interval->Start, End = Interval->Limit + 1, Step = 1;
	if (Step != 1) return ml_error("ValueError", "Invalid step size for slice slice");
	return ml_slice_slice(Slice, Start, End);
}

extern ml_value_t *EqualMethod;
extern ml_value_t *LessMethod;
extern ml_value_t *GreaterMethod;
extern ml_value_t *NotEqualMethod;
extern ml_value_t *LessEqualMethod;
extern ml_value_t *GreaterEqualMethod;

typedef struct {
	ml_comparison_state_t Base;
	ml_value_t *Result, *Order, *Default;
	ml_slice_node_t *A, *B;
	ml_value_t *Args[2];
} ml_slice_compare_state_t;

static void ml_slice_compare_equal_run(ml_slice_compare_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (Result == MLNil) ML_RETURN(State->Result);
	ml_slice_node_t *A = ++State->A;
	ml_slice_node_t *B = ++State->B;
	if (!A->Value || !B->Value) ML_RETURN(State->Default);
	State->Args[0] = A->Value;
	State->Args[1] = B->Value;
	return ml_call(State, EqualMethod, 2, State->Args);
}

ML_METHODX("=", MLSliceT, MLSliceT) {
	ml_slice_t *A = (ml_slice_t *)Args[0];
	ml_slice_t *B = (ml_slice_t *)Args[1];
	for (ml_state_t *State = Caller; State && State->Type == MLComparisonStateT; State = State->Caller) {
		ml_comparison_state_t *Previous = (ml_comparison_state_t *)State;
		if (Previous->A == (ml_value_t *)A && Previous->B == (ml_value_t *)B) ML_RETURN(B);
	}
	if (A->Length != B->Length) ML_RETURN(MLNil);
	if (!A->Length) ML_RETURN(B);
	ml_slice_compare_state_t *State = new(ml_slice_compare_state_t);
	State->Base.Base.Type = MLComparisonStateT;
	State->Base.Base.Caller = Caller;
	State->Base.Base.Context = Caller->Context;
	State->Base.Base.run = (ml_state_fn)ml_slice_compare_equal_run;
	State->Base.A = (ml_value_t *)A;
	State->Base.B = (ml_value_t *)B;
	State->Result = MLNil;
	State->Default = (ml_value_t *)B;
	State->A = A->Nodes + A->Offset;
	State->B = B->Nodes + B->Offset;
	State->Args[0] = State->A->Value;
	State->Args[1] = State->B->Value;
	return ml_call(State, EqualMethod, 2, State->Args);
}

ML_METHODX("!=", MLSliceT, MLSliceT) {
	ml_slice_t *A = (ml_slice_t *)Args[0];
	ml_slice_t *B = (ml_slice_t *)Args[1];
	for (ml_state_t *State = Caller; State && State->Type == MLComparisonStateT; State = State->Caller) {
		ml_comparison_state_t *Previous = (ml_comparison_state_t *)State;
		if (Previous->A == (ml_value_t *)A && Previous->B == (ml_value_t *)B) ML_RETURN(MLNil);
	}
	if (A->Length != B->Length) ML_RETURN(B);
	if (!A->Length) ML_RETURN(MLNil);
	ml_slice_compare_state_t *State = new(ml_slice_compare_state_t);
	State->Base.Base.Type = MLComparisonStateT;
	State->Base.Base.Caller = Caller;
	State->Base.Base.Context = Caller->Context;
	State->Base.Base.run = (ml_state_fn)ml_slice_compare_equal_run;
	State->Base.A = (ml_value_t *)A;
	State->Base.B = (ml_value_t *)B;
	State->Result = (ml_value_t *)B;
	State->Default = MLNil;
	State->A = A->Nodes + A->Offset;
	State->B = B->Nodes + B->Offset;
	State->Args[0] = State->A->Value;
	State->Args[1] = State->B->Value;
	return ml_call(State, EqualMethod, 2, State->Args);
}

static void ml_slice_compare_order_run(ml_slice_compare_state_t *State, ml_value_t *Result);

static void ml_slice_compare_order2_run(ml_slice_compare_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (Result == MLNil) ML_RETURN(MLNil);
	ml_slice_node_t *A = ++State->A;
	ml_slice_node_t *B = ++State->B;
	if (!A->Value || !B->Value) ML_RETURN(State->Default);
	State->Args[0] = A->Value;
	State->Args[1] = B->Value;
	State->Base.Base.run = (ml_state_fn)ml_slice_compare_order_run;
	return ml_call(State, State->Order, 2, State->Args);
}

static void ml_slice_compare_order_run(ml_slice_compare_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (Result != MLNil) ML_RETURN(State->Result);
	State->Args[0] = State->A->Value;
	State->Args[1] = State->B->Value;
	State->Base.Base.run = (ml_state_fn)ml_slice_compare_order2_run;
	return ml_call(State, EqualMethod, 2, State->Args);
}

ML_METHODX("<", MLSliceT, MLSliceT) {
	ml_slice_t *A = (ml_slice_t *)Args[0];
	ml_slice_t *B = (ml_slice_t *)Args[1];
	for (ml_state_t *State = Caller; State && State->Type == MLComparisonStateT; State = State->Caller) {
		ml_comparison_state_t *Previous = (ml_comparison_state_t *)State;
		if (Previous->A == (ml_value_t *)A && Previous->B == (ml_value_t *)B) ML_RETURN(MLNil);
	}
	if (!A->Length) {
		if (!B->Length) ML_RETURN(MLNil);
		ML_RETURN(B);
	}
	if (!B->Length) ML_RETURN(MLNil);
	ml_slice_compare_state_t *State = new(ml_slice_compare_state_t);
	State->Base.Base.Type = MLComparisonStateT;
	State->Base.Base.Caller = Caller;
	State->Base.Base.Context = Caller->Context;
	State->Base.Base.run = (ml_state_fn)ml_slice_compare_order_run;
	State->Base.A = (ml_value_t *)A;
	State->Base.B = (ml_value_t *)B;
	State->Result = (ml_value_t *)B;
	State->Order = LessMethod;
	if (A->Length >= B->Length) {
		State->Default = MLNil;
	} else {
		State->Default = (ml_value_t *)B;
	}
	State->A = A->Nodes + A->Offset;
	State->B = B->Nodes + B->Offset;
	State->Args[0] = State->A->Value;
	State->Args[1] = State->B->Value;
	return ml_call(State, LessMethod, 2, State->Args);
}

ML_METHODX("<=", MLSliceT, MLSliceT) {
	ml_slice_t *A = (ml_slice_t *)Args[0];
	ml_slice_t *B = (ml_slice_t *)Args[1];
	for (ml_state_t *State = Caller; State && State->Type == MLComparisonStateT; State = State->Caller) {
		ml_comparison_state_t *Previous = (ml_comparison_state_t *)State;
		if (Previous->A == (ml_value_t *)A && Previous->B == (ml_value_t *)B) ML_RETURN(B);
	}
	if (!A->Length) {
		if (!B->Length) ML_RETURN(B);
		ML_RETURN(B);
	}
	if (!B->Length) ML_RETURN(MLNil);
	ml_slice_compare_state_t *State = new(ml_slice_compare_state_t);
	State->Base.Base.Type = MLComparisonStateT;
	State->Base.Base.Caller = Caller;
	State->Base.Base.Context = Caller->Context;
	State->Base.Base.run = (ml_state_fn)ml_slice_compare_order_run;
	State->Base.A = (ml_value_t *)A;
	State->Base.B = (ml_value_t *)B;
	State->Result = (ml_value_t *)B;
	State->Order = LessMethod;
	if (A->Length > B->Length) {
		State->Default = MLNil;
	} else {
		State->Default = (ml_value_t *)B;
	}
	State->A = A->Nodes + A->Offset;
	State->B = B->Nodes + B->Offset;
	State->Args[0] = State->A->Value;
	State->Args[1] = State->B->Value;
	return ml_call(State, LessMethod, 2, State->Args);
}

ML_METHODX(">", MLSliceT, MLSliceT) {
	ml_slice_t *A = (ml_slice_t *)Args[0];
	ml_slice_t *B = (ml_slice_t *)Args[1];
	for (ml_state_t *State = Caller; State && State->Type == MLComparisonStateT; State = State->Caller) {
		ml_comparison_state_t *Previous = (ml_comparison_state_t *)State;
		if (Previous->A == (ml_value_t *)A && Previous->B == (ml_value_t *)B) ML_RETURN(MLNil);
	}
	if (!A->Length) {
		if (!B->Length) ML_RETURN(MLNil);
		ML_RETURN(MLNil);
	}
	if (!B->Length) ML_RETURN(B);
	ml_slice_compare_state_t *State = new(ml_slice_compare_state_t);
	State->Base.Base.Type = MLComparisonStateT;
	State->Base.Base.Caller = Caller;
	State->Base.Base.Context = Caller->Context;
	State->Base.Base.run = (ml_state_fn)ml_slice_compare_order_run;
	State->Base.A = (ml_value_t *)A;
	State->Base.B = (ml_value_t *)B;
	State->Result = (ml_value_t *)B;
	State->Order = GreaterMethod;
	if (A->Length <= B->Length) {
		State->Default = MLNil;
	} else {
		State->Default = (ml_value_t *)B;
	}
	State->A = A->Nodes + A->Offset;
	State->B = B->Nodes + B->Offset;
	State->Args[0] = State->A->Value;
	State->Args[1] = State->B->Value;
	return ml_call(State, GreaterMethod, 2, State->Args);
}

ML_METHODX(">=", MLSliceT, MLSliceT) {
	ml_slice_t *A = (ml_slice_t *)Args[0];
	ml_slice_t *B = (ml_slice_t *)Args[1];
	for (ml_state_t *State = Caller; State && State->Type == MLComparisonStateT; State = State->Caller) {
		ml_comparison_state_t *Previous = (ml_comparison_state_t *)State;
		if (Previous->A == (ml_value_t *)A && Previous->B == (ml_value_t *)B) ML_RETURN(B);
	}
	if (!A->Length) {
		if (!B->Length) ML_RETURN(B);
		ML_RETURN(MLNil);
	}
	if (!B->Length) ML_RETURN(B);
	ml_slice_compare_state_t *State = new(ml_slice_compare_state_t);
	State->Base.Base.Type = MLComparisonStateT;
	State->Base.Base.Caller = Caller;
	State->Base.Base.Context = Caller->Context;
	State->Base.Base.run = (ml_state_fn)ml_slice_compare_order_run;
	State->Base.A = (ml_value_t *)A;
	State->Base.B = (ml_value_t *)B;
	State->Result = (ml_value_t *)B;
	State->Order = GreaterMethod;
	if (A->Length < B->Length) {
		State->Default = MLNil;
	} else {
		State->Default = (ml_value_t *)B;
	}
	State->A = A->Nodes + A->Offset;
	State->B = B->Nodes + B->Offset;
	State->Args[0] = State->A->Value;
	State->Args[1] = State->B->Value;
	return ml_call(State, GreaterMethod, 2, State->Args);
}

typedef struct {
	ml_state_t Base;
	ml_stringbuffer_t *Buffer;
	ml_slice_node_t *Node;
	ml_value_t *Args[2];
	ml_hash_chain_t Chain[1];
	const char *Seperator;
	const char *Terminator;
	size_t SeperatorLength;
	size_t TerminatorLength;
} ml_slice_append_state_t;

extern ml_value_t *AppendMethod;

static void ml_slice_append_state_run(ml_slice_append_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_slice_node_t *Node = ++State->Node;
	if (!Node->Value) {
		ml_stringbuffer_write(State->Buffer, State->Terminator, State->TerminatorLength);
		if (State->Chain->Index) ml_stringbuffer_printf(State->Buffer, "<%d", State->Chain->Index);
		State->Buffer->Chain = State->Chain->Previous;
		ML_CONTINUE(State->Base.Caller, MLSome);
	}
	ml_stringbuffer_write(State->Buffer, State->Seperator, State->SeperatorLength);
	State->Node = Node;
	State->Args[1] = Node->Value;
	return ml_call(State, AppendMethod, 2, State->Args);
}

ML_METHODX("append", MLStringBufferT, MLSliceT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_slice_t *Slice = (ml_slice_t *)Args[1];
	for (ml_hash_chain_t *Link = Buffer->Chain; Link; Link = Link->Previous) {
		if (Link->Value == (ml_value_t *)Slice) {
			int Index = Link->Index;
			if (!Index) Index = Link->Index = ++Buffer->Index;
			ml_stringbuffer_printf(Buffer, ">%d", Index);
			ML_RETURN(Buffer);
		}
	}
	if (!Slice->Length) {
		ml_stringbuffer_write(Buffer, "[]", 2);
		ML_RETURN(MLSome);
	}
	ml_slice_node_t *Node = Slice->Nodes + Slice->Offset;
	ml_stringbuffer_put(Buffer, '[');
	ml_slice_append_state_t *State = new(ml_slice_append_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_append_state_run;
	State->Chain->Previous = Buffer->Chain;
	State->Chain->Value = (ml_value_t *)Slice;
	Buffer->Chain = State->Chain;
	State->Buffer = Buffer;
	State->Node = Node;
	State->Seperator = ", ";
	State->SeperatorLength = 2;
	State->Terminator = "]";
	State->TerminatorLength = 1;
	State->Args[0] = (ml_value_t *)Buffer;
	State->Args[1] = Node->Value;
	return ml_call(State, AppendMethod, 2, State->Args);
}

ML_METHODX("append", MLStringBufferT, MLSliceT, MLStringT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_slice_t *Slice = (ml_slice_t *)Args[1];
	for (ml_hash_chain_t *Link = Buffer->Chain; Link; Link = Link->Previous) {
		if (Link->Value == (ml_value_t *)Slice) {
			int Index = Link->Index;
			if (!Index) Index = Link->Index = ++Buffer->Index;
			ml_stringbuffer_printf(Buffer, ">%d", Index);
			ML_RETURN(Buffer);
		}
	}
	if (!Slice->Length) ML_RETURN(MLNil);
	ml_slice_node_t *Node = Slice->Nodes + Slice->Offset;
	ml_slice_append_state_t *State = new(ml_slice_append_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_append_state_run;
	State->Chain->Previous = Buffer->Chain;
	State->Chain->Value = (ml_value_t *)Slice;
	Buffer->Chain = State->Chain;
	State->Buffer = Buffer;
	State->Node = Node;
	State->Seperator = ml_string_value(Args[2]);
	State->SeperatorLength = ml_string_length(Args[2]);
	State->Args[0] = (ml_value_t *)Buffer;
	State->Args[1] = Node->Value;
	return ml_call(State, AppendMethod, 2, State->Args);
}

typedef struct {
	ml_type_t *Type;
	ml_slice_t *Slice;
	size_t Index;
} ml_slice_iter_t;

ML_TYPE(MLSliceIterT, (), "slice::iter");

#ifdef ML_MUTABLES
ML_TYPE(MLSliceMutableIterT, (MLSliceIterT), "slice::mutable::iter");
#else
#define MLSliceMutableIterT MLSliceIterT
#endif

static void ML_TYPED_FN(ml_iter_next, MLSliceIterT, ml_state_t *Caller, ml_slice_index_t *Iter) {
	ml_slice_t *Slice = Iter->Slice;
	if (Iter->Index >= Slice->Length) ML_RETURN(MLNil);
	++Iter->Index;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLSliceIterT, ml_state_t *Caller, ml_slice_index_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

#ifdef ML_MUTABLES

static void ML_TYPED_FN(ml_iter_value, MLSliceIterT, ml_state_t *Caller, ml_slice_index_t *Iter) {
	ml_slice_t *Slice = Iter->Slice;
	if (Iter->Index <= Slice->Length) {
		ML_RETURN(Slice->Nodes[Slice->Offset + Iter->Index - 1].Value);
	} else {
		ML_RETURN(MLNil);
	}
}

#endif

static void ML_TYPED_FN(ml_iter_value, MLSliceMutableIterT, ml_state_t *Caller, ml_slice_index_t *Iter) {
	ml_slice_index_t *Index = new(ml_slice_index_t);
	Index->Type = MLSliceIndexT;
	Index->Slice = Iter->Slice;
	Index->Index = Iter->Index;
	ML_RETURN(Index);
}

static void ML_TYPED_FN(ml_iterate, MLSliceT, ml_state_t *Caller, ml_slice_t *Slice) {
	if (!Slice->Length) ML_RETURN(MLNil);
	ml_slice_index_t *Iter = new(ml_slice_index_t);
	Iter->Type = MLSliceIterT;
	Iter->Slice = Slice;
	Iter->Index = 1;
	ML_RETURN(Iter);
}

#ifdef ML_MUTABLES

static void ML_TYPED_FN(ml_iterate, MLSliceMutableT, ml_state_t *Caller, ml_slice_t *Slice) {
	if (!Slice->Length) ML_RETURN(MLNil);
	ml_slice_index_t *Iter = new(ml_slice_index_t);
	Iter->Type = MLSliceMutableIterT;
	Iter->Slice = Slice;
	Iter->Index = 1;
	ML_RETURN(Iter);
}

#endif

typedef struct {
	ml_type_t *Type;
	ml_slice_t *Slice;
	size_t Count;
} ml_slice_skip_t;

ML_TYPE(MLSliceSkipT, (MLSequenceT), "slice::skip");
//!internal

#ifdef ML_MUTABLES
ML_TYPE(MLSliceMutableSkipT, (MLSliceSkipT), "slice::mutable::skip");
//!internal
#else
#define MLSliceMutableSkipT MLSliceSkipT
#endif

static void ML_TYPED_FN(ml_iterate, MLSliceSkipT, ml_state_t *Caller, ml_slice_skip_t *Skip) {
	//if (Skip->Count < 0) ML_RETURN(MLNil);
	if (Skip->Count >= Skip->Slice->Length) ML_RETURN(MLNil);
	ml_slice_index_t *Iter = new(ml_slice_index_t);
	Iter->Type = MLSliceIterT;
	Iter->Slice = Skip->Slice;
	Iter->Index = Skip->Count + 1;
	ML_RETURN(Iter);
}

#ifdef ML_MUTABLES

static void ML_TYPED_FN(ml_iterate, MLSliceMutableSkipT, ml_state_t *Caller, ml_slice_skip_t *Skip) {
	//if (Skip->Count < 0) ML_RETURN(MLNil);
	if (Skip->Count >= Skip->Slice->Length) ML_RETURN(MLNil);
	ml_slice_index_t *Iter = new(ml_slice_index_t);
	Iter->Type = MLSliceMutableIterT;
	Iter->Slice = Skip->Slice;
	Iter->Index = Skip->Count + 1;
	ML_RETURN(Iter);
}

#endif

ML_METHOD("skip", MLSliceT, MLIntegerT) {
//!internal
	ml_slice_skip_t *Skip = new(ml_slice_skip_t);
	Skip->Type = MLSliceSkipT;
	Skip->Slice = (ml_slice_t *)Args[0];
	Skip->Count = ml_integer_value(Args[1]);
	return (ml_value_t *)Skip;
}

ML_METHODV("push", MLSliceMutableT) {
	ml_value_t *Slice = Args[0];
	for (int I = 1; I < Count; ++I) ml_slice_push(Slice, Args[I]);
	return Args[0];
}

ML_METHODV("put", MLSliceMutableT) {
	ml_value_t *Slice = Args[0];
	for (int I = 1; I < Count; ++I) ml_slice_put(Slice, Args[I]);
	return Args[0];
}

ML_METHOD("pop", MLSliceMutableT) {
	return ml_slice_pop(Args[0]);
}

ML_METHOD("pull", MLSliceMutableT) {
	return ml_slice_pull(Args[0]);
}

ML_METHOD("empty", MLSliceMutableT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	Slice->Offset = Slice->Length = Slice->Capacity = 0;
	Slice->Nodes = Empty;
	return (ml_value_t *)Slice;
}

ML_METHOD("+", MLSliceT, MLSliceT) {
	ml_slice_t *Slice1 = (ml_slice_t *)Args[0];
	ml_slice_t *Slice2 = (ml_slice_t *)Args[1];
	ml_slice_t *Slice = (ml_slice_t *)ml_slice(Slice1->Length + Slice2->Length);
	void *Rest = mempcpy(Slice->Nodes, Slice1->Nodes + Slice1->Offset, Slice1->Length * sizeof(ml_slice_node_t));
	memcpy(Rest, Slice2->Nodes + Slice2->Offset, Slice2->Length * sizeof(ml_slice_node_t *));
	Slice->Length = Slice1->Length + Slice2->Length;
	return (ml_value_t *)Slice;
}

ML_METHOD("splice", MLSliceMutableT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	ml_slice_t *Removed = new(ml_slice_t);
	Removed[0] = Slice[0];
	Slice->Offset = Slice->Length = Slice->Capacity = 0;
	Slice->Nodes = Empty;
	return (ml_value_t *)Removed;
}

ML_METHOD("splice", MLSliceMutableT, MLIntegerT, MLIntegerT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	size_t Offset = Slice->Offset;
	size_t Length = Slice->Length;
	int Start = ml_integer_value_fast(Args[1]);
	if (Start <= 0) Start += Length + 1;
	if (Start <= 0) return MLNil;
	if (Start > Length + 1) return MLNil;
	int Remove = ml_integer_value_fast(Args[2]);
	if (Remove < 0) return MLNil;
	int End = Start + Remove - 1;
	if (End > Length) return MLNil;
	if (Remove == 0) return ml_slice(0);
	ml_slice_t *Removed = (ml_slice_t *)ml_slice(Remove);
	ml_slice_node_t *Nodes0 = Slice->Nodes + Offset + Start - 1;
	memcpy(Removed->Nodes, Nodes0, Remove * sizeof(ml_slice_node_t));
	Removed->Length = Remove;
	int After = Length - End;
	memmove(Nodes0, Nodes0 + Remove, After * sizeof(ml_slice_node_t));
	memset(Nodes0 + After, 0, Remove * sizeof(ml_slice_node_t));
	Slice->Length -= Remove;
	return (ml_value_t *)Removed;
}

ML_METHOD("splice", MLSliceMutableT, MLIntegerT, MLIntegerT, MLSliceMutableT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	size_t Offset = Slice->Offset;
	size_t Length = Slice->Length;
	int Start = ml_integer_value_fast(Args[1]);
	if (Start <= 0) Start += Length + 1;
	if (Start <= 0) return MLNil;
	if (Start > Length + 1) return MLNil;
	int Remove = ml_integer_value_fast(Args[2]);
	if (Remove < 0) return MLNil;
	int End = Start + Remove - 1;
	if (End > Length) return MLNil;
	ml_slice_t *Source = (ml_slice_t *)Args[3];
	int Insert = Source->Length;
	ml_slice_t *Removed = (ml_slice_t *)ml_slice(Remove);
	ml_slice_node_t *Nodes0 = Slice->Nodes + Offset + Start - 1;
	memcpy(Removed->Nodes, Nodes0, Remove * sizeof(ml_slice_node_t));
	Removed->Length = Remove;
	int After = Length - End;
	if (Offset + Length + Insert - Remove <= Slice->Capacity) {
		memmove(Nodes0 + Insert, Nodes0 + Remove, After * sizeof(ml_slice_node_t));
		memcpy(Nodes0, Source->Nodes + Source->Offset, Insert * sizeof(ml_slice_node_t));
		int Extra = Remove - Insert;
		if (Extra > 0) memset(Nodes0 + Insert + After, 0, Extra * sizeof(ml_slice_node_t));
		Slice->Length -= Extra;
	} else if (Offset >= Insert - Remove) {
		int Extra = Insert - Remove;
		Nodes0 = Slice->Nodes + Offset;
		memmove(Nodes0 - Extra, Nodes0, (Start - 1) * sizeof(ml_slice_node_t));
		Offset -= Extra;
		Slice->Offset = Offset;
		Nodes0 = Slice->Nodes + Offset + Start - 1;
		memcpy(Nodes0, Source->Nodes + Source->Offset, Insert * sizeof(ml_slice_node_t));
		Slice->Length += Extra;
	} else {
		size_t Capacity = Length + Insert - Remove;
		ml_slice_node_t *Nodes = anew(ml_slice_node_t, Capacity + 1);
		void *Next = mempcpy(Nodes, Slice->Nodes + Offset, (Start - 1) * sizeof(ml_slice_node_t));
		Next = mempcpy(Next, Source->Nodes + Source->Offset, Insert * sizeof(ml_slice_node_t));
		memcpy(Next, Nodes0 + Remove, After * sizeof(ml_slice_node_t));
		Slice->Length = Slice->Capacity = Capacity;
		Slice->Nodes = Nodes;
		Slice->Offset = 0;
	}
	Source->Offset = Source->Length = Source->Capacity = 0;
	Source->Nodes = Empty;
	return (ml_value_t *)Removed;
}

ML_METHOD("splice", MLSliceMutableT, MLIntegerT, MLSliceMutableT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	size_t Offset = Slice->Offset;
	size_t Length = Slice->Length;
	int Start = ml_integer_value_fast(Args[1]);
	if (Start <= 0) Start += Length + 1;
	if (Start <= 0) return MLNil;
	if (Start > Length + 1) return MLNil;
	int End = Start - 1;
	if (End > Length) return MLNil;
	ml_slice_t *Source = (ml_slice_t *)Args[2];
	int Insert = Source->Length;
	ml_slice_node_t *Nodes0 = Slice->Nodes + Offset + Start - 1;
	int After = Length - End;
	if (Offset + Length + Insert <= Slice->Capacity) {
		memmove(Nodes0 + Insert, Nodes0, After * sizeof(ml_slice_node_t));
		memcpy(Nodes0, Source->Nodes + Source->Offset, Insert * sizeof(ml_slice_node_t));
		Slice->Length += Insert;
	} else if (Offset >= Insert) {
		Nodes0 = Slice->Nodes + Offset;
		memmove(Nodes0 - Insert, Nodes0, (Start - 1) * sizeof(ml_slice_node_t));
		Offset -= Insert;
		Slice->Offset = Offset;
		Nodes0 = Slice->Nodes + Offset + Start - 1;
		memcpy(Nodes0, Source->Nodes + Source->Offset, Insert * sizeof(ml_slice_node_t));
		Slice->Length += Insert;
	} else {
		size_t Capacity = Length + Insert;
		ml_slice_node_t *Nodes = anew(ml_slice_node_t, Capacity + 1);
		void *Next = mempcpy(Nodes, Slice->Nodes + Offset, (Start - 1) * sizeof(ml_slice_node_t));
		Next = mempcpy(Next, Source->Nodes + Source->Offset, Insert * sizeof(ml_slice_node_t));
		memcpy(Next, Nodes0, After * sizeof(ml_slice_node_t));
		Slice->Length = Slice->Capacity = Capacity;
		Slice->Nodes = Nodes;
		Slice->Offset = 0;
	}
	Source->Offset = Source->Length = Source->Capacity = 0;
	Source->Nodes = Empty;
	return (ml_value_t *)MLNil;
}

ML_METHOD("reverse", MLSliceMutableT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	ml_slice_node_t *A = Slice->Nodes + Slice->Offset;
	ml_slice_node_t *B = A + Slice->Length - 1;
	while (B > A) {
		ml_value_t *Temp = A->Value;
		A->Value = B->Value;
		B->Value = Temp;
		++A; --B;
	}
	return (ml_value_t *)Slice;
}

typedef struct {
	ml_state_t Base;
	ml_slice_t *Slice;
	ml_value_t *Compare;
	ml_slice_node_t *Source, *Dest;
	ml_slice_node_t *IndexA, *LimitA, *IndexB, *LimitB;
	ml_slice_node_t *Target, *Limit;
	ml_value_t *Args[2];
	size_t Length, BlockSize;
} ml_slice_sort_state_t;

static void ml_slice_sort_state_run(ml_slice_sort_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_slice_node_t *Target = State->Target;
	if (Value != MLNil) {
		ml_slice_node_t *Index = State->IndexA;
		*Target++ = *Index++;
		if (Index < State->LimitA) {
			State->Target = Target;
			State->IndexA = Index;
			State->Args[0] = Index->Value;
			State->Args[1] = State->IndexB->Value;
			return ml_call(State, State->Compare, 2, State->Args);
		}
		Target = mempcpy(Target, State->IndexB, (State->LimitB - State->IndexB) * sizeof(ml_slice_node_t));
	} else {
		ml_slice_node_t *Index = State->IndexB;
		*Target++ = *Index++;
		if (Index < State->LimitB) {
			State->Target = Target;
			State->IndexB = Index;
			State->Args[0] = State->IndexA->Value;
			State->Args[1] = Index->Value;
			return ml_call(State, State->Compare, 2, State->Args);
		}
		Target = mempcpy(Target, State->IndexA, (State->LimitA - State->IndexA) * sizeof(ml_slice_node_t));
	}
	size_t Remaining = State->Limit - Target;
	size_t BlockSize = State->BlockSize;
	ml_slice_node_t *IndexA = State->LimitB;
	if (Remaining <= BlockSize) {
		memcpy(Target, State->LimitB, Remaining * sizeof(ml_slice_node_t));
		BlockSize *= 2;
		Remaining = State->Length;
		if (Remaining <= BlockSize) {
			ml_slice_t *Slice = State->Slice;
			memcpy(Slice->Nodes + Slice->Offset, State->Dest, Remaining * sizeof(ml_slice_node_t));
			ML_CONTINUE(State->Base.Caller, Slice);
		}
		State->BlockSize = BlockSize;
		ml_slice_node_t *Temp = State->Source;
		IndexA = State->Source = State->Dest;
		Target = State->Dest = Temp;
		State->Limit = Target + State->Length;
	}
	State->Target = Target;
	State->IndexA = IndexA;
	ml_slice_node_t *IndexB = IndexA + BlockSize;
	State->LimitA = State->IndexB = IndexB;
	Remaining -= BlockSize;
	State->LimitB = IndexB + (Remaining < BlockSize ? Remaining : BlockSize);
	State->Args[0] = IndexA->Value;
	State->Args[1] = IndexB->Value;
	return ml_call(State, State->Compare, 2, State->Args);
}

extern ml_value_t *LessMethod;

ML_METHODX("sort", MLSliceT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	size_t Length = Slice->Length;
	if (Length < 2) ML_RETURN(Slice);
	ml_slice_sort_state_t *State = new(ml_slice_sort_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_sort_state_run;
	State->Slice = Slice;
	State->Compare = LessMethod;
	ml_slice_node_t *Source = State->Source = anew(ml_slice_node_t, Length);
	ml_slice_node_t *Dest = State->Dest = anew(ml_slice_node_t, Length);
	memcpy(Source, Slice->Nodes + Slice->Offset, Length * sizeof(ml_slice_node_t));
	State->IndexA = Source;
	State->IndexB = State->LimitA = Source + 1;
	State->LimitB = Source + 2;
	State->Target = Dest;
	State->Limit = Dest + Length;
	State->Length = Length;
	State->BlockSize = 1;
	State->Args[0] = Source[0].Value;
	State->Args[1] = Source[1].Value;
	return ml_call(State, State->Compare, 2, State->Args);
}

ML_METHODX("sort", MLSliceT, MLFunctionT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	size_t Length = Slice->Length;
	if (Length < 2) ML_RETURN(Slice);
	ml_slice_sort_state_t *State = new(ml_slice_sort_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_sort_state_run;
	State->Slice = Slice;
	State->Compare = Args[1];
	ml_slice_node_t *Source = State->Source = anew(ml_slice_node_t, Length);
	ml_slice_node_t *Dest = State->Dest = anew(ml_slice_node_t, Length);
	memcpy(Source, Slice->Nodes + Slice->Offset, Length * sizeof(ml_slice_node_t));
	State->IndexA = Source;
	State->IndexB = State->LimitA = Source + 1;
	State->LimitB = Source + 2;
	State->Target = Dest;
	State->Limit = Dest + Length;
	State->Length = Length;
	State->BlockSize = 1;
	State->Args[0] = Source[0].Value;
	State->Args[1] = Source[1].Value;
	return ml_call(State, State->Compare, 2, State->Args);
}

typedef struct {
	ml_state_t Base;
	ml_slice_node_t *Node;
	ml_value_t *Compare;
	ml_value_t *Args[2];
	int Index;
} ml_slice_find_state_t;

extern ml_value_t *EqualMethod;

static void ml_slice_find_state_run(ml_slice_find_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Caller);
	if (Value != MLNil) ML_RETURN(ml_integer(State->Index));
	ml_slice_node_t *Node = State->Node + 1;
	if (!Node->Value) ML_RETURN(MLNil);
	State->Node = Node;
	State->Args[1] = Node->Value;
	++State->Index;
	return ml_call(State, State->Compare, 2, State->Args);
}

ML_METHODX("find", MLSliceT, MLAnyT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	if (!Slice->Length) ML_RETURN(MLNil);
	ml_slice_node_t *Node = Slice->Nodes + Slice->Offset;
	ml_slice_find_state_t *State = new(ml_slice_find_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_find_state_run;
	State->Args[0] = Args[1];
	State->Compare = EqualMethod;
	State->Node = Node;
	State->Args[1] = Node->Value;
	State->Index = 1;
	return ml_call(State, State->Compare, 2, State->Args);
}

ML_METHODX("find", MLSliceT, MLAnyT, MLFunctionT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	if (!Slice->Length) ML_RETURN(MLNil);
	ml_slice_node_t *Node = Slice->Nodes + Slice->Offset;
	ml_slice_find_state_t *State = new(ml_slice_find_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_find_state_run;
	State->Args[0] = Args[1];
	State->Compare = Args[2];
	State->Node = Node;
	State->Args[1] = Node->Value;
	State->Index = 1;
	return ml_call(State, State->Compare, 2, State->Args);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Slice, *Value, *Compare;
	ml_value_t *Args[2];
	int Index, Min, Max;
} ml_slice_bfind_state_t;

static void ml_slice_bfind_state_run(ml_slice_bfind_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Caller);
	int Compare = ml_integer_value(Value);
	if (Compare < 0) {
		if (State->Index > State->Min) {
			State->Max = State->Index - 1;
			State->Index = State->Min + (State->Max - State->Min) / 2;
			State->Args[0] = State->Value;
			State->Args[1] = ml_slice_get(State->Slice, State->Index);
			return ml_call(State, State->Compare, 2, State->Args);
		}
		ML_RETURN(ml_tuplev(2, MLNil, ml_integer(State->Index)));
	} else if (Compare > 0) {
		if (State->Index < State->Max) {
			State->Min = State->Index + 1;
			State->Index = State->Min + (State->Max - State->Min) / 2;
			State->Args[0] = State->Value;
			State->Args[1] = ml_slice_get(State->Slice, State->Index);
			return ml_call(State, State->Compare, 2, State->Args);
		}
		ML_RETURN(ml_tuplev(2, MLNil, ml_integer(State->Index + 1)));
	} else {
		ML_RETURN(ml_tuplev(2, ml_integer(State->Index), ml_integer(State->Index)));
	}
}

extern ml_value_t *CompareMethod;

ML_METHODX("bfind", MLSliceT, MLAnyT) {
	int Length = ml_slice_length(Args[0]);
	if (!Length) ML_RETURN(ml_tuplev(2, ml_integer(0), ml_integer(1)));
	ml_slice_bfind_state_t *State = new(ml_slice_bfind_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_bfind_state_run;
	State->Slice = Args[0];
	State->Value = Args[1];
	State->Compare = CompareMethod;
	State->Min = 1;
	State->Max = Length;
	State->Index = State->Min + (State->Max - State->Min) / 2;
	State->Args[0] = State->Value;
	State->Args[1] = ml_slice_get(State->Slice, State->Index);
	return ml_call(State, State->Compare, 2, State->Args);
}

ML_METHODX("bfind", MLSliceT, MLAnyT, MLFunctionT) {
	int Length = ml_slice_length(Args[0]);
	if (!Length) ML_RETURN(ml_tuplev(2, ml_integer(0), ml_integer(1)));
	ml_slice_bfind_state_t *State = new(ml_slice_bfind_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_bfind_state_run;
	State->Slice = Args[0];
	State->Value = Args[1];
	State->Compare = Args[2];
	State->Min = 1;
	State->Max = Length;
	State->Index = State->Min + (State->Max - State->Min) / 2;
	State->Args[0] = State->Value;
	State->Args[1] = ml_slice_get(State->Slice, State->Index);
	return ml_call(State, State->Compare, 2, State->Args);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Slice, *Value, *Approx, *Compare;
	ml_value_t *Args[3];
	int Index, Min, Max;
} ml_slice_afind_state_t;

static void ml_slice_afind_state_approx(ml_slice_afind_state_t *State, ml_value_t *Value);

static void ml_slice_afind_state_run(ml_slice_afind_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Caller);
	int Compare = ml_integer_value(Value);
	if (Compare < 0) {
		if (State->Index > State->Min) {
			State->Max = State->Index - 1;
			State->Args[0] = ml_integer(State->Min + (State->Max - State->Min) / 2);
			State->Args[1] = ml_integer(State->Min);
			State->Args[2] = ml_integer(State->Max);
			State->Base.run = (ml_state_fn)ml_slice_afind_state_approx;
			return ml_call(State, State->Approx, 3, State->Args);
		}
		ML_RETURN(ml_tuplev(2, MLNil, ml_integer(State->Index)));
	} else if (Compare > 0) {
		if (State->Index < State->Max) {
			State->Min = State->Index + 1;
			State->Args[0] = ml_integer(State->Min + (State->Max - State->Min) / 2);
			State->Args[1] = ml_integer(State->Min);
			State->Args[2] = ml_integer(State->Max);
			State->Base.run = (ml_state_fn)ml_slice_afind_state_approx;
			return ml_call(State, State->Approx, 3, State->Args);
		}
		ML_RETURN(ml_tuplev(2, MLNil, ml_integer(State->Index + 1)));
	} else {
		ML_RETURN(ml_tuplev(2, ml_integer(State->Index), ml_integer(State->Index)));
	}
}

static void ml_slice_afind_state_approx(ml_slice_afind_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Caller);
	State->Index = ml_integer_value(Value);
	if (State->Index < State->Min || State->Index > State->Max) {
		ML_ERROR("ValueError", "Approximate index %d is not between %d and %d", State->Index, State->Min, State->Max);
	}
	State->Args[0] = State->Value;
	State->Args[1] = ml_slice_get(State->Slice, State->Index);
	State->Base.run = (ml_state_fn)ml_slice_afind_state_run;
	return ml_call(State, State->Compare, 2, State->Args);
}

ML_METHODX("afind", MLSliceT, MLAnyT, MLFunctionT) {
	int Length = ml_slice_length(Args[0]);
	if (!Length) ML_RETURN(ml_tuplev(2, ml_integer(0), ml_integer(1)));
	ml_slice_afind_state_t *State = new(ml_slice_afind_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_afind_state_approx;
	State->Slice = Args[0];
	State->Value = Args[1];
	State->Approx = Args[2];
	State->Compare = CompareMethod;
	State->Min = 1;
	State->Max = Length;
	State->Args[0] = ml_integer(State->Min + (State->Max - State->Min) / 2);
	State->Args[1] = ml_integer(State->Min);
	State->Args[2] = ml_integer(State->Max);
	return ml_call(State, State->Approx, 3, State->Args);
}

ML_METHODX("afind", MLSliceT, MLAnyT, MLFunctionT, MLFunctionT) {
	int Length = ml_slice_length(Args[0]);
	if (!Length) ML_RETURN(ml_tuplev(2, ml_integer(0), ml_integer(1)));
	ml_slice_afind_state_t *State = new(ml_slice_afind_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_afind_state_approx;
	State->Slice = Args[0];
	State->Value = Args[1];
	State->Approx = Args[2];
	State->Compare = Args[3];
	State->Min = 1;
	State->Max = Length;
	State->Args[0] = ml_integer(State->Min + (State->Max - State->Min) / 2);
	State->Args[1] = ml_integer(State->Min);
	State->Args[2] = ml_integer(State->Max);
	return ml_call(State, State->Approx, 3, State->Args);
}

typedef struct {
	ml_type_t *Type;
	int Length;
	int Indices[];
} ml_afinder_t;

static void ml_afinder_call(ml_state_t *Caller, const ml_afinder_t *Finder, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(3);
	int Index = ml_integer_value(ml_deref(Args[0]));
	int Min = ml_integer_value(ml_deref(Args[1]));
	int Max = ml_integer_value(ml_deref(Args[2]));
	int A = 0, B = Finder->Length - 1;
	const int *Indices = Finder->Indices;
	for (;;) {
		int I = A + (B - A) / 2;
		if (Index < Indices[I]) {
			if (I > A) {
				B = I - 1;
			} else {
				B = I;
				break;
			}
		} else if (Index > Indices[I]) {
			if (I < B) {
				A = I + 1;
			} else {
				B = I + 1;
				break;
			}
		} else {
			ML_RETURN(ml_integer(Index));
		}
	}
	if (B == 0) {
		int Y = Indices[B];
		if (Min <= Y && Y <= Max) ML_RETURN(ml_integer(Y));
		ML_RETURN(ml_integer(Index));
	} else if (B == Finder->Length) {
		int X = Indices[B - 1];
		if (Min <= X && X <= Max) ML_RETURN(ml_integer(X));
		ML_RETURN(ml_integer(Index));
	} else {
		int X = Indices[B - 1];
		int Y = Indices[B];
		if (Index - X < Y - Index) {
			if (Min <= X && X <= Max) ML_RETURN(ml_integer(X));
			if (Min <= Y && Y <= Max) ML_RETURN(ml_integer(Y));
			ML_RETURN(ml_integer(Index));
		} else {
			if (Min <= Y && Y <= Max) ML_RETURN(ml_integer(Y));
			if (Min <= X && X <= Max) ML_RETURN(ml_integer(X));
			ML_RETURN(ml_integer(Index));
		}
	}
}

ML_TYPE(MLAfinderT, (MLFunctionT), "afinder",
//!internal
	.call = (void *)ml_afinder_call
);

ML_METHOD("afinder", MLSliceT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	if (!Slice->Length) return ml_integer(1);
	int Length = Slice->Length;
	ml_afinder_t *Finder = xnew(ml_afinder_t, Length, int);
	Finder->Type = MLAfinderT;
	Finder->Length = Length;
	ml_slice_node_t *Nodes = Slice->Nodes + Slice->Offset;
	int *Indices = Finder->Indices;
	while (--Length >= 0) {
		ml_value_t *Value = (Nodes++)->Value;
		*Indices++ = ml_integer_value(Value);
	}
	return (ml_value_t *)Finder;
}

ML_METHOD("insert", MLSliceMutableT, MLIntegerT, MLAnyT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	int Length = Slice->Length;
	if (Index <= 0) Index += Length + 1;
	if (Index <= 0 || Index > Length + 1) return MLNil;
	ml_slice_insert((ml_value_t *)Slice, Index, Args[2]);
	return (ml_value_t *)Slice;
}

ML_METHOD("delete", MLSliceMutableT, MLIntegerT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	int Length = Slice->Length;
	if (Index <= 0) Index += Length + 1;
	if (Index <= 0 || Index > Length) return MLNil;
	return ml_slice_delete((ml_value_t *)Slice, Index);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Slice;
	ml_value_t *Fn;
	ml_value_t *Args[1];
	int Index, Limit;
} ml_slice_remove_state_t;

static void ml_slice_pop_state_run(ml_slice_remove_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (Value != MLNil) {
		ML_RETURN(ml_slice_delete(State->Slice, State->Index));
	}
	if (State->Index >= State->Limit) ML_RETURN(MLNil);
	Value = ml_slice_get(State->Slice, ++State->Index);
	if (!Value) ML_ERROR("StateError", "Invalid slice state");
	State->Args[0] = Value;
	return ml_call(State, State->Fn, 1, State->Args);
}

ML_METHODX("pop", MLSliceMutableT, MLFunctionT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	if (!Slice->Length) ML_RETURN(MLNil);
	ml_slice_remove_state_t *State = new(ml_slice_remove_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_pop_state_run;
	State->Slice = (ml_value_t *)Slice;
	State->Fn = Args[1];
	State->Args[0] = ml_slice_get((ml_value_t *)Slice, 1);
	State->Index = 1;
	State->Limit = Slice->Length;
	return ml_call(State, State->Fn, 1, State->Args);
}

static void ml_slice_pull_state_run(ml_slice_remove_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (Value != MLNil) {
		ML_RETURN(ml_slice_delete(State->Slice, State->Index));
	}
	if (State->Index <= State->Limit) ML_RETURN(MLNil);
	Value = ml_slice_get(State->Slice, --State->Index);
	if (!Value) ML_ERROR("StateError", "Invalid slice state");
	State->Args[0] = Value;
	return ml_call(State, State->Fn, 1, State->Args);
}

ML_METHODX("pull", MLSliceMutableT, MLFunctionT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	if (!Slice->Length) ML_RETURN(MLNil);
	ml_slice_remove_state_t *State = new(ml_slice_remove_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_pull_state_run;
	State->Slice = (ml_value_t *)Slice;
	State->Fn = Args[1];
	State->Args[0] = ml_slice_get((ml_value_t *)Slice, Slice->Length);
	State->Index = Slice->Length;
	State->Limit = 1;
	return ml_call(State, State->Fn, 1, State->Args);
}

ML_METHOD("permute", MLSliceMutableT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	int N = Slice->Length;
	if (N <= 1) return (ml_value_t *)Slice;
	ml_slice_node_t *Nodes = Slice->Nodes + Slice->Offset;
	for (int I = N; --I > 0;) {
		int Divisor = RAND_MAX / (I + 1), J;
		do J = random() / Divisor; while (J > I);
		if (J != I) {
			ml_value_t *Old = Nodes[J].Value;
			Nodes[J].Value = Nodes[I].Value;
			Nodes[I].Value = Old;
		}
	}
	return (ml_value_t *)Slice;
}

ML_METHOD("shuffle", MLSliceMutableT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	int N = Slice->Length;
	if (N <= 1) return (ml_value_t *)Slice;
	ml_slice_node_t *Nodes = Slice->Nodes + Slice->Offset;
	for (int I = N; --I > 0;) {
		int Divisor = RAND_MAX / (I + 1), J;
		do J = random() / Divisor; while (J > I);
		if (J != I) {
			ml_value_t *Old = Nodes[J].Value;
			Nodes[J].Value = Nodes[I].Value;
			Nodes[I].Value = Old;
		}
	}
	return (ml_value_t *)Slice;
}

ML_METHOD("cycle", MLSliceMutableT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	int N = Slice->Length;
	if (N <= 1) return (ml_value_t *)Slice;
	ml_slice_node_t *Nodes = Slice->Nodes + Slice->Offset;
	for (int I = N; --I > 0;) {
		int Divisor = RAND_MAX / (I + 1), J;
		do J = random() / Divisor; while (J >= I);
		if (J != I) {
			ml_value_t *Old = Nodes[J].Value;
			Nodes[J].Value = Nodes[I].Value;
			Nodes[I].Value = Old;
		}
	}
	return (ml_value_t *)Slice;
}

typedef struct {
	ml_type_t *Type;
	ml_slice_t *Slice;
	ml_slice_node_t *Nodes;
	int Index, Length;
	int P[];
} ml_slice_permutations_t;

ML_TYPE(MLSlicePermutationsT, (MLSequenceT), "slice::permutations");
//!internal

static void ML_TYPED_FN(ml_iterate, MLSlicePermutationsT, ml_state_t *Caller, ml_slice_permutations_t *Permutations) {
	Permutations->Index = 1;
	ML_RETURN(Permutations);
}

static void ML_TYPED_FN(ml_iter_next, MLSlicePermutationsT, ml_state_t *Caller, ml_slice_permutations_t *Permutations) {
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
	ml_slice_node_t *Nodes = Permutations->Nodes;
	ml_value_t *Value = Nodes[Index].Value;
	Nodes[Index].Value = Nodes[J].Value;
	Nodes[J].Value = Value;
	++Permutations->Index;
	ML_RETURN(Permutations);
}

static void ML_TYPED_FN(ml_iter_key, MLSlicePermutationsT, ml_state_t *Caller, ml_slice_permutations_t *Permutations) {
	ML_RETURN(ml_integer(Permutations->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLSlicePermutationsT, ml_state_t *Caller, ml_slice_permutations_t *Permutations) {
	ML_RETURN(Permutations->Slice);
}

ML_METHOD("permutations", MLSliceMutableT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	int N = Slice->Length;
	ml_slice_permutations_t *Permutations = xnew(ml_slice_permutations_t, N + 1, int);
	Permutations->Type = MLSlicePermutationsT;
	Permutations->Length = N;
	Permutations->Nodes = Slice->Nodes + Slice->Offset;
	for (int I = 0; I < N; ++I) Permutations->P[I] = I;
	Permutations->P[N] = N;
	Permutations->Slice = Slice;
	return (ml_value_t *)Permutations;
}

ML_METHOD("random", MLSliceT) {
	ml_slice_t *Slice = (ml_slice_t *)Args[0];
	int Limit = Slice->Length;
	if (Limit <= 0) return MLNil;
	int Divisor = RAND_MAX / Limit;
	int Random;
	do Random = random() / Divisor; while (Random >= Limit);
	ml_slice_index_t *Iter = new(ml_slice_index_t);
	Iter->Type = MLSliceIndexT;
	Iter->Slice = Slice;
	Iter->Index = Random + 1;
	return (ml_value_t *)Iter;
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Visitor, *Dest;
	ml_slice_node_t *Node;
	ml_value_t *Args[1];
} ml_slice_visit_t;

static void ml_slice_visit_run(ml_slice_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_slice_node_t *Node = State->Node + 1;
	if (!Node->Value) ML_RETURN(MLNil);
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, State->Visitor, 1, State->Args);
}

ML_METHODX("visit", MLVisitorT, MLSliceT) {
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_slice_t *Slice = (ml_slice_t *)Args[1];
	ml_slice_node_t *Node = Slice->Nodes + Slice->Offset;
	if (!Node->Value) ML_RETURN(MLNil);
	ml_slice_visit_t *State = new(ml_slice_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_visit_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

static void ml_slice_copy_run(ml_slice_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_slice_put(State->Dest, Value);
	ml_slice_node_t *Node = State->Node + 1;
	if (!Node->Value) ML_RETURN(State->Dest);
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, State->Visitor, 1, State->Args);
}

ML_METHODX("copy", MLVisitorT, MLSliceT) {
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_slice_t *Slice = (ml_slice_t *)Args[1];
	ml_value_t *Dest = ml_slice(Slice->Length);
	inthash_insert(Visitor->Cache, (uintptr_t)Args[1], Dest);
	ml_slice_node_t *Node = Slice->Nodes + Slice->Offset;
	if (!Node->Value) ML_RETURN(Dest);
	ml_slice_visit_t *State = new(ml_slice_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_copy_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Dest = Dest;
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

static void ml_slice_const_run(ml_slice_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_slice_put(State->Dest, Value);
	ml_slice_node_t *Node = State->Node + 1;
	if (!Node->Value) {
#ifdef ML_GENERICS
		if (State->Dest->Type->Type == MLTypeGenericT) {
			ml_type_t *TArgs[2];
			ml_find_generic_parent(State->Dest->Type, MLSliceMutableT, 2, TArgs);
			TArgs[0] = MLSliceT;
			State->Dest->Type = ml_generic_type(2, TArgs);
		} else {
#endif
			State->Dest->Type = MLSliceT;
#ifdef ML_GENERICS
		}
#endif
		ML_RETURN(State->Dest);
	}
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, State->Visitor, 1, State->Args);
}

ML_METHODX("const", MLVisitorT, MLSliceMutableT) {
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_slice_t *Slice = (ml_slice_t *)Args[1];
	ml_value_t *Dest = ml_slice(Slice->Length);
	inthash_insert(Visitor->Cache, (uintptr_t)Args[1], Dest);
	ml_slice_node_t *Node = Slice->Nodes + Slice->Offset;
	if (!Node->Value) ML_RETURN(Dest);
	ml_slice_visit_t *State = new(ml_slice_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_const_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Dest = Dest;
	State->Node = Node;
	State->Args[0] = Node->Value;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

static int ML_TYPED_FN(ml_value_is_constant, MLSliceMutableT, ml_value_t *Slice) {
	return 0;
}

static int ML_TYPED_FN(ml_value_is_constant, MLSliceT, ml_value_t *Slice) {
	ML_SLICE_FOREACH(Slice, Iter) {
		if (!ml_value_is_constant(Iter->Value)) return 0;
	}
	return 1;
}

void ml_slice_init() {
#include "ml_slice_init.c"
	stringmap_insert(MLSliceT->Exports, "mutable", MLSliceMutableT);
	MLSliceMutableT->Constructor = MLSliceT->Constructor;
#ifdef ML_GENERICS
	ml_type_add_rule(MLSliceT, MLSequenceT, MLIntegerT, ML_TYPE_ARG(1), NULL);
#ifdef ML_MUTABLES
	ml_type_add_rule(MLSliceMutableT, MLSliceT, ML_TYPE_ARG(1), NULL);
#endif
#endif
}

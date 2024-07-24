#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include "ml_sequence.h"

#undef ML_CATEGORY
#define ML_CATEGORY "slice"

ML_TYPE(MLSliceT, (MLSequenceT), "slice");

#ifdef ML_MUTABLES
ML_TYPE(MLSliceMutableT, (MLSliceT), "slice::mutable");
#else
#define MLSliceMutableT MLListT
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

ML_TYPE(MLSliceIndexT, (), "slice::iter",
	.deref = (void *)ml_slice_index_deref,
	.assign = (void *)ml_slice_index_assign
);

static void ML_TYPED_FN(ml_iter_next, MLSliceIndexT, ml_state_t *Caller, ml_slice_index_t *Iter) {
	ml_slice_t *Slice = Iter->Slice;
	if (Iter->Index >= Slice->Length) ML_RETURN(MLNil);
	ml_slice_index_t *Next = new(ml_slice_index_t);
	Next->Type = MLSliceIndexT;
	Next->Slice = Slice;
	Next->Index = Iter->Index + 1;
	ML_RETURN(Next);
}

static void ML_TYPED_FN(ml_iter_key, MLSliceIndexT, ml_state_t *Caller, ml_slice_index_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLSliceIndexT, ml_state_t *Caller, ml_slice_index_t *Iter) {
	ML_RETURN(Iter);
}

ml_value_t *ml_slice(size_t Capacity) {
	ml_slice_t *Slice = new(ml_slice_t);
	Slice->Type = MLSliceMutableT;
	if (Capacity) {
		Slice->Capacity = Capacity;
		Slice->Nodes = anew(ml_slice_node_t, Capacity + 1);
	} else {
		static ml_slice_node_t Empty[1] = {{NULL}};
		Slice->Nodes = Empty;
	}
	return (ml_value_t *)Slice;
}

ML_METHOD(MLSliceT) {
	return ml_slice(0);
}

ML_METHOD(MLSliceT, MLTupleT) {
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
	ml_value_t *Sequence = State->Values[0];
	State->Values[0] = ml_slice(Value != MLNil ? ml_integer_value(Value) : 0);
	State->Base.run = (ml_state_fn)slice_iterate;
	return ml_iterate((ml_state_t *)State, Sequence);
}

static ML_METHOD_DECL(Precount, "precount");

ML_METHODVX(MLSliceT, MLSequenceT) {
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)slice_iterate_precount;
	State->Base.Context = Caller->Context;
	State->Values[0] = ml_chained(Count, Args);
	return ml_call(State, Precount, 1, State->Values);
}

ML_METHODVX("grow", MLSliceMutableT, MLSequenceT) {
	ml_iter_state_t *State = xnew(ml_iter_state_t, 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)slice_iterate;
	State->Base.Context = Caller->Context;
	State->Values[0] = Args[0];
	return ml_iterate((ml_state_t *)State, ml_chained(Count - 1, Args + 1));
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
		Capacity += (Capacity >> 1) + 4;
		ml_slice_node_t *Nodes = anew(ml_slice_node_t, Capacity + 1);
		memcpy(Nodes, Slice->Nodes, Length * sizeof(ml_slice_node_t));
		Slice->Nodes = Nodes;
		Slice->Nodes[Length].Value = Value;
	}
	++Slice->Length;
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
			Capacity += (Capacity >> 1) + 4;
			Offset = Capacity - Length;
			ml_slice_node_t *Nodes = anew(ml_slice_node_t, Capacity + 1);
			memcpy(Nodes + Offset, Slice->Nodes, Length * sizeof(ml_slice_node_t));
			Slice->Nodes = Nodes;
		}
	}
	Slice->Offset = --Offset;
	Slice->Nodes[Offset].Value = Value;
	++Slice->Length;
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

static void ML_TYPED_FN(ml_iterate, MLSliceT, ml_state_t *Caller, ml_slice_t *Slice) {
	if (!Slice->Length) ML_RETURN(MLNil);
	ml_slice_index_t *Iter = new(ml_slice_index_t);
	Iter->Type = MLSliceIndexT;
	Iter->Slice = Slice;
	Iter->Index = 1;
	ML_RETURN(Iter);
}

ML_METHOD("length", MLSliceT) {
	return ml_integer(((ml_slice_t *)Args[0])->Length);
}

ML_METHOD("capacity", MLSliceT) {
	return ml_integer(((ml_slice_t *)Args[0])->Capacity);
}

ML_METHOD("offset", MLSliceT) {
	return ml_integer(((ml_slice_t *)Args[0])->Offset);
}

ML_METHODV("push", MLSliceT) {
	ml_value_t *Slice = Args[0];
	for (int I = 1; I < Count; ++I) ml_slice_push(Slice, Args[I]);
	return Args[0];
}

ML_METHODV("put", MLSliceT) {
	ml_value_t *Slice = Args[0];
	for (int I = 1; I < Count; ++I) ml_slice_put(Slice, Args[I]);
	return Args[0];
}

ML_METHOD("pop", MLSliceT) {
	return ml_slice_pop(Args[0]);
}

ML_METHOD("pull", MLSliceT) {
	return ml_slice_pull(Args[0]);
}

ML_METHOD("[]", MLSliceT, MLIntegerT) {
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
	ml_state_t Base;
	ml_stringbuffer_t *Buffer;
	ml_slice_node_t *Node;
	ml_value_t *Args[2];
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
		ML_CONTINUE(State->Base.Caller, MLSome);
	}
	ml_stringbuffer_write(State->Buffer, State->Seperator, State->SeperatorLength);
	State->Node = Node;
	State->Args[1] = Node->Value;
	return ml_call(State, AppendMethod, 2, State->Args);
}

ML_METHODX("append", MLStringBufferT, MLSliceT) {
//<Buffer
//<Slice
// Appends a representation of :mini:`Slice` to :mini:`Buffer` of the form :mini:`"[" + repr(V/1) + ", " + repr(V/2) + ", " + ... + repr(V/n) + "]"`, where :mini:`repr(V/i)` is a representation of the *i*-th element (using :mini:`:append`).
//$- let B := string::buffer()
//$- B:append([1, 2, 3, 4])
//$= B:rest
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_slice_t *Slice = (ml_slice_t *)Args[1];
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
//<Buffer
//<Slice
//<Sep
// Appends a representation of :mini:`Slice` to :mini:`Buffer` of the form :mini:`repr(V/1) + Sep + repr(V/2) + Sep + ... + repr(V/n)`, where :mini:`repr(V/i)` is a representation of the *i*-th element (using :mini:`:append`).
//$- let B := string::buffer()
//$- B:append([1, 2, 3, 4], " - ")
//$= B:rest
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_slice_t *Slice = (ml_slice_t *)Args[1];
	if (!Slice->Length) ML_RETURN(MLNil);
	ml_slice_node_t *Node = Slice->Nodes + Slice->Offset;
	ml_slice_append_state_t *State = new(ml_slice_append_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_slice_append_state_run;
	State->Buffer = Buffer;
	State->Node = Node;
	State->Seperator = ml_string_value(Args[2]);
	State->SeperatorLength = ml_string_length(Args[2]);
	State->Args[0] = (ml_value_t *)Buffer;
	State->Args[1] = Node->Value;
	return ml_call(State, AppendMethod, 2, State->Args);
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

void ml_slice_init() {
#include "ml_slice_init.c"
//	stringmap_insert(MLSliceT->Exports, "mutable", MLSliceMutableT);
//	MLSliceMutableT->Constructor = MLSliceT->Constructor;
//#ifdef ML_GENERICS
//	ml_type_add_rule(MLSliceT, MLSequenceT, MLIntegerT, ML_TYPE_ARG(1), NULL);
//#ifdef ML_MUTABLES
//	ml_type_add_rule(MLSliceMutableT, MLSliceT, ML_TYPE_ARG(1), NULL);
//#endif
//#endif
}

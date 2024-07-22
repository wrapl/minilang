#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include "ml_sequence.h"

typedef struct {
	ml_type_t *Type;
	ml_value_t **Nodes;
	size_t Capacity, Offset, Length;
} ml_slice_t;

ML_TYPE(MLSliceT, (MLSequenceT), "slice");

ml_value_t *ml_slice(size_t Capacity) {
	ml_slice_t *Slice = new(ml_slice_t);
	Slice->Type = MLSliceT;
	if (Capacity) {
		Slice->Capacity = Capacity;
		Slice->Nodes = anew(ml_value_t *, Capacity);
	}
	return (ml_value_t *)Slice;
}

void ml_slice_put(ml_value_t *Slice0, ml_value_t *Value) {
	ml_slice_t *Slice = (ml_slice_t *)Slice0;
	size_t Offset = Slice->Offset;
	size_t Length = Slice->Length;
	size_t Capacity = Slice->Capacity;
	size_t Index = Offset + Length;
	if (Index < Capacity) {
		Slice->Nodes[Index] = Value;
	} else if (Offset) {
		memmove(Slice->Nodes, Slice->Nodes + Offset, Length * sizeof(ml_value_t *));
		Slice->Nodes[Length] = Value;
	} else {
		Capacity += (Capacity >> 1) + 4;
		ml_value_t **Nodes = anew(ml_value_t *, Capacity);
		memcpy(Nodes, Slice->Nodes, Length * sizeof(ml_value_t *));
		Slice->Nodes = Nodes;
		Slice->Nodes[Length] = Value;
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
			memmove(Slice->Nodes + Offset, Slice->Nodes, Length * sizeof(ml_value_t *));
		} else {
			Capacity += (Capacity >> 1) + 4;
			Offset = Capacity - Length;
			ml_value_t **Nodes = anew(ml_value_t *, Capacity);
			memcpy(Nodes + Offset, Slice->Nodes, Length * sizeof(ml_value_t *));
			Slice->Nodes = Nodes;
		}
	}
	Slice->Offset = --Offset;
	Slice->Nodes[Offset] = Value;
	++Slice->Length;
}

ml_value_t *ml_slice_pop(ml_value_t *Slice0) {
	ml_slice_t *Slice = (ml_slice_t *)Slice0;
	size_t Length = Slice->Length;
	if (!Length) return MLNil;
	size_t Offset = Slice->Offset;
	ml_value_t *Value = Slice->Nodes[Offset];
	Slice->Nodes[Offset] = NULL;
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
	ml_value_t *Value = Slice->Nodes[Index];
	Slice->Nodes[Index] = NULL;
	--Slice->Length;
	return Value;
}

typedef struct {
	ml_type_t *Type;
	ml_slice_t *Slice;
	size_t Index;
} ml_slice_index_t;

static ml_value_t *ml_slice_index_deref(ml_slice_index_t *Iter) {
	ml_slice_t *Slice = Iter->Slice;
	if (Iter->Index <= Slice->Length) {
		return Slice->Nodes[Slice->Offset + Iter->Index - 1];
	} else {
		return MLNil;
	}
}

static void ml_slice_index_assign(ml_state_t *Caller, ml_slice_index_t *Iter, ml_value_t *Value) {
	ml_slice_t *Slice = Iter->Slice;
	if (Iter->Index <= Slice->Length) {
		Slice->Nodes[Slice->Offset + Iter->Index - 1] = Value;
		ML_RETURN(Value);
	} else {
		ML_RETURN(MLNil);
	}
}

ML_TYPE(MLSliceIndexT, (), "slice::iter",
	.deref = (void *)ml_slice_index_deref,
	.assign = (void *)ml_slice_index_assign
);

static void ML_TYPED_FN(ml_iterate, MLSliceT, ml_state_t *Caller, ml_slice_t *Slice) {
	if (!Slice->Length) ML_RETURN(MLNil);
	ml_slice_index_t *Iter = new(ml_slice_index_t);
	Iter->Type = MLSliceIndexT;
	Iter->Slice = Slice;
	Iter->Index = 1;
	ML_RETURN(Iter);
}

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

ML_METHOD(MLSliceT) {
	return ml_slice(0);
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

ML_METHOD("append", MLStringBufferT, MLSliceT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_slice_t *Slice = (ml_slice_t *)Args[1];
	if (!Slice->Length) {
		ml_stringbuffer_write(Buffer, "[]", 2);
	} else {
		ml_stringbuffer_put(Buffer, '[');
		ml_value_t **Node = Slice->Nodes + Slice->Offset;
		ml_value_t **Limit = Node + Slice->Length;
		ml_stringbuffer_simple_append(Buffer, Node[0]);
		while (++Node < Limit) {
			ml_stringbuffer_write(Buffer, ", ", 2);
			ml_stringbuffer_simple_append(Buffer, Node[0]);
		}
		ml_stringbuffer_put(Buffer, ']');
	}
	return (ml_value_t *)MLSome;
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

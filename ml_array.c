#include "ml_array.h"
#include "ml_macros.h"
#include <stdint.h>
#include <string.h>

ml_type_t *MLArrayT;
ml_type_t *MLArrayAnyT;
ml_type_t *MLArrayInt8T;
ml_type_t *MLArrayInt16T;
ml_type_t *MLArrayInt32T;
ml_type_t *MLArrayInt64T;
ml_type_t *MLArrayUInt8T;
ml_type_t *MLArrayUInt16T;
ml_type_t *MLArrayUInt32T;
ml_type_t *MLArrayUInt64T;
ml_type_t *MLArrayFloat32T;
ml_type_t *MLArrayFloat64T;

ml_array_t *ml_array_new(ml_array_format_t Format, int Degree) {
	ml_type_t *Type = MLArrayT;
	switch (Format) {
	case ML_ARRAY_FORMAT_ANY: Type = MLArrayAnyT; break;
	case ML_ARRAY_FORMAT_I8: Type = MLArrayInt8T; break;
	case ML_ARRAY_FORMAT_U8: Type = MLArrayUInt8T; break;
	case ML_ARRAY_FORMAT_I16: Type = MLArrayInt16T; break;
	case ML_ARRAY_FORMAT_U16: Type = MLArrayUInt16T; break;
	case ML_ARRAY_FORMAT_I32: Type = MLArrayInt32T; break;
	case ML_ARRAY_FORMAT_U32: Type = MLArrayUInt32T; break;
	case ML_ARRAY_FORMAT_I64: Type = MLArrayInt64T; break;
	case ML_ARRAY_FORMAT_U64: Type = MLArrayUInt64T; break;
	case ML_ARRAY_FORMAT_F32: Type = MLArrayFloat32T; break;
	case ML_ARRAY_FORMAT_F64: Type = MLArrayFloat64T; break;
	};
	ml_array_t *Array = xnew(ml_array_t, Degree, ml_array_dimension_t);
	Array->Base.Type = Type;
	Array->Degree = Degree;
	Array->Format = Format;
	return Array;
}

typedef struct ml_array_init_state_t {
	ml_state_t Base;
	void *Address;
	ml_array_t *Array;
	ml_value_t *Function;
	ml_value_t *Args[];
} ml_array_init_state_t;

static ml_value_t *ml_array_init_run(ml_array_init_state_t *State, ml_value_t *Value) {
	ml_array_t *Array = State->Array;
	switch (Array->Format) {
	case ML_ARRAY_FORMAT_ANY:
		*(ml_value_t **)State->Address = Value;
		State->Address += sizeof(ml_value_t *);
		break;
	case ML_ARRAY_FORMAT_I8:
		*(int8_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(int8_t);
		break;
	case ML_ARRAY_FORMAT_U8:
		*(uint8_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(uint8_t);
		break;
	case ML_ARRAY_FORMAT_I16:
		*(int16_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(int16_t);
		break;
	case ML_ARRAY_FORMAT_U16:
		*(uint16_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(uint16_t);
		break;
	case ML_ARRAY_FORMAT_I32:
		*(int32_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(int32_t);
		break;
	case ML_ARRAY_FORMAT_U32:
		*(uint32_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(uint32_t);
		break;
	case ML_ARRAY_FORMAT_I64:
		*(int64_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(int64_t);
		break;
	case ML_ARRAY_FORMAT_U64:
		*(uint64_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(uint64_t);
		break;
	case ML_ARRAY_FORMAT_F32:
		*(float *)State->Address = ml_real_value(Value);
		State->Address += sizeof(float);
		break;
	case ML_ARRAY_FORMAT_F64:
		*(double *)State->Address = ml_real_value(Value);
		State->Address += sizeof(double);
		break;
	}
	for (int I = Array->Degree; --I >= 0;) {
		int Next = ml_integer_value(State->Args[I]) + 1;
		if (Next <= Array->Dimensions[I].Size) {
			State->Args[I] = ml_integer(Next);
			return State->Function->Type->call(State, State->Function, Array->Degree, State->Args);
		} else {
			State->Args[I] = ml_integer(1);
		}
	}
	ML_CONTINUE(State->Base.Caller, Array);
}

static ml_value_t *ml_array_new_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(2);
	ml_array_format_t Format;
	int ItemSize;
	if (Args[0] == (ml_value_t *)MLArrayAnyT) {
		Format = ML_ARRAY_FORMAT_ANY;
		ItemSize = sizeof(ml_value_t *);
	} else if (Args[0] == (ml_value_t *)MLArrayInt8T) {
		Format = ML_ARRAY_FORMAT_I8;
		ItemSize = 1;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt8T) {
		Format = ML_ARRAY_FORMAT_U8;
		ItemSize = 1;
	} else if (Args[0] == (ml_value_t *)MLArrayInt16T) {
		Format = ML_ARRAY_FORMAT_I16;
		ItemSize = 2;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt16T) {
		Format = ML_ARRAY_FORMAT_U16;
		ItemSize = 2;
	} else if (Args[0] == (ml_value_t *)MLArrayInt32T) {
		Format = ML_ARRAY_FORMAT_I32;
		ItemSize = 4;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt32T) {
		Format = ML_ARRAY_FORMAT_U32;
		ItemSize = 4;
	} else if (Args[0] == (ml_value_t *)MLArrayInt64T) {
		Format = ML_ARRAY_FORMAT_I64;
		ItemSize = 8;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt64T) {
		Format = ML_ARRAY_FORMAT_U64;
		ItemSize = 8;
	} else if (Args[0] == (ml_value_t *)MLArrayFloat32T) {
		Format = ML_ARRAY_FORMAT_F32;
		ItemSize = 4;
	} else if (Args[0] == (ml_value_t *)MLArrayFloat64T) {
		Format = ML_ARRAY_FORMAT_F64;
		ItemSize = 8;
	} else {
		return ml_error("TypeError", "Unknown type for array");
	}
	ml_array_t *Array;
	if (Args[1]->Type == MLListT) {
		int Degree = ml_list_length(Args[1]);
		Array = ml_array_new(Format, Degree);
		ml_list_node_t *Node = ml_list_head(Args[1]);
		for (int I = 0; I < Degree; ++I, Node = Node->Next) {
			if (Node->Value->Type != MLIntegerT) return ml_error("TypeError", "Dimension is not an integer");
			Array->Dimensions[I].Size = ml_integer_value(Node->Value);
		}
	} else {
		int Degree = Count - 1;
		Array = ml_array_new(Format, Degree);
		for (int I = 1; I < Count; ++I) {
			ML_CHECK_ARG_TYPE(I, MLIntegerT);
			Array->Dimensions[I - 1].Size = ml_integer_value(Args[I]);
		}
	}
	int DataSize = ItemSize;
	for (int I = Array->Degree; --I >= 0;) {
		Array->Dimensions[I].Stride = DataSize;
		DataSize *= Array->Dimensions[I].Size;
	}
	Array->Base.Address = GC_MALLOC_ATOMIC(DataSize);
	Array->Base.Size = DataSize;
	if (Count == 2) ML_CONTINUE(Caller, Array);
	ml_array_init_state_t *InitState = xnew(ml_array_init_state_t, Array->Degree, ml_value_t *);
	InitState->Base.Caller = Caller;
	InitState->Base.run = (void *)ml_array_init_run;
	InitState->Address = Array->Base.Address;
	InitState->Array = Array;
	ml_value_t *Function = InitState->Function = Args[2];
	for (int I = 0; I < Array->Degree; ++I) InitState->Args[I] = ml_integer(1);
	return Function->Type->call(InitState, Function, Array->Degree, InitState->Args);
}

ml_value_t *ml_array_wrap_fn(void *Address, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(3);
	ML_CHECK_ARG_TYPE(1, MLBufferT);
	ML_CHECK_ARG_TYPE(2, MLListT);
	ML_CHECK_ARG_TYPE(3, MLListT);
	ml_array_format_t Format;
	if (Args[0] == (ml_value_t *)MLArrayAnyT) {
		Format = ML_ARRAY_FORMAT_ANY;
	} else if (Args[0] == (ml_value_t *)MLArrayInt8T) {
		Format = ML_ARRAY_FORMAT_I8;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt8T) {
		Format = ML_ARRAY_FORMAT_U8;
	} else if (Args[0] == (ml_value_t *)MLArrayInt16T) {
		Format = ML_ARRAY_FORMAT_I16;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt16T) {
		Format = ML_ARRAY_FORMAT_U16;
	} else if (Args[0] == (ml_value_t *)MLArrayInt32T) {
		Format = ML_ARRAY_FORMAT_I32;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt32T) {
		Format = ML_ARRAY_FORMAT_U32;
	} else if (Args[0] == (ml_value_t *)MLArrayInt64T) {
		Format = ML_ARRAY_FORMAT_I64;
	} else if (Args[0] == (ml_value_t *)MLArrayUInt64T) {
		Format = ML_ARRAY_FORMAT_U64;
	} else if (Args[0] == (ml_value_t *)MLArrayFloat32T) {
		Format = ML_ARRAY_FORMAT_F32;
	} else if (Args[0] == (ml_value_t *)MLArrayFloat64T) {
		Format = ML_ARRAY_FORMAT_F64;
	} else {
		return ml_error("TypeError", "Unknown type for array");
	}
	int Degree = ml_list_length(Args[2]);
	if (Degree != ml_list_length(Args[3])) return ml_error("ValueError", "Dimensions and strides must have same length");
	ml_array_t *Array = ml_array_new(Format, Degree);
	ml_list_node_t *SizeNode = ml_list_head(Args[2]);
	ml_list_node_t *StrideNode = ml_list_head(Args[3]);
	for (int I = 0; I < Degree; ++I) {
		if (SizeNode->Value->Type != MLIntegerT) return ml_error("TypeError", "Dimension is not an integer");
		if (StrideNode->Value->Type != MLIntegerT) return ml_error("TypeError", "Stride is not an integer");
		Array->Dimensions[I].Size = ml_integer_value(SizeNode->Value);
		Array->Dimensions[I].Stride = ml_integer_value(StrideNode->Value);
		SizeNode = SizeNode->Next;
		StrideNode = StrideNode->Next;
	}
	Array->Base.Address = ((ml_buffer_t *)Args[1])->Address;
	Array->Base.Size = ((ml_buffer_t *)Args[1])->Size;
	return (ml_value_t *)Array;
}

ML_METHOD("shape", MLArrayT) {
	ml_array_t *Array = (ml_array_t *)Args[0];
	ml_value_t *Shape = ml_list();
	for (int I = 0; I < Array->Degree; ++I) {
		ml_list_put(Shape, ml_integer(Array->Dimensions[I].Size));
	}
	return (ml_value_t *)Shape;
}

ML_METHOD("strides", MLArrayT) {
	ml_array_t *Array = (ml_array_t *)Args[0];
	ml_value_t *Strides = ml_list();
	for (int I = 0; I < Array->Degree; ++I) {
		ml_list_put(Strides, ml_integer(Array->Dimensions[I].Stride));
	}
	return (ml_value_t *)Strides;
}

ML_METHOD("size", MLArrayT) {
	ml_array_t *Array = (ml_array_t *)Args[0];
	size_t Size = Array->Dimensions[Array->Degree - 1].Stride;
	for (int I = 1; I < Array->Degree; ++I) Size *= Array->Dimensions[I].Size;
	if (Array->Dimensions[0].Stride == Size) return ml_integer(Size * Array->Dimensions[0].Size);
	return MLNil;
}

ML_METHOD("degree", MLArrayT) {
	ml_array_t *Array = (ml_array_t *)Args[0];
	return ml_integer(Array->Degree);
}

ML_METHOD("transpose", MLArrayT) {
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	ml_array_t *Target = ml_array_new(Source->Format, Degree);
	for (int I = 0; I < Degree; ++I) {
		Target->Dimensions[I] = Source->Dimensions[Degree - I - 1];
	}
	Target->Base = Source->Base;
	return (ml_value_t *)Target;
}

ML_METHOD("permute", MLArrayT, MLListT) {
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	if (ml_list_length(Args[1]) != Degree) return ml_error("Error", "List length must match degree");
	ml_array_t *Target = ml_array_new(Source->Format, Degree);
	ml_list_node_t *Node = ml_list_head(Args[1]);
	for (int I = 0; I < Degree; ++I) {
		if (Node->Value->Type != MLIntegerT) return ml_error("Error", "Invalid index");
		int J = ml_integer_value(Node->Value);
		if (J <= 0) J += Degree + 1;
		if (J < 1 || J > Degree) return ml_error("Error", "Invalid index");
		Target->Dimensions[I] = Source->Dimensions[J - 1];
		Node = Node->Next;
	}
	Target->Base = Source->Base;
	return (ml_value_t *)Target;
}

typedef struct ml_integer_range_t {
	const ml_type_t *Type;
	long Start, Limit, Step;
} ml_integer_range_t;

extern ml_type_t MLIntegerRangeT[1];

static ml_value_t *ml_array_value(ml_array_t *Array, void *Address) {
	typeof(ml_array_value) *function = ml_typed_fn_get(Array->Base.Type, ml_array_value);
	return function(Array, Address);
}

static ml_value_t *ml_array_index(ml_array_t *Source, int Count, ml_value_t **Indices) {
	ml_array_dimension_t TargetDimensions[Source->Degree];
	ml_array_dimension_t *TargetDimension = TargetDimensions;
	ml_array_dimension_t *SourceDimension = Source->Dimensions;
	void *Address = Source->Base.Address;
	int Min, Max, Step, I;
	for (I = 0; I < Count; ++I) {
		ml_value_t *Index = Indices[I];
		if (Index->Type == MLIntegerT) {
			int IndexValue = ml_integer_value(Index);
			if (IndexValue <= 0) IndexValue += SourceDimension->Size + 1;
			if (--IndexValue < 0) return MLNil;
			if (IndexValue >= SourceDimension->Size) return MLNil;
			if (SourceDimension->Indices) IndexValue = SourceDimension->Indices[IndexValue];
			Address += SourceDimension->Stride * IndexValue;
		} else if (Index->Type == MLListT) {
			int Size = TargetDimension->Size = ml_list_length(Index);
			int *Indices = TargetDimension->Indices = (int *)GC_MALLOC_ATOMIC(Size * sizeof(int));
			int *IndexPtr = Indices;
			for (ml_list_node_t *Node = ml_list_head(Index); Node; Node = Node->Next) {
				int IndexValue = ml_integer_value(Node->Value);
				if (IndexValue <= 0) IndexValue += SourceDimension->Size + 1;
				if (--IndexValue < 0) return MLNil;
				if (IndexValue >= SourceDimension->Size) return MLNil;
				*IndexPtr++ = IndexValue;
			}
			TargetDimension->Stride = SourceDimension->Stride;
			++TargetDimension;
		} else if (Index->Type == MLIntegerRangeT) {
			ml_integer_range_t *IndexValue = (ml_integer_range_t *)Index;
			Min = IndexValue->Start;
			Max = IndexValue->Limit;
			Step = IndexValue->Step;
			if (Min < 1) Min += SourceDimension->Size + 1;
			if (Max < 1) Max += SourceDimension->Size + 1;
			if (--Min < 0) return MLNil;
			if (Min >= SourceDimension->Size) return MLNil;
			if (--Max < 0) return MLNil;
			if (Max >= SourceDimension->Size) return MLNil;
			if (Step == 0) return MLNil;
			int Size = TargetDimension->Size = (Max - Min) / Step + 1;
			if (Size < 0) return MLNil;
			TargetDimension->Indices = 0;
			TargetDimension->Stride = SourceDimension->Stride * Step;
			Address += SourceDimension->Stride * Min;
			++TargetDimension;
		} else if (Index == MLNil) {
			*TargetDimension = *SourceDimension;
			++TargetDimension;
		} else {
			return ml_error("TypeError", "Unknown index type");
		}
		++SourceDimension;
	}
	for (; I < Source->Degree; ++I) {
		*TargetDimension = *SourceDimension;
		++TargetDimension;
		++SourceDimension;
	}
	int Degree = TargetDimension - TargetDimensions;
	ml_array_t *Target = ml_array_new(Source->Format, Degree);
	for (int I = 0; I < Degree; ++I) Target->Dimensions[I] = TargetDimensions[I];
	Target->Base.Address = Address;
	return (ml_value_t *)Target;
}

ML_METHOD("[]", MLArrayT) {
	ml_array_t *Source = (ml_array_t *)Args[0];
	if (Count - 1 > Source->Degree) return ml_error("RangeError", "Too many indices");
	return ml_array_index(Source, Count - 1, Args + 1);
}

ML_METHOD("[]", MLArrayT, MLMapT) {
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	ml_value_t *Indices[Degree];
	for (int I = 0; I < Degree; ++I) Indices[I] = MLNil;
	ML_MAP_FOREACH(Args[1], Node) {
		int Index = ml_integer_value(Node->Key) - 1;
		if (Index < 0) Index += Degree + 1;
		if (Index < 0 || Index >= Degree) return ml_error("RangeError", "Index out of range");
		Indices[Index] = Node->Value;
	}
	return ml_array_index(Source, Degree, Indices);
}

#define UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, NAME, OP) \
\
static void NAME ## _array_suffix_ ## CTYPE1 ## _ ## CTYPE2(ml_array_dimension_t *TargetDimension, void *TargetData, ml_array_dimension_t *SourceDimension, void *SourceData) { \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				*(CTYPE1 *)(TargetData + TargetIndices[I] * TargetDimension->Stride) OP *(CTYPE2 *)(SourceData + SourceIndices[I] * SourceDimension->Stride); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				*(CTYPE1 *)(TargetData + TargetIndices[I] * TargetDimension->Stride) OP *(CTYPE2 *)SourceData; \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				*(CTYPE1 *)TargetData OP *(CTYPE2 *)(SourceData + SourceIndices[I] * SourceDimension->Stride); \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				*(CTYPE1 *)TargetData OP *(CTYPE2 *)SourceData; \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
} \
\
static void NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(ml_array_dimension_t *TargetDimension, void *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceData) { \
	if (SourceDegree == 0) return NAME ## _value_array0_ ## CTYPE1(TargetDimension, TargetData, *(CTYPE2 *)SourceData); \
	if (SourceDegree == 1) return NAME ## _array_suffix_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension, TargetData, SourceDimension, SourceData); \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData + SourceIndices[I] * SourceDimension->Stride); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData); \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData, SourceDegree - 1, SourceDimension + 1, SourceData + SourceIndices[I] * SourceDimension->Stride); \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData, SourceDegree - 1, SourceDimension + 1, SourceData); \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
} \
\
static void NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(int PrefixDegree, ml_array_dimension_t *TargetDimension, void *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceData) { \
	if (PrefixDegree == 0) return NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension, TargetData, SourceDegree, SourceDimension, SourceData); \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		for (int I = Size; --I >= 0;) { \
			NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(PrefixDegree - 1, TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree, SourceDimension, SourceData); \
		} \
	} else { \
		int Stride = TargetDimension->Stride; \
		for (int I = Size; --I >= 0;) { \
			NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(PrefixDegree - 1, TargetDimension + 1, TargetData, SourceDegree, SourceDimension, SourceData); \
			TargetData += Stride; \
		} \
	} \
} \
\
ML_METHOD(#NAME, ATYPE1, ATYPE2) { \
	ml_array_t *Target = (ml_array_t *)Args[0]; \
	ml_array_t *Source = (ml_array_t *)Args[1]; \
	if (Source->Degree > Target->Degree) return ml_error("Error", "Incompatible assignment"); \
	int PrefixDegree = Target->Degree - Source->Degree; \
	for (int I = 0; I < Source->Degree; ++I) { \
		if (Target->Dimensions[PrefixDegree + I].Size != Source->Dimensions[I].Size) return ml_error("Error", "Incompatible assignment"); \
	} \
	if (Target->Degree) { \
		NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(PrefixDegree, Target->Dimensions, Target->Base.Address, Source->Degree, Source->Dimensions, Source->Base.Address); \
	} else { \
		*(CTYPE1 *)Target->Base.Address OP *(CTYPE2 *)Source->Base.Address; \
	} \
	return Args[0]; \
}

#define UPDATE_ARRAY_METHODS(ATYPE1, CTYPE1, ATYPE2, CTYPE2) \
\
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, set, =); \
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, add, +=); \
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, sub, -=); \
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, mul, *=); \
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, div, /=);

#define UPDATE_METHOD(ATYPE, CTYPE, RFUNC, NAME, OP) \
\
static void NAME ## _value_array0_ ## CTYPE(ml_array_dimension_t *Dimension, void *Address, CTYPE Value) { \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		for (int I = 0; I < Dimension->Size; ++I) { \
			*(CTYPE *)(Address + (Indices[I]) * Dimension->Stride) OP Value; \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		for (int I = Dimension->Size; --I >= 0;) { \
			*(CTYPE *)Address OP Value; \
			Address += Stride; \
		} \
	} \
} \
\
static void NAME ## _value_array_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, void *Address, CTYPE Value) { \
	if (Degree == 0) { *(CTYPE *)Address OP Value; return; } \
	if (Degree == 1) return NAME ## _value_array0_ ## CTYPE(Dimension, Address, Value); \
	int Stride = Dimension->Stride; \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		for (int I = 0; I < Dimension->Size; ++I) { \
			NAME ## _value_array_ ## CTYPE(Degree - 1, Dimension + 1, Address + (Indices[I]) * Dimension->Stride, Value); \
		} \
	} else { \
		for (int I = Dimension->Size; --I >= 0;) { \
			NAME ## _value_array_ ## CTYPE(Degree - 1, Dimension + 1, Address, Value); \
			Address += Stride; \
		} \
	} \
} \
\
ML_METHOD(#NAME, ATYPE, MLNumberT) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	CTYPE Value = RFUNC(Args[1]); \
	if (Array->Degree == 0) { \
		*(CTYPE *)Array->Base.Address OP Value; \
	} else { \
		NAME ## _value_array_ ## CTYPE(Array->Degree, Array->Dimensions, Array->Base.Address, Value); \
	} \
	return Args[0]; \
}

typedef struct call_info_t {
	int Count;
	ml_value_t *Function;
	ml_value_t *Result;
	ml_value_t *Args[];
} call_info_t;

#define SETTER_CASE(CTYPE1, ATYPE2, CTYPE2) \
	} else if (Value->Type == ATYPE2) { \
		ml_array_t *Source = (ml_array_t *)Value; \
		if (Source->Degree > Target->Degree) return ml_error("Error", "Incompatible assignment"); \
		int PrefixDegree = Target->Degree - Source->Degree; \
		for (int I = 0; I < Source->Degree; ++I) { \
			if (Target->Dimensions[PrefixDegree + I].Size != Source->Dimensions[I].Size) return ml_error("Error", "Incompatible assignment"); \
		} \
		if (Target->Degree) { \
			set_array_prefix_ ## CTYPE1 ## _ ## CTYPE2(PrefixDegree, Target->Dimensions, Target->Base.Address, Source->Degree, Source->Dimensions, Source->Base.Address); \
		} else { \
			*(CTYPE1 *)Target->Base.Address = *(CTYPE2 *)Source->Base.Address; \
		} \
		return Value; \

#define ARITH_METHOD(ATYPE, CTYPE, OP) \
ML_METHOD(#OP, ATYPE, MLIntegerT) { \
	ml_array_t *Source = (ml_array_t *)Args[0]; \
	int Degree = Source->Degree; \
	ml_array_t *Target = ml_array_new(Source->Format, Degree); \
	int DataSize = sizeof(CTYPE); \
	for (int I = Degree; --I >= 0;) { \
		Target->Dimensions[I].Stride = DataSize; \
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	CTYPE *Values = Target->Base.Address = GC_MALLOC_ATOMIC(DataSize * sizeof(CTYPE)); \
	set_array_ ## CTYPE ## _ ## CTYPE(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address); \
	int Value = ml_integer_value(Args[1]); \
	for (int I = DataSize / sizeof(CTYPE); --I >= 0; ++Values) *Values = *Values OP Value; \
	return (ml_value_t *)Target; \
} \
\
ML_METHOD(#OP, MLIntegerT, ATYPE) { \
	ml_array_t *Source = (ml_array_t *)Args[1]; \
	int Degree = Source->Degree; \
	ml_array_t *Target = ml_array_new(Source->Format, Degree); \
	int DataSize = sizeof(CTYPE); \
	for (int I = Degree; --I >= 0;) { \
		Target->Dimensions[I].Stride = DataSize; \
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	CTYPE *Values = Target->Base.Address = GC_MALLOC_ATOMIC(DataSize * sizeof(CTYPE)); \
	set_array_ ## CTYPE ## _ ## CTYPE(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address); \
	int Value = ml_integer_value(Args[0]); \
	for (int I = DataSize / sizeof(CTYPE); --I >= 0; ++Values) *Values = Value OP *Values; \
	return (ml_value_t *)Target; \
} \
\
ML_METHOD(#OP, ATYPE, MLRealT) { \
	ml_array_t *Source = (ml_array_t *)Args[0]; \
	int Degree = Source->Degree; \
	ml_array_t *Target = ml_array_new(Source->Format, Degree); \
	int DataSize = sizeof(CTYPE); \
	for (int I = Degree; --I >= 0;) { \
		Target->Dimensions[I].Stride = DataSize; \
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	CTYPE *Values = Target->Base.Address = GC_MALLOC_ATOMIC(DataSize * sizeof(CTYPE)); \
	set_array_ ## CTYPE ## _ ## CTYPE(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address); \
	double Value = ml_real_value(Args[1]); \
	for (int I = DataSize / sizeof(CTYPE); --I >= 0; ++Values) *Values = *Values OP Value; \
	return (ml_value_t *)Target; \
} \
\
ML_METHOD(#OP, MLRealT, ATYPE) { \
	ml_array_t *Source = (ml_array_t *)Args[1]; \
	int Degree = Source->Degree; \
	ml_array_t *Target = ml_array_new(Source->Format, Degree); \
	int DataSize = sizeof(CTYPE); \
	for (int I = Degree; --I >= 0;) { \
		Target->Dimensions[I].Stride = DataSize; \
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	CTYPE *Values = Target->Base.Address = GC_MALLOC_ATOMIC(DataSize); \
	set_array_ ## CTYPE ## _ ## CTYPE(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address); \
	double Value = ml_real_value(Args[0]); \
	for (int I = DataSize / sizeof(CTYPE); --I >= 0; ++Values) *Values = Value OP *Values; \
	return (ml_value_t *)Target; \
}

#define ARITH_METHODS(ATYPE, CTYPE) \
	ARITH_METHOD(ATYPE, CTYPE, +) \
	ARITH_METHOD(ATYPE, CTYPE, -) \
	ARITH_METHOD(ATYPE, CTYPE, *) \
	ARITH_METHOD(ATYPE, CTYPE, /)

#define METHODS(ATYPE, CTYPE, FORMAT, RFUNC, RNEW) \
\
static void append_array0_ ## CTYPE(ml_stringbuffer_t *Buffer, ml_array_dimension_t *Dimension, void *Address) { \
	ml_stringbuffer_add(Buffer, "[", 1); \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Size) { \
			ml_stringbuffer_addf(Buffer, FORMAT, *(CTYPE *)(Address + (Indices[0]) * Dimension->Stride)); \
			for (int I = 1; I < Dimension->Size; ++I) { \
				ml_stringbuffer_addf(Buffer, ", "FORMAT, *(CTYPE *)(Address + (Indices[I]) * Dimension->Stride)); \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		ml_stringbuffer_addf(Buffer, FORMAT, *(CTYPE *)Address); \
		Address += Stride; \
		for (int I = Dimension->Size; --I > 0;) { \
			ml_stringbuffer_addf(Buffer, ", "FORMAT, *(CTYPE *)Address); \
			Address += Stride; \
		} \
	} \
	ml_stringbuffer_add(Buffer, "]", 1); \
} \
\
static void append_array_ ## CTYPE(ml_stringbuffer_t *Buffer, int Degree, ml_array_dimension_t *Dimension, void *Address) { \
	if (Degree == 1) { \
		append_array0_ ## CTYPE(Buffer, Dimension, Address); \
		return; \
	} \
	ml_stringbuffer_add(Buffer, "[", 1); \
	int Stride = Dimension->Stride; \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Size) { \
			append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address + (Indices[0]) * Dimension->Stride); \
			for (int I = 1; I < Dimension->Size; ++I) { \
				ml_stringbuffer_add(Buffer, ", ", 2); \
				append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address + (Indices[I]) * Dimension->Stride); \
			} \
		} \
	} else { \
		append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address); \
		Address += Stride; \
		for (int I = Dimension->Size; --I > 0;) { \
			ml_stringbuffer_add(Buffer, ", ", 2); \
			append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address); \
			Address += Stride; \
		} \
	} \
	ml_stringbuffer_add(Buffer, "]", 1); \
} \
\
ML_METHOD("string", ATYPE) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	if (Array->Degree == 0) { \
		return ml_string_format(FORMAT, *(CTYPE *)Array->Base.Address); \
	} else { \
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT}; \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Address); \
		return ml_stringbuffer_get_string(Buffer); \
	} \
} \
\
ML_METHOD("append", MLStringBufferT, ATYPE) { \
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0]; \
	ml_array_t *Array = (ml_array_t *)Args[1]; \
	if (Array->Degree == 0) { \
		ml_stringbuffer_addf(Buffer, FORMAT, *(CTYPE *)Array->Base.Address); \
	} else { \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Address); \
	} \
	return Args[0]; \
} \
\
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, set, =); \
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, add, +=); \
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, sub, -=); \
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, mul, *=); \
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, div, /=); \
\
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayInt8T, int8_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayUInt8T, uint8_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayInt16T, int16_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayUInt16T, uint16_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayInt32T, int32_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayUInt32T, uint32_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayInt64T, int64_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayUInt64T, uint64_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayFloat32T, float); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayFloat64T, double); \
\
static ml_value_t *ml_array_ ## CTYPE ## _assign(ml_array_t *Target, ml_value_t *Value) { \
	Value = Value->Type->deref(Value); \
	if (Value->Type == MLErrorT) { \
		return Value; \
	} else if (ml_is(Value, MLNumberT)) { \
		CTYPE CValue = RFUNC(Value); \
		if (Target->Degree == 0) { \
			*(CTYPE *)Target->Base.Address = CValue; \
		} else { \
			set_value_array_ ## CTYPE(Target->Degree, Target->Dimensions, Target->Base.Address, CValue); \
		} \
		return Value; \
	SETTER_CASE(CTYPE, MLArrayInt8T, int8_t) \
	SETTER_CASE(CTYPE, MLArrayInt16T, int16_t) \
	SETTER_CASE(CTYPE, MLArrayInt32T, int32_t) \
	SETTER_CASE(CTYPE, MLArrayInt64T, int64_t) \
	SETTER_CASE(CTYPE, MLArrayUInt8T, uint8_t) \
	SETTER_CASE(CTYPE, MLArrayUInt16T, uint16_t) \
	SETTER_CASE(CTYPE, MLArrayUInt32T, uint32_t) \
	SETTER_CASE(CTYPE, MLArrayUInt64T, uint64_t) \
	SETTER_CASE(CTYPE, MLArrayFloat32T, float) \
	SETTER_CASE(CTYPE, MLArrayFloat64T, double) \
	} else { \
		return ml_error("TypeError", "Cannot assign %s to array", Value->Type->Name); \
	} \
} \
\
static int set_function_array_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, void *Address, call_info_t *Info) { \
	if (Degree == 0) { \
		Info->Args[0] = RNEW(*(CTYPE *)Address); \
		Info->Result = ml_call(Info->Function, Info->Count, Info->Args);\
		if (Info->Result->Type == MLErrorT) return 1; \
		if (Info->Result != MLNil) *(CTYPE *)Address = RFUNC(Info->Result); \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Info->Args[Degree] = ml_integer(Indices[I] + 1); \
				if (set_function_array_ ## CTYPE(Degree - 1, Dimension + 1, Address + (Indices[I]) * Dimension->Stride, Info)) { \
					return 1; \
				} \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Info->Args[Degree] = ml_integer(I + 1); \
				if (set_function_array_ ## CTYPE(Degree - 1, Dimension + 1, Address, Info)) { \
					return 1; \
				} \
				Address += Stride; \
			} \
		} \
	} \
	return 0; \
} \
\
ML_METHOD("update", ATYPE, MLAnyT) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	if (Array->Degree == 0) { \
		ml_value_t *Result = ml_inline(Args[1], 1, RNEW(*(CTYPE *)Array->Base.Address)); \
		if (Result->Type == MLErrorT) return Result; \
		if (Result != MLNil) *(CTYPE *)Array->Base.Address = RFUNC(Result); \
	} else { \
		call_info_t *Info = xnew(call_info_t, Array->Degree + 1, ml_value_t *); \
		Info->Count = Array->Degree + 1; \
		Info->Function = Args[1]; \
		if (set_function_array_ ## CTYPE(Array->Degree, Array->Dimensions, Array->Base.Address, Info)) { \
			return Info->Result; \
		} \
	} \
	return Args[0]; \
}\
\
ML_METHOD("copy", ATYPE) { \
	ml_array_t *Source = (ml_array_t *)Args[0]; \
	int Degree = Source->Degree; \
	ml_array_t *Target = ml_array_new(Source->Format, Degree); \
	int DataSize = sizeof(CTYPE); \
	for (int I = Degree; --I >= 0;) { \
		Target->Dimensions[I].Stride = DataSize; \
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	Target->Base.Address = GC_MALLOC_ATOMIC(DataSize); \
	if (Degree == 0) { \
		*(CTYPE *)Target->Base.Address = *(CTYPE *)Source->Base.Address; \
	} else { \
		set_array_ ## CTYPE ## _ ## CTYPE(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address); \
	} \
	return (ml_value_t *)Target; \
} \
\
ARITH_METHODS(ATYPE, CTYPE) \
\
ML_METHOD("get", ATYPE) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	return RNEW(*(CTYPE *)Array->Base.Address); \
} \
\
void partial_sums_ ## CTYPE(int Target, int Degree, ml_array_dimension_t *Dimension, void *Address, int LastRow) { \
	if (Degree == 0) { \
		*(CTYPE *)Address += *(CTYPE *)(Address - LastRow); \
	} else if (Target == Degree) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 1; I < Dimension->Size; ++I) { \
				partial_sums_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address + Indices[I] * Stride, (Indices[I] - Indices[I - 1]) * Stride); \
			} \
		} else { \
			for (int I = 1; I < Dimension->Size; ++I) { \
				Address += Stride; \
				partial_sums_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address, Stride); \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				partial_sums_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address + Indices[I] * Stride, LastRow); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				partial_sums_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address, LastRow); \
				Address += Stride; \
			} \
		} \
	} \
} \
\
ML_METHOD("partial_sums", ATYPE, MLIntegerT) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	int Target = ml_integer_value(Args[1]); \
	if (Target <= 0) Target += Array->Degree + 1; \
	if (Target < 1 || Target > Array->Degree) return ml_error("Error", "Dimension index invalid"); \
	Target = Array->Degree + 1 - Target; \
	partial_sums_ ## CTYPE(Target, Array->Degree, Array->Dimensions, Array->Base.Address, 0); \
	return Args[0]; \
} \
\
static ml_value_t *ml_array_ ## CTYPE ## _value(ml_array_t *Array, void *Address) { \
	return RNEW(*(CTYPE *)Array->Base.Address); \
} \
\
CTYPE ml_array_get_ ## CTYPE (ml_value_t *Value, int Indices[]) { \
	ml_array_t *Array = (ml_array_t *)Value; \
	ml_array_dimension_t *Dimension = Array->Dimensions; \
	char *Address = Array->Base.Address; \
	for (int I = 0; I < Array->Degree; ++I) { \
		int Index = Indices[I]; \
		if (Index < 0 || Index >= Dimension->Size) return 0; \
		if (Dimension->Indices) { \
			Address += Dimension->Stride * Dimension->Indices[Index]; \
		} else { \
			Address += Dimension->Stride * Index; \
		} \
		++Dimension; \
	} \
	return *(CTYPE *)Address; \
}

static ml_value_t *CopyMethod;

#define ML_ARITH_METHOD(OP, NAME) \
static ml_value_t *NAME ## Method; \
ML_METHOD(#OP, MLArrayT, MLArrayT) { \
	Args[0] = ml_call(CopyMethod, 1, Args); \
	return ml_call(NAME ## Method, 2, Args); \
}

ML_ARITH_METHOD(+, Add);
ML_ARITH_METHOD(-, Sub);
ML_ARITH_METHOD(*, Mul);
ML_ARITH_METHOD(/, Div);

METHODS(MLArrayInt8T, int8_t, "%d", ml_integer_value, ml_integer);
METHODS(MLArrayUInt8T, uint8_t, "%ud", ml_integer_value, ml_integer);
METHODS(MLArrayInt16T, int16_t, "%d", ml_integer_value, ml_integer);
METHODS(MLArrayUInt16T, uint16_t, "%ud", ml_integer_value, ml_integer);
METHODS(MLArrayInt32T, int32_t, "%d", ml_integer_value, ml_integer);
METHODS(MLArrayUInt32T, uint32_t, "%ud", ml_integer_value, ml_integer);
METHODS(MLArrayInt64T, int64_t, "%ld", ml_integer_value, ml_integer);
METHODS(MLArrayUInt64T, uint64_t, "%lud", ml_integer_value, ml_integer);
METHODS(MLArrayFloat32T, float, "%f", ml_real_value, ml_real);
METHODS(MLArrayFloat64T, double, "%f", ml_real_value, ml_real);

#define TYPES(ATYPE, CTYPE) { \
	MLArray ## ATYPE ## T = ml_type(MLArrayT, #CTYPE "-array"); \
	MLArray ## ATYPE ## T->assign = (void *)ml_array_ ## CTYPE ## _assign; \
	ml_map_insert(Array, ml_string(#ATYPE "T", -1), (ml_value_t *)MLArray ##ATYPE ## T); \
	ml_typed_fn_set(MLArray ## ATYPE ## T, ml_array_value, ml_array_ ## CTYPE ## _value); \
}

#ifdef USE_ML_CBOR

#include "ml_cbor.h"

static void ml_cbor_write_array_dim(int Degree, ml_array_dimension_t *Dimension, void *Address, void *Data, ml_cbor_write_fn WriteFn) {
	if (Degree < 0) {
		WriteFn(Data, Address, Dimension->Size * Dimension->Stride);
	} else {
		int Stride = Dimension->Stride;
		if (Dimension->Indices) {
			int *Indices = Dimension->Indices;
			for (int I = 0; I < Dimension->Size; ++I) {
				ml_cbor_write_array_dim(Degree - 1, Dimension + 1, Address + (Indices[I]) * Dimension->Stride, Data, WriteFn);
			}
		} else {
			for (int I = Dimension->Size; --I >= 0;) {
				ml_cbor_write_array_dim(Degree - 1, Dimension + 1, Address, Data, WriteFn);
				Address += Stride;
			}
		}
	}
}

static void ml_cbor_write_array_fn(ml_array_t *Array, void *Data, ml_cbor_write_fn WriteFn) {
	static uint64_t Tags[] = {
		[ML_ARRAY_FORMAT_ANY] = 41,
		[ML_ARRAY_FORMAT_I8] = 72,
		[ML_ARRAY_FORMAT_U8] = 64,
		[ML_ARRAY_FORMAT_I16] = 77,
		[ML_ARRAY_FORMAT_U16] = 69,
		[ML_ARRAY_FORMAT_I32] = 78,
		[ML_ARRAY_FORMAT_U32] = 70,
		[ML_ARRAY_FORMAT_I64] = 79,
		[ML_ARRAY_FORMAT_U64] = 71,
		[ML_ARRAY_FORMAT_F32] = 85,
		[ML_ARRAY_FORMAT_F64] = 86
	};
	static size_t Sizes[] = {
		[ML_ARRAY_FORMAT_ANY] = 0,
		[ML_ARRAY_FORMAT_I8] = sizeof(int8_t),
		[ML_ARRAY_FORMAT_U8] = sizeof(uint8_t),
		[ML_ARRAY_FORMAT_I16] = sizeof(int16_t),
		[ML_ARRAY_FORMAT_U16] = sizeof(uint16_t),
		[ML_ARRAY_FORMAT_I32] = sizeof(int32_t),
		[ML_ARRAY_FORMAT_U32] = sizeof(uint32_t),
		[ML_ARRAY_FORMAT_I64] = sizeof(int64_t),
		[ML_ARRAY_FORMAT_U64] = sizeof(uint64_t),
		[ML_ARRAY_FORMAT_F32] = sizeof(float),
		[ML_ARRAY_FORMAT_F64] = sizeof(double)
	};
	ml_cbor_write_tag(Data, WriteFn, 40);
	ml_cbor_write_array(Data, WriteFn, 2);
	ml_cbor_write_array(Data, WriteFn, Array->Degree);
	for (int I = 0; I < Array->Degree; ++I) ml_cbor_write_integer(Data, WriteFn, Array->Dimensions[I].Size);
	size_t Size = Sizes[Array->Format];
	int FlatDegree = -1;
	for (int I = Array->Degree; --I >= 0;) {
		if (FlatDegree < 0) {
			if (Array->Dimensions[I].Indices) FlatDegree = I;
			if (Array->Dimensions[I].Stride != Size) FlatDegree = I;
		}
		Size *= Array->Dimensions[I].Size;
	}
	ml_cbor_write_tag(Data, WriteFn, Tags[Array->Format]);
	ml_cbor_write_bytes(Data, WriteFn, Size);
	ml_cbor_write_array_dim(FlatDegree, Array->Dimensions, Array->Base.Address, Data, WriteFn);
}
#endif

void ml_array_init(stringmap_t *Globals) {
	CopyMethod = ml_method("copy");
	AddMethod = ml_method("add");
	SubMethod = ml_method("sub");
	MulMethod = ml_method("mul");
	DivMethod = ml_method("div");
	ml_value_t *Array = ml_map();
	MLArrayT = ml_type(MLBufferT, "array");
	ml_map_insert(Array, ml_string("T", -1), (ml_value_t *)MLArrayT);
	MLArrayAnyT = ml_type(MLArrayT, "value-array");
	ml_map_insert(Array, ml_string("AnyT", -1), (ml_value_t *)MLArrayAnyT);
	TYPES(Int8, int8_t);
	TYPES(Int16, int16_t);
	TYPES(Int32, int32_t);
	TYPES(Int64, int64_t);
	TYPES(UInt8, uint8_t);
	TYPES(UInt16, uint16_t);
	TYPES(UInt32, uint32_t);
	TYPES(UInt64, uint64_t);
	TYPES(Float32, float);
	TYPES(Float64, double);
	if (Globals) {
		ml_map_insert(Array, ml_string("new", -1), ml_functionx(NULL, ml_array_new_fnx));
		ml_map_insert(Array, ml_string("wrap", -1), ml_function(NULL, ml_array_wrap_fn));
		stringmap_insert(Globals, "array", Array);
	}
#ifdef USE_ML_CBOR
	ml_typed_fn_set(MLArrayT, ml_cbor_write, ml_cbor_write_array_fn);
#endif
#include "ml_array_init.c"
}

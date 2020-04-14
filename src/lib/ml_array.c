#include "ml_array.h"
#include "../ml_macros.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <libgen.h>

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

size_t MLArraySizes[] = {
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

ml_array_t *ml_array(ml_array_format_t Format, int Degree, ...) {
	ml_array_t *Array = ml_array_new(Format, Degree);
	int DataSize = MLArraySizes[Format];
	va_list Sizes;
	va_start(Sizes, Degree);
	for (int I = Array->Degree; --I >= 0;) {
		Array->Dimensions[I].Stride = DataSize;
		int Size = Array->Dimensions[I].Size = va_arg(Sizes, int);
		DataSize *= Size;
	}
	va_end(Sizes);
	Array->Base.Address = GC_MALLOC_ATOMIC(DataSize);
	Array->Base.Size = DataSize;
	return Array;
}

typedef struct ml_array_init_state_t {
	ml_state_t Base;
	char *Address;
	ml_array_t *Array;
	ml_value_t *Function;
	ml_value_t *Args[];
} ml_array_init_state_t;

static void ml_array_init_run(ml_array_init_state_t *State, ml_value_t *Value) {
	Value = Value->Type->deref(Value);
	if (Value->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Value);
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
			return State->Function->Type->call((ml_state_t *)State, State->Function, Array->Degree, State->Args);
		} else {
			State->Args[I] = ml_integer(1);
		}
	}
	ML_CONTINUE(State->Base.Caller, Array);
}

static void ml_array_new_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(2);
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
		ML_RETURN(ml_error("TypeError", "Unknown type for array"));
	}
	ml_array_t *Array;
	if (Args[1]->Type == MLListT) {
		int Degree = ml_list_length(Args[1]);
		Array = ml_array_new(Format, Degree);
		ml_list_node_t *Node = ml_list_head(Args[1]);
		for (int I = 0; I < Degree; ++I, Node = Node->Next) {
			if (Node->Value->Type != MLIntegerT) ML_RETURN(ml_error("TypeError", "Dimension is not an integer"));
			Array->Dimensions[I].Size = ml_integer_value(Node->Value);
		}
	} else {
		int Degree = Count - 1;
		Array = ml_array_new(Format, Degree);
		for (int I = 1; I < Count; ++I) {
			ML_CHECKX_ARG_TYPE(I, MLIntegerT);
			Array->Dimensions[I - 1].Size = ml_integer_value(Args[I]);
		}
	}
	int DataSize = MLArraySizes[Format];
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
	InitState->Base.Context = Caller->Context;
	InitState->Address = Array->Base.Address;
	InitState->Array = Array;
	ml_value_t *Function = InitState->Function = Args[2];
	for (int I = 0; I < Array->Degree; ++I) InitState->Args[I] = ml_integer(1);
	return Function->Type->call((ml_state_t *)InitState, Function, Array->Degree, InitState->Args);
}

ml_value_t *ml_array_wrap_fn(void *Data, int Count, ml_value_t **Args) {
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
	return Array;
}

ML_METHOD("shape", MLArrayT) {
	ml_array_t *Array = (ml_array_t *)Args[0];
	ml_value_t *Shape = ml_list();
	for (int I = 0; I < Array->Degree; ++I) {
		ml_list_put(Shape, ml_integer(Array->Dimensions[I].Size));
	}
	return Shape;
}

ML_METHOD("strides", MLArrayT) {
	ml_array_t *Array = (ml_array_t *)Args[0];
	ml_value_t *Strides = ml_list();
	for (int I = 0; I < Array->Degree; ++I) {
		ml_list_put(Strides, ml_integer(Array->Dimensions[I].Stride));
	}
	return Strides;
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
	return Target;
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
	return Target;
}

typedef struct ml_integer_range_t {
	const ml_type_t *Type;
	long Start, Limit, Step;
} ml_integer_range_t;

extern ml_type_t MLIntegerRangeT[1];
static ML_METHOD_DECL(Range, "..");
static ML_METHOD_DECL(Symbol, "::");

static ml_value_t *ml_array_value(ml_array_t *Array, char *Address) {
	typeof(ml_array_value) *function = ml_typed_fn_get(Array->Base.Type, ml_array_value);
	return function(Array, Address);
}

static ml_value_t *ml_array_index(ml_array_t *Source, int Count, ml_value_t **Indices) {
	ml_array_dimension_t TargetDimensions[Source->Degree];
	ml_array_dimension_t *TargetDimension = TargetDimensions;
	ml_array_dimension_t *SourceDimension = Source->Dimensions;
	ml_array_dimension_t *Limit = SourceDimension + Source->Degree;
	char *Address = Source->Base.Address;
	int Min, Max, Step, I;
	for (I = 0; I < Count; ++I) {
		ml_value_t *Index = Indices[I];
		if (Index == RangeMethod) {
			ml_array_dimension_t *Skip = Limit - (Count - (I + 1));
			if (Skip > Limit) return ml_error("RangeError", "Too many indices");
			while (SourceDimension < Skip) {
				*TargetDimension = *SourceDimension;
				++TargetDimension;
				++SourceDimension;
			}
			continue;
		}
		if (SourceDimension >= Limit) return ml_error("RangeError", "Too many indices");
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
			return ml_error("TypeError", "Unknown index type: %s", Index->Type->Name);
		}
		++SourceDimension;
	}
	while (SourceDimension < Limit) {
		*TargetDimension = *SourceDimension;
		++TargetDimension;
		++SourceDimension;
	}
	int Degree = TargetDimension - TargetDimensions;
	ml_array_t *Target = ml_array_new(Source->Format, Degree);
	for (int I = 0; I < Degree; ++I) Target->Dimensions[I] = TargetDimensions[I];
	Target->Base.Address = Address;
	return Target;
}

ML_METHOD("[]", MLArrayT) {
	ml_array_t *Source = (ml_array_t *)Args[0];
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

static ml_value_t *ml_array_of_fn(void *Data, int Count, ml_value_t **Args);

#define UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, NAME, OP) \
\
static void NAME ## _array_suffix_ ## CTYPE1 ## _ ## CTYPE2(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData) { \
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
static void NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(ml_array_dimension_t *TargetDimension, char *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, char *SourceData) { \
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
static void NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(int PrefixDegree, ml_array_dimension_t *TargetDimension, char *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, char *SourceData) { \
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
static void NAME ## _value_array0_ ## CTYPE(ml_array_dimension_t *Dimension, char *Address, CTYPE Value) { \
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
static void NAME ## _value_array_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, char *Address, CTYPE Value) { \
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

#define METHODS(ATYPE, CTYPE, FORMAT, RFUNC, RNEW) \
\
static void append_array_ ## CTYPE(ml_stringbuffer_t *Buffer, int Degree, ml_array_dimension_t *Dimension, char *Address) { \
	ml_stringbuffer_add(Buffer, "[", 1); \
	int Stride = Dimension->Stride; \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Size) { \
			if (Degree == 1) { \
				ml_stringbuffer_addf(Buffer, FORMAT, *(CTYPE *)(Address + (Indices[0]) * Dimension->Stride)); \
				for (int I = 1; I < Dimension->Size; ++I) { \
					ml_stringbuffer_addf(Buffer, ", "FORMAT, *(CTYPE *)(Address + (Indices[I]) * Stride)); \
				} \
			} else { \
				append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address + (Indices[0]) * Dimension->Stride); \
				for (int I = 1; I < Dimension->Size; ++I) { \
					ml_stringbuffer_add(Buffer, ", ", 2); \
					append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address + (Indices[I]) * Dimension->Stride); \
				} \
			} \
		} \
	} else { \
		if (Degree == 1) { \
			ml_stringbuffer_addf(Buffer, FORMAT, *(CTYPE *)Address); \
			Address += Stride; \
			for (int I = Dimension->Size; --I > 0;) { \
				ml_stringbuffer_addf(Buffer, ", "FORMAT, *(CTYPE *)Address); \
				Address += Stride; \
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
	} \
	ml_stringbuffer_add(Buffer, "]", 1); \
} \
\
static ml_value_t *ML_TYPED_FN(ml_string_of, ATYPE, ml_array_t *Array) { \
	if (Array->Degree == 0) { \
		return ml_string_format(FORMAT, *(CTYPE *)Array->Base.Address); \
	} else { \
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT}; \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Address); \
		return ml_stringbuffer_get_string(Buffer); \
	} \
} \
\
ML_METHOD(MLStringOfMethod, ATYPE) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	if (Array->Degree == 0) { \
		return ml_string_format(FORMAT, *(CTYPE *)Array->Base.Address); \
	} else { \
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT}; \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Address); \
		return ml_stringbuffer_get_string(Buffer); \
	} \
} \
static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, ATYPE, ml_stringbuffer_t *Buffer, ml_array_t *Array) { \
	if (Array->Degree == 0) { \
		ml_stringbuffer_addf(Buffer, FORMAT, *(CTYPE *)Array->Base.Address); \
	} else { \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Address); \
	} \
	return Buffer; \
} \
\
ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, ATYPE) { \
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
static ml_value_t *ml_array_ ## CTYPE ## _deref(ml_array_t *Target, ml_value_t *Value) { \
	if (Target->Degree == 0)  return RNEW(*(CTYPE *)Target->Base.Address); \
	return Target; \
} \
\
static ml_value_t *ml_array_ ## CTYPE ## _assign(ml_array_t *Target, ml_value_t *Value) { \
	Value = Value->Type->deref(Value); \
	for (;;) if (Value->Type == MLErrorT) { \
		return Value; \
	} else if (ml_is(Value, MLNumberT)) { \
		CTYPE CValue = RFUNC(Value); \
		set_value_array_ ## CTYPE(Target->Degree, Target->Dimensions, Target->Base.Address, CValue); \
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
		Value = ml_array_of_fn(NULL, 1, &Value); \
	} \
} \
\
static int set_function_array_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, char *Address, call_info_t *Info) { \
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
} \
\
void partial_sums_ ## CTYPE(int Target, int Degree, ml_array_dimension_t *Dimension, char *Address, int LastRow) { \
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
static ml_value_t *ml_array_ ## CTYPE ## _value(ml_array_t *Array, char *Address) { \
	return RNEW(*(CTYPE *)Array->Base.Address); \
} \
\
CTYPE ml_array_get_ ## CTYPE(ml_array_t *Array, ...) { \
	ml_array_dimension_t *Dimension = Array->Dimensions; \
	char *Address = Array->Base.Address; \
	va_list Indices; \
	va_start(Indices, Array); \
	for (int I = 0; I < Array->Degree; ++I) { \
		int Index = va_arg(Indices, int); \
		if (Index < 0 || Index >= Dimension->Size) return 0; \
		if (Dimension->Indices) { \
			Address += Dimension->Stride * Dimension->Indices[Index]; \
		} else { \
			Address += Dimension->Stride * Index; \
		} \
		++Dimension; \
	} \
	va_end(Indices); \
	switch (Array->Format) { \
	case ML_ARRAY_FORMAT_ANY: return RFUNC(*(ml_value_t **)Address); \
	case ML_ARRAY_FORMAT_I8: return *(int8_t *)Address; \
	case ML_ARRAY_FORMAT_U8: return *(uint8_t *)Address; \
	case ML_ARRAY_FORMAT_I16: return *(int16_t *)Address; \
	case ML_ARRAY_FORMAT_U16: return *(uint16_t *)Address; \
	case ML_ARRAY_FORMAT_I32: return *(int32_t *)Address; \
	case ML_ARRAY_FORMAT_U32: return *(uint32_t *)Address; \
	case ML_ARRAY_FORMAT_I64: return *(int64_t *)Address; \
	case ML_ARRAY_FORMAT_U64: return *(uint64_t *)Address; \
	case ML_ARRAY_FORMAT_F32: return *(float *)Address; \
	case ML_ARRAY_FORMAT_F64: return *(double *)Address; \
	} \
	return (CTYPE)0; \
} \
\
void ml_array_set_ ## CTYPE(CTYPE Value, ml_array_t *Array, ...) { \
	ml_array_dimension_t *Dimension = Array->Dimensions; \
	char *Address = Array->Base.Address; \
	va_list Indices; \
	va_start(Indices, Array); \
	for (int I = 0; I < Array->Degree; ++I) { \
		int Index = va_arg(Indices, int); \
		if (Index < 0 || Index >= Dimension->Size) return; \
		if (Dimension->Indices) { \
			Address += Dimension->Stride * Dimension->Indices[Index]; \
		} else { \
			Address += Dimension->Stride * Index; \
		} \
		++Dimension; \
	} \
	va_end(Indices); \
	switch (Array->Format) { \
	case ML_ARRAY_FORMAT_ANY: *(ml_value_t **)Address = RNEW(Value); break; \
	case ML_ARRAY_FORMAT_I8: *(int8_t *)Address = Value; break; \
	case ML_ARRAY_FORMAT_U8: *(uint8_t *)Address = Value; break; \
	case ML_ARRAY_FORMAT_I16: *(int16_t *)Address = Value; break; \
	case ML_ARRAY_FORMAT_U16: *(uint16_t *)Address = Value; break; \
	case ML_ARRAY_FORMAT_I32: *(int32_t *)Address = Value; break; \
	case ML_ARRAY_FORMAT_U32: *(uint32_t *)Address = Value; break; \
	case ML_ARRAY_FORMAT_I64: *(int64_t *)Address = Value; break; \
	case ML_ARRAY_FORMAT_U64: *(uint64_t *)Address = Value; break; \
	case ML_ARRAY_FORMAT_F32: *(float *)Address = Value; break; \
	case ML_ARRAY_FORMAT_F64: *(double *)Address = Value; break; \
	} \
} \

#define ML_COPY_CASE(CTYPE) \
	switch (C->Format) { \
	case ML_ARRAY_FORMAT_ANY: return ml_error("ImplementationError", "Not implemented yet!"); \
	case ML_ARRAY_FORMAT_I8: set_array_int8_t_ ## CTYPE(C->Dimensions, C->Base.Address, Degree, A->Dimensions, A->Base.Address); break; \
	case ML_ARRAY_FORMAT_U8: set_array_uint8_t_ ## CTYPE(C->Dimensions, C->Base.Address, Degree, A->Dimensions, A->Base.Address); break; \
	case ML_ARRAY_FORMAT_I16: set_array_int16_t_ ## CTYPE(C->Dimensions, C->Base.Address, Degree, A->Dimensions, A->Base.Address); break; \
	case ML_ARRAY_FORMAT_U16: set_array_uint16_t_ ## CTYPE(C->Dimensions, C->Base.Address, Degree, A->Dimensions, A->Base.Address); break; \
	case ML_ARRAY_FORMAT_I32: set_array_int32_t_ ## CTYPE(C->Dimensions, C->Base.Address, Degree, A->Dimensions, A->Base.Address); break; \
	case ML_ARRAY_FORMAT_U32: set_array_uint32_t_ ## CTYPE(C->Dimensions, C->Base.Address, Degree, A->Dimensions, A->Base.Address); break; \
	case ML_ARRAY_FORMAT_I64: set_array_int64_t_ ## CTYPE(C->Dimensions, C->Base.Address, Degree, A->Dimensions, A->Base.Address); break; \
	case ML_ARRAY_FORMAT_U64: set_array_uint64_t_ ## CTYPE(C->Dimensions, C->Base.Address, Degree, A->Dimensions, A->Base.Address); break; \
	case ML_ARRAY_FORMAT_F32: set_array_float_ ## CTYPE(C->Dimensions, C->Base.Address, Degree, A->Dimensions, A->Base.Address); break; \
	case ML_ARRAY_FORMAT_F64: set_array_double_ ## CTYPE(C->Dimensions, C->Base.Address, Degree, A->Dimensions, A->Base.Address); break; \
	}

#define ML_ARITH_COPY \
	int DataSize = MLArraySizes[C->Format]; \
	for (int I = Degree; --I >= 0;) { \
		C->Dimensions[I].Stride = DataSize; \
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	C->Base.Address = GC_MALLOC_ATOMIC(DataSize); \
	switch (A->Format) { \
	case ML_ARRAY_FORMAT_ANY: \
		return ml_error("ImplementationError", "Not implemented yet!"); \
		break; \
	case ML_ARRAY_FORMAT_I8: ML_COPY_CASE(int8_t); break; \
	case ML_ARRAY_FORMAT_U8: ML_COPY_CASE(uint8_t); break; \
	case ML_ARRAY_FORMAT_I16: ML_COPY_CASE(int16_t); break; \
	case ML_ARRAY_FORMAT_U16: ML_COPY_CASE(uint16_t); break; \
	case ML_ARRAY_FORMAT_I32: ML_COPY_CASE(int32_t); break; \
	case ML_ARRAY_FORMAT_U32: ML_COPY_CASE(uint32_t); break; \
	case ML_ARRAY_FORMAT_I64: ML_COPY_CASE(int64_t); break; \
	case ML_ARRAY_FORMAT_U64: ML_COPY_CASE(uint64_t); break; \
	case ML_ARRAY_FORMAT_F32: ML_COPY_CASE(float); break; \
	case ML_ARRAY_FORMAT_F64: ML_COPY_CASE(double); break; \
	} \

#define MAX(X, Y) ((X > Y) ? X : Y)

#define ML_ARITH_METHOD(OP, NAME) \
static ML_METHOD_DECL(NAME, #OP); \
\
ML_METHOD(#OP, MLArrayT, MLArrayT) { \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	ml_array_t *B = (ml_array_t *)Args[1]; \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(MAX(A->Format, B->Format), Degree); \
	ML_ARITH_COPY \
	Args[0] = (ml_value_t *)C; \
	return ml_call(NAME ## Method, 2, Args); \
} \
\
ML_METHOD(#OP, MLArrayT, MLIntegerT) { \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	int64_t B = ml_integer_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(MAX(A->Format, ML_ARRAY_FORMAT_I64), Degree); \
	ML_ARITH_COPY \
	switch (C->Format) { \
	case ML_ARRAY_FORMAT_I64: { \
		int64_t *Values = (int64_t *)C->Base.Address; \
		for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = *Values OP B; \
		break; \
	} \
	case ML_ARRAY_FORMAT_U64: { \
		uint64_t *Values = (uint64_t *)C->Base.Address; \
		for (int I = DataSize / sizeof(uint64_t); --I >= 0; ++Values) *Values = *Values OP B; \
		break; \
	} \
	case ML_ARRAY_FORMAT_F32: { \
		float *Values = (float *)C->Base.Address; \
		for (int I = DataSize / sizeof(float); --I >= 0; ++Values) *Values = *Values OP B; \
		break; \
	} \
	case ML_ARRAY_FORMAT_F64: { \
		double *Values = (double *)C->Base.Address; \
		for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = *Values OP B; \
		break; \
	} \
	} \
	return C; \
} \
\
ML_METHOD(#OP, MLIntegerT, MLArrayT) { \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	int64_t B = ml_integer_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(MAX(A->Format, ML_ARRAY_FORMAT_I64), Degree); \
	ML_ARITH_COPY \
	switch (C->Format) { \
	case ML_ARRAY_FORMAT_I64: { \
		int64_t *Values = (int64_t *)C->Base.Address; \
		for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = B OP *Values; \
		break; \
	} \
	case ML_ARRAY_FORMAT_U64: { \
		uint64_t *Values = (uint64_t *)C->Base.Address; \
		for (int I = DataSize / sizeof(uint64_t); --I >= 0; ++Values) *Values = B OP *Values; \
		break; \
	} \
	case ML_ARRAY_FORMAT_F32: { \
		float *Values = (float *)C->Base.Address; \
		for (int I = DataSize / sizeof(float); --I >= 0; ++Values) *Values = B OP *Values; \
		break; \
	} \
	case ML_ARRAY_FORMAT_F64: { \
		double *Values = (double *)C->Base.Address; \
		for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = B OP *Values; \
		break; \
	} \
	} \
	return C; \
} \
\
ML_METHOD(#OP, MLArrayT, MLRealT) { \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	double B = ml_integer_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(ML_ARRAY_FORMAT_F64, Degree); \
	ML_ARITH_COPY \
	double *Values = (double *)C->Base.Address; \
	for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = *Values OP B; \
	return C; \
} \
\
ML_METHOD(#OP, MLRealT, MLArrayT) { \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	double B = ml_integer_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(ML_ARRAY_FORMAT_F64, Degree); \
	ML_ARITH_COPY \
	double *Values = (double *)C->Base.Address; \
	for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = B OP *Values; \
	return C; \
}

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

ML_ARITH_METHOD(+, Add);
ML_ARITH_METHOD(-, Sub);
ML_ARITH_METHOD(*, Mul);
ML_ARITH_METHOD(/, Div);

static int ml_array_of_has_real(ml_value_t *Value) {
	if (Value->Type == MLListT) {
		ML_LIST_FOREACH(Value, Node) {
			if (ml_array_of_has_real(Node->Value)) return 1;
		}
	} else if (Value->Type == MLTupleT) {
		ml_tuple_t *Tuple = (ml_tuple_t *)Value;
		for (int I = 0; I < Tuple->Size; ++I) {
			if (ml_array_of_has_real(Tuple->Values[I])) return 1;
		}
	} else if (Value->Type == MLArrayT) {
		ml_array_t *Array = (ml_array_t *)Value;
		if (Array->Format >= ML_ARRAY_FORMAT_F32) return 1;
	} else if (Value->Type == MLRealT) {
		return 1;
	}
	return 0;
}

static ml_array_t *ml_array_of_create(ml_value_t *Value, int Degree, int Real) {
	if (!Real) Real = ml_array_of_has_real(Value);
	if (Value->Type == MLListT) {
		int Size = ml_list_length(Value);
		if (!Size) return (ml_array_t *)ml_error("ValueError", "Empty dimension in array");
		ml_array_t *Array = ml_array_of_create(ml_list_head(Value)->Value, Degree + 1, Real);
		if (Array->Base.Type == MLErrorT) return Array;
		Array->Dimensions[Degree].Size = Size;
		if (Degree < Array->Degree - 1) {
			Array->Dimensions[Degree].Stride = Array->Dimensions[Degree + 1].Size * Array->Dimensions[Degree + 1].Stride;
		}
		return Array;
	} else if (Value->Type == MLTupleT) {
		int Size = ml_tuple_size(Value);
		if (!Size) return (ml_array_t *)ml_error("ValueError", "Empty dimension in array");
		ml_array_t *Array = ml_array_of_create(ml_tuple_get(Value, 0), Degree + 1, Real);
		if (Array->Base.Type == MLErrorT) return Array;
		Array->Dimensions[Degree].Size = Size;
		if (Degree < Array->Degree - 1) {
			Array->Dimensions[Degree].Stride = Array->Dimensions[Degree + 1].Size * Array->Dimensions[Degree + 1].Stride;
		}
		return Array;
	} else if (ml_is(Value, MLArrayT)) {
		ml_array_t *Nested = (ml_array_t *)Value;
		ml_array_t *Array;
		if (Real) {
			Array = ml_array_new(ML_ARRAY_FORMAT_F64, Degree + Nested->Degree);
		} else {
			Array = ml_array_new(ML_ARRAY_FORMAT_I64, Degree + Nested->Degree);
		}
		memcpy(Array->Dimensions + Degree, Nested->Dimensions, Nested->Degree * sizeof(ml_array_dimension_t));
		return Array;
	} else if (Real) {
		ml_array_t *Array = ml_array_new(ML_ARRAY_FORMAT_F64, Degree);
		if (Degree) {
			Array->Dimensions[Degree - 1].Size = 1;
			Array->Dimensions[Degree - 1].Stride = sizeof(double);
		}
		return Array;
	} else {
		ml_array_t *Array = ml_array_new(ML_ARRAY_FORMAT_I64, Degree);
		if (Degree) {
			Array->Dimensions[Degree - 1].Size = 1;
			Array->Dimensions[Degree - 1].Stride = sizeof(int64_t);
		}
		return Array;
	}
}

#define ML_ARRAY_FILL_NESTED(ATYPE, CTYPE) \
	} else if (Value->Type == ATYPE) { \
		ml_array_t *Source = (ml_array_t *)Value; \
		if (Source->Degree != Degree) return ml_error("Error", "Incompatible assignment"); \
		for (int I = 0; I < Degree; ++I) { \
			if (Dimension[I].Size != Source->Dimensions[I].Size) return ml_error("Error", "Incompatible assignment"); \
		} \
		switch (Format) { \
		case ML_ARRAY_FORMAT_ANY: return ml_error("ImplementationError", "Not implemented yet!"); \
		case ML_ARRAY_FORMAT_I8: set_array_int8_t_ ## CTYPE(Dimension, Address, Degree, Source->Dimensions, Source->Base.Address); break; \
		case ML_ARRAY_FORMAT_U8: set_array_uint8_t_ ## CTYPE(Dimension, Address, Degree, Source->Dimensions, Source->Base.Address); break; \
		case ML_ARRAY_FORMAT_I16: set_array_int16_t_ ## CTYPE(Dimension, Address, Degree, Source->Dimensions, Source->Base.Address); break; \
		case ML_ARRAY_FORMAT_U16: set_array_uint16_t_ ## CTYPE(Dimension, Address, Degree, Source->Dimensions, Source->Base.Address); break; \
		case ML_ARRAY_FORMAT_I32: set_array_int32_t_ ## CTYPE(Dimension, Address, Degree, Source->Dimensions, Source->Base.Address); break; \
		case ML_ARRAY_FORMAT_U32: set_array_uint32_t_ ## CTYPE(Dimension, Address, Degree, Source->Dimensions, Source->Base.Address); break; \
		case ML_ARRAY_FORMAT_I64: set_array_int64_t_ ## CTYPE(Dimension, Address, Degree, Source->Dimensions, Source->Base.Address); break; \
		case ML_ARRAY_FORMAT_U64: set_array_uint64_t_ ## CTYPE(Dimension, Address, Degree, Source->Dimensions, Source->Base.Address); break; \
		case ML_ARRAY_FORMAT_F32: set_array_float_ ## CTYPE(Dimension, Address, Degree, Source->Dimensions, Source->Base.Address); break; \
		case ML_ARRAY_FORMAT_F64: set_array_double_ ## CTYPE(Dimension, Address, Degree, Source->Dimensions, Source->Base.Address); break; \
		} \

static ml_value_t *ml_array_of_fill(ml_array_format_t Format, ml_array_dimension_t *Dimension, char *Address, int Degree, ml_value_t *Value) {
	if (Value->Type == MLListT) {
		if (!Degree) return ml_error("ValueError", "Inconsistent depth in array");
		if (ml_list_length(Value) != Dimension->Size) return ml_error("ValueError", "Inconsistent lengths in array");
		ML_LIST_FOREACH(Value, Node) {
			ml_value_t *Error = ml_array_of_fill(Format, Dimension + 1, Address, Degree - 1, Node->Value);
			if (Error) return Error;
			Address += Dimension->Stride;
		}
	} else if (Value->Type == MLTupleT) {
		if (!Degree) return ml_error("ValueError", "Inconsistent depth in array");
		if (ml_tuple_size(Value) != Dimension->Size) return ml_error("ValueError", "Inconsistent lengths in array");
		ml_tuple_t *Tuple = (ml_tuple_t *)Value;
		for (int I = 0; I < Tuple->Size; ++I) {
			ml_value_t *Error = ml_array_of_fill(Format, Dimension + 1, Address, Degree - 1, Tuple->Values[I]);
			if (Error) return Error;
			Address += Dimension->Stride;
		}
	ML_ARRAY_FILL_NESTED(MLArrayInt8T, int8_t)
	ML_ARRAY_FILL_NESTED(MLArrayUInt8T, uint8_t)
	ML_ARRAY_FILL_NESTED(MLArrayInt16T, int16_t)
	ML_ARRAY_FILL_NESTED(MLArrayUInt16T, uint16_t)
	ML_ARRAY_FILL_NESTED(MLArrayInt32T, int32_t)
	ML_ARRAY_FILL_NESTED(MLArrayUInt32T, uint32_t)
	ML_ARRAY_FILL_NESTED(MLArrayInt64T, int64_t)
	ML_ARRAY_FILL_NESTED(MLArrayUInt64T, uint64_t)
	ML_ARRAY_FILL_NESTED(MLArrayFloat32T, float)
	ML_ARRAY_FILL_NESTED(MLArrayFloat64T, double)
	} else {
		if (Degree) return ml_error("ValueError", "Inconsistent depth in array");
		switch (Format) {
		case ML_ARRAY_FORMAT_ANY: *(ml_value_t **)Address = Value; break;
		case ML_ARRAY_FORMAT_I8: *(int8_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_U8: *(uint8_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_I16: *(int16_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_U16: *(uint16_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_I32: *(int32_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_U32: *(uint32_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_I64: *(int64_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_U64: *(uint64_t *)Address = ml_integer_value(Value); break;
		case ML_ARRAY_FORMAT_F32: *(float *)Address = ml_real_value(Value); break;
		case ML_ARRAY_FORMAT_F64: *(double *)Address = ml_real_value(Value); break;
		}
	}
	return NULL;
}

static ml_value_t *ml_array_of_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_array_t *Array = ml_array_of_create(Args[0], 0, 0);
	if (Array->Base.Type == MLErrorT) return Array;
	size_t Size;
	if (Array->Degree) {
		Size = Array->Base.Size = Array->Dimensions[0].Stride * Array->Dimensions[0].Size;
	} else {
		Size = MLArraySizes[Array->Format];
	}
	char *Address = Array->Base.Address = GC_MALLOC_ATOMIC(Size);
	ml_array_of_fill(Array->Format, Array->Dimensions, Address, Array->Degree, Args[0]);
	return Array;
}

ML_METHOD("copy", MLArrayT) {
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	ml_array_t *Target = ml_array_new(Source->Format, Degree);
	int DataSize = MLArraySizes[Source->Format];
	for (int I = Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Address = GC_MALLOC_ATOMIC(DataSize);
	if (Degree == 0) {
		memcpy(Target->Base.Address, Source->Base.Address, DataSize);
	} else switch (Source->Format) {
	case ML_ARRAY_FORMAT_ANY:
	case ML_ARRAY_FORMAT_I8:
		set_array_int8_t_int8_t(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address);
		break;
	case ML_ARRAY_FORMAT_U8:
		set_array_uint8_t_uint8_t(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address);
		break;
	case ML_ARRAY_FORMAT_I16:
		set_array_int16_t_int16_t(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address);
		break;
	case ML_ARRAY_FORMAT_U16:
		set_array_uint16_t_uint16_t(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address);
		break;
	case ML_ARRAY_FORMAT_I32:
		set_array_int32_t_int32_t(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address);
		break;
	case ML_ARRAY_FORMAT_U32:
		set_array_uint32_t_uint32_t(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address);
		break;
	case ML_ARRAY_FORMAT_I64:
		set_array_int64_t_int64_t(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address);
		break;
	case ML_ARRAY_FORMAT_U64:
		set_array_uint64_t_uint64_t(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address);
		break;
	case ML_ARRAY_FORMAT_F32:
		set_array_float_float(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address);
		break;
	case ML_ARRAY_FORMAT_F64:
		set_array_double_double(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address);
		break;
	}
	return Target;
}

#define TYPES(ATYPE, CTYPE) { \
	MLArray ## ATYPE ## T = ml_type(MLArrayT, #CTYPE "-array"); \
	MLArray ## ATYPE ## T->deref = (void *)ml_array_ ## CTYPE ## _deref; \
	MLArray ## ATYPE ## T->assign = (void *)ml_array_ ## CTYPE ## _assign; \
	ml_typed_fn_set(MLArray ## ATYPE ## T, ml_array_value, ml_array_ ## CTYPE ## _value); \
}

#include "ml_cbor.h"

static void ml_cbor_write_array_dim(int Degree, ml_array_dimension_t *Dimension, char *Address, char *Data, ml_cbor_write_fn WriteFn) {
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

static void ML_TYPED_FN(ml_cbor_write, MLArrayT, ml_array_t *Array, char *Data, ml_cbor_write_fn WriteFn) {
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
	ml_cbor_write_tag(Data, WriteFn, 40);
	ml_cbor_write_array(Data, WriteFn, 2);
	ml_cbor_write_array(Data, WriteFn, Array->Degree);
	for (int I = 0; I < Array->Degree; ++I) ml_cbor_write_integer(Data, WriteFn, Array->Dimensions[I].Size);
	size_t Size = MLArraySizes[Array->Format];
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

static ml_value_t *ml_cbor_read_multi_array_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_TYPE(0, MLListT);
	ml_list_node_t *Node = ml_list_head(Args[0]);
	if (!Node) return ml_error("CborError", "Invalid multi-dimensional array");
	ml_value_t *Dimensions = Node->Value;
	if (Dimensions->Type != MLListT) return ml_error("CborError", "Invalid multi-dimensional array");
	Node = Node->Next;
	if (!Node) return ml_error("CborError", "Invalid multi-dimensional array");
	ml_array_t *Source = (ml_array_t *)Node->Value;
	if (!ml_is(Source, MLArrayT)) return ml_error("CborError", "Invalid multi-dimensional array");
	ml_array_t *Target = ml_array_new(Source->Format, ml_list_length(Dimensions));
	ml_array_dimension_t *Dimension = Target->Dimensions + Target->Degree;
	int Stride = MLArraySizes[Source->Format];
	ML_LIST_REVERSE(Dimensions, Node) {
		--Dimension;
		Dimension->Stride = Stride;
		int Size = Dimension->Size = ml_integer_value(Node->Value);
		Stride *= Size;
	}
	if (Stride != Source->Base.Size) return ml_error("CborError", "Invalid multi-dimensional array");
	Target->Base.Size = Stride;
	Target->Base.Address = Source->Base.Address;
	return Target;
}

static ml_value_t *ml_cbor_read_typed_array_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_TYPE(0, MLBufferT);
	ml_buffer_t *Buffer = (ml_buffer_t *)Args[0];
	ml_array_format_t Format = (intptr_t)Data;
	int ItemSize = MLArraySizes[Format];
	ml_array_t *Array = ml_array_new(Format, 1);
	Array->Dimensions[0].Size = Buffer->Size / ItemSize;
	Array->Dimensions[0].Stride = ItemSize;
	Array->Base.Size = Buffer->Size;
	Array->Base.Address = Buffer->Address;
	return Array;
}

void ml_library_entry(ml_value_t *Module, ml_getter_t GlobalGet, void *Globals) {
	const char *Dir = dirname(GC_strdup(ml_module_path(Module)));
	ml_value_t *Import = GlobalGet(Globals, "import");
	ml_value_t *Cbor = ml_inline(Import, 1, ml_string_format("%s/ml_cbor.so", Dir));

	MLArrayT = ml_type(MLBufferT, "array");
	MLArrayAnyT = ml_type(MLArrayT, "value-array");
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
#include "ml_array_init.c"
	ml_value_t *CborDefault = ml_inline(SymbolMethod, 2, Cbor, ml_string("Default", -1));
	ml_map_insert(CborDefault, ml_integer(40), ml_function(NULL, ml_cbor_read_multi_array_fn));
	ml_map_insert(CborDefault, ml_integer(72), ml_function((void *)ML_ARRAY_FORMAT_I8, ml_cbor_read_typed_array_fn));
	ml_map_insert(CborDefault, ml_integer(64), ml_function((void *)ML_ARRAY_FORMAT_U8, ml_cbor_read_typed_array_fn));
	ml_map_insert(CborDefault, ml_integer(77), ml_function((void *)ML_ARRAY_FORMAT_I16, ml_cbor_read_typed_array_fn));
	ml_map_insert(CborDefault, ml_integer(69), ml_function((void *)ML_ARRAY_FORMAT_U16, ml_cbor_read_typed_array_fn));
	ml_map_insert(CborDefault, ml_integer(78), ml_function((void *)ML_ARRAY_FORMAT_I32, ml_cbor_read_typed_array_fn));
	ml_map_insert(CborDefault, ml_integer(70), ml_function((void *)ML_ARRAY_FORMAT_U32, ml_cbor_read_typed_array_fn));
	ml_map_insert(CborDefault, ml_integer(79), ml_function((void *)ML_ARRAY_FORMAT_I64, ml_cbor_read_typed_array_fn));
	ml_map_insert(CborDefault, ml_integer(71), ml_function((void *)ML_ARRAY_FORMAT_U64, ml_cbor_read_typed_array_fn));
	ml_map_insert(CborDefault, ml_integer(85), ml_function((void *)ML_ARRAY_FORMAT_F32, ml_cbor_read_typed_array_fn));
	ml_map_insert(CborDefault, ml_integer(86), ml_function((void *)ML_ARRAY_FORMAT_F64, ml_cbor_read_typed_array_fn));
	ml_module_export(Module, "new", ml_functionx(NULL, ml_array_new_fnx));
	ml_module_export(Module, "wrap", ml_function(NULL, ml_array_wrap_fn));
	ml_module_export(Module, "of", ml_function(NULL, ml_array_of_fn));
	ml_module_export(Module, "T", (ml_value_t *)MLArrayT);
	ml_module_export(Module, "AnyT", (ml_value_t *)MLArrayAnyT);
	ml_module_export(Module, "Int8T", (ml_value_t *)MLArrayInt8T);
	ml_module_export(Module, "UInt8T", (ml_value_t *)MLArrayUInt8T);
	ml_module_export(Module, "Int16T", (ml_value_t *)MLArrayInt16T);
	ml_module_export(Module, "UInt16T", (ml_value_t *)MLArrayUInt16T);
	ml_module_export(Module, "Int32T", (ml_value_t *)MLArrayInt32T);
	ml_module_export(Module, "UInt32T", (ml_value_t *)MLArrayUInt32T);
	ml_module_export(Module, "Int64T", (ml_value_t *)MLArrayInt64T);
	ml_module_export(Module, "UInt64T", (ml_value_t *)MLArrayUInt64T);
	ml_module_export(Module, "Float32T", (ml_value_t *)MLArrayFloat32T);
	ml_module_export(Module, "Float64T", (ml_value_t *)MLArrayFloat64T);
}

#include "ml_array.h"
#include "ml_macros.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

ML_TYPE(MLArrayT, (MLBufferT), "array");

extern ml_type_t MLArrayInt8T[];
extern ml_type_t MLArrayUInt8T[];
extern ml_type_t MLArrayInt16T[];
extern ml_type_t MLArrayUInt16T[];
extern ml_type_t MLArrayInt32T[];
extern ml_type_t MLArrayUInt32T[];
extern ml_type_t MLArrayInt64T[];
extern ml_type_t MLArrayUInt64T[];
extern ml_type_t MLArrayFloat32T[];
extern ml_type_t MLArrayFloat64T[];
extern ml_type_t MLArrayAnyT[];

size_t MLArraySizes[] = {
	[ML_ARRAY_FORMAT_NONE] = sizeof(ml_value_t *),
	[ML_ARRAY_FORMAT_I8] = sizeof(int8_t),
	[ML_ARRAY_FORMAT_U8] = sizeof(uint8_t),
	[ML_ARRAY_FORMAT_I16] = sizeof(int16_t),
	[ML_ARRAY_FORMAT_U16] = sizeof(uint16_t),
	[ML_ARRAY_FORMAT_I32] = sizeof(int32_t),
	[ML_ARRAY_FORMAT_U32] = sizeof(uint32_t),
	[ML_ARRAY_FORMAT_I64] = sizeof(int64_t),
	[ML_ARRAY_FORMAT_U64] = sizeof(uint64_t),
	[ML_ARRAY_FORMAT_F32] = sizeof(float),
	[ML_ARRAY_FORMAT_F64] = sizeof(double),
	[ML_ARRAY_FORMAT_ANY] = sizeof(ml_value_t *)
};

ml_array_t *ml_array_new(ml_array_format_t Format, int Degree) {
	ml_type_t *Type = MLArrayT;
	switch (Format) {
	case ML_ARRAY_FORMAT_NONE: Type = MLArrayAnyT; break;
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
	case ML_ARRAY_FORMAT_ANY: Type = MLArrayAnyT; break;
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
	Value = ml_typeof(Value)->deref(Value);
	ml_array_t *Array = State->Array;
	switch (Array->Format) {
	case ML_ARRAY_FORMAT_NONE:
		break;
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
			return ml_call(State, State->Function, Array->Degree, State->Args);
		} else {
			State->Args[I] = ml_integer(1);
		}
	}
	ML_CONTINUE(State->Base.Caller, Array);
}

static void ml_array_typed_new_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLListT);
	ml_array_format_t Format = (intptr_t)Data;
	ml_array_t *Array;
	int Degree = ml_list_length(Args[0]);
	Array = ml_array_new(Format, Degree);
	int I = 0;
	ML_LIST_FOREACH(Args[0], Iter) {
		if (!ml_is(Iter->Value, MLIntegerT)) ML_RETURN(ml_error("TypeError", "Dimension is not an integer"));
		Array->Dimensions[I++].Size = ml_integer_value(Iter->Value);
	}
	int DataSize = MLArraySizes[Format];
	for (int I = Array->Degree; --I >= 0;) {
		Array->Dimensions[I].Stride = DataSize;
		DataSize *= Array->Dimensions[I].Size;
	}
	Array->Base.Address = GC_MALLOC_ATOMIC(DataSize);
	Array->Base.Size = DataSize;
	if (Count == 1) {
		if (Format == ML_ARRAY_FORMAT_ANY) {
			ml_value_t **Values = (ml_value_t **)Array->Base.Address;
			for (int I = DataSize / sizeof(ml_value_t *); --I >= 0;) {
				*Values++ = MLNil;
			}
		} else {
			memset(Array->Base.Address, 0, DataSize);
		}
		ML_RETURN(Array);
	}
	ml_array_init_state_t *State = xnew(ml_array_init_state_t, Array->Degree, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_array_init_run;
	State->Base.Context = Caller->Context;
	State->Address = Array->Base.Address;
	State->Array = Array;
	ml_value_t *Function = State->Function = Args[1];
	for (int I = 0; I < Array->Degree; ++I) State->Args[I] = ml_integer(1);
	return ml_call(State, Function, Array->Degree, State->Args);
}

static void ml_array_new_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLTypeT);
	ML_CHECKX_ARG_TYPE(1, MLListT);
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
	return ml_array_typed_new_fnx(Caller, (void *)Format, Count - 1, Args + 1);
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
	for (int I = 0; I < Degree; ++I) {
		ml_value_t *Size = ml_list_get(Args[2], I + 1);
		ml_value_t *Stride = ml_list_get(Args[3], I + 1);
		if (!ml_is(Size, MLIntegerT)) return ml_error("TypeError", "Dimension is not an integer");
		if (!ml_is(Stride, MLIntegerT)) return ml_error("TypeError", "Stride is not an integer");
		Array->Dimensions[I].Size = ml_integer_value(Size);
		Array->Dimensions[I].Stride = ml_integer_value(Stride);
	}
	Array->Base.Address = ((ml_buffer_t *)Args[1])->Address;
	Array->Base.Size = ((ml_buffer_t *)Args[1])->Size;
	return (ml_value_t *)Array;
}

ML_METHOD("shape", MLArrayT) {
//<Array
//>list
	ml_array_t *Array = (ml_array_t *)Args[0];
	ml_value_t *Shape = ml_list();
	for (int I = 0; I < Array->Degree; ++I) {
		ml_list_put(Shape, ml_integer(Array->Dimensions[I].Size));
	}
	return Shape;
}

ML_METHOD("strides", MLArrayT) {
//<Array
//>list
	ml_array_t *Array = (ml_array_t *)Args[0];
	ml_value_t *Strides = ml_list();
	for (int I = 0; I < Array->Degree; ++I) {
		ml_list_put(Strides, ml_integer(Array->Dimensions[I].Stride));
	}
	return Strides;
}

ML_METHOD("size", MLArrayT) {
//<Array
//>integer
	ml_array_t *Array = (ml_array_t *)Args[0];
	size_t Size = Array->Dimensions[Array->Degree - 1].Stride;
	for (int I = 1; I < Array->Degree; ++I) Size *= Array->Dimensions[I].Size;
	if (Array->Dimensions[0].Stride == Size) return ml_integer(Size * Array->Dimensions[0].Size);
	return MLNil;
}

ML_METHOD("degree", MLArrayT) {
//<Array
//>integer
	ml_array_t *Array = (ml_array_t *)Args[0];
	return ml_integer(Array->Degree);
}

ML_METHOD("transpose", MLArrayT) {
//<Array
//>array
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
//<Array
//<Indices
//>array
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	if (Degree > 64) return ml_error("ArrayError", "Not implemented for degree > 64 yet");
	if (ml_list_length(Args[1]) != Degree) return ml_error("ArrayError", "List length must match degree");
	ml_array_t *Target = ml_array_new(Source->Format, Degree);
	int I = 0;
	size_t Actual = 0;
	ML_LIST_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Value, MLIntegerT)) return ml_error("ArrayError", "Invalid index");
		int J = ml_integer_value(Iter->Value);
		if (J <= 0) J += Degree + 1;
		if (J < 1 || J > Degree) return ml_error("ArrayError", "Invalid index");
		Actual += 1 << (J - 1);
		Target->Dimensions[I++] = Source->Dimensions[J - 1];
	}
	size_t Expected = (1 << Degree) - 1;
	if (Actual != Expected) return ml_error("ArrayError", "Invalid permutation");
	Target->Base = Source->Base;
	return (ml_value_t *)Target;
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

static ml_value_t *ml_array_index_internal(ml_array_t *Source, int Count, ml_value_t **Indices) {
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
		if (ml_is(Index, MLIntegerT)) {
			int IndexValue = ml_integer_value(Index);
			if (IndexValue <= 0) IndexValue += SourceDimension->Size + 1;
			if (--IndexValue < 0) return MLNil;
			if (IndexValue >= SourceDimension->Size) return MLNil;
			if (SourceDimension->Indices) IndexValue = SourceDimension->Indices[IndexValue];
			Address += SourceDimension->Stride * IndexValue;
		} else if (ml_is(Index, MLListT)) {
			int Size = TargetDimension->Size = ml_list_length(Index);
			int *Indices = TargetDimension->Indices = (int *)GC_MALLOC_ATOMIC(Size * sizeof(int));
			int *IndexPtr = Indices;
			ML_LIST_FOREACH(Index, Iter) {
				int IndexValue = ml_integer_value(Iter->Value);
				if (IndexValue <= 0) IndexValue += SourceDimension->Size + 1;
				if (--IndexValue < 0) return MLNil;
				if (IndexValue >= SourceDimension->Size) return MLNil;
				*IndexPtr++ = IndexValue;
			}
			TargetDimension->Stride = SourceDimension->Stride;
			++TargetDimension;
		} else if (ml_is(Index, MLIntegerRangeT)) {
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
			return ml_error("TypeError", "Unknown index type: %s", ml_typeof(Index)->Name);
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
	return (ml_value_t *)Target;
}

ML_METHODV("[]", MLArrayT) {
//<Array
//<Indices...:any
//>array
	ml_array_t *Source = (ml_array_t *)Args[0];
	return ml_array_index_internal(Source, Count - 1, Args + 1);
}

ML_METHOD("[]", MLArrayT, MLMapT) {
//<Array
//<Indices
//>array
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	ml_value_t *Indices[Degree];
	for (int I = 0; I < Degree; ++I) Indices[I] = MLNil;
	ML_MAP_FOREACH(Args[1], Iter) {
		int Index = ml_integer_value(Iter->Key) - 1;
		if (Index < 0) Index += Degree + 1;
		if (Index < 0 || Index >= Degree) return ml_error("RangeError", "Index out of range");
		Indices[Index] = Iter->Value;
	}
	return ml_array_index_internal(Source, Degree, Indices);
}

static ml_value_t *ml_array_of_fn(void *Data, int Count, ml_value_t **Args);

static char *ml_array_index(ml_array_t *Array, va_list Indices) {
	ml_array_dimension_t *Dimension = Array->Dimensions;
	char *Address = Array->Base.Address;
	for (int I = 0; I < Array->Degree; ++I) {
		int Index = va_arg(Indices, int);
		if (Index < 0 || Index >= Dimension->Size) return 0;
		if (Dimension->Indices) {
			Address += Dimension->Stride * Dimension->Indices[Index];
		} else {
			Address += Dimension->Stride * Index;
		}
		++Dimension;
	}
	return Address;
}

#define UPDATE_ROW_IMPL(NAME, OP, TARGET, SOURCE) \
\
static void NAME ## _row_ ## TARGET ## _ ## SOURCE(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData) { \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				*(TARGET *)(TargetData + TargetIndices[I] * TargetDimension->Stride) OP *(SOURCE *)(SourceData + SourceIndices[I] * SourceDimension->Stride); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				*(TARGET *)(TargetData + TargetIndices[I] * TargetDimension->Stride) OP *(SOURCE *)SourceData; \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				*(TARGET *)TargetData OP *(SOURCE *)(SourceData + SourceIndices[I] * SourceDimension->Stride); \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				*(TARGET *)TargetData OP *(SOURCE *)SourceData; \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
}

#define ml_number(X) _Generic(X, ml_value_t *: ml_nop, double: ml_real, default: ml_integer)(X)

#define UPDATE_ROW_VALUE_IMPL(NAME, OP, SOURCE) \
\
static void NAME ## _row_value_ ## SOURCE(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData) { \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t **Target = (ml_value_t **)(TargetData + TargetIndices[I] * TargetDimension->Stride); \
				ml_value_t *Source = ml_number(*(SOURCE *)(SourceData + SourceIndices[I] * SourceDimension->Stride)); \
				*Target = Source; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t **Target = (ml_value_t **)(TargetData + TargetIndices[I] * TargetDimension->Stride); \
				ml_value_t *Source = ml_number(*(SOURCE *)SourceData); \
				*Target = Source; \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t **Target = (ml_value_t **)TargetData; \
				ml_value_t *Source = ml_number(*(SOURCE *)(SourceData + SourceIndices[I] * SourceDimension->Stride)); \
				*Target = Source; \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				ml_value_t **Target = (ml_value_t **)TargetData; \
				ml_value_t *Source = ml_number(*(SOURCE *)SourceData); \
				*Target = Source; \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
}

#define ml_number_value(T, X) _Generic(T, double: ml_real_value, default: ml_integer_value)(X)

#define UPDATE_ROW_IMPL_VALUE(NAME, OP, TARGET) \
\
static void NAME ## _row_ ## TARGET ## _value(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData) { \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				TARGET Source = ml_number_value((TARGET)0, *(ml_value_t **)(SourceData + SourceIndices[I] * SourceDimension->Stride)); \
				*(TARGET *)(TargetData + TargetIndices[I] * TargetDimension->Stride) OP Source; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				TARGET Source = ml_number_value((TARGET)0, *(ml_value_t **)SourceData); \
				*(TARGET *)(TargetData + TargetIndices[I] * TargetDimension->Stride) OP Source; \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				TARGET Source = ml_number_value((TARGET)0, *(ml_value_t **)(SourceData + SourceIndices[I] * SourceDimension->Stride)); \
				*(TARGET *)TargetData = Source; \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				TARGET Source = ml_number_value((TARGET)0, *(ml_value_t **)SourceData); \
				*(TARGET *)TargetData = Source; \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
}

#define UPDATE_ROW_VALUE_IMPL_VALUE(NAME, OP) \
\
static void NAME ## _row_value_value(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData) { \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Source = *(ml_value_t **)(SourceData + SourceIndices[I] * SourceDimension->Stride); \
				*(ml_value_t **)(TargetData + TargetIndices[I] * TargetDimension->Stride) = Source; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Source = *(ml_value_t **)SourceData; \
				*(ml_value_t **)(TargetData + TargetIndices[I] * TargetDimension->Stride) = Source; \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Source = *(ml_value_t **)(SourceData + SourceIndices[I] * SourceDimension->Stride); \
				*(ml_value_t **)TargetData = Source; \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				ml_value_t *Source = *(ml_value_t **)SourceData; \
				*(ml_value_t **)TargetData = Source; \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
}

#define UPDATE_ROW_TARGET_IMPL(NAME, OP, TARGET) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, int8_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, uint8_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, int16_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, uint16_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, int32_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, uint32_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, int64_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, uint64_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, float) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, double) \
UPDATE_ROW_IMPL_VALUE(NAME, OP, TARGET)

#define UPDATE_ROW_TARGET_VALUE_IMPL(NAME, OP) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, int8_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, uint8_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, int16_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, uint16_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, int32_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, uint32_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, int64_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, uint64_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, float) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, double) \
UPDATE_ROW_VALUE_IMPL_VALUE(NAME, OP)

#define UPDATE_ROW_OPS_IMPL(NAME, OP) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, int8_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, uint8_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, int16_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, uint16_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, int32_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, uint32_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, int64_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, uint64_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, float) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, double) \
UPDATE_ROW_TARGET_VALUE_IMPL(NAME, OP)

UPDATE_ROW_OPS_IMPL(set, =)
UPDATE_ROW_OPS_IMPL(add, +=)
UPDATE_ROW_OPS_IMPL(sub, -=)
UPDATE_ROW_OPS_IMPL(mul, *=)
UPDATE_ROW_OPS_IMPL(div, /=)

#define UPDATE_ROW_ENTRY(INDEX, NAME, TARGET, SOURCE) \
	[INDEX] = NAME ## _row_ ## TARGET ## _ ## SOURCE

#define UPDATE_ROW_VALUE_ENTRY(INDEX, NAME, SOURCE) \
	[INDEX] = NAME ## _row_value_ ## SOURCE

#define UPDATE_ROW_ENTRY_VALUE(INDEX, NAME, TARGET) \
	[INDEX] = NAME ## _row_ ## TARGET ## _value

#define UPDATE_ROW_VALUE_ENTRY_VALUE(INDEX, NAME) \
	[INDEX] = NAME ## _row_value_value

#define MAX_FORMATS 16

#define UPDATE_ROW_TARGET_ENTRIES(INDEX, NAME, TARGET) \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I8, NAME, TARGET, int8_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U8, NAME, TARGET, uint8_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I16, NAME, TARGET, int16_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U16, NAME, TARGET, uint16_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I32, NAME, TARGET, int32_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U32, NAME, TARGET, uint32_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I64, NAME, TARGET, int64_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U64, NAME, TARGET, uint64_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F32, NAME, TARGET, float), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, NAME, TARGET, double), \
UPDATE_ROW_ENTRY_VALUE(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_ANY, NAME, TARGET)

#define UPDATE_ROW_VALUE_TARGET_ENTRIES(INDEX, NAME) \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I8, NAME, int8_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U8, NAME, uint8_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I16, NAME, int16_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U16, NAME, uint16_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I32, NAME, int32_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U32, NAME, uint32_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I64, NAME, int64_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U64, NAME, uint64_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F32, NAME, float), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, NAME, double), \
UPDATE_ROW_VALUE_ENTRY_VALUE(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_ANY, NAME)

#define UPDATE_ROW_OPS_ENTRIES(INDEX, NAME) \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I8, NAME, int8_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U8, NAME, uint8_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I16, NAME, int16_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U16, NAME, uint16_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I32, NAME, int32_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U32, NAME, uint32_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I64, NAME, int64_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U64, NAME, uint64_t), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F32, NAME, float), \
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, NAME, double), \
UPDATE_ROW_VALUE_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_ANY, NAME)

typedef void (*update_row_fn_t)(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData);

static update_row_fn_t UpdateRowFns[] = {
	UPDATE_ROW_OPS_ENTRIES(0, set),
	UPDATE_ROW_OPS_ENTRIES(1, add),
	UPDATE_ROW_OPS_ENTRIES(2, sub),
	UPDATE_ROW_OPS_ENTRIES(3, mul),
	UPDATE_ROW_OPS_ENTRIES(4, div)
};

static void update_array(int Op, ml_array_dimension_t *TargetDimension, char *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, char *SourceData) {
	if (SourceDegree == 0) {
		ml_array_dimension_t ConstantDimension[1] = {{TargetDimension->Size, 0, NULL}};
		return UpdateRowFns[Op](TargetDimension, TargetData, ConstantDimension, SourceData);
	}
	if (SourceDegree == 1) {
		return UpdateRowFns[Op](TargetDimension, TargetData, SourceDimension, SourceData);
	}
	int Size = TargetDimension->Size;
	if (TargetDimension->Indices) {
		int *TargetIndices = TargetDimension->Indices;
		if (SourceDimension->Indices) {
			int *SourceIndices = SourceDimension->Indices;
			for (int I = 0; I < Size; ++I) {
				update_array(Op, TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData + SourceIndices[I] * SourceDimension->Stride);
			}
		} else {
			int SourceStride = SourceDimension->Stride;
			for (int I = 0; I < Size; ++I) {
				update_array(Op, TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData);
				SourceData += SourceStride;
			}
		}
	} else {
		int TargetStride = TargetDimension->Stride;
		if (SourceDimension->Indices) {
			int *SourceIndices = SourceDimension->Indices;
			for (int I = 0; I < Size; ++I) {
				update_array(Op, TargetDimension + 1, TargetData, SourceDegree - 1, SourceDimension + 1, SourceData + SourceIndices[I] * SourceDimension->Stride);
				TargetData += TargetStride;
			}
		} else {
			int SourceStride = SourceDimension->Stride;
			for (int I = Size; --I >= 0;) {
				update_array(Op, TargetDimension + 1, TargetData, SourceDegree - 1, SourceDimension + 1, SourceData);
				TargetData += TargetStride;
				SourceData += SourceStride;
			}
		}
	}
}

static void update_prefix(int Op, int PrefixDegree, ml_array_dimension_t *TargetDimension, char *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, char *SourceData) {
	if (PrefixDegree == 0) return update_array(Op, TargetDimension, TargetData, SourceDegree, SourceDimension, SourceData);
	int Size = TargetDimension->Size;
	if (TargetDimension->Indices) {
		int *TargetIndices = TargetDimension->Indices;
		for (int I = Size; --I >= 0;) {
			update_prefix(Op, PrefixDegree - 1, TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree, SourceDimension, SourceData);
		}
	} else {
		int Stride = TargetDimension->Stride;
		for (int I = Size; --I >= 0;) {
			update_prefix(Op, PrefixDegree - 1, TargetDimension + 1, TargetData, SourceDegree, SourceDimension, SourceData);
			TargetData += Stride;
		}
	}
}

static ml_value_t *update_array_fn(void *Data, int Count, ml_value_t **Args) {
	ml_array_t *Target = (ml_array_t *)Args[0];
	ml_array_t *Source = (ml_array_t *)Args[1];
	if (Source->Degree > Target->Degree) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
	int PrefixDegree = Target->Degree - Source->Degree;
	for (int I = 0; I < Source->Degree; ++I) {
		if (Target->Dimensions[PrefixDegree + I].Size != Source->Dimensions[I].Size) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
	}
	int Op = ((char *)Data - (char *)0) * MAX_FORMATS * MAX_FORMATS + Target->Format * MAX_FORMATS + Source->Format;
	if (!UpdateRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", Target->Base.Type->Name, Source->Base.Type->Name);
	if (Target->Degree) {
		update_prefix(Op, PrefixDegree, Target->Dimensions, Target->Base.Address, Source->Degree, Source->Dimensions, Source->Base.Address);
	} else {
		ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}};
		UpdateRowFns[Op](ValueDimension, Target->Base.Address, ValueDimension, Source->Base.Address);
	}
	return Args[0];
}

#define UPDATE_METHOD(NAME, BASE, ATYPE, CTYPE, FROM_VAL, FORMAT) \
\
ML_METHOD(#NAME, ATYPE, MLNumberT) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	CTYPE Value = FROM_VAL(Args[1]); \
	ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}}; \
	int Op = (BASE) * MAX_FORMATS * MAX_FORMATS + Array->Format * MAX_FORMATS + FORMAT; \
	if (!UpdateRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", ml_typeof(Args[0])->Name, ml_typeof(Args[1])->Name); \
	if (Array->Degree == 0) { \
		UpdateRowFns[Op](ValueDimension, Array->Base.Address, ValueDimension, (char *)&Value); \
	} else { \
		update_prefix(Op, Array->Degree - 1, Array->Dimensions, Array->Base.Address, 0, ValueDimension, (char *)&Value); \
	} \
	return Args[0]; \
}

#define UPDATE_METHODS(ATYPE, CTYPE, FROM_VAL, FORMAT) \
UPDATE_METHOD(set, 0, ATYPE, CTYPE, FROM_VAL, FORMAT); \
UPDATE_METHOD(add, 1, ATYPE, CTYPE, FROM_VAL, FORMAT); \
UPDATE_METHOD(sub, 2, ATYPE, CTYPE, FROM_VAL, FORMAT); \
UPDATE_METHOD(mul, 3, ATYPE, CTYPE, FROM_VAL, FORMAT); \
UPDATE_METHOD(div, 4, ATYPE, CTYPE, FROM_VAL, FORMAT);

#define BUFFER_APPEND(BUFFER, PRINTF, VALUE) ml_stringbuffer_append(BUFFER, VALUE)

#define METHODS(ATYPE, CTYPE, APPEND, PRINTF, FROM_VAL, TO_VAL, FROM_NUM, TO_NUM, FORMAT) \
\
static ml_value_t *ML_TYPED_FN(ml_array_value, ATYPE, ml_array_t *Array, char *Address) { \
	return TO_VAL(*(CTYPE *)Array->Base.Address); \
} \
\
static void append_array_ ## CTYPE(ml_stringbuffer_t *Buffer, int Degree, ml_array_dimension_t *Dimension, char *Address) { \
	ml_stringbuffer_add(Buffer, "[", 1); \
	int Stride = Dimension->Stride; \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Size) { \
			if (Degree == 1) { \
				APPEND(Buffer, PRINTF, *(CTYPE *)(Address + (Indices[0]) * Dimension->Stride)); \
				for (int I = 1; I < Dimension->Size; ++I) { \
					ml_stringbuffer_add(Buffer, ", ", 2); \
					APPEND(Buffer, PRINTF, *(CTYPE *)(Address + (Indices[I]) * Stride)); \
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
			APPEND(Buffer, PRINTF, *(CTYPE *)Address); \
			Address += Stride; \
			for (int I = Dimension->Size; --I > 0;) { \
				ml_stringbuffer_add(Buffer, ", ", 2); \
				APPEND(Buffer, PRINTF, *(CTYPE *)Address); \
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
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT}; \
	if (Array->Degree == 0) { \
		APPEND(Buffer, PRINTF, *(CTYPE *)Array->Base.Address); \
	} else { \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Address); \
	} \
	return ml_stringbuffer_get_string(Buffer); \
} \
\
ML_METHOD(MLStringOfMethod, ATYPE) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT}; \
	if (Array->Degree == 0) { \
		APPEND(Buffer, PRINTF, *(CTYPE *)Array->Base.Address); \
	} else { \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Address); \
	} \
	return ml_stringbuffer_get_string(Buffer); \
} \
static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, ATYPE, ml_stringbuffer_t *Buffer, ml_array_t *Array) { \
	if (Array->Degree == 0) { \
		APPEND(Buffer, PRINTF, *(CTYPE *)Array->Base.Address); \
	} else { \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Address); \
	} \
	return MLSome; \
} \
\
ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, ATYPE) { \
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0]; \
	ml_array_t *Array = (ml_array_t *)Args[1]; \
	if (Array->Degree == 0) { \
		APPEND(Buffer, PRINTF, *(CTYPE *)Array->Base.Address); \
	} else { \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Address); \
	} \
	return Args[0]; \
} \
\
UPDATE_METHODS(ATYPE, CTYPE, FROM_VAL, FORMAT); \
\
CTYPE ml_array_get_ ## CTYPE(ml_array_t *Array, ...) { \
	va_list Indices; \
	va_start(Indices, Array); \
	char *Address = ml_array_index(Array, Indices); \
	va_end(Indices); \
	switch (Array->Format) { \
	case ML_ARRAY_FORMAT_NONE: break; \
	case ML_ARRAY_FORMAT_I8: return FROM_NUM(*(int8_t *)Address); \
	case ML_ARRAY_FORMAT_U8: return FROM_NUM(*(uint8_t *)Address); \
	case ML_ARRAY_FORMAT_I16: return FROM_NUM(*(int16_t *)Address); \
	case ML_ARRAY_FORMAT_U16: return FROM_NUM(*(uint16_t *)Address); \
	case ML_ARRAY_FORMAT_I32: return FROM_NUM(*(int32_t *)Address); \
	case ML_ARRAY_FORMAT_U32: return FROM_NUM(*(uint32_t *)Address); \
	case ML_ARRAY_FORMAT_I64: return FROM_NUM(*(int64_t *)Address); \
	case ML_ARRAY_FORMAT_U64: return FROM_NUM(*(uint64_t *)Address); \
	case ML_ARRAY_FORMAT_F32: return FROM_NUM(*(float *)Address); \
	case ML_ARRAY_FORMAT_F64: return FROM_NUM(*(double *)Address); \
	case ML_ARRAY_FORMAT_ANY: return FROM_VAL(*(ml_value_t **)Address); \
	} \
	return (CTYPE)0; \
} \
\
void ml_array_set_ ## CTYPE(CTYPE Value, ml_array_t *Array, ...) { \
	va_list Indices; \
	va_start(Indices, Array); \
	char *Address = ml_array_index(Array, Indices); \
	va_end(Indices); \
	switch (Array->Format) { \
	case ML_ARRAY_FORMAT_NONE: break; \
	case ML_ARRAY_FORMAT_I8: *(int8_t *)Address = TO_NUM((int8_t)0, Value); break; \
	case ML_ARRAY_FORMAT_U8: *(uint8_t *)Address = TO_NUM((uint8_t)0, Value); break; \
	case ML_ARRAY_FORMAT_I16: *(int16_t *)Address = TO_NUM((int16_t)0, Value); break; \
	case ML_ARRAY_FORMAT_U16: *(uint16_t *)Address = TO_NUM((uint16_t)0, Value); break; \
	case ML_ARRAY_FORMAT_I32: *(int32_t *)Address = TO_NUM((int32_t)0, Value); break; \
	case ML_ARRAY_FORMAT_U32: *(uint32_t *)Address = TO_NUM((uint32_t)0, Value); break; \
	case ML_ARRAY_FORMAT_I64: *(int64_t *)Address = TO_NUM((int64_t)0, Value); break; \
	case ML_ARRAY_FORMAT_U64: *(uint64_t *)Address = TO_NUM((uint8_t)0, Value); break; \
	case ML_ARRAY_FORMAT_F32: *(float *)Address = TO_NUM((float)0, Value); break; \
	case ML_ARRAY_FORMAT_F64: *(double *)Address = TO_NUM((double)0, Value); break; \
	case ML_ARRAY_FORMAT_ANY: *(ml_value_t **)Address = TO_VAL(Value); break; \
	} \
} \
\
static ml_value_t *ml_array_ ## CTYPE ## _deref(ml_array_t *Target, ml_value_t *Value) { \
	if (Target->Degree == 0)  return TO_VAL(*(CTYPE *)Target->Base.Address); \
	return (ml_value_t *)Target; \
} \
\
static ml_value_t *ml_array_ ## CTYPE ## _assign(ml_array_t *Target, ml_value_t *Value) { \
	for (;;) if (Target->Format == ML_ARRAY_FORMAT_ANY && Target->Degree == 0) { \
			return *(ml_value_t **)Target->Base.Address = Value; \
		} else if (ml_is(Value, MLNumberT)) { \
		CTYPE CValue = FROM_VAL(Value); \
		ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}}; \
		int Op = Target->Format * MAX_FORMATS + Target->Format; \
		if (!UpdateRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", Target->Base.Type->Name, ml_typeof(Value)->Name); \
		if (Target->Degree == 0) { \
			UpdateRowFns[Op](ValueDimension, Target->Base.Address, ValueDimension, (char *)&CValue); \
		} else { \
			update_prefix(Op, Target->Degree - 1, Target->Dimensions, Target->Base.Address, 0, ValueDimension, (char *)&CValue); \
		} \
		return Value; \
	} else if (ml_is(Value, MLArrayT)) { \
		ml_array_t *Source = (ml_array_t *)Value; \
		if (Source->Degree > Target->Degree) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__); \
		int PrefixDegree = Target->Degree - Source->Degree; \
		for (int I = 0; I < Source->Degree; ++I) { \
			if (Target->Dimensions[PrefixDegree + I].Size != Source->Dimensions[I].Size) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__); \
		} \
		int Op = Target->Format * MAX_FORMATS + Source->Format; \
		if (!UpdateRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", Target->Base.Type->Name, Source->Base.Type->Name); \
		if (Target->Degree) { \
			update_prefix(Op, PrefixDegree, Target->Dimensions, Target->Base.Address, Source->Degree, Source->Dimensions, Source->Base.Address); \
		} else { \
			ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}}; \
			UpdateRowFns[Op](ValueDimension, Target->Base.Address, ValueDimension, Source->Base.Address); \
		} \
		return Value; \
	} else { \
		Value = ml_array_of_fn(NULL, 1, &Value); \
	} \
	return NULL; \
} \
\
ML_TYPE(ATYPE, (MLArrayT), #CTYPE "-array", \
	.deref = (void *)ml_array_ ## CTYPE ## _deref, \
	.assign = (void *)ml_array_ ## CTYPE ## _assign \
);

typedef ml_value_t *value;

#define NOP_VAL(T, X) X

METHODS(MLArrayInt8T, int8_t, ml_stringbuffer_addf, "%d", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_I8);
METHODS(MLArrayUInt8T, uint8_t, ml_stringbuffer_addf, "%ud", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_U8);
METHODS(MLArrayInt16T, int16_t, ml_stringbuffer_addf, "%d", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_I16);
METHODS(MLArrayUInt16T, uint16_t, ml_stringbuffer_addf, "%ud", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_U16);
METHODS(MLArrayInt32T, int32_t, ml_stringbuffer_addf, "%d", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_I32);
METHODS(MLArrayUInt32T, uint32_t, ml_stringbuffer_addf, "%ud", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_U32);
METHODS(MLArrayInt64T, int64_t, ml_stringbuffer_addf, "%ld", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_I64);
METHODS(MLArrayUInt64T, uint64_t, ml_stringbuffer_addf, "%lud", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_U64);
METHODS(MLArrayFloat32T, float, ml_stringbuffer_addf, "%f", ml_real_value, ml_real, , NOP_VAL, ML_ARRAY_FORMAT_F32);
METHODS(MLArrayFloat64T, double, ml_stringbuffer_addf, "%f", ml_real_value, ml_real, , NOP_VAL, ML_ARRAY_FORMAT_F64);
METHODS(MLArrayAnyT, value, BUFFER_APPEND, "?", ml_nop, ml_nop, ml_number, ml_number_value, ML_ARRAY_FORMAT_ANY);

#define PARTIAL_SUMS(ATYPE, CTYPE) \
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
	if (Target < 1 || Target > Array->Degree) return ml_error("ArrayError", "Dimension index invalid"); \
	Target = Array->Degree + 1 - Target; \
	partial_sums_ ## CTYPE(Target, Array->Degree, Array->Dimensions, Array->Base.Address, 0); \
	return Args[0]; \
}

PARTIAL_SUMS(MLArrayInt8T, int8_t);
PARTIAL_SUMS(MLArrayUInt8T, uint8_t);
PARTIAL_SUMS(MLArrayInt16T, int16_t);
PARTIAL_SUMS(MLArrayUInt16T, uint16_t);
PARTIAL_SUMS(MLArrayInt32T, int32_t);
PARTIAL_SUMS(MLArrayUInt32T, uint32_t);
PARTIAL_SUMS(MLArrayInt64T, int64_t);
PARTIAL_SUMS(MLArrayUInt64T, uint64_t);
PARTIAL_SUMS(MLArrayFloat32T, float);
PARTIAL_SUMS(MLArrayFloat64T, double);

static int array_copy(ml_array_t *Target, ml_array_t *Source) {
	int Degree = Source->Degree;
	int DataSize = MLArraySizes[Target->Format];
	for (int I = Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Address = GC_MALLOC_ATOMIC(DataSize);
	int Op1 = Target->Format * MAX_FORMATS + Source->Format;
	update_array(Op1, Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address);
	return DataSize;
}

ML_METHOD("-", MLArrayT) {
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation");
	int Degree = A->Degree;
	ml_array_t *C = ml_array_new(A->Format, Degree);
	int DataSize = array_copy(C, A);
	switch (C->Format) {
	case ML_ARRAY_FORMAT_I8: {
		int8_t *Values = (int8_t *)C->Base.Address;
		for (int I = DataSize / sizeof(int8_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_U8: {
		uint8_t *Values = (uint8_t *)C->Base.Address;
		for (int I = DataSize / sizeof(uint8_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_I16: {
		int16_t *Values = (int16_t *)C->Base.Address;
		for (int I = DataSize / sizeof(int16_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_U16: {
		uint16_t *Values = (uint16_t *)C->Base.Address;
		for (int I = DataSize / sizeof(uint16_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_I32: {
		int32_t *Values = (int32_t *)C->Base.Address;
		for (int I = DataSize / sizeof(int32_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_U32: {
		uint32_t *Values = (uint32_t *)C->Base.Address;
		for (int I = DataSize / sizeof(uint32_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_I64: {
		int64_t *Values = (int64_t *)C->Base.Address;
		for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_U64: {
		uint64_t *Values = (uint64_t *)C->Base.Address;
		for (int I = DataSize / sizeof(uint64_t); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_F32: {
		float *Values = (float *)C->Base.Address;
		for (int I = DataSize / sizeof(float); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	case ML_ARRAY_FORMAT_F64: {
		double *Values = (double *)C->Base.Address;
		for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = -*Values;
		break;
	}
	default: {
		return ml_error("TypeError", "Invalid types for array operation");
	}
	}
	return (ml_value_t *)C;
}

#define MAX(X, Y) ((X > Y) ? X : Y)

static ml_value_t *array_infix_fn(void *Data, int Count, ml_value_t **Args) {
	ml_array_t *A = (ml_array_t *)Args[0];
	ml_array_t *B = (ml_array_t *)Args[1];
	int Degree = A->Degree;
	ml_array_t *C = ml_array_new(MAX(A->Format, B->Format), Degree);
	array_copy(C, A);
	int Op2 = ((char *)Data - (char *)0) * MAX_FORMATS * MAX_FORMATS + C->Format * MAX_FORMATS + B->Format;
	update_prefix(Op2, C->Degree - B->Degree, C->Dimensions, C->Base.Address, B->Degree, B->Dimensions, B->Base.Address);
	return (ml_value_t *)C;
}

#define ML_ARITH_METHOD(BASE, OP) \
\
ML_METHOD(#OP, MLArrayT, MLIntegerT) { \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(MAX(A->Format, ML_ARRAY_FORMAT_I64), Degree); \
	int DataSize = array_copy(C, A); \
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
	default: { \
		return ml_error("TypeError", "Invalid types for array operation"); \
	} \
	} \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#OP, MLIntegerT, MLArrayT) { \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(MAX(A->Format, ML_ARRAY_FORMAT_I64), Degree); \
	int DataSize = array_copy(C, A); \
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
	default: { \
		return ml_error("TypeError", "Invalid types for array operation"); \
	} \
	} \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#OP, MLArrayT, MLRealT) { \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	double B = ml_integer_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(ML_ARRAY_FORMAT_F64, Degree); \
	int DataSize = array_copy(C, A); \
	double *Values = (double *)C->Base.Address; \
	for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = *Values OP B; \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#OP, MLRealT, MLArrayT) { \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	double B = ml_integer_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_new(ML_ARRAY_FORMAT_F64, Degree); \
	int DataSize = array_copy(C, A); \
	double *Values = (double *)C->Base.Address; \
	for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = B OP *Values; \
	return (ml_value_t *)C; \
}

ML_ARITH_METHOD(1, +);
ML_ARITH_METHOD(2, -);
ML_ARITH_METHOD(3, *);
ML_ARITH_METHOD(4, /);

static ml_array_t *ml_array_of_create(ml_value_t *Value, int Degree, ml_array_format_t Format) {
	if (ml_is(Value, MLListT)) {
		int Size = ml_list_length(Value);
		if (!Size) return (ml_array_t *)ml_error("ValueError", "Empty dimension in array");
		ml_array_t *Array = ml_array_of_create(ml_list_get(Value, 1), Degree + 1, Format);
		if (Array->Base.Type == MLErrorT) return Array;
		Array->Dimensions[Degree].Size = Size;
		if (Degree < Array->Degree - 1) {
			Array->Dimensions[Degree].Stride = Array->Dimensions[Degree + 1].Size * Array->Dimensions[Degree + 1].Stride;
		}
		return Array;
	} else if (ml_is(Value, MLTupleT)) {
		int Size = ml_tuple_size(Value);
		if (!Size) return (ml_array_t *)ml_error("ValueError", "Empty dimension in array");
		ml_array_t *Array = ml_array_of_create(ml_tuple_get(Value, 1), Degree + 1, Format);
		if (Array->Base.Type == MLErrorT) return Array;
		Array->Dimensions[Degree].Size = Size;
		if (Degree < Array->Degree - 1) {
			Array->Dimensions[Degree].Stride = Array->Dimensions[Degree + 1].Size * Array->Dimensions[Degree + 1].Stride;
		}
		return Array;
	} else if (ml_is(Value, MLArrayT)) {
		ml_array_t *Nested = (ml_array_t *)Value;
		ml_array_t *Array;
		Array = ml_array_new(Format, Degree + Nested->Degree);
		memcpy(Array->Dimensions + Degree, Nested->Dimensions, Nested->Degree * sizeof(ml_array_dimension_t));
		return Array;
	} else {
		ml_array_t *Array = ml_array_new(Format, Degree);
		if (Degree) {
			Array->Dimensions[Degree - 1].Size = 1;
			Array->Dimensions[Degree - 1].Stride = MLArraySizes[Format];
		}
		return Array;
	}
}

static ml_value_t *ml_array_of_fill(ml_array_format_t Format, ml_array_dimension_t *Dimension, char *Address, int Degree, ml_value_t *Value) {
	if (ml_is(Value, MLListT)) {
		if (!Degree) return ml_error("ValueError", "Inconsistent depth in array");
		if (ml_list_length(Value) != Dimension->Size) return ml_error("ValueError", "Inconsistent lengths in array");
		ML_LIST_FOREACH(Value, Iter) {
			ml_value_t *Error = ml_array_of_fill(Format, Dimension + 1, Address, Degree - 1, Iter->Value);
			if (Error) return Error;
			Address += Dimension->Stride;
		}
	} else if (ml_is(Value, MLTupleT)) {
		if (!Degree) return ml_error("ValueError", "Inconsistent depth in array");
		if (ml_tuple_size(Value) != Dimension->Size) return ml_error("ValueError", "Inconsistent lengths in array");
		ml_tuple_t *Tuple = (ml_tuple_t *)Value;
		for (int I = 0; I < Tuple->Size; ++I) {
			ml_value_t *Error = ml_array_of_fill(Format, Dimension + 1, Address, Degree - 1, Tuple->Values[I]);
			if (Error) return Error;
			Address += Dimension->Stride;
		}
	} else if (ml_is(Value, MLArrayT)) {
		ml_array_t *Source = (ml_array_t *)Value;
		if (Source->Degree != Degree) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
		for (int I = 0; I < Degree; ++I) {
			if (Dimension[I].Size != Source->Dimensions[I].Size) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
		}
		int Op = Format * MAX_FORMATS + Source->Format;
		update_array(Op, Dimension, Address, Degree, Source->Dimensions, Source->Base.Address);
	} else {
		if (Degree) return ml_error("ValueError", "Inconsistent depth in array");
		switch (Format) {
		case ML_ARRAY_FORMAT_NONE: break;
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
		case ML_ARRAY_FORMAT_ANY: *(ml_value_t **)Address = Value; break;
		}
	}
	return NULL;
}

static ml_array_format_t ml_array_of_type_guess(ml_value_t *Value, ml_array_format_t Format) {
	if (ml_is(Value, MLListT)) {
		ML_LIST_FOREACH(Value, Iter) {
			Format = ml_array_of_type_guess(Iter->Value, Format);
		}
	} else if (ml_is(Value, MLTupleT)) {
		ml_tuple_t *Tuple = (ml_tuple_t *)Value;
		for (int I = 0; I < Tuple->Size; ++I) {
			Format = ml_array_of_type_guess(Tuple->Values[I], Format);
		}
	} else if (ml_is(Value, MLArrayT)) {
		ml_array_t *Array = (ml_array_t *)Value;
		if (Format <= Array->Format) Format = Array->Format;
	} else if (ml_is(Value, MLRealT)) {
		if (Format < ML_ARRAY_FORMAT_F64) Format = ML_ARRAY_FORMAT_F64;
	} else if (ml_is(Value, MLIntegerT)) {
		if (Format < ML_ARRAY_FORMAT_I64) Format = ML_ARRAY_FORMAT_I64;
	} else {
		Format = ML_ARRAY_FORMAT_ANY;
	}
	return Format;
}

static ml_value_t *ml_array_of_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Source = Args[0];
	ml_array_format_t Format = ML_ARRAY_FORMAT_NONE;
	if (Count == 2) {
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
		Source = Args[1];
	} else {
		Format = ml_array_of_type_guess(Args[0], ML_ARRAY_FORMAT_NONE);
	}
	ml_array_t *Array = ml_array_of_create(Source, 0, Format);
	if (Array->Base.Type == MLErrorT) return (ml_value_t *)Array;
	size_t Size;
	if (Array->Degree) {
		Size = Array->Base.Size = Array->Dimensions[0].Stride * Array->Dimensions[0].Size;
	} else {
		Size = MLArraySizes[Array->Format];
	}
	char *Address = Array->Base.Address = GC_MALLOC_ATOMIC(Size);
	ml_value_t *Error = ml_array_of_fill(Array->Format, Array->Dimensions, Address, Array->Degree, Source);
	return Error ?: (ml_value_t *)Array;
}

ML_METHOD("copy", MLArrayT) {
//<Array
//>array
	ml_array_t *Source = (ml_array_t *)Args[0];
	ml_array_t *Target = ml_array_new(Source->Format, Source->Degree);
	array_copy(Target, Source);
	return (ml_value_t *)Target;
}

typedef struct {
	ml_state_t Base;
	union {
		void *Values;
		int8_t *I8;
		uint8_t *U8;
		int16_t *I16;
		uint16_t *U16;
		int32_t *I32;
		uint32_t *U32;
		int64_t *I64;
		uint64_t *U64;
		float *F32;
		double *F64;
		ml_value_t **Any;
	};
	ml_value_t *Function;
	ml_value_t *Array;
	ml_value_t *Args[1];
	int Remaining;
} ml_array_apply_state_t;

#define ARRAY_APPLY(NAME, CTYPE, TO_VAL, TO_NUM) \
static void ml_array_apply_ ## CTYPE(ml_array_apply_state_t *State, ml_value_t *Value) { \
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value); \
	*State->NAME++ = TO_NUM(Value); \
	if (--State->Remaining) { \
		State->Args[0] = TO_VAL(*State->NAME); \
		return ml_call(State, State->Function, 1, State->Args); \
	} else { \
		ML_CONTINUE(State->Base.Caller, State->Array); \
	} \
}

ARRAY_APPLY(I8, int8_t, ml_integer, ml_integer_value);
ARRAY_APPLY(U8, uint8_t, ml_integer, ml_integer_value);
ARRAY_APPLY(I16, int16_t, ml_integer, ml_integer_value);
ARRAY_APPLY(U16, uint16_t, ml_integer, ml_integer_value);
ARRAY_APPLY(I32, int32_t, ml_integer, ml_integer_value);
ARRAY_APPLY(U32, uint32_t, ml_integer, ml_integer_value);
ARRAY_APPLY(I64, int64_t, ml_integer, ml_integer_value);
ARRAY_APPLY(U64, uint64_t, ml_integer, ml_integer_value);
ARRAY_APPLY(F32, float, ml_real, ml_real_value);
ARRAY_APPLY(F64, double, ml_real, ml_real_value);
ARRAY_APPLY(Any, value, , );

ML_METHODX("copy", MLArrayT, MLFunctionT) {
//<Array
//<Function
//>array
	ml_array_t *A = (ml_array_t *)Args[0];
	int Degree = A->Degree;
	ml_array_t *C = ml_array_new(A->Format, Degree);
	int Remaining = array_copy(C, A) / MLArraySizes[C->Format];
	if (Remaining == 0) ML_RETURN(C);
	ml_array_apply_state_t *State = new(ml_array_apply_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	ml_value_t *Function = State->Function = Args[1];
	State->Remaining = Remaining;
	State->Values = C->Base.Address;
	State->Array = (ml_value_t *)C;
	switch (C->Format) {
	case ML_ARRAY_FORMAT_I8: {
		State->Base.run = (void *)ml_array_apply_int8_t;
		State->Args[0] = ml_integer(*State->I8);
		break;
	}
	case ML_ARRAY_FORMAT_U8: {
		State->Base.run = (void *)ml_array_apply_uint8_t;
		State->Args[0] = ml_integer(*State->U8);
		break;
	}
	case ML_ARRAY_FORMAT_I16: {
		State->Base.run = (void *)ml_array_apply_int16_t;
		State->Args[0] = ml_integer(*State->I16);
		break;
	}
	case ML_ARRAY_FORMAT_U16: {
		State->Base.run = (void *)ml_array_apply_uint16_t;
		State->Args[0] = ml_integer(*State->U16);
		break;
	}
	case ML_ARRAY_FORMAT_I32: {
		State->Base.run = (void *)ml_array_apply_int32_t;
		State->Args[0] = ml_integer(*State->I32);
		break;
	}
	case ML_ARRAY_FORMAT_U32: {
		State->Base.run = (void *)ml_array_apply_uint32_t;
		State->Args[0] = ml_integer(*State->U32);
		break;
	}
	case ML_ARRAY_FORMAT_I64: {
		State->Base.run = (void *)ml_array_apply_int64_t;
		State->Args[0] = ml_integer(*State->I64);
		break;
	}
	case ML_ARRAY_FORMAT_U64: {
		State->Base.run = (void *)ml_array_apply_uint64_t;
		State->Args[0] = ml_integer(*State->U64);
		break;
	}
	case ML_ARRAY_FORMAT_F32: {
		State->Base.run = (void *)ml_array_apply_float;
		State->Args[0] = ml_real(*State->F32);
		break;
	}
	case ML_ARRAY_FORMAT_F64: {
		State->Base.run = (void *)ml_array_apply_double;
		State->Args[0] = ml_real(*State->F64);
		break;
	}
	case ML_ARRAY_FORMAT_ANY: {
		State->Base.run = (void *)ml_array_apply_value;
		State->Args[0] = *State->Any;
		break;
	}
	default: ML_ERROR("ArrayError", "Unsupported format");
	}
	return ml_call(State, Function, 1, State->Args);
}

typedef struct {
	ml_state_t Base;
	char *Address;
	ml_value_t *Function;
	ml_array_t *Array;
	ml_value_t *Args[1];
	int Degree;
	int Indices[];
} ml_array_update_state_t;

#define ARRAY_UPDATE(CTYPE, TO_VAL, TO_NUM) \
static void ml_array_update_ ## CTYPE(ml_array_update_state_t *State, ml_value_t *Value) { \
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value); \
	*(CTYPE *)State->Address = TO_NUM(Value); \
	int *Indices = State->Indices; \
	for (int I = State->Degree; --I >= 0;) { \
		ml_array_dimension_t *Dimension = State->Array->Dimensions + I; \
		int Index = Indices[I]; \
		if (Index + 1 < Dimension->Size) { \
			if (Dimension->Indices) { \
				State->Address += (Dimension->Indices[Index + 1] - Dimension->Indices[Index]) * Dimension->Stride; \
			} else { \
				State->Address += Dimension->Stride; \
			} \
			State->Indices[I] = Index + 1; \
			State->Args[0] = TO_VAL(*(CTYPE *)State->Address); \
			return ml_call(State, State->Function, 1, State->Args); \
		} else { \
			if (Dimension->Indices) { \
				State->Address -= (Dimension->Indices[Index] - Dimension->Indices[0]) * Dimension->Stride; \
			} else { \
				State->Address -= Index * Dimension->Stride; \
			} \
			State->Indices[I] = 0; \
		} \
	} \
	ML_CONTINUE(State->Base.Caller, State->Array); \
}

ARRAY_UPDATE(int8_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(uint8_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(int16_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(uint16_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(int32_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(uint32_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(int64_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(uint64_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(float, ml_real, ml_real_value);
ARRAY_UPDATE(double, ml_real, ml_real_value);
ARRAY_UPDATE(value, , );

ML_METHODX("update", MLArrayT, MLFunctionT) {
//<Array
//<Function
//>array
	ml_array_t *A = (ml_array_t *)Args[0];
	int Degree = A->Degree;
	ml_array_update_state_t *State = xnew(ml_array_update_state_t, Degree, int);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	ml_value_t *Function = State->Function = Args[1];
	State->Array = A;
	State->Degree = Degree;
	char *Address = A->Base.Address;
	ml_array_dimension_t *Dimension = A->Dimensions;
	for (int I = 0; I < Degree; ++I, ++Dimension) {
		if (Dimension->Indices) {
			Address += Dimension->Indices[0] * Dimension->Stride;
		}
	}
	State->Address = Address;
	switch (A->Format) {
	case ML_ARRAY_FORMAT_I8: {
		State->Base.run = (void *)ml_array_update_int8_t;
		State->Args[0] = ml_integer(*(int8_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U8: {
		State->Base.run = (void *)ml_array_update_uint8_t;
		State->Args[0] = ml_integer(*(uint8_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I16: {
		State->Base.run = (void *)ml_array_update_int16_t;
		State->Args[0] = ml_integer(*(int16_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U16: {
		State->Base.run = (void *)ml_array_update_uint16_t;
		State->Args[0] = ml_integer(*(uint16_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I32: {
		State->Base.run = (void *)ml_array_update_int32_t;
		State->Args[0] = ml_integer(*(int32_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U32: {
		State->Base.run = (void *)ml_array_update_uint32_t;
		State->Args[0] = ml_integer(*(uint32_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I64: {
		State->Base.run = (void *)ml_array_update_int64_t;
		State->Args[0] = ml_integer(*(int64_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U64: {
		State->Base.run = (void *)ml_array_update_uint64_t;
		State->Args[0] = ml_integer(*(uint64_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_F32: {
		State->Base.run = (void *)ml_array_update_float;
		State->Args[0] = ml_real(*(float *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_F64: {
		State->Base.run = (void *)ml_array_update_double;
		State->Args[0] = ml_real(*(double *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_ANY: {
		State->Base.run = (void *)ml_array_update_value;
		State->Args[0] = *(ml_value_t **)Address;
		break;
	}
	default: ML_ERROR("ArrayError", "Unsupported format");
	}
	return ml_call(State, Function, 1, State->Args);
}

#ifdef __USE_ML_CBOR

static void ml_cbor_write_array_dim(int Degree, ml_array_dimension_t *Dimension, char *Address, char *Data, ml_cbor_write_fn WriteFn) {
	if (Degree < 0) {
		WriteFn(Data, (unsigned char *)Address, Dimension->Size * Dimension->Stride);
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
		[ML_ARRAY_FORMAT_I8] = 72,
		[ML_ARRAY_FORMAT_U8] = 64,
		[ML_ARRAY_FORMAT_I16] = 77,
		[ML_ARRAY_FORMAT_U16] = 69,
		[ML_ARRAY_FORMAT_I32] = 78,
		[ML_ARRAY_FORMAT_U32] = 70,
		[ML_ARRAY_FORMAT_I64] = 79,
		[ML_ARRAY_FORMAT_U64] = 71,
		[ML_ARRAY_FORMAT_F32] = 85,
		[ML_ARRAY_FORMAT_F64] = 86,
		[ML_ARRAY_FORMAT_ANY] = 41
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
	if (ml_list_length(Args[0]) != 2) return ml_error("CborError", "Invalid multi-dimensional array");
	ml_value_t *Dimensions = ml_list_get(Args[0], 1);
	if (!ml_is(Dimensions, MLListT)) return ml_error("CborError", "Invalid multi-dimensional array");
	ml_array_t *Source = (ml_array_t *)ml_list_get(Args[0], 2);
	if (!ml_is((ml_value_t *)Source, MLArrayT)) return ml_error("CborError", "Invalid multi-dimensional array");
	ml_array_t *Target = ml_array_new(Source->Format, ml_list_length(Dimensions));
	ml_array_dimension_t *Dimension = Target->Dimensions + Target->Degree;
	int Stride = MLArraySizes[Source->Format];
	ML_LIST_REVERSE(Dimensions, Iter) {
		--Dimension;
		Dimension->Stride = Stride;
		int Size = Dimension->Size = ml_integer_value(Iter->Value);
		Stride *= Size;
	}
	if (Stride != Source->Base.Size) return ml_error("CborError", "Invalid multi-dimensional array");
	Target->Base.Size = Stride;
	Target->Base.Address = Source->Base.Address;
	return (ml_value_t *)Target;
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
	return (ml_value_t *)Array;
}

#endif

void ml_array_init(stringmap_t *Globals) {
#include "ml_array_init.c"
	MLArrayAnyT->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_ANY, ml_array_typed_new_fnx);
	MLArrayInt8T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_I8, ml_array_typed_new_fnx);
	MLArrayUInt8T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_U8, ml_array_typed_new_fnx);
	MLArrayInt16T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_I16, ml_array_typed_new_fnx);
	MLArrayUInt16T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_U16, ml_array_typed_new_fnx);
	MLArrayInt32T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_I32, ml_array_typed_new_fnx);
	MLArrayUInt32T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_U32, ml_array_typed_new_fnx);
	MLArrayInt64T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_I64, ml_array_typed_new_fnx);
	MLArrayUInt64T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_U64, ml_array_typed_new_fnx);
	MLArrayFloat32T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_F32, ml_array_typed_new_fnx);
	MLArrayFloat64T->Constructor = ml_cfunctionx((void *)ML_ARRAY_FORMAT_F64, ml_array_typed_new_fnx);
	ml_method_by_name("set", 0 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("add", 1 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("sub", 2 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("mul", 3 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("div", 4 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("+", 1 + (char *)0, array_infix_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("-", 2 + (char *)0, array_infix_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("*", 3 + (char *)0, array_infix_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("/", 4 + (char *)0, array_infix_fn, MLArrayT, MLArrayT, NULL);
	stringmap_insert(MLArrayT->Exports, "new", ml_cfunctionx(NULL, ml_array_new_fnx));
	stringmap_insert(MLArrayT->Exports, "wrap", ml_cfunction(NULL, ml_array_wrap_fn));
	MLArrayT->Constructor = ml_cfunction(NULL, ml_array_of_fn);
	stringmap_insert(MLArrayT->Exports, "any", MLArrayAnyT);
	stringmap_insert(MLArrayT->Exports, "int8", MLArrayInt8T);
	stringmap_insert(MLArrayT->Exports, "uint8", MLArrayUInt8T);
	stringmap_insert(MLArrayT->Exports, "int16", MLArrayInt16T);
	stringmap_insert(MLArrayT->Exports, "uint16", MLArrayUInt16T);
	stringmap_insert(MLArrayT->Exports, "int32", MLArrayInt32T);
	stringmap_insert(MLArrayT->Exports, "uint32", MLArrayUInt32T);
	stringmap_insert(MLArrayT->Exports, "int64", MLArrayInt64T);
	stringmap_insert(MLArrayT->Exports, "uint64", MLArrayUInt64T);
	stringmap_insert(MLArrayT->Exports, "float32", MLArrayFloat32T);
	stringmap_insert(MLArrayT->Exports, "float64", MLArrayFloat64T);
	if (Globals) {
		stringmap_insert(Globals, "array", MLArrayT);
		
	}
}

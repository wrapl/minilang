#include <libgen.h>

#include "../ml_array.h"
#include "../ml_macros.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

ML_TYPE(MLArrayT, (MLBufferT), "array");
ML_TYPE(MLArrayAnyT, (MLArrayT), "value-array");

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
	Array->Base.Value = GC_MALLOC_ATOMIC(DataSize);
	Array->Base.Length = DataSize;
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
	Value = ml_deref(Value);
	if (Value->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Value);
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
		int I = 0;
		ML_LIST_FOREACH(Args[1], Iter) {
			if (Iter->Value->Type != MLIntegerT) ML_RETURN(ml_error("TypeError", "Dimension is not an integer"));
			Array->Dimensions[I++].Size = ml_integer_value(Iter->Value);
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
	Array->Base.Value = GC_MALLOC_ATOMIC(DataSize);
	Array->Base.Length = DataSize;
	if (Count == 2) ML_RETURN(Array);
	ml_array_init_state_t *State = xnew(ml_array_init_state_t, Array->Degree, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.run = (void *)ml_array_init_run;
	State->Base.Context = Caller->Context;
	State->Address = Array->Base.Value;
	State->Array = Array;
	ml_value_t *Function = State->Function = Args[2];
	for (int I = 0; I < Array->Degree; ++I) State->Args[I] = ml_integer(1);
	return ml_call(State, Function, Array->Degree, State->Args);
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
		if (Size->Type != MLIntegerT) return ml_error("TypeError", "Dimension is not an integer");
		if (Stride->Type != MLIntegerT) return ml_error("TypeError", "Stride is not an integer");
		Array->Dimensions[I].Size = ml_integer_value(Size);
		Array->Dimensions[I].Stride = ml_integer_value(Stride);
	}
	Array->Base.Value = ((ml_buffer_t *)Args[1])->Value;
	Array->Base.Length = ((ml_buffer_t *)Args[1])->Length;
	return (ml_value_t *)Array;
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
	return (ml_value_t *)Target;
}

ML_METHOD("permute", MLArrayT, MLListT) {
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	if (ml_list_length(Args[1]) != Degree) return ml_error("ArrayError", "List length must match degree");
	ml_array_t *Target = ml_array_new(Source->Format, Degree);
	int I = 0;
	ML_LIST_FOREACH(Args[1], Iter) {
		if (Iter->Value->Type != MLIntegerT) return ml_error("ArrayError", "Invalid index");
		int J = ml_integer_value(Iter->Value);
		if (J <= 0) J += Degree + 1;
		if (J < 1 || J > Degree) return ml_error("ArrayError", "Invalid index");
		Target->Dimensions[I++] = Source->Dimensions[J - 1];
	}
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
	char *Address = Source->Base.Value;
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
			ML_LIST_FOREACH(Index, Iter) {
				int IndexValue = ml_integer_value(Iter->Value);
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
	Target->Base.Value = Address;
	return (ml_value_t *)Target;
}

ML_METHODV("[]", MLArrayT) {
	ml_array_t *Source = (ml_array_t *)Args[0];
	return ml_array_index_internal(Source, Count - 1, Args + 1);
}

ML_METHOD("[]", MLArrayT, MLMapT) {
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
	char *Address = Array->Base.Value;
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
//UPDATE_ROW_FN(NAME, OP, TARGET, value)

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
//UPDATE_ROW_TARGET_IMPL(NAME, OP, value)

UPDATE_ROW_OPS_IMPL(set, =)
UPDATE_ROW_OPS_IMPL(add, +=)
UPDATE_ROW_OPS_IMPL(sub, -=)
UPDATE_ROW_OPS_IMPL(mul, *=)
UPDATE_ROW_OPS_IMPL(div, /=)

#define UPDATE_ROW_ENTRY(INDEX, NAME, TARGET, SOURCE) \
	[INDEX] = NAME ## _row_ ## TARGET ## _ ## SOURCE

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
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, NAME, TARGET, double) \
//UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) +  + ML_ARRAY_FORMAT_ANY, NAME, TARGET, value)

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
UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, NAME, double) \
//UPDATE_ROW_TARGET_ENTRIES(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_ANY, NAME, OP, TARGET, value)

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
		update_prefix(Op, PrefixDegree, Target->Dimensions, Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value);
	} else {
		ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}};
		UpdateRowFns[Op](ValueDimension, Target->Base.Value, ValueDimension, Source->Base.Value);
	}
	return Args[0];
}

#define UPDATE_METHOD(NAME, BASE, ATYPE, CTYPE, RFUNC, FORMAT) \
\
ML_METHOD(#NAME, ATYPE, MLNumberT) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	CTYPE Value = RFUNC(Args[1]); \
	ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}}; \
	int Op = (BASE) * MAX_FORMATS * MAX_FORMATS + Array->Format * MAX_FORMATS + FORMAT; \
	if (!UpdateRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", Args[0]->Type->Name, Args[1]->Type->Name); \
	if (Array->Degree == 0) { \
		UpdateRowFns[Op](ValueDimension, Array->Base.Address, ValueDimension, (char *)&Value); \
	} else { \
		update_prefix(Op, Array->Degree - 1, Array->Dimensions, Array->Base.Address, 0, ValueDimension, (char *)&Value); \
	} \
	return Args[0]; \
}

#define UPDATE_METHODS(ATYPE, CTYPE, RFUNC, FORMAT) \
UPDATE_METHOD(set, 0, ATYPE, CTYPE, RFUNC, FORMAT); \
UPDATE_METHOD(add, 1, ATYPE, CTYPE, RFUNC, FORMAT); \
UPDATE_METHOD(sub, 2, ATYPE, CTYPE, RFUNC, FORMAT); \
UPDATE_METHOD(mul, 3, ATYPE, CTYPE, RFUNC, FORMAT); \
UPDATE_METHOD(div, 4, ATYPE, CTYPE, RFUNC, FORMAT);

typedef struct call_info_t {
	int Count;
	ml_value_t *Function;
	ml_value_t *Result;
	ml_value_t *Args[];
} call_info_t;

#define METHODS(ATYPE, CTYPE, PRINTF, RFUNC, RNEW, FORMAT) \
\
static ml_value_t *ML_TYPED_FN(ml_array_value, ATYPE, ml_array_t *Array, char *Address) { \
	return RNEW(*(CTYPE *)Array->Base.Address); \
} \
\
static void append_array_ ## CTYPE(ml_stringbuffer_t *Buffer, int Degree, ml_array_dimension_t *Dimension, char *Address) { \
	ml_stringbuffer_add(Buffer, "[", 1); \
	int Stride = Dimension->Stride; \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Size) { \
			if (Degree == 1) { \
				ml_stringbuffer_addf(Buffer, PRINTF, *(CTYPE *)(Address + (Indices[0]) * Dimension->Stride)); \
				for (int I = 1; I < Dimension->Size; ++I) { \
					ml_stringbuffer_addf(Buffer, ", "PRINTF, *(CTYPE *)(Address + (Indices[I]) * Stride)); \
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
			ml_stringbuffer_addf(Buffer, PRINTF, *(CTYPE *)Address); \
			Address += Stride; \
			for (int I = Dimension->Size; --I > 0;) { \
				ml_stringbuffer_addf(Buffer, ", "PRINTF, *(CTYPE *)Address); \
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
		return ml_string_format(PRINTF, *(CTYPE *)Array->Base.Address); \
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
		return ml_string_format(PRINTF, *(CTYPE *)Array->Base.Address); \
	} else { \
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT}; \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Address); \
		return ml_stringbuffer_get_string(Buffer); \
	} \
} \
static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, ATYPE, ml_stringbuffer_t *Buffer, ml_array_t *Array) { \
	if (Array->Degree == 0) { \
		ml_stringbuffer_addf(Buffer, PRINTF, *(CTYPE *)Array->Base.Address); \
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
		ml_stringbuffer_addf(Buffer, PRINTF, *(CTYPE *)Array->Base.Address); \
	} else { \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Address); \
	} \
	return Args[0]; \
} \
\
UPDATE_METHODS(ATYPE, CTYPE, RFUNC, FORMAT); \
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
	if (Target < 1 || Target > Array->Degree) return ml_error("ArrayError", "Dimension index invalid"); \
	Target = Array->Degree + 1 - Target; \
	partial_sums_ ## CTYPE(Target, Array->Degree, Array->Dimensions, Array->Base.Address, 0); \
	return Args[0]; \
} \
\
CTYPE ml_array_get_ ## CTYPE(ml_array_t *Array, ...) { \
	va_list Indices; \
	va_start(Indices, Array); \
	char *Address = ml_array_index(Array, Indices); \
	va_end(Indices); \
	switch (Array->Format) { \
	case ML_ARRAY_FORMAT_NONE: break; \
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
	case ML_ARRAY_FORMAT_ANY: return RFUNC(*(ml_value_t **)Address); \
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
	case ML_ARRAY_FORMAT_ANY: *(ml_value_t **)Address = RNEW(Value); break; \
	} \
} \
\
static ml_value_t *ml_array_ ## CTYPE ## _deref(ml_array_t *Target, ml_value_t *Value) { \
	if (Target->Degree == 0)  return RNEW(*(CTYPE *)Target->Base.Address); \
	return (ml_value_t *)Target; \
} \
\
static ml_value_t *ml_array_ ## CTYPE ## _assign(ml_array_t *Target, ml_value_t *Value) { \
	for (;;) if (ml_is(Value, MLNumberT)) { \
		CTYPE CValue = RFUNC(Value); \
		ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}}; \
		int Op = Target->Format * MAX_FORMATS + Target->Format; \
		if (!UpdateRowFns[Op]) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", Target->Base.Type->Name, Value->Type->Name); \
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
} \
\
ML_TYPE(ATYPE, (MLArrayT), #CTYPE "-array", \
	.deref = (void *)ml_array_ ## CTYPE ## _deref, \
	.assign = (void *)ml_array_ ## CTYPE ## _assign \
);

METHODS(MLArrayInt8T, int8_t, "%d", ml_integer_value, ml_integer, ML_ARRAY_FORMAT_I8);
METHODS(MLArrayUInt8T, uint8_t, "%ud", ml_integer_value, ml_integer, ML_ARRAY_FORMAT_U8);
METHODS(MLArrayInt16T, int16_t, "%d", ml_integer_value, ml_integer, ML_ARRAY_FORMAT_I16);
METHODS(MLArrayUInt16T, uint16_t, "%ud", ml_integer_value, ml_integer, ML_ARRAY_FORMAT_U16);
METHODS(MLArrayInt32T, int32_t, "%d", ml_integer_value, ml_integer, ML_ARRAY_FORMAT_I32);
METHODS(MLArrayUInt32T, uint32_t, "%ud", ml_integer_value, ml_integer, ML_ARRAY_FORMAT_U32);
METHODS(MLArrayInt64T, int64_t, "%ld", ml_integer_value, ml_integer, ML_ARRAY_FORMAT_I64);
METHODS(MLArrayUInt64T, uint64_t, "%lud", ml_integer_value, ml_integer, ML_ARRAY_FORMAT_U64);
METHODS(MLArrayFloat32T, float, "%f", ml_real_value, ml_real, ML_ARRAY_FORMAT_F32);
METHODS(MLArrayFloat64T, double, "%f", ml_real_value, ml_real, ML_ARRAY_FORMAT_F64);

static int array_copy(ml_array_t *Target, ml_array_t *Source) {
	int Degree = Source->Degree;
	int DataSize = MLArraySizes[Target->Format];
	for (int I = Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Value = GC_MALLOC_ATOMIC(DataSize);
	int Op1 = Target->Format * MAX_FORMATS + Source->Format;
	update_array(Op1, Target->Dimensions, Target->Base.Value, Degree, Source->Dimensions, Source->Base.Value);
	return DataSize;
}

#define MAX(X, Y) ((X > Y) ? X : Y)

static ml_value_t *array_infix_fn(void *Data, int Count, ml_value_t **Args) {
	ml_array_t *A = (ml_array_t *)Args[0];
	ml_array_t *B = (ml_array_t *)Args[1];
	int Degree = A->Degree;
	ml_array_t *C = ml_array_new(MAX(A->Format, B->Format), Degree);
	array_copy(C, A);
	int Op2 = ((char *)Data - (char *)0) * MAX_FORMATS * MAX_FORMATS + C->Format * MAX_FORMATS + B->Format;
	update_prefix(Op2, C->Degree - B->Degree, C->Dimensions, C->Base.Value, B->Degree, B->Dimensions, B->Base.Value);
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
ML_METHOD(#OP, MLArrayT, MLDoubleT) { \
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
ML_METHOD(#OP, MLDoubleT, MLArrayT) { \
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
	if (Value->Type == MLListT) {
		int Size = ml_list_length(Value);
		if (!Size) return (ml_array_t *)ml_error("ValueError", "Empty dimension in array");
		ml_array_t *Array = ml_array_of_create(ml_list_get(Value, 1), Degree + 1, Format);
		if (Array->Base.Type == MLErrorT) return Array;
		Array->Dimensions[Degree].Size = Size;
		if (Degree < Array->Degree - 1) {
			Array->Dimensions[Degree].Stride = Array->Dimensions[Degree + 1].Size * Array->Dimensions[Degree + 1].Stride;
		}
		return Array;
	} else if (Value->Type == MLTupleT) {
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
	if (Value->Type == MLListT) {
		if (!Degree) return ml_error("ValueError", "Inconsistent depth in array");
		if (ml_list_length(Value) != Dimension->Size) return ml_error("ValueError", "Inconsistent lengths in array");
		ML_LIST_FOREACH(Value, Iter) {
			ml_value_t *Error = ml_array_of_fill(Format, Dimension + 1, Address, Degree - 1, Iter->Value);
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
	} else if (ml_is(Value, MLArrayT)) {
		ml_array_t *Source = (ml_array_t *)Value;
		if (Source->Degree != Degree) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
		for (int I = 0; I < Degree; ++I) {
			if (Dimension[I].Size != Source->Dimensions[I].Size) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
		}
		int Op = Format * MAX_FORMATS + Source->Format;
		update_array(Op, Dimension, Address, Degree, Source->Dimensions, Source->Base.Value);
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
	if (Value->Type == MLListT) {
		ML_LIST_FOREACH(Value, Iter) {
			Format = ml_array_of_type_guess(Iter->Value, Format);
		}
	} else if (Value->Type == MLTupleT) {
		ml_tuple_t *Tuple = (ml_tuple_t *)Value;
		for (int I = 0; I < Tuple->Size; ++I) {
			Format = ml_array_of_type_guess(Tuple->Values[I], Format);
		}
	} else if (ml_is(Value, MLArrayT)) {
		ml_array_t *Array = (ml_array_t *)Value;
		if (Format <= Array->Format) Format = Array->Format;
	} else if (Value->Type == MLDoubleT) {
		if (Format < ML_ARRAY_FORMAT_F64) Format = ML_ARRAY_FORMAT_F64;
	} else if (Value->Type == MLIntegerT) {
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
		Size = Array->Base.Length = Array->Dimensions[0].Stride * Array->Dimensions[0].Size;
	} else {
		Size = MLArraySizes[Array->Format];
	}
	char *Address = Array->Base.Value = GC_MALLOC_ATOMIC(Size);
	ml_array_of_fill(Array->Format, Array->Dimensions, Address, Array->Degree, Source);
	return (ml_value_t *)Array;
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
	Target->Base.Value = GC_MALLOC_ATOMIC(DataSize);
	if (Degree == 0) {
		memcpy(Target->Base.Value, Source->Base.Value, DataSize);
	} else {
		int Op = Target->Format * MAX_FORMATS + Source->Format;
		update_array(Op, Target->Dimensions, Target->Base.Value, Degree, Source->Dimensions, Source->Base.Value);
	}
	return (ml_value_t *)Target;
}

#include "../ml_cbor.h"

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
	ml_cbor_write_array_dim(FlatDegree, Array->Dimensions, Array->Base.Value, Data, WriteFn);
}

static ml_value_t *ml_cbor_read_multi_array_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_TYPE(0, MLListT);
	if (ml_list_length(Args[0]) != 2) return ml_error("CborError", "Invalid multi-dimensional array");
	ml_value_t *Dimensions = ml_list_get(Args[0], 1);
	if (Dimensions->Type != MLListT) return ml_error("CborError", "Invalid multi-dimensional array");
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
	if (Stride != Source->Base.Length) return ml_error("CborError", "Invalid multi-dimensional array");
	Target->Base.Length = Stride;
	Target->Base.Value = Source->Base.Value;
	return (ml_value_t *)Target;
}

static ml_value_t *ml_cbor_read_typed_array_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_TYPE(0, MLBufferT);
	ml_buffer_t *Buffer = (ml_buffer_t *)Args[0];
	ml_array_format_t Format = (intptr_t)Data;
	int ItemSize = MLArraySizes[Format];
	ml_array_t *Array = ml_array_new(Format, 1);
	Array->Dimensions[0].Size = Buffer->Length / ItemSize;
	Array->Dimensions[0].Stride = ItemSize;
	Array->Base.Length = Buffer->Length;
	Array->Base.Value = Buffer->Value;
	return (ml_value_t *)Array;
}

void ml_library_entry(ml_value_t *Module, ml_getter_t GlobalGet, void *Globals) {
	const char *Dir = dirname(GC_strdup(ml_module_path(Module)));
	ml_value_t *Import = GlobalGet(Globals, "import");
	ml_value_t *Cbor = ml_inline(Import, 1, ml_string_format("%s/ml_cbor.so", Dir));

#include "ml_array_init.c"
	ml_method_by_name("set", 0 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("add", 1 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("sub", 2 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("mul", 3 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("div", 4 + (char *)0, update_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("+", 1 + (char *)0, array_infix_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("-", 2 + (char *)0, array_infix_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("*", 3 + (char *)0, array_infix_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("/", 4 + (char *)0, array_infix_fn, MLArrayT, MLArrayT, NULL);
	ml_value_t *CborDefault = ml_inline(SymbolMethod, 2, Cbor, ml_cstring("Default"));
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

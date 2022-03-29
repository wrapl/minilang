#include "../ml_array.h"

#define UPDATE(OP, TARGET, SOURCE) { \
	typeof(TARGET) Target = TARGET; \
	typeof(SOURCE) Source = SOURCE; \
	*Target = OP(*Target, Source); \
}

#define UPDATE_ROW_IMPL(NAME, OP, TARGET, SOURCE) \
\
void NAME ## _row_ ## TARGET ## _ ## SOURCE(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData) { \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				UPDATE(OP, (TARGET *)(TargetData + TargetIndices[I] * TargetDimension->Stride), *(SOURCE *)(SourceData + SourceIndices[I] * SourceDimension->Stride)); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				UPDATE(OP, (TARGET *)(TargetData + TargetIndices[I] * TargetDimension->Stride), *(SOURCE *)SourceData); \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				UPDATE(OP, (TARGET *)TargetData, *(SOURCE *)(SourceData + SourceIndices[I] * SourceDimension->Stride)); \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				UPDATE(OP, (TARGET *)TargetData, *(SOURCE *)SourceData); \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
}

#define ml_number(X) _Generic(X, ml_value_t *: ml_nop, double: ml_real, default: ml_integer)(X)

#define UPDATE_ROW_VALUE_IMPL(NAME, OP, SOURCE) \
\
void NAME ## _row_any_ ## SOURCE(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData) { \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t **Target = (ml_value_t **)(TargetData + TargetIndices[I] * TargetDimension->Stride); \
				ml_value_t *Source = ml_number(*(SOURCE *)(SourceData + SourceIndices[I] * SourceDimension->Stride)); \
				*Target = OP(*Target, Source); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t **Target = (ml_value_t **)(TargetData + TargetIndices[I] * TargetDimension->Stride); \
				ml_value_t *Source = ml_number(*(SOURCE *)SourceData); \
				*Target = OP(*Target, Source); \
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
				*Target = OP(*Target, Source); \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				ml_value_t **Target = (ml_value_t **)TargetData; \
				ml_value_t *Source = ml_number(*(SOURCE *)SourceData); \
				*Target = OP(*Target, Source); \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
}

#define ml_number_value(T, X) _Generic(T, double: ml_real_value, default: ml_integer_value)(X)

#define UPDATE_ROW_IMPL_VALUE(NAME, OP, TARGET) \
\
void NAME ## _row_ ## TARGET ## _any(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData) { \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				TARGET Source = ml_number_value((TARGET)0, *(ml_value_t **)(SourceData + SourceIndices[I] * SourceDimension->Stride)); \
				UPDATE(OP, (TARGET *)(TargetData + TargetIndices[I] * TargetDimension->Stride), Source); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				TARGET Source = ml_number_value((TARGET)0, *(ml_value_t **)SourceData); \
				UPDATE(OP, (TARGET *)(TargetData + TargetIndices[I] * TargetDimension->Stride), Source); \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				TARGET Source = ml_number_value((TARGET)0, *(ml_value_t **)(SourceData + SourceIndices[I] * SourceDimension->Stride)); \
				UPDATE(OP, (TARGET *)TargetData, Source); \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				TARGET Source = ml_number_value((TARGET)0, *(ml_value_t **)SourceData); \
				UPDATE(OP, (TARGET *)TargetData, Source); \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
}

#define UPDATE_ROW_VALUE_IMPL_VALUE(NAME, OP) \
\
void NAME ## _row_any_any(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData) { \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Source = *(ml_value_t **)(SourceData + SourceIndices[I] * SourceDimension->Stride); \
				ml_value_t **Target = (ml_value_t **)(TargetData + TargetIndices[I] * TargetDimension->Stride); \
				*Target = OP(*Target, Source); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Source = *(ml_value_t **)SourceData; \
				ml_value_t **Target = (ml_value_t **)(TargetData + TargetIndices[I] * TargetDimension->Stride); \
				*Target = OP(*Target, Source); \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Source = *(ml_value_t **)(SourceData + SourceIndices[I] * SourceDimension->Stride); \
				ml_value_t **Target = (ml_value_t **)TargetData; \
				*Target = OP(*Target, Source); \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				ml_value_t *Source = *(ml_value_t **)SourceData; \
				ml_value_t **Target = (ml_value_t **)TargetData; \
				*Target = OP(*Target, Source); \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
}

#define UPDATE_ROW_TARGET_IMPL_BASE(NAME, OP, TARGET) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, uint8_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, int8_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, uint16_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, int16_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, uint32_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, int32_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, uint64_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, int64_t) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, float) \
UPDATE_ROW_IMPL(NAME, OP, TARGET, double) \
UPDATE_ROW_IMPL_VALUE(NAME, OP, TARGET)

#define UPDATE_ROW_TARGET_IMPL(NAME, OP, TARGET) \
UPDATE_ROW_TARGET_IMPL_BASE(NAME, OP, TARGET)

#define UPDATE_ROW_TARGET_VALUE_IMPL_BASE(NAME, OP) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, uint8_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, int8_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, uint16_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, int16_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, uint32_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, int32_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, uint64_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, int64_t) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, float) \
UPDATE_ROW_VALUE_IMPL(NAME, OP, double) \
UPDATE_ROW_VALUE_IMPL_VALUE(NAME, OP)

#define UPDATE_ROW_TARGET_VALUE_IMPL(NAME, OP) \
UPDATE_ROW_TARGET_VALUE_IMPL_BASE(NAME, OP)

#define UPDATE_ROW_OPS_IMPL_BASE(NAME, OP, OP2) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, uint8_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, int8_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, uint16_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, int16_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, uint32_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, int32_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, uint64_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, int64_t) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, float) \
UPDATE_ROW_TARGET_IMPL(NAME, OP, double) \
UPDATE_ROW_TARGET_VALUE_IMPL(NAME, OP2)

#define UPDATE_ROW_OPS_IMPL(NAME, OP, OP2) \
UPDATE_ROW_OPS_IMPL_BASE(NAME, OP, OP2)

#define UPDATE_ROW_ENTRY(INDEX, NAME, TARGET, SOURCE) \
	[INDEX] = NAME ## _row_ ## TARGET ## _ ## SOURCE

#define UPDATE_ROW_VALUE_ENTRY(INDEX, NAME, SOURCE) \
	[INDEX] = NAME ## _row_any_ ## SOURCE

#define UPDATE_ROW_ENTRY_VALUE(INDEX, NAME, TARGET) \
	[INDEX] = NAME ## _row_ ## TARGET ## _any

#define UPDATE_ROW_VALUE_ENTRY_VALUE(INDEX, NAME) \
	[INDEX] = NAME ## _row_any_any

#define UPDATE_ROW_TARGET_ENTRIES_BASE(INDEX, NAME, TARGET) \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U8, NAME, TARGET, uint8_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I8, NAME, TARGET, int8_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U16, NAME, TARGET, uint16_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I16, NAME, TARGET, int16_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U32, NAME, TARGET, uint32_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I32, NAME, TARGET, int32_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U64, NAME, TARGET, uint64_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I64, NAME, TARGET, int64_t), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F32, NAME, TARGET, float), \
UPDATE_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, NAME, TARGET, double), \
UPDATE_ROW_ENTRY_VALUE(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_ANY, NAME, TARGET)

#define UPDATE_ROW_TARGET_ENTRIES(INDEX, NAME, TARGET) \
UPDATE_ROW_TARGET_ENTRIES_BASE(INDEX, NAME, TARGET)

#define UPDATE_ROW_VALUE_TARGET_ENTRIES_BASE(INDEX, NAME) \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U8, NAME, uint8_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I8, NAME, int8_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U16, NAME, uint16_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I16, NAME, int16_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U32, NAME, uint32_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I32, NAME, int32_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U64, NAME, uint64_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I64, NAME, int64_t), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F32, NAME, float), \
UPDATE_ROW_VALUE_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, NAME, double), \
UPDATE_ROW_VALUE_ENTRY_VALUE(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_ANY, NAME)

#define UPDATE_ROW_VALUE_TARGET_ENTRIES(INDEX, NAME) \
UPDATE_ROW_VALUE_TARGET_ENTRIES_BASE(INDEX, NAME)

#define UPDATE_ROW_OPS_ENTRIES_BASE(NAME) \
UPDATE_ROW_TARGET_ENTRIES(ML_ARRAY_FORMAT_U8, NAME, uint8_t), \
UPDATE_ROW_TARGET_ENTRIES(ML_ARRAY_FORMAT_I8, NAME, int8_t), \
UPDATE_ROW_TARGET_ENTRIES(ML_ARRAY_FORMAT_U16, NAME, uint16_t), \
UPDATE_ROW_TARGET_ENTRIES(ML_ARRAY_FORMAT_I16, NAME, int16_t), \
UPDATE_ROW_TARGET_ENTRIES(ML_ARRAY_FORMAT_U32, NAME, uint32_t), \
UPDATE_ROW_TARGET_ENTRIES(ML_ARRAY_FORMAT_I32, NAME, int32_t), \
UPDATE_ROW_TARGET_ENTRIES(ML_ARRAY_FORMAT_U64, NAME, uint64_t), \
UPDATE_ROW_TARGET_ENTRIES(ML_ARRAY_FORMAT_I64, NAME, int64_t), \
UPDATE_ROW_TARGET_ENTRIES(ML_ARRAY_FORMAT_F32, NAME, float), \
UPDATE_ROW_TARGET_ENTRIES(ML_ARRAY_FORMAT_F64, NAME, double), \
UPDATE_ROW_VALUE_TARGET_ENTRIES(ML_ARRAY_FORMAT_ANY, NAME)

#define UPDATE_ROW_OPS_ENTRIES(NAME) \
UPDATE_ROW_OPS_ENTRIES_BASE(NAME)

typedef void (*update_row_fn_t)(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData);

#define UPDATE_FNS(TITLE, NAME, OP, OP2) \
	UPDATE_ROW_OPS_IMPL(NAME, OP, OP2) \
\
update_row_fn_t Update ## TITLE ## RowFns[MAX_FORMATS * MAX_FORMATS] = { \
	UPDATE_ROW_OPS_ENTRIES(NAME) \
}

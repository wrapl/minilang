#include "../ml_array.h"

#define UPDATE(OP, TARGET, SOURCE) { \
	typeof(TARGET) _Target = TARGET; \
	typeof(SOURCE) _Source = SOURCE; \
	*_Target = OP(*_Target, _Source); \
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


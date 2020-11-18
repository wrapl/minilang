#include "../ml_array.h"

#define COMPARE_ROW_IMPL(NAME, OP, LEFT, RIGHT) \
\
void NAME ## _row_ ## LEFT ## _ ## RIGHT(char *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData) { \
	int Size = LeftDimension->Size; \
	if (LeftDimension->Indices) { \
		int *LeftIndices = LeftDimension->Indices; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				*(Target++) = *(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride) OP *(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride); \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				*(Target++) = *(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride) OP *(RIGHT *)RightData; \
				RightData += RightStride; \
			} \
		} \
	} else { \
		int LeftStride = LeftDimension->Stride; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				*(Target++) = *(LEFT *)LeftData OP *(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride); \
				LeftData += LeftStride; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				*(Target++) = *(LEFT *)LeftData OP *(RIGHT *)RightData; \
				LeftData += LeftStride; \
				RightData += RightStride; \
			} \
		} \
	} \
}

#define COMPARE_ROW_LEFT_IMPL(NAME, OP, LEFT) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, int8_t) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, uint8_t) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, int16_t) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, uint16_t) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, int32_t) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, uint32_t) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, int64_t) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, uint64_t) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, float) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, double)

#define COMPARE_ROW_LEFT_VALUE_IMPL(NAME, OP) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, int8_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, uint8_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, int16_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, uint16_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, int32_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, uint32_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, int64_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, uint64_t) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, float) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, double)

#define COMPARE_ROW_OPS_IMPL(NAME, OP) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, int8_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, uint8_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, int16_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, uint16_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, int32_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, uint32_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, int64_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, uint64_t) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, float) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, double)

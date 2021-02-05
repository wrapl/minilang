#include "../ml_array.h"

#define COMPARE_ROW_IMPL(NAME, OP, LEFT, RIGHT, LCONV, RCONV) \
\
void NAME ## _row_ ## LEFT ## _ ## RIGHT(char *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData) { \
	int Size = LeftDimension->Size; \
	if (LeftDimension->Indices) { \
		int *LeftIndices = LeftDimension->Indices; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				*(Target++) = LCONV(*(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride)) OP RCONV(*(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride)); \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				*(Target++) = LCONV(*(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride)) OP RCONV(*(RIGHT *)RightData); \
				RightData += RightStride; \
			} \
		} \
	} else { \
		int LeftStride = LeftDimension->Stride; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				*(Target++) = LCONV(*(LEFT *)LeftData) OP RCONV(*(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride)); \
				LeftData += LeftStride; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				*(Target++) = LCONV(*(LEFT *)LeftData) OP RCONV(*(RIGHT *)RightData); \
				LeftData += LeftStride; \
				RightData += RightStride; \
			} \
		} \
	} \
}

#define COMPARE_ROW_LEFT_IMPL_BASE(NAME, OP, LEFT, LCONV) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, int8_t, LCONV, ) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, uint8_t, LCONV, ) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, int16_t, LCONV, ) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, uint16_t, LCONV, ) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, int32_t, LCONV, ) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, uint32_t, LCONV, ) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, int64_t, LCONV, ) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, uint64_t, LCONV, ) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, float, LCONV, ) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, double, LCONV, )

#ifdef ML_COMPLEX

#define COMPARE_ROW_LEFT_IMPL(NAME, OP, LEFT, LCONV) \
COMPARE_ROW_LEFT_IMPL_BASE(NAME, OP, LEFT, LCONV) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, complex_float, LCONV, cabs) \
COMPARE_ROW_IMPL(NAME, OP, LEFT, complex_double, LCONV, cabs)

#else

#define COMPARE_ROW_LEFT_IMPL(NAME, OP, LEFT, LCONV) \
COMPARE_ROW_LEFT_IMPL_BASE(NAME, OP, LEFT, LCONV)

#endif

#define COMPARE_ROW_LEFT_VALUE_IMPL_BASE(NAME, OP) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, int8_t, ) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, uint8_t, ) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, int16_t, ) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, uint16_t, ) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, int32_t, ) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, uint32_t, ) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, int64_t, ) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, uint64_t, ) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, float, ) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, double, )

#ifdef ML_COMPLEX

#define COMPARE_ROW_LEFT_VALUE_IMPL(NAME, OP) \
COMPARE_ROW_LEFT_VALUE_IMPL_BASE(NAME, OP) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, complex_float, cabs) \
COMPARE_ROW_VALUE_IMPL(NAME, OP, complex_double, cabs)

#else

#define COMPARE_ROW_LEFT_VALUE_IMPL(NAME, OP) \
COMPARE_ROW_LEFT_VALUE_IMPL_BASE(NAME, OP)

#endif

#define COMPARE_ROW_OPS_IMPL_BASE(NAME, OP) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, int8_t, ) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, uint8_t, ) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, int16_t, ) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, uint16_t, ) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, int32_t, ) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, uint32_t, ) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, int64_t, ) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, uint64_t, ) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, float, ) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, double, )

#ifdef ML_COMPLEX

#define COMPARE_ROW_OPS_IMPL(NAME, OP) \
COMPARE_ROW_OPS_IMPL_BASE(NAME, OP) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, complex_float, cabs) \
COMPARE_ROW_LEFT_IMPL(NAME, OP, complex_double, cabs)

#else

#define COMPARE_ROW_OPS_IMPL(NAME, OP) \
COMPARE_ROW_OPS_IMPL_BASE(NAME, OP)

#endif

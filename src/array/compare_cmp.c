#include "../ml_array.h"

#ifdef ML_COMPLEX
#include <complex.h>
#undef I
#endif

#define CMP_ROW_IMPL(LEFT, RIGHT, LTYPE, RTYPE, LCONV, RCONV) \
\
int cmp_row_ ## LEFT ## _ ## RIGHT(ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData) { \
	int Size = LeftDimension->Size; \
	if (LeftDimension->Indices) { \
		int *LeftIndices = LeftDimension->Indices; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				LTYPE Left = LCONV(*(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride)); \
				RTYPE Right = RCONV(*(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride)); \
				if (Left < Right) return -1; \
				if (Left > Right) return 1; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				LTYPE Left = LCONV(*(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride)); \
				RTYPE Right = RCONV(*(RIGHT *)RightData); \
				if (Left < Right) return -1; \
				if (Left > Right) return 1; \
				RightData += RightStride; \
			} \
		} \
	} else { \
		int LeftStride = LeftDimension->Stride; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				LTYPE Left = LCONV(*(LEFT *)LeftData); \
				RTYPE Right = RCONV(*(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride)); \
				if (Left < Right) return -1; \
				if (Left > Right) return 1; \
				LeftData += LeftStride; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				LTYPE Left = LCONV(*(LEFT *)LeftData); \
				RTYPE Right = RCONV(*(RIGHT *)RightData); \
				if (Left < Right) return -1; \
				if (Left > Right) return 1; \
				LeftData += LeftStride; \
				RightData += RightStride; \
			} \
		} \
	} \
	return 0; \
}

#define CMP_ROW_LEFT_IMPL_BASE(LEFT, LTYPE, LCONV) \
CMP_ROW_IMPL(LEFT, int8_t, LTYPE, int8_t, LCONV,) \
CMP_ROW_IMPL(LEFT, uint8_t, LTYPE, uint8_t, LCONV,) \
CMP_ROW_IMPL(LEFT, int16_t, LTYPE, int16_t, LCONV,) \
CMP_ROW_IMPL(LEFT, uint16_t, LTYPE, uint16_t, LCONV,) \
CMP_ROW_IMPL(LEFT, int32_t, LTYPE, int32_t, LCONV,) \
CMP_ROW_IMPL(LEFT, uint32_t, LTYPE, uint32_t, LCONV,) \
CMP_ROW_IMPL(LEFT, int64_t, LTYPE, int64_t, LCONV,) \
CMP_ROW_IMPL(LEFT, uint64_t, LTYPE, uint64_t, LCONV,) \
CMP_ROW_IMPL(LEFT, float, LTYPE, float, LCONV,) \
CMP_ROW_IMPL(LEFT, double, LTYPE, double, LCONV,)

#ifdef ML_COMPLEX

#define CMP_ROW_LEFT_IMPL(LEFT, LTYPE, LCONV) \
CMP_ROW_LEFT_IMPL_BASE(LEFT, LTYPE, LCONV) \
CMP_ROW_IMPL(LEFT, complex_float, LTYPE, double, LCONV, cabs) \
CMP_ROW_IMPL(LEFT, complex_double, LTYPE, double, LCONV, cabs)

#else

#define CMP_ROW_LEFT_IMPL(LEFT, LTYPE, LCONV) \
CMP_ROW_LEFT_IMPL_BASE(LEFT, LTYPE, LCONV)

#endif

#define CMP_ROW_LEFT_VALUE_IMPL_BASE() \
CMP_ROW_VALUE_IMPL(int8_t, int8_t,) \
CMP_ROW_VALUE_IMPL(uint8_t, uint8_t,) \
CMP_ROW_VALUE_IMPL(int16_t, int16_t,) \
CMP_ROW_VALUE_IMPL(uint16_t, uint16_t,) \
CMP_ROW_VALUE_IMPL(int32_t, int32_t,) \
CMP_ROW_VALUE_IMPL(uint32_t, uint32_t,) \
CMP_ROW_VALUE_IMPL(int64_t, int64_t,) \
CMP_ROW_VALUE_IMPL(uint64_t, uint64_t,) \
CMP_ROW_VALUE_IMPL(float, float,) \
CMP_ROW_VALUE_IMPL(double, double,)

#ifdef ML_COMPLEX

#define CMP_ROW_LEFT_VALUE_IMPL() \
CMP_ROW_LEFT_VALUE_IMPL_BASE() \
CMP_ROW_VALUE_IMPL(complex_float, double, cabs) \
CMP_ROW_VALUE_IMPL(complex_double, double, cabs)

#else

#define CMP_ROW_LEFT_VALUE_IMPL() \
CMP_ROW_LEFT_VALUE_IMPL_BASE()

#endif

#define CMP_ROW_OPS_IMPL_BASE() \
CMP_ROW_LEFT_IMPL(int8_t, int8_t,) \
CMP_ROW_LEFT_IMPL(uint8_t, uint8_t,) \
CMP_ROW_LEFT_IMPL(int16_t, int16_t,) \
CMP_ROW_LEFT_IMPL(uint16_t, uint16_t,) \
CMP_ROW_LEFT_IMPL(int32_t, int32_t,) \
CMP_ROW_LEFT_IMPL(uint32_t, uint32_t,) \
CMP_ROW_LEFT_IMPL(int64_t, int64_t,) \
CMP_ROW_LEFT_IMPL(uint64_t, uint64_t,) \
CMP_ROW_LEFT_IMPL(float, float,) \
CMP_ROW_LEFT_IMPL(double, double,)

#ifdef ML_COMPLEX

#define CMP_ROW_OPS_IMPL() \
CMP_ROW_OPS_IMPL_BASE() \
CMP_ROW_LEFT_IMPL(complex_float, double, cabs) \
CMP_ROW_LEFT_IMPL(complex_double, double, cabs)

#else

#define CMP_ROW_OPS_IMPL() \
CMP_ROW_OPS_IMPL_BASE()

#endif

CMP_ROW_OPS_IMPL()

#define CMP_ROW_ENTRY(INDEX, LEFT, RIGHT) \
	[INDEX] = cmp_row_ ## LEFT ## _ ## RIGHT

#define CMP_ROW_LEFT_ENTRIES_BASE(INDEX, LEFT) \
CMP_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I8, LEFT, int8_t), \
CMP_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U8, LEFT, uint8_t), \
CMP_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I16, LEFT, int16_t), \
CMP_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U16, LEFT, uint16_t), \
CMP_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I32, LEFT, int32_t), \
CMP_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U32, LEFT, uint32_t), \
CMP_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_I64, LEFT, int64_t), \
CMP_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_U64, LEFT, uint64_t), \
CMP_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F32, LEFT, float), \
CMP_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_F64, LEFT, double)

#ifdef ML_COMPLEX

#define CMP_ROW_LEFT_ENTRIES(INDEX, LEFT) \
CMP_ROW_LEFT_ENTRIES_BASE(INDEX, LEFT), \
CMP_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_C32, LEFT, complex_float), \
CMP_ROW_ENTRY(MAX_FORMATS * (INDEX) + ML_ARRAY_FORMAT_C64, LEFT, complex_double)

#else

#define CMP_ROW_LEFT_ENTRIES(INDEX, LEFT) \
CMP_ROW_LEFT_ENTRIES_BASE(INDEX, LEFT)

#endif

#define CMP_ROW_OPS_ENTRIES_BASE() \
CMP_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_I8, int8_t), \
CMP_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_U8, uint8_t), \
CMP_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_I16, int16_t), \
CMP_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_U16, uint16_t), \
CMP_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_I32, int32_t), \
CMP_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_U32, uint32_t), \
CMP_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_I64, int64_t), \
CMP_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_U64, uint64_t), \
CMP_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_F32, float), \
CMP_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_F64, double)

#ifdef ML_COMPLEX

#define CMP_ROW_OPS_ENTRIES() \
CMP_ROW_OPS_ENTRIES_BASE(), \
CMP_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_C32, complex_float), \
CMP_ROW_LEFT_ENTRIES(ML_ARRAY_FORMAT_C64, complex_double)

#else

#define CMP_ROW_OPS_ENTRIES() \
CMP_ROW_OPS_ENTRIES_BASE()

#endif

typedef int (*cmp_row_fn_t)(ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData);

static cmp_row_fn_t CmpRowFns[] = {
	CMP_ROW_OPS_ENTRIES()
};

static int cmp_array(int Op, int Degree, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData) {
	if (Degree == 1) {
		return CmpRowFns[Op](LeftDimension, LeftData, RightDimension, RightData);
	}
	int Size = LeftDimension->Size;
	if (LeftDimension->Indices) {
		int *LeftIndices = LeftDimension->Indices;
		if (RightDimension->Indices) {
			int *RightIndices = RightDimension->Indices;
			for (int I = 0; I < Size; ++I) {
				int Result = cmp_array(Op, Degree - 1, LeftDimension + 1, LeftData + LeftIndices[I] * LeftDimension->Stride, RightDimension + 1, RightData + RightIndices[I] * RightDimension->Stride);
				if (Result) return Result;
			}
		} else {
			int RightStride = RightDimension->Stride;
			for (int I = 0; I < Size; ++I) {
				int Result = cmp_array(Op, Degree - 1, LeftDimension + 1, LeftData + LeftIndices[I] * LeftDimension->Stride, RightDimension + 1, RightData);
				if (Result) return Result;
				RightData += RightStride;
			}
		}
	} else {
		int LeftStride = LeftDimension->Stride;
		if (RightDimension->Indices) {
			int *RightIndices = RightDimension->Indices;
			for (int I = 0; I < Size; ++I) {
				int Result = cmp_array(Op, Degree - 1, LeftDimension + 1, LeftData, RightDimension + 1, RightData + RightIndices[I] * RightDimension->Stride);
				if (Result) return Result;
				LeftData += LeftStride;
			}
		} else {
			int RightStride = RightDimension->Stride;
			for (int I = Size; --I >= 0;) {
				int Result = cmp_array(Op, Degree - 1, LeftDimension + 1, LeftData, RightDimension + 1, RightData);
				if (Result) return Result;
				LeftData += LeftStride;
				RightData += RightStride;
			}
		}
	}
	return 0;
}

int ml_array_compare(ml_array_t *A, ml_array_t *B) {
	int Degree = A->Degree;
	if (Degree < B->Degree) return -1;
	if (Degree > B->Degree) return 1;
	if (Degree == -1) return 0;
	for (int I = 0; I < Degree; ++I) {
		if (A->Dimensions[I].Size < B->Dimensions[I].Size) return -1;
		if (A->Dimensions[I].Size > B->Dimensions[I].Size) return 1;
	}
	int Op = A->Format * MAX_FORMATS + B->Format;
	if (!CmpRowFns[Op]) return -1;
	return cmp_array(Op, A->Degree, A->Dimensions, A->Base.Value, B->Dimensions, B->Base.Value);
}

#include "../ml_array.h"

#define __CTYPE(X) X
#define _CTYPE(CTYPE, LEFT, RIGHT) __CTYPE(CTYPE ## _ ## LEFT ## _ ## RIGHT)

#define INFIX_ROW_IMPL(NAME, OP, CTYPE, METH, LEFT, RIGHT) \
\
void NAME ## _row_ ## LEFT ## _ ## RIGHT(_CTYPE(CTYPE, LEFT, RIGHT) *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData) { \
	int Size = LeftDimension->Size; \
	if (LeftDimension->Indices) { \
		const int *LeftIndices = LeftDimension->Indices; \
		if (RightDimension->Indices) { \
			const int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				LEFT Left = *(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride); \
				RIGHT Right = *(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride); \
				*(Target++) = OP(Left, Right); \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				LEFT Left = *(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride); \
				RIGHT Right = *(RIGHT *)RightData; \
				*(Target++) = OP(Left, Right); \
				RightData += RightStride; \
			} \
		} \
	} else { \
		int LeftStride = LeftDimension->Stride; \
		if (RightDimension->Indices) { \
			const int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				LEFT Left = *(LEFT *)LeftData; \
				RIGHT Right = *(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride); \
				*(Target++) = OP(Left, Right); \
				LeftData += LeftStride; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				LEFT Left = *(LEFT *)LeftData; \
				RIGHT Right = *(RIGHT *)RightData; \
				*(Target++) = OP(Left, Right); \
				LeftData += LeftStride; \
				RightData += RightStride; \
			} \
		} \
	} \
}

#define ml_number(X) _Generic(X, ml_value_t *: ml_nop, double: ml_real, default: ml_integer)(X)

#define INFIX_ROW_VALUE_IMPL(NAME, OP, CTYPE, METH, FN, RIGHT) \
\
void NAME ## _row_any_ ## RIGHT(_CTYPE(CTYPE, any, RIGHT) *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData) { \
	int Size = LeftDimension->Size; \
	if (LeftDimension->Indices) { \
		const int *LeftIndices = LeftDimension->Indices; \
		if (RightDimension->Indices) { \
			const int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Left = *(ml_value_t **)(LeftData + LeftIndices[I] * LeftDimension->Stride); \
				ml_value_t *Right = ml_number(*(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride)); \
				*(Target++) = FN(ml_simple_inline(METH, 2, Left, Right)); \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Left = *(ml_value_t **)(LeftData + LeftIndices[I] * LeftDimension->Stride); \
				ml_value_t *Right = ml_number(*(RIGHT *)RightData); \
				*(Target++) = FN(ml_simple_inline(METH, 2, Left, Right)); \
				RightData += RightStride; \
			} \
		} \
	} else { \
		int LeftStride = LeftDimension->Stride; \
		if (RightDimension->Indices) { \
			const int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Left = *(ml_value_t **)LeftData; \
				ml_value_t *Right = ml_number(*(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride)); \
				*(Target++) = FN(ml_simple_inline(METH, 2, Left, Right)); \
				LeftData += LeftStride; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				ml_value_t *Left = *(ml_value_t **)LeftData; \
				ml_value_t *Right = ml_number(*(RIGHT *)RightData); \
				*(Target++) = FN(ml_simple_inline(METH, 2, Left, Right)); \
				LeftData += LeftStride; \
				RightData += RightStride; \
			} \
		} \
	} \
}

#define ml_number_value(T, X) _Generic(T, double: ml_real_value, default: ml_integer_value)(X)

#define INFIX_ROW_IMPL_VALUE(NAME, OP, CTYPE, METH, FN, LEFT) \
\
void NAME ## _row_ ## LEFT ## _any(_CTYPE(CTYPE, LEFT, any) *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData) { \
	int Size = LeftDimension->Size; \
	if (LeftDimension->Indices) { \
		const int *LeftIndices = LeftDimension->Indices; \
		if (RightDimension->Indices) { \
			const int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Left = ml_number(*(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride)); \
				ml_value_t *Right = *(ml_value_t **)(RightData + RightIndices[I] * RightDimension->Stride); \
				*(Target++) = FN(ml_simple_inline(METH, 2, Left, Right)); \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Left = ml_number(*(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride)); \
				ml_value_t *Right = *(ml_value_t **)RightData; \
				*(Target++) = FN(ml_simple_inline(METH, 2, Left, Right)); \
				RightData += RightStride; \
			} \
		} \
	} else { \
		int LeftStride = LeftDimension->Stride; \
		if (RightDimension->Indices) { \
			const int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Left = ml_number(*(LEFT *)LeftData); \
				ml_value_t *Right = *(ml_value_t **)(RightData + RightIndices[I] * RightDimension->Stride); \
				*(Target++) = FN(ml_simple_inline(METH, 2, Left, Right)); \
				LeftData += LeftStride; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				ml_value_t *Left = ml_number(*(LEFT *)LeftData); \
				ml_value_t *Right = *(ml_value_t **)RightData; \
				*(Target++) = FN(ml_simple_inline(METH, 2, Left, Right)); \
				LeftData += LeftStride; \
				RightData += RightStride; \
			} \
		} \
	} \
}

#define INFIX_ROW_VALUE_IMPL_VALUE(NAME, OP, CTYPE, METH, FN) \
\
void NAME ## _row_any_any(_CTYPE(CTYPE, any, any) *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData) { \
	int Size = LeftDimension->Size; \
	if (LeftDimension->Indices) { \
		const int *LeftIndices = LeftDimension->Indices; \
		if (RightDimension->Indices) { \
			const int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Right = *(ml_value_t **)(RightData + RightIndices[I] * RightDimension->Stride); \
				ml_value_t *Left = *(ml_value_t **)(LeftData + LeftIndices[I] * LeftDimension->Stride); \
				*(Target++) = FN(ml_simple_inline(METH, 2, Left, Right)); \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Right = *(ml_value_t **)RightData; \
				ml_value_t *Left = *(ml_value_t **)(LeftData + LeftIndices[I] * LeftDimension->Stride); \
				*(Target++) = FN(ml_simple_inline(METH, 2, Left, Right)); \
				RightData += RightStride; \
			} \
		} \
	} else { \
		int LeftStride = LeftDimension->Stride; \
		if (RightDimension->Indices) { \
			const int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Right = *(ml_value_t **)(RightData + RightIndices[I] * RightDimension->Stride); \
				ml_value_t *Left = *(ml_value_t **)LeftData; \
				*(Target++) = FN(ml_simple_inline(METH, 2, Left, Right)); \
				LeftData += LeftStride; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				ml_value_t *Right = *(ml_value_t **)RightData; \
				ml_value_t *Left = *(ml_value_t **)LeftData; \
				*(Target++) = FN(ml_simple_inline(METH, 2, Left, Right)); \
				LeftData += LeftStride; \
				RightData += RightStride; \
			} \
		} \
	} \
}

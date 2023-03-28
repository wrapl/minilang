#include "../ml_array.h"

#define COMPARE_ROW_IMPL(NAME, OP, METH, LEFT, RIGHT) \
\
void NAME ## _row_ ## LEFT ## _ ## RIGHT(char *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData) { \
	int Size = LeftDimension->Size; \
	if (LeftDimension->Indices) { \
		int *LeftIndices = LeftDimension->Indices; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
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
			int *RightIndices = RightDimension->Indices; \
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

#define COMPARE_ROW_VALUE_IMPL(NAME, OP, METH, RIGHT) \
\
void NAME ## _row_any_ ## RIGHT(char *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData) { \
	int Size = LeftDimension->Size; \
	if (LeftDimension->Indices) { \
		int *LeftIndices = LeftDimension->Indices; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Left = *(ml_value_t **)(LeftData + LeftIndices[I] * LeftDimension->Stride); \
				ml_value_t *Right = ml_number(*(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride)); \
				*(Target++) = ml_simple_inline(METH, 2, Left, Right) != MLNil; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Left = *(ml_value_t **)(LeftData + LeftIndices[I] * LeftDimension->Stride); \
				ml_value_t *Right = ml_number(*(RIGHT *)RightData); \
				*(Target++) = ml_simple_inline(METH, 2, Left, Right) != MLNil; \
				RightData += RightStride; \
			} \
		} \
	} else { \
		int LeftStride = LeftDimension->Stride; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Left = *(ml_value_t **)LeftData; \
				ml_value_t *Right = ml_number(*(RIGHT *)(RightData + RightIndices[I] * RightDimension->Stride)); \
				*(Target++) = ml_simple_inline(METH, 2, Left, Right) != MLNil; \
				LeftData += LeftStride; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				ml_value_t *Left = *(ml_value_t **)LeftData; \
				ml_value_t *Right = ml_number(*(RIGHT *)RightData); \
				*(Target++) = ml_simple_inline(METH, 2, Left, Right) != MLNil; \
				LeftData += LeftStride; \
				RightData += RightStride; \
			} \
		} \
	} \
}

#define ml_number_value(T, X) _Generic(T, double: ml_real_value, default: ml_integer_value)(X)

#define COMPARE_ROW_IMPL_VALUE(NAME, OP, METH, LEFT) \
\
void NAME ## _row_ ## LEFT ## _any(char *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData) { \
	int Size = LeftDimension->Size; \
	if (LeftDimension->Indices) { \
		int *LeftIndices = LeftDimension->Indices; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Left = ml_number(*(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride)); \
				ml_value_t *Right = *(ml_value_t **)(RightData + RightIndices[I] * RightDimension->Stride); \
				*(Target++) = ml_simple_inline(METH, 2, Left, Right) != MLNil; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Left = ml_number(*(LEFT *)(LeftData + LeftIndices[I] * LeftDimension->Stride)); \
				ml_value_t *Right = *(ml_value_t **)RightData; \
				*(Target++) = ml_simple_inline(METH, 2, Left, Right) != MLNil; \
				RightData += RightStride; \
			} \
		} \
	} else { \
		int LeftStride = LeftDimension->Stride; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Left = ml_number(*(LEFT *)LeftData); \
				ml_value_t *Right = *(ml_value_t **)(RightData + RightIndices[I] * RightDimension->Stride); \
				*(Target++) = ml_simple_inline(METH, 2, Left, Right) != MLNil; \
				LeftData += LeftStride; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				ml_value_t *Left = ml_number(*(LEFT *)LeftData); \
				ml_value_t *Right = *(ml_value_t **)RightData; \
				*(Target++) = ml_simple_inline(METH, 2, Left, Right) != MLNil; \
				LeftData += LeftStride; \
				RightData += RightStride; \
			} \
		} \
	} \
}

#define COMPARE_ROW_VALUE_IMPL_VALUE(NAME, OP, METH) \
\
void NAME ## _row_any_any(char *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData) { \
	int Size = LeftDimension->Size; \
	if (LeftDimension->Indices) { \
		int *LeftIndices = LeftDimension->Indices; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Right = *(ml_value_t **)(RightData + RightIndices[I] * RightDimension->Stride); \
				ml_value_t *Left = *(ml_value_t **)(LeftData + LeftIndices[I] * LeftDimension->Stride); \
				*(Target++) = ml_simple_inline(METH, 2, Left, Right) != MLNil; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Right = *(ml_value_t **)RightData; \
				ml_value_t *Left = *(ml_value_t **)(LeftData + LeftIndices[I] * LeftDimension->Stride); \
				*(Target++) = ml_simple_inline(METH, 2, Left, Right) != MLNil; \
				RightData += RightStride; \
			} \
		} \
	} else { \
		int LeftStride = LeftDimension->Stride; \
		if (RightDimension->Indices) { \
			int *RightIndices = RightDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				ml_value_t *Right = *(ml_value_t **)(RightData + RightIndices[I] * RightDimension->Stride); \
				ml_value_t *Left = *(ml_value_t **)LeftData; \
				*(Target++) = ml_simple_inline(METH, 2, Left, Right) != MLNil; \
				LeftData += LeftStride; \
			} \
		} else { \
			int RightStride = RightDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				ml_value_t *Right = *(ml_value_t **)RightData; \
				ml_value_t *Left = *(ml_value_t **)LeftData; \
				*(Target++) = ml_simple_inline(METH, 2, Left, Right) != MLNil; \
				LeftData += LeftStride; \
				RightData += RightStride; \
			} \
		} \
	} \
}

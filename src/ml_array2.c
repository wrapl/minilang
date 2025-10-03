#include "ml_array.h"
#include "ml_macros.h"
#include "ml_math.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>
#include <inttypes.h>

#ifdef ML_COMPLEX
#include <complex.h>
#undef I

#ifndef __USE_GNU
complex double clog10(complex double Z);
#endif

#endif

#undef ML_CATEGORY
#define ML_CATEGORY "array"

#define MIN(X, Y) ((X < Y) ? X : Y)
#define MAX(X, Y) ((X > Y) ? X : Y)

extern ml_type_t MLArrayIntegerT[];
extern ml_type_t MLArrayRealT[];
extern ml_type_t MLVectorIntegerT[];
extern ml_type_t MLVectorRealT[];
extern ml_type_t MLMatrixIntegerT[];
extern ml_type_t MLMatrixRealT[];

#ifdef ML_COMPLEX

extern ml_type_t MLArrayComplexT[];
extern ml_type_t MLVectorComplexT[];
extern ml_type_t MLMatrixComplexT[];

#endif

extern void *array_alloc(ml_array_format_t Format, size_t Size);

#define PARTIAL_FUNCTIONS(CTYPE) \
\
static void partial_sums_ ## CTYPE(int Target, int Degree, ml_array_dimension_t *Dimension, char *Address, int LastRow) { \
	if (Degree == 0) { \
		*(CTYPE *)Address += *(CTYPE *)(Address - LastRow); \
	} else if (Target == Degree) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
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
			const int *Indices = Dimension->Indices; \
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
static void partial_prods_ ## CTYPE(int Target, int Degree, ml_array_dimension_t *Dimension, char *Address, int LastRow) { \
	if (Degree == 0) { \
		*(CTYPE *)Address *= *(CTYPE *)(Address - LastRow); \
	} else if (Target == Degree) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 1; I < Dimension->Size; ++I) { \
				partial_prods_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address + Indices[I] * Stride, (Indices[I] - Indices[I - 1]) * Stride); \
			} \
		} else { \
			for (int I = 1; I < Dimension->Size; ++I) { \
				Address += Stride; \
				partial_prods_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address, Stride); \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				partial_prods_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address + Indices[I] * Stride, LastRow); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				partial_prods_ ## CTYPE(Target, Degree - 1, Dimension + 1, Address, LastRow); \
				Address += Stride; \
			} \
		} \
	} \
} \

#define COMPLETE_FUNCTIONS(CTYPE1, CTYPE2) \
\
static CTYPE1 compute_sums_ ## CTYPE2(int Degree, ml_array_dimension_t *Dimension, void *Address) { \
	CTYPE1 Sum = 0; \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += compute_sums_ ## CTYPE2(Degree - 1, Dimension + 1, Address + Indices[I] * Stride); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += compute_sums_ ## CTYPE2(Degree - 1, Dimension + 1, Address); \
				Address += Stride; \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += *(CTYPE2 *)(Address + Indices[I] * Stride); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += *(CTYPE2 *)Address; \
				Address += Stride; \
			} \
		} \
	} \
	return Sum; \
} \
\
static void fill_sums_ ## CTYPE2(int TargetDegree, ml_array_dimension_t *TargetDimension, void *TargetAddress, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceAddress) { \
	if (TargetDegree == 0) { \
		*(CTYPE1 *)TargetAddress = compute_sums_ ## CTYPE2(SourceDegree, SourceDimension, SourceAddress); \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		int SourceStride = SourceDimension->Stride; \
		if (SourceDimension->Indices) { \
			const int *Indices = SourceDimension->Indices; \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_sums_ ## CTYPE2(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress + Indices[I] * SourceStride); \
				TargetAddress += TargetStride; \
			} \
		} else { \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_sums_ ## CTYPE2(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress); \
				TargetAddress += TargetStride; \
				SourceAddress += SourceStride; \
			} \
		} \
	} \
} \
\
static CTYPE1 compute_prods_ ## CTYPE2(int Degree, ml_array_dimension_t *Dimension, void *Address) { \
	CTYPE1 Prod = 1; \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Prod *= compute_prods_ ## CTYPE2(Degree - 1, Dimension + 1, Address + Indices[I] * Stride); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Prod *= compute_prods_ ## CTYPE2(Degree - 1, Dimension + 1, Address); \
				Address += Stride; \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Prod *= *(CTYPE2 *)(Address + Indices[I] * Stride); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Prod *= *(CTYPE2 *)Address; \
				Address += Stride; \
			} \
		} \
	} \
	return Prod; \
} \
\
static void fill_prods_ ## CTYPE2(int TargetDegree, ml_array_dimension_t *TargetDimension, void *TargetAddress, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceAddress) { \
	if (TargetDegree == 0) { \
		*(CTYPE1 *)TargetAddress = compute_prods_ ## CTYPE2(SourceDegree, SourceDimension, SourceAddress); \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		int SourceStride = SourceDimension->Stride; \
		if (SourceDimension->Indices) { \
			const int *Indices = SourceDimension->Indices; \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_prods_ ## CTYPE2(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress + Indices[I] * SourceStride); \
				TargetAddress += TargetStride; \
			} \
		} else { \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_prods_ ## CTYPE2(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress); \
				TargetAddress += TargetStride; \
				SourceAddress += SourceStride; \
			} \
		} \
	} \
}

#define MINMAX_FUNCTIONS(CTYPE) \
\
static CTYPE compute_mins_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, void *Address) { \
	CTYPE Min = *(CTYPE *)Address; \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Min2 = compute_mins_ ## CTYPE(Degree - 1, Dimension + 1, Address + Indices[I] * Stride); \
				if (Min2 < Min) Min = Min2; \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Min2 = compute_mins_ ## CTYPE(Degree - 1, Dimension + 1, Address); \
				if (Min2 < Min) Min = Min2; \
				Address += Stride; \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Min2 = *(CTYPE *)(Address + Indices[I] * Stride); \
				if (Min2 < Min) Min = Min2; \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Min2 = *(CTYPE *)Address; \
				if (Min2 < Min) Min = Min2; \
				Address += Stride; \
			} \
		} \
	} \
	return Min; \
} \
\
static void fill_mins_ ## CTYPE(int TargetDegree, ml_array_dimension_t *TargetDimension, void *TargetAddress, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceAddress) { \
	if (TargetDegree == 0) { \
		*(CTYPE *)TargetAddress = compute_mins_ ## CTYPE(SourceDegree, SourceDimension, SourceAddress); \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		int SourceStride = SourceDimension->Stride; \
		if (SourceDimension->Indices) { \
			const int *Indices = SourceDimension->Indices; \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_mins_ ## CTYPE(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress + Indices[I] * SourceStride); \
				TargetAddress += TargetStride; \
			} \
		} else { \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_mins_ ## CTYPE(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress); \
				TargetAddress += TargetStride; \
				SourceAddress += SourceStride; \
			} \
		} \
	} \
} \
\
static CTYPE find_mins_ ## CTYPE(uint32_t *Target, int Degree, ml_array_dimension_t *Dimension, void *Address, CTYPE Min) { \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Min2 = find_mins_ ## CTYPE(Target + 1, Degree - 1, Dimension + 1, Address + Indices[I] * Stride, Min); \
				if (Min2 < Min) { Min = Min2; *Target = I + 1; } \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Min2 = find_mins_ ## CTYPE(Target + 1, Degree - 1, Dimension + 1, Address, Min); \
				if (Min2 < Min) { Min = Min2; *Target = I + 1; } \
				Address += Stride; \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Min2 = *(CTYPE *)(Address + Indices[I] * Stride); \
				if (Min2 < Min) { Min = Min2; *Target = I + 1; } \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Min2 = *(CTYPE *)Address; \
				if (Min2 < Min) { Min = Min2; *Target = I + 1; } \
				Address += Stride; \
			} \
		} \
	} \
	return Min; \
} \
\
static void index_mins_ ## CTYPE(int TargetDegree, ml_array_dimension_t *TargetDimension, void *TargetAddress, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceAddress) { \
	if (TargetDegree == 1) { \
		*(uint32_t *)TargetAddress = 1; \
		find_mins_ ## CTYPE((uint32_t *)TargetAddress, SourceDegree, SourceDimension, SourceAddress, *(CTYPE *)SourceAddress); \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		int SourceStride = SourceDimension->Stride; \
		if (SourceDimension->Indices) { \
			const int *Indices = SourceDimension->Indices; \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				index_mins_ ## CTYPE(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress + Indices[I] * SourceStride); \
				TargetAddress += TargetStride; \
			} \
		} else { \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				index_mins_ ## CTYPE(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress); \
				TargetAddress += TargetStride; \
				SourceAddress += SourceStride; \
			} \
		} \
	} \
} \
\
static CTYPE compute_maxs_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, void *Address) { \
	CTYPE Max = *(CTYPE *)Address; \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Max2 = compute_maxs_ ## CTYPE(Degree - 1, Dimension + 1, Address + Indices[I] * Stride); \
				if (Max2 > Max) Max = Max2; \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Max2 = compute_maxs_ ## CTYPE(Degree - 1, Dimension + 1, Address); \
				if (Max2 > Max) Max = Max2; \
				Address += Stride; \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Max2 = *(CTYPE *)(Address + Indices[I] * Stride); \
				if (Max2 > Max) Max = Max2; \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Max2 = *(CTYPE *)Address; \
				if (Max2 > Max) Max = Max2; \
				Address += Stride; \
			} \
		} \
	} \
	return Max; \
} \
\
static void fill_maxs_ ## CTYPE(int TargetDegree, ml_array_dimension_t *TargetDimension, void *TargetAddress, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceAddress) { \
	if (TargetDegree == 0) { \
		*(CTYPE *)TargetAddress = compute_maxs_ ## CTYPE(SourceDegree, SourceDimension, SourceAddress); \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		int SourceStride = SourceDimension->Stride; \
		if (SourceDimension->Indices) { \
			const int *Indices = SourceDimension->Indices; \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_maxs_ ## CTYPE(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress + Indices[I] * SourceStride); \
				TargetAddress += TargetStride; \
			} \
		} else { \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_maxs_ ## CTYPE(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress); \
				TargetAddress += TargetStride; \
				SourceAddress += SourceStride; \
			} \
		} \
	} \
} \
\
static CTYPE find_maxs_ ## CTYPE(uint32_t *Target, int Degree, ml_array_dimension_t *Dimension, void *Address, CTYPE Max) { \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Max2 = find_maxs_ ## CTYPE(Target + 1, Degree - 1, Dimension + 1, Address + Indices[I] * Stride, Max); \
				if (Max2 > Max) { Max = Max2; *Target = I + 1; } \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Max2 = find_maxs_ ## CTYPE(Target + 1, Degree - 1, Dimension + 1, Address, Max); \
				if (Max2 > Max) { Max = Max2; *Target = I + 1; } \
				Address += Stride; \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Max2 = *(CTYPE *)(Address + Indices[I] * Stride); \
				if (Max2 > Max) { Max = Max2; *Target = I + 1; } \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				CTYPE Max2 = *(CTYPE *)Address; \
				if (Max2 > Max) { Max = Max2; *Target = I + 1; } \
				Address += Stride; \
			} \
		} \
	} \
	return Max; \
} \
\
static void index_maxs_ ## CTYPE(int TargetDegree, ml_array_dimension_t *TargetDimension, void *TargetAddress, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceAddress) { \
	if (TargetDegree == 1) { \
		*(uint32_t *)TargetAddress = 1; \
		find_maxs_ ## CTYPE((uint32_t *)TargetAddress, SourceDegree, SourceDimension, SourceAddress, *(CTYPE *)SourceAddress); \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		int SourceStride = SourceDimension->Stride; \
		if (SourceDimension->Indices) { \
			const int *Indices = SourceDimension->Indices; \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				index_maxs_ ## CTYPE(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress + Indices[I] * SourceStride); \
				TargetAddress += TargetStride; \
			} \
		} else { \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				index_maxs_ ## CTYPE(TargetDegree - 1, TargetDimension + 1, TargetAddress, SourceDegree - 1, SourceDimension + 1, SourceAddress); \
				TargetAddress += TargetStride; \
				SourceAddress += SourceStride; \
			} \
		} \
	} \
}

PARTIAL_FUNCTIONS(uint64_t);
PARTIAL_FUNCTIONS(int64_t);
PARTIAL_FUNCTIONS(double);

COMPLETE_FUNCTIONS(uint64_t, uint8_t);
COMPLETE_FUNCTIONS(int64_t, int8_t);
COMPLETE_FUNCTIONS(uint64_t, uint16_t);
COMPLETE_FUNCTIONS(int64_t, int16_t);
COMPLETE_FUNCTIONS(uint64_t, uint32_t);
COMPLETE_FUNCTIONS(int64_t, int32_t);
COMPLETE_FUNCTIONS(uint64_t, uint64_t);
COMPLETE_FUNCTIONS(int64_t, int64_t);
COMPLETE_FUNCTIONS(double, float);
COMPLETE_FUNCTIONS(double, double);

MINMAX_FUNCTIONS(uint8_t);
MINMAX_FUNCTIONS(int8_t);
MINMAX_FUNCTIONS(uint16_t);
MINMAX_FUNCTIONS(int16_t);
MINMAX_FUNCTIONS(uint32_t);
MINMAX_FUNCTIONS(int32_t);
MINMAX_FUNCTIONS(uint64_t);
MINMAX_FUNCTIONS(int64_t);
MINMAX_FUNCTIONS(float);
MINMAX_FUNCTIONS(double);

#ifdef ML_COMPLEX

PARTIAL_FUNCTIONS(complex_double);

COMPLETE_FUNCTIONS(complex_double, complex_float);
COMPLETE_FUNCTIONS(complex_double, complex_double);

#endif

ML_METHOD("sums", MLArrayT, MLIntegerT) {
//<Array
//<Index
//>array
// Returns a new array with the partial sums of :mini:`Array` in the :mini:`Index`-th dimension.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index <= 0) Index += Source->Degree + 1;
	if (Index < 1 || Index > Source->Degree) return ml_error("ArrayError", "Dimension index invalid");
	Index = Source->Degree + 1 - Index;
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
	case ML_ARRAY_FORMAT_U16:
	case ML_ARRAY_FORMAT_U32:
	case ML_ARRAY_FORMAT_U64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_U64, Source->Degree);
		ml_array_copy(Target, Source);
		partial_sums_uint64_t(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
	case ML_ARRAY_FORMAT_I8:
	case ML_ARRAY_FORMAT_I16:
	case ML_ARRAY_FORMAT_I32:
	case ML_ARRAY_FORMAT_I64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_I64, Source->Degree);
		ml_array_copy(Target, Source);
		partial_sums_int64_t(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
	case ML_ARRAY_FORMAT_F32:
	case ML_ARRAY_FORMAT_F64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_F64, Source->Degree);
		ml_array_copy(Target, Source);
		partial_sums_double(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
	case ML_ARRAY_FORMAT_C64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_C64, Source->Degree);
		ml_array_copy(Target, Source);
		partial_sums_complex_double(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
#endif

	default:
		return ml_error("ArrayError", "Invalid array format");
	}
}

ML_METHOD("prods", MLArrayT, MLIntegerT) {
//<Array
//<Index
//>array
// Returns a new array with the partial products of :mini:`Array` in the :mini:`Index`-th dimension.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index <= 0) Index += Source->Degree + 1;
	if (Index < 1 || Index > Source->Degree) return ml_error("ArrayError", "Dimension index invalid");
	Index = Source->Degree + 1 - Index;
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
	case ML_ARRAY_FORMAT_U16:
	case ML_ARRAY_FORMAT_U32:
	case ML_ARRAY_FORMAT_U64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_U64, Source->Degree);
		ml_array_copy(Target, Source);
		partial_prods_uint64_t(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
	case ML_ARRAY_FORMAT_I8:
	case ML_ARRAY_FORMAT_I16:
	case ML_ARRAY_FORMAT_I32:
	case ML_ARRAY_FORMAT_I64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_I64, Source->Degree);
		ml_array_copy(Target, Source);
		partial_prods_int64_t(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
	case ML_ARRAY_FORMAT_F32:
	case ML_ARRAY_FORMAT_F64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_F64, Source->Degree);
		ml_array_copy(Target, Source);
		partial_prods_double(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
	case ML_ARRAY_FORMAT_C64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_C64, Source->Degree);
		ml_array_copy(Target, Source);
		partial_prods_complex_double(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
#endif
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
}

ML_METHOD("sum", MLArrayT) {
//<Array
//>number
// Returns the sum of the values in :mini:`Array`.
//$= let A := array([[1, 2, 3], [4, 5, 6]])
//$= A:sum
	ml_array_t *Source = (ml_array_t *)Args[0];
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		return ml_integer(compute_sums_uint8_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I8:
		return ml_integer(compute_sums_int8_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_U16:
		return ml_integer(compute_sums_uint16_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I16:
		return ml_integer(compute_sums_int16_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_U32:
		return ml_integer(compute_sums_uint32_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I32:
		return ml_integer(compute_sums_int32_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_U64:
		return ml_integer(compute_sums_uint64_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I64:
		return ml_integer(compute_sums_int64_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_F32:
		return ml_real(compute_sums_float(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_F64:
		return ml_real(compute_sums_double(Source->Degree, Source->Dimensions, Source->Base.Value));
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
		return ml_complex(compute_sums_complex_float(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_C64:
		return ml_complex(compute_sums_complex_double(Source->Degree, Source->Dimensions, Source->Base.Value));
#endif
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
}

ML_METHOD("sum", MLArrayT, MLIntegerT) {
//<Array
//<Index
//>array
// Returns a new array with the sums of :mini:`Array` in the last :mini:`Count` dimensions.
//$= let A := array([[1, 2, 3], [4, 5, 6]])
//$= A:sum(1)
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Slice = ml_integer_value(Args[1]);
	if (Slice <= 0 || Slice >= Source->Degree) return ml_error("IntervalError", "Invalid axes count for sum");
	ml_array_format_t Format;
	void (*fill_sums)(int, ml_array_dimension_t *, void *, int, ml_array_dimension_t *, void *);
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		Format = ML_ARRAY_FORMAT_U64;
		fill_sums = fill_sums_uint8_t;
		break;
	case ML_ARRAY_FORMAT_I8:
		Format = ML_ARRAY_FORMAT_I64;
		fill_sums = fill_sums_int8_t;
		break;
	case ML_ARRAY_FORMAT_U16:
		Format = ML_ARRAY_FORMAT_U64;
		fill_sums = fill_sums_uint16_t;
		break;
	case ML_ARRAY_FORMAT_I16:
		Format = ML_ARRAY_FORMAT_I64;
		fill_sums = fill_sums_int16_t;
		break;
	case ML_ARRAY_FORMAT_U32:
		Format = ML_ARRAY_FORMAT_U64;
		fill_sums = fill_sums_uint32_t;
		break;
	case ML_ARRAY_FORMAT_I32:
		Format = ML_ARRAY_FORMAT_I64;
		fill_sums = fill_sums_int32_t;
		break;
	case ML_ARRAY_FORMAT_U64:
		Format = ML_ARRAY_FORMAT_U64;
		fill_sums = fill_sums_uint64_t;
		break;
	case ML_ARRAY_FORMAT_I64:
		Format = ML_ARRAY_FORMAT_I64;
		fill_sums = fill_sums_int64_t;
		break;
	case ML_ARRAY_FORMAT_F32:
		Format = ML_ARRAY_FORMAT_F64;
		fill_sums = fill_sums_float;
		break;
	case ML_ARRAY_FORMAT_F64:
		Format = ML_ARRAY_FORMAT_F64;
		fill_sums = fill_sums_double;
		break;
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
		Format = ML_ARRAY_FORMAT_C64;
		fill_sums = fill_sums_complex_float;
		break;
	case ML_ARRAY_FORMAT_C64:
		Format = ML_ARRAY_FORMAT_C64;
		fill_sums = fill_sums_complex_double;
		break;
#endif
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
	ml_array_t *Target = ml_array_alloc(Format, Source->Degree - Slice);
	int DataSize = MLArraySizes[Target->Format];
	for (int I = Target->Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Value = array_alloc(Format, DataSize);
	fill_sums(Target->Degree, Target->Dimensions, Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value);
	return (ml_value_t *)Target;
}

ML_METHOD("prod", MLArrayT) {
//<Array
//>number
// Returns the product of the values in :mini:`Array`.
//$= let A := array([[1, 2, 3], [4, 5, 6]])
//$= A:prod
	ml_array_t *Source = (ml_array_t *)Args[0];
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		return ml_integer(compute_prods_uint8_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I8:
		return ml_integer(compute_prods_int8_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_U16:
		return ml_integer(compute_prods_uint16_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I16:
		return ml_integer(compute_prods_int16_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_U32:
		return ml_integer(compute_prods_uint32_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I32:
		return ml_integer(compute_prods_int32_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_U64:
		return ml_integer(compute_prods_uint64_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I64:
		return ml_integer(compute_prods_int64_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_F32:
		return ml_real(compute_prods_float(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_F64:
		return ml_real(compute_prods_double(Source->Degree, Source->Dimensions, Source->Base.Value));
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
		return ml_complex(compute_prods_complex_float(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_C64:
		return ml_complex(compute_prods_complex_double(Source->Degree, Source->Dimensions, Source->Base.Value));
#endif
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
}

ML_METHOD("prod", MLArrayT, MLIntegerT) {
//<Array
//<Count
//>array
// Returns a new array with the products of :mini:`Array` in the last :mini:`Count` dimensions.
//$= let A := array([[1, 2, 3], [4, 5, 6]])
//$= A:prod(1)
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Slice = ml_integer_value(Args[1]);
	if (Slice <= 0 || Slice >= Source->Degree) return ml_error("IntervalError", "Invalid axes count for prod");
	ml_array_format_t Format;
	void (*fill_prods)(int, ml_array_dimension_t *, void *, int, ml_array_dimension_t *, void *);
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		Format = ML_ARRAY_FORMAT_U64;
		fill_prods = fill_prods_uint8_t;
		break;
	case ML_ARRAY_FORMAT_I8:
		Format = ML_ARRAY_FORMAT_I64;
		fill_prods = fill_prods_int8_t;
		break;
	case ML_ARRAY_FORMAT_U16:
		Format = ML_ARRAY_FORMAT_U64;
		fill_prods = fill_prods_uint16_t;
		break;
	case ML_ARRAY_FORMAT_I16:
		Format = ML_ARRAY_FORMAT_I64;
		fill_prods = fill_prods_int16_t;
		break;
	case ML_ARRAY_FORMAT_U32:
		Format = ML_ARRAY_FORMAT_U64;
		fill_prods = fill_prods_uint32_t;
		break;
	case ML_ARRAY_FORMAT_I32:
		Format = ML_ARRAY_FORMAT_I64;
		fill_prods = fill_prods_int32_t;
		break;
	case ML_ARRAY_FORMAT_U64:
		Format = ML_ARRAY_FORMAT_U64;
		fill_prods = fill_prods_uint64_t;
		break;
	case ML_ARRAY_FORMAT_I64:
		Format = ML_ARRAY_FORMAT_I64;
		fill_prods = fill_prods_int64_t;
		break;
	case ML_ARRAY_FORMAT_F32:
		Format = ML_ARRAY_FORMAT_F64;
		fill_prods = fill_prods_float;
		break;
	case ML_ARRAY_FORMAT_F64:
		Format = ML_ARRAY_FORMAT_F64;
		fill_prods = fill_prods_double;
		break;
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
		Format = ML_ARRAY_FORMAT_C64;
		fill_prods = fill_prods_complex_float;
		break;
	case ML_ARRAY_FORMAT_C64:
		Format = ML_ARRAY_FORMAT_C64;
		fill_prods = fill_prods_complex_double;
		break;
#endif
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
	ml_array_t *Target = ml_array_alloc(Format, Source->Degree - Slice);
	int DataSize = MLArraySizes[Target->Format];
	for (int I = Target->Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Value = array_alloc(Format, DataSize);
	fill_prods(Target->Degree, Target->Dimensions, Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value);
	return (ml_value_t *)Target;
}

ML_METHOD("minval", MLArrayT) {
//<Array
//>number
// Returns the minimum of the values in :mini:`Array`.
//$- let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
//$= A:minval
	ml_array_t *Source = (ml_array_t *)Args[0];
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		return ml_integer(compute_mins_uint8_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I8:
		return ml_integer(compute_mins_int8_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_U16:
		return ml_integer(compute_mins_uint16_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I16:
		return ml_integer(compute_mins_int16_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_U32:
		return ml_integer(compute_mins_uint32_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I32:
		return ml_integer(compute_mins_int32_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_U64:
		return ml_integer(compute_mins_uint64_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I64:
		return ml_integer(compute_mins_int64_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_F32:
		return ml_real(compute_mins_float(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_F64:
		return ml_real(compute_mins_double(Source->Degree, Source->Dimensions, Source->Base.Value));
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
}

ML_METHOD("minval", MLArrayT, MLIntegerT) {
//<Array
//<Count
//>array
// Returns a new array with the minimums :mini:`Array` in the last :mini:`Count` dimensions.
//$- let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
//$= A:minval(1)
//$= A:minval(2)
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Slice = ml_integer_value(Args[1]);
	if (Slice <= 0 || Slice >= Source->Degree) return ml_error("IntervalError", "Invalid axes count for min");
	void (*fill_mins)(int, ml_array_dimension_t *, void *, int, ml_array_dimension_t *, void *);
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		fill_mins = fill_mins_uint8_t;
		break;
	case ML_ARRAY_FORMAT_I8:
		fill_mins = fill_mins_int8_t;
		break;
	case ML_ARRAY_FORMAT_U16:
		fill_mins = fill_mins_uint16_t;
		break;
	case ML_ARRAY_FORMAT_I16:
		fill_mins = fill_mins_int16_t;
		break;
	case ML_ARRAY_FORMAT_U32:
		fill_mins = fill_mins_uint32_t;
		break;
	case ML_ARRAY_FORMAT_I32:
		fill_mins = fill_mins_int32_t;
		break;
	case ML_ARRAY_FORMAT_U64:
		fill_mins = fill_mins_uint64_t;
		break;
	case ML_ARRAY_FORMAT_I64:
		fill_mins = fill_mins_int64_t;
		break;
	case ML_ARRAY_FORMAT_F32:
		fill_mins = fill_mins_float;
		break;
	case ML_ARRAY_FORMAT_F64:
		fill_mins = fill_mins_double;
		break;
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
	ml_array_t *Target = ml_array_alloc(Source->Format, Source->Degree - Slice);
	int DataSize = MLArraySizes[Target->Format];
	for (int I = Target->Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Value = array_alloc(Source->Format, DataSize);
	fill_mins(Target->Degree, Target->Dimensions, Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value);
	return (ml_value_t *)Target;
}

ML_METHOD("minidx", MLArrayT) {
//<Array
//>array
// Returns a new array with the indices of minimums of :mini:`Array` in the last :mini:`Count` dimensions.
//$- let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
//$= A:minidx
	ml_array_t *Source = (ml_array_t *)Args[0];
	ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_U32, 1);
	int DataSize = MLArraySizes[Target->Format];
	Target->Dimensions[Target->Degree - 1].Stride = DataSize;
	Target->Dimensions[Target->Degree - 1].Size = Source->Degree;
	uint32_t *Indices = (uint32_t *)(Target->Base.Value = snew(DataSize));
	for (int I = 0; I < Source->Degree; ++I) Indices[I] = 1;
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		find_mins_uint8_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(uint8_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_I8:
		find_mins_int8_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(int8_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_U16:
		find_mins_uint16_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(uint16_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_I16:
		find_mins_int16_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(int16_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_U32:
		find_mins_uint32_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(uint32_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_I32:
		find_mins_int32_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(int32_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_U64:
		find_mins_uint64_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(uint64_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_I64:
		find_mins_int64_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(int64_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_F32:
		find_mins_float(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(float *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_F64:
		find_mins_double(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(double *)Source->Base.Value);
		break;
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
	return (ml_value_t *)Target;
}

ML_METHOD("minidx", MLArrayT, MLIntegerT) {
//<Array
//<Count
//>array
// Returns a new array with the indices of minimums of :mini:`Array` in the last :mini:`Count` dimensions.
//$- let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
//$= A:minidx(1)
//$= A:minidx(2)
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Slice = ml_integer_value(Args[1]);
	if (Slice <= 0 || Slice >= Source->Degree) return ml_error("IntervalError", "Invalid axes count for min");
	void (*index_mins)(int, ml_array_dimension_t *, void *, int, ml_array_dimension_t *, void *);
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		index_mins = index_mins_uint8_t;
		break;
	case ML_ARRAY_FORMAT_I8:
		index_mins = index_mins_int8_t;
		break;
	case ML_ARRAY_FORMAT_U16:
		index_mins = index_mins_uint16_t;
		break;
	case ML_ARRAY_FORMAT_I16:
		index_mins = index_mins_int16_t;
		break;
	case ML_ARRAY_FORMAT_U32:
		index_mins = index_mins_uint32_t;
		break;
	case ML_ARRAY_FORMAT_I32:
		index_mins = index_mins_int32_t;
		break;
	case ML_ARRAY_FORMAT_U64:
		index_mins = index_mins_uint64_t;
		break;
	case ML_ARRAY_FORMAT_I64:
		index_mins = index_mins_int64_t;
		break;
	case ML_ARRAY_FORMAT_F32:
		index_mins = index_mins_float;
		break;
	case ML_ARRAY_FORMAT_F64:
		index_mins = index_mins_double;
		break;
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
	ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_U32, (Source->Degree - Slice) + 1);
	int DataSize = MLArraySizes[Target->Format];
	Target->Dimensions[Target->Degree - 1].Stride = DataSize;
	Target->Dimensions[Target->Degree - 1].Size = Slice;
	DataSize *= Slice;
	for (int I = Target->Degree - 1; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Value = snew(DataSize);
	index_mins(Target->Degree, Target->Dimensions, Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value);
	return (ml_value_t *)Target;
}

ML_METHOD("maxval", MLArrayT) {
//<Array
//>number
// Returns the maximum of the values in :mini:`Array`.
//$- let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
//$= A:maxval
	ml_array_t *Source = (ml_array_t *)Args[0];
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		return ml_integer(compute_maxs_uint8_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I8:
		return ml_integer(compute_maxs_int8_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_U16:
		return ml_integer(compute_maxs_uint16_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I16:
		return ml_integer(compute_maxs_int16_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_U32:
		return ml_integer(compute_maxs_uint32_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I32:
		return ml_integer(compute_maxs_int32_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_U64:
		return ml_integer(compute_maxs_uint64_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_I64:
		return ml_integer(compute_maxs_int64_t(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_F32:
		return ml_real(compute_maxs_float(Source->Degree, Source->Dimensions, Source->Base.Value));
	case ML_ARRAY_FORMAT_F64:
		return ml_real(compute_maxs_double(Source->Degree, Source->Dimensions, Source->Base.Value));
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
}

ML_METHOD("maxval", MLArrayT, MLIntegerT) {
//<Array
//<Count
//>array
// Returns a new array with the maximums of :mini:`Array` in the last :mini:`Count` dimensions.
//$- let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
//$= A:maxval(1)
//$= A:maxval(2)
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Slice = ml_integer_value(Args[1]);
	if (Slice <= 0 || Slice >= Source->Degree) return ml_error("IntervalError", "Invalid axes count for max");
	void (*fill_maxs)(int, ml_array_dimension_t *, void *, int, ml_array_dimension_t *, void *);
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		fill_maxs = fill_maxs_uint8_t;
		break;
	case ML_ARRAY_FORMAT_I8:
		fill_maxs = fill_maxs_int8_t;
		break;
	case ML_ARRAY_FORMAT_U16:
		fill_maxs = fill_maxs_uint16_t;
		break;
	case ML_ARRAY_FORMAT_I16:
		fill_maxs = fill_maxs_int16_t;
		break;
	case ML_ARRAY_FORMAT_U32:
		fill_maxs = fill_maxs_uint32_t;
		break;
	case ML_ARRAY_FORMAT_I32:
		fill_maxs = fill_maxs_int32_t;
		break;
	case ML_ARRAY_FORMAT_U64:
		fill_maxs = fill_maxs_uint64_t;
		break;
	case ML_ARRAY_FORMAT_I64:
		fill_maxs = fill_maxs_int64_t;
		break;
	case ML_ARRAY_FORMAT_F32:
		fill_maxs = fill_maxs_float;
		break;
	case ML_ARRAY_FORMAT_F64:
		fill_maxs = fill_maxs_double;
		break;
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
	ml_array_t *Target = ml_array_alloc(Source->Format, Source->Degree - Slice);
	int DataSize = MLArraySizes[Target->Format];
	for (int I = Target->Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Value = array_alloc(Source->Format, DataSize);
	fill_maxs(Target->Degree, Target->Dimensions, Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value);
	return (ml_value_t *)Target;
}

ML_METHOD("maxidx", MLArrayT) {
//<Array
//>array
// Returns a new array with the indices of maximums of :mini:`Array` in the last :mini:`Count` dimensions.
//$- let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
//$= A:maxidx
	ml_array_t *Source = (ml_array_t *)Args[0];
	ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_I32, 1);
	int DataSize = MLArraySizes[Target->Format];
	Target->Dimensions[Target->Degree - 1].Stride = DataSize;
	Target->Dimensions[Target->Degree - 1].Size = Source->Degree;
	uint32_t *Indices = (uint32_t *)(Target->Base.Value = snew(DataSize));
	for (int I = 0; I < Source->Degree; ++I) Indices[I] = 1;
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		find_maxs_uint8_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(uint8_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_I8:
		find_maxs_int8_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(int8_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_U16:
		find_maxs_uint16_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(uint16_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_I16:
		find_maxs_int16_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(int16_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_U32:
		find_maxs_uint32_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(uint32_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_I32:
		find_maxs_int32_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(int32_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_U64:
		find_maxs_uint64_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(uint64_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_I64:
		find_maxs_int64_t(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(int64_t *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_F32:
		find_maxs_float(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(float *)Source->Base.Value);
		break;
	case ML_ARRAY_FORMAT_F64:
		find_maxs_double(Indices, Source->Degree, Source->Dimensions, Source->Base.Value, *(double *)Source->Base.Value);
		break;
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
	return (ml_value_t *)Target;
}

ML_METHOD("maxidx", MLArrayT, MLIntegerT) {
//<Array
//<Count
//>array
// Returns a new array with the indices of maximums of :mini:`Array` in the last :mini:`Count` dimensions.
//$- let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
//$= A:maxidx(1)
//$= A:maxidx(2)
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Slice = ml_integer_value(Args[1]);
	if (Slice <= 0 || Slice >= Source->Degree) return ml_error("IntervalError", "Invalid axes count for max");
	void (*index_maxs)(int, ml_array_dimension_t *, void *, int, ml_array_dimension_t *, void *);
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		index_maxs = index_maxs_uint8_t;
		break;
	case ML_ARRAY_FORMAT_I8:
		index_maxs = index_maxs_int8_t;
		break;
	case ML_ARRAY_FORMAT_U16:
		index_maxs = index_maxs_uint16_t;
		break;
	case ML_ARRAY_FORMAT_I16:
		index_maxs = index_maxs_int16_t;
		break;
	case ML_ARRAY_FORMAT_U32:
		index_maxs = index_maxs_uint32_t;
		break;
	case ML_ARRAY_FORMAT_I32:
		index_maxs = index_maxs_int32_t;
		break;
	case ML_ARRAY_FORMAT_U64:
		index_maxs = index_maxs_uint64_t;
		break;
	case ML_ARRAY_FORMAT_I64:
		index_maxs = index_maxs_int64_t;
		break;
	case ML_ARRAY_FORMAT_F32:
		index_maxs = index_maxs_float;
		break;
	case ML_ARRAY_FORMAT_F64:
		index_maxs = index_maxs_double;
		break;
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
	ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_I32, (Source->Degree - Slice) + 1);
	int DataSize = MLArraySizes[Target->Format];
	Target->Dimensions[Target->Degree - 1].Stride = DataSize;
	Target->Dimensions[Target->Degree - 1].Size = Slice;
	DataSize *= Slice;
	for (int I = Target->Degree - 1; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Value = snew(DataSize);
	index_maxs(Target->Degree, Target->Dimensions, Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value);
	return (ml_value_t *)Target;
}

#define NORM_FUNCTION(CTYPE1, CTYPE2, NORM) \
\
static double compute_norm_ ## CTYPE2(int Degree, ml_array_dimension_t *Dimension, void *Address, double P) { \
	double Sum = 0; \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += compute_norm_ ## CTYPE2(Degree - 1, Dimension + 1, Address + Indices[I] * Stride, P); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += compute_norm_ ## CTYPE2(Degree - 1, Dimension + 1, Address, P); \
				Address += Stride; \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			const int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += pow(NORM(*(CTYPE2 *)(Address + Indices[I] * Stride)), P); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += pow(NORM(*(CTYPE2 *)Address), P); \
				Address += Stride; \
			} \
		} \
	} \
	return Sum; \
} \
\
static void fill_norms_ ## CTYPE2(int TargetDegree, double *Target, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceAddress, double P) { \
	if (TargetDegree == 0) { \
		*(double *)Target = pow(compute_norm_ ## CTYPE2(SourceDegree, SourceDimension, SourceAddress, P), 1 / P); \
	} else { \
		int SourceStride = SourceDimension->Stride; \
		if (SourceDimension->Indices) { \
			const int *Indices = SourceDimension->Indices; \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_norms_ ## CTYPE2(TargetDegree - 1, Target, SourceDegree - 1, SourceDimension + 1, SourceAddress + Indices[I] * SourceStride, P); \
				++Target; \
			} \
		} else { \
			for (int I = 0; I < SourceDimension->Size; ++I) { \
				fill_norms_ ## CTYPE2(TargetDegree - 1, Target, SourceDegree - 1, SourceDimension + 1, SourceAddress, P); \
				++Target; \
				SourceAddress += SourceStride; \
			} \
		} \
	} \
}

NORM_FUNCTION(double, uint8_t, );
NORM_FUNCTION(double, int8_t, labs);
NORM_FUNCTION(double, uint16_t, );
NORM_FUNCTION(double, int16_t, labs);
NORM_FUNCTION(double, uint32_t, );
NORM_FUNCTION(double, int32_t, labs);
NORM_FUNCTION(double, uint64_t, );
NORM_FUNCTION(double, int64_t, llabs);
NORM_FUNCTION(double, float, fabs);
NORM_FUNCTION(double, double, fabs);

#ifdef ML_COMPLEX

NORM_FUNCTION(double, complex_float, cabs);
NORM_FUNCTION(double, complex_double, cabs);

#endif

ML_METHOD("||", MLArrayT) {
//<Array
//>number
// Returns the norm of the values in :mini:`Array`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	double P = 2;
	double (*norm)(int Degree, ml_array_dimension_t *Dimension, void *Address, double P);
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8: norm = compute_norm_uint8_t; break;
	case ML_ARRAY_FORMAT_I8: norm = compute_norm_int8_t; break;
	case ML_ARRAY_FORMAT_U16: norm = compute_norm_uint16_t; break;
	case ML_ARRAY_FORMAT_I16: norm = compute_norm_int16_t; break;
	case ML_ARRAY_FORMAT_U32: norm = compute_norm_uint32_t; break;
	case ML_ARRAY_FORMAT_I32: norm = compute_norm_int32_t; break;
	case ML_ARRAY_FORMAT_U64: norm = compute_norm_uint64_t; break;
	case ML_ARRAY_FORMAT_I64: norm = compute_norm_int64_t; break;
	case ML_ARRAY_FORMAT_F32: norm = compute_norm_float; break;
	case ML_ARRAY_FORMAT_F64: norm = compute_norm_double; break;
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32: norm = compute_norm_complex_float; break;
	case ML_ARRAY_FORMAT_C64: norm = compute_norm_complex_double; break;
#endif
	default: return ml_error("ArrayError", "Invalid array format");
	}
	double Norm = norm(Source->Degree, Source->Dimensions, Source->Base.Value, P);
	return ml_real(pow(Norm, 1 / P));
}

ML_METHOD("||", MLArrayT, MLRealT) {
//<Array
//>number
// Returns the norm of the values in :mini:`Array`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	double P = ml_real_value(Args[1]);
	double (*norm)(int Degree, ml_array_dimension_t *Dimension, void *Address, double P);
	if (P < 1) return ml_error("ValueError", "Invalid p-value for norm");
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8: norm = compute_norm_uint8_t; break;
	case ML_ARRAY_FORMAT_I8: norm = compute_norm_int8_t; break;
	case ML_ARRAY_FORMAT_U16: norm = compute_norm_uint16_t; break;
	case ML_ARRAY_FORMAT_I16: norm = compute_norm_int16_t; break;
	case ML_ARRAY_FORMAT_U32: norm = compute_norm_uint32_t; break;
	case ML_ARRAY_FORMAT_I32: norm = compute_norm_int32_t; break;
	case ML_ARRAY_FORMAT_U64: norm = compute_norm_uint64_t; break;
	case ML_ARRAY_FORMAT_I64: norm = compute_norm_int64_t; break;
	case ML_ARRAY_FORMAT_F32: norm = compute_norm_float; break;
	case ML_ARRAY_FORMAT_F64: norm = compute_norm_double; break;
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32: norm = compute_norm_complex_float; break;
	case ML_ARRAY_FORMAT_C64: norm = compute_norm_complex_double; break;
#endif
	default: return ml_error("ArrayError", "Invalid array format");
	}
	double Norm = norm(Source->Degree, Source->Dimensions, Source->Base.Value, P);
	return ml_real(pow(Norm, 1 / P));
}

ML_METHOD("||", MLArrayT, MLRealT, MLIntegerT) {
//<Array
//>number
// Returns the norm of the values in :mini:`Array`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	double P = ml_real_value(Args[1]);
	int Slice = ml_integer_value(Args[2]);
	if (Slice <= 0 || Slice >= Source->Degree) return ml_error("IntervalError", "Invalid axes count for norm");
	void (*fill)(int TargetDegree, double *Target, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceAddress, double P);
	if (P < 1) return ml_error("ValueError", "Invalid p-value for norm");
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8: fill = fill_norms_uint8_t; break;
	case ML_ARRAY_FORMAT_I8: fill = fill_norms_int8_t; break;
	case ML_ARRAY_FORMAT_U16: fill = fill_norms_uint16_t; break;
	case ML_ARRAY_FORMAT_I16: fill = fill_norms_int16_t; break;
	case ML_ARRAY_FORMAT_U32: fill = fill_norms_uint32_t; break;
	case ML_ARRAY_FORMAT_I32: fill = fill_norms_int32_t; break;
	case ML_ARRAY_FORMAT_U64: fill = fill_norms_uint64_t; break;
	case ML_ARRAY_FORMAT_I64: fill = fill_norms_int64_t; break;
	case ML_ARRAY_FORMAT_F32: fill = fill_norms_float; break;
	case ML_ARRAY_FORMAT_F64: fill = fill_norms_double; break;
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32: fill = fill_norms_complex_float; break;
	case ML_ARRAY_FORMAT_C64: fill = fill_norms_complex_double; break;
#endif
	default: return ml_error("ArrayError", "Invalid array format");
	}
	ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_F64, Source->Degree - Slice);
	int DataSize = MLArraySizes[Target->Format];
	for (int I = Target->Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Value = array_alloc(ML_ARRAY_FORMAT_F64, DataSize);
	fill(Target->Degree, (double *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, P);
	return (ml_value_t *)Target;
}

ML_METHOD("-", MLArrayT) {
//<Array
//>array
// Returns an array with the negated values from :mini:`Array`.
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation");
	int Degree = A->Degree;
	switch (A->Format) {
	case ML_ARRAY_FORMAT_U8:
	case ML_ARRAY_FORMAT_I8: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I8, Degree);
		int DataSize = ml_array_copy(C, A);
		int8_t *Values = (int8_t *)C->Base.Value;
		for (int I = DataSize / sizeof(int8_t); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
	case ML_ARRAY_FORMAT_U16:
	case ML_ARRAY_FORMAT_I16: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I16, Degree);
		int DataSize = ml_array_copy(C, A);
		int16_t *Values = (int16_t *)C->Base.Value;
		for (int I = DataSize / sizeof(int16_t); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
	case ML_ARRAY_FORMAT_U32:
	case ML_ARRAY_FORMAT_I32: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I32, Degree);
		int DataSize = ml_array_copy(C, A);
		int32_t *Values = (int32_t *)C->Base.Value;
		for (int I = DataSize / sizeof(int32_t); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
	case ML_ARRAY_FORMAT_U64:
	case ML_ARRAY_FORMAT_I64: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I64, Degree);
		int DataSize = ml_array_copy(C, A);
		int64_t *Values = (int64_t *)C->Base.Value;
		for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
	case ML_ARRAY_FORMAT_F32: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_F32, Degree);
		int DataSize = ml_array_copy(C, A);
		float *Values = (float *)C->Base.Value;
		for (int I = DataSize / sizeof(float); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
	case ML_ARRAY_FORMAT_F64: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_F64, Degree);
		int DataSize = ml_array_copy(C, A);
		double *Values = (double *)C->Base.Value;
		for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_C32, Degree);
		int DataSize = ml_array_copy(C, A);
		complex_float *Values = (complex_float *)C->Base.Value;
		for (int I = DataSize / sizeof(complex_float); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
	case ML_ARRAY_FORMAT_C64: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_C64, Degree);
		int DataSize = ml_array_copy(C, A);
		complex_double *Values = (complex_double *)C->Base.Value;
		for (int I = DataSize / sizeof(complex_double); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
#endif
	default: {
		return ml_error("TypeError", "Invalid types for array operation");
	}
	}
}

static ml_value_t *array_math_integer_fn(int64_t (*fn)(int64_t), int Count, ml_value_t **Args) {
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Degree == -1) return (ml_value_t *)A;
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation");
	int Degree = A->Degree;
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I64, Degree);
	int DataSize = ml_array_copy(C, A);
	int64_t *Values = (int64_t *)C->Base.Value;
	for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = fn(*Values);
	return (ml_value_t *)C;
}

static ml_value_t *array_math_real_fn(double (*fn)(double), int Count, ml_value_t **Args) {
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Degree == -1) return (ml_value_t *)A;
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation");
	int Degree = A->Degree;
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_F64, Degree);
	int DataSize = ml_array_copy(C, A);
	double *Values = (double *)C->Base.Value;
	for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = fn(*Values);
	return (ml_value_t *)C;
}

#ifdef ML_COMPLEX

static ml_value_t *array_math_complex_fn(complex_double (*fn)(complex_double), int Count, ml_value_t **Args) {
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Degree == -1) return (ml_value_t *)A;
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation");
	int Degree = A->Degree;
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_C64, Degree);
	int DataSize = ml_array_copy(C, A);
	complex_double *Values = (complex_double *)C->Base.Value;
	for (int I = DataSize / sizeof(complex_double); --I >= 0; ++Values) *Values = fn(*Values);
	return (ml_value_t *)C;
}

static ml_value_t *array_math_complex_real_fn(double (*fn)(complex_double), int Count, ml_value_t **Args) {
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Degree == -1) return (ml_value_t *)A;
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation");
	int Degree = A->Degree;
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_C64, Degree);
	int DataSize = ml_array_copy(C, A);
	complex_double *Values = (complex_double *)C->Base.Value;
	for (int I = DataSize / sizeof(complex_double); --I >= 0; ++Values) *Values = fn(*Values);
	ml_array_t *D = ml_array_alloc(ML_ARRAY_FORMAT_F64, Degree);
	ml_array_copy(D, C);
	return (ml_value_t *)D;
}
#endif

#define UPDATE_FNS(TITLE) \
extern update_row_fn_t Update ## TITLE ## RowFns[];

UPDATE_FNS(Set);
UPDATE_FNS(Add);
UPDATE_FNS(Mul);
UPDATE_FNS(Sub);
UPDATE_FNS(RSub);
UPDATE_FNS(Div);
UPDATE_FNS(RDiv);
UPDATE_FNS(And);
UPDATE_FNS(Or);
UPDATE_FNS(Xor);
UPDATE_FNS(Min);
UPDATE_FNS(Max);

void update_array(update_row_fn_t Update, ml_array_dimension_t *TargetDimension, char *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, char *SourceData) {
	if (SourceDegree == 0) {
		ml_array_dimension_t ConstantDimension[1] = {{TargetDimension->Size, 0, NULL}};
		return Update(TargetDimension, TargetData, ConstantDimension, SourceData);
	}
	if (SourceDegree == 1) {
		return Update(TargetDimension, TargetData, SourceDimension, SourceData);
	}
	int Size = TargetDimension->Size;
	if (TargetDimension->Indices) {
		const int *TargetIndices = TargetDimension->Indices;
		if (SourceDimension->Indices) {
			const int *SourceIndices = SourceDimension->Indices;
			for (int I = 0; I < Size; ++I) {
				update_array(Update, TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData + SourceIndices[I] * SourceDimension->Stride);
			}
		} else {
			int SourceStride = SourceDimension->Stride;
			for (int I = 0; I < Size; ++I) {
				update_array(Update, TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData);
				SourceData += SourceStride;
			}
		}
	} else {
		int TargetStride = TargetDimension->Stride;
		if (SourceDimension->Indices) {
			const int *SourceIndices = SourceDimension->Indices;
			for (int I = 0; I < Size; ++I) {
				update_array(Update, TargetDimension + 1, TargetData, SourceDegree - 1, SourceDimension + 1, SourceData + SourceIndices[I] * SourceDimension->Stride);
				TargetData += TargetStride;
			}
		} else {
			int SourceStride = SourceDimension->Stride;
			for (int I = Size; --I >= 0;) {
				update_array(Update, TargetDimension + 1, TargetData, SourceDegree - 1, SourceDimension + 1, SourceData);
				TargetData += TargetStride;
				SourceData += SourceStride;
			}
		}
	}
}

void update_prefix(update_row_fn_t Update, int PrefixDegree, ml_array_dimension_t *TargetDimension, char *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, char *SourceData) {
	if (PrefixDegree == 0) return update_array(Update, TargetDimension, TargetData, SourceDegree, SourceDimension, SourceData);
	int Size = TargetDimension->Size;
	if (TargetDimension->Indices) {
		const int *TargetIndices = TargetDimension->Indices;
		for (int I = Size; --I >= 0;) {
			update_prefix(Update, PrefixDegree - 1, TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree, SourceDimension, SourceData);
		}
	} else {
		int Stride = TargetDimension->Stride;
		for (int I = Size; --I >= 0;) {
			update_prefix(Update, PrefixDegree - 1, TargetDimension + 1, TargetData, SourceDegree, SourceDimension, SourceData);
			TargetData += Stride;
		}
	}
}

static ml_value_t *update_array_fn(void *Data, int Count, ml_value_t **Args) {
	update_row_fn_t *Updates = (update_row_fn_t *)Data;
	ml_array_t *Target = (ml_array_t *)Args[0];
	if (Target->Degree == -1) return (ml_value_t *)Target;
	ml_array_t *Source = (ml_array_t *)Args[1];
	if (Source->Degree == -1) return (ml_value_t *)Target;
	if (Source->Degree > Target->Degree) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
	int PrefixDegree = Target->Degree - Source->Degree;
	for (int I = 0; I < Source->Degree; ++I) {
		if (Target->Dimensions[PrefixDegree + I].Size != Source->Dimensions[I].Size) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
	}
	update_row_fn_t Update = Updates[Target->Format * MAX_FORMATS + Source->Format];
	if (!Update) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", Target->Base.Type->Name, Source->Base.Type->Name);
	if (Target->Degree) {
		update_prefix(Update, PrefixDegree, Target->Dimensions, Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value);
	} else {
		ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}};
		Update(ValueDimension, Target->Base.Value, ValueDimension, Source->Base.Value);
	}
	return Args[0];
}

static ml_value_t *array_infix_fn(void *Data, int Count, ml_value_t **Args) {
	update_row_fn_t *Updates = (update_row_fn_t *)Data;
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Degree == -1) return (ml_value_t *)A;
	ml_array_t *B = (ml_array_t *)Args[1];
	if (B->Degree == -1) return (ml_value_t *)B;
	ml_array_format_t Format = MAX(A->Format, B->Format);
	if (Updates == UpdateDivRowFns) Format = MAX(Format, ML_ARRAY_FORMAT_F64);
	if (A->Degree < B->Degree) {
		ml_array_t *T = A;
		A = B;
		B = T;
		if (Updates == UpdateSubRowFns) {
			Updates = UpdateRSubRowFns;
		} else if (Updates == UpdateDivRowFns) {
			Updates = UpdateRDivRowFns;
		}
	}
	int Degree = A->Degree;
	for (int D = A->Degree - B->Degree, I = B->Degree; --I >= 0;) {
		if (A->Dimensions[D + I].Size != B->Dimensions[I].Size) {
			return ml_error("ShapeError", "Incompatible arrays");
		}
	}
	ml_array_t *C = ml_array_alloc(Format, Degree);
	ml_array_copy(C, A);
	update_row_fn_t Update = Updates[C->Format * MAX_FORMATS + B->Format];
	if (!Update) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", C->Base.Type->Name, B->Base.Type->Name);
	update_prefix(Update, C->Degree - B->Degree, C->Dimensions, C->Base.Value, B->Degree, B->Dimensions, B->Base.Value);
	return (ml_value_t *)C;
}

#define INFIX_METHOD(OP) \
/*
ML_METHOD(#OP, MLArrayT, MLArrayT) {
//<A
//<B
//>array
// Returns :mini:`A OP B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible, i.e. either
//
// * :mini:`A:shape = B:shape` or
// * :mini:`A:shape` is a prefix of :mini:`B:shape` or
// * :mini:`B:shape` is a prefix of :mini:`A:shape`.
//
// When the shapes are not the same, remaining dimensions are repeated (broadcast) to the required size.
//$= let A := array([[1, 2, 3], [4, 5, 6]])
//$= let B := array([[7, 8, 9], [10, 11, 12]])
//$= let C := array([5, 10, 15])
//$= A OP B
//$= B OP A
//$= A OP C
//$= C OP A
//$= B OP C
//$= C OP B
}
*/

INFIX_METHOD(+)
INFIX_METHOD(-)
INFIX_METHOD(*)
INFIX_METHOD(/)
INFIX_METHOD(/\\)
INFIX_METHOD(\\/)
INFIX_METHOD(><)
INFIX_METHOD(min)
INFIX_METHOD(max)

#ifdef ML_COMPLEX

#define op_complex_array_left(NAME) \
	case ML_ARRAY_FORMAT_C32: { \
		complex_float *Values = (complex_float *)C->Base.Value; \
		for (int I = DataSize / sizeof(complex_float); --I >= 0; ++Values) *Values = *Values NAME B; \
		break; \
	} \
	case ML_ARRAY_FORMAT_C64: { \
		complex_double *Values = (complex_double *)C->Base.Value; \
		for (int I = DataSize / sizeof(complex_double); --I >= 0; ++Values) *Values = *Values NAME B; \
		break; \
	}

#define op_complex_array_right(NAME) \
	case ML_ARRAY_FORMAT_C32: { \
		complex_float *Values = (complex_float *)C->Base.Value; \
		for (int I = DataSize / sizeof(complex_float); --I >= 0; ++Values) *Values = B NAME *Values; \
		break; \
	} \
	case ML_ARRAY_FORMAT_C64: { \
		complex_double *Values = (complex_double *)C->Base.Value; \
		for (int I = DataSize / sizeof(complex_double); --I >= 0; ++Values) *Values = B NAME *Values; \
		break; \
	}

#else

#define op_complex_array_left(NAME)
#define op_complex_array_right(NAME)

#endif

#define ML_ARITH_METHOD_BASE(NAME, MIN_FORMAT, METHOD) \
\
ML_METHOD(#NAME, MLArrayT, MLAnyT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A/v NAME B`.
//$= let A := array([[1, 2], [3, 4]])
//$= A NAME 2
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	ml_value_t *B = Args[1]; \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_ANY, Degree); \
	int DataSize = ml_array_copy(C, A); \
	ml_value_t **Values = (ml_value_t **)C->Base.Value; \
	for (int I = DataSize / sizeof(ml_value_t *); --I >= 0; ++Values) { \
		*Values = ml_simple_inline(METHOD, 2, *Values, B); \
	} \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLAnyT, MLArrayT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A NAME B/v`.
//$= let B := array([[1, 2], [3, 4]])
//$= 2 NAME B
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	ml_value_t *B = Args[0]; \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_ANY, Degree); \
	int DataSize = ml_array_copy(C, A); \
	ml_value_t **Values = (ml_value_t **)C->Base.Value; \
	for (int I = DataSize / sizeof(ml_value_t *); --I >= 0; ++Values) { \
		*Values = ml_simple_inline(METHOD, 2, B, *Values); \
	} \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLArrayIntegerT, MLIntegerT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A/v NAME B`.
//$= let A := array([[1, 2], [3, 4]])
//$= A NAME 2
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	int64_t B = ml_integer_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(MAX(A->Format, MIN_FORMAT), Degree); \
	int DataSize = ml_array_copy(C, A); \
	switch (C->Format) { \
	case ML_ARRAY_FORMAT_U64: { \
		uint64_t *Values = (uint64_t *)C->Base.Value; \
		for (int I = DataSize / sizeof(uint64_t); --I >= 0; ++Values) *Values = *Values NAME B; \
		break; \
	} \
	case ML_ARRAY_FORMAT_I64: { \
		int64_t *Values = (int64_t *)C->Base.Value; \
		for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = *Values NAME B; \
		break; \
	} \
	case ML_ARRAY_FORMAT_F32: { \
		float *Values = (float *)C->Base.Value; \
		for (int I = DataSize / sizeof(float); --I >= 0; ++Values) *Values = *Values NAME B; \
		break; \
	} \
	case ML_ARRAY_FORMAT_F64: { \
		double *Values = (double *)C->Base.Value; \
		for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = *Values NAME B; \
		break; \
	} \
	op_complex_array_left(NAME) \
	default: { \
		return ml_error("TypeError", "Invalid types for array operation"); \
	} \
	} \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLIntegerT, MLArrayIntegerT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A NAME B/v`.
//$= let A := array([[1, 2], [3, 4]])
//$= 2 NAME A
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	int64_t B = ml_integer_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(MAX(A->Format, MIN_FORMAT), Degree); \
	int DataSize = ml_array_copy(C, A); \
	switch (C->Format) { \
	case ML_ARRAY_FORMAT_U64: { \
		uint64_t *Values = (uint64_t *)C->Base.Value; \
		for (int I = DataSize / sizeof(uint64_t); --I >= 0; ++Values) *Values = B NAME *Values; \
		break; \
	} \
	case ML_ARRAY_FORMAT_I64: { \
		int64_t *Values = (int64_t *)C->Base.Value; \
		for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = B NAME *Values; \
		break; \
	} \
	case ML_ARRAY_FORMAT_F32: { \
		float *Values = (float *)C->Base.Value; \
		for (int I = DataSize / sizeof(float); --I >= 0; ++Values) *Values = B NAME *Values; \
		break; \
	} \
	case ML_ARRAY_FORMAT_F64: { \
		double *Values = (double *)C->Base.Value; \
		for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = B NAME *Values; \
		break; \
	} \
	op_complex_array_right(NAME) \
	default: { \
		return ml_error("TypeError", "Invalid types for array operation"); \
	} \
	} \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLArrayRealT, MLRealT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A/v NAME B`.
//$= let A := array([[1, 2], [3, 4]])
//$= A NAME 2.5
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	double B = ml_real_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(MAX(A->Format, ML_ARRAY_FORMAT_F64), Degree); \
	int DataSize = ml_array_copy(C, A); \
	switch (C->Format) { \
	case ML_ARRAY_FORMAT_F64: { \
		double *Values = (double *)C->Base.Value; \
		for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = *Values NAME B; \
		break; \
	} \
	op_complex_array_left(NAME) \
	default: { \
		return ml_error("TypeError", "Invalid types for array operation"); \
	} \
	} \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLRealT, MLArrayRealT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A NAME B/v`.
//$= let A := array([[1, 2], [3, 4]])
//$= 2.5 NAME A
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	double B = ml_real_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(MAX(A->Format, ML_ARRAY_FORMAT_F64), Degree); \
	int DataSize = ml_array_copy(C, A); \
	switch (C->Format) { \
	case ML_ARRAY_FORMAT_F64: { \
		double *Values = (double *)C->Base.Value; \
		for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = B NAME *Values; \
		break; \
	} \
	op_complex_array_right(NAME) \
	default: { \
		return ml_error("TypeError", "Invalid types for array operation"); \
	} \
	} \
	return (ml_value_t *)C; \
}

#ifdef ML_COMPLEX

#define ML_ARITH_METHOD(NAME, MIN_FORMAT, METHOD) \
ML_ARITH_METHOD_BASE(NAME, MIN_FORMAT, METHOD) \
\
ML_METHOD(#NAME, MLArrayComplexT, MLComplexT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A/v NAME B`.
//$= let A := array([[1, 2], [3, 4]])
//$= A NAME (1 + 1i)
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	complex_double B = ml_complex_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_C64, Degree); \
	int DataSize = ml_array_copy(C, A); \
	complex_double *Values = (complex_double *)C->Base.Value; \
	for (int I = DataSize / sizeof(complex_double); --I >= 0; ++Values) *Values = *Values NAME B; \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLComplexT, MLArrayComplexT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A NAME B/v`.
//$= let A := array([[1, 2], [3, 4]])
//$= (1 + 1i) NAME A
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	complex_double B = ml_complex_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_C64, Degree); \
	int DataSize = ml_array_copy(C, A); \
	complex_double *Values = (complex_double *)C->Base.Value; \
	for (int I = DataSize / sizeof(complex_double); --I >= 0; ++Values) *Values = B NAME *Values; \
	return (ml_value_t *)C; \
}

#else

#define ML_ARITH_METHOD(NAME, MIN_FORMAT, METHOD) \
ML_ARITH_METHOD_BASE(NAME, MIN_FORMAT, METHOD)

#endif

extern ml_value_t *MulMethod;
extern ml_value_t *AddMethod;
extern ml_value_t *SubMethod;
extern ml_value_t *DivMethod;

ML_ARITH_METHOD(+, ML_ARRAY_FORMAT_I64, AddMethod);
ML_ARITH_METHOD(*, ML_ARRAY_FORMAT_I64, MulMethod);
ML_ARITH_METHOD(-, ML_ARRAY_FORMAT_I64, SubMethod);
ML_ARITH_METHOD(/, ML_ARRAY_FORMAT_F64, DivMethod);

#define ML_ARITH_METHOD_BITWISE(NAME, SYMBOL, OP) \
\
ML_METHOD(#NAME, MLArrayT, MLIntegerT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A/v bitwise OP B`.
//$= let A := array([[1, 2], [3, 4]])
//$= A NAME 2
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I64, Degree); \
	int DataSize = ml_array_copy(C, A); \
	int64_t *Values = (int64_t *)C->Base.Value; \
	for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = *Values SYMBOL B; \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLIntegerT, MLArrayT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A bitwise OP B/v`.
//$= let A := array([[1, 2], [3, 4]])
//$= 2 NAME A
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I64, Degree); \
	int DataSize = ml_array_copy(C, A); \
	int64_t *Values = (int64_t *)C->Base.Value; \
	for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = B SYMBOL *Values; \
	return (ml_value_t *)C; \
}

ML_ARITH_METHOD_BITWISE(/\\, &, and)
ML_ARITH_METHOD_BITWISE(\\/, |, or)
ML_ARITH_METHOD_BITWISE(><, ^, xor)

#define ML_ARITH_METHOD_MINMAX(NAME, FN) \
\
ML_METHOD(#NAME, MLArrayT, MLIntegerT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := NAME(A/v, B)`.
//$= let A := array([[1, 2], [3, 4]])
//$= A NAME 2
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I64, Degree); \
	int DataSize = ml_array_copy(C, A); \
	int64_t *Values = (int64_t *)C->Base.Value; \
	for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = FN(*Values, B); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLIntegerT, MLArrayT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := NAME(A, B/v)`.
//$= let A := array([[1, 2], [3, 4]])
//$= 2 NAME A
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I64, Degree); \
	int DataSize = ml_array_copy(C, A); \
	int64_t *Values = (int64_t *)C->Base.Value; \
	for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = FN(B, *Values); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLArrayT, MLRealT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := NAME(A/v, B)`.
//$= let A := array([[1, 2], [3, 4]])
//$= A NAME 2.5
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	double B = ml_real_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_F64, Degree); \
	int DataSize = ml_array_copy(C, A); \
	double *Values = (double *)C->Base.Value; \
	for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = FN(*Values, B); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLRealT, MLArrayT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := NAME(A, B/v)`.
//$= let A := array([[1, 2], [3, 4]])
//$= 2.5 NAME A
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_real_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_F64, Degree); \
	int DataSize = ml_array_copy(C, A); \
	double *Values = (double *)C->Base.Value; \
	for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = FN(B, *Values); \
	return (ml_value_t *)C; \
}

ML_ARITH_METHOD_MINMAX(min, MIN)
ML_ARITH_METHOD_MINMAX(max, MAX)

#ifdef ML_COMPLEX

ML_METHOD("^", MLArrayComplexT, MLComplexT) {
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Degree == -1) return (ml_value_t *)A;
	complex_double B = ml_complex_value(Args[1]);
	int Degree = A->Degree;
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_C64, Degree);
	int DataSize = ml_array_copy(C, A);
	complex_double *Values = (complex_double *)C->Base.Value;
	for (int I = DataSize / sizeof(complex_double); --I >= 0; ++Values) *Values = cpow(*Values, B);
	return (ml_value_t *)C;
}

#else

ML_METHOD("^", MLArrayRealT, MLRealT) {
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Degree == -1) return (ml_value_t *)A;
	double B = ml_real_value(Args[1]);
	int Degree = A->Degree;
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_F64, Degree);
	int DataSize = ml_array_copy(C, A);
	double *Values = (double *)C->Base.Value;
	for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = pow(*Values, B);
	return (ml_value_t *)C;
}

#endif

extern int ml_array_compare(ml_array_t *A, ml_array_t *B);

typedef void (*compare_row_fn_t)(char *Target, ml_array_dimension_t *LeftDimension, char *LeftData, ml_array_dimension_t *RightDimension, char *RightData);

#define COMPARE_FNS(TITLE) \
extern compare_row_fn_t Compare ## TITLE ## RowFns[];

COMPARE_FNS(Eq);
COMPARE_FNS(Ne);
COMPARE_FNS(Lt);
COMPARE_FNS(Gt);
COMPARE_FNS(Le);
COMPARE_FNS(Ge);

static void compare_array(compare_row_fn_t Compare, ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *LeftDimension, char *LeftData, int RightDegree, ml_array_dimension_t *RightDimension, char *RightData) {
	if (RightDegree == 0) {
		ml_array_dimension_t ConstantDimension[1] = {{LeftDimension->Size, 0, NULL}};
		return Compare(TargetData, LeftDimension, LeftData, ConstantDimension, RightData);
	}
	if (RightDegree == 1) {
		return Compare(TargetData, LeftDimension, LeftData, RightDimension, RightData);
	}
	int Size = LeftDimension->Size;
	int TargetStride = TargetDimension->Stride;
	if (LeftDimension->Indices) {
		const int *LeftIndices = LeftDimension->Indices;
		if (RightDimension->Indices) {
			const int *RightIndices = RightDimension->Indices;
			for (int I = 0; I < Size; ++I) {
				compare_array(Compare, TargetDimension + 1, TargetData, LeftDimension + 1, LeftData + LeftIndices[I] * LeftDimension->Stride, RightDegree - 1, RightDimension + 1, RightData + RightIndices[I] * RightDimension->Stride);
				TargetData += TargetStride;
			}
		} else {
			int RightStride = RightDimension->Stride;
			for (int I = 0; I < Size; ++I) {
				compare_array(Compare, TargetDimension + 1, TargetData, LeftDimension + 1, LeftData + LeftIndices[I] * LeftDimension->Stride, RightDegree - 1, RightDimension + 1, RightData);
				RightData += RightStride;
				TargetData += TargetStride;
			}
		}
	} else {
		int LeftStride = LeftDimension->Stride;
		if (RightDimension->Indices) {
			const int *RightIndices = RightDimension->Indices;
			for (int I = 0; I < Size; ++I) {
				compare_array(Compare, TargetDimension + 1, TargetData, LeftDimension + 1, LeftData, RightDegree - 1, RightDimension + 1, RightData + RightIndices[I] * RightDimension->Stride);
				LeftData += LeftStride;
				TargetData += TargetStride;
			}
		} else {
			int RightStride = RightDimension->Stride;
			for (int I = Size; --I >= 0;) {
				compare_array(Compare, TargetDimension + 1, TargetData, LeftDimension + 1, LeftData, RightDegree - 1, RightDimension + 1, RightData);
				LeftData += LeftStride;
				RightData += RightStride;
				TargetData += TargetStride;
			}
		}
	}
}

static void compare_prefix(compare_row_fn_t Compare, ml_array_dimension_t *TargetDimension, char *TargetData, int PrefixDegree, ml_array_dimension_t *LeftDimension, char *LeftData, int RightDegree, ml_array_dimension_t *RightDimension, char *RightData) {
	if (PrefixDegree == 0) return compare_array(Compare, TargetDimension, TargetData, LeftDimension, LeftData, RightDegree, RightDimension, RightData);
	int Size = LeftDimension->Size;
	int TargetStride = TargetDimension->Stride;
	if (LeftDimension->Indices) {
		const int *LeftIndices = LeftDimension->Indices;
		for (int I = Size; --I >= 0;) {
			compare_prefix(Compare, TargetDimension + 1, TargetData, PrefixDegree - 1, LeftDimension + 1, LeftData + LeftIndices[I] * LeftDimension->Stride, RightDegree, RightDimension, RightData);
			TargetData += TargetStride;
		}
	} else {
		int Stride = LeftDimension->Stride;
		for (int I = Size; --I >= 0;) {
			compare_prefix(Compare, TargetDimension + 1, TargetData, PrefixDegree - 1, LeftDimension + 1, LeftData, RightDegree, RightDimension, RightData);
			LeftData += Stride;
			TargetData += TargetStride;
		}
	}
}

static ml_value_t *compare_array_fn(void *Data, int Count, ml_value_t **Args) {
	compare_row_fn_t *Compares = (compare_row_fn_t *)Data;
	ml_array_t *Left = (ml_array_t *)Args[0];
	ml_array_t *Right = (ml_array_t *)Args[1];
	int Degree = Left->Degree;
	if (Right->Degree > Degree) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
	int PrefixDegree = Degree - Right->Degree;
	for (int I = 0; I < Right->Degree; ++I) {
		if (Left->Dimensions[PrefixDegree + I].Size != Right->Dimensions[I].Size) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
	}
	ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_I8, Degree);
	int DataSize = 1;
	for (int I = Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Left->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Value = snew(DataSize);
	compare_row_fn_t Compare = Compares[Left->Format * MAX_FORMATS + Right->Format];
	if (!Compare) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", Left->Base.Type->Name, Right->Base.Type->Name);
	if (Degree) {
		compare_prefix(Compare, Target->Dimensions, Target->Base.Value, PrefixDegree, Left->Dimensions, Left->Base.Value, Right->Degree, Right->Dimensions, Right->Base.Value);
	} else {
		ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}};
		Compare(Target->Base.Value, ValueDimension, Left->Base.Value, ValueDimension, Right->Base.Value);
	}
	return (ml_value_t *)Target;
}

ML_METHOD("<>", MLArrayT, MLArrayT) {
//<A
//<B
//>integer
// Compare the degrees, dimensions and entries of :mini:`A` and :mini:`B` and returns :mini:`-1`, :mini:`0` or :mini:`1`. This method is only intending for sorting arrays or using them as keys in a map.
	ml_array_t *A = (ml_array_t *)Args[0];
	ml_array_t *B = (ml_array_t *)Args[1];
	return ml_integer(ml_array_compare(A, B));
}

ML_METHOD("=", MLArrayT, MLArrayT) {
//<A
//<B
//>integer
// Compare the degrees, dimensions and entries of :mini:`A` and :mini:`B` and returns :mini:`B` if they match and :mini:`nil` otherwise.
	ml_array_t *A = (ml_array_t *)Args[0];
	ml_array_t *B = (ml_array_t *)Args[1];
	return ml_array_compare(A, B) ? MLNil : (ml_value_t *)B;
}

ML_METHOD("!=", MLArrayT, MLArrayT) {
//<A
//<B
//>integer
// Compare the degrees, dimensions and entries of :mini:`A` and :mini:`B` and returns :mini:`nil` if they match and :mini:`B` otherwise.
	ml_array_t *A = (ml_array_t *)Args[0];
	ml_array_t *B = (ml_array_t *)Args[1];
	return ml_array_compare(A, B) ? (ml_value_t *)B : MLNil;
}

#define ML_COMPARE_METHOD_BASE(TITLE, TITLE2, OP, NAME) \
\
ML_METHOD(#NAME, MLArrayT, MLIntegerT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := if A/v OP B then 1 else 0 end`.
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	int64_t B = ml_integer_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I8, Degree); \
	int DataSize = 1; \
	for (int I = Degree; --I >= 0;) { \
		C->Dimensions[I].Stride = DataSize; \
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	C->Base.Value = snew(DataSize); \
	compare_row_fn_t Compare = Compare ## TITLE ## RowFns[A->Format * MAX_FORMATS + ML_ARRAY_FORMAT_I64]; \
	if (!Compare) return ml_error("ArrayError", "Unsupported array format pair (%s, integer)", A->Base.Type->Name); \
	compare_prefix(Compare, C->Dimensions, C->Base.Value, Degree - 1, A->Dimensions, A->Base.Value, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLIntegerT, MLArrayT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := if A OP B/v then 1 else 0 end`.
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	int64_t B = ml_integer_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I8, Degree); \
	int DataSize = 1; \
	for (int I = Degree; --I >= 0;) { \
		C->Dimensions[I].Stride = DataSize; \
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	C->Base.Value = snew(DataSize); \
	compare_row_fn_t Compare = Compare ## TITLE2 ## RowFns[A->Format * MAX_FORMATS + ML_ARRAY_FORMAT_I64]; \
	if (!Compare) return ml_error("ArrayError", "Unsupported array format pair (integer, %s)", A->Base.Type->Name); \
	compare_prefix(Compare, C->Dimensions, C->Base.Value, Degree - 1, A->Dimensions, A->Base.Value, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLArrayT, MLRealT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := if A/v OP B then 1 else 0 end`.
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	double B = ml_real_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I8, Degree); \
	int DataSize = 1; \
	for (int I = Degree; --I >= 0;) { \
		C->Dimensions[I].Stride = DataSize; \
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	C->Base.Value = snew(DataSize); \
	compare_row_fn_t Compare = Compare ## TITLE ## RowFns[A->Format * MAX_FORMATS + ML_ARRAY_FORMAT_F64]; \
	if (!Compare) return ml_error("ArrayError", "Unsupported array format pair (%s, real)", A->Base.Type->Name); \
	compare_prefix(Compare, C->Dimensions, C->Base.Value, Degree - 1, A->Dimensions, A->Base.Value, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLRealT, MLArrayT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := if A OP B/v then 1 else 0 end`.
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	double B = ml_real_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I8, Degree); \
	int DataSize = 1; \
	for (int I = Degree; --I >= 0;) { \
		C->Dimensions[I].Stride = DataSize; \
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	C->Base.Value = snew(DataSize); \
	compare_row_fn_t Compare = Compare ## TITLE2 ## RowFns[A->Format * MAX_FORMATS + ML_ARRAY_FORMAT_F64]; \
	if (!Compare) return ml_error("ArrayError", "Unsupported array format pair (real, %s)", A->Base.Type->Name); \
	compare_prefix(Compare, C->Dimensions, C->Base.Value, Degree - 1, A->Dimensions, A->Base.Value, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLArrayT, MLAnyT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := if A/v OP B then 1 else 0 end`.
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	ml_value_t *B = Args[1]; \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I8, Degree); \
	int DataSize = 1; \
	for (int I = Degree; --I >= 0;) { \
		C->Dimensions[I].Stride = DataSize; \
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	C->Base.Value = snew(DataSize); \
	compare_row_fn_t Compare = Compare ## TITLE ## RowFns[A->Format * MAX_FORMATS + ML_ARRAY_FORMAT_ANY]; \
	if (!Compare) return ml_error("ArrayError", "Unsupported array format pair (%s, any)", A->Base.Type->Name); \
	compare_prefix(Compare, C->Dimensions, C->Base.Value, Degree - 1, A->Dimensions, A->Base.Value, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLAnyT, MLArrayT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := if A OP B/v then 1 else 0 end`.
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	ml_value_t *B = Args[0]; \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I8, Degree); \
	int DataSize = 1; \
	for (int I = Degree; --I >= 0;) { \
		C->Dimensions[I].Stride = DataSize; \
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	C->Base.Value = snew(DataSize); \
	compare_row_fn_t Compare = Compare ## TITLE2 ## RowFns[A->Format * MAX_FORMATS + ML_ARRAY_FORMAT_ANY]; \
	if (!Compare) return ml_error("ArrayError", "Unsupported array format pair (any, %s)", A->Base.Type->Name); \
	compare_prefix(Compare, C->Dimensions, C->Base.Value, Degree - 1, A->Dimensions, A->Base.Value, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
}

#ifdef ML_COMPLEX

#define ML_COMPARE_METHOD(TITLE, TITLE2, OP, NAME) \
ML_COMPARE_METHOD_BASE(TITLE, TITLE2, OP, NAME) \
\
ML_METHOD(#NAME, MLArrayT, MLComplexT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := if A/v OP B then 1 else 0 end`.
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	complex double B = ml_complex_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I8, Degree); \
	int DataSize = 1; \
	for (int I = Degree; --I >= 0;) { \
		C->Dimensions[I].Stride = DataSize; \
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	C->Base.Value = snew(DataSize); \
	compare_row_fn_t Compare = Compare ## TITLE ## RowFns[A->Format * MAX_FORMATS + ML_ARRAY_FORMAT_C64]; \
	if (!Compare) return ml_error("ArrayError", "Unsupported array format pair (%s, complex)", A->Base.Type->Name); \
	compare_prefix(Compare, C->Dimensions, C->Base.Value, Degree - 1, A->Dimensions, A->Base.Value, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLComplexT, MLArrayT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := if A OP B/v then 1 else 0 end`.
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	complex double B = ml_complex_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I8, Degree); \
	int DataSize = 1; \
	for (int I = Degree; --I >= 0;) { \
		C->Dimensions[I].Stride = DataSize; \
		int Size = C->Dimensions[I].Size = A->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	C->Base.Value = snew(DataSize); \
	compare_row_fn_t Compare = Compare ## TITLE2 ## RowFns[A->Format * MAX_FORMATS + ML_ARRAY_FORMAT_C64]; \
	if (!Compare) return ml_error("ArrayError", "Unsupported array format pair (complex, %s)", A->Base.Type->Name); \
	compare_prefix(Compare, C->Dimensions, C->Base.Value, Degree - 1, A->Dimensions, A->Base.Value, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
}

#else

#define ML_COMPARE_METHOD(BASE, BASE2, OP, NAME) \
ML_COMPARE_METHOD_BASE(BASE, BASE2, OP, NAME)

#endif

ML_COMPARE_METHOD(Eq, Eq, =, ==);
ML_COMPARE_METHOD(Ne, Ne, !=, !==);
ML_COMPARE_METHOD(Lt, Gt, <, <);
ML_COMPARE_METHOD(Gt, Lt, >, >);
ML_COMPARE_METHOD(Le, Ge, <=, <=);
ML_COMPARE_METHOD(Ge, Le, >=, >=);

static int64_t isquare(int64_t X) {
	return X * X;
}

static double square(double X) {
	return X * X;
}

#ifdef ML_COMPLEX

static complex double csquare(complex double X) {
	return X * X;
}

#endif

#define ML_ARRAY_DOT(CTYPE) \
static void ml_array_dot_ ## CTYPE( \
	void *DataA, ml_array_dimension_t *DimA, ml_array_getter_ ## CTYPE GetterA, \
	void *DataB, ml_array_dimension_t *DimB, ml_array_getter_ ## CTYPE GetterB, \
	CTYPE *DataC \
) { \
	CTYPE Sum = 0; \
	int Size = DimA->Size; \
	int StrideA = DimA->Stride; \
	int StrideB = DimB->Stride; \
	if (DimA->Indices) { \
		const int *IndicesA = DimA->Indices; \
		if (DimB->Indices) { \
			const int *IndicesB = DimB->Indices; \
			for (int I = 0; I < Size; ++I) { \
				CTYPE ValueA = GetterA(DataA + IndicesA[I] * StrideA); \
				CTYPE ValueB = GetterB(DataB + IndicesB[I] * StrideB); \
				Sum += ValueA * ValueB; \
			} \
		} else { \
			for (int I = 0; I < Size; ++I) { \
				CTYPE ValueA = GetterA(DataA + IndicesA[I] * StrideA); \
				CTYPE ValueB = GetterB(DataB); \
				Sum += ValueA * ValueB; \
				DataB += StrideB; \
			} \
		} \
	} else { \
		if (DimB->Indices) { \
			const int *IndicesB = DimB->Indices; \
			for (int I = 0; I < Size; ++I) { \
				CTYPE ValueA = GetterA(DataA); \
				CTYPE ValueB = GetterB(DataB + IndicesB[I] * StrideB); \
				Sum += ValueA * ValueB; \
				DataA += StrideA; \
			} \
		} else { \
			for (int I = 0; I < Size; ++I) { \
				CTYPE ValueA = GetterA(DataA); \
				CTYPE ValueB = GetterB(DataB); \
				Sum += ValueA * ValueB; \
				DataA += StrideA; \
				DataB += StrideB; \
			} \
		} \
	} \
	*DataC = Sum; \
}

static void ml_array_dot_any(
	void *DataA, ml_array_dimension_t *DimA, ml_array_getter_any GetterA,
	void *DataB, ml_array_dimension_t *DimB, ml_array_getter_any GetterB,
	ml_value_t **DataC
) {
	ml_value_t *Args[3] = {MLNil, MLNil, MLNil};
	int Size = DimA->Size;
	int StrideA = DimA->Stride;
	int StrideB = DimB->Stride;
	if (DimA->Indices) {
		const int *IndicesA = DimA->Indices;
		if (DimB->Indices) {
			const int *IndicesB = DimB->Indices;
			Args[0] = GetterA(DataA + IndicesA[0] * StrideA);
			Args[1] = GetterB(DataB + IndicesB[0] * StrideB);
			Args[0] = ml_simple_call(MulMethod, 2, Args);
			for (int I = 1; I < Size; ++I) {
				Args[1] = GetterA(DataA + IndicesA[I] * StrideA);
				Args[2] = GetterB(DataB + IndicesB[I] * StrideB);
				Args[1] = ml_simple_call(MulMethod, 2, Args + 1);
				Args[0] = ml_simple_call(AddMethod, 2, Args);
			}
		} else {
			Args[0] = GetterA(DataA + IndicesA[0] * StrideA);
			Args[1] = GetterB(DataB);
			Args[0] = ml_simple_call(MulMethod, 2, Args);
			for (int I = 1; I < Size; ++I) {
				DataB += StrideB;
				Args[1] = GetterA(DataA + IndicesA[I] * StrideA);
				Args[2] = GetterB(DataB);
				Args[1] = ml_simple_call(MulMethod, 2, Args + 1);
				Args[0] = ml_simple_call(AddMethod, 2, Args);
			}
		}
	} else {
		if (DimB->Indices) {
			const int *IndicesB = DimB->Indices;
			Args[0] = GetterA(DataA);
			Args[1] = GetterB(DataB + IndicesB[0] * StrideB);
			Args[0] = ml_simple_call(MulMethod, 2, Args);
			for (int I = 1; I < Size; ++I) {
				DataA += StrideA;
				Args[1] = GetterA(DataA);
				Args[2] = GetterB(DataB + IndicesB[I] * StrideB);
				Args[1] = ml_simple_call(MulMethod, 2, Args + 1);
				Args[0] = ml_simple_call(AddMethod, 2, Args);
			}
		} else {
			Args[0] = GetterA(DataA);
			Args[1] = GetterB(DataB);
			Args[0] = ml_simple_call(MulMethod, 2, Args);
			for (int I = 1; I < Size; ++I) {
				DataA += StrideA;
				DataB += StrideB;
				Args[1] = GetterA(DataA);
				Args[2] = GetterB(DataB);
				Args[1] = ml_simple_call(MulMethod, 2, Args + 1);
				Args[0] = ml_simple_call(AddMethod, 2, Args);
			}
		}
	}
	*DataC = Args[0];
}

ML_ARRAY_DOT(uint8_t);
ML_ARRAY_DOT(int8_t);
ML_ARRAY_DOT(uint16_t);
ML_ARRAY_DOT(int16_t);
ML_ARRAY_DOT(uint32_t);
ML_ARRAY_DOT(int32_t);
ML_ARRAY_DOT(uint64_t);
ML_ARRAY_DOT(int64_t);
ML_ARRAY_DOT(float);
ML_ARRAY_DOT(double);

#ifdef ML_COMPLEX

ML_ARRAY_DOT(complex_float);
ML_ARRAY_DOT(complex_double);

#endif

typedef void (*ml_array_dot_fn)(
	void *DataA, ml_array_dimension_t *DimA, void *GetterA,
	void *DataB, ml_array_dimension_t *DimB, void *GetterB,
	void *DataC
);

static void ml_array_dot_fill(
	void *DataA, ml_array_dimension_t *DimA, void *GetterA, int DegreeA,
	void *DataB, ml_array_dimension_t *DimB, void *GetterB, int DegreeB,
	void *DataC, ml_array_dimension_t *DimC, ml_array_dot_fn DotFn, int DegreeC
) {
	if (DegreeA > 1) {
		int StrideA = DimA->Stride;
		int StrideC = DimC->Stride;
		if (DimA->Indices) {
			const int *Indices = DimA->Indices;
			for (int I = 0; I < DimA->Size; ++I) {
				ml_array_dot_fill(
					DataA + (Indices[I]) * StrideA, DimA + 1, GetterA, DegreeA - 1,
					DataB, DimB, GetterB, DegreeB,
					DataC, DimC + 1, DotFn, DegreeC - 1
				);
				DataC += StrideC;
			}
		} else {
			for (int I = DimA->Size; --I >= 0;) {
				ml_array_dot_fill(
					DataA, DimA + 1, GetterA, DegreeA - 1,
					DataB, DimB, GetterB, DegreeB,
					DataC, DimC + 1, DotFn, DegreeC - 1
				);
				DataA += StrideA;
				DataC += StrideC;
			}
		}
	} else if (DegreeB > 1) {
		int StrideB = DimB->Stride;
		int StrideC = (DimC + DegreeC - 1)->Stride;
		if (DimB->Indices) {
			const int *Indices = DimB->Indices;
			for (int I = 0; I < DimB->Size; ++I) {
				ml_array_dot_fill(
					DataA, DimA, GetterA, DegreeA,
					DataB + (Indices[I]) * StrideB, DimB - 1, GetterB, DegreeB - 1,
					DataC, DimC, DotFn, DegreeC - 1
				);
				DataC += StrideC;
			}
		} else {
			for (int I = DimB->Size; --I >= 0;) {
				ml_array_dot_fill(
					DataA, DimA, GetterA, DegreeA,
					DataB, DimB - 1, GetterB, DegreeB - 1,
					DataC, DimC, DotFn, DegreeC - 1
				);
				DataB += StrideB;
				DataC += StrideC;
			}
		}
	} else {
		DotFn(DataA, DimA, GetterA, DataB, DimB, GetterB, DataC);
	}
}

#define ML_ARRAY_INFIX_SETTER(NAME, OP, CTYPE) \
static void ml_array_infix_set_ ## NAME ## _ ## CTYPE(void *DataA, ml_array_getter_ ## CTYPE GetterA, void *DataB, ml_array_getter_ ## CTYPE GetterB, void *DataC) { \
	*(CTYPE *)DataC = GetterA(DataA) OP GetterB(DataB); \
}

#define ML_ARRAY_INFIX_SETTERS_BASE(NAME, OP, METHOD) \
ML_ARRAY_INFIX_SETTER(NAME, OP, uint8_t) \
ML_ARRAY_INFIX_SETTER(NAME, OP, int8_t) \
ML_ARRAY_INFIX_SETTER(NAME, OP, uint16_t) \
ML_ARRAY_INFIX_SETTER(NAME, OP, int16_t) \
ML_ARRAY_INFIX_SETTER(NAME, OP, uint32_t) \
ML_ARRAY_INFIX_SETTER(NAME, OP, int32_t) \
ML_ARRAY_INFIX_SETTER(NAME, OP, uint64_t) \
ML_ARRAY_INFIX_SETTER(NAME, OP, int64_t) \
ML_ARRAY_INFIX_SETTER(NAME, OP, float) \
ML_ARRAY_INFIX_SETTER(NAME, OP, double) \
\
static void ml_array_infix_set_ ## NAME ## _any(void *DataA, ml_array_getter_any GetterA, void *DataB, ml_array_getter_any GetterB, void *DataC) { \
	ml_value_t *Args[2] = {GetterA(DataA), GetterB(DataB)}; \
	*(ml_value_t **)DataC = ml_simple_call(METHOD, 2, Args); \
}

#ifdef ML_COMPLEX

#define ML_ARRAY_INFIX_SETTERS(NAME, OP, METHOD) \
ML_ARRAY_INFIX_SETTERS_BASE(NAME, OP, METHOD) \
ML_ARRAY_INFIX_SETTER(NAME, OP, complex_float) \
ML_ARRAY_INFIX_SETTER(NAME, OP, complex_double)

#else

#define ML_ARRAY_INFIX_SETTERS(NAME, OP, METHOD) ML_ARRAY_INFIX_SETTERS_BASE(NAME, OP, METHOD)

#endif

ML_ARRAY_INFIX_SETTERS(add, +, AddMethod)
ML_ARRAY_INFIX_SETTERS(sub, -, SubMethod)
ML_ARRAY_INFIX_SETTERS(mul, *, MulMethod)
ML_ARRAY_INFIX_SETTERS(div, /, DivMethod)

typedef void (*ml_array_infix_set_fn)(void *DataA, void *GetterA, void *DataB, void *GetterB, void *DataC);

static void ml_array_infix_fill(
	void *DataA, ml_array_dimension_t *DimA, void *GetterA, int DegreeA,
	void *DataB, ml_array_dimension_t *DimB, void *GetterB, int DegreeB,
	void *DataC, ml_array_dimension_t *DimC, ml_array_infix_set_fn InfixSetFn, int DegreeC
) {
	if (DegreeA > 0) {
		int StrideA = DimA->Stride;
		int StrideC = DimC->Stride;
		if (DimA->Indices) {
			const int *Indices = DimA->Indices;
			for (int I = 0; I < DimA->Size; ++I) {
				ml_array_infix_fill(
					DataA + (Indices[I]) * StrideA, DimA + 1, GetterA, DegreeA - 1,
					DataB, DimB, GetterB, DegreeB,
					DataC, DimC + 1, InfixSetFn, DegreeC - 1
				);
				DataC += StrideC;
			}
		} else {
			for (int I = DimA->Size; --I >= 0;) {
				ml_array_infix_fill(
					DataA, DimA + 1, GetterA, DegreeA - 1,
					DataB, DimB, GetterB, DegreeB,
					DataC, DimC + 1, InfixSetFn, DegreeC - 1
				);
				DataA += StrideA;
				DataC += StrideC;
			}
		}
	} else if (DegreeB > 0) {
		int StrideB = DimB->Stride;
		int StrideC = (DimC + DegreeC - 1)->Stride;
		if (DimB->Indices) {
			const int *Indices = DimB->Indices;
			for (int I = 0; I < DimB->Size; ++I) {
				ml_array_infix_fill(
					DataA, DimA, GetterA, DegreeA,
					DataB + (Indices[I]) * StrideB, DimB - 1, GetterB, DegreeB - 1,
					DataC, DimC, InfixSetFn, DegreeC - 1
				);
				DataC += StrideC;
			}
		} else {
			for (int I = DimB->Size; --I >= 0;) {
				ml_array_infix_fill(
					DataA, DimA, GetterA, DegreeA,
					DataB, DimB - 1, GetterB, DegreeB - 1,
					DataC, DimC, InfixSetFn, DegreeC - 1
				);
				DataB += StrideB;
				DataC += StrideC;
			}
		}
	} else {
		InfixSetFn(DataA, GetterA, DataB, GetterB, DataC);
	}
}

#ifdef ML_COMPLEX

#define ML_ARRAY_INFIX_FNS(NAME, KIND) \
static ml_array_infix_set_fn MLArrayInfix ## NAME ## Fns[] = { \
	[ML_ARRAY_FORMAT_U8] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _uint8_t, \
	[ML_ARRAY_FORMAT_I8] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _int8_t, \
	[ML_ARRAY_FORMAT_U16] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _uint16_t, \
	[ML_ARRAY_FORMAT_I16] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _int16_t, \
	[ML_ARRAY_FORMAT_U32] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _uint32_t, \
	[ML_ARRAY_FORMAT_I32] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _int32_t, \
	[ML_ARRAY_FORMAT_U64] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _uint64_t, \
	[ML_ARRAY_FORMAT_I64] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _int64_t, \
	[ML_ARRAY_FORMAT_F32] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _float, \
	[ML_ARRAY_FORMAT_F64] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _double, \
	[ML_ARRAY_FORMAT_C32] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _complex_float, \
	[ML_ARRAY_FORMAT_C64] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _complex_double,\
	[ML_ARRAY_FORMAT_ANY] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _any \
};

#else

#define ML_ARRAY_INFIX_FNS(NAME, KIND) \
static ml_array_infix_set_fn MLArrayInfix ## NAME ## Fns[] = { \
	[ML_ARRAY_FORMAT_I8] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _int8_t, \
	[ML_ARRAY_FORMAT_U8] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _uint8_t, \
	[ML_ARRAY_FORMAT_I16] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _int16_t, \
	[ML_ARRAY_FORMAT_U16] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _uint16_t, \
	[ML_ARRAY_FORMAT_I32] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _int32_t, \
	[ML_ARRAY_FORMAT_U32] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _uint32_t, \
	[ML_ARRAY_FORMAT_I64] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _int64_t, \
	[ML_ARRAY_FORMAT_U64] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _uint64_t,\
	[ML_ARRAY_FORMAT_F32] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _float, \
	[ML_ARRAY_FORMAT_F64] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _double, \
	[ML_ARRAY_FORMAT_ANY] = (ml_array_infix_set_fn)ml_array_infix_set_ ## KIND ## _any \
};

#endif

ML_ARRAY_INFIX_FNS(Add, add)
ML_ARRAY_INFIX_FNS(Sub, sub)
ML_ARRAY_INFIX_FNS(Mul, mul)
ML_ARRAY_INFIX_FNS(Div, div)

#define ML_ARRAY_ACCESSOR_IMPL(CTYPE, ATYPE) \
static CTYPE ml_array_getter_ ## CTYPE ## _ ## ATYPE(void *Data) { \
	return (CTYPE)(*(ATYPE *)Data); \
} \
\
static void ml_array_setter_ ## CTYPE ## _ ## ATYPE(void *Data, CTYPE Value) { \
	*(ATYPE *)Data = Value; \
}

#define ML_ARRAY_ACCESSORS_IMPL_BASE(CTYPE, FROM_VAL, TO_VAL) \
\
ML_ARRAY_ACCESSOR_IMPL(CTYPE, uint8_t) \
ML_ARRAY_ACCESSOR_IMPL(CTYPE, int8_t) \
ML_ARRAY_ACCESSOR_IMPL(CTYPE, uint16_t) \
ML_ARRAY_ACCESSOR_IMPL(CTYPE, int16_t) \
ML_ARRAY_ACCESSOR_IMPL(CTYPE, uint32_t) \
ML_ARRAY_ACCESSOR_IMPL(CTYPE, int32_t) \
ML_ARRAY_ACCESSOR_IMPL(CTYPE, uint64_t) \
ML_ARRAY_ACCESSOR_IMPL(CTYPE, int64_t) \
ML_ARRAY_ACCESSOR_IMPL(CTYPE, float) \
ML_ARRAY_ACCESSOR_IMPL(CTYPE, double) \
\
static CTYPE ml_array_getter_ ## CTYPE ## _any(void *Data) { \
	return FROM_VAL(*(ml_value_t **)Data); \
} \
\
static void ml_array_setter_ ## CTYPE ## _any(void *Data, CTYPE Value) { \
	*(any *)Data = TO_VAL(Value); \
}

#ifdef ML_COMPLEX

#define ML_ARRAY_ACCESSORS_IMPL(CTYPE, FROM_VAL, TO_VAL) \
ML_ARRAY_ACCESSORS_IMPL_BASE(CTYPE, FROM_VAL, TO_VAL) \
ML_ARRAY_ACCESSOR_IMPL(CTYPE, complex_float) \
ML_ARRAY_ACCESSOR_IMPL(CTYPE, complex_double)

#else

#define ML_ARRAY_ACCESSORS_IMPL(CTYPE, FROM_VAL, TO_VAL) ML_ARRAY_ACCESSORS_IMPL_BASE(CTYPE, FROM_VAL, TO_VAL)

#endif

ML_ARRAY_ACCESSORS_IMPL(uint8_t, ml_integer_value, ml_integer)
ML_ARRAY_ACCESSORS_IMPL(int8_t, ml_integer_value, ml_integer)
ML_ARRAY_ACCESSORS_IMPL(uint16_t, ml_integer_value, ml_integer)
ML_ARRAY_ACCESSORS_IMPL(int16_t, ml_integer_value, ml_integer)
ML_ARRAY_ACCESSORS_IMPL(uint32_t, ml_integer_value, ml_integer)
ML_ARRAY_ACCESSORS_IMPL(int32_t, ml_integer_value, ml_integer)
ML_ARRAY_ACCESSORS_IMPL(uint64_t, ml_integer_value, ml_integer)
ML_ARRAY_ACCESSORS_IMPL(int64_t, ml_integer_value, ml_integer)
ML_ARRAY_ACCESSORS_IMPL(float, ml_real_value, ml_real)
ML_ARRAY_ACCESSORS_IMPL(double, ml_real_value, ml_real)

#ifdef ML_COMPLEX

ML_ARRAY_ACCESSORS_IMPL(complex_float, ml_complex_value, ml_complex)
ML_ARRAY_ACCESSORS_IMPL(complex_double, ml_complex_value, ml_complex)

#endif

#define ML_ARRAY_ACCESSOR_ANY_IMPL(TO_VAL, FROM_VAL, ATYPE) \
static any ml_array_getter_any_ ## ATYPE(void *Data) { \
	return TO_VAL(*(ATYPE *)Data); \
} \
\
static void ml_array_setter_any_ ## ATYPE(void *Data, any Value) { \
	*(ATYPE *)Data = FROM_VAL(Value); \
}

typedef any (*ml_array_getter_any)(void *Data);

ML_ARRAY_ACCESSOR_ANY_IMPL(ml_integer, ml_integer_value, uint8_t)
ML_ARRAY_ACCESSOR_ANY_IMPL(ml_integer, ml_integer_value, int8_t)
ML_ARRAY_ACCESSOR_ANY_IMPL(ml_integer, ml_integer_value, uint16_t)
ML_ARRAY_ACCESSOR_ANY_IMPL(ml_integer, ml_integer_value, int16_t)
ML_ARRAY_ACCESSOR_ANY_IMPL(ml_integer, ml_integer_value, uint32_t)
ML_ARRAY_ACCESSOR_ANY_IMPL(ml_integer, ml_integer_value, int32_t)
ML_ARRAY_ACCESSOR_ANY_IMPL(ml_integer, ml_integer_value, uint64_t)
ML_ARRAY_ACCESSOR_ANY_IMPL(ml_integer, ml_integer_value, int64_t)
ML_ARRAY_ACCESSOR_ANY_IMPL(ml_real, ml_real_value, float)
ML_ARRAY_ACCESSOR_ANY_IMPL(ml_real, ml_real_value, double)

#ifdef ML_COMPLEX

ML_ARRAY_ACCESSOR_ANY_IMPL(ml_complex, ml_complex_value, complex_float)
ML_ARRAY_ACCESSOR_ANY_IMPL(ml_complex, ml_complex_value, complex_double)

#endif

static any ml_array_getter_any_any(void *Data) {
	return *(any *)Data;
}

static void ml_array_setter_any_any(void *Data, any Value) {
	*(any *)Data = Value;
}

#define ML_ARRAY_ACCESSOR_FNS(NAME, CTYPE) \
ml_array_getter_ ## CTYPE ml_array_ ## CTYPE ## _getter(ml_array_format_t Format) { \
	return MLArrayGetters ## NAME[Format]; \
} \
\
ml_array_setter_ ## CTYPE ml_array_ ## CTYPE ## _setter(ml_array_format_t Format) { \
	return MLArraySetters ## NAME[Format]; \
}

#ifdef ML_COMPLEX

#define ML_ARRAY_ACCESSOR_DECLS(NAME, CTYPE) \
static ml_array_getter_ ## CTYPE MLArrayGetters ## NAME[] = { \
	[ML_ARRAY_FORMAT_U8] = ml_array_getter_ ## CTYPE ## _uint8_t, \
	[ML_ARRAY_FORMAT_I8] = ml_array_getter_ ## CTYPE ## _int8_t, \
	[ML_ARRAY_FORMAT_U16] = ml_array_getter_ ## CTYPE ## _uint16_t, \
	[ML_ARRAY_FORMAT_I16] = ml_array_getter_ ## CTYPE ## _int16_t, \
	[ML_ARRAY_FORMAT_U32] = ml_array_getter_ ## CTYPE ## _uint32_t, \
	[ML_ARRAY_FORMAT_I32] = ml_array_getter_ ## CTYPE ## _int32_t, \
	[ML_ARRAY_FORMAT_U64] = ml_array_getter_ ## CTYPE ## _uint64_t, \
	[ML_ARRAY_FORMAT_I64] = ml_array_getter_ ## CTYPE ## _int64_t, \
	[ML_ARRAY_FORMAT_F32] = ml_array_getter_ ## CTYPE ## _float, \
	[ML_ARRAY_FORMAT_F64] = ml_array_getter_ ## CTYPE ## _double, \
	[ML_ARRAY_FORMAT_C32] = ml_array_getter_ ## CTYPE ## _complex_float, \
	[ML_ARRAY_FORMAT_C64] = ml_array_getter_ ## CTYPE ## _complex_double, \
	[ML_ARRAY_FORMAT_ANY] = ml_array_getter_ ## CTYPE ## _any, \
}; \
\
static ml_array_setter_ ## CTYPE MLArraySetters ## NAME[] = { \
	[ML_ARRAY_FORMAT_U8] = ml_array_setter_ ## CTYPE ## _uint8_t, \
	[ML_ARRAY_FORMAT_I8] = ml_array_setter_ ## CTYPE ## _int8_t, \
	[ML_ARRAY_FORMAT_U16] = ml_array_setter_ ## CTYPE ## _uint16_t, \
	[ML_ARRAY_FORMAT_I16] = ml_array_setter_ ## CTYPE ## _int16_t, \
	[ML_ARRAY_FORMAT_U32] = ml_array_setter_ ## CTYPE ## _uint32_t, \
	[ML_ARRAY_FORMAT_I32] = ml_array_setter_ ## CTYPE ## _int32_t, \
	[ML_ARRAY_FORMAT_U64] = ml_array_setter_ ## CTYPE ## _uint64_t, \
	[ML_ARRAY_FORMAT_I64] = ml_array_setter_ ## CTYPE ## _int64_t, \
	[ML_ARRAY_FORMAT_F32] = ml_array_setter_ ## CTYPE ## _float, \
	[ML_ARRAY_FORMAT_F64] = ml_array_setter_ ## CTYPE ## _double, \
	[ML_ARRAY_FORMAT_C32] = ml_array_setter_ ## CTYPE ## _complex_float, \
	[ML_ARRAY_FORMAT_C64] = ml_array_setter_ ## CTYPE ## _complex_double, \
	[ML_ARRAY_FORMAT_ANY] = ml_array_setter_ ## CTYPE ## _any, \
}; \
\
ML_ARRAY_ACCESSOR_FNS(NAME, CTYPE)

#else

#define ML_ARRAY_ACCESSOR_DECLS(NAME, CTYPE) \
static ml_array_getter_ ## CTYPE MLArrayGetters ## NAME[] = { \
	[ML_ARRAY_FORMAT_I8] = ml_array_getter_ ## CTYPE ## _int8_t, \
	[ML_ARRAY_FORMAT_U8] = ml_array_getter_ ## CTYPE ## _uint8_t, \
	[ML_ARRAY_FORMAT_I16] = ml_array_getter_ ## CTYPE ## _int16_t, \
	[ML_ARRAY_FORMAT_U16] = ml_array_getter_ ## CTYPE ## _uint16_t, \
	[ML_ARRAY_FORMAT_I32] = ml_array_getter_ ## CTYPE ## _int32_t, \
	[ML_ARRAY_FORMAT_U32] = ml_array_getter_ ## CTYPE ## _uint32_t, \
	[ML_ARRAY_FORMAT_I64] = ml_array_getter_ ## CTYPE ## _int64_t, \
	[ML_ARRAY_FORMAT_U64] = ml_array_getter_ ## CTYPE ## _uint64_t, \
	[ML_ARRAY_FORMAT_F32] = ml_array_getter_ ## CTYPE ## _float, \
	[ML_ARRAY_FORMAT_F64] = ml_array_getter_ ## CTYPE ## _double, \
	[ML_ARRAY_FORMAT_ANY] = ml_array_getter_ ## CTYPE ## _any, \
}; \
\
static ml_array_setter_ ## CTYPE MLArraySetters ## NAME[] = { \
	[ML_ARRAY_FORMAT_I8] = ml_array_setter_ ## CTYPE ## _int8_t, \
	[ML_ARRAY_FORMAT_U8] = ml_array_setter_ ## CTYPE ## _uint8_t, \
	[ML_ARRAY_FORMAT_I16] = ml_array_setter_ ## CTYPE ## _int16_t, \
	[ML_ARRAY_FORMAT_U16] = ml_array_setter_ ## CTYPE ## _uint16_t, \
	[ML_ARRAY_FORMAT_I32] = ml_array_setter_ ## CTYPE ## _int32_t, \
	[ML_ARRAY_FORMAT_U32] = ml_array_setter_ ## CTYPE ## _uint32_t, \
	[ML_ARRAY_FORMAT_I64] = ml_array_setter_ ## CTYPE ## _int64_t, \
	[ML_ARRAY_FORMAT_U64] = ml_array_setter_ ## CTYPE ## _uint64_t, \
	[ML_ARRAY_FORMAT_F32] = ml_array_setter_ ## CTYPE ## _float, \
	[ML_ARRAY_FORMAT_F64] = ml_array_setter_ ## CTYPE ## _double, \
	[ML_ARRAY_FORMAT_ANY] = ml_array_setter_ ## CTYPE ## _any, \
}; \
\
ML_ARRAY_ACCESSOR_FNS(NAME, CTYPE)

#endif

ML_ARRAY_ACCESSOR_DECLS(UInt8, uint8_t);
ML_ARRAY_ACCESSOR_DECLS(Int8, int8_t);
ML_ARRAY_ACCESSOR_DECLS(UInt16, uint16_t);
ML_ARRAY_ACCESSOR_DECLS(Int16, int16_t);
ML_ARRAY_ACCESSOR_DECLS(UInt32, uint32_t);
ML_ARRAY_ACCESSOR_DECLS(Int32, int32_t);
ML_ARRAY_ACCESSOR_DECLS(UInt64, uint64_t);
ML_ARRAY_ACCESSOR_DECLS(Int64, int64_t);
ML_ARRAY_ACCESSOR_DECLS(Float32, float);
ML_ARRAY_ACCESSOR_DECLS(Float64, double);

#ifdef ML_COMPLEX

ML_ARRAY_ACCESSOR_DECLS(Complex32, complex_float);
ML_ARRAY_ACCESSOR_DECLS(Complex64, complex_double);

#endif

typedef any (*ml_array_getter_any)(void *);
typedef void (*ml_array_setter_any)(void *, any);

ML_ARRAY_ACCESSOR_DECLS(Any, any);

static void **MLArrayGetters[] = {
	[ML_ARRAY_FORMAT_U8] = (void *)MLArrayGettersUInt8,
	[ML_ARRAY_FORMAT_I8] = (void *)MLArrayGettersInt8,
	[ML_ARRAY_FORMAT_U16] = (void *)MLArrayGettersUInt16,
	[ML_ARRAY_FORMAT_I16] = (void *)MLArrayGettersInt16,
	[ML_ARRAY_FORMAT_U32] = (void *)MLArrayGettersUInt32,
	[ML_ARRAY_FORMAT_I32] = (void *)MLArrayGettersInt32,
	[ML_ARRAY_FORMAT_U64] = (void *)MLArrayGettersUInt64,
	[ML_ARRAY_FORMAT_I64] = (void *)MLArrayGettersInt64,
	[ML_ARRAY_FORMAT_F32] = (void *)MLArrayGettersFloat32,
	[ML_ARRAY_FORMAT_F64] = (void *)MLArrayGettersFloat64,
#ifdef ML_COMPLEX
	[ML_ARRAY_FORMAT_C32] = (void *)MLArrayGettersComplex32,
	[ML_ARRAY_FORMAT_C64] = (void *)MLArrayGettersComplex64,
#endif
	[ML_ARRAY_FORMAT_ANY] = (void *)MLArrayGettersAny
};

static ml_value_t *ml_array_pairwise_infix(ml_array_infix_set_fn *InfixSetFns, int Count, ml_value_t **Args) {
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Degree == -1) return (ml_value_t *)A;
	ml_array_t *B = (ml_array_t *)Args[1];
	if (B->Degree == -1) return (ml_value_t *)B;
	int DegreeA = A->Degree;
	int DegreeB = B->Degree;
	if (!DegreeA || !DegreeB) return ml_error("ShapeError", "Empty array");
	ml_array_dimension_t *DimA = A->Dimensions;
	ml_array_dimension_t *DimB = B->Dimensions;
	int Degree = DegreeA + DegreeB;
	ml_array_format_t Format = MAX(A->Format, B->Format);
	if (InfixSetFns == MLArrayInfixDivFns) Format = MAX(Format, ML_ARRAY_FORMAT_F64);
	ml_array_t *C = ml_array_alloc(Format, Degree);
	int DataSize = MLArraySizes[C->Format];
	int Base = DegreeA;
	for (int I = DegreeB; --I >= 0;) {
		C->Dimensions[Base + I].Stride = DataSize;
		int Size = C->Dimensions[Base + I].Size = DimB[I].Size;
		DataSize *= Size;
	}
	for (int I = DegreeA; --I >= 0;) {
		C->Dimensions[I].Stride = DataSize;
		int Size = C->Dimensions[I].Size = DimA[I].Size;
		DataSize *= Size;
	}
	C->Base.Value = array_alloc(Format, DataSize);
	C->Base.Length = DataSize;

	void *GetterA = MLArrayGetters[C->Format][A->Format];
	void *GetterB = MLArrayGetters[C->Format][B->Format];
	ml_array_infix_set_fn InfixSetFn = InfixSetFns[C->Format];
	ml_array_infix_fill(
		A->Base.Value, DimA, GetterA, DegreeA,
		B->Base.Value, DimB + (DegreeB - 1), GetterB, DegreeB,
		C->Base.Value, C->Dimensions, InfixSetFn, C->Degree
	);
	return (ml_value_t *)C;
}

#define ML_ARRAY_PAIRWISE(NAME, OP) \
/*
ML_METHOD(NAME, MLArrayT, MLArrayT) {
//<A
//<B
//>array
// Returns an array with :mini:`A/i OP B/j` for each pair of elements of :mini:`A` and :mini:`B`. The result will have shape :mini:`A:shape + B:shape`.
//
//$= let A := array([1, 8, 3])
//$= let B := array([[7, 2], [4, 11]])
//$= A:shape
//$= B:shape
//$= let C := A OPOP B
//$= C:shape
}
*/

ML_ARRAY_PAIRWISE("++", +)
ML_ARRAY_PAIRWISE("--", -)
ML_ARRAY_PAIRWISE("**", *)
ML_ARRAY_PAIRWISE("//", /)

static ml_array_dot_fn MLArrayDotFns[] = {
	[ML_ARRAY_FORMAT_U8] = (ml_array_dot_fn)ml_array_dot_uint8_t,
	[ML_ARRAY_FORMAT_I8] = (ml_array_dot_fn)ml_array_dot_int8_t,
	[ML_ARRAY_FORMAT_U16] = (ml_array_dot_fn)ml_array_dot_uint16_t,
	[ML_ARRAY_FORMAT_I16] = (ml_array_dot_fn)ml_array_dot_int16_t,
	[ML_ARRAY_FORMAT_U32] = (ml_array_dot_fn)ml_array_dot_uint32_t,
	[ML_ARRAY_FORMAT_I32] = (ml_array_dot_fn)ml_array_dot_int32_t,
	[ML_ARRAY_FORMAT_U64] = (ml_array_dot_fn)ml_array_dot_uint64_t,
	[ML_ARRAY_FORMAT_I64] = (ml_array_dot_fn)ml_array_dot_int64_t,
	[ML_ARRAY_FORMAT_F32] = (ml_array_dot_fn)ml_array_dot_float,
	[ML_ARRAY_FORMAT_F64] = (ml_array_dot_fn)ml_array_dot_double,
#ifdef ML_COMPLEX
	[ML_ARRAY_FORMAT_C32] = (ml_array_dot_fn)ml_array_dot_complex_float,
	[ML_ARRAY_FORMAT_C64] = (ml_array_dot_fn)ml_array_dot_complex_double,
#endif
	[ML_ARRAY_FORMAT_ANY] = (ml_array_dot_fn)ml_array_dot_any
};

ML_METHOD(".", MLArrayT, MLArrayT) {
//<A
//<B
//>array
// Returns the inner product of :mini:`A` and :mini:`B`. The last dimension of :mini:`A` and the first dimension of :mini:`B` must match, skipping any dimensions of size :mini:`1`.
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Degree == -1) return (ml_value_t *)A;
	ml_array_t *B = (ml_array_t *)Args[1];
	if (B->Degree == -1) return (ml_value_t *)B;
	int DegreeA = A->Degree;
	int DegreeB = B->Degree;
	if (!DegreeA || !DegreeB) return ml_error("ShapeError", "Empty array");
	ml_array_dimension_t *DimA = A->Dimensions;
	ml_array_dimension_t *DimB = B->Dimensions;
	int SizeA = DimA[DegreeA - 1].Size;
	int SizeB = DimB[0].Size;
	int Degree = DegreeA + DegreeB - 2;
	ml_array_format_t Format = MAX(A->Format, B->Format);
	if (!Degree) {
		if (SizeA != SizeB) return ml_error("ShapeError", "Incompatible arrays");
		if (Format <= ML_ARRAY_FORMAT_F64) {
			void *GetterA = MLArrayGetters[ML_ARRAY_FORMAT_F64][A->Format];
			void *GetterB = MLArrayGetters[ML_ARRAY_FORMAT_F64][B->Format];
			ml_array_dot_fn DotFn = MLArrayDotFns[ML_ARRAY_FORMAT_F64];
			double Dot;
			DotFn(
				A->Base.Value, DimA, GetterA,
				B->Base.Value, DimB + (DegreeB - 1), GetterB,
				&Dot
			);
			return ml_real(Dot);
#ifdef ML_COMPLEX
		} else if (Format <= ML_ARRAY_FORMAT_C64) {
			void *GetterA = MLArrayGetters[ML_ARRAY_FORMAT_C64][A->Format];
			void *GetterB = MLArrayGetters[ML_ARRAY_FORMAT_C64][B->Format];
			ml_array_dot_fn DotFn = MLArrayDotFns[ML_ARRAY_FORMAT_C64];
			complex_double Dot;
			DotFn(
				A->Base.Value, DimA, GetterA,
				B->Base.Value, DimB + (DegreeB - 1), GetterB,
				&Dot
			);
			return ml_complex(Dot);
#endif
		} else {
			void *GetterA = MLArrayGetters[ML_ARRAY_FORMAT_ANY][A->Format];
			void *GetterB = MLArrayGetters[ML_ARRAY_FORMAT_ANY][B->Format];
			ml_array_dot_fn DotFn = MLArrayDotFns[ML_ARRAY_FORMAT_ANY];
			ml_value_t *Dot;
			DotFn(
				A->Base.Value, DimA, GetterA,
				B->Base.Value, DimB + (DegreeB - 1), GetterB,
				&Dot
			);
			return Dot;
		}
	}
	int UseProd = 0;
	if (SizeA == 1) {
		UseProd = 1;
		DegreeA -= 1;
		if (SizeB == 1) {
			DegreeB -= 1;
			DimB += 1;
		} else {
			Degree += 1;
		}
	} else if (SizeB == 1) {
		UseProd = 1;
		DegreeB -= 1;
		DimB += 1;
		Degree += 1;
	} else if (SizeA != SizeB) {
		return ml_error("ShapeError", "Incompatible arrays");
	}
	ml_array_t *C = ml_array_alloc(Format, Degree);
	int DataSize = MLArraySizes[C->Format];
	if (UseProd) {
		int Base = DegreeA;
		for (int I = DegreeB; --I >= 0;) {
			C->Dimensions[Base + I].Stride = DataSize;
			int Size = C->Dimensions[Base + I].Size = DimB[I].Size;
			DataSize *= Size;
		}
		for (int I = DegreeA; --I >= 0;) {
			C->Dimensions[I].Stride = DataSize;
			int Size = C->Dimensions[I].Size = DimA[I].Size;
			DataSize *= Size;
		}
	} else {
		int Base = DegreeA - 2;
		for (int I = DegreeB; --I >= 1;) {
			C->Dimensions[Base + I].Stride = DataSize;
			int Size = C->Dimensions[Base + I].Size = DimB[I].Size;
			DataSize *= Size;
		}
		for (int I = DegreeA - 1; --I >= 0;) {
			C->Dimensions[I].Stride = DataSize;
			int Size = C->Dimensions[I].Size = DimA[I].Size;
			DataSize *= Size;
		}
	}
	C->Base.Value = array_alloc(Format, DataSize);
	C->Base.Length = DataSize;
	if (UseProd) {
		void *GetterA = MLArrayGetters[C->Format][A->Format];
		void *GetterB = MLArrayGetters[C->Format][B->Format];
		ml_array_infix_set_fn InfixSetFn = MLArrayInfixMulFns[C->Format];
		ml_array_infix_fill(
			A->Base.Value, DimA, GetterA, DegreeA,
			B->Base.Value, DimB + (DegreeB - 1), GetterB, DegreeB,
			C->Base.Value, C->Dimensions, InfixSetFn, C->Degree
		);
	} else {
		void *GetterA = MLArrayGetters[C->Format][A->Format];
		void *GetterB = MLArrayGetters[C->Format][B->Format];
		ml_array_dot_fn DotFn = MLArrayDotFns[C->Format];
		ml_array_dot_fill(
			A->Base.Value, DimA, GetterA, DegreeA,
			B->Base.Value, DimB + (DegreeB - 1), GetterB, DegreeB,
			C->Base.Value, C->Dimensions, DotFn, C->Degree
		);
	}
	return (ml_value_t *)C;
}

ML_METHOD("@", MLMatrixT, MLVectorT) {
//<T
//<X
//>vector
// Returns :mini:`X` transformed by :mini:`T`. :mini:`T` must be a :mini:`N` |times| :mini:`N` matrix and :mini:`X` a vector of size :mini:`N - 1`.
	ml_array_t *A = (ml_array_t *)Args[0];
	ml_array_t *B = (ml_array_t *)Args[1];
	int N = A->Dimensions[0].Size;
	if (N != A->Dimensions[1].Size) return ml_error("ShapeError", "Square matrix required");
	if (N != B->Dimensions->Size + 1) return ml_error("ShapeError", "Invalid vector size for transformation");
	ml_array_format_t Format = MAX(A->Format, B->Format);
	if (Format <= ML_ARRAY_FORMAT_F64) {
		double Projection[N], *Result = anew(double, N);
		ml_array_getter_double GetterB = MLArrayGetters[ML_ARRAY_FORMAT_F64][B->Format];
		char *BData = B->Base.Value;
		int Stride = B->Dimensions->Stride;
		const int *Indices = B->Dimensions->Indices;
		if (Indices) {
			for (int I = 0; I < N - 1; ++I) Projection[I] = GetterB(BData + Stride * Indices[I]);
		} else {
			for (int I = 0; I < N - 1; ++I) Projection[I] = GetterB(BData + Stride * I);
		}
		Projection[N - 1] = 1;
		void *GetterA = MLArrayGetters[ML_ARRAY_FORMAT_F64][A->Format];
		ml_array_dot_fn DotFn = MLArrayDotFns[ML_ARRAY_FORMAT_F64];
		ml_array_dimension_t Dimension = {N, sizeof(double), NULL};
		ml_array_dot_fill(
			A->Base.Value, A->Dimensions, GetterA, 2,
			Projection, &Dimension, ml_array_getter_double_double, 1,
			Result, &Dimension, DotFn, 1
		);
		double Scale = Result[N - 1];
		for (int I = 0; I < N - 1; ++I) Result[I] /= Scale;
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_F64, 1);
		C->Base.Value = (char *)Result;
		C->Dimensions->Size = N - 1;
		C->Dimensions->Stride = sizeof(double);
		return (ml_value_t *)C;
#ifdef ML_COMPLEX
	} else if (Format <= ML_ARRAY_FORMAT_C64) {
		complex double Projection[N], *Result = anew(complex double, N);
		ml_array_getter_complex_double GetterB = MLArrayGetters[ML_ARRAY_FORMAT_C64][B->Format];
		char *BData = B->Base.Value;
		int Stride = B->Dimensions->Stride;
		const int *Indices = B->Dimensions->Indices;
		if (Indices) {
			for (int I = 0; I < N - 1; ++I) Projection[I] = GetterB(BData + Stride * Indices[I]);
		} else {
			for (int I = 0; I < N - 1; ++I) Projection[I] = GetterB(BData + Stride * I);
		}
		Projection[N - 1] = 1;
		void *GetterA = MLArrayGetters[ML_ARRAY_FORMAT_C64][A->Format];
		ml_array_dot_fn DotFn = MLArrayDotFns[ML_ARRAY_FORMAT_C64];
		ml_array_dimension_t Dimension = {N, sizeof(complex double), NULL};
		ml_array_dot_fill(
			A->Base.Value, A->Dimensions, GetterA, 2,
			Projection, &Dimension, ml_array_getter_complex_double_complex_double, 1,
			Result, &Dimension, DotFn, 1
		);
		complex double Scale = Result[N - 1];
		for (int I = 0; I < N - 1; ++I) Result[I] /= Scale;
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_C64, 1);
		C->Base.Value = (char *)Result;
		C->Dimensions->Size = N - 1;
		C->Dimensions->Stride = sizeof(complex double);
		return (ml_value_t *)C;
#endif
	} else {
		ml_value_t *Projection[N], **Result = anew(ml_value_t *, N);
		ml_array_getter_any GetterB = MLArrayGetters[ML_ARRAY_FORMAT_ANY][B->Format];
		char *BData = B->Base.Value;
		int Stride = B->Dimensions->Stride;
		const int *Indices = B->Dimensions->Indices;
		if (Indices) {
			for (int I = 0; I < N - 1; ++I) Projection[I] = GetterB(BData + Stride * Indices[I]);
		} else {
			for (int I = 0; I < N - 1; ++I) Projection[I] = GetterB(BData + Stride * I);
		}
		Projection[N - 1] = ml_real(1);
		void *GetterA = MLArrayGetters[ML_ARRAY_FORMAT_ANY][A->Format];
		ml_array_dot_fn DotFn = MLArrayDotFns[ML_ARRAY_FORMAT_ANY];
		ml_array_dimension_t Dimension = {N, sizeof(ml_value_t *), NULL};
		ml_array_dot_fill(
			A->Base.Value, A->Dimensions, GetterA, 2,
			Projection, &Dimension, ml_array_getter_any_any, 1,
			Result, &Dimension, DotFn, 1
		);
		ml_value_t *Scale = Result[N - 1];
		for (int I = 0; I < N - 1; ++I) {
			ml_value_t *Args2[2] = {Result[I], Scale};
			Result[I] = ml_simple_call(DivMethod, 2, Args2);
		}
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_ANY, 1);
		C->Base.Value = (char *)Result;
		C->Dimensions->Size = N - 1;
		C->Dimensions->Stride = sizeof(ml_value_t *);
		return (ml_value_t *)C;
	}
}

static int ml_lu_decomp_real(double **A, int *P, int N) {
	for (int I = 0; I <= N; ++I) P[I] = I;
	for (int I = 0; I < N; ++I) {
		double MaxA = 0;
		int IMax = I;
		for (int K = I; K < N; ++K) {
			double AbsA = fabs(A[K][I]);
			if (AbsA > MaxA) {
				MaxA = AbsA;
				IMax = K;
			}
		}
		if (MaxA < DBL_EPSILON) return 0;
		if (IMax != I) {
			int J = P[I];
			P[I] = P[IMax];
			P[IMax] = J;
			double *B = A[I];
			A[I] = A[IMax];
			A[IMax] = B;
			P[N]++;
		}
		for (int J = I + 1; J < N; ++J) {
			A[J][I] /= A[I][I];
			for (int K = I + 1; K < N; ++K) {
				A[J][K] -= A[J][I] * A[I][K];
			}
		}
	}
	return 1;
}

#ifdef ML_COMPLEX

static int ml_lu_decomp_complex(complex double **A, int *P, int N) {
	for (int I = 0; I <= N; ++I) P[I] = I;
	for (int I = 0; I < N; ++I) {
		double MaxA = 0;
		int IMax = I;
		for (int K = I; K < N; ++K) {
			double AbsA = cabs(A[K][I]);
			if (AbsA > MaxA) {
				MaxA = AbsA;
				IMax = K;
			}
		}
		if (MaxA < DBL_EPSILON) return 0;
		if (IMax != I) {
			int J = P[I];
			P[I] = P[IMax];
			P[IMax] = J;
			complex double *B = A[I];
			A[I] = A[IMax];
			A[IMax] = B;
			P[N]++;
		}
		for (int J = I + 1; J < N; ++J) {
			A[J][I] /= A[I][I];
			for (int K = I + 1; K < N; ++K) {
				A[J][K] -= A[J][I] * A[I][K];
			}
		}
	}
	return 1;
}

#endif

ML_METHOD("lu", MLMatrixT) {
//<A
//>tuple[matrix,matrix,matrix]
// Returns a tuple of matrices :mini:`(L, U, P)` such that :mini:`L` is lower triangular, :mini:`U` is upper triangular, :mini:`P` is a permutation matrix and :mini:`P . A = L . U`.
//$= let A := $[[0, 5, 22/3], [4, 2, 1], [2, 7, 9]]
//$= let (L, U, P) := A:lu
//$= \P . L . U
	ml_array_t *Source = (ml_array_t *)Args[0];
	int N = Source->Dimensions[0].Size;
	if (N != Source->Dimensions[1].Size) return ml_error("ShapeError", "Square matrix required");
	if (Source->Format <= ML_ARRAY_FORMAT_F64) {
		ml_array_t *LU = ml_array_alloc(ML_ARRAY_FORMAT_F64, 2);
		ml_array_copy(LU, Source);
		int Stride = LU->Dimensions->Stride;
		double *A[N];
		char *Data = LU->Base.Value;
		for (int I = 0; I < N; ++I) {
			A[I] = (double *)Data;
			Data += Stride;
		}
		int *P0 = anew(int, N + 1);
		if (!ml_lu_decomp_real(A, P0, N)) return ml_error("ArrayError", "Matrix is degenerate");
		ml_array_t *L = ml_array(ML_ARRAY_FORMAT_F64, 2, N, N);
		ml_array_t *U = ml_array(ML_ARRAY_FORMAT_F64, 2, N, N);
		ml_array_t *P = ml_array(ML_ARRAY_FORMAT_I8, 2, N, N);
		double *LData = (double *)L->Base.Value;
		double *UData = (double *)U->Base.Value;
		int8_t *PData = (int8_t *)P->Base.Value;
		memset(LData, 0, N * N * sizeof(double));
		memset(UData, 0, N * N * sizeof(double));
		memset(PData, 0, N * N * sizeof(int8_t));
		for (int I = 0; I < N; ++I) {
			memcpy(LData, A[I], I * sizeof(double));
			LData[I] = 1;
			LData += N;
			memcpy(UData + I, A[I] + I, (N - I) * sizeof(double));
			UData += N;
			PData[P0[I]] = 1;
			PData += N;
		}
		return ml_tuplev(3, L, U, P);
#ifdef ML_COMPLEX
	} else if (Source->Format <= ML_ARRAY_FORMAT_C64) {
		ml_array_t *LU = ml_array_alloc(ML_ARRAY_FORMAT_C64, 2);
		ml_array_copy(LU, Source);
		int Stride = LU->Dimensions->Stride;
		complex double *A[N];
		char *Data = LU->Base.Value;
		for (int I = 0; I < N; ++I) {
			A[I] = (complex double *)Data;
			Data += Stride;
		}
		int *P0 = anew(int, N + 1);
		if (!ml_lu_decomp_complex(A, P0, N)) return ml_error("ArrayError", "Matrix is degenerate");
		ml_array_t *L = ml_array(ML_ARRAY_FORMAT_C64, 2, N, N);
		ml_array_t *U = ml_array(ML_ARRAY_FORMAT_C64, 2, N, N);
		ml_array_t *P = ml_array(ML_ARRAY_FORMAT_I8, 2, N, N);
		complex double *LData = (complex double *)L->Base.Value;
		complex double *UData = (complex double *)U->Base.Value;
		int8_t *PData = (int8_t *)P->Base.Value;
		memset(LData, 0, N * N * sizeof(complex double));
		memset(UData, 0, N * N * sizeof(complex double));
		memset(PData, 0, N * N * sizeof(int8_t));
		for (int I = 0; I < N; ++I) {
			memcpy(LData, A[I], I * sizeof(complex double));
			LData[I] = 1;
			LData += N;
			memcpy(UData + I, A[I] + I, (N - I) * sizeof(complex double));
			UData += N;
			PData[P0[I]] = 1;
			PData += N;
		}
		return ml_tuplev(3, L, U, P);
#endif
	} else {
		return ml_error("ArrayError", "Invalid array type for operation");
	}
}

ML_METHOD("\\", MLMatrixT) {
//<A
//>matrix
// Returns the inverse of :mini:`A`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int N = Source->Dimensions[0].Size;
	if (N != Source->Dimensions[1].Size) return ml_error("ShapeError", "Square matrix required");
	if (Source->Format <= ML_ARRAY_FORMAT_F64) {
		ml_array_t *Inv = ml_array_alloc(ML_ARRAY_FORMAT_F64, 2);
		ml_array_copy(Inv, Source);
		int Stride = Inv->Dimensions->Stride;
		double *A[N];
		char *Data = Inv->Base.Value;
		for (int I = 0; I < N; ++I) {
			A[I] = (double *)Data;
			Data += Stride;
		}
		int *P = anew(int, N + 1);
		if (!ml_lu_decomp_real(A, P, N)) return ml_error("ArrayError", "Matrix is degenerate");
		double *IA[N];
		double *InvData = IA[0] = anew(double, N * N);
		for (int I = 1; I < N; ++I) IA[I] = IA[I - 1] + N;
		for (int J = 0; J < N; ++J) {
			for (int I = 0; I < N; ++I) {
				IA[I][J] = P[I] == J;
				for (int K = 0; K < I; ++K) {
					IA[I][J] -= A[I][K] * IA[K][J];
				}
			}
			for (int I = N - 1; I >= 0; --I) {
				for (int K = I + 1; K < N; ++K) {
					IA[I][J] -= A[I][K] * IA[K][J];
				}
				IA[I][J] /= A[I][I];
			}
		}
		Inv->Base.Value = (char *)InvData;
		return (ml_value_t *)Inv;
#ifdef ML_COMPLEX
	} else if (Source->Format <= ML_ARRAY_FORMAT_C64) {
		ml_array_t *Inv = ml_array_alloc(ML_ARRAY_FORMAT_C64, 2);
		ml_array_copy(Inv, Source);
		int N = Inv->Dimensions->Size;
		int Stride = Inv->Dimensions->Stride;
		complex double *A[N];
		char *Data = Inv->Base.Value;
		for (int I = 0; I < N; ++I) {
			A[I] = (complex double *)Data;
			Data += Stride;
		}
		int *P = anew(int, N + 1);
		if (!ml_lu_decomp_complex(A, P, N)) return ml_error("ArrayError", "Matrix is degenerate");
		complex double *IA[N];
		complex double *InvData = IA[0] = anew(complex double, N * N);
		for (int I = 1; I < N; ++I) IA[I] = IA[I - 1] + N;
		for (int J = 0; J < N; ++J) {
			for (int I = 0; I < N; ++I) {
				IA[I][J] = P[I] == J;
				for (int K = 0; K < I; ++K) {
					IA[I][J] -= A[I][K] * IA[K][J];
				}
			}
			for (int I = N - 1; I >= 0; --I) {
				for (int K = I + 1; K < N; ++K) {
					IA[I][J] -= A[I][K] * IA[K][J];
				}
				IA[I][J] /= A[I][I];
			}
		}
		Inv->Base.Value = (char *)InvData;
		return (ml_value_t *)Inv;
#endif
	} else {
		return ml_error("ArrayError", "Invalid array type for operation");
	}
}

ML_METHOD("\\", MLMatrixT, MLVectorT) {
//<A
//<B
//>vector
// Returns the solution :mini:`X` of :mini:`A . X = B`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int N = Source->Dimensions[0].Size;
	if (N != Source->Dimensions[1].Size) return ml_error("ShapeError", "Square matrix required");
	ml_array_t *B = (ml_array_t *)Args[1];
	if (B->Dimensions->Size != N) return ml_error("ArrayError", "Matrix and vector sizes do not match");
	ml_array_format_t Format = MAX(Source->Format, B->Format);
	if (Format <= ML_ARRAY_FORMAT_F64) {
		ml_array_t *Tmp = ml_array_alloc(ML_ARRAY_FORMAT_F64, 2);
		ml_array_copy(Tmp, Source);
		int Stride = Tmp->Dimensions->Stride;
		double *A[N];
		char *Data = Tmp->Base.Value;
		for (int I = 0; I < N; ++I) {
			A[I] = (double *)Data;
			Data += Stride;
		}
		int *P = anew(int, N + 1);
		if (!ml_lu_decomp_real(A, P, N)) return ml_error("ArrayError", "Matrix is degenerate");
		ml_array_t *Sol = ml_array_alloc(ML_ARRAY_FORMAT_F64, 1);
		ml_array_copy(Sol, B);
		double X[N], *SolData = (double *)Sol->Base.Value;
		for (int I = 0; I < N; ++I) {
			X[I] = SolData[P[I]];
			for (int K = 0; K < I; ++K) {
				X[I] -= A[I][K] * X[K];
			}
		}
		for (int I = N - 1; I >= 0; --I) {
			for (int K = I + 1; K < N; ++K) {
				X[I] -= A[I][K] * X[K];
			}
			X[I] /= A[I][I];
		}
		memcpy(Sol->Base.Value, X, N * sizeof(double));
		return (ml_value_t *)Sol;
#ifdef ML_COMPLEX
	} else if (Format <= ML_ARRAY_FORMAT_C64) {
		ml_array_t *Tmp = ml_array_alloc(ML_ARRAY_FORMAT_C64, 2);
		ml_array_copy(Tmp, Source);
		int Stride = Tmp->Dimensions->Stride;
		complex double *A[N];
		char *Data = Tmp->Base.Value;
		for (int I = 0; I < N; ++I) {
			A[I] = (complex double *)Data;
			Data += Stride;
		}
		int *P = anew(int, N + 1);
		if (!ml_lu_decomp_complex(A, P, N)) return ml_error("ArrayError", "Matrix is degenerate");
		ml_array_t *Sol = ml_array_alloc(ML_ARRAY_FORMAT_C64, 1);
		ml_array_copy(Sol, B);
		complex double X[N], *SolData = (complex double *)Sol->Base.Value;
		for (int I = 0; I < N; ++I) {
			X[I] = SolData[P[I]];
			for (int K = 0; K < I; ++K) {
				X[I] -= A[I][K] * X[K];
			}
		}
		for (int I = N - 1; I >= 0; --I) {
			for (int K = I + 1; K < N; ++K) {
				X[I] -= A[I][K] * X[K];
			}
			X[I] /= A[I][I];
		}
		memcpy(Sol->Base.Value, X, N * sizeof(complex double));
		return (ml_value_t *)Sol;
#endif
	} else {
		return ml_error("ArrayError", "Invalid array type for operation");
	}
}

static ml_value_t *determinant2(ml_array_t *M, int N, int *Rows, int *Cols) {
	char *Row = M->Base.Value;
	if (M->Dimensions[0].Indices) {
		Row += M->Dimensions[0].Indices[Rows[0]] * M->Dimensions[0].Stride;
	} else {
		Row += Rows[0] * M->Dimensions[0].Stride;
	}
	++Rows;
	if (--N == 0) {
		if (M->Dimensions[1].Indices) {
			return *(ml_value_t **)(Row + M->Dimensions[1].Indices[Cols[0]] * M->Dimensions[1].Stride);
		} else {
			return *(ml_value_t **)(Row + Cols[0] * M->Dimensions[1].Stride);
		}
	}
	int Cols2[N];
	for (int I = 0; I < N; ++I) Cols2[I] = Cols[I + 1];
	ml_value_t *Args[3];
	if (M->Dimensions[1].Indices) {
		Args[0] = *(ml_value_t **)(Row + M->Dimensions[1].Indices[Cols[0]] * M->Dimensions[1].Stride);
		Args[1] = determinant2(M, N, Rows, Cols2);
		Args[0] = ml_simple_call(MulMethod, 2, Args);
		for (int I = 0; I < N; ++I) {
			Cols2[I] = Cols[I];
			Args[1] = *(ml_value_t **)(Row + M->Dimensions[1].Indices[Cols[I + 1]] * M->Dimensions[1].Stride);
			Args[2] = determinant2(M, N, Rows, Cols2);
			Args[1] = ml_simple_call(MulMethod, 2, Args + 1);
			Args[0] = ml_simple_call(I % 2 ? AddMethod : SubMethod, 2, Args);
		}
	} else {
		Args[0] = *(ml_value_t **)(Row + Cols[0] * M->Dimensions[1].Stride);
		Args[1] = determinant2(M, N, Rows, Cols2);
		Args[0] = ml_simple_call(MulMethod, 2, Args);
		for (int I = 0; I < N; ++I) {
			Cols2[I] = Cols[I];
			Args[1] = *(ml_value_t **)(Row + Cols[I + 1] * M->Dimensions[1].Stride);
			Args[2] = determinant2(M, N, Rows, Cols2);
			Args[1] = ml_simple_call(MulMethod, 2, Args + 1);
			Args[0] = ml_simple_call(I % 2 ? AddMethod : SubMethod, 2, Args);
		}
	}
	return Args[0];
}

ML_METHOD("det", MLMatrixT) {
//<A
//>any
// Returns the determinant of :mini:`A`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int N = Source->Dimensions[0].Size;
	if (N != Source->Dimensions[1].Size) return ml_error("ShapeError", "Square matrix required");
	if (Source->Format <= ML_ARRAY_FORMAT_F64) {
		ml_array_t *Tmp = ml_array_alloc(ML_ARRAY_FORMAT_F64, 2);
		ml_array_copy(Tmp, Source);
		int Stride = Tmp->Dimensions->Stride;
		double *A[N];
		char *Data = Tmp->Base.Value;
		for (int I = 0; I < N; ++I) {
			A[I] = (double *)Data;
			Data += Stride;
		}
		int *P = anew(int, N + 1);
		if (!ml_lu_decomp_real(A, P, N)) return ml_real(0);
		double Det = A[0][0];
		for (int I = 1; I < N; ++I) Det *= A[I][I];
		if ((P[N] - N) % 2) {
			return ml_real(-Det);
		} else {
			return ml_real(Det);
		}
#ifdef ML_COMPLEX
	} else if (Source->Format <= ML_ARRAY_FORMAT_C64) {
		ml_array_t *Tmp = ml_array_alloc(ML_ARRAY_FORMAT_C64, 2);
		ml_array_copy(Tmp, Source);
		int Stride = Tmp->Dimensions->Stride;
		complex double *A[N];
		char *Data = Tmp->Base.Value;
		for (int I = 0; I < N; ++I) {
			A[I] = (complex double *)Data;
			Data += Stride;
		}
		int *P = anew(int, N + 1);
		if (!ml_lu_decomp_complex(A, P, N)) return ml_real(0);
		complex double Det = A[0][0];
		for (int I = 1; I < N; ++I) Det *= A[I][I];
		if ((P[N] - N) % 2) {
			return ml_complex(-Det);
		} else {
			return ml_complex(Det);
		}
#endif
	} else if (Source->Format == ML_ARRAY_FORMAT_ANY) {
		int Rows[N], Cols[N];
		for (int I = 0; I < N; ++I) Rows[I] = Cols[I] = I;
		return determinant2(Source, N, Rows, Cols);
	} else {
		return ml_error("ArrayError", "Invalid array type for operation");
	}
}

ML_METHOD("tr", MLMatrixT) {
//<A
//>any
// Returns the trace of :mini:`A`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int N = Source->Dimensions[0].Size;
	if (N != Source->Dimensions[1].Size) return ml_error("ShapeError", "Square matrix required");
	if (Source->Format <= ML_ARRAY_FORMAT_F64) {
		double Trace = 0;
		char *Data = Source->Base.Value;
		ml_array_getter_double get = MLArrayGettersFloat64[Source->Format];
		int Stride0 = Source->Dimensions[0].Stride;
		int Stride1 = Source->Dimensions[1].Stride;
		if (Source->Dimensions[0].Indices) {
			const int *Indices0 = Source->Dimensions[0].Indices;
			if (Source->Dimensions[1].Indices) {
				const int *Indices1 = Source->Dimensions[1].Indices;
				for (int I = 0; I < N; ++I) {
					Trace += get(Data + (Indices0[I] * Stride0) + (Indices1[I] * Stride1));
				}
			} else {
				for (int I = 0; I < N; ++I) {
					Trace += get(Data + (Indices0[I] * Stride0) + (I * Stride1));
				}
			}
		} else if (Source->Dimensions[1].Indices) {
			const int *Indices1 = Source->Dimensions[1].Indices;
			for (int I = 0; I < N; ++I) {
				Trace += get(Data + (I * Stride0) + (Indices1[I] * Stride1));
			}
		} else {
			for (int I = 0; I < N; ++I) {
				Trace += get(Data + (I * Stride0) + (I * Stride1));
			}
		}
		return ml_real(Trace);
#ifdef ML_COMPLEX
	} else if (Source->Format <= ML_ARRAY_FORMAT_C64) {
		complex double Trace = 0;
		char *Data = Source->Base.Value;
		ml_array_getter_complex_double get = MLArrayGettersComplex64[Source->Format];
		int Stride0 = Source->Dimensions[0].Stride;
		int Stride1 = Source->Dimensions[1].Stride;
		if (Source->Dimensions[0].Indices) {
			const int *Indices0 = Source->Dimensions[0].Indices;
			if (Source->Dimensions[1].Indices) {
				const int *Indices1 = Source->Dimensions[1].Indices;
				for (int I = 0; I < N; ++I) {
					Trace += get(Data + (Indices0[I] * Stride0) + (Indices1[I] * Stride1));
				}
			} else {
				for (int I = 0; I < N; ++I) {
					Trace += get(Data + (Indices0[I] * Stride0) + (I * Stride1));
				}
			}
		} else if (Source->Dimensions[1].Indices) {
			const int *Indices1 = Source->Dimensions[1].Indices;
			for (int I = 0; I < N; ++I) {
				Trace += get(Data + (I * Stride0) + (Indices1[I] * Stride1));
			}
		} else {
			for (int I = 0; I < N; ++I) {
				Trace += get(Data + (I * Stride0) + (I * Stride1));
			}
		}
		return ml_complex(Trace);
#endif
	} else if (Source->Format == ML_ARRAY_FORMAT_ANY) {
		ml_value_t *Args2[2];
		char *Data = Source->Base.Value;
		int Stride0 = Source->Dimensions[0].Stride;
		int Stride1 = Source->Dimensions[1].Stride;
		if (Source->Dimensions[0].Indices) {
			const int *Indices0 = Source->Dimensions[0].Indices;
			if (Source->Dimensions[1].Indices) {
				const int *Indices1 = Source->Dimensions[1].Indices;
				Args2[0] = *(ml_value_t **)(Data + (Indices0[0] * Stride0) + (Indices1[0] * Stride1));
				for (int I = 1; I < N; ++I) {
					Args2[1] = *(ml_value_t **)(Data + (Indices0[I] * Stride0) + (Indices1[I] * Stride1));
					Args2[0] = ml_simple_call(AddMethod, 2, Args2);
				}
			} else {
				Args2[0] = *(ml_value_t **)(Data + (Indices0[0] * Stride0) + (0 * Stride1));
				for (int I = 1; I < N; ++I) {
					Args2[1] = *(ml_value_t **)(Data + (Indices0[I] * Stride0) + (I * Stride1));
					Args2[0] = ml_simple_call(AddMethod, 2, Args2);
				}
			}
		} else if (Source->Dimensions[1].Indices) {
			const int *Indices1 = Source->Dimensions[1].Indices;
			Args2[0] = *(ml_value_t **)(Data + (0 * Stride0) + (Indices1[0] * Stride1));
			for (int I = 1; I < N; ++I) {
				Args2[1] = *(ml_value_t **)(Data + (I * Stride0) + (Indices1[I] * Stride1));
				Args2[0] = ml_simple_call(AddMethod, 2, Args2);
			}
		} else {
			Args2[0] = *(ml_value_t **)(Data + (0 * Stride0) + (0 * Stride1));
			for (int I = 1; I < N; ++I) {
				Args2[1] = *(ml_value_t **)(Data + (I * Stride0) + (I * Stride1));
				Args2[0] = ml_simple_call(AddMethod, 2, Args2);
			}
		}
		return Args2[0];
	} else {
		return ml_error("ArrayError", "Invalid array type for operation");
	}
}

ML_METHOD("softmax", MLVectorRealT) {
//<Vector
//>vector
// Returns :mini:`softmax(Vector)`.
//$= let A := array([1, 4.2, 0.6, 1.23, 4.3, 1.2, 2.5])
//$= let B := A:softmax
	ml_array_t *A = (ml_array_t *)Args[0];
	int N = A->Dimensions[0].Size;
	ml_array_t *B = ml_array(ML_ARRAY_FORMAT_F64, 1, N);
	ml_array_copy(B, A);
	double *Values = (double *)B->Base.Value;
	double M = -INFINITY;
	for (int I = 0; I < N; ++I) if (M < Values[I]) M = Values[I];
	double Sum = 0.0;
	for (int I = 0; I < N; ++I) Sum += exp(Values[I] - M);
	double C = M + log(Sum);
	for (int I = 0; I < N; ++I) Values[I] = exp(Values[I] - C);
	return (ml_value_t *)B;
}

static ml_value_t *ml_array_cat(int Axis, int Count, ml_value_t **Args) {
	ml_array_t *A = (ml_array_t *)Args[0];
	int Degree = A->Degree;
	if (Axis >= A->Degree) return ml_error("IntervalError", "Invalid axis");
	int Total = A->Dimensions[Axis].Size;
	ml_array_format_t Format = A->Format;
	for (int I = 1; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLArrayT);
		ml_array_t *B = (ml_array_t *)Args[I];
		if (B->Degree != Degree) return ml_error("ShapeError", "Incompatible array shapes");
		for (int J = 0; J < Degree; ++J) {
			if (J == Axis) {
				Total += B->Dimensions[J].Size;
			} else {
				if (B->Dimensions[J].Size != A->Dimensions[J].Size) return ml_error("ShapeError", "Incompatible array shapes");
			}
		}
		if (Format < B->Format) Format = B->Format;
	}
	ml_array_t *C = ml_array_alloc(Format, Degree);
	int Stride = MLArraySizes[Format];
	for (int I = Degree; --I >= 0;) {
		C->Dimensions[I].Stride = Stride;
		if (I == Axis) {
			C->Dimensions[I].Size = Total;
			Stride *= Total;
		} else {
			C->Dimensions[I].Size = A->Dimensions[I].Size;
			Stride *= A->Dimensions[I].Size;
		}
	}
	C->Base.Length = Stride;
	void *Value = C->Base.Value = array_alloc(Format, Stride);
	update_row_fn_t Update = UpdateSetRowFns[C->Format * MAX_FORMATS + A->Format];
	C->Dimensions[Axis].Size = A->Dimensions[Axis].Size;
	update_array(Update, C->Dimensions, Value, Degree, A->Dimensions, A->Base.Value);
	int Offset = A->Dimensions[Axis].Size;
	for (int I = 1; I < Count; ++I) {
		Value += Offset * C->Dimensions[Axis].Stride;
		ml_array_t *B = (ml_array_t *)Args[I];
		Update = UpdateSetRowFns[C->Format * MAX_FORMATS + B->Format];
		C->Dimensions[Axis].Size = B->Dimensions[Axis].Size;
		update_array(Update, C->Dimensions, Value, Degree, B->Dimensions, B->Base.Value);
		Offset = B->Dimensions[Axis].Size;
	}
	C->Dimensions[Axis].Size = Total;
	return (ml_value_t *)C;
}

ML_FUNCTION(MLArrayCat) {
//@array::cat
//<Index
//<Array/1...
//>array
// Returns a new array with the values of :mini:`Array/1, ..., Array/n` concatenated along the :mini:`Index`-th dimension.
//$= let A := $[[1, 2, 3], [4, 5, 6]]
//$= let B := $[[7, 8, 9], [10, 11, 12]]
//$= array::cat(1, A, B)
//$= array::cat(2, A, B)
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	int Axis = ml_integer_value(Args[0]) - 1;
	if (Axis < 0) return ml_error("IntervalError", "Invalid axis");
	ML_CHECK_ARG_TYPE(1, MLArrayT);
	return ml_array_cat(Axis, Count - 1, Args + 1);
}

ML_FUNCTION(MLArrayHCat) {
//@array::hcat
//<Array/1...
//>array
// Returns a new array with the values of :mini:`Array/1, ..., Array/n` concatenated along the last dimension.
//$= let A := $[[1, 2, 3], [4, 5, 6]]
//$= let B := $[[7, 8, 9], [10, 11, 12]]
//$= array::hcat(A, B)
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLArrayT);
	ml_array_t *A = (ml_array_t *)Args[0];
	return ml_array_cat(A->Degree - 1, Count, Args);
}

ML_FUNCTION(MLArrayVCat) {
//@array::vcat
//<Array/1...
//>array
// Returns a new array with the values of :mini:`Array/1, ..., Array/n` concatenated along the first dimension.
//$= let A := $[[1, 2, 3], [4, 5, 6]]
//$= let B := $[[7, 8, 9], [10, 11, 12]]
//$= array::vcat(A, B)
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLArrayT);
	return ml_array_cat(0, Count, Args);
}

#ifdef ML_MUTABLES

extern ml_type_t MLArrayMutableT[];

#else

#define MLArrayMutableT MLArrayT

#endif

void ml_array2_init(stringmap_t *Globals) {
#include "ml_array2_init.c"
	stringmap_insert(MLArrayT->Exports, "cat", MLArrayCat);
	stringmap_insert(MLArrayT->Exports, "hcat", MLArrayHCat);
	stringmap_insert(MLArrayT->Exports, "vcat", MLArrayVCat);
	ml_method_by_name("set", UpdateSetRowFns, update_array_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("add", UpdateAddRowFns, update_array_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("mul", UpdateMulRowFns, update_array_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("sub", UpdateSubRowFns, update_array_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("div", UpdateDivRowFns, update_array_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("+", UpdateAddRowFns, array_infix_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("*", UpdateMulRowFns, array_infix_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("-", UpdateSubRowFns, array_infix_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("/", UpdateDivRowFns, array_infix_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("/\\", UpdateAndRowFns, array_infix_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("\\/", UpdateOrRowFns, array_infix_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("><", UpdateXorRowFns, array_infix_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("min", UpdateMinRowFns, array_infix_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("max", UpdateMaxRowFns, array_infix_fn, MLArrayMutableT, MLArrayT, NULL);
	ml_method_by_name("==", CompareEqRowFns, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("!==", CompareNeRowFns, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("<", CompareLtRowFns, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name(">", CompareGtRowFns, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("<=", CompareLeRowFns, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name(">=", CompareGeRowFns, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("++", MLArrayInfixAddFns, (ml_callback_t)ml_array_pairwise_infix, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("**", MLArrayInfixMulFns, (ml_callback_t)ml_array_pairwise_infix, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("--", MLArrayInfixSubFns, (ml_callback_t)ml_array_pairwise_infix, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("//", MLArrayInfixDivFns, (ml_callback_t)ml_array_pairwise_infix, MLArrayT, MLArrayT, NULL);

	ml_method_by_value(AbsMethod, labs, (ml_callback_t)array_math_integer_fn, MLArrayIntegerT, NULL);
	ml_method_by_value(SquareMethod, isquare, (ml_callback_t)array_math_integer_fn, MLArrayIntegerT, NULL);

	ml_method_by_value(AcosMethod, acos, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(AsinMethod, asin, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(AtanMethod, atan, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(CeilMethod, ceil, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(CosMethod, cos, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(CoshMethod, cosh, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(ExpMethod, exp, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(AbsMethod, fabs, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(FloorMethod, floor, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(LogMethod, log, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(Log10Method, log10, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(LogitMethod, logit, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(SinMethod, sin, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(SinhMethod, sinh, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(SqrtMethod, sqrt, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(SquareMethod, square, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(TanMethod, tan, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(TanhMethod, tanh, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(ErfMethod, erf, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(ErfcMethod, erfc, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(GammaMethod, lgamma, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(AcoshMethod, acosh, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(AsinhMethod, asinh, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(AtanhMethod, atanh, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(CbrtMethod, cbrt, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(Expm1Method, expm1, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(Log1pMethod, log1p, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);
	ml_method_by_value(RoundMethod, round, (ml_callback_t)array_math_real_fn, MLArrayRealT, NULL);

#ifdef ML_COMPLEX
	ml_method_by_value(AcosMethod, cacos, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(AsinMethod, casin, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(AtanMethod, catan, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(CosMethod, ccos, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(CoshMethod, ccosh, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(ExpMethod, cexp, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(AbsMethod, cabs, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(LogMethod, clog, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(Log10Method, clog10, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(SinMethod, csin, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(SinhMethod, csinh, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(SqrtMethod, csqrt, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(SquareMethod, csquare, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(TanMethod, ctan, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(TanhMethod, ctanh, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(AcoshMethod, cacosh, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(AsinhMethod, casinh, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(AtanhMethod, catanh, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(ConjMethod, conj, (ml_callback_t)array_math_complex_fn, MLArrayComplexT, NULL);
	ml_method_by_value(AbsMethod, cabs, (ml_callback_t)array_math_complex_real_fn, MLArrayComplexT, NULL);
	ml_method_by_value(ArgMethod, carg, (ml_callback_t)array_math_complex_real_fn, MLArrayComplexT, NULL);
#endif
}

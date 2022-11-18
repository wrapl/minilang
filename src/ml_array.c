#include "ml_array.h"
#include "ml_macros.h"
#include "ml_math.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>

#ifdef ML_COMPLEX
#include <complex.h>
#undef I
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "array"

typedef ml_value_t *any;

static ml_value_t *ml_array_of_fn(void *Data, int Count, ml_value_t **Args);

ML_CFUNCTION(MLArray, NULL, ml_array_of_fn);
//@array
//<List:list
//>array
// Returns a new array containing the values in :mini:`List`.
// The shape and type of the array is determined from the elements in :mini:`List`.

ML_TYPE(MLArrayT, (MLAddressT, MLSequenceT), "array",
// Base type for multidimensional arrays.
	.Constructor = (ml_value_t *)MLArray
);

ML_TYPE(MLArrayMutableT, (MLArrayT, MLBufferT), "array::mutable");

ML_TYPE(MLVectorT, (MLArrayT), "vector",
// Arrays with exactly 1 dimension.
	.Constructor = (ml_value_t *)MLArray
);

ML_TYPE(MLVectorMutableT, (MLVectorT, MLArrayMutableT), "vector::mutable");

ML_TYPE(MLMatrixT, (MLArrayT), "matrix",
// Arrays with exactly 2 dimensions.
	.Constructor = (ml_value_t *)MLArray
);

ML_TYPE(MLMatrixMutableT, (MLMatrixT, MLArrayMutableT), "matrix::mutable");

#ifdef ML_COMPLEX

ML_TYPE(MLArrayComplexT, (MLArrayT), "array::complex");

extern ml_type_t MLArrayComplex32T[];
extern ml_type_t MLArrayComplex64T[];

ML_TYPE(MLArrayMutableComplexT, (MLArrayComplexT, MLArrayMutableT), "array::mutable::complex");
// Base type for arrays of complex numbers.

extern ml_type_t MLArrayMutableComplex32T[];
extern ml_type_t MLArrayMutableComplex64T[];

ML_TYPE(MLVectorComplexT, (MLArrayComplexT, MLVectorT), "vector::complex");

extern ml_type_t MLVectorComplex32T[];
extern ml_type_t MLVectorComplex64T[];

ML_TYPE(MLVectorMutableComplexT, (MLVectorComplexT, MLArrayMutableComplexT, MLVectorMutableT), "vector::mutable::complex");
// Base type for vectors of complex numbers.

extern ml_type_t MLVectorMutableComplex32T[];
extern ml_type_t MLVectorMutableComplex64T[];

ML_TYPE(MLMatrixComplexT, (MLArrayComplexT, MLMatrixT), "matrix::mutable::complex");

extern ml_type_t MLMatrixComplex32T[];
extern ml_type_t MLMatrixComplex64T[];

ML_TYPE(MLMatrixMutableComplexT, (MLArrayMutableComplexT, MLMatrixMutableT), "matrix::mutable::complex");
// Base type for matrices of complex numbers.

extern ml_type_t MLMatrixMutableComplex32T[];
extern ml_type_t MLMatrixMutableComplex64T[];

ML_TYPE(MLArrayRealT, (MLArrayComplexT), "array::mutable::real");

ML_TYPE(MLArrayMutableRealT, (MLArrayRealT, MLArrayMutableComplexT), "array::mutable::real");
// Base type for arrays of real numbers.

ML_TYPE(MLVectorRealT, (MLArrayRealT, MLVectorComplexT), "vector::real");

ML_TYPE(MLVectorMutableRealT, (MLVectorRealT, MLArrayMutableRealT, MLVectorMutableComplexT), "vector::mutable::real");
// Base type for vectors of real numbers.

ML_TYPE(MLMatrixRealT, (MLArrayRealT, MLMatrixComplexT), "matrix::real");

ML_TYPE(MLMatrixMutableRealT, (MLMatrixRealT, MLArrayMutableRealT, MLMatrixMutableComplexT), "matrix::mutable::real");
// Base type for matrices of real numbers.

#else

ML_TYPE(MLArrayRealT, (MLArrayT), "array::mutable::real");
//!internal

ML_TYPE(MLArrayMutableRealT, (MLArrayRealT, MLArrayMutableT), "array::mutable::real");
//!internal

ML_TYPE(MLVectorRealT, (MLArrayRealT, MLVectorT), "vector::real");
//!internal

ML_TYPE(MLVectorMutableRealT, (MLVectorRealT, MLArrayMutableRealT, MLVectorMutableT), "vector::mutable::real");
//!internal

ML_TYPE(MLMatrixRealT, (MLArrayRealT, MLMatrixT), "matrix::real");
//!internal

ML_TYPE(MLMatrixMutableRealT, (MLMatrixRealT, MLArrayMutableRealT, MLMatrixMutableT), "matrix::mutable::real");
//!internal

#endif

ML_TYPE(MLArrayIntegerT, (MLArrayRealT), "array::integer");

extern ml_type_t MLArrayUInt8T[];
extern ml_type_t MLArrayInt8T[];
extern ml_type_t MLArrayUInt16T[];
extern ml_type_t MLArrayInt16T[];
extern ml_type_t MLArrayUInt32T[];
extern ml_type_t MLArrayInt32T[];
extern ml_type_t MLArrayUInt64T[];
extern ml_type_t MLArrayInt64T[];
extern ml_type_t MLArrayFloat32T[];
extern ml_type_t MLArrayFloat64T[];

ML_TYPE(MLArrayMutableIntegerT, (MLArrayIntegerT, MLArrayMutableRealT), "array::mutable::integer");
// Base type for arrays of integers.

extern ml_type_t MLArrayMutableUInt8T[];
extern ml_type_t MLArrayMutableInt8T[];
extern ml_type_t MLArrayMutableUInt16T[];
extern ml_type_t MLArrayMutableInt16T[];
extern ml_type_t MLArrayMutableUInt32T[];
extern ml_type_t MLArrayMutableInt32T[];
extern ml_type_t MLArrayMutableUInt64T[];
extern ml_type_t MLArrayMutableInt64T[];
extern ml_type_t MLArrayMutableFloat32T[];
extern ml_type_t MLArrayMutableFloat64T[];

ML_TYPE(MLVectorIntegerT, (MLVectorRealT), "vector::integer");

extern ml_type_t MLVectorUInt8T[];
extern ml_type_t MLVectorInt8T[];
extern ml_type_t MLVectorUInt16T[];
extern ml_type_t MLVectorInt16T[];
extern ml_type_t MLVectorUInt32T[];
extern ml_type_t MLVectorInt32T[];
extern ml_type_t MLVectorUInt64T[];
extern ml_type_t MLVectorInt64T[];
extern ml_type_t MLVectorFloat32T[];
extern ml_type_t MLVectorFloat64T[];

ML_TYPE(MLVectorMutableIntegerT, (MLVectorIntegerT, MLVectorMutableRealT), "vector::mutable::integer");
// Base type for vectors of integers.

extern ml_type_t MLVectorMutableUInt8T[];
extern ml_type_t MLVectorMutableInt8T[];
extern ml_type_t MLVectorMutableUInt16T[];
extern ml_type_t MLVectorMutableInt16T[];
extern ml_type_t MLVectorMutableUInt32T[];
extern ml_type_t MLVectorMutableInt32T[];
extern ml_type_t MLVectorMutableUInt64T[];
extern ml_type_t MLVectorMutableInt64T[];
extern ml_type_t MLVectorMutableFloat32T[];
extern ml_type_t MLVectorMutableFloat64T[];

ML_TYPE(MLMatrixIntegerT, (MLMatrixRealT), "matrix::integer");

extern ml_type_t MLMatrixUInt8T[];
extern ml_type_t MLMatrixInt8T[];
extern ml_type_t MLMatrixUInt16T[];
extern ml_type_t MLMatrixInt16T[];
extern ml_type_t MLMatrixUInt32T[];
extern ml_type_t MLMatrixInt32T[];
extern ml_type_t MLMatrixUInt64T[];
extern ml_type_t MLMatrixInt64T[];
extern ml_type_t MLMatrixFloat32T[];
extern ml_type_t MLMatrixFloat64T[];

ML_TYPE(MLMatrixMutableIntegerT, (MLMatrixIntegerT, MLMatrixMutableRealT), "matrix::mutable::integer");
// Base type for matrices of integers.

extern ml_type_t MLMatrixMutableUInt8T[];
extern ml_type_t MLMatrixMutableInt8T[];
extern ml_type_t MLMatrixMutableUInt16T[];
extern ml_type_t MLMatrixMutableInt16T[];
extern ml_type_t MLMatrixMutableUInt32T[];
extern ml_type_t MLMatrixMutableInt32T[];
extern ml_type_t MLMatrixMutableUInt64T[];
extern ml_type_t MLMatrixMutableInt64T[];
extern ml_type_t MLMatrixMutableFloat32T[];
extern ml_type_t MLMatrixMutableFloat64T[];

extern ml_type_t MLArrayAnyT[];
extern ml_type_t MLVectorAnyT[];
extern ml_type_t MLMatrixAnyT[];

extern ml_type_t MLArrayMutableAnyT[];
extern ml_type_t MLVectorMutableAnyT[];
extern ml_type_t MLMatrixMutableAnyT[];

size_t MLArraySizes[] = {
	[ML_ARRAY_FORMAT_NONE] = sizeof(ml_value_t *),
	[ML_ARRAY_FORMAT_U8] = sizeof(uint8_t),
	[ML_ARRAY_FORMAT_I8] = sizeof(int8_t),
	[ML_ARRAY_FORMAT_U16] = sizeof(uint16_t),
	[ML_ARRAY_FORMAT_I16] = sizeof(int16_t),
	[ML_ARRAY_FORMAT_U32] = sizeof(uint32_t),
	[ML_ARRAY_FORMAT_I32] = sizeof(int32_t),
	[ML_ARRAY_FORMAT_U64] = sizeof(uint64_t),
	[ML_ARRAY_FORMAT_I64] = sizeof(int64_t),
	[ML_ARRAY_FORMAT_F32] = sizeof(float),
	[ML_ARRAY_FORMAT_F64] = sizeof(double),
#ifdef ML_COMPLEX
	[ML_ARRAY_FORMAT_C32] = sizeof(complex float),
	[ML_ARRAY_FORMAT_C64] = sizeof(complex double),
#endif
	[ML_ARRAY_FORMAT_ANY] = sizeof(ml_value_t *)
};

#ifdef ML_COMPLEX

#define ML_ARRAY_TYPES(NAME) \
static ml_type_t *ML ## NAME ## Types[] = { \
	[ML_ARRAY_FORMAT_NONE] = ML ## NAME ## T, \
	[ML_ARRAY_FORMAT_U8] = ML ## NAME ## UInt8T, \
	[ML_ARRAY_FORMAT_I8] = ML ## NAME ## Int8T, \
	[ML_ARRAY_FORMAT_U16] = ML ## NAME ## UInt16T, \
	[ML_ARRAY_FORMAT_I16] = ML ## NAME ## Int16T, \
	[ML_ARRAY_FORMAT_U32] = ML ## NAME ## UInt32T, \
	[ML_ARRAY_FORMAT_I32] = ML ## NAME ## Int32T, \
	[ML_ARRAY_FORMAT_U64] = ML ## NAME ## UInt64T, \
	[ML_ARRAY_FORMAT_I64] = ML ## NAME ## Int64T, \
	[ML_ARRAY_FORMAT_F32] = ML ## NAME ## Float32T, \
	[ML_ARRAY_FORMAT_F64] = ML ## NAME ## Float64T, \
	[ML_ARRAY_FORMAT_C32] = ML ## NAME ## Complex32T, \
	[ML_ARRAY_FORMAT_C64] = ML ## NAME ## Complex64T, \
	[ML_ARRAY_FORMAT_ANY] = ML ## NAME ## AnyT \
}

#else

#define ML_ARRAY_TYPES(NAME) \
static ml_type_t *ML ## NAME ## Types[] = { \
	[ML_ARRAY_FORMAT_NONE] = ML ## NAME ## T, \
	[ML_ARRAY_FORMAT_U8] = ML ## NAME ## UInt8T, \
	[ML_ARRAY_FORMAT_I8] = ML ## NAME ## Int8T, \
	[ML_ARRAY_FORMAT_U16] = ML ## NAME ## UInt16T, \
	[ML_ARRAY_FORMAT_I16] = ML ## NAME ## Int16T, \
	[ML_ARRAY_FORMAT_U32] = ML ## NAME ## UInt32T, \
	[ML_ARRAY_FORMAT_I32] = ML ## NAME ## Int32T, \
	[ML_ARRAY_FORMAT_U64] = ML ## NAME ## UInt64T, \
	[ML_ARRAY_FORMAT_I64] = ML ## NAME ## Int64T, \
	[ML_ARRAY_FORMAT_F32] = ML ## NAME ## Float32T, \
	[ML_ARRAY_FORMAT_F64] = ML ## NAME ## Float64T, \
	[ML_ARRAY_FORMAT_ANY] = ML ## NAME ## AnyT \
}

#endif

ML_ARRAY_TYPES(Array);
ML_ARRAY_TYPES(ArrayMutable);
ML_ARRAY_TYPES(Vector);
ML_ARRAY_TYPES(VectorMutable);
ML_ARRAY_TYPES(Matrix);
ML_ARRAY_TYPES(MatrixMutable);

static ml_array_format_t ml_array_format(ml_type_t *Type) {
	for (ml_array_format_t Format = ML_ARRAY_FORMAT_NONE; Format <= ML_ARRAY_FORMAT_ANY; ++Format) {
		if (MLArrayTypes[Format] == Type) return Format;
		if (MLArrayMutableTypes[Format] == Type) return Format;
		if (MLVectorTypes[Format] == Type) return Format;
		if (MLVectorMutableTypes[Format] == Type) return Format;
		if (MLMatrixTypes[Format] == Type) return Format;
		if (MLMatrixMutableTypes[Format] == Type) return Format;
	}
	return ML_ARRAY_FORMAT_NONE;
}

ml_array_t *ml_array_alloc(ml_array_format_t Format, int Degree) {
	ml_array_t *Array = xnew(ml_array_t, Degree, ml_array_dimension_t);
	if (Degree == 1) {
		Array->Base.Type = MLVectorMutableTypes[Format];
	} else if (Degree == 2) {
		Array->Base.Type = MLMatrixMutableTypes[Format];
	} else {
		Array->Base.Type = MLArrayMutableTypes[Format];
	}
	Array->Degree = Degree;
	Array->Format = Format;
	return Array;
}

ml_array_t *ml_array(ml_array_format_t Format, int Degree, ...) {
	ml_array_t *Array = ml_array_alloc(Format, Degree);
	int DataSize = MLArraySizes[Format];
	va_list Sizes;
	va_start(Sizes, Degree);
	for (int I = Degree; --I >= 0;) {
		Array->Dimensions[I].Stride = DataSize;
		int Size = Array->Dimensions[I].Size = va_arg(Sizes, int);
		DataSize *= Size;
	}
	va_end(Sizes);
	Array->Base.Value = snew(DataSize);
	Array->Base.Length = DataSize;
	return Array;
}

int ml_array_degree(ml_value_t *Value) {
	return ((ml_array_t *)Value)->Degree;
}

int ml_array_size(ml_value_t *Value, int Dim) {
	ml_array_t *Array = (ml_array_t *)Value;
	if (Dim < 0 || Dim >= Array->Degree) return 0;
	return Array->Dimensions[Dim].Size;
}

typedef struct ml_array_init_state_t {
	ml_state_t Base;
	char *Address;
	ml_array_t *Array;
	ml_value_t *Function;
	ml_value_t *Args[];
} ml_array_init_state_t;

static void ml_array_init_run(ml_array_init_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	Value = ml_deref(Value);
	ml_array_t *Array = State->Array;
	switch (Array->Format) {
	case ML_ARRAY_FORMAT_NONE:
		break;
	case ML_ARRAY_FORMAT_ANY:
		*(ml_value_t **)State->Address = Value;
		State->Address += sizeof(ml_value_t *);
		break;
	case ML_ARRAY_FORMAT_U8:
		*(uint8_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(uint8_t);
		break;
	case ML_ARRAY_FORMAT_I8:
		*(int8_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(int8_t);
		break;
	case ML_ARRAY_FORMAT_U16:
		*(uint16_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(uint16_t);
		break;
	case ML_ARRAY_FORMAT_I16:
		*(int16_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(int16_t);
		break;
	case ML_ARRAY_FORMAT_U32:
		*(uint32_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(uint32_t);
		break;
	case ML_ARRAY_FORMAT_I32:
		*(int32_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(int32_t);
		break;
	case ML_ARRAY_FORMAT_U64:
		*(uint64_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(uint64_t);
		break;
	case ML_ARRAY_FORMAT_I64:
		*(int64_t *)State->Address = ml_integer_value(Value);
		State->Address += sizeof(int64_t);
		break;
	case ML_ARRAY_FORMAT_F32:
		*(float *)State->Address = ml_real_value(Value);
		State->Address += sizeof(float);
		break;
	case ML_ARRAY_FORMAT_F64:
		*(double *)State->Address = ml_real_value(Value);
		State->Address += sizeof(double);
		break;
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
		*(complex float *)State->Address = ml_complex_value(Value);
		State->Address += sizeof(complex float);
		break;
	case ML_ARRAY_FORMAT_C64:
		*(complex double *)State->Address = ml_complex_value(Value);
		State->Address += sizeof(complex double);
		break;
#endif
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

static int array_copy(ml_array_t *Target, ml_array_t *Source);

static void ml_array_typed_new_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_array_format_t Format = (intptr_t)Data;
	if (ml_is(Args[0], MLListT)) {
		int Degree = ml_list_length(Args[0]);
		ml_array_t *Array = ml_array_alloc(Format, Degree);
		int I = 0;
		ML_LIST_FOREACH(Args[0], Iter) {
			if (!ml_is(Iter->Value, MLIntegerT)) ML_ERROR("TypeError", "Dimension is not an integer");
			Array->Dimensions[I++].Size = ml_integer_value(Iter->Value);
		}
		int DataSize = MLArraySizes[Format];
		for (int I = Array->Degree; --I >= 0;) {
			Array->Dimensions[I].Stride = DataSize;
			DataSize *= Array->Dimensions[I].Size;
		}
		Array->Base.Value = snew(DataSize);
		Array->Base.Length = DataSize;
		if (Count == 1) {
			if (Format == ML_ARRAY_FORMAT_ANY) {
				ml_value_t **Values = (ml_value_t **)Array->Base.Value;
				for (int I = DataSize / sizeof(ml_value_t *); --I >= 0;) {
					*Values++ = MLNil;
				}
			} else {
				memset(Array->Base.Value, 0, DataSize);
			}
			ML_RETURN(Array);
		}
		ml_array_init_state_t *State = xnew(ml_array_init_state_t, Array->Degree, ml_value_t *);
		State->Base.Caller = Caller;
		State->Base.run = (void *)ml_array_init_run;
		State->Base.Context = Caller->Context;
		State->Address = Array->Base.Value;
		State->Array = Array;
		ml_value_t *Function = State->Function = Args[1];
		for (int I = 0; I < Array->Degree; ++I) State->Args[I] = ml_integer(1);
		return ml_call(State, Function, Array->Degree, State->Args);
	} else if (ml_is(Args[0], MLArrayT)) {
		ml_array_t *Source = (ml_array_t *)Args[0];
		ml_array_t *Target = ml_array_alloc(Format, Source->Degree);
		array_copy(Target, Source);
		ML_RETURN(Target);
	} else if (ml_is(Args[0], MLIntegerT)) {
		for (int I = 1; I < Count - 1; ++I) ML_CHECKX_ARG_TYPE(I, MLIntegerT);
		int Degree = ml_is(Args[Count - 1], MLIntegerT) ? Count : (Count - 1);
		ml_array_t *Array = ml_array_alloc(Format, Degree);
		int DataSize = MLArraySizes[Format];
		for (int I = Array->Degree; --I >= 0;) {
			Array->Dimensions[I].Stride = DataSize;
			size_t Size = Array->Dimensions[I].Size = ml_integer_value(Args[I]);
			DataSize *= Size;
		}
		Array->Base.Value = snew(DataSize);
		Array->Base.Length = DataSize;
		if (Count == Degree) {
			if (Format == ML_ARRAY_FORMAT_ANY) {
				ml_value_t **Values = (ml_value_t **)Array->Base.Value;
				for (int I = DataSize / sizeof(ml_value_t *); --I >= 0;) {
					*Values++ = MLNil;
				}
			} else {
				memset(Array->Base.Value, 0, DataSize);
			}
			ML_RETURN(Array);
		}
		ml_array_init_state_t *State = xnew(ml_array_init_state_t, Array->Degree, ml_value_t *);
		State->Base.Caller = Caller;
		State->Base.run = (void *)ml_array_init_run;
		State->Base.Context = Caller->Context;
		State->Address = Array->Base.Value;
		State->Array = Array;
		ml_value_t *Function = State->Function = Args[Count - 1];
		for (int I = 0; I < Array->Degree; ++I) State->Args[I] = ml_integer(1);
		return ml_call(State, Function, Array->Degree, State->Args);
	} else if (ml_is(Args[0], MLAddressT)) {
		ml_array_t *Array = ml_array_alloc(Format, 1);
		if (!ml_is(Args[0], MLBufferT)) Array->Base.Type = MLVectorTypes[Format];
		size_t Size = Array->Dimensions[0].Size = ml_address_length(Args[0]) / MLArraySizes[Format];
		size_t Stride = Array->Dimensions[0].Stride = MLArraySizes[Format];
		Array->Base.Value = (void *)ml_address_value(Args[0]);
		Array->Base.Length = Size * Stride;
		ML_RETURN(Array);
	} else {
		ML_ERROR("TypeError", "expected list or array for argument 1");
	}
}

ML_FUNCTIONX(MLArrayNew) {
//@array::new
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLTypeT);
	ML_CHECKX_ARG_TYPE(1, MLListT);
	ml_array_format_t Format = ml_array_format((ml_type_t *)Args[0]);
	if (Format == ML_ARRAY_FORMAT_NONE) ML_ERROR("TypeError", "Unknown type for array");
	return ml_array_typed_new_fnx(Caller, (void *)Format, Count - 1, Args + 1);
}

ML_FUNCTION(MLArrayWrap) {
//@array::wrap
//<Type:type
//<Buffer
//<Sizes
//<Strides
//>array
// Returns an array pointing to the contents of :mini:`Address` with the corresponding sizes and strides.
//$= let B := buffer(16)
//$= array::wrap(array::uint16, B, [2, 2, 2], [8, 4, 2])
	ML_CHECK_ARG_COUNT(4);
	ML_CHECK_ARG_TYPE(0, MLTypeT);
	ML_CHECK_ARG_TYPE(1, MLAddressT);
	ML_CHECK_ARG_TYPE(2, MLListT);
	ML_CHECK_ARG_TYPE(3, MLListT);
	ml_array_format_t Format = ml_array_format((ml_type_t *)Args[0]);
	if (Format == ML_ARRAY_FORMAT_NONE) return ml_error("TypeError", "Unknown type for array");
	int Degree = ml_list_length(Args[2]);
	if (!Degree) return ml_error("ValueError", "Dimensions must not be empty");
	if (Degree != ml_list_length(Args[3])) return ml_error("ValueError", "Dimensions and strides must have same length");
	ml_array_t *Array = ml_array_alloc(Format, Degree);
	if (!ml_is(Args[1], MLBufferT)) {
		switch (Degree) {
		case 1: Array->Base.Type = MLVectorTypes[Format]; break;
		case 2: Array->Base.Type = MLMatrixTypes[Format]; break;
		default: Array->Base.Type = MLArrayTypes[Format]; break;
		}
	}
	size_t Total = 0;
	for (int I = 0; I < Degree; ++I) {
		ml_value_t *SizeValue = ml_list_get(Args[2], I + 1);
		ml_value_t *StrideValue = ml_list_get(Args[3], I + 1);
		if (!ml_is(SizeValue, MLIntegerT)) return ml_error("TypeError", "Dimension is not an integer");
		if (!ml_is(StrideValue, MLIntegerT)) return ml_error("TypeError", "Stride is not an integer");
		int Size = ml_integer_value(SizeValue);
		if (Size <= 0) return ml_error("ValueError", "Dimension must be positive");
		int Stride = ml_integer_value(StrideValue);
		if (Stride <= 0) return ml_error("ValueError", "Stride must be positive");
		Total += (Size - 1) * Stride;
		Array->Dimensions[I].Size = Size;
		Array->Dimensions[I].Stride = Stride;
	}
	Total += Array->Dimensions[Degree - 1].Stride;
	if (Total > ml_address_length(Args[1])) return ml_error("ValueError", "Size larger than buffer");
	Array->Base.Value = ((ml_address_t *)Args[1])->Value;
	Array->Base.Length = Total;
	return (ml_value_t *)Array;
}

ML_METHOD("degree", MLArrayT) {
//<Array
//>integer
// Return the degree of :mini:`Array`.
//$= let A := array([[1, 2, 3], [4, 5, 6]])
//$= A:degree
	ml_array_t *Array = (ml_array_t *)Args[0];
	return ml_integer(Array->Degree);
}

ML_METHOD("shape", MLArrayT) {
//<Array
//>list
// Return the shape of :mini:`Array`.
//$= let A := array([[1, 2, 3], [4, 5, 6]])
//$= A:shape
	ml_array_t *Array = (ml_array_t *)Args[0];
	ml_value_t *Shape = ml_list();
	for (int I = 0; I < Array->Degree; ++I) {
		ml_list_put(Shape, ml_integer(Array->Dimensions[I].Size));
	}
	return Shape;
}

ML_METHOD("count", MLArrayT) {
//<Array
//>integer
// Return the number of elements in :mini:`Array`.
//$= let A := array([[1, 2, 3], [4, 5, 6]])
//$= A:count
	ml_array_t *Array = (ml_array_t *)Args[0];
	size_t Size = 1;
	for (int I = 0; I < Array->Degree; ++I) Size *= Array->Dimensions[I].Size;
	return ml_integer(Size);
}

static ml_array_t *ml_array_transpose(ml_array_t *Source) {
	int Degree = Source->Degree;
	ml_array_t *Target = ml_array_alloc(Source->Format, Degree);
	for (int I = 0; I < Degree; ++I) {
		Target->Dimensions[I] = Source->Dimensions[Degree - I - 1];
	}
	Target->Base = Source->Base;
	return Target;
}

ML_METHOD("^", MLArrayT) {
//<Array
//>array
// Returns the transpose of :mini:`Array`, sharing the underlying data.
//$= let A := array([[1, 2, 3], [4, 5, 6]])
//$= ^A
	return (ml_value_t *)ml_array_transpose((ml_array_t *)Args[0]);
}

ML_METHOD("permute", MLArrayT, MLListT) {
//<Array
//<Indices
//>array
// Returns an array sharing the underlying data with :mini:`Array`, permuting the axes according to :mini:`Indices`.
//$= let A := array([[[1, 2, 3], [4, 5, 6]], [[7, 8, 9], [10, 11, 12]]])
//$= A:shape
//$= let B := A:permute([2, 3, 1])
//$= B:shape
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	if (Degree > 64) return ml_error("ArrayError", "Not implemented for degree > 64 yet");
	if (ml_list_length(Args[1]) != Degree) return ml_error("ArrayError", "List length must match degree");
	ml_array_t *Target = ml_array_alloc(Source->Format, Degree);
	ml_array_dimension_t *TargetDimension = Target->Dimensions;
	size_t Bits = (1 << Degree) - 1;
	ML_LIST_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Value, MLIntegerT)) return ml_error("ArrayError", "Invalid index");
		int J = ml_integer_value(Iter->Value);
		if (J <= 0) J += Degree + 1;
		if (J < 1 || J > Degree) return ml_error("ArrayError", "Invalid index");
		size_t Bit = 1 << (J - 1);
		if (!(Bits & Bit)) return ml_error("ArrayError", "Invalid permutation");
		Bits -= Bit;
		*TargetDimension++ = Source->Dimensions[J - 1];
	}
	if (Bits != 0) return ml_error("ArrayError", "Invalid permutation");
	Target->Base = Source->Base;
	return (ml_value_t *)Target;
}


ML_METHODV("permute", MLArrayT, MLIntegerT) {
//<Array
//<Indices
//>array
// Returns an array sharing the underlying data with :mini:`Array`, permuting the axes according to :mini:`Indices`.
//$= let A := array([[[1, 2, 3], [4, 5, 6]], [[7, 8, 9], [10, 11, 12]]])
//$= A:shape
//$= let B := A:permute(2, 3, 1)
//$= B:shape
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	if (Degree > 64) return ml_error("ArrayError", "Not implemented for degree > 64 yet");
	if (Count - 1 != Degree) return ml_error("ArrayError", "List length must match degree");
	ml_array_t *Target = ml_array_alloc(Source->Format, Degree);
	ml_array_dimension_t *TargetDimension = Target->Dimensions;
	size_t Bits = (1 << Degree) - 1;
	for (int I = 1; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLIntegerT);
		int J = ml_integer_value(Args[I]);
		if (J <= 0) J += Degree + 1;
		if (J < 1 || J > Degree) return ml_error("ArrayError", "Invalid index");
		size_t Bit = 1 << (J - 1);
		if (!(Bits & Bit)) return ml_error("ArrayError", "Invalid permutation");
		Bits -= Bit;
		*TargetDimension++ = Source->Dimensions[J - 1];
	}
	if (Bits != 0) return ml_error("ArrayError", "Invalid permutation");
	Target->Base = Source->Base;
	return (ml_value_t *)Target;
}

ML_METHOD("swap", MLArrayT) {
//<Array
//>array
// Returns the transpose of :mini:`Array`, sharing the underlying data.
//$= let A := array([[1, 2, 3], [4, 5, 6]])
//$= A:swap
	return (ml_value_t *)ml_array_transpose((ml_array_t *)Args[0]);
}

ML_METHOD("swap", MLArrayT, MLIntegerT, MLIntegerT) {
//<Array
//<Index/1
//<Index/2
//>array
// Returns an array sharing the underlying data with :mini:`Array` with dimensions :mini:`Index/1` and :mini:`Index/2` swapped.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	int IndexA = ml_integer_value(Args[1]);
	int IndexB = ml_integer_value(Args[2]);
	if (IndexA <= 0) IndexA += (Degree + 1);
	if (IndexB <= 0) IndexB += (Degree + 1);
	if (IndexA < 1 || IndexA > Degree) return ml_error("ArrayError", "Invalid index");
	if (IndexB < 1 || IndexB > Degree) return ml_error("ArrayError", "Invalid index");
	ml_array_t *Target = ml_array_alloc(Source->Format, Degree);
	for (int I = 0; I < Degree; ++I) Target->Dimensions[I] = Source->Dimensions[I];
	Target->Dimensions[IndexA - 1] = Source->Dimensions[IndexB - 1];
	Target->Dimensions[IndexB - 1] = Source->Dimensions[IndexA - 1];
	Target->Base = Source->Base;
	return (ml_value_t *)Target;
}

ML_METHOD("expand", MLArrayT, MLListT) {
//<Array
//<Indices
//>array
// Returns an array sharing the underlying data with :mini:`Array` with additional unit-length axes at the specified :mini:`Indices`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree + ml_list_length(Args[1]);
	int Expands[Source->Degree + 1];
	for (int I = 0; I <= Source->Degree; ++I) Expands[I] = 0;
	ML_LIST_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Value, MLIntegerT)) return ml_error("ArrayError", "Invalid index");
		int J = ml_integer_value(Iter->Value);
		if (J <= 0) J += Degree + 1;
		if (J < 1 || J >= Degree + 1) return ml_error("ArrayError", "Invalid index");
		Expands[J - 1] += 1;
	}
	ml_array_t *Target = ml_array_alloc(Source->Format, Degree);
	ml_array_dimension_t *Dim = Target->Dimensions;
	for (int I = 0; I < Degree; ++I) {
		for (int J = 0; J < Expands[I]; ++J) {
			Dim->Size = 1;
			Dim->Stride = 0;
			++Dim;
		}
		*Dim++ = Source->Dimensions[I];
	}
	for (int J = 0; J < Expands[Source->Degree]; ++J) {
		Dim->Size = 1;
		Dim->Stride = 0;
		++Dim;
	}
	Target->Base = Source->Base;
	return (ml_value_t *)Target;
}

ML_METHOD("split", MLArrayT, MLIntegerT, MLListT) {
//<Array
//<Index
//<Sizes
//>array
// Returns an array sharing the underlying data with :mini:`Array` replacing the dimension at :mini:`Index` with new dimensions with sizes :mini:`Sizes`. The total count :mini:`Sizes/1 * Sizes/2 * ... * Sizes/n` must equal the original size.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	int Expand = ml_integer_value(Args[1]);
	if (Expand <= 0) Expand += Degree + 1;
	if (Expand < 1 || Expand > Degree) return ml_error("ArrayError", "Invalid index");
	--Expand;
	if (Source->Dimensions[Expand].Indices) return ml_error("ArrayError", "Cannot split indexed dimension yet");
	size_t Total = 1;
	ML_LIST_FOREACH(Args[2], Iter) {
		if (!ml_is(Iter->Value, MLIntegerT)) return ml_error("ArrayError", "Invalid size");
		int Size = ml_integer_value(Iter->Value);
		if (Size <= 0) return ml_error("RangeError", "Invalid size");
		Total *= Size;
	}
	if (Source->Dimensions[Expand].Size != Total) return ml_error("ArrayError", "Invalid size");
	ml_array_t *Target = ml_array_alloc(Source->Format, Degree + ml_list_length(Args[2]) - 1);
	ml_array_dimension_t *SourceDimension = Source->Dimensions + Degree;
	ml_array_dimension_t *TargetDimension = Target->Dimensions + Target->Degree;
	for (int I = Expand + 1; I < Degree; ++I) *--TargetDimension = *--SourceDimension;
	int Stride = (--SourceDimension)->Stride;
	ML_LIST_REVERSE(Args[2], Iter) {
		--TargetDimension;
		int Size = TargetDimension->Size = ml_integer_value(Iter->Value);
		TargetDimension->Stride = Stride;
		Stride *= Size;
	}
	for (int I = 0; I < Expand; ++I) *--TargetDimension = *--SourceDimension;
	Target->Base = Source->Base;
	return (ml_value_t *)Target;
}

ML_METHOD("join", MLArrayT, MLIntegerT, MLIntegerT) {
//<Array
//<Start
//<Count
//>array
// Returns an array sharing the underlying data with :mini:`Array` replacing the dimensions at :mini:`Start .. (Start + Count)` with a single dimension with the same overall size.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	int Start = ml_integer_value(Args[1]);
	if (Start <= 0) Start += Degree + 1;
	if (Start < 1 || Start > Degree) return ml_error("ArrayError", "Invalid index");
	int Join = ml_integer_value(Args[2]);
	if (Join <= 0) return ml_error("ArrayError", "Invalid run");
	--Start;
	int End = Start + Join - 1;
	if (End >= Degree) return ml_error("RangeError", "Invalid run");
	ml_array_t *Target = ml_array_alloc(Source->Format, Degree + 1 - Join);
	ml_array_dimension_t *SourceDimension = Source->Dimensions + Degree;
	ml_array_dimension_t *TargetDimension = Target->Dimensions + Target->Degree;
	for (int I = End + 1; I < Degree; ++I) *--TargetDimension = *--SourceDimension;
	--TargetDimension;
	TargetDimension->Stride = SourceDimension[-1].Stride;
	int Size = 1;
	for (int I = 0; I < Join; ++I) {
		--SourceDimension;
		if (SourceDimension->Indices) return ml_error("ArrayError", "Cannot join indexed dimension yet");
		Size *= SourceDimension->Size;
	}
	TargetDimension->Size = Size;
	for (int I = 0; I < Start; ++I) *--TargetDimension = *--SourceDimension;
	Target->Base = Source->Base;
	return (ml_value_t *)Target;
}

ML_METHOD("strides", MLArrayT) {
//<Array
//>list
// Return the strides of :mini:`Array` in bytes.
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
// Return the size of :mini:`Array` in contiguous bytes, or :mini:`nil` if :mini:`Array` is not contiguous.
//$= let A := array([[1, 2, 3], [4, 5, 6]])
//$= A:size
//$= let B := ^A
//$= B:size
	ml_array_t *Array = (ml_array_t *)Args[0];
	size_t Size = Array->Dimensions[Array->Degree - 1].Stride;
	for (int I = 1; I < Array->Degree; ++I) Size *= Array->Dimensions[I].Size;
	if (Array->Dimensions[0].Stride == Size) return ml_integer(Size * Array->Dimensions[0].Size);
	return MLNil;
}

extern ml_value_t *RangeMethod;
extern ml_value_t *MulMethod;
extern ml_value_t *AddMethod;
ML_METHOD_DECL(SubMethod, "-");
ML_METHOD_DECL(DivMethod, "/");

static ml_value_t *ml_array_value(ml_array_t *Array, char *Address) {
	typeof(ml_array_value) *function = ml_typed_fn_get(Array->Base.Type, ml_array_value);
	return function(Array, Address);
}

typedef struct {
	ml_type_t *Type;
	ml_array_t Array[1];
} ml_array_ref_t;

static ml_value_t *ml_array_ref_deref(ml_array_ref_t *Ref);

static void ml_array_ref_assign(ml_state_t *Caller, ml_array_ref_t *Ref, ml_value_t *Value);

ML_TYPE(MLArrayRefT, (), "array::ref",
//!internal
	.deref = (void *)ml_array_ref_deref,
	.assign = (void *)ml_array_ref_assign
);

ml_array_ref_t *ml_array_ref_alloc(ml_array_format_t Format, int Degree) {
	ml_array_ref_t *Ref = xnew(ml_array_ref_t, Degree, ml_array_dimension_t);
	Ref->Type = MLArrayRefT;
	if (Degree == 1) {
		Ref->Array->Base.Type = MLVectorMutableTypes[Format];
	} else if (Degree == 2) {
		Ref->Array->Base.Type = MLMatrixMutableTypes[Format];
	} else {
		Ref->Array->Base.Type = MLArrayMutableTypes[Format];
	}
	Ref->Array->Degree = Degree;
	Ref->Array->Format = Format;
	return Ref;
}

typedef struct {
	void *Address;
	ml_array_dimension_t *Target, *Source, *Limit;
} ml_array_indexer_t;

static ml_value_t *ml_array_index_get(ml_value_t *Index, ml_array_indexer_t *Indexer) {
	typeof(ml_array_index_get) *function = ml_typed_fn_get(ml_typeof(Index), ml_array_index_get);
	if (!function) return ml_error("TypeError", "Unknown index type: %s", ml_typeof(Index)->Name);
	return function(Index, Indexer);
}

static ml_value_t *ML_TYPED_FN(ml_array_index_get, MLNilT, ml_value_t *Index, ml_array_indexer_t *Indexer) {
	*Indexer->Target = *Indexer->Source;
	++Indexer->Target;
	++Indexer->Source;
	return NULL;
}

static ml_value_t *ml_array_nil_deref(ml_value_t *Array) {
	return MLNil;
}

static void ml_array_nil_assign(ml_state_t *Caller, ml_value_t *Array, ml_value_t *Value) {
	ML_RETURN(Value);
}

ML_TYPE(MLArrayNilT, (MLArrayT), "array::nil",
//!internal
	.deref = (void *)ml_array_nil_deref,
	.assign = (void *)ml_array_nil_assign
);

static ml_array_t MLArrayNil[1] = {{
	{MLArrayNilT, NULL, 0}, -1, ML_ARRAY_FORMAT_NONE
}};

static ml_value_t *ML_TYPED_FN(ml_array_index_get, MLIntegerT, ml_value_t *Index, ml_array_indexer_t *Indexer) {
	int IndexValue = ml_integer_value_fast(Index);
	if (IndexValue <= 0) IndexValue += Indexer->Source->Size + 1;
	if (--IndexValue < 0) return (ml_value_t *)MLArrayNil;
	if (IndexValue >= Indexer->Source->Size) return (ml_value_t *)MLArrayNil;
	if (Indexer->Source->Indices) IndexValue = Indexer->Source->Indices[IndexValue];
	Indexer->Address += Indexer->Source->Stride * IndexValue;
	++Indexer->Source;
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_array_index_get, MLListT, ml_value_t *Index, ml_array_indexer_t *Indexer) {
	int Count = Indexer->Target->Size = ml_list_length(Index);
	if (!Count) return (ml_value_t *)MLArrayNil;
	int *Indices = Indexer->Target->Indices = (int *)snew(Count * sizeof(int));
	int *IndexPtr = Indices;
	ml_value_t *Index0 = ((ml_list_t *)Index)->Head->Value;
	if (ml_is(Index0, MLTupleT)) {
		int Size = ((ml_tuple_t *)Index0)->Size;
		if (Indexer->Source + Size > Indexer->Limit) return ml_error("RangeError", "Too many indices");
		ML_LIST_FOREACH(Index, Iter) {
			if (((ml_tuple_t *)Index0)->Size != Size) return ml_error("ShapeError", "Inconsistent tuple size");
			ml_value_t **Values = ((ml_tuple_t *)Iter->Value)->Values;
			ml_array_dimension_t *Source = Indexer->Source;
			void *Address = Indexer->Address;
			for (int I = 0; I < Size; ++I) {
				int IndexValue = ml_integer_value(Values[I]);
				if (IndexValue <= 0) IndexValue += Source->Size + 1;
				if (--IndexValue < 0) return (ml_value_t *)MLArrayNil;
				if (IndexValue >= Source->Size) return (ml_value_t *)MLArrayNil;
				if (Source->Indices) IndexValue = Source->Indices[IndexValue];
				Address += IndexValue * Source->Stride;
				++Source;
			}
			*IndexPtr++ = Address - Indexer->Address;
		}
		int First = Indices[0];
		for (int I = 0; I < Count; ++I) Indices[I] -= First;
		Indexer->Target->Stride = 1;
		Indexer->Address += First;
		Indexer->Source += Size;
	} else {
		if (Indexer->Source->Indices) {
			ML_LIST_FOREACH(Index, Iter) {
				int IndexValue = ml_integer_value(Iter->Value);
				if (IndexValue <= 0) IndexValue += Indexer->Source->Size + 1;
				if (--IndexValue < 0) return (ml_value_t *)MLArrayNil;
				if (IndexValue >= Indexer->Source->Size) return (ml_value_t *)MLArrayNil;
				*IndexPtr++ = Indexer->Source->Indices[IndexValue];
			}
		} else {
			ML_LIST_FOREACH(Index, Iter) {
				int IndexValue = ml_integer_value(Iter->Value);
				if (IndexValue <= 0) IndexValue += Indexer->Source->Size + 1;
				if (--IndexValue < 0) return (ml_value_t *)MLArrayNil;
				if (IndexValue >= Indexer->Source->Size) return (ml_value_t *)MLArrayNil;
				*IndexPtr++ = IndexValue;
			}
		}
		int First = Indices[0];
		for (int I = 0; I < Count; ++I) Indices[I] -= First;
		Indexer->Target->Stride = Indexer->Source->Stride;
		Indexer->Address += Indexer->Source->Stride * First;
		++Indexer->Source;
	}
	++Indexer->Target;
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_array_index_get, MLIntegerRangeT, ml_integer_range_t *Range, ml_array_indexer_t *Indexer) {
	int Min = Range->Start;
	int Max = Range->Limit;
	int Step = Range->Step;
	if (Min < 1) Min += Indexer->Source->Size + 1;
	if (Max < 1) Max += Indexer->Source->Size + 1;
	if (--Min < 0) return (ml_value_t *)MLArrayNil;
	if (Min >= Indexer->Source->Size) return (ml_value_t *)MLArrayNil;
	if (--Max < 0) return (ml_value_t *)MLArrayNil;
	if (Max >= Indexer->Source->Size) return (ml_value_t *)MLArrayNil;
	if (Step == 0) return (ml_value_t *)MLArrayNil;
	int Size = Indexer->Target->Size = (Max - Min) / Step + 1;
	if (Size < 0) return (ml_value_t *)MLArrayNil;
	if (Indexer->Source->Indices) {
		int *Indices = Indexer->Target->Indices = (int *)snew(Size * sizeof(int));
		int *IndexPtr = Indices;
		for (int I = Min; I <= Max; I += Step) {
			*IndexPtr++ = Indexer->Source->Indices[I];
		}
		Indexer->Target->Stride = Indexer->Source->Stride;
	} else {
		Indexer->Target->Indices = 0;
		Indexer->Address += Indexer->Source->Stride * Min;
		Indexer->Target->Stride = Indexer->Source->Stride * Step;
	}
	++Indexer->Target;
	++Indexer->Source;
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_array_index_get, MLTupleT, ml_tuple_t *Tuple, ml_array_indexer_t *Indexer) {
	int Size = Tuple->Size;
	if (Indexer->Source + Size > Indexer->Limit) return ml_error("RangeError", "Too many indices");
	ml_value_t **TupleValues = Tuple->Values;
	for (int I = 0; I < Size; ++I) {
		if (!ml_is(TupleValues[I], MLIntegerT)) return ml_error("TypeError", "Expected integer in tuple index");
		int IndexValue = ml_integer_value(TupleValues[I]);
		if (IndexValue <= 0) IndexValue += Indexer->Source->Size + 1;
		if (--IndexValue < 0) return (ml_value_t *)MLArrayNil;
		if (IndexValue >= Indexer->Source->Size) return (ml_value_t *)MLArrayNil;
		if (Indexer->Source->Indices) IndexValue = Indexer->Source->Indices[IndexValue];
		Indexer->Address += Indexer->Source->Stride * IndexValue;
		++Indexer->Source;
	}
	return NULL;
}

#define ARRAY_OFFSETS_NONZERO(CTYPE, ZERO) \
\
static int ml_array_count_nonzero_ ## CTYPE(void *Address, int Degree, ml_array_dimension_t *Dimension) { \
	int Count = 0; \
	if (!Degree) { \
		int Size = Dimension->Size; \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				if (*(CTYPE *)(Address + Indices[I] * Stride) != ZERO) ++Count; \
			} \
		} else { \
			for (int I = 0; I < Size; ++I) { \
				if (*(CTYPE *)(Address + I * Stride) != ZERO) ++Count; \
			} \
		} \
	} else { \
		int Size = Dimension->Size; \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices;\
			for (int I = 0; I < Size; ++I) { \
				Count += ml_array_count_nonzero_ ## CTYPE(Address + Indices[I] * Stride, Degree - 1, Dimension + 1); \
			} \
		} else { \
			for (int I = 0; I < Size; ++I) { \
				Count += ml_array_count_nonzero_ ## CTYPE(Address + I * Stride, Degree - 1, Dimension + 1); \
			} \
		} \
	} \
	return Count; \
} \
\
static int *ml_array_offsets_nonzero_ ## CTYPE(int *Offsets, void *Address, int Degree, ml_array_dimension_t *Dimension, int Offset, ml_array_dimension_t *Source) { \
	if (!Degree) { \
		int Size = Dimension->Size; \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				if (*(CTYPE *)(Address + Indices[I] * Stride) != ZERO) *Offsets++ = Offset + Indices[I] * Source->Stride; \
			} \
		} else { \
			for (int I = 0; I < Size; ++I) { \
				if (*(CTYPE *)(Address + I * Stride) != ZERO) *Offsets++ = Offset + Source->Stride * I; \
			} \
		} \
	} else { \
		int Size = Dimension->Size; \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices;\
			for (int I = 0; I < Size; ++I) { \
				Offsets = ml_array_offsets_nonzero_ ## CTYPE(Offsets, Address + Indices[I] * Stride, Degree - 1, Dimension + 1, Offset + Indices[I] * Source->Stride, Source + 1); \
			} \
		} else { \
			for (int I = 0; I < Size; ++I) { \
				Offsets = ml_array_offsets_nonzero_ ## CTYPE(Offsets, Address + I * Stride, Degree - 1, Dimension + 1, Offset + I * Source->Stride, Source + 1); \
			} \
		} \
	} \
	return Offsets; \
}

ARRAY_OFFSETS_NONZERO(uint8_t, 0);
ARRAY_OFFSETS_NONZERO(int8_t, 0);
ARRAY_OFFSETS_NONZERO(uint16_t, 0);
ARRAY_OFFSETS_NONZERO(int16_t, 0);
ARRAY_OFFSETS_NONZERO(uint32_t, 0);
ARRAY_OFFSETS_NONZERO(int32_t, 0);
ARRAY_OFFSETS_NONZERO(uint64_t, 0);
ARRAY_OFFSETS_NONZERO(int64_t, 0);
ARRAY_OFFSETS_NONZERO(float, 0);
ARRAY_OFFSETS_NONZERO(double, 0);

#ifdef ML_COMPLEX

ARRAY_OFFSETS_NONZERO(complex_float, 0);
ARRAY_OFFSETS_NONZERO(complex_double, 0);

#endif

ARRAY_OFFSETS_NONZERO(any, MLNil);

static int ml_array_count_nonzero(ml_array_t *A) {
	switch (A->Format) {
	case ML_ARRAY_FORMAT_U8:
		return ml_array_count_nonzero_uint8_t(A->Base.Value, A->Degree - 1, A->Dimensions);
	case ML_ARRAY_FORMAT_I8:
		return ml_array_count_nonzero_int8_t(A->Base.Value, A->Degree - 1, A->Dimensions);
	case ML_ARRAY_FORMAT_U16:
		return ml_array_count_nonzero_uint16_t(A->Base.Value, A->Degree - 1, A->Dimensions);
	case ML_ARRAY_FORMAT_I16:
		return ml_array_count_nonzero_int16_t(A->Base.Value, A->Degree - 1, A->Dimensions);
	case ML_ARRAY_FORMAT_U32:
		return ml_array_count_nonzero_uint32_t(A->Base.Value, A->Degree - 1, A->Dimensions);
	case ML_ARRAY_FORMAT_I32:
		return ml_array_count_nonzero_int32_t(A->Base.Value, A->Degree - 1, A->Dimensions);
	case ML_ARRAY_FORMAT_U64:
		return ml_array_count_nonzero_uint64_t(A->Base.Value, A->Degree - 1, A->Dimensions);
	case ML_ARRAY_FORMAT_I64:
		return ml_array_count_nonzero_int64_t(A->Base.Value, A->Degree - 1, A->Dimensions);
	case ML_ARRAY_FORMAT_F32:
		return ml_array_count_nonzero_float(A->Base.Value, A->Degree - 1, A->Dimensions);
	case ML_ARRAY_FORMAT_F64:
		return ml_array_count_nonzero_double(A->Base.Value, A->Degree - 1, A->Dimensions);
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
		return ml_array_count_nonzero_complex_float(A->Base.Value, A->Degree - 1, A->Dimensions);
	case ML_ARRAY_FORMAT_C64:
		return ml_array_count_nonzero_complex_double(A->Base.Value, A->Degree - 1, A->Dimensions);
#endif
	case ML_ARRAY_FORMAT_ANY:
		return ml_array_count_nonzero_any(A->Base.Value, A->Degree - 1, A->Dimensions);
	default:
		return 0;
	}
}

static int *ml_array_offsets_nonzero(ml_array_t *A, int *Offsets, ml_array_dimension_t *Source) {
	switch (A->Format) {
	case ML_ARRAY_FORMAT_U8:
		return ml_array_offsets_nonzero_uint8_t(Offsets, A->Base.Value, A->Degree - 1, A->Dimensions, 0, Source);
	case ML_ARRAY_FORMAT_I8:
		return ml_array_offsets_nonzero_int8_t(Offsets, A->Base.Value, A->Degree - 1, A->Dimensions, 0, Source);
	case ML_ARRAY_FORMAT_U16:
		return ml_array_offsets_nonzero_uint16_t(Offsets, A->Base.Value, A->Degree - 1, A->Dimensions, 0, Source);
	case ML_ARRAY_FORMAT_I16:
		return ml_array_offsets_nonzero_int16_t(Offsets, A->Base.Value, A->Degree - 1, A->Dimensions, 0, Source);
	case ML_ARRAY_FORMAT_U32:
		return ml_array_offsets_nonzero_uint32_t(Offsets, A->Base.Value, A->Degree - 1, A->Dimensions, 0, Source);
	case ML_ARRAY_FORMAT_I32:
		return ml_array_offsets_nonzero_int32_t(Offsets, A->Base.Value, A->Degree - 1, A->Dimensions, 0, Source);
	case ML_ARRAY_FORMAT_U64:
		return ml_array_offsets_nonzero_uint64_t(Offsets, A->Base.Value, A->Degree - 1, A->Dimensions, 0, Source);
	case ML_ARRAY_FORMAT_I64:
		return ml_array_offsets_nonzero_int64_t(Offsets, A->Base.Value, A->Degree - 1, A->Dimensions, 0, Source);
	case ML_ARRAY_FORMAT_F32:
		return ml_array_offsets_nonzero_float(Offsets, A->Base.Value, A->Degree - 1, A->Dimensions, 0, Source);
	case ML_ARRAY_FORMAT_F64:
		return ml_array_offsets_nonzero_double(Offsets, A->Base.Value, A->Degree - 1, A->Dimensions, 0, Source);
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
		return ml_array_offsets_nonzero_complex_float(Offsets, A->Base.Value, A->Degree - 1, A->Dimensions, 0, Source);
	case ML_ARRAY_FORMAT_C64:
		return ml_array_offsets_nonzero_complex_double(Offsets, A->Base.Value, A->Degree - 1, A->Dimensions, 0, Source);
#endif
	case ML_ARRAY_FORMAT_ANY:
		return ml_array_offsets_nonzero_any(Offsets, A->Base.Value, A->Degree - 1, A->Dimensions, 0, Source);
	default:
		return 0;
	}
}

static ml_value_t *ML_TYPED_FN(ml_array_index_get, MLArrayMutableInt8T, ml_array_t *Array, ml_array_indexer_t *Indexer) {
	int Degree = Array->Degree;
	if (Indexer->Source + Degree > Indexer->Limit) return ml_error("RangeError", "Too many indices");
	for (int I = 0; I < Degree; ++I) {
		if (Array->Dimensions[I].Size != Indexer->Source[I].Size) {
			return ml_error("ShapeError", "Array sizes do not match");
		}
	}
	int Count = Indexer->Target->Size = ml_array_count_nonzero(Array);
	if (!Count) return (ml_value_t *)MLArrayNil;
	int *Indices = Indexer->Target->Indices = (int *)snew(Count * sizeof(int));
	ml_array_offsets_nonzero(Array, Indices, Indexer->Source);
	int First = Indices[0];
	for (int I = 0; I < Count; ++I) Indices[I] -= First;
	Indexer->Target->Stride = 1;
	Indexer->Address += First;
	++Indexer->Target;
	Indexer->Source += Degree;
	return NULL;
}

static int *ml_array_to_indices(int *Indices, int Degree, ml_array_dimension_t *TargetDimension, int Offset, ml_array_dimension_t *IndexDimension, void *IndexData) {
	if (Degree == 0) {
		if (IndexDimension->Indices) {
			int *IndexIndices = IndexDimension->Indices;
			for (int I = 0; I < IndexDimension->Size; ++I) {
				int N = *(int32_t *)(IndexData + IndexIndices[I] * IndexDimension->Stride) - 1;
				if (N < 0 || N >= TargetDimension[I].Size) return NULL;
				Offset += N * TargetDimension[I].Stride;
			}
		} else {
			for (int I = 0; I < IndexDimension->Size; ++I) {
				int N = *(int32_t *)(IndexData + I * IndexDimension->Stride) - 1;
				if (N < 0 || N >= TargetDimension[I].Size) return NULL;
				Offset += N * TargetDimension[I].Stride;
			}
		}
		*Indices++ = Offset;
	} else {
		if (IndexDimension->Indices) {
			int *IndexIndices = IndexDimension->Indices;
			if (TargetDimension->Indices) {
				int *TargetIndices = TargetDimension->Indices;
				for (int I = 0; I < IndexDimension->Size; ++I) {
					Indices = ml_array_to_indices(Indices, Degree - 1,
						TargetDimension + 1,
						Offset + TargetIndices[I] * TargetDimension->Stride,
						IndexDimension + 1,
						IndexData +  IndexIndices[I] * IndexDimension->Stride
					);
					if (!Indices) return Indices;
				}
			} else {
				for (int I = 0; I < IndexDimension->Size; ++I) {
					Indices = ml_array_to_indices(Indices, Degree - 1,
						TargetDimension + 1,
						Offset + I * TargetDimension->Stride,
						IndexDimension + 1,
						IndexData +  IndexIndices[I] * IndexDimension->Stride
					);
					if (!Indices) return Indices;
				}
			}
		} else {
			if (TargetDimension->Indices) {
				int *TargetIndices = TargetDimension->Indices;
				for (int I = 0; I < IndexDimension->Size; ++I) {
					Indices = ml_array_to_indices(Indices, Degree - 1,
						TargetDimension + 1,
						Offset + TargetIndices[I] * TargetDimension->Stride,
						IndexDimension + 1,
						IndexData +  I * IndexDimension->Stride
					);
					if (!Indices) return Indices;
				}
			} else {
				for (int I = 0; I < IndexDimension->Size; ++I) {
					Indices = ml_array_to_indices(Indices, Degree - 1,
						TargetDimension + 1,
						Offset + I * TargetDimension->Stride,
						IndexDimension + 1,
						IndexData +  I * IndexDimension->Stride
					);
					if (!Indices) return Indices;
				}
			}
		}
	}
	return Indices;
}

static ml_value_t *ML_TYPED_FN(ml_array_index_get, MLArrayMutableInt32T, ml_array_t *Array, ml_array_indexer_t *Indexer) {
	int Degree = Array->Degree - 1;
	int Total = Degree + Array->Dimensions[Degree].Size;
	if (Indexer->Source + Total > Indexer->Limit) return ml_error("RangeError", "Too many indices");
	int Count = 1;
	for (int I = 0; I < Degree; ++I) {
		if (Array->Dimensions[I].Size != Indexer->Source[I].Size) {
			return ml_error("ShapeError", "Array sizes do not match");
		}
		Count *= Array->Dimensions[I].Size;
	}
	Indexer->Target->Size = Count;
	int *Indices = Indexer->Target->Indices = (int *)snew(Count * sizeof(int));
	if (!ml_array_to_indices(Indices, Degree, Indexer->Source, 0, Array->Dimensions, Array->Base.Value)) {
		return ml_error("IndexError", "Index out of bounds");
	}
	int First = Indices[0];
	for (int I = 0; I < Count; ++I) Indices[I] -= First;
	Indexer->Target->Stride = 1;
	Indexer->Address += First;
	++Indexer->Target;
	Indexer->Source += Total;
	return NULL;
}

ml_value_t *ml_array_index(ml_array_t *Source, int Count, ml_value_t **Indices) {
	ml_array_dimension_t TargetDimensions[Source->Degree];
	ml_array_indexer_t Indexer[1] = {{
		Source->Base.Value, TargetDimensions,
		Source->Dimensions, Source->Dimensions + Source->Degree
	}};
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Index = Indices[I];
		if (Index == RangeMethod) {
			ml_array_dimension_t *Skip = Indexer->Limit - (Count - (I + 1));
			if (Skip > Indexer->Limit) return ml_error("RangeError", "Too many indices");
			while (Indexer->Source < Skip) {
				*Indexer->Target = *Indexer->Source;
				++Indexer->Target;
				++Indexer->Source;
			}
		} else if (Index == MulMethod) {
			if (Indexer->Source >= Indexer->Limit) return ml_error("RangeError", "Too many indices");
			*Indexer->Target = *Indexer->Source;
			++Indexer->Target;
			++Indexer->Source;
		} else {
			if (Indexer->Source >= Indexer->Limit) return ml_error("RangeError", "Too many indices");
			ml_value_t *Result = ml_array_index_get(Index, Indexer);
			if (Result) return Result;
		}
	}
	while (Indexer->Source < Indexer->Limit) {
		*Indexer->Target = *Indexer->Source;
		++Indexer->Target;
		++Indexer->Source;
	}
	int Degree = Indexer->Target - TargetDimensions;
	if (ml_is_subtype(Source->Base.Type, MLArrayMutableT)) {
		ml_array_ref_t *Ref = ml_array_ref_alloc(Source->Format, Degree);
		for (int I = 0; I < Degree; ++I) Ref->Array->Dimensions[I] = TargetDimensions[I];
		Ref->Array->Base.Value = Indexer->Address;
		return (ml_value_t *)Ref;
	} else {
		ml_array_t *Val = ml_array_alloc(Source->Format, Degree);
		for (int I = 0; I < Degree; ++I) Val->Dimensions[I] = TargetDimensions[I];
		Val->Base.Value = Indexer->Address;
		return (ml_value_t *)Val;
	}
}

ML_METHODV("[]", MLArrayT) {
//<Array
//<Index/1
//>array
// Returns a sub-array of :mini:`Array` sharing the underlying data, indexed by :mini:`Index/i`.
// Dimensions are copied to the output array, applying the indices as follows:
//
// * If :mini:`Index/i` is :mini:`nil` or :mini:`*` then the next dimension is copied unchanged.
//
// * If :mini:`Index/i` is :mini:`..` then the remaining indices are applied to the last dimensions of :mini:`Array` and the dimensions in between are copied unchanged.
//
// * If :mini:`Index/i` is an :mini:`integer` then the :mini:`Index/i`-th value of the next dimension is selected and the dimension is dropped from the output.
//
// * If :mini:`Index/i` is an :mini:`integer::range` then the corresponding slice of the next dimension is copied to the output.
//
// * If :mini:`Index/i` is a :mini:`tuple[integer, ...]` then the next dimensions are indexed by the corresponding integer in turn (i.e. :mini:`A[(I, J, K)]` gives the same result as :mini:`A[I, J, K]`).
//
// * If :mini:`Index/i` is a :mini:`list[integer]` then the next dimension is copied as a sparse dimension with the respective entries.
//
// * If :mini:`Index/i` is a :mini:`list[tuple[integer, ...]]` then the appropriate dimensions are dropped and a single sparse dimension is added with the corresponding entries.
//
// * If :mini:`Index/i` is an :mini:`array::int8` with dimensions matching the corresponding dimensions of :mini:`A` then a sparse dimension is added with entries corresponding to the non-zero values in :mini:`Index/i` (i.e. :mini:`A[B]` is equivalent to :mini:`A[B:where]`).
// * If :mini:`Index/i` is an :mini:`array::int32` with all but last dimensions matching the corresponding dimensions of :mini:`A` then a sparse dimension is added with entries corresponding indices in the last dimension of :mini:`Index/i`.
//
// If fewer than :mini:`A:degree` indices are provided then the remaining dimensions are copied unchanged.
//$- let A := array([[[19, 16, 12], [4, 7, 20]], [[5, 17, 8], [20, 9, 20]]])
//$= A[1]
//$= A[1, 2]
//$= A[1, 2, 3]
//$= A[nil, 2]
//$= A[.., 3]
//$= A[.., 1 .. 2]
//$= A[(1, 2, 3)]
//$= A[[(1, 2, 3), (2, 1, 1)]]
//$= let B := A > 10
//$= type(B)
//$= A[B]
//$= let C := A:maxidx(2)
//$= type(C)
//$= A[C]
	ml_array_t *Source = (ml_array_t *)Args[0];
	return ml_array_index(Source, Count - 1, Args + 1);
}

ML_METHOD("[]", MLArrayT, MLMapT) {
//<Array
//<Indices
//>array
// Returns a sub-array of :mini:`Array` sharing the underlying data.
// The :mini:`i`-th dimension is indexed by :mini:`Indices[i]` if present, and :mini:`nil` otherwise.
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
	return ml_array_index(Source, Degree, Indices);
}

static char *ml_array_indexv(ml_array_t *Array, va_list Indices) {
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

typedef struct {
	char *Value;
	int *Indices;
	int Size, Stride, Index;
} ml_array_iter_dim_t;

typedef ml_value_t *(*ml_array_iter_val_t)(char *);

#define ML_ARRAY_ITER_FN(CTYPE, TO_VAL) \
\
static ml_value_t *ml_array_iter_val_ ## CTYPE(char *Address) { \
	return TO_VAL(*(CTYPE *)Address); \
}

ML_ARRAY_ITER_FN(uint8_t, ml_integer);
ML_ARRAY_ITER_FN(int8_t, ml_integer);
ML_ARRAY_ITER_FN(uint16_t, ml_integer);
ML_ARRAY_ITER_FN(int16_t, ml_integer);
ML_ARRAY_ITER_FN(uint32_t, ml_integer);
ML_ARRAY_ITER_FN(int32_t, ml_integer);
ML_ARRAY_ITER_FN(uint64_t, ml_integer);
ML_ARRAY_ITER_FN(int64_t, ml_integer);
ML_ARRAY_ITER_FN(float, ml_real);
ML_ARRAY_ITER_FN(double, ml_real);

#ifdef ML_COMPLEX
ML_ARRAY_ITER_FN(complex_float, ml_complex);
ML_ARRAY_ITER_FN(complex_double, ml_complex);
#endif

static ml_value_t *ml_array_iter_val_any(char *Address) {
	return *(ml_value_t **)Address;
}

typedef struct {
	ml_type_t *Type;
	char *Address;
	ml_array_iter_val_t ToVal;
	int Degree;
	ml_array_iter_dim_t Dimensions[];
} ml_array_iterator_t;

ML_TYPE(MLArrayIteratorT, (), "array-iterator");
//!internal

static void ML_TYPED_FN(ml_iter_next, MLArrayIteratorT, ml_state_t *Caller, ml_array_iterator_t *Iterator) {
	int I = Iterator->Degree;
	ml_array_iter_dim_t *Dimensions = Iterator->Dimensions;
	for (;;) {
		if (--I < 0) ML_RETURN(MLNil);
		if (++Dimensions[I].Index < Dimensions[I].Size) {
			char *Address = Dimensions[I].Value;
			if (Dimensions[I].Indices) {
				Address += Dimensions[I].Indices[Dimensions[I].Index] * Dimensions[I].Stride;
			} else {
				Address += Dimensions[I].Index * Dimensions[I].Stride;
			}
			for (int J = I + 1; J < Iterator->Degree; ++J) {
				Dimensions[J].Value = Address;
				Dimensions[J].Index = 0;
			}
			Iterator->Address = Address;
			ML_RETURN(Iterator);
		}
	}
}

static void ML_TYPED_FN(ml_iter_value, MLArrayIteratorT, ml_state_t *Caller, ml_array_iterator_t *Iterator) {
	ML_RETURN(Iterator->ToVal(Iterator->Address));
}

static void ML_TYPED_FN(ml_iter_key, MLArrayIteratorT, ml_state_t *Caller, ml_array_iterator_t *Iterator) {
	if (Iterator->Degree == 1) ML_RETURN(ml_integer(Iterator->Dimensions[0].Index + 1));
	ml_value_t *Tuple = ml_tuple(Iterator->Degree);
	for (int I = 0; I < Iterator->Degree; ++I) {
		ml_tuple_set(Tuple, I + 1, ml_integer(Iterator->Dimensions[I].Index + 1));
	}
	ML_RETURN(Tuple);
}

static void ML_TYPED_FN(ml_iterate, MLArrayT, ml_state_t *Caller, ml_array_t *Array) {
	ml_array_iterator_t *Iterator = xnew(ml_array_iterator_t, Array->Degree, ml_array_iter_dim_t);
	Iterator->Type = MLArrayIteratorT;
	Iterator->Address = Array->Base.Value;
	Iterator->Degree = Array->Degree;
	switch (Array->Format) {
	case ML_ARRAY_FORMAT_U8:
		Iterator->ToVal = ml_array_iter_val_uint8_t;
		break;
	case ML_ARRAY_FORMAT_I8:
		Iterator->ToVal = ml_array_iter_val_int8_t;
		break;
	case ML_ARRAY_FORMAT_U16:
		Iterator->ToVal = ml_array_iter_val_uint16_t;
		break;
	case ML_ARRAY_FORMAT_I16:
		Iterator->ToVal = ml_array_iter_val_int16_t;
		break;
	case ML_ARRAY_FORMAT_U32:
		Iterator->ToVal = ml_array_iter_val_uint32_t;
		break;
	case ML_ARRAY_FORMAT_I32:
		Iterator->ToVal = ml_array_iter_val_int32_t;
		break;
	case ML_ARRAY_FORMAT_U64:
		Iterator->ToVal = ml_array_iter_val_uint64_t;
		break;
	case ML_ARRAY_FORMAT_I64:
		Iterator->ToVal = ml_array_iter_val_int64_t;
		break;
	case ML_ARRAY_FORMAT_F32:
		Iterator->ToVal = ml_array_iter_val_float;
		break;
	case ML_ARRAY_FORMAT_F64:
		Iterator->ToVal = ml_array_iter_val_double;
		break;
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
		Iterator->ToVal = ml_array_iter_val_complex_float;
		break;
	case ML_ARRAY_FORMAT_C64:
		Iterator->ToVal = ml_array_iter_val_complex_double;
		break;
#endif
	case ML_ARRAY_FORMAT_ANY:
		Iterator->ToVal = ml_array_iter_val_any;
		break;
	default:
		ML_ERROR("TypeError", "Invalid array type for iteration");
	}
	for (int I = 0; I < Array->Degree; ++I) {
		Iterator->Dimensions[I].Size = Array->Dimensions[I].Size;
		Iterator->Dimensions[I].Stride = Array->Dimensions[I].Stride;
		Iterator->Dimensions[I].Indices = Array->Dimensions[I].Indices;
		Iterator->Dimensions[I].Index = 0;
		Iterator->Dimensions[I].Value = Array->Base.Value;
	}
	ML_RETURN(Iterator);
}

typedef void (*update_row_fn_t)(ml_array_dimension_t *TargetDimension, char *TargetData, ml_array_dimension_t *SourceDimension, char *SourceData);

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

static void update_array(update_row_fn_t Update, ml_array_dimension_t *TargetDimension, char *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, char *SourceData) {
	if (SourceDegree == 0) {
		ml_array_dimension_t ConstantDimension[1] = {{TargetDimension->Size, 0, NULL}};
		return Update(TargetDimension, TargetData, ConstantDimension, SourceData);
	}
	if (SourceDegree == 1) {
		return Update(TargetDimension, TargetData, SourceDimension, SourceData);
	}
	int Size = TargetDimension->Size;
	if (TargetDimension->Indices) {
		int *TargetIndices = TargetDimension->Indices;
		if (SourceDimension->Indices) {
			int *SourceIndices = SourceDimension->Indices;
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
			int *SourceIndices = SourceDimension->Indices;
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

static void update_prefix(update_row_fn_t Update, int PrefixDegree, ml_array_dimension_t *TargetDimension, char *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, char *SourceData) {
	if (PrefixDegree == 0) return update_array(Update, TargetDimension, TargetData, SourceDegree, SourceDimension, SourceData);
	int Size = TargetDimension->Size;
	if (TargetDimension->Indices) {
		int *TargetIndices = TargetDimension->Indices;
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

static ml_value_t *ml_array_cat(int Axis, int Count, ml_value_t **Args) {
	ml_array_t *A = (ml_array_t *)Args[0];
	int Degree = A->Degree;
	if (Axis >= A->Degree) return ml_error("RangeError", "Invalid axis");
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
	void *Value = C->Base.Value = snew(Stride);
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
	if (Axis < 0) return ml_error("RangeError", "Invalid axis");
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

#define UPDATE_METHOD(TITLE, NAME, ATYPE, CTYPE, FROM_VAL, FORMAT) \
\
 ML_METHOD(#NAME, ATYPE, MLNumberT) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	CTYPE Value = FROM_VAL(Args[1]); \
	ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}}; \
	update_row_fn_t Update = Update ## TITLE ## RowFns[Array->Format * MAX_FORMATS + FORMAT]; \
	if (!Update) return ml_error("ArrayError", "Unsupported array format pair (%s, %s)", ml_typeof(Args[0])->Name, ml_typeof(Args[1])->Name); \
	if (Array->Degree == 0) { \
		Update(ValueDimension, Array->Base.Value, ValueDimension, (char *)&Value); \
	} else { \
		update_prefix(Update, Array->Degree - 1, Array->Dimensions, Array->Base.Value, 0, ValueDimension, (char *)&Value); \
	} \
	return Args[0]; \
}

#define UPDATE_METHODS(ATYPE, CTYPE, FROM_VAL, FORMAT) \
UPDATE_METHOD(Set, set, ATYPE, CTYPE, FROM_VAL, FORMAT); \
UPDATE_METHOD(Add, add, ATYPE, CTYPE, FROM_VAL, FORMAT); \
UPDATE_METHOD(Sub, sub, ATYPE, CTYPE, FROM_VAL, FORMAT); \
UPDATE_METHOD(Mul, mul, ATYPE, CTYPE, FROM_VAL, FORMAT); \
UPDATE_METHOD(Div, div, ATYPE, CTYPE, FROM_VAL, FORMAT);

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
		int *LeftIndices = LeftDimension->Indices;
		if (RightDimension->Indices) {
			int *RightIndices = RightDimension->Indices;
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
			int *RightIndices = RightDimension->Indices;
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
		int *LeftIndices = LeftDimension->Indices;
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

#define COMPARE_METHOD(OP) \
/*
ML_METHOD(#OP, MLArrayT, MLArrayT) {
//<A
//<B
//>array
// Returns :mini:`A OP B` (element-wise). The shapes of :mini:`A` and :mini:`B` must be compatible, i.e. either
//
// * :mini:`A:shape = B:shape` or
// * :mini:`B:shape` is a prefix of :mini:`A:shape`.
//
// When the shapes are not the same, remaining dimensions are repeated (broadcast) to the required size.
//$= let A := array([[1, 8, 3], [4, 5, 12]])
//$= let B := array([[7, 2, 9], [4, 11, 6]])
//$= let C := array([1, 5, 10])
//$= A OP B
//$= A OP C
}
*/

COMPARE_METHOD(=)
COMPARE_METHOD(!=)
COMPARE_METHOD(<)
COMPARE_METHOD(<=)
COMPARE_METHOD(>)
COMPARE_METHOD(>=)

extern int ml_array_compare(ml_array_t *A, ml_array_t *B);

ML_METHOD("<>", MLArrayT, MLArrayT) {
//<A
//<B
//>integer
// Compare the degrees, dimensions and entries of  :mini:`A` and :mini:`B` and returns :mini:`-1`, :mini:`0` or :mini:`1`. This method is only intending for sorting arrays or using them as keys in a map.
	ml_array_t *A = (ml_array_t *)Args[0];
	ml_array_t *B = (ml_array_t *)Args[1];
	return ml_integer(ml_array_compare(A, B));
}

static long srotl(long X, unsigned int N) {
	const unsigned int Mask = (CHAR_BIT * sizeof(long) - 1);
	return (X << (N & Mask)) | (X >> ((-N) & Mask ));
}

#define BUFFER_APPEND(BUFFER, PRINTF, VALUE) ml_stringbuffer_simple_append(BUFFER, VALUE)

#ifdef ML_COMPLEX

#define COMPLEX_APPEND(BUFFER, PRINTF, VALUE) { \
	double Real = creal(VALUE), Imag = cimag(VALUE); \
	if (Imag < 0) { \
		ml_stringbuffer_printf(BUFFER, PRINTF " - " PRINTF "i", Real, -Imag); \
	} else { \
		ml_stringbuffer_printf(BUFFER, PRINTF " + " PRINTF "i", Real, Imag); \
	} \
}

#endif

#define ml_number(X) _Generic(X, ml_value_t *: ml_nop, double: ml_real, default: ml_integer)(X)

#define ml_number_value(T, X) _Generic(T, double: ml_real_value, default: ml_integer_value)(X)

#ifdef ML_COMPLEX

#define ml_array_get0_complex(FROM_NUM) \
	case ML_ARRAY_FORMAT_C32: return FROM_NUM(*(complex_float *)Address); \
	case ML_ARRAY_FORMAT_C64: return FROM_NUM(*(complex_double *)Address);

#define ml_array_set_complex(TO_NUM) \
	case ML_ARRAY_FORMAT_C32: *(complex_float *)Address = TO_NUM((complex_float)0, Value); break; \
	case ML_ARRAY_FORMAT_C64: *(complex_double *)Address = TO_NUM((complex_double)0, Value); break;

#else

#define ml_array_get0_complex(FROM_NUM)
#define ml_array_set_complex(TO_NUM)

#endif

#define ARRAY_DECL(PARENT, PREFIX, SUFFIX, CTYPE, APPEND, PRINTF, FROM_VAL, TO_VAL, FROM_NUM, TO_NUM, FORMAT, HASH) \
\
static void append_array_ ## CTYPE(ml_stringbuffer_t *Buffer, int Degree, ml_array_dimension_t *Dimension, char *Address) { \
	if (!Dimension->Size) { \
		ml_stringbuffer_write(Buffer, "<>", 2); \
		return; \
	} \
	ml_stringbuffer_write(Buffer, "<", 1); \
	int Stride = Dimension->Stride; \
	if (Degree == 1) { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Indices) { \
			APPEND(Buffer, PRINTF, *(CTYPE *)(Address + (Indices[0]) * Dimension->Stride)); \
			for (int I = 1; I < Dimension->Size; ++I) { \
				ml_stringbuffer_put(Buffer, ' '); \
				APPEND(Buffer, PRINTF, *(CTYPE *)(Address + (Indices[I]) * Stride)); \
			} \
		} else { \
			APPEND(Buffer, PRINTF, *(CTYPE *)Address); \
			for (int I = Dimension->Size; --I > 0;) { \
				ml_stringbuffer_put(Buffer, ' '); \
				Address += Stride; \
				APPEND(Buffer, PRINTF, *(CTYPE *)Address); \
			} \
		} \
	} else { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Indices) { \
			append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address + (Indices[0]) * Dimension->Stride); \
			for (int I = 1; I < Dimension->Size; ++I) { \
				ml_stringbuffer_put(Buffer, ' '); \
				append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address + (Indices[I]) * Dimension->Stride); \
			} \
		} else { \
			append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address); \
			for (int I = Dimension->Size; --I > 0;) { \
				ml_stringbuffer_put(Buffer, ' '); \
				Address += Stride; \
				append_array_ ## CTYPE(Buffer, Degree - 1, Dimension + 1, Address); \
			} \
		} \
	} \
	ml_stringbuffer_write(Buffer, ">", 1); \
} \
\
 ML_METHOD("append", MLStringBufferT, MLArray ## SUFFIX) { \
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0]; \
	ml_array_t *Array = (ml_array_t *)Args[1]; \
	if (Array->Degree == 0) { \
		APPEND(Buffer, PRINTF, *(CTYPE *)Array->Base.Value); \
	} else { \
		append_array_ ## CTYPE(Buffer, Array->Degree, Array->Dimensions, Array->Base.Value); \
	} \
	return MLSome; \
} \
\
UPDATE_METHODS(MLArrayMutable ## SUFFIX, CTYPE, FROM_VAL, FORMAT); \
static CTYPE ml_array_get0_ ## CTYPE(void *Address, int Format) { \
	switch (Format) { \
	case ML_ARRAY_FORMAT_NONE: break; \
	case ML_ARRAY_FORMAT_U8: return FROM_NUM(*(uint8_t *)Address); \
	case ML_ARRAY_FORMAT_I8: return FROM_NUM(*(int8_t *)Address); \
	case ML_ARRAY_FORMAT_U16: return FROM_NUM(*(uint16_t *)Address); \
	case ML_ARRAY_FORMAT_I16: return FROM_NUM(*(int16_t *)Address); \
	case ML_ARRAY_FORMAT_U32: return FROM_NUM(*(uint32_t *)Address); \
	case ML_ARRAY_FORMAT_I32: return FROM_NUM(*(int32_t *)Address); \
	case ML_ARRAY_FORMAT_U64: return FROM_NUM(*(uint64_t *)Address); \
	case ML_ARRAY_FORMAT_I64: return FROM_NUM(*(int64_t *)Address); \
	case ML_ARRAY_FORMAT_F32: return FROM_NUM(*(float *)Address); \
	case ML_ARRAY_FORMAT_F64: return FROM_NUM(*(double *)Address); \
	ml_array_get0_complex(FROM_NUM) \
	case ML_ARRAY_FORMAT_ANY: return FROM_VAL(*(ml_value_t **)Address); \
	} \
	return (CTYPE)0; \
} \
\
CTYPE ml_array_get_ ## CTYPE(ml_array_t *Array, ...) { \
	va_list Indices; \
	va_start(Indices, Array); \
	char *Address = ml_array_indexv(Array, Indices); \
	va_end(Indices); \
	if (!Address) return 0; \
	return ml_array_get0_ ## CTYPE(Address, Array->Format); \
} \
\
void ml_array_set_ ## CTYPE(CTYPE Value, ml_array_t *Array, ...) { \
	va_list Indices; \
	va_start(Indices, Array); \
	char *Address = ml_array_indexv(Array, Indices); \
	va_end(Indices); \
	if (!Address) return; \
	switch (Array->Format) { \
	case ML_ARRAY_FORMAT_NONE: break; \
	case ML_ARRAY_FORMAT_U8: *(uint8_t *)Address = TO_NUM((uint8_t)0, Value); break; \
	case ML_ARRAY_FORMAT_I8: *(int8_t *)Address = TO_NUM((int8_t)0, Value); break; \
	case ML_ARRAY_FORMAT_U16: *(uint16_t *)Address = TO_NUM((uint16_t)0, Value); break; \
	case ML_ARRAY_FORMAT_I16: *(int16_t *)Address = TO_NUM((int16_t)0, Value); break; \
	case ML_ARRAY_FORMAT_U32: *(uint32_t *)Address = TO_NUM((uint32_t)0, Value); break; \
	case ML_ARRAY_FORMAT_I32: *(int32_t *)Address = TO_NUM((int32_t)0, Value); break; \
	case ML_ARRAY_FORMAT_U64: *(uint64_t *)Address = TO_NUM((uint8_t)0, Value); break; \
	case ML_ARRAY_FORMAT_I64: *(int64_t *)Address = TO_NUM((int64_t)0, Value); break; \
	case ML_ARRAY_FORMAT_F32: *(float *)Address = TO_NUM((float)0, Value); break; \
	case ML_ARRAY_FORMAT_F64: *(double *)Address = TO_NUM((double)0, Value); break; \
	ml_array_set_complex(TO_NUM) \
	case ML_ARRAY_FORMAT_ANY: *(ml_value_t **)Address = TO_VAL(Value); break; \
	} \
} \
\
static long hash_array_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, char *Address) { \
	int Stride = Dimension->Stride; \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Size) { \
			if (Degree == 1) { \
				long Hash = HASH(*(CTYPE *)(Address + (Indices[0]) * Dimension->Stride)); \
				for (int I = 1; I < Dimension->Size; ++I) { \
					Hash = srotl(Hash, 1) | HASH(*(CTYPE *)(Address + (Indices[I]) * Stride)); \
				} \
				return srotl(Hash, Degree); \
			} else { \
				long Hash = hash_array_ ## CTYPE(Degree - 1, Dimension + 1, Address + (Indices[0]) * Dimension->Stride); \
				for (int I = 1; I < Dimension->Size; ++I) { \
					Hash = srotl(Hash, 1) | hash_array_ ## CTYPE(Degree - 1, Dimension + 1, Address + (Indices[I]) * Dimension->Stride); \
				} \
				return srotl(Hash, Degree); \
			} \
		} \
		return 0; \
	} else { \
		if (Degree == 1) { \
			long Hash = HASH(*(CTYPE *)Address); \
			Address += Stride; \
			for (int I = Dimension->Size; --I > 0;) { \
				Hash = srotl(Hash, 1) | HASH(*(CTYPE *)Address); \
				Address += Stride; \
			} \
			return srotl(Hash, Degree); \
		} else { \
			long Hash = hash_array_ ## CTYPE(Degree - 1, Dimension + 1, Address); \
			Address += Stride; \
			for (int I = Dimension->Size; --I > 0;) { \
				Hash = srotl(Hash, 1) | hash_array_ ## CTYPE(Degree - 1, Dimension + 1, Address); \
				Address += Stride; \
			} \
			return srotl(Hash, Degree); \
		} \
	} \
} \
\
static long ml_array_ ## CTYPE ## _hash(ml_array_t *Array) { \
	if (Array->Degree == 0) { \
		return Array->Format + (long)*(CTYPE *)Array->Base.Value; \
	} else { \
		return Array->Format + hash_array_ ## CTYPE(Array->Degree, Array->Dimensions, Array->Base.Value); \
	} \
} \
\
static ml_value_t *ml_array_ ## CTYPE ## _deref(ml_array_t *Target) { \
	if (Target->Degree == 0)  return TO_VAL(*(CTYPE *)Target->Base.Value); \
	return (ml_value_t *)Target; \
} \
\
static void ml_array_ ## CTYPE ## _assign(ml_state_t *Caller, ml_array_t *Target, ml_value_t *Value) { \
	for (;;) if (FORMAT == ML_ARRAY_FORMAT_ANY && !Target->Degree) { \
		*(ml_value_t **)Target->Base.Value = Value; \
		ML_RETURN(Value); \
	} else if (ml_is(Value, MLNumberT)) { \
		CTYPE CValue = FROM_VAL(Value); \
		ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}}; \
		update_row_fn_t Update = UpdateSetRowFns[Target->Format * MAX_FORMATS + Target->Format]; \
		if (!Update) ML_ERROR("ArrayError", "Unsupported array format pair (%s, %s)", Target->Base.Type->Name, ml_typeof(Value)->Name); \
		if (Target->Degree == 0) { \
			Update(ValueDimension, Target->Base.Value, ValueDimension, (char *)&CValue); \
		} else { \
			update_prefix(Update, Target->Degree - 1, Target->Dimensions, Target->Base.Value, 0, ValueDimension, (char *)&CValue); \
		} \
		ML_RETURN(Value); \
	} else if (ml_is(Value, MLArrayT)) { \
		ml_array_t *Source = (ml_array_t *)Value; \
		if (Source->Degree > Target->Degree) ML_ERROR("ArrayError", "Incompatible assignment (%d)", __LINE__); \
		int PrefixDegree = Target->Degree - Source->Degree; \
		for (int I = 0; I < Source->Degree; ++I) { \
			if (Target->Dimensions[PrefixDegree + I].Size != Source->Dimensions[I].Size) ML_ERROR("ArrayError", "Incompatible assignment (%d)", __LINE__); \
		} \
		update_row_fn_t Update = UpdateSetRowFns[Target->Format * MAX_FORMATS + Source->Format]; \
		if (!Update) ML_ERROR("ArrayError", "Unsupported array format pair (%s, %s)", Target->Base.Type->Name, Source->Base.Type->Name); \
		if (Target->Degree) { \
			update_prefix(Update, PrefixDegree, Target->Dimensions, Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value); \
		} else { \
			ml_array_dimension_t ValueDimension[1] = {{1, 0, NULL}}; \
			Update(ValueDimension, Target->Base.Value, ValueDimension, Source->Base.Value); \
		} \
		ML_RETURN(Value); \
	} else { \
		Value = ml_array_of_fn(NULL, 1, &Value); \
	} \
} \
\
ML_CFUNCTIONX(MLArray ## SUFFIX ## New, (void *)FORMAT, ml_array_typed_new_fnx); \
/*@array::PREFIX
//<Sizes:list[integer]
//>array::PREFIX
//  Returns a new array of PREFIX values with the specified dimensions.
*/\
ML_TYPE(MLArray ## SUFFIX, (MLArray ## PARENT), "array::" #PREFIX, \
/*@array::PREFIX
*/ \
	.hash = (void *)ml_array_ ## CTYPE ## _hash, \
	.Constructor = (ml_value_t *)MLArray ## SUFFIX ## New \
); \
\
ML_TYPE(MLArrayMutable ## SUFFIX, (MLArray ## SUFFIX, MLArrayMutable ## PARENT), "array::mutable::" #PREFIX, \
/*@array::mutable::PREFIX
// An array of PREFIX values.
//
// :mini:`(A: array::mutable::PREFIX) := (B: number)`
//    Sets the values in :mini:`A` to :mini:`B`.
// :mini:`(A: array::mutable::PREFIX) := (B: array | list)`
//    Sets the values in :mini:`A` to those in :mini:`B`, broadcasting as necessary. The shape of :mini:`B` must match the last dimensions of :mini:`A`.
*/\
	.hash = (void *)ml_array_ ## CTYPE ## _hash, \
	.Constructor = (ml_value_t *)MLArray ## SUFFIX ## New \
); \
\
ML_TYPE(MLVector ## SUFFIX, (MLVector ## PARENT, MLArray ## SUFFIX), "vector::" #PREFIX, \
/*@vector::PREFIX
*/ \
	.hash = (void *)ml_array_ ## CTYPE ## _hash, \
	.Constructor = (ml_value_t *)MLArray ## SUFFIX ## New \
); \
\
ML_TYPE(MLVectorMutable ## SUFFIX, (MLVector ## SUFFIX, MLVectorMutable ## PARENT, MLArrayMutable ## SUFFIX), "vector::mutable::" #PREFIX, \
/*@vector::mutable::PREFIX
// A vector of PREFIX values.
*/\
	.hash = (void *)ml_array_ ## CTYPE ## _hash, \
	.Constructor = (ml_value_t *)MLArray ## SUFFIX ## New \
); \
ML_TYPE(MLMatrix ## SUFFIX, (MLMatrix ## PARENT, MLArray ## SUFFIX), "matrix::" #PREFIX, \
/*@matrix::PREFIX
*/ \
	.hash = (void *)ml_array_ ## CTYPE ## _hash, \
	.Constructor = (ml_value_t *)MLArray ## SUFFIX ## New \
); \
\
ML_TYPE(MLMatrixMutable ## SUFFIX, (MLMatrix ## SUFFIX, MLMatrixMutable ## PARENT, MLArrayMutable ## SUFFIX), "matrix::mutable::" #PREFIX, \
/*@matrix::mutable::PREFIX
// A matrix of PREFIX values.
*/\
	.hash = (void *)ml_array_ ## CTYPE ## _hash, \
	.Constructor = (ml_value_t *)MLArray ## SUFFIX ## New \
); \
\
static ml_value_t *ML_TYPED_FN(ml_array_value, MLArray ## SUFFIX, ml_array_t *Array, char *Address) { \
	return TO_VAL(*(CTYPE *)Array->Base.Value); \
}

#define NOP_VAL(T, X) X

ARRAY_DECL(IntegerT, uint8, UInt8T, uint8_t, ml_stringbuffer_printf, "%u", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_U8, (long));
ARRAY_DECL(IntegerT, int8, Int8T, int8_t, ml_stringbuffer_printf, "%d", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_I8, (long));
ARRAY_DECL(IntegerT, uint16, UInt16T, uint16_t, ml_stringbuffer_printf, "%u", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_U16, (long));
ARRAY_DECL(IntegerT, int16, Int16T, int16_t, ml_stringbuffer_printf, "%d", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_I16, (long));
ARRAY_DECL(IntegerT, uint32, UInt32T, uint32_t, ml_stringbuffer_printf, "%u", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_U32, (long));
ARRAY_DECL(IntegerT, int32, Int32T, int32_t, ml_stringbuffer_printf, "%d", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_I32, (long));
ARRAY_DECL(IntegerT, uint64, UInt64T, uint64_t, ml_stringbuffer_printf, "%lu", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_U64, (long));
ARRAY_DECL(IntegerT, int64, Int64T, int64_t, ml_stringbuffer_printf, "%ld", ml_integer_value, ml_integer, , NOP_VAL, ML_ARRAY_FORMAT_I64, (long));
ARRAY_DECL(RealT, float32, Float32T, float, ml_stringbuffer_printf, "%g", ml_real_value, ml_real, , NOP_VAL, ML_ARRAY_FORMAT_F32, (long));
ARRAY_DECL(RealT, float64, Float64T, double, ml_stringbuffer_printf, "%g", ml_real_value, ml_real, , NOP_VAL, ML_ARRAY_FORMAT_F64, (long));

#ifdef ML_COMPLEX

ARRAY_DECL(ComplexT, complex32, Complex32T, complex_float, COMPLEX_APPEND, "%g", ml_complex_value, ml_complex, , NOP_VAL, ML_ARRAY_FORMAT_C32, (long));
ARRAY_DECL(ComplexT, complex64, Complex64T, complex_double, COMPLEX_APPEND, "%g", ml_complex_value, ml_complex, , NOP_VAL, ML_ARRAY_FORMAT_C64, (long));

#endif

ARRAY_DECL(T, any, AnyT, any, BUFFER_APPEND, "?", ml_nop, ml_nop, ml_number, ml_number_value, ML_ARRAY_FORMAT_ANY, ml_hash);

static ml_value_t *ml_array_ref_deref(ml_array_ref_t *Ref) {
	switch (Ref->Array->Format) {
	case ML_ARRAY_FORMAT_NONE: break;
	case ML_ARRAY_FORMAT_U8: return ml_array_uint8_t_deref(Ref->Array);
	case ML_ARRAY_FORMAT_I8: return ml_array_int8_t_deref(Ref->Array);
	case ML_ARRAY_FORMAT_U16: return ml_array_uint16_t_deref(Ref->Array);
	case ML_ARRAY_FORMAT_I16: return ml_array_int16_t_deref(Ref->Array);
	case ML_ARRAY_FORMAT_U32: return ml_array_uint32_t_deref(Ref->Array);
	case ML_ARRAY_FORMAT_I32: return ml_array_int32_t_deref(Ref->Array);
	case ML_ARRAY_FORMAT_U64: return ml_array_uint64_t_deref(Ref->Array);
	case ML_ARRAY_FORMAT_I64: return ml_array_int64_t_deref(Ref->Array);
	case ML_ARRAY_FORMAT_F32: return ml_array_float_deref(Ref->Array);
	case ML_ARRAY_FORMAT_F64: return ml_array_double_deref(Ref->Array);
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32: return ml_array_complex_float_deref(Ref->Array);
	case ML_ARRAY_FORMAT_C64: return ml_array_complex_double_deref(Ref->Array);
#endif
	case ML_ARRAY_FORMAT_ANY: return ml_array_any_deref(Ref->Array);
	}
	__builtin_unreachable();
}

static void ml_array_ref_assign(ml_state_t *Caller, ml_array_ref_t *Ref, ml_value_t *Value) {
	switch (Ref->Array->Format) {
	case ML_ARRAY_FORMAT_NONE: break;
	case ML_ARRAY_FORMAT_U8: return ml_array_uint8_t_assign(Caller, Ref->Array, Value);
	case ML_ARRAY_FORMAT_I8: return ml_array_int8_t_assign(Caller, Ref->Array, Value);
	case ML_ARRAY_FORMAT_U16: return ml_array_uint16_t_assign(Caller, Ref->Array, Value);
	case ML_ARRAY_FORMAT_I16: return ml_array_int16_t_assign(Caller, Ref->Array, Value);
	case ML_ARRAY_FORMAT_U32: return ml_array_uint32_t_assign(Caller, Ref->Array, Value);
	case ML_ARRAY_FORMAT_I32: return ml_array_int32_t_assign(Caller, Ref->Array, Value);
	case ML_ARRAY_FORMAT_U64: return ml_array_uint64_t_assign(Caller, Ref->Array, Value);
	case ML_ARRAY_FORMAT_I64: return ml_array_int64_t_assign(Caller, Ref->Array, Value);
	case ML_ARRAY_FORMAT_F32: return ml_array_float_assign(Caller, Ref->Array, Value);
	case ML_ARRAY_FORMAT_F64: return ml_array_double_assign(Caller, Ref->Array, Value);
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32: return ml_array_complex_float_assign(Caller, Ref->Array, Value);
	case ML_ARRAY_FORMAT_C64: return ml_array_complex_double_assign(Caller, Ref->Array, Value);
#endif
	case ML_ARRAY_FORMAT_ANY: return ml_array_any_assign(Caller, Ref->Array, Value);
	}
}

#define PARTIAL_FUNCTIONS(CTYPE) \
\
static void partial_sums_ ## CTYPE(int Target, int Degree, ml_array_dimension_t *Dimension, char *Address, int LastRow) { \
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
static void partial_prods_ ## CTYPE(int Target, int Degree, ml_array_dimension_t *Dimension, char *Address, int LastRow) { \
	if (Degree == 0) { \
		*(CTYPE *)Address *= *(CTYPE *)(Address - LastRow); \
	} else if (Target == Degree) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
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
			int *Indices = Dimension->Indices; \
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
			int *Indices = Dimension->Indices; \
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
			int *Indices = Dimension->Indices; \
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
			int *Indices = SourceDimension->Indices; \
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
			int *Indices = Dimension->Indices; \
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
			int *Indices = Dimension->Indices; \
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
			int *Indices = SourceDimension->Indices; \
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

#define MINMAX_FUNCTIONS(CTYPE, MINVAL, MAXVAL) \
\
static CTYPE compute_mins_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, void *Address) { \
	CTYPE Min = MAXVAL; \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
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
			int *Indices = Dimension->Indices; \
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
			int *Indices = SourceDimension->Indices; \
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
static CTYPE find_mins_ ## CTYPE(int *Target, int Degree, ml_array_dimension_t *Dimension, void *Address, CTYPE Min) { \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
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
			int *Indices = Dimension->Indices; \
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
		find_mins_ ## CTYPE((int *)TargetAddress, SourceDegree, SourceDimension, SourceAddress, MAXVAL); \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		int SourceStride = SourceDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *Indices = SourceDimension->Indices; \
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
	CTYPE Max = MINVAL; \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
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
			int *Indices = Dimension->Indices; \
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
			int *Indices = SourceDimension->Indices; \
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
static CTYPE find_maxs_ ## CTYPE(int *Target, int Degree, ml_array_dimension_t *Dimension, void *Address, CTYPE Max) { \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
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
			int *Indices = Dimension->Indices; \
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
		find_maxs_ ## CTYPE((int *)TargetAddress, SourceDegree, SourceDimension, SourceAddress, MINVAL); \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		int SourceStride = SourceDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *Indices = SourceDimension->Indices; \
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

MINMAX_FUNCTIONS(uint8_t, 0, UINT8_MAX);
MINMAX_FUNCTIONS(int8_t, INT8_MIN, INT8_MAX);
MINMAX_FUNCTIONS(uint16_t, 0, UINT16_MAX);
MINMAX_FUNCTIONS(int16_t, INT16_MIN, INT16_MAX);
MINMAX_FUNCTIONS(uint32_t, 0, UINT32_MAX);
MINMAX_FUNCTIONS(int32_t, INT32_MIN, INT32_MAX);
MINMAX_FUNCTIONS(uint64_t, 0, UINT64_MAX);
MINMAX_FUNCTIONS(int64_t, INT64_MIN, INT64_MAX);
MINMAX_FUNCTIONS(float, -INFINITY, INFINITY);
MINMAX_FUNCTIONS(double, -INFINITY, INFINITY);

#ifdef ML_COMPLEX

PARTIAL_FUNCTIONS(complex_double);

COMPLETE_FUNCTIONS(complex_double, complex_float);
COMPLETE_FUNCTIONS(complex_double, complex_double);

#endif

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

#define NORM_FUNCTION(CTYPE1, CTYPE2, NORM) \
\
static double compute_norm_ ## CTYPE2(int Degree, ml_array_dimension_t *Dimension, void *Address, double P) { \
	double Sum = 0; \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
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
			int *Indices = Dimension->Indices; \
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
static double compute_norm0_ ## CTYPE2(int Degree, ml_array_dimension_t *Dimension, void *Address, double P) { \
	double Sum = 1; \
	if (Degree > 1) { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += compute_norm0_ ## CTYPE2(Degree - 1, Dimension + 1, Address + Indices[I] * Stride, P); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum += compute_norm0_ ## CTYPE2(Degree - 1, Dimension + 1, Address, P); \
				Address += Stride; \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum *= pow(NORM(*(CTYPE2 *)(Address + Indices[I] * Stride)), P); \
			} \
		} else { \
			for (int I = 0; I < Dimension->Size; ++I) { \
				Sum *= pow(NORM(*(CTYPE2 *)Address), P); \
				Address += Stride; \
			} \
		} \
	} \
	return Sum; \
}

NORM_FUNCTION(double, uint8_t, labs);
NORM_FUNCTION(double, int8_t, labs);
NORM_FUNCTION(double, uint16_t, labs);
NORM_FUNCTION(double, int16_t, labs);
NORM_FUNCTION(double, uint32_t, labs);
NORM_FUNCTION(double, int32_t, labs);
NORM_FUNCTION(double, uint64_t, labs);
NORM_FUNCTION(double, int64_t, labs);
NORM_FUNCTION(double, float, fabs);
NORM_FUNCTION(double, double, fabs);

#ifdef ML_COMPLEX

NORM_FUNCTION(double, complex_float, cabs);
NORM_FUNCTION(double, complex_double, cabs);

#endif

static char *array_flatten_to(char *Target, int Size, int Degree, int FlatDegree, ml_array_dimension_t *Dimension, char *Source) {
	if (Degree == FlatDegree) {
		int Total = Dimension->Size * Dimension->Stride;
		memcpy(Target, Source, Total);
		return Target + Total;
	} else if (Degree == 1) {
		if (!Dimension->Indices) {
			int Stride = Dimension->Stride;
			switch (Size) {
			case 1:
				for (int I = Dimension->Size; --I >= 0;) {
					memcpy(Target, Source, 1);
					Target += 1;
					Source += Stride;
				}
				break;
			case 2:
				for (int I = Dimension->Size; --I >= 0;) {
					memcpy(Target, Source, 2);
					Target += 2;
					Source += Stride;
				}
				break;
			case 4:
				for (int I = Dimension->Size; --I >= 0;) {
					memcpy(Target, Source, 4);
					Target += 4;
					Source += Stride;
				}
				break;
			case 8:
				for (int I = Dimension->Size; --I >= 0;) {
					memcpy(Target, Source, 8);
					Target += 8;
					Source += Stride;
				}
				break;
			case 16:
				for (int I = Dimension->Size; --I >= 0;) {
					memcpy(Target, Source, 16);
					Target += 16;
					Source += Stride;
				}
				break;
			}
		} else {
			int Stride = Dimension->Stride;
			int *Indices = Dimension->Indices;
			switch (Size) {
			case 1:
				for (int I = Dimension->Size; --I >= 0;) {
					memcpy(Target, Source + Stride * *Indices++, 1);
					Target += 1;
				}
				break;
			case 2:
				for (int I = Dimension->Size; --I >= 0;) {
					memcpy(Target, Source + Stride * *Indices++, 2);
					Target += 2;
				}
				break;
			case 4:
				for (int I = Dimension->Size; --I >= 0;) {
					memcpy(Target, Source + Stride * *Indices++, 4);
					Target += 4;
				}
				break;
			case 8:
				for (int I = Dimension->Size; --I >= 0;) {
					memcpy(Target, Source + Stride * *Indices++, 8);
					Target += 8;
				}
				break;
			case 16:
				for (int I = Dimension->Size; --I >= 0;) {
					memcpy(Target, Source + Stride * *Indices++, 16);
					Target += 16;
				}
				break;
			}
		}
		return Target;
	} else if (!Dimension->Indices) {
		int Stride = Dimension->Stride;
		for (int I = Dimension->Size; --I >= 0;) {
			Target = array_flatten_to(Target, Size, Degree - 1, FlatDegree, Dimension + 1, Source);
			Source += Stride;
		}
		return Target;
	} else {
		int Stride = Dimension->Stride;
		int *Indices = Dimension->Indices;
		for (int I = Dimension->Size; --I >= 0;) {
			Target = array_flatten_to(Target, Size, Degree - 1, FlatDegree, Dimension + 1, Source + Stride * *Indices++);
		}
		return Target;
	}
}

static char *array_flatten(ml_array_t *Source) {
	size_t Size = MLArraySizes[Source->Format];
	int FlatDegree = Source->Degree;
	for (int I = Source->Degree; --I >= 0;) {
		if (Size == Source->Dimensions[I].Stride) FlatDegree = I;
		Size *= Source->Dimensions[I].Size;
	}
	FlatDegree = Source->Degree - FlatDegree;
	char *Data = snew(Size);
	array_flatten_to(Data, MLArraySizes[Source->Format], Source->Degree, FlatDegree, Source->Dimensions, Source->Base.Value);
	return Data;
}

static int array_copy(ml_array_t *Target, ml_array_t *Source) {
	int Degree = Source->Degree;
	int DataSize = MLArraySizes[Target->Format];
	for (int I = Degree; --I >= 0;) {
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size;
		DataSize *= Size;
	}
	Target->Base.Length = DataSize;
	if (Target->Format == Source->Format) {
		Target->Base.Value = array_flatten(Source);
	} else {
		Target->Base.Value = snew(DataSize);
		update_row_fn_t Update = UpdateSetRowFns[Target->Format * MAX_FORMATS + Source->Format];
		update_array(Update, Target->Dimensions, Target->Base.Value, Degree, Source->Dimensions, Source->Base.Value);
	}
	return DataSize;
}

ML_METHOD("reshape", MLArrayT, MLListT) {
//<Array
//<Sizes
//>array
// Returns a copy of :mini:`Array` with dimensions specified by :mini:`Sizes`.
// .. note::
//
//    This method always makes a copy of the data so that changes to the returned array do not affect the original.
	int TargetDegree = ml_list_length(Args[1]);
	size_t TargetCount = 1;
	ML_LIST_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Value, MLIntegerT)) return ml_error("ArrayError", "Invalid size");
		int Size = ml_integer_value(Iter->Value);
		if (Size <= 0) return ml_error("ArrayError", "Invalid size");
		TargetCount *= Size;
	}
	ml_array_t *Source = (ml_array_t *)Args[0];
	int SourceDegree = Source->Degree;
	size_t SourceCount = 1;
	for (int I = 0; I < SourceDegree; ++I) SourceCount *= Source->Dimensions[I].Size;
	if (TargetCount != SourceCount) return ml_error("ArrayError", "Incompatible shapes");

	ml_array_t *Target = ml_array_alloc(Source->Format, TargetDegree);
	Target->Base.Value = array_flatten(Source);
	size_t DataSize = MLArraySizes[Target->Format];
	int I = TargetDegree;
	ML_LIST_REVERSE(Args[1], Iter) {
		--I;
		Target->Dimensions[I].Stride = DataSize;
		int Size = Target->Dimensions[I].Size = ml_integer_value(Iter->Value);
		DataSize *= Size;
	}
	Target->Base.Length = DataSize;
	return (ml_value_t *)Target;
}

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
		array_copy(Target, Source);
		partial_sums_uint64_t(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
	case ML_ARRAY_FORMAT_I8:
	case ML_ARRAY_FORMAT_I16:
	case ML_ARRAY_FORMAT_I32:
	case ML_ARRAY_FORMAT_I64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_I64, Source->Degree);
		array_copy(Target, Source);
		partial_sums_int64_t(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
	case ML_ARRAY_FORMAT_F32:
	case ML_ARRAY_FORMAT_F64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_F64, Source->Degree);
		array_copy(Target, Source);
		partial_sums_double(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
	case ML_ARRAY_FORMAT_C64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_C64, Source->Degree);
		array_copy(Target, Source);
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
		array_copy(Target, Source);
		partial_prods_uint64_t(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
	case ML_ARRAY_FORMAT_I8:
	case ML_ARRAY_FORMAT_I16:
	case ML_ARRAY_FORMAT_I32:
	case ML_ARRAY_FORMAT_I64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_I64, Source->Degree);
		array_copy(Target, Source);
		partial_prods_int64_t(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
	case ML_ARRAY_FORMAT_F32:
	case ML_ARRAY_FORMAT_F64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_F64, Source->Degree);
		array_copy(Target, Source);
		partial_prods_double(Index, Target->Degree, Target->Dimensions, Target->Base.Value, 0);
		return (ml_value_t *)Target;
	}
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
	case ML_ARRAY_FORMAT_C64: {
		ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_C64, Source->Degree);
		array_copy(Target, Source);
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
	if (Slice <= 0 || Slice >= Source->Degree) return ml_error("RangeError", "Invalid axes count for sum");
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
	Target->Base.Value = snew(DataSize);
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
	if (Slice <= 0 || Slice >= Source->Degree) return ml_error("RangeError", "Invalid axes count for prod");
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
	Target->Base.Value = snew(DataSize);
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
	if (Slice <= 0 || Slice >= Source->Degree) return ml_error("RangeError", "Invalid axes count for min");
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
	Target->Base.Value = snew(DataSize);
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
	ml_array_t *Target = ml_array_alloc(ML_ARRAY_FORMAT_I32, 1);
	int DataSize = MLArraySizes[Target->Format];
	Target->Dimensions[Target->Degree - 1].Stride = DataSize;
	Target->Dimensions[Target->Degree - 1].Size = Source->Degree;
	Target->Base.Value = snew(DataSize);
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		find_mins_uint8_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, UINT8_MAX);
		break;
	case ML_ARRAY_FORMAT_I8:
		find_mins_int8_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, INT8_MAX);
		break;
	case ML_ARRAY_FORMAT_U16:
		find_mins_uint16_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, UINT16_MAX);
		break;
	case ML_ARRAY_FORMAT_I16:
		find_mins_int16_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, INT16_MAX);
		break;
	case ML_ARRAY_FORMAT_U32:
		find_mins_uint32_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, UINT32_MAX);
		break;
	case ML_ARRAY_FORMAT_I32:
		find_mins_int32_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, INT32_MAX);
		break;
	case ML_ARRAY_FORMAT_U64:
		find_mins_uint64_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, UINT64_MAX);
		break;
	case ML_ARRAY_FORMAT_I64:
		find_mins_int64_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, INT64_MAX);
		break;
	case ML_ARRAY_FORMAT_F32:
		find_mins_float((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, FLT_MAX);
		break;
	case ML_ARRAY_FORMAT_F64:
		find_mins_double((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, DBL_MAX);
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
	if (Slice <= 0 || Slice >= Source->Degree) return ml_error("RangeError", "Invalid axes count for min");
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
	if (Slice <= 0 || Slice >= Source->Degree) return ml_error("RangeError", "Invalid axes count for max");
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
	Target->Base.Value = snew(DataSize);
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
	Target->Base.Value = snew(DataSize);
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		find_maxs_uint8_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, 0);
		break;
	case ML_ARRAY_FORMAT_I8:
		find_maxs_int8_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, INT8_MIN);
		break;
	case ML_ARRAY_FORMAT_U16:
		find_maxs_uint16_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, 0);
		break;
	case ML_ARRAY_FORMAT_I16:
		find_maxs_int16_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, INT16_MIN);
		break;
	case ML_ARRAY_FORMAT_U32:
		find_maxs_uint32_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, 0);
		break;
	case ML_ARRAY_FORMAT_I32:
		find_maxs_int32_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, INT32_MIN);
		break;
	case ML_ARRAY_FORMAT_U64:
		find_maxs_uint64_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, 0);
		break;
	case ML_ARRAY_FORMAT_I64:
		find_maxs_int64_t((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, INT64_MIN);
		break;
	case ML_ARRAY_FORMAT_F32:
		find_maxs_float((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, FLT_MIN);
		break;
	case ML_ARRAY_FORMAT_F64:
		find_maxs_double((int *)Target->Base.Value, Source->Degree, Source->Dimensions, Source->Base.Value, DBL_MIN);
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
	if (Slice <= 0 || Slice >= Source->Degree) return ml_error("RangeError", "Invalid axes count for max");
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

ML_METHOD("||", MLArrayT) {
//<Array
//>number
// Returns the norm of the values in :mini:`Array`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	double P = 2, Norm;
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		Norm = compute_norm_uint8_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_I8:
		Norm = compute_norm_int8_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_U16:
		Norm = compute_norm_uint16_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_I16:
		Norm = compute_norm_int16_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_U32:
		Norm = compute_norm_uint32_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_I32:
		Norm = compute_norm_int32_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_U64:
		Norm = compute_norm_uint64_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_I64:
		Norm = compute_norm_int64_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_F32:
		Norm = compute_norm_float(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_F64:
		Norm = compute_norm_double(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
		Norm = compute_norm_complex_float(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_C64:
		Norm = compute_norm_complex_double(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
#endif
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
	return ml_real(pow(Norm, 1 / P));
}

ML_METHOD("||", MLArrayT, MLRealT) {
//<Array
//>number
// Returns the norm of the values in :mini:`Array`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	double P = ml_real_value(Args[1]), Norm;
	if (P < 1) return ml_error("ValueError", "Invalid p-value for norm");
	switch (Source->Format) {
	case ML_ARRAY_FORMAT_U8:
		Norm = compute_norm_uint8_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_I8:
		Norm = compute_norm_int8_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_U16:
		Norm = compute_norm_uint16_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_I16:
		Norm = compute_norm_int16_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_U32:
		Norm = compute_norm_uint32_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_I32:
		Norm = compute_norm_int32_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_U64:
		Norm = compute_norm_uint64_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_I64:
		Norm = compute_norm_int64_t(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_F32:
		Norm = compute_norm_float(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_F64:
		Norm = compute_norm_double(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32:
		Norm = compute_norm_complex_float(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
	case ML_ARRAY_FORMAT_C64:
		Norm = compute_norm_complex_double(Source->Degree, Source->Dimensions, Source->Base.Value, P);
		break;
#endif
	default:
		return ml_error("ArrayError", "Invalid array format");
	}
	return ml_real(pow(Norm, 1 / P));
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
		int DataSize = array_copy(C, A);
		int8_t *Values = (int8_t *)C->Base.Value;
		for (int I = DataSize / sizeof(int8_t); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
	case ML_ARRAY_FORMAT_U16:
	case ML_ARRAY_FORMAT_I16: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I16, Degree);
		int DataSize = array_copy(C, A);
		int16_t *Values = (int16_t *)C->Base.Value;
		for (int I = DataSize / sizeof(int16_t); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
	case ML_ARRAY_FORMAT_U32:
	case ML_ARRAY_FORMAT_I32: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I32, Degree);
		int DataSize = array_copy(C, A);
		int32_t *Values = (int32_t *)C->Base.Value;
		for (int I = DataSize / sizeof(int32_t); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
	case ML_ARRAY_FORMAT_U64:
	case ML_ARRAY_FORMAT_I64: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I64, Degree);
		int DataSize = array_copy(C, A);
		int64_t *Values = (int64_t *)C->Base.Value;
		for (int I = DataSize / sizeof(int64_t); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
	case ML_ARRAY_FORMAT_F32: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_F32, Degree);
		int DataSize = array_copy(C, A);
		float *Values = (float *)C->Base.Value;
		for (int I = DataSize / sizeof(float); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
	case ML_ARRAY_FORMAT_F64: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_F64, Degree);
		int DataSize = array_copy(C, A);
		double *Values = (double *)C->Base.Value;
		for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_C32, Degree);
		int DataSize = array_copy(C, A);
		complex_float *Values = (complex_float *)C->Base.Value;
		for (int I = DataSize / sizeof(complex_float); --I >= 0; ++Values) *Values = -*Values;
		return (ml_value_t *)C;
	}
	case ML_ARRAY_FORMAT_C64: {
		ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_C64, Degree);
		int DataSize = array_copy(C, A);
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
	int DataSize = array_copy(C, A);
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
	int DataSize = array_copy(C, A);
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
	int DataSize = array_copy(C, A);
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
	int DataSize = array_copy(C, A);
	complex_double *Values = (complex_double *)C->Base.Value;
	for (int I = DataSize / sizeof(complex_double); --I >= 0; ++Values) *Values = fn(*Values);
	ml_array_t *D = ml_array_alloc(ML_ARRAY_FORMAT_F64, Degree);
	array_copy(D, C);
	return (ml_value_t *)D;
}
#endif

#define MIN(X, Y) ((X < Y) ? X : Y)
#define MAX(X, Y) ((X > Y) ? X : Y)

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
	array_copy(C, A);
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

#define ML_ARITH_METHOD_BASE(NAME, MIN_FORMAT) \
\
ML_METHOD(#NAME, MLArrayT, MLIntegerT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A/v NAME B`.
//$= let A := array([[1, 2], [3, 4]])
//$= A NAME 2
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value_fast(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(MAX(A->Format, MIN_FORMAT), Degree); \
	int DataSize = array_copy(C, A); \
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
ML_METHOD(#NAME, MLIntegerT, MLArrayT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A NAME B/v`.
//$= let A := array([[1, 2], [3, 4]])
//$= 2 NAME A
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value_fast(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(MAX(A->Format, MIN_FORMAT), Degree); \
	int DataSize = array_copy(C, A); \
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
ML_METHOD(#NAME, MLArrayT, MLRealT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A/v NAME B`.
//$= let A := array([[1, 2], [3, 4]])
//$= A NAME 2.5
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	double B = ml_real_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(MAX(A->Format, ML_ARRAY_FORMAT_F64), Degree); \
	int DataSize = array_copy(C, A); \
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
ML_METHOD(#NAME, MLRealT, MLArrayT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A NAME B/v`.
//$= let A := array([[1, 2], [3, 4]])
//$= 2.5 NAME A
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	double B = ml_real_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(MAX(A->Format, ML_ARRAY_FORMAT_F64), Degree); \
	int DataSize = array_copy(C, A); \
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

#define ML_ARITH_METHOD(NAME, MIN_FORMAT) \
ML_ARITH_METHOD_BASE(NAME, MIN_FORMAT) \
\
ML_METHOD(#NAME, MLArrayT, MLComplexT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A/v NAME B`.
//$= let A := array([[1, 2], [3, 4]])
//$= A NAME (1 + 1i)
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	complex_double B = ml_complex_value(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_C64, Degree); \
	int DataSize = array_copy(C, A); \
	complex_double *Values = (complex_double *)C->Base.Value; \
	for (int I = DataSize / sizeof(complex_double); --I >= 0; ++Values) *Values = *Values NAME B; \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#NAME, MLComplexT, MLArrayT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := A NAME B/v`.
//$= let A := array([[1, 2], [3, 4]])
//$= (1 + 1i) NAME A
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	complex_double B = ml_complex_value(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_C64, Degree); \
	int DataSize = array_copy(C, A); \
	complex_double *Values = (complex_double *)C->Base.Value; \
	for (int I = DataSize / sizeof(complex_double); --I >= 0; ++Values) *Values = B NAME *Values; \
	return (ml_value_t *)C; \
}

#else

#define ML_ARITH_METHOD(NAME, MIN_FORMAT) \
ML_ARITH_METHOD_BASE(NAME, MIN_FORMAT)

#endif

ML_ARITH_METHOD(+, ML_ARRAY_FORMAT_I64);
ML_ARITH_METHOD(*, ML_ARRAY_FORMAT_I64);
ML_ARITH_METHOD(-, ML_ARRAY_FORMAT_I64);
ML_ARITH_METHOD(/, ML_ARRAY_FORMAT_F64);

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
	int64_t B = ml_integer_value_fast(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I64, Degree); \
	int DataSize = array_copy(C, A); \
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
	int64_t B = ml_integer_value_fast(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I64, Degree); \
	int DataSize = array_copy(C, A); \
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
	int64_t B = ml_integer_value_fast(Args[1]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I64, Degree); \
	int DataSize = array_copy(C, A); \
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
	int64_t B = ml_integer_value_fast(Args[0]); \
	int Degree = A->Degree; \
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_I64, Degree); \
	int DataSize = array_copy(C, A); \
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
	int DataSize = array_copy(C, A); \
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
	int DataSize = array_copy(C, A); \
	double *Values = (double *)C->Base.Value; \
	for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = FN(B, *Values); \
	return (ml_value_t *)C; \
}

ML_ARITH_METHOD_MINMAX(min, MIN)
ML_ARITH_METHOD_MINMAX(max, MAX)

#ifdef ML_COMPLEX

ML_METHOD("^", MLArrayMutableComplexT, MLComplexT) {
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Degree == -1) return (ml_value_t *)A;
	complex_double B = ml_complex_value(Args[1]);
	int Degree = A->Degree;
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_C64, Degree);
	int DataSize = array_copy(C, A);
	complex_double *Values = (complex_double *)C->Base.Value;
	for (int I = DataSize / sizeof(complex_double); --I >= 0; ++Values) *Values = cpow(*Values, B);
	return (ml_value_t *)C;
}

#else

ML_METHOD("^", MLArrayMutableRealT, MLRealT) {
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Degree == -1) return (ml_value_t *)A;
	double B = ml_real_value(Args[1]);
	int Degree = A->Degree;
	ml_array_t *C = ml_array_alloc(ML_ARRAY_FORMAT_F64, Degree);
	int DataSize = array_copy(C, A);
	double *Values = (double *)C->Base.Value;
	for (int I = DataSize / sizeof(double); --I >= 0; ++Values) *Values = pow(*Values, B);
	return (ml_value_t *)C;
}

#endif

#define ML_COMPARE_METHOD_BASE(TITLE, TITLE2, OP) \
\
ML_METHOD(#OP, MLArrayT, MLIntegerT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := if A/v OP B then 1 else 0 end`.
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value_fast(Args[1]); \
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
ML_METHOD(#OP, MLIntegerT, MLArrayT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := if A OP B/v then 1 else 0 end`.
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
	int64_t B = ml_integer_value_fast(Args[0]); \
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
ML_METHOD(#OP, MLArrayT, MLRealT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := if A/v OP B then 1 else 0 end`.
*/ \
	ml_array_t *A = (ml_array_t *)Args[0]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
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
	if (!Compare) return ml_error("ArrayError", "Unsupported array format pair (%s, integer)", A->Base.Type->Name); \
	compare_prefix(Compare, C->Dimensions, C->Base.Value, Degree - 1, A->Dimensions, A->Base.Value, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#OP, MLRealT, MLArrayT) { \
/*<A
//<B
//>array
// Returns an array :mini:`C` where each :mini:`C/v := if A OP B/v then 1 else 0 end`.
*/ \
	ml_array_t *A = (ml_array_t *)Args[1]; \
	if (A->Degree == -1) return (ml_value_t *)A; \
	if (A->Format == ML_ARRAY_FORMAT_ANY) return ml_error("TypeError", "Invalid types for array operation"); \
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
	if (!Compare) return ml_error("ArrayError", "Unsupported array format pair (%s, integer)", A->Base.Type->Name); \
	compare_prefix(Compare, C->Dimensions, C->Base.Value, Degree - 1, A->Dimensions, A->Base.Value, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
}

#ifdef ML_COMPLEX

#define ML_COMPARE_METHOD(TITLE, TITLE2, OP) \
ML_COMPARE_METHOD_BASE(TITLE, TITLE2, OP) \
\
ML_METHOD(#OP, MLArrayT, MLComplexT) { \
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
	if (!Compare) return ml_error("ArrayError", "Unsupported array format pair (%s, integer)", A->Base.Type->Name); \
	compare_prefix(Compare, C->Dimensions, C->Base.Value, Degree - 1, A->Dimensions, A->Base.Value, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
} \
\
ML_METHOD(#OP, MLComplexT, MLArrayT) { \
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
	if (!Compare) return ml_error("ArrayError", "Unsupported array format pair (%s, integer)", A->Base.Type->Name); \
	compare_prefix(Compare, C->Dimensions, C->Base.Value, Degree - 1, A->Dimensions, A->Base.Value, 0, NULL, (char *)&B); \
	return (ml_value_t *)C; \
}

#else

#define ML_COMPARE_METHOD(BASE, BASE2, OP) \
ML_COMPARE_METHOD_BASE(BASE, BASE2, OP)

#endif

ML_COMPARE_METHOD(Eq, Eq, =);
ML_COMPARE_METHOD(Ne, Ne, !=);
ML_COMPARE_METHOD(Lt, Gt, <);
ML_COMPARE_METHOD(Gt, Lt, >);
ML_COMPARE_METHOD(Le, Ge, <=);
ML_COMPARE_METHOD(Ge, Le, >=);

static ml_array_format_t ml_array_of_type_guess(ml_value_t *Value, ml_array_format_t Format) {
	typeof(ml_array_of_type_guess) *function = ml_typed_fn_get(ml_typeof(Value), ml_array_of_type_guess);
	if (function) return function(Value, Format);
	return ML_ARRAY_FORMAT_ANY;
}

static ml_array_format_t ML_TYPED_FN(ml_array_of_type_guess, MLListT, ml_value_t *Value, ml_array_format_t Format) {
	ML_LIST_FOREACH(Value, Iter) {
		Format = ml_array_of_type_guess(Iter->Value, Format);
	}
	return Format;
}

static ml_array_format_t ML_TYPED_FN(ml_array_of_type_guess, MLTupleT, ml_value_t *Value, ml_array_format_t Format) {
	ml_tuple_t *Tuple = (ml_tuple_t *)Value;
	for (int I = 0; I < Tuple->Size; ++I) {
		Format = ml_array_of_type_guess(Tuple->Values[I], Format);
	}
	return Format;
}

static ml_array_format_t ML_TYPED_FN(ml_array_of_type_guess, MLArrayT, ml_value_t *Value, ml_array_format_t Format) {
	ml_array_t *Array = (ml_array_t *)Value;
	if (Format <= Array->Format) Format = Array->Format;
	return Format;
}

static ml_array_format_t ML_TYPED_FN(ml_array_of_type_guess, MLIntegerT, ml_value_t *Value, ml_array_format_t Format) {
	if (Format < ML_ARRAY_FORMAT_I64) Format = ML_ARRAY_FORMAT_I64;
	return Format;
}

static ml_array_format_t ML_TYPED_FN(ml_array_of_type_guess, MLRealT, ml_value_t *Value, ml_array_format_t Format) {
	if (Format < ML_ARRAY_FORMAT_F64) Format = ML_ARRAY_FORMAT_F64;
	return Format;
}

#ifdef ML_COMPLEX
static ml_array_format_t ML_TYPED_FN(ml_array_of_type_guess, MLComplexT, ml_value_t *Value, ml_array_format_t Format) {
	if (Format < ML_ARRAY_FORMAT_C64) Format = ML_ARRAY_FORMAT_C64;
	return Format;
}
#endif

static ml_array_format_t ML_TYPED_FN(ml_array_of_type_guess, MLIntegerRangeT, ml_value_t *Value, ml_array_format_t Format) {
	if (Format < ML_ARRAY_FORMAT_I64) Format = ML_ARRAY_FORMAT_I64;
	return Format;
}

static ml_array_format_t ML_TYPED_FN(ml_array_of_type_guess, MLRealRangeT, ml_value_t *Value, ml_array_format_t Format) {
	if (Format < ML_ARRAY_FORMAT_F64) Format = ML_ARRAY_FORMAT_F64;
	return Format;
}

static ml_array_t *ml_array_of_create(ml_value_t *Value, int Degree, ml_array_format_t Format) {
	typeof(ml_array_of_create) *function = ml_typed_fn_get(ml_typeof(Value), ml_array_of_create);
	if (function) return function(Value, Degree, Format);
	ml_array_t *Array = ml_array_alloc(Format, Degree);
	if (Degree) {
		Array->Dimensions[Degree - 1].Size = 1;
		Array->Dimensions[Degree - 1].Stride = MLArraySizes[Format];
	}
	return Array;
}

static ml_array_t *ML_TYPED_FN(ml_array_of_create, MLListT, ml_value_t *Value, int Degree, ml_array_format_t Format) {
	int Size = ml_list_length(Value);
	if (!Size) return (ml_array_t *)ml_error("ValueError", "Empty dimension in array");
	ml_array_t *Array = ml_array_of_create(ml_list_get(Value, 1), Degree + 1, Format);
	if (Array->Base.Type == MLErrorT) return Array;
	Array->Dimensions[Degree].Size = Size;
	if (Degree < Array->Degree - 1) {
		Array->Dimensions[Degree].Stride = Array->Dimensions[Degree + 1].Size * Array->Dimensions[Degree + 1].Stride;
	}
	return Array;
}

static ml_array_t *ML_TYPED_FN(ml_array_of_create, MLTupleT, ml_value_t *Value, int Degree, ml_array_format_t Format) {
	int Size = ml_tuple_size(Value);
	if (!Size) return (ml_array_t *)ml_error("ValueError", "Empty dimension in array");
	ml_array_t *Array = ml_array_of_create(ml_tuple_get(Value, 1), Degree + 1, Format);
	if (Array->Base.Type == MLErrorT) return Array;
	Array->Dimensions[Degree].Size = Size;
	if (Degree < Array->Degree - 1) {
		Array->Dimensions[Degree].Stride = Array->Dimensions[Degree + 1].Size * Array->Dimensions[Degree + 1].Stride;
	}
	return Array;
}

static ml_array_t *ML_TYPED_FN(ml_array_of_create, MLArrayT, ml_array_t *Value, int Degree, ml_array_format_t Format) {
	ml_array_t *Array = ml_array_alloc(Format, Degree + Value->Degree);
	ml_array_dimension_t *Dimensions = Array->Dimensions + Degree;
	size_t Stride = MLArraySizes[Format];
	for (int I = Value->Degree; --I >= 0;) {
		Dimensions[I].Stride = Stride;
		size_t Size = Dimensions[I].Size = Value->Dimensions[I].Size;
		Stride *= Size;
	}
	return Array;
}

static ml_array_t *ML_TYPED_FN(ml_array_of_create, MLIntegerRangeT, ml_integer_range_t *Range, int Degree, ml_array_format_t Format) {
	size_t Count = 0;
	int64_t Diff = Range->Limit - Range->Start;
	if (!Range->Step) {
	} else if (Diff < 0 && Range->Step > 0) {
	} else if (Diff > 0 && Range->Step < 0) {
	} else {
		Count = Diff / Range->Step + 1;
	}
	if (!Count) return (ml_array_t *)ml_error("ValueError", "Empty dimension in array");
	ml_array_t *Array = ml_array_alloc(Format, Degree + 1);
	ml_array_dimension_t *Dimension = Array->Dimensions + Degree;
	Dimension->Stride = MLArraySizes[Format];
	Dimension->Size = Count;
	return Array;
}

static ml_array_t *ML_TYPED_FN(ml_array_of_create, MLRealRangeT, ml_real_range_t *Range, int Degree, ml_array_format_t Format) {
	if (!Range->Count) return (ml_array_t *)ml_error("ValueError", "Empty dimension in array");
	ml_array_t *Array = ml_array_alloc(Format, Degree + 1);
	ml_array_dimension_t *Dimension = Array->Dimensions + Degree;
	Dimension->Stride = MLArraySizes[Format];
	Dimension->Size = Range->Count;
	return Array;
}

static ml_value_t *ml_array_of_fill(ml_array_format_t Format, ml_array_dimension_t *Dimension, char *Address, int Degree, ml_value_t *Value) {
	typeof(ml_array_of_fill) *function = ml_typed_fn_get(ml_typeof(Value), ml_array_of_fill);
	if (function) return function(Format, Dimension, Address, Degree, Value);
	if (Degree) return ml_error("ValueError", "Inconsistent depth in array");
	switch (Format) {
	case ML_ARRAY_FORMAT_NONE: break;
	case ML_ARRAY_FORMAT_U8: *(uint8_t *)Address = ml_integer_value(Value); break;
	case ML_ARRAY_FORMAT_I8: *(int8_t *)Address = ml_integer_value(Value); break;
	case ML_ARRAY_FORMAT_U16: *(uint16_t *)Address = ml_integer_value(Value); break;
	case ML_ARRAY_FORMAT_I16: *(int16_t *)Address = ml_integer_value(Value); break;
	case ML_ARRAY_FORMAT_U32: *(uint32_t *)Address = ml_integer_value(Value); break;
	case ML_ARRAY_FORMAT_I32: *(int32_t *)Address = ml_integer_value(Value); break;
	case ML_ARRAY_FORMAT_U64: *(uint64_t *)Address = ml_integer_value(Value); break;
	case ML_ARRAY_FORMAT_I64: *(int64_t *)Address = ml_integer_value(Value); break;
	case ML_ARRAY_FORMAT_F32: *(float *)Address = ml_real_value(Value); break;
	case ML_ARRAY_FORMAT_F64: *(double *)Address = ml_real_value(Value); break;
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32: *(complex_float *)Address = ml_complex_value(Value); break;
	case ML_ARRAY_FORMAT_C64: *(complex_double *)Address = ml_complex_value(Value); break;
#endif
	case ML_ARRAY_FORMAT_ANY: *(ml_value_t **)Address = Value; break;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_array_of_fill, MLListT, ml_array_format_t Format, ml_array_dimension_t *Dimension, char *Address, int Degree, ml_value_t *Value) {
	if (!Degree) return ml_error("ValueError", "Inconsistent depth in array");
	if (ml_list_length(Value) != Dimension->Size) return ml_error("ValueError", "Inconsistent lengths in array");
	ML_LIST_FOREACH(Value, Iter) {
		ml_value_t *Error = ml_array_of_fill(Format, Dimension + 1, Address, Degree - 1, Iter->Value);
		if (Error) return Error;
		Address += Dimension->Stride;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_array_of_fill, MLTupleT, ml_array_format_t Format, ml_array_dimension_t *Dimension, char *Address, int Degree, ml_value_t *Value) {
	if (!Degree) return ml_error("ValueError", "Inconsistent depth in array");
	if (ml_tuple_size(Value) != Dimension->Size) return ml_error("ValueError", "Inconsistent lengths in array");
	ml_tuple_t *Tuple = (ml_tuple_t *)Value;
	for (int I = 0; I < Tuple->Size; ++I) {
		ml_value_t *Error = ml_array_of_fill(Format, Dimension + 1, Address, Degree - 1, Tuple->Values[I]);
		if (Error) return Error;
		Address += Dimension->Stride;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_array_of_fill, MLArrayT, ml_array_format_t Format, ml_array_dimension_t *Dimension, char *Address, int Degree, ml_value_t *Value) {
	ml_array_t *Source = (ml_array_t *)Value;
	if (Source->Degree != Degree) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
	for (int I = 0; I < Degree; ++I) {
		if (Dimension[I].Size != Source->Dimensions[I].Size) return ml_error("ArrayError", "Incompatible assignment (%d)", __LINE__);
	}
	update_row_fn_t Update = UpdateSetRowFns[Format * MAX_FORMATS + Source->Format];
	update_array(Update, Dimension, Address, Degree, Source->Dimensions, Source->Base.Value);
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_array_of_fill, MLIntegerRangeT, ml_array_format_t Format, ml_array_dimension_t *Dimension, char *Address, int Degree, ml_integer_range_t *Range) {
	if (!Degree) return ml_error("ValueError", "Inconsistent depth in array");
	size_t Count = 0;
	int64_t Diff = Range->Limit - Range->Start;
	if (!Range->Step) {
		Count = Dimension->Size;
	} else if (Diff < 0 && Range->Step > 0) {
		return ml_error("ValueError", "Inconsistent lengths in array");
	} else if (Diff > 0 && Range->Step < 0) {
		return ml_error("ValueError", "Inconsistent lengths in array");
	} else {
		Count = Diff / Range->Step + 1;
		if (Count < Dimension->Size) return ml_error("ValueError", "Inconsistent lengths in array");
		if (Count > Dimension->Size) Count = Dimension->Size;
	}
	int64_t Value = Range->Start, Step = Range->Step;
	switch (Format) {
	case ML_ARRAY_FORMAT_NONE: break;
	case ML_ARRAY_FORMAT_U8: {
		for (int I = 0; I < Count; ++I) {
			*(uint8_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_I8: {
		for (int I = 0; I < Count; ++I) {
			*(int8_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_U16: {
		for (int I = 0; I < Count; ++I) {
			*(uint16_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_I16: {
		for (int I = 0; I < Count; ++I) {
			*(int16_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_U32: {
		for (int I = 0; I < Count; ++I) {
			*(uint32_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_I32: {
		for (int I = 0; I < Count; ++I) {
			*(int32_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_U64: {
		for (int I = 0; I < Count; ++I) {
			*(uint64_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_I64: {
		for (int I = 0; I < Count; ++I) {
			*(int64_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_F32: {
		for (int I = 0; I < Count; ++I) {
			*(float *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_F64: {
		for (int I = 0; I < Count; ++I) {
			*(double *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32: {
		for (int I = 0; I < Count; ++I) {
			*(complex_float *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_C64: {
		for (int I = 0; I < Count; ++I) {
			*(complex_double *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
#endif
	case ML_ARRAY_FORMAT_ANY: {
		for (int I = 0; I < Count; ++I) {
			*(ml_value_t **)Address = ml_integer(Value);
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_array_of_fill, MLRealRangeT, ml_array_format_t Format, ml_array_dimension_t *Dimension, char *Address, int Degree, ml_real_range_t *Range) {
	if (!Degree) return ml_error("ValueError", "Inconsistent depth in array");
	size_t Count = Range->Count;
	if (Count != Dimension->Size) return ml_error("ValueError", "Inconsistent lengths in array");
	double Value = Range->Start, Step = Range->Step;
	switch (Format) {
	case ML_ARRAY_FORMAT_NONE: break;
	case ML_ARRAY_FORMAT_U8: {
		for (int I = 0; I < Count; ++I) {
			*(uint8_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_I8: {
		for (int I = 0; I < Count; ++I) {
			*(int8_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_U16: {
		for (int I = 0; I < Count; ++I) {
			*(uint16_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_I16: {
		for (int I = 0; I < Count; ++I) {
			*(int16_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_U32: {
		for (int I = 0; I < Count; ++I) {
			*(uint32_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_I32: {
		for (int I = 0; I < Count; ++I) {
			*(int32_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_U64: {
		for (int I = 0; I < Count; ++I) {
			*(uint64_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_I64: {
		for (int I = 0; I < Count; ++I) {
			*(int64_t *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_F32: {
		for (int I = 0; I < Count; ++I) {
			*(float *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_F64: {
		for (int I = 0; I < Count; ++I) {
			*(double *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32: {
		for (int I = 0; I < Count; ++I) {
			*(complex_float *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	case ML_ARRAY_FORMAT_C64: {
		for (int I = 0; I < Count; ++I) {
			*(complex_double *)Address = Value;
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
#endif
	case ML_ARRAY_FORMAT_ANY: {
		for (int I = 0; I < Count; ++I) {
			*(ml_value_t **)Address = ml_integer(Value);
			Value += Step;
			Address += Dimension->Stride;
		}
		break;
	}
	}
	return NULL;
}

static ml_value_t *ml_array_of_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Source = Args[0];
	ml_array_format_t Format = ML_ARRAY_FORMAT_NONE;
	if (Count == 2) {
		ML_CHECK_ARG_TYPE(0, MLTypeT);
		Format = ml_array_format((ml_type_t *)Args[0]);
		if (Format == ML_ARRAY_FORMAT_NONE) return ml_error("TypeError", "Unknown type for array");
		Source = Args[1];
	} else {
		Format = ml_array_of_type_guess(Args[0], ML_ARRAY_FORMAT_NONE);
	}
	ml_array_t *Array = ml_array_of_create(Source, 0, Format);
	if (Array->Base.Type == MLErrorT) return (ml_value_t *)Array;
	int Degree = Array->Degree;
	ml_array_dimension_t *Dimensions = Array->Dimensions;
	size_t Size = Dimensions->Size * Dimensions->Stride;
	char *Address = Array->Base.Value = snew(Size);
	ml_value_t *Error = ml_array_of_fill(Array->Format, Dimensions, Address, Degree, Source);
	return Error ?: (ml_value_t *)Array;
}

ML_METHOD("copy", MLArrayT) {
//<Array
//>array
// Return a new array with the same values of :mini:`Array` but not sharing the underlying data.
	ml_array_t *Source = (ml_array_t *)Args[0];
	if (Source->Degree == -1) return (ml_value_t *)Source;
	ml_array_t *Target = ml_array_alloc(Source->Format, Source->Degree);
	array_copy(Target, Source);
	return (ml_value_t *)Target;
}

ML_METHOD("copy", MLVisitorT, MLArrayT) {
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_array_t *Source = (ml_array_t *)Args[1];
	if (Source->Degree == -1) return (ml_value_t *)Source;
	ml_array_t *Target = ml_array_alloc(Source->Format, Source->Degree);
	inthash_insert(Visitor->Cache, (uintptr_t)Source, Target);
	if (Source->Format == ML_ARRAY_FORMAT_ANY) {
		array_copy(Target, Source);
		// TODO: Use Visitor to make a copy of each value in Target.
	} else {
		array_copy(Target, Source);
	}
	return (ml_value_t *)Target;
}

ML_METHOD("const", MLVisitorT, MLArrayT) {
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_array_t *Source = (ml_array_t *)Args[1];
	if (Source->Degree == -1) return (ml_value_t *)Source;
	ml_array_t *Target = ml_array_alloc(Source->Format, Source->Degree);
	switch (Target->Degree) {
	case 1: Target->Base.Type = MLVectorTypes[Target->Format]; break;
	case 2: Target->Base.Type = MLMatrixTypes[Target->Format]; break;
	default: Target->Base.Type = MLArrayTypes[Target->Format]; break;
	}
	inthash_insert(Visitor->Cache, (uintptr_t)Source, Target);
	if (Source->Format == ML_ARRAY_FORMAT_ANY) {
		array_copy(Target, Source);
		// TODO: Use Visitor to make a copy of each value in Target.
	} else {
		array_copy(Target, Source);
	}
	return (ml_value_t *)Target;
}

/*
ML_METHOD("$", MLListT) {
//<List
//>array
// Returns an array with the contents of :mini:`List`.
}
*/

ML_METHOD("^", MLListT) {
//<List
//>array
// Returns an array with the contents of :mini:`List`, transposed.
	ml_value_t *Source = Args[0];
	ml_array_format_t Format = ML_ARRAY_FORMAT_NONE;
	Format = ml_array_of_type_guess(Args[0], ML_ARRAY_FORMAT_NONE);
	ml_array_t *Array = ml_array_of_create(Source, 1, Format);
	if (Array->Base.Type == MLErrorT) return (ml_value_t *)Array;
	int Degree = Array->Degree;
	ml_array_dimension_t *Dimensions = Array->Dimensions;
	for (int I = 1; I < Array->Degree; ++I) Dimensions[I - 1] = Dimensions[I];
	size_t Size = Dimensions->Size * Dimensions->Stride;
	char *Address = Array->Base.Value = snew(Size);
	ml_value_t *Error = ml_array_of_fill(Array->Format, Dimensions, Address, Degree - 1, Source);
	Array->Dimensions[Degree - 1].Size = 1;
	return Error ?: (ml_value_t *)Array;
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
#ifdef ML_COMPLEX
		complex_float *C32;
		complex_double *C64;
#endif
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

ARRAY_APPLY(U8, uint8_t, ml_integer, ml_integer_value);
ARRAY_APPLY(I8, int8_t, ml_integer, ml_integer_value);
ARRAY_APPLY(U16, uint16_t, ml_integer, ml_integer_value);
ARRAY_APPLY(I16, int16_t, ml_integer, ml_integer_value);
ARRAY_APPLY(U32, uint32_t, ml_integer, ml_integer_value);
ARRAY_APPLY(I32, int32_t, ml_integer, ml_integer_value);
ARRAY_APPLY(U64, uint64_t, ml_integer, ml_integer_value);
ARRAY_APPLY(I64, int64_t, ml_integer, ml_integer_value);
ARRAY_APPLY(F32, float, ml_real, ml_real_value);
ARRAY_APPLY(F64, double, ml_real, ml_real_value);

#ifdef ML_COMPLEX

ARRAY_APPLY(C32, complex_float, ml_complex, ml_complex_value);
ARRAY_APPLY(C64, complex_double, ml_complex, ml_complex_value);

#endif

ARRAY_APPLY(Any, any, , );

ML_METHODX("copy", MLArrayT, MLFunctionT) {
//<Array
//<Function
//>array
// Return a new array with the results of applying :mini:`Function` to each value of :mini:`Array`.
	ml_array_t *A = (ml_array_t *)Args[0];
	int Degree = A->Degree;
	if (Degree == -1) ML_RETURN(A);
	ml_array_t *C = ml_array_alloc(A->Format, Degree);
	int Remaining = array_copy(C, A) / MLArraySizes[C->Format];
	if (Remaining == 0) ML_RETURN(C);
	ml_array_apply_state_t *State = new(ml_array_apply_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	ml_value_t *Function = State->Function = Args[1];
	State->Remaining = Remaining;
	State->Values = C->Base.Value;
	State->Array = (ml_value_t *)C;
	switch (C->Format) {
	case ML_ARRAY_FORMAT_U8: {
		State->Base.run = (void *)ml_array_apply_uint8_t;
		State->Args[0] = ml_integer(*State->U8);
		break;
	}
	case ML_ARRAY_FORMAT_I8: {
		State->Base.run = (void *)ml_array_apply_int8_t;
		State->Args[0] = ml_integer(*State->I8);
		break;
	}
	case ML_ARRAY_FORMAT_U16: {
		State->Base.run = (void *)ml_array_apply_uint16_t;
		State->Args[0] = ml_integer(*State->U16);
		break;
	}
	case ML_ARRAY_FORMAT_I16: {
		State->Base.run = (void *)ml_array_apply_int16_t;
		State->Args[0] = ml_integer(*State->I16);
		break;
	}
	case ML_ARRAY_FORMAT_U32: {
		State->Base.run = (void *)ml_array_apply_uint32_t;
		State->Args[0] = ml_integer(*State->U32);
		break;
	}
	case ML_ARRAY_FORMAT_I32: {
		State->Base.run = (void *)ml_array_apply_int32_t;
		State->Args[0] = ml_integer(*State->I32);
		break;
	}
	case ML_ARRAY_FORMAT_U64: {
		State->Base.run = (void *)ml_array_apply_uint64_t;
		State->Args[0] = ml_integer(*State->U64);
		break;
	}
	case ML_ARRAY_FORMAT_I64: {
		State->Base.run = (void *)ml_array_apply_int64_t;
		State->Args[0] = ml_integer(*State->I64);
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
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32: {
		State->Base.run = (void *)ml_array_apply_complex_float;
		State->Args[0] = ml_complex(*State->C32);
		break;
	}
	case ML_ARRAY_FORMAT_C64: {
		State->Base.run = (void *)ml_array_apply_complex_double;
		State->Args[0] = ml_complex(*State->C64);
		break;
	}
#endif
	case ML_ARRAY_FORMAT_ANY: {
		State->Base.run = (void *)ml_array_apply_any;
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

ARRAY_UPDATE(uint8_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(int8_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(uint16_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(int16_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(uint32_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(int32_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(uint64_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(int64_t, ml_integer, ml_integer_value);
ARRAY_UPDATE(float, ml_real, ml_real_value);
ARRAY_UPDATE(double, ml_real, ml_real_value);

#ifdef ML_COMPLEX

ARRAY_UPDATE(complex_float, ml_complex, ml_complex_value);
ARRAY_UPDATE(complex_double, ml_complex, ml_complex_value);

#endif

ARRAY_UPDATE(any, , );

ML_METHODX("update", MLArrayMutableT, MLFunctionT) {
//<Array
//<Function
//>array
// Update the values in :mini:`Array` in place by applying :mini:`Function` to each value.
	ml_array_t *A = (ml_array_t *)Args[0];
	int Degree = A->Degree;
	if (Degree == -1) ML_RETURN(A);
	ml_array_update_state_t *State = xnew(ml_array_update_state_t, Degree, int);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	ml_value_t *Function = State->Function = Args[1];
	State->Array = A;
	State->Degree = Degree;
	char *Address = A->Base.Value;
	ml_array_dimension_t *Dimension = A->Dimensions;
	for (int I = 0; I < Degree; ++I, ++Dimension) {
		if (Dimension->Indices) {
			Address += Dimension->Indices[0] * Dimension->Stride;
		}
	}
	State->Address = Address;
	switch (A->Format) {
	case ML_ARRAY_FORMAT_U8: {
		State->Base.run = (void *)ml_array_update_uint8_t;
		State->Args[0] = ml_integer(*(uint8_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I8: {
		State->Base.run = (void *)ml_array_update_int8_t;
		State->Args[0] = ml_integer(*(int8_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U16: {
		State->Base.run = (void *)ml_array_update_uint16_t;
		State->Args[0] = ml_integer(*(uint16_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I16: {
		State->Base.run = (void *)ml_array_update_int16_t;
		State->Args[0] = ml_integer(*(int16_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U32: {
		State->Base.run = (void *)ml_array_update_uint32_t;
		State->Args[0] = ml_integer(*(uint32_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I32: {
		State->Base.run = (void *)ml_array_update_int32_t;
		State->Args[0] = ml_integer(*(int32_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U64: {
		State->Base.run = (void *)ml_array_update_uint64_t;
		State->Args[0] = ml_integer(*(uint64_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I64: {
		State->Base.run = (void *)ml_array_update_int64_t;
		State->Args[0] = ml_integer(*(int64_t *)Address);
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
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32: {
		State->Base.run = (void *)ml_array_update_complex_float;
		State->Args[0] = ml_complex(*(complex_float *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_C64: {
		State->Base.run = (void *)ml_array_update_complex_double;
		State->Args[0] = ml_complex(*(complex_double *)Address);
		break;
	}
#endif
	case ML_ARRAY_FORMAT_ANY: {
		State->Base.run = (void *)ml_array_update_any;
		State->Args[0] = *(ml_value_t **)Address;
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
	ml_value_t *Result;
	int Degree;
	int Indices[];
} ml_array_where_state_t;

#define ARRAY_WHERE(CTYPE, TO_VAL, TO_NUM) \
static void ml_array_where_ ## CTYPE(ml_array_where_state_t *State, ml_value_t *Value) { \
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value); \
	if (Value != MLNil) { \
		int Degree = State->Array->Degree; \
		if (Degree > 1) { \
			ml_value_t *Index = ml_tuple(Degree); \
			for (int J = 1; J < Degree; ++J) { \
				((ml_tuple_t *)Index)->Values[J] = ml_integer(State->Indices[J] + 1); \
			} \
			ml_tuple_set(Index, 1, ml_integer(State->Indices[0] + 1)); \
			ml_list_put(State->Result, Index); \
		} else { \
			ml_list_put(State->Result, ml_integer(State->Indices[0] + 1)); \
		} \
	} \
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
	ML_CONTINUE(State->Base.Caller, State->Result); \
}

ARRAY_WHERE(uint8_t, ml_integer, ml_integer_value);
ARRAY_WHERE(int8_t, ml_integer, ml_integer_value);
ARRAY_WHERE(uint16_t, ml_integer, ml_integer_value);
ARRAY_WHERE(int16_t, ml_integer, ml_integer_value);
ARRAY_WHERE(uint32_t, ml_integer, ml_integer_value);
ARRAY_WHERE(int32_t, ml_integer, ml_integer_value);
ARRAY_WHERE(uint64_t, ml_integer, ml_integer_value);
ARRAY_WHERE(int64_t, ml_integer, ml_integer_value);
ARRAY_WHERE(float, ml_real, ml_real_value);
ARRAY_WHERE(double, ml_real, ml_real_value);

#ifdef ML_COMPLEX

ARRAY_WHERE(complex_float, ml_complex, ml_complex_value);
ARRAY_WHERE(complex_double, ml_complex, ml_complex_value);

#endif

ARRAY_WHERE(any, , );

ML_METHODX("where", MLArrayT, MLFunctionT) {
//<Array
//<Function
//>list[tuple]
// Returns list of indices :mini:`Array` where :mini:`Function(Array/i)` returns a non-nil value.
	ml_array_t *A = (ml_array_t *)Args[0];
	int Degree = A->Degree;
	if (Degree == -1) ML_RETURN(ml_list());
	ml_array_where_state_t *State = xnew(ml_array_where_state_t, Degree, int);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	ml_value_t *Function = State->Function = Args[1];
	State->Result = ml_list();
	State->Array = A;
	State->Degree = Degree;
	char *Address = A->Base.Value;
	ml_array_dimension_t *Dimension = A->Dimensions;
	for (int I = 0; I < Degree; ++I, ++Dimension) {
		if (Dimension->Indices) {
			Address += Dimension->Indices[0] * Dimension->Stride;
		}
	}
	State->Address = Address;
	switch (A->Format) {
	case ML_ARRAY_FORMAT_U8: {
		State->Base.run = (void *)ml_array_where_uint8_t;
		State->Args[0] = ml_integer(*(uint8_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I8: {
		State->Base.run = (void *)ml_array_where_int8_t;
		State->Args[0] = ml_integer(*(int8_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U16: {
		State->Base.run = (void *)ml_array_where_uint16_t;
		State->Args[0] = ml_integer(*(uint16_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I16: {
		State->Base.run = (void *)ml_array_where_int16_t;
		State->Args[0] = ml_integer(*(int16_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U32: {
		State->Base.run = (void *)ml_array_where_uint32_t;
		State->Args[0] = ml_integer(*(uint32_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I32: {
		State->Base.run = (void *)ml_array_where_int32_t;
		State->Args[0] = ml_integer(*(int32_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_U64: {
		State->Base.run = (void *)ml_array_where_uint64_t;
		State->Args[0] = ml_integer(*(uint64_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_I64: {
		State->Base.run = (void *)ml_array_where_int64_t;
		State->Args[0] = ml_integer(*(int64_t *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_F32: {
		State->Base.run = (void *)ml_array_where_float;
		State->Args[0] = ml_real(*(float *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_F64: {
		State->Base.run = (void *)ml_array_where_double;
		State->Args[0] = ml_real(*(double *)Address);
		break;
	}
#ifdef ML_COMPLEX
	case ML_ARRAY_FORMAT_C32: {
		State->Base.run = (void *)ml_array_where_complex_float;
		State->Args[0] = ml_complex(*(complex_float *)Address);
		break;
	}
	case ML_ARRAY_FORMAT_C64: {
		State->Base.run = (void *)ml_array_where_complex_double;
		State->Args[0] = ml_complex(*(complex_double *)Address);
		break;
	}
#endif
	case ML_ARRAY_FORMAT_ANY: {
		State->Base.run = (void *)ml_array_where_any;
		State->Args[0] = *(ml_value_t **)Address;
		break;
	}
	default: ML_ERROR("ArrayError", "Unsupported format");
	}
	return ml_call(State, Function, 1, State->Args);
}

typedef struct {
	ml_value_t *Result;
	ml_value_t **Indices;
	int Degree;
} ml_array_where_nonzero_t;

#define ARRAY_WHERE_NONZERO(CTYPE, ZERO) \
static void ml_array_where_nonzero_ ## CTYPE(ml_array_where_nonzero_t *Where, void *Address, int Degree, ml_array_dimension_t *Dimension) { \
	if (Degree == Where->Degree) { \
		int Size = Dimension->Size; \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				if (*(CTYPE *)(Address + Indices[I] * Stride) != ZERO) { \
					ml_value_t *Index = ml_tuple(Where->Degree); \
					for (int J = 0; J < Where->Degree - 1; ++J) { \
						((ml_tuple_t *)Index)->Values[J] = Where->Indices[J]; \
					} \
					ml_tuple_set(Index, Where->Degree, ml_integer(I + 1)); \
					ml_list_put(Where->Result, Index); \
				} \
			} \
		} else { \
			for (int I = 0; I < Size; ++I) { \
				if (*(CTYPE *)(Address + I * Stride) != ZERO) { \
					ml_value_t *Index = ml_tuple(Where->Degree); \
					for (int J = 0; J < Where->Degree - 1; ++J) { \
						((ml_tuple_t *)Index)->Values[J] = Where->Indices[J]; \
					} \
					ml_tuple_set(Index, Where->Degree, ml_integer(I + 1)); \
					ml_list_put(Where->Result, Index); \
				} \
			} \
		} \
	} else { \
		int Size = Dimension->Size; \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices;\
			for (int I = 0; I < Size; ++I) { \
				Where->Indices[Degree - 1] = ml_integer(I + 1); \
				ml_array_where_nonzero_ ## CTYPE(Where, Address + Indices[I] * Stride, Degree + 1, Dimension + 1); \
			} \
		} else { \
			for (int I = 0; I < Size; ++I) { \
				Where->Indices[Degree - 1] = ml_integer(I + 1); \
				ml_array_where_nonzero_ ## CTYPE(Where, Address + I * Stride, Degree + 1, Dimension + 1); \
			} \
		} \
	} \
} \
\
static void ml_array_where_nonzero1_ ## CTYPE(ml_value_t *Result, void *Address, ml_array_dimension_t *Dimension) { \
	int Size = Dimension->Size; \
	int Stride = Dimension->Stride; \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		for (int I = 0; I < Size; ++I) { \
			if (*(CTYPE *)(Address + Indices[I] * Stride) != ZERO) { \
				ml_list_put(Result, ml_integer(I + 1)); \
			} \
		} \
	} else { \
		for (int I = 0; I < Size; ++I) { \
			if (*(CTYPE *)(Address + I * Stride) != ZERO) { \
				ml_list_put(Result, ml_integer(I + 1)); \
			} \
		} \
	} \
}

ARRAY_WHERE_NONZERO(uint8_t, 0);
ARRAY_WHERE_NONZERO(int8_t, 0);
ARRAY_WHERE_NONZERO(uint16_t, 0);
ARRAY_WHERE_NONZERO(int16_t, 0);
ARRAY_WHERE_NONZERO(uint32_t, 0);
ARRAY_WHERE_NONZERO(int32_t, 0);
ARRAY_WHERE_NONZERO(uint64_t, 0);
ARRAY_WHERE_NONZERO(int64_t, 0);
ARRAY_WHERE_NONZERO(float, 0);
ARRAY_WHERE_NONZERO(double, 0);

#ifdef ML_COMPLEX

ARRAY_WHERE_NONZERO(complex_float, 0);
ARRAY_WHERE_NONZERO(complex_double, 0);

#endif

ARRAY_WHERE_NONZERO(any, MLNil);

ML_METHOD("where", MLArrayT) {
//<Array
//>list
// Returns a list of non-zero indices of :mini:`Array`.
	ml_array_t *A = (ml_array_t *)Args[0];
	if (A->Degree == -1) return ml_list();
	if (A->Degree == 1) {
		ml_value_t *Result = ml_list();
		switch (A->Format) {
		case ML_ARRAY_FORMAT_U8: {
			ml_array_where_nonzero1_uint8_t(Result, A->Base.Value, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_I8: {
			ml_array_where_nonzero1_int8_t(Result, A->Base.Value, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_U16: {
			ml_array_where_nonzero1_uint16_t(Result, A->Base.Value, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_I16: {
			ml_array_where_nonzero1_int16_t(Result, A->Base.Value, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_U32: {
			ml_array_where_nonzero1_uint32_t(Result, A->Base.Value, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_I32: {
			ml_array_where_nonzero1_int32_t(Result, A->Base.Value, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_U64: {
			ml_array_where_nonzero1_uint64_t(Result, A->Base.Value, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_I64: {
			ml_array_where_nonzero1_int64_t(Result, A->Base.Value, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_F32: {
			ml_array_where_nonzero1_float(Result, A->Base.Value, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_F64: {
			ml_array_where_nonzero1_double(Result, A->Base.Value, A->Dimensions);
			break;
		}
	#ifdef ML_COMPLEX
		case ML_ARRAY_FORMAT_C32: {
			ml_array_where_nonzero1_complex_float(Result, A->Base.Value, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_C64: {
			ml_array_where_nonzero1_complex_double(Result, A->Base.Value, A->Dimensions);
			break;
		}
	#endif
		case ML_ARRAY_FORMAT_ANY: {
			ml_array_where_nonzero1_any(Result, A->Base.Value, A->Dimensions);
			break;
		}
		default: return ml_error("ArrayError", "Unsupported format");
		}
		return Result;
	} else {
		ml_array_where_nonzero_t Where[1];
		int Degree = Where->Degree = A->Degree;
		ml_value_t *Indices[Degree];
		Where->Indices = Indices;
		Where->Result = ml_list();
		switch (A->Format) {
		case ML_ARRAY_FORMAT_U8: {
			ml_array_where_nonzero_uint8_t(Where, A->Base.Value, 1, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_I8: {
			ml_array_where_nonzero_int8_t(Where, A->Base.Value, 1, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_U16: {
			ml_array_where_nonzero_uint16_t(Where, A->Base.Value, 1, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_I16: {
			ml_array_where_nonzero_int16_t(Where, A->Base.Value, 1, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_U32: {
			ml_array_where_nonzero_uint32_t(Where, A->Base.Value, 1, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_I32: {
			ml_array_where_nonzero_int32_t(Where, A->Base.Value, 1, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_U64: {
			ml_array_where_nonzero_uint64_t(Where, A->Base.Value, 1, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_I64: {
			ml_array_where_nonzero_int64_t(Where, A->Base.Value, 1, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_F32: {
			ml_array_where_nonzero_float(Where, A->Base.Value, 1, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_F64: {
			ml_array_where_nonzero_double(Where, A->Base.Value, 1, A->Dimensions);
			break;
		}
	#ifdef ML_COMPLEX
		case ML_ARRAY_FORMAT_C32: {
			ml_array_where_nonzero_complex_float(Where, A->Base.Value, 1, A->Dimensions);
			break;
		}
		case ML_ARRAY_FORMAT_C64: {
			ml_array_where_nonzero_complex_double(Where, A->Base.Value, 1, A->Dimensions);
			break;
		}
	#endif
		case ML_ARRAY_FORMAT_ANY: {
			ml_array_where_nonzero_any(Where, A->Base.Value, 1, A->Dimensions);
			break;
		}
		default: return ml_error("ArrayError", "Unsupported format");
		}
		return Where->Result;
	}
}

#define ML_ARRAY_GETTER_IMPL(CTYPE, ATYPE) \
static CTYPE ml_array_getter_ ## CTYPE ## _ ## ATYPE(void *Data) { \
	return (CTYPE)(*(ATYPE *)Data); \
}

#define ML_ARRAY_GETTERS_IMPL_BASE(CTYPE, FROM_VAL) \
\
typedef CTYPE (*ml_array_getter_ ## CTYPE)(void *Data); \
\
ML_ARRAY_GETTER_IMPL(CTYPE, uint8_t) \
ML_ARRAY_GETTER_IMPL(CTYPE, int8_t) \
ML_ARRAY_GETTER_IMPL(CTYPE, uint16_t) \
ML_ARRAY_GETTER_IMPL(CTYPE, int16_t) \
ML_ARRAY_GETTER_IMPL(CTYPE, uint32_t) \
ML_ARRAY_GETTER_IMPL(CTYPE, int32_t) \
ML_ARRAY_GETTER_IMPL(CTYPE, uint64_t) \
ML_ARRAY_GETTER_IMPL(CTYPE, int64_t) \
ML_ARRAY_GETTER_IMPL(CTYPE, float) \
ML_ARRAY_GETTER_IMPL(CTYPE, double) \
\
static CTYPE ml_array_getter_ ## CTYPE ## _ ## any(void *Data) { \
	return FROM_VAL(*(ml_value_t **)Data); \
}

#ifdef ML_COMPLEX

#define ML_ARRAY_GETTERS_IMPL(CTYPE, FROM_VAL) \
ML_ARRAY_GETTERS_IMPL_BASE(CTYPE, FROM_VAL) \
ML_ARRAY_GETTER_IMPL(CTYPE, complex_float) \
ML_ARRAY_GETTER_IMPL(CTYPE, complex_double)

#else

#define ML_ARRAY_GETTERS_IMPL(CTYPE, FROM_VAL) ML_ARRAY_GETTERS_IMPL_BASE(CTYPE, FROM_VAL)

#endif

ML_ARRAY_GETTERS_IMPL(uint8_t, ml_integer_value)
ML_ARRAY_GETTERS_IMPL(int8_t, ml_integer_value)
ML_ARRAY_GETTERS_IMPL(uint16_t, ml_integer_value)
ML_ARRAY_GETTERS_IMPL(int16_t, ml_integer_value)
ML_ARRAY_GETTERS_IMPL(uint32_t, ml_integer_value)
ML_ARRAY_GETTERS_IMPL(int32_t, ml_integer_value)
ML_ARRAY_GETTERS_IMPL(uint64_t, ml_integer_value)
ML_ARRAY_GETTERS_IMPL(int64_t, ml_integer_value)
ML_ARRAY_GETTERS_IMPL(float, ml_real_value)
ML_ARRAY_GETTERS_IMPL(double, ml_real_value)

#ifdef ML_COMPLEX

ML_ARRAY_GETTERS_IMPL(complex_float, ml_complex_value)
ML_ARRAY_GETTERS_IMPL(complex_double, ml_complex_value)

#endif

#define ML_ARRAY_GETTER_ANY_IMPL(TO_VAL, ATYPE) \
static any ml_array_getter_any_ ## ATYPE(void *Data) { \
	return TO_VAL(*(ATYPE *)Data); \
}

typedef any (*ml_array_getter_any)(void *Data);

ML_ARRAY_GETTER_ANY_IMPL(ml_integer, uint8_t)
ML_ARRAY_GETTER_ANY_IMPL(ml_integer, int8_t)
ML_ARRAY_GETTER_ANY_IMPL(ml_integer, uint16_t)
ML_ARRAY_GETTER_ANY_IMPL(ml_integer, int16_t)
ML_ARRAY_GETTER_ANY_IMPL(ml_integer, uint32_t)
ML_ARRAY_GETTER_ANY_IMPL(ml_integer, int32_t)
ML_ARRAY_GETTER_ANY_IMPL(ml_integer, uint64_t)
ML_ARRAY_GETTER_ANY_IMPL(ml_integer, int64_t)
ML_ARRAY_GETTER_ANY_IMPL(ml_real, float)
ML_ARRAY_GETTER_ANY_IMPL(ml_real, double)

#ifdef ML_COMPLEX

ML_ARRAY_GETTER_ANY_IMPL(ml_complex, complex_float)
ML_ARRAY_GETTER_ANY_IMPL(ml_complex, complex_double)

#endif

static ml_value_t *ml_array_getter_any_any(void *Data) {
	return *(ml_value_t **)Data;
}

#ifdef ML_COMPLEX

#define ML_ARRAY_GETTER_DECLS(NAME, CTYPE) \
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
}

#else

#define ML_ARRAY_GETTER_DECLS(NAME, CTYPE) \
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
}

#endif

ML_ARRAY_GETTER_DECLS(UInt8, uint8_t);
ML_ARRAY_GETTER_DECLS(Int8, int8_t);
ML_ARRAY_GETTER_DECLS(UInt16, uint16_t);
ML_ARRAY_GETTER_DECLS(Int16, int16_t);
ML_ARRAY_GETTER_DECLS(UInt32, uint32_t);
ML_ARRAY_GETTER_DECLS(Int32, int32_t);
ML_ARRAY_GETTER_DECLS(UInt64, uint64_t);
ML_ARRAY_GETTER_DECLS(Int64, int64_t);
ML_ARRAY_GETTER_DECLS(Float32, float);
ML_ARRAY_GETTER_DECLS(Float64, double);

#ifdef ML_COMPLEX

ML_ARRAY_GETTER_DECLS(Complex32, complex_float);
ML_ARRAY_GETTER_DECLS(Complex64, complex_double);

#endif

ML_ARRAY_GETTER_DECLS(Any, any);

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
		int *IndicesA = DimA->Indices; \
		if (DimB->Indices) { \
			int *IndicesB = DimB->Indices; \
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
			int *IndicesB = DimB->Indices; \
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
		int *IndicesA = DimA->Indices;
		if (DimB->Indices) {
			int *IndicesB = DimB->Indices;
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
			int *IndicesB = DimB->Indices;
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
			int *Indices = DimA->Indices;
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
			int *Indices = DimB->Indices;
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
			int *Indices = DimA->Indices;
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
			int *Indices = DimB->Indices;
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
	C->Base.Value = snew(DataSize);
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
		int Stride = B->Dimensions->Stride, *Indices = B->Dimensions->Indices;
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
		int Stride = B->Dimensions->Stride, *Indices = B->Dimensions->Indices;
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
		int Stride = B->Dimensions->Stride, *Indices = B->Dimensions->Indices;
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
	C->Base.Value = snew(DataSize);
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

ML_METHOD("\\", MLMatrixT) {
//<A
//>matrix
// Returns the inverse of :mini:`A`.
	ml_array_t *Source = (ml_array_t *)Args[0];
	int N = Source->Dimensions[0].Size;
	if (N != Source->Dimensions[1].Size) return ml_error("ShapeError", "Square matrix required");
	if (Source->Format <= ML_ARRAY_FORMAT_F64) {
		ml_array_t *Inv = ml_array_alloc(ML_ARRAY_FORMAT_F64, 2);
		array_copy(Inv, Source);
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
		array_copy(Inv, Source);
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
		array_copy(Tmp, Source);
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
		array_copy(Sol, B);
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
		array_copy(Tmp, Source);
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
		array_copy(Sol, B);
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
		array_copy(Tmp, Source);
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
		array_copy(Tmp, Source);
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
			int *Indices0 = Source->Dimensions[0].Indices;
			if (Source->Dimensions[1].Indices) {
				int *Indices1 = Source->Dimensions[1].Indices;
				for (int I = 0; I < N; ++I) {
					Trace += get(Data + (Indices0[I] * Stride0) + (Indices1[I] * Stride1));
				}
			} else {
				for (int I = 0; I < N; ++I) {
					Trace += get(Data + (Indices0[I] * Stride0) + (I * Stride1));
				}
			}
		} else if (Source->Dimensions[1].Indices) {
			int *Indices1 = Source->Dimensions[1].Indices;
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
			int *Indices0 = Source->Dimensions[0].Indices;
			if (Source->Dimensions[1].Indices) {
				int *Indices1 = Source->Dimensions[1].Indices;
				for (int I = 0; I < N; ++I) {
					Trace += get(Data + (Indices0[I] * Stride0) + (Indices1[I] * Stride1));
				}
			} else {
				for (int I = 0; I < N; ++I) {
					Trace += get(Data + (Indices0[I] * Stride0) + (I * Stride1));
				}
			}
		} else if (Source->Dimensions[1].Indices) {
			int *Indices1 = Source->Dimensions[1].Indices;
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
			int *Indices0 = Source->Dimensions[0].Indices;
			if (Source->Dimensions[1].Indices) {
				int *Indices1 = Source->Dimensions[1].Indices;
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
			int *Indices1 = Source->Dimensions[1].Indices;
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

ML_METHOD("softmax", MLVectorMutableRealT) {
//<Vector
//>vector
// Returns :mini:`softmax(Vector)`.
//$= let A := array([1, 4.2, 0.6, 1.23, 4.3, 1.2, 2.5])
//$= let B := A:softmax
	ml_array_t *A = (ml_array_t *)Args[0];
	int N = A->Dimensions[0].Size;
	ml_array_t *B = ml_array(ML_ARRAY_FORMAT_F64, 1, N);
	array_copy(B, A);
	double *Values = (double *)B->Base.Value;
	double M = -INFINITY;
	for (int I = 0; I < N; ++I) if (M < Values[I]) M = Values[I];
	double Sum = 0.0;
	for (int I = 0; I < N; ++I) Sum += exp(Values[I] - M);
	double C = M + log(Sum);
	for (int I = 0; I < N; ++I) Values[I] = exp(Values[I] - C);
	return (ml_value_t *)B;
}

#ifdef ML_CBOR

#include "ml_cbor.h"
#include "minicbor/minicbor.h"

static void ml_cbor_write_array_typed(int Degree, size_t FlatSize, ml_array_dimension_t *Dimension, char *Address, ml_cbor_writer_t *Writer) {
	if (Degree < 0) {
		ml_cbor_write_raw(Writer, (unsigned char *)Address, FlatSize);
	} else {
		int Stride = Dimension->Stride;
		if (Dimension->Indices) {
			int *Indices = Dimension->Indices;
			for (int I = 0; I < Dimension->Size; ++I) {
				ml_cbor_write_array_typed(Degree - 1, FlatSize, Dimension + 1, Address + Indices[I] * Stride, Writer);
			}
		} else {
			for (int I = Dimension->Size; --I >= 0;) {
				ml_cbor_write_array_typed(Degree - 1, FlatSize, Dimension + 1, Address, Writer);
				Address += Stride;
			}
		}
	}
}

static void ml_cbor_write_array_any(int Degree, ml_array_dimension_t *Dimension, char *Address, ml_cbor_writer_t *Writer) {
	if (Degree == 0) {
		ml_cbor_write(Writer, *(ml_value_t **)Address);
	} else {
		int Stride = Dimension->Stride;
		if (Dimension->Indices) {
			int *Indices = Dimension->Indices;
			for (int I = 0; I < Dimension->Size; ++I) {
				ml_cbor_write_array_any(Degree - 1, Dimension + 1, Address + Indices[I] * Stride, Writer);
			}
		} else {
			for (int I = Dimension->Size; --I >= 0;) {
				ml_cbor_write_array_any(Degree - 1, Dimension + 1, Address, Writer);
				Address += Stride;
			}
		}
	}
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLArrayT, ml_cbor_writer_t *Writer, ml_array_t *Array) {
	static uint64_t Tags[] = {
		[ML_ARRAY_FORMAT_U8] = 64,
		[ML_ARRAY_FORMAT_I8] = 72,
		[ML_ARRAY_FORMAT_U16] = 69,
		[ML_ARRAY_FORMAT_I16] = 77,
		[ML_ARRAY_FORMAT_U32] = 70,
		[ML_ARRAY_FORMAT_I32] = 78,
		[ML_ARRAY_FORMAT_U64] = 71,
		[ML_ARRAY_FORMAT_I64] = 79,
		[ML_ARRAY_FORMAT_F32] = 85,
		[ML_ARRAY_FORMAT_F64] = 86
	};
	if (Array->Degree == -1) {
		ml_cbor_write_simple(Writer, CBOR_SIMPLE_NULL);
		return NULL;
	}
	ml_cbor_write_tag(Writer, 40);
	ml_cbor_write_array(Writer, 2);
	ml_cbor_write_array(Writer, Array->Degree);
	if (Array->Format == ML_ARRAY_FORMAT_ANY) {
		size_t Size = 1;
		for (int I = 0; I < Array->Degree; ++I) {
			Size *= Array->Dimensions[I].Size;
			ml_cbor_write_integer(Writer, Array->Dimensions[I].Size);
		}
		ml_cbor_write_tag(Writer, 41);
		ml_cbor_write_array(Writer, Size);
		ml_cbor_write_array_any(Array->Degree, Array->Dimensions, Array->Base.Value, Writer);
	} else {
		for (int I = 0; I < Array->Degree; ++I) ml_cbor_write_integer(Writer, Array->Dimensions[I].Size);
		size_t Size = MLArraySizes[Array->Format];
		int FlatDegree = -1;
		size_t FlatSize = Size;
		for (int I = Array->Degree; --I >= 0;) {
			if (FlatDegree < 0) {
				if (Array->Dimensions[I].Indices) {
					FlatDegree = I;
				} else if (Array->Dimensions[I].Stride != Size) {
					FlatDegree = I;
				} else {
					FlatSize = Size * Array->Dimensions[I].Size;
				}
			}
			Size *= Array->Dimensions[I].Size;
		}
#ifdef ML_COMPLEX
		if (Array->Format == ML_ARRAY_FORMAT_C32) {
			ml_cbor_write_tag(Writer, 27);
			ml_cbor_write_array(Writer, 2);
			ml_cbor_write_string(Writer, 16);
			ml_cbor_write_raw(Writer, (unsigned const char *)"array::complex32", 16);
		} else if (Array->Format == ML_ARRAY_FORMAT_C64) {
			ml_cbor_write_tag(Writer, 27);
			ml_cbor_write_array(Writer, 2);
			ml_cbor_write_string(Writer, 16);
			ml_cbor_write_raw(Writer, (unsigned const char *)"array::complex64", 16);
		} else {
			ml_cbor_write_tag(Writer, Tags[Array->Format]);
		}
#else
		ml_cbor_write_tag(Writer, Tags[Array->Format]);
#endif
		ml_cbor_write_bytes(Writer, Size);
		ml_cbor_write_array_typed(FlatDegree, FlatSize, Array->Dimensions, Array->Base.Value, Writer);
	}
	return NULL;
}

static ml_value_t *ml_cbor_read_multi_array_fn(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (!ml_is(Value, MLListT)) return ml_error("TagError", "Array requires list");
	if (ml_list_length(Value) != 2) return ml_error("CborError", "Invalid multi-dimensional array");
	ml_value_t *Dimensions = ml_list_get(Value, 1);
	if (!ml_is(Dimensions, MLListT)) return ml_error("CborError", "Invalid multi-dimensional array");
	ml_array_t *Source = (ml_array_t *)ml_list_get(Value, 2);
	if (!ml_is((ml_value_t *)Source, MLArrayT)) return ml_error("CborError", "Invalid multi-dimensional array");
	ml_array_t *Target = ml_array_alloc(Source->Format, ml_list_length(Dimensions));
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

static ml_value_t *ml_cbor_read_typed_array_fn(ml_value_t *Value, ml_array_format_t Format) {
	if (!ml_is(Value, MLAddressT)) return ml_error("TagError", "Array requires bytes");
	ml_address_t *Buffer = (ml_address_t *)Value;
	int ItemSize = MLArraySizes[Format];
	ml_array_t *Array = ml_array_alloc(Format, 1);
	Array->Dimensions[0].Size = Buffer->Length / ItemSize;
	Array->Dimensions[0].Stride = ItemSize;
	Array->Base.Length = Buffer->Length;
	Array->Base.Value = Buffer->Value;
	return (ml_value_t *)Array;
}

#define ML_CBOR_READ_TYPED_ARRAY(TYPE, FORMAT) \
static ml_value_t *ml_cbor_read_ ## TYPE ## _array_fn(ml_cbor_reader_t *Reader, ml_value_t *Value) { \
	return ml_cbor_read_typed_array_fn(Value, ML_ARRAY_FORMAT_ ## FORMAT); \
}

ML_CBOR_READ_TYPED_ARRAY(uint8, U8)
ML_CBOR_READ_TYPED_ARRAY(int8, I8)
ML_CBOR_READ_TYPED_ARRAY(uint16, U16)
ML_CBOR_READ_TYPED_ARRAY(int16, I16)
ML_CBOR_READ_TYPED_ARRAY(uint32, U32)
ML_CBOR_READ_TYPED_ARRAY(int32, I32)
ML_CBOR_READ_TYPED_ARRAY(uint64, U64)
ML_CBOR_READ_TYPED_ARRAY(int64, I64)
ML_CBOR_READ_TYPED_ARRAY(float32, F32)
ML_CBOR_READ_TYPED_ARRAY(float64, F64)

static ml_value_t *ml_cbor_read_any_array_fn(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (!ml_is(Value, MLListT)) return ml_error("TagError", "Array requires list");
	size_t Size = ml_list_length(Value);
	ml_array_t *Array = ml_array_alloc(ML_ARRAY_FORMAT_ANY, 1);
	Array->Dimensions[0].Size = Size;
	Array->Dimensions[0].Stride = MLArraySizes[ML_ARRAY_FORMAT_ANY];
	ml_value_t **Values = anew(ml_value_t *, Size);
	Array->Base.Length = Size * MLArraySizes[ML_ARRAY_FORMAT_ANY];
	Array->Base.Value = (char *)Values;
	ML_LIST_FOREACH(Value, Iter) *Values++ = Iter->Value;
	return (ml_value_t *)Array;
}

#ifdef ML_COMPLEX

ML_FUNCTION(MLCborReadComplex32) {
//!internal
	ML_CHECK_ARG_COUNT(1);
	return ml_cbor_read_typed_array_fn(Args[0], ML_ARRAY_FORMAT_C32);
}

ML_FUNCTION(MLCborReadComplex64) {
//!internal
	ML_CHECK_ARG_COUNT(1);
	return ml_cbor_read_typed_array_fn(Args[0], ML_ARRAY_FORMAT_C64);
}

#endif

#endif

void ml_array_init(stringmap_t *Globals) {
#include "ml_array_init.c"
	ml_method_by_name("set", UpdateSetRowFns, update_array_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("add", UpdateAddRowFns, update_array_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("mul", UpdateMulRowFns, update_array_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("sub", UpdateSubRowFns, update_array_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("div", UpdateDivRowFns, update_array_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("+", UpdateAddRowFns, array_infix_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("*", UpdateMulRowFns, array_infix_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("-", UpdateSubRowFns, array_infix_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("/", UpdateDivRowFns, array_infix_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("/\\", UpdateAndRowFns, array_infix_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("\\/", UpdateOrRowFns, array_infix_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("><", UpdateXorRowFns, array_infix_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("min", UpdateMinRowFns, array_infix_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("max", UpdateMaxRowFns, array_infix_fn, MLArrayMutableT, MLArrayMutableT, NULL);
	ml_method_by_name("=", CompareEqRowFns, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("!=", CompareNeRowFns, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("<", CompareLtRowFns, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name(">", CompareGtRowFns, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("<=", CompareLeRowFns, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name(">=", CompareGeRowFns, compare_array_fn, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("++", MLArrayInfixAddFns, (ml_callback_t)ml_array_pairwise_infix, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("**", MLArrayInfixMulFns, (ml_callback_t)ml_array_pairwise_infix, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("--", MLArrayInfixSubFns, (ml_callback_t)ml_array_pairwise_infix, MLArrayT, MLArrayT, NULL);
	ml_method_by_name("//", MLArrayInfixDivFns, (ml_callback_t)ml_array_pairwise_infix, MLArrayT, MLArrayT, NULL);

	ml_method_by_value(AbsMethod, labs, (ml_callback_t)array_math_integer_fn, MLArrayMutableIntegerT, NULL);
	ml_method_by_value(SquareMethod, isquare, (ml_callback_t)array_math_integer_fn, MLArrayMutableIntegerT, NULL);

	ml_method_by_value(AcosMethod, acos, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(AsinMethod, asin, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(AtanMethod, atan, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(CeilMethod, ceil, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(CosMethod, cos, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(CoshMethod, cosh, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(ExpMethod, exp, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(AbsMethod, fabs, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(FloorMethod, floor, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(LogMethod, log, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(Log10Method, log10, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(LogitMethod, logit, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(SinMethod, sin, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(SinhMethod, sinh, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(SqrtMethod, sqrt, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(SquareMethod, square, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(TanMethod, tan, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(TanhMethod, tanh, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(ErfMethod, erf, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(ErfcMethod, erfc, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(GammaMethod, gamma, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(AcoshMethod, acosh, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(AsinhMethod, asinh, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(AtanhMethod, atanh, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(CbrtMethod, cbrt, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(Expm1Method, expm1, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(Log1pMethod, log1p, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);
	ml_method_by_value(RoundMethod, round, (ml_callback_t)array_math_real_fn, MLArrayMutableRealT, NULL);

#ifdef ML_COMPLEX
	ml_method_by_value(AcosMethod, cacos, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(AsinMethod, casin, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(AtanMethod, catan, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(CosMethod, ccos, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(CoshMethod, ccosh, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(ExpMethod, cexp, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(AbsMethod, cabs, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(LogMethod, clog, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(Log10Method, clog10, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(SinMethod, csin, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(SinhMethod, csinh, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(SqrtMethod, csqrt, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(SquareMethod, csquare, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(TanMethod, ctan, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(TanhMethod, ctanh, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(AcoshMethod, cacosh, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(AsinhMethod, casinh, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(AtanhMethod, catanh, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(ConjMethod, conj, (ml_callback_t)array_math_complex_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(AbsMethod, cabs, (ml_callback_t)array_math_complex_real_fn, MLArrayMutableComplexT, NULL);
	ml_method_by_value(ArgMethod, carg, (ml_callback_t)array_math_complex_real_fn, MLArrayMutableComplexT, NULL);
#endif

	ml_method_definev(ml_method("$"), MLArrayT->Constructor, 0, MLListT, NULL);
	stringmap_insert(MLArrayT->Exports, "new", MLArrayNew);
	stringmap_insert(MLArrayT->Exports, "wrap", MLArrayWrap);
	stringmap_insert(MLArrayT->Exports, "cat", MLArrayCat);
	stringmap_insert(MLArrayT->Exports, "hcat", MLArrayHCat);
	stringmap_insert(MLArrayT->Exports, "vcat", MLArrayVCat);
	stringmap_insert(MLArrayT->Exports, "nil", MLArrayNil);
	stringmap_insert(MLArrayT->Exports, "any", MLArrayAnyT);
	stringmap_insert(MLArrayT->Exports, "uint8", MLArrayUInt8T);
	stringmap_insert(MLArrayT->Exports, "int8", MLArrayInt8T);
	stringmap_insert(MLArrayT->Exports, "uint16", MLArrayUInt16T);
	stringmap_insert(MLArrayT->Exports, "int16", MLArrayInt16T);
	stringmap_insert(MLArrayT->Exports, "uint32", MLArrayUInt32T);
	stringmap_insert(MLArrayT->Exports, "int32", MLArrayInt32T);
	stringmap_insert(MLArrayT->Exports, "uint64", MLArrayUInt64T);
	stringmap_insert(MLArrayT->Exports, "int64", MLArrayInt64T);
	stringmap_insert(MLArrayT->Exports, "float32", MLArrayFloat32T);
	stringmap_insert(MLArrayT->Exports, "float64", MLArrayFloat64T);

	stringmap_insert(MLVectorT->Exports, "any", MLVectorAnyT);
	stringmap_insert(MLVectorT->Exports, "uint8", MLVectorUInt8T);
	stringmap_insert(MLVectorT->Exports, "int8", MLVectorInt8T);
	stringmap_insert(MLVectorT->Exports, "uint16", MLVectorUInt16T);
	stringmap_insert(MLVectorT->Exports, "int16", MLVectorInt16T);
	stringmap_insert(MLVectorT->Exports, "uint32", MLVectorUInt32T);
	stringmap_insert(MLVectorT->Exports, "int32", MLVectorInt32T);
	stringmap_insert(MLVectorT->Exports, "uint64", MLVectorUInt64T);
	stringmap_insert(MLVectorT->Exports, "int64", MLVectorInt64T);
	stringmap_insert(MLVectorT->Exports, "float32", MLVectorFloat32T);
	stringmap_insert(MLVectorT->Exports, "float64", MLVectorFloat64T);

	stringmap_insert(MLMatrixT->Exports, "any", MLMatrixAnyT);
	stringmap_insert(MLMatrixT->Exports, "uint8", MLMatrixUInt8T);
	stringmap_insert(MLMatrixT->Exports, "int8", MLMatrixInt8T);
	stringmap_insert(MLMatrixT->Exports, "uint16", MLMatrixUInt16T);
	stringmap_insert(MLMatrixT->Exports, "int16", MLMatrixInt16T);
	stringmap_insert(MLMatrixT->Exports, "uint32", MLMatrixUInt32T);
	stringmap_insert(MLMatrixT->Exports, "int32", MLMatrixInt32T);
	stringmap_insert(MLMatrixT->Exports, "uint64", MLMatrixUInt64T);
	stringmap_insert(MLMatrixT->Exports, "int64", MLMatrixInt64T);
	stringmap_insert(MLMatrixT->Exports, "float32", MLMatrixFloat32T);
	stringmap_insert(MLMatrixT->Exports, "float64", MLMatrixFloat64T);

#ifdef ML_COMPLEX
	stringmap_insert(MLArrayT->Exports, "complex32", MLArrayComplex32T);
	stringmap_insert(MLArrayT->Exports, "complex64", MLArrayComplex64T);
	stringmap_insert(MLVectorT->Exports, "complex32", MLVectorComplex32T);
	stringmap_insert(MLVectorT->Exports, "complex64", MLVectorComplex64T);
	stringmap_insert(MLMatrixT->Exports, "complex32", MLMatrixComplex32T);
	stringmap_insert(MLMatrixT->Exports, "complex64", MLMatrixComplex64T);
#endif
	if (Globals) {
		stringmap_insert(Globals, "array", MLArrayT);
		stringmap_insert(Globals, "vector", MLVectorT);
		stringmap_insert(Globals, "matrix", MLMatrixT);
	}
#ifdef ML_CBOR
	ml_cbor_default_tag(40, ml_cbor_read_multi_array_fn);
	ml_cbor_default_tag(41, ml_cbor_read_any_array_fn);
	ml_cbor_default_tag(64, ml_cbor_read_uint8_array_fn);
	ml_cbor_default_tag(72, ml_cbor_read_int8_array_fn);
	ml_cbor_default_tag(69, ml_cbor_read_uint16_array_fn);
	ml_cbor_default_tag(77, ml_cbor_read_int16_array_fn);
	ml_cbor_default_tag(70, ml_cbor_read_uint32_array_fn);
	ml_cbor_default_tag(78, ml_cbor_read_int32_array_fn);
	ml_cbor_default_tag(71, ml_cbor_read_uint64_array_fn);
	ml_cbor_default_tag(79, ml_cbor_read_int64_array_fn);
	ml_cbor_default_tag(85, ml_cbor_read_float32_array_fn);
	ml_cbor_default_tag(86, ml_cbor_read_float64_array_fn);
#ifdef ML_COMPLEX
	ml_cbor_default_object("array::complex32", (ml_value_t *)MLCborReadComplex32);
	ml_cbor_default_object("array::complex64", (ml_value_t *)MLCborReadComplex64);
#endif
	ml_externals_add("array", MLArrayT);
	ml_externals_add("vector", MLVectorT);
	ml_externals_add("matrix", MLMatrixT);
#endif
}

#include "ml_array.h"
#include "ml_macros.h"
#include <stdint.h>

typedef struct ml_array_dimension_t {
	int Size, Stride;
	int *Indices;
} ml_array_dimension_t;

typedef enum {
	ML_ARRAY_FORMAT_ANY,
	ML_ARRAY_FORMAT_I8, ML_ARRAY_FORMAT_U8,
	ML_ARRAY_FORMAT_I16, ML_ARRAY_FORMAT_U16,
	ML_ARRAY_FORMAT_I32, ML_ARRAY_FORMAT_U32,
	ML_ARRAY_FORMAT_I64, ML_ARRAY_FORMAT_U64,
	ML_ARRAY_FORMAT_F32, ML_ARRAY_FORMAT_F64
} ml_array_format_t;

typedef struct ml_array_t {
	ml_buffer_t Base;
	int Degree, Format;
	ml_array_dimension_t Dimensions[];
} ml_array_t;

ml_type_t *MLArrayT;
ml_type_t *MLArrayAnyT;
ml_type_t *MLArrayInt8T;
ml_type_t *MLArrayInt16T;
ml_type_t *MLArrayInt32T;
ml_type_t *MLArrayInt64T;
ml_type_t *MLArrayUInt8T;
ml_type_t *MLArrayUInt16T;
ml_type_t *MLArrayUInt32T;
ml_type_t *MLArrayUInt64T;
ml_type_t *MLArrayFloat32T;
ml_type_t *MLArrayFloat64T;

ml_array_t *ml_array_new(ml_array_format_t Format, int Degree) {
	ml_type_t *Type = MLArrayT;
	switch (Format) {
	case ML_ARRAY_FORMAT_ANY: Type = MLArrayAnyT; break;
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
	};
	ml_array_t *Array = xnew(ml_array_t, Degree, ml_array_dimension_t);
	Array->Base.Type = Type;
	Array->Degree = Degree;
	Array->Format = Format;
	return Array;
}

ml_value_t *ml_array_new_fn(void *Address, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ml_array_format_t Format;
	int ItemSize;
	if (Args[0] == MLArrayAnyT) {
		Format = ML_ARRAY_FORMAT_ANY;
		ItemSize = sizeof(ml_value_t *);
	} else if (Args[0] == MLArrayInt8T) {
		Format = ML_ARRAY_FORMAT_I8;
		ItemSize = 1;
	} else if (Args[0] == MLArrayUInt8T) {
		Format = ML_ARRAY_FORMAT_U8;
		ItemSize = 1;
	} else if (Args[0] == MLArrayInt16T) {
		Format = ML_ARRAY_FORMAT_I16;
		ItemSize = 2;
	} else if (Args[0] == MLArrayUInt16T) {
		Format = ML_ARRAY_FORMAT_U16;
		ItemSize = 2;
	} else if (Args[0] == MLArrayInt32T) {
		Format = ML_ARRAY_FORMAT_I32;
		ItemSize = 4;
	} else if (Args[0] == MLArrayUInt32T) {
		Format = ML_ARRAY_FORMAT_U32;
		ItemSize = 4;
	} else if (Args[0] == MLArrayInt64T) {
		Format = ML_ARRAY_FORMAT_I64;
		ItemSize = 8;
	} else if (Args[0] == MLArrayUInt64T) {
		Format = ML_ARRAY_FORMAT_U64;
		ItemSize = 8;
	} else if (Args[0] == MLArrayFloat32T) {
		Format = ML_ARRAY_FORMAT_F32;
		ItemSize = 4;
	} else if (Args[0] == MLArrayFloat64T) {
		Format = ML_ARRAY_FORMAT_F64;
		ItemSize = 8;
	} else {
		return ml_error("TypeError", "Unknown type for array");
	}
	ml_array_t *Array;
	if (Args[1]->Type == MLListT) {
		int Degree = ml_list_length(Args[1]);
		Array = ml_array_new(Format, Degree);
		ml_list_node_t *Node = ml_list_head(Args[1]);
		for (int I = 0; I < Degree; ++I, Node = Node->Next) {
			if (Node->Value->Type != MLIntegerT) return ml_error("TypeError", "Dimension is not an integer");
			Array->Dimensions[I].Size = ml_integer_value(Node->Value);
		}
	} else {
		int Degree = Count - 1;
		Array = ml_array_new(Format, Degree);
		for (int I = 1; I < Count; ++I) {
			ML_CHECK_ARG_TYPE(I, MLIntegerT);
			Array->Dimensions[I - 1].Size = ml_integer_value(Args[I]);
		}
	}
	int DataSize = ItemSize;
	for (int I = Array->Degree; --I >= 0;) {
		Array->Dimensions[I].Stride = DataSize;
		DataSize *= Array->Dimensions[I].Size;
	}
	Array->Base.Address = GC_malloc_atomic(DataSize);
	Array->Base.Size = DataSize;
	return (ml_value_t *)Array;
}

ml_value_t *ml_array_wrap_fn(void *Address, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(3);
	ML_CHECK_ARG_TYPE(1, MLBufferT);
	ML_CHECK_ARG_TYPE(2, MLListT);
	ML_CHECK_ARG_TYPE(3, MLListT);
	ml_array_format_t Format;
	if (Args[0] == MLArrayAnyT) {
		Format = ML_ARRAY_FORMAT_ANY;
	} else if (Args[0] == MLArrayInt8T) {
		Format = ML_ARRAY_FORMAT_I8;
	} else if (Args[0] == MLArrayUInt8T) {
		Format = ML_ARRAY_FORMAT_U8;
	} else if (Args[0] == MLArrayInt16T) {
		Format = ML_ARRAY_FORMAT_I16;
	} else if (Args[0] == MLArrayUInt16T) {
		Format = ML_ARRAY_FORMAT_U16;
	} else if (Args[0] == MLArrayInt32T) {
		Format = ML_ARRAY_FORMAT_I32;
	} else if (Args[0] == MLArrayUInt32T) {
		Format = ML_ARRAY_FORMAT_U32;
	} else if (Args[0] == MLArrayInt64T) {
		Format = ML_ARRAY_FORMAT_I64;
	} else if (Args[0] == MLArrayUInt64T) {
		Format = ML_ARRAY_FORMAT_U64;
	} else if (Args[0] == MLArrayFloat32T) {
		Format = ML_ARRAY_FORMAT_F32;
	} else if (Args[0] == MLArrayFloat64T) {
		Format = ML_ARRAY_FORMAT_F64;
	} else {
		return ml_error("TypeError", "Unknown type for array");
	}
	int Degree = ml_list_length(Args[2]);
	if (Degree != ml_list_length(Args[3])) return ml_error("ValueError", "Dimensions and strides must have same length");
	ml_array_t *Array = ml_array_new(Format, Degree);
	ml_list_node_t *SizeNode = ml_list_head(Args[2]);
	ml_list_node_t *StrideNode = ml_list_head(Args[3]);
	for (int I = 0; I < Degree; ++I) {
		if (SizeNode->Value->Type != MLIntegerT) return ml_error("TypeError", "Dimension is not an integer");
		if (StrideNode->Value->Type != MLIntegerT) return ml_error("TypeError", "Stride is not an integer");
		Array->Dimensions[I].Size = ml_integer_value(SizeNode->Value);
		Array->Dimensions[I].Stride = ml_integer_value(StrideNode->Value);
		SizeNode = SizeNode->Next;
		StrideNode = StrideNode->Next;
	}
	Array->Base.Address = ((ml_buffer_t *)Args[1])->Address;
	Array->Base.Size = ((ml_buffer_t *)Args[1])->Size;
	return (ml_value_t *)Array;
}

ML_METHOD("shape", MLArrayT) {
	ml_array_t *Array = (ml_array_t *)Args[0];
	ml_value_t *Shape = ml_list();
	for (int I = 0; I < Array->Degree; ++I) {
		ml_list_put(Shape, ml_integer(Array->Dimensions[I].Size));
	}
	return (ml_value_t *)Shape;
}

ML_METHOD("strides", MLArrayT) {
	ml_array_t *Array = (ml_array_t *)Args[0];
	ml_value_t *Strides = ml_list();
	for (int I = 0; I < Array->Degree; ++I) {
		ml_list_put(Strides, ml_integer(Array->Dimensions[I].Stride));
	}
	return (ml_value_t *)Strides;
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

ML_METHOD("permute", MLArrayT, TYP, MLListT) {
	ml_array_t *Source = (ml_array_t *)Args[0];
	int Degree = Source->Degree;
	if (ml_list_length(Args[1]) != Degree) return ml_error("Error", "List length must match degree");
	ml_array_t *Target = ml_array_new(Source->Format, Degree);
	ml_list_node_t *Node = ml_list_head(Args[1]);
	for (int I = 0; I < Degree; ++I) {
		if (Node->Value->Type != MLIntegerT) return ml_error("Error", "Invalid index");
		int J = ml_integer_value(Node->Value);
		if (J <= 0) J += Degree + 1;
		if (J < 1 || J > Degree) return ml_error("Error", "Invalid index");
		Target->Dimensions[I] = Source->Dimensions[J - 1];
		Node = Node->Next;
	}
	Target->Base = Source->Base;
	return (ml_value_t *)Target;
}

typedef struct ml_integer_range_t {
	const ml_type_t *Type;
	long Start, Limit, Step;
} ml_integer_range_t;

extern ml_type_t MLIntegerRangeT[1];

ML_METHOD("[]", MLArrayT) {
	ml_array_t *Source = (ml_array_t *)Args[0];
	if (Count - 1 > Source->Degree) return ml_error("Error", "Too many indices");
	ml_array_dimension_t TargetDimensions[Source->Degree];
	ml_array_dimension_t *TargetDimension = TargetDimensions;
	ml_array_dimension_t *SourceDimension = Source->Dimensions;
	void *Address = Source->Base.Address;
	int Min, Max, Step;
	for (int I = 1; I < Count; ++I) {
		ml_value_t *Index = Args[I];
		if (Index->Type == MLIntegerT) {
			int IndexValue = ml_integer_value(Index);
			if (IndexValue <= 0) IndexValue += SourceDimension->Size + 1;
			if (--IndexValue < 0) return MLNil;
			if (IndexValue >= SourceDimension->Size) return MLNil;
			if (SourceDimension->Indices) IndexValue = SourceDimension->Indices[IndexValue];
			Address += SourceDimension->Stride * IndexValue;
		} else if (Index->Type == MLListT) {
			int Size = TargetDimension->Size = ml_list_length(Index);
			int *Indices = TargetDimension->Indices = (int *)GC_malloc_atomic(Size * sizeof(int));
			int *IndexPtr = Indices;
			for (ml_list_node_t *Node = ml_list_head(Index); Node; Node = Node->Next) {
				if (Node->Value->Type != MLIntegerT) return ml_error("Error", "Invalid index");
				int IndexValue = ml_integer_value(Node->Value);
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
		as_range:
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
			return ml_error("Error", "Unknown index type");
		}
		++SourceDimension;
	}
	for (int I = Count - 1; I < Source->Degree; ++I) {
		*TargetDimension = *SourceDimension;
		++TargetDimension;
		++SourceDimension;
	}
	int Degree = TargetDimension - TargetDimensions;
	ml_array_t *Target = ml_array_new(Source->Format, Degree);
	for (int I = 0; I < Degree; ++I) Target->Dimensions[I] = TargetDimensions[I];
	Target->Base.Address = Address;
	return (ml_value_t *)Target;
}

#define UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, NAME, OP) \
\
static void NAME ## _array_suffix_ ## CTYPE1 ## _ ## CTYPE2(ml_array_dimension_t *TargetDimension, void *TargetData, ml_array_dimension_t *SourceDimension, void *SourceData) { \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				*(CTYPE1 *)(TargetData + TargetIndices[I] * TargetDimension->Stride) OP *(CTYPE2 *)(SourceData + SourceIndices[I] * SourceDimension->Stride); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				*(CTYPE1 *)(TargetData + TargetIndices[I] * TargetDimension->Stride) OP *(CTYPE2 *)SourceData; \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				*(CTYPE1 *)TargetData OP *(CTYPE2 *)(SourceData + SourceIndices[I] * SourceDimension->Stride); \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				*(CTYPE1 *)TargetData OP *(CTYPE2 *)SourceData; \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
} \
\
static void NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(ml_array_dimension_t *TargetDimension, void *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceData) { \
	if (SourceDegree == 0) return NAME ## _value_array0_ ## CTYPE1(TargetDimension, TargetData, *(CTYPE2 *)SourceData); \
	if (SourceDegree == 1) return NAME ## _array_suffix_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension, TargetData, SourceDimension, SourceData); \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData + SourceIndices[I] * SourceDimension->Stride); \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = 0; I < Size; ++I) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree - 1, SourceDimension + 1, SourceData); \
				SourceData += SourceStride; \
			} \
		} \
	} else { \
		int TargetStride = TargetDimension->Stride; \
		if (SourceDimension->Indices) { \
			int *SourceIndices = SourceDimension->Indices; \
			for (int I = 0; I < Size; ++I) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData, SourceDegree - 1, SourceDimension + 1, SourceData + SourceIndices[I] * SourceDimension->Stride); \
				TargetData += TargetStride; \
			} \
		} else { \
			int SourceStride = SourceDimension->Stride; \
			for (int I = Size; --I >= 0;) { \
				NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension + 1, TargetData, SourceDegree - 1, SourceDimension + 1, SourceData); \
				TargetData += TargetStride; \
				SourceData += SourceStride; \
			} \
		} \
	} \
} \
\
static void NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(int PrefixDegree, ml_array_dimension_t *TargetDimension, void *TargetData, int SourceDegree, ml_array_dimension_t *SourceDimension, void *SourceData) { \
	if (PrefixDegree == 0) return NAME ## _array_ ## CTYPE1 ## _ ## CTYPE2(TargetDimension, TargetData, SourceDegree, SourceDimension, SourceData); \
	int Size = TargetDimension->Size; \
	if (TargetDimension->Indices) { \
		int *TargetIndices = TargetDimension->Indices; \
		for (int I = Size; --I >= 0;) { \
			NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(PrefixDegree - 1, TargetDimension + 1, TargetData + TargetIndices[I] * TargetDimension->Stride, SourceDegree, SourceDimension, SourceData); \
		} \
	} else { \
		int Stride = TargetDimension->Stride; \
		for (int I = Size; --I >= 0;) { \
			NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(PrefixDegree - 1, TargetDimension + 1, TargetData, SourceDegree, SourceDimension, SourceData); \
			TargetData += Stride; \
		} \
	} \
} \
\
ML_METHOD(#NAME, TYP, ATYPE1, TYP, ATYPE2) { \
	ml_array_t *Target = (ml_array_t *)Args[0]; \
	ml_array_t *Source = (ml_array_t *)Args[1]; \
	if (Source->Degree > Target->Degree) return ml_error("Error", "Incompatible assignment"); \
	int PrefixDegree = Target->Degree - Source->Degree; \
	for (int I = 0; I < Source->Degree; ++I) { \
		if (Target->Dimensions[PrefixDegree + I].Size != Source->Dimensions[I].Size) return ml_error("Error", "Incompatible assignment"); \
	} \
	NAME ## _array_prefix_ ## CTYPE1 ## _ ## CTYPE2(PrefixDegree, Target->Dimensions, Target->Base.Address, Source->Degree, Source->Dimensions, Source->Base.Address); \
	return Args[0]; \
}

#define UPDATE_ARRAY_METHODS(ATYPE1, CTYPE1, ATYPE2, CTYPE2) \
\
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, set, =); \
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, add, +=); \
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, sub, -=); \
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, mul, *=); \
UPDATE_ARRAY_METHOD(ATYPE1, CTYPE1, ATYPE2, CTYPE2, div, /=);

#define UPDATE_METHOD(ATYPE, CTYPE, RFUNC, NAME, OP) \
\
static void NAME ## _value_array0_ ## CTYPE(ml_array_dimension_t *Dimension, void *Address, CTYPE Value) { \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		for (int I = 0; I < Dimension->Size; ++I) { \
			*(CTYPE *)(Address + (Indices[I]) * Dimension->Stride) OP Value; \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		for (int I = Dimension->Size; --I >= 0;) { \
			*(CTYPE *)Address OP Value; \
			Address += Stride; \
		} \
	} \
} \
\
static void NAME ## _value_array_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, void *Address, CTYPE Value) { \
	if (Degree == 1) return NAME ## _value_array0_ ## CTYPE(Dimension, Address, Value); \
	int Stride = Dimension->Stride; \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		for (int I = 0; I < Dimension->Size; ++I) { \
			NAME ## _value_array_ ## CTYPE(Degree - 1, Dimension + 1, Address + (Indices[I]) * Dimension->Stride, Value); \
		} \
	} else { \
		for (int I = Dimension->Size; --I >= 0;) { \
			NAME ## _value_array_ ## CTYPE(Degree - 1, Dimension + 1, Address, Value); \
			Address += Stride; \
		} \
	} \
} \
\
ML_METHOD(#NAME, TYP, ATYPE, TYP, Std$Number$T) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	CTYPE Value = RFUNC(Args[1]); \
	if (Array->Degree == 0) { \
		*(CTYPE *)Array->Base.Address OP Value; \
	} else { \
		NAME ## _value_array_ ## CTYPE(Array->Degree, Array->Dimensions, Array->Base.Address, Value); \
	} \
	return Args[0]; \
}

typedef struct call_info_t {
	int Count;
	ml_value_t *Function;
	ml_value_t *Result;
	ml_value_t *Args[];
} call_info_t;

#define METHODS(ATYPE, CTYPE, FORMAT, RFUNC, RNEW) \
\
static ml_value_t *to_string_array0_ ## CTYPE(ml_array_dimension_t *Dimension, void *Address) { \
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT}; \
	ml_stringbuffer_add(Buffer, "[", 1); \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Size) { \
			ml_stringbuffer_addf(Buffer, FORMAT, *(CTYPE *)(Address + (Indices[0]) * Dimension->Stride)); \
			for (int I = 1; I < Dimension->Size; ++I) { \
				ml_stringbuffer_addf(Buffer, ", "FORMAT, *(CTYPE *)(Address + (Indices[I]) * Dimension->Stride)); \
			} \
		} \
	} else { \
		int Stride = Dimension->Stride; \
		ml_stringbuffer_addf(Buffer, FORMAT, *(CTYPE *)Address); \
		Address += Stride; \
		for (int I = Dimension->Size; --I > 0;) { \
			ml_stringbuffer_addf(Buffer, ", "FORMAT, *(CTYPE *)Address); \
			Address += Stride; \
		} \
	} \
	ml_stringbuffer_add(Buffer, "]", 1); \
	return ml_stringbuffer_get_string(Buffer); \
} \
\
static ml_value_t *to_string_array_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, void *Address) { \
	if (Degree == 1) return to_string_array0_ ## CTYPE(Dimension, Address); \
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT}; \
	ml_stringbuffer_add(Buffer, "[", 1); \
	int Stride = Dimension->Stride; \
	if (Dimension->Indices) { \
		int *Indices = Dimension->Indices; \
		if (Dimension->Size) { \
			ml_stringbuffer_append(Buffer, to_string_array_ ## CTYPE(Degree - 1, Dimension + 1, Address + (Indices[0]) * Dimension->Stride)); \
			for (int I = 1; I < Dimension->Size; ++I) { \
				ml_stringbuffer_add(Buffer, ", ", 2); \
				ml_stringbuffer_append(Buffer, to_string_array_ ## CTYPE(Degree - 1, Dimension + 1, Address + (Indices[I]) * Dimension->Stride)); \
			} \
		} \
	} else { \
		ml_stringbuffer_append(Buffer, to_string_array_ ## CTYPE(Degree - 1, Dimension + 1, Address)); \
		Address += Stride; \
		for (int I = Dimension->Size; --I > 0;) { \
			ml_stringbuffer_add(Buffer, ", ", 2); \
			ml_stringbuffer_append(Buffer, to_string_array_ ## CTYPE(Degree - 1, Dimension + 1, Address)); \
			Address += Stride; \
		} \
	} \
	ml_stringbuffer_add(Buffer, "]", 1); \
	return ml_stringbuffer_get_string(Buffer); \
} \
\
ML_METHOD("string", ATYPE) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	if (Array->Degree == 0) { \
		return ml_string_format(FORMAT, *(CTYPE *)Array->Base.Address); \
	} else { \
		return to_string_array_ ## CTYPE(Array->Degree, Array->Dimensions, Array->Base.Address); \
	} \
} \
\
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, set, =); \
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, add, +=); \
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, sub, -=); \
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, mul, *=); \
UPDATE_METHOD(ATYPE, CTYPE, RFUNC, div, /=); \
\
static int set_function_array_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, void *Address, call_info_t *Info) { \
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
ML_METHOD("setf", TYP, ATYPE, ANY) { \
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
}\
\
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayInt8T, int8_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayUInt8T, uint8_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayInt16T, int16_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayUInt16T, uint16_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayInt32T, int32_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayUInt32T, uint32_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayInt64T, int64_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayUInt64T, uint64_t); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayFloat32T, float); \
UPDATE_ARRAY_METHODS(ATYPE, CTYPE, MLArrayFloat64T, double); \
\
ML_METHOD("copy", TYP, ATYPE) { \
	ml_array_t *Source = (ml_array_t *)Args[0]; \
	int Degree = Source->Degree; \
	ml_array_t *Target = ml_array_new(Source->Format, Degree); \
	int DataSize = sizeof(CTYPE); \
	for (int I = Degree; --I >= 0;) { \
		Target->Dimensions[I].Stride = DataSize; \
		int Size = Target->Dimensions[I].Size = Source->Dimensions[I].Size; \
		DataSize *= Size; \
	} \
	Target->Base.Address = GC_malloc_atomic(DataSize); \
	set_array_ ## CTYPE ## _ ## CTYPE(Target->Dimensions, Target->Base.Address, Degree, Source->Dimensions, Source->Base.Address); \
	return (ml_value_t *)Target; \
} \
\
ML_METHOD("get", TYP, ATYPE) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	return RNEW(*(CTYPE *)Array->Base.Address); \
} \
\
void partial_sums_ ## CTYPE(int Target, int Degree, ml_array_dimension_t *Dimension, void *Address, int LastRow) { \
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
ML_METHOD("partial_sums", TYP, ATYPE, TYP, MLIntegerT) { \
	ml_array_t *Array = (ml_array_t *)Args[0]; \
	int Target = ml_integer_value(Args[1]); \
	if (Target <= 0) Target += Array->Degree + 1; \
	if (Target < 1 || Target > Array->Degree) return ml_error("Error", "Dimension index invalid"); \
	Target = Array->Degree + 1 - Target; \
	partial_sums_ ## CTYPE(Target, Array->Degree, Array->Dimensions, Array->Base.Address, 0); \
	return Args[0]; \
}

METHODS(MLArrayInt8T, int8_t, "%d", ml_integer_value, ml_integer);
METHODS(MLArrayUInt8T, uint8_t, "%ud", ml_integer_value, ml_integer);
METHODS(MLArrayInt16T, int16_t, "%d", ml_integer_value, ml_integer);
METHODS(MLArrayUInt16T, uint16_t, "%ud", ml_integer_value, ml_integer);
METHODS(MLArrayInt32T, int32_t, "%d", ml_integer_value, ml_integer);
METHODS(MLArrayUInt32T, uint32_t, "%ud", ml_integer_value, ml_integer);
METHODS(MLArrayInt64T, int64_t, "%ld", ml_integer_value, ml_integer);
METHODS(MLArrayUInt64T, uint64_t, "%uld", ml_integer_value, ml_integer);
METHODS(MLArrayFloat32T, float, "%f", ml_real_value, ml_real);
METHODS(MLArrayFloat64T, double, "%f", ml_real_value, ml_real);


void ml_array_init(stringmap_t *Globals) {
	MLArrayT = ml_type(MLBufferT, "array");
	MLArrayAnyT = ml_type(MLArrayT, "value-array");
	MLArrayInt8T = ml_type(MLArrayT, "int8-array");
	MLArrayInt16T = ml_type(MLArrayT, "int16-array");
	MLArrayInt32T = ml_type(MLArrayT, "int32-array");
	MLArrayInt64T = ml_type(MLArrayT, "int64-array");
	MLArrayUInt8T = ml_type(MLArrayT, "uint8-array");
	MLArrayUInt16T = ml_type(MLArrayT, "uint16-array");
	MLArrayUInt32T = ml_type(MLArrayT, "uint32-array");
	MLArrayUInt64T = ml_type(MLArrayT, "uint64-array");
	MLArrayFloat32T = ml_type(MLArrayT, "float32-array");
	MLArrayFloat64T = ml_type(MLArrayT, "float64-array");
	if (Globals) {
		ml_value_t *Array = ml_map();
		ml_map_insert(Array, ml_string("T", -1), MLArrayT);
		ml_map_insert(Array, ml_string("AnyT", -1), MLArrayAnyT);
		ml_map_insert(Array, ml_string("Int8T", -1), MLArrayInt8T);
		ml_map_insert(Array, ml_string("Int16T", -1), MLArrayInt16T);
		ml_map_insert(Array, ml_string("Int32T", -1), MLArrayInt32T);
		ml_map_insert(Array, ml_string("Int64T", -1), MLArrayInt64T);
		ml_map_insert(Array, ml_string("UInt8T", -1), MLArrayUInt8T);
		ml_map_insert(Array, ml_string("UInt16T", -1), MLArrayUInt16T);
		ml_map_insert(Array, ml_string("UInt32T", -1), MLArrayUInt32T);
		ml_map_insert(Array, ml_string("UInt64T", -1), MLArrayUInt64T);
		ml_map_insert(Array, ml_string("Float32T", -1), MLArrayFloat32T);
		ml_map_insert(Array, ml_string("Float64T", -1), MLArrayFloat64T);
		ml_map_insert(Array, ml_string("new", -1), ml_function(NULL, ml_array_new_fn));
		ml_map_insert(Array, ml_string("wrap", -1), ml_function(NULL, ml_array_wrap_fn));
		stringmap_insert(Globals, "array", Array);
	}
}

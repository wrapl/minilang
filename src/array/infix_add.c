#include "infix_impl_complex.h"
#include "ctypes_arith.h"

#define OP_ADD(A, B) A + B

extern ml_value_t *AddMethod;

static ml_value_t *value_add(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	return ml_simple_call(AddMethod, 2, Args);
}

#define TYPE_ADD_uint8_t uint16_t
#define TYPE_ADD_int8_t int16_t
#define TYPE_ADD_uint16_t uint32_t
#define TYPE_ADD_int16_t int32_t
#define TYPE_ADD_uint32_t uint64_t
#define TYPE_ADD_int32_t int64_t
#define TYPE_ADD_uint64_t uint64_t
#define TYPE_ADD_int64_t int64_t
#define TYPE_ADD_float float
#define TYPE_ADD_double double
#define TYPE_ADD_complex_float complex_float
#define TYPE_ADD_complex_double complex_double
#define TYPE_ADD_any any

ml_array_format_t InfixAddFormat[] = {
	[ML_ARRAY_FORMAT_NONE] = ML_ARRAY_FORMAT_NONE,
	[ML_ARRAY_FORMAT_U8] = ML_ARRAY_FORMAT_U16,
	[ML_ARRAY_FORMAT_I8] = ML_ARRAY_FORMAT_I16,
	[ML_ARRAY_FORMAT_U16] = ML_ARRAY_FORMAT_U32,
	[ML_ARRAY_FORMAT_I16] = ML_ARRAY_FORMAT_I32,
	[ML_ARRAY_FORMAT_U32] = ML_ARRAY_FORMAT_U64,
	[ML_ARRAY_FORMAT_I32] = ML_ARRAY_FORMAT_I64,
	[ML_ARRAY_FORMAT_U64] = ML_ARRAY_FORMAT_U64,
	[ML_ARRAY_FORMAT_I64] = ML_ARRAY_FORMAT_I64,
	[ML_ARRAY_FORMAT_F32] = ML_ARRAY_FORMAT_F32,
	[ML_ARRAY_FORMAT_F64] = ML_ARRAY_FORMAT_F64,
#ifdef ML_COMPLEX
	[ML_ARRAY_FORMAT_C32] = ML_ARRAY_FORMAT_C32,
	[ML_ARRAY_FORMAT_C64] = ML_ARRAY_FORMAT_C64,
#endif
	[ML_ARRAY_FORMAT_ANY] = ML_ARRAY_FORMAT_ANY
};

INFIX_FNS(Add, add, OP_ADD, TYPE_ADD_, value_add, );



#include "infix_impl_complex.h"
#include "ctypes_arith.h"

#define OP_ADD(A, B) A + B

extern ml_value_t *AddMethod;

static ml_value_t *value_add(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	return ml_simple_call(AddMethod, 2, Args);
}

INFIX_FNS(Add, add, OP_ADD, CTYPE_ARITH, value_add, TEST);

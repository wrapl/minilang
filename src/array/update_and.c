#include "update_impl_bitwise.h"

#define OP_AND(A, B) A & B

extern ml_value_t *AndMethod;

static ml_value_t *value_and(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	return ml_simple_call(AndMethod, 2, Args);
}

UPDATE_FNS(And, and, OP_AND, value_and);

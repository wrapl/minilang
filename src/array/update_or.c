#include "update_impl_bitwise.h"

#define OP_OR(A, B) A | B

extern ml_value_t *OrMethod;

static ml_value_t *value_or(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	return ml_simple_call(OrMethod, 2, Args);
}

UPDATE_FNS(Or, or, OP_OR, value_or);

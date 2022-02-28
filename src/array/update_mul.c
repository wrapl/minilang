#include "update_impl.h"

#define OP_MUL(A, B) A * B

extern ml_value_t *MulMethod;

static ml_value_t *value_mul(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	return ml_simple_call(MulMethod, 2, Args);
}

UPDATE_FNS(Mul, mul, OP_MUL, value_mul);

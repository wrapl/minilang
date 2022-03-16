#include "update_impl_complex.h"

#define OP_SUB(A, B) A - B

extern ml_value_t *SubMethod;

static ml_value_t *value_sub(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	return ml_simple_call(SubMethod, 2, Args);
}

UPDATE_FNS(Sub, sub, OP_SUB, value_sub);

#define OP_RSUB(A, B) B - A

static ml_value_t *value_rsub(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {B, A};
	return ml_simple_call(SubMethod, 2, Args);
}

UPDATE_FNS(RSub, rsub, OP_RSUB, value_rsub);

#include "update_impl_complex.h"

#define OP_DIV(A, B) A / B

extern ml_value_t *DivMethod;

static ml_value_t *value_div(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	return ml_simple_call(DivMethod, 2, Args);
}

UPDATE_FNS(Div, div, OP_DIV, value_div);

#define OP_RDIV(A, B) B / A

static ml_value_t *value_rdiv(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {B, A};
	return ml_simple_call(DivMethod, 2, Args);
}

UPDATE_FNS(RDiv, rdiv, OP_RDIV, value_rdiv);

#include "update_impl_real.h"

#define OP_MIN(A, B) (A < B ? A : B)

extern ml_value_t *MinMethod;

static ml_value_t *value_min(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	return ml_simple_call(MinMethod, 2, Args);
}

UPDATE_FNS(Min, min, OP_MIN, value_min);

#include "update_impl_real.h"

#define OP_MAX(A, B) (A > B ? A : B)

extern ml_value_t *MaxMethod;

static ml_value_t *value_max(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	return ml_simple_call(MaxMethod, 2, Args);
}

UPDATE_FNS(Max, max, OP_MAX, value_max);

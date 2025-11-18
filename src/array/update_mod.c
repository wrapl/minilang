#include "update_impl_real.h"
#include <math.h>

static inline int64_t mod(int64_t A, int64_t B) {
	return A % B;
}

#define OP_MOD(A, B) _Generic(A, double: fmod, float: fmod, default: mod)(A, B)

extern ml_value_t *ModMethod;

static ml_value_t *value_mod(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	return ml_simple_call(ModMethod, 2, Args);
}

UPDATE_FNS(Mod, mod, OP_MOD, value_mod);

#define OP_RMOD(A, B) OP_MOD(B, A)

static ml_value_t *value_rmod(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {B, A};
	return ml_simple_call(ModMethod, 2, Args);
}

UPDATE_FNS(RMod, rmod, OP_RMOD, value_rmod);

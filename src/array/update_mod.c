#include "update_impl_real.h"

static inline int64_t mod(int64_t A, int64_t B) {
	return A % B;
}

#define OP_MOD(A, B) _Generic(A, double: fmod, float: fmod, int64_t: mod)(A, B)

extern ml_value_t *ModMethod;

static ml_value_t *value_mod(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	return ml_simple_call(ModMethod, 2, Args);
}

UPDATE_FNS(Mod, mod, OP_MOD, value_mod);

static inline int64_t rmod(int64_t A, int64_t B) {
	return B % A;
}

static inline int64_t rfmod(double A, double B) {
	return fmod(B, A);
}

#define OP_RMOD(A, B) _Generic(A, double: rfmod, float: rfmod, int64_t: rmod)(A, B)

static ml_value_t *value_rmod(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {B, A};
	return ml_simple_call(ModMethod, 2, Args);
}

UPDATE_FNS(RMod, rmod, OP_RMOD, value_rmod);

#include "update_impl_integer.h"

#define OP_XOR(A, B) A | B

extern ml_value_t *XorMethod;

static ml_value_t *value_xor(ml_value_t *A, ml_value_t *B) {
	ml_value_t *Args[2] = {A, B};
	return ml_simple_call(XorMethod, 2, Args);
}

UPDATE_FNS(Xor, xor, OP_XOR, value_xor);

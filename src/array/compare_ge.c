#include "compare_impl_real.h"

#define GE(A, B) A >= B

extern ml_value_t *GreaterEqualMethod;

COMPARE_FNS(Ge, ge, GE, GreaterEqualMethod);

#include "compare_impl_real.h"

#define LE(A, B) A <= B

extern ml_value_t *LessEqualMethod;

COMPARE_FNS(Le, le, LE, LessEqualMethod);

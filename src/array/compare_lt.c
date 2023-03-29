#include "compare_impl_real.h"

#define LT(A, B) A < B

extern ml_value_t *LessMethod;

COMPARE_FNS(Lt, lt, LT, LessMethod);

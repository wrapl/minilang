#include "compare_impl_real.h"

#define GT(A, B) A > B

extern ml_value_t *GreaterMethod;

COMPARE_FNS(Gt, gt, GT, GreaterMethod);

#include "compare_impl_complex.h"

#define NE(A, B) A != B

extern ml_value_t *NotEqualMethod;

COMPARE_FNS(Ne, ne, NE, NotEqualMethod);

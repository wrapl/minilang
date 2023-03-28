#include "compare_impl_complex.h"

#define EQ(A, B) A == B

extern ml_value_t *EqualMethod;

COMPARE_FNS(Eq, eq, EQ, EqualMethod);

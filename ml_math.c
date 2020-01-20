#include "ml_math.h"
#include "ml_macros.h"
#include <math.h>

#define MATH_REAL(NAME, CNAME) \
ML_METHOD(NAME, MLNumberT) { \
	return ml_real(CNAME(ml_real_value(Args[0]))); \
}

#define MATH_REAL_REAL(NAME, CNAME) \
ML_METHOD(NAME, MLNumberT, MLNumberT) { \
	return ml_real(CNAME(ml_real_value(Args[0]), ml_real_value(Args[1]))); \
}

MATH_REAL("acos", acos);
MATH_REAL("asin", asin);
MATH_REAL("atan", atan);
MATH_REAL_REAL("atan", atan2);
MATH_REAL("ceil", ceil);
MATH_REAL("cos", cos);
MATH_REAL("cosh", cosh);
MATH_REAL("exp", exp);
MATH_REAL("abs", fabs);
MATH_REAL("floor", floor);
MATH_REAL_REAL("%", fmod);
MATH_REAL("log", log);
MATH_REAL("log10", log10);
MATH_REAL_REAL("^", pow);
MATH_REAL("sin", sin);
MATH_REAL("sinh", sinh);
MATH_REAL("sqrt", sqrt);
MATH_REAL("tan", tan);
MATH_REAL("tanh", tanh);
MATH_REAL("erf", erf);
MATH_REAL("erfc", erfc);
MATH_REAL("gamma", gamma);
MATH_REAL_REAL("hypot", hypot);
MATH_REAL("lgamma", lgamma);
MATH_REAL("acosh", acosh);
MATH_REAL("asinh", asinh);
MATH_REAL("atanh", atanh);
MATH_REAL("cbrt", cbrt);
MATH_REAL("expm1", expm1);
MATH_REAL("log1p", log1p);
MATH_REAL_REAL("rem", remainder);
MATH_REAL("round", rint);

void ml_math_init(stringmap_t *Globals) {
	if (Globals) {
		ml_value_t *Math = ml_map();

		stringmap_insert(Globals, "math", Math);
	}
#include "ml_math_init.c"
}

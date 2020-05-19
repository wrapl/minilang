#include "ml_math.h"
#include "ml_macros.h"
#include <math.h>

#define MATH_REAL(NAME, CNAME) \
ML_FUNCTION(NAME) { \
	ML_CHECK_ARG_COUNT(1); \
	ML_CHECK_ARG_TYPE(0, MLNumberT); \
	return ml_real(CNAME(ml_real_value(Args[0]))); \
}

#define MATH_REAL_REAL(NAME, CNAME) \
ML_FUNCTION(NAME) { \
	ML_CHECK_ARG_COUNT(2); \
	ML_CHECK_ARG_TYPE(0, MLNumberT); \
	ML_CHECK_ARG_TYPE(1, MLNumberT); \
	return ml_real(CNAME(ml_real_value(Args[0]), ml_real_value(Args[1]))); \
}

ML_METHOD("%", MLNumberT, MLNumberT) {
	return ml_real(fmod(ml_real_value(Args[0]), ml_real_value(Args[1])));
}

ML_METHOD("^", MLNumberT, MLNumberT) {
	return ml_real(pow(ml_real_value(Args[0]), ml_real_value(Args[1])));
}

MATH_REAL(Acos, acos);
MATH_REAL(Asin, asin);

ML_FUNCTION(Atan) {
	ML_CHECK_ARG_COUNT(1);
	if (Count > 1) {
		return ml_real(atan2(ml_real_value(Args[0]), ml_real_value(Args[1])));
	} else {
		return ml_real(atan(ml_real_value(Args[0])));
	}
}

MATH_REAL(Ceil, ceil);
MATH_REAL(Cos, cos);
MATH_REAL(Cosh, cosh);
MATH_REAL(Exp, exp);
MATH_REAL(Abs, fabs);
MATH_REAL(Floor, floor);
MATH_REAL(Log, log);
MATH_REAL(Log10, log10);
MATH_REAL(Sin, sin);
MATH_REAL(Sinh, sinh);
MATH_REAL(Sqrt, sqrt);
MATH_REAL(Tan, tan);
MATH_REAL(Tanh, tanh);
MATH_REAL(Erf, erf);
MATH_REAL(Erfc, erfc);
MATH_REAL_REAL(Hypot, hypot);
MATH_REAL(Gamma, lgamma);
MATH_REAL(Acosh, acosh);
MATH_REAL(Asinh, asinh);
MATH_REAL(Atanh, atanh);
MATH_REAL(Cbrt, cbrt);
MATH_REAL(Expm1, expm1);
MATH_REAL(Log1p, log1p);
MATH_REAL_REAL(Rem, remainder);
MATH_REAL(Round, rint);

void ml_math_init(stringmap_t *Globals) {
#include "ml_math_init.c"
	if (Globals) {
		stringmap_insert(Globals, "math", ml_module("math",
			"acos", Acos,
			"asin", Asin,
			"atan", Atan,
			"ceil", Ceil,
			"cos", Cos,
			"cosh", Cosh,
			"exp", Exp,
			"abs", Abs,
			"floor", Floor,
			"log", Log,
			"log10", Log10,
			"sin", Sin,
			"sinh", Sinh,
			"sqrt", Sqrt,
			"√", Sqrt,
			"tan", Tan,
			"tanh", Tanh,
			"erf", Erf,
			"erfc", Erfc,
			"hypot", Hypot,
			"gamma", Gamma,
			"acosh", Acosh,
			"asinh", Asinh,
			"atanh", Atanh,
			"cbrt", Cbrt,
			"∛", Cbrt,
			"expm1", Expm1,
			"log1p", Log1p,
			"rem", Rem,
			"round", Round,
			"Pi", ml_real(M_PI),
			"E", ml_real(M_E),
		NULL));
	}
}

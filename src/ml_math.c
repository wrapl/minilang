#include "ml_math.h"
#include "ml_macros.h"
#include <math.h>

#define MATH_REAL(NAME, CNAME) \
ML_METHOD_DECL(NAME, NULL); \
\
ML_METHOD(NAME ## Method, MLNumberT) { \
	return ml_real(CNAME(ml_real_value(Args[0]))); \
}

#define MATH_REAL_REAL(NAME, CNAME) \
ML_METHOD_DECL(NAME, NULL); \
\
ML_METHOD(NAME ## Method, MLNumberT, MLNumberT) { \
	return ml_real(CNAME(ml_real_value(Args[0]), ml_real_value(Args[1]))); \
}

ML_METHOD("%", MLNumberT, MLNumberT) {
	return ml_real(fmod(ml_real_value(Args[0]), ml_real_value(Args[1])));
}

ML_METHOD("^", MLNumberT, MLNumberT) {
	return ml_real(pow(ml_real_value(Args[0]), ml_real_value(Args[1])));
}

ML_METHOD("^", MLIntegerT, MLIntegerT) {
	int64_t Base = ml_integer_value(Args[0]);
	int64_t Exponent = ml_integer_value(Args[1]);
	if (Exponent >= 0) {
		int64_t N = 1;
		while (Exponent) {
			if (Exponent & 1) N *= Base;
			Base *= Base;
			Exponent >>= 1;
		}
		return ml_integer(N);
	} else {
		return ml_real(pow(Base, Exponent));
	}
}

MATH_REAL(Acos, acos);
MATH_REAL(Asin, asin);
MATH_REAL(Atan, atan);
ML_METHOD(AtanMethod, MLNumberT, MLNumberT) {
	return ml_real(atan2(ml_real_value(Args[0]), ml_real_value(Args[1])));
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
			"acos", AcosMethod,
			"asin", AsinMethod,
			"atan", AtanMethod,
			"ceil", CeilMethod,
			"cos", CosMethod,
			"cosh", CoshMethod,
			"exp", ExpMethod,
			"abs", AbsMethod,
			"floor", FloorMethod,
			"log", LogMethod,
			"log10", Log10Method,
			"sin", SinMethod,
			"sinh", SinhMethod,
			"sqrt", SqrtMethod,
			"√", SqrtMethod,
			"tan", TanMethod,
			"tanh", TanhMethod,
			"erf", ErfMethod,
			"erfc", ErfcMethod,
			"hypot", HypotMethod,
			"gamma", GammaMethod,
			"acosh", AcoshMethod,
			"asinh", AsinhMethod,
			"atanh", AtanhMethod,
			"cbrt", CbrtMethod,
			"∛", CbrtMethod,
			"expm1", Expm1Method,
			"log1p", Log1pMethod,
			"rem", RemMethod,
			"round", RoundMethod,
			"pi", ml_real(M_PI),
			"π", ml_real(M_PI),
			"e", ml_real(M_E),
		NULL));
	}
}

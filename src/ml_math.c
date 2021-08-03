#include "ml_math.h"
#include "ml_macros.h"
#include <math.h>
#include <float.h>
#ifdef ML_COMPLEX
#include <complex.h>
#endif

#define MATH_REAL(NAME, CNAME) \
ML_METHOD_DECL(NAME ## Method, NULL); \
\
ML_METHOD(NAME ## Method, MLRealT) { \
	return ml_real(CNAME(ml_real_value(Args[0]))); \
}

#ifdef ML_COMPLEX

#define MATH_NUMBER(NAME, CNAME) \
ML_METHOD_DECL(NAME ## Method, NULL); \
\
ML_METHOD(NAME ## Method, MLRealT) { \
	return ml_real(CNAME(ml_real_value(Args[0]))); \
} \
\
ML_METHOD(NAME ## Method, MLComplexT) { \
	complex double Result = c ## CNAME(ml_complex_value(Args[0])); \
	if (fabs(cimag(Result)) <= DBL_EPSILON) { \
		return ml_real(creal(Result)); \
	} else { \
		return ml_complex(Result); \
	} \
}

#else

#define MATH_NUMBER(NAME, CNAME) \
ML_METHOD_DECL(NAME ## Method, NULL); \
\
ML_METHOD(NAME ## Method, MLRealT) { \
	return ml_real(CNAME(ml_real_value(Args[0]))); \
}

#endif

#define MATH_REAL_REAL(NAME, CNAME) \
ML_METHOD_DECL(NAME ## Method, NULL); \
\
ML_METHOD(NAME ## Method, MLRealT, MLRealT) { \
	return ml_real(CNAME(ml_real_value(Args[0]), ml_real_value(Args[1]))); \
}

ML_METHOD("%", MLRealT, MLRealT) {
	return ml_real(fmod(ml_real_value(Args[0]), ml_real_value(Args[1])));
}

ML_METHOD("^", MLIntegerT, MLIntegerT) {
	int64_t Base = ml_integer_value_fast(Args[0]);
	int64_t Exponent = ml_integer_value_fast(Args[1]);
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

ML_METHOD("^", MLRealT, MLIntegerT) {
	return ml_real(pow(ml_double_value_fast(Args[0]), ml_integer_value_fast(Args[1])));
}

ML_METHOD("^", MLRealT, MLRealT) {
	return ml_real(pow(ml_double_value_fast(Args[0]), ml_double_value_fast(Args[1])));
}

#ifdef ML_COMPLEX

ML_METHOD("^", MLComplexT, MLIntegerT) {
	complex double Base = ml_complex_value_fast(Args[0]);
	int64_t Power = ml_integer_value_fast(Args[1]);
	if (Power == 0) return ml_real(0);
	complex double Result;
	if (Power > 0 && Power < 10) {
		Result = Base;
		while (--Power > 0) Result *= Base;
	} else {
		Result = cpow(Base, Power);
	}
	if (fabs(cimag(Result)) <= DBL_EPSILON) {
		return ml_real(creal(Result));
	} else {
		return ml_complex(Result);
	}
}

ML_METHOD("^", MLComplexT, MLNumberT) {
	complex double V = cpow(ml_complex_value_fast(Args[0]), ml_complex_value(Args[1]));
	if (fabs(cimag(V)) <= DBL_EPSILON) {
		return ml_real(creal(V));
	} else {
		return ml_complex(V);
	}
}

ML_METHOD("^", MLNumberT, MLComplexT) {
	complex double V = cpow(ml_complex_value(Args[0]), ml_complex_value_fast(Args[1]));
	if (fabs(cimag(V)) < DBL_EPSILON) {
		return ml_real(creal(V));
	} else {
		return ml_complex(V);
	}
}

#endif

ML_METHOD("!", MLIntegerT) {
	int N = ml_integer_value(Args[0]);
	if (N > 20) return ml_error("RangeError", "Factorials over 20 are not supported yet");
	int64_t F = N;
	while (--N > 1) F *= N;
	return ml_integer(F);
}

#define MIN(X, Y) (X < Y) ? X : Y

ML_METHOD("!", MLIntegerT, MLIntegerT) {
	int N = ml_integer_value(Args[0]);
	int K = ml_integer_value(Args[1]);
	if (K > 20) return ml_error("RangeError", "Factorials over 20 are not supported yet");
	int64_t C = 1;
	if (K > N - K) K = N - K;
	for (int I = 0; I < K; ++I) {
		C *= (N - I);
		C /= (I + 1);
	}
	return ml_integer(C);
}

MATH_NUMBER(Acos, acos);
MATH_NUMBER(Asin, asin);
MATH_NUMBER(Atan, atan);
ML_METHOD(AtanMethod, MLRealT, MLRealT) {
//@atan
//>number
	return ml_real(atan2(ml_real_value(Args[0]), ml_real_value(Args[1])));
}
MATH_REAL(Ceil, ceil);
MATH_NUMBER(Cos, cos);
MATH_NUMBER(Cosh, cosh);
MATH_NUMBER(Exp, exp);
MATH_REAL(Abs, fabs);
MATH_REAL(Floor, floor);
MATH_NUMBER(Log, log);
MATH_NUMBER(Log10, log10);
MATH_NUMBER(Sin, sin);
MATH_NUMBER(Sinh, sinh);
MATH_NUMBER(Sqrt, sqrt);
ML_METHOD(SqrtMethod, MLIntegerT) {
//@sqrt
//>number
	int64_t N = ml_integer_value(Args[0]);
	if (N < 0) return ml_real(-NAN);
	if (N <= 1) return Args[0];
	int64_t X = N >> 1;
	for (;;) {
		int64_t X1 = (X + N / X) >> 1;
		if (X1 >= X) break;
		X = X1;
	}
	if (X * X == N) return ml_integer(X);
	return ml_real(sqrt(N));
}
MATH_NUMBER(Tan, tan);
MATH_NUMBER(Tanh, tanh);
MATH_REAL(Erf, erf);
MATH_REAL(Erfc, erfc);
MATH_REAL_REAL(Hypot, hypot);
MATH_REAL(Gamma, lgamma);
MATH_NUMBER(Acosh, acosh);
MATH_NUMBER(Asinh, asinh);
MATH_NUMBER(Atanh, atanh);
MATH_REAL(Cbrt, cbrt);
MATH_REAL(Expm1, expm1);
MATH_REAL(Log1p, log1p);
MATH_REAL_REAL(Rem, remainder);
MATH_REAL(Round, round);

ML_FUNCTION(IntegerRandom) {
//@integer::random
//<Min?:number
//<Max?:number
//>integer
// Returns a random integer between :mini:`Min` and :mini:`Max` (where :mini:`Max <= 2³² - 1`.
// If omitted, :mini:`Min` defaults to :mini:`0` and :mini:`Max` defaults to :mini:`2³² - 1`.
	if (Count == 2) {
		ML_CHECK_ARG_TYPE(0, MLRealT);
		ML_CHECK_ARG_TYPE(1, MLRealT);
		int Base = ml_integer_value(Args[0]);
		int Limit = ml_integer_value(Args[1]) + 1 - Base;
		if (Limit <= 0) return Args[0];
		int Divisor = RAND_MAX / Limit;
		int Random;
		do Random = random() / Divisor; while (Random > Limit);
		return ml_integer(Base + Random);
	} else if (Count == 1) {
		ML_CHECK_ARG_TYPE(0, MLRealT);
		int Limit = ml_integer_value(Args[0]);
		if (Limit <= 0) return Args[0];
		int Divisor = RAND_MAX / Limit;
		int Random;
		do Random = random() / Divisor; while (Random > Limit);
		return ml_integer(Random + 1);
	} else {
		return ml_integer(random());
	}
}

ML_FUNCTION(IntegerRandomPermutation) {
//@integer::random_permutation
//<Max:number
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	int Limit = ml_integer_value(Args[0]);
	if (Limit <= 0) return ml_error("ValueError", "Permutation requires positive size");
	ml_value_t *Permutation = ml_list();
	ml_list_put(Permutation, ml_integer(1));
	for (int I = 2; I <= Limit; ++I) {
		int Divisor = RAND_MAX / I, J;
		do J = random() / Divisor; while (J > I);
		++J;
		if (J == I) {
			ml_list_put(Permutation, ml_integer(I));
		} else {
			ml_value_t *Old = ml_list_get(Permutation, J);
			ml_list_set(Permutation, J, ml_integer(I));
			ml_list_put(Permutation, Old);
		}
	}
	return Permutation;
}

ML_FUNCTION(IntegerRandomCycle) {
//@integer::random_cycle
//<Max:number
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	int Limit = ml_integer_value(Args[0]);
	if (Limit <= 0) return ml_error("ValueError", "Permutation requires positive size");
	ml_value_t *Permutation = ml_list();
	ml_list_put(Permutation, ml_integer(1));
	if (Limit == 1) return Permutation;
	ml_list_push(Permutation, ml_integer(2));
	for (int I = 2; I < Limit; ++I) {
		int Divisor = RAND_MAX / I, J;
		do J = random() / Divisor; while (J > I);
		++J;
		ml_value_t *Old = ml_list_get(Permutation, J);
		ml_list_set(Permutation, J, ml_integer(I + 1));
		ml_list_put(Permutation, Old);
	}
	return Permutation;
}

ML_FUNCTION(RealRandom) {
//@real::random
//<Min?:number
//<Max?:number
//>real
// Returns a random real between :mini:`Min` and :mini:`Max`.
// If omitted, :mini:`Min` defaults to :mini:`0` and :mini:`Max` defaults to :mini:`1`.
	if (Count == 2) {
		ML_CHECK_ARG_TYPE(0, MLRealT);
		ML_CHECK_ARG_TYPE(1, MLRealT);
		double Base = ml_real_value(Args[0]);
		double Limit = ml_real_value(Args[1]) - Base;
		if (Limit <= 0) return Args[0];
		double Scale = Limit / (double)RAND_MAX;
		return ml_real(Base + random() * Scale);
	} else if (Count == 1) {
		double Limit = ml_real_value(Args[0]);
		if (Limit <= 0) return Args[0];
		double Scale = Limit / (double)RAND_MAX;
		return ml_real(random() * Scale);
	} else {
		return ml_real(random() / (double)RAND_MAX);
	}
}

void ml_math_init(stringmap_t *Globals) {
	srandom(time(NULL));
#include "ml_math_init.c"
	stringmap_insert(MLIntegerT->Exports, "random", IntegerRandom);
	stringmap_insert(MLIntegerT->Exports, "permutation", IntegerRandomPermutation);
	stringmap_insert(MLIntegerT->Exports, "cycle", IntegerRandomCycle);
	stringmap_insert(MLRealT->Exports, "random", RealRandom);
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
			"ℯ", ml_real(M_E),
		NULL));
	}
}

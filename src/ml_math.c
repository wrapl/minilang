#include "ml_math.h"
#include "ml_macros.h"
#include <math.h>
#include <float.h>

#ifdef ML_COMPLEX
#include <complex.h>
#undef I
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "math"

#define MATH_REAL(NAME, CNAME, EXPORT) \
ML_METHOD_DECL(NAME ## Method, NULL); \
\
ML_METHOD(NAME ## Method, MLRealT) { \
/*@math::EXPORT
//>real
// Returns :mini:`EXPORT(Arg/1)`.
//$= math::EXPORT(1.2345)
//$= math::EXPORT(-1.2345)
*/\
	return ml_real(CNAME(ml_real_value(Args[0]))); \
}

#ifdef ML_COMPLEX

#define MATH_NUMBER(NAME, CNAME, EXPORT) \
ML_METHOD_DECL(NAME ## Method, NULL); \
\
ML_METHOD(NAME ## Method, MLRealT) { \
/*@math::EXPORT
//>real
// Returns :mini:`EXPORT(Arg/1)`.
//$= math::EXPORT(1.2345)
//$= math::EXPORT(-1.2345)
*/\
	complex double Result = c ## CNAME(ml_real_value(Args[0])); \
	if (fabs(cimag(Result)) <= DBL_EPSILON) { \
		return ml_real(creal(Result)); \
	} else { \
		return ml_complex(Result); \
	} \
} \
\
ML_METHOD(NAME ## Method, MLComplexT) { \
/*@math::EXPORT
//>complex
// Returns :mini:`EXPORT(Arg/1)`.
//$= math::EXPORT(1.2345 + 6.789i)
//$= math::EXPORT(-1.2345 + 6.789i)
*/\
	complex double Result = c ## CNAME(ml_complex_value(Args[0])); \
	if (fabs(cimag(Result)) <= DBL_EPSILON) { \
		return ml_real(creal(Result)); \
	} else { \
		return ml_complex(Result); \
	} \
}

#define MATH_NUMBER_KEEP_REAL(NAME, CNAME, EXPORT) \
ML_METHOD_DECL(NAME ## Method, NULL); \
\
ML_METHOD(NAME ## Method, MLRealT) { \
/*@math::EXPORT
//>real
// Returns :mini:`EXPORT(Arg/1)`.
//$= math::EXPORT(1.2345)
//$= math::EXPORT(-1.2345)
*/\
	return ml_real(CNAME(ml_real_value(Args[0]))); \
} \
\
ML_METHOD(NAME ## Method, MLComplexT) { \
/*@math::EXPORT
//>complex
// Returns :mini:`EXPORT(Arg/1)`.
//$= math::EXPORT(1.2345 + 6.789i)
//$= math::EXPORT(-1.2345 + 6.789i)
*/\
	complex double Result = c ## CNAME(ml_complex_value(Args[0])); \
	if (fabs(cimag(Result)) <= DBL_EPSILON) { \
		return ml_real(creal(Result)); \
	} else { \
		return ml_complex(Result); \
	} \
}

#else

#define MATH_NUMBER(NAME, CNAME, EXPORT) \
ML_METHOD_DECL(NAME ## Method, NULL); \
\
ML_METHOD(NAME ## Method, MLRealT) { \
/*@math::EXPORT
//>real
// Returns :mini:`EXPORT(Arg/1)`.
//$= math::EXPORT(1.2345)
//$= math::EXPORT(-1.2345)
*/\
	return ml_real(CNAME(ml_real_value(Args[0]))); \
}

#define MATH_NUMBER_KEEP_REAL(NAME, CNAME, EXPORT) MATH_NUMBER(NAME, CNAME, EXPORT)

#endif

#define MATH_REAL_REAL(NAME, CNAME, EXPORT) \
ML_METHOD_DECL(NAME ## Method, NULL); \
\
ML_METHOD(NAME ## Method, MLRealT, MLRealT) { \
/*@math::EXPORT
//>real
// Returns :mini:`EXPORT(Arg/1, Arg/2)`.
*/\
	return ml_real(CNAME(ml_real_value(Args[0]), ml_real_value(Args[1]))); \
}

ML_METHOD("%", MLRealT, MLRealT) {
//<X
//<Y
//>real
// Returns the remainder of :mini:`X` on division by :mini:`Y`.
	return ml_real(fmod(ml_real_value(Args[0]), ml_real_value(Args[1])));
}

ML_METHOD("^", MLIntegerT, MLIntegerT) {
//<X
//<Y
//>number
// Returns :mini:`X` raised to the power of :mini:`Y`.
//$= let N := 2 ^ 2
//$= type(N)
//$= let R := 2 ^ -1
//$= type(R)
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
//<X
//<Y
//>number
// Returns :mini:`X` raised to the power of :mini:`Y`.
//$= 2.3 ^ 2
	return ml_real(pow(ml_real_value(Args[0]), ml_integer_value_fast(Args[1])));
}

ML_METHOD("^", MLRealT, MLRealT) {
//<X
//<Y
//>number
// Returns :mini:`X` raised to the power of :mini:`Y`.
//$= let R := 2.3 ^ 1.5
//$= type(R)
//$= let C := -2.3 ^ 1.5
//$= type(C)
	double Base = ml_real_value(Args[0]);
	double Exponent = ml_real_value(Args[1]);
#ifdef ML_COMPLEX
	if (Base < 0) {
		complex double Result = cpow(Base, Exponent);
		if (fabs(cimag(Result)) <= DBL_EPSILON) {
			return ml_real(creal(Result));
		} else {
			return ml_complex(Result);
		}
	}
#endif
	return ml_real(pow(Base, Exponent));
}

#ifdef ML_COMPLEX

ML_METHOD("^", MLComplexT, MLIntegerT) {
//<X
//<Y
//>number
// Returns :mini:`X` raised to the power of :mini:`Y`.
//$= (1 + 2i) ^ 2
	complex double Base = ml_complex_value(Args[0]);
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
//<X
//<Y
//>number
// Returns :mini:`X` raised to the power of :mini:`Y`.
//$= (1 + 2i) ^ (2 + 3i)
	complex double V = cpow(ml_complex_value(Args[0]), ml_complex_value(Args[1]));
	if (fabs(cimag(V)) <= DBL_EPSILON) {
		return ml_real(creal(V));
	} else {
		return ml_complex(V);
	}
}

ML_METHOD("^", MLNumberT, MLComplexT) {
//<X
//<Y
//>number
// Returns :mini:`X` raised to the power of :mini:`Y`.
//$= 2.3 ^ (1 + 2i)
	complex double V = cpow(ml_complex_value(Args[0]), ml_complex_value(Args[1]));
	if (fabs(cimag(V)) < DBL_EPSILON) {
		return ml_real(creal(V));
	} else {
		return ml_complex(V);
	}
}

#endif

ML_METHOD("!", MLIntegerT) {
//<N
//>integer
// Returns the factorial of :mini:`N`.
//$= !10
	int N = ml_integer_value_fast(Args[0]);
	if (N > 20) return ml_error("RangeError", "Factorials over 20 are not supported yet");
	int64_t F = N;
	while (--N > 1) F *= N;
	return ml_integer(F);
}

#define MIN(X, Y) (X < Y) ? X : Y

ML_METHOD("!", MLIntegerT, MLIntegerT) {
//<N
//<R
//>integer
// Returns the number of ways of choosing :mini:`R` elements from :mini:`N`.
	int N = ml_integer_value_fast(Args[0]);
	int K = ml_integer_value_fast(Args[1]);
	int64_t C = 1;
	if (K > N - K) K = N - K;
	for (int I = 0; I < K; ++I) {
		C *= (N - I);
		C /= (I + 1);
	}
	return ml_integer(C);
}

ML_METHOD_DECL(GCDMethod, "gcd");

ML_METHOD(GCDMethod, MLIntegerT, MLIntegerT) {
//@gcd
//<A
//<B
//>integer
// Returns the greatest common divisor of :mini:`A` and :mini:`B`.
	long A = labs(ml_integer_value_fast(Args[0]));
	long B = labs(ml_integer_value_fast(Args[1]));
	if (A == 0) return Args[1];
	if (B == 0) return Args[0];
	int Shift = __builtin_ctzl(A | B);
	A >>= __builtin_ctz(A);
	do {
		B >>= __builtin_ctz(B);
		if (A > B) {
			unsigned int C = B;
			B = A;
			A = C;
		}
		B = B - A;
	} while (B != 0);
	return ml_integer(A << Shift);
}

MATH_NUMBER_KEEP_REAL(Acos, acos, acos);
MATH_NUMBER_KEEP_REAL(Asin, asin, asin);
MATH_NUMBER_KEEP_REAL(Atan, atan, atan);
ML_METHOD(AtanMethod, MLRealT, MLRealT) {
//@math::atan
//>real
// Returns :mini:`atan(Arg/2 / Arg/1)`.
	return ml_real(atan2(ml_real_value(Args[0]), ml_real_value(Args[1])));
}
MATH_REAL(Ceil, ceil, ceil);
MATH_NUMBER_KEEP_REAL(Cos, cos, cos);
MATH_NUMBER_KEEP_REAL(Cosh, cosh, cosh);
MATH_NUMBER_KEEP_REAL(Exp, exp, exp);
MATH_REAL(Abs, fabs, abs);
ML_METHOD(AbsMethod, MLIntegerT) {
//@abs
//<N
//>integer
// Returns the absolute value of :mini:`N`.
	return ml_integer(labs(ml_integer_value_fast(Args[0])));
}

MATH_REAL(Floor, floor, floor);
ML_METHOD(FloorMethod, MLIntegerT) {
//@floor
//<N
//>integer
// Returns the floor of :mini:`N` (:mini:`= N` for an integer).
	return Args[0];
}
MATH_NUMBER(Log, log, log);
MATH_NUMBER(Log10, log10, log10);
MATH_NUMBER_KEEP_REAL(Sin, sin, sin);
MATH_NUMBER_KEEP_REAL(Sinh, sinh, sinh);
MATH_NUMBER(Sqrt, sqrt, sqrt);
ML_METHOD(SqrtMethod, MLIntegerT) {
//@math::sqrt
//>integer|real
// Returns the square root of :mini:`Arg/1`.
	int64_t N = ml_integer_value_fast(Args[0]);
	if (N < 0) {
#ifdef ML_COMPLEX
		return ml_complex(csqrt(N));
#else
		return ml_real(-NAN);
#endif
	}
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

ML_METHOD_DECL(SquareMethod, NULL);
ML_METHOD(SquareMethod, MLIntegerT) {
//@math::square
//<N
//>integer
// Returns :mini:`N * N`
//$= math::square(10)
	int64_t N = ml_integer_value_fast(Args[0]);
	return ml_integer(N * N);
}
ML_METHOD(SquareMethod, MLRealT) {
//@math::square
//<R
//>real
// Returns :mini:`R * R`
//$= math::square(1.234)
	double N = ml_real_value(Args[0]);
	return ml_real(N * N);
}
#ifdef ML_COMPLEX
ML_METHOD(SquareMethod, MLComplexT) {
//@math::square
//<C
//>complex
// Returns :mini:`C * C`
//$= math::square(1 + 2i)
	complex double N = ml_complex_value(Args[0]);
	return ml_complex(N * N);
}
#endif

MATH_NUMBER_KEEP_REAL(Tan, tan, tan);
MATH_NUMBER_KEEP_REAL(Tanh, tanh, tanh);
MATH_REAL(Erf, erf, erf);
MATH_REAL(Erfc, erfc, erfc);
MATH_REAL_REAL(Hypot, hypot, hypot);
MATH_REAL(Gamma, lgamma, gamma);
MATH_NUMBER_KEEP_REAL(Acosh, acosh, acosh);
MATH_NUMBER_KEEP_REAL(Asinh, asinh, asinh);
MATH_NUMBER_KEEP_REAL(Atanh, atanh, atanh);
MATH_REAL(Cbrt, cbrt, cbrt);
MATH_REAL(Expm1, expm1, expm1);
MATH_REAL(Log1p, log1p, log1p);
MATH_REAL_REAL(Rem, remainder, rem);
MATH_REAL(Round, round, round);

double logit(double X) {
	return log(X / (1 - X));
}

MATH_REAL(Logit, logit, logit);

ML_METHOD_DECL(ArgMethod, "arg");

ML_METHOD(ArgMethod, MLRealT) {
//@arg
//<R
//>real
// Returns the complex argument of :mini:`R` (:mini:`= 0` for a real number).
	return ml_real(0.0);
}

ML_METHOD_DECL(ConjMethod, "conj");

ML_METHOD(ConjMethod, MLRealT) {
//@conj
//<R
//>real
// Returns the complex conjugate of :mini:`R` (:mini:`= R` for a real number).
	return Args[0];
}

#ifdef ML_COMPLEX

ML_METHOD(AbsMethod, MLComplexT) {
//@abs
//<Z
//>real
// Returns the absolute value (magnitude) of :mini:`Z`.
	return ml_real(cabs(ml_complex_value(Args[0])));
}

ML_METHOD(ArgMethod, MLComplexT) {
//@arg
//<Z
//>real
// Returns the complex argument of :mini:`Z`.
	return ml_real(carg(ml_complex_value(Args[0])));
}

ML_METHOD(ConjMethod, MLComplexT) {
//@conj
//<Z
//>real
// Returns the complex conjugate of :mini:`Z`.
	return ml_complex(conj(ml_complex_value(Args[0])));
}

#endif

/*
ML_DEF(math::pi);
//>real
// Pi.

ML_DEF(math::e);
//>real
// Euler's constant.
*/

void ml_math_init(stringmap_t *Globals) {
#include "ml_math_init.c"
	if (Globals) {
		stringmap_insert(Globals, "math", ml_module("math",
			"gcd", GCDMethod,
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
			"logit", LogitMethod,
			"sin", SinMethod,
			"sinh", SinhMethod,
			"sqrt", SqrtMethod,
			"square", SquareMethod,
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
			"arg", ArgMethod,
			"conj", ConjMethod,
			"pi", ml_real(M_PI),
			"π", ml_real(M_PI),
			"e", ml_real(M_E),
			"ℯ", ml_real(M_E),
		NULL));
	}
}

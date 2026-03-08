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
ML_METHOD_DECL(NAME ## Method, "math::" #EXPORT); \
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
ML_METHOD_DECL(NAME ## Method, "math::" #EXPORT); \
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
ML_METHOD_DECL(NAME ## Method, "math::" #EXPORT); \
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
ML_METHOD_DECL(NAME ## Method, "math::" #EXPORT); \
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
ML_METHOD_DECL(NAME ## Method, "math::" #EXPORT); \
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
	int64_t Exponent = ml_integer_value(Args[1]);
#ifdef ML_BIGINT
	if (Exponent < 0) {
		mpz_t A; ml_integer_mpz_init(A, Args[0]);
		if (!A->_mp_size) return ml_real(NAN);
#ifdef ML_RATIONAL
		mpq_t Result;
		mpz_init_set_ui(mpq_numref(Result), 1);
		ml_integer_mpz_init(mpq_denref(Result), Args[0]);
		mpz_pow_ui(mpq_denref(Result), mpq_denref(Result), -Exponent);
		return ml_rational_mpq(Result);
#else
		double Base = ml_real_value(Args[0]);
		return ml_real(pow(Base, Exponent));
#endif
	} else if (Exponent > 0) {
		mpz_t Result; ml_integer_mpz_init(Result, Args[0]);
		if (!Result->_mp_size) return ml_real(NAN);
		mpz_pow_ui(Result, Result, Exponent);
		return ml_integer_mpz(Result);
	} else {
		mpz_t A; ml_integer_mpz_init(A, Args[0]);
		if (!A->_mp_size) return ml_real(NAN);
		return ml_integer(1);
	}
#else
	int64_t Base = ml_integer_value(Args[0]);
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
#endif
}

#ifdef ML_RATIONAL

ML_METHOD("^", MLRationalT, MLIntegerT) {
	int64_t Exponent = ml_integer_value(Args[1]);
	if (!Exponent) return ml_integer(1);
	int Invert = 0;
	if (Exponent < 0) {
		Exponent = -Exponent;
		Invert = 1;
	}
#ifdef ML_BIGINT
	mpq_t Result; ml_rational_mpq_init(Result, Args[0]);
	mpz_pow_ui(mpq_numref(Result), mpq_numref(Result), Exponent);
	mpz_pow_ui(mpq_denref(Result), mpq_denref(Result), Exponent);
	if (Invert) mpq_inv(Result, Result);
	return ml_rational_mpq(Result);
#else
	rat64_t Base = ml_rational_value(Args[0]);
	rat64_t Result = {1, 1};
	while (Exponent) {
		if (Exponent & 1) {
			Result.Num *= Base.Num;
			Result.Den *= Base.Den;
		}
		Base.Num *= Base.Num;
		Base.Den *= Base.Den;
		Exponent >>= 1;
	}
	if (Invert) {
		if (Result.Num < 0) return ml_rational(-(int64_t)Result.Den, -Result.Num);
		return ml_rational(Result.Den, Result.Num);
	}
	return ml_rational(Result.Num, Result.Den);
#endif
}

ML_METHOD("^", MLIntegerT, MLRationalT) {
	rat64_t Exponent = ml_rational_value(Args[1]);
#ifdef ML_BIGINT
	if (Exponent.Num < 0) {
		mpz_t A; ml_integer_mpz_init(A, Args[0]);
		if (!A->_mp_size) return ml_integer(0);
		mpq_t Result; mpq_init(Result);
		if ((Exponent.Den % 2 || mpz_sgn(A) > 0)) {
			if (mpz_root(mpq_denref(Result), A, Exponent.Den)) {
				mpz_pow_ui(mpq_denref(Result), mpq_denref(Result), -Exponent.Num);
				mpz_init_set_si(mpq_numref(Result), 1);
				return ml_rational_mpq(Result);
			}
		}
	} else if (Exponent.Num > 0) {
		mpz_t A; ml_integer_mpz_init(A, Args[0]);
		if (!A->_mp_size) return ml_integer(0);
		mpz_t Result; mpz_init(Result);
		if (Exponent.Den % 2 || mpz_sgn(A) > 0) {
			if (mpz_root(Result, A, Exponent.Den)) {
				mpz_pow_ui(Result, Result, Exponent.Num);
				return ml_integer_mpz(Result);
			}
		}
	} else {
		mpz_t A; ml_integer_mpz_init(A, Args[0]);
		if (!A->_mp_size) return ml_real(NAN);
		return ml_integer(1);
	}
#endif
	double Base = ml_real_value(Args[0]);
	if (Exponent.Den % 2 || Base >= 0) {
		return ml_real(pow(rootn(Base, Exponent.Den), Exponent.Num));
	}
#ifdef ML_COMPLEX
	return ml_complex(cpow(Base, (double)Exponent.Num / Exponent.Den));
#else
	return ml_real(NAN);
#endif
}

ML_METHOD("^", MLRationalT, MLRationalT) {
	rat64_t Exponent = ml_rational_value(Args[1]);
#ifdef ML_BIGINT
	if (Exponent.Num < 0) {
		mpq_t A; ml_rational_mpq_init(A, Args[0]);
		if (!mpq_numref(A)->_mp_size) return ml_integer(0);
		mpq_t Result; mpq_init(Result);
		if ((Exponent.Den % 2 || mpz_sgn(mpq_numref(A)) > 0)) {
			if (mpz_root(mpq_denref(Result), mpq_numref(A), Exponent.Den) &&
				mpz_root(mpq_numref(Result), mpq_denref(A), Exponent.Den)
			) {
				mpz_pow_ui(mpq_denref(Result), mpq_denref(Result), -Exponent.Num);
				mpz_pow_ui(mpq_numref(Result), mpq_numref(Result), -Exponent.Num);
				return ml_rational_mpq(Result);
			}
		}
	} else if (Exponent.Num > 0) {
		mpq_t A; ml_rational_mpq_init(A, Args[0]);
		if (!mpq_numref(A)->_mp_size) return ml_integer(0);
		mpq_t Result; mpq_init(Result);
		if ((Exponent.Den % 2 || mpz_sgn(mpq_numref(A)) > 0)) {
			if (mpz_root(mpq_denref(Result), mpq_denref(A), Exponent.Den) &&
				mpz_root(mpq_numref(Result), mpq_numref(A), Exponent.Den)
			) {
				mpz_pow_ui(mpq_denref(Result), mpq_denref(Result), Exponent.Num);
				mpz_pow_ui(mpq_numref(Result), mpq_numref(Result), Exponent.Num);
				return ml_rational_mpq(Result);
			}
		}
	} else {
		mpq_t A; ml_rational_mpq_init(A, Args[0]);
		if (!mpq_numref(A)->_mp_size) return ml_real(NAN);
		return ml_integer(1);
	}
#endif
	double Base = ml_real_value(Args[0]);
	if (Exponent.Den % 2 || Base >= 0) {
		return ml_real(pow(rootn(Base, Exponent.Den), Exponent.Num));
	}
#ifdef ML_COMPLEX
	return ml_complex(cpow(Base, (double)Exponent.Num / Exponent.Den));
#else
	return ml_real(NAN);
#endif
}

#endif

ML_METHOD("^", MLRealT, MLIntegerT) {
//<X
//<Y
//>number
// Returns :mini:`X` raised to the power of :mini:`Y`.
//$= 2.3 ^ 2
	return ml_real(pow(ml_real_value(Args[0]), ml_integer_value(Args[1])));
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
	int64_t Power = ml_integer_value(Args[1]);
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
#ifdef ML_BIGINT
	mpz_t F; mpz_init(F);
	mpz_fac_ui(F, ml_integer_value(Args[0]));
	return ml_integer_mpz(F);
#else
	int N = ml_integer_value(Args[0]);
	if (N > 20) return ml_error("RangeError", "Factorials over 20 are not supported yet");
	int64_t F = N;
	while (--N > 1) F *= N;
	return ml_integer(F);
#endif
}

ML_METHOD("!", MLIntegerT, MLIntegerT) {
//<N
//<R
//>integer
// Returns the number of ways of choosing :mini:`R` elements from :mini:`N`.
#ifdef ML_BIGINT
	mpz_t N; ml_integer_mpz_init(N, Args[0]);
	mpz_t K; ml_integer_mpz_init(K, Args[1]);
	if (!mpz_fits_ulong_p(K)) mpz_sub(K, N, K);
	if (!mpz_fits_ulong_p(K)) return ml_error("RangeError", "Value out of bounds");
	mpz_t C; mpz_init(C);
	mpz_bin_ui(C, N, mpz_get_s64(K));
	return ml_integer_mpz(C);
#else
	int N = ml_integer_value(Args[0]);
	int K = ml_integer_value(Args[1]);
	int64_t C = 1;
	if (K > N - K) K = N - K;
	for (int I = 0; I < K; ++I) {
		C *= (N - I);
		C /= (I + 1);
	}
	return ml_integer(C);
#endif
}

ML_METHOD_DECL(GCDMethod, "math::gcd");

ML_METHOD(GCDMethod, MLIntegerT, MLIntegerT) {
//@gcd
//<A
//<B
//>integer
// Returns the greatest common divisor of :mini:`A` and :mini:`B`.
#ifdef ML_BIGINT
	mpz_t A; ml_integer_mpz_init(A, Args[0]);
	mpz_t B; ml_integer_mpz_init(B, Args[1]);
	mpz_t C; mpz_init(C);
	mpz_gcd(C, A, B);
	return ml_integer_mpz(C);
#else
	unsigned long A = labs(ml_integer_value(Args[0]));
	unsigned long B = labs(ml_integer_value(Args[1]));
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
#endif
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

ML_METHOD(CeilMethod, MLRealT, MLRealT) {
//@math::ceil
//>real
// Returns :mini:`ceil(Arg/1 * Arg/2) / Arg/2`.
//$= math::ceil(1.2345, 100)
//$= math::ceil(-1.2345, 32)
	double Scale = ml_real_value(Args[1]);
	return ml_real(ceil(ml_real_value(Args[0]) * Scale) / Scale);
}

MATH_NUMBER_KEEP_REAL(Cos, cos, cos);
MATH_NUMBER_KEEP_REAL(Cosh, cosh, cosh);
MATH_NUMBER_KEEP_REAL(Exp, exp, exp);
MATH_REAL(Abs, fabs, abs);
ML_METHOD(AbsMethod, MLIntegerT) {
//@abs
//<N
//>integer
// Returns the absolute value of :mini:`N`.
	return ml_integer(labs(ml_integer_value(Args[0])));
}

#ifdef ML_COMPLEX
#ifndef __USE_GNU

complex double clog10(complex double Z) {
	return clog(Z) / log(10);
}

#endif
#endif

MATH_REAL(Floor, floor, floor);
ML_METHOD(FloorMethod, MLIntegerT) {
//@floor
//<N
//>integer
// Returns the floor of :mini:`N` (:mini:`= N` for an integer).
	return Args[0];
}

ML_METHOD(FloorMethod, MLRealT, MLRealT) {
//@math::floor
//>real
// Returns :mini:`floor(Arg/1 * Arg/2) / Arg/2`.
//$= math::floor(1.2345, 100)
//$= math::floor(-1.2345, 32)
	double Scale = ml_real_value(Args[1]);
	return ml_real(floor(ml_real_value(Args[0]) * Scale) / Scale);
}

MATH_NUMBER(Log, log, log);
MATH_NUMBER(Log10, log10, log10);
MATH_NUMBER_KEEP_REAL(Sin, sin, sin);
MATH_NUMBER_KEEP_REAL(Sinh, sinh, sinh);
MATH_NUMBER(Sqrt, sqrt, sqrt);

static uint64_t ml_integer_sqrt(uint64_t N) {
	if (N <= 1) return N;
	uint64_t X = N >> 1;
	for (;;) {
		uint64_t X1 = (X + N / X) >> 1;
		if (X1 >= X) break;
		X = X1;
	}
	if (X * X == N) return X;
	return UINT64_MAX;
}

ML_METHOD(SqrtMethod, MLIntegerT) {
//@math::sqrt
//>integer|real
// Returns the square root of :mini:`Arg/1`.
#ifdef ML_BIGINT
	mpz_t N; ml_integer_mpz_init(N, Args[0]);
	switch (mpz_sgn(N)) {
	case 0: return ml_integer(0);
	case 1: {
		mpz_t Result; mpz_init(Result);
		if (mpz_root(Result, N, 2)) {
			return ml_integer_mpz(Result);
		} else {
			return ml_real(sqrt(mpz_get_d(N)));
		}
	}
	case -1: {
#ifdef ML_COMPLEX
		return ml_complex(csqrt(mpz_get_d(N)));
#else
		return ml_real(-NAN);
#endif
	}
	default: __builtin_unreachable();
	}
#else
	int64_t N = ml_integer_value(Args[0]);
	if (N < 0) {
#ifdef ML_COMPLEX
		return ml_complex(csqrt(N));
#else
		return ml_real(-NAN);
#endif
	}
	uint64_t Sqrt = ml_integer_sqrt(N);
	if (Sqrt == UINT64_MAX) return ml_real(sqrt(N));
	return ml_integer(Sqrt);
#endif
}

#ifdef ML_NANBOXING

ML_METHOD(SqrtMethod, MLInteger32T) {
//!internal
	int64_t N = ml_integer_value(Args[0]);
	if (N < 0) {
#ifdef ML_COMPLEX
		return ml_complex(csqrt(N));
#else
		return ml_real(-NAN);
#endif
	}
	uint64_t Sqrt = ml_integer_sqrt(N);
	if (Sqrt == UINT64_MAX) return ml_real(sqrt(N));
	return ml_integer(Sqrt);
}

#endif

#ifdef ML_RATIONAL

ML_METHOD(SqrtMethod, MLRationalT) {
//@math::sqrt
//>rational|real
// Returns the square root of :mini:`Arg/1`.
#ifdef ML_BIGINT
	mpq_t R; ml_rational_mpq_init(R, Args[0]);
	switch (mpz_sgn(mpq_numref(R))) {
	case 0: return ml_integer(0);
	case 1: {
		mpq_t Result; mpq_init(Result);
		if (mpz_root(mpq_numref(Result), mpq_numref(R), 2) && mpz_root(mpq_denref(Result), mpq_denref(R), 2)) {
			return ml_rational_mpq(Result);
		} else {
			return ml_real(sqrt(mpq_get_d(R)));
		}
	}
	case -1: {
#ifdef ML_COMPLEX
		return ml_complex(csqrt(mpq_get_d(R)));
#else
		return ml_real(-NAN);
#endif
	}
	default: __builtin_unreachable();
	}
#else
	rat64_t R = ml_rational_value(Args[0]);
	if (R.Num < 0) {
#ifdef ML_COMPLEX
		return ml_complex(csqrt((double)R.Num / (double)R.Den));
#else
		return ml_real(-NAN);
#endif
	}
	uint64_t Num = ml_integer_sqrt(R.Num);
	if (Num == UINT64_MAX) return ml_real(sqrt((double)R.Num / (double)R.Den));
	uint64_t Den = ml_integer_sqrt(R.Den);
	if (Den == UINT64_MAX) return ml_real(sqrt((double)R.Num / (double)R.Den));
	return ml_rational(Num, Den);
#endif
}

#ifdef ML_NANBOXING

ML_METHOD(SqrtMethod, MLRational48T) {
//!internal
	rat64_t R = ml_rational_value(Args[0]);
	if (R.Num < 0) {
#ifdef ML_COMPLEX
		return ml_complex(csqrt((double)R.Num / (double)R.Den));
#else
		return ml_real(-NAN);
#endif
	}
	uint64_t Num = ml_integer_sqrt(R.Num);
	if (Num == UINT64_MAX) return ml_real(sqrt((double)R.Num / (double)R.Den));
	uint64_t Den = ml_integer_sqrt(R.Den);
	if (Den == UINT64_MAX) return ml_real(sqrt((double)R.Num / (double)R.Den));
	return ml_rational(Num, Den);
}

#endif

#endif

ML_METHOD_DECL(SquareMethod, "math::square");
ML_METHOD(SquareMethod, MLIntegerT) {
//@math::square
//<N
//>integer
// Returns :mini:`N * N`
//$= math::square(10)
	int64_t N = ml_integer_value(Args[0]);
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

ML_METHOD(RoundMethod, MLRealT, MLRealT) {
//@math::round
//>real
// Returns :mini:`round(Arg/1 * Arg/2) / Arg/2`.
//$= math::round(1.2345, 100)
//$= math::round(-1.2345, 32)
	double Scale = ml_real_value(Args[1]);
	return ml_real(round(ml_real_value(Args[0]) * Scale) / Scale);
}

double logit(double X) {
	return log(X / (1 - X));
}

MATH_REAL(Logit, logit, logit);

ML_METHOD_DECL(ArgMethod, "math::arg");

ML_METHOD(ArgMethod, MLRealT) {
//@arg
//<R
//>real
// Returns the complex argument of :mini:`R` (:mini:`= 0` for a real number).
	return ml_real(0.0);
}

ML_METHOD_DECL(ConjMethod, "math::conj");

ML_METHOD(ConjMethod, MLRealT) {
//@conj
//<R
//>real
// Returns the complex conjugate of :mini:`R` (:mini:`= R` for a real number).
	return Args[0];
}

ML_METHOD_DECL(DeltaMethod, "math::delta");

ML_METHOD(DeltaMethod, MLIntegerT, MLIntegerT) {
//@math::delta
#ifdef ML_BIGINT
	mpz_t A; ml_integer_mpz_init(A, Args[0]);
	mpz_t B; ml_integer_mpz_init(B, Args[1]);
	if (!mpz_cmp(A, B)) return ml_integer(1);
#else
	if (ml_integer_value(Args[0]) == ml_integer_value(Args[1])) {
		return ml_integer(1);
	}
#endif
	return ml_integer(0);
}

ML_METHOD(DeltaMethod, MLRealT, MLRealT) {
//@math::delta
	if (ml_real_value(Args[0]) == ml_real_value(Args[1])) {
		return ml_integer(1);
	} else {
		return ml_integer(0);
	}
}

#ifdef ML_RATIONAL

ML_METHOD(DeltaMethod, MLRationalT, MLRationalT) {
//@math::delta
#ifdef ML_BIGINT
	mpq_t A; ml_rational_mpq_init(A, Args[0]);
	mpq_t B; ml_rational_mpq_init(B, Args[1]);
	if (!mpq_cmp(A, B)) return ml_integer(1);
#else
	rat64_t A = ml_rational_value(Args[0]);
	rat64_t B = ml_rational_value(Args[1]);
	if (A.Num == B.Num && A.Den == B.Den) {
		return ml_integer(1);
	}
#endif
	return ml_integer(0);
}

#endif

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

ML_METHOD(DeltaMethod, MLComplexT, MLComplexT) {
//@math::delta
	if (ml_complex_value(Args[0]) == ml_complex_value(Args[1])) {
		return ml_integer(1);
	} else {
		return ml_integer(0);
	}
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

ML_TYPE(MLRandomT, (MLFunctionT), "random");

ML_FUNCTION(MLRandomSeed) {
//@random::seed
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	srand(ml_integer_value(Args[0]));
	return MLNil;
}

typedef struct {
	ml_type_t *Type;
	double Cases[];
} ml_random_switch_t;

static void ml_random_switch(ml_state_t *Caller, ml_random_switch_t *Switch, int Count, ml_value_t **Args) {
	double X = ml_random_real();
	for (double *Case = Switch->Cases;; ++Case) {
		if (X < *Case) ML_RETURN(ml_integer(Case - Switch->Cases));
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLRandomSwitchT, (MLFunctionT), "random-switch",
//!internal
	.call = (void *)ml_random_switch
);

ML_FUNCTION_INLINE(MLRandomSwitch) {
//!internal
	ml_random_switch_t *Switch = xnew(ml_random_switch_t, Count, double);
	Switch->Type = MLRandomSwitchT;
	double *Case = Switch->Cases;
	double Total = 0;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		if (ml_list_length(Args[I]) != 1) return ml_error("ValueError", "Each random case must be a single value");
		Total += ml_real_value(ml_list_get(Args[I], 1));
		*Case++ = Total;
	}
	*--Case = 1.0;
	while (--Count > 0) *--Case /= Total;
	return (ml_value_t *)Switch;
}

static void ml_random_choice(ml_state_t *Caller, ml_random_switch_t *Switch, int Count, ml_value_t **Args) {
	double X = ml_random_real();
	for (double *Case = Switch->Cases;; ++Case) {
		if (X < *Case) ML_RETURN(ml_integer((Case - Switch->Cases) + 1));
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLRandomChoiceT, (MLFunctionT), "random::choice",
	.call = (void *)ml_random_choice
);

ML_FUNCTION(MLRandomChoice) {
	ML_CHECK_ARG_COUNT(1);
	ml_random_switch_t *Choice = xnew(ml_random_switch_t, Count, double);
	Choice->Type = MLRandomChoiceT;
	double *Case = Choice->Cases;
	double Total = 0;
	for (int I = 0; I < Count; ++I) {
		Total += ml_real_value(Args[I]);
		*Case++ = Total;
	}
	*--Case = 1.0;
	while (--Count > 0) *--Case /= Total;
	return (ml_value_t *)Choice;
}

void ml_math_init(stringmap_t *Globals) {
#include "ml_math_init.c"
	MLRandomT->Constructor = ml_method("random");
	stringmap_insert(MLRandomT->Exports, "seed", MLRandomSeed);
	stringmap_insert(MLRandomT->Exports, "switch", MLRandomSwitch);
	stringmap_insert(MLRandomT->Exports, "choice", MLRandomChoice);
	stringmap_insert(MLRandomT->Exports, "by", ml_method("random::by"));
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
			"delta", DeltaMethod,
			"pi", ml_real(M_PI),
			"π", ml_real(M_PI),
			"e", ml_real(M_E),
			"ℯ", ml_real(M_E),
		NULL));
		stringmap_insert(Globals, "random", MLRandomT);
	}
}

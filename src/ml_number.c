#include "minilang.h"
#include "ml_macros.h"
#include <math.h>
#include <float.h>
#include <limits.h>
#include <inttypes.h>

#ifdef ML_COMPLEX
#include <complex.h>
#undef I
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "number"

typedef struct {
	ml_state_t Base;
	ml_value_t *Function;
	int Index, Count;
	ml_value_t *Args[];
} ml_infix_state_t;

static void ml_infix_run(ml_infix_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	int Index = State->Index;
	if (Index + 1 == State->Count) ML_RETURN(Value);
	State->Args[Index] = Value;
	State->Index = Index + 1;
	return ml_call(State, State->Function, 2, State->Args + Index);
}

static void ml_infix_many_fn(ml_state_t *Caller, void *Infix, int Count, ml_value_t **Args) {
	ml_infix_state_t *State = xnew(ml_infix_state_t, Count, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_infix_run;
	State->Function = (ml_value_t *)Infix;
	State->Count = Count;
	State->Index = 1;
	for (int I = 0; I < Count; ++I) State->Args[I] = Args[I];
	return ml_call(State, Infix, 2, State->Args);
}

static void ml_infix_many(const char *Name) {
	ml_value_t *Method = ml_method(Name);
	ml_method_definev(Method, ml_cfunctionx(Method, ml_infix_many_fn), MLAnyT, MLAnyT, MLAnyT, MLAnyT, NULL);
}

ML_TYPE(MLNumberT, (), "number");
// Base type for numbers.

static int ML_TYPED_FN(ml_value_is_constant, MLNumberT, ml_value_t *Value) {
	return 1;
}

#define ml_neg(X) (-(X))
#define ml_com(X) (~(X))

#define ml_add(X, Y) ((X) + (Y))
#define ml_sub(X, Y) ((X) - (Y))
#define ml_mul(X, Y) ((X) * (Y))
#define ml_div(X, Y) ((X) / (Y))

#define ml_and(X, Y) ((X) & (Y))
#define ml_ior(X, Y) ((X) | (Y))
#define ml_xor(X, Y) ((X) ^ (Y))

#define ml_eq(X, Y) ((X) == (Y))
#define ml_neq(X, Y) ((X) != (Y))
#define ml_lt(X, Y) ((X) < (Y))
#define ml_gt(X, Y) ((X) > (Y))
#define ml_lte(X, Y) ((X) <= (Y))
#define ml_gte(X, Y) ((X) >= (Y))

#define ml_diff(X, Y) (_Generic((X) - (Y), long: labs, default: fabs)((X) - (Y)))

#ifdef ML_COMPLEX

static long ml_complex_hash(ml_complex_t *Complex, ml_hash_chain_t *Chain) {
	return (long)creal(Complex->Value);
}

ML_TYPE(MLComplexT, (MLNumberT), "complex",
	.hash = (void *)ml_complex_hash
);

ml_value_t *ml_complex(complex double Value) {
	ml_complex_t *Complex = new(ml_complex_t);
	Complex->Type = MLComplexT;
	Complex->Value = Value;
	return (ml_value_t *)Complex;
}

ML_METHOD(MLComplexT, MLRealT) {
	return ml_complex(ml_real_value(Args[0]));
}

ML_METHOD(MLComplexT, MLRealT, MLRealT) {
	return ml_complex(ml_real_value(Args[0]) + ml_real_value(Args[1]) * _Complex_I);
}

ML_METHOD(MLRealT, MLComplexT) {
	return ml_real(creal(ml_complex_value(Args[0])));
}

extern complex double ml_complex_value(const ml_value_t *Value);

#ifdef ML_NANBOXING

complex double ml_complex_value(const ml_value_t *Value) {
	int Tag = ml_tag(Value);
	if (Tag == 1) return (int32_t)(intptr_t)Value;
	if (Tag >= 7) return ml_double_value(Value);
	if (Tag == 0 && Value->Type == MLComplexT) return ((ml_complex_t *)Value)->Value;
	typeof(ml_complex_value) *fn = ml_typed_fn_get(ml_typeof(Value), ml_complex_value);
	if (fn) return fn(Value);
	typeof(ml_real_value) *fn2 = ml_typed_fn_get(ml_typeof(Value), ml_real_value);
	if (fn2) return fn2(Value);
	typeof(ml_integer_value) *fn3 = ml_typed_fn_get(ml_typeof(Value), ml_integer_value);
	if (fn3) return fn3(Value);
	return 0;
}

static complex double ML_TYPED_FN(ml_complex_value, MLInteger64T, ml_integer_t *Value) {
#ifdef ML_BIGINT
	return mpz_get_d(Value->Value);
#else
	return Value->Value;
#endif
}

#else

complex double ml_complex_value(const ml_value_t *Value) {
	if (Value->Type == MLIntegerT) {
#ifdef ML_BIGINT
		return mpz_get_d(((ml_integer_t *)Value)->Value);
#else
		return ((ml_integer_t *)Value)->Value;
#endif
	} else if (Value->Type == MLDoubleT) {
		return ((ml_double_t *)Value)->Value;
	} else if (Value->Type == MLComplexT) {
		return ((ml_complex_t *)Value)->Value;
	}
	typeof(ml_complex_value) *fn = ml_typed_fn_get(ml_typeof(Value), ml_complex_value);
	if (fn) return fn(Value);
	typeof(ml_real_value) *fn2 = ml_typed_fn_get(ml_typeof(Value), ml_real_value);
	if (fn2) return fn2(Value);
	typeof(ml_integer_value) *fn3 = ml_typed_fn_get(ml_typeof(Value), ml_integer_value);
	if (fn3) return fn3(Value);
	return 0;
}

static complex double ML_TYPED_FN(ml_complex_value, MLIntegerT, ml_integer_t *Value) {
#ifdef ML_BIGINT
	return mpz_get_d(Value->Value);
#else
	return Value->Value;
#endif
}

static complex double ML_TYPED_FN(ml_complex_value, MLDoubleT, ml_double_t *Value) {
	return Value->Value;
}

#endif

static complex double ML_TYPED_FN(ml_complex_value, MLComplexT, ml_complex_t *Value) {
	return Value->Value;
}

#define ml_arith_method_complex(NAME, FUNC) \
ML_METHOD(#NAME, MLComplexT) { \
/*<A
//>complex
// Returns :mini:`NAMEA`.
*/\
	complex double ComplexA = ml_complex_value(Args[0]); \
	complex double ComplexB = ml_ ## FUNC(ComplexA); \
	if (fabs(cimag(ComplexB)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexB)); \
	} else { \
		return ml_complex(ComplexB); \
	} \
}

#define ml_arith_method_complex_complex(NAME, FUNC) \
ML_METHOD(#NAME, MLComplexT, MLComplexT) { \
/*<A
//<B
//>real
// complex :mini:`A NAME B`.
*/\
	complex double ComplexA = ml_complex_value(Args[0]); \
	complex double ComplexB = ml_complex_value(Args[1]); \
	complex double ComplexC = ml_ ## FUNC(ComplexA, ComplexB); \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

#define ml_arith_method_complex_integer(NAME, FUNC) \
ML_METHOD(#NAME, MLComplexT, MLIntegerT) { \
/*<A
//<B
//>complex
// Returns :mini:`A NAME B`.
*/\
	complex double ComplexA = ml_complex_value(Args[0]); \
	int64_t IntegerB = ml_integer_value(Args[1]); \
	complex double ComplexC = ml_ ## FUNC(ComplexA, IntegerB); \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

#define ml_arith_method_integer_complex(NAME, FUNC) \
ML_METHOD(#NAME, MLIntegerT, MLComplexT) { \
/*<A
//<B
//>complex
// Returns :mini:`A NAME B`.
*/\
	int64_t IntegerA = ml_integer_value(Args[0]); \
	complex double ComplexB = ml_complex_value(Args[1]); \
	complex double ComplexC = ml_ ## FUNC(IntegerA, ComplexB); \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

#define ml_arith_method_complex_real(NAME, FUNC) \
ML_METHOD(#NAME, MLComplexT, MLDoubleT) { \
/*<A
//<B
//>complex
// Returns :mini:`A NAME B`.
*/\
	complex double ComplexA = ml_complex_value(Args[0]); \
	double RealB = ml_double_value(Args[1]); \
	complex double ComplexC = ml_ ## FUNC(ComplexA, RealB); \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

#define ml_arith_method_real_complex(NAME, FUNC) \
ML_METHOD(#NAME, MLDoubleT, MLComplexT) { \
/*<A
//<B
//>complex
// Returns :mini:`A NAME B`.
*/\
	double RealA = ml_double_value(Args[0]); \
	complex double ComplexB = ml_complex_value(Args[1]); \
	complex double ComplexC = ml_ ## FUNC(RealA, ComplexB); \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

ML_METHOD("r", MLComplexT) {
//<Z
//>real
// Returns the real component of :mini:`Z`.
	return ml_real(creal(ml_complex_value(Args[0])));
}

ML_METHOD("i", MLComplexT) {
//<Z
//>real
// Returns the imaginary component of :mini:`Z`.
	return ml_real(cimag(ml_complex_value(Args[0])));
}

ML_TYPE(MLRealT, (MLComplexT), "real");
#else
ML_TYPE(MLRealT, (MLNumberT), "real");
//!internal
#endif
// Base type for real numbers.

ML_TYPE(MLIntegerT, (MLRealT, MLFunctionT), "integer");
// Base type of integers.

/*
ML_DEF(Inf);
//!number
//@real::Inf
//>real
// Positive infinity.

ML_DEF(NaN);
//!number
//@real::NaN
//>real
// Not a number.
*/

#ifdef ML_BIGINT

#define LIMBS64 ((64 + GMP_NUMB_BITS  - 1) / GMP_NUMB_BITS)

int64_t mpz_get_s64(const mpz_t Z) {
	int N = Z->_mp_size;
#if LIMBS64 == 1
	if (N < 0) return -Z->_mp_d[0];
	if (N > 0) return Z->_mp_d[0];
	return 0;
#else
	uint64_t V = 0;
	if (N < 0) {
		N = -N;
		if (N > LIMBS64) N = LIMBS64;
		while (N--) V = (V << GMP_NUMB_BITS) | Z->_mp_d[N];
		return -V;
	} else if (N > 0) {
		if (N > LIMBS64) N = LIMBS64;
		while (N--) V = (V << GMP_NUMB_BITS) | Z->_mp_d[N];
		return V;
	} else {
		return 0;
	}
#endif
}

void mpz_set_s64(mpz_t Z, int64_t V) {
	mpz_init2(Z, 64);
	uint64_t Abs = V < 0 ? -V : V;
#if LIMBS64 == 1
	Z->_mp_d[0] = (mp_limb_t)Abs;
	Z->_mp_size = V < 0 ? -1 : V ? 1 : 0;
#else
	int I = 0;
	while (I < LIMBS64 && Abs) {
		Z->_mp_d[I++] = Abs & GMP_NUMB_MASK;
		Abs >>= GMP_NUMB_BITS;
	}
	Z->_mp_size = V < 0 ? -I : V ? I : 0;
#endif
}

uint64_t mpz_get_u64(const mpz_t Z) {
	int N = Z->_mp_size;
#if LIMBS64 == 1
	if (N > 0) return Z->_mp_d[0];
	return 0;
#else
	uint64_t V = 0;
	if (N < 0) N = -N;
	if (N > LIMBS64) N = LIMBS64;
	while (N--) V = (V << GMP_NUMB_BITS) | Z->_mp_d[N];
	return V;
#endif
}

void mpz_set_u64(mpz_t Z, uint64_t V) {
	mpz_init2(Z, 64);
#if LIMBS64 == 1
	Z->_mp_d[0] = (mp_limb_t)V;
	Z->_mp_size = V ? 1 : 0;
#else
	int I = 0;
	while (I < LIMBS64 && V) {
		Z->_mp_d[I++] = V & GMP_NUMB_MASK;
		V >>= GMP_NUMB_BITS;
	}
	Z->_mp_size = V ? I : 0;
#endif
}

#endif

#ifdef ML_NANBOXING

static long ml_integer32_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return (int32_t)(intptr_t)Value;
}

static void ml_integer32_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	long Index = (int32_t)(intptr_t)Value;
	if (Index <= 0) Index += Count + 1;
	if (Index <= 0) ML_RETURN(MLNil);
	if (Index > Count) ML_RETURN(MLNil);
	ML_RETURN(Args[Index - 1]);
}

ML_TYPE(MLInteger32T, (MLIntegerT), "integer32",
//!internal
	.hash = (void *)ml_integer32_hash,
	.call = (void *)ml_integer32_call,
	.NoInherit = 1
);

static long ml_integer64_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return ml_integer64_value(Value);
}

ML_TYPE(MLInteger64T, (MLIntegerT), "integer64",
//!internal
	.hash = (void *)ml_integer64_hash,
	.NoInherit = 1
);

ml_value_t *ml_integer64(int64_t Integer) {
	ml_integer_t *Value = new(ml_integer_t);
	Value->Type = MLInteger64T;
#ifdef ML_BIGINT
	mpz_init2(Value->Value, 64);
	uint64_t Abs = Integer < 0 ? -Integer : Integer;
#if LIMBS64 == 1
	Value->Value->_mp_d[0] = (mp_limb_t)Abs;
	Value->Value->_mp_size = Integer < 0 ? -1 : Integer ? 1 : 0;
#else
	int I = 0;
	while (I < LIMBS64 && Abs) {
		Value->Value->_mp_d[I++] = Abs & GMP_NUMB_MASK;
		Abs >>= GMP_NUMB_BITS;
	}
	Value->Value->_mp_size = Integer < 0 ? -I : Integer ? I : 0;
#endif
#else
	Value->Value = Integer;
#endif
	return (ml_value_t *)Value;
}

int64_t ml_integer_value(const ml_value_t *Value) {
	int Tag = ml_tag(Value);
	if (Tag == 1) return (int32_t)(intptr_t)Value;
	if (Tag >= 7) return ml_double_value(Value);
	if (Tag == 0 && Value->Type == MLInteger64T) return ml_integer64_value(Value);
	typeof(ml_integer_value) *fn = ml_typed_fn_get(ml_typeof(Value), ml_integer_value);
	if (fn) return fn(Value);
	return 0;
}

static int64_t ML_TYPED_FN(ml_integer_value, MLInteger64T, const ml_value_t *Value) {
	return ml_integer64_value(Value);
}

ml_value_t *ml_integer_parse(char *String) {
	return ml_integer(strtoll(String, NULL, 10));
}

uint64_t ml_gcd(uint64_t A, uint64_t B) {
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
	return A << Shift;
}

#ifdef ML_RATIONAL

ML_TYPE(MLRationalT, (MLRealT), "rational");
//!internal

static long ml_rational48_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return (uint64_t)(intptr_t)Value & 0xFFFFFFFFFFFF;
}

ML_TYPE(MLRational48T, (MLRationalT), "rational48",
//!internal
	.hash = (void *)ml_rational48_hash,
	.NoInherit = 1
);

static long ml_rational64_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
#ifdef ML_BIGINT
	return fmpq_get_d(((ml_rational_t *)Value)->Value);
#else
	return ((ml_integer_t *)Value)->Value;
#endif
}

ML_TYPE(MLRational64T, (MLIntegerT), "rational64",
//!internal
	.hash = (void *)ml_rational64_hash,
	.NoInherit = 1
);

ml_value_t *ml_rational64(int64_t Num, uint64_t Den) {
	ml_rational_t *Value = new(ml_rational_t);
	Value->Type = MLRational64T;
#ifdef ML_BIGINT
	fmpq_set_si(Value->Value, Num, Den);
#else
	Value->Num = Num;
	Value->Den = Den;
#endif
	return (ml_value_t *)Value;
}

rat64_t ml_rational_value(const ml_value_t *Value) {
	int Tag = ml_tag(Value);
	if (Tag == 1) return (rat64_t){(int32_t)(intptr_t)Value, 1};
	if (Tag == 2) return (rat64_t){(int32_t)(intptr_t)Value, ((uint64_t)(intptr_t)Value >> 48) & 0xFFFF};
	if (Tag >= 7) {
		double Dbl = ml_double_value(Value);
		// TODO: Approximate rational here
		return (rat64_t){Dbl, 1};
	}
	if (Tag != 0) return (rat64_1){0, 1};
	if (ml_is_subtype(Value->Type, MLInteger64T)) {
#ifdef ML_BIGINT
		return (rat64_t){mpz_get_si(((ml_integer_t *)Value)->Value), 1};
#else
		return (rat64_t){((ml_integer_t *)Value)->Value, 1};
#endif
	}
	if (ml_is_subtype(Value->Type, MLRational64T)) {
#ifdef ML_BIGINT
		return (rat64_t){mpz_get_si(((ml_integer_t *)Value)->Value), 1};
#else
		return (rat64_t){((ml_integer_t *)Value)->Value, 1};
#endif
	}
	return (rat64_t){0, 1};
}

#endif

ML_METHOD(MLRealT, MLInteger32T) {
//!internal
	return ml_real((int32_t)(intptr_t)Args[0]);
}

ML_METHOD(MLRealT, MLInteger64T) {
//!internal
#ifdef ML_BIGINT
	return ml_real(mpz_get_d(((ml_integer_t *)Args[0])->Value));
#else
	return ml_real(((ml_integer_t *)Args[0])->Value);
#endif
}

#else

static long ml_integer64_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return ml_integer64_value(Value);
}

static void ml_integer64_call(ml_state_t *Caller, ml_value_t *Integer, int Count, ml_value_t **Args) {
	long Index = ml_integer64_value(Integer);
	if (Index <= 0) Index += Count + 1;
	if (Index <= 0) ML_RETURN(MLNil);
	if (Index > Count) ML_RETURN(MLNil);
	ML_RETURN(Args[Index - 1]);
}

ML_TYPE(MLInteger64T, (MLIntegerT), "integer",
// A 64-bit signed integer value.
//
// :mini:`fun (I: integer)(Arg/1, ..., Arg/n): any | nil`
//    Returns the :mini:`I`-th argument or :mini:`nil` if there is no :mini:`I`-th argument. Negative values of :mini:`I` are counted from the last argument.
//    In particular, :mini:`0(...)` always returns :mini:`nil` and :mini:`1` behaves as the identity function.
//$= 2("a", "b", "c")
//$= -1("a", "b", "c")
//$= 4("a", "b", "c")
//$= 0("a", "b", "c")
	.hash = (void *)ml_integer64_hash,
	.call = (void *)ml_integer64_call
);

ml_value_t *ml_integer(int64_t Value) {
	ml_integer_t *Integer = new(ml_integer_t);
	Integer->Type = MLInteger64T;
#ifdef ML_BIGINT
	mpz_set_s64(Integer->Value, Value);
#else
	Integer->Value = Value;
#endif
	return (ml_value_t *)Integer;
}

int64_t ml_integer_value(const ml_value_t *Value) {
	if (Value->Type == MLInteger64T) {
		return ml_integer64_value(Value);
	} else if (Value->Type == MLDoubleT) {
		return ((ml_double_t *)Value)->Value;
	} else {
		typeof(ml_integer_value) *fn = ml_typed_fn_get(ml_typeof(Value), ml_integer_value);
		if (fn) return fn(Value);
		return 0;
	}
}

ML_METHOD(MLRealT, MLIntegerT) {
	return ml_real(ml_integer_value(Args[0]));
}

#endif

static long ml_double_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return (long)ml_double_value(Value);
}

ML_TYPE(MLDoubleT, (MLRealT), "double",
	.hash = (void *)ml_double_hash,
	.NoInherit = 1
);

#ifdef ML_NANBOXING

double ml_real_value(const ml_value_t *Value) {
	int Tag = ml_tag(Value);
	if (Tag == 1) return (int32_t)(intptr_t)Value;
	if (Tag >= 7) return ml_double_value(Value);
	typeof(ml_real_value) *fn = ml_typed_fn_get(ml_typeof(Value), ml_real_value);
	if (fn) return fn(Value);
	typeof(ml_integer_value) *fn2 = ml_typed_fn_get(ml_typeof(Value), ml_integer_value);
	if (fn2) return fn2(Value);
	return 0;
}

static double ML_TYPED_FN(ml_real_value, MLInteger64T, ml_integer_t *Value) {
#ifdef ML_BIGINT
	return mpz_get_d(Value->Value);
#else
	return Value->Value;
#endif
}

ml_value_t *ml_real_parse(char *String) {
	return ml_real(strtod(String, NULL));
}

ML_METHOD(MLDoubleT, MLInteger32T) {
//!internal
	return ml_real((int32_t)(intptr_t)Args[0]);
}

ML_METHOD(MLDoubleT, MLInteger64T) {
//!internal
#ifdef ML_BIGINT
	return ml_real(mpz_get_d(((ml_integer_t *)Args[0])->Value));
#else
	return ml_real(((ml_integer_t *)Args[0])->Value);
#endif
}

#else

ml_value_t *ml_real(double Value) {
	ml_double_t *Real = new(ml_double_t);
	Real->Type = MLDoubleT;
	Real->Value = Value;
	return (ml_value_t *)Real;
}

double ml_real_value(const ml_value_t *Value) {
	if (Value->Type == MLIntegerT) {
#ifdef ML_BIGINT
		return mpz_get_d(((ml_integer_t *)Value)->Value);
#else
		return ((ml_integer_t *)Value)->Value;
#endif
	} else if (Value->Type == MLDoubleT) {
		return ((ml_double_t *)Value)->Value;
	}
	typeof(ml_real_value) *fn = ml_typed_fn_get(ml_typeof(Value), ml_real_value);
	if (fn) return fn(Value);
	typeof(ml_integer_value) *fn2 = ml_typed_fn_get(ml_typeof(Value), ml_integer_value);
	if (fn2) return fn2(Value);
	return 0;
}

static double ML_TYPED_FN(ml_real_value, MLIntegerT, ml_integer_t *Value) {
#ifdef ML_BIGINT
	return mpz_get_d(Value->Value);
#else
	return Value->Value;
#endif
}

ML_METHOD(MLDoubleT, MLIntegerT) {
//!internal
	return ml_real(ml_integer_value(Args[0]));
}

#endif

ML_METHOD(MLIntegerT, MLDoubleT) {
//<Real
//>integer
// Converts :mini:`Real` to an integer (using default rounding).
	return ml_integer(ml_double_value(Args[0]));
}

#ifdef ML_NANBOXING

#define ml_arith_method_integer32(NAME, FUNC) \
ML_METHOD(#NAME, MLInteger32T) { \
/*<A
//>integer
// Returns :mini:`NAMEA`.
*/\
	int64_t IntegerA = ml_integer32_value(Args[0]); \
	return ml_integer(ml_ ## FUNC(IntegerA)); \
}

#define ml_arith_method_integer32_integer32(NAME, FUNC) \
ML_METHOD(#NAME, MLInteger32T, MLInteger32T) { \
/*<A
//<B
//>integer
// Returns :mini:`A NAME B`.
*/\
	int64_t IntegerA = ml_integer32_value(Args[0]); \
	int64_t IntegerB = ml_integer32_value(Args[1]); \
	return ml_integer(ml_ ## FUNC(IntegerA, IntegerB)); \
}

#define ml_arith_method_integer32_integer32_bitwise(NAME, FUNC, OP) \
ML_METHOD(#NAME, MLInteger32T, MLInteger32T) { \
/*<A
//<B
//>integer
// Returns the bitwise OP of :mini:`A` and :mini:`B`.
*/\
	int64_t IntegerA = ml_integer32_value(Args[0]); \
	int64_t IntegerB = ml_integer32_value(Args[1]); \
	return ml_integer(ml_ ## FUNC(IntegerA, IntegerB)); \
}

#else

#define ml_arith_method_integer32(NAME, FUNC)
#define ml_arith_method_integer32_integer32(NAME, FUNC)
#define ml_arith_method_integer32_integer32_bitwise(NAME, FUNC, OP)

#endif

#ifdef ML_BIGINT

#ifdef ML_NANBOXING

void ml_integer_mpz_init(mpz_t Dest, ml_value_t *Source) {
	int Tag = ml_tag(Source);
	if (__builtin_expect(Tag, 1)) {
		return mpz_init_set_si(Dest, ml_integer32_value(Source));
	} else if (Tag == 0 && Source->Type == MLInteger64T) {
		return mpz_init_set(Dest, ((ml_integer_t *)Source)->Value);
	} else if (Tag >= 7) {
		return mpz_init_set_d(Dest, ml_double_value(Source));
	}
	typeof(ml_integer_mpz_init) *fn = ml_typed_fn_get(Source->Type, ml_integer_mpz_init);
	if (fn) return fn(Dest, Source);
	mpz_init(Dest);
}

ml_value_t *ml_integer_mpz_copy(const mpz_t Source) {
	if (mpz_fits_sint_p(Source)) {
		return ml_integer32(mpz_get_si(Source));
	} else {
		ml_integer_t *Value = new(ml_integer_t);
		Value->Type = MLInteger64T;
		mpz_init_set(Value->Value, Source);
		return (ml_value_t *)Value;
	}
}

ml_value_t *ml_integer_mpz(mpz_t Source) {
	if (mpz_fits_sint_p(Source)) {
		return ml_integer32(mpz_get_si(Source));
	} else {
		ml_integer_t *Value = new(ml_integer_t);
		Value->Type = MLInteger64T;
		mpz_init(Value->Value);
		Value->Value[0] = Source[0];
		return (ml_value_t *)Value;
	}
}

#else

void ml_integer_mpz_init(mpz_t Dest, ml_value_t *Source) {
	if (Source->Type == MLInteger64T) {
		return mpz_init_set(Dest, ((ml_integer_t *)Source)->Value);
	} else if (Source->Type == MLDoubleT) {
		return mpz_init_set_d(Dest, ml_double_value(Source));
	}
	typeof(ml_integer_mpz_init) *fn = ml_typed_fn_get(Source->Type, ml_integer_mpz_init);
	if (fn) return fn(Dest, Source);
	mpz_init(Dest);
}

ml_value_t *ml_integer_mpz(const mpz_t Source) {
	ml_integer_t *Value = new(ml_integer_t);
	Value->Type = MLInteger64T;
	mpz_init_set(Value->Value, Source);
	return (ml_value_t *)Value;
}

#endif

#ifdef ML_RATIONAL

ml_value_t *ml_rational_mpq(mpq_t Source) {
	__mpz_struct *Num = mpq_numref(Source);
	__mpz_struct *Den = mpq_denref(Source);
	if (mpz_fits_sint_p(Num) && mpz_fits_ushort_p(Den)) {
		return ml_rational48(mpz_get_si(Num), mpz_get_ui(Den));
	} else {
		ml_rational_t *Value = new(ml_rational_t);
		Value->Type = MLRational64T;
		fmpq_set(Value->Value, Source);
		return (ml_value_t *)Value;
	}
}

#endif

static void mpz_diff(mpz_t f, const mpz_t g, const mpz_t h) {
	mpz_sub(f, g, h);
	mpz_abs(f, f);
}

#define ml_arith_method_integer(NAME, FUNC) \
\
ml_arith_method_integer32(NAME, FUNC) \
\
ML_METHOD(#NAME, MLInteger64T) { \
/*<A
//>integer
// Returns :mini:`NAMEA`.
*/\
	ml_integer_t *A = (ml_integer_t *)Args[0]; \
	mpz_t Result; mpz_init(Result); \
	mpz_ ## FUNC(Result, A->Value); \
	return ml_integer_mpz(Result); \
} \
\
ML_METHOD(#NAME, MLIntegerT) { \
/*<A
//>integer
// Returns :mini:`NAMEA`.
*/\
	mpz_t IntegerA; ml_integer_mpz_init(IntegerA, Args[0]); \
	mpz_t Result; mpz_init(Result); \
	mpz_ ## FUNC(Result, IntegerA); \
	return ml_integer_mpz(Result); \
}

#define ml_arith_method_integer_integer(NAME, FUNC) \
\
ml_arith_method_integer32_integer32(NAME, FUNC) \
\
ML_METHOD(#NAME, MLInteger64T, MLInteger64T) { \
/*<A
//<B
//>integer
// Returns :mini:`A NAME B`.
*/\
	ml_integer_t *A = (ml_integer_t *)Args[0]; \
	ml_integer_t *B = (ml_integer_t *)Args[1]; \
	mpz_t Result; mpz_init(Result); \
	mpz_ ## FUNC(Result, A->Value, B->Value); \
	return ml_integer_mpz(Result); \
} \
\
ML_METHOD(#NAME, MLIntegerT, MLIntegerT) { \
/*<A
//<B
//>integer
// Returns :mini:`A NAME B`.
*/\
	mpz_t Result; ml_integer_mpz_init(Result, Args[0]); \
	mpz_t IntegerB; ml_integer_mpz_init(IntegerB, Args[1]); \
	mpz_ ## FUNC(Result, Result, IntegerB); \
	return ml_integer_mpz(Result); \
}

#define ml_arith_method_integer_integer_bitwise(NAME, FUNC, OP) \
\
ml_arith_method_integer32_integer32_bitwise(NAME, FUNC, OP) \
\
ML_METHOD(#NAME, MLInteger64T, MLInteger64T) { \
/*<A
//<B
//>integer
// Returns :mini:`A NAME B`.
*/\
	ml_integer_t *A = (ml_integer_t *)Args[0]; \
	ml_integer_t *B = (ml_integer_t *)Args[1]; \
	mpz_t Result; mpz_init(Result); \
	mpz_ ## FUNC(Result, A->Value, B->Value); \
	return ml_integer_mpz(Result); \
} \
\
ML_METHOD(#NAME, MLIntegerT, MLIntegerT) { \
/*<A
//<B
//>integer
// Returns the bitwise OP of :mini:`A` and :mini:`B`.
*/\
	mpz_t Result; ml_integer_mpz_init(Result, Args[0]); \
	mpz_t IntegerB; ml_integer_mpz_init(IntegerB, Args[1]); \
	mpz_ ## FUNC(Result, Result, IntegerB); \
	return ml_integer_mpz(Result); \
}

#ifdef ML_NANBOXING

ML_METHOD("+", MLInteger64T, MLInteger32T) {
	ml_integer_t *A = (ml_integer_t *)Args[0];
	int32_t B = ml_integer32_value(Args[1]);
	mpz_t Result; mpz_init_set(Result, A->Value);
	if (B > 0) {
		mpz_add_ui(Result, Result, (uint32_t)B);
	} else {
		mpz_sub_ui(Result, Result, (uint32_t)-(int64_t)B);
	}
	return ml_integer_mpz(Result);
}

ML_METHOD("+", MLInteger32T, MLInteger64T) {
	int32_t A = ml_integer32_value(Args[0]);
	ml_integer_t *B = (ml_integer_t *)Args[1];
	mpz_t Result; mpz_init_set(Result, B->Value);
	if (A > 0) {
		mpz_add_ui(Result, Result, (uint32_t)A);
	} else {
		mpz_sub_ui(Result, Result, (uint32_t)-(int64_t)A);
	}
	return ml_integer_mpz(Result);
}

ML_METHOD("-", MLInteger64T, MLInteger32T) {
	ml_integer_t *A = (ml_integer_t *)Args[0];
	int32_t B = ml_integer32_value(Args[1]);
	mpz_t Result; mpz_init_set(Result, A->Value);
	if (B > 0) {
		mpz_sub_ui(Result, Result, (uint32_t)B);
	} else {
		mpz_add_ui(Result, Result, (uint32_t)-(int64_t)B);
	}
	return ml_integer_mpz(Result);
}

ML_METHOD("-", MLInteger32T, MLInteger64T) {
	int32_t A = ml_integer32_value(Args[0]);
	ml_integer_t *B = (ml_integer_t *)Args[1];
	mpz_t Result; mpz_init_set(Result, B->Value);
	mpz_neg(Result, Result);
	if (A > 0) {
		mpz_add_ui(Result, Result, (uint32_t)A);
	} else {
		mpz_sub_ui(Result, Result, (uint32_t)-(int64_t)A);
	}
	return ml_integer_mpz(Result);
}

ML_METHOD("*", MLInteger64T, MLInteger32T) {
	ml_integer_t *A = (ml_integer_t *)Args[0];
	int32_t B = ml_integer32_value(Args[1]);
	mpz_t Result; mpz_init_set(Result, A->Value);
	mpz_mul_si(Result, Result, B);
	return ml_integer_mpz(Result);
}

ML_METHOD("*", MLInteger32T, MLInteger64T) {
	int32_t A = ml_integer32_value(Args[0]);
	ml_integer_t *B = (ml_integer_t *)Args[1];
	mpz_t Result; mpz_init_set(Result, B->Value);
	mpz_mul_si(Result, Result, A);
	return ml_integer_mpz(Result);
}

ML_METHOD("/", MLInteger64T, MLInteger32T) {
	ml_integer_t *A = (ml_integer_t *)Args[0];
	int32_t B = ml_integer32_value(Args[1]);
	if (!B) return ml_error("ValueError", "Division by 0");
	mpq_t Quotient;
	mpz_init_set_si(mpq_denref(Quotient), B);
	mpz_init_set(mpq_numref(Quotient), A->Value);
	mpq_canonicalize(Quotient);
	if (!mpz_cmp_ui(mpq_denref(Quotient), 1)) {
		return ml_integer_mpz(mpq_numref(Quotient));
	} else {
		return ml_real(mpq_get_d(Quotient));
	}
}

ML_METHOD("/", MLInteger32T, MLInteger64T) {
	int32_t A = ml_integer32_value(Args[0]);
	ml_integer_t *B = (ml_integer_t *)Args[1];
	if (!B->Value->_mp_size) return ml_error("ValueError", "Division by 0");
	mpq_t Quotient;
	mpz_init_set(mpq_denref(Quotient), B->Value);
	mpz_init_set_si(mpq_numref(Quotient), A);
	mpq_canonicalize(Quotient);
	if (!mpz_cmp_ui(mpq_denref(Quotient), 1)) {
		return ml_integer_mpz(mpq_numref(Quotient));
	} else {
		return ml_real(mpq_get_d(Quotient));
	}
}

#endif

#else

#define ml_arith_method_integer(NAME, FUNC) \
\
ml_arith_method_integer32(NAME, FUNC) \
\
ML_METHOD(#NAME, MLIntegerT) { \
/*<A
//>integer
// Returns :mini:`NAMEA`.
*/\
	int64_t IntegerA = ml_integer_value(Args[0]); \
	return ml_integer(ml_ ## FUNC(IntegerA)); \
}

#define ml_arith_method_integer_integer(NAME, FUNC) \
\
ml_arith_method_integer32_integer32(NAME, FUNC) \
\
ML_METHOD(#NAME, MLIntegerT, MLIntegerT) { \
/*<A
//<B
//>integer
// Returns :mini:`A NAME B`.
*/\
	int64_t IntegerA = ml_integer_value(Args[0]); \
	int64_t IntegerB = ml_integer_value(Args[1]); \
	return ml_integer(ml_ ## FUNC(IntegerA, IntegerB)); \
}

#define ml_arith_method_integer_integer_bitwise(NAME, FUNC, OP) \
\
ml_arith_method_integer32_integer32_bitwise(NAME, FUNC, OP) \
\
ML_METHOD(#NAME, MLIntegerT, MLIntegerT) { \
/*<A
//<B
//>integer
// Returns the bitwise OP of :mini:`A` and :mini:`B`.
*/\
	int64_t IntegerA = ml_integer_value(Args[0]); \
	int64_t IntegerB = ml_integer_value(Args[1]); \
	return ml_integer(ml_ ## FUNC(IntegerA, IntegerB)); \
}

#endif

#define ml_arith_method_real(NAME, FUNC) \
ML_METHOD(#NAME, MLDoubleT) { \
/*<A
//>real
// Returns :mini:`NAMEA`.
*/\
	double RealA = ml_double_value(Args[0]); \
	return ml_real(ml_ ## FUNC(RealA)); \
} \
\
ML_METHOD(#NAME, MLRealT) { \
/*<A
//>real
// Returns :mini:`NAMEA`.
*/\
	double RealA = ml_real_value(Args[0]); \
	return ml_real(ml_ ## FUNC(RealA)); \
}

#define ml_arith_method_real_real(NAME, FUNC) \
ML_METHOD(#NAME, MLDoubleT, MLDoubleT) { \
/*<A
//<B
//>real
// Returns :mini:`A NAME B`.
*/\
	double RealA = ml_double_value(Args[0]); \
	double RealB = ml_double_value(Args[1]); \
	return ml_real(ml_ ## FUNC(RealA, RealB)); \
} \
\
ML_METHOD(#NAME, MLRealT, MLRealT) { \
/*<A
//<B
//>real
// Returns :mini:`A NAME B`.
*/\
	double RealA = ml_real_value(Args[0]); \
	double RealB = ml_real_value(Args[1]); \
	return ml_real(ml_ ## FUNC(RealA, RealB)); \
}

#ifdef ML_COMPLEX

#define ml_arith_method_number(NAME, FUNC) \
ml_arith_method_integer(NAME, FUNC) \
ml_arith_method_real(NAME, FUNC) \
ml_arith_method_complex(NAME, FUNC)

#define ml_arith_method_number_number(NAME, FUNC) \
ml_arith_method_integer_integer(NAME, FUNC) \
ml_arith_method_real_real(NAME, FUNC) \
ml_arith_method_complex_complex(NAME, FUNC) \
ml_arith_method_complex_real(NAME, FUNC) \
ml_arith_method_complex_integer(NAME, FUNC) \
ml_arith_method_integer_complex(NAME, FUNC) \
ml_arith_method_real_complex(NAME, FUNC) \

#else

#define ml_arith_method_number(NAME, FUNC) \
ml_arith_method_integer(NAME, FUNC) \
ml_arith_method_real(NAME, FUNC)

#define ml_arith_method_number_number(NAME, FUNC) \
ml_arith_method_integer_integer(NAME, FUNC) \
ml_arith_method_real_real(NAME, FUNC)

#endif

ml_arith_method_number(-, neg)
ml_arith_method_number_number(+, add)
ml_arith_method_number_number(-, sub)
ml_arith_method_number_number(~, diff)
ml_arith_method_number_number(*, mul)
ml_arith_method_integer(~, com);
ml_arith_method_integer_integer_bitwise(/\\, and, and);
ml_arith_method_integer_integer_bitwise(\\/, ior, or);
ml_arith_method_integer_integer_bitwise(><, xor, xor);

ML_METHOD("popcount", MLIntegerT) {
//<A
//>integer
// Returns the number of bits set in :mini:`A`.
	int64_t A = ml_integer_value(Args[0]);
	return ml_integer(__builtin_popcount(A));
}

ML_METHOD("<<", MLIntegerT, MLIntegerT) {
//<A
//<B
//>integer
// Returns :mini:`A << B`.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	int64_t IntegerC;
	if (IntegerB > 0) {
		IntegerC = IntegerA << IntegerB;
	} else if (IntegerB < 0) {
		if (IntegerA < 0) {
			IntegerC = -(-IntegerA >> -IntegerB);
		} else {
			IntegerC = IntegerA >> -IntegerB;
		}
	} else {
		IntegerC = IntegerA;
	}
	return ml_integer(IntegerC);
}

ML_METHOD(">>", MLIntegerT, MLIntegerT) {
//<A
//<B
//>integer
// Returns :mini:`A >> B`.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	int64_t IntegerC;
	if (IntegerB > 0) {
		if (IntegerA < 0) {
			IntegerC = -(-IntegerA >> IntegerB);
		} else {
			IntegerC = IntegerA >> IntegerB;
		}
	} else if (IntegerB < 0) {
		IntegerC = IntegerA << -IntegerB;
	} else {
		IntegerC = IntegerA;
	}
	return ml_integer(IntegerC);
}

ML_METHOD("++", MLIntegerT) {
//<Int
//>integer
// Returns :mini:`Int + 1`
	return ml_integer(ml_integer_value(Args[0]) + 1);
}

ML_METHOD("--", MLIntegerT) {
//<Int
//>integer
// Returns :mini:`Int - 1`
	return ml_integer(ml_integer_value(Args[0]) - 1);
}

ML_METHODZ("inc", MLIntegerT) {
//<X
//>integer
// Atomic equivalent to :mini:`X := old + 1`.
	int64_t Value = ml_integer_value(ml_deref(Args[0]));
	return ml_assign(Caller, Args[0], ml_integer(Value + 1));
}

ML_METHODZ("dec", MLIntegerT) {
//<X
//>integer
// Atomic equivalent to :mini:`X := old - 1`.
	int64_t Value = ml_integer_value(ml_deref(Args[0]));
	return ml_assign(Caller, Args[0], ml_integer(Value - 1));
}

ML_METHODZ("inc", MLIntegerT, MLIntegerT) {
//<X
//<Y
//>integer
// Atomic equivalent to :mini:`X := old + Y`.
	int64_t Value = ml_integer_value(ml_deref(Args[0]));
	int64_t Amount = ml_integer_value(ml_deref(Args[1]));
	return ml_assign(Caller, Args[0], ml_integer(Value + Amount));
}

ML_METHODZ("dec", MLIntegerT, MLIntegerT) {
//<X
//<Y
//>integer
// Atomic equivalent to :mini:`X := old - Y`.
	int64_t Value = ml_integer_value(ml_deref(Args[0]));
	int64_t Amount = ml_integer_value(ml_deref(Args[1]));
	return ml_assign(Caller, Args[0], ml_integer(Value - Amount));
}

ML_METHOD("++", MLDoubleT) {
//<Real
//>real
// Returns :mini:`Real + 1`
	return ml_real(ml_double_value(Args[0]) + 1);
}

ML_METHOD("--", MLDoubleT) {
//<Real
//>real
// Returns :mini:`Real - 1`
	return ml_real(ml_double_value(Args[0]) - 1);
}

ml_arith_method_real_real(/, div)

#ifdef ML_COMPLEX

ml_arith_method_complex_complex(/, div)
ml_arith_method_complex_integer(/, div)
ml_arith_method_integer_complex(/, div)
ml_arith_method_complex_real(/, div)
ml_arith_method_real_complex(/, div)
ml_arith_method_complex(~, com);

#endif

#ifdef ML_NANBOXING

ML_METHOD("/", MLInteger32T, MLInteger32T) {
//<Int/1
//<Int/2
//>integer | real
// Returns :mini:`Int/1 / Int/2` as an integer if the division is exact, otherwise as a real.
//$= let N := 10 / 2
//$= type(N)
//$= let R := 10 / 3
//$= type(R)
	int64_t IntegerA = ml_integer32_value(Args[0]);
	int64_t IntegerB = ml_integer32_value(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	ldiv_t Div = ldiv(IntegerA, IntegerB);
	if (Div.rem) {
		return ml_real((double)IntegerA / (double)IntegerB);
	} else {
		return ml_integer(Div.quot);
	}
}

ML_METHOD("%", MLInteger32T, MLInteger32T) {
//<Int/1
//<Int/2
//>integer
// Returns the remainder of :mini:`Int/1` divided by :mini:`Int/2`.
// Note: the result is calculated by rounding towards 0. In particular, if :mini:`Int/1` is negative, the result will be negative.
// For a nonnegative remainder, use :mini:`Int/1 mod Int/2`.
	int64_t IntegerA = ml_integer32_value(Args[0]);
	int64_t IntegerB = ml_integer32_value(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	return ml_integer(IntegerA % IntegerB);
}

ML_METHOD("|", MLInteger32T, MLInteger32T) {
//<Int/1
//<Int/2
//>integer
// Returns :mini:`Int/2` if it is divisible by :mini:`Int/1` and :mini:`nil` otherwise.
	int64_t IntegerA = ml_integer32_value(Args[0]);
	int64_t IntegerB = ml_integer32_value(Args[1]);
	if (!IntegerA) return ml_error("ValueError", "Division by 0");
	return (IntegerB % IntegerA) ? MLNil : Args[1];
}

ML_METHOD("!|", MLInteger32T, MLInteger32T) {
//<Int/1
//<Int/2
//>integer
// Returns :mini:`Int/2` if it is not divisible by :mini:`Int/1` and :mini:`nil` otherwise.
	int64_t IntegerA = ml_integer32_value(Args[0]);
	int64_t IntegerB = ml_integer32_value(Args[1]);
	if (!IntegerA) return ml_error("ValueError", "Division by 0");
	return (IntegerB % IntegerA) ? Args[1] : MLNil;
}

ML_METHOD("div", MLInteger32T, MLInteger32T) {
//<Int/1
//<Int/2
//>integer
// Returns the quotient of :mini:`Int/1` divided by :mini:`Int/2`.
// The result is calculated by rounding down in all cases.
	int64_t IntegerA = ml_integer32_value(Args[0]);
	int64_t IntegerB = ml_integer32_value(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	ldiv_t Div = ldiv(IntegerA, IntegerB);
	if (Div.rem < 0) {
		if (IntegerB < 0) {
			return ml_integer(Div.quot + 1);
		} else {
			return ml_integer(Div.quot - 1);
		}
	} else {
		return ml_integer(Div.quot);
	}
}

ML_METHOD("mod", MLInteger32T, MLInteger32T) {
//<Int/1
//<Int/2
//>integer
// Returns the remainder of :mini:`Int/1` divided by :mini:`Int/2`.
// Note: the result is calculated by rounding down in all cases. In particular, the result is always nonnegative.
	int64_t IntegerA = ml_integer32_value(Args[0]);
	int64_t IntegerB = ml_integer32_value(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	long A = IntegerA;
	long B = IntegerB;
	long R = A % B;
	if (R < 0) R += labs(B);
	return ml_integer(R);
}

ML_METHOD("bsf", MLInteger32T) {
//<A
//>integer
// Returns the index of the least significant 1-bit of :mini:`A`, or :mini:`0` if :mini:`A = 0`.
//$= 16:bsf
//$= 10:bsf
//$= 0:bsf
	uint64_t A = (uint64_t)ml_integer32_value(Args[0]);
	return ml_integer(__builtin_ffsl(A));
}

ML_METHOD("bsr", MLInteger32T) {
//<A
//>integer
// Returns the index of the most significant 1-bit of :mini:`A`, or :mini:`0` if :mini:`A = 0`.
//$= 16:bsr
//$= 10:bsr
//$= 0:bsr
	uint64_t A = (uint64_t)ml_integer32_value(Args[0]);
	return ml_integer(A ? 64 - __builtin_clzl(A) : 0);
}

#endif

ML_METHOD("/", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer | real
// Returns :mini:`Int/1 / Int/2` as an integer if the division is exact, otherwise as a real.
//$= let N := 10 / 2
//$= type(N)
//$= let R := 10 / 3
//$= type(R)
#ifdef ML_BIGINT
	mpq_t Quotient;
	ml_integer_mpz_init(mpq_denref(Quotient), Args[1]);
	if (!mpq_denref(Quotient)->_mp_size) return ml_error("ValueError", "Division by 0");
	ml_integer_mpz_init(mpq_numref(Quotient), Args[0]);
	mpq_canonicalize(Quotient);
	if (!mpz_cmp_ui(mpq_denref(Quotient), 1)) {
		return ml_integer_mpz(mpq_numref(Quotient));
	} else {
		return ml_real(mpq_get_d(Quotient));
	}
#else
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	if (IntegerA % IntegerB == 0) {
		return ml_integer(IntegerA / IntegerB);
	} else {
		return ml_real((double)IntegerA / (double)IntegerB);
	}
#endif
}

ML_METHOD("%", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer
// Returns the remainder of :mini:`Int/1` divided by :mini:`Int/2`.
// Note: the result is calculated by rounding towards 0. In particular, if :mini:`Int/1` is negative, the result will be negative.
// For a nonnegative remainder, use :mini:`Int/1 mod Int/2`.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	return ml_integer(IntegerA % IntegerB);
}

ML_METHOD("|", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer
// Returns :mini:`Int/2` if it is divisible by :mini:`Int/1` and :mini:`nil` otherwise.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	if (!IntegerA) return ml_error("ValueError", "Division by 0");
	return (IntegerB % IntegerA) ? MLNil : Args[1];
}

ML_METHOD("!|", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer
// Returns :mini:`Int/2` if it is not divisible by :mini:`Int/1` and :mini:`nil` otherwise.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	if (!IntegerA) return ml_error("ValueError", "Division by 0");
	return (IntegerB % IntegerA) ? Args[1] : MLNil;
}

ML_METHOD("div", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer
// Returns the quotient of :mini:`Int/1` divided by :mini:`Int/2`.
// The result is calculated by rounding down in all cases.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	ldiv_t Div = ldiv(IntegerA, IntegerB);
	if (Div.rem < 0) {
		if (IntegerB < 0) {
			return ml_integer(Div.quot + 1);
		} else {
			return ml_integer(Div.quot - 1);
		}
	} else {
		return ml_integer(Div.quot);
	}
}

ML_METHOD("mod", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer
// Returns the remainder of :mini:`Int/1` divided by :mini:`Int/2`.
// Note: the result is calculated by rounding down in all cases. In particular, the result is always nonnegative.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	long A = IntegerA;
	long B = IntegerB;
	long R = A % B;
	if (R < 0) R += labs(B);
	return ml_integer(R);
}

ML_METHOD("bsf", MLIntegerT) {
//<A
//>integer
// Returns the index of the least significant 1-bit of :mini:`A`, or :mini:`0` if :mini:`A = 0`.
//$= 16:bsf
//$= 10:bsf
//$= 0:bsf
	uint64_t A = (uint64_t)ml_integer_value(Args[0]);
	return ml_integer(__builtin_ffsl(A));
}

ML_METHOD("bsr", MLIntegerT) {
//<A
//>integer
// Returns the index of the most significant 1-bit of :mini:`A`, or :mini:`0` if :mini:`A = 0`.
//$= 16:bsr
//$= 10:bsr
//$= 0:bsr
	uint64_t A = (uint64_t)ml_integer_value(Args[0]);
	return ml_integer(A ? 64 - __builtin_clzl(A) : 0);
}

#ifdef ML_NANBOXING

#define ml_comp_method_integer32_integer32(NAME, FUNC) \
ML_METHOD(#NAME, MLInteger32T, MLInteger32T) { \
/*<A
//<B
//>integer
// Returns :mini:`B` if :mini:`A NAME B`, otherwise returns :mini:`nil`.
*/\
	int64_t IntegerA = ml_integer32_value(Args[0]); \
	int64_t IntegerB = ml_integer32_value(Args[1]); \
	return ml_ ## FUNC(IntegerA, IntegerB) ? Args[1] : MLNil; \
}

#else

#define ml_comp_method_integer32_integer32(NAME, FUNC)

#endif

#ifdef ML_BIGINT

#define ml_comp_method_integer_integer(NAME, FUNC) \
\
ml_comp_method_integer32_integer32(NAME, FUNC) \
\
ML_METHOD(#NAME, MLIntegerT, MLIntegerT) { \
/*<A
//<B
//>integer
// Returns :mini:`B` if :mini:`A NAME B`, otherwise returns :mini:`nil`.
*/\
	mpz_t IntegerA; ml_integer_mpz_init(IntegerA, Args[0]); \
	mpz_t IntegerB; ml_integer_mpz_init(IntegerB, Args[1]); \
	return ml_ ## FUNC(mpz_cmp(IntegerA, IntegerB), 0) ? Args[1] : MLNil; \
} \
\
ML_METHOD(#NAME, MLInteger64T, MLInteger64T) { \
/*<A
//<B
//>integer
// Returns :mini:`B` if :mini:`A NAME B`, otherwise returns :mini:`nil`.
*/\
	ml_integer_t *A = (ml_integer_t *)Args[0]; \
	ml_integer_t *B = (ml_integer_t *)Args[1]; \
	return ml_ ## FUNC(mpz_cmp(A->Value, B->Value), 0) ? Args[1] : MLNil; \
}

#else

#define ml_comp_method_integer_integer(NAME, FUNC) \
\
ml_comp_method_integer32_integer32(NAME, FUNC) \
\
ML_METHOD(#NAME, MLIntegerT, MLIntegerT) { \
/*<A
//<B
//>integer
// Returns :mini:`B` if :mini:`A NAME B`, otherwise returns :mini:`nil`.
*/\
	int64_t IntegerA = ml_integer_value(Args[0]); \
	int64_t IntegerB = ml_integer_value(Args[1]); \
	return ml_ ## FUNC(IntegerA, IntegerB) ? Args[1] : MLNil; \
}

#endif

#define ml_comp_method_real_real(NAME, FUNC) \
\
ML_METHOD(#NAME, MLDoubleT, MLDoubleT) { \
/*<A
//<B
//>real
// Returns :mini:`B` if :mini:`A NAME B`, otherwise returns :mini:`nil`.
*/\
	double RealA = ml_double_value(Args[0]); \
	double RealB = ml_double_value(Args[1]); \
	return ml_ ## FUNC(RealA, RealB) ? Args[1] : MLNil; \
} \
\
ML_METHOD(#NAME, MLRealT, MLRealT) { \
/*<A
//<B
//>real
// Returns :mini:`B` if :mini:`A NAME B`, otherwise returns :mini:`nil`.
*/\
	double RealA = ml_real_value(Args[0]); \
	double RealB = ml_real_value(Args[1]); \
	return ml_ ## FUNC(RealA, RealB) ? Args[1] : MLNil; \
}

#define ml_comp_method_number_number(NAME, FUNC) \
ml_comp_method_integer_integer(NAME, FUNC) \
ml_comp_method_real_real(NAME, FUNC)

ml_comp_method_number_number(=, eq)
ml_comp_method_number_number(!=, neq)
ml_comp_method_number_number(<, lt)
ml_comp_method_number_number(>, gt)
ml_comp_method_number_number(<=, lte)
ml_comp_method_number_number(>=, gte)

#define ml_select_method_integer_integer(NAME, FUNC) \
ML_METHOD(#NAME, MLIntegerT, MLIntegerT) { \
/*<A
//<B
//>integer
// Returns :mini:`NAME(A, B)`.
*/\
	int64_t IntegerA = ml_integer_value(Args[0]); \
	int64_t IntegerB = ml_integer_value(Args[1]); \
	return ml_ ## FUNC(IntegerA, IntegerB) ? Args[0] : Args[1]; \
}

#define ml_select_method_real_real(NAME, FUNC) \
ML_METHOD(#NAME, MLDoubleT, MLDoubleT) { \
/*<A
//<B
//>real
// Returns :mini:`NAME(A, B)`.
*/\
	double RealA = ml_double_value(Args[0]); \
	double RealB = ml_double_value(Args[1]); \
	return ml_ ## FUNC(RealA, RealB) ? Args[0] : Args[1]; \
}

#define ml_select_method_real_integer(NAME, FUNC) \
ML_METHOD(#NAME, MLDoubleT, MLIntegerT) { \
/*<A
//<B
//>real
// Returns :mini:`NAME(A, B)`.
*/\
	double RealA = ml_double_value(Args[0]); \
	int64_t IntegerB = ml_integer_value(Args[1]); \
	return ml_ ## FUNC(RealA, IntegerB) ? Args[0] : Args[1]; \
}

#define ml_select_method_integer_real(NAME, FUNC) \
ML_METHOD(#NAME, MLIntegerT, MLDoubleT) { \
/*<A
//<B
//>real
// Returns :mini:`NAME(A, B)`.
*/\
	int64_t IntegerA = ml_integer_value(Args[0]); \
	double RealB = ml_double_value(Args[1]); \
	return ml_ ## FUNC(IntegerA, RealB) ? Args[0] : Args[1]; \
}

#define ml_select_method_number_number(NAME, FUNC) \
ml_select_method_integer_integer(NAME, FUNC) \
ml_select_method_real_real(NAME, FUNC) \
ml_select_method_real_integer(NAME, FUNC) \
ml_select_method_integer_real(NAME, FUNC)

ml_select_method_number_number(min, lt);
ml_select_method_number_number(max, gt);

#ifndef ML_NANBOXING

#ifdef ML_BIGINT

ml_integer_t One[1] = {{MLIntegerT}};
ml_integer_t NegOne[1] = {{MLIntegerT}};
ml_integer_t Zero[1] = {{MLIntegerT}};

#else

ml_integer_t One[1] = {{MLIntegerT, 1}};
ml_integer_t NegOne[1] = {{MLIntegerT, -1}};
ml_integer_t Zero[1] = {{MLIntegerT, 0}};

#endif

#endif

ML_METHOD("<>", MLIntegerT, MLIntegerT) {
//<Int/1
//<Int/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int/1` is less than, equal to or greater than :mini:`Int/2`.
#ifdef ML_BIGINT
	mpz_t IntegerA; ml_integer_mpz_init(IntegerA, Args[0]);
	mpz_t IntegerB; ml_integer_mpz_init(IntegerB, Args[1]);
	int Cmp = mpz_cmp(IntegerA, IntegerB);
	if (Cmp < 0) return (ml_value_t *)NegOne;
	if (Cmp > 0) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
#else
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	if (IntegerA < IntegerB) return (ml_value_t *)NegOne;
	if (IntegerA > IntegerB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
#endif
}

#ifdef ML_NANBOXING

ML_METHOD("<>", MLInteger32T, MLInteger32T) {
//<Int/1
//<Int/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int/1` is less than, equal to or greater than :mini:`Int/2`.
	int64_t IntegerA = ml_integer32_value(Args[0]);
	int64_t IntegerB = ml_integer32_value(Args[1]);
	if (IntegerA < IntegerB) return (ml_value_t *)NegOne;
	if (IntegerA > IntegerB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLInteger64T, MLInteger32T) {
//<Int/1
//<Int/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int/1` is less than, equal to or greater than :mini:`Int/2`.
#ifdef ML_BIGINT
	ml_integer_t *A = (ml_integer_t *)Args[0];
	int64_t IntegerB = ml_integer32_value(Args[1]);
	int Cmp = mpz_cmp_si(A->Value, IntegerB);
	if (Cmp < 0) return (ml_value_t *)NegOne;
	if (Cmp > 0) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
#else
	ml_integer_t *A = (ml_integer_t *)Args[0];
	int64_t IntegerB = ml_integer32_value(Args[1]);
	if (A->Value < IntegerB) return (ml_value_t *)NegOne;
	if (A->Value > IntegerB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
#endif
}

ML_METHOD("<>", MLInteger32T, MLInteger64T) {
//<Int/1
//<Int/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int/1` is less than, equal to or greater than :mini:`Int/2`.
#ifdef ML_BIGINT
	int64_t IntegerA = ml_integer32_value(Args[0]);
	ml_integer_t *B = (ml_integer_t *)Args[1];
	int Cmp = mpz_cmp_ui(B->Value, IntegerA);
	if (Cmp > 0) return (ml_value_t *)NegOne;
	if (Cmp < 0) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
#else
	int64_t IntegerA = ml_integer32_value(Args[0]);
	ml_integer_t *B = (ml_integer_t *)Args[1];
	if (IntegerA < B->Value) return (ml_value_t *)NegOne;
	if (IntegerA > B->Value) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
#endif
}

#endif

#ifdef ML_BIGINT

ML_METHOD("<>", MLInteger64T, MLInteger64T) {
//<Int/1
//<Int/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int/1` is less than, equal to or greater than :mini:`Int/2`.
	ml_integer_t *A = (ml_integer_t *)Args[0];
	ml_integer_t *B = (ml_integer_t *)Args[1];
	int Cmp = mpz_cmp(A->Value, B->Value);
	if (Cmp < 0) return (ml_value_t *)NegOne;
	if (Cmp > 0) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

#endif

ML_METHOD("<>", MLDoubleT, MLIntegerT) {
//<Real/1
//<Int/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real/1` is less than, equal to or greater than :mini:`Int/2`.
	double RealA = ml_double_value(Args[0]);
	double RealB = ml_real_value(Args[1]);
	if (isnan(RealA)) return (ml_value_t *)NegOne;
	if (RealA < RealB) return (ml_value_t *)NegOne;
	if (RealA > RealB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLIntegerT, MLDoubleT) {
//<Int/1
//<Real/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int/1` is less than, equal to or greater than :mini:`Real/2`.
	double RealA = ml_real_value(Args[0]);
	double RealB = ml_double_value(Args[1]);
	if (isnan(RealB)) return (ml_value_t *)One;
	if (RealA < RealB) return (ml_value_t *)NegOne;
	if (RealA > RealB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLDoubleT, MLDoubleT) {
//<Real/1
//<Real/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real/1` is less than, equal to or greater than :mini:`Real/2`.
	double RealA = ml_double_value(Args[0]);
	double RealB = ml_double_value(Args[1]);
	if (isnan(RealA)) {
		if (isnan(RealB)) return (ml_value_t *)Zero;
		return (ml_value_t *)NegOne;
	} else if (isnan(RealB)) {
		return (ml_value_t *)One;
	}
	if (RealA < RealB) return (ml_value_t *)NegOne;
	if (RealA > RealB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("div", MLRealT, MLRealT) {
//<Real/1
//<Real/2
//>integer
// Returns the quotient of :mini:`Real/1` divided by :mini:`Real/2`.
// The result is calculated by rounding down in all cases.
	double RealA = ml_real_value(Args[0]);
	double RealB = ml_real_value(Args[1]);
	if (fabs(RealB) < DBL_EPSILON) return ml_error("ValueError", "Division by 0");
	return ml_integer(floor(RealA / RealB));
}

ML_METHOD("mod", MLRealT, MLRealT) {
//<Int/1
//<Int/2
//>integer
// Returns the remainder of :mini:`Int/1` divided by :mini:`Int/2`.
// Note: the result is calculated by rounding down in all cases. In particular, the result is always nonnegative.
	double RealA = ml_real_value(Args[0]);
	double RealB = ml_real_value(Args[1]);
	if (fabs(RealB) < DBL_EPSILON) return ml_error("ValueError", "Division by 0");
	return ml_real(RealA - floor(RealA / RealB) * RealB);
}

ML_METHOD("%", MLRealT, MLRealT) {
//<Real/1
//<Real/2
//>integer
// Returns the remainder of :mini:`Real/1` divided by :mini:`Real/2`.
// Note: the result is calculated by rounding towards 0. In particular, if :mini:`Real/1` is negative, the result will be negative.
// For a nonnegative remainder, use :mini:`Real/1 mod Real/2`.
	double RealA = ml_real_value(Args[0]);
	double RealB = ml_real_value(Args[1]);
	if (fabs(RealB) < DBL_EPSILON) return ml_error("ValueError", "Division by 0");
	double Q = RealA / RealB;
	double D = Q < 0 ? ceil(Q) : floor(Q);
	return ml_real(RealA - D * RealB);
}

ML_METHOD("isfinite", MLDoubleT) {
//<Number:number
//>number|nil
// Returns :mini:`Number` if it is finite (neither |plusmn|\ |infin| nor ``NaN``), otherwise returns :mini:`nil`.
	double X = ml_double_value(Args[0]);
	if (!isfinite(X)) return MLNil;
	return Args[0];
}

ML_METHOD("isnan", MLDoubleT) {
//<Number:number
//>number|nil
// Returns :mini:`Number` if it is ``NaN``, otherwise returns :mini:`Number`.
	double X = ml_double_value(Args[0]);
	if (!isnan(X)) return MLNil;
	return Args[0];
}

ML_FUNCTION(RandomInteger) {
//@integer::random
//<Min?:number
//<Max?:number
//>integer
// Returns a random integer between :mini:`Min` and :mini:`Max` (where :mini:`Max` :math:`\leq 2^{32} - 1`).
// If omitted, :mini:`Min` defaults to :mini:`0` and :mini:`Max` defaults to :math:`2^{32} - 1`.
	if (Count == 2) {
		ML_CHECK_ARG_TYPE(0, MLRealT);
		ML_CHECK_ARG_TYPE(1, MLRealT);
		int Base = ml_integer_value(Args[0]);
		int Limit = ml_integer_value(Args[1]) + 1 - Base;
		if (Limit <= 0) return Args[0];
		int Divisor = RAND_MAX / Limit;
		int Random;
		do Random = random() / Divisor; while (Random >= Limit);
		return ml_integer(Base + Random);
	} else if (Count == 1) {
		ML_CHECK_ARG_TYPE(0, MLRealT);
		int Limit = ml_integer_value(Args[0]);
		if (Limit <= 0) return Args[0];
		int Divisor = RAND_MAX / Limit;
		int Random;
		do Random = random() / Divisor; while (Random >= Limit);
		return ml_integer(Random + 1);
	} else {
		return ml_integer(random());
	}
}

extern ml_cfunction_t RandomPermutation[1];
extern ml_cfunction_t RandomCycle[1];

#ifndef ML_MATH

ML_FUNCTION(RandomPermutation) {
//@integer::random_permutation
//<Max:number
//>list
// Returns a random permutation of :mini:`1, ..., Max`.
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	int Limit = ml_integer_value(Args[0]);
	if (Limit <= 0) return ml_error("ValueError", "Permutation requires positive size");
	int *Values = asnew(int, Limit);
	Values[0] = 1;
	for (int I = 2; I <= Limit; ++I) {
		int Divisor = RAND_MAX / I, J;
		do J = random() / Divisor; while (J >= I);
		if (J + 1 == I) {
			Values[I - 1] = I;
		} else {
			int Old = Values[J];
			Values[J] = I;
			Values[I - 1] = Old;
		}
	}
	ml_value_t *Permutation = ml_list();
	for (int I = 0; I < Limit; ++I) ml_list_put(Permutation, ml_integer(Values[I]));
	return Permutation;
}

ML_FUNCTION(RandomCycle) {
//@integer::random_cycle
//<Max:number
//>list
// Returns a random cyclic permutation (no sub-cycles) of :mini:`1, ..., Max`.
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	int Limit = ml_integer_value(Args[0]);
	if (Limit <= 0) return ml_error("ValueError", "Permutation requires positive size");
	if (Limit == 1) {
		ml_value_t *Permutation = ml_list();
		ml_list_put(Permutation, ml_integer(1));
		return Permutation;
	}
	int *Values = asnew(int, Limit);
	Values[0] = 2;
	Values[1] = 1;
	for (int I = 2; I < Limit; ++I) {
		int Divisor = RAND_MAX / I, J;
		do J = random() / Divisor; while (J >= I);
		int Old = Values[J];
		Values[J] = I + 1;
		Values[I] = Old;
	}
	ml_value_t *Permutation = ml_list();
	for (int I = 0; I < Limit; ++I) ml_list_put(Permutation, ml_integer(Values[I]));
	return Permutation;
}

#endif

ML_FUNCTION(RandomReal) {
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

typedef struct {
	const ml_type_t *Type;
	ml_value_t *Value;
	long Index;
} ml_constant_iter_t;

ML_TYPE(MLConstantIterT, (), "constant-iter");
//!internal

static void ML_TYPED_FN(ml_iter_value, MLConstantIterT, ml_state_t *Caller, ml_constant_iter_t *Iter) {
	ML_RETURN(Iter->Value);
}

static void ML_TYPED_FN(ml_iter_next, MLConstantIterT, ml_state_t *Caller, ml_constant_iter_t *Iter) {
	++Iter->Index;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLConstantIterT, ml_state_t *Caller, ml_constant_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

typedef struct {
	const ml_type_t *Type;
	long Current, Step, Limit;
	long Index;
} ml_integer_iter_t;

ML_TYPE(MLIntegerUpIterT, (), "integer-iter");
//!internal

static void ML_TYPED_FN(ml_iter_value, MLIntegerUpIterT, ml_state_t *Caller, ml_integer_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Current));
}

static void ML_TYPED_FN(ml_iter_next, MLIntegerUpIterT, ml_state_t *Caller, ml_integer_iter_t *Iter) {
	Iter->Current += Iter->Step;
	if (Iter->Current > Iter->Limit) ML_RETURN(MLNil);
	++Iter->Index;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLIntegerUpIterT, ml_state_t *Caller, ml_integer_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

ML_TYPE(MLIntegerDownIterT, (), "integer-iter");
//!internal

static void ML_TYPED_FN(ml_iter_value, MLIntegerDownIterT, ml_state_t *Caller, ml_integer_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Current));
}

static void ML_TYPED_FN(ml_iter_next, MLIntegerDownIterT, ml_state_t *Caller, ml_integer_iter_t *Iter) {
	Iter->Current += Iter->Step;
	if (Iter->Current < Iter->Limit) ML_RETURN(MLNil);
	++Iter->Index;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLIntegerDownIterT, ml_state_t *Caller, ml_integer_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

ML_TYPE(MLIntegerRangeT, (MLSequenceT), "integer-range");
//!interval

static void ML_TYPED_FN(ml_iterate, MLIntegerRangeT, ml_state_t *Caller, ml_value_t *Value) {
	ml_integer_range_t *Range = (ml_integer_range_t *)Value;
	if (Range->Step > 0) {
		if (Range->Start > Range->Limit) ML_RETURN(MLNil);
		ml_integer_iter_t *Iter = new(ml_integer_iter_t);
		Iter->Type = MLIntegerUpIterT;
		Iter->Index = 1;
		Iter->Current = Range->Start;
		Iter->Limit = Range->Limit;
		Iter->Step = Range->Step;
		ML_RETURN(Iter);
	} else if (Range->Step < 0) {
		if (Range->Start < Range->Limit) ML_RETURN(MLNil);
		ml_integer_iter_t *Iter = new(ml_integer_iter_t);
		Iter->Type = MLIntegerDownIterT;
		Iter->Index = 1;
		Iter->Current = Range->Start;
		Iter->Limit = Range->Limit;
		Iter->Step = Range->Step;
		ML_RETURN(Iter);
	} else {
		ml_constant_iter_t *Iter = new(ml_constant_iter_t);
		Iter->Type = MLConstantIterT;
		Iter->Value = ml_integer(Range->Start);
		Iter->Index = 1;
		ML_RETURN(Iter);
	}
}

ML_TYPE(MLIntegerIntervalT, (MLSequenceT), "integer-interval");
//!interval

static void ML_TYPED_FN(ml_iterate, MLIntegerIntervalT, ml_state_t *Caller, ml_value_t *Value) {
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Value;
	if (Interval->Start > Interval->Limit) ML_RETURN(MLNil);
	ml_integer_iter_t *Iter = new(ml_integer_iter_t);
	Iter->Type = MLIntegerUpIterT;
	Iter->Index = 1;
	Iter->Current = Interval->Start;
	Iter->Limit = Interval->Limit;
	Iter->Step = 1;
	ML_RETURN(Iter);
}

ML_METHOD("..", MLIntegerT, MLIntegerT) {
//!interval
//<Start
//<Limit
//>integer::interval
// Returns a interval from :mini:`Start` to :mini:`Limit` (inclusive).
	ml_integer_interval_t *Interval = new(ml_integer_interval_t);
	Interval->Type = MLIntegerIntervalT;
	Interval->Start = ml_integer_value(Args[0]);
	Interval->Limit = ml_integer_value(Args[1]);
	return (ml_value_t *)Interval;
}

ML_METHOD("..<", MLIntegerT, MLIntegerT) {
//!interval
//<Start
//<Limit
//>integer::interval
// Returns a interval from :mini:`Start` to :mini:`Limit` (exclusive).
	ml_integer_interval_t *Interval = new(ml_integer_interval_t);
	Interval->Type = MLIntegerIntervalT;
	Interval->Start = ml_integer_value(Args[0]);
	Interval->Limit = ml_integer_value(Args[1]) - 1;
	return (ml_value_t *)Interval;
}

ML_METHOD("..", MLIntegerT, MLIntegerT, MLIntegerT) {
//!interval
//<Start
//<Limit
//<Step
//>integer::range
// Returns a range from :mini:`Start` to :mini:`Limit` (inclusive) with step :mini:`Step`.
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = ml_integer_value(Args[0]);
	Range->Limit = ml_integer_value(Args[1]);
	Range->Step = ml_integer_value(Args[2]);
	return (ml_value_t *)Range;
}

ML_METHOD("up", MLIntegerT) {
//!interval
//<Start
//>integer::interval
// Returns an unlimited interval from :mini:`Start`.
	ml_integer_interval_t *Interval = new(ml_integer_interval_t);
	Interval->Type = MLIntegerIntervalT;
	Interval->Start = ml_integer_value(Args[0]);
	Interval->Limit = LONG_MAX;
	return (ml_value_t *)Interval;
}

ML_METHOD("up", MLIntegerT, MLIntegerT) {
//!interval
//<Start
//<Count
//>integer::interval
// Returns an interval from :mini:`Start` to :mini:`Start + Count - 1` (inclusive).
	ml_integer_interval_t *Interval = new(ml_integer_interval_t);
	Interval->Type = MLIntegerIntervalT;
	Interval->Start = ml_integer_value(Args[0]);
	Interval->Limit = Interval->Start + ml_integer_value(Args[1]) - 1;
	return (ml_value_t *)Interval;
}

ML_METHOD("down", MLIntegerT) {
//!interval
//<Start
//>integer::range
// Returns an unlimited range from :mini:`Start`.
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = ml_integer_value(Args[0]);
	Range->Limit = LONG_MIN;
	Range->Step = -1;
	return (ml_value_t *)Range;
}

ML_METHOD("down", MLIntegerT, MLIntegerT) {
//!range
//<Start
//<Count
//>integer::range
// Returns a range from :mini:`Start` to :mini:`Start - Count + 1` (inclusive).
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = ml_integer_value(Args[0]);
	Range->Limit = Range->Start - ml_integer_value(Args[1]) + 1;
	Range->Step = -1;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerT, MLIntegerT) {
//!interval
//<Start
//<Step
//>integer::range
// Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = ml_integer_value(Args[0]);
	Range->Step = ml_integer_value(Args[1]);
	if (Range->Step < 0) {
		Range->Limit = LONG_MIN;
	} else if (Range->Step > 0) {
		Range->Limit = LONG_MAX;
	} else {
		Range->Limit = Range->Start;
	}
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerIntervalT, MLIntegerT) {
//!interval
//<Interval
//<Step
//>integer::range
// Returns a range with the same limits as :mini:`Interval` but with step :mini:`Step`.
	ml_integer_interval_t *Interval0 = (ml_integer_interval_t *)Args[0];
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = Interval0->Start;
	Range->Limit = Interval0->Limit;
	Range->Step = ml_integer_value(Args[1]);
	return (ml_value_t *)Range;
}

ML_METHOD("+", MLIntegerRangeT, MLIntegerT) {
//!range
//<Range
//<Shift
//>integer::range
// Returns a range
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	int64_t Shift = ml_integer_value(Args[1]);
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = Range0->Start;
	Range->Limit = Range0->Limit + Shift;
	Range->Step = Range0->Step;
	return (ml_value_t *)Range;
}

ML_METHOD("-", MLIntegerRangeT, MLIntegerT) {
//!range
//<Range
//<Shift
//>integer::range
// Returns a range
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	int64_t Shift = ml_integer_value(Args[1]);
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = Range0->Start;
	Range->Limit = Range0->Limit - Shift;
	Range->Step = Range0->Step;
	return (ml_value_t *)Range;
}

ML_METHOD("*", MLIntegerRangeT, MLIntegerT) {
//!range
//<Range
//<Scale
//>integer::range
// Returns a range
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	int64_t Scale = ml_integer_value(Args[1]);
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = Range0->Start * Scale;
	Range->Limit = Range0->Limit * Scale;
	Range->Step = Range0->Step * Scale;
	return (ml_value_t *)Range;
}

ML_METHOD("+", MLIntegerT, MLIntegerRangeT) {
//!range
//<Shift
//<Range
//>integer::range
// Returns a range
	int64_t Shift = ml_integer_value(Args[0]);
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[1];
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = Shift + Range0->Start;
	Range->Limit = Range0->Limit;
	Range->Step = Range0->Step;
	return (ml_value_t *)Range;
}

ML_METHOD("*", MLIntegerT, MLIntegerRangeT) {
//!range
//<Scale
//<Range
//>integer::range
// Returns a range
	int64_t Scale = ml_integer_value(Args[0]);
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[1];
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = Scale * Range0->Start;
	Range->Limit = Scale * Range0->Limit;
	Range->Step = Scale * Range0->Step;
	return (ml_value_t *)Range;
}

ML_METHOD("=", MLIntegerRangeT, MLIntegerRangeT) {
//!range
//<A
//<B
//>integer::range|nil
// Returns a range
	ml_integer_range_t *A = (ml_integer_range_t *)Args[0];
	ml_integer_range_t *B = (ml_integer_range_t *)Args[1];
	if (A->Start != B->Start) return MLNil;
	if (A->Limit != B->Limit) return MLNil;
	if (A->Step != B->Step) return MLNil;
	return (ml_value_t *)B;
}

ML_METHOD("!=", MLIntegerRangeT, MLIntegerRangeT) {
//!range
//<A
//<B
//>integer::range|nil
// Returns a range
	ml_integer_range_t *A = (ml_integer_range_t *)Args[0];
	ml_integer_range_t *B = (ml_integer_range_t *)Args[1];
	if (A->Start != B->Start) return (ml_value_t *)B;
	if (A->Limit != B->Limit) return (ml_value_t *)B;
	if (A->Step != B->Step) return (ml_value_t *)B;
	return MLNil;
}

ML_METHOD("precount", MLIntegerRangeT) {
//!range
//<Range
//>integer
// Returns the number of values in :mini:`Range`.
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	if (Range->Step > 0) {
		if (Range->Start > Range->Limit) return ml_integer(0);
		int64_t Diff = Range->Limit - Range->Start;
		return ml_integer(Diff / Range->Step + 1);
	} else if (Range->Step < 0) {
		if (Range->Start < Range->Limit) return ml_integer(0);
		int64_t Diff = Range->Limit - Range->Start;
		return ml_integer(Diff / Range->Step + 1);
	} else {
		return ml_real(INFINITY);
	}
}

ML_METHOD("count", MLIntegerRangeT) {
//!range
//<Range
//>integer
// Returns the number of values in :mini:`Range`.
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	if (Range->Step > 0) {
		if (Range->Start > Range->Limit) return ml_integer(0);
		int64_t Diff = Range->Limit - Range->Start;
		return ml_integer(Diff / Range->Step + 1);
	} else if (Range->Step < 0) {
		if (Range->Start < Range->Limit) return ml_integer(0);
		int64_t Diff = Range->Limit - Range->Start;
		return ml_integer(Diff / Range->Step + 1);
	} else {
		return ml_real(INFINITY);
	}
}

ML_METHOD("start", MLIntegerRangeT) {
//!range
//<Range
//>integer
// Returns the start of :mini:`Range`.
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	return ml_integer(Range->Start);
}

ML_METHOD("limit", MLIntegerRangeT) {
//!range
//<Range
//>integer
// Returns the limit of :mini:`Range`.
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	return ml_integer(Range->Limit);
}

ML_METHOD("step", MLIntegerRangeT) {
//!range
//<Range
//>integer
// Returns the limit of :mini:`Range`.
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	return ml_integer(Range->Step);
}

ML_METHOD("first", MLIntegerRangeT) {
//!range
//<Range
//>integer
// Returns the start of :mini:`Range`.
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	int64_t Diff = Range->Limit - Range->Start;
	if (!Range->Step) {
		return ml_integer(Range->Start);
	} else if (Diff < 0 && Range->Step > 0) {
		return MLNil;
	} else if (Diff > 0 && Range->Step < 0) {
		return MLNil;
	} else {
		return ml_integer(Range->Start);
	}
}

ML_METHOD("last", MLIntegerRangeT) {
//!range
//<Range
//>integer
// Returns the limit of :mini:`Range`.
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	int64_t Diff = Range->Limit - Range->Start;
	if (!Range->Step) {
		return ml_integer(Range->Start);
	} else if (Diff < 0 && Range->Step > 0) {
		return MLNil;
	} else if (Diff > 0 && Range->Step < 0) {
		return MLNil;
	} else {
		return ml_integer(Range->Start + (Diff / Range->Step) * Range->Step);
	}
}

ML_METHOD("find", MLIntegerRangeT, MLIntegerT) {
//!range
//<Range
//<X
//>integer | nil
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	long Value = ml_integer_value(Args[1]);
	if (Range->Step < 0) {
		if (Value > Range->Start) return MLNil;
		if (Value < Range->Limit) return MLNil;
	} else if (Range->Step > 0) {
		if (Value < Range->Start) return MLNil;
		if (Value > Range->Limit) return MLNil;
	} else {
		return Value == Range->Start ? ml_integer(1) : MLNil;
	}
	long Diff = Value - Range->Start;
	if (Diff % Range->Step) return MLNil;
	return ml_integer(Diff / Range->Step + 1);
}

ML_METHOD("random", MLIntegerRangeT) {
//!range
//<Range
//>integer
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	if (!Range->Step) return ml_integer(Range->Start);
	int64_t Diff = Range->Limit - Range->Start;
	if (Diff < 0 && Range->Step > 0) return ml_integer(Range->Start);
	if (Diff > 0 && Range->Step < 0) return ml_integer(Range->Start);
	int Limit = Diff / Range->Step + 1;
	int Divisor = RAND_MAX / Limit;
	int Random;
	do Random = random() / Divisor; while (Random >= Limit);
	return ml_integer(Range->Start + Random * Range->Step);
}

ML_METHOD("+", MLIntegerIntervalT, MLIntegerT) {
//!interval
//<Interval
//<Shift
//>integer::interval
// Returns a interval
	ml_integer_interval_t *Interval0 = (ml_integer_interval_t *)Args[0];
	int64_t Shift = ml_integer_value(Args[1]);
	ml_integer_interval_t *Interval = new(ml_integer_interval_t);
	Interval->Type = MLIntegerIntervalT;
	Interval->Start = Interval0->Start;
	Interval->Limit = Interval0->Limit + Shift;
	return (ml_value_t *)Interval;
}

ML_METHOD("-", MLIntegerIntervalT, MLIntegerT) {
//!interval
//<Interval
//<Shift
//>integer::interval
// Returns a interval
	ml_integer_interval_t *Interval0 = (ml_integer_interval_t *)Args[0];
	int64_t Shift = ml_integer_value(Args[1]);
	ml_integer_interval_t *Interval = new(ml_integer_interval_t);
	Interval->Type = MLIntegerIntervalT;
	Interval->Start = Interval0->Start;
	Interval->Limit = Interval0->Limit - Shift;
	return (ml_value_t *)Interval;
}

ML_METHOD("*", MLIntegerIntervalT, MLIntegerT) {
//!interval
//<Interval
//<Scale
//>integer::interval
// Returns a interval
	ml_integer_interval_t *Interval0 = (ml_integer_interval_t *)Args[0];
	int64_t Scale = ml_integer_value(Args[1]);
	ml_integer_interval_t *Interval = new(ml_integer_interval_t);
	Interval->Type = MLIntegerIntervalT;
	Interval->Start = Interval0->Start * Scale;
	Interval->Limit = Interval0->Limit * Scale;
	return (ml_value_t *)Interval;
}

ML_METHOD("+", MLIntegerT, MLIntegerIntervalT) {
//!interval
//<Shift
//<Interval
//>integer::interval
// Returns a interval
	int64_t Shift = ml_integer_value(Args[0]);
	ml_integer_interval_t *Interval0 = (ml_integer_interval_t *)Args[1];
	ml_integer_interval_t *Interval = new(ml_integer_interval_t);
	Interval->Type = MLIntegerIntervalT;
	Interval->Start = Shift + Interval0->Start;
	Interval->Limit = Interval0->Limit;
	return (ml_value_t *)Interval;
}

ML_METHOD("*", MLIntegerT, MLIntegerIntervalT) {
//!interval
//<Scale
//<Interval
//>integer::interval
// Returns a interval
	int64_t Scale = ml_integer_value(Args[0]);
	ml_integer_interval_t *Interval0 = (ml_integer_interval_t *)Args[1];
	ml_integer_interval_t *Interval = new(ml_integer_interval_t);
	Interval->Type = MLIntegerIntervalT;
	Interval->Start = Scale * Interval0->Start;
	Interval->Limit = Scale * Interval0->Limit;
	return (ml_value_t *)Interval;
}

ML_METHOD("=", MLIntegerIntervalT, MLIntegerIntervalT) {
//!interval
//<A
//<B
//>integer::interval|nil
// Returns a interval
	ml_integer_interval_t *A = (ml_integer_interval_t *)Args[0];
	ml_integer_interval_t *B = (ml_integer_interval_t *)Args[1];
	if (A->Start != B->Start) return MLNil;
	if (A->Limit != B->Limit) return MLNil;
	return (ml_value_t *)B;
}

ML_METHOD("!=", MLIntegerIntervalT, MLIntegerIntervalT) {
//!interval
//<A
//<B
//>integer::interval|nil
// Returns a interval
	ml_integer_interval_t *A = (ml_integer_interval_t *)Args[0];
	ml_integer_interval_t *B = (ml_integer_interval_t *)Args[1];
	if (A->Start != B->Start) return (ml_value_t *)B;
	if (A->Limit != B->Limit) return (ml_value_t *)B;
	return MLNil;
}

ML_METHOD("precount", MLIntegerIntervalT) {
//!interval
//<Interval
//>integer
// Returns the number of values in :mini:`Interval`.
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Args[0];
	if (Interval->Start > Interval->Limit) return ml_integer(0);
	int64_t Diff = Interval->Limit - Interval->Start;
	return ml_integer(Diff + 1);
}

ML_METHOD("count", MLIntegerIntervalT) {
//!interval
//<Interval
//>integer
// Returns the number of values in :mini:`Interval`.
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Args[0];
	if (Interval->Start > Interval->Limit) return ml_integer(0);
	int64_t Diff = Interval->Limit - Interval->Start;
	return ml_integer(Diff + 1);
}

ML_METHOD("start", MLIntegerIntervalT) {
//!interval
//<Interval
//>integer
// Returns the start of :mini:`Interval`.
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Args[0];
	return ml_integer(Interval->Start);
}

ML_METHOD("limit", MLIntegerIntervalT) {
//!interval
//<Interval
//>integer
// Returns the limit of :mini:`Interval`.
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Args[0];
	return ml_integer(Interval->Limit);
}

ML_METHOD("first", MLIntegerIntervalT) {
//!interval
//<Interval
//>integer
// Returns the start of :mini:`Interval`.
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Args[0];
	return ml_integer(Interval->Start);
}

ML_METHOD("last", MLIntegerIntervalT) {
//!interval
//<Interval
//>integer
// Returns the limit of :mini:`Interval`.
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Args[0];
	return ml_integer(Interval->Limit);
}

ML_METHOD("between", MLIntegerT, MLIntegerIntervalT) {
//!interval
//<X
//<Interval
//>X | nil
	long Value = ml_integer_value(Args[0]);
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Args[1];
	if (Value < Interval->Start) return MLNil;
	if (Value > Interval->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("between", MLDoubleT, MLIntegerIntervalT) {
//!interval
//<X
//<Interval
//>X | nil
	double Value = ml_double_value(Args[0]);
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Args[1];
	if (Value < Interval->Start) return MLNil;
	if (Value > Interval->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("random", MLIntegerIntervalT) {
//!interval
//<Interval
//>integer
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Args[0];
	int64_t Diff = Interval->Limit - Interval->Start;
	int Limit = Diff + 1;
	int Divisor = RAND_MAX / Limit;
	int Random;
	do Random = random() / Divisor; while (Random >= Limit);
	return ml_integer(Interval->Start + Random);
}

typedef struct {
	ml_type_t *Type;
	int Z, S, M, N, K;
	int Indices[];
} ml_rangeset_iter_t;

ML_TYPE(MLRangesetIterT, (), "rangeset_iter");
//!internal

static void ML_TYPED_FN(ml_iter_next, MLRangesetIterT, ml_state_t *Caller, ml_rangeset_iter_t *Iter) {
	int *Indices = Iter->Indices;
	int M = Iter->M, I = M - 1, N = Iter->N;
	while (I >= 0) {
		if (Indices[I] + (M - I) < N) {
			int J = Indices[I] + 1;
			do { Indices[I] = J; ++J; ++I; } while (I < M);
			++Iter->K;
			ML_RETURN(Iter);
		}
		--I;
	}
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLRangesetIterT, ml_state_t *Caller, ml_rangeset_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->K));
}

static void ML_TYPED_FN(ml_iter_value, MLRangesetIterT, ml_state_t *Caller, ml_rangeset_iter_t *Iter) {
	ml_value_t *Set = ml_set();
	for (int I = 0; I < Iter->M; ++I) {
		ml_set_insert(Set, ml_integer(Iter->Z + Iter->S * Iter->Indices[I]));
	}
	ML_RETURN(Set);
}

typedef struct {
	ml_type_t *Type;
	int Z, S, M, N;
} ml_rangesets_t;

ML_TYPE(MLRangesetsT, (MLSequenceT), "rangesets");
//!internal

static void ML_TYPED_FN(ml_iterate, MLRangesetsT, ml_state_t *Caller, ml_rangesets_t *Rangesets) {
	int M = Rangesets->M;
	int N = Rangesets->N;
	if (M > N) ML_RETURN(MLNil);
	ml_rangeset_iter_t *Iter = xnew(ml_rangeset_iter_t, M, int);
	Iter->Type = MLRangesetIterT;
	Iter->Z = Rangesets->Z;
	Iter->S = Rangesets->S;
	for (int I = 0; I < M; ++I) Iter->Indices[I] = I;
	Iter->M = M;
	Iter->N = N;
	Iter->K = 1;
	ML_RETURN(Iter);
}

ML_METHOD("subsets", MLIntegerRangeT, MLIntegerT) {
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	int M = ml_integer_value(Args[1]);
	if (M < 0) return ml_error("RangeError", "Rangeset size must be non-negative");
	ml_rangesets_t *Rangesets = new(ml_rangesets_t);
	Rangesets->Type = MLRangesetsT;
	Rangesets->Z = Range->Start;
	Rangesets->S = Range->Step;
	Rangesets->N = (Range->Limit - Range->Start) / Range->Step + 1;
	Rangesets->M = M;
	return (ml_value_t *)Rangesets;
}

ML_METHOD("subsets", MLIntegerIntervalT, MLIntegerT) {
	ml_integer_interval_t *Range = (ml_integer_interval_t *)Args[0];
	int M = ml_integer_value(Args[1]);
	if (M < 0) return ml_error("RangeError", "Rangeset size must be non-negative");
	ml_rangesets_t *Rangesets = new(ml_rangesets_t);
	Rangesets->Type = MLRangesetsT;
	Rangesets->Z = Range->Start;
	Rangesets->S = 1;
	Rangesets->N = Range->Limit - Range->Start + 1;
	Rangesets->M = M;
	return (ml_value_t *)Rangesets;
}

typedef struct ml_real_iter_t {
	const ml_type_t *Type;
	double Current, Step, Limit;
	long Index;
} ml_real_iter_t;

ML_TYPE(MLRealUpIterT, (), "real-iter");
//!internal

static void ML_TYPED_FN(ml_iter_value, MLRealUpIterT, ml_state_t *Caller, ml_real_iter_t *Iter) {
	ML_RETURN(ml_real(Iter->Current));
}

static void ML_TYPED_FN(ml_iter_next, MLRealUpIterT, ml_state_t *Caller, ml_real_iter_t *Iter) {
	Iter->Current += Iter->Step;
	if (Iter->Current > Iter->Limit) ML_RETURN(MLNil);
	++Iter->Index;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLRealUpIterT, ml_state_t *Caller, ml_real_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

ML_TYPE(MLRealDownIterT, (), "real-iter");
//!internal

static void ML_TYPED_FN(ml_iter_value, MLRealDownIterT, ml_state_t *Caller, ml_real_iter_t *Iter) {
	ML_RETURN(ml_real(Iter->Current));
}

static void ML_TYPED_FN(ml_iter_next, MLRealDownIterT, ml_state_t *Caller, ml_real_iter_t *Iter) {
	Iter->Current += Iter->Step;
	if (Iter->Current < Iter->Limit) ML_RETURN(MLNil);
	++Iter->Index;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLRealDownIterT, ml_state_t *Caller, ml_real_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

ML_TYPE(MLRealRangeT, (MLSequenceT), "real-range");
//!interval

static void ML_TYPED_FN(ml_iterate, MLRealRangeT, ml_state_t *Caller, ml_value_t *Value) {
	ml_real_range_t *Range = (ml_real_range_t *)Value;
	if (Range->Step > 0) {
		if (Range->Start > Range->Limit) ML_RETURN(MLNil);
		ml_real_iter_t *Iter = new(ml_real_iter_t);
		Iter->Type = MLRealUpIterT;
		Iter->Index = 1;
		Iter->Current = Range->Start;
		Iter->Limit = Range->Limit;
		Iter->Step = Range->Step;
		ML_RETURN(Iter);
	} else if (Range->Step < 0) {
		if (Range->Start < Range->Limit) ML_RETURN(MLNil);
		ml_real_iter_t *Iter = new(ml_real_iter_t);
		Iter->Type = MLRealDownIterT;
		Iter->Index = 1;
		Iter->Current = Range->Start;
		Iter->Limit = Range->Limit;
		Iter->Step = Range->Step;
		ML_RETURN(Iter);
	} else {
		ml_constant_iter_t *Iter = new(ml_constant_iter_t);
		Iter->Type = MLConstantIterT;
		Iter->Value = ml_real(Range->Start);
		Iter->Index = 1;
		ML_RETURN(Iter);
	}
}

ML_TYPE(MLRealIntervalT, (MLSequenceT), "real-interval");
//!interval

static void ML_TYPED_FN(ml_iterate, MLRealIntervalT, ml_state_t *Caller, ml_value_t *Value) {
	ml_real_interval_t *Interval = (ml_real_interval_t *)Value;
	if (Interval->Start > Interval->Limit) ML_RETURN(MLNil);
	ml_real_iter_t *Iter = new(ml_real_iter_t);
	Iter->Type = MLRealUpIterT;
	Iter->Index = 1;
	Iter->Current = Interval->Start;
	Iter->Limit = Interval->Limit;
	Iter->Step = 1;
	ML_RETURN(Iter);
}

ML_METHOD("..", MLNumberT, MLNumberT) {
//!interval
//<Start
//<Limit
//>real::interval
	ml_real_interval_t *Interval = new(ml_real_interval_t);
	Interval->Type = MLRealIntervalT;
	Interval->Start = ml_real_value(Args[0]);
	Interval->Limit = ml_real_value(Args[1]);
	return (ml_value_t *)Interval;
}

ML_METHOD("..", MLNumberT, MLNumberT, MLNumberT) {
//!range
//<Start
//<Limit
//>real::range
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = ml_real_value(Args[0]);
	Range->Limit = ml_real_value(Args[1]);
	Range->Step = ml_real_value(Args[2]);
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLNumberT, MLNumberT) {
//!range
//<Start
//<Step
//>real::range
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = ml_real_value(Args[0]);
	Range->Step = ml_real_value(Args[1]);
	Range->Limit = Range->Step > 0.0 ? INFINITY : -INFINITY;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLRealIntervalT, MLNumberT) {
//!interval
//<Interval
//<Step
//>real::range
	ml_real_interval_t *Interval0 = (ml_real_interval_t *)Args[0];
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = Interval0->Start;
	Range->Limit = Interval0->Limit;
	Range->Step = ml_real_value(Args[1]);
	return (ml_value_t *)Range;
}

ML_METHOD("in", MLIntegerIntervalT, MLIntegerT) {
//!interval
//<Interval
//<Count
//>integer::range|real::range
	ml_integer_interval_t *Interval0 = (ml_integer_interval_t *)Args[0];
	long C = ml_integer_value(Args[1]);
	if (C <= 0) return ml_error("IntervalError", "Invalid step count");
	if ((Interval0->Limit - Interval0->Start) % C) {
		ml_real_range_t *Range = new(ml_real_range_t);
		Range->Type = MLRealRangeT;
		Range->Start = Interval0->Start;
		Range->Limit = Interval0->Limit;
		Range->Step = (Range->Limit - Range->Start) / C;
		return (ml_value_t *)Range;
	} else {
		ml_integer_range_t *Range = new(ml_integer_range_t);
		Range->Type = MLIntegerRangeT;
		Range->Start = Interval0->Start;
		Range->Limit = Interval0->Limit;
		Range->Step = (Range->Limit - Range->Start) / C;
		return (ml_value_t *)Range;
	}
}

ML_METHOD("in", MLRealIntervalT, MLIntegerT) {
//!interval
//<Interval
//<Count
//>real::range
	ml_real_interval_t *Interval0 = (ml_real_interval_t *)Args[0];
	long C = ml_integer_value(Args[1]);
	if (C <= 0) return ml_error("IntervalError", "Invalid step count");
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = Interval0->Start;
	Range->Limit = Interval0->Limit;
	Range->Step = (Range->Limit - Range->Start) / C;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerIntervalT, MLDoubleT) {
//!interval
//<Interval
//<Step
//>real::range
	ml_integer_interval_t *Interval0 = (ml_integer_interval_t *)Args[0];
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = Interval0->Start;
	Range->Limit = Interval0->Limit;
	Range->Step = ml_double_value(Args[1]);
	return (ml_value_t *)Range;
}

ML_METHOD("clamp", MLIntegerT, MLIntegerIntervalT) {
//!interval
//<Value
//<Interval
//>integer
	int64_t Value = ml_integer_value(Args[0]);
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Args[1];
	if (Value < Interval->Start) return ml_integer(Interval->Start);
	if (Value > Interval->Limit) return ml_integer(Interval->Limit);
	return ml_integer(Value);
}

ML_METHOD("clamp", MLRealT, MLIntegerIntervalT) {
//!interval
//<Value
//<Interval
//>integer
	double Value = ml_real_value(Args[0]);
	ml_integer_interval_t *Interval = (ml_integer_interval_t *)Args[1];
	if (Value < Interval->Start) return ml_integer(Interval->Start);
	if (Value > Interval->Limit) return ml_integer(Interval->Limit);
	return ml_real(Value);
}

ML_METHOD("clamp", MLIntegerT, MLRealIntervalT) {
//!interval
//<Value
//<Interval
//>real
	int64_t Value = ml_integer_value(Args[0]);
	ml_real_interval_t *Interval = (ml_real_interval_t *)Args[1];
	if (Value < Interval->Start) return ml_real(Interval->Start);
	if (Value > Interval->Limit) return ml_real(Interval->Limit);
	return ml_integer(Value);
}

ML_METHOD("clamp", MLRealT, MLRealIntervalT) {
//!interval
//<Value
//<Interval
//>integer
	double Value = ml_real_value(Args[0]);
	ml_real_interval_t *Interval = (ml_real_interval_t *)Args[1];
	if (Value < Interval->Start) return ml_real(Interval->Start);
	if (Value > Interval->Limit) return ml_real(Interval->Limit);
	return ml_real(Value);
}

ML_METHOD("bin", MLIntegerRangeT, MLIntegerT) {
//!interval
//<Interval
//<Value
//>integer | nil
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	int64_t Value = ml_integer_value(Args[1]);
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return ml_integer((Value - Range->Start) / Range->Step + 1);
}

ML_METHOD("bin", MLIntegerRangeT, MLDoubleT) {
//!interval
//<Interval
//<Value
//>integer | nil
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	double Value = ml_real_value(Args[1]);
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return ml_integer(floor((Value - Range->Start) / Range->Step) + 1);
}

size_t ml_real_range_count(ml_real_range_t *Range) {
	if (Range->Step > 0) {
		if (Range->Start > Range->Limit) return 0;
		double Diff = Range->Limit - Range->Start;
		return Diff / Range->Step + 1;
	} else if (Range->Step < 0) {
		if (Range->Start < Range->Limit) return 0;
		double Diff = Range->Limit - Range->Start;
		return Diff / Range->Step + 1;
	} else {
		return 0;
	}
}

size_t ml_real_interval_count(ml_real_interval_t *Interval) {
	if (Interval->Start > Interval->Limit) return 0;
	double Diff = Interval->Limit - Interval->Start;
	return Diff + 1;
}

ML_METHOD("precount", MLRealRangeT) {
//!range
//<Range
//>integer
// Returns the number of values in :mini:`Range`.
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	if (Range->Step > 0) {
		if (Range->Start > Range->Limit) return ml_integer(0);
		double Diff = Range->Limit - Range->Start;
		return ml_integer(Diff / Range->Step + 1);
	} else if (Range->Step < 0) {
		if (Range->Start < Range->Limit) return ml_integer(0);
		double Diff = Range->Limit - Range->Start;
		return ml_integer(Diff / Range->Step + 1);
	} else {
		return ml_real(INFINITY);
	}
}

ML_METHOD("count", MLRealRangeT) {
//!range
//<Range
//>integer
// Returns the number of values in :mini:`Range`.
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	if (Range->Step > 0) {
		if (Range->Start > Range->Limit) return ml_integer(0);
		double Diff = Range->Limit - Range->Start;
		return ml_integer(Diff / Range->Step + 1);
	} else if (Range->Step < 0) {
		if (Range->Start < Range->Limit) return ml_integer(0);
		double Diff = Range->Limit - Range->Start;
		return ml_integer(Diff / Range->Step + 1);
	} else {
		return ml_real(INFINITY);
	}
}

ML_METHOD("start", MLRealRangeT) {
//!range
//<Range
//>real
// Returns the start of :mini:`Range`.
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	return ml_real(Range->Start);
}

ML_METHOD("limit", MLRealRangeT) {
//!range
//<Range
//>real
// Returns the limit of :mini:`Range`.
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	return ml_real(Range->Limit);
}

ML_METHOD("step", MLRealRangeT) {
//!range
//<Range
//>real
// Returns the step of :mini:`Range`.
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	return ml_real(Range->Step);
}

ML_METHOD("first", MLRealRangeT) {
//!range
//<Range
//>real
// Returns the start of :mini:`Range`.
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	if (Range->Step > 0) {
		if (Range->Start > Range->Limit) return MLNil;
		return ml_real(Range->Start);
	} else if (Range->Step < 0) {
		if (Range->Start < Range->Limit) return MLNil;
		return ml_real(Range->Start);
	} else {
		return ml_real(Range->Start);
	}
}

ML_METHOD("last", MLRealRangeT) {
//!range
//<Range
//>real
// Returns the limit of :mini:`Range`.
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	if (Range->Step > 0) {
		if (Range->Start > Range->Limit) return MLNil;
		double Diff = Range->Limit - Range->Start;
		return ml_real(Range->Start + floor(Diff / Range->Step) * Range->Step);
	} else if (Range->Step < 0) {
		if (Range->Start < Range->Limit) return MLNil;
		double Diff = Range->Limit - Range->Start;
		return ml_real(Range->Start + floor(Diff / Range->Step) * Range->Step);
	} else {
		return ml_real(Range->Start);
	}
}

ML_METHOD("precount", MLRealIntervalT) {
//!interval
//<Interval
//>integer
// Returns the number of values in :mini:`Interval`.
	ml_real_interval_t *Interval = (ml_real_interval_t *)Args[0];
	if (Interval->Start > Interval->Limit) return ml_integer(0);
	double Diff = Interval->Limit - Interval->Start;
	return ml_integer(Diff + 1);
}

ML_METHOD("count", MLRealIntervalT) {
//!interval
//<Interval
//>integer
// Returns the number of values in :mini:`Interval`.
	ml_real_interval_t *Interval = (ml_real_interval_t *)Args[0];
	if (Interval->Start > Interval->Limit) return ml_integer(0);
	double Diff = Interval->Limit - Interval->Start;
	return ml_integer(Diff + 1);
}

ML_METHOD("start", MLRealIntervalT) {
//!interval
//<Interval
//>real
// Returns the start of :mini:`Interval`.
	ml_real_interval_t *Interval = (ml_real_interval_t *)Args[0];
	return ml_real(Interval->Start);
}

ML_METHOD("limit", MLRealIntervalT) {
//!interval
//<Interval
//>real
// Returns the limit of :mini:`Interval`.
	ml_real_interval_t *Interval = (ml_real_interval_t *)Args[0];
	return ml_real(Interval->Limit);
}

ML_METHOD("first", MLRealIntervalT) {
//!interval
//<Interval
//>real
// Returns the start of :mini:`Interval`.
	ml_real_interval_t *Interval = (ml_real_interval_t *)Args[0];
	return ml_real(Interval->Start);
}

ML_METHOD("last", MLRealIntervalT) {
//!interval
//<Interval
//>real
// Returns the limit of :mini:`Interval`.
	ml_real_interval_t *Interval = (ml_real_interval_t *)Args[0];
	return ml_real(Interval->Start + floor(Interval->Limit - Interval->Start));
}

ML_METHOD("between", MLIntegerT, MLRealIntervalT) {
//!interval
//<X
//<Interval
//>X | nil
	long Value = ml_integer_value(Args[0]);
	ml_real_interval_t *Interval = (ml_real_interval_t *)Args[1];
	if (Value < Interval->Start) return MLNil;
	if (Value > Interval->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("between", MLDoubleT, MLRealIntervalT) {
//!interval
//<X
//<Interval
//>X | nil
	double Value = ml_double_value(Args[0]);
	ml_real_interval_t *Interval = (ml_real_interval_t *)Args[1];
	if (Value < Interval->Start) return MLNil;
	if (Value > Interval->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("random", MLRealRangeT) {
//!range
//<Range
//>real
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	int Limit;
	if (Range->Step > 0) {
		if (Range->Start > Range->Limit) return MLNil;
		double Diff = Range->Limit - Range->Start;
		Limit = floor(Diff / Range->Step);
	} else if (Range->Step < 0) {
		if (Range->Start < Range->Limit) return MLNil;
		double Diff = Range->Limit - Range->Start;
		Limit = floor(Diff / Range->Step);
	} else {
		return ml_real(Range->Start);
	}
	int Divisor = RAND_MAX / Limit;
	int Random;
	do Random = random() / Divisor; while (Random >= Limit);
	return ml_real(Range->Start + Random * Range->Step);
}

ML_METHOD("random", MLRealIntervalT) {
//!interval
//<Interval
//>real
	ml_real_interval_t *Interval = (ml_real_interval_t *)Args[0];
	int Limit;
	if (Interval->Start > Interval->Limit) return MLNil;
	double Diff = Interval->Limit - Interval->Start;
	Limit = floor(Diff);
	int Divisor = RAND_MAX / Limit;
	int Random;
	do Random = random() / Divisor; while (Random >= Limit);
	return ml_real(Interval->Start + Random);
}

ML_METHOD("bin", MLRealRangeT, MLIntegerT) {
//!range
//<Range
//<Value
//>integer | nil
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	int64_t Value = ml_integer_value(Args[1]);
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return ml_integer((Value - Range->Start) / Range->Step + 1);
}

ML_METHOD("bin", MLRealRangeT, MLDoubleT) {
//!range
//<Range
//<Value
//>integer | nil
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	double Value = ml_real_value(Args[1]);
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return ml_integer(floor((Value - Range->Start) / Range->Step) + 1);
}

// Switch Functions //

typedef struct {
	ml_value_t *Index;
	int64_t Min, Max;
} ml_integer_case_t;

typedef struct {
	ml_type_t *Type;
	ml_integer_case_t Cases[];
} ml_integer_switch_t;

static void ml_integer_switch(ml_state_t *Caller, ml_integer_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (!ml_is(Arg, MLNumberT)) {
		ML_ERROR("TypeError", "Expected number for argument 1");
	}
	int64_t Value = ml_integer_value(Arg);
	for (ml_integer_case_t *Case = Switch->Cases;; ++Case) {
		if (Case->Min <= Value && Value <= Case->Max) ML_RETURN(Case->Index);
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLIntegerSwitchT, (MLFunctionT), "integer-switch",
//!internal
	.call = (void *)ml_integer_switch
);

ML_FUNCTION_INLINE(MLIntegerSwitch) {
//!internal
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_integer_switch_t *Switch = xnew(ml_integer_switch_t, Total, ml_integer_case_t);
	Switch->Type = MLIntegerSwitchT;
	ml_integer_case_t *Case = Switch->Cases;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, MLIntegerT)) {
				Case->Min = Case->Max = ml_integer_value(Value);
			} else if (ml_is(Value, MLDoubleT)) {
				double Real = ml_real_value(Value), Int = floor(Real);
				if (Real != Int) return ml_error("ValueError", "Non-integer value in integer case");
				Case->Min = Case->Max = Int;
			} else if (ml_is(Value, MLIntegerIntervalT)) {
				ml_integer_interval_t *Interval = (ml_integer_interval_t *)Value;
				Case->Min = Interval->Start;
				Case->Max = Interval->Limit;
			} else if (ml_is(Value, MLRealIntervalT)) {
				ml_real_interval_t *Interval = (ml_real_interval_t *)Value;
				Case->Min = ceil(Interval->Start);
				Case->Max = floor(Interval->Limit);
			} else {
				return ml_error("ValueError", "Unsupported value in integer case");
			}
			Case->Index = ml_integer(I);
			++Case;
		}
	}
	Case->Min = LONG_MIN;
	Case->Max = LONG_MAX;
	Case->Index = ml_integer(Count);
	return (ml_value_t *)Switch;
}

static ml_value_t *ML_TYPED_FN(ml_serialize, MLIntegerSwitchT, ml_integer_switch_t *Switch) {
	ml_value_t *Result = ml_list();
	ml_list_put(Result, ml_cstring("integer-switch"));
	ml_value_t *Index = NULL, *Last = NULL;
	for (ml_integer_case_t *Case = Switch->Cases;; ++Case) {
		if (Case->Min > LONG_MIN && Case->Max < LONG_MAX) {
			if (Case->Index != Index) {
				Index = Case->Index;
				Last = ml_list();
				ml_list_put(Result, Last);
			}
			ml_value_t *Interval = ml_list();
			ml_list_put(Interval, ml_integer(Case->Min));
			ml_list_put(Interval, ml_integer(Case->Max));
			ml_list_put(Last, Interval);
		} else {
			break;
		}
	}
	return Result;
}

ML_DESERIALIZER("integer-switch") {
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_integer_switch_t *Switch = xnew(ml_integer_switch_t, Total, ml_integer_case_t);
	Switch->Type = MLIntegerSwitchT;
	ml_integer_case_t *Case = Switch->Cases;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, MLListT) && ml_list_length(Value) == 2) {
				Case->Min = ml_integer_value(ml_list_get(Value, 1));
				Case->Max = ml_integer_value(ml_list_get(Value, 2));
			} else {
				return ml_error("ValueError", "Unsupported value in integer case");
			}
			Case->Index = ml_integer(I);
			++Case;
		}
	}
	Case->Min = LONG_MIN;
	Case->Max = LONG_MAX;
	Case->Index = ml_integer(Count);
	return (ml_value_t *)Switch;
}

typedef struct {
	ml_value_t *Index;
	double Min, Max;
} ml_real_case_t;

typedef struct {
	ml_type_t *Type;
	ml_real_case_t Cases[];
} ml_real_switch_t;

static void ml_real_switch(ml_state_t *Caller, ml_real_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (!ml_is(Arg, MLNumberT)) {
		ML_ERROR("TypeError", "Expected number for argument 1");
	}
	double Value = ml_real_value(Arg);
	for (ml_real_case_t *Case = Switch->Cases;; ++Case) {
		if (Case->Min <= Value && Value <= Case->Max) ML_RETURN(Case->Index);
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLRealSwitchT, (MLFunctionT), "real-switch",
//!internal
	.call = (void *)ml_real_switch
);

ML_FUNCTION_INLINE(MLRealSwitch) {
//!internal
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_real_switch_t *Switch = xnew(ml_real_switch_t, Total, ml_real_case_t);
	Switch->Type = MLRealSwitchT;
	ml_real_case_t *Case = Switch->Cases;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, MLIntegerT)) {
				Case->Min = Case->Max = ml_integer_value(Value);
			} else if (ml_is(Value, MLDoubleT)) {
				Case->Min = Case->Max = ml_real_value(Value);
			} else if (ml_is(Value, MLIntegerIntervalT)) {
				ml_integer_interval_t *Interval = (ml_integer_interval_t *)Value;
				Case->Min = Interval->Start;
				Case->Max = Interval->Limit;
			} else if (ml_is(Value, MLRealIntervalT)) {
				ml_real_interval_t *Interval = (ml_real_interval_t *)Value;
				Case->Min = Interval->Start;
				Case->Max = Interval->Limit;
			} else {
				return ml_error("ValueError", "Unsupported value in real case");
			}
			Case->Index = ml_integer(I);
			++Case;
		}
	}
	Case->Min = -INFINITY;
	Case->Max = INFINITY;
	Case->Index = ml_integer(Count);
	return (ml_value_t *)Switch;
}

static ml_value_t *ML_TYPED_FN(ml_serialize, MLRealSwitchT, ml_real_switch_t *Switch) {
	ml_value_t *Result = ml_list();
	ml_list_put(Result, ml_cstring("real-switch"));
	ml_value_t *Index = NULL, *Last = NULL;
	for (ml_real_case_t *Case = Switch->Cases;; ++Case) {
		if (Case->Min > DBL_MIN && Case->Max < DBL_MAX) {
			if (Case->Index != Index) {
				Index = Case->Index;
				Last = ml_list();
				ml_list_put(Result, Last);
			}
			ml_value_t *Interval = ml_list();
			ml_list_put(Interval, ml_real(Case->Min));
			ml_list_put(Interval, ml_real(Case->Max));
			ml_list_put(Last, Interval);
		} else {
			break;
		}
	}
	return Result;
}

ML_DESERIALIZER("real-switch") {
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_real_switch_t *Switch = xnew(ml_real_switch_t, Total, ml_integer_case_t);
	Switch->Type = MLRealSwitchT;
	ml_real_case_t *Case = Switch->Cases;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, MLListT) && ml_list_length(Value) == 2) {
				Case->Min = ml_real_value(ml_list_get(Value, 1));
				Case->Max = ml_real_value(ml_list_get(Value, 2));
			} else {
				return ml_error("ValueError", "Unsupported value in integer case");
			}
			Case->Index = ml_integer(I);
			++Case;
		}
	}
	Case->Min = -INFINITY;
	Case->Max = INFINITY;
	Case->Index = ml_integer(Count);
	return (ml_value_t *)Switch;
}

#ifdef ML_DECIMAL

static long ml_decimal_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return ml_integer_value(Value);
}

ML_TYPE(MLDecimalT, (MLRealT), "decimal",
	.hash = (void *)ml_decimal_hash
);

ml_value_t *ml_decimal(ml_value_t *Unscaled, int32_t Scale) {
	ml_decimal_t *Decimal = new(ml_decimal_t);
	Decimal->Type = MLDecimalT;
#ifdef ML_BIGINT
	ml_integer_mpz_init(Decimal->Unscaled, Unscaled);
#else
	Decimal->Unscaled = ml_integer_value(Unscaled);
#endif
	Decimal->Scale = Scale;
	return (ml_value_t *)Decimal;
}

ML_METHOD("unscaled", MLDecimalT) {
	ml_decimal_t *Decimal = (ml_decimal_t *)Args[0];
#ifdef ML_BIGINT
	return ml_integer_mpz_copy(Decimal->Unscaled);
#else
	return ml_integer(Decimal->Unscaled);
#endif
}

ML_METHOD("scale", MLDecimalT) {
	ml_decimal_t *Decimal = (ml_decimal_t *)Args[0];
	return ml_integer(Decimal->Scale);
}

static int64_t ML_TYPED_FN(ml_integer_value, MLDecimalT, ml_decimal_t *Decimal) {
#ifdef ML_BIGINT
	if (Decimal->Scale > 0) {
		mpz_t Value;
		mpz_init(Value);
		mpz_ui_pow_ui(Value, 10, Decimal->Scale);
		mpz_fdiv_q(Value, Decimal->Unscaled, Value);
		return mpz_get_s64(Value);
	} else if (Decimal->Scale < 0) {
		mpz_t Value;
		mpz_init(Value);
		mpz_ui_pow_ui(Value, 10, -Decimal->Scale);
		mpz_mul(Value, Decimal->Unscaled, Value);
		return mpz_get_s64(Value);
	} else {
		return mpz_get_s64(Decimal->Unscaled);
	}
#else
	return Decimal->Unscaled / exp10(Decimal->Scale);
#endif
}

#ifdef ML_BIGINT

static void ML_TYPED_FN(ml_integer_mpz_init, MLDecimalT, mpz_t Value, ml_decimal_t *Decimal) {
	mpz_init(Value);
	if (Decimal->Scale > 0) {
		mpz_ui_pow_ui(Value, 10, Decimal->Scale);
		mpz_fdiv_q(Value, Decimal->Unscaled, Value);
	} else if (Decimal->Scale < 0) {
		mpz_ui_pow_ui(Value, 10, -Decimal->Scale);
		mpz_mul(Value, Decimal->Unscaled, Value);
	} else {
		mpz_set(Value, Decimal->Unscaled);
	}
}

#endif

static double ML_TYPED_FN(ml_real_value, MLDecimalT, ml_decimal_t *Decimal) {
#ifdef ML_BIGINT
	if (Decimal->Scale > 0) {
		mpq_t Scaled;
		mpz_init_set(mpq_numref(Scaled), Decimal->Unscaled);
		mpz_init(mpq_denref(Scaled));
		mpz_ui_pow_ui(mpq_denref(Scaled), 10, Decimal->Scale);
		return mpq_get_d(Scaled);
	} else if (Decimal->Scale < 0) {
		mpz_t Scaled;
		mpz_init(Scaled);
		mpz_ui_pow_ui(Scaled, 10, -Decimal->Scale);
		mpz_mul(Scaled, Decimal->Unscaled, Scaled);
		return mpz_get_d(Scaled);
	} else {
		return mpz_get_d(Decimal->Unscaled);
	}
#else
	return Decimal->Unscaled / exp10(Decimal->Scale);
#endif
}

ML_METHOD(MLDecimalT, MLIntegerT, MLIntegerT) {
	ml_decimal_t *Decimal = new(ml_decimal_t);
	Decimal->Type = MLDecimalT;
#ifdef ML_BIGINT
	ml_integer_mpz_init(Decimal->Unscaled, Args[0]);
#else
	Decimal->Unscaled = ml_integer_value(Args[0]);
#endif
	Decimal->Scale = ml_integer_value(Args[1]);
	return (ml_value_t *)Decimal;
}

ML_METHOD(MLDecimalT, MLIntegerT) {
	ml_decimal_t *Decimal = new(ml_decimal_t);
	Decimal->Type = MLDecimalT;
#ifdef ML_BIGINT
	ml_integer_mpz_init(Decimal->Unscaled, Args[0]);
#else
	Decimal->Unscaled = ml_integer_value(Args[0]);
#endif
	Decimal->Scale = 0;
	return (ml_value_t *)Decimal;
}



ML_METHOD("d", MLIntegerT, MLIntegerT) {
	ml_decimal_t *Decimal = new(ml_decimal_t);
	Decimal->Type = MLDecimalT;
#ifdef ML_BIGINT
	ml_integer_mpz_init(Decimal->Unscaled, Args[0]);
#else
	Decimal->Unscaled = ml_integer_value(Args[0]);
#endif
	Decimal->Scale = ml_integer_value(Args[1]);
	return (ml_value_t *)Decimal;
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

ML_METHOD(MLDecimalT, MLRealT) {
	char Temp[DBL_DIG + 10];
	int Length = sprintf(Temp, "%." TOSTRING(DBL_DIG) "g", ml_real_value(Args[0]));
	int Scale = 0;
	char *Tail = strchr(Temp, 'e');
	if (Tail) {
		Scale = -strtol(Tail + 1, NULL, 10);
		*Tail = 0;
	} else {
		Tail = Temp + Length;
	}
	char *Point = strchr(Temp, '.');
	if (Point) {
		int Digits = Tail - (Point + 1);
		Scale += Digits;
		memmove(Point, Point + 1, Digits);
		*--Tail = 0;
	}
	ml_decimal_t *Decimal = new(ml_decimal_t);
	Decimal->Type = MLDecimalT;
#ifdef ML_BIGINT
	mpz_init_set_str(Decimal->Unscaled, Temp, 10);
#else
	Decimal->Unscaled = strtoll(Temp, NULL, 10);
#endif
	Decimal->Scale = Scale;
	return (ml_value_t *)Decimal;
}

ML_METHOD(MLRealT, MLDecimalT) {
	ml_decimal_t *Decimal = (ml_decimal_t *)Args[0];
#ifdef ML_BIGINT
	if (Decimal->Scale > 0) {
		mpq_t Scaled;
		mpz_init_set(mpq_numref(Scaled), Decimal->Unscaled);
		mpz_init(mpq_denref(Scaled));
		mpz_ui_pow_ui(mpq_denref(Scaled), 10, Decimal->Scale);
		return ml_real(mpq_get_d(Scaled));
	} else if (Decimal->Scale < 0) {
		mpz_t Scaled;
		mpz_init(Scaled);
		mpz_ui_pow_ui(Scaled, 10, -Decimal->Scale);
		mpz_mul(Scaled, Decimal->Unscaled, Scaled);
		return ml_real(mpz_get_d(Scaled));
	} else {
		return ml_real(mpz_get_d(Decimal->Unscaled));
	}
#else
	return ml_real(Decimal->Unscaled / exp10(Decimal->Scale));
#endif
}

ML_METHOD("+", MLDecimalT, MLDecimalT) {
	ml_decimal_t *A = (ml_decimal_t *)Args[0];
	ml_decimal_t *B = (ml_decimal_t *)Args[1];
	ml_decimal_t *C = new(ml_decimal_t);
	C->Type = MLDecimalT;
#ifdef ML_BIGINT
	mpz_init(C->Unscaled);
	if (A->Scale > B->Scale) {
		mpz_ui_pow_ui(C->Unscaled, 10, A->Scale - B->Scale);
		mpz_mul(C->Unscaled, C->Unscaled, B->Unscaled);
		mpz_add(C->Unscaled, A->Unscaled, C->Unscaled);
		C->Scale = A->Scale;
	} else if (A->Scale < B->Scale) {
		mpz_ui_pow_ui(C->Unscaled, 10, B->Scale - A->Scale);
		mpz_mul(C->Unscaled, C->Unscaled, A->Unscaled);
		mpz_add(C->Unscaled, C->Unscaled, B->Unscaled);
		C->Scale = B->Scale;
	} else {
		mpz_add(C->Unscaled, A->Unscaled, B->Unscaled);
		C->Scale = A->Scale;
	}
#else
	if (A->Scale > B->Scale) {
		int64_t Scale = exp10(A->Scale - B->Scale);
		C->Unscaled = A->Unscaled + Scale * B->Unscaled;
		C->Scale = A->Scale;
	} else if (A->Scale < B->Scale) {
		int64_t Scale = exp10(B->Scale - A->Scale);
		C->Unscaled = Scale * A->Unscaled + B->Unscaled;
		C->Scale = B->Scale;
	} else {
		C->Unscaled = A->Unscaled + B->Unscaled;
		C->Scale = A->Scale;
	}
#endif
	return (ml_value_t *)C;
}

ML_METHOD("-", MLDecimalT, MLDecimalT) {
	ml_decimal_t *A = (ml_decimal_t *)Args[0];
	ml_decimal_t *B = (ml_decimal_t *)Args[1];
	ml_decimal_t *C = new(ml_decimal_t);
	C->Type = MLDecimalT;
#ifdef ML_BIGINT
	mpz_init(C->Unscaled);
	if (A->Scale > B->Scale) {
		mpz_ui_pow_ui(C->Unscaled, 10, A->Scale - B->Scale);
		mpz_mul(C->Unscaled, C->Unscaled, B->Unscaled);
		mpz_sub(C->Unscaled, A->Unscaled, C->Unscaled);
		C->Scale = A->Scale;
	} else if (A->Scale < B->Scale) {
		mpz_ui_pow_ui(C->Unscaled, 10, B->Scale - A->Scale);
		mpz_mul(C->Unscaled, C->Unscaled, A->Unscaled);
		mpz_sub(C->Unscaled, C->Unscaled, B->Unscaled);
		C->Scale = B->Scale;
	} else {
		mpz_sub(C->Unscaled, A->Unscaled, B->Unscaled);
		C->Scale = A->Scale;
	}
#else
	if (A->Scale > B->Scale) {
		int64_t Scale = exp10(A->Scale - B->Scale);
		C->Unscaled = A->Unscaled - Scale * B->Unscaled;
		C->Scale = A->Scale;
	} else if (A->Scale < B->Scale) {
		int64_t Scale = exp10(B->Scale - A->Scale);
		C->Unscaled = Scale * A->Unscaled - B->Unscaled;
		C->Scale = B->Scale;
	} else {
		C->Unscaled = A->Unscaled - B->Unscaled;
		C->Scale = A->Scale;
	}
#endif
	return (ml_value_t *)C;
}

ML_METHOD("*", MLDecimalT, MLDecimalT) {
	ml_decimal_t *A = (ml_decimal_t *)Args[0];
	ml_decimal_t *B = (ml_decimal_t *)Args[1];
	ml_decimal_t *C = new(ml_decimal_t);
	C->Type = MLDecimalT;
#ifdef ML_BIGINT
	mpz_init(C->Unscaled);
	mpz_mul(C->Unscaled, A->Unscaled, B->Unscaled);
#else
	C->Unscaled = A->Unscaled * B->Unscaled;
#endif
	C->Scale = A->Scale + B->Scale;
	return (ml_value_t *)C;
}

ML_METHOD("*", MLDecimalT, MLIntegerT) {
	ml_decimal_t *A = (ml_decimal_t *)Args[0];
	ml_decimal_t *C = new(ml_decimal_t);
	C->Type = MLDecimalT;
#ifdef ML_BIGINT
	ml_integer_mpz_init(C->Unscaled, Args[1]);
	mpz_mul(C->Unscaled, A->Unscaled, C->Unscaled);
#else
	C->Unscaled = A->Unscaled * ml_integer_value(Args[1]);
#endif
	C->Scale = A->Scale;
	return (ml_value_t *)C;
}

ML_METHOD("*", MLIntegerT, MLDecimalT) {
	ml_decimal_t *B = (ml_decimal_t *)Args[1];
	ml_decimal_t *C = new(ml_decimal_t);
	C->Type = MLDecimalT;
#ifdef ML_BIGINT
	ml_integer_mpz_init(C->Unscaled, Args[0]);
	mpz_mul(C->Unscaled, C->Unscaled, B->Unscaled);
#else
	C->Unscaled = ml_integer_value(Args[0]) * B->Unscaled;
#endif
	C->Scale = B->Scale;
	return (ml_value_t *)C;
}

#endif

void ml_number_init() {
#if defined(ML_BIGINT) && !defined(ML_NANBOXING)
	mpz_init_set_si(One->Value, 1);
	mpz_init_set_si(Zero->Value, 0);
	mpz_init_set_si(NegOne->Value, -1);
#endif
#include "ml_number_init.c"
#ifdef ML_GENERICS
	ml_type_t *TArgs[3] = {MLSequenceT, MLIntegerT, MLIntegerT};
	ml_type_add_parent(MLIntegerIntervalT, ml_generic_type(3, TArgs));
	TArgs[2] = MLRealT;
	ml_type_add_parent(MLRealIntervalT, ml_generic_type(3, TArgs));
#endif
	stringmap_insert(MLIntegerT->Exports, "interval", MLIntegerIntervalT);
	stringmap_insert(MLIntegerT->Exports, "switch", MLIntegerSwitch);
	stringmap_insert(MLIntegerT->Exports, "random", RandomInteger);
	stringmap_insert(MLIntegerT->Exports, "random_permutation", RandomPermutation);
	stringmap_insert(MLIntegerT->Exports, "random_cycle", RandomCycle);
	stringmap_insert(MLRealT->Exports, "interval", MLRealIntervalT);
	stringmap_insert(MLRealT->Exports, "switch", MLRealSwitch);
	stringmap_insert(MLRealT->Exports, "random", RandomReal);
	ml_method_by_value(MLIntegerT->Constructor, NULL, ml_identity, MLIntegerT, NULL);
	ml_method_by_name("isfinite", NULL, ml_identity, MLIntegerT, NULL);
	ml_method_by_name("isnan", NULL, ml_return_nil, MLIntegerT, NULL);
	ml_method_by_value(MLDoubleT->Constructor, NULL, ml_identity, MLDoubleT, NULL);
	ml_method_by_value(MLRealT->Constructor, NULL, ml_identity, MLDoubleT, NULL);
	stringmap_insert(MLRealT->Exports, "Inf", ml_real(INFINITY));
	stringmap_insert(MLRealT->Exports, "NaN", ml_real(NAN));
	ml_method_by_value(MLNumberT->Constructor, NULL, ml_identity, MLNumberT, NULL);
	ml_infix_many("+");
	ml_infix_many("*");
	ml_infix_many("/\\");
	ml_infix_many("\\/");
}

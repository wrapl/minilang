#ifndef ML_TYPES_H
#define ML_TYPES_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "stringmap.h"
#include "inthash.h"
#include "ml_config.h"

/// \defgroup types
/// @{
///

#ifdef ML_BIGINT

#include <gmp.h>

#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ml_value_t ml_value_t;
typedef struct ml_type_t ml_type_t;
typedef struct ml_context_t ml_context_t;
typedef struct ml_state_t ml_state_t;

// Macros //

#define _CONCAT2(X, Y) X ## Y
#define CONCAT2(X, Y) _CONCAT2(X, Y)

#define _CONCAT3(X, Y, Z) X ## Y ## _ ## Z
#define CONCAT3(X, Y, Z) _CONCAT3(X, Y, Z)

#ifdef ML_ASSERTS
#define ml_assert(CONDITION) { if (!(CONDITION)) asm("int3"); }
#else
#define ml_assert(CONDITION) {}
#endif

// Values and Types //

struct ml_value_t {
	ml_type_t *Type;
};

#define ML_DEF(NAME) ml_value_t *NAME = NULL

typedef struct ml_hash_chain_t ml_hash_chain_t;

struct ml_hash_chain_t {
	ml_hash_chain_t *Previous;
	ml_value_t *Value;
	int Index;
};

#ifdef ML_GENERICS

typedef struct ml_generic_rule_t ml_generic_rule_t;

#endif

struct ml_type_t {
	ml_type_t *Type;
	const char *Name;
	long (*hash)(ml_value_t *, ml_hash_chain_t *);
	void (*call)(ml_state_t *, ml_value_t *, int, ml_value_t **);
	ml_value_t *(*deref)(ml_value_t *);
	void (*assign)(ml_state_t *, ml_value_t *, ml_value_t *);
	ml_value_t *Constructor;
#ifdef ML_GENERICS
	ml_generic_rule_t *Rules;
#endif
	inthash_t Parents[1];
	inthash_t TypedFns[1];
	stringmap_t Exports[1];
	unsigned int Rank:30;
	unsigned int Interface:1;
	unsigned int NoInherit:1;
};

extern ml_type_t MLTypeT[];

long ml_default_hash(ml_value_t *Value, ml_hash_chain_t *Chain);
void ml_default_call(ml_state_t *Frame, ml_value_t *Value, int Count, ml_value_t **Args);

long ml_value_hash(ml_value_t *Value, ml_hash_chain_t *Chain);

//ml_value_t *ml_default_deref(ml_value_t *Ref);
#define ml_default_deref NULL

void ml_default_assign(ml_state_t *Caller, ml_value_t *Ref, ml_value_t *Value);

long ml_type_hash(ml_type_t *Type);
void ml_type_call(ml_state_t *Caller, ml_type_t *Type, int Count, ml_value_t **Args);

#define ML_TYPE_INIT(CONSTRUCTOR, PARENTS, NAME, ...) { \
	.Type = MLTypeT, \
	.Name = NAME, \
	.hash = ml_default_hash, \
	.call = ml_default_call, \
	.deref = ml_default_deref, \
	.assign = ml_default_assign, \
	.Constructor = CONSTRUCTOR, \
	.TypedFns = {INTHASH_INIT}, \
	.Exports = {STRINGMAP_INIT}, \
	.Rank = 0, \
	.Interface = 0, \
	__VA_ARGS__ \
}

#ifndef GENERATE_INIT

#define ML_TYPE(TYPE, PARENTS, NAME, ...) \
static ml_method_t CONCAT2(TYPE, Of)[1] = {{MLMethodT, NAME "::of"}}; \
\
ml_type_t TYPE[1] = {ML_TYPE_INIT((ml_value_t *)CONCAT2(TYPE, Of), PARENTS, NAME, __VA_ARGS__)}

#else

#define UNWRAP(ARGS...) , ##ARGS
#define ML_TYPE(TYPE, PARENTS, NAME, ...) INIT_CODE ml_type_init(TYPE UNWRAP PARENTS, NULL);

#endif

#define ML_INTERFACE(TYPE, PARENTS, NAME, ...) ML_TYPE(TYPE, PARENTS, NAME, .Rank = 1, .Interface = 1, __VA_ARGS__)

void ml_type_init(ml_type_t *Type, ...) __attribute__ ((sentinel));

ml_type_t *ml_type(ml_type_t *Parent, const char *Name);
static inline const char *ml_type_name(const ml_type_t *Value) {
	return Value->Name;
}

void ml_type_add_parent(ml_type_t *Type, ml_type_t *Parent);

extern ml_type_t MLTypeUnionT[];

ml_type_t *ml_union_type(int NumTypes, ml_type_t *Types[]);

#ifndef GENERATE_INIT

#define ML_UNION_TYPE(TYPE, ...) ml_value_t *TYPE

#else

#define ML_UNION_TYPE(TYPE, ...) \
INIT_CODE TYPE = (ml_value_t *)ml_union_type(PP_NARG(__VA_ARGS__), (ml_type_t *[]){__VA_ARGS__})

#endif

#ifdef ML_GENERICS

typedef struct ml_generic_type_t ml_generic_type_t;

struct ml_generic_type_t {
	ml_type_t Base;
	int NumArgs;
	ml_generic_type_t *NextGeneric;
	ml_type_t *Args[];
};

extern ml_type_t MLTypeGenericT[];

ml_type_t *ml_generic_type(int NumArgs, ml_type_t *Args[]);

#define ml_generic_type_num_args(TYPE) ((ml_generic_type_t *)TYPE)->NumArgs
#define ml_generic_type_args(TYPE) ((ml_generic_type_t *)TYPE)->Args

int ml_find_generic_parent(ml_type_t *T, ml_type_t *U, int Max, ml_type_t **Args);

#ifndef GENERATE_INIT

#define ML_GENERIC_TYPE(TYPE, PARENT, ...) ml_type_t *TYPE

#else

#define ML_GENERIC_TYPE(TYPE, ...) \
INIT_CODE TYPE = ml_generic_type(PP_NARG(__VA_ARGS__), (ml_type_t *[]){__VA_ARGS__})

#endif

#endif

void ml_type_add_rule(ml_type_t *Type, ml_type_t *Parent, ...) __attribute__ ((sentinel));

#define ML_TYPE_ARG(N) ((N << 1) + 1)

int ml_is_subtype(ml_type_t *Type1, ml_type_t *Type2) __attribute__ ((pure));
ml_type_t *ml_type_max(ml_type_t *Type1, ml_type_t *Type2);

typedef struct {
	ml_type_t *Type;
	ml_value_t *Fn, *Error;
	ml_value_t *Args[2];
	inthash_t Cache[1];
} ml_visitor_t;

extern ml_type_t MLVisitorT[];

#ifdef ML_NANBOXING

extern ml_type_t MLInteger32T[];
extern ml_type_t MLInteger64T[];
extern ml_type_t MLDoubleT[];

#ifdef ML_RATIONAL

extern ml_type_t MLRational48T[];

#endif

__attribute__ ((pure)) static inline int ml_tag(const ml_value_t *Value) {
	return (uint64_t)Value >> 48;
}

static inline ml_value_t *ml_deref(ml_value_t *Value) {
	unsigned Tag = ml_tag(Value);
	if (__builtin_expect(Tag == 0, 1)) {
		ml_value_t *(*Deref)(ml_value_t *) = Value->Type->deref;
		if (__builtin_expect(Deref != ml_default_deref, 0)) {
			return Deref(Value);
		}
	}
	return Value;
}

__attribute__ ((pure)) static inline ml_type_t *ml_typeof(const ml_value_t *Value) {
	unsigned Tag = ml_tag(Value);
	if (__builtin_expect(Tag == 0, 1)) {
		return Value->Type;
	} else if (Tag == 1) {
		return MLInteger32T;
#ifdef ML_RATIONAL
	} else if (Tag == 2) {
		return MLRational48T;
#endif
	} else {
		return MLDoubleT;
	}
}

#define ml_typeof_deref(VALUE) ml_typeof(ml_deref(VALUE))

#else

static inline ml_type_t *ml_typeof(const ml_value_t *Value) {
	return Value->Type;
}

static inline ml_value_t *ml_deref(ml_value_t *Value) {
	if (__builtin_expect(Value->Type->deref != ml_default_deref, 0)) {
		return Value->Type->deref(Value);
	}
	return Value;
}

static inline ml_type_t *ml_typeof_deref(ml_value_t *Value) {
	ml_type_t *Type = Value->Type;
	if (__builtin_expect(Type->deref != ml_default_deref, 0)) {
		return ml_typeof(Type->deref(Value));
	}
	return Type;
}

#endif

int ml_is(const ml_value_t *Value, const ml_type_t *Expected);

long ml_hash_chain(ml_value_t *Value, ml_hash_chain_t *Chain);

static inline long ml_hash(ml_value_t *Value) {
	return ml_hash_chain(Value, NULL);
}

#define SHA256_BLOCK_SIZE 32

void ml_value_sha256(ml_value_t *Value, ml_hash_chain_t *Chain, unsigned char Hash[SHA256_BLOCK_SIZE]);

#define ml_call(CALLER, VALUE, COUNT, ARGS) ml_typeof(VALUE)->call((ml_state_t *)CALLER, VALUE, COUNT, ARGS)

#define ml_inline(STATE, VALUE, COUNT, ARGS ...) ml_call(STATE, VALUE, COUNT, (ml_value_t **)(void *[]){ARGS})

#define ml_assign(CALLER, VALUE, VALUE2) ml_typeof(VALUE)->assign((ml_state_t *)CALLER, VALUE, VALUE2)

void *ml_typed_fn_get(ml_type_t *Type, void *TypedFn);
void ml_typed_fn_set(ml_type_t *Type, void *TypedFn, void *Function);

#ifndef GENERATE_INIT

#define ML_TYPED_FN(FUNCTION, TYPE, ARGS ...) CONCAT3(FUNCTION ## _, __LINE__, __COUNTER__)(ARGS)

#else

#define ML_TYPED_FN(FUNCTION, TYPE, ARGS ...) INIT_CODE ml_typed_fn_set(TYPE, (void *)FUNCTION, (void *)(typeof(FUNCTION)*)CONCAT3(FUNCTION ## _, __LINE__, __COUNTER__));

#endif

#define ML_VALUE(NAME, TYPE) \
ml_value_t NAME[1] = {{TYPE}}

extern ml_type_t MLAnyT[];
extern ml_type_t MLNilT[];
extern ml_type_t MLSomeT[];
extern ml_type_t MLBlankT[];

extern ml_value_t MLNil[];
extern ml_value_t MLSome[];
extern ml_value_t MLBlank[];

void ml_value_set_name(ml_value_t *Value, const char *Name);

typedef ml_value_t *(*ml_callback_t)(void *Data, int Count, ml_value_t **Args);
typedef void (*ml_callbackx_t)(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args);

typedef int (*ml_value_find_fn)(void *Data, ml_value_t *Value, int HasRefs);
void ml_value_find_all(ml_value_t *Value, void *Data, ml_value_find_fn RefFn);

int ml_value_is_constant(ml_value_t *Value);

/// @}

/// \defgroup iterators
/// @{
///

// Iterators //

extern ml_type_t MLSequenceT[];

void ml_count(ml_state_t *Caller, ml_value_t *Value);
void ml_iterate(ml_state_t *Caller, ml_value_t *Value);
void ml_iter_value(ml_state_t *Caller, ml_value_t *Iter);
void ml_iter_key(ml_state_t *Caller, ml_value_t *Iter);
void ml_iter_next(ml_state_t *Caller, ml_value_t *Iter);

ml_value_t *ml_chained(int Count, ml_value_t **Functions);
ml_value_t *ml_chainedv(int Count, ...);
ml_value_t *ml_doubled(ml_value_t *Sequence, ml_value_t *Function);

ml_value_t *ml_dup(ml_value_t *Sequence);
ml_value_t *ml_swap(ml_value_t *Sequence);

/// @}

/// \defgroup functions
/// @{
///

// Functions //

extern ml_type_t MLFunctionT[];

int ml_function_source(ml_value_t *Value, const char **Source, int *Line);

typedef struct ml_cfunction_t ml_cfunction_t;
typedef struct ml_cfunctionx_t ml_cfunctionx_t;

struct ml_cfunction_t {
	ml_type_t *Type;
	ml_callback_t Callback;
	void *Data;
	const char *Source;
	int Line;
};

struct ml_cfunctionx_t {
	ml_type_t *Type;
	ml_callbackx_t Callback;
	void *Data;
	const char *Source;
	int Line;
};

extern ml_type_t MLCFunctionT[];
extern ml_type_t MLCFunctionXT[];
extern ml_type_t MLCFunctionZT[];

#define ML_CFUNCTION(NAME, DATA, CALLBACK) static ml_cfunction_t NAME[1] = {{MLCFunctionT, CALLBACK, DATA, ML_CATEGORY, __LINE__}}

#define ML_CFUNCTIONX(NAME, DATA, CALLBACK) static ml_cfunctionx_t NAME[1] = {{MLCFunctionXT, CALLBACK, DATA, ML_CATEGORY, __LINE__}}

#define ML_CFUNCTIONZ(NAME, DATA, CALLBACK) static ml_cfunctionx_t NAME[1] = {{MLCFunctionZT, CALLBACK, DATA, ML_CATEGORY, __LINE__}}

extern ml_cfunctionx_t MLCallCC[];
extern ml_cfunctionx_t MLMarkCC[];
extern ml_cfunctionx_t MLCallDC[];
extern ml_cfunctionx_t MLSwapCC[];

extern ml_type_t MLContextKeyT[];
extern ml_cfunction_t MLContextKey[];

ml_value_t *ml_cfunction(void *Data, ml_callback_t Function) __attribute__((malloc));
ml_value_t *ml_cfunctionx(void *Data, ml_callbackx_t Function) __attribute__((malloc));
ml_value_t *ml_cfunctionz(void *Data, ml_callbackx_t Function) __attribute__((malloc));

ml_value_t *ml_cfunction2(void *Data, ml_callback_t Function, const char *Source, int Line) __attribute__((malloc));
ml_value_t *ml_cfunctionx2(void *Data, ml_callbackx_t Function, const char *Source, int Line) __attribute__((malloc));
ml_value_t *ml_cfunctionz2(void *Data, ml_callbackx_t Function, const char *Source, int Line) __attribute__((malloc));

ml_value_t *ml_return_nil(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_identity(void *Data, int Count, ml_value_t **Args);

ml_value_t *ml_partial_function(ml_value_t *Function, int Count) __attribute__((malloc));
ml_value_t *ml_partial_function_set(ml_value_t *Partial, size_t Index, ml_value_t *Value);

ml_value_t *ml_value_function(ml_value_t *Value);

#define ML_FUNCTION2(NAME, FUNCTION) static ml_value_t *FUNCTION(void *Data, int Count, ml_value_t **Args); \
\
ml_cfunction_t NAME[1] = {{MLCFunctionT, FUNCTION, NULL, ML_CATEGORY, __LINE__}}; \
\
static ml_value_t *FUNCTION(void *Data, int Count, ml_value_t **Args)

#define ML_FUNCTION(NAME) ML_FUNCTION2(NAME, CONCAT3(ml_cfunction_, __LINE__, __COUNTER__))

#define ML_FUNCTIONX2(NAME, FUNCTION) static void FUNCTION(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args); \
\
ml_cfunctionx_t NAME[1] = {{MLCFunctionXT, FUNCTION, NULL, ML_CATEGORY, __LINE__}}; \
\
static void FUNCTION(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args)

#define ML_FUNCTIONX(NAME, TYPES ...) ML_FUNCTIONX2(NAME, CONCAT3(ml_cfunctionx_, __LINE__, __COUNTER__))

#define ML_FUNCTIONZ2(NAME, FUNCTION) static void FUNCTION(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args); \
\
ml_cfunctionx_t NAME[1] = {{MLCFunctionZT, FUNCTION, NULL, ML_CATEGORY, __LINE__}}; \
\
static void FUNCTION(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args)

#define ML_FUNCTIONZ(NAME, TYPES ...) ML_FUNCTIONZ2(NAME, CONCAT3(ml_cfunctionx_, __LINE__, __COUNTER__))

#define ML_CHECK_ARG_TYPE(N, TYPE) \
	if (!ml_is(Args[N], TYPE)) { \
		return ml_error("TypeError", "Expected %s for argument %d", TYPE->Name, N + 1); \
	}

#define ML_CHECK_ARG_COUNT(N) \
	if (Count < N) { \
		return ml_error("CallError", "%d arguments required", N); \
	}

#define ML_CHECKX_ARG_TYPE(N, TYPE) \
	if (!ml_is(Args[N], TYPE)) { \
		ML_ERROR("TypeError", "Expected %s for argument %d", TYPE->Name, N + 1); \
	}

#define ML_CHECKX_ARG_COUNT(N) \
	if (Count < N) { \
		ML_ERROR("CallError", "%d arguments required", N); \
	}

#ifdef ML_TRAMPOLINE

#define ML_CONTINUE(STATE, VALUE) { \
	ml_state_t *__State = (ml_state_t *)(STATE); \
	ml_value_t *__Value = (ml_value_t *)(VALUE); \
	ml_state_schedule(__State, __Value); \
	return; \
}

#else

#ifdef ML_TIMESCHED

#define ML_CONTINUE(STATE, VALUE) return ml_state_continue((ml_state_t *)(STATE), (ml_value_t *)(VALUE))

#else

#define ML_CONTINUE(STATE, VALUE) { \
	ml_state_t *__State = (ml_state_t *)(STATE); \
	ml_value_t *__Value = (ml_value_t *)(VALUE); \
	return __State->run(__State, __Value); \
}

#endif

#endif

#define ML_RETURN(VALUE) ML_CONTINUE(Caller, VALUE)
#define ML_ERROR(ARGS...) ML_RETURN(ml_error(ARGS))

/// @}

/// \defgroup tuples
/// @{
///

// Tuples //

typedef struct ml_tuple_t ml_tuple_t;

extern ml_type_t MLTupleT[];

struct ml_tuple_t {
	ml_type_t *Type;
	int Size, NoRefs;
	ml_value_t *Values[];
};

ml_value_t *ml_tuple(size_t Size) __attribute__((malloc));
ml_value_t *ml_tuplen(size_t Size, ml_value_t **Values) __attribute__((malloc));
ml_value_t *ml_tuplev(size_t Size, ...) __attribute__((malloc));

static inline int ml_tuple_size(ml_value_t *Tuple) {
	return ((ml_tuple_t *)Tuple)->Size;
}

static inline ml_value_t *ml_tuple_get(ml_value_t *Tuple, int Index) {
	return ((ml_tuple_t *)Tuple)->Values[Index - 1];
}

#ifdef ML_GENERICS

ml_value_t *ml_tuple_set(ml_value_t *Tuple0, int Index, ml_value_t *Value);

#else

static inline ml_value_t *ml_tuple_set(ml_value_t *Tuple0, int Index, ml_value_t *Value) {
	ml_tuple_t *Tuple = (ml_tuple_t *)Tuple0;
	return Tuple->Values[Index - 1] = Value;
}

#endif

ml_value_t *ml_unpack(ml_value_t *Value, int Index);

/// @}

/// \defgroup booleans
/// @{
///

// Booleans //

typedef struct ml_boolean_t {
	ml_type_t *Type;
	const char *Name;
	int Value;
} ml_boolean_t;

extern ml_type_t MLBooleanT[];
extern ml_boolean_t MLTrue[];
extern ml_boolean_t MLFalse[];

ml_value_t *ml_boolean(int Value) __attribute__ ((const));
int ml_boolean_value(const ml_value_t *Value) __attribute__ ((const));

/// @}

/// \defgroup numbers
/// @{
///

// Numbers //

extern ml_type_t MLNumberT[];
extern ml_type_t MLRealT[];
extern ml_type_t MLIntegerT[];
extern ml_type_t MLDoubleT[];
extern ml_type_t MLInteger64T[];

uint64_t ml_gcd(uint64_t A, uint64_t B);

#ifdef ML_RATIONAL

extern ml_type_t MLRationalT[];

typedef struct { int64_t Num; uint64_t Den; } rat64_t;

#endif

int64_t ml_integer_value(const ml_value_t *Value) __attribute__ ((const));
double ml_real_value(const ml_value_t *Value) __attribute__ ((const));

ml_value_t *ml_integer_parse(char *String);
ml_value_t *ml_real_parse(char *String);

typedef struct {
	ml_type_t *Type;
#ifdef ML_BIGINT
	mpz_t Value;
#else
	int64_t Value;
#endif
} ml_integer_t;

#ifdef ML_BIGINT

void ml_integer_mpz_init(mpz_t Dest, ml_value_t *Source);
ml_value_t *ml_integer_mpz_copy(const mpz_t Source);
ml_value_t *ml_integer_mpz(mpz_t Source);

void mpz_set_s64(mpz_t Z, int64_t V);
int64_t mpz_get_s64(const mpz_t Z);

uint64_t mpz_get_u64(const mpz_t Z);
void mpz_set_u64(mpz_t Z, uint64_t V);

#define ml_integer_mpz_value(VALUE) (((ml_integer_t *)VALUE)->Value)

#endif

#ifdef ML_NANBOXING

#define NegOne ml_integer32(-1)
#define One ml_integer32(1)
#define Zero ml_integer32(0)

static inline ml_value_t *ml_integer32(int32_t Integer) {
	return (ml_value_t *)(((uint64_t)1 << 48) + (uint32_t)Integer);
}

ml_value_t *ml_integer64(int64_t Integer);

static inline ml_value_t *ml_integer(int64_t Integer) {
	if (Integer >= INT32_MIN && Integer <= INT32_MAX) {
		return ml_integer32(Integer);
	} else {
		return ml_integer64(Integer);
	}
}

static inline ml_value_t *ml_real(double Value) {
	union { ml_value_t *Value; uint64_t Bits; double Double; } Boxed;
	Boxed.Double = Value;
	Boxed.Bits += 0x07000000000000;
	return Boxed.Value;
}

#ifdef ML_RATIONAL

typedef struct {
	ml_type_t *Type;
#ifdef ML_BIGINT
	fmpq_t Value;
#else
	int64_t Num;
	int64_t Den;
#endif
} ml_rational_t;

static inline ml_value_t *ml_rational48(int32_t Num, uint16_t Den) {
	return (ml_value_t *)(((uint64_t)2 << 48) + ((uint64_t)Den << 32) + (uint32_t)Num);
}

ml_value_t *ml_rational64(int64_t Num, uint64_t Den);

static inline ml_value_t *ml_rational(int64_t Num, uint64_t Den) {
	if (Den <= UINT16_MAX && Num >= INT32_MIN && Num <= INT32_MAX) {
		return ml_rational48(Num, Den);
	} else {
		return ml_rational64(Num, Den);
	}
}

#ifdef ML_BIGINT

ml_value_t *ml_rational_fmpq(fmpq_t Source);

#endif

#endif

static inline int ml_is_double(ml_value_t *Value) {
	return ml_tag(Value) >= 7;
}

static inline int64_t ml_integer32_value(const ml_value_t *Value) {
	return (int32_t)(intptr_t)Value;
}

static inline int64_t ml_integer64_value(const ml_value_t *Value) {
#ifdef ML_BIGINT
	return mpz_get_s64(((ml_integer_t *)Value)->Value);
#else
	return ((ml_integer_t *)Value)->Value;
#endif
}

static inline double ml_double_value(const ml_value_t *Value) {
	union { const ml_value_t *Value; uint64_t Bits; double Double; } Boxed;
	Boxed.Value = Value;
	Boxed.Bits -= 0x07000000000000;
	return Boxed.Double;
}

#else

extern ml_integer_t One[1];
extern ml_integer_t NegOne[1];
extern ml_integer_t Zero[1];

ml_value_t *ml_integer(int64_t Value) __attribute__((malloc));
ml_value_t *ml_real(double Value) __attribute__((malloc));

inline int64_t ml_integer64_value(const ml_value_t *Value) {
#ifdef ML_BIGINT
	return mpz_get_s64(((ml_integer_t *)Value)->Value);
#else
	return ((ml_integer_t *)Value)->Value;
#endif
}

typedef struct {
	ml_type_t *Type;
	double Value;
} ml_double_t;

inline double ml_double_value(const ml_value_t *Value) {
	return ((ml_double_t *)Value)->Value;
}

#ifdef ML_RATIONAL

ml_value_t *ml_rational(int64_t Num, uint64_t Den) __attribute__((malloc));

#endif

#endif

#ifdef ML_COMPLEX

typedef _Complex float complex_float;
typedef _Complex double complex_double;

typedef struct {
	ml_type_t *Type;
	complex_double Value;
} ml_complex_t;

extern ml_type_t MLComplexT[];

ml_value_t *ml_complex(complex_double Value);
complex_double ml_complex_value(const ml_value_t *Value);

#endif

#ifdef ML_DECIMAL

typedef struct {
	ml_type_t *Type;
#ifdef ML_BIGINT
	mpz_t Unscaled;
#else
	int64_t Unscaled;
#endif
	int32_t Scale;
} ml_decimal_t;

extern ml_type_t MLDecimalT[];

ml_value_t *ml_decimal(ml_value_t *Unscaled, int32_t Scale);

#endif

extern ml_type_t MLIntegerRangeT[];

typedef struct {
	const ml_type_t *Type;
	long Start, Limit, Step;
} ml_integer_range_t;

extern ml_type_t MLIntegerIntervalT[];

typedef struct {
	const ml_type_t *Type;
	long Start, Limit;
} ml_integer_interval_t;

extern ml_type_t MLRealRangeT[];

typedef struct {
	const ml_type_t *Type;
	double Start, Limit, Step;
} ml_real_range_t;

size_t ml_real_range_count(ml_real_range_t *Interval);

extern ml_type_t MLRealIntervalT[];

typedef struct {
	const ml_type_t *Type;
	double Start, Limit;
} ml_real_interval_t;

size_t ml_real_interval_count(ml_real_interval_t *Interval);

/// @}

/// \defgroup strings
/// @{
///

// Strings //

int GC_vasprintf(char **Ptr, const char *Format, va_list Args);
int GC_asprintf(char **Ptr, const char *Format, ...) __attribute__((format(printf, 2, 3)));

typedef struct ml_address_t ml_address_t;
typedef struct ml_string_t ml_string_t;

struct ml_address_t {
	ml_type_t *Type;
	char *Value;
	size_t Length;
};

struct ml_string_t {
	ml_type_t *Type;
	const char *Value;
	size_t Length;
	long Hash;
};

extern ml_type_t MLAddressT[];
extern ml_type_t MLBufferT[];
extern ml_type_t MLStringT[];

extern ml_type_t MLRegexT[];
extern ml_type_t MLStringBufferT[];

ml_value_t *ml_address(const char *Value, int Length) __attribute__((malloc));

static inline const char *ml_address_value(const ml_value_t *Value) {
	return ((ml_address_t *)Value)->Value;
}

static inline size_t ml_address_length(const ml_value_t *Value) {
	return ((ml_address_t *)Value)->Length;
}

ml_value_t *ml_buffer(char *Value, int Length) __attribute__((malloc));

static inline char *ml_buffer_value(const ml_value_t *Value) {
	return ((ml_address_t *)Value)->Value;
}

static inline size_t ml_buffer_length(const ml_value_t *Value) {
	return ((ml_address_t *)Value)->Length;
}

ml_value_t *ml_string(const char *Value, int Length) __attribute__((malloc));
ml_value_t *ml_string_checked(const char *Value, int Length) __attribute__((malloc));
ml_value_t *ml_string_unchecked(const char *Value, int Length) __attribute__((malloc));
ml_value_t *ml_string_copy(const char *Value, int Length) __attribute__((malloc));
ml_value_t *ml_string_format(const char *Format, ...) __attribute__((malloc, format(printf, 1, 2)));
#define ml_string_value ml_address_value
#define ml_string_length ml_address_length

//#define ml_cstring(VALUE) ml_string(VALUE, strlen(VALUE))
#define ml_cstring(VALUE) ({ \
	static ml_string_t String ## __COUNTER__ = {MLStringT, VALUE, strlen(VALUE), 0}; \
	(ml_value_t *)&String ## __COUNTER__; \
})

ml_value_t *ml_regex(const char *Value, int Length) __attribute__((malloc));
ml_value_t *ml_regexi(const char *Value, int Length) __attribute__((malloc));
const char *ml_regex_pattern(const ml_value_t *Value) __attribute__((pure));

int ml_regex_match(ml_value_t *Value, const char *Subject, int Length);

typedef struct ml_stringbuffer_t ml_stringbuffer_t;
typedef struct ml_stringbuffer_node_t ml_stringbuffer_node_t;

struct ml_stringbuffer_t {
	ml_type_t *Type;
	ml_stringbuffer_node_t *Head, *Tail;
	ml_hash_chain_t *Chain;
	int Space, Length, Start, Index;
};

#define ML_STRINGBUFFER_NODE_SIZE 248

struct ml_stringbuffer_node_t {
	ml_stringbuffer_node_t *Next;
	char Chars[ML_STRINGBUFFER_NODE_SIZE];
};

#define ML_STRINGBUFFER_INIT (ml_stringbuffer_t){MLStringBufferT, 0,}

ml_value_t *ml_stringbuffer();

static inline int ml_stringbuffer_length(ml_stringbuffer_t *Buffer) {
	return Buffer->Length;
}

char *ml_stringbuffer_writer(ml_stringbuffer_t *Buffer, size_t Length);
ssize_t ml_stringbuffer_printf(ml_stringbuffer_t *Buffer, const char *Format, ...) __attribute__ ((format(printf, 2, 3)));
char ml_stringbuffer_last(ml_stringbuffer_t *Buffer);
void ml_stringbuffer_append(ml_state_t *Caller, ml_stringbuffer_t *Buffer, ml_value_t *Value);
void ml_stringbuffer_clear(ml_stringbuffer_t *Buffer);
void ml_stringbuffer_escape_string(ml_stringbuffer_t *Buffer, const char *String, int Length);

void ml_stringbuffer_put_actual(ml_stringbuffer_t *Buffer, char Char);
static inline void ml_stringbuffer_put(ml_stringbuffer_t *Buffer, char Char) {
	if (!Buffer->Space) return ml_stringbuffer_put_actual(Buffer, Char);
	Buffer->Tail->Chars[ML_STRINGBUFFER_NODE_SIZE - Buffer->Space] = Char;
	Buffer->Space -= 1;
	Buffer->Length += 1;
}

ssize_t ml_stringbuffer_write_actual(ml_stringbuffer_t *Buffer, const char *String, size_t Length);
static inline ssize_t ml_stringbuffer_write(ml_stringbuffer_t *Buffer, const char *String, size_t Length) {
	if (Buffer->Space < Length) return ml_stringbuffer_write_actual(Buffer, String, Length);
	memcpy(Buffer->Tail->Chars + ML_STRINGBUFFER_NODE_SIZE - Buffer->Space, String, Length);
	Buffer->Space -= Length;
	Buffer->Length += Length;
	return Length;
}

static inline void ml_stringbuffer_put32(ml_stringbuffer_t *Buffer, uint32_t Code) {
	char Val[8];
	uint32_t LeadByteMax = 0x7F;
	int I = 8;
	while (Code > LeadByteMax) {
		Val[--I] = (Code & 0x3F) | 0x80;
		Code >>= 6;
		LeadByteMax >>= (I == 7 ? 2 : 1);
	}
	Val[--I] = (Code & LeadByteMax) | (~LeadByteMax << 1);
	ml_stringbuffer_write(Buffer, Val + I, 8 - I);
}

ml_value_t *ml_stringbuffer_simple_append(ml_stringbuffer_t *Buffer, ml_value_t *Value);

char *ml_stringbuffer_get_string(ml_stringbuffer_t *Buffer) __attribute__ ((malloc));
char *ml_stringbuffer_get_uncollectable(ml_stringbuffer_t *Buffer) __attribute__ ((malloc));
ml_value_t *ml_stringbuffer_get_value(ml_stringbuffer_t *Buffer) __attribute__ ((malloc));

ml_value_t *ml_stringbuffer_to_address(ml_stringbuffer_t *Buffer) __attribute__ ((malloc));
ml_value_t *ml_stringbuffer_to_buffer(ml_stringbuffer_t *Buffer) __attribute__ ((malloc));
ml_value_t *ml_stringbuffer_to_string(ml_stringbuffer_t *Buffer) __attribute__ ((malloc));

size_t ml_stringbuffer_reader(ml_stringbuffer_t *Buffer, size_t Length);

int ml_stringbuffer_drain(ml_stringbuffer_t *Buffer, void *Data, int (*callback)(void *, const char *, size_t));

// Defines for old function names

#define ml_stringbuffer_add ml_stringbuffer_write
#define ml_stringbuffer_addf ml_stringbuffer_printf
#define ml_stringbuffer_string ml_stringbuffer_get_string
#define ml_stringbuffer_uncollectable ml_stringbuffer_get_uncollectable
#define ml_stringbuffer_value ml_stringbuffer_get_value

/// @}

/// \defgroup lists
/// @{
///

// Lists //

typedef struct ml_list_node_t ml_list_node_t;
typedef struct ml_list_t ml_list_t;

extern ml_type_t MLListT[];

struct ml_list_node_t {
	ml_type_t *Type;
	ml_list_node_t *Next, *Prev;
	ml_value_t *Value;
	int Index;
};

struct ml_list_t {
	ml_type_t *Type;
	ml_list_node_t *Head, *Tail;
	ml_list_node_t *CachedNode;
	int Length, CachedIndex;
};

ml_value_t *ml_list() __attribute__((malloc));
void ml_list_grow(ml_value_t *List, int Count);
void ml_list_push(ml_value_t *List, ml_value_t *Value);
void ml_list_put(ml_value_t *List, ml_value_t *Value);
ml_value_t *ml_list_pop(ml_value_t *List);
ml_value_t *ml_list_pull(ml_value_t *List);

ml_value_t *ml_list_get(ml_value_t *List, int Index);
ml_value_t *ml_list_set(ml_value_t *List, int Index, ml_value_t *Value);

#define ml_list_append ml_list_put

void ml_list_to_array(ml_value_t *List, ml_value_t **Array);
int ml_list_foreach(ml_value_t *List, void *Data, int (*callback)(ml_value_t *, void *));

static inline int ml_list_length(ml_value_t *List) {
	return ((ml_list_t *)List)->Length;
}

typedef struct {
	ml_list_node_t *Node;
	ml_value_t *Value;
} ml_list_iter_t;

static inline int ml_list_iter_forward(ml_value_t *List0, ml_list_iter_t *Iter) {
	ml_list_t *List = (ml_list_t *)List0;
	if ((Iter->Node = List->Head)) {
		Iter->Value = Iter->Node->Value;
		return 1;
	} else {
		Iter->Node = NULL;
		return 0;
	}
}

static inline int ml_list_iter_next(ml_list_iter_t *Iter) {
	if ((Iter->Node = Iter->Node->Next)) {
		Iter->Value = Iter->Node->Value;
		return 1;
	} else {
		return 0;
	}
}

static inline int ml_list_iter_backward(ml_value_t *List0, ml_list_iter_t *Iter) {
	ml_list_t *List = (ml_list_t *)List0;
	if ((Iter->Node = List->Tail)) {
		Iter->Value = Iter->Node->Value;
		return 1;
	} else {
		Iter->Node = NULL;
		return 0;
	}
}

static inline int ml_list_iter_prev(ml_list_iter_t *Iter) {
	if ((Iter->Node = Iter->Node->Prev)) {
		Iter->Value = Iter->Node->Value;
		return 1;
	} else {
		return 0;
	}
}

static inline int ml_list_iter_valid(ml_list_iter_t *Iter) {
	return Iter->Node != NULL;
}

static inline void ml_list_iter_update(ml_list_iter_t *Iter, ml_value_t *Value) {
	Iter->Value = Iter->Node->Value = Value;
}

#define ML_LIST_FOREACH(LIST, ITER) \
	for (ml_list_node_t *ITER = ((ml_list_t *)LIST)->Head; ITER; ITER = ITER->Next)

#define ML_LIST_REVERSE(LIST, ITER) \
	for (ml_list_node_t *ITER = ((ml_list_t *)LIST)->Tail; ITER; ITER = ITER->Prev)

/// @}

/// \defgroup slices
/// @{
///

// Slices //

typedef struct { ml_value_t *Value; } ml_slice_node_t;

typedef struct {
	ml_type_t *Type;
	ml_slice_node_t *Nodes;
	size_t Capacity, Offset, Length;
} ml_slice_t;

extern ml_type_t MLSliceT[];

ml_value_t *ml_slice(size_t Capacity) __attribute__((malloc));
void ml_slice_grow(ml_value_t *Slice, int Count);
void ml_slice_put(ml_value_t *Slice, ml_value_t *Value);
void ml_slice_push(ml_value_t *Slice, ml_value_t *Value);
ml_value_t *ml_slice_pop(ml_value_t *Slice);
ml_value_t *ml_slice_pull(ml_value_t *Slice);

ml_value_t *ml_slice_get(ml_value_t *Slice, int Index);
ml_value_t *ml_slice_set(ml_value_t *Slice, int Index, ml_value_t *Value);

static inline ml_slice_node_t *ml_slice_head(ml_slice_t *Slice) {
	return Slice->Nodes + Slice->Offset;
}

static inline size_t ml_slice_length(ml_value_t *Value) {
	return ((ml_slice_t *)Value)->Length;
}

#define ML_SLICE_FOREACH(SLICE, ITER) \
	for (ml_slice_node_t *ITER = ml_slice_head((ml_slice_t *)SLICE); ITER->Value; ++ITER)

/// @}

/// \defgroup methods
/// @{
///

// Methods //

extern ml_value_t *MLMethodDefine;
extern ml_value_t *MLMethodDefault;

typedef struct ml_method_t ml_method_t;
typedef struct ml_methods_t ml_methods_t;

struct ml_method_t {
	ml_type_t *Type;
	const char *Name;
};

extern ml_type_t MLMethodT[];

ml_value_t *ml_method(const char *Name) __attribute__ ((const));
ml_value_t *ml_method_anon(const char *Name) __attribute__ ((malloc));
const char *ml_method_name(const ml_value_t *Value) __attribute__((pure));

void ml_method_by_name(const char *Method, void *Data, ml_callback_t Function, ...) __attribute__ ((sentinel));
void ml_method_by_value(void *Method, void *Data, ml_callback_t Function, ...) __attribute__ ((sentinel));

void ml_methodx_by_name(const char *Method, void *Data, ml_callbackx_t Function, ...) __attribute__ ((sentinel));
void ml_methodx_by_value(void *Method, void *Data, ml_callbackx_t Function, ...) __attribute__ ((sentinel));

void ml_method_define(ml_value_t *Value, ml_value_t *Function, int Count, ml_type_t *Variadic, ml_type_t **Types);
void ml_method_definev(ml_value_t *Method, ml_value_t *Function, ml_type_t *Variadic, ...);

void ml_method_insert(ml_methods_t *Methods, ml_method_t *Method, ml_value_t *Callback, int Count, ml_type_t *Variadic, ml_type_t **Types);

ml_value_t *ml_method_search(ml_methods_t *Methods, ml_method_t *Method, int Count, ml_value_t **Args);

ml_value_t *ml_method_wrap(ml_value_t *Function, int Count, ml_type_t **Types);

typedef struct ml_method_cached_t ml_method_cached_t;

struct ml_method_cached_t {
	ml_method_cached_t *Next, *MethodNext;
	ml_methods_t *Methods;
	ml_method_t *Method;
	ml_value_t *Callback;
	int Count, Score;
	ml_type_t *Types[];
};

ml_method_cached_t *ml_method_search_cached(ml_methods_t *Methods, ml_method_t *Method, int Count, ml_value_t **Args);
ml_method_cached_t *ml_method_check_cached(ml_methods_t *Methods, ml_method_t *Method, ml_method_cached_t *Cached, int Count, ml_value_t **Args);

ml_value_t *ml_no_method_error(ml_method_t *Method, int Count, ml_value_t **Args);

#define ML_CATEGORY "?"

#ifndef GENERATE_INIT

static inline ml_value_t *ml_type_constructor(ml_type_t *Type) {
	return Type->Constructor;
}

#define ML_METHOD(METHOD, TYPES ...) static ml_value_t *CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)(void *Data, int Count, ml_value_t **Args)

#define ML_METHODX(METHOD, TYPES ...) static void CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args)

#define ML_METHODZ(METHOD, TYPES ...) static void CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args)

#define ML_METHODV(METHOD, TYPES ...) static ml_value_t *CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)(void *Data, int Count, ml_value_t **Args)

#define ML_METHODVX(METHOD, TYPES ...) static void CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args)

#define ML_METHODVZ(METHOD, TYPES ...) static void CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args)

static inline ml_value_t *ml_nop(void *Value) {
	return (ml_value_t *)Value;
}

#define ML_METHOD_DECL(NAME, METHOD) ml_value_t *NAME
#define ML_METHOD_ANON(NAME, METHOD) ml_value_t *NAME

#else

#ifndef __cplusplus

#define ML_METHOD(METHOD, TYPES ...) INIT_CODE ml_method_definev(_Generic(METHOD, char *: ml_method, ml_type_t *: ml_type_constructor, default: ml_nop)(METHOD), ml_cfunction2(NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), ML_CATEGORY, __LINE__), NULL, ##TYPES, NULL);

#define ML_METHODX(METHOD, TYPES ...) INIT_CODE ml_method_definev(_Generic(METHOD, char *: ml_method, ml_type_t *: ml_type_constructor, default: ml_nop)(METHOD), ml_cfunctionx2(NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), ML_CATEGORY, __LINE__), NULL, ##TYPES, NULL);

#define ML_METHODZ(METHOD, TYPES ...) INIT_CODE ml_method_definev(_Generic(METHOD, char *: ml_method, ml_type_t *: ml_type_constructor, default: ml_nop)(METHOD), ml_cfunctionz2(NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), ML_CATEGORY, __LINE__), NULL, ##TYPES, NULL);

#define ML_METHODV(METHOD, TYPES ...) INIT_CODE ml_method_definev(_Generic(METHOD, char *: ml_method, ml_type_t *: ml_type_constructor, default: ml_nop)(METHOD), ml_cfunction2(NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), ML_CATEGORY, __LINE__), MLAnyT, ##TYPES, NULL);

#define ML_METHODVX(METHOD, TYPES ...) INIT_CODE ml_method_definev(_Generic(METHOD, char *: ml_method, ml_type_t *: ml_type_constructor, default: ml_nop)(METHOD), ml_cfunctionx2(NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), ML_CATEGORY, __LINE__), MLAnyT, ##TYPES, NULL);

#define ML_METHODVZ(METHOD, TYPES ...) INIT_CODE ml_method_definev(_Generic(METHOD, char *: ml_method, ml_type_t *: ml_type_constructor, default: ml_nop)(METHOD), ml_cfunctionz2(NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), ML_CATEGORY, __LINE__), MLAnyT, ##TYPES, NULL);

#else

#define ML_METHOD(METHOD, TYPES ...) INIT_CODE ml_method_by_auto(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, (void *)NULL);

#define ML_METHODX(METHOD, TYPES ...) INIT_CODE ml_methodx_by_auto(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, (void *)NULL);

#define ML_METHODZ(METHOD, TYPES ...) INIT_CODE ml_methodz_by_auto(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, (void *)NULL);

#define ML_METHODV(METHOD, TYPES ...) INIT_CODE ml_method_by_auto(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, (void *)NULL);

#define ML_METHODVX(METHOD, TYPES ...) INIT_CODE ml_methodx_by_auto(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, (void *)NULL);

#define ML_METHODVZ(METHOD, TYPES ...) INIT_CODE ml_methodz_by_auto(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, (void *)NULL);

#endif

#define ML_METHOD_DECL(NAME, METHOD) INIT_CODE NAME = ml_method(METHOD);
#define ML_METHOD_ANON(NAME, METHOD) INIT_CODE NAME = ml_method_anon(METHOD);

#endif

void ml_methods_prevent_changes(ml_methods_t *Methods, int PreventChanges);
ml_methods_t *ml_methods_context(ml_context_t *Context);

/// @}

/// \defgroup maps
/// @{
///

// Maps //

typedef struct ml_map_t ml_map_t;
typedef struct ml_map_node_t ml_map_node_t;
typedef enum {
	MAP_ORDER_INSERT,
	MAP_ORDER_LRU,
	MAP_ORDER_MRU,
	MAP_ORDER_ASC,
	MAP_ORDER_DESC
} ml_map_order_t;

extern ml_type_t MLMapT[];
extern ml_type_t MLMapTemplateT[];

struct ml_map_t {
	ml_type_t *Type;
	ml_map_node_t *Head, *Tail, *Root;
	ml_method_cached_t *Cached;
	int Size;
	ml_map_order_t Order;
};

struct ml_map_node_t {
	ml_type_t *Type;
	ml_map_node_t *Next, *Prev;
	ml_value_t *Key;
	ml_map_node_t *Left, *Right;
	ml_map_t *Map;
	ml_value_t *Value;
	long Hash;
	int Depth;
};

ml_value_t *ml_map() __attribute__((malloc));
ml_value_t *ml_map_search(ml_value_t *Map, ml_value_t *Key);
ml_value_t *ml_map_search0(ml_value_t *Map, ml_value_t *Key);
ml_map_node_t *ml_map_slot(ml_value_t *Map, ml_value_t *Key);
ml_value_t *ml_map_insert(ml_value_t *Map, ml_value_t *Key, ml_value_t *Value);
ml_value_t *ml_map_delete(ml_value_t *Map, ml_value_t *Key);

static inline int ml_map_size(ml_value_t *Map) {
	return ((ml_map_t *)Map)->Size;
}

int ml_map_foreach(ml_value_t *Map, void *Data, int (*callback)(ml_value_t *, ml_value_t *, void *));

typedef struct {
	ml_map_node_t *Node;
	ml_value_t *Key, *Value;
} ml_map_iter_t;

static inline int ml_map_iter_forward(ml_value_t *Map0, ml_map_iter_t *Iter) {
	ml_map_t *Map = (ml_map_t *)Map0;
	ml_map_node_t *Node = Iter->Node = Map->Head;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		Iter->Value = Node->Value;
		return 1;
	}
}

static inline int ml_map_iter_next(ml_map_iter_t *Iter) {
	ml_map_node_t *Node = Iter->Node = Iter->Node->Next;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		Iter->Value = Node->Value;
		return 1;
	}
}

static inline int ml_map_iter_backward(ml_value_t *Map0, ml_map_iter_t *Iter) {
	ml_map_t *Map = (ml_map_t *)Map0;
	ml_map_node_t *Node = Iter->Node = Map->Tail;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		Iter->Value = Node->Value;
		return 1;
	}
}

static inline int ml_map_iter_prev(ml_map_iter_t *Iter) {
	ml_map_node_t *Node = Iter->Node = Iter->Node->Prev;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		Iter->Value = Node->Value;
		return 1;
	}
}

static inline int ml_map_iter_valid(ml_map_iter_t *Iter) {
	return Iter->Node != NULL;
}

static inline void ml_map_iter_update(ml_map_iter_t *Iter, ml_value_t *Value) {
	Iter->Value = Iter->Node->Value = Value;
}

#define ML_MAP_FOREACH(MAP, ITER) \
	for (ml_map_node_t *ITER = ((ml_map_t *)MAP)->Head; ITER; ITER = ITER->Next)

/// @}

/// \defgroup names
/// @{
///

// Names //

extern ml_type_t MLNamesT[];

ml_value_t *ml_names();
void ml_names_add(ml_value_t *Names, ml_value_t *Value);
#define ml_names_length ml_list_length

#define ML_NAMES_CHECK_ARG_COUNT(N) { \
	int Required = ml_names_length(ml_deref(Args[N])) + N + 1; \
	if (Count < Required) { \
		return ml_error("CallError", "%d arguments required", Required); \
	} \
}

#define ML_NAMES_CHECKX_ARG_COUNT(N) { \
	int Required = ml_names_length(ml_deref(Args[N])) + N + 1; \
	if (Count < Required) { \
		ML_ERROR("CallError", "%d arguments required", Required); \
	} \
}

#define ML_NAMES_FOREACH(NAMES, ITER) ML_LIST_FOREACH(ml_deref(NAMES), ITER)

/// @}

/// \defgroup sets
/// @{
///

// Sets //

typedef struct ml_set_t ml_set_t;
typedef struct ml_set_node_t ml_set_node_t;
typedef enum {
	SET_ORDER_INSERT,
	SET_ORDER_LRU,
	SET_ORDER_MRU,
	SET_ORDER_ASC,
	SET_ORDER_DESC
} ml_set_order_t;

extern ml_type_t MLSetT[];

struct ml_set_t {
	ml_type_t *Type;
	ml_set_node_t *Head, *Tail, *Root;
	ml_method_cached_t *Cached;
	int Size;
	ml_set_order_t Order;
};

struct ml_set_node_t {
	ml_type_t *Type;
	ml_set_node_t *Next, *Prev;
	ml_value_t *Key;
	ml_set_node_t *Left, *Right;
	long Hash;
	int Depth;
};

ml_value_t *ml_set() __attribute__((malloc));
ml_value_t *ml_set_search(ml_value_t *Set, ml_value_t *Key);
ml_value_t *ml_set_search0(ml_value_t *Set, ml_value_t *Key);
ml_set_node_t *ml_set_slot(ml_value_t *Set, ml_value_t *Key);
ml_value_t *ml_set_insert(ml_value_t *Set, ml_value_t *Key);
ml_value_t *ml_set_delete(ml_value_t *Set, ml_value_t *Key);

static inline int ml_set_size(ml_value_t *Set) {
	return ((ml_set_t *)Set)->Size;
}

int ml_set_foreach(ml_value_t *Set, void *Data, int (*callback)(ml_value_t *, void *));

typedef struct {
	ml_set_node_t *Node;
	ml_value_t *Key;
} ml_set_iter_t;

static inline int ml_set_iter_forward(ml_value_t *Set0, ml_set_iter_t *Iter) {
	ml_set_t *Set = (ml_set_t *)Set0;
	ml_set_node_t *Node = Iter->Node = Set->Head;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		return 1;
	}
}

static inline int ml_set_iter_next(ml_set_iter_t *Iter) {
	ml_set_node_t *Node = Iter->Node = Iter->Node->Next;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		return 1;
	}
}

static inline int ml_set_iter_backward(ml_value_t *Set0, ml_set_iter_t *Iter) {
	ml_set_t *Set = (ml_set_t *)Set0;
	ml_set_node_t *Node = Iter->Node = Set->Tail;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		return 1;
	}
}

static inline int ml_set_iter_prev(ml_set_iter_t *Iter) {
	ml_set_node_t *Node = Iter->Node = Iter->Node->Prev;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		return 1;
	}
}

static inline int ml_set_iter_valid(ml_set_iter_t *Iter) {
	return Iter->Node != NULL;
}

#define ML_SET_FOREACH(SET, ITER) \
	for (ml_set_node_t *ITER = ((ml_set_t *)SET)->Head; ITER; ITER = ITER->Next)

/// @}

/// \defgroup modules
/// @{
///

// Modules //

extern ml_type_t MLModuleT[];

typedef struct {
	const ml_type_t *Type;
	const char *Path;
	stringmap_t Exports[1];
} ml_module_t;

ml_value_t *ml_module(const char *Path, ...) __attribute__ ((malloc, sentinel));
const char *ml_module_path(ml_value_t *Module) __attribute__ ((pure));
ml_value_t *ml_module_import(ml_value_t *Module, const char *Name) __attribute__ ((pure));
ml_value_t *ml_module_export(ml_value_t *Module, const char *Name, ml_value_t *Value);

ml_value_t *ml_callable_module(const char *Path, ml_value_t *Fn, ...) __attribute__ ((malloc, sentinel));

/// @}

/// \defgroup externals
/// @{
///

// Externals //

extern ml_type_t MLExternalT[];
extern ml_type_t MLExternalSetT[];

typedef struct {
	ml_type_t *Type;
	const char *Name;
	const char *Source;
	stringmap_t Exports[1];
	int Length, Line;
} ml_external_t;

ml_value_t *ml_external(const char *Name, const char *Source, int Line) __attribute__ ((malloc));

typedef struct ml_externals_t ml_externals_t;

struct ml_externals_t {
	ml_type_t *Type;
	ml_externals_t *Next;
	inthash_t Values[1];
	stringmap_t Names[1];
};

extern ml_externals_t MLExternals[1];
const char *ml_externals_get_name(ml_externals_t *Externals, ml_value_t *Value);
ml_value_t *ml_externals_get_value(ml_externals_t *Externals, const char *Name);
void ml_externals_add(ml_externals_t *Externals, const char *Name, void *Value);

void ml_externals_default_add(const char *Name, void *Value);

ml_value_t *ml_serialize(ml_value_t *Value);

typedef ml_value_t *(*ml_deserializer_t)(const char *Type, int Count, ml_value_t **Args);

void ml_deserializer_define(const char *Type, ml_deserializer_t Deserializer);
ml_value_t *ml_deserialize(const char *Type, int Count, ml_value_t **Args);

#ifndef GENERATE_INIT

#define ML_DESERIALIZER(TYPE) static ml_value_t *CONCAT3(ml_deserializer_, __LINE__, __COUNTER__)(const char *Type, int Count, ml_value_t **Args)

#else

#define ML_DESERIALIZER(TYPE) INIT_CODE ml_deserializer_define(TYPE, CONCAT3(ml_deserializer_, __LINE__, __COUNTER__));

#endif

/// @}

/// \defgroup symbols
/// @{
///

// Symbols //

typedef struct {
	ml_type_t *Type;
	const char *Name;
} ml_symbol_t;

extern ml_type_t MLSymbolT[];

ml_value_t *ml_symbol(const char *Name);

#define ml_symbol_name(VALUE) ((ml_symbol_t *)VALUE)->Name

typedef struct {
	ml_type_t *Type;
	const char *First, *Last;
} ml_symbol_interval_t;

extern ml_type_t MLSymbolIntervalT[];

#define ml_symbol_interval_first(VALUE) ((ml_symbol_interval_t *)VALUE)->First
#define ml_symbol_interval_last(VALUE) ((ml_symbol_interval_t *)VALUE)->Last

/// @}

/// \defgroup init
/// @{
///

// Init //

void ml_types_init(stringmap_t *Globals);

#ifdef __cplusplus
}

template <typename T> inline T *ml_typed_fn_get(ml_type_t *Type, T *Key) {
	return (T *)ml_typed_fn_get(Type, (void *)Key);
}

template <typename... args> void ml_method_by_auto(const char *Cached, void *Data, ml_callback_t Function, args... Args) {
	ml_method_definev(ml_method(Cached), ml_cfunction(Data, Function), NULL, Args...);
}

template <typename... args> void ml_method_by_auto(ml_value_t *Cached, void *Data, ml_callback_t Function, args... Args) {
	ml_method_definev(Cached, ml_cfunction(Data, Function), NULL, Args...);
}

template <typename... args> void ml_method_by_auto(ml_type_t *Type, void *Data, ml_callback_t Function, args... Args) {
	ml_method_definev(Type->Constructor, ml_cfunction(Data, Function), NULL, Args...);
}

template <typename... args> void ml_methodx_by_auto(const char *Cached, void *Data, ml_callbackx_t Function, args... Args) {
	ml_method_definev(ml_method(Cached), ml_cfunctionx(Data, Function), NULL, Args...);
}

template <typename... args> void ml_methodx_by_auto(ml_value_t *Cached, void *Data, ml_callbackx_t Function, args... Args) {
	ml_methodx_define(Cached, ml_cfunctionx(Data, Function), NULL, Args...);
}

template <typename... args> void ml_methodx_by_auto(ml_type_t *Type, void *Data, ml_callbackx_t Function, args... Args) {
	ml_methodx_define(Type->Constructor, ml_cfunctionx(Data, Function), NULL, Args...);
}

#endif

/// @}

#endif

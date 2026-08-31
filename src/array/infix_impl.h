#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <stdint.h>

#ifdef COMPLEX_OPERATIONS

#include <complex.h>

typedef complex float complex_float;
typedef complex double complex_double;

#endif

typedef void (*operation_t)(void *, const void *, int);

#define OPERATION_IMPL_FNS(NAME, OP, TARGET, SOURCE) \
\
static void operation_ ## NAME ## _ ## TARGET ## _ ## SOURCE(void *Target, const void *Source, int Count) { \
	TARGET *Target1 = (TARGET *)Target; \
	SOURCE *Source1 = (SOURCE *)Source; \
	for (int I = Count; --I >= 0; ++Source1, ++Target1) *Target1 = OP(*Target1, *Source1); \
} \
\
static void operation_ ## NAME ## _ ## TARGET ## _ ## SOURCE ## 1(void *Target, const void *Source, int Count) { \
	TARGET *Target1 = (TARGET *)Target; \
	SOURCE Source1 = *(SOURCE *)Source; \
	for (int I = Count; --I >= 0; ++Source1, ++Target1) *Target1 = OP(*Target1, Source1); \
}

#define OPERATION_IMPL_TARGET_BASE(NAME, OP, TARGET) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, uint8_t) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, int8_t) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, uint16_t) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, int16_t) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, uint32_t) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, int32_t) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, uint64_t) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, int64_t)

#if defined(COMPLEX_OPERATIONS)
#define OPERATION_IMPL_TARGET(NAME, OP, TARGET) \
OPERATION_IMPL_TARGET_BASE(NAME, OP, TARGET) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, float) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, double) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, complex_float) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, complex_double)
#elif defined(REAL_OPERATIONS)
#define OPERATION_IMPL_TARGET(NAME, OP, TARGET) \
OPERATION_IMPL_TARGET_BASE(NAME, OP, TARGET) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, float) \
OPERATION_IMPL_FNS(NAME, OP, TARGET, double)
#else
#define OPERATION_IMPL_TARGET(NAME, OP, TARGET) \
OPERATION_IMPL_TARGET_BASE(NAME, OP, TARGET)
#endif

#define OPERATION_IMPL_BASE(NAME, OP) \
OPERATION_IMPL_TARGET(NAME, OP, uint8_t) \
OPERATION_IMPL_TARGET(NAME, OP, int8_t) \
OPERATION_IMPL_TARGET(NAME, OP, uint16_t) \
OPERATION_IMPL_TARGET(NAME, OP, int16_t) \
OPERATION_IMPL_TARGET(NAME, OP, uint32_t) \
OPERATION_IMPL_TARGET(NAME, OP, int32_t) \
OPERATION_IMPL_TARGET(NAME, OP, uint64_t) \
OPERATION_IMPL_TARGET(NAME, OP, int64_t) \
OPERATION_IMPL_TARGET(NAME, OP, float) \
OPERATION_IMPL_TARGET(NAME, OP, double)

#if defined(COMPLEX_OPERATIONS)
#define OPERATION_IMPL(NAME, OP) \
OPERATION_IMPL_BASE(NAME, OP) \
OPERATION_IMPL_TARGET(NAME, OP, float) \
OPERATION_IMPL_TARGET(NAME, OP, double) \
OPERATION_IMPL_TARGET(NAME, OP, complex_float) \
OPERATION_IMPL_TARGET(NAME, OP, complex_double)
#elif defined(REAL_OPERATIONS)
#define OPERATION_IMPL(NAME, OP) \
OPERATION_IMPL_BASE(NAME, OP) \
OPERATION_IMPL_TARGET(NAME, OP, float) \
OPERATION_IMPL_TARGET(NAME, OP, double)
#else
#define OPERATION_IMPL(NAME, OP) \
OPERATION_IMPL_BASE(NAME, OP)
#endif

#endif

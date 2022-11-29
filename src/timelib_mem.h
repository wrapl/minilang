#ifndef TIMELIB_MEM_H
#define TIMELIB_MEM_H

#include <gc/gc.h>
#include <string.h>

#define timelib_malloc GC_malloc
#define timelib_realloc GC_realloc
#define timelib_free nop_free
#define timelib_calloc GC_calloc
#define timelib_strdup GC_strdup

static inline void nop_free(void *Ptr) {}
static inline void *GC_calloc(size_t N, size_t M) {
	return GC_malloc(N * M);
}

static inline char *timelib_strndup(const char *S, size_t N) {
	char *C;
	size_t M;
	M = strlen(S);
	if (N > M) N = M;
	C = GC_malloc_atomic(N + 1);
	C[N] = 0;
	return memcpy(C, S, N);
}

#endif

#ifndef ML_UTILS_H
#define ML_UTILS_H

#include <string.h>

#if Mingw

extern void *memmem (const void *__haystack, size_t __haystacklen,
		     const void *__needle, size_t __needlelen);

static inline void *stpcpy(void *__restrict Dest, const void *__restrict Source) {
	size_t Length = strlen(Source);
	memcpy(Dest, Source, Length);
	return Dest + Length;
}

#endif

#ifdef Darwin

static inline void *stpcpy(void *__restrict Dest, const void *__restrict Source) {
	size_t Length = strlen(Source);
	memcpy(Dest, Source, Length);
	return Dest + Length;
}

static inline void *mempcpy(void *__restrict Dest, const void *__restrict Source, size_t Size) {
	memcpy(Dest, Source, Size);
	return Dest + Size;
}

#endif

#endif

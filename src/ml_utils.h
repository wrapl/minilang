#ifndef ML_UTILS_H
#define ML_UTILS_H

#include <string.h>

#ifdef Darwin

static inline void *mempcpy(void *__restrict Dest, const void *__restrict Source, size_t Size) {
	memcpy(Dest, Source, Size);
	return Dest + Size;
}

#endif

#endif

#include "ml_utils.h"
#include "ml_uuid.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef Mingw

void *memmem (const void *__haystack, size_t __haystacklen, const void *__needle, size_t __needlelen) {
	int C = *(unsigned char *)__needle;
	while (__haystacklen >= __needlelen) {
		const void *H = memchr(__haystack, C, __haystacklen + 1 - __needlelen);
		if (H == NULL) return NULL;
		if (!memcmp(H, __needle, __needlelen)) return H;
		__haystacklen -= (H + 1 - __haystack);
		__haystack = H + 1;
	}
	return NULL;
}

int uuid_compare(const uuid_t uu1, const uuid_t uu2) {
	return memcmp(uu1, uu2, 16);
}

void uuid_generate(uuid_t out) {
	long *P = (long *)out;
	for (int I = 0; I < sizeof(uuid_t) / sizeof(long); ++I) P[I] = rand();
}

int uuid_parse(const char *in, uuid_t uu) {

}

void uuid_unparse_lower(const uuid_t uu, char *out) {

}

char *strptime(const char *buf, const char *fmt, struct tm *tm) {

}

#endif

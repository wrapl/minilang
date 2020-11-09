#ifndef INTHASH_H
#define INTHASH_H

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct inthash_t inthash_t;

struct inthash_t {
	uintptr_t *Keys;
	void **Values;
	int Size, Space;
};

#define INTHASH_INIT {NULL, 0, 0}

inthash_t *inthash_new() __attribute__ ((malloc));

void *inthash_search(const inthash_t *Map, uintptr_t Key) __attribute__ ((pure));
void *inthash_insert(inthash_t *Map, uintptr_t Key, void *Value);
int inthash_foreach(inthash_t *Map, void *Data, int (*callback)(uintptr_t, void *, void *));

typedef struct {void *Value; int Present;} inthash_result_t;

inthash_result_t inthash_search2(const inthash_t *Map, uintptr_t Key) __attribute__ ((pure));

#ifdef	__cplusplus
}
#endif


#endif

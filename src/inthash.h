#ifndef INTHASH_H
#define INTHASH_H

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct inthash_t inthash_t;
typedef struct inthash_node_t inthash_node_t;

struct inthash_t {
	inthash_node_t *Nodes;
	int Size, Space;
};

#define INTHASH_INIT {NULL, 0, 0}

inthash_t *inthash_new();

void *inthash_search(const inthash_t *Map, uintptr_t Key);
void *inthash_insert(inthash_t *Map, uintptr_t Key, void *Value);
int inthash_foreach(inthash_t *Map, void *Data, int (*callback)(uintptr_t, void *, void *));

#ifdef	__cplusplus
}
#endif


#endif

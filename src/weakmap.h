#ifndef WEAKMAP_H
#define WEAKMAP_H

#include <stdlib.h>

typedef struct weakmap_node_t weakmap_node_t;

typedef struct {
	weakmap_node_t *Nodes;
	size_t Mask, Space; //, Deleted;
} weakmap_t;

#define WEAKMAP_INIT (weakmap_t){NULL, 0, 0}

void *weakmap_insert(weakmap_t *Map, const char *Key, int Length, void *(*missing)(const char *, int));

int weakmap_foreach(weakmap_t *Map, void *Data, int (*callback)(const char *, void *, void *));

#endif

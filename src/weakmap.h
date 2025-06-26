#ifndef WEAKMAP_H
#define WEAKMAP_H

#include "ml_config.h"
#include <stdlib.h>

#ifdef ML_THREADSAFE

#include <pthread.h>

#endif

typedef struct weakmap_node_t weakmap_node_t;

typedef struct {
	weakmap_node_t *Nodes;
#ifdef ML_THREADSAFE
	pthread_mutex_t Lock[1];
#endif
	size_t Mask, Space; //, Deleted;
} weakmap_t;

#ifdef ML_THREADSAFE

#define WEAKMAP_INIT (weakmap_t){NULL, {PTHREAD_MUTEX_INITIALIZER}, 0, 0}

#else

#define WEAKMAP_INIT (weakmap_t){NULL, 0, 0}

#endif

void *weakmap_insert(weakmap_t *Map, const char *Key, int Length, void *(*missing)(const char *, int));

int weakmap_foreach(weakmap_t *Map, void *Data, int (*callback)(const char *, void *, void *));

int weakmap_check(weakmap_t *Map);

#endif

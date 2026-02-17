#ifndef STRINGMAP_H
#define STRINGMAP_H

typedef struct stringmap_node_t stringmap_node_t;

typedef struct {
	stringmap_node_t *Nodes;
	int Size, Mask;
} stringmap_t;

#define STRINGMAP_INIT (stringmap_t){0,}

stringmap_t *stringmap_new() __attribute__ ((malloc));
stringmap_t *stringmap_copy(stringmap_t *Map) __attribute__ ((malloc));

void *stringmap_search(const stringmap_t *Map, const char *Key) __attribute__ ((pure));
void *stringmap_insert(stringmap_t *Map, const char *Key, void *Value);
void *stringmap_remove(stringmap_t *Map, const char *Key);
void **stringmap_slot(stringmap_t *Map, const char *Key);
int stringmap_foreach(stringmap_t *Map, void *Data, int (*callback)(const char *, void *, void *));

unsigned long stringmap_hash(const char *Key) __attribute__ ((pure));

#endif

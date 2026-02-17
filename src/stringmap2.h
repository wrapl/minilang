#ifndef STRINGMAP2_H
#define STRINGMAP2_H

typedef struct stringmap2_node_t stringmap2_node_t;

typedef struct {
	stringmap2_node_t *Nodes;
	int Size, Mask;
} stringmap2_t;

#define STRINGMAP2_INIT (stringmap2_t){0,}

stringmap2_t *stringmap2_new() __attribute__ ((malloc));
stringmap2_t *stringmap2_copy(stringmap2_t *Map) __attribute__ ((malloc));

void *stringmap2_search(const stringmap2_t *Map, const char *Key) __attribute__ ((pure));
void *stringmap2_insert(stringmap2_t *Map, const char *Key, void *Value);
void *stringmap2_remove(stringmap2_t *Map, const char *Key);
void **stringmap2_slot(stringmap2_t *Map, const char *Key);
int stringmap2_foreach(stringmap2_t *Map, void *Data, int (*callback)(const char *, void *, void *));

#endif

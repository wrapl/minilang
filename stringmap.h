#ifndef STRINGMAP_H
#define STRINGMAP_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct stringmap_t stringmap_t;
typedef struct stringmap_node_t stringmap_node_t;

struct stringmap_t {
	stringmap_node_t *Root;
	int Size;
};

#define STRINGMAP_INIT {0,}

stringmap_t *stringmap_new();

void *stringmap_search(stringmap_t *Map, const char *Key);
void *stringmap_insert(stringmap_t *Map, const char *Key, void *Value);
void *stringmap_remove(stringmap_t *Map, const char *Key);
void **stringmap_slot(stringmap_t *Map, const char *Key);
int stringmap_foreach(stringmap_t *Map, void *Data, int (*callback)(const char *, void *, void *));

unsigned long stringmap_hash(const char *Key);
void *stringmap_hash_insert(stringmap_t *Map, long Hash, const char *Key, void *Value);
void *stringmap_hash_search(stringmap_t *Map, long Hash, const char *Key);
void *stringmap_hash_remove(stringmap_t *Map, long Hash, const char *Key);

#ifdef	__cplusplus
}
#endif

#endif

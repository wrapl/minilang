#ifndef STRINGMAP_H
#define STRINGMAP_H

typedef struct stringmap_t stringmap_t;
typedef struct stringmap_node_t stringmap_node_t;

struct stringmap_t {
	stringmap_node_t *Root;
	int Size;
};

#define STRINGMAP_INIT (stringmap_t){0,}

void *stringmap_search(stringmap_t *Tree, const char *Key);
void *stringmap_insert(stringmap_t *Tree, const char *Key, void *Value);
void *stringmap_remove(stringmap_t *Tree, const char *Key);
int stringmap_foreach(stringmap_t *Tree, void *Data, int (*callback)(const char *, void *, void *));

#endif

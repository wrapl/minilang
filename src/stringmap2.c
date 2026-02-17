#include "stringmap2.h"
#include "ml_macros.h"
#include <string.h>

unsigned long stringmap_hash(const char *Key) {
	unsigned long Hash = 5381;
	for (const char *P = Key; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

struct stringmap_node_t {
	const char *Key;
	void *Value;
	unsigned long Hash;
	int Offset;
};

stringmap_t *stringmap_new() {
	return new(stringmap_t);
}

stringmap_t *stringmap_copy(stringmap_t *Map) {
}

void *stringmap_search(const stringmap_t *Map, const char *Key) {

}

void *stringmap_insert(stringmap_t *Map, const char *Key, void *Value) {

}

void *stringmap_remove(stringmap_t *Map, const char *Key) {

}

void **stringmap_slot(stringmap_t *Map, const char *Key) {

}

int stringmap_foreach(stringmap_t *Map, void *Data, int (*callback)(const char *, void *, void *)) {

}

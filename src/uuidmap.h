#ifndef UUIDMAP_H
#define UUIDMAP_H

/// \defgroup uuidmap
/// @{
///

#include <uuid/uuid.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct uuidmap_t uuidmap_t;
typedef struct uuidmap_node_t uuidmap_node_t;

struct uuidmap_t {
	uuidmap_node_t *Root;
	int Size;
};

#define UUIDMAP_INIT (uuidmap_t){0,}

uuidmap_t *uuidmap_new() __attribute__ ((malloc));
uuidmap_t *uuidmap_copy(uuidmap_t *Map) __attribute__ ((malloc));

void *uuidmap_search(const uuidmap_t *Map, const uuid_t Key) __attribute__ ((pure));
void *uuidmap_insert(uuidmap_t *Map, const uuid_t Key, void *Value);
void *uuidmap_remove(uuidmap_t *Map, const uuid_t Key);
void **uuidmap_slot(uuidmap_t *Map, const uuid_t Key);
int uuidmap_foreach(uuidmap_t *Map, void *Data, int (*callback)(const uuid_t, void *, void *));

#ifdef __cplusplus
}
#endif

/// @}

#endif

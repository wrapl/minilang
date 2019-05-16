#ifndef POINTERSET_H
#define POINTERSET_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct pointerset_t pointerset_t;

struct pointerset_t {
	void **Pointers;
	int Size, Space;
};

#define POINTERSET_INIT {0,}

void pointerset_init(pointerset_t *Set, int Size);
int pointerset_insert(pointerset_t *Set, void *Pointer);
int pointerset_foreach(pointerset_t *Set, void *Data, int (*callback)(void *Pointer, void *Data));

#define targetset_size(Set) (Set->Size - Set->Space)

#ifdef	__cplusplus
}
#endif

#endif

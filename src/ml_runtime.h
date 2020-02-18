#ifndef ML_INTERNAL_H
#define ML_INTERNAL_H

#include "ml_types.h"

struct ml_reference_t {
	const ml_type_t *Type;
	ml_value_t **Address;
	ml_value_t *Value[];
};

typedef struct ml_slot_t ml_slot_t;

struct ml_slot_t {
	ml_slot_t *Next;
	ml_value_t **Value;
};

typedef struct {
	const ml_type_t *Type;
	ml_slot_t *Slots;
} ml_uninitialized_t;

ml_value_t *ml_string_new(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_list_new(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_tuple_new(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_map_new(void *Data, int Count, ml_value_t **Args);

void ml_runtime_init();

#endif

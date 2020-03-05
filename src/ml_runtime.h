#ifndef ML_INTERNAL_H
#define ML_INTERNAL_H

#include "ml_types.h"

extern ml_type_t MLReferenceT[];
extern ml_type_t MLUninitializedT[];

typedef ml_value_t *(*ml_getter_t)(void *Data, const char *Name);
typedef ml_value_t *(*ml_setter_t)(void *Data, const char *Name, ml_value_t *Value);

typedef struct ml_reference_t ml_reference_t;

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

typedef struct ml_source_t {
	const char *Name;
	int Line;
} ml_source_t;

/****************************** Errors ******************************/

typedef struct ml_error_t ml_error_t;

extern ml_type_t MLErrorT[];
extern ml_type_t MLErrorValueT[];

ml_value_t *ml_error(const char *Error, const char *Format, ...) __attribute__ ((format(printf, 2, 3)));
const char *ml_error_type(ml_value_t *Value);
const char *ml_error_message(ml_value_t *Value);
int ml_error_trace(ml_value_t *Value, int Level, const char **Source, int *Line);
void ml_error_trace_add(ml_value_t *Error, ml_source_t Source);
void ml_error_print(ml_value_t *Error);

ml_value_t *ml_reference(ml_value_t **Address);

ml_value_t *ml_string_fn(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_list_fn(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_tuple_fn(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_map_fn(void *Data, int Count, ml_value_t **Args);

void ml_runtime_init();


#endif

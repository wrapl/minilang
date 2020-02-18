#ifndef ML_OBJECT_H
#define ML_OBJECT_H

#include "ml_types.h"
#include "stringmap.h"

#ifdef	__cplusplus
extern "C" {
#endif

void ml_object_init(stringmap_t *Globals);

ml_value_t *ml_field_fn(void *Data, int Count, ml_value_t **Args);

#define ml_field(TYPE, FIELD) ml_function(&((TYPE *)0)->FIELD, ml_field_fn)

extern ml_type_t MLClassT[];
extern ml_type_t MLObjectT[];

size_t ml_class_size(ml_value_t *Value);
ml_value_t *ml_class_field(ml_value_t *Value, size_t Field);

size_t ml_object_size(ml_value_t *Value);
ml_value_t *ml_object_field(ml_value_t *Value, size_t Field);

#ifdef __cplusplus
}
#endif

#endif

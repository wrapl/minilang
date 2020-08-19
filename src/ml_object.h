#ifndef ML_OBJECT_H
#define ML_OBJECT_H

#include "ml_types.h"
#include "stringmap.h"

#ifdef	__cplusplus
extern "C" {
#endif

void ml_object_init(stringmap_t *Globals);

ml_value_t *ml_field_fn(void *Data, int Count, ml_value_t **Args) __attribute__ ((malloc));

#define ml_field(TYPE, FIELD) ml_function(&((TYPE *)0)->FIELD, ml_field_fn)

extern ml_type_t MLClassT[];
extern ml_type_t MLObjectT[];

const char *ml_class_name(const ml_value_t *Value) __attribute__ ((pure));
size_t ml_class_size(const ml_value_t *Value) __attribute__ ((pure));
ml_value_t *ml_class_field(const ml_value_t *Value, size_t Field) __attribute__ ((pure));

ml_value_t *ml_object_class(const ml_value_t *Value) __attribute__ ((pure));
size_t ml_object_size(const ml_value_t *Value) __attribute__ ((pure));
ml_value_t *ml_object_field(const ml_value_t *Value, size_t Field) __attribute__ ((pure));

#ifdef __cplusplus
}
#endif

#endif

#ifndef ML_OBJECT_H
#define ML_OBJECT_H

#include "stringmap.h"

#ifdef	__cplusplus
extern "C" {
#endif

void ml_object_init(void *Globals, ml_setter_t GlobalSet);

ml_value_t *ml_field_fn(void *Data, int Count, ml_value_t **Args);

#define ml_field(TYPE, FIELD) ml_function(&((TYPE *)0)->FIELD, ml_field_fn)

#ifdef __cplusplus
}
#endif

#endif

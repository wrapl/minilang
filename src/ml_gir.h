#ifndef ML_GIR_H
#define ML_GIR_H

#include "minilang.h"
#include <girepository.h>

extern ml_type_t TypelibT[];

GITypelib *ml_gir_get_typelib(ml_value_t *Value);
const char *ml_gir_get_namespace(ml_value_t *Value);
ml_value_t *ml_gir_import(ml_value_t *Value, const char *Name);

void ml_gir_init(stringmap_t *Globals);

ml_value_t *ml_gir_instance_get(void *Handle, GIBaseInfo *Info);

void ml_gir_queue_add(ml_state_t *State, ml_value_t *Value);
ml_schedule_t ml_gir_scheduler(ml_context_t *Context);

#endif

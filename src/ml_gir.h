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

#endif

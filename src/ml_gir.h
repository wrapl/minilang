#ifndef ML_GIR_H
#define ML_GIR_H

#include "minilang.h"
#include <girepository.h>

void ml_gir_init(stringmap_t *Globals);

ml_value_t *ml_gir_instance_get(void *Handle, GIBaseInfo *Info);

#endif

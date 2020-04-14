#ifndef ML_MODULE_H
#define ML_MODULE_H

#include "minilang.h"

void ml_module_init(stringmap_t *Globals);

void ml_module_load_file(ml_state_t *Caller, const char *FileName, ml_getter_t GlobalGet, void *Globals, ml_value_t **Slot);

#endif

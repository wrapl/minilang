#ifndef ML_MODULE_H
#define ML_MODULE_H

#include "minilang.h"

typedef struct ml_module_t ml_module_t;

void ml_module_init(stringmap_t *Globals);

ml_module_t *ml_module(const char *Name, ...) __attribute__ ((sentinel));
void ml_module_export(ml_module_t *Module, const char *Name, ml_value_t *Value);

#endif

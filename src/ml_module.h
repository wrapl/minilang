#ifndef ML_MODULE_H
#define ML_MODULE_H

#include "minilang.h"

#define MLMF_USE_GLOBALS 1

void ml_module_init(stringmap_t *Globals);

void ml_module_compile(ml_state_t *Caller, ml_parser_t *Parser, ml_compiler_t *Compiler, ml_value_t **Slot);
void ml_module_compile2(ml_state_t *Caller, ml_parser_t *Parser, ml_compiler_t *Compiler, ml_value_t **Slot, int Flags);

#endif

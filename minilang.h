#ifndef MINILANG_H
#define MINILANG_H

#include "sha256.h"
#include <stdlib.h>
#include <unistd.h>

#include "ml_types.h"
#include "ml_compiler.h"
#include "ml_runtime.h"
#include "ml_macros.h"

void ml_init();

ml_value_t *ml_load(ml_getter_t GlobalGet, void *Globals, const char *FileName);
ml_value_t *ml_call(ml_value_t *Value, int Count, ml_value_t **Args);
ml_value_t *ml_inline(ml_value_t *Value, int Count, ...);

#endif

#ifndef MINILANG_H
#define MINILANG_H

#include <stdlib.h>
#include <unistd.h>

#include "sha256.h"
#include "ml_compiler.h"

#ifdef	__cplusplus
extern "C" {
#endif

void ml_init();

ml_value_t *ml_load(ml_getter_t GlobalGet, void *Globals, const char *FileName);
ml_value_t *ml_call(ml_value_t *Value, int Count, ml_value_t **Args);
ml_value_t *ml_inline(ml_value_t *Value, int Count, ...);

#ifdef __cplusplus
}
#endif

#endif

#ifndef MINILANG_H
#define MINILANG_H

#include <stdlib.h>
#include <unistd.h>

#include "ml_compiler.h"

#ifdef	__cplusplus
extern "C" {
#endif

void ml_init();

void ml_load(ml_state_t *Caller, ml_getter_t GlobalGet, void *Globals, const char *FileName, const char *Parameters[]);

#ifdef __cplusplus
}
#endif

#endif

#ifndef MINILANG_H
#define MINILANG_H

#include <stdlib.h>
#include <unistd.h>

#include "ml_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

extern stringmap_t MLGlobals[];

void ml_init(const char *ExecName, stringmap_t *Globals);

#define MINILANG_VERSION 2, 11, 3

#ifdef __cplusplus
}
#endif

#endif

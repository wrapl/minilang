#ifndef MINILANG_H
#define MINILANG_H

#include <stdlib.h>
#include <unistd.h>

#include "ml_compiler.h"

#ifdef	__cplusplus
extern "C" {
#endif

void ml_init(stringmap_t *Globals);

#ifdef __cplusplus
}
#endif

#endif

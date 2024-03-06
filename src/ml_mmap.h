#ifndef ML_MMAP_H
#define ML_MMAP_H

#include <stdio.h>
#include "minilang.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ml_type_t MLMMapT[];

void ml_mmap_init(stringmap_t *Globals);

#ifdef __cplusplus
}
#endif

#endif

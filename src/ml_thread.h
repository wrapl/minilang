#ifndef ML_THREAD_H
#define ML_THREAD_H

#include "minilang.h"
#include <pthread.h>

#ifdef	__cplusplus
extern "C" {
#endif

extern ml_type_t MLThreadT[];

void ml_thread_init(stringmap_t *Globals);

#ifdef	__cplusplus
}
#endif

#endif

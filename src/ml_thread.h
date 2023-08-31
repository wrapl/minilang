#ifndef ML_THREAD_H
#define ML_THREAD_H

#include "minilang.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

extern ml_type_t MLThreadT[];

void ml_thread_init(stringmap_t *Globals);
ml_value_t *ml_is_threadsafe(ml_value_t *Value);

#ifdef __cplusplus
}
#endif

#endif

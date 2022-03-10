#ifndef ML_PQUEUE_H
#define ML_PQUEUE_H

#include "stringmap.h"
#include "ml_types.h"

#ifdef __cplusplus
extern "C" {
#endif

ml_value_t *ml_pqueue(ml_value_t *Compare);

void ml_pqueue_init(stringmap_t *Globals);

#ifdef __cplusplus
}
#endif

#endif

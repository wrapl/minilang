#ifndef ML_QUEUE_H
#define ML_QUEUE_H

#include "stringmap.h"
#include "ml_types.h"

#ifdef __cplusplus
extern "C" {
#endif

ml_value_t *ml_queue(ml_value_t *Compare);

void ml_queue_init(stringmap_t *Globals);

#ifdef __cplusplus
}
#endif

#endif

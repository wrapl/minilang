#ifndef ML_ITERFNS_H
#define ML_ITERFNS_H

#include "stringmap.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct ml_iter_state_t {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Values[];
} ml_iter_state_t;

void ml_iterfns_init(stringmap_t *Globals);
ml_value_t *ml_chained(int Count, ml_value_t **Functions);

#ifdef __cplusplus
}
#endif

#endif

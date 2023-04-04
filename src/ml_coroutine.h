#ifndef ML_COROUTINE_H
#define ML_COROUTINE_H

#include "ml_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void ml_coro_create(ml_state_t *Caller, ml_callback_t Function, int Count, ml_value_t **Args);

ml_value_t *ml_coro_call(ml_value_t *Function, int Count, ml_value_t **Args);

#ifdef __cplusplus
}
#endif

#endif

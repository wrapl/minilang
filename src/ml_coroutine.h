#ifndef ML_COROUTINE_H
#define ML_COROUTINE_H

#include "ml_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void ml_coro_enter(ml_state_t *Caller, ml_callback_t Function, int Count, ml_value_t **Args);

ml_value_t *ml_coro_call(ml_value_t *Function, int Count, ml_value_t **Args);

typedef struct ml_coro_state_t ml_coro_state_t;

void *ml_coro_escape(void *Data, void (*Callback)(ml_coro_state_t *, void *));

void ml_coro_resume(ml_coro_state_t *State, void *Data);

#ifdef __cplusplus
}
#endif

#endif

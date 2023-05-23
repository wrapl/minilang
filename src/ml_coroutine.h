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

extern ml_type_t MLCoFunctionT[];

#define ML_COFUNCTION2(NAME, FUNCTION) static ml_value_t *FUNCTION(ml_state_t *Caller, int Count, ml_value_t **Args); \
\
ml_cfunction_t NAME[1] = {{MLCoFunctionT, FUNCTION, NULL}}; \
\
static ml_value_t *FUNCTION(ml_state_t *Caller, int Count, ml_value_t **Args)

#define ML_COFUNCTION(NAME) ML_COFUNCTION2(NAME, CONCAT3(ml_cofunction_, __LINE__, __COUNTER__))

#ifdef __cplusplus
}
#endif

#endif

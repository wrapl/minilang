#ifndef ML_METHOD_H
#define ML_METHOD_H

#include "minilang.h"

typedef struct ml_method_cached_t ml_method_cached_t;

struct ml_method_cached_t {
	ml_method_cached_t *Next, *MethodNext;
	ml_method_t *Method;
	ml_value_t *Callback;
	int Count, Score;
	ml_type_t *Types[];
};

ml_method_cached_t *ml_method_search_cached(ml_state_t *Caller, ml_method_t *Method, int Count, ml_value_t **Args);

ml_value_t *ml_no_method_error(ml_method_t *Method, int Count, ml_value_t **Args);

void ml_method_init();

#endif

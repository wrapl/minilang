#ifndef ML_RUNTIME_H
#define ML_RUNTIME_H

#include "ml_types.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct ml_source_t ml_source_t;

struct ml_source_t {
	const char *Name;
	int Line;
};

#define MAX_TRACE 16

struct ml_error_t {
	const ml_type_t *Type;
	const char *Error;
	const char *Message;
	ml_source_t Trace[MAX_TRACE];
};

void ml_error_trace_add(ml_value_t *Value, ml_source_t Source);

void ml_closure_debug(ml_value_t *Value);

#ifdef	__cplusplus
}
#endif

#endif

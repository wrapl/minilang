#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "stringmap.h"
#include "ml_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct interactive_debugger_t interactive_debugger_t;

ml_value_t *interactive_debugger(
	void (*Enter)(void *Data, interactive_debugger_t *Debugger),
	void (*Exit)(ml_state_t *Caller, void *Data),
	void (*Log)(void *Data, ml_value_t *Value),
	void *Data,
	ml_getter_t GlobalGet,
	void *Globals
);

ml_value_t *interactive_debugger_get(interactive_debugger_t *Debugger, const char *Name);
void interactive_debugger_resume(interactive_debugger_t *Debugger);

#ifdef __cplusplus
}
#endif

#endif

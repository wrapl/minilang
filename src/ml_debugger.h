#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "stringmap.h"
#include "ml_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct interactive_debugger_t interactive_debugger_t;

ml_value_t *interactive_debugger(
	void (*enter)(void *Data, interactive_debugger_t *Debugger, ml_source_t Source, int Index),
	void (*exit)(void *Data, interactive_debugger_t *Debugger, ml_state_t *Caller, int Index),
	void (*log)(void *Data, ml_value_t *Value),
	void *Data,
	ml_getter_t GlobalGet,
	void *Globals
) __attribute__ ((malloc));

ml_value_t *interactive_debugger_get(interactive_debugger_t *Debugger, const char *Name);
ml_source_t interactive_debugger_switch(interactive_debugger_t *Debugger, int Index);
void interactive_debugger_resume(interactive_debugger_t *Debugger);

#ifdef __cplusplus
}
#endif

#endif

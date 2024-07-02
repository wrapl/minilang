#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "stringmap.h"
#include "ml_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ml_interactive_debugger_t ml_interactive_debugger_t;

ml_value_t *ml_interactive_debugger(
	void (*enter)(void *Data, ml_interactive_debugger_t *Debugger, ml_source_t Source, int Index),
	void (*exit)(void *Data, ml_interactive_debugger_t *Debugger, ml_state_t *Caller, int Index),
	void (*log)(void *Data, ml_value_t *Value),
	void *Data,
	ml_getter_t GlobalGet,
	void *Globals
) __attribute__ ((malloc));

ml_value_t *ml_interactive_debugger_get(ml_interactive_debugger_t *Debugger, const char *Name);
ml_source_t ml_interactive_debugger_switch(ml_interactive_debugger_t *Debugger, int Index);
void ml_interactive_debugger_resume(ml_interactive_debugger_t *Debugger, int Index);

#ifdef __cplusplus
}
#endif

#endif

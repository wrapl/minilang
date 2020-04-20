#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "ml_bytecode.h"
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIZE_BITS (CHAR_BIT * sizeof(size_t))

typedef struct ml_debugger_t ml_debugger_t;

struct ml_debugger_t {
	void (*run)(ml_debugger_t *Debugger, ml_state_t *Frame, ml_value_t *Value);
	size_t *(*breakpoints)(ml_debugger_t *Debugger, const char *Source, int LineNo);
	ml_state_t *StepOverFrame;
	ml_state_t *StepOutFrame;
	size_t Revision;
	int StepIn:1;
	int BreakOnError:1;
};

#define ML_DEBUGGER_INDEX 1

void ml_debugger_init(stringmap_t *Globals);

#ifdef __cplusplus
}
#endif

#endif

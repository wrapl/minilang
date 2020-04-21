#ifndef ML_CONSOLE_H
#define ML_CONSOLE_H

#include "minilang.h"

#ifdef	__cplusplus
extern "C" {
#endif

extern ml_value_t MLConsoleBreak[];

void ml_console(ml_getter_t GlobalGet, void *Globals, const char *DefaultPrompt, const char *ContinuePrompt);

#ifdef	__cplusplus
}
#endif

#endif

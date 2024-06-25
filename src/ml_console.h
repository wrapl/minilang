#ifndef ML_CONSOLE_H
#define ML_CONSOLE_H

#include "minilang.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void ml_console(ml_context_t *Context, ml_getter_t GlobalGet, void *Globals, const char *DefaultPrompt, const char *ContinuePrompt);
void ml_file_console(ml_context_t *Context, ml_getter_t GlobalGet, void *Globals, const char *DefaultPrompt, const char *ContinuePrompt, FILE *Input, FILE *Output);

#ifdef __cplusplus
}
#endif

#endif

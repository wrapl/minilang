#ifndef DEBUGGER_H
#define DEBUGGER_H

#include "stringmap.h"

#ifdef __cplusplus
extern "C" {
#endif

void ml_debugger_init(stringmap_t *Globals);

#ifdef __cplusplus
}
#endif

#endif

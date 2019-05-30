#ifndef ML_OBJECT_H
#define ML_OBJECT_H

#include "stringmap.h"

#ifdef	__cplusplus
extern "C" {
#endif

void ml_object_init(void *Globals, ml_setter_t GlobalSet);

#ifdef __cplusplus
}
#endif

#endif

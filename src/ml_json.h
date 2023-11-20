#ifndef ML_JSON_H
#define ML_JSON_H

#include "minilang.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ml_type_t MLJsonT[];

void ml_json_init(stringmap_t *Globals);

#ifdef __cplusplus
}
#endif

#endif

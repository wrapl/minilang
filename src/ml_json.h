#ifndef ML_JSON_H
#define ML_JSON_H

#include "minilang.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ml_type_t MLJsonT[];

void ml_json_init(stringmap_t *Globals);

ml_value_t *ml_json_decode(const char *Json, size_t Size);
ml_value_t *ml_json_encode(ml_stringbuffer_t *Buffer, ml_value_t *Value);

#ifdef __cplusplus
}
#endif

#endif

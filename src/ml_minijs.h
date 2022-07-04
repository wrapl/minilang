#ifndef ML_JSENCODE_H
#define ML_JSENCODE_H

#include "minilang.h"
#include <jansson.h>

typedef struct ml_minijs_encoder_t ml_minijs_encoder_t;

json_t *ml_minijs_encode(ml_minijs_encoder_t *Cache, ml_value_t *Value);

void ml_minijs_init(stringmap_t *Globals);

#endif

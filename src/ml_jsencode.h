#ifndef ML_JSENCODE_H
#define ML_JSENCODE_H

#include "minilang.h"
#include <jansson.h>

typedef struct ml_json_encoder_cache_t ml_json_encoder_cache_t;
typedef struct ml_json_decoder_cache_t ml_json_decoder_cache_t;

ml_json_encoder_cache_t *ml_json_encoder(inthash_t *Special);
json_t *ml_json_encode(ml_json_encoder_cache_t *Encoder, ml_value_t *Value);
void ml_jsencode_init(stringmap_t *Globals);

#endif

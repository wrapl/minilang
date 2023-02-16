#ifndef ML_JSENCODE_H
#define ML_JSENCODE_H

#include "minilang.h"

typedef struct ml_minijs_encoder_t ml_minijs_encoder_t;

struct ml_minijs_encoder_t {
	ml_externals_t *Externals;
	inthash_t Cached[1];
	int LastIndex;
};

ml_value_t *ml_minijs_encode(ml_minijs_encoder_t *Encoder, ml_value_t *Value);

void ml_minijs_init(stringmap_t *Globals);

#endif

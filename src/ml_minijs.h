#ifndef ML_JSENCODE_H
#define ML_JSENCODE_H

#include "minilang.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value;
} ml_minijs_t;

extern ml_type_t MLMinijsT[];

typedef struct ml_minijs_encoder_t ml_minijs_encoder_t;

struct ml_minijs_encoder_t {
	ml_externals_t *Externals;
	inthash_t Cached[1];
	int LastIndex;
};

ml_value_t *ml_minijs_encode(ml_minijs_encoder_t *Encoder, ml_value_t *Value);

void ml_minijs_init(stringmap_t *Globals);

#ifdef __cplusplus
}
#endif

#endif

#ifndef ML_ARRAY_H
#define ML_ARRAY_H

#include "minilang.h"
#include "ml_cbor.h"

extern ml_type_t *MLArrayT;

void ml_array_init(stringmap_t *Globals);

int ml_array_degree(ml_value_t *Array);
int ml_array_size(ml_value_t *Array, int Dim);

#endif

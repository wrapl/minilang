#ifndef ML_CBOR_H
#define ML_CBOR_H

#include <stringmap.h>
#include <cbor.h>

void ml_cbor_init(stringmap_t *Globals);

cbor_item_t *ml_to_cbor_item(ml_value_t *Value);

typedef struct {void *Data; size_t Length;} ml_cbor_t;

ml_cbor_t ml_to_cbor(ml_value_t *Value);
ml_value_t *ml_from_cbor(ml_cbor_t Cbor, ml_value_t *TagFn);

#endif

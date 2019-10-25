#ifndef ML_CBOR_H
#define ML_CBOR_H

#include <minilang.h>
#include <stringmap.h>
#include <cbor.h>

void ml_cbor_init(stringmap_t *Globals);

cbor_item_t *ml_to_cbor_item(ml_value_t *Value);

typedef struct {const void *Data; size_t Length;} ml_cbor_t;

typedef ml_value_t *(ml_tag_t)(ml_value_t *Value, void *Data);

ml_cbor_t ml_to_cbor(ml_value_t *Value);
ml_value_t *ml_from_cbor(ml_cbor_t Cbor, ml_tag_t *(*TagFn)(void *TagFnData, void **TagData), void *TagFnData);

#endif

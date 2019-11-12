#ifndef ML_CBOR_H
#define ML_CBOR_H

#include <stdint.h>

#include "minilang.h"
#include "stringmap.h"

void ml_cbor_init(stringmap_t *Globals);

void ml_cbor_write(ml_value_t *Value, void *Data, int (*WriteFn)(void *Data, const unsigned char *Bytes, unsigned Size));

typedef struct ml_cbor_reader_t ml_cbor_reader_t;

typedef ml_value_t *(*ml_tag_t)(void *Data, ml_value_t *Value);

ml_cbor_reader_t *ml_cbor_reader_new(void *TagFnData, ml_tag_t (*TagFn)(void *, uint64_t, void **));
void ml_cbor_reader_read(ml_cbor_reader_t *Reader, unsigned char *Bytes, int Size);
ml_value_t *ml_cbor_reader_get(ml_cbor_reader_t *Reader);

typedef struct {const void *Data; size_t Length;} ml_cbor_t;

ml_cbor_t ml_to_cbor(ml_value_t *Value);
ml_value_t *ml_from_cbor(ml_cbor_t Cbor, void *TagFnData, ml_tag_t (*TagFn)(void *, uint64_t, void **));

#endif

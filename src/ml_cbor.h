#ifndef ML_CBOR_H
#define ML_CBOR_H

#include <stdint.h>

#include "minilang.h"
#include "stringmap.h"

#ifdef	__cplusplus
extern "C" {
#endif

void ml_cbor_init(stringmap_t *Globals);

typedef struct ml_cbor_reader_t ml_cbor_reader_t;

typedef ml_value_t *(*ml_tag_t)(ml_cbor_reader_t *Reader, ml_value_t *Value);

void ml_cbor_default_tag(long Tag, ml_tag_t TagFn);

ml_cbor_reader_t *ml_cbor_reader_new();
void ml_cbor_reader_read(ml_cbor_reader_t *Reader, unsigned char *Bytes, int Size);
ml_value_t *ml_cbor_reader_get(ml_cbor_reader_t *Reader);
int ml_cbor_reader_extra(ml_cbor_reader_t *Reader);

typedef void (*ml_cbor_write_fn)(void *Data, const unsigned char *Bytes, unsigned Size);

void ml_cbor_write_integer(void *Data, ml_cbor_write_fn WriteFn, int64_t Number);
void ml_cbor_write_positive(void *Data, ml_cbor_write_fn WriteFn, uint64_t Number);
void ml_cbor_write_negative(void *Data, ml_cbor_write_fn WriteFn, uint64_t Number);
void ml_cbor_write_bytes(void *Data, ml_cbor_write_fn WriteFn, unsigned Size);
void ml_cbor_write_indef_bytes(void *Data, ml_cbor_write_fn WriteFn);
void ml_cbor_write_string(void *Data, ml_cbor_write_fn WriteFn, unsigned Size);
void ml_cbor_write_indef_string(void *Data, ml_cbor_write_fn WriteFn);
void ml_cbor_write_array(void *Data, ml_cbor_write_fn WriteFn, unsigned Size);
void ml_cbor_write_indef_array(void *Data, ml_cbor_write_fn WriteFn);
void ml_cbor_write_map(void *Data, ml_cbor_write_fn WriteFn, unsigned Size);
void ml_cbor_write_indef_map(void *Data, ml_cbor_write_fn WriteFn);
void ml_cbor_write_float2(void *Data, ml_cbor_write_fn WriteFn, double Number);
void ml_cbor_write_float4(void *Data, ml_cbor_write_fn WriteFn, double Number);
void ml_cbor_write_float8(void *Data, ml_cbor_write_fn WriteFn, double Number);
void ml_cbor_write_simple(void *Data, ml_cbor_write_fn WriteFn, unsigned char Simple);
void ml_cbor_write_break(void *Data, ml_cbor_write_fn WriteFn);
void ml_cbor_write_tag(void *Data, ml_cbor_write_fn WriteFn, uint64_t Tag);

ml_value_t *ml_cbor_write(ml_value_t *Value, void *Data, ml_cbor_write_fn WriteFn);

typedef struct {
	union {
		const void *Data;
		ml_value_t *Error;
	};
	size_t Length;
} ml_cbor_t;

typedef struct ml_cbor_writer_t ml_cbor_writer_t;

void ml_cbor_writer_set(ml_cbor_writer_t *Writer, void *Key, void *Value);
void *ml_cbor_writer_get(ml_cbor_writer_t *Writer, void *Key);
ml_value_t *ml_cbor_writer_write(ml_cbor_writer_t *Writer, ml_value_t *Value);
ml_cbor_t ml_cbor_writer_encode(ml_value_t *Value);

ml_cbor_t ml_to_cbor(ml_value_t *Value);
ml_value_t *ml_from_cbor(ml_cbor_t Cbor);

typedef struct {ml_value_t *Value; int Extra;} ml_cbor_result_t;

ml_cbor_result_t ml_from_cbor_extra(ml_cbor_t Cbor);

#ifdef	__cplusplus
}
#endif

#endif

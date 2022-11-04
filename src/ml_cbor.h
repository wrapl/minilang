#ifndef ML_CBOR_H
#define ML_CBOR_H

#include <stdint.h>

#include "minilang.h"
#include "stringmap.h"

#ifdef __cplusplus
extern "C" {
#endif

void ml_cbor_init(stringmap_t *Globals);

void ml_cbor_default_object(const char *Name, ml_value_t *Constructor);

int ml_cbor_setting();

typedef struct ml_cbor_reader_t ml_cbor_reader_t;

typedef ml_value_t *(*ml_cbor_tag_fn)(ml_cbor_reader_t *Reader, ml_value_t *Value);
typedef struct ml_cbor_tag_fns_t ml_cbor_tag_fns_t;

ml_cbor_tag_fns_t *ml_cbor_tag_fns(int Default);
ml_cbor_tag_fns_t *ml_cbor_tag_fns_copy(ml_cbor_tag_fns_t *TagFns);
ml_cbor_tag_fn ml_cbor_tag_fn_get(ml_cbor_tag_fns_t *TagFns, uint64_t Tag);
void ml_cbor_tag_fn_set(ml_cbor_tag_fns_t *TagFns, uint64_t Tag, ml_cbor_tag_fn Fn);

void ml_cbor_default_tag(uint64_t Tag, ml_cbor_tag_fn TagFn);
void ml_cbor_default_global(const char *Name, void *Value);

ml_cbor_reader_t *ml_cbor_reader(ml_cbor_tag_fns_t *TagFns, ml_getter_t GlobalGet, void *Globals);
void ml_cbor_reader_reset(ml_cbor_reader_t *Reader);
void ml_cbor_reader_set_setting(ml_cbor_reader_t *Reader, int Key, void *Value);
void *ml_cbor_reader_get_setting(ml_cbor_reader_t *Reader, int Key);
int ml_cbor_reader_read(ml_cbor_reader_t *Reader, unsigned char *Bytes, int Size);
ml_value_t *ml_cbor_reader_get(ml_cbor_reader_t *Reader);
int ml_cbor_reader_extra(ml_cbor_reader_t *Reader);

typedef struct {
	union {
		const void *Data;
		ml_value_t *Error;
	};
	size_t Length;
} ml_cbor_t;

typedef void (*ml_cbor_write_fn)(void *Data, const unsigned char *Bytes, size_t Size);
typedef struct ml_cbor_writer_t ml_cbor_writer_t;

ml_cbor_writer_t *ml_cbor_writer(void *Data, ml_cbor_write_fn WriteFn, ml_externals_t *Externals);
void ml_cbor_writer_reset(ml_cbor_writer_t *Writer, void *Data);
void ml_cbor_writer_set_setting(ml_cbor_writer_t *Writer, int Setting, void *Value);
void *ml_cbor_writer_get_setting(ml_cbor_writer_t *Writer, int Setting);
void ml_cbor_writer_find_refs(ml_cbor_writer_t *Writer, ml_value_t *Value);
ml_cbor_t ml_cbor_writer_encode(ml_value_t *Value);

ml_value_t *ml_cbor_write(ml_cbor_writer_t *Writer, ml_value_t *Value);

void ml_cbor_write_integer(ml_cbor_writer_t *Writer, int64_t Number);
void ml_cbor_write_positive(ml_cbor_writer_t *Writer, uint64_t Number);
void ml_cbor_write_negative(ml_cbor_writer_t *Writer, uint64_t Number);
void ml_cbor_write_bytes(ml_cbor_writer_t *Writer, unsigned Size);
void ml_cbor_write_indef_bytes(ml_cbor_writer_t *Writer);
void ml_cbor_write_string(ml_cbor_writer_t *Writer, unsigned Size);
void ml_cbor_write_indef_string(ml_cbor_writer_t *Writer);
void ml_cbor_write_array(ml_cbor_writer_t *Writer, unsigned Size);
void ml_cbor_write_indef_array(ml_cbor_writer_t *Writer);
void ml_cbor_write_map(ml_cbor_writer_t *Writer, unsigned Size);
void ml_cbor_write_indef_map(ml_cbor_writer_t *Writer);
void ml_cbor_write_float2(ml_cbor_writer_t *Writer, double Number);
void ml_cbor_write_float4(ml_cbor_writer_t *Writer, double Number);
void ml_cbor_write_float8(ml_cbor_writer_t *Writer, double Number);
void ml_cbor_write_simple(ml_cbor_writer_t *Writer, unsigned char Simple);
void ml_cbor_write_break(ml_cbor_writer_t *Writer);
void ml_cbor_write_tag(ml_cbor_writer_t *Writer, uint64_t Tag);

void ml_cbor_write_raw(ml_cbor_writer_t *Writer, const void *Bytes, size_t Length);

ml_cbor_t ml_to_cbor(ml_value_t *Value);
ml_value_t *ml_from_cbor(ml_cbor_t Cbor, ml_cbor_tag_fns_t *TagFns);

typedef struct {ml_value_t *Value; int Extra;} ml_cbor_result_t;

ml_cbor_result_t ml_from_cbor_extra(ml_cbor_t Cbor, ml_cbor_tag_fns_t *TagFns);

#define ML_CBOR_TAG_TIME_STRING 0
#define ML_CBOR_TAG_TIME_EPOCH 1
#define ML_CBOR_TAG_OBJECT 27
#define ML_CBOR_TAG_MARK_REUSED 28
#define ML_CBOR_TAG_USE_PREVIOUS 29
#define ML_CBOR_TAG_REGEX 35
#define ML_CBOR_TAG_UUID 37
#define ML_CBOR_TAG_IDENTIFIER 39
#define ML_CBOR_TAG_MULTI_ARRAY 40
#define ML_CBOR_TAG_ARRAY_ANY 41
#define ML_CBOR_TAG_ARRAY_UINT8 64
#define ML_CBOR_TAG_ARRAY_UINT16_LE 69
#define ML_CBOR_TAG_ARRAY_UINT32_LE 70
#define ML_CBOR_TAG_ARRAY_UINT64_LE 71
#define ML_CBOR_TAG_ARRAY_INT8 72
#define ML_CBOR_TAG_ARRAY_INT16_LE 77
#define ML_CBOR_TAG_ARRAY_INT32_LE 78
#define ML_CBOR_TAG_ARRAY_INT64_LE 79
#define ML_CBOR_TAG_ARRAY_FLOAT32_LE 85
#define ML_CBOR_TAG_ARRAY_FLOAT64_LE 86
#define ML_CBOR_TAG_FINITE_SET 258
#define ML_CBOR_TAG_COMPLEX 43000
#define ML_CBOR_TAG_COMPLEX_ARRAY 43001

#ifdef __cplusplus
}
#endif

#endif

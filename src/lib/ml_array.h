#ifndef ML_ARRAY_H
#define ML_ARRAY_H

#include "../minilang.h"
#include "ml_cbor.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct ml_array_dimension_t {
	int Size, Stride;
	int *Indices;
} ml_array_dimension_t;

typedef enum {
	ML_ARRAY_FORMAT_NONE,
	ML_ARRAY_FORMAT_I8, ML_ARRAY_FORMAT_U8,
	ML_ARRAY_FORMAT_I16, ML_ARRAY_FORMAT_U16,
	ML_ARRAY_FORMAT_I32, ML_ARRAY_FORMAT_U32,
	ML_ARRAY_FORMAT_I64, ML_ARRAY_FORMAT_U64,
	ML_ARRAY_FORMAT_F32, ML_ARRAY_FORMAT_F64,
	ML_ARRAY_FORMAT_ANY
} ml_array_format_t;

extern size_t MLArraySizes[];

typedef struct ml_array_t {
	ml_buffer_t Base;
	int Degree, Format;
	ml_array_dimension_t Dimensions[];
} ml_array_t;

extern ml_type_t *MLArrayT;

ml_array_t *ml_array_new(ml_array_format_t Format, int Degree);
ml_array_t *ml_array(ml_array_format_t Format, int Degree, ...);
int ml_array_degree(ml_value_t *Array);
int ml_array_size(ml_value_t *Array, int Dim);

#define ML_ARRAY_ACCESSORS(CTYPE) \
CTYPE ml_array_get_ ## CTYPE (ml_array_t *Array, ...); \
void ml_array_set_ ## CTYPE (CTYPE Value, ml_array_t *Array, ...);

ML_ARRAY_ACCESSORS(int8_t)
ML_ARRAY_ACCESSORS(uint8_t)
ML_ARRAY_ACCESSORS(int16_t)
ML_ARRAY_ACCESSORS(uint16_t)
ML_ARRAY_ACCESSORS(int32_t)
ML_ARRAY_ACCESSORS(uint32_t)
ML_ARRAY_ACCESSORS(int64_t)
ML_ARRAY_ACCESSORS(uint64_t)
ML_ARRAY_ACCESSORS(float)
ML_ARRAY_ACCESSORS(double)

#ifdef	__cplusplus
}
#endif

#endif

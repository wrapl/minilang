#ifndef ML_ARRAY_H
#define ML_ARRAY_H

#include "minilang.h"
#include "ml_cbor.h"

typedef struct ml_array_dimension_t {
	int Size, Stride;
	int *Indices;
} ml_array_dimension_t;

typedef enum {
	ML_ARRAY_FORMAT_ANY,
	ML_ARRAY_FORMAT_I8, ML_ARRAY_FORMAT_U8,
	ML_ARRAY_FORMAT_I16, ML_ARRAY_FORMAT_U16,
	ML_ARRAY_FORMAT_I32, ML_ARRAY_FORMAT_U32,
	ML_ARRAY_FORMAT_I64, ML_ARRAY_FORMAT_U64,
	ML_ARRAY_FORMAT_F32, ML_ARRAY_FORMAT_F64
} ml_array_format_t;

typedef struct ml_array_t {
	ml_buffer_t Base;
	int Degree, Format;
	ml_array_dimension_t Dimensions[];
} ml_array_t;

extern ml_type_t *MLArrayT;

void ml_array_init(stringmap_t *Globals);

ml_array_t *ml_array_new(ml_array_format_t Format, int Degree);
int ml_array_degree(ml_value_t *Array);
int ml_array_size(ml_value_t *Array, int Dim);

#define ML_ARRAY_GETTER_DECL(CTYPE) \
CTYPE ml_array_get_ ## CTYPE (ml_value_t *Array, int Indices[])

ML_ARRAY_GETTER_DECL(int8_t);
ML_ARRAY_GETTER_DECL(uint8_t);
ML_ARRAY_GETTER_DECL(int16_t);
ML_ARRAY_GETTER_DECL(uint16_t);
ML_ARRAY_GETTER_DECL(int32_t);
ML_ARRAY_GETTER_DECL(uint32_t);
ML_ARRAY_GETTER_DECL(int64_t);
ML_ARRAY_GETTER_DECL(uint64_t);
ML_ARRAY_GETTER_DECL(float);
ML_ARRAY_GETTER_DECL(double);

#endif

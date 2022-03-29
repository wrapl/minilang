#ifndef ML_ARRAY_H
#define ML_ARRAY_H

#include "minilang.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int Size, Stride;
	int *Indices;
} ml_array_dimension_t;

typedef enum {
	ML_ARRAY_FORMAT_NONE,
	ML_ARRAY_FORMAT_U8, ML_ARRAY_FORMAT_I8,
	ML_ARRAY_FORMAT_U16, ML_ARRAY_FORMAT_I16,
	ML_ARRAY_FORMAT_U32, ML_ARRAY_FORMAT_I32,
	ML_ARRAY_FORMAT_U64, ML_ARRAY_FORMAT_I64,
	ML_ARRAY_FORMAT_F32, ML_ARRAY_FORMAT_F64,
#ifdef ML_COMPLEX
	ML_ARRAY_FORMAT_C32, ML_ARRAY_FORMAT_C64,
#endif
	ML_ARRAY_FORMAT_ANY
} ml_array_format_t;

#define MAX_FORMATS 16

extern size_t MLArraySizes[];

typedef struct {
	ml_address_t Base;
	int Degree;
	ml_array_format_t Format;
	ml_array_dimension_t Dimensions[];
} ml_array_t;

extern ml_type_t MLArrayT[];

void ml_array_init(stringmap_t *Globals);

ml_array_t *ml_array_alloc(ml_array_format_t Format, int Degree);
ml_array_t *ml_array(ml_array_format_t Format, int Degree, ...);
int ml_array_degree(ml_value_t *Array);
int ml_array_size(ml_value_t *Array, int Dim);
ml_value_t *ml_array_index(ml_array_t *Array, int Count, ml_value_t **Indices);

#define ML_ARRAY_ACCESSORS(CTYPE) \
CTYPE ml_array_get_ ## CTYPE (ml_array_t *Array, ...); \
void ml_array_set_ ## CTYPE (CTYPE Value, ml_array_t *Array, ...)

ML_ARRAY_ACCESSORS(int8_t);
ML_ARRAY_ACCESSORS(uint8_t);
ML_ARRAY_ACCESSORS(int16_t);
ML_ARRAY_ACCESSORS(uint16_t);
ML_ARRAY_ACCESSORS(int32_t);
ML_ARRAY_ACCESSORS(uint32_t);
ML_ARRAY_ACCESSORS(int64_t);
ML_ARRAY_ACCESSORS(uint64_t);
ML_ARRAY_ACCESSORS(float);
ML_ARRAY_ACCESSORS(double);

#ifdef ML_COMPLEX

ML_ARRAY_ACCESSORS(complex_float);
ML_ARRAY_ACCESSORS(complex_double);

#endif

#ifdef __cplusplus
}
#endif

#endif

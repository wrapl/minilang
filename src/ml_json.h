#ifndef ML_JSON_H
#define ML_JSON_H

#include "minilang.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ml_type_t MLJsonT[];

void ml_json_init(stringmap_t *Globals);

ml_value_t *ml_json_decode(const char *Json, size_t Size);
ml_value_t *ml_json_encode(ml_stringbuffer_t *Buffer, ml_value_t *Value);

typedef struct json_decoder_t json_decoder_t;

json_decoder_t *json_decoder(void (*emit)(json_decoder_t *Decoder, ml_value_t *Value), void *Data);
void *json_decoder_data(json_decoder_t *Decoder);
ml_value_t *json_decoder_parse(json_decoder_t *Decoder, const char *Input, size_t Size);

#ifdef __cplusplus
}
#endif

#endif

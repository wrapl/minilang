#ifndef ML_UUID_H
#define ML_UUID_H

#include "minilang.h"
#include <uuid/uuid.h>

#ifndef UUID_STR_LEN
#define UUID_STR_LEN 37
#endif

typedef struct {
	const ml_type_t *Type;
	uuid_t Value;
} ml_uuid_t;

extern ml_type_t MLUUIDT[];

void ml_uuid_init(stringmap_t *Globals);
ml_value_t *ml_uuid(const uuid_t Value);
ml_value_t *ml_uuid_parse(const char *Value, int Length);
#define ml_uuid_value(UUID) ((ml_uuid_t *)UUID)->Value

#endif

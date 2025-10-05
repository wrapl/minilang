#ifndef ML_UUID_H
#define ML_UUID_H

#include "minilang.h"

#ifdef Mingw

typedef unsigned char uuid_t[16];

extern int uuid_compare(const uuid_t uu1, const uuid_t uu2);
extern void uuid_generate(uuid_t out);
extern int uuid_parse(const char *in, uuid_t uu);
extern void uuid_unparse_lower(const uuid_t uu, char *out);

#ifndef uuid_t
#define uuid_t uuid_t
#endif

#else

#include <uuid/uuid.h>

#endif

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif

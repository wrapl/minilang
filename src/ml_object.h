#ifndef ML_OBJECT_H
#define ML_OBJECT_H

#include "ml_types.h"
#include "stringmap.h"

#ifdef	__cplusplus
extern "C" {
#endif

void ml_object_init(stringmap_t *Globals);

ml_value_t *ml_field_fn(void *Data, int Count, ml_value_t **Args) __attribute__ ((malloc));

#define ml_field(TYPE, FIELD) ml_function(&((TYPE *)0)->FIELD, ml_field_fn)

extern ml_type_t MLClassT[];
extern ml_type_t MLObjectT[];

size_t ml_class_size(const ml_value_t *Value) __attribute__ ((pure));
const char *ml_class_field_name(const ml_value_t *Value, size_t Field) __attribute__ ((pure));

ml_value_t *ml_object_class(const ml_value_t *Value) __attribute__ ((pure));
size_t ml_object_size(const ml_value_t *Value) __attribute__ ((pure));
ml_value_t *ml_object_field(const ml_value_t *Value, size_t Field) __attribute__ ((pure));

extern ml_type_t MLEnumT[];
extern ml_type_t MLEnumValueT[];

ml_type_t *ml_enum(const char *Name, ...);
ml_type_t *ml_enum2(const char *Name, ...);

ml_value_t *ml_enum_value(ml_type_t *Type, uint64_t Enum);
uint64_t ml_enum_value_value(ml_value_t *Value);
const char *ml_enum_value_name(ml_value_t *Value);

extern ml_type_t MLFlagsT[];
extern ml_type_t MLFlagsValueT[];

ml_type_t *ml_flags(const char *Name, ...);
ml_type_t *ml_flags2(const char *Name, ...);

ml_value_t *ml_flags_value(ml_type_t *Type, uint64_t Flags);
uint64_t ml_flags_value_value(ml_value_t *Value);
const char *ml_flags_value_name(ml_value_t *Value);

#ifndef GENERATE_INIT

#define ML_ENUM(TYPE, NAME, VALUES...) ml_type_t *TYPE
#define ML_FLAGS(TYPE, NAME, VALUES...) ml_type_t *TYPE
#define ML_ENUM2(TYPE, NAME, VALUES...) ml_type_t *TYPE
#define ML_FLAGS2(TYPE, NAME, VALUES...) ml_type_t *TYPE

#else

#define ML_ENUM(TYPE, NAME, VALUES...) INIT_CODE TYPE = ml_enum(NAME, VALUES, NULL)
#define ML_FLAGS(TYPE, NAME, VALUES...) INIT_CODE TYPE = ml_flags(NAME, VALUES, NULL)
#define ML_ENUM2(TYPE, NAME, VALUES...) INIT_CODE TYPE = ml_enum2(NAME, VALUES, NULL)
#define ML_FLAGS2(TYPE, NAME, VALUES...) INIT_CODE TYPE = ml_flags2(NAME, VALUES, NULL)

#endif

#ifdef __cplusplus
}
#endif

#endif

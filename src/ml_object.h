#ifndef ML_OBJECT_H
#define ML_OBJECT_H

/// \defgroup objects
/// @{
///

#include "ml_types.h"
#include "ml_uuid.h"
#include "stringmap.h"
#include "uuidmap.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ml_class_t ml_class_t;
typedef struct ml_object_t ml_object_t;
typedef struct ml_field_t ml_field_t;
typedef struct ml_field_info_t ml_field_info_t;

struct ml_field_info_t {
	ml_field_info_t *Next;
	ml_value_t *Method;
	ml_type_t *Type;
	int Index;
};

struct ml_class_t {
	ml_type_t Base;
	ml_value_t *Initializer, *Call;
	ml_field_info_t *Fields;
	stringmap_t Names[1];
	uuid_t Id;
	int NumFields;
};

struct ml_field_t {
	const ml_type_t *Type;
	ml_value_t *Value;
};

struct ml_object_t {
	ml_class_t *Type;
	ml_field_t Fields[];
};

void ml_object_init(stringmap_t *Globals);

ml_value_t *ml_field_fn(void *Data, int Count, ml_value_t **Args) __attribute__ ((malloc));

extern ml_type_t MLClassT[];
extern ml_type_t MLObjectT[];
extern ml_type_t MLFieldT[];
extern ml_type_t MLFieldMutableT[];

ml_object_t *ml_field_owner(ml_field_t *Field);
ml_value_t *ml_field_name(ml_field_t *Field);

ml_type_t *ml_class(const char *Name);
void ml_class_add_parent(ml_context_t *Context, ml_type_t *Class, ml_type_t *Parent);
void ml_class_add_field(ml_context_t *Context, ml_type_t *Class, ml_value_t *Field, ml_type_t *Type);

ml_value_t *ml_class_modify(ml_context_t *Context, ml_class_t *Class, ml_value_t *Modifier);

ml_value_t *ml_modified_field(ml_value_t *Field, ml_type_t *Type);
ml_value_t *ml_watched_field(ml_value_t *Callback);

size_t ml_class_size(const ml_type_t *Value) __attribute__ ((pure));
const unsigned char *ml_class_id(const ml_type_t *Value) __attribute__ ((pure));
const char *ml_class_field_name(const ml_type_t *Value, int Index) __attribute__ ((pure));

ml_value_t *ml_object(ml_type_t *Class, ...) __attribute__ ((sentinel));
size_t ml_object_size(const ml_value_t *Value) __attribute__ ((pure));
ml_value_t *ml_object_field(const ml_value_t *Value, int Index) __attribute__ ((pure));
void ml_object_foreach(const ml_value_t *Value, void *Data, int (*)(const char *, ml_value_t *, void *));

typedef struct ml_class_table_t ml_class_table_t;

struct ml_class_table_t {
	ml_class_table_t *Prev;
	ml_class_t *(*lookup)(ml_class_table_t *, uuid_t);
	ml_value_t *(*insert)(ml_class_table_t *, ml_class_t *);
};

typedef struct {
	ml_class_table_t Base;
	uuidmap_t Classes[1];
} ml_default_class_table_t;

ml_class_t *ml_default_class_table_lookup(ml_class_table_t *ClassTable, uuid_t Id);
ml_value_t *ml_default_class_table_insert(ml_class_table_t *ClassTable, ml_class_t *Class);

extern ml_type_t MLPseudoClassT[];
extern ml_type_t MLPseudoObjectT[];

ml_class_t *ml_pseudo_class(const char *Name, const uuid_t Id);
void ml_pseudo_class_add_field(ml_class_t *Class, const char *Name);

typedef struct {
	ml_type_t *Type;
	void *Value;
} ml_struct_instance_t;

ml_value_t *ml_struct_instance(ml_type_t *Type, void *Value);

static inline void *ml_struct_instance_value(ml_value_t *Value) {
	return ((ml_struct_instance_t *)Value)->Value;
}

/// @}

/// \defgroup enums
/// @{
///

typedef struct {
	ml_integer_t Base;
	ml_value_t *Name;
} ml_enum_value_t;

extern ml_type_t MLEnumT[];
extern ml_type_t MLEnumValueT[];

ml_type_t *ml_enum(const char *Name, ...);
ml_type_t *ml_enum_cyclic(const char *Name, ...);
ml_type_t *ml_enum2(const char *Name, ...);
ml_type_t *ml_sub_enum(const char *TypeName, ml_type_t *Parent, ...);

ml_value_t *ml_enum_value(ml_type_t *Type, int64_t Enum);

static inline int64_t ml_enum_value_value(ml_value_t *Value) {
	return ml_integer64_value(Value);
}

static inline const char *ml_enum_value_name(ml_value_t *Value) {
	return ml_string_value(((ml_enum_value_t *)Value)->Name);
}

/// @}

/// \defgroup flags
/// @{
///

extern ml_type_t MLFlagsT[];
extern ml_type_t MLFlagsValueT[];

ml_type_t *ml_flags(const char *Name, ...);
ml_type_t *ml_flags2(const char *Name, ...);

ml_value_t *ml_flags_value(ml_type_t *Type, uint64_t Flags);
uint64_t ml_flags_value_value(ml_value_t *Value);
const char *ml_flags_value_name(ml_value_t *Value);

#ifndef GENERATE_INIT

#define ML_CLASS(TYPE, PARENTS, NAME) ml_type_t *TYPE
#define ML_CLASS_ADD_PARENTS(TYPE, PARENTS ...) { \
	ml_type_t *Parents[] = {PARENTS}; \
	for (int I = 0; I < (sizeof(Parents) / sizeof(ml_type_t *)); ++I) { \
		ml_class_add_parent(NULL, TYPE, Parents[I]); \
	} \
}
#define ML_FIELD(FIELD, TYPE)
#define ML_ENUM(TYPE, NAME, VALUES ...) ml_type_t *TYPE
#define ML_ENUM_CYCLIC(TYPE, NAME, VALUES ...) ml_type_t *TYPE
#define ML_FLAGS(TYPE, NAME, VALUES ...) ml_type_t *TYPE
#define ML_ENUM2(TYPE, NAME, VALUES ...) ml_type_t *TYPE
#define ML_FLAGS2(TYPE, NAME, VALUES ...) ml_type_t *TYPE
#define ML_SUB_ENUM(TYPE, NAME, VALUES ...) ml_type_t *TYPE

#else

#define ML_CLASS(TYPE, PARENTS, NAME) \
	INIT_CODE TYPE = ml_class(NAME); \
	INIT_CODE ML_CLASS_ADD_PARENTS(TYPE UNWRAP PARENTS)
#define ML_FIELD(FIELD, TYPE) INIT_CODE ml_class_add_field(NULL, TYPE, _Generic(FIELD, char *: ml_method, default: ml_nop)(FIELD), MLFieldMutableT)
#define ML_ENUM(TYPE, NAME, VALUES...) INIT_CODE TYPE = ml_enum(NAME, VALUES, NULL)
#define ML_ENUM_CYCLIC(TYPE, NAME, VALUES...) INIT_CODE TYPE = ml_enum_cyclic(NAME, VALUES, NULL)
#define ML_FLAGS(TYPE, NAME, VALUES...) INIT_CODE TYPE = ml_flags(NAME, VALUES, NULL)
#define ML_ENUM2(TYPE, NAME, VALUES...) INIT_CODE TYPE = ml_enum2(NAME, VALUES, NULL)
#define ML_FLAGS2(TYPE, NAME, VALUES...) INIT_CODE TYPE = ml_flags2(NAME, VALUES, NULL)
#define ML_SUB_ENUM(TYPE, NAME, VALUES...) INIT_CODE TYPE = ml_sub_enum(NAME, VALUES, NULL)

#endif

#ifdef __cplusplus
}
#endif

/// @}

#endif

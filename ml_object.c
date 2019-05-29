#include <gc.h>
#include <string.h>
#include "minilang.h"
#include "ml_macros.h"
#include "ml_types.h"
#include "ml_object.h"

typedef struct ml_class_t {
	ml_type_t Base;
	int NumFields;
	ml_value_t *Fields[];
} ml_class_t;

typedef struct ml_object_t {
	const ml_type_t *Type;
	ml_value_t *Fields[];
} ml_object_t;

ml_type_t MLObjectT[1] = {{
	MLTypeT,
	MLAnyT, "object",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_iterate,
	ml_default_current,
	ml_default_next,
	ml_default_key
}};

static ml_value_t *ml_class_call(ml_class_t *Class, int Count, ml_value_t **Args) {
	ml_object_t *Object = xnew(ml_object_t, Class->NumFields, ml_value_t *);
	Object->Type = (ml_type_t *)Class;
	if (Count > Class->NumFields) Count = Class->NumFields;
	for (int I = 0; I < Count; ++I) Object->Fields[I] = Args[I];
	for (int I = Count; I < Class->NumFields; ++I) Object->Fields[I] = MLNil;
	return (ml_value_t *)Object;
}

ml_type_t MLClassT[1] = {{
	MLTypeT,
	MLTypeT, "class",
	ml_default_hash,
	(void *)ml_class_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_iterate,
	ml_default_current,
	ml_default_next,
	ml_default_key
}};

extern ml_value_t *AppendMethod;
static ml_value_t *StringMethod;

static ml_value_t *ml_object_append(void *Data, int Count, ml_value_t **Args) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_value_t *String = ml_inline(StringMethod, 1, Args[1]);
	if (String->Type == MLStringT) {
		ml_stringbuffer_add(Buffer, ml_string_value(String), ml_string_length(String));
		return MLSome;
	} else if (String->Type == MLErrorT) {
		return String;
	} else {
		return ml_error("ResultError", "string method did not return string");
	}
}

static ml_value_t *ml_object_string(void *Data, int Count, ml_value_t **Args) {
	ml_object_t *Object = (ml_object_t *)Args[0];
	ml_class_t *Class = (ml_class_t *)Object->Type;
	if (Class->NumFields > 0) {
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_add(Buffer, "object(", strlen("object("));
		const char *Name = ml_method_name(Class->Fields[0]);
		ml_stringbuffer_add(Buffer, Name, strlen(Name));
		ml_stringbuffer_add(Buffer, ": ", 2);
		ml_inline(AppendMethod, 2, Buffer, Object->Fields[0]);
		for (int I = 1; I < Class->NumFields; ++I) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			const char *Name = ml_method_name(Class->Fields[I]);
			ml_stringbuffer_add(Buffer, Name, strlen(Name));
			ml_stringbuffer_add(Buffer, ": ", 2);
			ml_inline(AppendMethod, 2, Buffer, Object->Fields[I]);
		}
		ml_stringbuffer_add(Buffer, ")", 1);
		return ml_stringbuffer_get_string(Buffer);
	} else {
		return ml_string("object()", strlen("object()"));
	}
}

static ml_value_t *ml_field_fn(void *Data, int Count, ml_value_t **Args) {
	int Index = (ml_value_t **)Data - ((ml_object_t *)0)->Fields;
	ml_object_t *Object = (ml_object_t *)Args[0];
	return ml_reference(Object->Fields + Index);
}

static ml_value_t *ml_class_fn(void *Data, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLMethodT);
	ml_class_t *Class = xnew(ml_class_t, Count, ml_method_t *);
	Class->Base = MLTypeT[0];
	Class->Base.Type = MLClassT;
	Class->Base.Parent = MLObjectT;
	Class->Base.Name = "object";
	Class->NumFields = Count;
	for (int I = 0; I < Count; ++I) {
		Class->Fields[I] = Args[I];
		ml_method_by_value(Args[I], ((ml_object_t *)0)->Fields + I, ml_field_fn, Class, NULL);
	}
	return (ml_value_t *)Class;
}

static ml_value_t *ml_method_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLMethodT);
	for (int I = 1; I < Count - 1; ++I) ML_CHECK_ARG_TYPE(I, MLTypeT);
	ML_CHECK_ARG_TYPE(Count - 1, MLFunctionT);
	ml_type_t *Types[Count - 2];
	for (int I = 1; I < Count - 1; ++I) Types[I - 1] = (ml_type_t *)Args[I];
	ml_method_by_array(Args[0], Args[Count - 1], Count - 2, Types);
	return Args[Count - 1];
}

static ml_value_t *ml_type_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	return (ml_value_t *)Args[0]->Type;
}

static ml_value_t *ml_class_subclass(void *Data, int Count, ml_value_t **Args) {
	ml_class_t *Parent = (ml_class_t *)Args[0];
	for (int I = 1; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLMethodT);
	ml_class_t *Class = xnew(ml_class_t, Parent->NumFields + Count - 1, ml_method_t *);
	Class->Base = MLTypeT[0];
	Class->Base.Type = MLClassT;
	Class->Base.Parent = (ml_type_t *)Parent;
	Class->Base.Name = "object";
	Class->NumFields = Parent->NumFields + Count - 1;
	memcpy(Class->Fields, Parent->Fields, Parent->NumFields * sizeof(ml_value_t *));
	for (int I = 1; I < Count; ++I) Class->Fields[Parent->NumFields + I - 1] = Args[I];
	for (int I = 0; I < Class->NumFields; ++I) {
		ml_method_by_value(Class->Fields[I], ((ml_object_t *)0)->Fields + I, ml_field_fn, Class, NULL);
	}
	return (ml_value_t *)Class;
}

static ml_value_t *ml_type_member(void *Data, int Count, ml_value_t **Args) {
	if (ml_is(Args[1], (ml_type_t *)Args[0])) return Args[1];
	return MLNil;
}

void ml_object_init(stringmap_t *Globals) {
	if (Globals) {
		stringmap_insert(Globals, "class", ml_function(NULL, ml_class_fn));
		stringmap_insert(Globals, "method", ml_function(NULL, ml_method_fn));
		stringmap_insert(Globals, "AnyT", MLAnyT);
		stringmap_insert(Globals, "TypeT", MLTypeT);
		stringmap_insert(Globals, "NilT", MLNilT);
		stringmap_insert(Globals, "FunctionT", MLFunctionT);
		stringmap_insert(Globals, "NumberT", MLNumberT);
		stringmap_insert(Globals, "IntegerT", MLIntegerT);
		stringmap_insert(Globals, "RealT", MLRealT);
		stringmap_insert(Globals, "StringT", MLStringT);
		stringmap_insert(Globals, "RegexT", MLRegexT);
		stringmap_insert(Globals, "MethodT", MLMethodT);
		stringmap_insert(Globals, "ListT", MLListT);
		stringmap_insert(Globals, "MapT", MLMapT);
	}
	StringMethod = ml_method("string");
	ml_method_by_value(AppendMethod, NULL, ml_object_append, MLStringBufferT, MLObjectT, NULL);
	ml_method_by_value(StringMethod, NULL, ml_object_string, MLObjectT, NULL);
	ml_method_by_name("subclass", NULL, ml_class_subclass, MLClassT, NULL);
	ml_method_by_name("?", NULL, ml_type_member, MLTypeT, MLAnyT, NULL);
}

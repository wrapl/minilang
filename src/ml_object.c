#include <gc.h>
#include <string.h>
#include "minilang.h"
#include "ml_macros.h"
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
	MLIteratableT, "object",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

static void ml_class_call(ml_state_t *Caller, ml_class_t *Class, int Count, ml_value_t **Args) {
	ml_object_t *Object = xnew(ml_object_t, Class->NumFields, ml_value_t *);
	Object->Type = (ml_type_t *)Class;
	ml_value_t **Slot = Object->Fields;
	for (int I = Class->NumFields; --I >= 0; ++Slot) *Slot = MLNil;
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I]->Type->deref(Args[I]);
		if (Arg->Type == MLErrorT) ML_RETURN(Arg);
		if (Arg->Type == MLNamesT) {
			ML_NAMES_FOREACH(Args[I], Node) {
				++I;
				ml_value_t *Field = Node->Value;
				for (int J = 0; J < Class->NumFields; ++J) {
					if (Class->Fields[J] == Field) {
						ml_value_t *Arg = Args[I]->Type->deref(Args[I]);
						if (Arg->Type == MLErrorT) ML_RETURN(Arg);
						Object->Fields[J] = Arg;
						goto found;
					}
				}
				ML_RETURN(ml_error("ValueError", "Class %s does not have field %s", Class->Base.Name, ml_method_name(Field)));
				found: 0;
			}
			break;
		} else if (I > Class->NumFields) {
			break;
		} else {
			Object->Fields[I] = Arg;
		}
	}
	ML_RETURN(Object);
}

ml_type_t MLClassT[1] = {{
	MLTypeT,
	MLTypeT, "class",
	ml_default_hash,
	(void *)ml_class_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLObjectT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_value_t *String = ml_inline(MLStringOfMethod, 1, Args[1]);
	if (String->Type == MLStringT) {
		ml_stringbuffer_add(Buffer, ml_string_value(String), ml_string_length(String));
		return MLSome;
	} else if (String->Type == MLErrorT) {
		return String;
	} else {
		return ml_error("ResultError", "string method did not return string");
	}
}

ML_METHOD(MLStringOfMethod, MLObjectT) {
	ml_object_t *Object = (ml_object_t *)Args[0];
	ml_class_t *Class = (ml_class_t *)Object->Type;
	if (Class->NumFields > 0) {
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_addf(Buffer, "%s(", Class->Base.Name);
		const char *Name = ml_method_name(Class->Fields[0]);
		ml_stringbuffer_add(Buffer, Name, strlen(Name));
		ml_stringbuffer_add(Buffer, ": ", 2);
		ml_inline(MLStringBufferAppendMethod, 2, Buffer, Object->Fields[0]);
		for (int I = 1; I < Class->NumFields; ++I) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			const char *Name = ml_method_name(Class->Fields[I]);
			ml_stringbuffer_add(Buffer, Name, strlen(Name));
			ml_stringbuffer_add(Buffer, ": ", 2);
			ml_inline(MLStringBufferAppendMethod, 2, Buffer, Object->Fields[I]);
		}
		ml_stringbuffer_add(Buffer, ")", 1);
		return ml_stringbuffer_get_string(Buffer);
	} else {
		return ml_string_format("%s()", Class->Base.Name);
	}
}

ml_value_t *ml_field_fn(void *Data, int Count, ml_value_t **Args) {
	int Index = (char *)Data - (char *)0;
	ml_object_t *Object = (ml_object_t *)Args[0];
	return ml_reference((ml_value_t **)((char *)Object + Index));
}

static ml_value_t *ml_class_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Name = ml_string_value(Args[0]);
	if (Count > 1 && !ml_is(Args[1], MLMethodT)) {
		ML_CHECK_ARG_TYPE(1, MLClassT);
		ml_class_t *Parent = (ml_class_t *)Args[1];
		for (int I = 2; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLMethodT);
		ml_class_t *Class = xnew(ml_class_t, Parent->NumFields + Count - 2, ml_value_t *);
		Class->Base.Type = MLClassT;
		Class->Base.Parent = (ml_type_t *)Parent;
		Class->Base.Name = Name;
		Class->Base.hash = ml_default_hash;
		Class->Base.call = ml_default_call;
		Class->Base.deref = ml_default_deref;
		Class->Base.assign = ml_default_assign;
		Class->NumFields = Parent->NumFields + Count - 2;
		memcpy(Class->Fields, Parent->Fields, Parent->NumFields * sizeof(ml_value_t *));
		for (int I = 2; I < Count; ++I) {
			Class->Fields[Parent->NumFields + I - 2] = Args[I];
		}
		for (int I = 0; I < Class->NumFields; ++I) {
			ml_method_by_value(Class->Fields[I], ((ml_object_t *)0)->Fields + I, ml_field_fn, Class, NULL);
		}
		return (ml_value_t *)Class;
	} else {
		for (int I = 1; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLMethodT);
		ml_class_t *Class = xnew(ml_class_t, Count, ml_value_t *);
		Class->Base.Type = MLClassT;
		Class->Base.Parent = MLObjectT;
		Class->Base.Name = Name;
		Class->Base.hash = ml_default_hash;
		Class->Base.call = ml_default_call;
		Class->Base.deref = ml_default_deref;
		Class->Base.assign = ml_default_assign;
		Class->NumFields = Count - 1;
		for (int I = 1; I < Count; ++I) {
			Class->Fields[I - 1] = Args[I];
		}
		for (int I = 0; I < Class->NumFields; ++I) {
			ml_method_by_value(Class->Fields[I], ((ml_object_t *)0)->Fields + I, ml_field_fn, Class, NULL);
		}
		return (ml_value_t *)Class;
	}
}

static ml_value_t *ml_method_set_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	if (Count == 1) {
		if (Args[0]->Type == MLMethodT) return Args[0];
		ML_CHECK_ARG_TYPE(0, MLStringT);
		return ml_method(ml_string_value(Args[0]));
	}
	ml_value_t *Method = Args[0];
	if (Method->Type == MLStringT) {
		Method = ml_method(ml_string_value(Args[0]));
	} else {
		ML_CHECK_ARG_TYPE(0, MLMethodT);
	}
	for (int I = 1; I < Count - 1; ++I) ML_CHECK_ARG_TYPE(I, MLTypeT);
	ML_CHECK_ARG_TYPE(Count - 1, MLFunctionT);
	ml_method_by_array(Method, Args[Count - 1], Count - 2, (ml_type_t **)(Args + 1));
	return Args[Count - 1];
}

typedef struct ml_assignable_t {
	const ml_type_t *Type;
	ml_value_t *Get, *Set;
} ml_assignable_t;

static ml_value_t *ml_assignable_deref(ml_assignable_t *Assignable) {
	return ml_call(Assignable->Get, 0, NULL);
}

static ml_value_t *ml_assignable_assign(ml_assignable_t *Assignable, ml_value_t *Value) {
	return ml_call(Assignable->Set, 1, &Value);
}

static ml_type_t MLAssignableT[1] = {{
	MLTypeT,
	MLAnyT, "assignable",
	ml_default_hash,
	ml_default_call,
	(void *)ml_assignable_deref,
	(void *)ml_assignable_assign,
	NULL, 0, 0
}};

static ml_value_t *ml_property_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(2);
	ml_assignable_t *Assignable = new(ml_assignable_t);
	Assignable->Type = MLAssignableT;
	Assignable->Get = Args[0];
	Assignable->Set = Args[1];
	return (ml_value_t *)Assignable;
}

ML_METHOD("?", MLAnyT) {
	return (ml_value_t *)Args[0]->Type;
}

ML_METHOD("?", MLTypeT, MLAnyT) {
	if (ml_is(Args[1], (ml_type_t *)Args[0])) return Args[1];
	return MLNil;
}

size_t ml_class_size(ml_value_t *Value) {
	return ((ml_class_t *)Value)->NumFields;
}

ml_value_t *ml_class_field(ml_value_t *Value, size_t Field) {
	return ((ml_class_t *)Value)->Fields[Field];
}

size_t ml_object_size(ml_value_t *Value) {
	return ((ml_class_t *)Value->Type)->NumFields;
}

ml_value_t *ml_object_field(ml_value_t *Value, size_t Field) {
	return ((ml_object_t *)Value)->Fields[Field];
}

void ml_object_init(stringmap_t *Globals) {
	stringmap_insert(Globals, "class", ml_function(NULL, ml_class_fn));
	ml_value_t *Method = (ml_value_t *)stringmap_search(Globals, "method");
	if (Method) {
		ml_module_export(Method, "set", ml_function(NULL, ml_method_set_fn));
	}
	stringmap_insert(Globals, "property", ml_function(NULL, ml_property_fn));
	stringmap_insert(Globals, "ObjectT", MLObjectT);
	stringmap_insert(Globals, "ClassT", MLClassT);
#include "ml_object_init.c"
}

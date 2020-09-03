#include <gc.h>
#include <string.h>
#include "minilang.h"
#include "ml_macros.h"
#include "ml_object.h"

typedef struct ml_class_t ml_class_t;
typedef struct ml_object_t ml_object_t;

struct ml_class_t {
	ml_type_t Base;
	int NumFields;
	ml_value_t *Initializer;
	ml_value_t *Fields[];
};

struct ml_object_t {
	const ml_type_t *Type;
	ml_value_t *Fields[];
};

ML_INTERFACE(MLObjectT, (), "object");
// Parent type of all object classes.

static void ml_class_call(ml_state_t *Caller, ml_class_t *Class, int Count, ml_value_t **Args) {
	ml_value_t *Constructor = Class->Base.Constructor;
	return ml_call(Caller, Constructor, Count, Args);
}

extern ml_cfunction_t MLClass[];

ML_TYPE(MLClassT, (MLTypeT), "class",
//!object
// Type of all object classes.
	.call = (void *)ml_class_call,
	.Constructor = (ml_value_t *)MLClass
);

ML_METHOD(MLStringOfMethod, MLObjectT) {
	ml_object_t *Object = (ml_object_t *)Args[0];
	ml_class_t *Class = (ml_class_t *)Object->Type;
	if (Class->NumFields > 0) {
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_addf(Buffer, "%s(", Class->Base.Name);
		const char *Name = ml_method_name(Class->Fields[0]);
		ml_stringbuffer_add(Buffer, Name, strlen(Name));
		ml_stringbuffer_add(Buffer, ": ", 2);
		ml_simple_inline(MLStringBufferAppendMethod, 2, Buffer, Object->Fields[0]);
		for (int I = 1; I < Class->NumFields; ++I) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			const char *Name = ml_method_name(Class->Fields[I]);
			ml_stringbuffer_add(Buffer, Name, strlen(Name));
			ml_stringbuffer_add(Buffer, ": ", 2);
			ml_simple_inline(MLStringBufferAppendMethod, 2, Buffer, Object->Fields[I]);
		}
		ml_stringbuffer_add(Buffer, ")", 1);
		return ml_stringbuffer_value(Buffer);
	} else {
		return ml_string_format("%s()", Class->Base.Name);
	}
}

ml_value_t *ml_field_fn(void *Data, int Count, ml_value_t **Args) {
	int Index = (char *)Data - (char *)0;
	ml_object_t *Object = (ml_object_t *)Args[0];
	return ml_reference((ml_value_t **)((char *)Object + Index));
}

static void ml_init_state_run(ml_value_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	ML_RETURN(State->Value);
}

static void ml_constructor_fn(ml_state_t *Caller, ml_class_t *Class, int Count, ml_value_t **Args) {
	ml_object_t *Object = xnew(ml_object_t, Class->NumFields, ml_value_t *);
	Object->Type = (ml_type_t *)Class;
	ml_value_t **Slot = Object->Fields;
	for (int I = Class->NumFields; --I >= 0; ++Slot) *Slot = MLNil;
	if (Class->Initializer) {
		ml_value_t **Args2 = anew(ml_value_t *, Count + 1);
		Args2[0] = (ml_value_t *)Object;
		for (int I = 0; I < Count; ++I) Args2[I + 1] = Args[I];
		ml_value_state_t *State = new(ml_value_state_t);
		State->Base.run = (void *)ml_init_state_run;
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Value = (ml_value_t *)Object;
		return ml_call(State, Class->Initializer, Count + 1, Args2);
	}
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = ml_typeof(Args[I])->deref(Args[I]);
		if (ml_is_error(Arg)) ML_RETURN(Arg);
		if (ml_is(Arg, MLNamesT)) {
			ML_NAMES_FOREACH(Args[I], Node) {
				++I;
				ml_value_t *Field = Node->Value;
				for (int J = 0; J < Class->NumFields; ++J) {
					if (Class->Fields[J] == Field) {
						ml_value_t *Arg = ml_typeof(Args[I])->deref(Args[I]);
						if (ml_is_error(Arg)) ML_RETURN(Arg);
						Object->Fields[J] = Arg;
						goto found;
					}
				}
				ML_RETURN(ml_error("ValueError", "Class %s does not have field %s", Class->Base.Name, ml_method_name(Field)));
				found:;
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

ML_FUNCTION(MLClass) {
//!object
//@class
//<?Name:string
//<?Parents...:class
//<?Fields...:method
//<?Exports...:named
	static ml_value_t **FieldFns = NULL;
	static int NumFieldFns = 0;
	ML_CHECK_ARG_COUNT(1);
	const char *Name = NULL;
	int Start = 0;
	if (Count > 0 && ml_is(Args[0], MLStringT)) {
		Name = ml_string_value(Args[0]);
		Start = 1;
	}
	int NumFields = 0, NumParents = 0, Rank = 0;
	for (int I = Start; I < Count; ++I) {
		if (ml_typeof(Args[I]) == MLMethodT) {
			++NumFields;
		} else if (ml_is(Args[I], MLClassT)) {
			ml_class_t *Parent = (ml_class_t *)Args[I];
			NumFields += Parent->NumFields;
			const ml_type_t **Types = Parent->Base.Types;
			do ++NumParents; while (*++Types != MLObjectT);
			if (Rank < Parent->Base.Rank) Rank = Parent->Base.Rank;
		} else if (ml_is(Args[I], MLTypeT)) {
			ml_type_t *Parent = (ml_type_t *)Args[I];
			if (Parent->Rank >= ML_RANK_NATIVE) return ml_error("TypeError", "Classes can not inherit from native types");
			const ml_type_t **Types = Parent->Types;
			do ++NumParents; while (*++Types);
			if (Rank < Parent->Rank) Rank = Parent->Rank;
		} else if (ml_is(Args[I], MLNamesT)) {
			break;
		} else {
			return ml_error("TypeError", "Unexpected argument type: <%s>", ml_typeof(Args[I])->Name);
		}
	}
	ml_class_t *Class = xnew(ml_class_t, NumFields, ml_value_t *);
	Class->Base.Type = MLClassT;
	if (Name) {
		Class->Base.Name = Name;
	} else {
		asprintf((char **)&Class->Base.Name, "object:%lx", (uintptr_t)Class);
	}
	Class->Base.hash = ml_default_hash;
	Class->Base.call = ml_default_call;
	Class->Base.deref = ml_default_deref;
	Class->Base.assign = ml_default_assign;
	Class->Base.Rank = Rank + 1;
	const ml_type_t **Parents = Class->Base.Types = anew(const ml_type_t *, NumParents + 4);
	*Parents++ = (ml_type_t *)Class;
	Class->NumFields = NumFields;
	ml_value_t **Fields = Class->Fields;
	ml_value_t *Constructor = ml_cfunctionx(Class, (void *)ml_constructor_fn);
	Class->Base.Constructor = Constructor;
	for (int I = Start; I < Count; ++I) {
		if (ml_typeof(Args[I]) == MLMethodT) {
			*Fields++ = Args[I];
		} else if (ml_is(Args[I], MLClassT)) {
			ml_class_t *Parent = (ml_class_t *)Args[I];
			for (int I = 0; I < Parent->NumFields; ++I) *Fields++ = Parent->Fields[I];
			const ml_type_t **Types = Parent->Base.Types;
			while (*Types != MLObjectT) *Parents++ = *Types++;
		} else if (ml_is(Args[I], MLTypeT)) {
			ml_type_t *Parent = (ml_type_t *)Args[I];
			const ml_type_t **Types = Parent->Types;
			while (*Types) *Parents++ = *Types++;
		} else if (ml_is(Args[I], MLNamesT)) {
			ML_LIST_FOREACH(Args[I], Iter) {
				ml_value_t *Key = Iter->Value;
				const char *Name = ml_method_name(Key);
				if (!strcmp(Name, "of")) {
					Class->Base.Constructor = Args[++I];
				} else if (!strcmp(Name, "init")) {
					Class->Initializer = Args[++I];
				} else {
					stringmap_insert(Class->Base.Exports, Name, Args[++I]);
				}
			}
			break;
		}
	}
	*Parents++ = MLObjectT;
	*Parents++ = MLAnyT;
	if (Class->NumFields > NumFieldFns) {
		ml_value_t **NewFieldFns = anew(ml_value_t *, Class->NumFields);
		memcpy(NewFieldFns, FieldFns, NumFieldFns * sizeof(ml_value_t *));
		for (int I = NumFieldFns; I < Class->NumFields; ++I) {
			NewFieldFns[I] = ml_cfunction(((ml_object_t *)0)->Fields + I, ml_field_fn);
		}
		FieldFns = NewFieldFns;
		NumFieldFns = Class->NumFields;
	}
	for (int I = 0; I < Class->NumFields; ++I) {
		ml_method_by_array(Class->Fields[I], FieldFns[I], 1, (ml_type_t **)&Class);
	}
	stringmap_insert(Class->Base.Exports, "new", Constructor);
	return (ml_value_t *)Class;
}

typedef struct ml_property_t {
	const ml_type_t *Type;
	ml_value_t *Get, *Set;
} ml_property_t;

static ml_value_t *ml_property_deref(ml_property_t *Property) {
	return ml_simple_call(Property->Get, 0, NULL);
}

static ml_value_t *ml_property_assign(ml_property_t *Property, ml_value_t *Value) {
	return ml_simple_call(Property->Set, 1, &Value);
}

extern ml_cfunction_t MLProperty[];

ML_TYPE(MLPropertyT, (), "property",
	.deref = (void *)ml_property_deref,
	.assign = (void *)ml_property_assign,
	.Constructor = (ml_value_t *)MLProperty
);

ML_FUNCTION(MLProperty) {
//!object
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLFunctionT);
	ML_CHECK_ARG_TYPE(1, MLFunctionT);
	ml_property_t *Property = new(ml_property_t);
	Property->Type = MLPropertyT;
	Property->Get = Args[0];
	Property->Set = Args[1];
	return (ml_value_t *)Property;
}

size_t ml_class_size(const ml_value_t *Value) {
	return ((ml_class_t *)Value)->NumFields;
}

ml_value_t *ml_class_field(const ml_value_t *Value, size_t Field) {
	return ((ml_class_t *)Value)->Fields[Field];
}

size_t ml_object_size(const ml_value_t *Value) {
	return ((ml_class_t *)ml_typeof(Value))->NumFields;
}

ml_value_t *ml_object_field(const ml_value_t *Value, size_t Field) {
	return ((ml_object_t *)Value)->Fields[Field];
}

typedef struct {
	ml_type_t Base;
	int NumValues;
	ml_value_t *Values[];
} ml_enum_t;

typedef struct {
	const ml_type_t *Type;
	ml_value_t *Name;
	int Index;
} ml_enum_value_t;

extern ml_type_t MLEnumT[];

static long ml_enum_value_hash(ml_enum_value_t *Value, ml_hash_chain_t *Chain) {
	return (long)Value->Type + Value->Index;
}

ML_TYPE(MLEnumValueT, (), "enum-value");

static ml_value_t *ML_TYPED_FN(ml_string_of, MLEnumValueT, ml_enum_value_t *Value) {
	return Value->Name;
}

ML_FUNCTION(MLEnum) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	for (int I = 1; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLStringT);
	ml_enum_t *Enum = xnew(ml_enum_t, Count - 1, ml_value_t *);
	Enum->Base.Type = MLEnumT;
	Enum->Base.Name = ml_string_value(Args[0]);
	Enum->Base.deref = ml_default_deref;
	Enum->Base.assign = ml_default_assign;
	Enum->Base.hash = (void *)ml_enum_value_hash;
	Enum->Base.call = ml_default_call;
	Enum->Base.Rank = ML_RANK_NATIVE;
	Enum->NumValues = Count - 1;
	ml_type_init((ml_type_t *)Enum, MLEnumValueT, NULL);
	for (int I = 1; I < Count; ++I) {
		ml_enum_value_t *Value = new(ml_enum_value_t);
		Value->Type = (ml_type_t *)Enum;
		Value->Name = Args[I];
		Value->Index = I;
		Enum->Values[I - 1] = (ml_value_t *)Value;
		stringmap_insert(Enum->Base.Exports, ml_string_value(Args[I]), Value);
	}
	return (ml_value_t *)Enum;
}

static void ml_enum_call(ml_state_t *Caller, ml_enum_t *Enum, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	if (ml_is(Args[0], MLStringT)) {
		ml_value_t *Value = stringmap_search(Enum->Base.Exports, ml_string_value(Args[0]));
		if (!Value) ML_ERROR("EnumError", "Invalid enum name");
		ML_RETURN(Value);
	} else if (ml_is(Args[0], MLIntegerT)) {
		int Index = ml_integer_value(Args[0]) - 1;
		if (Index < 0 || Index >= Enum->NumValues) ML_ERROR("EnumError", "Invalid enum index");
		ML_RETURN(Enum->Values[Index]);
	} else {
		ML_ERROR("TypeError", "Expected <integer> or <string> not <%s>", ml_typeof(Args[0])->Name);
	}
}

ML_TYPE(MLEnumT, (MLTypeT), "enum",
	.call = (void *)ml_enum_call,
	.Constructor = (void *)MLEnum
);

void ml_object_init(stringmap_t *Globals) {
#include "ml_object_init.c"
	stringmap_insert(Globals, "property", MLPropertyT);
	stringmap_insert(Globals, "object", MLObjectT);
	stringmap_insert(Globals, "class", MLClassT);
	stringmap_insert(Globals, "enum", MLEnumT);
}

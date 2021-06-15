#include <gc.h>
#include <string.h>
#include "minilang.h"
#include "ml_macros.h"
#include "ml_object.h"

typedef struct ml_class_t ml_class_t;
typedef struct ml_object_t ml_object_t;

struct ml_class_t {
	ml_type_t Base;
	ml_value_t *Initializer;
	stringmap_t Fields[1];
};

typedef struct {
	const ml_type_t *Type;
	ml_value_t *Value;
} ml_field_t;

static ml_value_t *ml_field_deref(ml_field_t *Field) {
	return Field->Value;
}

static ml_value_t *ml_field_assign(ml_field_t *Field, ml_value_t *Value) {
	return Field->Value = Value;
}

static void ml_field_call(ml_state_t *Caller, ml_field_t *Field, int Count, ml_value_t **Args) {
	return ml_call(Caller, Field->Value, Count, Args);
}

ML_TYPE(MLFieldT, (), "field",
	.deref = (void *)ml_field_deref,
	.assign = (void *)ml_field_assign,
	.call = (void *)ml_field_call
);

struct ml_object_t {
	const ml_type_t *Type;
	ml_field_t Fields[];
};

ML_INTERFACE(MLObjectT, (), "object");
// Parent type of all object classes.

static void ml_class_call(ml_state_t *Caller, ml_type_t *Class, int Count, ml_value_t **Args) {
	ml_value_t *Constructor = Class->Constructor;
	return ml_call(Caller, Constructor, Count, Args);
}

extern ml_cfunctionx_t MLClass[];

ML_TYPE(MLClassT, (MLTypeT), "class",
//!object
// Type of all object classes.
	.call = (void *)ml_class_call,
	.Constructor = (ml_value_t *)MLClass
);

typedef struct {
	ml_object_t *Object;
	ml_stringbuffer_t Buffer[1];
	int Comma;
} ml_object_stringer_t;

static int field_string(const char *Name, void *Offset, ml_object_stringer_t *Stringer) {
	if (Stringer->Comma++) ml_stringbuffer_add(Stringer->Buffer, ", ", 2);
	ml_stringbuffer_add(Stringer->Buffer, Name, strlen(Name));
	ml_stringbuffer_add(Stringer->Buffer, ": ", 2);
	ml_stringbuffer_append(Stringer->Buffer, ((ml_field_t *)((char *)Stringer->Object + (uintptr_t)Offset))->Value);
	return 0;
}

ML_METHOD(MLStringT, MLObjectT) {
	ml_object_t *Object = (ml_object_t *)Args[0];
	ml_class_t *Class = (ml_class_t *)Object->Type;
	ml_object_stringer_t Stringer = {Object, {ML_STRINGBUFFER_INIT}, 0};
	ml_stringbuffer_addf(Stringer.Buffer, "%s(", Class->Base.Name);
	stringmap_foreach(Class->Fields, &Stringer, (void *)field_string);
	ml_stringbuffer_add(Stringer.Buffer, ")", 1);
	return ml_stringbuffer_value(Stringer.Buffer);
}

ml_value_t *ml_field_fn(void *Data, int Count, ml_value_t **Args) {
	ml_object_t *Object = (ml_object_t *)Args[0];
	return (ml_value_t *)((char *)Object + (uintptr_t)Data);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Object;
	ml_value_t *Args[];
} ml_init_state_t;

static void ml_init_state_run(ml_init_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	ML_RETURN(State->Object);
}

static void ml_object_constructor_fn(ml_state_t *Caller, ml_class_t *Class, int Count, ml_value_t **Args) {
	int NumFields = Class->Fields->Size;
	ml_object_t *Object = xnew(ml_object_t, NumFields, ml_field_t);
	Object->Type = (ml_type_t *)Class;
	ml_field_t *Slot = Object->Fields;
	for (int I = NumFields; --I >= 0; ++Slot) {
		Slot->Type = MLFieldT;
		Slot->Value = MLNil;
	}
	if (Class->Initializer) {
		ml_init_state_t *State = xnew(ml_init_state_t, Count + 1, ml_value_t *);
		State->Args[0] = (ml_value_t *)Object;
		for (int I = 0; I < Count; ++I) State->Args[I + 1] = Args[I];
		State->Base.run = (void *)ml_init_state_run;
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Object = (ml_value_t *)Object;
		return ml_call(State, Class->Initializer, Count + 1, State->Args);
	}
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = ml_typeof(Args[I])->deref(Args[I]);
		if (ml_is_error(Arg)) ML_RETURN(Arg);
		if (ml_is(Arg, MLNamesT)) {
			ML_NAMES_FOREACH(Args[I], Iter) {
				++I;
				const char *Name = ml_string_value(Iter->Value);
				void *Offset = stringmap_search(Class->Fields, Name);
				if (!Offset) {
					ML_ERROR("ValueError", "Class %s does not have field %s", Class->Base.Name, Name);
				}
				ml_field_t *Field = (ml_field_t *)((char *)Object + (uintptr_t)Offset);
				Field->Value = Arg;
			}
			break;
		} else if (I > NumFields) {
			break;
		} else {
			Object->Fields[I].Value = Arg;
		}
	}
	ML_RETURN(Object);
}

typedef struct {
	ml_type_t Base;
	ml_type_t *Native;
	ml_value_t *Constructor;
	ml_value_t *Initializer;
} ml_named_type_t;

ML_TYPE(MLNamedTypeT, (MLTypeT), "named-type",
	.call = (void *)ml_class_call
);

typedef struct {
	ml_state_t Base;
	ml_value_t *Object;
	ml_type_t *Old, *New;
	ml_value_t *Init;
	int Count;
	ml_value_t *Args[];
} ml_named_init_state_t;

static void ml_named_init_state_run(ml_named_init_state_t *State, ml_value_t *Result) {
	ml_type_t *Old = ml_typeof(Result);
	if (Old == State->Old) {
		Result->Type = State->New;
#ifdef ML_GENERICS
	} else if (Old->Type == MLGenericTypeT && ml_generic_type_args(Old)[0] == State->Old) {
		Result->Type = State->New;
#endif
	} else {
		ML_CONTINUE(State->Base.Caller, Result);
	}
	if (State->Init) {
		State->Object = State->Args[0] = Result;
		State->Base.run = (void *)ml_init_state_run;
		return ml_call(State, State->Init, State->Count, State->Args);
	}
	ML_CONTINUE(State->Base.Caller, Result);
}

static void ml_named_constructor_fn(ml_state_t *Caller, ml_named_type_t *Class, int Count, ml_value_t **Args) {
	ml_named_init_state_t *State = new(ml_named_init_state_t);
	State->Base.run = (void *)ml_named_init_state_run;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Old = Class->Native;
	State->New = (ml_type_t *)Class;
	return ml_call(State, Class->Native->Constructor, Count, Args);
}

static void ml_named_initializer_fn(ml_state_t *Caller, ml_named_type_t *Class, int Count, ml_value_t **Args) {
	ml_named_init_state_t *State = xnew(ml_named_init_state_t, Count + 1, ml_value_t *);
	for (int I = 0; I < Count; ++I) State->Args[I + 1] = Args[I];
	State->Base.run = (void *)ml_named_init_state_run;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Old = Class->Native;
	State->New = (ml_type_t *)Class;
	State->Init = Class->Initializer;
	State->Count = Count + 1;
	return ml_call(State, Class->Native->Constructor, 0, NULL);
}

static int add_field(const char *Name, void *Value, ml_class_t *Class) {
	//printf("Adding field %s to class %s\n", Name, Class->Base.Name);
	void **Slot = stringmap_slot(Class->Fields, Name);
	if (!Slot[0]) Slot[0] = &((ml_object_t *)0)->Fields[Class->Fields->Size - 1];
	return 0;
}

typedef struct {
	ml_methods_t *Methods;
	ml_value_t **FieldFns;
	ml_class_t *Class;
} class_setup_t;


static int setup_field(const char *Name, char *Offset, class_setup_t *Setup) {
	//printf("Adding method %s:%s\n", Setup->Class->Base.Name, Name);
	int Index = (Offset - (char *)&((ml_object_t *)0)->Fields) / sizeof(ml_field_t);
	ml_type_t *Types[1] = {(ml_type_t *)Setup->Class};
	ml_method_t *Method = (ml_method_t *)ml_method(Name);
	ml_method_insert(Setup->Methods, Method, Setup->FieldFns[Index], 1, 1, Types);
	return 0;
}

ML_FUNCTIONX(MLClass) {
//!object
//@class
//<Name?:string
//<Parents...:class
//<Fields...:method
//<Exports...:named
	static ml_value_t **FieldFns = NULL;
	static int NumFieldFns = 0;
	const char *Name = NULL;
	int Start = 0;
	if (Count > 0 && ml_is(Args[0], MLStringT)) {
		Name = ml_string_value(Args[0]);
		Start = 1;
	}
	int Rank = 0;
	ml_type_t *NativeType = NULL;
	for (int I = Start; I < Count; ++I) {
		if (ml_typeof(Args[I]) == MLMethodT) {
		} else if (ml_is(Args[I], MLClassT)) {
			ml_class_t *Parent = (ml_class_t *)Args[I];
			if (Rank < Parent->Base.Rank) Rank = Parent->Base.Rank;
		} else if (ml_is(Args[I], MLNamedTypeT)) {
			ml_named_type_t *Parent = (ml_named_type_t *)Args[I];
			if (NativeType && NativeType != Parent->Native) {
				ML_ERROR("TypeError", "Classes can not inherit from multiple native types");
			}
			NativeType = Parent->Native;
		} else if (ml_is(Args[I], MLTypeT)) {
			ml_type_t *Parent = (ml_type_t *)Args[I];
			if (Parent->NoInherit) {
				ML_ERROR("TypeError", "Classes can not inherit from <%s>", Parent->Name);
			}
			if (!Parent->Interface) {
				if (NativeType && NativeType != Parent) {
					ML_ERROR("TypeError", "Classes can not inherit from multiple native types");
				}
				NativeType = Parent;
			}
			if (Rank < Parent->Rank) Rank = Parent->Rank;
		} else if (ml_is(Args[I], MLNamesT)) {
			break;
		} else {
			ML_ERROR("TypeError", "Unexpected argument type: <%s>", ml_typeof(Args[I])->Name);
		}
	}
	if (NativeType) {
		ml_named_type_t *Class = new(ml_named_type_t);
		Class->Base.Type = MLNamedTypeT;
		if (Name) {
			Class->Base.Name = Name;
		} else {
			asprintf((char **)&Class->Base.Name, "named-%s:%lx", NativeType->Name, (uintptr_t)Class);
		}
		Class->Base.hash = ml_default_hash;
		Class->Base.call = ml_default_call;
		Class->Base.deref = ml_default_deref;
		Class->Base.assign = ml_default_assign;
		Class->Base.Rank = Rank + 1;
		Class->Native = NativeType;
		ml_value_t *Constructor = ml_cfunctionx(Class, (void *)ml_named_constructor_fn);
		Class->Base.Constructor = Constructor;
		for (int I = Start; I < Count; ++I) {
			if (ml_is(Args[I], MLTypeT)) {
				ml_type_t *Parent = (ml_type_t *)Args[I];
				ml_type_add_parent((ml_type_t *)Class, Parent);
			} else if (ml_is(Args[I], MLNamesT)) {
				ML_LIST_FOREACH(Args[I], Iter) {
					ml_value_t *Key = Iter->Value;
					const char *Name = ml_string_value(Key);
					ml_value_t *Value = Args[++I];
					stringmap_insert(Class->Base.Exports, Name, Value);
					if (!strcmp(Name, "of")) {
						Class->Base.Constructor = Value;
					} else if (!strcmp(Name, "init")) {
						Class->Initializer = Value;
						Class->Base.Constructor = ml_cfunctionx(Class, (void *)ml_named_initializer_fn);
					}
				}
				break;
			}
		}
		stringmap_insert(Class->Base.Exports, "new", Constructor);
		ML_RETURN(Class);
	} else {
		ml_class_t *Class = new(ml_class_t);
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
		ml_value_t *Constructor = ml_cfunctionx(Class, (void *)ml_object_constructor_fn);
		Class->Base.Constructor = Constructor;
		for (int I = Start; I < Count; ++I) {
			if (ml_typeof(Args[I]) == MLMethodT) {
				add_field(ml_method_name(Args[I]), NULL, Class);
			} else if (ml_is(Args[I], MLClassT)) {
				ml_class_t *Parent = (ml_class_t *)Args[I];
				stringmap_foreach(Parent->Fields, Class, (void *)add_field);
				ml_type_add_parent((ml_type_t *)Class, (ml_type_t *)Parent);
			} else if (ml_is(Args[I], MLTypeT)) {
				ml_type_t *Parent = (ml_type_t *)Args[I];
				ml_type_add_parent((ml_type_t *)Class, Parent);
			} else if (ml_is(Args[I], MLNamesT)) {
				ML_LIST_FOREACH(Args[I], Iter) {
					ml_value_t *Key = Iter->Value;
					const char *Name = ml_string_value(Key);
					ml_value_t *Value = Args[++I];
					stringmap_insert(Class->Base.Exports, Name, Value);
					if (!strcmp(Name, "of")) {
						Class->Base.Constructor = Value;
					} else if (!strcmp(Name, "init")) {
						Class->Initializer = Value;
					}
				}
				break;
			}
		}
		ml_type_add_parent((ml_type_t *)Class, MLObjectT);
		int NumFields = Class->Fields->Size;
		if (NumFields > NumFieldFns) {
			ml_value_t **NewFieldFns = anew(ml_value_t *, NumFields);
			memcpy(NewFieldFns, FieldFns, NumFieldFns * sizeof(ml_value_t *));
			for (int I = NumFieldFns; I < NumFields; ++I) {
				void *Offset = &((ml_object_t *)0)->Fields[I];
				NewFieldFns[I] = ml_cfunction(Offset, ml_field_fn);
			}
			FieldFns = NewFieldFns;
			NumFieldFns = NumFields;
		}
		class_setup_t Setup = {Caller->Context->Values[ML_METHODS_INDEX], FieldFns, Class};
		stringmap_foreach(Class->Fields, &Setup, (void *)setup_field);
		stringmap_insert(Class->Base.Exports, "new", Constructor);
		ML_RETURN(Class);
	}
}

typedef struct ml_property_t {
	const ml_type_t *Type;
	ml_value_t *Get, *Set;
	ml_result_state_t State[1];
} ml_property_t;

static ml_value_t *ml_property_deref(ml_property_t *Property) {
	Property->State->Value = NULL;
	ml_call(Property->State, Property->Get, 0, NULL);
	return Property->State->Value ?: ml_error("StateError", "Property functions must not suspend");
}

static ml_value_t *ml_property_assign(ml_property_t *Property, ml_value_t *Value) {
	Property->State->Value = NULL;
	ml_call(Property->State, Property->Set, 1, &Value);
	return Property->State->Value ?: ml_error("StateError", "Property functions must not suspend");
}

static void ml_property_call(ml_state_t *Caller, ml_property_t *Property, int Count, ml_value_t **Args) {
	Property->State->Value = NULL;
	ml_call(Property->State, Property->Get, 0, NULL);
	return ml_call(Caller, Property->State->Value ?: MLNil, Count, Args);
}

extern ml_cfunctionx_t MLProperty[];

ML_TYPE(MLPropertyT, (), "property",
	.deref = (void *)ml_property_deref,
	.assign = (void *)ml_property_assign,
	.call = (void *)ml_property_call,
	.Constructor = (ml_value_t *)MLProperty
);

ML_FUNCTIONX(MLProperty) {
//@property
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLFunctionT);
	ML_CHECKX_ARG_TYPE(1, MLFunctionT);
	ml_property_t *Property = new(ml_property_t);
	Property->Type = MLPropertyT;
	Property->Get = Args[0];
	Property->Set = Args[1];
	Property->State->Base.run = (ml_state_fn)ml_result_state_run;
	Property->State->Base.Context = Caller->Context;
	ML_RETURN(Property);
}

size_t ml_class_size(const ml_value_t *Value) {
	return ((ml_class_t *)Value)->Fields->Size;
}

size_t ml_object_size(const ml_value_t *Value) {
	return ((ml_class_t *)ml_typeof(Value))->Fields->Size;
}

ml_value_t *ml_object_field(const ml_value_t *Value, size_t Field) {
	return ((ml_object_t *)Value)->Fields[Field].Value;
}

typedef struct {
	ml_type_t Base;
	ml_value_t *Values[];
} ml_enum_t;

typedef struct {
#ifdef ML_GENERICS
	ml_int64_t Base;
#else
	ml_integer_t Base;
#endif
	ml_value_t *Name;
} ml_enum_value_t;

extern ml_type_t MLEnumT[];

static long ml_enum_value_hash(ml_enum_value_t *Value, ml_hash_chain_t *Chain) {
	return (long)Value->Base.Type + Value->Base.Value;
}

#ifdef ML_GENERICS
ML_TYPE(MLEnumValueT, (MLInt64T), "enum-value");
#else
ML_TYPE(MLEnumValueT, (MLIntegerT), "enum-value");
#endif

ML_METHOD(MLStringT, MLEnumValueT) {
	ml_enum_value_t *Value = (ml_enum_value_t *)Args[0];
	return Value->Name;
}

ML_FUNCTION(MLEnum) {
//@enum
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLListT);
	int Size = ml_list_length(Args[1]);
	ml_enum_t *Enum = xnew(ml_enum_t, Size, ml_value_t *);
	Enum->Base.Type = MLEnumT;
	Enum->Base.Name = ml_string_value(Args[0]);
	Enum->Base.deref = ml_default_deref;
	Enum->Base.assign = ml_default_assign;
	Enum->Base.hash = (void *)ml_enum_value_hash;
	Enum->Base.call = ml_default_call;
	Enum->Base.Rank = 1;
	ml_type_init((ml_type_t *)Enum, MLEnumValueT, NULL);
	Enum->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	int Index = 0;
	ML_LIST_FOREACH(Args[1], Iter) {
		ml_value_t *Name = Iter->Value;
		if (!ml_is(Name, MLStringT)) {
			return ml_error("TypeError", "Enum names must be strings");
		}
		ml_enum_value_t *Value = new(ml_enum_value_t);
		Value->Base.Type = (ml_type_t *)Enum;
		Value->Name = Name;
		Enum->Values[Index] = (ml_value_t *)Value;
		Value->Base.Value = ++Index;
		stringmap_insert(Enum->Base.Exports, ml_string_value(Name), Value);
	}
	return (ml_value_t *)Enum;
}

ml_type_t *ml_enum(const char *TypeName, ...) {
	va_list Args;
	int Size = 0;
	va_start(Args, TypeName);
	while (va_arg(Args, const char *)) ++Size;
	va_end(Args);
	ml_enum_t *Enum = xnew(ml_enum_t, Size, ml_value_t *);
	Enum->Base.Type = MLEnumT;
	Enum->Base.Name = TypeName;
	Enum->Base.deref = ml_default_deref;
	Enum->Base.assign = ml_default_assign;
	Enum->Base.hash = (void *)ml_enum_value_hash;
	Enum->Base.call = ml_default_call;
	Enum->Base.Rank = 1;
	ml_type_init((ml_type_t *)Enum, MLEnumValueT, NULL);
	Enum->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	int Index = 0;
	va_start(Args, TypeName);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_value_t *Name = ml_cstring(String);
		ml_enum_value_t *Value = new(ml_enum_value_t);
		Value->Base.Type = (ml_type_t *)Enum;
		Value->Name = Name;
		Enum->Values[Index] = (ml_value_t *)Value;
		Value->Base.Value = ++Index;
		stringmap_insert(Enum->Base.Exports, String, Value);
	}
	return (ml_type_t *)Enum;
}

uint64_t ml_enum_value(ml_value_t *Value) {
	return (uint64_t)((ml_enum_value_t *)Value)->Base.Value;
}

static void ml_enum_call(ml_state_t *Caller, ml_enum_t *Enum, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	if (ml_is(Args[0], MLStringT)) {
		ml_value_t *Value = stringmap_search(Enum->Base.Exports, ml_string_value(Args[0]));
		if (!Value) ML_ERROR("EnumError", "Invalid enum name");
		ML_RETURN(Value);
	} else if (ml_is(Args[0], MLIntegerT)) {
		int Index = ml_integer_value_fast(Args[0]);
		if (Index <= 0 || Index > Enum->Base.Exports->Size) ML_ERROR("EnumError", "Invalid enum index");
		ML_RETURN(Enum->Values[Index - 1]);
	} else {
		ML_ERROR("TypeError", "Expected <integer> or <string> not <%s>", ml_typeof(Args[0])->Name);
	}
}

ML_TYPE(MLEnumT, (MLTypeT, MLIteratableT), "enum",
	.call = (void *)ml_enum_call,
	.Constructor = (void *)MLEnum
);

ML_METHOD("count", MLEnumT) {
	ml_enum_t *Enum = (ml_enum_t *)Args[0];
	return ml_integer(Enum->Base.Exports->Size);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t **Values;
	int Index, Size;
} ml_enum_iter_t;

ML_TYPE(MLEnumIterT, (), "enum-iter");

static void ML_TYPED_FN(ml_iterate, MLEnumT, ml_state_t *Caller, ml_enum_t *Enum) {
	int Size = Enum->Base.Exports->Size;
	if (!Size) ML_RETURN(MLNil);
	ml_enum_iter_t *Iter = new(ml_enum_iter_t);
	Iter->Type = MLEnumIterT;
	Iter->Values = Enum->Values;
	Iter->Index = 0;
	Iter->Size = Size;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLEnumIterT, ml_state_t *Caller, ml_enum_iter_t *Iter) {
	if (++Iter->Index == Iter->Size) ML_RETURN(MLNil);
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLEnumIterT, ml_state_t *Caller, ml_enum_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index + 1));
}

static void ML_TYPED_FN(ml_iter_value, MLEnumIterT, ml_state_t *Caller, ml_enum_iter_t *Iter) {
	ML_RETURN(Iter->Values[Iter->Index]);
}

typedef struct {
	ml_type_t Base;
	ml_value_t *Names[];
} ml_flags_t;

#ifdef ML_GENERICS
typedef ml_int64_t ml_flags_value_t;
#else
typedef ml_integer_t ml_flags_value_t;
#endif

extern ml_type_t MLFlagsT[];

static long ml_flag_value_hash(ml_flags_value_t *Value, ml_hash_chain_t *Chain) {
	return (long)Value->Type + Value->Value;
}

#ifdef ML_GENERICS
ML_TYPE(MLFlagsValueT, (MLInt64T), "flag-value");
#else
ML_TYPE(MLFlagsValueT, (MLIntegerT), "flag-value");
#endif

ML_METHOD(MLStringT, MLFlagsValueT) {
	ml_flags_value_t *Value = (ml_flags_value_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	uint64_t Flags = Value->Value;
	ml_value_t **Names = ((ml_flags_t *)Value->Type)->Names;
	while (Flags) {
		if (Flags & 1) {
			if (Buffer->Length) ml_stringbuffer_add(Buffer, "|", 1);
			ml_stringbuffer_add(Buffer, ml_string_value(Names[0]), ml_string_length(Names[0]));
		}
		++Names;
		Flags >>= 1;
	}
	return ml_stringbuffer_value(Buffer);
}

ML_FUNCTION(MLFlags) {
//@flags
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLListT);
	int Size = ml_list_length(Args[1]);
	ml_flags_t *Flags = xnew(ml_flags_t, Size, ml_value_t *);
	Flags->Base.Type = MLFlagsT;
	Flags->Base.Name = ml_string_value(Args[0]);
	Flags->Base.deref = ml_default_deref;
	Flags->Base.assign = ml_default_assign;
	Flags->Base.hash = (void *)ml_flag_value_hash;
	Flags->Base.call = ml_default_call;
	Flags->Base.Rank = 1;
	ml_type_init((ml_type_t *)Flags, MLFlagsValueT, NULL);
	Flags->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	uint64_t Flag = 1;
	int Index = 0;
	ML_LIST_FOREACH(Args[1], Iter) {
		ml_value_t *Name = Iter->Value;
		if (!ml_is(Name, MLStringT)) {
			return ml_error("TypeError", "Flag names must be strings");
		}
		ml_flags_value_t *Value = new(ml_flags_value_t);
		Value->Type = (ml_type_t *)Flags;
		Flags->Names[Index++] = Name;
		Value->Value = Flag;
		Flag <<= 1;
		stringmap_insert(Flags->Base.Exports, ml_string_value(Name), Value);
	}
	return (ml_value_t *)Flags;
}

ml_type_t *ml_flags(const char *TypeName, ...) {
	va_list Args;
	int Size = 0;
	va_start(Args, TypeName);
	while (va_arg(Args, const char *)) ++Size;
	va_end(Args);
	ml_flags_t *Flags = xnew(ml_flags_t, Size, ml_value_t *);
	Flags->Base.Type = MLFlagsT;
	Flags->Base.Name = TypeName;
	Flags->Base.deref = ml_default_deref;
	Flags->Base.assign = ml_default_assign;
	Flags->Base.hash = (void *)ml_flag_value_hash;
	Flags->Base.call = ml_default_call;
	Flags->Base.Rank = 1;
	ml_type_init((ml_type_t *)Flags, MLFlagsValueT, NULL);
	Flags->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	uint64_t Flag = 1;
	int Index = 0;
	va_start(Args, TypeName);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_value_t *Name = ml_cstring(String);
		ml_flags_value_t *Value = new(ml_flags_value_t);
		Value->Type = (ml_type_t *)Flags;
		Flags->Names[Index++] = Name;
		Value->Value = Flag;
		Flag <<= 1;
		stringmap_insert(Flags->Base.Exports, String, Value);
	}
	return (ml_type_t *)Flags;
}

uint64_t ml_flags_value(ml_value_t *Value) {
	return (uint64_t)((ml_flags_value_t *)Value)->Value;
}

static void ml_flags_call(ml_state_t *Caller, ml_enum_t *Enum, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	if (ml_is(Args[0], MLStringT)) {
		ml_value_t *Value = stringmap_search(Enum->Base.Exports, ml_string_value(Args[0]));
		if (!Value) ML_ERROR("EnumError", "Invalid enum name");
		ML_RETURN(Value);
	} else if (ml_is(Args[0], MLIntegerT)) {
		int Index = ml_integer_value_fast(Args[0]);
		if (Index <= 0 || Index > Enum->Base.Exports->Size) ML_ERROR("EnumError", "Invalid enum index");
		ML_RETURN(Enum->Values[Index - 1]);
	} else {
		ML_ERROR("TypeError", "Expected <integer> or <string> not <%s>", ml_typeof(Args[0])->Name);
	}
}

ML_TYPE(MLFlagsT, (MLTypeT), "flags",
	.call = (void *)ml_flags_call,
	.Constructor = (void *)MLFlags
);

ML_METHOD("+", MLFlagsValueT, MLFlagsValueT) {
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, A->Type);
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	ml_flags_value_t *C = new(ml_flags_value_t);
	C->Type = A->Type;
	C->Value = A->Value | B->Value;
	return (ml_value_t *)C;
}

ML_METHOD("-", MLFlagsValueT, MLFlagsValueT) {
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, A->Type);
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	ml_flags_value_t *C = new(ml_flags_value_t);
	C->Type = A->Type;
	C->Value = A->Value & ~B->Value;
	return (ml_value_t *)C;
}

ML_METHOD("<", MLFlagsValueT, MLFlagsValueT) {
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, A->Type);
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	if ((A->Value & B->Value) == A->Value) {
		return Args[1];
	} else {
		return MLNil;
	}
}

ML_METHOD("<=", MLFlagsValueT, MLFlagsValueT) {
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, A->Type);
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	if ((A->Value & B->Value) == A->Value) {
		return Args[1];
	} else {
		return MLNil;
	}
}

ML_METHOD(">", MLFlagsValueT, MLFlagsValueT) {
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, A->Type);
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	if ((A->Value & B->Value) == B->Value) {
		return Args[1];
	} else {
		return MLNil;
	}
}

ML_METHOD(">=", MLFlagsValueT, MLFlagsValueT) {
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, A->Type);
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	if ((A->Value & B->Value) == B->Value) {
		return Args[1];
	} else {
		return MLNil;
	}
}

void ml_object_init(stringmap_t *Globals) {
#include "ml_object_init.c"
	if (Globals) {
		stringmap_insert(Globals, "property", MLPropertyT);
		stringmap_insert(Globals, "object", MLObjectT);
		stringmap_insert(Globals, "class", MLClassT);
		stringmap_insert(Globals, "enum", MLEnumT);
		stringmap_insert(Globals, "flags", MLFlagsT);
	}
}

#include "minilang.h"
#include "ml_uuid.h"
#include "ml_macros.h"
#include "ml_object.h"
#include <string.h>
#include <stdarg.h>

#undef ML_CATEGORY
#define ML_CATEGORY "object"

static ml_type_t *ml_default_object_table_lookup(ml_object_table_t *_ObjectTable, uuid_t Id) {
	ml_default_object_table_t *ObjectTable = (ml_default_object_table_t *)_ObjectTable;
	return uuidmap_search(ObjectTable->Types, Id);
}

static ml_value_t *ml_default_object_table_insert(ml_object_table_t *_ObjectTable, ml_type_t *Type) {
	ml_default_object_table_t *ObjectTable = (ml_default_object_table_t *)_ObjectTable;
	ml_type_t **Slot = (ml_type_t **)uuidmap_slot(ObjectTable->Types, Type->Id);
	if (Slot[0]) {
		char IdString[UUID_STR_LEN];
		uuid_unparse_lower(Type->Id, IdString);
		return ml_error("ClassError", "Duplicate class id: %s", IdString);
	}
	Slot[0] = Type;
	return NULL;
}

static ml_default_object_table_t MLRootObjectTable[1] = {{
	{NULL, ml_default_object_table_lookup, ml_default_object_table_insert},
	{UUIDMAP_INIT}
}};

static ml_value_t *ml_field_deref(ml_field_t *Field) {
	return Field->Value;
}

ML_TYPE(MLFieldT, (), "field",
	.deref = (void *)ml_field_deref
);

static void ml_field_assign(ml_state_t *Caller, ml_field_t *Field, ml_value_t *Value) {
	if (ml_typeof(Value) == MLUninitializedT) ml_uninitialized_use(Value, &Field->Value);
	Field->Value = Value;
	ML_RETURN(Value);
}

ML_TYPE(MLFieldMutableT, (MLFieldT), "field::mutable",
	.deref = (void *)ml_field_deref,
	.assign = (void *)ml_field_assign
);

ml_value_t *ml_field_fn(void *Data, int Count, ml_value_t **Args) {
	ml_object_t *Object = (ml_object_t *)Args[0];
	return (ml_value_t *)((char *)Object + (uintptr_t)Data);
}

ML_TYPE(MLFieldOwnerT, (), "field::owner");
//!internal

ml_object_t *ml_field_owner(ml_field_t *Field) {
	do --Field; while (Field->Type != MLFieldOwnerT);
	return (ml_object_t *)Field->Value;
}

ml_value_t *ml_field_name(ml_field_t *Field) {
	int Index = 0;
	do { ++Index; --Field; } while (Field->Type != MLFieldOwnerT);
	ml_class_t *Class = ((ml_object_t *)Field->Value)->Type;
	ml_field_info_t *Info = Class->Fields;
	while (--Index) Info = Info->Next;
	return Info->Method;
}

ML_INTERFACE(MLObjectT, (), "object");
// Parent type of all object classes.

typedef struct {
	ml_state_t Base;
	ml_value_t *Object;
	ml_value_t *Args[];
} ml_init_state_t;

static void ml_init_state_run(ml_init_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	ml_object_t *Object = (ml_object_t *)State->Object;
	ml_class_t *Class = (ml_class_t *)State->Object->Type;
	ml_field_t *Field = Object->Fields + 1;
	for (ml_field_info_t *Info = Class->Fields; Info; Info = Info->Next) {
		(Field++)->Type = Info->Type;
	}
	ML_RETURN(State->Object);
}

static void ml_object_constructor_fn(ml_state_t *Caller, ml_class_t *Class, int Count, ml_value_t **Args) {
	ml_object_t *Object = xnew(ml_object_t, Class->NumFields + 1, ml_field_t);
	Object->Type = Class;
	ml_field_t *Slot = Object->Fields;
	Slot->Type = MLFieldOwnerT;
	Slot->Value = (ml_value_t *)Object;
	++Slot;
	for (int I = Class->NumFields + 1; --I > 0; ++Slot) {
		Slot->Type = MLFieldMutableT;
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
		ml_value_t *Arg = ml_deref(Args[I]);
		if (ml_is_error(Arg)) ML_RETURN(Arg);
		if (ml_typeof(Arg) == MLNamesT) {
			ML_NAMES_CHECKX_ARG_COUNT(I);
			ml_value_t **Arg2 = Args + I;
			ML_NAMES_FOREACH(Args[I], Iter) {
				const char *Name = ml_string_value(Iter->Value);
				ml_field_info_t *Info = stringmap_search(Class->Names, Name);
				if (!Info) {
					ML_ERROR("ValueError", "Class %s does not have field %s", Class->Base.Name, Name);
				}
				ml_field_t *Field = &Object->Fields[Info->Index];
				ml_value_t *Value = *++Arg2;
				if (ml_typeof(Value) == MLUninitializedT) ml_uninitialized_use(Value, &Field->Value);
				Field->Value = Value;
			}
			break;
		} else if (I >= Class->NumFields) {
			break;
		} else {
			if (ml_typeof(Arg) == MLUninitializedT) ml_uninitialized_use(Arg, &Object->Fields[I + 1].Value);
			Object->Fields[I + 1].Value = Arg;
		}
	}
	if (Class->Defaults) {
		ml_init_state_t *State = xnew(ml_init_state_t, Count + 1, ml_value_t *);
		State->Args[0] = (ml_value_t *)Object;
		for (int I = 0; I < Count; ++I) State->Args[I + 1] = Args[I];
		State->Base.run = (void *)ml_init_state_run;
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Object = (ml_value_t *)Object;
		return ml_call(State, Class->Defaults, 1, State->Args);
	}
	ml_field_t *Field = Object->Fields + 1;
	for (ml_field_info_t *Info = Class->Fields; Info; Info = Info->Next) {
		(Field++)->Type = Info->Type;
	}
	ML_RETURN(Object);
}

static void ML_TYPED_FN(ml_value_find_all, MLObjectT, ml_object_t *Value, void *Data, ml_value_find_fn RefFn) {
	if (!RefFn(Data, (ml_value_t *)Value, 1)) return;
	int NumFields = ((ml_object_t *)Value)->Type->NumFields;
	for (int I = 1; I <= NumFields; ++I) ml_value_find_all(Value->Fields[I].Value, Data, RefFn);
}

static int ML_TYPED_FN(ml_value_is_constant, MLObjectT, ml_object_t *Value) {
	int NumFields = ((ml_object_t *)Value)->Type->NumFields;
	for (int I = 1; I <= NumFields; ++I) {
		if (Value->Fields[I].Type != MLFieldT) return 0;
		if (!ml_value_is_constant(Value->Fields[I].Value)) return 0;
	}
	return 1;
}

ML_METHOD("::", MLObjectT, MLStringT) {
//<Object
//<Field
//>field
// Retrieves the field :mini:`Field` from :mini:`Object`. Mainly intended for unpacking objects.
	ml_object_t *Object = (ml_object_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_field_info_t *Info = stringmap_search(Object->Type->Names, Name);
	if (!Info) return ml_error("NameError", "Type %s has no field %s", Object->Type->Base.Name, Name);
	return (ml_value_t *)&Object->Fields[Info->Index];
}

extern ml_cfunctionx_t MLClass[];

ML_TYPE(MLClassT, (MLTypeT), "class",
//!object
// Type of all object classes.
	.call = (void *)ml_type_call,
	.Constructor = (ml_value_t *)MLClass
);

ML_METHOD("fields", MLClassT) {
	ml_class_t *Class = (ml_class_t *)Args[0];
	ml_value_t *Fields = ml_list();
	for (ml_field_info_t *Info = Class->Fields; Info; Info = Info->Next) {
		ml_list_put(Fields, Info->Method);
	}
	return Fields;
}

typedef struct {
	ml_state_t Base;
	ml_stringbuffer_t *Buffer;
	ml_object_t *Object;
	ml_field_info_t *Info;
	ml_value_t *Args[2];
	ml_hash_chain_t Chain[1];
} ml_object_append_state_t;

extern ml_value_t *AppendMethod;

static void ml_object_append_state_run(ml_object_append_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_field_info_t *Info = State->Info->Next;
skip:
	if (!Info) {
		ml_stringbuffer_put(State->Buffer, ')');
		if (State->Chain->Index) ml_stringbuffer_printf(State->Buffer, "<%d", State->Chain->Index);
		State->Buffer->Chain = State->Chain->Previous;
		ML_CONTINUE(State->Base.Caller, MLSome);
	}
	const char *Name = ml_method_name(Info->Method);
	if (!Name) {
		Info = Info->Next;
		goto skip;
	}
	ml_stringbuffer_write(State->Buffer, ", ", 2);
	ml_stringbuffer_write(State->Buffer, Name, strlen(Name));
	ml_stringbuffer_write(State->Buffer, " is ", 4);
	State->Info = Info;
	State->Args[1] = State->Object->Fields[Info->Index].Value;
	return ml_call(State, AppendMethod, 2, State->Args);
}

ML_METHODX("append", MLStringBufferT, MLObjectT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_object_t *Object = (ml_object_t *)Args[1];
	for (ml_hash_chain_t *Link = Buffer->Chain; Link; Link = Link->Previous) {
		if (Link->Value == (ml_value_t *)Object) {
			int Index = Link->Index;
			if (!Index) Index = Link->Index = ++Buffer->Index;
			ml_stringbuffer_printf(Buffer, ">%d", Index);
			ML_RETURN(Buffer);
		}
	}
	ml_stringbuffer_printf(Buffer, "%s(", Object->Type->Base.Name);
	for (ml_field_info_t *Info = Object->Type->Fields; Info; Info = Info->Next) {
		const char *Name = ml_method_name(Info->Method);
		if (Name) {
			ml_stringbuffer_write(Buffer, Name, strlen(Name));
			ml_stringbuffer_write(Buffer, " is ", 4);
			ml_object_append_state_t *State = new(ml_object_append_state_t);
			State->Base.Caller = Caller;
			State->Base.Context = Caller->Context;
			State->Base.run = (ml_state_fn)ml_object_append_state_run;
			State->Chain->Previous = Buffer->Chain;
			State->Chain->Value = (ml_value_t *)Object;
			Buffer->Chain = State->Chain;
			State->Buffer = Buffer;
			State->Object = Object;
			State->Info = Info;
			State->Args[0] = (ml_value_t *)Buffer;
			State->Args[1] = Object->Fields[Info->Index].Value;
			return ml_call(State, AppendMethod, 2, State->Args);
		}
	}
	ml_stringbuffer_put(Buffer, ')');
	ML_RETURN(MLSome);
}

typedef struct {
	ml_type_t Base;
	ml_type_t *Native;
	ml_value_t *Constructor;
	ml_value_t *Initializer;
} ml_named_type_t;

ML_TYPE(MLNamedTypeT, (MLTypeT), "named-type",
//!internal
	.call = (void *)ml_type_call
);

typedef struct {
	ml_state_t Base;
	ml_value_t *Object;
	ml_type_t *Old, *New;
	ml_value_t *Init;
	int Count;
	ml_value_t *Args[];
} ml_named_init_state_t;

static void ml_named_init_state_init(ml_init_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	ML_RETURN(State->Object);
}

static void ml_named_init_state_run(ml_named_init_state_t *State, ml_value_t *Result) {
	ml_type_t *Old = ml_typeof(Result);
	if (Old == State->Old) {
		Result->Type = State->New;
#ifdef ML_GENERICS
	} else if (Old->Type == MLTypeGenericT && ml_generic_type_args(Old)[0] == State->Old) {
		Result->Type = State->New;
#endif
	} else {
		ML_CONTINUE(State->Base.Caller, Result);
	}
	if (State->Init) {
		State->Object = State->Args[0] = Result;
		State->Base.run = (void *)ml_named_init_state_init;
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

static ml_value_t *get_field_fn(int Index) {
	static ml_value_t **FieldFns = NULL;
	static int NumFieldFns = 0;
	if (Index >= NumFieldFns) {
		ml_value_t **NewFieldFns = anew(ml_value_t *, Index + 1);
		memcpy(NewFieldFns, FieldFns, NumFieldFns * sizeof(ml_value_t *));
		for (int I = NumFieldFns; I <= Index; ++I) {
			void *Offset = &((ml_object_t *)0)->Fields[I];
			NewFieldFns[I] = ml_cfunction(Offset, ml_field_fn);
		}
		FieldFns = NewFieldFns;
		NumFieldFns = Index + 1;
	}
	return FieldFns[Index];
}

static void add_field(ml_context_t *Context, ml_class_t *Class, ml_value_t *Method, ml_type_t *Type) {
	ml_field_info_t **Slot = &Class->Fields;
	int Index = 1;
	while (Slot[0]) {
		if (Slot[0]->Method == Method) return;
		++Index;
		Slot = &Slot[0]->Next;
	}
	++Class->NumFields;
	ml_field_info_t *Info = Slot[0] = new(ml_field_info_t);
	Info->Method = Method;
	Info->Type = Type;
	Info->Index = Index;
	const char *Name = ml_method_name(Method);
	if (Name) stringmap_insert(Class->Names, Name, Info);
	ml_methods_t *Methods = ml_context_get_static(Context, ML_METHODS_INDEX);
	ml_type_t *Types[1] = {(ml_type_t *)Class};
	ml_method_insert(Methods, (ml_method_t *)Info->Method, get_field_fn(Index), 1, NULL, Types);
}

ml_type_t *ml_class(const char *Name) {
	ml_class_t *Class = new(ml_class_t);
	Class->Base.Type = MLClassT;
	if (Name) {
		Class->Base.Name = Name;
	} else {
		GC_asprintf((char **)&Class->Base.Name, "class:%lx", (uintptr_t)Class);
	}
	Class->Base.hash = ml_default_hash;
	Class->Base.call = ml_default_call;
	Class->Base.deref = ml_default_deref;
	Class->Base.assign = ml_default_assign;
	Class->Base.iterate = ml_iterate;
	Class->Base.iter_value = ml_iter_value;
	Class->Base.iter_key = ml_iter_key;
	Class->Base.iter_next = ml_iter_next;
	ml_value_t *Constructor = ml_cfunctionx(Class, (void *)ml_object_constructor_fn);
	Class->Base.Constructor = Constructor;
	ml_type_add_parent((ml_type_t *)Class, MLObjectT);
	stringmap_insert(Class->Base.Exports, "new", Constructor);
	return (ml_type_t *)Class;
}

void ml_class_add_parent(ml_context_t *Context, ml_type_t *Class0, ml_type_t *Parent0) {
	Context = Context ?: MLRootContext;
	ml_class_t *Class = (ml_class_t *)Class0;
	ml_class_t *Parent = (ml_class_t *)Parent0;
	for (ml_field_info_t *Info = Parent->Fields; Info; Info = Info->Next) {
		add_field(Context, Class, Info->Method, Info->Type);
	}
	ml_type_add_parent((ml_type_t *)Class, (ml_type_t *)Parent);
}

void ml_class_add_field(ml_context_t *Context, ml_type_t *Class0, ml_value_t *Field, ml_type_t *Type) {
	Context = Context ?: MLRootContext;
	add_field(Context, (ml_class_t *)Class0, Field, Type);
}

static long ml_object_hash(ml_object_t *Object, ml_hash_chain_t *Chain) {
	ml_class_t *Class = Object->Type;
	long Hash = 5381;
	for (const char *P = Class->Base.Name; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	for (int I = 1; I <= Class->NumFields; ++I) {
		ml_value_t *Field = Object->Fields[I].Value;
		Hash ^= ml_hash_chain(Field, Chain);
	}
	return Hash;
}

static void ml_object_call(ml_state_t *Caller, ml_object_t *Object, int Count, ml_value_t **Args) {
	ml_value_t **Args2 = ml_alloc_args(Count + 1);
	memmove(Args2 + 1, Args, Count * sizeof(ml_value_t *));
	Args2[0] = (ml_value_t *)Object;
	return ml_call(Caller, Object->Type->Call, Count + 1, Args2);
}

ml_value_t *ml_class_modify(ml_context_t *Context, ml_class_t *Class, ml_value_t *Modifier) {
	typeof(ml_class_modify) *function = ml_typed_fn_get(ml_typeof(Modifier), ml_class_modify);
	if (function) return function(Context, Class, Modifier);
	return ml_error("TypeError", "Unexpected class modifier: <%s>", ml_typeof(Modifier)->Name);
}

static ml_value_t *ML_TYPED_FN(ml_class_modify, MLMethodT, ml_context_t *Context, ml_class_t *Class, ml_value_t *Field) {
	add_field(Context, Class, Field, MLFieldMutableT);
	return NULL;
}

static int ml_class_add_export(const char *Name, void *Value, ml_class_t *Class) {
	stringmap_insert(Class->Base.Exports, Name, Value);
	return 0;
}

static ml_value_t *ML_TYPED_FN(ml_class_modify, MLClassT, ml_context_t *Context, ml_class_t *Class, ml_class_t *Parent) {
	for (ml_field_info_t *Info = Parent->Fields; Info; Info = Info->Next) {
		add_field(Context, Class, Info->Method, Info->Type);
	}
	ml_type_add_parent((ml_type_t *)Class, (ml_type_t *)Parent);
	stringmap_foreach(Parent->Base.Exports, Class, (void *)ml_class_add_export);
	if (Parent->Call) {
		Class->Base.call = (void *)ml_object_call;
		Class->Call = Parent->Call;
		ml_type_add_parent((ml_type_t *)Class, MLFunctionT);
	}
	if (Parent->Base.hash == (void *)ml_object_hash) {
		Class->Base.hash = (void *)ml_object_hash;
	} else if (Parent->Base.hash == (void *)ml_value_hash) {
		Class->Base.hash = (void *)ml_value_hash;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_class_modify, MLTypeT, ml_context_t *Context, ml_class_t *Class, ml_type_t *Parent) {
	if (Parent->NoInherit) return ml_error("TypeError", "Classes can not inherit from <%s>", Parent->Name);
	ml_type_add_parent((ml_type_t *)Class, Parent);
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_class_modify, MLUUIDT, ml_context_t *Context, ml_class_t *Class, ml_uuid_t *Id) {
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_class_modify, MLFunctionT, ml_context_t *Context, ml_class_t *Class, ml_value_t *Function) {
	Class->Defaults = Function;
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_class_modify, MLStringT, ml_context_t *Context, ml_class_t *Class, ml_value_t *Option) {
	return ml_error("ValueError", "Unknown option %s", ml_string_value(Option));
}

ML_FUNCTIONZ(MLClass) {
//!object
//@class
//<Parents...:class
//<Fields...:method
//<Exports...:names
//>class
// Returns a new class inheriting from :mini:`Parents`, with fields :mini:`Fields` and exports :mini:`Exports`. The special exports :mini:`::of` and :mini:`::init` can be set to override the default conversion and initialization behaviour. The :mini:`::new` export will *always* be set to the original constructor for this class.
	ml_type_t *NativeType = NULL;
	int NumFields = 0;
	ml_value_t *Id = NULL;
	for (int I = 0; I < Count; ++I) {
		if (ml_typeof(Args[I]) == MLNamesT) {
			ML_NAMES_CHECKX_ARG_COUNT(I);
			break;
		}
		Args[I] = ml_deref(Args[I]);
		if (ml_typeof(Args[I]) == MLMethodT) {
			++NumFields;
		} else if (ml_is(Args[I], MLClassT)) {
		} else if (ml_is(Args[I], MLUUIDT)) {
			Id = Args[I];
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
		}
	}
	if (NativeType) {
		ml_named_type_t *Class = new(ml_named_type_t);
		Class->Base.Type = MLNamedTypeT;
		GC_asprintf((char **)&Class->Base.Name, "named-%s:%lx", NativeType->Name, (uintptr_t)Class);
		Class->Base.hash = NativeType->hash;
		Class->Base.call = NativeType->call;
		Class->Base.deref = NativeType->deref;
		Class->Base.assign = NativeType->assign;
		Class->Base.iterate = NativeType->iterate;
		Class->Base.iter_value = NativeType->iter_value;
		Class->Base.iter_key = NativeType->iter_key;
		Class->Base.iter_next = NativeType->iter_next;
		Class->Native = NativeType;
		ml_value_t *Constructor = ml_cfunctionz(Class, (void *)ml_named_constructor_fn);
		Class->Base.Constructor = Constructor;
		for (int I = 0; I < Count; ++I) {
			if (ml_typeof(Args[I]) == MLNamesT) {
				ML_NAMES_FOREACH(Args[I], Iter) {
					ml_value_t *Key = Iter->Value;
					const char *Name = ml_string_value(Key);
					ml_value_t *Value = Args[++I];
					stringmap_insert(Class->Base.Exports, Name, Value);
					if (!strcmp(Name, "of")) {
						Class->Base.Constructor = Value;
					} else if (!strcmp(Name, "init")) {
						Class->Initializer = Value;
						Class->Base.Constructor = ml_cfunctionz(Class, (void *)ml_named_initializer_fn);
					}
				}
				break;
			} else if (ml_is(Args[I], MLTypeT)) {
				ml_type_t *Parent = (ml_type_t *)Args[I];
				ml_type_add_parent((ml_type_t *)Class, Parent);
			} else {
				ML_ERROR("TypeError", "Unexpected argument type: <%s>", ml_typeof(Args[I])->Name);
			}
		}
		stringmap_insert(Class->Base.Exports, "new", Constructor);
		ML_RETURN(Class);
	} else {
		ml_class_t *Class;
		if (Id) {
			for (ml_object_table_t *ObjectTable = ml_context_get_static(Caller->Context, ML_OBJECTS_INDEX); ObjectTable; ObjectTable = ObjectTable->Prev) {
				Class = (ml_class_t *)ObjectTable->lookup(ObjectTable, ml_uuid_value(Id));
				if (Class) {
					if (Class->Base.Type != MLPseudoClassT) ML_ERROR("ClassError", "Class id must be unique");
					if (Class->NumFields < NumFields) ML_ERROR("ClassError", "Class id previously declared with more fields");
					goto have_class;
				}
			}
		}
		Class = new(ml_class_t);
	have_class:
		GC_asprintf((char **)&Class->Base.Name, "class:%lx", (uintptr_t)Class);
#ifdef ML_GENERICS
		ml_type_t *TypeArgs[2] = {MLClassT, (ml_type_t *)Class};
		Class->Base.Type = ml_generic_type(2, TypeArgs);
#else
		Class->Base.Type = MLClassT;
#endif
		Class->Base.hash = ml_default_hash;
		Class->Base.call = ml_default_call;
		Class->Base.deref = ml_default_deref;
		Class->Base.assign = ml_default_assign;
		Class->Base.iterate = ml_iterate;
		Class->Base.iter_value = ml_iter_value;
		Class->Base.iter_key = ml_iter_key;
		Class->Base.iter_next = ml_iter_next;
		ml_value_t *Constructor = ml_cfunctionz(Class, (void *)ml_object_constructor_fn);
		Class->Base.Constructor = Constructor;
		for (int I = 0; I < Count; ++I) {
			if (ml_is(Args[I], MLNamesT)) {
				ML_NAMES_FOREACH(Args[I], Iter) {
					ml_value_t *Key = Iter->Value;
					const char *Name = ml_string_value(Key);
					ml_value_t *Value = Args[++I];
					stringmap_insert(Class->Base.Exports, Name, Value);
					if (!strcmp(Name, "of")) {
						Class->Base.Constructor = Value;
					} else if (!strcmp(Name, "#") || !strcmp(Name, "hash")) {
						Value = ml_deref(Value);
						if (ml_is(Value, MLStringT)) {
							const char *Hash = ml_string_value(Value);
							if (!strcmp(Hash, "value")) {
								Class->Base.hash = ml_value_hash;
							} else if (!strcmp(Hash, "type")) {
								Class->Base.hash = ml_default_hash;
							} else if (!strcmp(Hash, "all")) {
								Class->Base.hash = (void *)ml_object_hash;
							} else {
								ML_ERROR("ValueError", "Unknown hash option: %s", Hash);
							}
						}
					} else if (!strcmp(Name, "init")) {
						Class->Initializer = Value;
					} else if (!strcmp(Name, "defaults")) {
						Class->Defaults = Value;
					} else if (!strcmp(Name, "()") || !strcmp(Name, "call")) {
						Class->Base.call = (void *)ml_object_call;
						Class->Call = Value;
						ml_type_add_parent((ml_type_t *)Class, MLFunctionT);
					}
				}
				break;
			} else {
				ml_value_t *Error = ml_class_modify(Caller->Context, Class, ml_deref(Args[I]));
				if (Error) ML_RETURN(Error);
			}
		}
		if (Class->Initializer && Class->Defaults) ML_ERROR("CallError", "Only one of init and defaults can be specified");
		if (Id) {
			memcpy(Class->Base.Id, ml_uuid_value(Id), sizeof(uuid_t));
		} else {
			uuid_generate(Class->Base.Id);
		}
		ml_object_table_t *ObjectTable = ml_context_get_static(Caller->Context, ML_OBJECTS_INDEX);
		if (ObjectTable) ObjectTable->insert(ObjectTable, (ml_type_t *)Class);
		ml_type_add_parent((ml_type_t *)Class, MLObjectT);
		stringmap_insert(Class->Base.Exports, "new", Constructor);
		ML_RETURN(Class);
	}
}

static int ml_class_set_name_fn(const char *Export, ml_value_t *Value, const char *Prefix) {
	const char *Name;
	GC_asprintf((char **)&Name, "%s::%s", Prefix, Export);
	ml_value_set_name(Value, Name);
	return 0;
}

static void ML_TYPED_FN(ml_value_set_name, MLNamedTypeT, ml_named_type_t *Class, const char *Name) {
	Class->Base.Name = Name;
	stringmap_foreach(Class->Base.Exports, (void *)Name, (void *)ml_class_set_name_fn);
}

static void ML_TYPED_FN(ml_value_set_name, MLClassT, ml_class_t *Class, const char *Name) {
	Class->Base.Name = Name;
	GC_asprintf((char **)&Class->Base.Type->Name, "class[%s]", Name);
	stringmap_foreach(Class->Base.Exports, (void *)Name, (void *)ml_class_set_name_fn);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Field;
	ml_type_t *FieldType;
} ml_modified_field_t;

ML_TYPE(MLModifiedFieldT, (), "modified-field");
//!internal

ml_value_t *ml_modified_field(ml_value_t *Field, ml_type_t *Type) {
	ml_modified_field_t *Typed = new(ml_modified_field_t);
	Typed->Type = MLModifiedFieldT;
	Typed->Field = Field;
	Typed->FieldType = Type;
	return (ml_value_t *)Typed;
}

static ml_value_t *ML_TYPED_FN(ml_class_modify, MLModifiedFieldT, ml_context_t *Context, ml_class_t *Class, ml_modified_field_t *Modified) {
	add_field(Context, Class, Modified->Field, Modified->FieldType);
	return NULL;
}

typedef struct {
	ml_type_t *Type;
	ml_type_t *FieldType;
} ml_field_modifier_t;

ML_TYPE(MLFieldModifierT, (), "field-modifier");
//!internal

ml_value_t *ml_field_modifier(ml_type_t *Type) {
	ml_field_modifier_t *Modifier = new(ml_field_modifier_t);
	Modifier->Type = MLFieldModifierT;
	Modifier->FieldType = Type;
	return (ml_value_t *)Modifier;
}

ML_METHOD(MLMethodDefault, MLMethodT, MLFieldModifierT) {
	ml_field_modifier_t *Modifier = (ml_field_modifier_t *)Args[1];
	return ml_modified_field(Args[0], Modifier->FieldType);
}

typedef struct {
	ml_type_t Base;
	ml_value_t *Callback;
} ml_watcher_type_t;

static ml_value_t *ml_watched_field_deref(ml_field_t *Field) {
	return Field->Value;
}

static void ml_watched_field_assign(ml_state_t *Caller, ml_field_t *Field, ml_value_t *Value) {
	Field->Value = Value;
	ml_watcher_type_t *Watcher = (ml_watcher_type_t *)Field->Type;
	int Index = 0;
	do { ++Index; --Field; } while (Field->Type != MLFieldOwnerT);
	ml_object_t *Owner = (ml_object_t *)Field->Value;
	ml_field_info_t *Info = Owner->Type->Fields;
	while (--Index) Info = Info->Next;
	ml_value_t **Args = ml_alloc_args(4);
	Args[0] = Info->Method;
	Args[1] = (ml_value_t *)Owner;
	Args[2] = Value;
	Args[3] = Info->Method;
	return ml_call(Caller, Watcher->Callback, 4, Args);
}

ML_TYPE(MLWatchedT, (MLTypeT), "field-watcher");
//!internal

ml_value_t *ml_watched_field(ml_value_t *Callback) {
	ml_watcher_type_t *Watcher = new(ml_watcher_type_t);
	Watcher->Base.Type = MLWatchedT;
	GC_asprintf((char **)&Watcher->Base.Name, "watcher:%lx", (uintptr_t)Watcher);
	Watcher->Base.deref = (void *)ml_watched_field_deref;
	Watcher->Base.assign = (void *)ml_watched_field_assign;
	Watcher->Base.iterate = ml_iterate;
	Watcher->Base.iter_value = ml_iter_value;
	Watcher->Base.iter_key = ml_iter_key;
	Watcher->Base.iter_next = ml_iter_next;
	Watcher->Base.hash = ml_default_hash;
	ml_type_init((ml_type_t *)Watcher, MLFieldMutableT, NULL);
	Watcher->Callback = Callback;
	return (ml_value_t *)Watcher;
}

ML_FUNCTION(MLWatched) {
//!internal
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLFunctionT);
	return ml_watched_field(Args[0]);
}

ML_METHOD(MLMethodDefault, MLMethodT, MLWatchedT) {
	return ml_modified_field(Args[0], (ml_type_t *)Args[1]);
}

typedef struct ml_property_t {
	const ml_type_t *Type;
	ml_value_t *Value, *Setter;
} ml_property_t;

static ml_value_t *ml_property_deref(ml_property_t *Property) {
	return Property->Value;
}

static void ml_property_assign(ml_state_t *Caller, ml_property_t *Property, ml_value_t *Value) {
	return ml_call(Caller, Property->Setter, 1, &Value);
}

static void ml_property_call(ml_state_t *Caller, ml_property_t *Property, int Count, ml_value_t **Args) {
	return ml_call(Caller, Property->Value, Count, Args);
}

extern ml_type_t MLPropertyT[];

ML_FUNCTIONX(MLProperty) {
//@property
//<Value
//<Set:function
//>property
// Returns a new property which dereferences to :mini:`Value`. Assigning to the property will call :mini:`Set(NewValue)`.
	ML_CHECKX_ARG_COUNT(2);
	ml_property_t *Property = new(ml_property_t);
	Property->Type = MLPropertyT;
	Property->Value = Args[0];
	Property->Setter = Args[1];
	ML_RETURN(Property);
}

ML_TYPE(MLPropertyT, (), "property",
// A value with an associated setter function.
	.deref = (void *)ml_property_deref,
	.assign = (void *)ml_property_assign,
	.call = (void *)ml_property_call,
	.Constructor = (ml_value_t *)MLProperty
);

typedef struct {
	const char *Name;
	int Index;
} ml_class_field_find_t;

static int ml_class_field_fn(const char *Name, ml_field_info_t *Info, ml_class_field_find_t *Find) {
	if (Find->Index == Info->Index) {
		Find->Name = Name;
		return 1;
	}
	return 0;
}

const char *ml_class_field_name(const ml_type_t *Class, int Index) {
	ml_class_field_find_t Find = {NULL, Index};
	stringmap_foreach(((ml_class_t *)Class)->Names, &Find, (void *)ml_class_field_fn);
	return Find.Name;
}

ml_value_t *ml_object(ml_type_t *Class0, ...) {
	ml_class_t *Class = (ml_class_t *)Class0;
	ml_object_t *Object = xnew(ml_object_t, Class->NumFields + 1, ml_field_t);
	Object->Type = Class;
	ml_field_t *Slot = Object->Fields;
	Slot->Type = MLFieldOwnerT;
	Slot->Value = (ml_value_t *)Object;
	++Slot;
	for (int I = Class->NumFields + 1; --I > 0; ++Slot) {
		Slot->Type = MLFieldMutableT;
		Slot->Value = MLNil;
	}
	va_list Arg;
	va_start(Arg, Class0);
	const char *Name;
	while ((Name = va_arg(Arg, const char *))) {
		ml_field_info_t *Info = stringmap_search(Class->Names, Name);
		if (!Info) {
			va_end(Arg);
			return ml_error("ValueError", "Class %s does not have field %s", Class->Base.Name, Name);
		}
		ml_field_t *Field = &Object->Fields[Info->Index];
		Field->Value = va_arg(Arg, ml_value_t *);
	}
	va_end(Arg);
	for (ml_field_info_t *Info = Class->Fields; Info; Info = Info->Next) {
		Object->Fields[Info->Index].Type = Info->Type;
	}
	return (ml_value_t *)Object;
}

size_t ml_object_size(const ml_value_t *Value) {
	return ml_class_size(Value->Type);
}

ml_value_t *ml_object_field(const ml_value_t *Value, int Index) {
	return ((ml_object_t *)Value)->Fields[Index].Value;
}

void ml_object_foreach(const ml_value_t *Value, void *Data, int (*callback)(const char *, ml_value_t *, void *)) {
	ml_object_t *Object = (ml_object_t *)Value;
	ml_field_t *Field = Object->Fields + 1;
	ml_field_info_t *Info = Object->Type->Fields;
	while (Info) {
		if (callback(ml_method_name(Info->Method), Field->Value, Data)) return;
		++Field;
		Info = Info->Next;
	}
}

static void ML_TYPED_FN(ml_value_set_name, MLObjectT, ml_object_t *Object, const char *Name) {
	ml_value_t *NameField = stringmap_search(Object->Type->Base.Exports, "name");
	if (!NameField) return;
	if (!ml_is(NameField, MLMethodT)) return;
	ml_field_info_t *Info = stringmap_search(Object->Type->Names, ml_method_name(NameField));
	if (!Info) return;
	ml_field_t *Field = &Object->Fields[Info->Index];
	Field->Value = ml_string(Name, -1);
}

ML_TYPE(MLPseudoClassT, (MLClassT), "pseudo::class");
ML_TYPE(MLPseudoObjectT, (MLObjectT), "pseudo::object");

ml_class_t *ml_pseudo_class(const char *Name, const uuid_t Id) {
	ml_class_t *Class = new(ml_class_t);
	Class->Base.Type = MLPseudoClassT;
	if (Name) {
		Class->Base.Name = Name;
	} else {
		GC_asprintf((char **)&Class->Base.Name, "class:%lx", (uintptr_t)Class);
	}
	Class->Base.hash = ml_default_hash;
	Class->Base.call = ml_default_call;
	Class->Base.deref = ml_default_deref;
	Class->Base.assign = ml_default_assign;
	Class->Base.iterate = ml_iterate;
	Class->Base.iter_value = ml_iter_value;
	Class->Base.iter_key = ml_iter_key;
	Class->Base.iter_next = ml_iter_next;
	ml_value_t *Constructor = ml_cfunctionx(Class, (void *)ml_object_constructor_fn);
	Class->Base.Constructor = Constructor;
	ml_type_add_parent((ml_type_t *)Class, MLPseudoObjectT);
	stringmap_insert(Class->Base.Exports, "new", Constructor);
	memcpy(Class->Base.Id, Id, sizeof(uuid_t));
	return Class;
}

void ml_pseudo_class_add_field(ml_class_t *Class, const char *Name) {
	ml_value_t *Method = ml_method(Name);
	ml_field_info_t **Slot = &Class->Fields;
	int Index = 1;
	while (Slot[0]) {
		if (Slot[0]->Method == Method) return;
		++Index;
		Slot = &Slot[0]->Next;
	}
	++Class->NumFields;
	ml_field_info_t *Info = Slot[0] = new(ml_field_info_t);
	Info->Method = Method;
	Info->Type = MLFieldMutableT;
	Info->Index = Index;
	if (Name) stringmap_insert(Class->Names, Name, Info);
}

ML_METHOD(MLMethodDefault, MLMethodT, MLPseudoObjectT) {
	const char *Name = ml_method_name(Args[0]);
	ml_object_t *Object = (ml_object_t *)Args[1];
	ml_class_t *Class = Object->Type;
	ml_field_info_t *Info = stringmap_search(Class->Names, Name);
	if (!Info) return ml_no_method_error((ml_method_t *)Args[0], 1, Args + 1);
	ml_field_t *Field = &Object->Fields[Info->Index];
	return (ml_value_t *)Field;
}

ML_METHODX("register", MLPseudoClassT) {
	ml_class_t *Class = (ml_class_t *)Args[0];
	ml_object_table_t *ObjectTable = ml_context_get_static(Caller->Context, ML_OBJECTS_INDEX);
	if (!ObjectTable) ML_ERROR("ClassError", "No class table found");
	ml_value_t *Error = ObjectTable->insert(ObjectTable, (ml_type_t *)Class);
	if (Error) ML_RETURN(Error);
	ML_RETURN(Class);
}

ml_value_t *ml_struct_instance(ml_type_t *Type, void *Value) {
	ml_struct_instance_t *Instance = new(ml_struct_instance_t);
	Instance->Type = Type;
	Instance->Value = Value;
	return (ml_value_t *)Instance;
}

//!enum

static long ml_enum_value_hash(ml_enum_value_t *Value, ml_hash_chain_t *Chain) {
	return (long)Value->Type + Value->Value;
}

ML_TYPE(MLEnumValueT, (), "enum-value");
//@enum::value
// An instance of an enumeration type.

ML_METHOD("append", MLStringBufferT, MLEnumValueT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_enum_value_t *Value = (ml_enum_value_t *)Args[1];
	ml_stringbuffer_write(Buffer, ml_string_value(Value->Name), ml_string_length(Value->Name));
	return Args[0];
}

static void ml_enum_call(ml_state_t *Caller, ml_enum_t *Enum, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (ml_is(Arg, MLStringT)) {
		ml_value_t *Value = stringmap_search(Enum->Base.Exports, ml_string_value(Arg));
		if (!Value) ML_ERROR("EnumError", "Invalid enum name");
		ML_RETURN(Value);
	} else if (ml_is(Arg, MLIntegerT)) {
		ml_enum_value_t *Current = Enum->Values;
		int64_t Value = ml_integer_value(Arg);
		do {
			if (Current->Value == Value) ML_RETURN(Current);
			Current = Current->Next;
		} while (Current && Current != Enum->Values);
		ML_ERROR("EnumError", "Invalid enum value");
	} else {
		ML_ERROR("TypeError", "Expected <integer> or <string> not <%s>", ml_typeof(Arg)->Name);
	}
}

static void ml_enum_constructor(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ml_value_t *Id = NULL;
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I];
		if (ml_typeof(Arg) == MLNamesT) {
			ML_NAMES_CHECKX_ARG_COUNT(I);
			break;
		} else if (ml_is(Arg, MLStringT)) {
		} else if (ml_is(Arg, MLMapT)) {
		} else if (ml_is(Arg, MLListT)) {
		} else if (ml_is(Arg, MLUUIDT)) {
			Id = Arg;
		} else {
			ML_ERROR("TypeError", "Unsupported type for enum: %s", ml_typeof(Arg)->Name);
		}
	}
	ml_enum_t *Enum;
	if (Id) {
		for (ml_object_table_t *ObjectTable = ml_context_get_static(Caller->Context, ML_OBJECTS_INDEX); ObjectTable; ObjectTable = ObjectTable->Prev) {
			Enum = (ml_enum_t *)ObjectTable->lookup(ObjectTable, ml_uuid_value(Id));
			if (Enum) {
				if (!ml_is((ml_value_t *)Enum, MLEnumT)) ML_ERROR("NameError", "Enum id must be unique");
				goto have_enum;
			}
		}
	}
	Enum = new(ml_enum_t);
have_enum:
	// TODO: Change code so that reusing an existing enum works properly
	Enum->Base.Type = (ml_type_t *)Data;
	GC_asprintf((char **)&Enum->Base.Name, "enum:%lx", (uintptr_t)Enum);
	Enum->Base.deref = ml_default_deref;
	Enum->Base.assign = ml_default_assign;
	Enum->Base.iterate = ml_iterate;
	Enum->Base.iter_value = ml_iter_value;
	Enum->Base.iter_key = ml_iter_key;
	Enum->Base.iter_next = ml_iter_next;
	Enum->Base.hash = (void *)ml_enum_value_hash;
	Enum->Base.call = ml_default_call;
	ml_type_init((ml_type_t *)Enum, MLEnumValueT, NULL);
	Enum->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	ml_enum_value_t **Slot = &Enum->Values;
	int64_t LastEnum = 0;
	int LastIndex = 0;
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I];
		if (ml_typeof(Arg) == MLNamesT) {
			ML_NAMES_CHECKX_ARG_COUNT(I);
			ML_NAMES_FOREACH(Arg, Iter) {
				ml_enum_value_t *EnumValue = Slot[0] = new(ml_enum_value_t);
				EnumValue->Type = Enum;
				EnumValue->Name = Iter->Value;
				EnumValue->Index = LastIndex++;
				EnumValue->Value = ml_integer_value(Args[++I]);
				stringmap_insert(Enum->Base.Exports, ml_string_value(Iter->Value), EnumValue);
				Slot = &EnumValue->Next;
			}
			break;
		} else if (ml_is(Arg, MLStringT)) {
			ml_enum_value_t *EnumValue = Slot[0] = new(ml_enum_value_t);
			EnumValue->Type = Enum;
			EnumValue->Name = Args[I];
			EnumValue->Index = LastIndex++;
			EnumValue->Value = ++LastEnum;
			stringmap_insert(Enum->Base.Exports, ml_string_value(Args[I]), EnumValue);
			Slot = &EnumValue->Next;
		} else if (ml_is(Arg, MLMapT)) {
			ML_MAP_FOREACH(Arg, Iter) {
				if (!ml_is(Iter->Key, MLStringT)) ML_ERROR("TypeError", "Enum names must be strings");
				ml_enum_value_t *EnumValue = Slot[0] = new(ml_enum_value_t);
				EnumValue->Type = Enum;
				EnumValue->Name = Iter->Key;
				EnumValue->Index = LastIndex++;
				EnumValue->Value = LastEnum = ml_integer_value(Iter->Value);
				stringmap_insert(Enum->Base.Exports, ml_string_value(Iter->Key), EnumValue);
				Slot = &EnumValue->Next;
			}
		} else if (ml_is(Arg, MLListT)) {
			ML_LIST_FOREACH(Arg, Iter) {
				if (!ml_is(Iter->Value, MLStringT)) ML_ERROR("TypeError", "Enum names must be strings");
				ml_enum_value_t *EnumValue = Slot[0] = new(ml_enum_value_t);
				EnumValue->Type = Enum;
				EnumValue->Name = Iter->Value;
				EnumValue->Index = LastIndex++;
				EnumValue->Value = ++LastEnum;
				stringmap_insert(Enum->Base.Exports, ml_string_value(Iter->Value), EnumValue);
				Slot = &EnumValue->Next;
			}
		} else if (ml_is(Arg, MLUUIDT)) {
			Id = Args[I];
		}
	}
	if (Enum->Base.Type == MLEnumCyclicT) Slot[0] = Enum->Values;
	if (Id) {
		memcpy(Enum->Base.Id, ml_uuid_value(Id), sizeof(uuid_t));
	} else {
		uuid_generate(Enum->Base.Id);
	}
	ml_object_table_t *ObjectTable = ml_context_get_static(Caller->Context, ML_OBJECTS_INDEX);
	if (ObjectTable) ObjectTable->insert(ObjectTable, (ml_type_t *)Enum);
	ML_RETURN(Enum);
}

ML_CFUNCTIONX(MLEnum, MLEnumT, ml_enum_constructor);

ML_TYPE(MLEnumT, (MLTypeT, MLSequenceT), "enum",
// The base type of enumeration types.
	.call = (void *)ml_enum_call,
	.Constructor = (ml_value_t *)MLEnum
);

static void ml_enum_cyclic_call(ml_state_t *Caller, ml_enum_t *Enum, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (ml_is(Arg, MLStringT)) {
		ml_value_t *Value = stringmap_search(Enum->Base.Exports, ml_string_value(Arg));
		if (!Value) ML_ERROR("EnumError", "Invalid enum name");
		ML_RETURN(Value);
	} else if (ml_is(Arg, MLIntegerT)) {
		ml_enum_value_t *Current = Enum->Values;
		int64_t Value = ml_integer_value(Arg);
		do {
			if (Current->Value == Value) ML_RETURN(Current);
			Current = Current->Next;
		} while (Current && Current != Enum->Values);
		ML_ERROR("EnumError", "Invalid enum index");
	} else {
		ML_ERROR("TypeError", "Expected <integer> or <string> not <%s>", ml_typeof(Arg)->Name);
	}
}

ML_CFUNCTIONX(MLEnumCyclic, MLEnumCyclicT, ml_enum_constructor);

ML_TYPE(MLEnumCyclicT, (MLEnumT), "enum::cyclic",
//@enum::cyclic
	.call = (void *)ml_enum_cyclic_call,
	.Constructor = (ml_value_t *)MLEnumCyclic
);

static void ML_TYPED_FN(ml_value_set_name, MLEnumT, ml_enum_t *Enum, const char *Name) {
	Enum->Base.Name = Name;
}

/*
ML_METHODV(MLEnumT, MLStringT) {
//<Names...
//>enum
// Returns a new enumeration type.
//$= let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
//$= day::Wed
//$= integer(day::Fri)
}

ML_METHODV(MLEnumT, MLNamesT) {
//<Name,Value
//>enum
// Returns a new enumeration type.
//$= let colour := enum(Red is 10, Green is 20, Blue is 30)
//$= colour::Red
//$= list(colour, integer)
}

ML_METHODV(MLEnumCyclicT, MLStringT) {
//@enum::cyclic
//<Names...
//>enum
// Returns a new enumeration type.
//$= let day := enum::cyclic("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
//$= day::Wed
//$= day::Fri + 0
}

ML_METHODV(MLEnumCyclicT, MLNamesT) {
//@enum::cyclic
//<Name,Value
//>enum
}
*/

ml_enum_t *ml_enum_alloc(ml_type_t *Type, const char *Name) {
	ml_enum_t *Enum = new(ml_enum_t);
	Enum->Base.Type = Type;
	if (Name) {
		Enum->Base.Name = Name;
	} else {
		GC_asprintf((char **)&Enum->Base.Name, "enum:%lx", (uintptr_t)Enum);
	}
	Enum->Base.deref = ml_default_deref;
	Enum->Base.assign = ml_default_assign;
	Enum->Base.iterate = ml_iterate;
	Enum->Base.iter_value = ml_iter_value;
	Enum->Base.iter_key = ml_iter_key;
	Enum->Base.iter_next = ml_iter_next;
	Enum->Base.hash = (void *)ml_enum_value_hash;
	Enum->Base.call = ml_default_call;
	ml_type_init((ml_type_t *)Enum, MLEnumValueT, NULL);
	Enum->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	return Enum;
}

void ml_enum_add_value(ml_enum_t *Enum, ml_value_t *Name, int64_t N) {
	int Index = Enum->Base.Exports->Size;
	ml_enum_value_t *Value = new(ml_enum_value_t);
	Value->Type = Enum;
	Value->Name = Name;
	Value->Index = Index;
	Value->Value = N;
	stringmap_insert(Enum->Base.Exports, ml_string_value(Name), Value);
	if (Enum->Base.Type == MLEnumCyclicT) {
		if (!Enum->Values) {
			Enum->Values = Value;
			Value->Next = Value;
		} else {
			ml_enum_value_t *Prev = Enum->Values;
			while (Prev->Next != Enum->Values) Prev = Prev->Next;
			Prev->Next = Value;
			Value->Next = Enum->Values;
		}
	} else {
		ml_enum_value_t **Slot = &Enum->Values;
		while (Slot[0]) Slot = &Slot[0]->Next;
		Slot[0] = Value;
	}
}

ml_type_t *ml_enum(const char *TypeName, ...) {
	va_list Args;
	ml_enum_t *Enum = ml_enum_alloc(MLEnumT, TypeName);
	uuid_generate(Enum->Base.Id);
	int Index = 0;
	va_start(Args, TypeName);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_enum_add_value(Enum, ml_string(String, -1), ++Index);
	}
	va_end(Args);
	uuidmap_insert(MLRootObjectTable->Types, Enum->Base.Id, Enum);
	return (ml_type_t *)Enum;
}

ml_type_t *ml_enum_cyclic(const char *TypeName, ...) {
	va_list Args;
	ml_enum_t *Enum = ml_enum_alloc(MLEnumCyclicT, TypeName);
	uuid_generate(Enum->Base.Id);
	int Index = 0;
	va_start(Args, TypeName);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_enum_add_value(Enum, ml_string(String, -1), ++Index);
	}
	va_end(Args);
	uuidmap_insert(MLRootObjectTable->Types, Enum->Base.Id, Enum);
	return (ml_type_t *)Enum;
}

ml_type_t *ml_enum2(const char *TypeName, ...) {
	va_list Args;
	ml_enum_t *Enum = ml_enum_alloc(MLEnumT, TypeName);
	uuid_generate(Enum->Base.Id);
	va_start(Args, TypeName);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_enum_add_value(Enum, ml_string(String, -1), va_arg(Args, int));
	}
	va_end(Args);
	uuidmap_insert(MLRootObjectTable->Types, Enum->Base.Id, Enum);
	return (ml_type_t *)Enum;
}

ml_type_t *ml_enum_with_id(const char *TypeName, const char *Id, ...) {
	va_list Args;
	ml_enum_t *Enum = ml_enum_alloc(MLEnumT, TypeName);
	uuid_parse(Id, Enum->Base.Id);
	int Index = 0;
	va_start(Args, Id);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_enum_add_value(Enum, ml_string(String, -1), ++Index);
	}
	va_end(Args);
	uuidmap_insert(MLRootObjectTable->Types, Enum->Base.Id, Enum);
	return (ml_type_t *)Enum;
}

ml_type_t *ml_enum_cyclic_with_id(const char *TypeName, const char *Id, ...) {
	va_list Args;
	ml_enum_t *Enum = ml_enum_alloc(MLEnumCyclicT, TypeName);
	uuid_parse(Id, Enum->Base.Id);
	int Index = 0;
	va_start(Args, Id);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_enum_add_value(Enum, ml_string(String, -1), ++Index);
	}
	va_end(Args);
	uuidmap_insert(MLRootObjectTable->Types, Enum->Base.Id, Enum);
	return (ml_type_t *)Enum;
}

ml_type_t *ml_enum2_with_id(const char *TypeName, const char *Id, ...) {
	va_list Args;
	ml_enum_t *Enum = ml_enum_alloc(MLEnumT, TypeName);
	uuid_parse(Id, Enum->Base.Id);
	va_start(Args, Id);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_enum_add_value(Enum, ml_string(String, -1), va_arg(Args, int));
	}
	va_end(Args);
	uuidmap_insert(MLRootObjectTable->Types, Enum->Base.Id, Enum);
	return (ml_type_t *)Enum;
}

ml_type_t *ml_sub_enum(const char *TypeName, ml_type_t *Parent, ...) {
	ml_enum_t *SubType = ml_enum_alloc(Parent->Type, TypeName);
	ml_type_add_parent((ml_type_t *)SubType, Parent);
	SubType->Base.Constructor = (ml_value_t *)Parent;
	//SubType->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	stringmap_t *ParentValues = ((ml_enum_t *)Parent)->Base.Exports;
	va_list Args;
	va_start(Args, Parent);
	const char *Name;
	while ((Name = va_arg(Args, const char *))) {
		ml_enum_value_t *Value = (ml_enum_value_t *)stringmap_search(ParentValues, Name);
		Value->Type = SubType;
		stringmap_insert(SubType->Base.Exports, Name, Value);
		++Value;
	}
	va_end(Args);
	return (ml_type_t *)SubType;
}

ml_value_t *ml_enum_value(ml_type_t *Type, int64_t Value) {
	const ml_enum_t *Enum = (ml_enum_t *)Type;
	ml_enum_value_t *Current = Enum->Values;
	do {
		if (Current->Value == Value) return (ml_value_t *)Current;
		Current = Current->Next;
	} while (Current && Current != Enum->Values);
	return ml_error("EnumError", "Invalid enum index");
}

ml_value_t *ml_enum_index(ml_enum_t *Enum, int Index) {
	if (Index < 0 || Index >= Enum->Base.Exports->Size) {
		if (Enum->Base.Type != MLEnumCyclicT) return MLNil;
		Index %= Enum->Base.Exports->Size;
		if (Index < 0) Index += Enum->Base.Exports->Size;
	}
	ml_enum_value_t *EnumValue = Enum->Values;
	while (--Index >= 0) EnumValue = EnumValue->Next;
	return (ml_value_t *)EnumValue;
}

ML_METHOD("[]", MLEnumT, MLIntegerT) {
	return ml_enum_index((ml_enum_t *)Args[0], ml_integer_value(Args[1]) - 1);
}

ML_METHOD("count", MLEnumT) {
//<Enum
//>integer
// Returns the size of the enumeration :mini:`Enum`.
//$= let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
//$= day:count
	ml_enum_t *Enum = (ml_enum_t *)Args[0];
	return ml_integer(Enum->Base.Exports->Size);
}

ML_METHOD("random", MLEnumT) {
//<Enum
//>enum::value
	ml_enum_t *Enum = (ml_enum_t *)Args[0];
	int Limit = Enum->Base.Exports->Size;
	return (ml_value_t *)(Enum->Values + ml_random_integer(Limit));
}

typedef struct {
	ml_type_t *Type;
	ml_enum_value_t *Current, *End;
} ml_enum_iter_t;

ML_TYPE(MLEnumIterT, (), "enum-iter");
//!internal

static void ML_TYPED_FN(ml_iterate, MLEnumT, ml_state_t *Caller, ml_enum_t *Enum) {
	int Size = Enum->Base.Exports->Size;
	if (!Size) ML_RETURN(MLNil);
	ml_enum_iter_t *Iter = new(ml_enum_iter_t);
	Iter->Type = MLEnumIterT;
	Iter->Current = Enum->Values;
	Iter->End = Enum->Values;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_next, MLEnumIterT, ml_state_t *Caller, ml_enum_iter_t *Iter) {
	Iter->Current = Iter->Current->Next;
	if (!Iter->Current) ML_RETURN(MLNil);
	if (Iter->Current == Iter->End) ML_RETURN(MLNil);
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLEnumIterT, ml_state_t *Caller, ml_enum_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Current->Index + 1));
}

static void ML_TYPED_FN(ml_iter_value, MLEnumIterT, ml_state_t *Caller, ml_enum_iter_t *Iter) {
	ML_RETURN(Iter->Current);
}

ML_TYPE(MLEnumIntervalT, (MLSequenceT), "enum-interval");
//!internal

typedef struct {
	ml_type_t *Type;
	ml_enum_value_t *Start, *End;
} ml_enum_interval_t;

ML_METHOD("..", MLEnumValueT, MLEnumValueT) {
//<Min
//<Max
//>enum::interval
// Returns a interval of enum values. :mini:`Min` and :mini:`Max` must belong to the same enumeration.
//$= let day := enum("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")
//$= day::Mon .. day::Fri
	ml_enum_value_t *Start = (ml_enum_value_t *)Args[0];
	ml_enum_value_t *End = (ml_enum_value_t *)Args[1];
	// TODO: Make sure this works for sub-enums
	if (Start->Type != End->Type) return ml_error("TypeError", "Enum types do not match");
	ml_enum_interval_t *Interval = new(ml_enum_interval_t);
#ifdef ML_GENERICS
	ml_type_t *Types[2] = {MLEnumIntervalT, (ml_type_t *)Start->Type};
	Interval->Type = ml_generic_type(2, Types);
#else
	Interval->Type = MLEnumIntervalT;
#endif
	Interval->Start = Start;
	Interval->End = End;
	return (ml_value_t *)Interval;
}

static void ML_TYPED_FN(ml_iterate, MLEnumIntervalT, ml_state_t *Caller, ml_enum_interval_t *Interval) {
	ml_enum_value_t *Start = Interval->Start;
	ml_enum_value_t *End = Interval->End;
	if (Start->Index > End->Index && Start->Type->Base.Type != MLEnumCyclicT) ML_RETURN(MLNil);
	ml_enum_iter_t *Iter = new(ml_enum_iter_t);
	Iter->Type = MLEnumIterT;
	Iter->Current = Interval->Start;
	Iter->End = Interval->End->Next;
	ML_RETURN(Iter);
}

ML_TYPE(MLEnumOpenIntervalT, (MLSequenceT), "enum-open-interval");
//!internal

ML_METHOD("up", MLEnumValueT) {
	ml_enum_value_t *Start = (ml_enum_value_t *)Args[0];
	ml_enum_interval_t *Interval = new(ml_enum_interval_t);
#ifdef ML_GENERICS
	ml_type_t *Types[2] = {MLEnumOpenIntervalT, (ml_type_t *)Start->Type};
	Interval->Type = ml_generic_type(2, Types);
#else
	Interval->Type = MLEnumOpenIntervalT;
#endif
	Interval->Start = Start;
	Interval->End = NULL;
	return (ml_value_t *)Interval;
}

static void ML_TYPED_FN(ml_iterate, MLEnumOpenIntervalT, ml_state_t *Caller, ml_enum_interval_t *Interval) {
	ml_enum_iter_t *Iter = new(ml_enum_iter_t);
	Iter->Type = MLEnumIterT;
	Iter->Current = Interval->Start;
	Iter->End = NULL;
	ML_RETURN(Iter);
}

typedef struct {
	ml_type_t *Type;
	ml_enum_t *Enum;
	ml_value_t *Cases[];
} ml_enum_switch_t;

static void ml_enum_switch(ml_state_t *Caller, ml_enum_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (!ml_is(Arg, (ml_type_t *)Switch->Enum)) {
		ML_ERROR("TypeError", "expected %s for argument 1", Switch->Enum->Base.Name);
	}
	ML_RETURN(Switch->Cases[ml_enum_value_index(Arg)]);
}

ML_TYPE(MLEnumSwitchT, (MLFunctionT), "enum-switch",
//!internal
	.call = (void *)ml_enum_switch
);

static ml_value_t *ml_enum_switch_fn(ml_enum_t *Enum, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLListT);
	int Size = Enum->Base.Exports->Size;
	ml_enum_switch_t *Switch = xnew(ml_enum_switch_t, Size, ml_value_t *);
	Switch->Type = MLEnumSwitchT;
	Switch->Enum = Enum;
	ml_value_t *Default = ml_integer(Count);
	for (int I = 0; I < Size; ++I) Switch->Cases[I] = Default;
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Case = ml_integer(I);
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, (ml_type_t *)Enum)) {
				Switch->Cases[ml_enum_value_index(Value)] = Case;
			} else if (ml_is(Value, MLSymbolT)) {
				ml_enum_value_t *EnumValue = stringmap_search(Enum->Base.Exports, ml_symbol_name(Value));
				if (!EnumValue) return ml_error("EnumError", "Invalid enum name");
				Switch->Cases[EnumValue->Index] = Case;
			} else if (ml_is(Value, MLStringT)) {
				ml_enum_value_t *EnumValue = stringmap_search(Enum->Base.Exports, ml_string_value(Value));
				if (!EnumValue) return ml_error("EnumError", "Invalid enum name");
				Switch->Cases[EnumValue->Index] = Case;
			} else if (ml_is(Value, MLEnumIntervalT)) {
				ml_enum_interval_t *Interval = (ml_enum_interval_t *)Value;
				if (Interval->Start->Type != Enum) return ml_error("ValueError", "Unsupported value in enum case");
				int MinIndex = Interval->Start->Index;
				int MaxIndex = Interval->End->Index;
				if (MaxIndex >= MinIndex) {
					for (int J = MinIndex; J <= MaxIndex; ++J) Switch->Cases[J] = Case;
				} else if (Enum->Base.Type == MLEnumCyclicT) {
					for (int J = MinIndex; J < Size; ++J) Switch->Cases[J] = Case;
					for (int J = 0; J <= MaxIndex; ++J) Switch->Cases[J] = Case;
				}
			} else if (ml_is(Value, MLSymbolIntervalT)) {
				ml_symbol_interval_t *Interval = (ml_symbol_interval_t *)Value;
				ml_enum_value_t *Min = stringmap_search(Enum->Base.Exports, Interval->First);
				ml_enum_value_t *Max = stringmap_search(Enum->Base.Exports, Interval->Last);
				if (!Min) return ml_error("EnumError", "Invalid enum name");
				if (!Max) return ml_error("EnumError", "Invalid enum name");
				int MinIndex = Min->Index;
				int MaxIndex = Max->Index;
				if (MaxIndex >= MinIndex) {
					for (int J = MinIndex; J <= MaxIndex; ++J) Switch->Cases[J] = Case;
				} else if (Enum->Base.Type == MLEnumCyclicT) {
					for (int J = MinIndex; J < Size; ++J) Switch->Cases[J] = Case;
					for (int J = 0; J <= MaxIndex; ++J) Switch->Cases[J] = Case;
				}
			} else {
				return ml_error("ValueError", "Unsupported value in enum case");
			}
		}
	}
	return (ml_value_t *)Switch;
}

ML_METHOD(MLCompilerSwitch, MLEnumT) {
//!internal
	ml_enum_t *Enum = (ml_enum_t *)Args[0];
	if (!Enum->Switch) Enum->Switch = ml_inline_function(ml_cfunction(Enum, (ml_callback_t)ml_enum_switch_fn));
	return Enum->Switch;
}

ML_METHOD("index", MLEnumValueT) {
	return ml_integer(ml_enum_value_index(Args[0]) + 1);
}

ML_METHOD("value", MLEnumValueT) {
	return ml_integer(ml_enum_value_value(Args[0]));
}

ML_METHOD(MLIntegerT, MLEnumValueT) {
	return ml_integer(ml_enum_value_value(Args[0]));
}

ML_METHOD("<>", MLEnumValueT, MLIntegerT) {
	ml_type_t *TypeA = ml_typeof(Args[0]);
	ml_type_t *TypeB = ml_typeof(Args[1]);
	if (TypeA < TypeB) return ml_integer(-1);
	if (TypeA > TypeB) return ml_integer(1);
	int64_t A = ml_enum_value_value(Args[0]);
	int64_t B = ml_integer_value(Args[1]);
	if (A < B) return ml_integer(-1);
	if (A > B) return ml_integer(1);
	return ml_integer(0);
}

ML_METHOD("<>", MLIntegerT, MLEnumValueT) {
	ml_type_t *TypeA = ml_typeof(Args[0]);
	ml_type_t *TypeB = ml_typeof(Args[1]);
	if (TypeA < TypeB) return ml_integer(-1);
	if (TypeA > TypeB) return ml_integer(1);
	int64_t A = ml_integer_value(Args[0]);
	int64_t B = ml_enum_value_value(Args[1]);
	if (A < B) return ml_integer(-1);
	if (A > B) return ml_integer(1);
	return ml_integer(0);
}

ML_METHOD("+", MLEnumValueT, MLIntegerT) {
	return ml_enum_index((ml_enum_t *)Args[0]->Type, ml_enum_value_index(Args[0]) + ml_integer_value(Args[1]));
}

ML_METHOD("+", MLIntegerT, MLEnumValueT) {
	return ml_enum_index((ml_enum_t *)Args[1]->Type, ml_enum_value_index(Args[1]) + ml_integer_value(Args[0]));
}

ML_METHOD("-", MLEnumValueT, MLIntegerT) {
	return ml_enum_index((ml_enum_t *)Args[0]->Type, ml_enum_value_index(Args[0]) - ml_integer_value(Args[1]));
}

ML_METHOD("next", MLEnumValueT) {
	return ml_enum_index((ml_enum_t *)Args[0]->Type, ml_enum_value_index(Args[0]) + 1);
}

ML_METHOD("prev", MLEnumValueT) {
	return ml_enum_index((ml_enum_t *)Args[0]->Type, ml_enum_value_index(Args[0]) - 1);
}

//!flags

static void ml_flags_call(ml_state_t *Caller, ml_flags_t *Flags, int Count, ml_value_t **Args) {
	ml_flags_value_t *Value = new(ml_flags_value_t);
	Value->Type = Flags;
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = ml_deref(Args[I]);
		if (ml_is(Arg, MLStringT)) {
			ml_value_t *Flag = stringmap_search(Flags->Base.Exports, ml_string_value(Arg));
			if (!Flag) ML_ERROR("FlagError", "Invalid flag name");
			Value->Value |= ml_flags_value_value(Flag);
		} else if (ml_is(Arg, MLIntegerT)) {
			uint64_t Flag = ml_integer_value(Arg);
			Value->Value |= Flag;
			for (ml_flags_value_t *FlagValue = Flags->Values; FlagValue; FlagValue = FlagValue->Next) {
				Flag &= ~FlagValue->Value;
				if (!Flag) break;
			}
			if (Flag) ML_ERROR("FlagError", "Invalid flags value");
		} else {
			ML_ERROR("TypeError", "Expected <integer> or <string> not <%s>", ml_typeof(Arg)->Name);
		}
	}
	ML_RETURN(Value);
}

ML_FUNCTIONX(MLFlags) {
	ml_value_t *Id = NULL;
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I];
		if (ml_typeof(Arg) == MLNamesT) {
			ML_NAMES_CHECKX_ARG_COUNT(I);
			break;
		} else if (ml_is(Arg, MLStringT)) {
		} else if (ml_is(Arg, MLMapT)) {
		} else if (ml_is(Arg, MLListT)) {
		} else if (ml_is(Arg, MLUUIDT)) {
			Id = Arg;
		} else {
			ML_ERROR("TypeError", "Unsupported type for flags: %s", ml_typeof(Arg)->Name);
		}
	}
	ml_flags_t *Flags;
	if (Id) {
		for (ml_object_table_t *ObjectTable = ml_context_get_static(Caller->Context, ML_OBJECTS_INDEX); ObjectTable; ObjectTable = ObjectTable->Prev) {
			Flags = (ml_flags_t *)ObjectTable->lookup(ObjectTable, ml_uuid_value(Id));
			if (Flags) {
				if (!ml_is((ml_value_t *)Flags, MLFlagsT)) ML_ERROR("NameError", "Flags id must be unique");
				goto have_flags;
			}
		}
	}
	Flags = new(ml_flags_t);
have_flags:
	// TODO: Change code so that reusing an existing flags works properly
	Flags->Base.Type = (ml_type_t *)MLFlagsT;
	GC_asprintf((char **)&Flags->Base.Name, "flags:%lx", (uintptr_t)Flags);
	Flags->Base.deref = ml_default_deref;
	Flags->Base.assign = ml_default_assign;
	Flags->Base.iterate = ml_iterate;
	Flags->Base.iter_value = ml_iter_value;
	Flags->Base.iter_key = ml_iter_key;
	Flags->Base.iter_next = ml_iter_next;
	Flags->Base.hash = (void *)ml_enum_value_hash;
	Flags->Base.call = ml_default_call;
	ml_type_init((ml_type_t *)Flags, MLFlagsValueT, NULL);
	Flags->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	ml_flags_value_t **Slot = &Flags->Values;
	uint64_t LastFlag = 1;
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I];
		if (ml_typeof(Arg) == MLNamesT) {
			ML_NAMES_CHECKX_ARG_COUNT(I);
			ML_NAMES_FOREACH(Arg, Iter) {
				ml_flags_value_t *FlagsValue = Slot[0] = new(ml_flags_value_t);
				FlagsValue->Type = Flags;
				FlagsValue->Name = Iter->Value;
				FlagsValue->Value = ml_integer_value(Args[++I]);
				stringmap_insert(Flags->Base.Exports, ml_string_value(Iter->Value), FlagsValue);
				Slot = &FlagsValue->Next;
			}
			break;
		} else if (ml_is(Arg, MLStringT)) {
			ml_flags_value_t *FlagsValue = Slot[0] = new(ml_flags_value_t);
			FlagsValue->Type = Flags;
			FlagsValue->Name = Args[I];
			FlagsValue->Value = LastFlag;
			LastFlag <<= 1;
			stringmap_insert(Flags->Base.Exports, ml_string_value(Args[I]), FlagsValue);
			Slot = &FlagsValue->Next;
		} else if (ml_is(Arg, MLMapT)) {
			ML_MAP_FOREACH(Arg, Iter) {
				if (!ml_is(Iter->Key, MLStringT)) ML_ERROR("TypeError", "Flags names must be strings");
				ml_flags_value_t *FlagsValue = Slot[0] = new(ml_flags_value_t);
				FlagsValue->Type = Flags;
				FlagsValue->Name = Iter->Key;
				FlagsValue->Value = LastFlag = ml_integer_value(Iter->Value);
				LastFlag <<= 1;
				stringmap_insert(Flags->Base.Exports, ml_string_value(Iter->Key), FlagsValue);
				Slot = &FlagsValue->Next;
			}
		} else if (ml_is(Arg, MLListT)) {
			ML_LIST_FOREACH(Arg, Iter) {
				if (!ml_is(Iter->Value, MLStringT)) ML_ERROR("TypeError", "Flags names must be strings");
				ml_flags_value_t *FlagsValue = Slot[0] = new(ml_flags_value_t);
				FlagsValue->Type = Flags;
				FlagsValue->Name = Iter->Value;
				FlagsValue->Value = LastFlag;
				LastFlag <<= 1;
				stringmap_insert(Flags->Base.Exports, ml_string_value(Iter->Value), FlagsValue);
				Slot = &FlagsValue->Next;
			}
		} else if (ml_is(Arg, MLUUIDT)) {
			Id = Args[I];
		}
	}
	if (Id) {
		memcpy(Flags->Base.Id, ml_uuid_value(Id), sizeof(uuid_t));
	} else {
		uuid_generate(Flags->Base.Id);
	}
	ml_object_table_t *ObjectTable = ml_context_get_static(Caller->Context, ML_OBJECTS_INDEX);
	if (ObjectTable) ObjectTable->insert(ObjectTable, (ml_type_t *)Flags);
	ML_RETURN(Flags);
}

ML_TYPE(MLFlagsT, (MLTypeT), "flags",
// The base type of flag types.
	.call = (void *)ml_flags_call,
	.Constructor = (ml_value_t *)MLFlags
);

static long ml_flag_value_hash(ml_flags_value_t *Value, ml_hash_chain_t *Chain) {
	return (long)Value->Type + Value->Value;
}

ML_TYPE(MLFlagsValueT, (), "flag-value");
//@flags::value
// An instance of a flags type.

typedef struct {
	ml_stringbuffer_t *Buffer;
	uint64_t Value;
	int Length;
} ml_flags_value_append_t;

static int ml_flags_value_append(const char *Name, ml_flags_value_t *Flags, ml_flags_value_append_t *Append) {
	if ((Append->Value & Flags->Value) == Flags->Value) {
		ml_stringbuffer_t *Buffer = Append->Buffer;
		if (ml_stringbuffer_length(Buffer) > Append->Length) ml_stringbuffer_put(Buffer, ',');
		ml_stringbuffer_write(Buffer, Name, strlen(Name));
	}
	return 0;
}

ML_METHOD("append", MLStringBufferT, MLFlagsValueT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	uint64_t Value = ml_flags_value_value(Args[1]);
	size_t OldLength = Buffer->Length;
	for (ml_flags_value_t *Flag = ((ml_flags_value_t *)Args[1])->Type->Values; Flag; Flag = Flag->Next) {
		if (Flag->Value & Value) {
			if (Buffer->Length > OldLength) ml_stringbuffer_put(Buffer, ',');
			ml_stringbuffer_write(Buffer, ml_string_value(Flag->Name), ml_string_length(Flag->Name));
		}
	}
	return Buffer->Length > OldLength ? MLSome : MLNil;
}

typedef struct {
	ml_type_t *Type;
	ml_flags_t *Flags;
	uint64_t Include;
	uint64_t Exclude;
} ml_flags_spec_t;

ML_TYPE(MLFlagsSpecT, (), "flag-spec");
//@flags::spec
// A pair of flag sets for including and excluding flags.

ML_METHOD("append", MLStringBufferT, MLFlagsSpecT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_flags_spec_t *Value = (ml_flags_spec_t *)Args[1];
	ml_stringbuffer_put(Buffer, '(');
	ml_flags_value_append_t Append[1] = {{Buffer, Value->Include, ml_stringbuffer_length(Buffer)}};
	stringmap_foreach(Value->Flags->Base.Exports, Append, (void *)ml_flags_value_append);
	if (Value->Exclude == ~Value->Include) {
		ml_stringbuffer_write(Buffer, "/*", 2);
	} else if (Value->Exclude) {
		ml_stringbuffer_put(Buffer, '/');
		Append->Value = Value->Exclude;
		Append->Length = ml_stringbuffer_length(Buffer);
		stringmap_foreach(Value->Flags->Base.Exports, Append, (void *)ml_flags_value_append);
	}
	ml_stringbuffer_put(Buffer, ')');
	return ml_stringbuffer_length(Buffer) > Append->Length ? MLSome : MLNil;
}

ml_flags_t *ml_flags_alloc(const char *Name) {
	ml_flags_t *Flags = new(ml_flags_t);
	Flags->Base.Type = MLFlagsT;
	Flags->Base.Name = Name;
	Flags->Base.deref = ml_default_deref;
	Flags->Base.assign = ml_default_assign;
	Flags->Base.iterate = ml_iterate;
	Flags->Base.iter_value = ml_iter_value;
	Flags->Base.iter_key = ml_iter_key;
	Flags->Base.iter_next = ml_iter_next;
	Flags->Base.hash = (void *)ml_flag_value_hash;
	Flags->Base.call = ml_default_call;
	ml_type_init((ml_type_t *)Flags, MLFlagsValueT, NULL);
	Flags->Base.Exports[0] = (stringmap_t)STRINGMAP_INIT;
	return Flags;
}

void ml_flags_add_value(ml_flags_t *Flags, ml_value_t *Name, uint64_t N) {
	ml_flags_value_t *Value = new(ml_flags_value_t);
	Value->Type = Flags;
	Value->Name = Name;
	Value->Value = N;
	stringmap_insert(Flags->Base.Exports, ml_string_value(Name), Value);
	ml_flags_value_t **Slot = &Flags->Values;
	while (Slot[0] != Flags->Values) Slot = &Slot[0]->Next;
	Slot[0] = Value;
}

ml_type_t *ml_flags(const char *TypeName, ...) {
	va_list Args;
	ml_flags_t *Flags = ml_flags_alloc(TypeName);
	uint64_t Flag = 1;
	va_start(Args, TypeName);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_value_t *Name = ml_string(String, -1);
		ml_flags_add_value(Flags, Name, Flag);
		Flag <<= 1;
	}
	va_end(Args);
	return (ml_type_t *)Flags;
}

ml_type_t *ml_flags2(const char *TypeName, ...) {
	va_list Args;
	ml_flags_t *Flags = ml_flags_alloc(TypeName);
	va_start(Args, TypeName);
	const char *String;
	while ((String = va_arg(Args, const char *))) {
		ml_value_t *Name = ml_string(String, -1);
		ml_flags_add_value(Flags, Name, va_arg(Args, int));
	}
	va_end(Args);
	return (ml_type_t *)Flags;
}

static void ML_TYPED_FN(ml_value_set_name, MLFlagsT, ml_flags_t *Flags, const char *Name) {
	Flags->Base.Name = Name;
}

ml_value_t *ml_flags_value(ml_type_t *Type, uint64_t Flags) {
	ml_flags_value_t *Value = new(ml_flags_value_t);
	Value->Type = (ml_flags_t *)Type;
	Value->Value = Flags;
	return (ml_value_t *)Value;
}

const char *ml_flags_value_name(ml_value_t *Value) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_flags_value_append_t Append[1] = {{Buffer, ml_flags_value_value(Value), ml_stringbuffer_length(Buffer)}};
	stringmap_foreach(Value->Type->Exports, Append, (void *)ml_flags_value_append);
	return ml_stringbuffer_get_string(Buffer);
}

typedef struct {
	ml_value_t *Index;
	uint64_t Value;
} ml_flags_case_t;

typedef struct {
	ml_type_t *Type;
	ml_flags_t *Flags;
	ml_flags_case_t Cases[];
} ml_flags_switch_t;

static void ml_flags_switch(ml_state_t *Caller, ml_flags_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (!ml_is(Arg, (ml_type_t *)Switch->Flags)) {
		ML_ERROR("TypeError", "expected %s for argument 1", Switch->Flags->Base.Name);
	}
	uint64_t Value = ml_enum_value_value(Arg);
	for (ml_flags_case_t *Case = Switch->Cases;; ++Case) {
		if ((Case->Value & Value) == Case->Value) ML_RETURN(Case->Index);
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLFlagsSwitchT, (MLFunctionT), "flags-switch",
//!internal
	.call = (void *)ml_flags_switch
);

static ml_value_t *ml_flags_switch_fn(ml_flags_t *Flags, int Count, ml_value_t **Args) {
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_flags_switch_t *Switch = xnew(ml_flags_switch_t, Total, ml_flags_case_t);
	Switch->Type = MLFlagsSwitchT;
	Switch->Flags = Flags;
	ml_flags_case_t *Case = Switch->Cases;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, (ml_type_t *)Flags)) {
				Case->Value = ml_flags_value_value(Value);
			} else if (ml_is(Value, MLStringT)) {
				ml_value_t *FlagsValue = stringmap_search(Flags->Base.Exports, ml_string_value(Value));
				if (!FlagsValue) return ml_error("FlagsError", "Invalid flags name");
				Case->Value = ml_flags_value_value(FlagsValue);
			} else if (ml_is(Value, MLTupleT)) {
				ml_tuple_t *Tuple = (ml_tuple_t *)Value;
				for (int J = 0; J < Tuple->Size; ++J) {
					ml_value_t *Value = Tuple->Values[J];
					if (!ml_is(Value, MLStringT)) return ml_error("ValueError", "Unsupported value in flags case");
					ml_value_t *FlagsValue = stringmap_search(Flags->Base.Exports, ml_string_value(Tuple->Values[J]));
					if (!FlagsValue) return ml_error("FlagsError", "Invalid flags name");
					Case->Value |= ml_flags_value_value(FlagsValue);
				}
			} else if (Value == MLNil) {
				Case->Value = 0;
			} else {
				return ml_error("ValueError", "Unsupported value in flags case");
			}
			Case->Index = ml_integer(I);
			++Case;
		}
	}
	Case->Value = 0;
	Case->Index = ml_integer(Count);
	return (ml_value_t *)Switch;
}

ML_METHOD(MLCompilerSwitch, MLFlagsT) {
//!internal
	return ml_inline_call_macro(ml_cfunction(Args[0], (ml_callback_t)ml_flags_switch_fn));
}

ML_METHOD("+", MLFlagsValueT, MLFlagsValueT) {
//<Flags/1
//<Flags/2
//>flags::value
// Returns the union of :mini:`Flags/1` and :mini:`Flags/2`. :mini:`Flags/1` and :mini:`Flags/2` must have the same flags type.
//$= let mode := flags("Read", "Write", "Execute")
//$= mode::Read + mode::Write
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, ((ml_type_t *)A->Type));
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	ml_flags_value_t *C = new(ml_flags_value_t);
	C->Type = A->Type;
	C->Value = A->Value | B->Value;
	return (ml_value_t *)C;
}

ML_METHOD("-", MLFlagsValueT, MLFlagsValueT) {
//<Flags/1
//<Flags/2
//>flags::value
// Returns the difference of :mini:`Flags/1` and :mini:`Flags/2`. :mini:`Flags/1` and :mini:`Flags/2` must have the same flags type.
//$= let mode := flags("Read", "Write", "Execute")
//$= mode("Read", "Write") - mode::Write
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, ((ml_type_t *)A->Type));
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	ml_flags_value_t *C = new(ml_flags_value_t);
	C->Type = A->Type;
	C->Value = A->Value & ~B->Value;
	return (ml_value_t *)C;
}

ML_METHOD("<", MLFlagsValueT, MLFlagsValueT) {
//<Flags/1
//<Flags/2
//>flags::value
// Returns the :mini:`Flags/2` if it contains all of :mini:`Flags/1`. :mini:`Flags/1` and :mini:`Flags/2` must have the same flags type.
//$= let mode := flags("Read", "Write", "Execute")
//$= mode("Read", "Write") < mode("Read", "Write", "Execute")
//$= mode("Read", "Write", "Execute") < mode("Read", "Write")
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, ((ml_type_t *)A->Type));
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	if ((A->Value & B->Value) == A->Value) {
		return Args[1];
	} else {
		return MLNil;
	}
}

ML_METHOD("<=", MLFlagsValueT, MLFlagsValueT) {
//<Flags/1
//<Flags/2
//>flags::value
// Returns the :mini:`Flags/2` if it contains all of :mini:`Flags/1`. :mini:`Flags/1` and :mini:`Flags/2` must have the same flags type.
//$= let mode := flags("Read", "Write", "Execute")
//$= mode("Read", "Write") <= mode("Read", "Write", "Execute")
//$= mode("Read", "Write", "Execute") <= mode("Read", "Write")
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, ((ml_type_t *)A->Type));
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	if ((A->Value & B->Value) == A->Value) {
		return Args[1];
	} else {
		return MLNil;
	}
}

ML_METHOD(">", MLFlagsValueT, MLFlagsValueT) {
//<Flags/1
//<Flags/2
//>flags::value
// Returns the :mini:`Flags/2` if it is contained in :mini:`Flags/1`. :mini:`Flags/1` and :mini:`Flags/2` must have the same flags type.
//$= let mode := flags("Read", "Write", "Execute")
//$= mode("Read", "Write") > mode("Read", "Write", "Execute")
//$= mode("Read", "Write", "Execute") > mode("Read", "Write")
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, ((ml_type_t *)A->Type));
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	if ((A->Value & B->Value) == B->Value) {
		return Args[1];
	} else {
		return MLNil;
	}
}

ML_METHOD(">=", MLFlagsValueT, MLFlagsValueT) {
//<Flags/1
//<Flags/2
//>flags::value
// Returns the :mini:`Flags/2` if it is contained in :mini:`Flags/1`. :mini:`Flags/1` and :mini:`Flags/2` must have the same flags type.
//$= let mode := flags("Read", "Write", "Execute")
//$= mode("Read", "Write") >= mode("Read", "Write", "Execute")
//$= mode("Read", "Write", "Execute") >= mode("Read", "Write")
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, ((ml_type_t *)A->Type));
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	if ((A->Value & B->Value) == B->Value) {
		return Args[1];
	} else {
		return MLNil;
	}
}

ML_METHOD("/", MLFlagsValueT, MLFlagsValueT) {
//<Flags/1
//<Flags/2
//>flags::spec
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ML_CHECK_ARG_TYPE(1, ((ml_type_t *)A->Type));
	ml_flags_value_t *B = (ml_flags_value_t *)Args[1];
	ml_flags_spec_t *Spec = new(ml_flags_spec_t);
	Spec->Type = MLFlagsSpecT;
	Spec->Flags = (ml_flags_t *)A->Type;
	Spec->Include = A->Value;
	Spec->Exclude = B->Value;
	return (ml_value_t *)Spec;
}

ML_METHOD("/", MLFlagsValueT) {
//<Flags
//>flags::spec
	ml_flags_value_t *A = (ml_flags_value_t *)Args[0];
	ml_flags_spec_t *Spec = new(ml_flags_spec_t);
	Spec->Type = MLFlagsSpecT;
	Spec->Flags = (ml_flags_t *)A->Type;
	Spec->Include = A->Value;
	Spec->Exclude = ~A->Value;
	return (ml_value_t *)Spec;
}

ML_METHOD("in", MLFlagsValueT, MLFlagsSpecT) {
//<Flags
//<Spec
	ml_flags_spec_t *Spec = (ml_flags_spec_t *)Args[1];
	ML_CHECK_ARG_TYPE(0, ((ml_type_t *)Spec->Flags));
	ml_flags_value_t *Flags = (ml_flags_value_t *)Args[0];
	if ((Flags->Value & Spec->Include) != Spec->Include) {
		return MLNil;
	} else if (Flags->Value & Spec->Exclude) {
		return MLNil;
	} else {
		return (ml_value_t *)Flags;
	}
}

ML_METHOD(MLIntegerT, MLFlagsValueT) {
	return ml_integer(ml_flags_value_value(Args[0]));
}

typedef struct {
	ml_value_t *Values;
	uint64_t Value;
} ml_flags_value_list_t;

static int ml_flags_value_list(const char *Name, ml_flags_value_t *Flags, ml_flags_value_list_t *List) {
	if ((List->Value & Flags->Value) == Flags->Value) ml_list_put(List->Values, (ml_value_t *)Flags);
	return 0;
}

ML_METHOD(MLListT, MLFlagsValueT) {
	ml_flags_value_list_t List[1] = {{ml_list(), ml_flags_value_value(Args[0])}};
	stringmap_foreach(Args[0]->Type->Exports, &List, (void *)ml_flags_value_list);
	return List->Values;
}

void ml_object_init(stringmap_t *Globals) {
#include "ml_object_init.c"
	ml_context_set_static(MLRootContext, ML_OBJECTS_INDEX, MLRootObjectTable);
	stringmap_insert(MLEnumT->Exports, "cyclic", MLEnumCyclicT);
	stringmap_insert(MLObjectT->Exports, "pseudo", MLPseudoObjectT);
	stringmap_insert(MLClassT->Exports, "pseudo", MLPseudoClassT);
	ml_externals_default_add("property", MLPropertyT);
	ml_externals_default_add("object", MLObjectT);
	ml_externals_default_add("class", MLClassT);
	ml_externals_default_add("enum", MLEnumT);
	ml_externals_default_add("flags", MLFlagsT);
#ifdef ML_GENERICS
	ml_type_add_rule(MLEnumIntervalT, MLSequenceT, MLIntegerT, ML_TYPE_ARG(1), NULL);
#endif
	if (Globals) {
		stringmap_insert(Globals, "property", MLPropertyT);
		stringmap_insert(Globals, "object", MLObjectT);
		stringmap_insert(Globals, "class", MLClassT);
		stringmap_insert(Globals, "enum", MLEnumT);
		stringmap_insert(Globals, "flags", MLFlagsT);
		stringmap_insert(Globals, "const", ml_field_modifier(MLFieldT));
		stringmap_insert(Globals, "watched", MLWatched);
	}
}

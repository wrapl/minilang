#include "ml_gir.h"
#include "ml_macros.h"
#include <gc/gc.h>
#include <girffi.h>
#include <gtk/gtk.h>
#include <stdio.h>

//!gobject

typedef struct typelib_t {
	ml_type_t *Type;
	GITypelib *Handle;
	const char *Namespace;
} typelib_t;

ML_TYPE(TypelibT, (MLIteratableT), "gir-typelib");
// A gobject-introspection typelib.

typedef struct typelib_iter_t {
	ml_type_t *Type;
	GITypelib *Handle;
	const char *Namespace;
	GIBaseInfo *Current;
	int Index, Total;
} typelib_iter_t;

typedef struct {
	ml_type_t *Type;
	GIBaseInfo *Info;
} baseinfo_t;

ML_TYPE(BaseInfoT, (MLTypeT), "gir-base-info");

static ml_value_t *baseinfo_to_value(GIBaseInfo *Info);
static void _ml_to_value(ml_value_t *Source, GValue *Dest);
static ml_value_t *_value_to_ml(const GValue *Value, GIBaseInfo *Info);

static void typelib_iter_value(ml_state_t *Caller, typelib_iter_t *Iter) {
	const char *Type = g_info_type_to_string(g_base_info_get_type(Iter->Current));
	ML_CONTINUE(Caller, ml_string(Type, -1));
}

static void typelib_iter_next(ml_state_t *Caller, typelib_iter_t *Iter) {
	if (++Iter->Index >= Iter->Total) ML_CONTINUE(Caller, MLNil);
	Iter->Current = g_irepository_get_info(NULL, Iter->Namespace, Iter->Index);
	ML_CONTINUE(Caller, Iter);
}

static void typelib_iter_key(ml_state_t *Caller, typelib_iter_t *Iter) {
	ML_CONTINUE(Caller, ml_string(g_base_info_get_name(Iter->Current), -1));
}

ML_TYPE(TypelibIterT, (), "typelib-iter");
//!internal

static ml_value_t *ml_gir_require(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	gtk_init(0, 0);
	typelib_t *Typelib = new(typelib_t);
	Typelib->Type = TypelibT;
	GError *Error = 0;
	Typelib->Namespace = ml_string_value(Args[0]);
	const char *Version = NULL;
	if (Count > 1) {
		ML_CHECK_ARG_TYPE(1, MLStringT);
		Version = ml_string_value(Args[1]);
	}
	Typelib->Handle = g_irepository_require(NULL, Typelib->Namespace, Version, 0, &Error);
	if (!Typelib->Handle) return ml_error("GirError", Error->message);
	return (ml_value_t *)Typelib;
}

typedef struct object_t {
	ml_type_t Base;
	GIObjectInfo *Info;
} object_t;

typedef struct object_instance_t {
	const object_t *Type;
	void *Handle;
} object_instance_t;

ML_TYPE(ObjectT, (BaseInfoT), "gir-object-type");
// A gobject-introspection object type.

ML_TYPE(ObjectInstanceT, (), "gir-object");
// A gobject-introspection object instance.

static object_instance_t *ObjectInstanceNil;

static ml_type_t *object_info_lookup(GIObjectInfo *Info);
static ml_type_t *interface_info_lookup(GIInterfaceInfo *Info);
static ml_type_t *struct_info_lookup(GIStructInfo *Info);
static ml_type_t *enum_info_lookup(GIEnumInfo *Info);

static void instance_finalize(object_instance_t *Instance, void *Data) {
	g_object_unref(Instance->Handle);
}

static GQuark MLQuark;

ml_value_t *ml_gir_instance_get(void *Handle, GIBaseInfo *Fallback) {
	if (Handle == 0) return (ml_value_t *)ObjectInstanceNil;
	object_instance_t *Instance = (object_instance_t *)g_object_get_qdata(Handle, MLQuark);
	if (Instance) return (ml_value_t *)Instance;
	Instance = new(object_instance_t);
	Instance->Handle = Handle;
	g_object_ref_sink(Handle);
	GC_register_finalizer(Instance, (GC_finalization_proc)instance_finalize, 0, 0, 0);
	GType Type = G_OBJECT_TYPE(Handle);
	GIBaseInfo *Info = g_irepository_find_by_gtype(NULL, Type);
	if (Info) {
		switch (g_base_info_get_type(Info)) {
		case GI_INFO_TYPE_OBJECT: {
			Instance->Type = (object_t *)object_info_lookup((GIObjectInfo *)Info);
			break;
		}
		case GI_INFO_TYPE_INTERFACE: {
			Instance->Type = (object_t *)interface_info_lookup((GIInterfaceInfo *)Info);
			break;
		}
		default: {
			return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(Info), __LINE__);
		}
		}
		g_object_set_qdata(Handle, MLQuark, Instance);
	} else if (Fallback) {
		switch (g_base_info_get_type(Fallback)) {
		case GI_INFO_TYPE_OBJECT: {
			Instance->Type = (object_t *)object_info_lookup((GIObjectInfo *)Fallback);
			break;
		}
		case GI_INFO_TYPE_INTERFACE: {
			Instance->Type = (object_t *)interface_info_lookup((GIInterfaceInfo *)Fallback);
			break;
		}
		default: {
			return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(Fallback), __LINE__);
		}
		}
	} else {
		return ml_error("UnknownType", "Type %s not found", g_type_name(Type));
	}
	return (ml_value_t *)Instance;
}

ML_METHOD(MLStringT, ObjectInstanceT) {
//<Object
//>string
	object_instance_t *Instance = (object_instance_t *)Args[0];
	return ml_string_format("<%s>", g_base_info_get_name((GIBaseInfo *)Instance->Type->Info));
}

typedef struct struct_t {
	ml_type_t Base;
	GIStructInfo *Info;
} struct_t;

typedef struct struct_instance_t {
	const struct_t *Type;
	void *Value;
} struct_instance_t;

ML_TYPE(StructT, (BaseInfoT), "gir-struct-type");
// A gobject-introspection struct type.

ML_TYPE(StructInstanceT, (), "gir-struct");
// A gobject-introspection struct instance.

static ml_value_t *struct_instance_new(struct_t *Struct, int Count, ml_value_t **Args) {
	struct_instance_t *Instance = new(struct_instance_t);
	Instance->Type = Struct;
	Instance->Value = GC_MALLOC(g_struct_info_get_size(Struct->Info));
	return (ml_value_t *)Instance;
}

ML_METHOD(MLStringT, StructInstanceT) {
//<Struct
//>string
	struct_instance_t *Instance = (struct_instance_t *)Args[0];
	return ml_string_format("<%s>", g_base_info_get_name((GIBaseInfo *)Instance->Type->Info));
}

typedef struct field_ref_t {
	ml_type_t *Type;
	void *Address;
} field_ref_t;

#define FIELD_REF(UNAME, LNAME, GTYPE, GETTER, SETTER) \
static ml_value_t *field_ref_ ## LNAME ## _deref(field_ref_t *Ref) { \
	GTYPE Value = *(GTYPE *)Ref->Address; \
	return GETTER; \
} \
\
static ml_value_t *field_ref_ ## LNAME ## _assign(field_ref_t *Ref, ml_value_t *Value) { \
	GTYPE *Address = (GTYPE *)Ref->Address; \
	*Address = SETTER; \
	return Value; \
} \
\
ML_TYPE(FieldRef ## UNAME ## T, (), "field-ref-" #LNAME, \
	.deref = (void *)field_ref_ ## LNAME ## _deref, \
	.assign = (void *)field_ref_ ## LNAME ## _assign \
);

FIELD_REF(Boolean, boolean, gboolean, ml_boolean(Value), ml_boolean_value(Value));
FIELD_REF(Int8, int8, gint8, ml_integer(Value), ml_integer_value(Value));
FIELD_REF(UInt8, uint8, guint8, ml_integer(Value), ml_integer_value(Value));
FIELD_REF(Int16, int16, gint16, ml_integer(Value), ml_integer_value(Value));
FIELD_REF(UInt16, uint16, guint16, ml_integer(Value), ml_integer_value(Value));
FIELD_REF(Int32, int32, gint32, ml_integer(Value), ml_integer_value(Value));
FIELD_REF(UInt32, uint32, guint32, ml_integer(Value), ml_integer_value(Value));
FIELD_REF(Int64, int64, gint64, ml_integer(Value), ml_integer_value(Value));
FIELD_REF(UInt64, uint64, guint64, ml_integer(Value), ml_integer_value(Value));
FIELD_REF(Float, float, gfloat, ml_real(Value), ml_real_value(Value));
FIELD_REF(Double, double, gdouble, ml_real(Value), ml_real_value(Value));
FIELD_REF(Utf8, utf8, gchar *, ml_string(Value, -1), (char *)ml_string_value(Value));

static ml_value_t *struct_field_ref(GIFieldInfo *Info, int Count, ml_value_t **Args) {
	struct_instance_t *Instance = (struct_instance_t *)Args[0];
	field_ref_t *Ref = new(field_ref_t);
	Ref->Address = (char *)Instance->Value + g_field_info_get_offset(Info);
	GITypeInfo *TypeInfo = g_field_info_get_type(Info);
	switch (g_type_info_get_tag(TypeInfo)) {
	case GI_TYPE_TAG_VOID: return ml_error("TodoError", "Field ref not implemented yet");
	case GI_TYPE_TAG_BOOLEAN: Ref->Type = FieldRefBooleanT; break;
	case GI_TYPE_TAG_INT8: Ref->Type = FieldRefInt8T; break;
	case GI_TYPE_TAG_UINT8: Ref->Type = FieldRefUInt8T; break;
	case GI_TYPE_TAG_INT16: Ref->Type = FieldRefInt16T; break;
	case GI_TYPE_TAG_UINT16: Ref->Type = FieldRefUInt16T; break;
	case GI_TYPE_TAG_INT32: Ref->Type = FieldRefInt32T; break;
	case GI_TYPE_TAG_UINT32: Ref->Type = FieldRefUInt32T; break;
	case GI_TYPE_TAG_INT64: Ref->Type = FieldRefInt64T; break;
	case GI_TYPE_TAG_UINT64: Ref->Type = FieldRefUInt64T; break;
	case GI_TYPE_TAG_FLOAT: Ref->Type = FieldRefFloatT; break;
	case GI_TYPE_TAG_DOUBLE: Ref->Type = FieldRefDoubleT; break;
	case GI_TYPE_TAG_GTYPE: return ml_error("TodoError", "Field ref not implemented yet");
	case GI_TYPE_TAG_UTF8: Ref->Type = FieldRefUtf8T; break;
	case GI_TYPE_TAG_FILENAME: Ref->Type = FieldRefUtf8T; break;
	case GI_TYPE_TAG_ARRAY: return ml_error("TodoError", "Field ref not implemented yet");
	case GI_TYPE_TAG_INTERFACE: {
		GIBaseInfo *InterfaceInfo = g_type_info_get_interface(TypeInfo);
		switch (g_base_info_get_type(InterfaceInfo)) {
		case GI_INFO_TYPE_INVALID:
		case GI_INFO_TYPE_INVALID_0: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_FUNCTION: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_CALLBACK: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_STRUCT: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_BOXED: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_ENUM: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_FLAGS: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_OBJECT: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_INTERFACE: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_CONSTANT: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_UNION: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_VALUE: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_SIGNAL: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_VFUNC: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_PROPERTY: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_FIELD: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_ARG: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_TYPE: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_UNRESOLVED: return ml_error("TodoError", "Field ref not implemented yet");
		}
		break;
	}
	case GI_TYPE_TAG_GLIST: return ml_error("TodoError", "Field ref not implemented yet");
	case GI_TYPE_TAG_GSLIST: return ml_error("TodoError", "Field ref not implemented yet");
	case GI_TYPE_TAG_GHASH: return ml_error("TodoError", "Field ref not implemented yet");
	case GI_TYPE_TAG_ERROR: return ml_error("TodoError", "Field ref not implemented yet");
	case GI_TYPE_TAG_UNICHAR: return ml_error("TodoError", "Field ref not implemented yet");
	}
	return (ml_value_t *)Ref;
}

typedef struct enum_t {
	ml_type_t Base;
	GIEnumInfo *Info;
	ml_value_t *ByIndex[];
} enum_t;

typedef struct enum_value_t {
	const enum_t *Type;
	ml_value_t *Name;
	gint64 Value;
} enum_value_t;

ML_TYPE(EnumT, (BaseInfoT), "gir-enum-type");
// A gobject-instrospection enum type.

ML_TYPE(EnumValueT, (), "gir-enum");
// A gobject-instrospection enum value.

ML_METHOD(MLStringT, EnumValueT) {
//<Value
//>string
	enum_value_t *Value = (enum_value_t *)Args[0];
	return Value->Name;
}

ML_METHOD(MLIntegerT, EnumValueT) {
//<Value
//>integer
	enum_value_t *Value = (enum_value_t *)Args[0];
	return ml_integer(Value->Value);
}

ML_METHOD("|", EnumValueT, MLNilT) {
//<Value/1
//<Value/2
//>EnumValueT
	return Args[0];
}

ML_METHOD("|", MLNilT, EnumValueT) {
//<Value/1
//<Value/2
//>EnumValueT
	return Args[1];
}

ML_METHOD("|", EnumValueT, EnumValueT) {
//<Value/1
//<Value/2
//>EnumValueT
	enum_value_t *A = (enum_value_t *)Args[0];
	enum_value_t *B = (enum_value_t *)Args[1];
	if (A->Type != B->Type) return ml_error("TypeError", "Flags are of different types");
	enum_value_t *C = new(enum_value_t);
	C->Type = A->Type;
	size_t LengthA = ml_string_length(A->Name);
	size_t LengthB = ml_string_length(B->Name);
	size_t Length = LengthA + LengthB + 1;
	char *Name = GC_MALLOC_ATOMIC(Length + 1);
	memcpy(Name, ml_string_value(A->Name), LengthA);
	Name[LengthA] = '|';
	memcpy(Name + LengthA + 1, ml_string_value(B->Name), LengthB);
	Name[Length] = 0;
	C->Name = ml_string(Name, Length);
	C->Value = A->Value | B->Value;
	return (ml_value_t *)C;
}

static size_t array_element_size(GITypeInfo *Info) {
	switch (g_type_info_get_tag(Info)) {
	case GI_TYPE_TAG_VOID: return sizeof(char);
	case GI_TYPE_TAG_BOOLEAN: return sizeof(gboolean);
	case GI_TYPE_TAG_INT8: return sizeof(gint8);
	case GI_TYPE_TAG_UINT8: return sizeof(guint8);
	case GI_TYPE_TAG_INT16: return sizeof(gint16);
	case GI_TYPE_TAG_UINT16: return sizeof(guint16);
	case GI_TYPE_TAG_INT32: return sizeof(gint32);
	case GI_TYPE_TAG_UINT32: return sizeof(guint32);
	case GI_TYPE_TAG_INT64: return sizeof(gint64);
	case GI_TYPE_TAG_UINT64: return sizeof(guint64);
	case GI_TYPE_TAG_FLOAT: return sizeof(gfloat);
	case GI_TYPE_TAG_DOUBLE: return sizeof(gdouble);
	case GI_TYPE_TAG_GTYPE: return sizeof(GType);
	case GI_TYPE_TAG_UTF8: return sizeof(char *);
	case GI_TYPE_TAG_FILENAME: return sizeof(char *);
	case GI_TYPE_TAG_ARRAY: return sizeof(void *);
	case GI_TYPE_TAG_INTERFACE: return sizeof(void *);
	case GI_TYPE_TAG_GLIST: return sizeof(GList *);
	case GI_TYPE_TAG_GSLIST: return sizeof(GSList *);
	case GI_TYPE_TAG_GHASH: return sizeof(GHashTable *);
	case GI_TYPE_TAG_ERROR: return sizeof(GError *);
	case GI_TYPE_TAG_UNICHAR: return sizeof(gunichar);
	}
	return 0;
}

typedef struct ml_gir_callback_t {
	ml_value_t *Function;
	GICallbackInfo *Info;
	ffi_cif Cif[1];
} ml_gir_callback_t;

static void callback_invoke(ffi_cif *Cif, void *Return, void **Params, ml_gir_callback_t *Callback) {
	GICallbackInfo *Info = Callback->Info;
	int Count = g_callable_info_get_n_args((GICallableInfo *)Info);
	ml_value_t *Args[Count];
	for (int I = 0; I < Count; ++I) {
		Args[I] = MLNil;
		GIArgInfo *ArgInfo = g_callable_info_get_arg((GICallableInfo *)Info, I);
		GITypeInfo TypeInfo[1];
		g_arg_info_load_type(ArgInfo, TypeInfo);
		switch (g_type_info_get_tag(TypeInfo)) {
		case GI_TYPE_TAG_VOID:
			Args[I] = MLNil;
			break;
		case GI_TYPE_TAG_BOOLEAN:
			Args[I] = ml_boolean(*(int *)Params[I]);
			break;
		case GI_TYPE_TAG_INT8:
			Args[I] = ml_integer(*(int8_t *)Params[I]);
			break;
		case GI_TYPE_TAG_UINT8:
			Args[I] = ml_integer(*(uint8_t *)Params[I]);
			break;
		case GI_TYPE_TAG_INT16:
			Args[I] = ml_integer(*(int16_t *)Params[I]);
			break;
		case GI_TYPE_TAG_UINT16:
			Args[I] = ml_integer(*(uint16_t *)Params[I]);
			break;
		case GI_TYPE_TAG_INT32:
			Args[I] = ml_integer(*(int32_t *)Params[I]);
			break;
		case GI_TYPE_TAG_UINT32:
			Args[I] = ml_integer(*(uint32_t *)Params[I]);
			break;
		case GI_TYPE_TAG_INT64:
			Args[I] = ml_integer(*(int64_t *)Params[I]);
			break;
		case GI_TYPE_TAG_UINT64:
			Args[I] = ml_integer(*(uint64_t *)Params[I]);
			break;
		case GI_TYPE_TAG_FLOAT:
		case GI_TYPE_TAG_DOUBLE:
			Args[I] = ml_real(*(double *)Params[I]);
			break;
		case GI_TYPE_TAG_GTYPE:
			break;
		case GI_TYPE_TAG_UTF8:
		case GI_TYPE_TAG_FILENAME:
			Args[I] = ml_string(*(char **)Params[I], -1);
			break;
		case GI_TYPE_TAG_ARRAY:
			break;
		case GI_TYPE_TAG_INTERFACE: {
			GIBaseInfo *InterfaceInfo = g_type_info_get_interface(TypeInfo);
			switch (g_base_info_get_type(InterfaceInfo)) {
			case GI_INFO_TYPE_INVALID:
			case GI_INFO_TYPE_INVALID_0: {
				break;
			}
			case GI_INFO_TYPE_FUNCTION: {
				break;
			}
			case GI_INFO_TYPE_CALLBACK: {
				break;
			}
			case GI_INFO_TYPE_STRUCT: {
				struct_instance_t *Instance = new(struct_instance_t);
				Instance->Type = (struct_t *)struct_info_lookup((GIStructInfo *)InterfaceInfo);
				Instance->Value = *(void **)Params[I];
				Args[I] = (ml_value_t *)Instance;
				break;
			}
			case GI_INFO_TYPE_BOXED: {
				break;
			}
			case GI_INFO_TYPE_ENUM: {
				enum_t *Enum = (enum_t *)enum_info_lookup((GIEnumInfo *)InterfaceInfo);
				Args[I] = Enum->ByIndex[*(int *)Params[I]];
				break;
			}
			case GI_INFO_TYPE_FLAGS: {
				break;
			}
			case GI_INFO_TYPE_OBJECT:
			case GI_INFO_TYPE_INTERFACE: {
				Args[I] = ml_gir_instance_get(*(void **)Params[I], InterfaceInfo);
				break;
			}
			case GI_INFO_TYPE_CONSTANT: {
				break;
			}
			case GI_INFO_TYPE_UNION: {
				break;
			}
			case GI_INFO_TYPE_VALUE: {
				break;
			}
			case GI_INFO_TYPE_SIGNAL: {
				break;
			}
			case GI_INFO_TYPE_VFUNC: {
				break;
			}
			case GI_INFO_TYPE_PROPERTY: {
				break;
			}
			case GI_INFO_TYPE_FIELD: {
				break;
			}
			case GI_INFO_TYPE_ARG: {
				break;
			}
			case GI_INFO_TYPE_TYPE: {
				break;
			}
			case GI_INFO_TYPE_UNRESOLVED: {
				break;
			}
			}
			break;
		}
		case GI_TYPE_TAG_GLIST: {
			Args[I] = ml_list();
			break;
		}
		case GI_TYPE_TAG_GSLIST: {
			Args[I] = ml_list();
			break;
		}
		case GI_TYPE_TAG_GHASH: {
			Args[I] = ml_map();
			break;
		}
		case GI_TYPE_TAG_ERROR: {
			GError *Error = *(GError **)Params[I];
			Args[I] = ml_error("GError", "%s", Error->message);
			break;
		}
		case GI_TYPE_TAG_UNICHAR:
			Args[I] = ml_integer(*(gunichar *)Params[I]);
			break;
		}
	}
	ml_value_t *Result = ml_simple_call(Callback->Function, Count, Args);
	GITypeInfo *ReturnInfo = g_callable_info_get_return_type((GICallableInfo *)Info);
	switch (g_type_info_get_tag(ReturnInfo)) {
	case GI_TYPE_TAG_VOID: break;
	case GI_TYPE_TAG_BOOLEAN:
		*(int *)Return = ml_boolean_value(Result);
		break;
	case GI_TYPE_TAG_INT8:
		*(int8_t *)Return = ml_integer_value(Result);
		break;
	case GI_TYPE_TAG_UINT8:
		*(uint8_t *)Return = ml_integer_value(Result);
		break;
	case GI_TYPE_TAG_INT16:
		*(int16_t *)Return = ml_integer_value(Result);
		break;
	case GI_TYPE_TAG_UINT16:
		*(uint16_t *)Return = ml_integer_value(Result);
		break;
	case GI_TYPE_TAG_INT32:
		*(int32_t *)Return = ml_integer_value(Result);
		break;
	case GI_TYPE_TAG_UINT32:
		*(uint32_t *)Return = ml_integer_value(Result);
		break;
	case GI_TYPE_TAG_INT64:
		*(int64_t *)Return = ml_integer_value(Result);
		break;
	case GI_TYPE_TAG_UINT64:
		*(uint64_t *)Return = ml_integer_value(Result);
		break;
	case GI_TYPE_TAG_FLOAT:
	case GI_TYPE_TAG_DOUBLE:
		*(double *)Return = ml_real_value(Result);
		break;
	case GI_TYPE_TAG_GTYPE: {
		break;
	}
	case GI_TYPE_TAG_UTF8:
	case GI_TYPE_TAG_FILENAME:
		*(const char **)Return = ml_string_value(Result);
		break;
	case GI_TYPE_TAG_ARRAY: {
		if (!ml_is(Result, MLListT)) {
			*(void **)Return = 0;
			break;
		}
		GITypeInfo *ElementInfo = g_type_info_get_param_type(ReturnInfo, 0);
		size_t ElementSize = array_element_size(ElementInfo);
		char *Array = GC_MALLOC_ATOMIC((ml_list_length(Result) + 1) * ElementSize);
		// TODO: fill array
		*(void **)Result = Array;
		break;
	}
	case GI_TYPE_TAG_INTERFACE: {
		GIBaseInfo *InterfaceInfo = g_type_info_get_interface(ReturnInfo);
		switch (g_base_info_get_type(InterfaceInfo)) {
		case GI_INFO_TYPE_INVALID:
		case GI_INFO_TYPE_INVALID_0: break;
		case GI_INFO_TYPE_FUNCTION: break;
		case GI_INFO_TYPE_CALLBACK: {
			ml_gir_callback_t *Callback = new(ml_gir_callback_t);
			Callback->Info = InterfaceInfo;
			Callback->Function = Result;
			*(void **)Return = g_callable_info_prepare_closure(
				InterfaceInfo,
				Callback->Cif,
				(GIFFIClosureCallback)callback_invoke,
				Callback
			);
			break;
		}
		case GI_INFO_TYPE_STRUCT: {
			if (ml_is(Result, StructInstanceT)) {
				*(void **)Return = ((struct_instance_t *)Result)->Value;
			} else {
				*(void **)Return = 0;
			}
			break;
		}
		case GI_INFO_TYPE_BOXED: break;
		case GI_INFO_TYPE_ENUM: {
			if (ml_is(Result, EnumValueT)) {
				*(int *)Return = ((enum_value_t *)Result)->Value;
			} else {
				*(int *)Return = 0;
			}
			break;
		}
		case GI_INFO_TYPE_FLAGS: break;
		case GI_INFO_TYPE_OBJECT: {
			if (ml_is(Result, ObjectInstanceT)) {
				*(void **)Return = ((object_instance_t *)Result)->Handle;
			} else {
				*(void **)Return = 0;
			}
			break;
		}
		case GI_INFO_TYPE_INTERFACE: {
			if (ml_is(Result, ObjectInstanceT)) {
				*(void **)Return = ((object_instance_t *)Result)->Handle;
			} else {
				*(void **)Return = 0;
			}
			break;
		}
		case GI_INFO_TYPE_CONSTANT: break;
		case GI_INFO_TYPE_UNION: break;
		case GI_INFO_TYPE_VALUE: break;
		case GI_INFO_TYPE_SIGNAL: break;
		case GI_INFO_TYPE_VFUNC: break;
		case GI_INFO_TYPE_PROPERTY: break;
		case GI_INFO_TYPE_FIELD: break;
		case GI_INFO_TYPE_ARG: break;
		case GI_INFO_TYPE_TYPE: break;
		case GI_INFO_TYPE_UNRESOLVED: break;
		}
		break;
	}
	case GI_TYPE_TAG_GLIST: {
		break;
	}
	case GI_TYPE_TAG_GSLIST: {
		break;
	}
	case GI_TYPE_TAG_GHASH: {
		break;
	}
	case GI_TYPE_TAG_ERROR: {
		break;
	}
	case GI_TYPE_TAG_UNICHAR: {
		break;
	}
	}
}

static GIBaseInfo *GValueInfo;

static ml_value_t *argument_to_value(GIArgument *Argument, GITypeInfo *Info) {
	switch (g_type_info_get_tag(Info)) {
	case GI_TYPE_TAG_VOID: {
		return MLNil;
	}
	case GI_TYPE_TAG_BOOLEAN: {
		return ml_boolean(Argument->v_boolean);
	}
	case GI_TYPE_TAG_INT8: {
		return ml_integer(Argument->v_int8);
	}
	case GI_TYPE_TAG_UINT8: {
		return ml_integer(Argument->v_uint8);
	}
	case GI_TYPE_TAG_INT16: {
		return ml_integer(Argument->v_int16);
	}
	case GI_TYPE_TAG_UINT16: {
		return ml_integer(Argument->v_uint16);
	}
	case GI_TYPE_TAG_INT32: {
		return ml_integer(Argument->v_int32);
	}
	case GI_TYPE_TAG_UINT32: {
		return ml_integer(Argument->v_uint32);
	}
	case GI_TYPE_TAG_INT64: {
		return ml_integer(Argument->v_int64);
	}
	case GI_TYPE_TAG_UINT64: {
		return ml_integer(Argument->v_uint64);
	}
	case GI_TYPE_TAG_FLOAT: {
		return ml_real(Argument->v_float);
	}
	case GI_TYPE_TAG_DOUBLE: {
		return ml_real(Argument->v_double);
	}
	case GI_TYPE_TAG_GTYPE: {
		return ml_cstring(g_type_name(Argument->v_size));
	}
	case GI_TYPE_TAG_UTF8:
	case GI_TYPE_TAG_FILENAME: {
		return ml_string(Argument->v_string, -1);
	}
	case GI_TYPE_TAG_ARRAY: {
		break;
	}
	case GI_TYPE_TAG_INTERFACE: {
		GIBaseInfo *InterfaceInfo = g_type_info_get_interface(Info);
		//printf("Interface = %s\n", g_base_info_get_name(InterfaceInfo));
		if (g_base_info_equal(InterfaceInfo, GValueInfo)) {
			return _value_to_ml(Argument->v_pointer, NULL);
		} else switch (g_base_info_get_type(InterfaceInfo)) {
		case GI_INFO_TYPE_INVALID:
		case GI_INFO_TYPE_INVALID_0: break;
		case GI_INFO_TYPE_FUNCTION: break;
		case GI_INFO_TYPE_CALLBACK: break;
		case GI_INFO_TYPE_STRUCT: {
			struct_instance_t *Instance = new(struct_instance_t);
			Instance->Type = (struct_t *)struct_info_lookup((GIStructInfo *)InterfaceInfo);
			Instance->Value = Argument->v_pointer;
			return (ml_value_t *)Instance;
		}
		case GI_INFO_TYPE_BOXED: {
			break;
		}
		case GI_INFO_TYPE_ENUM: {
			enum_t *Enum = (enum_t *)enum_info_lookup((GIEnumInfo *)InterfaceInfo);
			return Enum->ByIndex[Argument->v_int];
		}
		case GI_INFO_TYPE_FLAGS: {
			break;
		}
		case GI_INFO_TYPE_OBJECT:
		case GI_INFO_TYPE_INTERFACE: {
			return ml_gir_instance_get(Argument->v_pointer, InterfaceInfo);
			break;
		}
		case GI_INFO_TYPE_CONSTANT: {
			break;
		}
		case GI_INFO_TYPE_UNION: {
			break;
		}
		case GI_INFO_TYPE_VALUE: {
			break;
		}
		case GI_INFO_TYPE_SIGNAL: {
			break;
		}
		case GI_INFO_TYPE_VFUNC: {
			break;
		}
		case GI_INFO_TYPE_PROPERTY: {
			break;
		}
		case GI_INFO_TYPE_FIELD: {
			break;
		}
		case GI_INFO_TYPE_ARG: {
			break;
		}
		case GI_INFO_TYPE_TYPE: {
			break;
		}
		case GI_INFO_TYPE_UNRESOLVED: {
			break;
		}
		}
		break;
	}
	case GI_TYPE_TAG_GLIST:
	case GI_TYPE_TAG_GSLIST: {
		return ml_list();
	}
	case GI_TYPE_TAG_GHASH: {
		return ml_map();
	}
	case GI_TYPE_TAG_ERROR: {
		GError *Error = Argument->v_pointer;
		return ml_error("GError", "%s", Error->message);
	}
	case GI_TYPE_TAG_UNICHAR: {
		return ml_integer(Argument->v_uint32);
	}
	}
	asm("int3");
	return ml_error("ValueError", "Unsupported situtation");
}

static void *list_to_array(ml_value_t *List, GITypeInfo *TypeInfo) {
	size_t ElementSize = array_element_size(TypeInfo);
	char *Array = GC_MALLOC_ATOMIC((ml_list_length(List) + 1) * ElementSize);
	switch (g_type_info_get_tag(TypeInfo)) {
	case GI_TYPE_TAG_BOOLEAN: {
		gboolean *Ptr = (gboolean *)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLBooleanT)) {
				*Ptr++ = ml_boolean_value(Iter->Value);
			}
		}
		break;
	}
	case GI_TYPE_TAG_INT8: {
		gint8 *Ptr = (gint8 *)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLIntegerT)) {
				*Ptr++ = ml_integer_value(Iter->Value);
			}
		}
		break;
	}
	case GI_TYPE_TAG_UINT8: {
		guint8 *Ptr = (guint8 *)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLIntegerT)) {
				*Ptr++ = ml_integer_value(Iter->Value);
			}
		}
		break;
	}
	case GI_TYPE_TAG_INT16: {
		gint16 *Ptr = (gint16 *)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLIntegerT)) {
				*Ptr++ = ml_integer_value(Iter->Value);
			}
		}
		break;
	}
	case GI_TYPE_TAG_UINT16: {
		guint16 *Ptr = (guint16 *)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLIntegerT)) {
				*Ptr++ = ml_integer_value(Iter->Value);
			}
		}
		break;
	}
	case GI_TYPE_TAG_INT32: {
		gint32 *Ptr = (gint32 *)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLIntegerT)) {
				*Ptr++ = ml_integer_value(Iter->Value);
			}
		}
		break;
	}
	case GI_TYPE_TAG_UINT32: {
		guint32 *Ptr = (guint32 *)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLIntegerT)) {
				*Ptr++ = ml_integer_value(Iter->Value);
			}
		}
		break;
	}
	case GI_TYPE_TAG_INT64: {
		gint64 *Ptr = (gint64 *)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLIntegerT)) {
				*Ptr++ = ml_integer_value(Iter->Value);
			}
		}
		break;
	}
	case GI_TYPE_TAG_UINT64: {
		guint64 *Ptr = (guint64 *)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLIntegerT)) {
				*Ptr++ = ml_integer_value(Iter->Value);
			}
		}
		break;
	}
	case GI_TYPE_TAG_FLOAT: {
		gfloat *Ptr = (gfloat *)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLRealT)) {
				*Ptr++ = ml_real_value(Iter->Value);
			}
		}
		break;
	}
	case GI_TYPE_TAG_DOUBLE: {
		gdouble *Ptr = (gdouble *)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLRealT)) {
				*Ptr++ = ml_real_value(Iter->Value);
			}
		}
		break;
	}
	case GI_TYPE_TAG_GTYPE: {
		break;
	}
	case GI_TYPE_TAG_UTF8:
	case GI_TYPE_TAG_FILENAME: {
		const gchar **Ptr = (const gchar **)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (Iter->Value == MLNil) {
				*Ptr++ = NULL;
			} else if (ml_is(Iter->Value, MLStringT)) {
				*Ptr++ = ml_string_value(Iter->Value);
			}
		}
		break;
	}
	default:
		break;
	}
	return Array;
}

static GSList *list_to_slist(ml_value_t *List, GITypeInfo *TypeInfo) {
	GSList *Head = NULL, **Slot = &Head;
	switch (g_type_info_get_tag(TypeInfo)) {
	case GI_TYPE_TAG_BOOLEAN: {
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLBooleanT)) {
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = GINT_TO_POINTER(ml_boolean_value(Iter->Value));
				Slot = &Node->next;
			}
		}
		break;
	}
	case GI_TYPE_TAG_INT8:
	case GI_TYPE_TAG_UINT8:
	case GI_TYPE_TAG_INT16:
	case GI_TYPE_TAG_UINT16:
	case GI_TYPE_TAG_INT32:
	case GI_TYPE_TAG_UINT32:
	case GI_TYPE_TAG_INT64:
	case GI_TYPE_TAG_UINT64: {
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLIntegerT)) {
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = GINT_TO_POINTER(ml_integer_value(Iter->Value));
				Slot = &Node->next;
			}
		}
		break;
	}
	case GI_TYPE_TAG_GTYPE: {
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, BaseInfoT)) {
				baseinfo_t *Base = (baseinfo_t *)Iter->Value;
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = GINT_TO_POINTER(g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)Base->Info));
				Slot = &Node->next;
			} else if (ml_is(Iter->Value, MLStringT)) {
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = GINT_TO_POINTER(g_type_from_name(ml_string_value(Iter->Value)));
				Slot = &Node->next;
			}
		}
		break;
	}
	case GI_TYPE_TAG_UTF8:
	case GI_TYPE_TAG_FILENAME: {
		ML_LIST_FOREACH(List, Iter) {
			if (Iter->Value == MLNil) {
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = NULL;
				Slot = &Node->next;
			} else if (ml_is(Iter->Value, MLStringT)) {
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = (void *)ml_string_value(Iter->Value);
				Slot = &Node->next;
			}
		}
		break;
	}
	case GI_TYPE_TAG_INTERFACE: {
		GIBaseInfo *InterfaceInfo = g_type_info_get_interface(TypeInfo);
		if (g_base_info_equal(InterfaceInfo, GValueInfo)) {
			GValue *GValues = anew(GValue, ml_list_length(List));
			GValue *Value = GValues;
			ML_LIST_FOREACH(List, Iter) {
				GSList *Node = Slot[0] = g_slist_alloc();
				_ml_to_value(Iter->Value, Value);
				Node->data = Value++;
				Slot = &Node->next;
			}
		} else switch (g_base_info_get_type(InterfaceInfo)) {
		case GI_INFO_TYPE_CALLBACK: {
			ML_LIST_FOREACH(List, Iter) {
				GSList *Node = Slot[0] = g_slist_alloc();
				ml_gir_callback_t *Callback = new(ml_gir_callback_t);
				Callback->Info = InterfaceInfo;
				Callback->Function = Iter->Value;
				Node->data = g_callable_info_prepare_closure(
					InterfaceInfo,
					Callback->Cif,
					(GIFFIClosureCallback)callback_invoke,
					Callback
				);
				Slot = &Node->next;
			}
			break;
		}
		case GI_INFO_TYPE_STRUCT: {
			ML_LIST_FOREACH(List, Iter) {
				GSList *Node = Slot[0] = g_slist_alloc();
				if (Iter->Value == MLNil) {
					Node->data = NULL;
				} else if (ml_is(Iter->Value, StructInstanceT)) {
					Node->data = ((struct_instance_t *)Iter->Value)->Value;
				}
				Slot = &Node->next;
			}
			break;
		}
		case GI_INFO_TYPE_ENUM:
		case GI_INFO_TYPE_FLAGS: {
			ML_LIST_FOREACH(List, Iter) {
				GSList *Node = Slot[0] = g_slist_alloc();
				if (Iter->Value == MLNil) {
					Node->data = GINT_TO_POINTER(0);
				} else if (ml_is(Iter->Value, EnumValueT)) {
					Node->data = GINT_TO_POINTER(((enum_value_t *)Iter->Value)->Value);
				}
				Slot = &Node->next;
			}
			break;
		}
		case GI_INFO_TYPE_OBJECT:
		case GI_INFO_TYPE_INTERFACE: {
			ML_LIST_FOREACH(List, Iter) {
				GSList *Node = Slot[0] = g_slist_alloc();
				if (Iter->Value == MLNil) {
					Node->data = NULL;
				} else if (ml_is(Iter->Value, ObjectInstanceT)) {
					Node->data = ((object_instance_t *)Iter->Value)->Handle;
				}
				Slot = &Node->next;
			}
			break;
		}
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
	return Head;
}

static void set_input_length(GICallableInfo *Info, int Index, GIArgument *ArgsIn, gsize Length) {
	if (g_function_info_get_flags(Info) & GI_FUNCTION_IS_METHOD) ++ArgsIn;
	for (int I = 0; I < Index; ++I) {
		GIArgInfo *ArgInfo = g_callable_info_get_arg((GICallableInfo *)Info, I);
		if (g_arg_info_get_direction(ArgInfo) != GI_DIRECTION_OUT) {
			++ArgsIn;
		}
	}
	ArgsIn->v_uint64 = Length;
}

static size_t get_output_length(GICallableInfo *Info, int Index, GIArgument *ArgsOut) {
	for (int I = 0; I < Index; ++I) {
		GIArgInfo *ArgInfo = g_callable_info_get_arg((GICallableInfo *)Info, I);
		if (g_arg_info_get_direction(ArgInfo) != GI_DIRECTION_IN) {
			++ArgsOut;
		}
	}
	return ((GIArgument *)ArgsOut->v_pointer)->v_uint64;
}

static GIBaseInfo *DestroyNotifyInfo;

static ml_value_t *function_info_invoke(GIFunctionInfo *Info, int Count, ml_value_t **Args) {
	int NArgs = g_callable_info_get_n_args((GICallableInfo *)Info);
	int NArgsIn = 0, NArgsOut = 0;
	for (int I = 0; I < NArgs; ++I) {
		GIArgInfo *ArgInfo = g_callable_info_get_arg((GICallableInfo *)Info, I);
		switch (g_arg_info_get_direction(ArgInfo)) {
		case GI_DIRECTION_IN: ++NArgsIn; break;
		case GI_DIRECTION_OUT: ++NArgsOut; break;
		case GI_DIRECTION_INOUT: ++NArgsIn; ++NArgsOut; break;
		}
	}
	//GIFunctionInfoFlags Flags = g_function_info_get_flags(Info);
	GIArgument ArgsIn[NArgsIn];
	GIArgument ArgsOut[NArgsOut];
	GIArgument ResultsOut[NArgsOut];
	GValue GValues[NArgs];
	for (int I = 0; I < NArgsIn; ++I) ArgsIn[I].v_pointer = NULL;
	int IndexIn = 0, IndexOut = 0, IndexResult = 0, IndexValue = 0, N = 0;
	if (g_function_info_get_flags(Info) & GI_FUNCTION_IS_METHOD) {
		ArgsIn[0].v_pointer = ((object_instance_t *)Args[0])->Handle;
		N = 1;
		IndexIn = 1;
	}
	//printf("%s(", g_base_info_get_name((GIBaseInfo *)Info));
	uint64_t Skips = 0;
	for (int I = 0; I < NArgs; ++I) {
		GIArgInfo *ArgInfo = g_callable_info_get_arg((GICallableInfo *)Info, I);
		int ClosureArg = g_arg_info_get_closure(ArgInfo);
		if (ClosureArg >= 0) Skips |= 1 << ClosureArg;
		GITypeInfo TypeInfo[1];
		g_arg_info_load_type(ArgInfo, TypeInfo);
		//printf(I ? ", %s : %d" : "%s : %d", g_base_info_get_name((GIBaseInfo *)ArgInfo), g_type_info_get_tag(TypeInfo));
		if (g_type_info_get_tag(TypeInfo) == GI_TYPE_TAG_ARRAY) {
			int LengthIndex = g_type_info_get_array_length(TypeInfo);
			if (LengthIndex >= 0) {
				Skips |= 1 << LengthIndex;
			}
		}
	}
	//printf(")\n");
	for (int I = 0; I < NArgs; ++I, Skips >>= 1) {
		GIArgInfo *ArgInfo = g_callable_info_get_arg((GICallableInfo *)Info, I);
		GITypeInfo TypeInfo[1];
		g_arg_info_load_type(ArgInfo, TypeInfo);
		switch (g_arg_info_get_direction(ArgInfo)) {
		case GI_DIRECTION_IN: {
			if (Skips % 2) goto skip_in_arg;
			GITypeTag Tag = g_type_info_get_tag(TypeInfo);
			if (N >= Count) {
				if (Tag == GI_TYPE_TAG_INTERFACE) {
					GIBaseInfo *InterfaceInfo = g_type_info_get_interface(TypeInfo);
					if (g_base_info_equal(InterfaceInfo, DestroyNotifyInfo)) {
						ArgsIn[IndexIn].v_pointer = NULL;
						goto skip_in_arg;
					}
				}
				return ml_error("InvokeError", "Not enough arguments");
			}
			ml_value_t *Arg = Args[N++];
			switch (Tag) {
			case GI_TYPE_TAG_VOID: break;
			case GI_TYPE_TAG_BOOLEAN: {
				ArgsIn[IndexIn].v_boolean = ml_boolean_value(Arg);
				break;
			}
			case GI_TYPE_TAG_INT8: {
				ArgsIn[IndexIn].v_int8 = ml_integer_value(Arg);
				break;
			}
			case GI_TYPE_TAG_UINT8: {
				ArgsIn[IndexIn].v_uint8 = ml_integer_value(Arg);
				break;
			}
			case GI_TYPE_TAG_INT16: {
				ArgsIn[IndexIn].v_int16 = ml_integer_value(Arg);
				break;
			}
			case GI_TYPE_TAG_UINT16: {
				ArgsIn[IndexIn].v_uint16 = ml_integer_value(Arg);
				break;
			}
			case GI_TYPE_TAG_INT32: {
				ArgsIn[IndexIn].v_int32 = ml_integer_value(Arg);
				break;
			}
			case GI_TYPE_TAG_UINT32: {
				ArgsIn[IndexIn].v_uint32 = ml_integer_value(Arg);
				break;
			}
			case GI_TYPE_TAG_INT64: {
				ArgsIn[IndexIn].v_int64 = ml_integer_value(Arg);
				break;
			}
			case GI_TYPE_TAG_UINT64: {
				ArgsIn[IndexIn].v_uint64 = ml_integer_value(Arg);
				break;
			}
			case GI_TYPE_TAG_FLOAT: {
				ArgsIn[IndexIn].v_float = ml_real_value(Arg);
				break;
			}
			case GI_TYPE_TAG_DOUBLE: {
				ArgsIn[IndexIn].v_double = ml_real_value(Arg);
				break;
			}
			case GI_TYPE_TAG_GTYPE: {
				if (ml_is(Arg, BaseInfoT)) {
					baseinfo_t *Base = (baseinfo_t *)Arg;
					ArgsIn[IndexIn].v_size = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)Base->Info);
				} else if (ml_is(Arg, MLStringT)) {
					ArgsIn[IndexIn].v_size = g_type_from_name(ml_string_value(Arg));
				} else if (Arg == (ml_value_t *)MLNilT) {
					ArgsIn[IndexIn].v_size = G_TYPE_NONE;
				} else if (Arg == (ml_value_t *)MLIntegerT) {
					ArgsIn[IndexIn].v_size = G_TYPE_INT64;
				} else if (Arg == (ml_value_t *)MLStringT) {
					ArgsIn[IndexIn].v_size = G_TYPE_STRING;
				} else if (Arg == (ml_value_t *)MLRealT) {
					ArgsIn[IndexIn].v_size = G_TYPE_DOUBLE;
				} else if (Arg == (ml_value_t *)MLBooleanT) {
					ArgsIn[IndexIn].v_size = G_TYPE_BOOLEAN;
				} else {
					return ml_error("TypeError", "Expected type for parameter %d", I);
				}
				break;
			}
			case GI_TYPE_TAG_UTF8:
			case GI_TYPE_TAG_FILENAME: {
				if (Arg == MLNil) {
					ArgsIn[IndexIn].v_string = NULL;
				} else {
					ML_CHECK_ARG_TYPE(N - 1, MLStringT);
					ArgsIn[IndexIn].v_string = (char *)ml_string_value(Arg);
				}
				break;
			}
			case GI_TYPE_TAG_ARRAY: {
				GITypeInfo *ElementInfo = g_type_info_get_param_type(TypeInfo, 0);
				int LengthIndex = g_type_info_get_array_length(TypeInfo);
				switch (g_type_info_get_tag(ElementInfo)) {
				case GI_TYPE_TAG_INT8:
				case GI_TYPE_TAG_UINT8: {
					if (ml_is(Arg, MLStringT)) {
						ArgsIn[IndexIn].v_pointer = (void *)ml_string_value(Arg);
						if (LengthIndex >= 0) {
							set_input_length(Info, LengthIndex, ArgsIn, ml_string_length(Arg));
						}
					} else if (ml_is(Arg, MLListT)) {
						ArgsIn[IndexIn].v_pointer = list_to_array(Arg, ElementInfo);
						if (LengthIndex >= 0) {
							set_input_length(Info, LengthIndex, ArgsIn, ml_list_length(Arg));
						}
					} else {
						return ml_error("TypeError", "Expected list for parameter %d", I);
					}
					break;
				}
				default: {
					if (!ml_is(Arg, MLListT)) {
						return ml_error("TypeError", "Expected list for parameter %d", I);
					}
					ArgsIn[IndexIn].v_pointer = list_to_array(Arg, ElementInfo);
					if (LengthIndex >= 0) {
						set_input_length(Info, LengthIndex, ArgsIn, ml_list_length(Arg));
					}
					break;
				}
				}
				break;
			}
			case GI_TYPE_TAG_INTERFACE: {
				GIBaseInfo *InterfaceInfo = g_type_info_get_interface(TypeInfo);
				if (g_base_info_equal(InterfaceInfo, GValueInfo)) {
					ArgsIn[IndexIn].v_pointer = &GValues[IndexValue];
					memset(&GValues[IndexValue], 0, sizeof(GValue));
					_ml_to_value(Arg, &GValues[IndexValue]);
					++IndexValue;
				} else switch (g_base_info_get_type(InterfaceInfo)) {
				case GI_INFO_TYPE_INVALID:
				case GI_INFO_TYPE_INVALID_0: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_FUNCTION: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_CALLBACK: {
					ml_gir_callback_t *Callback = new(ml_gir_callback_t);
					Callback->Info = InterfaceInfo;
					Callback->Function = Arg;
					ArgsIn[IndexIn].v_pointer = g_callable_info_prepare_closure(
						InterfaceInfo,
						Callback->Cif,
						(GIFFIClosureCallback)callback_invoke,
						Callback
					);
					break;
				}
				case GI_INFO_TYPE_STRUCT: {
					if (Arg == MLNil) {
						ArgsIn[IndexIn].v_pointer = NULL;
					} else if (ml_is(Arg, StructInstanceT)) {
						ArgsIn[IndexIn].v_pointer = ((struct_instance_t *)Arg)->Value;
					} else {
						return ml_error("TypeError", "Expected gir struct not %s for parameter %d", ml_typeof(Args[I])->Name, I);
					}
					break;
				}
				case GI_INFO_TYPE_BOXED: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_ENUM:
				case GI_INFO_TYPE_FLAGS: {
					if (Arg == MLNil) {
						ArgsIn[IndexIn].v_int64 = 0;
					} else if (ml_is(Arg, EnumValueT)) {
						ArgsIn[IndexIn].v_int64 = ((enum_value_t *)Arg)->Value;
					} else {
						return ml_error("TypeError", "Expected gir enum not %s for parameter %d", ml_typeof(Args[I])->Name, I);
					}
					break;
				}
				case GI_INFO_TYPE_OBJECT:
				case GI_INFO_TYPE_INTERFACE: {
					if (Arg == MLNil) {
						ArgsIn[IndexIn].v_pointer = NULL;
					} else if (ml_is(Arg, ObjectInstanceT)) {
						ArgsIn[IndexIn].v_pointer = ((object_instance_t *)Arg)->Handle;
					} else {
						return ml_error("TypeError", "Expected gir object not %s for parameter %d", ml_typeof(Args[I])->Name, I);
					}
					break;
				}
				case GI_INFO_TYPE_CONSTANT: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_UNION: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_VALUE: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_SIGNAL: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_VFUNC: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_PROPERTY: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_FIELD: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_ARG: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_TYPE: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_UNRESOLVED: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				}
				break;
			}
			case GI_TYPE_TAG_GLIST: {
				break;
			}
			case GI_TYPE_TAG_GSLIST: {
				GITypeInfo *ElementInfo = g_type_info_get_param_type(TypeInfo, 0);
				ArgsIn[IndexIn].v_pointer = list_to_slist(Arg, ElementInfo);
				break;
			}
			case GI_TYPE_TAG_GHASH: {
				break;
			}
			case GI_TYPE_TAG_ERROR: {
				break;
			}
			case GI_TYPE_TAG_UNICHAR: {
				break;
			}
			}
		skip_in_arg:
			++IndexIn;
			break;
		}
		case GI_DIRECTION_OUT: {
			switch (g_type_info_get_tag(TypeInfo)) {
			case GI_TYPE_TAG_VOID: break;
			case GI_TYPE_TAG_BOOLEAN:
			case GI_TYPE_TAG_INT8:
			case GI_TYPE_TAG_UINT8:
			case GI_TYPE_TAG_INT16:
			case GI_TYPE_TAG_UINT16:
			case GI_TYPE_TAG_INT32:
			case GI_TYPE_TAG_UINT32:
			case GI_TYPE_TAG_INT64:
			case GI_TYPE_TAG_UINT64:
			case GI_TYPE_TAG_FLOAT:
			case GI_TYPE_TAG_DOUBLE:
			case GI_TYPE_TAG_GTYPE:
			case GI_TYPE_TAG_UTF8:
			case GI_TYPE_TAG_FILENAME:
			case GI_TYPE_TAG_ARRAY: {
				ArgsOut[IndexOut].v_pointer = &ResultsOut[IndexResult++];
				break;
			}
			case GI_TYPE_TAG_INTERFACE: {
				GIBaseInfo *InterfaceInfo = g_type_info_get_interface(TypeInfo);
				if (g_base_info_equal(InterfaceInfo, GValueInfo)) {
					ArgsOut[IndexOut].v_pointer = &GValues[IndexValue];
					ResultsOut[IndexResult++].v_pointer = &GValues[IndexValue];
					memset(&GValues[IndexValue], 0, sizeof(GValue));
					++IndexValue;
				} else switch (g_base_info_get_type(InterfaceInfo)) {
				case GI_INFO_TYPE_INVALID:
				case GI_INFO_TYPE_INVALID_0: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_FUNCTION: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_CALLBACK: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_STRUCT: {
					if (N >= Count) return ml_error("InvokeError", "Not enough arguments");
					ml_value_t *Arg = Args[N++];
					if (ml_is(Arg, StructInstanceT)) {
						ArgsOut[IndexOut].v_pointer = ((struct_instance_t *)Arg)->Value;
					} else {
						return ml_error("TypeError", "Expected gir struct not %s for parameter %d", ml_typeof(Args[I])->Name, I);
					}
					break;
				}
				case GI_INFO_TYPE_BOXED: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_ENUM: {
					break;
				}
				case GI_INFO_TYPE_FLAGS: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_OBJECT:
				case GI_INFO_TYPE_INTERFACE: {
					ArgsOut[IndexOut].v_pointer = &ResultsOut[IndexResult++];
					break;
				}
				case GI_INFO_TYPE_CONSTANT: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_UNION: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_VALUE: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_SIGNAL: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_VFUNC: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_PROPERTY: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_FIELD: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_ARG: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_TYPE: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_UNRESOLVED: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				}
				break;
			}
			case GI_TYPE_TAG_GLIST: {
				break;
			}
			case GI_TYPE_TAG_GSLIST: {
				break;
			}
			case GI_TYPE_TAG_GHASH: {
				break;
			}
			case GI_TYPE_TAG_ERROR: {
				break;
			}
			case GI_TYPE_TAG_UNICHAR: {
				break;
			}
			}
			++IndexOut;
			break;
		}
		case GI_DIRECTION_INOUT: {
			if (N >= Count) return ml_error("InvokeError", "Not enough arguments");
			ml_value_t *Arg = Args[N++];
			switch (g_type_info_get_tag(TypeInfo)) {
			case GI_TYPE_TAG_VOID: break;
			case GI_TYPE_TAG_BOOLEAN:
			case GI_TYPE_TAG_INT8:
			case GI_TYPE_TAG_UINT8:
			case GI_TYPE_TAG_INT16:
			case GI_TYPE_TAG_UINT16:
			case GI_TYPE_TAG_INT32:
			case GI_TYPE_TAG_UINT32:
			case GI_TYPE_TAG_INT64:
			case GI_TYPE_TAG_UINT64:
			case GI_TYPE_TAG_FLOAT:
			case GI_TYPE_TAG_DOUBLE:
			case GI_TYPE_TAG_GTYPE:
			case GI_TYPE_TAG_UTF8:
			case GI_TYPE_TAG_FILENAME:
			case GI_TYPE_TAG_ARRAY: {
				return ml_error("NotImplemented", "Not able to marshal in-out args yet at %d", __LINE__);
			}
			case GI_TYPE_TAG_INTERFACE: {
				GIBaseInfo *InterfaceInfo = g_type_info_get_interface(TypeInfo);
				if (g_base_info_equal(InterfaceInfo, GValueInfo)) {
					ArgsIn[IndexIn].v_pointer = &GValues[IndexValue];
					ArgsOut[IndexOut].v_pointer = &GValues[IndexValue];
					ResultsOut[IndexResult++].v_pointer = &GValues[IndexValue];
					_ml_to_value(Arg, &GValues[IndexValue]);
					++IndexValue;
				} else switch (g_base_info_get_type(InterfaceInfo)) {
				case GI_INFO_TYPE_INVALID:
				case GI_INFO_TYPE_INVALID_0: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_FUNCTION: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_CALLBACK: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_STRUCT: {
					if (ml_is(Arg, StructInstanceT)) {
						ArgsIn[IndexIn].v_pointer = ArgsOut[IndexOut].v_pointer = ((struct_instance_t *)Arg)->Value;
					} else {
						return ml_error("TypeError", "Expected gir struct not %s for parameter %d", ml_typeof(Args[I])->Name, I);
					}
					break;
				}
				case GI_INFO_TYPE_BOXED: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_ENUM: {
					break;
				}
				case GI_INFO_TYPE_FLAGS: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_OBJECT: {
					break;
				}
				case GI_INFO_TYPE_INTERFACE: {
					break;
				}
				case GI_INFO_TYPE_CONSTANT: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_UNION: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_VALUE: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_SIGNAL: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_VFUNC: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_PROPERTY: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_FIELD: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_ARG: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_TYPE: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_UNRESOLVED: {
					return ml_error("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				}
				break;
			}
			case GI_TYPE_TAG_GLIST: {
				break;
			}
			case GI_TYPE_TAG_GSLIST: {
				break;
			}
			case GI_TYPE_TAG_GHASH: {
				break;
			}
			case GI_TYPE_TAG_ERROR: {
				break;
			}
			case GI_TYPE_TAG_UNICHAR: {
				break;
			}
			}
			++IndexIn;
			++IndexOut;
			break;
		}
		}
	}
	GError *Error = 0;
	GIArgument ReturnValue[1];
	gboolean Invoked = g_function_info_invoke(Info, ArgsIn, IndexIn, ArgsOut, IndexOut, ReturnValue, &Error);
	if (!Invoked || Error) return ml_error("InvokeError", "Error: %s", Error->message);
	GITypeInfo *ReturnInfo = g_callable_info_get_return_type((GICallableInfo *)Info);
	if (!IndexResult) return argument_to_value(ReturnValue, ReturnInfo);
	ml_value_t *Result = ml_tuple(IndexResult + 1);
	ml_tuple_set(Result, 1, argument_to_value(ReturnValue, ReturnInfo));
	IndexResult = 0;
	for (int I = 0; I < NArgs; ++I) {
		GIArgInfo *ArgInfo = g_callable_info_get_arg((GICallableInfo *)Info, I);
		GITypeInfo TypeInfo[1];
		g_arg_info_load_type(ArgInfo, TypeInfo);
		switch (g_arg_info_get_direction(ArgInfo)) {
		case GI_DIRECTION_IN: break;
		case GI_DIRECTION_OUT: {
			switch (g_type_info_get_tag(TypeInfo)) {
			case GI_TYPE_TAG_VOID: break;
			case GI_TYPE_TAG_BOOLEAN:
				ml_tuple_set(Result, IndexResult + 2, ml_boolean(ResultsOut[IndexResult].v_boolean));
				++IndexResult;
				break;
			case GI_TYPE_TAG_INT8:
				ml_tuple_set(Result, IndexResult + 2, ml_integer(ResultsOut[IndexResult].v_int8));
				++IndexResult;
				break;
			case GI_TYPE_TAG_UINT8:
				ml_tuple_set(Result, IndexResult + 2, ml_integer(ResultsOut[IndexResult].v_uint8));
				++IndexResult;
				break;
			case GI_TYPE_TAG_INT16:
				ml_tuple_set(Result, IndexResult + 2, ml_integer(ResultsOut[IndexResult].v_int16));
				++IndexResult;
				break;
			case GI_TYPE_TAG_UINT16:
				ml_tuple_set(Result, IndexResult + 2, ml_integer(ResultsOut[IndexResult].v_uint16));
				++IndexResult;
				break;
			case GI_TYPE_TAG_INT32:
				ml_tuple_set(Result, IndexResult + 2, ml_integer(ResultsOut[IndexResult].v_int32));
				++IndexResult;
				break;
			case GI_TYPE_TAG_UINT32:
				ml_tuple_set(Result, IndexResult + 2, ml_integer(ResultsOut[IndexResult].v_uint32));
				++IndexResult;
				break;
			case GI_TYPE_TAG_INT64:
				ml_tuple_set(Result, IndexResult + 2, ml_integer(ResultsOut[IndexResult].v_int64));
				++IndexResult;
				break;
			case GI_TYPE_TAG_UINT64:
				ml_tuple_set(Result, IndexResult + 2, ml_integer(ResultsOut[IndexResult].v_uint64));
				++IndexResult;
				break;
			case GI_TYPE_TAG_FLOAT:
				ml_tuple_set(Result, IndexResult + 2, ml_real(ResultsOut[IndexResult].v_float));
				++IndexResult;
				break;
			case GI_TYPE_TAG_DOUBLE:
				ml_tuple_set(Result, IndexResult + 2, ml_real(ResultsOut[IndexResult].v_double));
				++IndexResult;
				break;
			case GI_TYPE_TAG_GTYPE:
				ml_tuple_set(Result, IndexResult + 2, MLNil);
				++IndexResult;
				break;
			case GI_TYPE_TAG_UTF8:
			case GI_TYPE_TAG_FILENAME:
				ml_tuple_set(Result, IndexResult + 2, ml_string(ResultsOut[IndexResult].v_string, -1));
				++IndexResult;
				break;
			case GI_TYPE_TAG_ARRAY: {
				GITypeInfo *ElementInfo = g_type_info_get_param_type(TypeInfo, 0);
				switch (g_type_info_get_tag(ElementInfo)) {
				case GI_TYPE_TAG_INT8:
				case GI_TYPE_TAG_UINT8: {
					if (g_type_info_is_zero_terminated(TypeInfo)) {
						ml_tuple_set(Result, IndexResult + 2, ml_cstring(ResultsOut[IndexResult].v_string));
					} else {
						size_t Length;
						int LengthIndex = g_type_info_get_array_length(TypeInfo);
						if (LengthIndex < 0) {
							Length = g_type_info_get_array_fixed_size(TypeInfo);
						} else {
							Length = get_output_length(Info, LengthIndex, ArgsOut);
						}
						ml_tuple_set(Result, IndexResult + 2, ml_string(ResultsOut[IndexResult].v_string, Length));
					}
					++IndexResult;
					break;
				}
				default: {
					ml_tuple_set(Result, IndexResult + 2, MLNil);
					++IndexResult;
					break;
				}
				}
				break;
			}
			case GI_TYPE_TAG_INTERFACE: {
				GIBaseInfo *InterfaceInfo = g_type_info_get_interface(TypeInfo);
				if (g_base_info_equal(InterfaceInfo, GValueInfo)) {
					ml_tuple_set(Result, IndexResult + 2, _value_to_ml((const GValue *)ResultsOut[IndexResult].v_pointer, NULL));
				} else switch (g_base_info_get_type(InterfaceInfo)) {
				case GI_INFO_TYPE_OBJECT:
				case GI_INFO_TYPE_INTERFACE: {
					ml_tuple_set(Result, IndexResult + 2, ml_gir_instance_get(ResultsOut[IndexResult].v_pointer, InterfaceInfo));
					++IndexResult;
					break;
				}
				default: break;
				}
				break;
			}
			default: break;
			}
			break;
		}
		case GI_DIRECTION_INOUT: break;
		}
	}
	return Result;
}

static ml_value_t *constructor_invoke(GIFunctionInfo *Info, int Count, ml_value_t **Args) {
	return function_info_invoke(Info, Count, Args);
}

static ml_value_t *method_invoke(GIFunctionInfo *Info, int Count, ml_value_t **Args) {
	return function_info_invoke(Info, Count, Args);
}

static void interface_add_methods(object_t *Object, GIInterfaceInfo *Info) {
	int NumMethods = g_interface_info_get_n_methods(Info);
	for (int I = 0; I < NumMethods; ++I) {
		GIFunctionInfo *MethodInfo = g_interface_info_get_method(Info, I);
		const char *MethodName = g_base_info_get_name((GIBaseInfo *)MethodInfo);
		GIFunctionInfoFlags Flags = g_function_info_get_flags(MethodInfo);
		if (Flags & GI_FUNCTION_IS_METHOD) {
			ml_method_by_name(MethodName, MethodInfo, (ml_callback_t)method_invoke, Object, NULL);
		}
	}
}

static void object_add_methods(object_t *Object, GIObjectInfo *Info) {
	GIObjectInfo *Parent = g_object_info_get_parent(Info);
	if (Parent) object_add_methods(Object, Parent);
	int NumInterfaces = g_object_info_get_n_interfaces(Info);
	for (int I = 0; I < NumInterfaces; ++I) {
		interface_add_methods(Object, g_object_info_get_interface(Info, I));
	}
	int NumMethods = g_object_info_get_n_methods(Info);
	for (int I = 0; I < NumMethods; ++I) {
		GIFunctionInfo *MethodInfo = g_object_info_get_method(Info, I);
		const char *MethodName = g_base_info_get_name((GIBaseInfo *)MethodInfo);
		GIFunctionInfoFlags Flags = g_function_info_get_flags(MethodInfo);
		if (Flags & GI_FUNCTION_IS_METHOD) {
			ml_method_by_name(MethodName, MethodInfo, (ml_callback_t)method_invoke, Object, NULL);
		} else if (Flags & GI_FUNCTION_IS_CONSTRUCTOR) {
			stringmap_insert(Object->Base.Exports, MethodName, ml_cfunction(MethodInfo, (void *)constructor_invoke));
		}
	}
}

static stringmap_t TypeMap[1] = {STRINGMAP_INIT};

static ml_value_t *object_instance_new(object_t *Object, int Count, ml_value_t **Args) {
	object_instance_t *Instance = new(object_instance_t);
	Instance->Type = Object;
	GType Type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)Object->Info);
	if (Count > 0) {
		ML_CHECK_ARG_TYPE(0, MLNamesT);
		int NumProperties = Count - 1;
		const char *Names[NumProperties];
		GValue Values[NumProperties];
		memset(Values, 0, NumProperties * sizeof(GValue));
		int Index = 0;
		ML_NAMES_FOREACH(Args[0], Iter) {
			Names[Index] = ml_string_value(Iter->Value);
			_ml_to_value(Args[Index + 1], Values + Index);
		}
		Instance->Handle = g_object_new_with_properties(Type, NumProperties, Names, Values);
	} else {
		Instance->Handle = g_object_new_with_properties(Type, 0, NULL, NULL);
	}
	g_object_set_qdata(Instance->Handle, MLQuark, Instance);
	g_object_ref_sink(Instance->Handle);
	GC_register_finalizer(Instance, (GC_finalization_proc)instance_finalize, 0, 0, 0);
	return (ml_value_t *)Instance;
}

static ml_type_t *object_info_lookup(GIObjectInfo *Info) {
	const char *TypeName = g_base_info_get_name((GIBaseInfo *)Info);
	ml_type_t **Slot = (ml_type_t **)stringmap_slot(TypeMap, TypeName);
	if (!Slot[0]) {
		object_t *Object = new(object_t);
		Object->Base.Type = ObjectT;
		Object->Base.Name = TypeName;
		Object->Base.hash = ml_default_hash;
		Object->Base.call = ml_default_call;
		Object->Base.deref = ml_default_deref;
		Object->Base.assign = ml_default_assign;
		Object->Info = Info;
		ml_type_init((ml_type_t *)Object, ObjectInstanceT, NULL);
		Object->Base.Constructor = ml_cfunction(Object, (ml_callback_t)object_instance_new);
		object_add_methods(Object, Info);
		Slot[0] = (ml_type_t *)Object;
	}
	return Slot[0];
}

static ml_type_t *interface_info_lookup(GIInterfaceInfo *Info) {
	const char *TypeName = g_base_info_get_name((GIBaseInfo *)Info);
	ml_type_t **Slot = (ml_type_t **)stringmap_slot(TypeMap, TypeName);
	if (!Slot[0]) {
		object_t *Object = new(object_t);
		Object->Base.Type = ObjectT;
		Object->Base.Name = TypeName;
		Object->Base.hash = ml_default_hash;
		Object->Base.call = ml_default_call;
		Object->Base.deref = ml_default_deref;
		Object->Base.assign = ml_default_assign;
		Object->Info = Info;
		ml_type_init((ml_type_t *)Object, ObjectInstanceT, NULL);
		Object->Base.Constructor = ml_cfunction(Object, (ml_callback_t)object_instance_new);
		interface_add_methods(Object, Info);
		Slot[0] = (ml_type_t *)Object;
	}
	return Slot[0];
}

static ml_type_t *struct_info_lookup(GIStructInfo *Info) {
	const char *TypeName = g_base_info_get_name((GIBaseInfo *)Info);
	ml_type_t **Slot = (ml_type_t **)stringmap_slot(TypeMap, TypeName);
	if (!Slot[0]) {
		struct_t *Struct = new(struct_t);
		Struct->Base.Type = StructT;
		Struct->Base.Name = TypeName;
		Struct->Base.hash = ml_default_hash;
		Struct->Base.call = ml_default_call;
		Struct->Base.deref = ml_default_deref;
		Struct->Base.assign = ml_default_assign;
		Struct->Info = Info;
		ml_type_init((ml_type_t *)Struct, StructInstanceT, NULL);
		Struct->Base.Constructor = ml_cfunction(Struct, (void *)struct_instance_new);
		Slot[0] = (ml_type_t *)Struct;
		int NumFields = g_struct_info_get_n_fields(Info);
		for (int I = 0; I < NumFields; ++I) {
			GIFieldInfo *FieldInfo = g_struct_info_get_field(Info, I);
			const char *FieldName = g_base_info_get_name((GIBaseInfo *)FieldInfo);
			ml_method_by_name(FieldName, FieldInfo, (ml_callback_t)struct_field_ref, Struct, NULL);
		}
		int NumMethods = g_struct_info_get_n_methods(Info);
		for (int I = 0; I < NumMethods; ++I) {
			GIFunctionInfo *MethodInfo = g_struct_info_get_method(Info, I);
			const char *MethodName = g_base_info_get_name((GIBaseInfo *)MethodInfo);
			GIFunctionInfoFlags Flags = g_function_info_get_flags(MethodInfo);
			if (Flags & GI_FUNCTION_IS_METHOD) {
				ml_method_by_name(MethodName, MethodInfo, (ml_callback_t)method_invoke, Struct, NULL);
			} else if (Flags & GI_FUNCTION_IS_CONSTRUCTOR) {
				stringmap_insert(Struct->Base.Exports, MethodName, ml_cfunction(MethodInfo, (void *)constructor_invoke));
			}
		}
	}
	return Slot[0];
}

/*
static void enum_iterate(ml_state_t *Caller, enum_t *Enum) {
	return ml_iterate(Caller, Enum->ByIndex);
}
*/

static ml_type_t *enum_info_lookup(GIEnumInfo *Info) {
	const char *TypeName = g_base_info_get_name((GIBaseInfo *)Info);
	ml_type_t **Slot = (ml_type_t **)stringmap_slot(TypeMap, TypeName);
	if (!Slot[0]) {
		int NumValues = g_enum_info_get_n_values(Info);
		enum_t *Enum = xnew(enum_t, NumValues, ml_value_t *);
		Enum->Base.Type = EnumT;
		Enum->Base.Name = TypeName;
		Enum->Base.hash = ml_default_hash;
		Enum->Base.call = ml_default_call;
		Enum->Base.deref = ml_default_deref;
		Enum->Base.assign = ml_default_assign;
		Enum->Base.Rank = EnumT->Rank + 1;
		ml_type_init((ml_type_t *)Enum, EnumValueT, NULL);
		for (int I = 0; I < NumValues; ++I) {
			GIValueInfo *ValueInfo = g_enum_info_get_value(Info, I);
			const char *ValueName = GC_strdup(g_base_info_get_name((GIBaseInfo *)ValueInfo));
			enum_value_t *Value = new(enum_value_t);
			Value->Type = Enum;
			Value->Name = ml_cstring(ValueName);
			Value->Value = g_value_info_get_value(ValueInfo);
			stringmap_insert(Enum->Base.Exports, ValueName, (ml_value_t *)Value);
			Enum->ByIndex[I] = (ml_value_t *)Value;
		}
		Enum->Info = Info;
		Slot[0] = (ml_type_t *)Enum;
	}
	return Slot[0];
}

static ml_value_t *constant_info_lookup(GIConstantInfo *Info) {
	const char *TypeName = g_base_info_get_name((GIBaseInfo *)Info);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(TypeMap, TypeName);
	if (!Slot[0]) {
		GIArgument Argument[1];
		g_constant_info_get_value(Info, Argument);
		ml_value_t *Value = argument_to_value(Argument, g_constant_info_get_type(Info));
		Slot[0] = Value;
	}
	return Slot[0];
}

static ml_value_t *baseinfo_to_value(GIBaseInfo *Info) {
	switch (g_base_info_get_type(Info)) {
	case GI_INFO_TYPE_INVALID:
	case GI_INFO_TYPE_INVALID_0: {
		break;
	}
	case GI_INFO_TYPE_FUNCTION: {
		return ml_cfunction(Info, (ml_callback_t)function_info_invoke);
	}
	case GI_INFO_TYPE_CALLBACK: {
		break;
	}
	case GI_INFO_TYPE_STRUCT: {
		return (ml_value_t *)struct_info_lookup((GIStructInfo *)Info);
	}
	case GI_INFO_TYPE_BOXED: {
		break;
	}
	case GI_INFO_TYPE_ENUM: {
		return (ml_value_t *)enum_info_lookup((GIEnumInfo *)Info);
	}
	case GI_INFO_TYPE_FLAGS: {
		return (ml_value_t *)enum_info_lookup((GIEnumInfo *)Info);
	}
	case GI_INFO_TYPE_OBJECT: {
		return (ml_value_t *)object_info_lookup((GIObjectInfo *)Info);
	}
	case GI_INFO_TYPE_INTERFACE: {
		break;
	}
	case GI_INFO_TYPE_CONSTANT: {
		return constant_info_lookup((GIConstantInfo *)Info);
	}
	case GI_INFO_TYPE_UNION: {
		break;
	}
	case GI_INFO_TYPE_VALUE: {
		break;
	}
	case GI_INFO_TYPE_SIGNAL: {
		break;
	}
	case GI_INFO_TYPE_VFUNC: {
		break;
	}
	case GI_INFO_TYPE_PROPERTY: {
		break;
	}
	case GI_INFO_TYPE_FIELD: {
		break;
	}
	case GI_INFO_TYPE_ARG: {
		break;
	}
	case GI_INFO_TYPE_TYPE: {
		break;
	}
	case GI_INFO_TYPE_UNRESOLVED: {
		break;
	}
	}
	printf("Unsupported info type: %s\n", g_info_type_to_string(g_base_info_get_type(Info)));
	return MLNil;
}

ML_METHOD("::", TypelibT, MLStringT) {
//<Typelib
//<Name
//>any | error
	typelib_t *Typelib = (typelib_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	GIBaseInfo *Info = g_irepository_find_by_name(NULL, Typelib->Namespace, Name);
	if (!Info) {
		return ml_error("NameError", "Symbol %s not found in %s", Name, Typelib->Namespace);
	} else {
		return baseinfo_to_value(Info);
	}
}

static void typelib_iterate(ml_state_t *Caller, typelib_t *Typelib) {
	typelib_iter_t *Iter = new(typelib_iter_t);
	Iter->Type = TypelibIterT;
	Iter->Namespace = Typelib->Namespace;
	Iter->Handle = Typelib->Handle;
	Iter->Index = 0;
	Iter->Total = g_irepository_get_n_infos(NULL, Iter->Namespace);
	Iter->Current = g_irepository_get_info(NULL, Iter->Namespace, 0);
	ML_CONTINUE(Caller, Iter);
}

static ml_value_t *_value_to_ml(const GValue *Value, GIBaseInfo *Info) {
	switch (G_VALUE_TYPE(Value)) {
	case G_TYPE_NONE: return MLNil;
	case G_TYPE_CHAR: return ml_integer(g_value_get_schar(Value));
	case G_TYPE_UCHAR: return ml_integer(g_value_get_uchar(Value));
	case G_TYPE_BOOLEAN: return ml_boolean(g_value_get_boolean(Value));
	case G_TYPE_INT: return ml_integer(g_value_get_int(Value));
	case G_TYPE_UINT: return ml_integer(g_value_get_uint(Value));
	case G_TYPE_LONG: return ml_integer(g_value_get_long(Value));
	case G_TYPE_ULONG: return ml_integer(g_value_get_ulong(Value));
	case G_TYPE_ENUM: {
		GType Type = G_VALUE_TYPE(Value);
		GIBaseInfo *Info = g_irepository_find_by_gtype(NULL, Type);
		enum_t *Enum = (enum_t *)enum_info_lookup((GIEnumInfo *)Info);
		return Enum->ByIndex[g_value_get_enum(Value)];
	}
	case G_TYPE_FLAGS: return ml_integer(g_value_get_flags(Value));
	case G_TYPE_FLOAT: return ml_real(g_value_get_float(Value));
	case G_TYPE_DOUBLE: return ml_real(g_value_get_double(Value));
	case G_TYPE_STRING: return ml_string(g_value_get_string(Value), -1);
	case G_TYPE_POINTER: return MLNil; //Std$Address$new(g_value_get_pointer(Value));
	default: {
		if (G_VALUE_HOLDS(Value, G_TYPE_OBJECT)) {
			return ml_gir_instance_get(g_value_get_object(Value), Info);
		} else {
			printf("Warning: Unknown parameter type: %s\n", G_VALUE_TYPE_NAME(Value));
			return MLNil;
		}
	}
	}
}

static void _ml_to_value(ml_value_t *Source, GValue *Dest) {
	if (Source == MLNil) {
		g_value_init(Dest, G_TYPE_NONE);
	} else if (ml_is(Source, MLBooleanT)) {
		g_value_init(Dest, G_TYPE_BOOLEAN);
		g_value_set_boolean(Dest, ml_boolean_value(Source));
	} else if (ml_is(Source, MLIntegerT)) {
		g_value_init(Dest, G_TYPE_LONG);
		g_value_set_long(Dest, ml_integer_value(Source));
	} else if (ml_is(Source, MLRealT)) {
		g_value_init(Dest, G_TYPE_DOUBLE);
		g_value_set_double(Dest, ml_real_value(Source));
	} else if (ml_is(Source, MLStringT)) {
		g_value_init(Dest, G_TYPE_STRING);
		g_value_set_string(Dest, ml_string_value(Source));
	} else if (ml_is(Source, ObjectInstanceT)) {
		void *Object = ((object_instance_t *)Source)->Handle;
		g_value_init(Dest, G_OBJECT_TYPE(Object));
		g_value_set_object(Dest, Object);
	} else if (ml_is(Source, StructInstanceT)) {
		void *Value = ((struct_instance_t *)Source)->Value;
		g_value_init(Dest, G_TYPE_POINTER);
		g_value_set_object(Dest, Value);
	} else if (ml_is(Source, EnumValueT)) {
		enum_t *Enum = (enum_t *)((enum_value_t *)Source)->Type;
		GType Type = g_type_from_name(g_base_info_get_name((GIBaseInfo *)Enum->Info));
		g_value_init(Dest, Type);
		g_value_set_enum(Dest, ((enum_value_t *)Source)->Value);
	} else {
		g_value_init(Dest, G_TYPE_NONE);
	}
}

static void __marshal(GClosure *Closure, GValue *Result, guint NumArgs, const GValue *Args, gpointer Hint, ml_value_t *Function) {
	GICallableInfo *SignalInfo = (GICallableInfo *)Closure->data;
	ml_value_t *MLArgs[NumArgs];
	MLArgs[0] = _value_to_ml(Args, NULL);
	for (guint I = 1; I < NumArgs; ++I) {
		GIArgInfo *ArgInfo = g_callable_info_get_arg(SignalInfo, I - 1);
		GITypeInfo TypeInfo[1];
		g_arg_info_load_type(ArgInfo, TypeInfo);
		MLArgs[I] = _value_to_ml(Args + I, g_type_info_get_interface(TypeInfo));
	}
	ml_value_t *MLResult = ml_simple_call(Function, NumArgs, MLArgs);
	if (Result) _ml_to_value(MLResult, Result);
}

ML_METHOD("connect", ObjectInstanceT, MLStringT, MLFunctionT) {
//<Object
//<Signal
//<Handler
//>Object
	object_instance_t *Instance = (object_instance_t *)Args[0];
	const char *Signal = ml_string_value(Args[1]);
	GISignalInfo *SignalInfo = NULL;
	if (GI_IS_OBJECT_INFO(Instance->Type->Info)) {
		SignalInfo = g_object_info_find_signal(Instance->Type->Info, Signal);
	} else if (GI_IS_INTERFACE_INFO(Instance->Type->Info)) {
		SignalInfo = g_interface_info_find_signal(Instance->Type->Info, Signal);
	}
	if (!SignalInfo) return ml_error("NameError", "Signal %s not found", Signal);
	GClosure *Closure = g_closure_new_simple(sizeof(GClosure), SignalInfo);
	g_closure_set_meta_marshal(Closure, Args[2], (GClosureMarshal)__marshal);
	g_signal_connect_closure(Instance->Handle, Signal, Closure, Count > 3 && Args[3] != MLNil);
	return Args[0];
}

typedef struct {
	ml_type_t *Type;
	GObject *Object;
	const char *Name;
} object_property_t;

static ml_value_t *object_property_deref(object_property_t *Property) {
	GValue Value[1] = {G_VALUE_INIT};
	g_object_get_property(Property->Object, Property->Name, Value);
	return _value_to_ml(Value, NULL);
}

static ml_value_t *object_property_assign(object_property_t *Property, ml_value_t *Value0) {
	GValue Value[1];
	memset(Value, 0, sizeof(GValue));
	_ml_to_value(Value0, Value);
	g_object_set_property(Property->Object, Property->Name, Value);
	return Value0;
}

ML_TYPE(ObjectPropertyT, (), "gir-object-property",
	.deref = (void *)object_property_deref,
	.assign = (void *)object_property_assign
);

ML_METHOD("::", ObjectInstanceT, MLStringT) {
//<Object
//<Property
//>any
	object_instance_t *Instance = (object_instance_t *)Args[0];
	object_property_t *Property = new(object_property_t);
	Property->Type = ObjectPropertyT;
	Property->Object = Instance->Handle;
	Property->Name = ml_string_value(Args[1]);
	return (ml_value_t *)Property;
}

void ml_gir_init(stringmap_t *Globals) {
	GError *Error = 0;
	g_irepository_require(NULL, "GLib", NULL, 0, &Error);
	g_irepository_require(NULL, "GObject", NULL, 0, &Error);
	DestroyNotifyInfo = g_irepository_find_by_name(NULL, "GLib", "DestroyNotify");
	GValueInfo = g_irepository_find_by_name(NULL, "GObject", "Value");
	ml_typed_fn_set(TypelibT, ml_iterate, typelib_iterate);
	ml_typed_fn_set(TypelibIterT, ml_iter_next, typelib_iter_next);
	ml_typed_fn_set(TypelibIterT, ml_iter_value, typelib_iter_value);
	ml_typed_fn_set(TypelibIterT, ml_iter_key, typelib_iter_key);
	MLQuark = g_quark_from_static_string("<<minilang>>");
	ObjectInstanceNil = new(object_instance_t);
	ObjectInstanceNil->Type = (object_t *)ObjectInstanceT;
	//ml_typed_fn_set(EnumT, ml_iterate, enum_iterate);
	stringmap_insert(Globals, "gir", ml_cfunction(NULL, ml_gir_require));
	ObjectT->call = MLTypeT->call;
	StructT->call = MLTypeT->call;
#include "ml_gir_init.c"
}

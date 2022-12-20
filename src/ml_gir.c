#include "ml_gir.h"
#include "ml_macros.h"
#include <girffi.h>
#include <stdio.h>

#undef ML_CATEGORY
#define ML_CATEGORY "gir"

typedef struct {
	void **Ptrs;
	int Size;
} ptrset_t;

#define PTRSET_INIT {NULL, 0}
#define PTRSET_INITIAL_SIZE 4

static void ptrset_insert(ptrset_t *Set, void *Ptr) {
	if (!Set->Size) {
		void **Ptrs = anew(void *, PTRSET_INITIAL_SIZE);
		Ptrs[PTRSET_INITIAL_SIZE - 1] = Ptr;
		Set->Ptrs = Ptrs;
		Set->Size = PTRSET_INITIAL_SIZE;
		return;
	}
	void **Slot = Set->Ptrs, **Space = NULL;
	for (int I = Set->Size; --I >= 0; ++Slot) {
		if (*Slot == Ptr) return;
		if (!*Slot) Space = Slot;
	}
	if (Space) {
		*Space = Ptr;
		return;
	}
	int Size = Set->Size + PTRSET_INITIAL_SIZE;
	void **Ptrs = anew(void *, Size);
	memcpy(Ptrs, Set->Ptrs, Set->Size * sizeof(void *));
	Ptrs[Set->Size] = Ptr;
	Set->Ptrs = Ptrs;
	Set->Size = Size;
}

static void ptrset_remove(ptrset_t *Set, void *Ptr) {
	void **Slot = Set->Ptrs;
	for (int I = Set->Size; --I >= 0; ++Slot) if (*Slot == Ptr) {
		*Slot = NULL;
		return;
	}
}

typedef struct {
	ml_type_t *Type;
	GITypelib *Handle;
	const char *Namespace;
} typelib_t;

ML_TYPE(GirTypelibT, (MLSequenceT), "typelib");
//@gir
// A gobject-introspection typelib.

GITypelib *ml_gir_get_typelib(ml_value_t *Value) {
	return ((typelib_t *)Value)->Handle;
}

const char *ml_gir_get_namespace(ml_value_t *Value) {
	return ((typelib_t *)Value)->Namespace;
}

typedef struct {
	ml_type_t *Type;
	GITypelib *Handle;
	const char *Namespace;
	GIBaseInfo *Current;
	int Index, Total;
} typelib_iter_t;

typedef struct {
	ml_type_t Type;
	GIBaseInfo *Info;
} baseinfo_t;

ML_TYPE(GirBaseInfoT, (MLTypeT), "base-info");

ML_METHOD("name", GirBaseInfoT) {
	baseinfo_t *Info = (baseinfo_t *)Args[0];
	const char *Name = g_base_info_get_name(Info->Info);
	return ml_string_copy(Name, strlen(Name));
}

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

ml_value_t *ml_gir_typelib(const char *Name, const char *Version) {
	typelib_t *Typelib = new(typelib_t);
	Typelib->Type = GirTypelibT;
	Typelib->Namespace = Name;
	GError *Error = 0;
	Typelib->Handle = g_irepository_require(NULL, Typelib->Namespace, Version, 0, &Error);
	if (!Typelib->Handle) return ml_error("GirError", "%s", Error->message);
	return (ml_value_t *)Typelib;
}

ML_METHOD(GirTypelibT, MLStringT) {
	return ml_gir_typelib(ml_string_value(Args[0]), NULL);
}

ML_METHOD(GirTypelibT, MLStringT, MLStringT) {
	return ml_gir_typelib(ml_string_value(Args[0]), ml_string_value(Args[1]));
}

#ifdef ML_LIBRARY

#include "ml_library.h"

ML_TYPE(GirModuleT, (), "gir::module");
ML_VALUE(GirModule, GirModuleT);

ML_METHOD("::", GirModuleT, MLStringT) {
	char *Name = GC_strdup(ml_string_value(Args[1]));
	char *Version = strchr(Name, '@');
	if (Version) *Version++ = 0;
	return ml_gir_typelib(Name, Version);
}

#endif

typedef struct {
	ml_type_t Base;
	GIObjectInfo *Info;
	stringmap_t Signals[1];
} object_t;

typedef struct {
	const object_t *Type;
	void *Handle;
	ptrset_t Handlers[1];
} object_instance_t;

ML_TYPE(GirObjectT, (GirBaseInfoT), "object-type");
// A gobject-introspection object type.

ML_TYPE(GirObjectInstanceT, (), "object-instance");
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
static GType MLType;

ml_value_t *ml_gir_instance_get(void *Handle, GIBaseInfo *Fallback) {
	//if (Handle == 0) return (ml_value_t *)ObjectInstanceNil;
	if (Handle == 0) return MLNil;
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

ML_METHOD("append", MLStringBufferT, GirObjectInstanceT) {
//<Object
//>string
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	object_instance_t *Instance = (object_instance_t *)Args[1];
	if (Instance == ObjectInstanceNil) {
		ml_stringbuffer_write(Buffer, "nil-object", 6);
	} else {
		ml_stringbuffer_printf(Buffer, "<%s>", g_base_info_get_name((GIBaseInfo *)Instance->Type->Info));
	}
	return MLSome;
}

typedef struct {
	ml_type_t Base;
	GIStructInfo *Info;
} struct_t;

typedef struct {
	const struct_t *Type;
	void *Value;
} struct_instance_t;

ML_TYPE(GirStructT, (GirBaseInfoT), "struct-type");
// A gobject-introspection struct type.

ML_TYPE(GirStructInstanceT, (), "struct-instance");
// A gobject-introspection struct instance.

ml_value_t *ml_gir_struct_instance(ml_value_t *Struct, void *Value) {
	struct_instance_t *Instance = new(struct_instance_t);
	Instance->Type = (struct_t *)Struct;
	Instance->Value = Value;
	return (ml_value_t *)Instance;
}

void *ml_gir_struct_instance_value(ml_value_t *Value) {
	return ((struct_instance_t *)Value)->Value;
}

static ml_value_t *struct_instance(struct_t *Struct, int Count, ml_value_t **Args) {
	struct_instance_t *Instance = new(struct_instance_t);
	Instance->Type = Struct;
	Instance->Value = GC_MALLOC(g_struct_info_get_size(Struct->Info));
	return (ml_value_t *)Instance;
}

ML_METHOD("append", MLStringBufferT, GirStructInstanceT) {
//<Struct
//>string
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	struct_instance_t *Instance = (struct_instance_t *)Args[1];
	ml_stringbuffer_printf(Buffer, "<%s>", g_base_info_get_name((GIBaseInfo *)Instance->Type->Info));
	return MLSome;
}

typedef struct {
	ml_type_t Base;
	GIUnionInfo *Info;
} union_t;

typedef struct {
	const union_t *Type;
	void *Value;
} union_instance_t;

ML_TYPE(GirUnionT, (GirBaseInfoT), "union-type");
// A gobject-introspection struct type.

ML_TYPE(GirUnionInstanceT, (), "union-instance");
// A gobject-introspection struct instance.

ml_value_t *ml_gir_union_instance(ml_value_t *Union, void *Value) {
	union_instance_t *Instance = new(union_instance_t);
	Instance->Type = (union_t *)Union;
	Instance->Value = Value;
	return (ml_value_t *)Instance;
}

void *ml_gir_union_instance_value(ml_value_t *Value) {
	return ((union_instance_t *)Value)->Value;
}

static ml_value_t *union_instance(union_t *Union, int Count, ml_value_t **Args) {
	union_instance_t *Instance = new(union_instance_t);
	Instance->Type = Union;
	Instance->Value = GC_MALLOC(g_union_info_get_size(Union->Info));
	return (ml_value_t *)Instance;
}

ML_METHOD("append", MLStringBufferT, GirUnionInstanceT) {
//<Union
//>string
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	union_instance_t *Instance = (union_instance_t *)Args[1];
	ml_stringbuffer_printf(Buffer, "<%s>", g_base_info_get_name((GIBaseInfo *)Instance->Type->Info));
	return MLSome;
}

typedef struct field_ref_t {
	ml_type_t *Type;
	void *Address;
} field_ref_t;

ML_TYPE(GirFieldRefT, (), "field-ref");
//@gir::fieldref

#define FIELD_REF(UNAME, LNAME, GTYPE, GETTER, SETTER) \
static ml_value_t *field_ref_ ## LNAME ## _deref(field_ref_t *Ref) { \
	GTYPE Value = *(GTYPE *)Ref->Address; \
	return GETTER; \
} \
\
static void field_ref_ ## LNAME ## _assign(ml_state_t *Caller, field_ref_t *Ref, ml_value_t *Value) { \
	GTYPE *Address = (GTYPE *)Ref->Address; \
	*Address = SETTER; \
	ML_RETURN(Value); \
} \
\
ML_TYPE(GirFieldRef ## UNAME ## T, (GirFieldRefT), "field-ref-" #LNAME, \
/*@gir::fieldref-LNAME
*/ \
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
	case GI_TYPE_TAG_BOOLEAN: Ref->Type = GirFieldRefBooleanT; break;
	case GI_TYPE_TAG_INT8: Ref->Type = GirFieldRefInt8T; break;
	case GI_TYPE_TAG_UINT8: Ref->Type = GirFieldRefUInt8T; break;
	case GI_TYPE_TAG_INT16: Ref->Type = GirFieldRefInt16T; break;
	case GI_TYPE_TAG_UINT16: Ref->Type = GirFieldRefUInt16T; break;
	case GI_TYPE_TAG_INT32: Ref->Type = GirFieldRefInt32T; break;
	case GI_TYPE_TAG_UINT32: Ref->Type = GirFieldRefUInt32T; break;
	case GI_TYPE_TAG_INT64: Ref->Type = GirFieldRefInt64T; break;
	case GI_TYPE_TAG_UINT64: Ref->Type = GirFieldRefUInt64T; break;
	case GI_TYPE_TAG_FLOAT: Ref->Type = GirFieldRefFloatT; break;
	case GI_TYPE_TAG_DOUBLE: Ref->Type = GirFieldRefDoubleT; break;
	case GI_TYPE_TAG_GTYPE: return ml_error("TodoError", "Field ref not implemented yet");
	case GI_TYPE_TAG_UTF8: Ref->Type = GirFieldRefUtf8T; break;
	case GI_TYPE_TAG_FILENAME: Ref->Type = GirFieldRefUtf8T; break;
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

static ml_value_t *union_field_ref(GIFieldInfo *Info, int Count, ml_value_t **Args) {
	union_instance_t *Instance = (union_instance_t *)Args[0];
	field_ref_t *Ref = new(field_ref_t);
	Ref->Address = (char *)Instance->Value + g_field_info_get_offset(Info);

	GITypeInfo *TypeInfo = g_field_info_get_type(Info);
	switch (g_type_info_get_tag(TypeInfo)) {
	case GI_TYPE_TAG_VOID: return ml_error("TodoError", "Field ref not implemented yet");
	case GI_TYPE_TAG_BOOLEAN: Ref->Type = GirFieldRefBooleanT; break;
	case GI_TYPE_TAG_INT8: Ref->Type = GirFieldRefInt8T; break;
	case GI_TYPE_TAG_UINT8: Ref->Type = GirFieldRefUInt8T; break;
	case GI_TYPE_TAG_INT16: Ref->Type = GirFieldRefInt16T; break;
	case GI_TYPE_TAG_UINT16: Ref->Type = GirFieldRefUInt16T; break;
	case GI_TYPE_TAG_INT32: Ref->Type = GirFieldRefInt32T; break;
	case GI_TYPE_TAG_UINT32: Ref->Type = GirFieldRefUInt32T; break;
	case GI_TYPE_TAG_INT64: Ref->Type = GirFieldRefInt64T; break;
	case GI_TYPE_TAG_UINT64: Ref->Type = GirFieldRefUInt64T; break;
	case GI_TYPE_TAG_FLOAT: Ref->Type = GirFieldRefFloatT; break;
	case GI_TYPE_TAG_DOUBLE: Ref->Type = GirFieldRefDoubleT; break;
	case GI_TYPE_TAG_GTYPE: return ml_error("TodoError", "Field ref not implemented yet");
	case GI_TYPE_TAG_UTF8: Ref->Type = GirFieldRefUtf8T; break;
	case GI_TYPE_TAG_FILENAME: Ref->Type = GirFieldRefUtf8T; break;
	case GI_TYPE_TAG_ARRAY: return ml_error("TodoError", "Field ref not implemented yet");
	case GI_TYPE_TAG_INTERFACE: {
		GIBaseInfo *InterfaceInfo = g_type_info_get_interface(TypeInfo);
		switch (g_base_info_get_type(InterfaceInfo)) {
		case GI_INFO_TYPE_INVALID:
		case GI_INFO_TYPE_INVALID_0: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_FUNCTION: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_CALLBACK: return ml_error("TodoError", "Field ref not implemented yet");
		case GI_INFO_TYPE_STRUCT: {
			struct_instance_t *Instance2 = new(struct_instance_t);
			Instance2->Type = (struct_t *)struct_info_lookup((GIStructInfo *)InterfaceInfo);
			Instance2->Value = Instance->Value;
			return (ml_value_t *)Instance2;
		}
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


typedef struct enum_t enum_t;
typedef struct enum_value_t enum_value_t;

struct enum_t {
	ml_type_t Base;
	GIEnumInfo *Info;
	enum_value_t *ByIndex[];
};

struct enum_value_t {
	const enum_t *Type;
	ml_value_t *Name;
	gint64 Value;
};

ML_TYPE(GirEnumT, (GirBaseInfoT), "enum-type");
// A gobject-instrospection enum type.

ML_TYPE(GirEnumValueT, (), "enum-value");
// A gobject-instrospection enum value.

gint64 ml_gir_enum_value_value(ml_value_t *Value) {
	return ((enum_value_t *)Value)->Value;
}

ML_METHOD("append", MLStringT, GirEnumValueT) {
//<Value
//>string
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	enum_value_t *Value = (enum_value_t *)Args[1];
	ml_stringbuffer_write(Buffer, ml_string_value(Value->Name), ml_string_length(Value->Name));
	return MLSome;
}

ML_METHOD(MLIntegerT, GirEnumValueT) {
//<Value
//>integer
	enum_value_t *Value = (enum_value_t *)Args[0];
	return ml_integer(Value->Value);
}

ML_METHOD("|", GirEnumValueT, MLNilT) {
//<Value/1
//<Value/2
//>EnumValueT
	return Args[0];
}

ML_METHOD("|", MLNilT, GirEnumValueT) {
//<Value/1
//<Value/2
//>EnumValueT
	return Args[1];
}

ML_METHOD("|", GirEnumValueT, GirEnumValueT) {
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
	char *Name = snew(Length + 1);
	memcpy(Name, ml_string_value(A->Name), LengthA);
	Name[LengthA] = '|';
	memcpy(Name + LengthA + 1, ml_string_value(B->Name), LengthB);
	Name[Length] = 0;
	C->Name = ml_string(Name, Length);
	C->Value = A->Value | B->Value;
	return (ml_value_t *)C;
}

static ml_value_t *gir_enum_value(enum_t *Type, int64_t Value) {
	for (enum_value_t **Ptr = Type->ByIndex; Value && *Ptr; ++Ptr) {
		if (Ptr[0]->Value == Value) return (ml_value_t *)Ptr[0];
	}
	enum_value_t *Enum = new(enum_value_t);
	Enum->Type = Type;
	Enum->Value = Value;
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (enum_value_t **Ptr = Type->ByIndex; Value && *Ptr; ++Ptr) {
		if ((Ptr[0]->Value & Value) == Ptr[0]->Value) {
			Value &= ~Ptr[0]->Value;
			if (Buffer->Length) ml_stringbuffer_put(Buffer, '|');
			const char *Name = ml_string_value(Ptr[0]->Name);
			size_t Length = ml_string_length(Ptr[0]->Name);
			ml_stringbuffer_write(Buffer, Name, Length);
		}
	}
	Enum->Name = ml_stringbuffer_get_value(Buffer);
	return (ml_value_t *)Enum;
}

ml_value_t *ml_gir_enum_value(ml_value_t *Enum, gint64 Value) {
	return gir_enum_value((enum_t *)Enum, Value);
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

typedef struct {
	ml_context_t *Context;
	ml_value_t *Function;
	GICallbackInfo *Info;
	ffi_cif Cif[1];
} ml_gir_callback_t;

static ml_value_t *argument_to_ml(GIArgument *Argument, GITypeInfo *TypeInfo, GICallableInfo *Info, GIArgument *ArgsOut);

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
		case GI_TYPE_TAG_INTERFACE:
		case GI_TYPE_TAG_GLIST:
		case GI_TYPE_TAG_GSLIST:
		case GI_TYPE_TAG_GHASH: {
			GIArgument Argument = {.v_pointer = *(void **)Params[I]};
			Args[I] = argument_to_ml(&Argument, TypeInfo, NULL, NULL);
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
	ml_result_state_t *State = ml_result_state(Callback->Context);
	ml_call(State, Callback->Function, Count, Args);
	GMainContext *MainContext = g_main_context_default();
	while (!State->Value) g_main_context_iteration(MainContext, TRUE);
	ml_value_t *Result = State->Value;
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
		char *Array = snew((ml_list_length(Result) + 1) * ElementSize);
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
			ml_gir_callback_t *Callback2 = (ml_gir_callback_t *)GC_MALLOC_UNCOLLECTABLE(sizeof(ml_gir_callback_t));
			Callback2->Context = Callback->Context;
			Callback2->Info = InterfaceInfo;
			Callback2->Function = Result;
			*(void **)Return = g_callable_info_prepare_closure(
				InterfaceInfo,
				Callback2->Cif,
				(GIFFIClosureCallback)callback_invoke,
				Callback2
			);
			break;
		}
		case GI_INFO_TYPE_STRUCT: {
			if (ml_is(Result, GirStructInstanceT)) {
				*(void **)Return = ((struct_instance_t *)Result)->Value;
			} else {
				*(void **)Return = 0;
			}
			break;
		}
		case GI_INFO_TYPE_BOXED: break;
		case GI_INFO_TYPE_ENUM: {
			if (ml_is(Result, GirEnumValueT)) {
				*(int *)Return = ((enum_value_t *)Result)->Value;
			} else {
				*(int *)Return = 0;
			}
			break;
		}
		case GI_INFO_TYPE_FLAGS: break;
		case GI_INFO_TYPE_OBJECT: {
			if (ml_is(Result, GirObjectInstanceT)) {
				*(void **)Return = ((object_instance_t *)Result)->Handle;
			} else {
				*(void **)Return = 0;
			}
			break;
		}
		case GI_INFO_TYPE_INTERFACE: {
			if (ml_is(Result, GirObjectInstanceT)) {
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

static size_t get_output_length(GICallableInfo *Info, int Index, GIArgument *ArgsOut) {
	for (int I = 0; I < Index; ++I) {
		GIArgInfo *ArgInfo = g_callable_info_get_arg((GICallableInfo *)Info, I);
		if (g_arg_info_get_direction(ArgInfo) != GI_DIRECTION_IN) {
			++ArgsOut;
		}
	}
	return ((GIArgument *)ArgsOut->v_pointer)->v_uint64;
}

typedef struct {
	ml_value_t *Result;
	GITypeInfo *KeyInfo;
	GITypeInfo *ValueInfo;
} hashtable_to_map_t;

static void hashtable_to_map(gpointer KeyPtr, gpointer ValuePtr, hashtable_to_map_t *Info) {
	GIArgument Argument = {.v_pointer = KeyPtr};
	ml_value_t *Key = argument_to_ml(&Argument, Info->KeyInfo, NULL, NULL);
	Argument.v_pointer = ValuePtr;
	ml_value_t *Value = argument_to_ml(&Argument, Info->ValueInfo, NULL, NULL);
	ml_map_insert(Info->Result, Key, Value);
}

static ml_value_t *argument_to_ml(GIArgument *Argument, GITypeInfo *TypeInfo, GICallableInfo *Info, GIArgument *ArgsOut) {
	switch (g_type_info_get_tag(TypeInfo)) {
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
		return ml_string(g_type_name(Argument->v_size), -1);
	}
	case GI_TYPE_TAG_UTF8:
	case GI_TYPE_TAG_FILENAME: {
		return ml_string(Argument->v_string, -1);
	}
	case GI_TYPE_TAG_ARRAY: {
		GITypeInfo *ElementInfo = g_type_info_get_param_type(TypeInfo, 0);
		switch (g_type_info_get_tag(ElementInfo)) {
		case GI_TYPE_TAG_INT8:
		case GI_TYPE_TAG_UINT8: {
			if (g_type_info_is_zero_terminated(TypeInfo)) {
				return ml_string(Argument->v_string, -1);
			} else {
				size_t Length;
				int LengthIndex = g_type_info_get_array_length(TypeInfo);
				if (LengthIndex < 0) {
					Length = g_type_info_get_array_fixed_size(TypeInfo);
				} else {
					if (!Info) return ml_error("ValueError", "Unsupported situtation");
					Length = get_output_length(Info, LengthIndex, ArgsOut);
				}
				return ml_string(Argument->v_string, Length);
			}
			break;
		}
		default: break;
		}
		break;
	}
	case GI_TYPE_TAG_INTERFACE: {
		GIBaseInfo *InterfaceInfo = g_type_info_get_interface(TypeInfo);
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
			return (ml_value_t *)Enum->ByIndex[Argument->v_int];
		}
		case GI_INFO_TYPE_FLAGS: {
			enum_t *Enum = (enum_t *)enum_info_lookup((GIEnumInfo *)InterfaceInfo);
			return gir_enum_value(Enum, Argument->v_int);
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
	case GI_TYPE_TAG_GLIST: {
		if (!Argument->v_pointer) return MLNil;
		ml_value_t *Result = ml_list();
		GITypeInfo *ElementInfo = g_type_info_get_param_type(TypeInfo, 0);
		for (GList *List = (GList *)Argument->v_pointer; List; List = List->next) {
			GIArgument Element = {.v_pointer = List->data};
			ml_list_put(Result, argument_to_ml(&Element, ElementInfo, NULL, NULL));
		}
		return Result;
	}
	case GI_TYPE_TAG_GSLIST: {
		if (!Argument->v_pointer) return MLNil;
		ml_value_t *Result = ml_list();
		GITypeInfo *ElementInfo = g_type_info_get_param_type(TypeInfo, 0);
		for (GSList *List = (GSList *)Argument->v_pointer; List; List = List->next) {
			GIArgument Element = {.v_pointer = List->data};
			ml_list_put(Result, argument_to_ml(&Element, ElementInfo, NULL, NULL));
		}
		return Result;
	}
	case GI_TYPE_TAG_GHASH: {
		if (!Argument->v_pointer) return MLNil;
		hashtable_to_map_t Info = {
			ml_map(),
			g_type_info_get_param_type(TypeInfo, 0),
			g_type_info_get_param_type(TypeInfo, 1)
		};
		g_hash_table_foreach((GHashTable *)Argument->v_pointer, (GHFunc)hashtable_to_map, &Info);
		return Info.Result;
	}
	case GI_TYPE_TAG_ERROR: {
		GError *Error = Argument->v_pointer;
		return ml_error("GError", "%s", Error->message);
	}
	case GI_TYPE_TAG_UNICHAR: {
		return ml_integer(Argument->v_uint32);
	}
	}
	return ml_error("ValueError", "Unsupported situtation: %s", g_base_info_get_name((GIBaseInfo *)TypeInfo));
}

static void *list_to_array(ml_value_t *List, GITypeInfo *TypeInfo) {
	size_t ElementSize = array_element_size(TypeInfo);
	size_t Length = ml_list_length(List);
	char *Array = snew((Length + 1) * ElementSize);
	memset(Array, 0, (Length + 1) * ElementSize);
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
			if (ml_is(Iter->Value, MLDoubleT)) {
				*Ptr++ = ml_real_value(Iter->Value);
			}
		}
		break;
	}
	case GI_TYPE_TAG_DOUBLE: {
		gdouble *Ptr = (gdouble *)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, MLDoubleT)) {
				*Ptr++ = ml_real_value(Iter->Value);
			}
		}
		break;
	}
	case GI_TYPE_TAG_GTYPE: {
		GType *Ptr = (GType *)Array;
		ML_LIST_FOREACH(List, Iter) {
			if (ml_is(Iter->Value, GirBaseInfoT)) {
				baseinfo_t *Base = (baseinfo_t *)Iter->Value;
				*Ptr++ = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)Base->Info);
			} else if (ml_is(Iter->Value, MLStringT)) {
				*Ptr++ = g_type_from_name(ml_string_value(Iter->Value));
			} else if (Iter->Value == (ml_value_t *)MLNilT) {
				*Ptr++ = G_TYPE_NONE;
			} else if (Iter->Value == (ml_value_t *)MLIntegerT) {
				*Ptr++ = G_TYPE_INT64;
			} else if (Iter->Value == (ml_value_t *)MLStringT) {
				*Ptr++ = G_TYPE_STRING;
			} else if (Iter->Value == (ml_value_t *)MLDoubleT) {
				*Ptr++ = G_TYPE_DOUBLE;
			} else if (Iter->Value == (ml_value_t *)MLBooleanT) {
				*Ptr++ = G_TYPE_BOOLEAN;
			} else if (Iter->Value == (ml_value_t *)MLAddressT) {
				*Ptr++ = G_TYPE_POINTER;
			}
		}
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

static GSList *list_to_slist(ml_context_t *Context, ml_value_t *List, GITypeInfo *TypeInfo) {
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
			if (ml_is(Iter->Value, GirBaseInfoT)) {
				baseinfo_t *Base = (baseinfo_t *)Iter->Value;
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = GINT_TO_POINTER(g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)Base->Info));
				Slot = &Node->next;
			} else if (ml_is(Iter->Value, MLStringT)) {
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = GINT_TO_POINTER(g_type_from_name(ml_string_value(Iter->Value)));
				Slot = &Node->next;
			} else if (Iter->Value == (ml_value_t *)MLNilT) {
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = GINT_TO_POINTER(G_TYPE_NONE);
				Slot = &Node->next;
			} else if (Iter->Value == (ml_value_t *)MLIntegerT) {
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = GINT_TO_POINTER(G_TYPE_INT64);
				Slot = &Node->next;
			} else if (Iter->Value == (ml_value_t *)MLStringT) {
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = GINT_TO_POINTER(G_TYPE_STRING);
				Slot = &Node->next;
			} else if (Iter->Value == (ml_value_t *)MLDoubleT) {
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = GINT_TO_POINTER(G_TYPE_DOUBLE);
				Slot = &Node->next;
			} else if (Iter->Value == (ml_value_t *)MLBooleanT) {
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = GINT_TO_POINTER(G_TYPE_BOOLEAN);
				Slot = &Node->next;
			} else if (Iter->Value == (ml_value_t *)MLAddressT) {
				GSList *Node = Slot[0] = g_slist_alloc();
				Node->data = GINT_TO_POINTER(G_TYPE_POINTER);
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
				ml_gir_callback_t *Callback = (ml_gir_callback_t *)GC_MALLOC_UNCOLLECTABLE(sizeof(ml_gir_callback_t));
				Callback->Context = Context;
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
				} else if (ml_is(Iter->Value, GirStructInstanceT)) {
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
				} else if (ml_is(Iter->Value, GirEnumValueT)) {
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
				} else if (ml_is(Iter->Value, GirObjectInstanceT)) {
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

static GIBaseInfo *DestroyNotifyInfo;

static void function_info_invoke(ml_state_t *Caller, GIFunctionInfo *Info, int Count, ml_value_t **Args) {
	int NArgs = g_callable_info_get_n_args((GICallableInfo *)Info);
	int NArgsIn = 1, NArgsOut = 0;
	for (int I = 0; I < NArgs; ++I) {
		GIArgInfo *ArgInfo = g_callable_info_get_arg((GICallableInfo *)Info, I);
		switch (g_arg_info_get_direction(ArgInfo)) {
		case GI_DIRECTION_IN: ++NArgsIn; break;
		case GI_DIRECTION_OUT: ++NArgsOut; break;
		case GI_DIRECTION_INOUT: ++NArgsIn; ++NArgsOut; break;
		}
	}
	//const char *Name = g_base_info_get_name((GIBaseInfo *)Info);
	//printf("Calling %s(In: %d, Out: %d)\n", Name, NArgsIn, NArgsOut);
	//GIFunctionInfoFlags Flags = g_function_info_get_flags(Info);
	GIArgument ArgsIn[NArgsIn];
	GIArgument ArgsOut[NArgsOut];
	GIArgument ResultsOut[NArgsOut];
	GValue GValues[NArgs];
	//for (int I = 0; I <= NArgsIn; ++I) ArgsIn[I].v_pointer = NULL;
	//for (int I = 0; I < NArgsOut; ++I) ArgsIn[I].v_pointer = NULL;
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
		GIDirection Direction = g_arg_info_get_direction(ArgInfo);
		if (Direction == GI_DIRECTION_IN || Direction == GI_DIRECTION_INOUT) {
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
				ML_ERROR("InvokeError", "Not enough arguments");
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
				if (ml_is(Arg, GirBaseInfoT)) {
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
				} else if (Arg == (ml_value_t *)MLDoubleT) {
					ArgsIn[IndexIn].v_size = G_TYPE_DOUBLE;
				} else if (Arg == (ml_value_t *)MLBooleanT) {
					ArgsIn[IndexIn].v_size = G_TYPE_BOOLEAN;
				} else if (Arg == (ml_value_t *)MLAddressT) {
					ArgsIn[IndexIn].v_size = G_TYPE_POINTER;
				} else {
					ML_ERROR("TypeError", "Expected type for parameter %d", I);
				}
				break;
			}
			case GI_TYPE_TAG_UTF8:
			case GI_TYPE_TAG_FILENAME: {
				if (Arg == MLNil) {
					ArgsIn[IndexIn].v_string = NULL;
				} else {
					ML_CHECKX_ARG_TYPE(N - 1, MLStringT);
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
					if (Arg == MLNil) {
						ArgsIn[IndexIn].v_pointer = 0;
						if (LengthIndex >= 0) {
							set_input_length(Info, LengthIndex, ArgsIn, 0);
						}
					} else if (ml_is(Arg, MLAddressT)) {
						ArgsIn[IndexIn].v_pointer = (void *)ml_address_value(Arg);
						if (LengthIndex >= 0) {
							set_input_length(Info, LengthIndex, ArgsIn, ml_address_length(Arg));
						}
					} else if (ml_is(Arg, MLListT)) {
						ArgsIn[IndexIn].v_pointer = list_to_array(Arg, ElementInfo);
						if (LengthIndex >= 0) {
							set_input_length(Info, LengthIndex, ArgsIn, ml_list_length(Arg));
						}
					} else {
						ML_ERROR("TypeError", "Expected list for parameter %d", I);
					}
					break;
				}
				default: {
					if (Arg == MLNil) {
						ArgsIn[IndexIn].v_pointer = 0;
						if (LengthIndex >= 0) {
							set_input_length(Info, LengthIndex, ArgsIn, 0);
						}
					} else if (ml_is(Arg, MLListT)) {
						ArgsIn[IndexIn].v_pointer = list_to_array(Arg, ElementInfo);
						if (LengthIndex >= 0) {
							set_input_length(Info, LengthIndex, ArgsIn, ml_list_length(Arg));
						}
					} else {
						ML_ERROR("TypeError", "Expected list for parameter %d", I);
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
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_FUNCTION: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_CALLBACK: {
					ml_gir_callback_t *Callback = (ml_gir_callback_t *)GC_MALLOC_UNCOLLECTABLE(sizeof(ml_gir_callback_t));
					Callback->Context = Caller->Context;
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
					} else if (ml_is(Arg, GirStructInstanceT)) {
						ArgsIn[IndexIn].v_pointer = ((struct_instance_t *)Arg)->Value;
					} else {
						ML_ERROR("TypeError", "Expected gir struct not %s for parameter %d", ml_typeof(Args[I])->Name, I);
					}
					break;
				}
				case GI_INFO_TYPE_BOXED: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_ENUM:
				case GI_INFO_TYPE_FLAGS: {
					if (Arg == MLNil) {
						ArgsIn[IndexIn].v_int64 = 0;
					} else if (ml_is(Arg, GirEnumValueT)) {
						ArgsIn[IndexIn].v_int64 = ((enum_value_t *)Arg)->Value;
					} else {
						ML_ERROR("TypeError", "Expected gir enum not %s for parameter %d", ml_typeof(Args[I])->Name, I);
					}
					break;
				}
				case GI_INFO_TYPE_OBJECT:
				case GI_INFO_TYPE_INTERFACE: {
					if (Arg == MLNil) {
						ArgsIn[IndexIn].v_pointer = NULL;
					} else if (ml_is(Arg, GirObjectInstanceT)) {
						ArgsIn[IndexIn].v_pointer = ((object_instance_t *)Arg)->Handle;
					} else {
						ML_ERROR("TypeError", "Expected gir object not %s for parameter %d", ml_typeof(Args[I])->Name, I);
					}
					break;
				}
				case GI_INFO_TYPE_CONSTANT: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_UNION: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_VALUE: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_SIGNAL: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_VFUNC: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_PROPERTY: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_FIELD: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_ARG: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_TYPE: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_UNRESOLVED: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				}
				break;
			}
			case GI_TYPE_TAG_GLIST: {
				break;
			}
			case GI_TYPE_TAG_GSLIST: {
				GITypeInfo *ElementInfo = g_type_info_get_param_type(TypeInfo, 0);
				ArgsIn[IndexIn].v_pointer = list_to_slist(Caller->Context, Arg, ElementInfo);
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
		}
		if (Direction == GI_DIRECTION_OUT || Direction == GI_DIRECTION_INOUT) {
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
			case GI_TYPE_TAG_FILENAME: {
				ArgsOut[IndexOut].v_pointer = &ResultsOut[IndexResult++];
				break;
			}
			case GI_TYPE_TAG_ARRAY: {
				if (g_arg_info_is_caller_allocates(ArgInfo)) {
					if (N >= Count) ML_ERROR("InvokeError", "Not enough arguments");
					ml_value_t *Arg = Args[N++];
					GITypeInfo *ElementInfo = g_type_info_get_param_type(TypeInfo, 0);
					int LengthIndex = g_type_info_get_array_length(TypeInfo);
					switch (g_type_info_get_tag(ElementInfo)) {
					case GI_TYPE_TAG_INT8:
					case GI_TYPE_TAG_UINT8: {
						if (Arg == MLNil) {
							ArgsOut[IndexOut].v_pointer = 0;
							if (LengthIndex >= 0) {
								set_input_length(Info, LengthIndex, ArgsIn, 0);
							}
						} else if (ml_is(Arg, MLBufferT)) {
							ArgsOut[IndexOut].v_pointer = (char *)ml_address_value(Arg);
							if (LengthIndex >= 0) {
								set_input_length(Info, LengthIndex, ArgsIn, ml_address_length(Arg));
							}
						} else {
							ML_ERROR("TypeError", "Expected buffer for parameter %d", I);
						}
						break;
					}
					default: {
						ML_ERROR("NotImplemented", "Not able to marshal out-arrays yet at %d", __LINE__);
					}
					}
				} else {
					ArgsOut[IndexOut].v_pointer = &ResultsOut[IndexResult++];
				}
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
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_FUNCTION: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_CALLBACK: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_STRUCT: {
					if (g_arg_info_is_caller_allocates(ArgInfo)) {
						if (N >= Count) ML_ERROR("InvokeError", "Not enough arguments");
						ml_value_t *Arg = Args[N++];
						if (ml_is(Arg, GirStructInstanceT)) {
							ArgsOut[IndexOut].v_pointer = ((struct_instance_t *)Arg)->Value;
						} else {
							ML_ERROR("TypeError", "Expected gir struct not %s for parameter %d", ml_typeof(Args[I])->Name, I);
						}
					} else {
						ResultsOut[IndexResult].v_pointer = NULL;
						ArgsOut[IndexOut].v_pointer = &ResultsOut[IndexResult++];
					}
					break;
				}
				case GI_INFO_TYPE_BOXED: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_ENUM: {
					break;
				}
				case GI_INFO_TYPE_FLAGS: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_OBJECT:
				case GI_INFO_TYPE_INTERFACE: {
					ResultsOut[IndexResult].v_pointer = NULL;
					ArgsOut[IndexOut].v_pointer = &ResultsOut[IndexResult++];
					break;
				}
				case GI_INFO_TYPE_CONSTANT: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_UNION: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_VALUE: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_SIGNAL: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_VFUNC: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_PROPERTY: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_FIELD: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_ARG: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_TYPE: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
				}
				case GI_INFO_TYPE_UNRESOLVED: {
					ML_ERROR("NotImplemented", "Not able to marshal %s yet at %d", g_base_info_get_name(InterfaceInfo), __LINE__);
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
		}
	}
	GError *Error = 0;
	GIArgument ReturnValue[1];
	gboolean Invoked = g_function_info_invoke(Info, ArgsIn, IndexIn, ArgsOut, IndexOut, ReturnValue, &Error);
	if (!Invoked || Error) ML_ERROR("InvokeError", "Error: %s", Error->message);
	GITypeInfo *ReturnInfo = g_callable_info_get_return_type((GICallableInfo *)Info);
	if (!IndexResult) ML_RETURN(argument_to_ml(ReturnValue, ReturnInfo, (GICallableInfo *)Info, ArgsOut));
	ml_value_t *Result = ml_tuple(IndexResult + 1);
	ml_tuple_set(Result, 1, argument_to_ml(ReturnValue, ReturnInfo, (GICallableInfo *)Info, ArgsOut));
	IndexResult = 0;
	for (int I = 0; I < NArgs; ++I) {
		GIArgInfo *ArgInfo = g_callable_info_get_arg((GICallableInfo *)Info, I);
		GITypeInfo TypeInfo[1];
		g_arg_info_load_type(ArgInfo, TypeInfo);
		GIDirection Direction = g_arg_info_get_direction(ArgInfo);
		if (Direction == GI_DIRECTION_OUT || Direction == GI_DIRECTION_INOUT) {
			if (!g_arg_info_is_caller_allocates(ArgInfo)) {
				ml_tuple_set(Result, IndexResult + 2, argument_to_ml(ResultsOut + IndexResult, TypeInfo, (GICallableInfo *)Info, ArgsOut));
				++IndexResult;
			}
		}
	}
	ML_RETURN(Result);
}

static void constructor_invoke(ml_state_t *Caller, GIFunctionInfo *Info, int Count, ml_value_t **Args) {
	return function_info_invoke(Caller, Info, Count, Args);
}

static void method_invoke(ml_state_t *Caller, GIFunctionInfo *Info, int Count, ml_value_t **Args) {
	return function_info_invoke(Caller, Info, Count, Args);
}

static void method_register(const char *Name, GIFunctionInfo *Info, object_t *Object) {
	int NArgs = g_callable_info_get_n_args((GICallableInfo *)Info);
	int NArgsIn = 0;
	for (int I = 0; I < NArgs; ++I) {
		GIArgInfo *ArgInfo = g_callable_info_get_arg((GICallableInfo *)Info, I);
		int ClosureArg = g_arg_info_get_closure(ArgInfo);
		if (ClosureArg >= 0) goto done;
		GITypeInfo TypeInfo[1];
		g_arg_info_load_type(ArgInfo, TypeInfo);
		int LengthIndex = g_type_info_get_array_length(TypeInfo);
		if (LengthIndex >= 0) goto done;
		GITypeTag Tag = g_type_info_get_tag(TypeInfo);
		switch (g_arg_info_get_direction(ArgInfo)) {
		case GI_DIRECTION_IN:
		case GI_DIRECTION_INOUT:
			if (Tag == GI_TYPE_TAG_INTERFACE) {
				GIBaseInfo *InterfaceInfo = g_type_info_get_interface(TypeInfo);
				if (!g_base_info_equal(InterfaceInfo, DestroyNotifyInfo)) ++NArgsIn;
				g_base_info_unref(InterfaceInfo);
			} else {
				++NArgsIn;
			}
			break;
		case GI_DIRECTION_OUT:
			if (g_arg_info_is_caller_allocates(ArgInfo)) ++NArgsIn;
			break;
		}
	done:
		g_base_info_unref(ArgInfo);
	}
	++NArgsIn;
	ml_type_t *Types[NArgsIn];
	Types[0] = (ml_type_t *)Object;
	for (int I = 1; I < NArgsIn; ++I) Types[I] = MLAnyT;
	ml_method_define(ml_method(Name), ml_cfunctionx(Info, (ml_callbackx_t)method_invoke), NArgsIn, 0, Types);
}

static void interface_add_signals(object_t *Object, GIInterfaceInfo *Info) {
	int NumSignals = g_interface_info_get_n_signals(Info);
	for (int I = 0; I < NumSignals; ++I) {
		GISignalInfo *SignalInfo = g_interface_info_get_signal(Info, I);
		const char *SignalName = g_base_info_get_name((GIBaseInfo *)SignalInfo);
		stringmap_insert(Object->Signals, SignalName, SignalInfo);
	}
}

static void interface_add_methods(object_t *Object, GIInterfaceInfo *Info) {
	interface_add_signals(Object, Info);
	int NumMethods = g_interface_info_get_n_methods(Info);
	for (int I = 0; I < NumMethods; ++I) {
		GIFunctionInfo *MethodInfo = g_interface_info_get_method(Info, I);
		const char *MethodName = g_base_info_get_name((GIBaseInfo *)MethodInfo);
		GIFunctionInfoFlags Flags = g_function_info_get_flags(MethodInfo);
		if (Flags & GI_FUNCTION_IS_METHOD) {
			//ml_method_by_name(MethodName, MethodInfo, (ml_callback_t)method_invoke, Object, NULL);
			method_register(MethodName, MethodInfo, Object);
		} else {
			stringmap_insert(Object->Base.Exports, MethodName, ml_cfunctionx(MethodInfo, (void *)constructor_invoke));
		}
	}
}

static void object_add_signals(object_t *Object, GIObjectInfo *Info) {
	int NumSignals = g_object_info_get_n_signals(Info);
	for (int I = 0; I < NumSignals; ++I) {
		GISignalInfo *SignalInfo = g_object_info_get_signal(Info, I);
		const char *SignalName = g_base_info_get_name((GIBaseInfo *)SignalInfo);
		stringmap_insert(Object->Signals, SignalName, SignalInfo);
	}
}

static void object_add_methods(object_t *Object, GIObjectInfo *Info) {
	object_add_signals(Object, Info);
	GIObjectInfo *ParentInfo = g_object_info_get_parent(Info);
	while (ParentInfo) {
		object_add_signals(Object, ParentInfo);
		ml_type_t *Parent = object_info_lookup(ParentInfo);
		ml_type_add_parent((ml_type_t *)Object, Parent);
		ParentInfo = g_object_info_get_parent(ParentInfo);
	}
	int NumInterfaces = g_object_info_get_n_interfaces(Info);
	for (int I = 0; I < NumInterfaces; ++I) {
		GIInterfaceInfo *InterfaceInfo = g_object_info_get_interface(Info, I);
		interface_add_signals(Object, InterfaceInfo);
		ml_type_t *Interface = interface_info_lookup(InterfaceInfo);
		ml_type_add_parent((ml_type_t *)Object, Interface);
	}
	int NumMethods = g_object_info_get_n_methods(Info);
	for (int I = 0; I < NumMethods; ++I) {
		GIFunctionInfo *MethodInfo = g_object_info_get_method(Info, I);
		const char *MethodName = g_base_info_get_name((GIBaseInfo *)MethodInfo);
		GIFunctionInfoFlags Flags = g_function_info_get_flags(MethodInfo);
		if (Flags & GI_FUNCTION_IS_METHOD) {
			//ml_method_by_name(MethodName, MethodInfo, (ml_callback_t)method_invoke, Object, NULL);
			method_register(MethodName, MethodInfo, Object);
		//} else if (Flags & GI_FUNCTION_IS_CONSTRUCTOR) {
		} else {
			stringmap_insert(Object->Base.Exports, MethodName, ml_cfunctionx(MethodInfo, (void *)constructor_invoke));
		}
	}
}

static stringmap_t TypeMap[1] = {STRINGMAP_INIT};

static ml_value_t *object_instance(object_t *Object, int Count, ml_value_t **Args) {
	object_instance_t *Instance = new(object_instance_t);
	Instance->Type = Object;
	GType Type = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)Object->Info);
	if (Count > 0) {
		ML_CHECK_ARG_TYPE(0, MLNamesT);
		ML_NAMES_CHECK_ARG_COUNT(0);
		int NumProperties = Count - 1;
		const char *Names[NumProperties];
		GValue Values[NumProperties];
		memset(Values, 0, NumProperties * sizeof(GValue));
		int Index = 0;
		ML_NAMES_FOREACH(Args[0], Iter) {
			Names[Index] = ml_string_value(Iter->Value);
			_ml_to_value(Args[Index + 1], Values + Index);
			++Index;
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
		Object->Base.Type = GirObjectT;
		Object->Base.Name = TypeName;
		Object->Base.hash = ml_default_hash;
		Object->Base.call = ml_default_call;
		Object->Base.deref = ml_default_deref;
		Object->Base.assign = ml_default_assign;
		Object->Base.Constructor = ml_cfunction(Object, (ml_callback_t)object_instance);
		Object->Info = Info;
		ml_type_init((ml_type_t *)Object, GirObjectInstanceT, NULL);
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
		Object->Base.Type = GirObjectT;
		Object->Base.Name = TypeName;
		Object->Base.hash = ml_default_hash;
		Object->Base.call = ml_default_call;
		Object->Base.deref = ml_default_deref;
		Object->Base.assign = ml_default_assign;
		Object->Base.Constructor = ml_cfunction(Object, (ml_callback_t)object_instance);
		Object->Info = Info;
		ml_type_init((ml_type_t *)Object, GirObjectInstanceT, NULL);
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
		Struct->Base.Type = GirStructT;
		Struct->Base.Name = TypeName;
		Struct->Base.hash = ml_default_hash;
		Struct->Base.call = ml_default_call;
		Struct->Base.deref = ml_default_deref;
		Struct->Base.assign = ml_default_assign;
		Struct->Base.Constructor = ml_cfunction(Struct, (void *)struct_instance);
		Struct->Info = Info;
		ml_type_init((ml_type_t *)Struct, GirStructInstanceT, NULL);
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
				ml_methodx_by_name(MethodName, MethodInfo, (ml_callbackx_t)method_invoke, Struct, NULL);
			} else if (Flags & GI_FUNCTION_IS_CONSTRUCTOR) {
				stringmap_insert(Struct->Base.Exports, MethodName, ml_cfunctionx(MethodInfo, (void *)constructor_invoke));
			}
		}
	}
	return Slot[0];
}

static ml_type_t *union_info_lookup(GIUnionInfo *Info) {
	const char *TypeName = g_base_info_get_name((GIBaseInfo *)Info);
	ml_type_t **Slot = (ml_type_t **)stringmap_slot(TypeMap, TypeName);
	if (!Slot[0]) {
		union_t *Union = new(union_t);
		Union->Base.Type = GirUnionT;
		Union->Base.Name = TypeName;
		Union->Base.hash = ml_default_hash;
		Union->Base.call = ml_default_call;
		Union->Base.deref = ml_default_deref;
		Union->Base.assign = ml_default_assign;
		Union->Base.Constructor = ml_cfunction(Union, (void *)union_instance);
		Union->Info = Info;
		ml_type_init((ml_type_t *)Union, GirUnionInstanceT, NULL);
		Slot[0] = (ml_type_t *)Union;
		int NumFields = g_union_info_get_n_fields(Info);
		for (int I = 0; I < NumFields; ++I) {
			GIFieldInfo *FieldInfo = g_union_info_get_field(Info, I);
			const char *FieldName = g_base_info_get_name((GIBaseInfo *)FieldInfo);
			ml_method_by_name(FieldName, FieldInfo, (ml_callback_t)union_field_ref, Union, NULL);
		}
		int NumMethods = g_union_info_get_n_methods(Info);
		for (int I = 0; I < NumMethods; ++I) {
			GIFunctionInfo *MethodInfo = g_union_info_get_method(Info, I);
			const char *MethodName = g_base_info_get_name((GIBaseInfo *)MethodInfo);
			GIFunctionInfoFlags Flags = g_function_info_get_flags(MethodInfo);
			if (Flags & GI_FUNCTION_IS_METHOD) {
				ml_methodx_by_name(MethodName, MethodInfo, (ml_callbackx_t)method_invoke, Union, NULL);
			} else if (Flags & GI_FUNCTION_IS_CONSTRUCTOR) {
				stringmap_insert(Union->Base.Exports, MethodName, ml_cfunctionx(MethodInfo, (void *)constructor_invoke));
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
		enum_t *Enum = xnew(enum_t, NumValues + 1, ml_value_t *);
		Enum->Base.Type = GirEnumT;
		Enum->Base.Name = TypeName;
		Enum->Base.hash = ml_default_hash;
		Enum->Base.call = ml_default_call;
		Enum->Base.deref = ml_default_deref;
		Enum->Base.assign = ml_default_assign;
		Enum->Base.Rank = GirEnumT->Rank + 1;
		ml_type_init((ml_type_t *)Enum, GirEnumValueT, NULL);
		for (int I = 0; I < NumValues; ++I) {
			GIValueInfo *ValueInfo = g_enum_info_get_value(Info, I);
			const char *ValueName = GC_strdup(g_base_info_get_name((GIBaseInfo *)ValueInfo));
			enum_value_t *Value = new(enum_value_t);
			Value->Type = Enum;
			Value->Name = ml_string(ValueName, -1);
			Value->Value = g_value_info_get_value(ValueInfo);
			stringmap_insert(Enum->Base.Exports, ValueName, (ml_value_t *)Value);
			Enum->ByIndex[I] = Value;
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
		ml_value_t *Value = argument_to_ml(Argument, g_constant_info_get_type(Info), NULL, NULL);
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
		return ml_cfunctionx(Info, (ml_callbackx_t)function_info_invoke);
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
		return (ml_value_t *)interface_info_lookup((GIInterfaceInfo *)Info);
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

ml_value_t *ml_gir_import(ml_value_t *Typelib, const char *Name) {
	const char *Namespace = ml_gir_get_namespace(Typelib);
	GIBaseInfo *Info = g_irepository_find_by_name(NULL, Namespace, Name);
	if (!Info) {
		return ml_error("NameError", "Symbol %s not found in %s", Name, Namespace);
	} else {
		return baseinfo_to_value(Info);
	}
}

ML_METHOD("::", GirTypelibT, MLStringT) {
//<Typelib
//<Name
//>any | error
	return ml_gir_import(Args[0], ml_string_value(Args[1]));

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
		return (ml_value_t *)Enum->ByIndex[g_value_get_enum(Value)];
	}
	case G_TYPE_FLAGS:  {
		GType Type = G_VALUE_TYPE(Value);
		GIBaseInfo *Info = g_irepository_find_by_gtype(NULL, Type);
		enum_t *Enum = (enum_t *)enum_info_lookup((GIEnumInfo *)Info);
		return gir_enum_value(Enum, g_value_get_enum(Value));
	}
	case G_TYPE_FLOAT: return ml_real(g_value_get_float(Value));
	case G_TYPE_DOUBLE: return ml_real(g_value_get_double(Value));
	case G_TYPE_STRING: return ml_string(g_value_get_string(Value), -1);
	case G_TYPE_POINTER: return MLNil; //Std$Address$new(g_value_get_pointer(Value));
	default: {
		if (G_VALUE_HOLDS(Value, G_TYPE_OBJECT)) {
			return ml_gir_instance_get(g_value_get_object(Value), Info);
		} else {
			GIBaseInfo *InterfaceInfo = g_irepository_find_by_gtype(NULL, G_VALUE_TYPE(Value));
			if (InterfaceInfo) {
				switch (g_base_info_get_type(InterfaceInfo)) {
				case GI_INFO_TYPE_INVALID:
				case GI_INFO_TYPE_INVALID_0: break;
				case GI_INFO_TYPE_FUNCTION: break;
				case GI_INFO_TYPE_CALLBACK: break;
				case GI_INFO_TYPE_STRUCT: {
					struct_instance_t *Instance = new(struct_instance_t);
					Instance->Type = (struct_t *)struct_info_lookup((GIStructInfo *)InterfaceInfo);
					Instance->Value = g_value_get_boxed(Value);
					return (ml_value_t *)Instance;
				}
				case GI_INFO_TYPE_BOXED: {
					break;
				}
				case GI_INFO_TYPE_ENUM: {
					enum_t *Enum = (enum_t *)enum_info_lookup((GIEnumInfo *)InterfaceInfo);
					return (ml_value_t *)Enum->ByIndex[g_value_get_uint(Value)];
				}
				case GI_INFO_TYPE_FLAGS: {
					enum_t *Enum = (enum_t *)enum_info_lookup((GIEnumInfo *)InterfaceInfo);
					return gir_enum_value(Enum, g_value_get_uint(Value));
				}
				case GI_INFO_TYPE_OBJECT:
				case GI_INFO_TYPE_INTERFACE: {
					return ml_gir_instance_get(g_value_get_pointer(Value), InterfaceInfo);
				}
				case GI_INFO_TYPE_CONSTANT: {
					break;
				}
				case GI_INFO_TYPE_UNION: {
					union_instance_t *Instance = new(union_instance_t);
					Instance->Type = (union_t *)union_info_lookup((GIUnionInfo *)InterfaceInfo);
					Instance->Value = g_value_get_boxed(Value);
					return (ml_value_t *)Instance;
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
			}
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
	} else if (ml_is(Source, MLDoubleT)) {
		g_value_init(Dest, G_TYPE_DOUBLE);
		g_value_set_double(Dest, ml_real_value(Source));
	} else if (ml_is(Source, MLStringT)) {
		g_value_init(Dest, G_TYPE_STRING);
		g_value_set_string(Dest, ml_string_value(Source));
	} else if (ml_is(Source, GirObjectInstanceT)) {
		void *Object = ((object_instance_t *)Source)->Handle;
		g_value_init(Dest, G_OBJECT_TYPE(Object));
		g_value_set_object(Dest, Object);
	} else if (ml_is(Source, GirStructInstanceT)) {
		void *Value = ((struct_instance_t *)Source)->Value;
		g_value_init(Dest, G_TYPE_POINTER);
		g_value_set_object(Dest, Value);
	} else if (ml_is(Source, GirEnumValueT)) {
		enum_t *Enum = (enum_t *)((enum_value_t *)Source)->Type;
		GType Type = g_type_from_name(g_base_info_get_name((GIBaseInfo *)Enum->Info));
		g_value_init(Dest, Type);
		g_value_set_enum(Dest, ((enum_value_t *)Source)->Value);
	} else {
		g_value_init(Dest, G_TYPE_NONE);
	}
}

typedef struct {
	object_instance_t *Instance;
	GISignalInfo *SignalInfo;
	ml_context_t *Context;
	ml_value_t *Function;
} gir_closure_info_t;

static void gir_closure_marshal(GClosure *Closure, GValue *Dest, guint NumArgs, const GValue *Args, gpointer Hint, void *Data) {
	gir_closure_info_t *Info = (gir_closure_info_t *)Closure->data;
	GICallableInfo *SignalInfo = (GICallableInfo *)Info->SignalInfo;
	ml_value_t *MLArgs[NumArgs];
	MLArgs[0] = _value_to_ml(Args, NULL);
	for (guint I = 1; I < NumArgs; ++I) {
		GIArgInfo *ArgInfo = g_callable_info_get_arg(SignalInfo, I - 1);
		GITypeInfo TypeInfo[1];
		g_arg_info_load_type(ArgInfo, TypeInfo);
		MLArgs[I] = _value_to_ml(Args + I, g_type_info_get_interface(TypeInfo));
	}
	ml_result_state_t *State = ml_result_state(Info->Context);
	ml_call(State, Info->Function, NumArgs, MLArgs);
	GMainContext *MainContext = g_main_context_default();
	while (!State->Value) g_main_context_iteration(MainContext, TRUE);
	ml_value_t *Value = State->Value;
	if (ml_is_error(Value)) {
		fprintf(stderr, "%s: %s\n", ml_error_type(Value), ml_error_message(Value));
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Value, Level++, &Source)) {
			fprintf(stderr, "\t%s:%d\n", Source.Name, Source.Line);
		}
	}
	if (Dest) {
		if (ml_is(Value, MLBooleanT)) {
			g_value_set_boolean(Dest, ml_boolean_value(Value));
		} else if (ml_is(Value, MLIntegerT)) {
			g_value_set_long(Dest, ml_integer_value(Value));
		} else if (ml_is(Value, MLDoubleT)) {
			g_value_set_double(Dest, ml_real_value(Value));
		} else if (ml_is(Value, MLStringT)) {
			g_value_set_string(Dest, ml_string_value(Value));
		} else if (ml_is(Value, GirObjectInstanceT)) {
			void *Object = ((object_instance_t *)Value)->Handle;
			g_value_set_object(Dest, Object);
		} else if (ml_is(Value, GirStructInstanceT)) {
			void *Struct = ((struct_instance_t *)Value)->Value;
			g_value_set_object(Dest, Struct);
		} else if (ml_is(Value, GirEnumValueT)) {
			enum_t *Enum = (enum_t *)((enum_value_t *)Value)->Type;
			GType Type = g_type_from_name(g_base_info_get_name((GIBaseInfo *)Enum->Info));
			g_value_init(Dest, Type);
			g_value_set_enum(Dest, ((enum_value_t *)Value)->Value);
		}
	}
}

static void gir_closure_finalize(gir_closure_info_t *Info, GClosure *Closure) {
	if (Info->Instance) ptrset_remove(Info->Instance->Handlers, Info);
	GC_free(Info);
}

ML_METHODX("connect", GirObjectInstanceT, MLStringT, MLFunctionT) {
//<Object
//<Signal
//<Handler
//>Object
	object_instance_t *Instance = (object_instance_t *)Args[0];
	const char *Signal = ml_string_value(Args[1]);
	GISignalInfo *SignalInfo = (GISignalInfo *)stringmap_search(Instance->Type->Signals, Signal);
	if (!SignalInfo) ML_ERROR("NameError", "Signal %s::%s not found", Instance->Type->Base.Name, Signal);
	gir_closure_info_t *Info = GC_malloc_uncollectable(sizeof(gir_closure_info_t));
	Info->Instance = Instance;
	GC_register_disappearing_link((void **)&Info->Instance);
	Info->Context = Caller->Context;
	Info->Function = Args[2];
	Info->SignalInfo = SignalInfo;
	ptrset_insert(Instance->Handlers, Info);
	GClosure *Closure = g_closure_new_simple(sizeof(GClosure), Info);
	g_closure_set_marshal(Closure, gir_closure_marshal);
	g_closure_add_finalize_notifier(Closure, Info, (void *)gir_closure_finalize);
	gulong Id = g_signal_connect_closure(Instance->Handle, Signal, Closure, Count > 3 && Args[3] != MLNil);
	ML_RETURN(ml_integer(Id));
}

ML_METHOD("disconnect", GirObjectInstanceT, MLIntegerT) {
	object_instance_t *Instance = (object_instance_t *)Args[0];
	gulong Id = ml_integer_value(Args[1]);
	g_signal_handler_disconnect(Instance->Handle, Id);
	return MLNil;
}

typedef struct {
	ml_type_t *Type;
	GObject *Object;
	const char *Name;
} object_property_t;

static ml_value_t *object_property_deref(object_property_t *Property) {
	GValue Value[1] = {G_VALUE_INIT};
	g_object_get_property(Property->Object, Property->Name, Value);
	if (G_VALUE_TYPE(Value) == 0) {
		return ml_error("PropertyError", "Invalid property %s", Property->Name);
	}
	return _value_to_ml(Value, NULL);
}

static void object_property_assign(ml_state_t *Caller, object_property_t *Property, ml_value_t *Value0) {
	GValue Value[1];
	memset(Value, 0, sizeof(GValue));
	_ml_to_value(Value0, Value);
	g_object_set_property(Property->Object, Property->Name, Value);
	ML_RETURN(Value0);
}

ML_TYPE(GirObjectPropertyT, (), "object-property",
	.deref = (void *)object_property_deref,
	.assign = (void *)object_property_assign
);

ML_METHOD("::", GirObjectInstanceT, MLStringT) {
//<Object
//<Property
//>any
	object_instance_t *Instance = (object_instance_t *)Args[0];
	object_property_t *Property = new(object_property_t);
	Property->Type = GirObjectPropertyT;
	Property->Object = Instance->Handle;
	Property->Name = ml_string_value(Args[1]);
	return (ml_value_t *)Property;
}

#ifdef ML_SCHEDULER

void ml_gir_queue_add(ml_state_t *State, ml_value_t *Value);

ml_schedule_t GirSchedule[1] = {{256, ml_gir_queue_add}};

static gboolean ml_gir_queue_run(void *Data) {
	ml_queued_state_t QueuedState = ml_default_queue_next();
	if (!QueuedState.State) return FALSE;
	GirSchedule->Counter = 256;
	QueuedState.State->run(QueuedState.State, QueuedState.Value);
	return TRUE;
}

void ml_gir_queue_add(ml_state_t *State, ml_value_t *Value) {
	if (ml_default_queue_add(State, Value) == 1) g_idle_add(ml_gir_queue_run, NULL);
}

static ptrset_t SleepSet[1] = {PTRSET_INIT};

static gboolean sleep_run(void *Data) {
	ptrset_remove(SleepSet, Data);
	ml_state_schedule((ml_state_t *)Data, MLNil);
	return G_SOURCE_REMOVE;
}

ML_FUNCTIONX(MLSleep) {
//@sleep
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLNumberT);
	guint Interval = ml_real_value(Args[0]) * 1000;
	ptrset_insert(SleepSet, Caller);
	g_timeout_add(Interval, sleep_run, Caller);
}

ML_FUNCTIONX(GirRun) {
	ML_CHECKX_ARG_COUNT(1);
	ml_state_t *State = ml_state(Caller);
	ml_context_set(State->Context, ML_SCHEDULER_INDEX, GirSchedule);
	return ml_call(State, Args[0], 0, NULL);
}

#endif

#include "ml_stream.h"
#include <gio/gio.h>

ML_GIR_TYPELIB(Gio, "Gio", NULL);

typedef struct ml_gio_callback_t ml_gio_callback_t;

struct ml_gio_callback_t {
	ml_gio_callback_t *Next, *Prev;
	ml_state_t *Caller;
};

static ml_gio_callback_t GioCallbacks[1] = {{GioCallbacks, GioCallbacks, NULL}};
static ml_gio_callback_t *GioCallbackCache = NULL;

static ml_gio_callback_t *ml_gio_callback(ml_state_t *Caller) {
	ml_gio_callback_t *Callback = GioCallbackCache;
	if (Callback) {
		GioCallbackCache = Callback->Next;
	} else {
		Callback = new(ml_gio_callback_t);
	}
	Callback->Prev = GioCallbacks;
	Callback->Next = GioCallbacks->Next;
	GioCallbacks->Next->Prev = Callback;
	GioCallbacks->Next = Callback;
	Callback->Caller = Caller;
	return Callback;
}

static ml_state_t *ml_gio_callback_use(ml_gio_callback_t *Callback) {
	Callback->Prev->Next = Callback->Next;
	Callback->Next->Prev = Callback->Prev;
	ml_state_t *Caller = Callback->Caller;
	Callback->Next = GioCallbackCache;
	GioCallbackCache = Callback;
	return Caller;
}

ML_GIR_IMPORT(GInputStreamT, Gio, "InputStream");

static void g_input_stream_callback(GObject *Object, GAsyncResult *Result, gpointer Data) {
	GInputStream *Stream = (GInputStream *)Object;
	ml_state_t *Caller = ml_gio_callback_use((ml_gio_callback_t *)Data);
	GError *Error = NULL;
	gssize Count = g_input_stream_read_finish(Stream, Result, &Error);
	if (Error) ML_ERROR("GirError", "%s", Error->message);
	ML_RETURN(ml_integer(Count));
}

static void ML_TYPED_FN(ml_stream_read, (ml_type_t *)GInputStreamT, ml_state_t *Caller, object_instance_t *Value, void *Address, int Count) {
	GInputStream *Stream = (GInputStream *)Value->Handle;
	g_input_stream_read_async(Stream, Address, Count, 0, NULL, g_input_stream_callback, ml_gio_callback(Caller));
}

ML_GIR_IMPORT(GOutputStreamT, Gio, "OutputStream");

static void g_output_stream_callback(GObject *Object, GAsyncResult *Result, gpointer Data) {
	GOutputStream *Stream = (GOutputStream *)Object;
	ml_state_t *Caller = ml_gio_callback_use((ml_gio_callback_t *)Data);
	GError *Error = NULL;
	gssize Count = g_output_stream_write_finish(Stream, Result, &Error);
	if (Error) ML_ERROR("GirError", "%s", Error->message);
	ML_RETURN(ml_integer(Count));
}

static void ML_TYPED_FN(ml_stream_write, (ml_type_t *)GOutputStreamT, ml_state_t *Caller, object_instance_t *Value, void *Address, int Count) {
	GOutputStream *Stream = (GOutputStream *)Value->Handle;
	g_output_stream_write_async(Stream, Address, Count, 0, NULL, g_output_stream_callback, ml_gio_callback(Caller));
}

typedef struct ml_gir_value_t ml_gir_value_t;

struct ml_gir_value_t {

};

static GMainLoop *MainLoop = NULL;

void ml_gir_loop_init(ml_context_t *Context) {
	MainLoop = g_main_loop_new(NULL, TRUE);
	ml_context_set(Context, ML_SCHEDULER_INDEX, GirSchedule);
}

void ml_gir_loop_run() {
	g_main_loop_run(MainLoop);
}

void ml_gir_loop_quit() {
	g_main_loop_quit(MainLoop);
}

void ml_gir_init(stringmap_t *Globals) {
	//g_setenv("G_SLICE", "always-malloc", 1);
	GError *Error = 0;
	g_irepository_require(NULL, "GLib", NULL, 0, &Error);
	g_irepository_require(NULL, "GObject", NULL, 0, &Error);
	DestroyNotifyInfo = g_irepository_find_by_name(NULL, "GLib", "DestroyNotify");
	GValueInfo = g_irepository_find_by_name(NULL, "GObject", "Value");
	ml_typed_fn_set(GirTypelibT, ml_iterate, typelib_iterate);
	ml_typed_fn_set(TypelibIterT, ml_iter_next, typelib_iter_next);
	ml_typed_fn_set(TypelibIterT, ml_iter_value, typelib_iter_value);
	ml_typed_fn_set(TypelibIterT, ml_iter_key, typelib_iter_key);
	MLQuark = g_quark_from_static_string("<<minilang>>");
	MLType = g_pointer_type_register_static("minilang");
	ObjectInstanceNil = new(object_instance_t);
	ObjectInstanceNil->Type = (object_t *)GirObjectInstanceT;
	//ml_typed_fn_set(EnumT, ml_iterate, enum_iterate);
	GirObjectT->call = MLTypeT->call;
	GirStructT->call = MLTypeT->call;
#include "ml_gir_init.c"
	ml_type_add_parent((ml_type_t *)GInputStreamT, MLStreamT);
	ml_type_add_parent((ml_type_t *)GOutputStreamT, MLStreamT);
#ifdef ML_SCHEDULER
	stringmap_insert(Globals, "sleep", MLSleep);
	stringmap_insert(GirTypelibT->Exports, "run", GirRun);
#endif
	stringmap_insert(Globals, "gir", GirTypelibT);
#ifdef ML_LIBRARY
	ml_library_register("gir", GirModule);
#endif
}

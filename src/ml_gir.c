#include "ml_gir.h"
#include "ml_macros.h"
#include "ml_logging.h"
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

ML_METHOD("append", MLStringBufferT, GirEnumValueT) {
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
			if (ml_stringbuffer_length(Buffer)) ml_stringbuffer_put(Buffer, '|');
			const char *Name = ml_string_value(Ptr[0]->Name);
			size_t Length = ml_string_length(Ptr[0]->Name);
			ml_stringbuffer_write(Buffer, Name, Length);
		}
	}
	Enum->Name = ml_stringbuffer_to_string(Buffer);
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
				return ml_address(Argument->v_string, -1);
			} else {
				size_t Length;
				int LengthIndex = g_type_info_get_array_length(TypeInfo);
				if (LengthIndex < 0) {
					Length = g_type_info_get_array_fixed_size(TypeInfo);
				} else {
					if (!Info) return ml_error("ValueError", "Unsupported situtation");
					Length = get_output_length(Info, LengthIndex, ArgsOut);
				}
				return ml_address(Argument->v_string, Length);
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

static GIBaseInfo *DestroyNotifyInfo;

static ml_value_t *function_info_compile(GIFunctionInfo *Info);

static void method_register(const char *Name, GIFunctionInfo *Info, ml_type_t *Object) {
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
	Types[0] = Object;
	for (int I = 1; I < NArgsIn; ++I) Types[I] = MLAnyT;
	//if (!strcmp(Name, "wait")) asm("int3");
	ml_method_define(ml_method(Name), function_info_compile(Info), NArgsIn, 0, Types);
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
			method_register(MethodName, MethodInfo, (ml_type_t *)Object);
		} else {
			stringmap_insert(Object->Base.Exports, MethodName, function_info_compile(MethodInfo));
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
		//if (!strcmp(MethodName, "query")) asm("int3");
		if (Flags & GI_FUNCTION_IS_METHOD) {
			method_register(MethodName, MethodInfo, (ml_type_t *)Object);
		} else {
			stringmap_insert(Object->Base.Exports, MethodName, function_info_compile(MethodInfo));
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
		g_base_info_ref(Info);
		ml_type_init((ml_type_t *)Object, GirObjectInstanceT, NULL);
		Slot[0] = (ml_type_t *)Object;
		object_add_methods(Object, Info);
		g_base_info_ref(Info);
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
		g_base_info_ref(Info);
		ml_type_init((ml_type_t *)Object, GirObjectInstanceT, NULL);
		Slot[0] = (ml_type_t *)Object;
		interface_add_methods(Object, Info);
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
		g_base_info_ref(Info);
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
				method_register(MethodName, MethodInfo, (ml_type_t *)Struct);
			} else if (Flags & GI_FUNCTION_IS_CONSTRUCTOR) {
				stringmap_insert(Struct->Base.Exports, MethodName, function_info_compile(MethodInfo));
			}
			g_base_info_unref((GIBaseInfo *)MethodInfo);
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
		g_base_info_ref(Info);
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
				method_register(MethodName, MethodInfo, (ml_type_t *)Union);
			} else if (Flags & GI_FUNCTION_IS_CONSTRUCTOR) {
				method_register(MethodName, MethodInfo, (ml_type_t *)Union);
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
		Slot[0] = (ml_type_t *)Enum;
		for (int I = 0; I < NumValues; ++I) {
			GIValueInfo *ValueInfo = g_enum_info_get_value(Info, I);
			const char *ValueName = GC_strdup(g_base_info_get_name((GIBaseInfo *)ValueInfo));
			enum_value_t *Value = new(enum_value_t);
			Value->Type = Enum;
			Value->Name = ml_string(ValueName, -1);
			Value->Value = g_value_info_get_value(ValueInfo);
			stringmap_insert(Enum->Base.Exports, ValueName, (ml_value_t *)Value);
			Enum->ByIndex[I] = Value;
			g_base_info_unref(ValueInfo);
		}
		Enum->Info = Info;
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

static ml_type_t *callback_info_lookup(GICallbackInfo *Info);

static ml_value_t *baseinfo_to_value(GIBaseInfo *Info) {
	switch (g_base_info_get_type(Info)) {
	case GI_INFO_TYPE_INVALID:
	case GI_INFO_TYPE_INVALID_0: {
		break;
	}
	case GI_INFO_TYPE_FUNCTION: {
		return function_info_compile((GIFunctionInfo *)Info);
	}
	case GI_INFO_TYPE_CALLBACK: {
		return (ml_value_t *)callback_info_lookup((GICallbackInfo *)Info);
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

static GMainLoop *MainLoop = NULL;

typedef struct {
	object_instance_t *Instance;
	ml_context_t *Context;
	ml_value_t *Function;
	int NumArgs;
	GIBaseInfo *Args[];
} gir_closure_info_t;

static void gir_closure_marshal(GClosure *Closure, GValue *Dest, guint NumArgs, const GValue *Args, gpointer Hint, void *Data) {
	gir_closure_info_t *Info = (gir_closure_info_t *)Closure->data;
	ml_value_t *MLArgs[NumArgs];
	MLArgs[0] = _value_to_ml(Args, NULL);
	for (guint I = 1; I < NumArgs; ++I) MLArgs[I] = _value_to_ml(Args + I, Info->Args[I]);
	ml_result_state_t *State = ml_result_state(Info->Context);
	ml_call(State, Info->Function, NumArgs, MLArgs);
	ml_scheduler_t *Scheduler = ml_context_get(Info->Context, ML_SCHEDULER_INDEX);
	while (!State->Value) Scheduler->run(Scheduler);
	ml_value_t *Value = State->Value;
	if (ml_is_error(Value)) ML_LOG_ERROR(Value, "Closure returned error");
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
	for (int I = 0; I < Info->NumArgs; ++I) if (Info->Args[I]) g_base_info_unref(Info->Args[I]);
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
	int NumArgs = g_callable_info_get_n_args((GICallableInfo *)SignalInfo);
	gir_closure_info_t *Info = GC_malloc_uncollectable(sizeof(gir_closure_info_t) + NumArgs * sizeof(GIBaseInfo *));
	Info->Instance = Instance;
	GC_register_disappearing_link((void **)&Info->Instance);
	Info->Context = Caller->Context;
	Info->Function = Args[2];
	Info->NumArgs = NumArgs;
	for (int I = 0; I < NumArgs; ++I) {
		GIArgInfo *ArgInfo = g_callable_info_get_arg(SignalInfo, I);
		GITypeInfo TypeInfo[1];
		g_arg_info_load_type(ArgInfo, TypeInfo);
		Info->Args[I] = g_type_info_get_interface(TypeInfo);
		g_base_info_unref(ArgInfo);
	}
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

ML_INTERFACE(GirInstanceT, (), "instance");

typedef struct {
	ml_type_t Base;
} ml_gir_type_t;

static void instance_constructor_fn(ml_state_t *Caller, ml_gir_type_t *Class, int Count, ml_value_t **Args) {

}

#ifdef ML_SCHEDULER

typedef struct {
	ml_scheduler_t Base;
	ml_scheduler_queue_t *Queue;
	GMainContext *MainContext;
} gir_scheduler_t;

int ml_gir_queue_add(gir_scheduler_t *Scheduler, ml_state_t *State, ml_value_t *Value) {
	int Fill = ml_scheduler_queue_add(Scheduler->Queue, State, Value);
	g_main_context_wakeup(Scheduler->MainContext);
	return Fill;
}

void ml_gir_queue_run(gir_scheduler_t *Scheduler) {
	g_main_context_iteration(Scheduler->MainContext, !ml_scheduler_queue_fill(Scheduler->Queue));
	ml_queued_state_t QueuedState = ml_scheduler_queue_next(Scheduler->Queue);
	if (QueuedState.State) QueuedState.State->run(QueuedState.State, QueuedState.Value);
}

static gir_scheduler_t *gir_scheduler(ml_context_t *Context) {
	gir_scheduler_t *Scheduler = new(gir_scheduler_t);
	Scheduler->Base.add = (ml_scheduler_add_fn)ml_gir_queue_add;
	Scheduler->Base.run = (ml_scheduler_run_fn)ml_gir_queue_run;
	Scheduler->Queue = ml_default_queue_init(Context, 256);
	Scheduler->MainContext = g_main_context_default();
	ml_context_set(Context, ML_SCHEDULER_INDEX, Scheduler);
	return Scheduler;
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

ML_FUNCTIONX(GirInstall) {
//@gir::install
	gir_scheduler(Caller->Context);
	ML_RETURN(MLNil);
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
	gsize Count;
	g_output_stream_write_all_finish(Stream, Result, &Count, &Error);
	if (Error) ML_ERROR("GirError", "%s", Error->message);
	ML_RETURN(ml_integer(Count));
}

static void ML_TYPED_FN(ml_stream_write, (ml_type_t *)GOutputStreamT, ml_state_t *Caller, object_instance_t *Value, void *Address, int Count) {
	GOutputStream *Stream = (GOutputStream *)Value->Handle;
	g_output_stream_write_all_async(Stream, Address, Count, 0, NULL, g_output_stream_callback, ml_gio_callback(Caller));
}

typedef struct ml_gir_value_t ml_gir_value_t;

struct ml_gir_value_t {

};

void ml_gir_loop_init(ml_context_t *Context) {
	MainLoop = g_main_loop_new(NULL, TRUE);
	gir_scheduler(Context);
}

void ml_gir_loop_run() {
	g_main_loop_run(MainLoop);
}

void ml_gir_loop_quit() {
	g_main_loop_quit(MainLoop);
}

#include "ml_array.h"

#ifndef ML_CATEGORY
#define ML_CATEGORY "gir"
#endif

typedef enum {
	GIB_DONE,
	GIB_BOOLEAN,
	GIB_INT8,
	GIB_UINT8,
	GIB_INT16,
	GIB_UINT16,
	GIB_INT32,
	GIB_UINT32,
	GIB_INT64,
	GIB_UINT64,
	GIB_FLOAT,
	GIB_DOUBLE,
	GIB_STRING,
	GIB_GTYPE,
	//GIB_BYTES,
	GIB_ARRAY,
	GIB_CALLBACK,
	GIB_STRUCT,
	GIB_UNION,
	GIB_ENUM,
	GIB_OBJECT,
	GIB_SELF,
	GIB_VALUE,
	GIB_LIST,
	GIB_SLIST,
	GIB_HASH,
	GIB_ERROR,
	//GIB_BYTES_FIXED,
	//GIB_BYTES_LENGTH,
	GIB_ARRAY_ZERO,
	GIB_ARRAY_FIXED,
	GIB_ARRAY_LENGTH,
	GIB_SKIP,
	GIB_OUTPUT_VALUE,
	GIB_OUTPUT_ARRAY,
	GIB_OUTPUT_ARRAY_LENGTH,
	GIB_OUTPUT_STRUCT,
	GIB_FREE
} gi_opcode_t;

#define VALUE_CONVERTOR(GTYPE, MLTYPE) \
static ml_value_t *GTYPE ## _to_array(ml_value_t *Value, void *Ptr) { \
	*(g ## GTYPE *)Ptr = ml_ ## MLTYPE ## _value(Value); \
	return NULL; \
} \
\
static ml_value_t *GTYPE ## _to_glist(ml_value_t *Value, void **Ptr) { \
	*Ptr = (void *)(uintptr_t)ml_ ## MLTYPE ## _value(Value); \
	return NULL; \
} \
\
static ml_value_t *GTYPE ## _to_value(void *Ptr, void *Aux) { \
	return ml_ ## MLTYPE((g ## GTYPE)(uintptr_t)Ptr); \
}

VALUE_CONVERTOR(boolean, boolean);
VALUE_CONVERTOR(int8, integer);
VALUE_CONVERTOR(uint8, integer)
VALUE_CONVERTOR(int16, integer);
VALUE_CONVERTOR(uint16, integer)
VALUE_CONVERTOR(int32, integer);
VALUE_CONVERTOR(uint32, integer)
VALUE_CONVERTOR(int64, integer);
VALUE_CONVERTOR(uint64, integer)
VALUE_CONVERTOR(float, real);
VALUE_CONVERTOR(double, real);

static ml_value_t *string_to_array(ml_value_t *Value, void *Ptr) {
	if (Value == MLNil) {
		*(gchararray *)Ptr = NULL;
	} else if (ml_is(Value, MLAddressT)) {
		*(gchararray *)Ptr = (gchararray)ml_address_value(Value);
	} else {
		return ml_error("TypeError", "Expected string not %s", ml_typeof(Value)->Name);
	}
	return NULL;
}

static ml_value_t *string_to_glist(ml_value_t *Value, void **Ptr) {
	if (Value == MLNil) {
		*Ptr = NULL;
	} else if (ml_is(Value, MLAddressT)) {
		*Ptr = (void *)ml_address_value(Value);
	} else {
		return ml_error("TypeError", "Expected string not %s", ml_typeof(Value)->Name);
	}
	return NULL;
}

static ml_value_t *string_to_value(void *Ptr, void *Aux) {
	return ml_string_copy(Ptr, -1);
}

static ml_value_t *gtype_to_array(ml_value_t *Value, void *Ptr) {
	if (ml_is(Value, GirBaseInfoT)) {
		baseinfo_t *Base = (baseinfo_t *)Value;
		*(GType *)Ptr = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)Base->Info);
	} else if (ml_is(Value, MLStringT)) {
		*(GType *)Ptr = g_type_from_name(ml_string_value(Value));
	} else if (Value == (ml_value_t *)MLNilT) {
		*(GType *)Ptr = G_TYPE_NONE;
	} else if (Value == (ml_value_t *)MLIntegerT) {
		*(GType *)Ptr = G_TYPE_INT64;
	} else if (Value == (ml_value_t *)MLStringT) {
		*(GType *)Ptr = G_TYPE_STRING;
	} else if (Value == (ml_value_t *)MLDoubleT) {
		*(GType *)Ptr = G_TYPE_DOUBLE;
	} else if (Value == (ml_value_t *)MLBooleanT) {
		*(GType *)Ptr = G_TYPE_BOOLEAN;
	} else if (Value == (ml_value_t *)MLAddressT) {
		*(GType *)Ptr = G_TYPE_POINTER;
	} else {
		return ml_error("TypeError", "Expected type not %s", ml_typeof(Value)->Name);
	}
	return NULL;
}

static ml_value_t *gtype_to_glist(ml_value_t *Value, void **Ptr) {
	if (ml_is(Value, GirBaseInfoT)) {
		baseinfo_t *Base = (baseinfo_t *)Value;
		*Ptr = (void *)(uintptr_t)g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)Base->Info);
	} else if (ml_is(Value, MLStringT)) {
		*Ptr = (void *)(uintptr_t)g_type_from_name(ml_string_value(Value));
	} else if (Value == (ml_value_t *)MLNilT) {
		*Ptr = (void *)(uintptr_t)G_TYPE_NONE;
	} else if (Value == (ml_value_t *)MLIntegerT) {
		*Ptr = (void *)(uintptr_t)G_TYPE_INT64;
	} else if (Value == (ml_value_t *)MLStringT) {
		*Ptr = (void *)(uintptr_t)G_TYPE_STRING;
	} else if (Value == (ml_value_t *)MLDoubleT) {
		*Ptr = (void *)(uintptr_t)G_TYPE_DOUBLE;
	} else if (Value == (ml_value_t *)MLBooleanT) {
		*Ptr = (void *)(uintptr_t)G_TYPE_BOOLEAN;
	} else if (Value == (ml_value_t *)MLAddressT) {
		*Ptr = (void *)(uintptr_t)G_TYPE_POINTER;
	} else {
		return ml_error("TypeError", "Expected type not %s", ml_typeof(Value)->Name);
	}
	return NULL;
}

static ml_value_t *gtype_to_value(void *Ptr, void *Aux) {
	return ml_string(g_type_name((GType)Ptr), -1);
}

/*static ml_value_t *bytes_to_array(ml_value_t *Value, void *Ptr) {
	if (Value == MLNil) {
		*(gchararray *)Ptr = NULL;
	} else if (ml_is(Value, MLAddressT)) {
		*(gchararray *)Ptr = (gchararray)ml_address_value(Value);
	} else if (ml_is(Value, MLListT)) {
		size_t Length = ml_list_length(Value);
		void *Bytes = snew(Length + 1), *Ptr2 = Bytes;
		ML_LIST_FOREACH(Value, Iter) {
			ml_value_t *Error = int8_to_array(Iter->Value, Ptr2);
			if (Error) return Error;
			Ptr2 += 1;
		}
		*(gchararray *)Ptr = Bytes;
	} else {
		return ml_error("TypeError", "Expected bytes not %s", ml_typeof(Value)->Name);
	}
	return NULL;
}*/

/*static ml_value_t *bytes_to_glist(ml_value_t *Value, void **Ptr) {
	if (Value == MLNil) {
		*Ptr = NULL;
	} else if (ml_is(Value, MLAddressT)) {
		*Ptr = (void *)ml_address_value(Value);
	} else if (ml_is(Value, MLListT)) {
		size_t Length = ml_list_length(Value);
		void *Bytes = snew(Length + 1), *Ptr2 = Bytes;
		ML_LIST_FOREACH(Value, Iter) {
			ml_value_t *Error = int8_to_array(Iter->Value, Ptr2);
			if (Error) return Error;
			Ptr2 += 1;
		}
		*Ptr = Bytes;
	} else {
		return ml_error("TypeError", "Expected bytes not %s", ml_typeof(Value)->Name);
	}
	return NULL;
}*/

static ml_value_t *unknown_to_value(void *Ptr, void *Aux) {
	return ml_error("TypeError", "Unsupported value type");
}

static void to_gvalue(ml_value_t *Source, GValue *Dest) {
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

typedef union {
	gi_opcode_t Opcode;
	int Aux;
} gi_inst_t;

typedef struct {
	ml_type_t Base;
	GICallableInfo *Info;
	gi_inst_t *InstIn, *InstOut;
	int Provided;
	void *Aux[];
} callback_t;

typedef struct {
	ml_type_t *Type;
	ml_context_t *Context;
	ml_value_t *Function;
	ffi_cif Cif[1];
} callback_instance_t;

ML_TYPE(GirCallbackT, (GirBaseInfoT), "callback-type");
// A gobject-introspection callback type.

ML_TYPE(GirCallbackInstanceT, (), "callback-instance");
// A gobject-introspection callback instance.

static ml_value_t *convert_string(void *Arg) {
	return ml_string(Arg, -1);
}

static ml_value_t *convert_gtype(void *Arg) {
	return MLNil;
}

/*static ml_value_t *convert_bytes(void *Arg) {
	return MLNil;
}*/

static void callback_fn(ffi_cif *Cif, void *Return, void **Params, callback_instance_t *Instance) {
	callback_t *Callback = (callback_t *)Instance->Type;
	ml_value_t *Args[Callback->Provided];
	ml_value_t **Arg = Args;
	void **Param = Params;
	for (gi_inst_t *Inst = Callback->InstIn; Inst->Opcode != GIB_DONE;) switch ((Inst++)->Opcode) {
	case GIB_SKIP: Param++; break;
	case GIB_BOOLEAN: *Arg++ = ml_boolean(*(int *)(*Param++)); break;
	case GIB_INT8: *Arg++ = ml_integer(*(gint8 *)(*Param++)); break;
	case GIB_UINT8: *Arg++ = ml_integer(*(guint8 *)(*Param++)); break;
	case GIB_INT16: *Arg++ = ml_integer(*(gint16 *)(*Param++)); break;
	case GIB_UINT16: *Arg++ = ml_integer(*(guint16 *)(*Param++)); break;
	case GIB_INT32: *Arg++ = ml_integer(*(gint32 *)(*Param++)); break;
	case GIB_UINT32: *Arg++ = ml_integer(*(guint32 *)(*Param++)); break;
	case GIB_INT64: *Arg++ = ml_integer(*(gint64 *)(*Param++)); break;
	case GIB_UINT64: *Arg++ = ml_integer(*(guint64 *)(*Param++)); break;
	case GIB_FLOAT: *Arg++ = ml_real(*(gfloat *)(*Param++)); break;
	case GIB_DOUBLE: *Arg++ = ml_real(*(gdouble *)(*Param++)); break;
	case GIB_GTYPE: *Arg++ = MLNil; Param++; break;
	case GIB_STRING: *Arg++ = ml_string(*(gchararray *)(*Param++), -1); break;
	case GIB_ARRAY_LENGTH: {
		void *Address = *(void **)(*Param++);
		int Length = *(int *)(Params[(Inst++)->Aux]);
		int Size = 0;
		ml_array_format_t Format = ML_ARRAY_FORMAT_NONE;
		ml_value_t *(*to_value)(void *, void *) = NULL;
		switch ((Inst++)->Opcode) {
		case GIB_BOOLEAN: {
			Size = sizeof(gboolean);
			switch (Size) {
			case 1: Format = ML_ARRAY_FORMAT_I8;
			case 2: Format = ML_ARRAY_FORMAT_I16;
			case 4: Format = ML_ARRAY_FORMAT_I32;
			case 8: Format = ML_ARRAY_FORMAT_I64;
			}
			break;
		}
		case GIB_INT8: Format = ML_ARRAY_FORMAT_I8; Size = sizeof(gint8); break;
		case GIB_UINT8: Format = ML_ARRAY_FORMAT_U8; Size = sizeof(guint8); break;
		case GIB_INT16: Format = ML_ARRAY_FORMAT_I16; Size = sizeof(gint16); break;
		case GIB_UINT16: Format = ML_ARRAY_FORMAT_U16; Size = sizeof(guint16); break;
		case GIB_INT32: Format = ML_ARRAY_FORMAT_I32; Size = sizeof(gint32); break;
		case GIB_UINT32: Format = ML_ARRAY_FORMAT_U32; Size = sizeof(guint32); break;
		case GIB_INT64: Format = ML_ARRAY_FORMAT_I64; Size = sizeof(gint64); break;
		case GIB_UINT64: Format = ML_ARRAY_FORMAT_U64; Size = sizeof(guint64); break;
		case GIB_FLOAT: Format = ML_ARRAY_FORMAT_F32; Size = sizeof(gfloat); break;
		case GIB_DOUBLE: Format = ML_ARRAY_FORMAT_F64; Size = sizeof(gdouble); break;
		case GIB_STRING: Format = ML_ARRAY_FORMAT_ANY; to_value = string_to_value; break;
		case GIB_GTYPE: Format = ML_ARRAY_FORMAT_ANY; to_value = gtype_to_value; break;
		//case GIB_BYTES: Format = ML_ARRAY_FORMAT_ANY; convert = convert_bytes; Size = sizeof(gpointer); break;
		}
		ml_array_t *Array;
		if (Format == ML_ARRAY_FORMAT_ANY) {
			Array = ml_array(Format, 1, Length);
			ml_value_t **Slot = (ml_value_t **)Array->Base.Value;
			void **Input = (void **)Address;
			for (int I = 0; I < Length; ++I) *Slot++ = to_value(*Input++, NULL);
		} else {
			Array = ml_array_alloc(Format, 1);
			Array->Dimensions[0].Stride = Size;
			Array->Dimensions[0].Size = Length;
			Array->Base.Length = Length * Size;
			Array->Base.Value = Address;
		}
		*Arg++ = (ml_value_t *)Array;
		break;
	}
	case GIB_ARRAY_FIXED: {
		void *Address = *(void **)(*Param++);
		int Length = Callback->Aux[(Inst++)->Aux] - (void *)0;
		int Size = 0;
		ml_array_format_t Format = ML_ARRAY_FORMAT_NONE;
		ml_value_t *(*convert)(void *) = NULL;
		switch ((Inst++)->Opcode) {
		case GIB_BOOLEAN: {
			Size = sizeof(gboolean);
			switch (Size) {
			case 1: Format = ML_ARRAY_FORMAT_I8;
			case 2: Format = ML_ARRAY_FORMAT_I16;
			case 4: Format = ML_ARRAY_FORMAT_I32;
			case 8: Format = ML_ARRAY_FORMAT_I64;
			}
			break;
		}
		case GIB_INT8: Format = ML_ARRAY_FORMAT_I8; Size = sizeof(gint8); break;
		case GIB_UINT8: Format = ML_ARRAY_FORMAT_U8; Size = sizeof(guint8); break;
		case GIB_INT16: Format = ML_ARRAY_FORMAT_I16; Size = sizeof(gint16); break;
		case GIB_UINT16: Format = ML_ARRAY_FORMAT_U16; Size = sizeof(guint16); break;
		case GIB_INT32: Format = ML_ARRAY_FORMAT_I32; Size = sizeof(gint32); break;
		case GIB_UINT32: Format = ML_ARRAY_FORMAT_U32; Size = sizeof(guint32); break;
		case GIB_INT64: Format = ML_ARRAY_FORMAT_I64; Size = sizeof(gint64); break;
		case GIB_UINT64: Format = ML_ARRAY_FORMAT_U64; Size = sizeof(guint64); break;
		case GIB_FLOAT: Format = ML_ARRAY_FORMAT_F32; Size = sizeof(gfloat); break;
		case GIB_DOUBLE: Format = ML_ARRAY_FORMAT_F64; Size = sizeof(gdouble); break;
		case GIB_STRING: Format = ML_ARRAY_FORMAT_ANY; convert = convert_string; break;
		case GIB_GTYPE: Format = ML_ARRAY_FORMAT_ANY; convert = convert_gtype; break;
		//case GIB_BYTES: Format = ML_ARRAY_FORMAT_ANY; convert = convert_bytes; Size = sizeof(gpointer); break;
		}
		ml_array_t *Array;
		if (Format == ML_ARRAY_FORMAT_ANY) {
			Array = ml_array(Format, 1, Length);
			ml_value_t **Slot = (ml_value_t **)Array->Base.Value;
			void **Input = (void **)Address;
			for (int I = 0; I < Length; ++I) *Slot++ = convert(*Input++);
		} else {
			Array = ml_array_alloc(Format, 1);
			Array->Dimensions[0].Stride = Size;
			Array->Dimensions[0].Size = Length;
			Array->Base.Length = Length * Size;
			Array->Base.Value = Address;
		}
		*Arg++ = (ml_value_t *)Array;
		break;
	}
	case GIB_STRUCT: {
		struct_instance_t *Instance = new(struct_instance_t);
		Instance->Type = (struct_t *)Callback->Aux[(Inst++)->Aux];
		Instance->Value = *(void **)(*Param++);
		*Arg++ = (ml_value_t *)Instance;
		break;
	}
	case GIB_UNION: {
		union_instance_t *Instance = new(union_instance_t);
		Instance->Type = (union_t *)Callback->Aux[(Inst++)->Aux];
		Instance->Value = *(void **)(*Param++);
		*Arg++ = (ml_value_t *)Instance;
		break;
	}
	case GIB_ENUM: {
		enum_t *Enum = (enum_t *)Callback->Aux[(Inst++)->Aux];
		*Arg++ = gir_enum_value(Enum, *(int *)(*Param++));
		break;
	}
	case GIB_OBJECT: {
		object_t *Object = (object_t *)Callback->Aux[(Inst++)->Aux];
		*Arg++ = ml_gir_instance_get(*(void **)(*Param++), (GIBaseInfo *)Object->Info);
		break;
	}
	case GIB_VALUE: {
		*Arg++ = _value_to_ml(*(void **)(*Param++), NULL);
		break;
	}
	case GIB_LIST: {
		ml_value_t *(*to_value)(void *, void *) = NULL;
		switch ((Inst++)->Opcode) {
		case GIB_BOOLEAN: to_value = boolean_to_value; break;
		case GIB_INT8: to_value = int8_to_value; break;
		case GIB_UINT8: to_value = uint8_to_value; break;
		case GIB_INT16: to_value = int16_to_value; break;
		case GIB_UINT16: to_value = uint16_to_value; break;
		case GIB_INT32: to_value = int32_to_value; break;
		case GIB_UINT32: to_value = uint32_to_value; break;
		case GIB_INT64: to_value = int64_to_value; break;
		case GIB_UINT64: to_value = uint64_to_value; break;
		case GIB_FLOAT: to_value = float_to_value; break;
		case GIB_DOUBLE: to_value = double_to_value; break;
		case GIB_STRING: to_value = string_to_value; break;
		case GIB_GTYPE: to_value = gtype_to_value; break;
		//case GIB_BYTES: to_value = bytes_to_value; break;
		default: to_value = unknown_to_value; break;
		}
		ml_value_t *List = ml_list();
		for (GList *Node = (GList *)(*(void **)(*Param++)); Node; Node = Node->next) {
			ml_list_put(List, to_value(Node->data, NULL));
		}
		*Arg++ = List;
		break;
	}
	case GIB_SLIST: {
		ml_value_t *(*to_value)(void *, void *) = NULL;
		switch ((Inst++)->Opcode) {
		case GIB_BOOLEAN: to_value = boolean_to_value; break;
		case GIB_INT8: to_value = int8_to_value; break;
		case GIB_UINT8: to_value = uint8_to_value; break;
		case GIB_INT16: to_value = int16_to_value; break;
		case GIB_UINT16: to_value = uint16_to_value; break;
		case GIB_INT32: to_value = int32_to_value; break;
		case GIB_UINT32: to_value = uint32_to_value; break;
		case GIB_INT64: to_value = int64_to_value; break;
		case GIB_UINT64: to_value = uint64_to_value; break;
		case GIB_FLOAT: to_value = float_to_value; break;
		case GIB_DOUBLE: to_value = double_to_value; break;
		case GIB_STRING: to_value = string_to_value; break;
		case GIB_GTYPE: to_value = gtype_to_value; break;
		//case GIB_BYTES: to_value = bytes_to_value; break;
		default: to_value = unknown_to_value; break;
		}
		ml_value_t *List = ml_list();
		for (GSList *Node = (GSList *)(*(void **)(*Param++)); Node; Node = Node->next) {
			ml_list_put(List, to_value(Node->data, NULL));
		}
		*Arg++ = List;
		break;
	}
	case GIB_HASH: {
		ml_value_t *Map = ml_map();
		GHashTable *Node;
		++Param; // TODO: Populate Map properly
		*Arg++ = Map;
		break;
	}
	}
	ml_result_state_t *State = ml_result_state(Instance->Context);
	ml_call(State, Instance->Function, Arg - Args, Args);
	ml_scheduler_t *Scheduler = ml_context_get(Instance->Context, ML_SCHEDULER_INDEX);
	while (!State->Value) Scheduler->run(Scheduler);
	ml_value_t *Result = State->Value;
	if (ml_is_error(Result)) ML_LOG_ERROR(Result, "Callback returned error");
	for (gi_inst_t *Inst = Callback->InstOut; Inst->Opcode != GIB_DONE;) switch ((Inst++)->Opcode) {
	case GIB_BOOLEAN: *(gboolean *)Return = ml_boolean_value(Result); break;
	case GIB_INT8: *(gint8 *)Return = ml_integer_value(Result); break;
	case GIB_UINT8: *(guint8 *)Return = ml_integer_value(Result); break;
	case GIB_INT16: *(gint16 *)Return = ml_integer_value(Result); break;
	case GIB_UINT16: *(guint16 *)Return = ml_integer_value(Result); break;
	case GIB_INT32: *(gint32 *)Return = ml_integer_value(Result); break;
	case GIB_UINT32: *(guint32 *)Return = ml_integer_value(Result); break;
	case GIB_INT64: *(gint64 *)Return = ml_integer_value(Result); break;
	case GIB_UINT64: *(guint64 *)Return = ml_integer_value(Result); break;
	case GIB_FLOAT: *(gfloat *)Return = ml_real_value(Result); break;
	case GIB_DOUBLE: *(gdouble *)Return = ml_real_value(Result); break;
	case GIB_STRING: {
		ml_value_t *Value = Result;
		if (Value == MLNil) {
			*(gchararray *)Return = NULL;
		} else if (!ml_is(Value, MLStringT)) {
		} else {
			*(gchararray *)Return = (char *)ml_string_value(Value);
		}
		break;
	}
	}
}

typedef struct {
	GIArgInfo Info[1];
	GITypeInfo Type[1];
	GIDirection Direction;
	int SkipIn, SkipOut;
	int In, Out, Value;
} arg_info_t;

static GIBaseInfo *GValueInfo;

#define BASIC_CASES_SIZE(NUM) \
	case GI_TYPE_TAG_BOOLEAN: \
	case GI_TYPE_TAG_INT8: \
	case GI_TYPE_TAG_UINT8: \
	case GI_TYPE_TAG_INT16: \
	case GI_TYPE_TAG_UINT16: \
	case GI_TYPE_TAG_INT32: \
	case GI_TYPE_TAG_UINT32: \
	case GI_TYPE_TAG_INT64: \
	case GI_TYPE_TAG_UINT64: \
	case GI_TYPE_TAG_FLOAT: \
	case GI_TYPE_TAG_DOUBLE: \
	case GI_TYPE_TAG_GTYPE: \
	case GI_TYPE_TAG_UTF8: \
	case GI_TYPE_TAG_FILENAME: \
	case GI_TYPE_TAG_UNICHAR: (NUM++); break;

#define BASIC_CASES_INST(DEST) \
	case GI_TYPE_TAG_BOOLEAN: (DEST++)->Opcode = GIB_BOOLEAN; break; \
	case GI_TYPE_TAG_INT8: (DEST++)->Opcode = GIB_INT8; break; \
	case GI_TYPE_TAG_UINT8: (DEST++)->Opcode = GIB_UINT8; break; \
	case GI_TYPE_TAG_INT16: (DEST++)->Opcode = GIB_INT16; break; \
	case GI_TYPE_TAG_UINT16: (DEST++)->Opcode = GIB_UINT16; break; \
	case GI_TYPE_TAG_INT32: (DEST++)->Opcode = GIB_INT32; break; \
	case GI_TYPE_TAG_UINT32: (DEST++)->Opcode = GIB_UINT32; break; \
	case GI_TYPE_TAG_INT64: (DEST++)->Opcode = GIB_INT64; break; \
	case GI_TYPE_TAG_UINT64: (DEST++)->Opcode = GIB_UINT64; break; \
	case GI_TYPE_TAG_FLOAT: (DEST++)->Opcode = GIB_FLOAT; break; \
	case GI_TYPE_TAG_DOUBLE: (DEST++)->Opcode = GIB_DOUBLE; break; \
	case GI_TYPE_TAG_GTYPE: (DEST++)->Opcode = GIB_GTYPE; break; \
	case GI_TYPE_TAG_UTF8: (DEST++)->Opcode = GIB_STRING; break; \
	case GI_TYPE_TAG_FILENAME: (DEST++)->Opcode = GIB_STRING; break; \
	case GI_TYPE_TAG_UNICHAR: (DEST++)->Opcode = GIB_UINT32; break;

static ml_type_t *callback_info_compile(const char *TypeName, GICallbackInfo *Info) {
	int NumArgs = g_callable_info_get_n_args(Info);
	arg_info_t Args[NumArgs];
	memset(Args, 0, NumArgs * sizeof(arg_info_t));
	int NumAux = 0, Provided = 0;
	int InSize = 1, OutSize = 1;
	for (int I = 0; I < NumArgs; ++I) {
		g_callable_info_load_arg(Info, I, Args[I].Info);
		GIArgInfo *ArgInfo = Args[I].Info;
		g_arg_info_load_type(ArgInfo, Args[I].Type);
		int Closure = g_arg_info_get_closure(ArgInfo);
		if (Closure >= 0) {
			Args[I].SkipIn = 1;
		} else {
			Provided++;
			switch (g_type_info_get_tag(Args[I].Type)) {
			case GI_TYPE_TAG_VOID:
				Provided--;
				Args[I].SkipIn = 1;
				break;
			case GI_TYPE_TAG_ARRAY: {
				int Length = g_type_info_get_array_length(Args[I].Type);
				int Fixed = g_type_info_get_array_fixed_size(Args[I].Type);
				if (Length >= 0) {
					Provided--;
					Args[Length].SkipIn = 1;
					InSize += 1;
				} else if (Fixed >= 0) {
					NumAux++;
					InSize += 1;
				}
				InSize += 2;
				break;
			}
			case GI_TYPE_TAG_INTERFACE: {
				GIBaseInfo *InterfaceInfo = g_type_info_get_interface(Args[I].Type);
				NumAux++;
				if (g_base_info_get_type(InterfaceInfo) == GI_INFO_TYPE_CALLBACK) {
					int Closure = g_arg_info_get_closure(ArgInfo);
					if (Closure >= 0) Args[Closure].SkipIn = 1;
				}
				InSize += 2;
				g_base_info_unref(InterfaceInfo);
				break;
			}
			case GI_TYPE_TAG_GLIST:
			case GI_TYPE_TAG_GSLIST:
			case GI_TYPE_TAG_GHASH:
				InSize += 2;
				break;
			default:
				InSize += 1;
				break;
			}
		}
	}
	GITypeInfo *Return = g_callable_info_get_return_type((GICallableInfo *)Info);
	switch (g_type_info_get_tag(Return)) {
	case GI_TYPE_TAG_VOID:
		break;
	 case GI_TYPE_TAG_INTERFACE:
		OutSize += 2;
		NumAux++;
		break;
	case GI_TYPE_TAG_ARRAY:
	case GI_TYPE_TAG_GLIST:
	case GI_TYPE_TAG_GSLIST:
		OutSize += 2;
		break;
	case GI_TYPE_TAG_GHASH:
		OutSize += 3;
		break;
	default:
		OutSize += 1;
		break;
	}
	callback_t *Callback = xnew(callback_t, NumAux, void *);
	Callback->Base.Type = GirCallbackT;
	Callback->Base.Name = TypeName;
	Callback->Base.hash = ml_default_hash;
	Callback->Base.call = ml_default_call;
	Callback->Base.deref = ml_default_deref;
	Callback->Base.assign = ml_default_assign;
	ml_type_init((ml_type_t *)Callback, GirCallbackInstanceT, NULL);
	Callback->Info = Info;
	g_base_info_ref(Info);
	Callback->Provided = Provided;
	gi_inst_t *InstIn = Callback->InstIn = (gi_inst_t *)snew(InSize * sizeof(gi_inst_t));
	gi_inst_t *InstOut = Callback->InstOut = (gi_inst_t *)snew(OutSize * sizeof(gi_inst_t));
	NumAux = 0;
	switch (g_type_info_get_tag(Return)) {
	case GI_TYPE_TAG_VOID:
		break;
	BASIC_CASES_INST(InstOut)
	case GI_TYPE_TAG_ARRAY: {
		GITypeInfo *ElementInfo = g_type_info_get_param_type(Return, 0);
		if (g_type_info_is_zero_terminated(Return)) {
			(InstOut++)->Opcode = GIB_ARRAY_ZERO;
		} else {
			(InstOut++)->Opcode = GIB_ARRAY;
		}
		switch (g_type_info_get_tag(ElementInfo)) {
		BASIC_CASES_INST(InstOut)
		default: // TODO: handle this.
		}
		g_base_info_unref(ElementInfo);
		break;
	}
	case GI_TYPE_TAG_INTERFACE: {
		GIBaseInfo *InterfaceInfo = g_type_info_get_interface(Return);
		switch (g_base_info_get_type(InterfaceInfo)) {
		case GI_INFO_TYPE_CALLBACK: {
			(InstOut++)->Opcode = GIB_CALLBACK;
			(InstOut++)->Aux = NumAux;
			Callback->Aux[NumAux++] = callback_info_lookup((GICallbackInfo *)InterfaceInfo);
			break;
		}
		case GI_INFO_TYPE_STRUCT: {
			(InstOut++)->Opcode = GIB_STRUCT;
			(InstOut++)->Aux = NumAux;
			Callback->Aux[NumAux++] = struct_info_lookup((GIStructInfo *)InterfaceInfo);
			break;
		}
		case GI_INFO_TYPE_UNION: {
			(InstOut++)->Opcode = GIB_UNION;
			(InstOut++)->Aux = NumAux;
			Callback->Aux[NumAux++] = union_info_lookup((GIUnionInfo *)InterfaceInfo);
			break;
		}
		case GI_INFO_TYPE_ENUM:
		case GI_INFO_TYPE_FLAGS: {
			(InstOut++)->Opcode = GIB_ENUM;
			(InstOut++)->Aux = NumAux;
			Callback->Aux[NumAux++] = enum_info_lookup((GIEnumInfo *)InterfaceInfo);
			break;
		}
		case GI_INFO_TYPE_OBJECT: {
			(InstOut++)->Opcode = GIB_OBJECT;
			(InstOut++)->Aux = NumAux;
			Callback->Aux[NumAux++] = object_info_lookup((GIObjectInfo *)InterfaceInfo);
			break;
		}
		case GI_INFO_TYPE_INTERFACE: {
			(InstOut++)->Opcode = GIB_OBJECT;
			(InstOut++)->Aux = NumAux;
			Callback->Aux[NumAux++] = interface_info_lookup((GIInterfaceInfo *)InterfaceInfo);
			break;
		}
		default: {
			// TODO: raise error or add support
		}
		}
		g_base_info_unref(InterfaceInfo);
		break;
	}
	case GI_TYPE_TAG_GLIST: {
		(InstOut++)->Opcode = GIB_LIST;
		GITypeInfo *ElementInfo = g_type_info_get_param_type(Return, 0);
		switch (g_type_info_get_tag(ElementInfo)) {
		BASIC_CASES_INST(InstOut)
		default: // TODO: handle this.
		}
		g_base_info_unref(ElementInfo);
		break;
	}
	case GI_TYPE_TAG_GSLIST: {
		(InstOut++)->Opcode = GIB_SLIST;
		GITypeInfo *ElementInfo = g_type_info_get_param_type(Return, 0);
		switch (g_type_info_get_tag(ElementInfo)) {
		BASIC_CASES_INST(InstOut)
		default: // TODO: handle this.
		}
		g_base_info_unref(ElementInfo);
		break;
	}
	case GI_TYPE_TAG_GHASH: {
		(InstOut++)->Opcode = GIB_HASH;
		GITypeInfo *ElementInfo = g_type_info_get_param_type(Return, 0);
		switch (g_type_info_get_tag(ElementInfo)) {
		BASIC_CASES_INST(InstOut)
		default: // TODO: handle this.
		}
		g_base_info_unref(ElementInfo);
		break;
	}
	default: // TODO: handle this.
	}
	g_base_info_unref(Return);
	for (int I = 0; I < NumArgs; ++I) {
		if (Args[I].SkipIn) {
			(InstIn++)->Opcode = GIB_SKIP;
		} else switch (g_type_info_get_tag(Args[I].Type)) {
		case GI_TYPE_TAG_VOID:
			(InstIn++)->Opcode = GIB_SKIP;
			break;
		BASIC_CASES_INST(InstIn)
		case GI_TYPE_TAG_ARRAY: {
			int Length = g_type_info_get_array_length(Args[I].Type);
			int Fixed = g_type_info_get_array_fixed_size(Args[I].Type);
			//g_type_info_is_zero_terminated(Args[I].Type);
			if (Length >= 0) {
				(InstIn++)->Opcode = GIB_ARRAY_LENGTH;
				(InstIn++)->Aux = Length;
			} else if (Fixed >= 0) {
				(InstIn++)->Opcode = GIB_ARRAY_FIXED;
				(InstIn++)->Aux = NumAux;
				Callback->Aux[NumAux++] = (void *)0 + Fixed;
			} else if (g_type_info_is_zero_terminated(Args[I].Type)) {
				(InstIn++)->Opcode = GIB_ARRAY_ZERO;
			} else {
				(InstIn++)->Opcode = GIB_ARRAY;
			}
			GITypeInfo *ElementInfo = g_type_info_get_param_type(Args[I].Type, 0);
			switch (g_type_info_get_tag(ElementInfo)) {
			BASIC_CASES_INST(InstIn)
			default: // TODO: handle this.
			}
			g_base_info_unref(ElementInfo);
			break;
		}
		case GI_TYPE_TAG_INTERFACE: {
			GIBaseInfo *InterfaceInfo = g_type_info_get_interface(Args[I].Type);
			switch (g_base_info_get_type(InterfaceInfo)) {
			case GI_INFO_TYPE_CALLBACK: {
				(InstIn++)->Opcode = GIB_CALLBACK;
				(InstIn++)->Aux = NumAux;
				Callback->Aux[NumAux++] = callback_info_lookup((GICallbackInfo *)InterfaceInfo);
				break;
			}
			case GI_INFO_TYPE_STRUCT: {
				(InstIn++)->Opcode = GIB_STRUCT;
				(InstIn++)->Aux = NumAux;
				Callback->Aux[NumAux++] = struct_info_lookup((GIStructInfo *)InterfaceInfo);
				break;
			}
			case GI_INFO_TYPE_UNION: {
				(InstIn++)->Opcode = GIB_UNION;
				(InstIn++)->Aux = NumAux;
				Callback->Aux[NumAux++] = union_info_lookup((GIUnionInfo *)InterfaceInfo);
				break;
			}
			case GI_INFO_TYPE_ENUM:
			case GI_INFO_TYPE_FLAGS: {
				(InstIn++)->Opcode = GIB_ENUM;
				(InstIn++)->Aux = NumAux;
				Callback->Aux[NumAux++] = enum_info_lookup((GIEnumInfo *)InterfaceInfo);
				break;
			}
			case GI_INFO_TYPE_OBJECT: {
				(InstIn++)->Opcode = GIB_OBJECT;
				(InstIn++)->Aux = NumAux;
				Callback->Aux[NumAux++] = object_info_lookup((GIObjectInfo *)InterfaceInfo);
				break;
			}
			case GI_INFO_TYPE_INTERFACE: {
				(InstIn++)->Opcode = GIB_OBJECT;
				(InstIn++)->Aux = NumAux;
				Callback->Aux[NumAux++] = interface_info_lookup((GIInterfaceInfo *)InterfaceInfo);
				break;
			}
			default: {
				// TODO: raise error or add support
			}
			}
			g_base_info_unref(InterfaceInfo);
			break;
		}
		case GI_TYPE_TAG_GLIST: {
			(InstIn++)->Opcode = GIB_LIST;
			GITypeInfo *ElementInfo = g_type_info_get_param_type(Args[I].Type, 0);
			switch (g_type_info_get_tag(ElementInfo)) {
			BASIC_CASES_INST(InstIn)
			default: // TODO: handle this.
			}
			g_base_info_unref(ElementInfo);
			break;
		}
		case GI_TYPE_TAG_GSLIST: {
			(InstIn++)->Opcode = GIB_SLIST;
			GITypeInfo *ElementInfo = g_type_info_get_param_type(Args[I].Type, 0);
			switch (g_type_info_get_tag(ElementInfo)) {
			BASIC_CASES_INST(InstIn)
			default: // TODO: handle this.
			}
			g_base_info_unref(ElementInfo);
			break;
		}
		case GI_TYPE_TAG_GHASH: {
			(InstIn++)->Opcode = GIB_HASH;
			GITypeInfo *ElementInfo = g_type_info_get_param_type(Args[I].Type, 0);
			switch (g_type_info_get_tag(ElementInfo)) {
			BASIC_CASES_INST(InstIn)
			default: // TODO: handle this.
			}
			g_base_info_unref(ElementInfo);
			break;
		}
		default:
		}
	}
	InstIn->Opcode = InstOut->Opcode = GIB_DONE;
	return (ml_type_t *)Callback;
}

static ml_type_t *callback_info_lookup(GICallbackInfo *Info) {
	const char *TypeName = g_base_info_get_name((GIBaseInfo *)Info);
	ml_type_t **Slot = (ml_type_t **)stringmap_slot(TypeMap, TypeName);
	if (!Slot[0]) Slot[0] = callback_info_compile(TypeName, Info);
	return Slot[0];
}

typedef ml_value_t *(*ptr_to_value_fn)(void *);

typedef struct {
	ml_value_t *(*to_value)(void *, void *);
	void *Aux;
	int Size;
} ptr_to_value_t;

static void ptr_to_value(gi_inst_t **Inst, void **Aux, ptr_to_value_t *Convert) {
	gi_opcode_t Opcode = ((*Inst)++)->Opcode;
	switch (Opcode) {
	case GIB_BOOLEAN:
		Convert->to_value = boolean_to_value;
		Convert->Size = sizeof(gboolean);
		break;
	case GIB_INT8:
		Convert->to_value = int8_to_value;
		Convert->Size = sizeof(gint8);
		break;
	case GIB_UINT8:
		Convert->to_value = uint8_to_value;
		Convert->Size = sizeof(guint8);
		break;
	case GIB_INT16:
		Convert->to_value = int16_to_value;
		Convert->Size = sizeof(gint16);
		break;
	case GIB_UINT16:
		Convert->to_value = uint16_to_value;
		Convert->Size = sizeof(guint16);
		break;
	case GIB_INT32:
		Convert->to_value = int32_to_value;
		Convert->Size = sizeof(gint32);
		break;
	case GIB_UINT32:
		Convert->to_value = uint32_to_value;
		Convert->Size = sizeof(guint32);
		break;
	case GIB_INT64:
		Convert->to_value = int64_to_value;
		Convert->Size = sizeof(gint64);
		break;
	case GIB_UINT64:
		Convert->to_value = uint64_to_value;
		Convert->Size = sizeof(guint64);
		break;
	case GIB_FLOAT:
		Convert->to_value = float_to_value;
		Convert->Size = sizeof(gfloat);
		break;
	case GIB_DOUBLE:
		Convert->to_value = double_to_value;
		Convert->Size = sizeof(gdouble);
		break;
	case GIB_STRING:
		Convert->to_value = string_to_value;
		Convert->Size = sizeof(gchararray);
		break;
	case GIB_GTYPE:
		Convert->to_value = gtype_to_value;
		Convert->Size = sizeof(GType);
		break;
	}
}

typedef struct {
	ml_type_t *Type;
	GIFunctionInfo *Info;
	gi_inst_t *InstIn, *InstOut;
	int NumArgsIn, NumArgsOut;
	int NumValues, NumInputs;
	int NumOutputs, NumResults;
	void *Aux[];
} gir_function_t;

typedef struct {
	ml_value_t *Map;
	ptr_to_value_t Key[1];
	ptr_to_value_t Value[1];
} ghash_to_map_t;

static void ghash_to_map(gpointer Key, gpointer Value, ghash_to_map_t *Convert) {
	ml_map_insert(Convert->Map,
		Convert->Key->to_value(Key, Convert->Key->Aux),
		Convert->Value->to_value(Value, Convert->Value->Aux)
	);
}

static void gir_function_call(ml_state_t *Caller, gir_function_t *Function, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(Function->NumInputs);
	size_t NumArgs = Function->NumArgsIn + Function->NumArgsOut + Function->NumOutputs;
	GIArgument Arguments[NumArgs];
	memset(Arguments, 0, NumArgs * sizeof(GIArgument));
	GValue Values[Function->NumValues];
	memset(Values, 0, Function->NumValues * sizeof(GValue));
	GIArgument *ArgIn = Arguments, *ArgOut = ArgIn + Function->NumArgsIn, *Outputs = ArgOut + Function->NumArgsOut + 1;
	GValue *ValuePtr = Values;
	ml_value_t **Arg = Args;
	for (int I = 0; I < Count; ++I) Args[I] = ml_deref(Args[I]);
	for (gi_inst_t *Inst = Function->InstIn; Inst->Opcode != GIB_DONE;) switch ((Inst++)->Opcode) {
	case GIB_SELF: {
		ml_value_t *Value = *Arg++;
		(ArgIn++)->v_pointer = ((object_instance_t *)Value)->Handle;
		break;
	}
	case GIB_SKIP: ArgIn++; break;
	case GIB_BOOLEAN: (ArgIn++)->v_boolean = ml_boolean_value(*Arg++); break;
	case GIB_INT8: (ArgIn++)->v_int8 = ml_integer_value(*Arg++); break;
	case GIB_UINT8: (ArgIn++)->v_uint8 = ml_integer_value(*Arg++); break;
	case GIB_INT16: (ArgIn++)->v_int16 = ml_integer_value(*Arg++); break;
	case GIB_UINT16: (ArgIn++)->v_uint16 = ml_integer_value(*Arg++); break;
	case GIB_INT32: (ArgIn++)->v_int32 = ml_integer_value(*Arg++); break;
	case GIB_UINT32: (ArgIn++)->v_uint32 = ml_integer_value(*Arg++); break;
	case GIB_INT64: (ArgIn++)->v_int64 = ml_integer_value(*Arg++); break;
	case GIB_UINT64: (ArgIn++)->v_uint64 = ml_integer_value(*Arg++); break;
	case GIB_FLOAT: (ArgIn++)->v_float = ml_real_value(*Arg++); break;
	case GIB_DOUBLE: (ArgIn++)->v_double = ml_real_value(*Arg++); break;
	case GIB_STRING: {
		ml_value_t *Value = *Arg++;
		if (Value == MLNil) {
			(ArgIn++)->v_string = NULL;
		} else if (!ml_is(Value, MLStringT)) {
			ML_ERROR("TypeError", "Expected string for argument %ld", Arg - Args);
		} else {
			(ArgIn++)->v_string = (char *)ml_string_value(Value);
		}
		break;
	}
	case GIB_GTYPE: {
		ml_value_t *Value = *Arg++;
		if (ml_is(Value, GirBaseInfoT)) {
			baseinfo_t *Base = (baseinfo_t *)Value;
			(ArgIn++)->v_size = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)Base->Info);
		} else if (ml_is(Value, MLStringT)) {
			(ArgIn++)->v_size = g_type_from_name(ml_string_value(Value));
		} else if (Value == (ml_value_t *)MLNilT) {
			(ArgIn++)->v_size = G_TYPE_NONE;
		} else if (Value == (ml_value_t *)MLIntegerT) {
			(ArgIn++)->v_size = G_TYPE_INT64;
		} else if (Value == (ml_value_t *)MLStringT) {
			(ArgIn++)->v_size = G_TYPE_STRING;
		} else if (Value == (ml_value_t *)MLDoubleT) {
			(ArgIn++)->v_size = G_TYPE_DOUBLE;
		} else if (Value == (ml_value_t *)MLBooleanT) {
			(ArgIn++)->v_size = G_TYPE_BOOLEAN;
		} else if (Value == (ml_value_t *)MLAddressT) {
			(ArgIn++)->v_size = G_TYPE_POINTER;
		} else {
			ML_ERROR("TypeError", "Expected type for parameter %ld", Arg - Args);
		}
		break;
	}
	/*case GIB_BYTES: {
		ml_value_t *Value = *Arg++;
		void *Bytes;
		if (Value == MLNil) {
			Bytes = NULL;
		} else if (ml_is(Value, MLAddressT)) {
			Bytes = (void *)ml_address_value(Value);
		} else if (ml_is(Value, MLListT)) {
			size_t Length = ml_list_length(Value);
			Bytes = snew(Length + 1);
			void *Ptr = Bytes;
			ML_LIST_FOREACH(Value, Iter) {
				ml_value_t *Error = int8_to_array(Iter->Value, Ptr);
				if (Error) ML_RETURN(Error);
				Ptr += 1;
			}
		} else {
			ML_ERROR("TypeError", "Expected list or address for parameter %ld", Arg - Args);
		}
		(Argument++)->v_pointer = Bytes;
		break;
	}*/
	case GIB_ARRAY:
	case GIB_ARRAY_ZERO: {
		ml_value_t *Value = *Arg++;
		ml_value_t *(*to_array)(ml_value_t *, void *);
		int Size;
		switch ((Inst++)->Opcode) {
		case GIB_BOOLEAN: to_array = boolean_to_array; Size = sizeof(gboolean); break;
		case GIB_INT8: to_array = int8_to_array; Size = sizeof(gint8); break;
		case GIB_UINT8: to_array = uint8_to_array; Size = sizeof(guint8); break;
		case GIB_INT16: to_array = int16_to_array; Size = sizeof(gint16); break;
		case GIB_UINT16: to_array = uint16_to_array; Size = sizeof(guint16); break;
		case GIB_INT32: to_array = int32_to_array; Size = sizeof(gint32); break;
		case GIB_UINT32: to_array = uint32_to_array; Size = sizeof(guint32); break;
		case GIB_INT64: to_array = int64_to_array; Size = sizeof(gint64); break;
		case GIB_UINT64: to_array = uint64_to_array; Size = sizeof(guint64); break;
		case GIB_FLOAT: to_array = float_to_array; Size = sizeof(gfloat); break;
		case GIB_DOUBLE: to_array = double_to_array; Size = sizeof(gdouble); break;
		case GIB_STRING: to_array = string_to_array; Size = sizeof(gchararray); break;
		case GIB_GTYPE: to_array = gtype_to_array; Size = sizeof(GType); break;
		//case GIB_BYTES: to_array = bytes_to_array; Size = sizeof(gpointer); break;
		default: ML_ERROR("TypeError", "Unsupported array type");
		}
		void *Array;
		if (Value == MLNil) {
			Array = NULL;
		} else if (ml_is(Value, MLListT)) {
			size_t Length = ml_list_length(Value);
			Array = snew((Length + 1) * Size);
			void *Ptr = Array;
			ML_LIST_FOREACH(Value, Iter) {
				ml_value_t *Error = to_array(Iter->Value, Ptr);
				if (Error) ML_RETURN(Error);
				Ptr += Size;
			}
		} else if (ml_is(Value, MLAddressT)) {
			Array = (void *)ml_address_value(Value);
		} else {
			ML_ERROR("TypeError", "Expected list for parameter %ld", Arg - Args);
		}
		(ArgIn++)->v_pointer = Array;
		break;
	}
	case GIB_ARRAY_LENGTH: {
		ml_value_t *Value = *Arg++;
		int Aux = (Inst++)->Aux;
		ml_value_t *(*to_array)(ml_value_t *, void *);
		int Size;
		switch ((Inst++)->Opcode) {
		case GIB_BOOLEAN: to_array = boolean_to_array; Size = sizeof(gboolean); break;
		case GIB_INT8: to_array = int8_to_array; Size = sizeof(gint8); break;
		case GIB_UINT8: to_array = uint8_to_array; Size = sizeof(guint8); break;
		case GIB_INT16: to_array = int16_to_array; Size = sizeof(gint16); break;
		case GIB_UINT16: to_array = uint16_to_array; Size = sizeof(guint16); break;
		case GIB_INT32: to_array = int32_to_array; Size = sizeof(gint32); break;
		case GIB_UINT32: to_array = uint32_to_array; Size = sizeof(guint32); break;
		case GIB_INT64: to_array = int64_to_array; Size = sizeof(gint64); break;
		case GIB_UINT64: to_array = uint64_to_array; Size = sizeof(guint64); break;
		case GIB_FLOAT: to_array = float_to_array; Size = sizeof(gfloat); break;
		case GIB_DOUBLE: to_array = double_to_array; Size = sizeof(gdouble); break;
		case GIB_STRING: to_array = string_to_array; Size = sizeof(gchararray); break;
		case GIB_GTYPE: to_array = gtype_to_array; Size = sizeof(GType); break;
		//case GIB_BYTES: to_array = bytes_to_array; Size = sizeof(gpointer); break;
		default: ML_ERROR("TypeError", "Unsupported array type");
		}
		void *Array;
		if (Value == MLNil) {
			Array = NULL;
			Arguments[Aux].v_int64 = 0;
		} else if (ml_is(Value, MLListT)) {
			size_t Length = ml_list_length(Value);
			Arguments[Aux].v_int64 = Length;
			Array = snew((Length + 1) * Size);
			void *Ptr = Array;
			ML_LIST_FOREACH(Value, Iter) {
				ml_value_t *Error = to_array(Iter->Value, Ptr);
				if (Error) ML_RETURN(Error);
				Ptr += Size;
			}
		} else if (ml_is(Value, MLAddressT)) {
			size_t Length = ml_address_length(Value);
			Arguments[Aux].v_int64 = Length / Size;
			Array = (void *)ml_address_value(Value);
		} else {
			ML_ERROR("TypeError", "Expected list for parameter %ld", Arg - Args);
		}
		(ArgIn++)->v_pointer = Array;
		break;
	}
	case GIB_CALLBACK: {
		ml_value_t *Value = *Arg++;
		callback_t *Type = (callback_t *)Function->Aux[(Inst++)->Aux];
		callback_instance_t *Instance = (callback_instance_t *)GC_MALLOC_UNCOLLECTABLE(sizeof(callback_instance_t));
		Instance->Type = (ml_type_t *)Type;
		Instance->Context = Caller->Context;
		Instance->Function = Value;
		(ArgIn++)->v_pointer = g_callable_info_create_closure(
			Type->Info,
			Instance->Cif,
			(GIFFIClosureCallback)callback_fn,
			Instance
		);
		break;
	}
	case GIB_STRUCT: {
		ml_value_t *Value = *Arg++;
		ml_type_t *Type = (ml_type_t *)Function->Aux[(Inst++)->Aux];
		if (Value == MLNil) {
			(ArgIn++)->v_pointer = NULL;
		} else if (ml_is(Value, Type)) {
			(ArgIn++)->v_pointer = ((struct_instance_t *)Value)->Value;
		} else {
			ML_ERROR("TypeError", "Expected %s not %s for parameter %ld", Type->Name, ml_typeof(Value)->Name, Arg - Args);
		}
		break;
	}
	case GIB_UNION: {
		ml_value_t *Value = *Arg++;
		ml_type_t *Type = (ml_type_t *)Function->Aux[(Inst++)->Aux];
		if (Value == MLNil) {
			(ArgIn++)->v_pointer = NULL;
		} else if (ml_is(Value, Type)) {
			(ArgIn++)->v_pointer = ((union_instance_t *)Value)->Value;
		} else {
			ML_ERROR("TypeError", "Expected %s not %s for parameter %ld", Type->Name, ml_typeof(Value)->Name, Arg - Args);
		}
		break;
	}
	case GIB_ENUM: {
		ml_value_t *Value = *Arg++;
		ml_type_t *Type = (ml_type_t *)Function->Aux[(Inst++)->Aux];
		if (Value == MLNil) {
			(ArgIn++)->v_int64 = 0;
		} else if (ml_is(Value, Type)) {
			(ArgIn++)->v_int64 = ((enum_value_t *)Value)->Value;
		} else {
			ML_ERROR("TypeError", "Expected %s not %s for parameter %ld", Type->Name, ml_typeof(Value)->Name, Arg - Args);
		}
		break;
	}
	case GIB_OBJECT: {
		ml_value_t *Value = *Arg++;
		ml_type_t *Type = (ml_type_t *)Function->Aux[(Inst++)->Aux];
		if (Value == MLNil) {
			(ArgIn++)->v_pointer = NULL;
		} else if (ml_is(Value, Type)) {
			(ArgIn++)->v_pointer = ((object_instance_t *)Value)->Handle;
		} else {
			ML_ERROR("TypeError", "Expected %s not %s for parameter %ld", Type->Name, ml_typeof(Value)->Name, Arg - Args);
		}
		break;
	}
	case GIB_VALUE: {
		ml_value_t *Value = *Arg++;
		to_gvalue(Value, ValuePtr);
		(ArgIn++)->v_pointer = ValuePtr++;
		break;
	}
	case GIB_LIST: {
		ml_value_t *Value = *Arg++;
		ml_value_t *(*to_list)(ml_value_t *, void **);
		switch ((Inst++)->Opcode) {
		case GIB_BOOLEAN: to_list = boolean_to_glist; break;
		case GIB_INT8: to_list = int8_to_glist; break;
		case GIB_UINT8: to_list = uint8_to_glist; break;
		case GIB_INT16: to_list = int16_to_glist; break;
		case GIB_UINT16: to_list = uint16_to_glist; break;
		case GIB_INT32: to_list = int32_to_glist; break;
		case GIB_UINT32: to_list = uint32_to_glist; break;
		case GIB_INT64: to_list = int64_to_glist; break;
		case GIB_UINT64: to_list = uint64_to_glist; break;
		case GIB_FLOAT: to_list = float_to_glist; break;
		case GIB_DOUBLE: to_list = double_to_glist; break;
		case GIB_STRING: to_list = string_to_glist; break;
		case GIB_GTYPE: to_list = gtype_to_glist; break;
		//case GIB_BYTES: to_list = bytes_to_glist; break;
		default: ML_ERROR("TypeError", "Unsupported list type");
		}
		GList *List = NULL, **Slot = &List, *Prev = NULL;
		if (ml_is(Value, MLListT)) {
			ML_LIST_FOREACH(Value, Iter) {
				GList *Node = Slot[0] = g_list_alloc();
				ml_value_t *Error = to_list(Iter->Value, &Node->data);
				if (Error) ML_RETURN(Error);
				Slot = &Node->next;
				Node->prev = Prev;
				Prev = Node;
			}
		}
		(ArgIn++)->v_pointer = List;
		break;
	}
	case GIB_SLIST: {
		ml_value_t *Value = *Arg++;
		ml_value_t *(*to_list)(ml_value_t *, void **);
		switch ((Inst++)->Opcode) {
		case GIB_BOOLEAN: to_list = boolean_to_glist; break;
		case GIB_INT8: to_list = int8_to_glist; break;
		case GIB_UINT8: to_list = uint8_to_glist; break;
		case GIB_INT16: to_list = int16_to_glist; break;
		case GIB_UINT16: to_list = uint16_to_glist; break;
		case GIB_INT32: to_list = int32_to_glist; break;
		case GIB_UINT32: to_list = uint32_to_glist; break;
		case GIB_INT64: to_list = int64_to_glist; break;
		case GIB_UINT64: to_list = uint64_to_glist; break;
		case GIB_FLOAT: to_list = float_to_glist; break;
		case GIB_DOUBLE: to_list = double_to_glist; break;
		case GIB_STRING: to_list = string_to_glist; break;
		case GIB_GTYPE: to_list = gtype_to_glist; break;
		//case GIB_BYTES: to_list = bytes_to_glist; break;
		default: ML_ERROR("TypeError", "Unsupported list type");
		}
		GSList *List = NULL, **Slot = &List;
		if (ml_is(Value, MLListT)) {
			ML_LIST_FOREACH(Value, Iter) {
				GSList *Node = Slot[0] = g_slist_alloc();
				ml_value_t *Error = to_list(Iter->Value, &Node->data);
				if (Error) ML_RETURN(Error);
				Slot = &Node->next;
			}
		}
		(ArgIn++)->v_pointer = List;
		break;
	}
	case GIB_HASH: {
		ML_ERROR("TypeError", "Hash arguments not supported yet");
		break;
	}
	case GIB_OUTPUT_VALUE: {
		(ArgOut++)->v_pointer = Outputs++;
		break;
	}
	case GIB_OUTPUT_ARRAY: {
		(ArgOut++)->v_pointer = Outputs++;
		break;
	}
	case GIB_OUTPUT_STRUCT: {
		ml_value_t *Value = *Arg++;
		ml_type_t *Type = (ml_type_t *)Function->Aux[(Inst++)->Aux];
		if (Value == MLNil) {
			(ArgOut++)->v_pointer = NULL;
		} else if (ml_is(Value, Type)) {
			(ArgOut++)->v_pointer = ((struct_instance_t *)Value)->Value;
		} else {
			ML_ERROR("TypeError", "Expected %s not %s for parameter %ld", Type->Name, ml_typeof(Value)->Name, Arg - Args);
		}
		break;
	}
	}
	GError *Error = 0;
	gboolean Invoked = g_function_info_invoke(
		Function->Info,
		Arguments, Function->NumArgsIn,
		ArgIn, Function->NumArgsOut,
		ArgOut,
		&Error
	);
	Outputs = ArgOut;
	if (!Invoked || Error) ML_ERROR("InvokeError", "Error: %s", Error->message);
	ml_value_t *Results, **Result;
	if (Function->NumResults > 1) {
		Results = ml_tuple(Function->NumResults);
		Result = ((ml_tuple_t *)Results)->Values;
	} else {
		Results = MLNil;
		Result = &Results;
	}
	int Free = 0;
	for (gi_inst_t *Inst = Function->InstOut; Inst->Opcode != GIB_DONE;) switch ((Inst++)->Opcode) {
	case GIB_SKIP: ArgOut++; break;
	case GIB_BOOLEAN: *Result++ = ml_boolean((ArgOut++)->v_boolean); break;
	case GIB_INT8: *Result++ = ml_integer((ArgOut++)->v_int8); break;
	case GIB_UINT8: *Result++ = ml_integer((ArgOut++)->v_uint8); break;
	case GIB_INT16: *Result++ = ml_integer((ArgOut++)->v_int16); break;
	case GIB_UINT16: *Result++ = ml_integer((ArgOut++)->v_uint16); break;
	case GIB_INT32: *Result++ = ml_integer((ArgOut++)->v_int32); break;
	case GIB_UINT32: *Result++ = ml_integer((ArgOut++)->v_uint32); break;
	case GIB_INT64: *Result++ = ml_integer((ArgOut++)->v_int64); break;
	case GIB_UINT64: *Result++ = ml_integer((ArgOut++)->v_uint64); break;
	case GIB_FLOAT: *Result++ = ml_real((ArgOut++)->v_float); break;
	case GIB_DOUBLE: *Result++ = ml_real((ArgOut++)->v_double); break;
	case GIB_STRING: *Result++ = ml_string((ArgOut++)->v_string, -1); break;
	case GIB_ARRAY: {
		break;
	}
	case GIB_ARRAY_ZERO: {
		break;
	}
	case GIB_ARRAY_LENGTH: {
		int Aux = (Inst++)->Aux;
		ml_value_t *(*to_value)(void *, void *);
		int Size;
		switch ((Inst++)->Opcode) {
		case GIB_BOOLEAN: to_value = boolean_to_value; Size = sizeof(gboolean); break;
		case GIB_INT8: to_value = int8_to_value; Size = sizeof(gint8); break;
		case GIB_UINT8: to_value = uint8_to_value; Size = sizeof(guint8); break;
		case GIB_INT16: to_value = int16_to_value; Size = sizeof(gint16); break;
		case GIB_UINT16: to_value = uint16_to_value; Size = sizeof(guint16); break;
		case GIB_INT32: to_value = int32_to_value; Size = sizeof(gint32); break;
		case GIB_UINT32: to_value = uint32_to_value; Size = sizeof(guint32); break;
		case GIB_INT64: to_value = int64_to_value; Size = sizeof(gint64); break;
		case GIB_UINT64: to_value = uint64_to_value; Size = sizeof(guint64); break;
		case GIB_FLOAT: to_value = float_to_value; Size = sizeof(gfloat); break;
		case GIB_DOUBLE: to_value = double_to_value; Size = sizeof(gdouble); break;
		case GIB_STRING: to_value = string_to_value; Size = sizeof(gchararray); break;
		case GIB_GTYPE: to_value = gtype_to_value; Size = sizeof(GType); break;
		//case GIB_BYTES: to_value = bytes_to_value; Size = sizeof(gpointer); break;
		default: ML_ERROR("TypeError", "Unsupported array type");
		}
		void *Array = (ArgOut++)->v_pointer;
		if (to_value == int8_to_value || to_value == uint8_to_value) {
			if (Array) {
				size_t Length = Outputs[Aux].v_int64;
				char *Buffer = snew(Length + 1);
				memcpy(Buffer, Array, Length);
				Buffer[Length] = 0;
				*Result++ = ml_address(Buffer, Length);
			} else {
				*Result++ = MLNil;
			}
		} else {
			ml_value_t *List = ml_list();
			if (Array) {
				size_t Length = Outputs[Aux].v_int64;
				void *Ptr = Array;
				for (size_t I = 0; I < Length; ++I) {
					ml_list_put(List, to_value(Ptr, NULL));
					Ptr += Size;
				}
			}
			*Result++ = List;
		}
		if (Free) {
			g_free(Array);
			Free = 0;
		}
		break;
	}
	case GIB_STRUCT: {
		struct_instance_t *Instance = new(struct_instance_t);
		Instance->Type = Function->Aux[(Inst++)->Aux];
		Instance->Value = (ArgOut++)->v_pointer;
		*Result++ = (ml_value_t *)Instance;
		if (Free) {
			// TODO: mark instance for cleanup
			Free = 0;
		}
		break;
	}
	case GIB_UNION: {
		union_instance_t *Instance = new(union_instance_t);
		Instance->Type = Function->Aux[(Inst++)->Aux];
		Instance->Value = (ArgOut++)->v_pointer;
		*Result++ = (ml_value_t *)Instance;
		if (Free) {
			// TODO: mark instance for cleanup
			Free = 0;
		}
		break;
	}
	case GIB_ENUM: {
		enum_t *Enum = (enum_t *)Function->Aux[(Inst++)->Aux];
		*Result++ = (ml_value_t *)Enum->ByIndex[(ArgOut++)->v_int];
		break;
	}
	case GIB_OBJECT: {
		object_t *Object = (object_t *)Function->Aux[(Inst++)->Aux];
		*Result++ = ml_gir_instance_get((ArgOut++)->v_pointer, Object->Info);
		break;
	}
	case GIB_HASH: {
		ghash_to_map_t Convert[1];
		Convert->Map = ml_map();
		ptr_to_value(&Inst, Function->Aux, Convert->Key);
		ptr_to_value(&Inst, Function->Aux, Convert->Value);
		GHashTable *Hash = (GHashTable *)((ArgOut++)->v_pointer);
		g_hash_table_foreach(Hash, (GHFunc)ghash_to_map, Convert);
		*Result++ = Convert->Map;
		if (Free) {
			g_hash_table_destroy(Hash);
			Free = 0;
		}
		break;
	}
	case GIB_FREE: Free = 1; break;
	}
	ML_RETURN(Results);
}

ML_TYPE(GirFunctionT, (MLFunctionT), "gir::function",
	.call = (void *)gir_function_call
);

static ml_value_t * ML_TYPED_FN(ml_method_wrap, GirFunctionT, ml_value_t *Function, int Count, ml_type_t **Types) {
	return Function;
}

static void type_param_size(GITypeInfo *Info, int Index, int *Size, int *NumAux) {
	GITypeInfo *ElementInfo = g_type_info_get_param_type(Info, Index);
	switch (g_type_info_get_tag(ElementInfo)) {
	BASIC_CASES_SIZE((*Size))
	case GI_TYPE_TAG_INTERFACE: *Size += 2; (*NumAux)++; break;
	default: // TODO: handle this.
	}
	g_base_info_unref(ElementInfo);
}

static void type_param_inst(GITypeInfo *Info, int Index, gi_inst_t **Inst, int *NumAux, void **Aux) {
	GITypeInfo *ElementInfo = g_type_info_get_param_type(Info, Index);
	switch (g_type_info_get_tag(ElementInfo)) {
	BASIC_CASES_INST((*Inst))
	case GI_TYPE_TAG_INTERFACE: {
		GIBaseInfo *InterfaceInfo = g_type_info_get_interface(ElementInfo);
		switch (g_base_info_get_type(InterfaceInfo)) {
		case GI_INFO_TYPE_CALLBACK:
			((*Inst)++)->Opcode = GIB_CALLBACK;
			((*Inst)++)->Aux = *NumAux;
			Aux[(*NumAux)++] = callback_info_lookup((GICallbackInfo *)InterfaceInfo);
			break;
		case GI_INFO_TYPE_STRUCT:
			((*Inst)++)->Opcode = GIB_STRUCT;
			((*Inst)++)->Aux = *NumAux;
			Aux[(*NumAux)++] = struct_info_lookup((GIStructInfo *)InterfaceInfo);
			break;
		case GI_INFO_TYPE_UNION:
			((*Inst)++)->Opcode = GIB_UNION;
			((*Inst)++)->Aux = *NumAux;
			Aux[(*NumAux)++] = union_info_lookup((GIUnionInfo *)InterfaceInfo);
			break;
		case GI_INFO_TYPE_ENUM:
		case GI_INFO_TYPE_FLAGS:
			((*Inst)++)->Opcode = GIB_ENUM;
			((*Inst)++)->Aux = *NumAux;
			Aux[(*NumAux)++] = enum_info_lookup((GIEnumInfo *)InterfaceInfo);
			break;
		case GI_INFO_TYPE_OBJECT:
			((*Inst)++)->Opcode = GIB_OBJECT;
			((*Inst)++)->Aux = *NumAux;
			Aux[(*NumAux)++] = object_info_lookup((GIObjectInfo *)InterfaceInfo);
			break;
		case GI_INFO_TYPE_INTERFACE:
			((*Inst)++)->Opcode = GIB_OBJECT;
			((*Inst)++)->Aux = *NumAux;
			Aux[(*NumAux)++] = interface_info_lookup((GIInterfaceInfo *)InterfaceInfo);
			break;
		default: {
			// TODO: raise error or add support
		}
		}
		g_base_info_unref(InterfaceInfo);
		break;
	}
	default: // TODO: handle this.
	}
	g_base_info_unref(ElementInfo);
}

static ml_value_t *function_info_compile(GIFunctionInfo *Info) {
	int NumArgs = g_callable_info_get_n_args(Info);
	arg_info_t Args[NumArgs];
	memset(Args, 0, NumArgs * sizeof(arg_info_t));
	int NumIn = 0, NumOut = 0, NumAux = 0, NumValues = 0, NumInputs = 0, NumOutputs = 1;
	//int InSize = 1, OutSize = 1;
	int InSize = 1, OutSize = 1;
	GIFunctionInfoFlags Flags = g_function_info_get_flags(Info);
	if (Flags & GI_FUNCTION_IS_METHOD) {
		NumInputs++;
		NumIn++;
		InSize++;
	}
	for (int I = 0; I < NumArgs; ++I) {
		g_callable_info_load_arg(Info, I, Args[I].Info);
		GIArgInfo *ArgInfo = Args[I].Info;
		GIDirection Direction = Args[I].Direction = g_arg_info_get_direction(ArgInfo);
		g_arg_info_load_type(ArgInfo, Args[I].Type);
		if (Direction == GI_DIRECTION_IN || Direction == GI_DIRECTION_INOUT) {
			NumInputs++;
			Args[I].In = NumIn++;
			switch (g_type_info_get_tag(Args[I].Type)) {
			case GI_TYPE_TAG_ARRAY: {
				InSize += 1;
				int Length = g_type_info_get_array_length(Args[I].Type);
				if (Length >= 0) {
					NumInputs--;
					Args[Length].SkipIn = 1;
					InSize += 1;
				}
				type_param_size(Args[I].Type, 0, &InSize, &NumAux);
				break;
			}
			case GI_TYPE_TAG_INTERFACE: {
				GIBaseInfo *InterfaceInfo = g_type_info_get_interface(Args[I].Type);
				if (g_base_info_equal(InterfaceInfo, GValueInfo)) {
					Args[I].Value = NumValues++;
					InSize += 2;
				} else if (g_base_info_equal(InterfaceInfo, DestroyNotifyInfo)) {
					NumInputs--;
					Args[I].SkipIn = 1;
					InSize += 1;
				} else {
					NumAux++;
					if (g_base_info_get_type(InterfaceInfo) == GI_INFO_TYPE_CALLBACK) {
						int Closure = g_arg_info_get_closure(ArgInfo);
						if (Closure >= 0) {
							NumInputs--;
							Args[Closure].SkipIn = 1;
						}
					}
					InSize += 2;
				}
				g_base_info_unref(InterfaceInfo);
				break;
			}
			case GI_TYPE_TAG_GLIST:
			case GI_TYPE_TAG_GSLIST:
				InSize += 1;
				type_param_size(Args[I].Type, 0, &InSize, &NumAux);
				break;
			case GI_TYPE_TAG_GHASH:
				InSize += 1;
				type_param_size(Args[I].Type, 0, &InSize, &NumAux);
				type_param_size(Args[I].Type, 1, &InSize, &NumAux);
				break;
			default:
				InSize += 1;
				break;
			}
		}
		if (Direction == GI_DIRECTION_OUT || Direction == GI_DIRECTION_INOUT) {
			NumOut++;
			if (g_arg_info_is_caller_allocates(Args[I].Info)) {
				switch (g_type_info_get_tag(Args[I].Type)) {
				case GI_TYPE_TAG_ARRAY: {
					int Length = g_type_info_get_array_length(Args[I].Type);
					NumInputs++;
					Args[I].In = NumIn++;
					if (Length >= 0) {
						NumInputs--;
						Args[Length].SkipIn = 1;
						InSize += 1;
					}
					InSize += 1;
					type_param_size(Args[I].Type, 0, &InSize, &NumAux);
					break;
				}
				case GI_TYPE_TAG_INTERFACE: {
					GIBaseInfo *InterfaceInfo = g_type_info_get_interface(Args[I].Type);
					if (g_base_info_get_type(InterfaceInfo) == GI_INFO_TYPE_STRUCT) {
						NumInputs++;
						NumAux++;
						InSize += 2;
					}
					g_base_info_unref(InterfaceInfo);
					break;
				}
				}
			} else {
				Args[I].Out = NumOutputs++;
				switch (g_type_info_get_tag(Args[I].Type)) {
				case GI_TYPE_TAG_VOID:
					Args[I].SkipOut = 1;
					InSize += 1;
					OutSize += 1;
					break;
				case GI_TYPE_TAG_ARRAY: {
					if (g_arg_info_get_ownership_transfer(Args[I].Info) != GI_TRANSFER_NOTHING) {
						OutSize += 1;
					}
					OutSize += 1;
					int Length = g_type_info_get_array_length(Args[I].Type);
					if (Length >= 0) {
						Args[Length].SkipOut = 1;
						//InSize -= 1;
						OutSize += 1;
					}
					InSize += 1;
					type_param_size(Args[I].Type, 0, &OutSize, &NumAux);
					break;
				}
				case GI_TYPE_TAG_INTERFACE: {
					GIBaseInfo *InterfaceInfo = g_type_info_get_interface(Args[I].Type);
					switch (g_base_info_get_type(InterfaceInfo)) {
					case GI_INFO_TYPE_STRUCT:
					case GI_INFO_TYPE_ENUM:
					case GI_INFO_TYPE_FLAGS: {
						NumAux++;
						OutSize += 2;
						break;
					}
					}
					g_base_info_unref(InterfaceInfo);
					break;
				}
				case GI_TYPE_TAG_GLIST:
				case GI_TYPE_TAG_GSLIST:
					OutSize += 1;
					type_param_size(Args[I].Type, 0, &OutSize, &NumAux);
					break;
				case GI_TYPE_TAG_GHASH:
					OutSize += 1;
					type_param_size(Args[I].Type, 0, &OutSize, &NumAux);
					type_param_size(Args[I].Type, 1, &OutSize, &NumAux);
					break;
				default:
					InSize += 1;
					OutSize += 1;
					break;
				}
			}
		}
	}
	GITypeInfo Return[1];
	g_callable_info_load_return_type((GICallableInfo *)Info, Return);
	int NumResults = 1;
	if (g_callable_info_get_caller_owns((GICallableInfo *)Info) != GI_TRANSFER_NOTHING) {
		OutSize += 1;
	}
	switch (g_type_info_get_tag(Return)) {
	case GI_TYPE_TAG_VOID:
		NumResults = 0;
		break;
	case GI_TYPE_TAG_ARRAY: {
		int Length = g_type_info_get_array_length(Return);
		if (Length >= 0) {
			Args[Length].SkipOut = 1;
			OutSize += 1;
		}
		OutSize += 1;
		type_param_size(Return, 0, &OutSize, &NumAux);
		break;
	}
	case GI_TYPE_TAG_INTERFACE:
		OutSize += 2;
		NumAux++;
		break;
	case GI_TYPE_TAG_GLIST:
	case GI_TYPE_TAG_GSLIST:
		OutSize += 1;
		type_param_size(Return, 0, &OutSize, &NumAux);
		break;
	case GI_TYPE_TAG_GHASH:
		OutSize += 1;
		type_param_size(Return, 0, &OutSize, &NumAux);
		type_param_size(Return, 1, &OutSize, &NumAux);
		break;
	default:
		OutSize += 1;
		break;
	}
	gir_function_t *Function = xnew(gir_function_t, NumAux, void *);
	Function->Type = GirFunctionT;
	Function->Info = Info;
	g_base_info_ref(Info);
	Function->NumInputs = NumInputs;
	Function->NumOutputs = NumOutputs;
	Function->NumArgsIn = NumIn;
	Function->NumArgsOut = NumOut;
	Function->NumValues = NumValues;
	gi_inst_t *InstIn = Function->InstIn = (gi_inst_t *)snew(InSize * sizeof(gi_inst_t));
	gi_inst_t *InstOut = Function->InstOut = (gi_inst_t *)snew(OutSize * sizeof(gi_inst_t));
	NumAux = 0;
	if (Flags & GI_FUNCTION_IS_METHOD) (InstIn++)->Opcode = GIB_SELF;
	if (g_callable_info_get_caller_owns((GICallableInfo *)Info) != GI_TRANSFER_NOTHING) {
		(InstOut++)->Opcode = GIB_FREE;
	}
	switch (g_type_info_get_tag(Return)) {
	case GI_TYPE_TAG_VOID:
		break;
	BASIC_CASES_INST(InstOut)
	case GI_TYPE_TAG_ARRAY: {
		int Length = g_type_info_get_array_length(Return);
		if (Length >= 0) {
			(InstOut++)->Opcode = GIB_ARRAY_LENGTH;
			(InstOut++)->Aux = Args[Length].Out;
		} else if (g_type_info_is_zero_terminated(Return)) {
			(InstOut++)->Opcode = GIB_ARRAY_ZERO;
		} else {
			(InstOut++)->Opcode = GIB_ARRAY;
		}
		type_param_inst(Return, 0, &InstOut, &NumAux, Function->Aux);
		break;
	}
	case GI_TYPE_TAG_INTERFACE: {
		GIBaseInfo *InterfaceInfo = g_type_info_get_interface(Return);
		switch (g_base_info_get_type(InterfaceInfo)) {
		case GI_INFO_TYPE_CALLBACK:
			(InstOut++)->Opcode = GIB_CALLBACK;
			(InstOut++)->Aux = NumAux;
			Function->Aux[NumAux++] = callback_info_lookup((GICallbackInfo *)InterfaceInfo);
			break;
		case GI_INFO_TYPE_STRUCT:
			(InstOut++)->Opcode = GIB_STRUCT;
			(InstOut++)->Aux = NumAux;
			Function->Aux[NumAux++] = struct_info_lookup((GIStructInfo *)InterfaceInfo);
			break;
		case GI_INFO_TYPE_UNION:
			(InstOut++)->Opcode = GIB_UNION;
			(InstOut++)->Aux = NumAux;
			Function->Aux[NumAux++] = union_info_lookup((GIUnionInfo *)InterfaceInfo);
			break;
		case GI_INFO_TYPE_ENUM:
		case GI_INFO_TYPE_FLAGS:
			(InstOut++)->Opcode = GIB_ENUM;
			(InstOut++)->Aux = NumAux;
			Function->Aux[NumAux++] = enum_info_lookup((GIEnumInfo *)InterfaceInfo);
			break;
		case GI_INFO_TYPE_OBJECT:
			(InstOut++)->Opcode = GIB_OBJECT;
			(InstOut++)->Aux = NumAux;
			Function->Aux[NumAux++] = object_info_lookup((GIObjectInfo *)InterfaceInfo);
			break;
		case GI_INFO_TYPE_INTERFACE:
			(InstOut++)->Opcode = GIB_OBJECT;
			(InstOut++)->Aux = NumAux;
			Function->Aux[NumAux++] = interface_info_lookup((GIInterfaceInfo *)InterfaceInfo);
			break;
		default: {
			// TODO: raise error or add support
		}
		}
		g_base_info_unref(InterfaceInfo);
		break;
	}
	case GI_TYPE_TAG_GLIST: {
		(InstOut++)->Opcode = GIB_LIST;
		type_param_inst(Return, 0, &InstOut, &NumAux, Function->Aux);
		break;
	}
	case GI_TYPE_TAG_GSLIST: {
		(InstOut++)->Opcode = GIB_SLIST;
		type_param_inst(Return, 0, &InstOut, &NumAux, Function->Aux);
		break;
	}
	case GI_TYPE_TAG_GHASH: {
		(InstOut++)->Opcode = GIB_HASH;
		type_param_inst(Return, 0, &InstOut, &NumAux, Function->Aux);
		type_param_inst(Return, 1, &InstOut, &NumAux, Function->Aux);
		break;
	}
	default: // TODO: handle this.
	}
	for (int I = 0; I < NumArgs; ++I) {
		GIDirection Direction = Args[I].Direction;
		if (Direction == GI_DIRECTION_IN || Direction == GI_DIRECTION_INOUT) {
			if (Args[I].SkipIn) {
				(InstIn++)->Opcode = GIB_SKIP;
			} else switch (g_type_info_get_tag(Args[I].Type)) {
			BASIC_CASES_INST(InstIn)
			case GI_TYPE_TAG_ARRAY: {
				int Length = g_type_info_get_array_length(Args[I].Type);
				if (Length >= 0) {
					(InstIn++)->Opcode = GIB_ARRAY_LENGTH;
					(InstIn++)->Aux = Args[Length].In;
				} else if (g_type_info_is_zero_terminated(Args[I].Type)) {
					(InstIn++)->Opcode = GIB_ARRAY_ZERO;
				} else {
					(InstIn++)->Opcode = GIB_ARRAY;
				}
				type_param_inst(Args[I].Type, 0, &InstIn, &NumAux, Function->Aux);
				break;
			}
			case GI_TYPE_TAG_INTERFACE: {
				GIBaseInfo *InterfaceInfo = g_type_info_get_interface(Args[I].Type);
				if (g_base_info_equal(InterfaceInfo, GValueInfo)) {
					(InstIn++)->Opcode = GIB_VALUE;
					(InstIn++)->Aux = Args[I].Value;
				} else {
					switch (g_base_info_get_type(InterfaceInfo)) {
					case GI_INFO_TYPE_CALLBACK: {
						(InstIn++)->Opcode = GIB_CALLBACK;
						(InstIn++)->Aux = NumAux;
						Function->Aux[NumAux++] = callback_info_lookup((GICallbackInfo *)InterfaceInfo);
						break;
					}
					case GI_INFO_TYPE_STRUCT: {
						(InstIn++)->Opcode = GIB_STRUCT;
						(InstIn++)->Aux = NumAux;
						Function->Aux[NumAux++] = struct_info_lookup((GIStructInfo *)InterfaceInfo);
						break;
					}
					case GI_INFO_TYPE_UNION: {
						(InstIn++)->Opcode = GIB_UNION;
						(InstIn++)->Aux = NumAux;
						Function->Aux[NumAux++] = union_info_lookup((GIUnionInfo *)InterfaceInfo);
						break;
					}
					case GI_INFO_TYPE_ENUM:
					case GI_INFO_TYPE_FLAGS: {
						(InstIn++)->Opcode = GIB_ENUM;
						(InstIn++)->Aux = NumAux;
						Function->Aux[NumAux++] = enum_info_lookup((GIEnumInfo *)InterfaceInfo);
						break;
					}
					case GI_INFO_TYPE_OBJECT: {
						(InstIn++)->Opcode = GIB_OBJECT;
						(InstIn++)->Aux = NumAux;
						Function->Aux[NumAux++] = object_info_lookup((GIObjectInfo *)InterfaceInfo);
						break;
					}
					case GI_INFO_TYPE_INTERFACE: {
						(InstIn++)->Opcode = GIB_OBJECT;
						(InstIn++)->Aux = NumAux;
						Function->Aux[NumAux++] = interface_info_lookup((GIInterfaceInfo *)InterfaceInfo);
						break;
					}
					default: {
						// TODO: raise error or add support
					}
					}
				}
				g_base_info_unref(InterfaceInfo);
				break;
			}
			case GI_TYPE_TAG_GLIST: {
				(InstIn++)->Opcode = GIB_LIST;
				type_param_inst(Args[I].Type, 0, &InstIn, &NumAux, Function->Aux);
				break;
			}
			case GI_TYPE_TAG_GSLIST: {
				(InstIn++)->Opcode = GIB_SLIST;
				type_param_inst(Args[I].Type, 0, &InstIn, &NumAux, Function->Aux);
				break;
			}
			case GI_TYPE_TAG_GHASH: {
				(InstIn++)->Opcode = GIB_HASH;
				type_param_inst(Args[I].Type, 0, &InstIn, &NumAux, Function->Aux);
				type_param_inst(Args[I].Type, 1, &InstIn, &NumAux, Function->Aux);
				break;
			}
			default:
				(InstIn++)->Opcode = GIB_SKIP;
			}
		}
		if (Direction == GI_DIRECTION_OUT || Direction == GI_DIRECTION_INOUT) {
			if (g_arg_info_is_caller_allocates(Args[I].Info)) {
				switch (g_type_info_get_tag(Args[I].Type)) {
				case GI_TYPE_TAG_ARRAY: {
					int Length = g_type_info_get_array_length(Args[I].Type);
					if (Length >= 0) {
						(InstIn++)->Opcode = GIB_OUTPUT_ARRAY_LENGTH;
						(InstIn++)->Aux = Args[Length].In;
					} else {
						(InstIn++)->Opcode = GIB_OUTPUT_ARRAY;
					}
					type_param_inst(Args[I].Type, 0, &InstIn, &NumAux, Function->Aux);
					break;
				}
				case GI_TYPE_TAG_INTERFACE: {
					GIBaseInfo *InterfaceInfo = g_type_info_get_interface(Args[I].Type);
					switch (g_base_info_get_type(InterfaceInfo)) {
					case GI_INFO_TYPE_STRUCT: {
						(InstIn++)->Opcode = GIB_OUTPUT_STRUCT;
						(InstIn++)->Aux = NumAux;
						Function->Aux[NumAux++] = struct_info_lookup((GIStructInfo *)InterfaceInfo);
						break;
					}
					}
					g_base_info_unref(InterfaceInfo);
					break;
				}
				}
			} else {
				if (!Args[I].SkipOut) ++NumResults;
				switch (g_type_info_get_tag(Args[I].Type)) {
				case GI_TYPE_TAG_VOID:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = GIB_SKIP;
					break;
				case GI_TYPE_TAG_BOOLEAN:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_BOOLEAN;
					break;
				case GI_TYPE_TAG_INT8:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_INT8;
					break;
				case GI_TYPE_TAG_UINT8:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_UINT8;
					break;
				case GI_TYPE_TAG_INT16:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_INT16;
					break;
				case GI_TYPE_TAG_UINT16:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_UINT16;
					break;
				case GI_TYPE_TAG_INT32:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_INT32;
					break;
				case GI_TYPE_TAG_UINT32:
				case GI_TYPE_TAG_UNICHAR:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_UINT32;
					break;
				case GI_TYPE_TAG_INT64:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_INT64;
					break;
				case GI_TYPE_TAG_UINT64:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_UINT64;
					break;
				case GI_TYPE_TAG_FLOAT:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_FLOAT;
					break;
				case GI_TYPE_TAG_DOUBLE:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_DOUBLE;
					break;
				case GI_TYPE_TAG_GTYPE:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_GTYPE;
					break;
				case GI_TYPE_TAG_UTF8:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_STRING;
					break;
				case GI_TYPE_TAG_FILENAME:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					(InstOut++)->Opcode = Args[I].SkipOut ? GIB_SKIP : GIB_STRING;
					break;
				case GI_TYPE_TAG_ARRAY: {
					if (g_arg_info_get_ownership_transfer(Args[I].Info) != GI_TRANSFER_NOTHING) {
						(InstOut++)->Opcode = GIB_FREE;
					}
					int Length = g_type_info_get_array_length(Args[I].Type);
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					if (Length >= 0) {
						(InstOut++)->Opcode = GIB_ARRAY_LENGTH;
						(InstOut++)->Aux= Args[Length].Out;
					} else {
						(InstOut++)->Opcode = GIB_ARRAY;
					}
					type_param_inst(Args[I].Type, 0, &InstOut, &NumAux, Function->Aux);
					break;
				}
				case GI_TYPE_TAG_INTERFACE: {
					GIBaseInfo *InterfaceInfo = g_type_info_get_interface(Args[I].Type);
					switch (g_base_info_get_type(InterfaceInfo)) {
					case GI_INFO_TYPE_STRUCT: {
						(InstOut++)->Opcode = GIB_STRUCT;
						(InstOut++)->Aux = NumAux;
						Function->Aux[NumAux++] = struct_info_lookup((GIStructInfo *)InterfaceInfo);
						break;
					}
					case GI_INFO_TYPE_ENUM:
					case GI_INFO_TYPE_FLAGS: {
						(InstOut++)->Opcode = GIB_ENUM;
						(InstOut++)->Aux = NumAux;
						Function->Aux[NumAux++] = enum_info_lookup((GIEnumInfo *)InterfaceInfo);
						break;
					}
					}
					g_base_info_unref(InterfaceInfo);
					break;
				}
				case GI_TYPE_TAG_GLIST: {
					(InstOut++)->Opcode = GIB_LIST;
					type_param_inst(Args[I].Type, 0, &InstOut, &NumAux, Function->Aux);
					break;
				}
				case GI_TYPE_TAG_GSLIST: {
					(InstOut++)->Opcode = GIB_SLIST;
					type_param_inst(Args[I].Type, 0, &InstOut, &NumAux, Function->Aux);
					break;
				}
				case GI_TYPE_TAG_GHASH: {
					(InstOut++)->Opcode = GIB_HASH;
					type_param_inst(Args[I].Type, 0, &InstOut, &NumAux, Function->Aux);
					type_param_inst(Args[I].Type, 1, &InstOut, &NumAux, Function->Aux);
					break;
				}
				default:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					break;
				}
			}
		}
	}
	(InstIn++)->Opcode = (InstOut++)->Opcode = GIB_DONE;
	if (InstIn - Function->InstIn != InSize) {
		//fprintf(stderr, "Function = 0x%lx\n", Function);
		printf("Warning: InSize = %d, InstIn - Function->InstIn = %ld\n", InSize, InstIn - Function->InstIn);
		//asm("int3");
	}
	if (InstOut - Function->InstOut != OutSize) {
		//fprintf(stderr, "Function = 0x%lx\n", Function);
		GIBaseInfo *Container = g_base_info_get_container(Info);
		printf("Warning: OutSize = %d, InstOut - Function->InstOut = %ld\n\t%s\n\t%s\n\t%s\n", OutSize, InstOut - Function->InstOut,
			g_base_info_get_namespace(Info),
			Container ? g_base_info_get_name(Container) : "<none>",
			g_base_info_get_name(Info)
		);
		//asm("int3");
	}
	Function->NumResults = NumResults;
	return (ml_value_t *)Function;
}

static const char *GIBInstNames[] = {
	[GIB_DONE] = "done",
	[GIB_BOOLEAN] = "boolean",
	[GIB_INT8] = "int8",
	[GIB_UINT8] = "uint8",
	[GIB_INT16] = "int16",
	[GIB_UINT16] = "uint16",
	[GIB_INT32] = "int32",
	[GIB_UINT32] = "uint32",
	[GIB_INT64] = "int64",
	[GIB_UINT64] = "uint64",
	[GIB_FLOAT] = "float",
	[GIB_DOUBLE] = "double",
	[GIB_STRING] = "string",
	[GIB_GTYPE] = "gtype",
	//[GIB_BYTES] = "bytes",
	[GIB_ARRAY] = "array",
	[GIB_CALLBACK] = "callback",
	[GIB_STRUCT] = "struct",
	[GIB_ENUM] = "enum",
	[GIB_OBJECT] = "object",
	[GIB_SELF] = "self",
	[GIB_VALUE] = "value",
	[GIB_LIST] = "list",
	[GIB_SLIST] = "slist",
	[GIB_HASH] = "hash",
	[GIB_ERROR] = "error",
	//[GIB_BYTES_FIXED] = "bytes_fixed",
	//[GIB_BYTES_LENGTH] = "bytes_length",
	[GIB_ARRAY_ZERO] = "array_zero",
	[GIB_ARRAY_FIXED] = "array_fixed",
	[GIB_ARRAY_LENGTH] = "array_length",
	[GIB_SKIP] = "skip",
	[GIB_OUTPUT_VALUE] = "output_value",
	[GIB_OUTPUT_ARRAY] = "output_array",
	[GIB_OUTPUT_ARRAY_LENGTH] = "output_array_length",
	[GIB_OUTPUT_STRUCT] = "output_struct"
};

static void type_list(ml_stringbuffer_t *Buffer, gi_inst_t **Inst, void **Aux) {
	gi_opcode_t Opcode = ((*Inst)++)->Opcode;
	ml_stringbuffer_printf(Buffer, ", %s", GIBInstNames[Opcode]);
	switch (Opcode) {
	case GIB_CALLBACK:
	case GIB_STRUCT:
	case GIB_ENUM:
	case GIB_OBJECT: {
		ml_type_t *Type = (ml_type_t *)Aux[((*Inst)++)->Aux];
		ml_stringbuffer_printf(Buffer, "(%s)", Type->Name);
		break;
	}
	}
}

static void callback_list(ml_stringbuffer_t *Buffer, callback_t *Callback, const char *Indent) {
	for (gi_inst_t *Inst = Callback->InstIn; Inst->Opcode != GIB_DONE;) {
		gi_opcode_t Opcode = (Inst++)->Opcode;
		ml_stringbuffer_printf(Buffer, "%s\tin %s", Indent, GIBInstNames[Opcode]);
		switch (Opcode) {
		case GIB_ARRAY_LENGTH:
		case GIB_VALUE:
		case GIB_OUTPUT_ARRAY_LENGTH:
			ml_stringbuffer_printf(Buffer, " %d", (Inst++)->Aux);
			ml_stringbuffer_printf(Buffer, ", %s", GIBInstNames[(Inst++)->Opcode]);
			break;
		case GIB_ARRAY_ZERO:
		case GIB_ARRAY:
		case GIB_LIST:
		case GIB_SLIST:
			type_list(Buffer, &Inst, Callback->Aux);
			break;
		case GIB_HASH:
			type_list(Buffer, &Inst, Callback->Aux);
			type_list(Buffer, &Inst, Callback->Aux);
			break;
		case GIB_CALLBACK:
		case GIB_STRUCT:
		case GIB_ENUM:
		case GIB_OBJECT: {
			ml_type_t *Type = (ml_type_t *)Callback->Aux[(Inst++)->Aux];
			ml_stringbuffer_printf(Buffer, "(%s)", Type->Name);
			break;
		}
		}
		ml_stringbuffer_write(Buffer, "\n", strlen("\n"));
	}
	ml_stringbuffer_printf(Buffer, "%s\tcall(%d)\n", Indent, Callback->Provided);
	for (gi_inst_t *Inst = Callback->InstOut; Inst->Opcode != GIB_DONE;) {
		gi_opcode_t Opcode = (Inst++)->Opcode;
		ml_stringbuffer_printf(Buffer, "%s\tout %s", Indent, GIBInstNames[Opcode]);
		switch (Opcode) {
		case GIB_ARRAY_LENGTH:
		case GIB_VALUE:
			ml_stringbuffer_printf(Buffer, " %d", (Inst++)->Aux);
			ml_stringbuffer_printf(Buffer, ", %s", GIBInstNames[(Inst++)->Opcode]);
			break;
		case GIB_ARRAY_ZERO:
		case GIB_ARRAY:
		case GIB_LIST:
		case GIB_SLIST:
			type_list(Buffer, &Inst, Callback->Aux);
			break;
		case GIB_HASH:
			type_list(Buffer, &Inst, Callback->Aux);
			type_list(Buffer, &Inst, Callback->Aux);
			break;
		case GIB_CALLBACK:
		case GIB_STRUCT:
		case GIB_ENUM:
		case GIB_OBJECT: {
			ml_type_t *Type = (ml_type_t *)Callback->Aux[(Inst++)->Aux];
			ml_stringbuffer_printf(Buffer, "(%s)", Type->Name);
			break;
		}
		}
		ml_stringbuffer_write(Buffer, "\n", strlen("\n"));
	}
}

ML_METHOD("list", GirCallbackT) {
	callback_t *Callback = (callback_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	callback_list(Buffer, Callback, "");
	return ml_stringbuffer_to_string(Buffer);
}

ML_METHOD("list", GirFunctionT) {
	gir_function_t *Function = (gir_function_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_printf(Buffer, "\tinputs = %d\n", Function->NumInputs);
	ml_stringbuffer_printf(Buffer, "\toutputs = %d\n", Function->NumResults);
	for (gi_inst_t *Inst = Function->InstIn; Inst->Opcode != GIB_DONE;) {
		gi_opcode_t Opcode = (Inst++)->Opcode;
		ml_stringbuffer_printf(Buffer, "\tin %s", GIBInstNames[Opcode]);
		switch (Opcode) {
		case GIB_ARRAY_LENGTH:
		case GIB_VALUE:
		case GIB_OUTPUT_ARRAY_LENGTH:
			ml_stringbuffer_printf(Buffer, " %d", (Inst++)->Aux);
			ml_stringbuffer_printf(Buffer, ", %s", GIBInstNames[(Inst++)->Opcode]);
			break;
		case GIB_ARRAY_ZERO:
		case GIB_ARRAY:
		case GIB_LIST:
		case GIB_SLIST:
			type_list(Buffer, &Inst, Function->Aux);
			break;
		case GIB_HASH:
			type_list(Buffer, &Inst, Function->Aux);
			type_list(Buffer, &Inst, Function->Aux);
			break;
		case GIB_CALLBACK: {
			callback_t *Callback = (callback_t *)Function->Aux[(Inst++)->Aux];
			ml_stringbuffer_printf(Buffer, "(%s)\n", Callback->Base.Name);
			callback_list(Buffer, Callback, "\t");
			continue;
		}
		case GIB_STRUCT:
		case GIB_ENUM:
		case GIB_OBJECT:
		case GIB_OUTPUT_STRUCT: {
			ml_type_t *Type = (ml_type_t *)Function->Aux[(Inst++)->Aux];
			ml_stringbuffer_printf(Buffer, "(%s)", Type->Name);
			break;
		}
		}
		ml_stringbuffer_write(Buffer, "\n", strlen("\n"));
	}
	const char *Name = g_base_info_get_name((GIBaseInfo *)Function->Info);
	ml_stringbuffer_printf(Buffer, "\t%s(%d, %d)\n", Name, Function->NumArgsIn, Function->NumArgsOut);
	for (gi_inst_t *Inst = Function->InstOut; Inst->Opcode != GIB_DONE;) {
		gi_opcode_t Opcode = (Inst++)->Opcode;
		ml_stringbuffer_printf(Buffer, "\tout %s", GIBInstNames[Opcode]);
		switch (Opcode) {
		case GIB_ARRAY_LENGTH:
		case GIB_VALUE:
			ml_stringbuffer_printf(Buffer, " %d", (Inst++)->Aux);
			ml_stringbuffer_printf(Buffer, ", %s", GIBInstNames[(Inst++)->Opcode]);
			break;
		case GIB_ARRAY_ZERO:
		case GIB_ARRAY:
		case GIB_LIST:
		case GIB_SLIST:
			type_list(Buffer, &Inst, Function->Aux);
			break;
		case GIB_HASH:
			type_list(Buffer, &Inst, Function->Aux);
			type_list(Buffer, &Inst, Function->Aux);
			break;
		case GIB_CALLBACK:
		case GIB_STRUCT:
		case GIB_ENUM:
		case GIB_OBJECT: {
			ml_type_t *Type = (ml_type_t *)Function->Aux[(Inst++)->Aux];
			ml_stringbuffer_printf(Buffer, "(%s)", Type->Name);
			break;
		}
		}
		ml_stringbuffer_write(Buffer, "\n", strlen("\n"));
	}
	return ml_stringbuffer_to_string(Buffer);
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
	stringmap_insert(GirTypelibT->Exports, "install", GirInstall);
#endif
	stringmap_insert(Globals, "gir", GirTypelibT);
#ifdef ML_LIBRARY
	ml_library_register("gir", GirModule);
#endif
}

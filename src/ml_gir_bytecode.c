#include "ml_gir.h"
#include "ml_macros.h"
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
	GIB_OUTPUT_STRUCT
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

static ml_value_t *string_to_value(void *Ptr) {
	return ml_string(Ptr, -1);
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

static ml_value_t *gtype_to_value(void *Ptr) {
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

static ml_value_t *unknown_to_value(void *Ptr) {
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
		ml_value_t *(*to_value)(void *) = NULL;
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
			for (int I = 0; I < Length; ++I) *Slot++ = to_value(*Input++);
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
		ml_value_t *(*to_value)(void *) = NULL;
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
			ml_list_put(List, to_value(Node->data));
		}
		*Arg++ = List;
		break;
	}
	case GIB_SLIST: {
		ml_value_t *(*to_value)(void *) = NULL;
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
			ml_list_put(List, to_value(Node->data));
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
	ml_call(State, Instance->Function, Callback->Provided, Args);
	GMainContext *MainContext = g_main_context_default();
	while (!State->Value) g_main_context_iteration(MainContext, TRUE);
	ml_value_t *Result = State->Value;
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
	case GI_TYPE_TAG_FILENAME: (NUM++); break;

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
	case GI_TYPE_TAG_FILENAME: (DEST++)->Opcode = GIB_STRING; break;

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
		(ArgIn++)->v_pointer = g_callable_info_prepare_closure(
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
				*Result++ = ml_string(Buffer, Length);
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
		break;
	}
	case GIB_STRUCT: {
		struct_instance_t *Instance = new(struct_instance_t);
		Instance->Type = Function->Aux[(Inst++)->Aux];
		Instance->Value = (ArgOut++)->v_pointer;
		*Result++ = (ml_value_t *)Instance;
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
		g_hash_table_foreach((GHashTable *)((ArgOut++)->v_pointer), ghash_to_map, Convert);
		*Result++ = Convert->Map;
	}
	}
	ML_RETURN(Results);
}

ML_TYPE(GirFunctionT, (MLFunctionT), "gir::function",
	.call = (void *)gir_function_call
);

static int ML_TYPED_FN(ml_method_is_safe, GirFunctionT, ml_value_t *Function) {
	return 1;
}

static void type_param_size(GITypeInfo *Info, int Index, int *Size, int *NumAux) {
	GITypeInfo *ElementInfo = g_type_info_get_param_type(Info, Index);
	switch (g_type_info_get_tag(ElementInfo)) {
	BASIC_CASES_SIZE((*Size))
	case GI_TYPE_TAG_INTERFACE: (*Size)++; (*NumAux)++; break;
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
				int Length = g_type_info_get_array_length(Args[I].Type);
				if (Length >= 0) {
					NumInputs--;
					Args[Length].SkipIn = 1;
					InSize += 1;
				}
				InSize += 2;
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
					InSize += 2;
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
				case GI_TYPE_TAG_ARRAY: {
					int Length = g_type_info_get_array_length(Args[I].Type);
					if (Length >= 0) {
						Args[Length].SkipOut = 1;
						InSize -= 1;
						OutSize += 1;
					}
					InSize += 1;
					OutSize += 2;
					break;
				}
				default:
					InSize += 1;
					OutSize += 1;
					break;
				}
			}
		}
	}
	GITypeInfo *Return = g_callable_info_get_return_type((GICallableInfo *)Info);
	int NumResults = 1;
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
		OutSize += 2;
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
	switch (g_type_info_get_tag(Return)) {
	case GI_TYPE_TAG_VOID:
		break;
	BASIC_CASES_INST(InstOut)
	case GI_TYPE_TAG_ARRAY: {
		int Length = g_type_info_get_array_length(Return);
		GITypeInfo *ElementInfo = g_type_info_get_param_type(Return, 0);
		if (Length >= 0) {
			(InstOut++)->Opcode = GIB_ARRAY_LENGTH;
			(InstOut++)->Aux = Args[Length].Out;
		} else if (g_type_info_is_zero_terminated(Return)) {
			(InstIn++)->Opcode = GIB_ARRAY_ZERO;
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
	g_base_info_unref(Return);
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
				type_param_inst(Args[I].Type, 0, &InstOut, &NumAux, Function->Aux);
				break;
			}
			case GI_TYPE_TAG_GSLIST: {
				(InstIn++)->Opcode = GIB_SLIST;
				type_param_inst(Args[I].Type, 0, &InstOut, &NumAux, Function->Aux);
				break;
			}
			case GI_TYPE_TAG_GHASH: {
				(InstIn++)->Opcode = GIB_HASH;
				type_param_inst(Args[I].Type, 0, &InstOut, &NumAux, Function->Aux);
				type_param_inst(Args[I].Type, 1, &InstOut, &NumAux, Function->Aux);
				break;
			}
			default:
			}
		}
		if (Direction == GI_DIRECTION_OUT || Direction == GI_DIRECTION_INOUT) {
			if (g_arg_info_is_caller_allocates(Args[I].Info)) {
				switch (g_type_info_get_tag(Args[I].Type)) {
				case GI_TYPE_TAG_ARRAY: {
					int Length = g_type_info_get_array_length(Args[I].Type);
					GITypeInfo *ElementInfo = g_type_info_get_param_type(Args[I].Type, 0);
					if (Length >= 0) {
						(InstIn++)->Opcode = GIB_OUTPUT_ARRAY_LENGTH;
						(InstIn++)->Aux = Args[Length].In;
					} else {
						(InstIn++)->Opcode = GIB_OUTPUT_ARRAY;
					}
					switch (g_type_info_get_tag(ElementInfo)) {
					BASIC_CASES_INST(InstIn)
					default: // TODO: handle this.
					}
					g_base_info_unref(ElementInfo);
					break;
				}
				case GI_TYPE_TAG_INTERFACE: {
					GIBaseInfo *InterfaceInfo = g_type_info_get_interface(Args[I].Type);
					if (g_base_info_get_type(InterfaceInfo) == GI_INFO_TYPE_STRUCT) {
						(InstIn++)->Opcode = GIB_OUTPUT_STRUCT;
						(InstIn++)->Aux = NumAux;
						Function->Aux[NumAux++] = struct_info_lookup((GIStructInfo *)InterfaceInfo);
					}
					g_base_info_unref(InterfaceInfo);
					break;
				}
				}
			} else {
				if (!Args[I].SkipOut) ++NumResults;
				switch (g_type_info_get_tag(Args[I].Type)) {
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
					int Length = g_type_info_get_array_length(Args[I].Type);
					GITypeInfo *ElementInfo = g_type_info_get_param_type(Args[I].Type, 0);
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					if (Length >= 0) {
						(InstOut++)->Opcode = GIB_ARRAY_LENGTH;
						(InstOut++)->Aux= Args[Length].Out;
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
				default:
					(InstIn++)->Opcode = GIB_OUTPUT_VALUE;
					break;
				}
			}
		}
	}
	InstIn->Opcode = InstOut->Opcode = GIB_DONE;
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
			callback_t *Callback = (ml_callback_t *)Function->Aux[(Inst++)->Aux];
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


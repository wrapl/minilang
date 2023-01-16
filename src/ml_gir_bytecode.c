#include "ml_gir.h"
#include "ml_macros.h"

enum {
	GI_DONE,
	GI_VOID,
	GI_BOOLEAN,
	GI_INT8,
	GI_UINT8,
	GI_INT16,
	GI_UINT16,
	GI_INT32,
	GI_UINT32,
	GI_INT64,
	GI_UINT64,
	GI_FLOAT,
	GI_DOUBLE,
	GI_STRING,
	GI_GTYPE,
	GI_BYTES,
	GI_ARRAY,
	GI_LENGTH,
	GI_CALLBACK,
	GI_STRUCT,
	GI_ENUM,
	GI_OBJECT,
	GI_VALUE,
	GI_LIST,
	GI_SLIST,
	GI_HASH,
	GI_ERROR,
	GI_BYTES_FIXED,
	GI_BYTES_LENGTH,
	GI_ARRAY_NULL,
	GI_ARRAY_FIXED,
	GI_ARRAY_LENGTH,
	GI_SKIP,
	GI_OUTPUT_VALUE,
	GI_OUTPUT_ARRAY
};

#define VALUE_CONVERTOR(GTYPE, MLTYPE) \
static ml_value_t *GTYPE ## _to_array(ml_value_t *Value, void *Ptr) { \
	*(g ## GTYPE *)Ptr = ml_ ## MLTYPE ## _value(Value); \
	return NULL; \
} \
\
static ml_value_t *GTYPE ## _to_list(ml_value_t *Value, void **Ptr) { \
	*Ptr = (void *)(uintptr_t)ml_ ## MLTYPE ## _value(Value); \
	return NULL; \
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

static ml_value_t *string_to_list(ml_value_t *Value, void **Ptr) {
	if (Value == MLNil) {
		*Ptr = NULL;
	} else if (ml_is(Value, MLAddressT)) {
		*Ptr = (void *)ml_address_value(Value);
	} else {
		return ml_error("TypeError", "Expected string not %s", ml_typeof(Value)->Name);
	}
	return NULL;
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

static ml_value_t *gtype_to_list(ml_value_t *Value, void **Ptr) {
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

static ml_value_t *bytes_to_array(ml_value_t *Value, void *Ptr) {
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
		return ml_error("TypeError", "Expected string not %s", ml_typeof(Value)->Name);
	}
	return NULL;
}

static ml_value_t *bytes_to_list(ml_value_t *Value, void **Ptr) {
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
		return ml_error("TypeError", "Expected string not %s", ml_typeof(Value)->Name);
	}
	return NULL;
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

typedef struct {
	ml_context_t *Context;
	ml_value_t *Function;
	char *InstIn, *InstOut;
	ffi_cif Cif[1];
	int NumArgsIn;
} callback_t;

static void callback_fn(ffi_cif *Cif, void *Return, void **Params, callback_t *Callback) {

}

typedef struct {
	GICallableInfo *Info;
	char *InstIn, *InstOut;
	int NumArgsIn;
} callback_info_t;

typedef struct {
	ml_type_t *Type;
	GIFunctionInfo *Info;
	char *InstIn, *InstOut;
	int NumArgsIn, NumArgsOut;
	int NumValues, NumArgs;
	void *Aux[];
} gir_function_t;

static void gir_function_call(ml_state_t *Caller, gir_function_t *Function, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(Function->NumArgs);
	size_t NumArgs = Function->NumArgsIn + Function->NumArgsOut + 1 + Function->NumArgsOut;
	GIArgument Arguments[NumArgs];
	GValue Values[Function->NumValues];
	memset(Values, 0, Function->NumValues * sizeof(GValue));
	GIArgument *ArgPtr = Arguments;
	GValue *ValuePtr = Values;
	ml_value_t **Arg = Args;
	void **Aux = Function->Aux;
	for (int I = 0; I < Count; ++I) Args[I] = ml_deref(Args[I]);
	for (char *Inst = Function->InstIn; *Inst != GI_DONE;) switch (*Inst++) {
	case GI_VOID: (ArgPtr++)->v_pointer = NULL; ++Arg; break;
	case GI_BOOLEAN: (ArgPtr++)->v_boolean = ml_boolean_value(*Arg++); break;
	case GI_INT8: (ArgPtr++)->v_int8 = ml_integer_value(*Arg++); break;
	case GI_UINT8: (ArgPtr++)->v_uint8 = ml_integer_value(*Arg++); break;
	case GI_INT16: (ArgPtr++)->v_int16 = ml_integer_value(*Arg++); break;
	case GI_UINT16: (ArgPtr++)->v_uint16 = ml_integer_value(*Arg++); break;
	case GI_INT32: (ArgPtr++)->v_int32 = ml_integer_value(*Arg++); break;
	case GI_UINT32: (ArgPtr++)->v_uint32 = ml_integer_value(*Arg++); break;
	case GI_INT64: (ArgPtr++)->v_int64 = ml_integer_value(*Arg++); break;
	case GI_UINT64: (ArgPtr++)->v_uint64 = ml_integer_value(*Arg++); break;
	case GI_FLOAT: (ArgPtr++)->v_float = ml_real_value(*Arg++); break;
	case GI_DOUBLE: (ArgPtr++)->v_double = ml_real_value(*Arg++); break;
	case GI_STRING: {
		ml_value_t *Value = *Arg++;
		if (Value == MLNil) {
			(ArgPtr++)->v_string = NULL;
		} else if (!ml_is(Value, MLStringT)) {
			ML_ERROR("TypeError", "Expected string for argument %ld", Arg - Args);
		} else {
			(ArgPtr++)->v_string = (char *)ml_string_value(Value);
		}
		break;
	}
	case GI_GTYPE: {
		ml_value_t *Value = *Arg++;
		if (ml_is(Value, GirBaseInfoT)) {
			baseinfo_t *Base = (baseinfo_t *)Value;
			(ArgPtr++)->v_size = g_registered_type_info_get_g_type((GIRegisteredTypeInfo *)Base->Info);
		} else if (ml_is(Value, MLStringT)) {
			(ArgPtr++)->v_size = g_type_from_name(ml_string_value(Value));
		} else if (Value == (ml_value_t *)MLNilT) {
			(ArgPtr++)->v_size = G_TYPE_NONE;
		} else if (Value == (ml_value_t *)MLIntegerT) {
			(ArgPtr++)->v_size = G_TYPE_INT64;
		} else if (Value == (ml_value_t *)MLStringT) {
			(ArgPtr++)->v_size = G_TYPE_STRING;
		} else if (Value == (ml_value_t *)MLDoubleT) {
			(ArgPtr++)->v_size = G_TYPE_DOUBLE;
		} else if (Value == (ml_value_t *)MLBooleanT) {
			(ArgPtr++)->v_size = G_TYPE_BOOLEAN;
		} else if (Value == (ml_value_t *)MLAddressT) {
			(ArgPtr++)->v_size = G_TYPE_POINTER;
		} else {
			ML_ERROR("TypeError", "Expected type for parameter %ld", Arg - Args);
		}
		break;
	}
	case GI_BYTES: {
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
		(ArgPtr++)->v_pointer = Bytes;
		break;
	}
	case GI_ARRAY: {
		ml_value_t *Value = *Arg++;
		ml_value_t *(*to_array)(ml_value_t *, void *);
		size_t Size;
		switch (*Inst++) {
		case GI_BOOLEAN: to_array = boolean_to_array; Size = sizeof(gboolean); break;
		case GI_INT8: to_array = int8_to_array; Size = sizeof(gint8); break;
		case GI_UINT8: to_array = uint8_to_array; Size = sizeof(guint8); break;
		case GI_INT16: to_array = int16_to_array; Size = sizeof(gint16); break;
		case GI_UINT16: to_array = uint16_to_array; Size = sizeof(guint16); break;
		case GI_INT32: to_array = int32_to_array; Size = sizeof(gint32); break;
		case GI_UINT32: to_array = uint32_to_array; Size = sizeof(guint32); break;
		case GI_INT64: to_array = int64_to_array; Size = sizeof(gint64); break;
		case GI_UINT64: to_array = uint64_to_array; Size = sizeof(guint64); break;
		case GI_FLOAT: to_array = float_to_array; Size = sizeof(gfloat); break;
		case GI_DOUBLE: to_array = double_to_array; Size = sizeof(gdouble); break;
		case GI_STRING: to_array = string_to_array; Size = sizeof(gchararray); break;
		case GI_GTYPE: to_array = gtype_to_array; Size = sizeof(GType); break;
		case GI_BYTES: to_array = bytes_to_array; Size = sizeof(gpointer); break;
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
		} else {
			ML_ERROR("TypeError", "Expected list for parameter %ld", Arg - Args);
		}
		(ArgPtr++)->v_pointer = Array;
		break;
	}
	case GI_LENGTH: {
		int Index = *Inst++;
		ml_value_t *Value = Args[Index];
		if (Value == MLNil) {
			(ArgPtr++)->v_int64 = 0;
		} else if (ml_is(Value, MLAddressT)) {
			(ArgPtr++)->v_int64 = ml_address_length(Value);
		} else if (ml_is(Value, MLListT)) {
			(ArgPtr++)->v_int64 = ml_list_length(Value);
		} else {
			ML_ERROR("TypeError", "Expected list or address for parameter %d", Index + 1);
		}
		break;
	}
	case GI_CALLBACK: {
		ml_value_t *Value = *Arg++;
		callback_info_t *Info = (callback_info_t *)(*Aux++);
		callback_t *Callback = (callback_t *)GC_MALLOC_UNCOLLECTABLE(sizeof(callback_t));
		Callback->Context = Caller->Context;
		Callback->Function = Value;
		Callback->InstIn = Info->InstIn;
		Callback->InstOut = Info->InstOut;
		Callback->NumArgsIn = Info->NumArgsIn;
		(ArgPtr++)->v_pointer = g_callable_info_prepare_closure(
			Info->Info,
			Callback->Cif,
			(GIFFIClosureCallback)callback_fn,
			Callback
		);
		break;
	}
	case GI_STRUCT: {
		ml_value_t *Value = *Arg++;
		ml_type_t *Type = (ml_type_t *)(*Aux++);
		if (Value == MLNil) {
			(ArgPtr++)->v_pointer = NULL;
		} else if (ml_is(Value, Type)) {
			(ArgPtr++)->v_pointer = ((struct_instance_t *)Value)->Value;
		} else {
			ML_ERROR("TypeError", "Expected %s not %s for parameter %ld", Type->Name, ml_typeof(Value)->Name, Arg - Args);
		}
		break;
	}
	case GI_ENUM: {
		ml_value_t *Value = *Arg++;
		ml_type_t *Type = (ml_type_t *)(*Aux++);
		if (Value == MLNil) {
			(ArgPtr++)->v_int64 = 0;
		} else if (ml_is(Value, Type)) {
			(ArgPtr++)->v_int64 = ((enum_value_t *)Value)->Value;
		} else {
			ML_ERROR("TypeError", "Expected %s not %s for parameter %ld", Type->Name, ml_typeof(Value)->Name, Arg - Args);
		}
		break;
	}
	case GI_OBJECT: {
		ml_value_t *Value = *Arg++;
		ml_type_t *Type = (ml_type_t *)(*Aux++);
		if (Value == MLNil) {
			(ArgPtr++)->v_pointer = NULL;
		} else if (ml_is(Value, Type)) {
			(ArgPtr++)->v_pointer = ((object_instance_t *)Value)->Handle;
		} else {
			ML_ERROR("TypeError", "Expected %s not %s for parameter %ld", Type->Name, ml_typeof(Value)->Name, Arg - Args);
		}
		break;
	}
	case GI_VALUE: {
		ml_value_t *Value = *Arg++;
		to_gvalue(Value, ValuePtr);
		(ArgPtr++)->v_pointer = ValuePtr++;
		break;
	}
	case GI_LIST: {
		ml_value_t *Value = *Arg++;
		ml_value_t *(*to_list)(ml_value_t *, void **);
		switch (*Inst++) {
		case GI_BOOLEAN: to_list = boolean_to_list; break;
		case GI_INT8: to_list = int8_to_list; break;
		case GI_UINT8: to_list = uint8_to_list; break;
		case GI_INT16: to_list = int16_to_list; break;
		case GI_UINT16: to_list = uint16_to_list; break;
		case GI_INT32: to_list = int32_to_list; break;
		case GI_UINT32: to_list = uint32_to_list; break;
		case GI_INT64: to_list = int64_to_list; break;
		case GI_UINT64: to_list = uint64_to_list; break;
		case GI_FLOAT: to_list = float_to_list; break;
		case GI_DOUBLE: to_list = double_to_list; break;
		case GI_STRING: to_list = string_to_list; break;
		case GI_GTYPE: to_list = gtype_to_list; break;
		case GI_BYTES: to_list = bytes_to_list; break;
		default: ML_ERROR("TypeError", "Unsupported array type");
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
		(ArgPtr++)->v_pointer = List;
		break;
	}
	case GI_SLIST: {
		ml_value_t *Value = *Arg++;
		ml_value_t *(*to_list)(ml_value_t *, void **);
		switch (*Inst++) {
		case GI_BOOLEAN: to_list = boolean_to_list; break;
		case GI_INT8: to_list = int8_to_list; break;
		case GI_UINT8: to_list = uint8_to_list; break;
		case GI_INT16: to_list = int16_to_list; break;
		case GI_UINT16: to_list = uint16_to_list; break;
		case GI_INT32: to_list = int32_to_list; break;
		case GI_UINT32: to_list = uint32_to_list; break;
		case GI_INT64: to_list = int64_to_list; break;
		case GI_UINT64: to_list = uint64_to_list; break;
		case GI_FLOAT: to_list = float_to_list; break;
		case GI_DOUBLE: to_list = double_to_list; break;
		case GI_STRING: to_list = string_to_list; break;
		case GI_GTYPE: to_list = gtype_to_list; break;
		case GI_BYTES: to_list = bytes_to_list; break;
		default: ML_ERROR("TypeError", "Unsupported array type");
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
		(ArgPtr++)->v_pointer = List;
		break;
	}
	case GI_HASH: {
		ML_ERROR("TypeError", "Hash arguments not supported yet");
		break;
	}
	case GI_SKIP: {
		(ArgPtr++)->v_pointer = NULL;
		break;
	}
	case GI_OUTPUT_VALUE: {
		GIArgument *Output = ArgPtr + Function->NumArgsOut;
		(ArgPtr++)->v_pointer = Output;
		break;
	}
	case GI_OUTPUT_ARRAY: {
		GIArgument *Output = ArgPtr + Function->NumArgsOut;
		(ArgPtr++)->v_pointer = Output;
	}
	}
	GError *Error = 0;
	gboolean Invoked = g_function_info_invoke(Function->Info, Arguments, Function->NumArgsIn, Arguments + Function->NumArgsIn, Function->NumArgsOut, ArgPtr, &Error);
	if (!Invoked || Error) ML_ERROR("InvokeError", "Error: %s", Error->message);
	ml_value_t *Results, **Result;
	if (Function->NumArgsOut) {
		Results = ml_tuple(Function->NumArgsOut + 1);
		Result = ((ml_tuple_t *)Result)->Values;
	} else {
		Results = MLNil;
		Result = &Results;
	}
	for (char *Inst = Function->InstOut; *Inst != GI_DONE;) switch (*Inst++) {
	case GI_VOID: *Result++ = MLNil; break;
	case GI_BOOLEAN: *Result++ = ml_boolean((ArgPtr++)->v_boolean); break;
	case GI_INT8: *Result++ = ml_integer((ArgPtr++)->v_int8); break;
	case GI_UINT8: *Result++ = ml_integer((ArgPtr++)->v_uint8); break;
	case GI_INT16: *Result++ = ml_integer((ArgPtr++)->v_int16); break;
	case GI_UINT16: *Result++ = ml_integer((ArgPtr++)->v_uint16); break;
	case GI_INT32: *Result++ = ml_integer((ArgPtr++)->v_int32); break;
	case GI_UINT32: *Result++ = ml_integer((ArgPtr++)->v_uint32); break;
	case GI_INT64: *Result++ = ml_integer((ArgPtr++)->v_int64); break;
	case GI_UINT64: *Result++ = ml_integer((ArgPtr++)->v_uint64); break;
	case GI_FLOAT: *Result++ = ml_real((ArgPtr++)->v_float); break;
	case GI_DOUBLE: *Result++ = ml_real((ArgPtr++)->v_double); break;
	}
	ML_RETURN(Results);
}

ML_TYPE(GirFunctionT, (MLFunctionT), "gir::function",
	.call = (void *)gir_function_call
);

typedef struct {
	GIArgInfo *Info;
	int Aux;
	int IsClosure:1;
	int IsLength:1;
} gir_function_arg_info_t;

typedef struct {
	GIFunctionInfo *Info;
	int Count;
	gir_function_arg_info_t Args[];
} gir_function_compiler_t;

static gir_function_t *function_info_compile_arg(gir_function_compiler_t *Compiler, int Index, int In, int Out, int Aux) {
	if (Index == Compiler->Count) {
		gir_function_t *Function = xnew(gir_function_t, Aux, sizeof(void *));
		Function->Info = Compiler->Info;
		Function->InstIn = snew(In + 1);
		Function->InstIn[In] = GI_DONE;
		Function->InstOut = snew(Out + 1);
		Function->InstOut[Out] = GI_DONE;
		return Function;
	}
	gir_function_arg_info_t *Arg = Compiler->Args + Index;
	GIDirection Direction = g_arg_info_get_direction(Arg->Info);
	switch (Direction) {
	case GI_DIRECTION_IN: {
		if (Arg->IsClosure) {
			gir_function_t *Function = function_info_compile_arg(Compiler, Index + 1, In + 1, Out, Aux);
			Function->InstIn[In] = GI_SKIP;
			return Function;
		}
		if (Arg->IsLength) {
			gir_function_t *Function = function_info_compile_arg(Compiler, Index + 1, In + 2, Out, Aux);
			Function->InstIn[In] = GI_LENGTH;
			Function->InstIn[In + 1] = Compiler->Args[Index].Aux;
			return Function;
		}
		GITypeInfo TypeInfo[1];
		g_arg_info_load_type(Arg->Info, TypeInfo);
		GITypeTag Tag = g_type_info_get_tag(TypeInfo);
		switch (Tag) {
		}
		break;
	}
	case GI_DIRECTION_OUT: {
		break;
	}
	case GI_DIRECTION_INOUT: {
		break;
	}
	}
}


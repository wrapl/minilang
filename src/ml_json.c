#include "ml_json.h"
#include "ml_macros.h"
#include <yajl/yajl_common.h>
#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>

#define ML_JSON_STACK_SIZE 10

typedef struct ml_json_stack_t ml_json_stack_t;

struct ml_json_stack_t {
	ml_value_t *Values[ML_JSON_STACK_SIZE];
	ml_json_stack_t *Prev;
	int Index;
};

typedef struct {
	ml_value_t *Key, *Value;
	ml_json_stack_t *Stack;
	ml_json_stack_t Stack0;
} ml_json_decoder_t;

static int value_handler(ml_json_decoder_t *Decoder, ml_value_t *Value) {
	if (Decoder->Value) {
		if (Decoder->Key) {
			ml_map_insert(Decoder->Value, Decoder->Key, Value);
			Decoder->Key = NULL;
		} else {
			ml_list_put(Decoder->Value, Value);
		}
	} else {
		Decoder->Value = Value;
	}
	return 1;
}

static int null_handler(ml_json_decoder_t *Decoder) {
	return value_handler(Decoder, MLNil);
}

static int boolean_handler(ml_json_decoder_t *Decoder, int Value) {
	return value_handler(Decoder, ml_boolean(Value));
}

static int integer_handler(ml_json_decoder_t *Decoder, long long Value) {
	return value_handler(Decoder, ml_integer(Value));
}

static int real_handler(ml_json_decoder_t *Decoder, double Value) {
	return value_handler(Decoder, ml_real(Value));
}

static int string_handler(ml_json_decoder_t *Decoder, const char *Value, size_t Length) {
	return value_handler(Decoder, ml_string(Value, Length));
}

static int push_value(ml_json_decoder_t *Decoder, ml_value_t *Value) {
	if (Decoder->Value) {
		if (Decoder->Key) {
			ml_map_insert(Decoder->Value, Decoder->Key, Value);
			Decoder->Key = NULL;
		} else {
			ml_list_put(Decoder->Value, Value);
		}
		ml_json_stack_t *Stack = Decoder->Stack;
		if (Stack->Index == ML_JSON_STACK_SIZE) {
			ml_json_stack_t *NewStack = new(ml_json_stack_t);
			NewStack->Prev = Stack;
			Stack = Decoder->Stack = NewStack;
		}
		Stack->Values[Stack->Index++] = Decoder->Value;
	}
	Decoder->Value = Value;
	return 1;
}

static int pop_value(ml_json_decoder_t *Decoder) {
	ml_json_stack_t *Stack = Decoder->Stack;
	if (Stack->Index == 0) {
		if (!Stack->Prev) return 1;
		Stack = Decoder->Stack = Stack->Prev;
	}
	Decoder->Value = Stack->Values[Stack->Index];
	Stack->Values[Stack->Index] = NULL;
	--Stack->Index;
	return 1;
}

static int start_map_handler(ml_json_decoder_t *Decoder) {
	return push_value(Decoder, ml_map());
}

static int map_key_handler(ml_json_decoder_t *Decoder, const char *Key, size_t Length) {
	Decoder->Key = ml_string(Key, Length);
	return 1;
}

static int end_map_handler(ml_json_decoder_t *Decoder) {
	return pop_value(Decoder);
}

static int start_array_handler(ml_json_decoder_t *Decoder) {
	return push_value(Decoder, ml_list());
}

static int end_array_handler(ml_json_decoder_t *Decoder) {
	return pop_value(Decoder);
}

static yajl_callbacks Callbacks = {
	.yajl_null = (void *)null_handler,
	.yajl_boolean = (void *)boolean_handler,
	.yajl_integer = (void *)integer_handler,
	.yajl_double = (void *)real_handler,
	.yajl_number = (void *)NULL,
	.yajl_string = (void *)string_handler,
	.yajl_start_map = (void *)start_map_handler,
	.yajl_map_key = (void *)map_key_handler,
	.yajl_end_map = (void *)end_map_handler,
	.yajl_start_array = (void *)start_array_handler,
	.yajl_end_array = (void *)end_array_handler
};

static void *ml_alloc(void *Ctx, size_t Size) {
	return GC_MALLOC(Size);
}

static void *ml_realloc(void *Ctx, void *Ptr, size_t Size) {
	return GC_REALLOC(Ptr, Size);
}

static void ml_free(void *Ctx, void *Ptr) {
}

static yajl_alloc_funcs AllocFuncs = {ml_alloc, ml_realloc, ml_free, NULL};

ML_FUNCTION(JsonDecode) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_json_decoder_t Decoder = {0,};
	Decoder.Stack = &Decoder.Stack0;
	yajl_handle Handle = yajl_alloc(&Callbacks, &AllocFuncs, &Decoder);
	const unsigned char *Text = (const unsigned char *)ml_string_value(Args[0]);
	size_t Length = ml_string_length(Args[0]);
	if (yajl_parse(Handle, Text, Length) == yajl_status_error) {
		const unsigned char *Error = yajl_get_error(Handle, 0, Text, Length);
		size_t Position = yajl_get_bytes_consumed(Handle);
		return ml_error("JSONError", "@%ld: %s", Position, Error);
	}
	yajl_complete_parse(Handle);
	return Decoder.Value;
}

static ml_value_t *ml_json_encode(yajl_gen Handle, ml_value_t *Value) {
	if (Value == MLNil) {
		yajl_gen_null(Handle);
	} else if (ml_is(Value, MLBooleanT)) {
		yajl_gen_bool(Handle, ml_boolean_value(Value));
	} else if (ml_is(Value, MLIntegerT)) {
		yajl_gen_integer(Handle, ml_integer_value(Value));
	} else if (ml_is(Value, MLRealT)) {
		yajl_gen_double(Handle, ml_real_value(Value));
	} else if (ml_is(Value, MLStringT)) {
		yajl_gen_string(Handle, (const unsigned char *)ml_string_value(Value), ml_string_length(Value));
	} else if (ml_is(Value, MLListT)) {
		yajl_gen_array_open(Handle);
		ML_LIST_FOREACH(Value, Iter) {
			ml_value_t *Error = ml_json_encode(Handle, Iter->Value);
			if (Error) return Error;
		}
		yajl_gen_array_close(Handle);
	} else if (ml_is(Value, MLMapT)) {
		yajl_gen_map_open(Handle);
		ML_MAP_FOREACH(Value, Iter) {
			if (!ml_is(Iter->Key, MLStringT)) return ml_error("JSONError", "Invalid map key: expected string not %s", ml_typeof(Iter->Key)->Name);
			yajl_gen_string(Handle, (const unsigned char *)ml_string_value(Iter->Key), ml_string_length(Iter->Key));
			ml_value_t *Error = ml_json_encode(Handle, Iter->Value);
			if (Error) return Error;
		}
		yajl_gen_map_close(Handle);
	} else {
		return ml_error("JSONError", "Invalid type for JSON: %s", ml_typeof(Value)->Name);
	}
	return NULL;
}

ML_FUNCTION(JsonEncode) {
	ML_CHECK_ARG_COUNT(1);
	yajl_gen Handle = yajl_gen_alloc(&AllocFuncs);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	yajl_gen_config(Handle, yajl_gen_print_callback, ml_stringbuffer_add, Buffer);
	return ml_json_encode(Handle, Args[0]) ?: ml_stringbuffer_value(Buffer);
}

void ml_json_init(stringmap_t *Globals) {
#include "ml_json_init.c"
	if (Globals) {
		stringmap_insert(Globals, "json", ml_module("json",
			"encode", JsonEncode,
			"decode", JsonDecode,
		NULL));
	}
}

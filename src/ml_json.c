#include "ml_json.h"
#include "ml_macros.h"
#include "ml_stream.h"

#undef ML_CATEGORY
#define ML_CATEGORY "json"

// Overview
// JSON values are mapped to Minilang as follows:
//
// * :json:`null` |harr| :mini:`nil`
// * :json:`true` |harr| :mini:`true`
// * :json:`false` |harr| :mini:`false`
// * *integer* |harr| :mini:`integer`
// * *real* |harr| :mini:`real`
// * *string* |harr| :mini:`string`
// * *array* |harr| :mini:`list`
// * *object* |harr| :mini:`map`

typedef enum {
	JS_VALUE,
	JS_OBJECT_START,
	JS_OBJECT_REST,
	JS_ARRAY_START,
	JS_ARRAY_REST,
	JS_KEY,
	JS_COLON,
	JS_KEYWORD,
	JS_SIGN,
	JS_ZERO,
	JS_DIGITS,
	JS_FRACTION_FIRST,
	JS_FRACTION,
	JS_EXPONENT_SIGN,
	JS_EXPONENT_FIRST,
	JS_EXPONENT,
	JS_STRING,
	JS_ESCAPE,
	JS_UNICODE_1,
	JS_UNICODE_2,
	JS_UNICODE_3,
	JS_UNICODE_4,
	JS_TRAILING
} json_state_t;

typedef enum {
	JSM_VALUE,
	JSM_ARRAY,
	JSM_OBJECT_KEY,
	JSM_OBJECT_VALUE
} json_mode_t;

#define ML_JSON_STACK_SIZE 10

typedef struct json_stack_t json_stack_t;

struct json_stack_t {
	ml_value_t *Values[ML_JSON_STACK_SIZE];
	json_stack_t *Prev;
	int Index;
};

typedef struct json_decoder_t json_decoder_t;

struct json_decoder_t {
	void (*emit)(json_decoder_t *Decoder, ml_value_t *Value);
	void *Data;
	ml_value_t *Collection, *Key;
	json_stack_t *Stack;
	ml_stringbuffer_t Buffer[1];
	json_stack_t Stack0;
	uint32_t CodePoint;
	json_state_t State;
	json_mode_t Mode;
};

static void json_decoder_init(json_decoder_t *Decoder, void (*emit)(json_decoder_t *Decoder, ml_value_t *Value), void *Data) {
	memset(Decoder, 0, sizeof(json_decoder_t));
	Decoder->emit = emit;
	Decoder->Data = Data;
	Decoder->Stack = &Decoder->Stack0;
	Decoder->Buffer[0] = ML_STRINGBUFFER_INIT;
	Decoder->State = JS_VALUE;
	Decoder->Mode = JSM_VALUE;
}

static void json_decoder_push(json_decoder_t *Decoder, ml_value_t *Collection) {
	if (Decoder->Collection) {
		if (Decoder->Key) {
			ml_map_insert(Decoder->Collection, Decoder->Key, Collection);
			Decoder->Key = NULL;
		} else {
			ml_list_put(Decoder->Collection, Collection);
		}
	}
	json_stack_t *Stack = Decoder->Stack;
	if (Stack->Index == ML_JSON_STACK_SIZE) {
		json_stack_t *NewStack = new(json_stack_t);
		NewStack->Prev = Stack;
		Stack = Decoder->Stack = NewStack;
	}
	Stack->Values[Stack->Index] = Decoder->Collection;
	++Stack->Index;
	Decoder->Collection = Collection;
}

static void json_decoder_pop(json_decoder_t *Decoder) {
	json_stack_t *Stack = Decoder->Stack;
	if (Stack->Index == 0) {
		Stack = Decoder->Stack = Stack->Prev;
	}
	ml_value_t *Collection = Decoder->Collection;
	--Stack->Index;
	Decoder->Collection = Stack->Values[Stack->Index];
	Stack->Values[Stack->Index] = NULL;
	if (!Decoder->Collection) {
		Decoder->State = JS_VALUE;
		Decoder->emit(Decoder, Collection);
	} else if (ml_is(Decoder->Collection, MLListT)) {
		Decoder->State = JS_ARRAY_REST;
	} else if (ml_is(Decoder->Collection, MLMapT)) {
		Decoder->State = JS_OBJECT_REST;
	}
}

static void json_finish_number(json_decoder_t *Decoder) {
	char *String = ml_stringbuffer_get_string(Decoder->Buffer);
	ml_value_t *Number;
	for (char *P = String; *P; ++P) {
		if (*P == '.' || *P == 'e' || *P == 'E') {
			Number = ml_real(strtod(String, NULL));
			goto real;
		}
	}
	Number = ml_integer(strtoll(String, NULL, 10));
real:
	if (Decoder->Collection) {
		if (Decoder->Key) {
			ml_map_insert(Decoder->Collection, Decoder->Key, Number);
			Decoder->Key = NULL;
			Decoder->State = JS_OBJECT_REST;
		} else if (ml_is(Decoder->Collection, MLListT)) {
			ml_list_put(Decoder->Collection, Number);
			Decoder->State = JS_ARRAY_REST;
		}
	} else {
		Decoder->State = JS_VALUE;
		Decoder->emit(Decoder, Number);
	}
}

static ml_value_t *json_finish_keyword(json_decoder_t *Decoder) {
	char *String = ml_stringbuffer_get_string(Decoder->Buffer);
	ml_value_t *Value;
	if (!strcmp(String, "true")) {
		Value = (ml_value_t *)MLTrue;
	} else if (!strcmp(String, "false")) {
		Value = (ml_value_t *)MLFalse;
	} else if (!strcmp(String, "null")) {
		Value = MLNil;
	} else {
		return ml_error("JSONError", "Invalid keyword: %s", String);
	}
	if (Decoder->Collection) {
		if (Decoder->Key) {
			ml_map_insert(Decoder->Collection, Decoder->Key, Value);
			Decoder->Key = NULL;
			Decoder->State = JS_OBJECT_REST;
		} else if (ml_is(Decoder->Collection, MLListT)) {
			ml_list_put(Decoder->Collection, Value);
			Decoder->State = JS_ARRAY_REST;
		}
	} else {
		Decoder->State = JS_VALUE;
		Decoder->emit(Decoder, Value);
	}
	return NULL;
}

static ml_value_t *json_decoder_parse(json_decoder_t *Decoder, const char *Input, size_t Size) {
	while (Size-- > 0) {
		char Char = *Input++;
		switch (Decoder->State) {
		case JS_ARRAY_START:
			if (Char == ']') {
				json_decoder_pop(Decoder);
				break;
			}
			// no break
		case JS_VALUE:
			switch (Char) {
			case ' ': case '\r': case '\n': case '\t': break;
			case '\"':
				Decoder->State = JS_STRING;
				break;
			case '-':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_SIGN;
				break;
			case '0':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_ZERO;
				break;
			case '1' ... '9':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_DIGITS;
				break;
			case '{':
				json_decoder_push(Decoder, ml_map());
				Decoder->State = JS_OBJECT_START;
				break;
			case '[':
				json_decoder_push(Decoder, ml_list());
				Decoder->State = JS_ARRAY_START;
				break;
			case 'a' ... 'z':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_KEYWORD;
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_ARRAY_REST:
			switch (Char) {
			case ' ': case '\r': case '\n': case '\t': break;
			case ',':
				Decoder->State = JS_VALUE;
				break;
			case ']':
				json_decoder_pop(Decoder);
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_OBJECT_START:
			if (Char == '}') {
				json_decoder_pop(Decoder);
				break;
			}
			//no break
		case JS_KEY:
			switch (Char) {
			case ' ': case '\r': case '\n': case '\t': break;
			case '\"':
				Decoder->State = JS_STRING;
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_COLON:
			switch (Char) {
			case ' ': case '\r': case '\n': case '\t': break;
			case ':':
				Decoder->State = JS_VALUE;
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_OBJECT_REST:
			switch (Char) {
			case ' ': case '\r': case '\n': case '\t': break;
			case ',':
				Decoder->State = JS_KEY;
				break;
			case '}':
				json_decoder_pop(Decoder);
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_STRING:
			switch (Char) {
			case '\"': {
				ml_value_t *String = ml_stringbuffer_to_string(Decoder->Buffer);
				if (Decoder->Collection) {
					if (Decoder->Key) {
						ml_map_insert(Decoder->Collection, Decoder->Key, String);
						Decoder->Key = NULL;
						Decoder->State = JS_OBJECT_REST;
					} else if (ml_is(Decoder->Collection, MLMapT)) {
						Decoder->Key = String;
						Decoder->State = JS_COLON;
					} else if (ml_is(Decoder->Collection, MLListT)) {
						ml_list_put(Decoder->Collection, String);
						Decoder->State = JS_ARRAY_REST;
					}
				} else {
					Decoder->State = JS_VALUE;
					Decoder->emit(Decoder, String);
				}
				break;
			}
			case '\\':
				Decoder->State = JS_ESCAPE;
				break;
			case 0 ... 31:
				return ml_error("JSONError", "Invalid character: %c", Char);
			default:
				ml_stringbuffer_put(Decoder->Buffer, Char);
				break;
			}
			break;
		case JS_ESCAPE:
			switch (Char) {
			case '\"': case '\\': case '/':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_STRING;
				break;
			case 'b':
				ml_stringbuffer_put(Decoder->Buffer, '\b');
				Decoder->State = JS_STRING;
				break;
			case 'f':
				ml_stringbuffer_put(Decoder->Buffer, '\f');
				Decoder->State = JS_STRING;
				break;
			case 'n':
				ml_stringbuffer_put(Decoder->Buffer, '\n');
				Decoder->State = JS_STRING;
				break;
			case 'r':
				ml_stringbuffer_put(Decoder->Buffer, '\r');
				Decoder->State = JS_STRING;
				break;
			case 't':
				ml_stringbuffer_put(Decoder->Buffer, '\t');
				Decoder->State = JS_STRING;
				break;
			case 'u':
				Decoder->CodePoint = 0;
				Decoder->State = JS_UNICODE_1;
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_UNICODE_1:
			switch (Char) {
			case 'a' ... 'f':
				Decoder->CodePoint += ((Char - 'a') + 10) << 12;
				Decoder->State = JS_UNICODE_2;
				break;
			case 'A' ... 'F':
				Decoder->CodePoint += ((Char - 'A') + 10) << 12;
				Decoder->State = JS_UNICODE_2;
				break;
			case '0' ... '9':
				Decoder->CodePoint += (Char - '0') << 12;
				Decoder->State = JS_UNICODE_2;
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_UNICODE_2:
			switch (Char) {
			case 'a' ... 'f':
				Decoder->CodePoint += ((Char - 'a') + 10) << 8;
				Decoder->State = JS_UNICODE_3;
				break;
			case 'A' ... 'F':
				Decoder->CodePoint += ((Char - 'A') + 10) << 8;
				Decoder->State = JS_UNICODE_3;
				break;
			case '0' ... '9':
				Decoder->CodePoint += (Char - '0') << 8;
				Decoder->State = JS_UNICODE_3;
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_UNICODE_3:
			switch (Char) {
			case 'a' ... 'f':
				Decoder->CodePoint += ((Char - 'a') + 10) << 4;
				Decoder->State = JS_UNICODE_4;
				break;
			case 'A' ... 'F':
				Decoder->CodePoint += ((Char - 'A') + 10) << 4;
				Decoder->State = JS_UNICODE_4;
				break;
			case '0' ... '9':
				Decoder->CodePoint += (Char - '0') << 4;
				Decoder->State = JS_UNICODE_4;
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_UNICODE_4:
			switch (Char) {
			case 'a' ... 'f':
				Decoder->CodePoint += ((Char - 'a') + 10);
				ml_stringbuffer_put32(Decoder->Buffer, Decoder->CodePoint);
				Decoder->State = JS_STRING;
				break;
			case 'A' ... 'F':
				Decoder->CodePoint += ((Char - 'A') + 10);
				ml_stringbuffer_put32(Decoder->Buffer, Decoder->CodePoint);
				Decoder->State = JS_STRING;
				break;
			case '0' ... '9':
				Decoder->CodePoint += (Char - '0');
				ml_stringbuffer_put32(Decoder->Buffer, Decoder->CodePoint);
				Decoder->State = JS_STRING;
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_SIGN:
			switch (Char) {
			case '0':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_ZERO;
				break;
			case '1' ... '9':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_DIGITS;
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_DIGITS:
			switch (Char) {
			case '0' ... '9':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				break;
			case '.':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_FRACTION_FIRST;
				break;
			case 'e': case 'E':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_EXPONENT_SIGN;
				break;
			default:
				json_finish_number(Decoder);
				--Input;
				++Size;
				break;
			}
			break;
		case JS_ZERO:
			switch (Char) {
			case '.':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_FRACTION_FIRST;
				break;
			case 'e': case 'E':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_EXPONENT_SIGN;
				break;
			default:
				json_finish_number(Decoder);
				--Input;
				++Size;
				break;
			}
			break;
		case JS_FRACTION_FIRST:
			switch (Char) {
			case '0' ... '9':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_FRACTION;
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_FRACTION:
			switch (Char) {
			case '0' ... '9':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				break;
			case 'e': case 'E':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_EXPONENT_SIGN;
				break;
			default:
				json_finish_number(Decoder);
				--Input;
				++Size;
				break;
			}
			break;
		case JS_EXPONENT_SIGN:
			switch (Char) {
			case '0' ... '9':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_EXPONENT;
				break;
			case '+': case '-':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_EXPONENT_FIRST;
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_EXPONENT_FIRST:
			switch (Char) {
			case '0' ... '9':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				Decoder->State = JS_EXPONENT;
				break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		case JS_EXPONENT:
			switch (Char) {
			case '0' ... '9':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				break;
			default: {
				json_finish_number(Decoder);
				--Input;
				++Size;
				break;
			}
			}
			break;
		case JS_KEYWORD:
			switch (Char) {
			case 'a' ... 'z':
				ml_stringbuffer_put(Decoder->Buffer, Char);
				break;
			default: {
				ml_value_t *Error = json_finish_keyword(Decoder);
				if (Error) return Error;
				--Input;
				++Size;
				break;
			}
			}
			break;
		case JS_TRAILING:
			switch (Char) {
			case ' ': case '\r': case '\n': case '\t': break;
			default:
				return ml_error("JSONError", "Invalid character: %c", Char);
			}
			break;
		}
	}
	return NULL;
}

static ml_value_t *json_decoder_finish(json_decoder_t *Decoder) {
	switch (Decoder->State) {
	case JS_DIGITS:
		json_finish_number(Decoder);
		break;
	case JS_ZERO:
		json_finish_number(Decoder);
		break;
	case JS_FRACTION:
		json_finish_number(Decoder);
		break;
	case JS_EXPONENT_SIGN:
		json_finish_number(Decoder);
		break;
	case JS_EXPONENT:
		json_finish_number(Decoder);
		break;
	case JS_KEYWORD: {
		ml_value_t *Error = json_finish_keyword(Decoder);
		if (Error) return Error;
		break;
	}
	default: break;
	}
	return NULL;
}

static void json_decode_single_fn(json_decoder_t *Decoder, ml_value_t *Value) {
	Decoder->State = JS_TRAILING;
	*(ml_value_t **)Decoder->Data = Value;
}

ML_METHOD_ANON(JsonDecode, "json::decode");

ML_METHOD(JsonDecode, MLStringT) {
//@json::decode
//<Json
//>any
// Decodes :mini:`Json` into a Minilang value.
	ml_value_t *Result = NULL;
	json_decoder_t Decoder[1];
	json_decoder_init(Decoder, (void *)json_decode_single_fn, &Result);
	ml_value_t *Error = json_decoder_parse(Decoder, ml_string_value(Args[0]), ml_string_length(Args[0]));
	if (Error) return Error;
	Error = json_decoder_finish(Decoder);
	if (Error) return Error;
	return Result ?: ml_error("JSONError", "Incomplete JSON");
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Stream;
	typeof(ml_stream_read) *read;
	ml_value_t *Result;
	json_decoder_t Decoder[1];
	char Text[ML_STRINGBUFFER_NODE_SIZE];
} ml_json_stream_state_t;

static void ml_json_stream_state_run(ml_json_stream_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	size_t Length = ml_integer_value(Result);
	if (Length) {
		ml_value_t *Error = json_decoder_parse(State->Decoder, State->Text, Length);
		if (Error) ML_RETURN(Error);
		return State->read((ml_state_t *)State, State->Stream, State->Text, ML_STRINGBUFFER_NODE_SIZE);
	} else {
		ml_value_t *Error = json_decoder_finish(State->Decoder);
		if (Error) ML_RETURN(Error);
		ML_RETURN(State->Result ?: ml_error("XMLError", "Incomplete JSON"));
	}
}

ML_METHODX(JsonDecode, MLStreamT) {
//@json::decode
//<Stream
//>any
// Decodes the content of :mini:`Json` into a Minilang value.
	ml_json_stream_state_t *State = new(ml_json_stream_state_t);
	json_decoder_init(State->Decoder, (void *)json_decode_single_fn, &State->Result);
	State->Stream = Args[0];
	State->read = ml_typed_fn_get(ml_typeof(Args[0]), ml_stream_read) ?: ml_stream_read_method;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_json_stream_state_run;
	return State->read((ml_state_t *)State, State->Stream, State->Text, ML_STRINGBUFFER_NODE_SIZE);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Values, *Callback, *Result;
	ml_value_t *Args[1];
	json_decoder_t Decoder[1];
} ml_json_decoder_t;

extern ml_type_t MLJsonDecoderT[];

static void json_decode_fn(json_decoder_t *Decoder, ml_value_t *Value) {
	ml_json_decoder_t *Decoder2 = (ml_json_decoder_t *)Decoder->Data;
	ml_list_put(Decoder2->Values, Value);
}

static void ml_json_decoder_run(ml_json_decoder_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (!ml_list_length(State->Values)) ML_RETURN(State->Result);
	State->Args[0] = ml_list_pop(State->Values);
	return ml_call(State, State->Callback, 1, State->Args);
}

ML_FUNCTIONX(JsonDecoder) {
//@json::decoder
//<Callback
//>json::decoder
// Returns a new JSON decoder that calls :mini:`Callback(Value)` whenever a complete JSON value is written to the decoder.
	ML_CHECKX_ARG_COUNT(1);
	ml_json_decoder_t *Decoder = new(ml_json_decoder_t);
	Decoder->Base.Type = MLJsonDecoderT;
	Decoder->Base.run = (ml_state_fn)ml_json_decoder_run;
	Decoder->Values = ml_list();
	Decoder->Callback = Args[0];
	json_decoder_init(Decoder->Decoder, json_decode_fn, Decoder);
	ML_RETURN(Decoder);
}

ML_TYPE(MLJsonDecoderT, (MLStreamT), "json-decoder",
//@json::decoder
// A JSON decoder that can be written to as a stream and calls a user-supplied callback whenever a complete value is decoded.
	.Constructor = (ml_value_t *)JsonDecoder
);

static void ML_TYPED_FN(ml_stream_write, MLJsonDecoderT, ml_state_t *Caller, ml_json_decoder_t *Decoder, const void *Address, int Count) {
	Decoder->Result = ml_integer(Count);
	ml_value_t *Error = json_decoder_parse(Decoder->Decoder, Address, Count);
	if (Error) ML_RETURN(Error);
	if (!ml_list_length(Decoder->Values)) ML_RETURN(Decoder->Result);
	Decoder->Base.Caller = Caller;
	Decoder->Base.Context = Caller->Context;
	return ml_json_decoder_run(Decoder, MLNil);
}

static void ML_TYPED_FN(ml_stream_flush, MLJsonDecoderT, ml_state_t *Caller, ml_json_decoder_t *Decoder) {
	ml_value_t *Error = json_decoder_finish(Decoder->Decoder);
	if (Error) ML_RETURN(Error);
	if (ml_list_length(Decoder->Values)) {
		Decoder->Base.Caller = Caller;
		Decoder->Base.Context = Caller->Context;
		Decoder->Result = (ml_value_t *)Decoder;
		return ml_json_decoder_run(Decoder, MLNil);
	}
	ML_RETURN(Decoder);
}

static void ml_json_encode_string(ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	ml_stringbuffer_write(Buffer, "\"", 1);
	const unsigned char *String = (const unsigned char *)ml_string_value(Value);
	const unsigned char *End = String + ml_string_length(Value);
	while (String < End) {
		unsigned char Char = *String++;
		switch (Char) {
		case '\r': ml_stringbuffer_write(Buffer, "\\r", 2); break;
		case '\n': ml_stringbuffer_write(Buffer, "\\n", 2); break;
		case '\\': ml_stringbuffer_write(Buffer, "\\\\", 2); break;
		case '\"': ml_stringbuffer_write(Buffer, "\\\"", 2); break;
		case '\f': ml_stringbuffer_write(Buffer, "\\f", 2); break;
		case '\b': ml_stringbuffer_write(Buffer, "\\b", 2); break;
		case '\t': ml_stringbuffer_write(Buffer, "\\t", 2); break;
		default:
			if (Char < 32) {
				ml_stringbuffer_printf(Buffer, "\\u00%02x", Char);
			} else {
				ml_stringbuffer_put(Buffer, Char);
			}
			break;
		}
	}
	ml_stringbuffer_write(Buffer, "\"", 1);
}

static ml_value_t *ml_json_encode(ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	if (Value == MLNil) {
		ml_stringbuffer_write(Buffer, "null", 4);
	} else if (ml_is(Value, MLBooleanT)) {
		if (ml_boolean_value(Value)) {
			ml_stringbuffer_write(Buffer, "true", 4);
		} else {
			ml_stringbuffer_write(Buffer, "false", 5);
		}
	} else if (ml_is(Value, MLIntegerT)) {
		ml_stringbuffer_printf(Buffer, "%ld", ml_integer_value(Value));
	} else if (ml_is(Value, MLDoubleT)) {
		ml_stringbuffer_printf(Buffer, "%.20g", ml_real_value(Value));
	} else if (ml_is(Value, MLStringT)) {
		ml_json_encode_string(Buffer, Value);
	} else if (ml_is(Value, MLListT)) {
		ml_list_node_t *Node = ((ml_list_t *)Value)->Head;
		if (Node) {
			ml_stringbuffer_write(Buffer, "[", 1);
			ml_value_t *Error = ml_json_encode(Buffer, Node->Value);
			if (Error) return Error;
			while ((Node = Node->Next)) {
				ml_stringbuffer_write(Buffer, ",", 1);
				ml_value_t *Error = ml_json_encode(Buffer, Node->Value);
				if (Error) return Error;
			}
			ml_stringbuffer_write(Buffer, "]", 1);
		} else {
			ml_stringbuffer_write(Buffer, "[]", 2);
		}
	} else if (ml_is(Value, MLMapT)) {
		ml_map_node_t *Node = ((ml_map_t *)Value)->Head;
		if (Node) {
			ml_stringbuffer_write(Buffer, "{", 1);
			if (!ml_is(Node->Key, MLStringT)) return ml_error("JSONError", "JSON keys must be strings");
			ml_json_encode_string(Buffer, Node->Key);
			ml_stringbuffer_write(Buffer, ":", 1);
			ml_value_t *Error = ml_json_encode(Buffer, Node->Value);
			if (Error) return Error;
			while ((Node = Node->Next)) {
				ml_stringbuffer_write(Buffer, ",", 1);
				if (!ml_is(Node->Key, MLStringT)) return ml_error("JSONError", "JSON keys must be strings");
				ml_json_encode_string(Buffer, Node->Key);
				ml_stringbuffer_write(Buffer, ":", 1);
				ml_value_t *Error = ml_json_encode(Buffer, Node->Value);
				if (Error) return Error;
			}
			ml_stringbuffer_write(Buffer, "}", 1);
		} else {
			ml_stringbuffer_write(Buffer, "{}", 2);
		}
	} else {
		return ml_error("JSONError", "Invalid type for JSON: %s", ml_typeof(Value)->Name);
	}
	return NULL;
}

ML_METHOD_ANON(JsonEncode, "json::encode");

ML_METHOD(JsonEncode, MLAnyT) {
//@json::encode
//<Value
//>string|error
// Encodes :mini:`Value` into JSON, raising an error if :mini:`Value` cannot be represented as JSON.
	ML_CHECK_ARG_COUNT(1);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	return ml_json_encode(Buffer, Args[0]) ?: ml_stringbuffer_get_value(Buffer);
}

ML_FUNCTION(MLJson) {
//@json
//<Value:any
//>json
// Encodes :mini:`Value` into JSON.
	ML_CHECK_ARG_COUNT(1);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_value_t *Error = ml_json_encode(Buffer, Args[0]);
	if (Error) return Error;
	ml_value_t *Json = ml_stringbuffer_to_string(Buffer);
	Json->Type = MLJsonT;
	return (ml_value_t *)Json;
}

ML_TYPE(MLJsonT, (MLStringT), "json",
// Contains a JSON encoded value. Primarily used to distinguish strings containing JSON from other strings (e.g. for CBOR encoding).
	.Constructor = (ml_value_t *)MLJson
);

ML_METHOD("decode", MLJsonT) {
//<Json
//>any|error
// Decodes the JSON string in :mini:`Json` into a Minilang value.
	ml_value_t *Result = NULL;
	json_decoder_t Decoder[1];
	json_decoder_init(Decoder, json_decode_single_fn, &Result);
	const char *Text = ml_string_value(Args[0]);
	size_t Length = ml_string_length(Args[0]);
	ml_value_t *Error = json_decoder_parse(Decoder, Text, Length);
	if (Error) return Error;
	Error = json_decoder_finish(Decoder);
	if (Error) return Error;
	return Result ?: ml_error("JSONError", "Incomplete JSON");
}

ML_METHOD("value", MLJsonT) {
//<Json
//>any|error
// Decodes the JSON string in :mini:`Json` into a Minilang value.
	ml_value_t *Result = NULL;
	json_decoder_t Decoder[1];
	json_decoder_init(Decoder, json_decode_single_fn, &Result);
	const char *Text = ml_string_value(Args[0]);
	size_t Length = ml_string_length(Args[0]);
	ml_value_t *Error = json_decoder_parse(Decoder, Text, Length);
	if (Error) return Error;
	Error = json_decoder_finish(Decoder);
	if (Error) return Error;
	return Result ?: ml_error("JSONError", "Incomplete JSON");
}

#ifdef ML_CBOR

#include "ml_cbor.h"
#include "minicbor/minicbor.h"

static void ML_TYPED_FN(ml_cbor_write, MLJsonT, ml_cbor_writer_t *Writer, ml_string_t *Value) {
	ml_cbor_write_tag(Writer, 262);
	ml_cbor_write_bytes(Writer, Value->Length);
	ml_cbor_write_raw(Writer, Value->Value, Value->Length);
}

static ml_value_t *ml_cbor_read_json(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (!ml_is(Value, MLAddressT)) return ml_error("TagError", "Json requires bytes or string");
	ml_value_t *Json = ml_string_copy(ml_address_value(Value), ml_address_length(Value));
	Json->Type = MLJsonT;
	return (ml_value_t *)Json;
}

#endif

void ml_json_init(stringmap_t *Globals) {
#include "ml_json_init.c"
	stringmap_insert(MLJsonT->Exports, "encode", JsonEncode);
	stringmap_insert(MLJsonT->Exports, "decode", JsonDecode);
	stringmap_insert(MLJsonT->Exports, "decoder", MLJsonDecoderT);
	if (Globals) {
		stringmap_insert(Globals, "json", MLJsonT);
	}
#ifdef ML_CBOR
	ml_cbor_default_tag(262, ml_cbor_read_json);
#endif
}

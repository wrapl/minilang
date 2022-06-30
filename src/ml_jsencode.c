#include "ml_jsencode.h"
#include "ml_macros.h"
#include "ml_bytecode.h"
#include <string.h>

#ifdef ML_UUID
#include "ml_uuid.h"
#endif

#ifdef ML_TIME
#include "ml_time.h"
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "jsencode"

struct ml_json_encoder_cache_t {
	//inthash_t *Globals;
	inthash_t Cached[1];
	int LastIndex;
};

ml_json_encoder_cache_t *ml_json_encoder(inthash_t *Globals) {
	ml_json_encoder_cache_t *Cache = new(ml_json_encoder_cache_t);
	//Cache->Globals = Globals;
	return Cache;
}

void ml_json_encoder_add(ml_json_encoder_cache_t *Cache, ml_value_t *Value, json_t *Json) {
	inthash_insert(Cache->Cached, (uintptr_t)Value, Json);
}

json_t *ml_json_encode(ml_json_encoder_cache_t *Cache, ml_value_t *Value) {
	json_t *Json = inthash_search(Cache->Cached, (uintptr_t)Value);
	if (Json) {
		json_t *First = json_array_get(Json, 0);
		if (!json_is_integer(First)) {
			First = json_integer(Cache->LastIndex++);
			json_array_insert(Json, 0, First);
		}
		return json_pack("[o]", First);
	}
	//Json = inthash_search(Cache->Globals, (uintptr_t)Value);
	//if (Json) return Json;
	typeof(ml_json_encode) *encode = ml_typed_fn_get(ml_typeof(Value), ml_json_encode);
	if (!encode) return json_pack("[ss]", "unsupported", ml_typeof(Value)->Name);
	return encode(Cache, Value);
}

static json_t *ML_TYPED_FN(ml_json_encode, MLNilT, ml_json_encoder_cache_t *Cache, ml_value_t *Value) {
	return json_null();
}

static json_t *ML_TYPED_FN(ml_json_encode, MLBlankT, ml_json_encoder_cache_t *Cache, ml_value_t *Value) {
	return json_pack("[s]", "_");
}

static json_t *ML_TYPED_FN(ml_json_encode, MLBooleanT, ml_json_encoder_cache_t *Cache, ml_value_t *Value) {
	return json_boolean(Value == (ml_value_t *)MLTrue);
}

static json_t *ML_TYPED_FN(ml_json_encode, MLIntegerT, ml_json_encoder_cache_t *Cache, ml_value_t *Value) {
	return json_integer(ml_integer_value(Value));
}

static json_t *ML_TYPED_FN(ml_json_encode, MLDoubleT, ml_json_encoder_cache_t *Cache, ml_value_t *Value) {
	return json_real(ml_real_value(Value));
}

static json_t *ML_TYPED_FN(ml_json_encode, MLStringT, ml_json_encoder_cache_t *Cache, ml_string_t *Value) {
	return json_stringn(Value->Value, Value->Length);
}

static json_t *ML_TYPED_FN(ml_json_encode, MLRegexT, ml_json_encoder_cache_t *Cache, ml_value_t *Value) {
	return json_pack("[ss]", "r", ml_regex_pattern(Value));
}

static json_t *ML_TYPED_FN(ml_json_encode, MLMethodT, ml_json_encoder_cache_t *Cache, ml_value_t *Value) {
	return json_pack("[ss]", ":", ml_method_name(Value));
}

static json_t *ML_TYPED_FN(ml_json_encode, MLListT, ml_json_encoder_cache_t *Cache, ml_list_t *Value) {
	json_t *Json = json_array();
	json_array_append_new(Json, json_string("l"));
	inthash_insert(Cache->Cached, (uintptr_t)Value, Json);
	ML_LIST_FOREACH(Value, Iter) {
		json_array_append_new(Json, ml_json_encode(Cache, Iter->Value));
	}
	return Json;
}

static json_t *ML_TYPED_FN(ml_json_encode, MLNamesT, ml_json_encoder_cache_t *Cache, ml_value_t *Value) {
	json_t *Json = json_array();
	json_array_append_new(Json, json_string("n"));
	inthash_insert(Cache->Cached, (uintptr_t)Value, Json);
	ML_NAMES_FOREACH(Value, Iter) {
		json_array_append_new(Json, ml_json_encode(Cache, Iter->Value));
	}
	return Json;
}

static json_t *ML_TYPED_FN(ml_json_encode, MLMapT, ml_json_encoder_cache_t *Cache, ml_map_t *Value) {
	json_t *Json = json_array();
	json_array_append_new(Json, json_string("m"));
	inthash_insert(Cache->Cached, (uintptr_t)Value, Json);
	ML_MAP_FOREACH(Value, Iter) {
		json_array_append_new(Json, ml_json_encode(Cache, Iter->Key));
		json_array_append_new(Json, ml_json_encode(Cache, Iter->Value));
	}
	return Json;
}

static json_t *ML_TYPED_FN(ml_json_encode, MLTypeT, ml_json_encoder_cache_t *Cache, ml_type_t *Value) {
	return json_pack("[ss]", "t", Value->Name);
}

static json_t *ML_TYPED_FN(ml_json_encode, MLGlobalT, ml_json_encoder_cache_t *Cache, ml_value_t *Value) {
	return ml_json_encode(Cache, ml_global_get(Value));
}

static json_t *ML_TYPED_FN(ml_json_encode, MLVariableT, ml_json_encoder_cache_t *Cache, ml_value_t *Value) {
	json_t *Json = json_array();
	json_array_append_new(Json, json_string("v"));
	inthash_insert(Cache->Cached, (uintptr_t)Value, Json);
	return Json;
}

#ifdef ML_UUID

static json_t *ML_TYPED_FN(ml_json_encode, MLUUIDT, ml_json_encoder_cache_t *Cache, ml_value_t *Value) {
	char IdString[UUID_STR_LEN];
	uuid_unparse_lower(ml_uuid_value(Value), IdString);
	return json_pack("[ss]", "uuid", IdString);
}

#endif

#ifdef ML_TIME

static json_t *ML_TYPED_FN(ml_json_encode, MLTimeT, ml_json_encoder_cache_t *Cache, ml_value_t *Value) {
	struct timespec Time[1];
	ml_time_value(Value, Time);
	struct tm TM = {0,};
	gmtime_r(&Time->tv_sec, &TM);
	char Buffer[60];
	char *End = Buffer + strftime(Buffer, 50, "%FT%T", &TM);
	unsigned long NSec = Time->tv_nsec;
	*End++ = '.';
	*End++ = '0' + (NSec / 100000000) % 10;
	*End++ = '0' + (NSec / 10000000) % 10;
	*End++ = '0' + (NSec / 1000000) % 10;
	*End++ = '0' + (NSec / 100000) % 10;
	*End++ = '0' + (NSec / 10000) % 10;
	*End++ = '0' + (NSec / 1000) % 10;
	*End++ = 'Z';
	*End = 0;
	return json_pack("[ss]", "time", Buffer);
}

#endif

#ifdef ML_MATH

#include "ml_array.h"

#define ML_JSON_ENCODE_ARRAY(CTYPE, JSON) \
\
static void ml_json_encode_array_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, char *Address, json_t *Json) { \
	if (Degree == 0) { \
		json_array_append_new(Json, JSON(*(CTYPE *)Address)); \
		return; \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				ml_json_encode_array_ ## CTYPE(Degree - 1, Dimension + 1, Address + Indices[I] * Stride, Json); \
			} \
		} else { \
			for (int I = Dimension->Size; --I >= 0;) { \
				ml_json_encode_array_ ## CTYPE(Degree - 1, Dimension + 1, Address, Json); \
				Address += Stride; \
			} \
		} \
	} \
}

ML_JSON_ENCODE_ARRAY(int8_t, json_integer)
ML_JSON_ENCODE_ARRAY(uint8_t, json_integer)
ML_JSON_ENCODE_ARRAY(int16_t, json_integer)
ML_JSON_ENCODE_ARRAY(uint16_t, json_integer)
ML_JSON_ENCODE_ARRAY(int32_t, json_integer)
ML_JSON_ENCODE_ARRAY(uint32_t, json_integer)
ML_JSON_ENCODE_ARRAY(int64_t, json_integer)
ML_JSON_ENCODE_ARRAY(uint64_t, json_integer)
ML_JSON_ENCODE_ARRAY(float, json_real)
ML_JSON_ENCODE_ARRAY(double, json_real)

static void ml_json_encode_array_any(int Degree, ml_array_dimension_t *Dimension, char *Address, json_t *Json, ml_json_encoder_cache_t *Cache) {
	if (Degree == 0) {
		json_array_append_new(Json, ml_json_encode(Cache, *(ml_value_t **)Address));
	} else {
		int Stride = Dimension->Stride;
		if (Dimension->Indices) {
			int *Indices = Dimension->Indices;
			for (int I = 0; I < Dimension->Size; ++I) {
				ml_json_encode_array_any(Degree - 1, Dimension + 1, Address + Indices[I] * Stride, Json, Cache);
			}
		} else {
			for (int I = Dimension->Size; --I >= 0;) {
				ml_json_encode_array_any(Degree - 1, Dimension + 1, Address, Json, Cache);
				Address += Stride;
			}
		}
	}
}

static json_t *ML_TYPED_FN(ml_json_encode, MLArrayT, ml_json_encoder_cache_t *Cache, ml_array_t *Array) {
	const char *Type;
	json_t *Values = json_array();
	json_t *Shape = json_array();
	for (int I = 0; I < Array->Degree; ++I) json_array_append_new(Shape, json_integer(Array->Dimensions[I].Size));
	switch (Array->Format) {
	case ML_ARRAY_FORMAT_U8:
		Type = "uint8";
		ml_json_encode_array_uint8_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_I8:
		Type = "int8";
		ml_json_encode_array_int8_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_U16:
		Type = "uint16";
		ml_json_encode_array_uint16_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_I16:
		Type = "int16";
		ml_json_encode_array_int16_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_U32:
		Type = "uint32";
		ml_json_encode_array_uint32_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_I32:
		Type = "int32";
		ml_json_encode_array_int32_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_U64:
		Type = "uint64";
		ml_json_encode_array_uint64_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_I64:
		Type = "int64";
		ml_json_encode_array_int64_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_F32:
		Type = "float32";
		ml_json_encode_array_float(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_F64:
		Type = "float64";
		ml_json_encode_array_double(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_ANY:
		Type = "any";
		ml_json_encode_array_any(Array->Degree, Array->Dimensions, Array->Base.Value, Values, Cache);
		break;
	default:
		return json_pack("[ss]", "unsupported", Array->Base.Type->Name);
	}
	return json_pack("[ssoo]", "array", Type, Shape, Values);
}

#endif

static json_t *ml_closure_info_encode(ml_closure_info_t *Info, ml_json_encoder_cache_t *Cache);

typedef struct {
	json_t *Json;
	inthash_t Cache[1];
} ml_decls_json_t;

static json_t *ml_closure_decl_encode(ml_decl_t *Decl, ml_decls_json_t *Decls) {
	if (!Decl) return json_integer(-1);
	json_t *Index = (json_t *)inthash_search(Decls->Cache, (uintptr_t)Decl);
	if (Index) return Index;
	json_t *Next = ml_closure_decl_encode(Decl->Next, Decls);
	Index = json_integer(json_array_size(Decls->Json));
	json_array_append_new(Decls->Json, json_pack("[Osiii]",
		Next, Decl->Ident,
		Decl->Source.Line,
		Decl->Index, Decl->Flags
	));
	inthash_insert(Decls->Cache, (uintptr_t)Decl, Index);
	return Index;
}

static int ml_closure_info_param_fn(const char *Name, void *Index, json_t *Params) {
	json_array_set(Params, (intptr_t)Index - 1, json_string(Name));
	return 0;
}

static int ml_closure_find_labels(ml_inst_t *Inst, uintptr_t *Offset) {
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_NONE: *Offset += 2; return 1;
	case MLIT_INST: *Offset += 3; return 2;
	case MLIT_INST_COUNT_DECL: *Offset += 5; return 4;
	case MLIT_COUNT_COUNT: *Offset += 4; return 3;
	case MLIT_COUNT: *Offset += 3; return 2;
	case MLIT_VALUE: *Offset += 3; return 2;
	case MLIT_VALUE_DATA: *Offset += 3; return 3;
	case MLIT_VALUE_COUNT: *Offset += 4; return 3;
	case MLIT_VALUE_COUNT_DATA: *Offset += 4; return 4;
	case MLIT_COUNT_CHARS: *Offset += 3; return 3;
	case MLIT_DECL: *Offset += 3; return 2;
	case MLIT_COUNT_DECL: *Offset += 4; return 3;
	case MLIT_COUNT_COUNT_DECL: *Offset += 5; return 4;
	case MLIT_CLOSURE:
		*Offset += 3 + Inst[1].ClosureInfo->NumUpValues;
		return 2 + Inst[1].ClosureInfo->NumUpValues;
	case MLIT_SWITCH: *Offset += 3; return 3;
	default: __builtin_unreachable();
	}
}

static int ml_closure_inst_encode(ml_inst_t *Inst, ml_json_encoder_cache_t *Cache, json_t *Json, inthash_t *Labels, ml_decls_json_t *Decls) {
	json_array_append_new(Json, json_integer(Inst->Opcode));
	json_array_append_new(Json, json_integer(Inst->Line));
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_NONE:
		return 1;
	case MLIT_INST:
		json_array_append_new(Json, json_integer((uintptr_t)inthash_search(Labels, Inst[1].Inst->Label)));
		return 2;
	case MLIT_INST_COUNT_DECL:
		json_array_append_new(Json, json_integer((uintptr_t)inthash_search(Labels, Inst[1].Inst->Label)));
		json_array_append_new(Json, json_integer(Inst[2].Count));
		json_array_append(Json, ml_closure_decl_encode(Inst[3].Decls, Decls));
		return 4;
	case MLIT_COUNT_COUNT:
		json_array_append_new(Json, json_integer(Inst[1].Count));
		json_array_append_new(Json, json_integer(Inst[2].Count));
		return 3;
	case MLIT_COUNT:
		json_array_append_new(Json, json_integer(Inst[1].Count));
		return 2;
	case MLIT_VALUE:
		json_array_append_new(Json, ml_json_encode(Cache, Inst[1].Value));
		return 2;
	case MLIT_VALUE_DATA:
		json_array_append_new(Json, ml_json_encode(Cache, Inst[1].Value));
		return 3;
	case MLIT_VALUE_COUNT:
		json_array_append_new(Json, ml_json_encode(Cache, Inst[1].Value));
		json_array_append_new(Json, json_integer(Inst[2].Count));
		return 3;
	case MLIT_VALUE_COUNT_DATA:
		json_array_append_new(Json, ml_json_encode(Cache, Inst[1].Value));
		json_array_append_new(Json, json_integer(Inst[2].Count));
		return 4;
	case MLIT_COUNT_CHARS:
		json_array_append_new(Json, json_stringn(Inst[2].Chars, Inst[1].Count));
		return 3;
	case MLIT_DECL:
		json_array_append(Json, ml_closure_decl_encode(Inst[1].Decls, Decls));
		return 2;
	case MLIT_COUNT_DECL:
		json_array_append_new(Json, json_integer(Inst[1].Count));
		json_array_append(Json, ml_closure_decl_encode(Inst[2].Decls, Decls));
		return 3;
	case MLIT_COUNT_COUNT_DECL:
		json_array_append_new(Json, json_integer(Inst[1].Count));
		json_array_append_new(Json, json_integer(Inst[2].Count));
		json_array_append(Json, ml_closure_decl_encode(Inst[3].Decls, Decls));
		return 4;
	case MLIT_CLOSURE: {
		ml_closure_info_t *Info = Inst[1].ClosureInfo;
		json_array_append_new(Json, ml_closure_info_encode(Info, Cache));
		for (int N = 0; N < Info->NumUpValues; ++N) {
			json_array_append_new(Json, json_integer(Inst[2 + N].Count));
		}
		return 2 + Info->NumUpValues;
	}
	case MLIT_SWITCH: {
		json_t *Insts = json_array();
		json_array_append_new(Insts, json_string("l"));
		for (int N = 0; N < Inst[1].Count; ++N) {
			json_array_append_new(Insts, json_integer((uintptr_t)inthash_search(Labels, Inst[2].Insts[N]->Label)));
		}
		json_array_append_new(Json, Insts);
		return 3;
	}
	default: __builtin_unreachable();
	}
}

static json_t *ml_closure_info_encode(ml_closure_info_t *Info, ml_json_encoder_cache_t *Cache) {
	json_t *Json = json_array();
	json_array_append_new(Json, json_string("!"));
	json_array_append_new(Json, json_integer(ML_BYTECODE_VERSION));
	json_array_append_new(Json, json_string(Info->Source ?: ""));
	json_array_append_new(Json, json_integer(Info->StartLine));
	json_array_append_new(Json, json_integer(Info->FrameSize));
	json_array_append_new(Json, json_integer(Info->NumParams));
	json_array_append_new(Json, json_integer(Info->NumUpValues));
	json_array_append_new(Json, json_integer(!!(Info->Flags & ML_CLOSURE_EXTRA_ARGS)));
	json_array_append_new(Json, json_integer(!!(Info->Flags & ML_CLOSURE_NAMED_ARGS)));
	json_t *Params = json_array();
	for (int I = 0; I < Info->Params->Size; ++I) json_array_append_new(Params, json_null());
	stringmap_foreach(Info->Params, Params, (void *)ml_closure_info_param_fn);
	json_array_append_new(Json, Params);
	json_t *Instructions = json_array();
	inthash_t Labels[1] = {INTHASH_INIT};
	uintptr_t Offset = 0, Return = 0;
	ml_closure_info_labels(Info);
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Label) inthash_insert(Labels, Inst->Label, (void *)Offset);
		if (Inst->Opcode == MLI_LINK) {
			Inst = Inst[1].Inst;
		} else {
			if (Inst == Info->Return) Return = Offset;
			Inst += ml_closure_find_labels(Inst, &Offset);
		}
	}
	ml_decls_json_t Decls[1] = {{json_array(), {INTHASH_INIT}}};
	json_t *InitDecls = ml_closure_decl_encode(Info->Decls, Decls);
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Opcode == MLI_LINK) {
			Inst = Inst[1].Inst;
		} else {
			Inst += ml_closure_inst_encode(Inst, Cache, Instructions, Labels, Decls);
		}
	}
	json_array_append_new(Json, json_integer(0));
	json_array_append_new(Json, json_integer(Return));
	json_array_append(Json, InitDecls);
	json_array_append_new(Json, Instructions);
	json_array_append_new(Json, Decls->Json);
	return Json;
}


static json_t *ML_TYPED_FN(ml_json_encode, MLClosureT, ml_json_encoder_cache_t *Cache, ml_closure_t *Value) {
	json_t *Json = json_array();
	json_array_append_new(Json, json_string("z"));
	inthash_insert(Cache->Cached, (uintptr_t)Value, Json);
	ml_closure_info_t *Info = Value->Info;
	json_array_append_new(Json, ml_closure_info_encode(Info, Cache));
	for (int I = 0; I < Info->NumUpValues; ++I) {
		json_array_append_new(Json, ml_json_encode(Cache, Value->UpValues[I]));
	}
	return Json;
}

static json_t *ML_TYPED_FN(ml_json_encode, MLExternalT, ml_json_encoder_cache_t *Cache, ml_external_t *Value) {
	return json_pack("[ss]", "^", Value->Name);
}

typedef struct {
	ml_type_t *Type;
	//inthash_t Globals[1];
} ml_json_encoder_t;

extern ml_type_t JSEncoderT[1];

ML_FUNCTION(JSEncoder) {
	ml_json_encoder_t *Encoder = new(ml_json_encoder_t);
	Encoder->Type = JSEncoderT;
	return (ml_value_t *)Encoder;
}

ML_TYPE(JSEncoderT, (), "js-encoder",
	.Constructor = (ml_value_t *)JSEncoder
);

/*ML_METHOD("add", JSEncoderT, MLAnyT, MLStringT) {
	ml_json_encoder_t *Encoder = (ml_json_encoder_t *)Args[0];
	inthash_insert(Encoder->Globals,
		(uintptr_t)Args[1],
		json_pack("[ss]", "^", ml_string_value(Args[2]))
	);
	return Args[0];
}*/

ML_METHOD("encode", JSEncoderT, MLAnyT) {
	ml_json_encoder_t *Encoder = (ml_json_encoder_t *)Args[0];
	ml_json_encoder_cache_t Cache[1] = {0,};
	//Cache->Globals = Encoder->Globals;
	json_t *Json = ml_json_encode(Cache, Args[1]);
	const char *String = json_dumps(Json, JSON_ENCODE_ANY | JSON_COMPACT);
	return ml_string(String, -1);
}

struct ml_json_decoder_cache_t {
	stringmap_t *Globals;
	inthash_t Cached[1];
};

typedef struct {
	ml_type_t *Type;
	stringmap_t Globals[1];
} ml_json_decoder_t;

static stringmap_t Decoders[1] = {STRINGMAP_INIT};

typedef ml_value_t *(*ml_json_decode_fn)(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Count);

ml_value_t *ml_json_decode(ml_json_decoder_cache_t *Cache, json_t *Json) {
	switch (Json->type) {
	case JSON_OBJECT: return ml_error("JSONError", "Unsupported JSON value");
	case JSON_ARRAY: {
		size_t Size = json_array_size(Json);
		if (!Size) return ml_error("JSONError", "Unsupported JSON value");
		json_t *First = json_array_get(Json, 0);
		json_incref(First);
		json_array_remove(Json, 0);
		intptr_t Index = -1;
		if (First->type == JSON_INTEGER) {
			Index = json_integer_value(First);
			if (Size == 1) {
				return inthash_search(Cache->Cached, Index) ?: ml_error("JSONError", "Unknown cached reference");
			} else {
				First = json_array_get(Json, 0);
				json_incref(First);
				json_array_remove(Json, 0);
			}
		}
		if (First->type == JSON_STRING) {
			const char *Name = json_string_value(First);
			ml_json_decode_fn decode = (ml_json_decode_fn)stringmap_search(Decoders, Name);
			if (!decode) return ml_error("JSONError", "Unsupported JSON decoder: %s", Name);
			return decode(Cache, Json, Index);
		} else {
			 return ml_error("JSONError", "Unsupported JSON value");
		}
	}
	case JSON_STRING: return ml_string(json_string_value(Json), json_string_length(Json));
	case JSON_INTEGER: return ml_integer(json_integer_value(Json));
	case JSON_REAL: return ml_real(json_real_value(Json));
	case JSON_TRUE: return (ml_value_t *)MLTrue;
	case JSON_FALSE: return (ml_value_t *)MLFalse;
	case JSON_NULL: return MLNil;
	default: return MLNil;
	}
}

static ml_value_t *ml_json_decode_global(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Index) {
	json_t *Value = json_array_get(Json, 0);
	if (!json_is_string(Value)) return ml_error("TypeError", "Global requires string name");
	const char *Name = json_string_value(Value);
	return stringmap_search(Cache->Globals, Name) ?: ml_error("NameError", "Unknown global %s", Name);
}

static ml_value_t *ml_json_decode_blank(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Index) {
	return MLBlank;
}

static ml_value_t *ml_json_decode_some(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Index) {
	return MLSome;
}

static ml_value_t *ml_json_decode_list(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Index) {
	ml_value_t *List = ml_list();
	if (Index >= 0) inthash_insert(Cache->Cached, Index, List);
	for (int I = 0; I < json_array_size(Json); ++I) {
		ml_value_t *Value = ml_json_decode(Cache, json_array_get(Json, I));
		if (ml_is_error(Value)) return Value;
		ml_list_put(List, Value);
	}
	return List;
}

static ml_value_t *ml_json_decode_names(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Index) {
	ml_value_t *Names = ml_names();
	if (Index >= 0) inthash_insert(Cache->Cached, Index, Names);
	for (int I = 0; I < json_array_size(Json); ++I) {
		json_t *Value = json_array_get(Json, I);
		if (!json_is_string(Value)) return ml_error("TypeError", "Names requires strings");
		ml_names_add(Names, ml_string(json_string_value(Value), json_string_length(Value)));
	}
	return Names;
}

static ml_value_t *ml_json_decode_map(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Index) {
	int Size = json_array_size(Json);
	if (Size % 2) return ml_error("JsonError", "Invalid JSON map");
	ml_value_t *Map = ml_map();
	if (Index >= 0) inthash_insert(Cache->Cached, Index, Map);
	for (int I = 0; I < Size; I += 2) {
		ml_value_t *Key = ml_json_decode(Cache, json_array_get(Json, I));
		if (ml_is_error(Key)) return Key;
		ml_value_t *Value = ml_json_decode(Cache, json_array_get(Json, I + 1));
		if (ml_is_error(Value)) return Value;
		ml_map_insert(Map, Key, Value);
	}
	return Map;
}

static ml_value_t *ml_json_decode_regex(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Index) {
	json_t *Value = json_array_get(Json, 0);
	if (!json_is_string(Value)) return ml_error("TypeError", "Regex requires strings");
	return ml_regex(json_string_value(Value), json_string_length(Value));
}

static ml_value_t *ml_json_decode_method(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Index) {
	json_t *Value = json_array_get(Json, 0);
	if (!json_is_string(Value)) return ml_error("TypeError", "Method requires strings");
	return ml_method(json_string_value(Value));
}

static ml_value_t *ml_json_decode_type(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Index) {
	json_t *Value = json_array_get(Json, 0);
	if (!json_is_string(Value)) return ml_error("TypeError", "Type requires strings");
	return MLNil;
}

#ifdef ML_UUID

static ml_value_t *ml_json_decode_uuid(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Count) {
	json_t *Value = json_array_get(Json, 0);
	if (!json_is_string(Value)) return ml_error("TypeError", "UUID requires strings");
	return ml_uuid_parse(json_string_value(Value), json_string_length(Value));
}

#endif

#ifdef ML_TIME

static ml_value_t *ml_json_decode_time(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Count) {
	json_t *Value = json_array_get(Json, 0);
	if (!json_is_string(Value)) return ml_error("TypeError", "Time requires strings");
	return ml_time_parse(json_string_value(Value), json_string_length(Value));
}

#endif

#ifdef ML_MATH

static ml_value_t *ml_json_decode_array(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Count) {
	json_t *Value = json_array_get(Json, 0);
	if (!json_is_string(Value)) return ml_error("TypeError", "Time requires strings");
	return ml_time_parse(json_string_value(Value), json_string_length(Value));
}

#endif

static ml_value_t *ml_json_decode_variable(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Index) {
	return ml_variable(MLNil, NULL);
}

static ml_closure_info_t *ml_json_decode_closure_info(ml_json_decoder_cache_t *Cache, json_t *Json) {
	ml_closure_info_t *Info = new(ml_closure_info_t);
	Info->Name = "<js-closure>";
	const char *Exclaimation;
	int Version = -1, ExtraArgs, NamedArgs, InitDecl;
	json_t *Params, *Instructions, *DeclsJson;
	int Entry, Return;
	json_unpack(Json, "[sisiiiiiioiiioo]",
		&Exclaimation,
		&Version,
		&Info->Source,
		&Info->StartLine,
		&Info->FrameSize,
		&Info->NumParams,
		&Info->NumUpValues,
		&ExtraArgs,
		&NamedArgs,
		&Params,
		&Entry,
		&Return,
		&InitDecl,
		&Instructions,
		&DeclsJson
	);
	if (Version != ML_BYTECODE_VERSION) return NULL;
	if (ExtraArgs) Info->Flags |= ML_CLOSURE_EXTRA_ARGS;
	if (NamedArgs) Info->Flags |= ML_CLOSURE_NAMED_ARGS;
	for (int I = 0; I < json_array_size(Params); ++I) {
		stringmap_insert(Info->Params, json_string_value(json_array_get(Params, I)), (void *)(uintptr_t)(I + 1));
	}
	int *Offsets = anew(int, json_array_size(Instructions));
	int Offset = 0;
	for (int I = 0; I < json_array_size(Instructions);) {
		Offsets[I] = Offset;
		ml_opcode_t Opcode = json_integer_value(json_array_get(Instructions, I));
		switch (MLInstTypes[Opcode]) {
		case MLIT_NONE:
			I += 2; Offset += 1; break;
		case MLIT_INST:
		case MLIT_COUNT:
		case MLIT_VALUE:
			I += 3; Offset += 2; break;
		case MLIT_VALUE_DATA:
			I += 3; Offset += 3; break;
		case MLIT_COUNT_COUNT:
		case MLIT_VALUE_COUNT:
			I += 4; Offset += 3; break;
		case MLIT_VALUE_COUNT_DATA:
			I += 4; Offset += 4; break;
		case MLIT_COUNT_CHARS:
			I += 3; Offset += 3; break;
		case MLIT_DECL:
			I += 3; Offset += 2; break;
		case MLIT_COUNT_DECL:
			I += 4; Offset += 3; break;
		case MLIT_INST_COUNT_DECL:
		case MLIT_COUNT_COUNT_DECL:
			I += 5; Offset += 4; break;
		case MLIT_CLOSURE: {
			int NumUpValues = json_integer_value(json_array_get(json_array_get(Instructions, I + 2), 5));
			I += 3 + NumUpValues; Offset += 2 + NumUpValues;
			break;
		}
		case MLIT_SWITCH:
			I += 3; Offset += 3;
			break;
		}
	}
	ml_decl_t **Decls = anew(ml_decl_t *, json_array_size(DeclsJson));
	for (int I = 0; I < json_array_size(DeclsJson); ++I) {
		int Next;
		ml_decl_t *Decl = Decls[I] = new(ml_decl_t);
		Decl->Source.Name = Info->Source;
		json_unpack(json_array_get(DeclsJson, I), "[isiii]",
			&Next, &Decl->Ident,
			&Decl->Source.Line,
			&Decl->Index, &Decl->Flags
		);
		if (Next >= 0) Decl->Next = Decls[Next];
	}
	if (InitDecl >= 0) Info->Decls = Decls[InitDecl];
	ml_inst_t *Code = anew(ml_inst_t, Offset);
	Info->Halt = Code + Offset;
	ml_inst_t *Inst = Code;
	for (int I = 0; I < json_array_size(Instructions);) {
		Inst->Opcode = json_integer_value(json_array_get(Instructions, I++));
		int Line = Inst->Line = json_integer_value(json_array_get(Instructions, I++));
		if (Line > Info->EndLine) Info->EndLine = Line;
		switch (MLInstTypes[Inst->Opcode]) {
		case MLIT_NONE:
			Inst += 1; break;
		case MLIT_INST:
			Inst[1].Inst = Code + Offsets[json_integer_value(json_array_get(Instructions, I++))];
			Inst += 2; break;
		case MLIT_INST_COUNT_DECL:
			Inst[1].Inst = Code + Offsets[json_integer_value(json_array_get(Instructions, I++))];
			Inst[2].Count = json_integer_value(json_array_get(Instructions, I++));
			Inst[3].Decls = Decls[json_integer_value(json_array_get(Instructions, I++))];
			Inst += 4; break;
		case MLIT_COUNT:
			Inst[1].Count = json_integer_value(json_array_get(Instructions, I++));
			Inst += 2; break;
		case MLIT_VALUE:
			Inst[1].Value = ml_json_decode(Cache, json_array_get(Instructions, I++));
			Inst += 2; break;
		case MLIT_VALUE_DATA:
			Inst[1].Value = ml_json_decode(Cache, json_array_get(Instructions, I++));
			Inst += 3; break;
		case MLIT_COUNT_COUNT:
			Inst[1].Count = json_integer_value(json_array_get(Instructions, I++));
			Inst[2].Count = json_integer_value(json_array_get(Instructions, I++));
			Inst += 3; break;
		case MLIT_VALUE_COUNT:
			Inst[1].Value = ml_json_decode(Cache, json_array_get(Instructions, I++));
			Inst[2].Count = json_integer_value(json_array_get(Instructions, I++));
			Inst += 3; break;
		case MLIT_VALUE_COUNT_DATA:
			Inst[1].Value = ml_json_decode(Cache, json_array_get(Instructions, I++));
			Inst[2].Count = json_integer_value(json_array_get(Instructions, I++));
			Inst += 4; break;
		case MLIT_COUNT_CHARS: {
			json_t *Chars = json_array_get(Instructions, I++);
			Inst[1].Count = json_string_length(Chars);
			Inst[2].Chars = json_string_value(Chars);
			Inst += 3; break;
		}
		case MLIT_DECL:
			Inst[1].Decls = Decls[json_integer_value(json_array_get(Instructions, I++))];
			Inst += 2; break;
		case MLIT_COUNT_DECL:
			Inst[1].Count = json_integer_value(json_array_get(Instructions, I++));
			Inst[2].Decls = Decls[json_integer_value(json_array_get(Instructions, I++))];
			Inst += 3; break;
		case MLIT_COUNT_COUNT_DECL:
			Inst[1].Count = json_integer_value(json_array_get(Instructions, I++));
			Inst[2].Count = json_integer_value(json_array_get(Instructions, I++));
			Inst[3].Decls = Decls[json_integer_value(json_array_get(Instructions, I++))];
			Inst += 4; break;
		case MLIT_CLOSURE:
			Inst[1].ClosureInfo = ml_json_decode_closure_info(Cache, json_array_get(Instructions, I++));
			for (int J = 0; J < Inst[1].ClosureInfo->NumUpValues; ++J) {
				Inst[J + 2].Count = json_integer_value(json_array_get(Instructions, I++));
			}
			Inst += 2 + Inst[1].ClosureInfo->NumUpValues;
			break;
		case MLIT_SWITCH: {
			json_t *Switches = json_array_get(Instructions, I++);
			Inst[1].Count = json_array_size(Switches) - 1;
			ml_inst_t **Ptr = Inst[2].Insts = anew(ml_inst_t *, Inst[1].Count);
			for (int J = 0; J < Inst[1].Count; ++J) {
				*Ptr++ = Code + Offsets[json_integer_value(json_array_get(Switches, J + 1))];
			}
			Inst += 3;
			break;
		}
		}
	}
	Info->Entry = Code + Offsets[Entry];
	Info->Return = Code + Offsets[Return];
	return Info;
}

static ml_value_t *ml_json_decode_closure(ml_json_decoder_cache_t *Cache, json_t *Json, intptr_t Index) {
	json_t *InfoJson = json_array_get(Json, 0);
	int NumUpValues = json_integer_value(json_array_get(InfoJson, 6));
	ml_closure_t *Closure = xnew(ml_closure_t, NumUpValues, ml_value_t *);
	Closure->Type = MLClosureT;
	if (Index >= 0) inthash_insert(Cache->Cached, Index, Closure);
	Closure->Info = ml_json_decode_closure_info(Cache, InfoJson);
	if (!Closure->Info) return ml_error("VersionError", "Bytecode version mismatch");
	for (int I = 0; I < NumUpValues; ++I) {
		Closure->UpValues[I] = ml_json_decode(Cache, json_array_get(Json, I + 1));
	}
	return (ml_value_t *)Closure;
}

extern ml_type_t JSDecoderT[1];

ML_FUNCTION(JSDecoder) {
	ml_json_decoder_t *Decoder = new(ml_json_decoder_t);
	Decoder->Type = JSDecoderT;
	return (ml_value_t *)Decoder;
}

ML_TYPE(JSDecoderT, (), "js-decoder",
	.Constructor = (ml_value_t *)JSDecoder
);

ML_METHOD("add", JSDecoderT, MLStringT, MLAnyT) {
	ml_json_decoder_t *Decoder = (ml_json_decoder_t *)Args[0];
	stringmap_insert(Decoder->Globals, ml_string_value(Args[1]), Args[2]);
	return Args[0];
}

ML_METHOD("decode", JSDecoderT, MLStringT) {
	ml_json_decoder_t *Decoder = (ml_json_decoder_t *)Args[0];
	ml_json_decoder_cache_t Cache[1] = {0,};
	Cache->Globals = Decoder->Globals;
	json_error_t Error;
	json_t *Json = json_loadb(ml_string_value(Args[1]), ml_string_length(Args[1]), JSON_DECODE_ANY, &Error);
	if (!Json) {
		return ml_error("JSONError", "%d: %s", Error.position, Error.text);
	}
	return ml_json_decode(Cache, Json);
}

void ml_jsencode_init(stringmap_t *Globals) {
	stringmap_insert(Decoders, "^", ml_json_decode_global);
	stringmap_insert(Decoders, "blank", ml_json_decode_blank);
	stringmap_insert(Decoders, "_", ml_json_decode_blank);
	stringmap_insert(Decoders, "some", ml_json_decode_some);
	stringmap_insert(Decoders, "list", ml_json_decode_list);
	stringmap_insert(Decoders, "l", ml_json_decode_list);
	stringmap_insert(Decoders, "names", ml_json_decode_names);
	stringmap_insert(Decoders, "n", ml_json_decode_names);
	stringmap_insert(Decoders, "map", ml_json_decode_map);
	stringmap_insert(Decoders, "m", ml_json_decode_map);
	stringmap_insert(Decoders, "type", ml_json_decode_type);
	stringmap_insert(Decoders, "t", ml_json_decode_type);
	stringmap_insert(Decoders, "regex", ml_json_decode_regex);
	stringmap_insert(Decoders, "r", ml_json_decode_regex);
	stringmap_insert(Decoders, "method", ml_json_decode_method);
	stringmap_insert(Decoders, ":", ml_json_decode_method);
	stringmap_insert(Decoders, "variable", ml_json_decode_variable);
	stringmap_insert(Decoders, "v", ml_json_decode_variable);
	stringmap_insert(Decoders, "closure", ml_json_decode_closure);
	stringmap_insert(Decoders, "z", ml_json_decode_closure);
#ifdef ML_TIME
	stringmap_insert(Decoders, "time", ml_json_decode_time);
#endif
#ifdef ML_MATH
	stringmap_insert(Decoders, "array", ml_json_decode_array);
#endif
#ifdef ML_UUID
	stringmap_insert(Decoders, "uuid", ml_json_decode_uuid);
#endif
#include "ml_jsencode_init.c"
	if (Globals) {
		stringmap_insert(Globals, "jsencoder", JSEncoderT);
		stringmap_insert(Globals, "jsdecoder", JSDecoderT);
	}
}

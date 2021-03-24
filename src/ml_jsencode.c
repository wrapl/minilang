#include "ml_jsencode.h"
#include "ml_macros.h"
#include "ml_bytecode.h"
#include <string.h>

struct ml_json_encoder_t {
	inthash_t *Specials;
	inthash_t Cached[1];
	int LastIndex;
};

ml_json_encoder_t *ml_json_encoder(inthash_t *Specials) {
	ml_json_encoder_t *Encoder = new(ml_json_encoder_t);
	Encoder->Specials = Specials;
	return Encoder;
}

void ml_json_encoder_add(ml_json_encoder_t *Encoder, ml_value_t *Value, json_t *Json) {
	inthash_insert(Encoder->Cached, (uintptr_t)Value, Json);
}

json_t *ml_json_encode(ml_json_encoder_t *Encoder, ml_value_t *Value) {
	json_t *Json = inthash_search(Encoder->Cached, (uintptr_t)Value);
	if (Json) {
		json_t *First = json_array_get(Json, 0);
		if (!json_is_integer(First)) {
			First = json_integer(Encoder->LastIndex++);
			json_array_insert(Json, 0, First);
		}
		return json_pack("[o]", First);
	}
	Json = inthash_search(Encoder->Specials, (uintptr_t)Value);
	if (Json) return Json;
	typeof(ml_json_encode) *encode = ml_typed_fn_get(ml_typeof(Value), ml_json_encode);
	if (!encode) return json_pack("[ss]", "unsupported", ml_typeof(Value)->Name);
	return encode(Encoder, Value);
}

static json_t *ML_TYPED_FN(ml_json_encode, MLNilT, ml_json_encoder_t *Encoder, ml_value_t *Value) {
	return json_null();
}

static json_t *ML_TYPED_FN(ml_json_encode, MLBooleanT, ml_json_encoder_t *Encoder, ml_value_t *Value) {
	return json_boolean(Value == (ml_value_t *)MLTrue);
}

static json_t *ML_TYPED_FN(ml_json_encode, MLIntegerT, ml_json_encoder_t *Encoder, ml_value_t *Value) {
	return json_integer(ml_integer_value(Value));
}

static json_t *ML_TYPED_FN(ml_json_encode, MLRealT, ml_json_encoder_t *Encoder, ml_value_t *Value) {
	return json_real(ml_real_value(Value));
}

static json_t *ML_TYPED_FN(ml_json_encode, MLStringT, ml_json_encoder_t *Encoder, ml_string_t *Value) {
	return json_stringn(Value->Value, Value->Length);
}

static json_t *ML_TYPED_FN(ml_json_encode, MLMethodT, ml_json_encoder_t *Encoder, ml_value_t *Value) {
	return json_pack("[ss]", "method", ml_method_name(Value));
}

static json_t *ML_TYPED_FN(ml_json_encode, MLListT, ml_json_encoder_t *Encoder, ml_list_t *Value) {
	json_t *Json = json_array();
	json_array_append(Json, json_string("list"));
	inthash_insert(Encoder->Cached, (uintptr_t)Value, Json);
	ML_LIST_FOREACH(Value, Iter) {
		json_array_append(Json, ml_json_encode(Encoder, Iter->Value));
	}
	return Json;
}

static json_t *ML_TYPED_FN(ml_json_encode, MLNamesT, ml_json_encoder_t *Encoder, ml_list_t *Value) {
	json_t *Json = json_array();
	json_array_append(Json, json_string("names"));
	inthash_insert(Encoder->Cached, (uintptr_t)Value, Json);
	ML_LIST_FOREACH(Value, Iter) {
		json_array_append(Json, ml_json_encode(Encoder, Iter->Value));
	}
	return Json;
}

static json_t *ML_TYPED_FN(ml_json_encode, MLMapT, ml_json_encoder_t *Encoder, ml_map_t *Value) {
	json_t *Json = json_array();
	json_array_append(Json, json_string("map"));
	inthash_insert(Encoder->Cached, (uintptr_t)Value, Json);
	ML_MAP_FOREACH(Value, Iter) {
		json_array_append(Json, ml_json_encode(Encoder, Iter->Key));
		json_array_append(Json, ml_json_encode(Encoder, Iter->Value));
	}
	return Json;
}

static json_t *ML_TYPED_FN(ml_json_encode, MLTypeT, ml_json_encoder_t *Encoder, ml_type_t *Value) {
	return json_pack("[ss]", "type", Value->Name);
}

static json_t *ML_TYPED_FN(ml_json_encode, MLGlobalT, ml_json_encoder_t *Encoder, ml_global_t *Value) {
	json_t *Json = json_array();
	json_array_append(Json, json_string("global"));
	inthash_insert(Encoder->Cached, (uintptr_t)Value, Json);
	json_array_append(Json, ml_json_encode(Encoder, Value->Value));
	return Json;
}

static json_t *ml_closure_info_encode(ml_closure_info_t *Info, ml_json_encoder_t *Encoder);

static int ml_closure_inst_encode(ml_inst_t *Inst, ml_json_encoder_t *Encoder, json_t *Json, inthash_t *Labels) {
	json_array_append(Json, json_integer(Inst->Opcode));
	json_array_append(Json, json_integer(Inst->LineNo));
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_NONE:
		return 1;
	case MLIT_INST:
		json_array_append(Json, json_integer((uintptr_t)inthash_search(Labels, Inst[1].Inst->Label)));
		return 2;
	case MLIT_INST_TYPES: {
		json_array_append(Json, json_integer((uintptr_t)inthash_search(Labels, Inst[1].Inst->Label)));
		json_t *Types = json_array();
		for (const char **Ptr= Inst[2].Ptrs; *Ptr; ++Ptr) {
			json_array_append(Types, json_string(*Ptr));
		}
		return 3;
	}
	case MLIT_COUNT_COUNT:
		json_array_append(Json, json_integer(Inst[1].Count));
		json_array_append(Json, json_integer(Inst[2].Count));
		return 3;
	case MLIT_COUNT:
		json_array_append(Json, json_integer(Inst[1].Count));
		return 2;
	case MLIT_INDEX:
		json_array_append(Json, json_integer(Inst[1].Index));
		return 2;
	case MLIT_VALUE:
		json_array_append(Json, ml_json_encode(Encoder, Inst[1].Value));
		return 2;
	case MLIT_VALUE_VALUE:
		json_array_append(Json, ml_json_encode(Encoder, Inst[1].Value));
		json_array_append(Json, ml_json_encode(Encoder, Inst[2].Value));
		return 3;
	case MLIT_INDEX_COUNT:
		json_array_append(Json, json_integer(Inst[1].Index));
		json_array_append(Json, json_integer(Inst[2].Count));
		return 3;
	case MLIT_INDEX_CHARS:
		json_array_append(Json, json_integer(Inst[1].Index));
		json_array_append(Json, json_string(Inst[2].Ptr));
		return 3;
	case MLIT_COUNT_VALUE:
		json_array_append(Json, json_integer(Inst[1].Count));
		json_array_append(Json, ml_json_encode(Encoder, Inst[2].Value));
		return 3;
	case MLIT_COUNT_CHARS:
		json_array_append(Json, json_stringn(Inst[2].Ptr, Inst[1].Count));
		return 3;
	case MLIT_DECL:
		return 2;
	case MLIT_INDEX_DECL:
		json_array_append(Json, json_integer(Inst[1].Index));
		return 3;
	case MLIT_COUNT_DECL:
		json_array_append(Json, json_integer(Inst[1].Count));
		return 3;
	case MLIT_COUNT_COUNT_DECL:
		json_array_append(Json, json_integer(Inst[1].Count));
		json_array_append(Json, json_integer(Inst[2].Count));
		return 4;
	case MLIT_CLOSURE: {
		ml_closure_info_t *Info = Inst[1].ClosureInfo;
		json_array_append(Json, ml_closure_info_encode(Info, Encoder));
		for (int N = 0; N < Info->NumUpValues; ++N) {
			json_array_append(Json, json_integer(Inst[2 + N].Index));
		}
		return 2 + Inst[1].ClosureInfo->NumUpValues;
	}
	default: return 0;
	}
}

static int ml_closure_info_param_fn(const char *Name, void *Index, json_t *Params) {
	json_array_set(Params, (intptr_t)Index - 1, json_string(Name));
	return 0;
}

static int ml_closure_find_labels(ml_inst_t *Inst, inthash_t *Labels, uintptr_t *Offset) {
	if (Inst->Label) inthash_insert(Labels, Inst->Label, (void *)*Offset);
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_NONE: *Offset += 2; return 1;
	case MLIT_INST: *Offset += 3; return 2;
	case MLIT_INST_TYPES: *Offset += 4; return 3;
	case MLIT_COUNT_COUNT: *Offset += 4; return 3;
	case MLIT_COUNT: *Offset += 3; return 2;
	case MLIT_INDEX: *Offset += 3; return 2;
	case MLIT_VALUE: *Offset += 3; return 2;
	case MLIT_VALUE_VALUE: *Offset += 4; return 3;
	case MLIT_INDEX_COUNT: *Offset += 4; return 3;
	case MLIT_INDEX_CHARS: *Offset += 4; return 3;
	case MLIT_COUNT_VALUE: *Offset += 4; return 3;
	case MLIT_COUNT_CHARS: *Offset += 3; return 3;
	case MLIT_DECL: *Offset += 2; return 2;
	case MLIT_INDEX_DECL: *Offset += 3; return 3;
	case MLIT_COUNT_DECL: *Offset += 3; return 3;
	case MLIT_COUNT_COUNT_DECL: *Offset += 4; return 4;
	case MLIT_CLOSURE:
		*Offset += 3 + Inst[1].ClosureInfo->NumUpValues;
		return 2 + Inst[1].ClosureInfo->NumUpValues;
	default: return 0;
	}
}

static json_t *ml_closure_info_encode(ml_closure_info_t *Info, ml_json_encoder_t *Encoder) {
	json_t *Json = json_array();
	json_array_append(Json, json_string("!"));
	json_array_append(Json, json_string(Info->Source || ""));
	json_array_append(Json, json_integer(Info->LineNo));
	json_array_append(Json, json_integer(Info->FrameSize));
	json_array_append(Json, json_integer(Info->NumParams));
	json_array_append(Json, json_integer(Info->NumUpValues));
	json_array_append(Json, json_integer(Info->ExtraArgs));
	json_array_append(Json, json_integer(Info->NamedArgs));
	json_t *Params = json_array();
	for (int I = 0; I < Info->Params->Size; ++I) json_array_append(Params, json_null());
	stringmap_foreach(Info->Params, Params, (void *)ml_closure_info_param_fn);
	json_array_append(Json, Params);
	json_t *Instructions = json_array();
	inthash_t Labels[1] = {INTHASH_INIT};
	uintptr_t Offset = 0, Return = 0;
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Opcode == MLI_LINK) {
			Inst = Inst[1].Inst;
		} else {
			if (Inst == Info->Return) Return = Offset;
			Inst += ml_closure_find_labels(Inst, Labels, &Offset);
		}
	}
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Opcode == MLI_LINK) {
			Inst = Inst[1].Inst;
		} else {
			Inst += ml_closure_inst_encode(Inst, Encoder, Instructions, Labels);
		}
	}
	json_array_append(Json, json_integer(0));
	json_array_append(Json, json_integer(Return));
	json_array_append(Json, Instructions);
	return Json;
}


static json_t *ML_TYPED_FN(ml_json_encode, MLClosureT, ml_json_encoder_t *Encoder, ml_closure_t *Value) {
	json_t *Json = json_array();
	json_array_append(Json, json_string("closure"));
	inthash_insert(Encoder->Cached, (uintptr_t)Value, Json);
	ml_closure_info_t *Info = Value->Info;
	json_array_append(Json, ml_closure_info_encode(Info, Encoder));
	for (int I = 0; I < Info->NumUpValues; ++I) {
		json_array_append(Json, ml_json_encode(Encoder, Value->UpValues[I]));
	}
	return Json;
}

typedef struct {
	ml_type_t *Type;
	json_t *Json;
} ml_json_value_t;

extern ml_type_t JSValueT[1];

ML_FUNCTION(JSValue) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_json_value_t *Value = new(ml_json_value_t);
	Value->Type = JSValueT;
	Value->Json = json_pack("[ss]", "^", ml_string_value(Args[0]));
	return (ml_value_t *)Value;
}

ML_TYPE(JSValueT, (), "js-value",
	.Constructor = (ml_value_t *)JSValue
);

static json_t *ML_TYPED_FN(ml_json_encode, JSValueT, ml_json_encoder_t *Encoder, ml_json_value_t *Value) {
	return Value->Json;
}

typedef struct {
	ml_type_t *Type;
	inthash_t Globals[1];
} ml_json_globals_t;

extern ml_type_t JSEncoderT[1];

ML_FUNCTION(JSEncoder) {
	ml_json_globals_t *Globals = new(ml_json_globals_t);
	Globals->Type = JSEncoderT;
	return (ml_value_t *)Globals;
}

ML_TYPE(JSEncoderT, (), "js-encoder",
	.Constructor = (ml_value_t *)JSEncoder
);

ML_METHOD("add", JSEncoderT, MLAnyT, MLStringT) {
	ml_json_globals_t *Globals = (ml_json_globals_t *)Args[0];
	inthash_insert(Globals->Globals,
		(uintptr_t)Args[1],
		json_pack("[ss]", "^", ml_string_value(Args[2]))
	);
	return Args[0];
}

ML_METHOD("encode", JSEncoderT, MLAnyT) {
	ML_CHECK_ARG_COUNT(1);
	ml_json_globals_t *Globals = (ml_json_globals_t *)Args[0];
	ml_json_encoder_t Encoder[1] = {0,};
	Encoder->Specials = Globals->Globals;
	json_t *Json = ml_json_encode(Encoder, Args[1]);
	const char *String = json_dumps(Json, JSON_ENCODE_ANY | JSON_COMPACT);
	return ml_cstring(String);
}

void ml_jsencode_init(stringmap_t *Globals) {
#include "ml_jsencode_init.c"
	if (Globals) {
		stringmap_insert(Globals, "jsvalue", JSValue);
		stringmap_insert(Globals, "jsencoder", JSEncoder);
	}
}

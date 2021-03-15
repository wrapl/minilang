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

static void ml_closure_clear_label(int Process, ml_inst_t *Inst) {
	if (Inst->Processed == Process) return;
	Inst->Processed = Process;
	Inst->Label = 0;
	if (MLInstTypes[Inst->Opcode] != MLIT_NONE) {
		ml_closure_clear_label(Process, Inst->Params[0].Inst);
	}
	if (MLInstTypes[Inst->Opcode] == MLIT_INST_INST) {
		ml_closure_clear_label(Process, Inst->Params[1].Inst);
	}
	if (MLInstTypes[Inst->Opcode] == MLIT_INST_INST_INDEX_CHARS) {
		ml_closure_clear_label(Process, Inst->Params[1].Inst);
	}
}

static json_t *ml_closure_info_encode(ml_closure_info_t *Info, ml_json_encoder_t *Encoder);

static int ml_closure_inst_encode(int Process, ml_inst_t *Inst, ml_json_encoder_t *Encoder, json_t *Json) {
	if (Inst->Processed == Process) return Inst->Label;
	Inst->Processed = Process;
	int Index = Inst->Label = json_array_size(Json);
	json_array_append(Json, json_integer(Inst->Opcode));
	if (MLInstTypes[Inst->Opcode] != MLIT_NONE) json_array_append(Json, json_null());
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_INST_INST: {
		json_array_append(Json, json_null());
		break;
	}
	case MLIT_INST_INST_INDEX_CHARS: {
		json_array_append(Json, json_null());
		json_array_append(Json, json_integer(Inst->Params[2].Index));
		json_array_append(Json, json_string(Inst->Params[3].Ptr));
		break;
	}
	case MLIT_INST_COUNT_COUNT:
		json_array_append(Json, json_integer(Inst->Params[1].Count));
		json_array_append(Json, json_integer(Inst->Params[2].Count));
		break;
	case MLIT_INST_COUNT:
		json_array_append(Json, json_integer(Inst->Params[1].Count));
		break;
	case MLIT_INST_INDEX:
		json_array_append(Json, json_integer(Inst->Params[1].Index));
		break;
	case MLIT_INST_VALUE:
		json_array_append(Json, ml_json_encode(Encoder, Inst->Params[1].Value));
		break;
	case MLIT_INST_VALUE_VALUE:
		json_array_append(Json, ml_json_encode(Encoder, Inst->Params[1].Value));
		json_array_append(Json, ml_json_encode(Encoder, Inst->Params[2].Value));
		break;
	case MLIT_INST_INDEX_COUNT:
		json_array_append(Json, json_integer(Inst->Params[1].Index));
		json_array_append(Json, json_integer(Inst->Params[2].Count));
		break;
	case MLIT_INST_INDEX_CHARS:
		json_array_append(Json, json_integer(Inst->Params[1].Index));
		json_array_append(Json, json_string(Inst->Params[2].Ptr));
		break;
	case MLIT_INST_COUNT_VALUE:
		json_array_append(Json, json_integer(Inst->Params[1].Count));
		json_array_append(Json, ml_json_encode(Encoder, Inst->Params[2].Value));
		break;
	case MLIT_INST_COUNT_CHARS:
		json_array_append(Json, json_integer(Inst->Params[1].Count));
		json_array_append(Json, json_string(Inst->Params[2].Ptr));
		break;
	case MLIT_INST_CLOSURE: {
		ml_closure_info_t *Info = Inst->Params[1].ClosureInfo;
		json_array_append(Json, ml_closure_info_encode(Info, Encoder));
		for (int N = 0; N < Info->NumUpValues; ++N) {
			json_array_append(Json, json_integer(Inst->Params[2 + N].Index));
		}
		break;
	}
	default:
		break;
	}
	if (MLInstTypes[Inst->Opcode] != MLIT_NONE) {
		int Label = ml_closure_inst_encode(Process, Inst->Params[0].Inst, Encoder, Json);
		json_array_set(Json, Index + 1, json_integer(Label));
	}
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_INST_INST:
	case MLIT_INST_INST_INDEX_CHARS: {
		int Label = ml_closure_inst_encode(Process, Inst->Params[1].Inst, Encoder, Json);
		json_array_set(Json, Index + 2, json_integer(Label));
		break;
	}
	default: break;
	}
	return Inst->Label;
}

static int ml_closure_info_param_fn(const char *Name, void *Index, json_t *Params) {
	json_object_set(Params, Name, json_integer((intptr_t)Index - 1));
	return 0;
}

static json_t *ml_closure_info_encode(ml_closure_info_t *Info, ml_json_encoder_t *Encoder) {
	json_t *Json = json_array();
	json_array_append(Json, json_string(Info->Source));
	json_array_append(Json, json_integer(Info->LineNo));
	json_array_append(Json, json_integer(Info->FrameSize));
	json_array_append(Json, json_integer(Info->NumParams));
	json_array_append(Json, json_integer(Info->NumUpValues));
	json_array_append(Json, json_integer(Info->ExtraArgs));
	json_array_append(Json, json_integer(Info->NamedArgs));
	json_t *Params = json_object();
	stringmap_foreach(Info->Params, Params, (void *)ml_closure_info_param_fn);
	json_array_append(Json, Params);
	json_t *Instructions = json_array();
	ml_closure_clear_label(!Info->Entry->Processed, Info->Entry);
	ml_closure_inst_encode(!Info->Entry->Processed, Info->Entry, Encoder, Instructions);
	json_array_append(Json, json_integer(Info->Entry->Label));
	json_array_append(Json, json_integer(Info->Return->Label));
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

ML_FUNCTION(JSEncode) {
	ML_CHECK_ARG_COUNT(1);
	ml_json_encoder_t *Encoder = ml_json_encoder(inthash_new());
	json_t *Json = ml_json_encode(Encoder, Args[0]);
	const char *String = json_dumps(Json, JSON_ENCODE_ANY | JSON_COMPACT);
	return ml_cstring(String);
}

void ml_jsencode_init(stringmap_t *Globals) {
#include "ml_jsencode_init.c"
	if (Globals) {
		stringmap_insert(Globals, "jsencode", JSEncode);
	}
}


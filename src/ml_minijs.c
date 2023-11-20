#include "ml_macros.h"
#include "ml_bytecode.h"
#include <string.h>
#include "ml_minijs.h"

#ifdef ML_UUID
#include "ml_uuid.h"
#endif

#ifdef ML_TIME
#include "ml_time.h"
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "minijs"

// Overview
// Provides a specialized encoding of Minilang values to and from JSON with support for complex or cyclic data structures.
//
// * :json:`null` |harr| :mini:`nil`
// * :json:`true` |harr| :mini:`true`
// * :json:`false` |harr| :mini:`false`
// * *integer* |harr| :mini:`integer`
// * *real* |harr| :mini:`real`
// * *string* |harr| :mini:`string`
// * ``[type, ...]`` |harr| *other*

ml_value_t *ml_minijs_encode(ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	//if (Value == MLNil) return Value;
	//if (Value == (ml_value_t *)MLTrue) return Value;
	//if (Value == (ml_value_t *)MLFalse) return Value;
	ml_value_t *Json = inthash_search(Encoder->Cached, (uintptr_t)Value);
	if (Json) {
		ml_value_t *First = ml_list_get(Json, 1);
		if (!ml_is(First, MLIntegerT)) {
			First = ml_integer(Encoder->LastIndex++);
			ml_list_push(Json, First);
		}
		Json = ml_list();
		ml_list_put(Json, First);
		return Json;
	}
	const char *Name = ml_externals_get_name(Encoder->Externals, Value);
	if (Name) {
		Json = ml_list();
		ml_list_put(Json, ml_cstring("^"));
		ml_list_put(Json, ml_string(Name, -1));
		return Json;
	}
	typeof(ml_minijs_encode) *encode = ml_typed_fn_get(ml_typeof(Value), ml_minijs_encode);
	if (encode) return encode(Encoder, Value);
	ml_value_t *Serialized = ml_serialize(Value);
	if (ml_is_error(Serialized)) {
		Json = ml_list();
		ml_list_put(Json, ml_cstring("unsupported"));
		ml_list_put(Json, ml_string(ml_typeof(Value)->Name, -1));
		return Json;
	}
	Json = ml_list();
	ml_list_put(Json, ml_cstring("o"));
	ML_LIST_FOREACH(Serialized, Iter) ml_list_put(Json, ml_minijs_encode(Encoder, Iter->Value));
	return Json;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLNilT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	return Value;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLBlankT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("_"));
	return Json;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLSomeT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("^"));
	ml_list_put(Json, ml_cstring("some"));
	return Json;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLBooleanT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	return Value;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLIntegerT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	return Value;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLDoubleT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	return Value;
}

#ifdef ML_COMPLEX
#include <complex.h>
#undef I

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLComplexT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("c"));
	complex_double Complex = ml_complex_value(Value);
	ml_list_put(Json, ml_real(creal(Complex)));
	ml_list_put(Json, ml_real(cimag(Complex)));
	return Json;
}

#endif

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLStringT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	return Value;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLRegexT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("r"));
	ml_list_put(Json, ml_string(ml_regex_pattern(Value), -1));
	return Json;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLMethodT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring(":"));
	ml_list_put(Json, ml_string(ml_method_name(Value), -1));
	return Json;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLTupleT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("()"));
	inthash_insert(Encoder->Cached, (uintptr_t)Value, Json);
	int Size = ml_tuple_size(Value);
	for (int I = 1; I <= Size; ++I) {
		ml_list_put(Json, ml_minijs_encode(Encoder, ml_tuple_get(Value, I)));
	}
	return Json;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLListT, ml_minijs_encoder_t *Encoder, ml_list_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("l"));
	inthash_insert(Encoder->Cached, (uintptr_t)Value, Json);
	ML_LIST_FOREACH(Value, Iter) {
		ml_list_put(Json, ml_minijs_encode(Encoder, Iter->Value));
	}
	return Json;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLNamesT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("n"));
	inthash_insert(Encoder->Cached, (uintptr_t)Value, Json);
	ML_NAMES_FOREACH(Value, Iter) {
		ml_list_put(Json, ml_minijs_encode(Encoder, Iter->Value));
	}
	return Json;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLMapT, ml_minijs_encoder_t *Encoder, ml_map_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("m"));
	inthash_insert(Encoder->Cached, (uintptr_t)Value, Json);
	ML_MAP_FOREACH(Value, Iter) {
		ml_list_put(Json, ml_minijs_encode(Encoder, Iter->Key));
		ml_list_put(Json, ml_minijs_encode(Encoder, Iter->Value));
	}
	return Json;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLTypeT, ml_minijs_encoder_t *Encoder, ml_type_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("t"));
	ml_list_put(Json, ml_string(Value->Name, -1));
	return Json;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLGlobalT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	return ml_minijs_encode(Encoder, ml_global_get(Value));
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLVariableT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("v"));
	inthash_insert(Encoder->Cached, (uintptr_t)Value, Json);
	ml_list_put(Json, ml_minijs_encode(Encoder, ml_deref(Value)));
	return Json;
}

#ifdef ML_UUID

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLUUIDT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("uuid"));
	char *IdString = snew(UUID_STR_LEN);
	uuid_unparse_lower(ml_uuid_value(Value), IdString);
	ml_list_put(Json, ml_string(IdString, UUID_STR_LEN - 1));
	return Json;
}

#endif

#ifdef ML_TIME

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLTimeT, ml_minijs_encoder_t *Encoder, ml_value_t *Value) {
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
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("time"));
	ml_list_put(Json, ml_string_copy(Buffer, End - Buffer));
	return Json;
}

#endif

#ifdef ML_MATH

#include "ml_array.h"

#define ML_JSON_ENCODE_ARRAY(CTYPE, JSON) \
\
static void ml_minijs_encode_array_ ## CTYPE(int Degree, ml_array_dimension_t *Dimension, char *Address, ml_value_t *Json) { \
	if (Degree == 0) { \
		ml_list_put(Json, JSON(*(CTYPE *)Address)); \
		return; \
	} else { \
		int Stride = Dimension->Stride; \
		if (Dimension->Indices) { \
			int *Indices = Dimension->Indices; \
			for (int I = 0; I < Dimension->Size; ++I) { \
				ml_minijs_encode_array_ ## CTYPE(Degree - 1, Dimension + 1, Address + Indices[I] * Stride, Json); \
			} \
		} else { \
			for (int I = Dimension->Size; --I >= 0;) { \
				ml_minijs_encode_array_ ## CTYPE(Degree - 1, Dimension + 1, Address, Json); \
				Address += Stride; \
			} \
		} \
	} \
}

ML_JSON_ENCODE_ARRAY(int8_t, ml_integer)
ML_JSON_ENCODE_ARRAY(uint8_t, ml_integer)
ML_JSON_ENCODE_ARRAY(int16_t, ml_integer)
ML_JSON_ENCODE_ARRAY(uint16_t, ml_integer)
ML_JSON_ENCODE_ARRAY(int32_t, ml_integer)
ML_JSON_ENCODE_ARRAY(uint32_t, ml_integer)
ML_JSON_ENCODE_ARRAY(int64_t, ml_integer)
ML_JSON_ENCODE_ARRAY(uint64_t, ml_integer)
ML_JSON_ENCODE_ARRAY(float, ml_real)
ML_JSON_ENCODE_ARRAY(double, ml_real)

static void ml_minijs_encode_array_any(int Degree, ml_array_dimension_t *Dimension, char *Address, ml_value_t *Json, ml_minijs_encoder_t *Encoder) {
	if (Degree == 0) {
		ml_list_put(Json, ml_minijs_encode(Encoder, *(ml_value_t **)Address));
	} else {
		int Stride = Dimension->Stride;
		if (Dimension->Indices) {
			int *Indices = Dimension->Indices;
			for (int I = 0; I < Dimension->Size; ++I) {
				ml_minijs_encode_array_any(Degree - 1, Dimension + 1, Address + Indices[I] * Stride, Json, Encoder);
			}
		} else {
			for (int I = Dimension->Size; --I >= 0;) {
				ml_minijs_encode_array_any(Degree - 1, Dimension + 1, Address, Json, Encoder);
				Address += Stride;
			}
		}
	}
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLArrayT, ml_minijs_encoder_t *Encoder, ml_array_t *Array) {
	ml_value_t *Type;
	ml_value_t *Values = ml_list();
	ml_value_t *Shape = ml_list();
	for (int I = 0; I < Array->Degree; ++I) ml_list_put(Shape, ml_integer(Array->Dimensions[I].Size));
	switch (Array->Format) {
	case ML_ARRAY_FORMAT_U8:
		Type = ml_cstring("uint8");
		ml_minijs_encode_array_uint8_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_I8:
		Type = ml_cstring("int8");
		ml_minijs_encode_array_int8_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_U16:
		Type = ml_cstring("uint16");
		ml_minijs_encode_array_uint16_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_I16:
		Type = ml_cstring("int16");
		ml_minijs_encode_array_int16_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_U32:
		Type = ml_cstring("uint32");
		ml_minijs_encode_array_uint32_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_I32:
		Type = ml_cstring("int32");
		ml_minijs_encode_array_int32_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_U64:
		Type = ml_cstring("uint64");
		ml_minijs_encode_array_uint64_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_I64:
		Type = ml_cstring("int64");
		ml_minijs_encode_array_int64_t(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_F32:
		Type = ml_cstring("float32");
		ml_minijs_encode_array_float(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_F64:
		Type = ml_cstring("float64");
		ml_minijs_encode_array_double(Array->Degree, Array->Dimensions, Array->Base.Value, Values);
		break;
	case ML_ARRAY_FORMAT_ANY:
		Type = ml_cstring("any");
		ml_minijs_encode_array_any(Array->Degree, Array->Dimensions, Array->Base.Value, Values, Encoder);
		break;
	default: {
		ml_value_t *Json = ml_list();
		ml_list_put(Json, ml_cstring("unsupported"));
		ml_list_put(Json, ml_string(Array->Base.Type->Name, -1));
		return Json;
	}
	}
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("array"));
	ml_list_put(Json, Type);
	ml_list_put(Json, Shape);
	ml_list_put(Json, Values);
	return Json;
}

#endif

static ml_value_t *ml_closure_info_encode(ml_closure_info_t *Info, ml_minijs_encoder_t *Encoder);

typedef struct {
	ml_value_t *Json;
	inthash_t Cache[1];
} ml_decls_json_t;

static ml_value_t *ml_closure_decl_encode(ml_decl_t *Decl, ml_decls_json_t *Decls) {
	if (!Decl) return ml_integer(-1);
	ml_value_t *Index = (ml_value_t *)inthash_search(Decls->Cache, (uintptr_t)Decl);
	if (Index) return Index;
	ml_value_t *Next = ml_closure_decl_encode(Decl->Next, Decls);
	Index = ml_integer(ml_list_length(Decls->Json));
	ml_value_t *Json = ml_list();
	ml_list_put(Json, Next);
	ml_list_put(Json, ml_string(Decl->Ident, -1));
	ml_list_put(Json, ml_integer(Decl->Source.Line));
	ml_list_put(Json, ml_integer(Decl->Index));
	ml_list_put(Json, ml_integer(Decl->Flags));
	ml_list_put(Decls->Json, Json);
	inthash_insert(Decls->Cache, (uintptr_t)Decl, Index);
	return Index;
}

static int ml_closure_info_param_fn(const char *Name, void *Index, ml_value_t *Params) {
	ml_list_set(Params, (intptr_t)Index - 1, ml_string(Name, -1));
	return 0;
}

static int ml_closure_find_labels(ml_inst_t *Inst, uintptr_t *Offset) {
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_NONE: *Offset += 2; return 1;
	case MLIT_INST: *Offset += 3; return 2;
	case MLIT_INST_COUNT: *Offset += 4; return 3;
	case MLIT_INST_COUNT_DECL: *Offset += 5; return 4;
	case MLIT_COUNT_COUNT: *Offset += 4; return 3;
	case MLIT_COUNT: *Offset += 3; return 2;
	case MLIT_VALUE: *Offset += 3; return 2;
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

static int ml_closure_inst_encode(ml_inst_t *Inst, ml_minijs_encoder_t *Encoder, ml_value_t *Json, inthash_t *Labels, ml_decls_json_t *Decls) {
	ml_list_put(Json, ml_integer(Inst->Opcode));
	ml_list_put(Json, ml_integer(Inst->Line));
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_NONE:
		return 1;
	case MLIT_INST:
		ml_list_put(Json, ml_integer((uintptr_t)inthash_search(Labels, Inst[1].Inst->Label)));
		return 2;
	case MLIT_INST_COUNT:
		ml_list_put(Json, ml_integer((uintptr_t)inthash_search(Labels, Inst[1].Inst->Label)));
		ml_list_put(Json, ml_integer(Inst[2].Count));
		return 3;
	case MLIT_INST_COUNT_DECL:
		ml_list_put(Json, ml_integer((uintptr_t)inthash_search(Labels, Inst[1].Inst->Label)));
		ml_list_put(Json, ml_integer(Inst[2].Count));
		ml_list_put(Json, ml_closure_decl_encode(Inst[3].Decls, Decls));
		return 4;
	case MLIT_COUNT_COUNT:
		ml_list_put(Json, ml_integer(Inst[1].Count));
		ml_list_put(Json, ml_integer(Inst[2].Count));
		return 3;
	case MLIT_COUNT:
		ml_list_put(Json, ml_integer(Inst[1].Count));
		return 2;
	case MLIT_VALUE:
		ml_list_put(Json, ml_minijs_encode(Encoder, Inst[1].Value));
		return 2;
	case MLIT_VALUE_COUNT:
		ml_list_put(Json, ml_minijs_encode(Encoder, Inst[1].Value));
		ml_list_put(Json, ml_integer(Inst[2].Count));
		return 3;
	case MLIT_VALUE_COUNT_DATA:
		ml_list_put(Json, ml_minijs_encode(Encoder, Inst[1].Value));
		ml_list_put(Json, ml_integer(Inst[2].Count));
		return 4;
	case MLIT_COUNT_CHARS:
		ml_list_put(Json, ml_string(Inst[2].Chars, Inst[1].Count));
		return 3;
	case MLIT_DECL:
		ml_list_put(Json, ml_closure_decl_encode(Inst[1].Decls, Decls));
		return 2;
	case MLIT_COUNT_DECL:
		ml_list_put(Json, ml_integer(Inst[1].Count));
		ml_list_put(Json, ml_closure_decl_encode(Inst[2].Decls, Decls));
		return 3;
	case MLIT_COUNT_COUNT_DECL:
		ml_list_put(Json, ml_integer(Inst[1].Count));
		ml_list_put(Json, ml_integer(Inst[2].Count));
		ml_list_put(Json, ml_closure_decl_encode(Inst[3].Decls, Decls));
		return 4;
	case MLIT_CLOSURE: {
		ml_closure_info_t *Info = Inst[1].ClosureInfo;
		ml_list_put(Json, ml_closure_info_encode(Info, Encoder));
		for (int N = 0; N < Info->NumUpValues; ++N) {
			ml_list_put(Json, ml_integer(Inst[2 + N].Count));
		}
		return 2 + Info->NumUpValues;
	}
	case MLIT_SWITCH: {
		ml_value_t *Insts = ml_list();
		ml_list_put(Insts, ml_cstring("l"));
		for (int N = 0; N < Inst[1].Count; ++N) {
			ml_list_put(Insts, ml_integer((uintptr_t)inthash_search(Labels, Inst[2].Insts[N]->Label)));
		}
		ml_list_put(Json, Insts);
		return 3;
	}
	default: __builtin_unreachable();
	}
}

static ml_value_t *ml_closure_info_encode(ml_closure_info_t *Info, ml_minijs_encoder_t *Encoder) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("!"));
	ml_list_put(Json, ml_integer(ML_BYTECODE_VERSION));
	ml_list_put(Json, Info->Source ? ml_string(Info->Source, -1) : ml_cstring(""));
	ml_list_put(Json, ml_integer(Info->StartLine));
	ml_list_put(Json, ml_integer(Info->FrameSize));
	ml_list_put(Json, ml_integer(Info->NumParams));
	ml_list_put(Json, ml_integer(Info->NumUpValues));
	ml_list_put(Json, ml_integer(!!(Info->Flags & ML_CLOSURE_EXTRA_ARGS)));
	ml_list_put(Json, ml_integer(!!(Info->Flags & ML_CLOSURE_NAMED_ARGS)));
	ml_value_t *Params = ml_list();
	for (int I = 0; I < Info->Params->Size; ++I) ml_list_put(Params, MLNil);
	stringmap_foreach(Info->Params, Params, (void *)ml_closure_info_param_fn);
	ml_list_put(Json, Params);
	ml_value_t *Instructions = ml_list();
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
	ml_decls_json_t Decls[1] = {{ml_list(), {INTHASH_INIT}}};
	ml_value_t *InitDecls = ml_closure_decl_encode(Info->Decls, Decls);
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Opcode == MLI_LINK) {
			Inst = Inst[1].Inst;
		} else {
			Inst += ml_closure_inst_encode(Inst, Encoder, Instructions, Labels, Decls);
		}
	}
	ml_list_put(Json, ml_integer(0));
	ml_list_put(Json, ml_integer(Return));
	ml_list_put(Json, InitDecls);
	ml_list_put(Json, Instructions);
	ml_list_put(Json, Decls->Json);
	return Json;
}


static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLClosureT, ml_minijs_encoder_t *Encoder, ml_closure_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("z"));
	inthash_insert(Encoder->Cached, (uintptr_t)Value, Json);
	ml_closure_info_t *Info = Value->Info;
	ml_list_put(Json, ml_closure_info_encode(Info, Encoder));
	for (int I = 0; I < Info->NumUpValues; ++I) {
		ml_list_put(Json, ml_minijs_encode(Encoder, Value->UpValues[I]));
	}
	return Json;
}

static ml_value_t *ML_TYPED_FN(ml_minijs_encode, MLExternalT, ml_minijs_encoder_t *Encoder, ml_external_t *Value) {
	ml_value_t *Json = ml_list();
	ml_list_put(Json, ml_cstring("^"));
	ml_list_put(Json, ml_string(Value->Name, -1));
	ml_list_put(Json, ml_string(Value->Source, -1));
	ml_list_put(Json, ml_integer(Value->Line));
	return Json;
}

ML_METHOD_ANON(MinijsEncode, "minijs::encode");

ML_METHOD(MinijsEncode, MLAnyT) {
//@minijs::encode
//<Value
//>any
	ML_CHECK_ARG_COUNT(1);
	ml_minijs_encoder_t Encoder[1] = {MLExternals, {INTHASH_INIT}, 0};
	return ml_minijs_encode(Encoder, Args[0]);
}

ML_METHOD(MinijsEncode, MLAnyT, MLExternalSetT) {
//@minijs::encode
//<Value
//<Externals
//>any
	ML_CHECK_ARG_COUNT(1);
	ml_minijs_encoder_t Encoder[1] = {(ml_externals_t *)Args[1], {INTHASH_INIT}, 0};
	return ml_minijs_encode(Encoder, Args[0]);
}

typedef struct {
	ml_externals_t *Externals;
	inthash_t Cached[1];
} ml_minijs_decoder_t;

static stringmap_t Decoders[1] = {STRINGMAP_INIT};

typedef ml_value_t *(*ml_minijs_decode_fn)(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index);

ml_value_t *ml_minijs_decode(ml_minijs_decoder_t *Decoder, ml_value_t *Json) {
	if (Json == MLNil) return Json;
	if (ml_is(Json, MLBooleanT)) return Json;
	if (ml_is(Json, MLNumberT)) return Json;
	if (ml_is(Json, MLStringT)) return Json;
	if (!ml_is(Json, MLListT)) return ml_error("MinijsError", "Unsupported JSON value");
	ml_list_node_t *Node = ((ml_list_t *)Json)->Head;
	if (!Node) return ml_error("MinijsError", "Unsupported JSON value");
	int Count = ((ml_list_t *)Json)->Length - 1;
	ml_value_t *First = Node->Value;
	Node = Node->Next;
	intptr_t Index = -1;
	if (ml_is(First, MLIntegerT)) {
		Index = ml_integer_value(First);
		if (Node) {
			First = Node->Value;
			Node = Node->Next;
			--Count;
		} else {
			ml_value_t *Value = inthash_search(Decoder->Cached, Index);
			if (Value) return Value;
			char *Name;
			GC_asprintf(&Name, "@%d", Index);
			Value = ml_uninitialized(Name, (ml_source_t){"minijs", 0});
			inthash_insert(Decoder->Cached, Index, Value);
			return Value;
		}
	}
	if (!ml_is(First, MLStringT)) return ml_error("MinijsError", "Unsupported JSON value");
	const char *Name = ml_string_value(First);
	ml_minijs_decode_fn decode = (ml_minijs_decode_fn)stringmap_search(Decoders, Name);
	if (!decode) return ml_error("MinijsError", "Unsupported JSON decoder: %s", Name);
	return decode(Decoder, Node, Index);
}

static ml_value_t *ml_minijs_decode_object(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	if (!Node || !ml_is(Node->Value, MLStringT)) return ml_error("MinijsError", "Invalid object");
	const char *Name = ml_string_value(Node->Value);
	int Count = 0;
	for (ml_list_node_t *Tail = Node->Next; Tail; Tail = Tail->Next) ++Count;
	ml_value_t *Args[Count];
	for (int I = 0; I < Count; ++I) {
		Node = Node->Next;
		ml_value_t *Arg = ml_minijs_decode(Decoder, Node->Value);
		if (ml_is_error(Arg)) return Arg;
		Args[I] = Arg;
	}
	ml_value_t *Value = ml_deserialize(Name, Count, Args);
	if (Index >= 0) {
		ml_value_t *Uninitialized = inthash_insert(Decoder->Cached, Index, Value);
		ml_uninitialized_set(Uninitialized, Value);
	}
	return Value;
}

static ml_value_t *ml_minijs_decode_global(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	if (!Node) return ml_error("TypeError", "Global requires string name");
	ml_value_t *Value = Node->Value;
	if (!ml_is(Value, MLStringT)) return ml_error("TypeError", "Global requires string name");
	const char *Name = ml_string_value(Value);
	return ml_externals_get_value(Decoder->Externals, Name) ?: ml_error("NameError", "Unknown global %s", Name);
}

static ml_value_t *ml_minijs_decode_blank(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	return MLBlank;
}

static ml_value_t *ml_minijs_decode_some(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	return MLSome;
}

static ml_value_t *ml_minijs_decode_tuple(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	int Size = 0;
	for (ml_list_node_t *Tail = Node; Tail; Tail = Tail->Next) ++Size;
	ml_value_t *Tuple = ml_tuple(Size);
	if (Index >= 0) inthash_insert(Decoder->Cached, Index, Tuple);
	for (int I = 1; Node; Node = Node->Next, ++I) {
		ml_value_t *Value = ml_minijs_decode(Decoder, Node->Value);
		if (ml_is_error(Value)) return Value;
		ml_tuple_set(Tuple, Index, Value);
	}
	return Tuple;
}

static ml_value_t *ml_minijs_decode_list(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	ml_value_t *List = ml_list();
	if (Index >= 0) inthash_insert(Decoder->Cached, Index, List);
	while (Node) {
		ml_value_t *Value = ml_minijs_decode(Decoder, Node->Value);
		if (ml_is_error(Value)) return Value;
		ml_list_put(List, Value);
		Node = Node->Next;
	}
	return List;
}

static ml_value_t *ml_minijs_decode_names(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	ml_value_t *Names = ml_names();
	if (Index >= 0) inthash_insert(Decoder->Cached, Index, Names);
	while (Node) {
		ml_value_t *Value = ml_minijs_decode(Decoder, Node->Value);
		if (!ml_is(Value, MLStringT)) return ml_error("TypeError", "Names requires strings");
		ml_names_add(Names, Value);
		Node = Node->Next;
	}
	return Names;
}

static ml_value_t *ml_minijs_decode_map(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	ml_value_t *Map = ml_map();
	if (Index >= 0) inthash_insert(Decoder->Cached, Index, Map);
	while (Node) {
		ml_value_t *Key = ml_minijs_decode(Decoder, Node->Value);
		if (ml_is_error(Key)) return Key;
		Node = Node->Next;
		if (!Node) return ml_error("MinijsError", "Map requires matched keys and values");
		ml_value_t *Value = ml_minijs_decode(Decoder, Node->Value);
		if (ml_is_error(Value)) return Value;
		ml_map_insert(Map, Key, Value);
		Node = Node->Next;
	}
	return Map;
}

#ifdef ML_COMPLEX

static ml_value_t *ml_minijs_decode_complex(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	if (!Node) return ml_error("TypeError", "Complex requires real + imaginary parts");
	if (!ml_is(Node->Value, MLRealT)) return ml_error("TypeError", "Complex requires reals");
	double Real = ml_real_value(Node->Value);
	Node = Node->Next;
	if (!Node) return ml_error("TypeError", "Complex requires real + imaginary parts");
	if (!ml_is(Node->Value, MLRealT)) return ml_error("TypeError", "Complex requires reals");
	double Imag = ml_real_value(Node->Value);
	return ml_complex(Real + Imag * _Complex_I);
}

#endif

static ml_value_t *ml_minijs_decode_regex(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	if (!Node) return ml_error("TypeError", "Regex requires string name");
	ml_value_t *Value = Node->Value;
	if (!ml_is(Value, MLStringT)) return ml_error("TypeError", "Regex requires strings");
	return ml_regex(ml_string_value(Value), ml_string_length(Value));
}

static ml_value_t *ml_minijs_decode_method(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	if (!Node) return ml_error("TypeError", "Method requires string name");
	ml_value_t *Value = Node->Value;
	if (!ml_is(Value, MLStringT)) return ml_error("TypeError", "Method requires strings");
	return ml_method(ml_string_value(Value));
}

static ml_value_t *ml_minijs_decode_type(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	if (!Node) return ml_error("TypeError", "Type requires string name");
	ml_value_t *Value = Node->Value;
	if (!ml_is(Value, MLStringT)) return ml_error("TypeError", "Type requires strings");
	return MLNil;
}

#ifdef ML_UUID

static ml_value_t *ml_minijs_decode_uuid(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	if (!Node) return ml_error("TypeError", "Global requires string name");
	ml_value_t *Value = Node->Value;
	if (!ml_is(Value, MLStringT)) return ml_error("TypeError", "UUID requires strings");
	return ml_uuid_parse(ml_string_value(Value), ml_string_length(Value));
}

#endif

#ifdef ML_TIME

static ml_value_t *ml_minijs_decode_time(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	if (!Node) return ml_error("TypeError", "Global requires string name");
	ml_value_t *Value = Node->Value;
	if (!ml_is(Value, MLStringT)) return ml_error("TypeError", "Time requires strings");
	return ml_time_parse(ml_string_value(Value), ml_string_length(Value));
}

#endif

#ifdef ML_MATH

static ml_value_t *ml_minijs_decode_array(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	if (!Node) return ml_error("TypeError", "Global requires string name");
	ml_value_t *Value = Node->Value;
	if (!ml_is(Value, MLStringT)) return ml_error("TypeError", "Array requires strings");
	return ml_error("ImplementationError", "Arrays not supported yet");
}

#endif

static ml_value_t *ml_minijs_decode_variable(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	ml_value_t *Variable = ml_variable(MLNil, NULL);
	if (Index >= 0) inthash_insert(Decoder->Cached, Index, Variable);
	if (Node) ml_variable_set(Variable, ml_minijs_decode(Decoder, Node->Value));
	return Variable;
}

#define CLOSURE_NEXT(TYPE, FN) ({ \
	if (!Node) return NULL; \
	if (!ml_is(Node->Value, TYPE)) return NULL; \
	ml_value_t *Value = Node->Value; \
	Node = Node->Next; \
	FN(Value); \
})

#define CLOSURE_NEXT_STRING() CLOSURE_NEXT(MLStringT, ml_string_value)
#define CLOSURE_NEXT_INT() CLOSURE_NEXT(MLIntegerT, ml_integer_value)
#define CLOSURE_NEXT_LIST() CLOSURE_NEXT(MLListT, )

static ml_closure_info_t *ml_minijs_decode_closure_info(ml_minijs_decoder_t *Decoder, ml_value_t *Value) {
	if (!ml_is(Value, MLListT)) return NULL;
	ml_list_node_t *Node = ((ml_list_t *)Value)->Head;
	ml_closure_info_t *Info = new(ml_closure_info_t);
	Info->Name = "<js-closure>";
	CLOSURE_NEXT_STRING();
	int Version = CLOSURE_NEXT_INT();
	Info->Source = CLOSURE_NEXT_STRING();
	Info->StartLine = CLOSURE_NEXT_INT();
	Info->FrameSize = CLOSURE_NEXT_INT();
	Info->NumParams = CLOSURE_NEXT_INT();
	Info->NumUpValues = CLOSURE_NEXT_INT();
	int ExtraArgs = CLOSURE_NEXT_INT();
	int NamedArgs = CLOSURE_NEXT_INT();
	ml_value_t *Params = CLOSURE_NEXT_LIST();
	int Entry = CLOSURE_NEXT_INT();
	int Return = CLOSURE_NEXT_INT();
	int InitDecl = CLOSURE_NEXT_INT();
	ml_value_t *Instructions = CLOSURE_NEXT_LIST();
	ml_value_t *DeclsJson = CLOSURE_NEXT_LIST();
	if (Version != ML_BYTECODE_VERSION) return NULL;
	if (ExtraArgs) Info->Flags |= ML_CLOSURE_EXTRA_ARGS;
	if (NamedArgs) Info->Flags |= ML_CLOSURE_NAMED_ARGS;
	int Index = 1;
	ML_LIST_FOREACH(Params, Iter) {
		stringmap_insert(Info->Params, ml_string_value(Iter->Value), (void *)(uintptr_t)(Index));
		++Index;
	}
	int Length = ml_list_length(Instructions);
	int *Offsets = anew(int, Length);
	int Offset = 0;
	Index = 0;
	while (Index < Length) {
		Offsets[Index] = Offset;
		ml_opcode_t Opcode = ml_integer_value(ml_list_get(Instructions, Index + 1));
		switch (MLInstTypes[Opcode]) {
		case MLIT_NONE:
			Index += 2; Offset += 1; break;
		case MLIT_INST:
		case MLIT_COUNT:
		case MLIT_VALUE:
			Index += 3; Offset += 2; break;
		case MLIT_INST_COUNT:
		case MLIT_COUNT_COUNT:
		case MLIT_VALUE_COUNT:
			Index += 4; Offset += 3; break;
		case MLIT_VALUE_COUNT_DATA:
			Index += 4; Offset += 4; break;
		case MLIT_COUNT_CHARS:
			Index += 3; Offset += 3; break;
		case MLIT_DECL:
			Index += 3; Offset += 2; break;
		case MLIT_COUNT_DECL:
			Index += 4; Offset += 3; break;
		case MLIT_INST_COUNT_DECL:
		case MLIT_COUNT_COUNT_DECL:
			Index += 5; Offset += 4; break;
		case MLIT_CLOSURE: {
			ml_value_t *UpValues = ml_list_get(Instructions, Index + 3);
			if (!ml_is(UpValues, MLListT)) return NULL;
			int NumUpValues = ml_integer_value(ml_list_get(UpValues, 7));
			Index += 3 + NumUpValues; Offset += 2 + NumUpValues;
			break;
		}
		case MLIT_SWITCH:
			Index += 3; Offset += 3;
			break;
		}
	}
	ml_decl_t **Decls = anew(ml_decl_t *, ml_list_length(DeclsJson));
	Index = 0;
	ML_LIST_FOREACH(DeclsJson, Iter) {
		ml_decl_t *Decl = Decls[Index] = new(ml_decl_t);
		Decl->Source.Name = Info->Source;
		if (!ml_is(Iter->Value, MLListT)) return NULL;
		ml_list_node_t *Node = ((ml_list_t *)Iter->Value)->Head;
		int Next = CLOSURE_NEXT_INT();
		Decl->Ident = CLOSURE_NEXT_STRING();
		Decl->Source.Line = CLOSURE_NEXT_INT();
		Decl->Index = CLOSURE_NEXT_INT();
		Decl->Flags = CLOSURE_NEXT_INT();
		if (Next >= 0) Decl->Next = Decls[Next];
		++Index;
	}
	if (InitDecl >= 0) Info->Decls = Decls[InitDecl];
	ml_inst_t *Code = anew(ml_inst_t, Offset);
	Info->Halt = Code + Offset;
	ml_inst_t *Inst = Code;
	Index = 0;
	ml_list_iter_t Iter[1];
	ml_list_iter_forward(Instructions, Iter);
	while (ml_list_iter_valid(Iter)) {
		Inst->Opcode = ml_integer_value(Iter->Value);
		ml_list_iter_next(Iter);
		int Line = Inst->Line = ml_integer_value(Iter->Value);
		ml_list_iter_next(Iter);
		if (Line > Info->EndLine) Info->EndLine = Line;
		switch (MLInstTypes[Inst->Opcode]) {
		case MLIT_NONE:
			Inst += 1; break;
		case MLIT_INST:
			Inst[1].Inst = Code + Offsets[ml_integer_value(Iter->Value)];
			ml_list_iter_next(Iter);
			Inst += 2; break;
		case MLIT_INST_COUNT:
			Inst[1].Inst = Code + Offsets[ml_integer_value(Iter->Value)];
			ml_list_iter_next(Iter);
			Inst[2].Count = ml_integer_value(Iter->Value);
			ml_list_iter_next(Iter);
			Inst += 3; break;
		case MLIT_INST_COUNT_DECL:
			Inst[1].Inst = Code + Offsets[ml_integer_value(Iter->Value)];
			ml_list_iter_next(Iter);
			Inst[2].Count = ml_integer_value(Iter->Value);
			ml_list_iter_next(Iter);
			Inst[3].Decls = Decls[ml_integer_value(Iter->Value)];
			ml_list_iter_next(Iter);
			Inst += 4; break;
		case MLIT_COUNT:
			Inst[1].Count = ml_integer_value(Iter->Value);
			ml_list_iter_next(Iter);
			Inst += 2; break;
		case MLIT_VALUE:
			Inst[1].Value = ml_minijs_decode(Decoder, Iter->Value);
			ml_list_iter_next(Iter);
			Inst += 2; break;
		case MLIT_COUNT_COUNT:
			Inst[1].Count = ml_integer_value(Iter->Value);
			ml_list_iter_next(Iter);
			Inst[2].Count = ml_integer_value(Iter->Value);
			ml_list_iter_next(Iter);
			Inst += 3; break;
		case MLIT_VALUE_COUNT:
			Inst[1].Value = ml_minijs_decode(Decoder, Iter->Value);
			ml_list_iter_next(Iter);
			Inst[2].Count = ml_integer_value(Iter->Value);
			ml_list_iter_next(Iter);
			Inst += 3; break;
		case MLIT_VALUE_COUNT_DATA:
			Inst[1].Value = ml_minijs_decode(Decoder, Iter->Value);
			ml_list_iter_next(Iter);
			Inst[2].Count = ml_integer_value(Iter->Value);
			ml_list_iter_next(Iter);
			Inst += 4; break;
		case MLIT_COUNT_CHARS: {
			ml_value_t *Chars = Iter->Value;
			if (!ml_is(Chars, MLAddressT)) return NULL;
			ml_list_iter_next(Iter);
			Inst[1].Count = ml_address_length(Chars);
			Inst[2].Chars = ml_address_value(Chars);
			Inst += 3; break;
		}
		case MLIT_DECL:
			Inst[1].Decls = Decls[ml_integer_value(Iter->Value)];
			ml_list_iter_next(Iter);
			Inst += 2; break;
		case MLIT_COUNT_DECL:
			Inst[1].Count = ml_integer_value(Iter->Value);
			ml_list_iter_next(Iter);
			Inst[2].Decls = Decls[ml_integer_value(Iter->Value)];
			ml_list_iter_next(Iter);
			Inst += 3; break;
		case MLIT_COUNT_COUNT_DECL:
			Inst[1].Count = ml_integer_value(Iter->Value);
			ml_list_iter_next(Iter);
			Inst[2].Count = ml_integer_value(Iter->Value);
			ml_list_iter_next(Iter);
			Inst[3].Decls = Decls[ml_integer_value(Iter->Value)];
			ml_list_iter_next(Iter);
			Inst += 4; break;
		case MLIT_CLOSURE:
			Inst[1].ClosureInfo = ml_minijs_decode_closure_info(Decoder, Iter->Value);
			if (!Inst[1].ClosureInfo) return NULL;
			ml_list_iter_next(Iter);
			for (int J = 0; J < Inst[1].ClosureInfo->NumUpValues; ++J) {
				Inst[J + 2].Count = ml_integer_value(Iter->Value);
				ml_list_iter_next(Iter);
			}
			Inst += 2 + Inst[1].ClosureInfo->NumUpValues;
			break;
		case MLIT_SWITCH: {
			ml_value_t *Switches = Iter->Value;
			if (!ml_is(Switches, MLListT)) return NULL;
			ml_list_iter_next(Iter);
			Inst[1].Count = ml_list_length(Switches) - 1;
			ml_inst_t **Ptr = Inst[2].Insts = anew(ml_inst_t *, Inst[1].Count);
			for (int J = 1; J <= Inst[1].Count; ++J) {
				*Ptr++ = Code + Offsets[ml_integer_value(ml_list_get(Switches, J + 1))];
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

static ml_value_t *ml_minijs_decode_closure(ml_minijs_decoder_t *Decoder, ml_list_node_t *Node, intptr_t Index) {
	if (!Node) return ml_error("TypeError", "Closure requires additional fields");
	ml_value_t *InfoJson = Node->Value;
	Node = Node->Next;
	if (!ml_is(InfoJson, MLListT)) return ml_error("TypeError", "Closure info requires list");
	int NumUpValues = ml_integer_value(ml_list_get(InfoJson, 7));
	ml_closure_t *Closure = xnew(ml_closure_t, NumUpValues, ml_value_t *);
	Closure->Type = MLClosureT;
	if (Index >= 0) inthash_insert(Decoder->Cached, Index, Closure);
	Closure->Info = ml_minijs_decode_closure_info(Decoder, InfoJson);
	if (!Closure->Info) return ml_error("MinijsError", "Invalid closure information");
	for (int I = 0; I < NumUpValues; ++I) {
		if (!Node) return ml_error("TypeError", "Closure requires additional fields");
		Closure->UpValues[I] = ml_minijs_decode(Decoder, Node->Value);
		Node = Node->Next;
	}
	return (ml_value_t *)Closure;
}

ML_METHOD_ANON(MinijsDecode, "minijs::decode");

ML_METHOD(MinijsDecode, MLAnyT) {
//@minijs::decode
//<Json
//>any|error
	ml_minijs_decoder_t Decoder[1] = {MLExternals, {INTHASH_INIT}};
	return ml_minijs_decode(Decoder, Args[0]);
}

ML_METHOD(MinijsDecode, MLAnyT, MLExternalSetT) {
//@minijs::decode
//<Json
//<Externals
//>any|error
	ml_minijs_decoder_t Decoder[1] = {(ml_externals_t *)Args[1], {INTHASH_INIT}};
	return ml_minijs_decode(Decoder, Args[0]);
}

ML_FUNCTION(MLMinijs) {
//@minijs
//<Value:any
//>minijs
	ML_CHECK_ARG_COUNT(1);
	ml_minijs_encoder_t Encoder[1] = {MLExternals, {INTHASH_INIT}, 0};
	ml_value_t *Value = ml_minijs_encode(Encoder, Args[0]);
	if (ml_is_error(Value)) return Value;
	ml_minijs_t *Minijs = new(ml_minijs_t);
	Minijs->Type = MLMinijsT;
	Minijs->Value = Value;
	return (ml_value_t *)Minijs;
}

ML_TYPE(MLMinijsT, (), "minijs",
	.Constructor = (ml_value_t *)MLMinijs
);

ML_METHOD("value", MLMinijsT) {
	ml_minijs_t *Minijs = (ml_minijs_t *)Args[0];
	return Minijs->Value;
}

#ifdef ML_CBOR

#include "ml_cbor.h"
#include "minicbor/minicbor.h"

static void ML_TYPED_FN(ml_cbor_write, MLMinijsT, ml_cbor_writer_t *Writer, ml_minijs_t *Value) {
	ml_cbor_write_tag(Writer, 27);
	ml_cbor_write_array(Writer, 2);
	ml_cbor_write_string(Writer, strlen("minijs"));
	ml_cbor_write_raw(Writer, "minijs", strlen("minijs"));
	ml_cbor_write(Writer, Value->Value);
}

#endif

void ml_minijs_init(stringmap_t *Globals) {
	stringmap_insert(Decoders, "^", ml_minijs_decode_global);
	stringmap_insert(Decoders, "blank", ml_minijs_decode_blank);
	stringmap_insert(Decoders, "_", ml_minijs_decode_blank);
	stringmap_insert(Decoders, "some", ml_minijs_decode_some);
	stringmap_insert(Decoders, "tuple", ml_minijs_decode_tuple);
	stringmap_insert(Decoders, "()", ml_minijs_decode_tuple);
	stringmap_insert(Decoders, "list", ml_minijs_decode_list);
	stringmap_insert(Decoders, "l", ml_minijs_decode_list);
	stringmap_insert(Decoders, "names", ml_minijs_decode_names);
	stringmap_insert(Decoders, "n", ml_minijs_decode_names);
	stringmap_insert(Decoders, "map", ml_minijs_decode_map);
	stringmap_insert(Decoders, "m", ml_minijs_decode_map);
	stringmap_insert(Decoders, "type", ml_minijs_decode_type);
	stringmap_insert(Decoders, "t", ml_minijs_decode_type);
#ifdef ML_COMPLEX
	stringmap_insert(Decoders, "complex", ml_minijs_decode_complex);
	stringmap_insert(Decoders, "c", ml_minijs_decode_complex);
#endif
	stringmap_insert(Decoders, "regex", ml_minijs_decode_regex);
	stringmap_insert(Decoders, "r", ml_minijs_decode_regex);
	stringmap_insert(Decoders, "method", ml_minijs_decode_method);
	stringmap_insert(Decoders, ":", ml_minijs_decode_method);
	stringmap_insert(Decoders, "variable", ml_minijs_decode_variable);
	stringmap_insert(Decoders, "v", ml_minijs_decode_variable);
	stringmap_insert(Decoders, "closure", ml_minijs_decode_closure);
	stringmap_insert(Decoders, "z", ml_minijs_decode_closure);
	stringmap_insert(Decoders, "o", ml_minijs_decode_object);
#ifdef ML_TIME
	stringmap_insert(Decoders, "time", ml_minijs_decode_time);
#endif
#ifdef ML_MATH
	stringmap_insert(Decoders, "array", ml_minijs_decode_array);
#endif
#ifdef ML_UUID
	stringmap_insert(Decoders, "uuid", ml_minijs_decode_uuid);
#endif
#include "ml_minijs_init.c"
	stringmap_insert(MLMinijsT->Exports, "encode", MinijsEncode);
	stringmap_insert(MLMinijsT->Exports, "decode", MinijsDecode);
	if (Globals) {
		stringmap_insert(Globals, "minijs", MLMinijsT);
	}
}

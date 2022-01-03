#include "ml_uuid.h"
#include "ml_macros.h"

#undef ML_CATEGORY
#define ML_CATEGORY "uuid"

static long ml_uuid_hash(ml_uuid_t *UUID, ml_hash_chain_t *Chain) {
	return *(long *)UUID->Value;
}

ML_TYPE(MLUUIDT, (), "uuid",
	.hash = (void *)ml_uuid_hash
);

ml_value_t *ml_uuid(const uuid_t Value) {
	ml_uuid_t *UUID = new(ml_uuid_t);
	UUID->Type = MLUUIDT;
	uuid_copy(UUID->Value, Value);
	return (ml_value_t *)UUID;
}

ml_value_t *ml_uuid_parse(const char *Value, int Length) {
	ml_uuid_t *UUID = new(ml_uuid_t);
	UUID->Type = MLUUIDT;
	if (uuid_parse(Value, UUID->Value)) {
		return ml_error("UUIDError", "Invalid UUID string");
	}
	return (ml_value_t *)UUID;
}

ML_METHOD(MLUUIDT) {
	ml_uuid_t *UUID = new(ml_uuid_t);
	UUID->Type = MLUUIDT;
	uuid_generate(UUID->Value);
	return (ml_value_t *)UUID;
}

ML_METHOD(MLUUIDT, MLStringT) {
	return ml_uuid_parse(ml_string_value(Args[0]), ml_string_length(Args[0]));
}

ML_METHOD("append", MLStringBufferT, MLUUIDT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_uuid_t *UUID = (ml_uuid_t *)Args[1];
	char String[UUID_STR_LEN];
	uuid_unparse_lower(UUID->Value, String);
	ml_stringbuffer_write(Buffer, String, UUID_STR_LEN - 1);
	return Args[0];
}

ML_METHOD("<>", MLUUIDT, MLUUIDT) {
	ml_uuid_t *UUIDA = (ml_uuid_t *)Args[0];
	ml_uuid_t *UUIDB = (ml_uuid_t *)Args[1];
	return ml_integer(uuid_compare(UUIDA->Value, UUIDB->Value));
}

#define ml_comp_method_time_time(NAME, SYMBOL) \
	ML_METHOD(NAME, MLUUIDT, MLUUIDT) { \
		ml_uuid_t *UUIDA = (ml_uuid_t *)Args[0]; \
		ml_uuid_t *UUIDB = (ml_uuid_t *)Args[1]; \
		return uuid_compare(UUIDA->Value, UUIDB->Value) SYMBOL 0 ? Args[1] : MLNil; \
	}

ml_comp_method_time_time("=", ==);
ml_comp_method_time_time("!=", !=);
ml_comp_method_time_time("<", <);
ml_comp_method_time_time(">", >);
ml_comp_method_time_time("<=", <=);
ml_comp_method_time_time(">=", >=);

#ifdef ML_CBOR

#include "ml_cbor.h"

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLUUIDT, ml_uuid_t *UUID, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_tag(Data, WriteFn, 37);
	ml_cbor_write_bytes(Data, WriteFn, 16);
	WriteFn(Data, UUID->Value, 16);
	return NULL;
}

static ml_value_t *ml_cbor_read_uuid_fn(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (!ml_is(Value, MLStringT)) return ml_error("TagError", "UUID requires string");
	if (ml_string_length(Value) != 16) return ml_error("TagError", "UUID requires 16 bytes");
	return ml_uuid((unsigned char *)ml_string_value(Value));
}

#endif

void ml_uuid_init(stringmap_t *Globals) {
#include "ml_uuid_init.c"
	if (Globals) stringmap_insert(Globals, "uuid", MLUUIDT);
	ml_string_fn_register("U", ml_uuid_parse);
#ifdef ML_CBOR
	ml_cbor_default_tag(37, ml_cbor_read_uuid_fn);
#endif
}

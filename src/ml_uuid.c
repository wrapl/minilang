#include "ml_uuid.h"
#include "ml_macros.h"
#include "ml_compiler2.h"

#undef ML_CATEGORY
#define ML_CATEGORY "uuid"

// Overview
// .. note::
//    Depending on how *Minilang* is built, :mini:`uuid` might need to be imported using :mini:`import: uuid("util/uuid")`.

static long ml_uuid_hash(ml_uuid_t *UUID, ml_hash_chain_t *Chain) {
	return *(long *)UUID->Value;
}

ML_TYPE(MLUUIDT, (), "uuid",
// A UUID.
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

ml_value_t *ml_parser_escape_uuid(ml_parser_t *Parser) {
	const char *Next = ml_parser_clear(Parser);
	char Quote = *Next++;
	const char *End = Next;
	while (End[0] != Quote) {
		if (!End[0]) {
			ml_parse_warn(Parser, "ParseError", "End of input while parsing string");
			break;
		}
		++End;
	}
	int Length = End - Next;
	char *Raw = snew(Length + 1);
	memcpy(Raw, Next, Length);
	Raw[Length] = 0;
	ml_parser_input(Parser, End + 1, 0);
	ml_value_t *Value = ml_uuid_parse(Raw, Length);
	if (ml_is_error(Value)) return Value;
	mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
	ValueExpr->compile = ml_value_expr_compile;
	ml_source_t Source = ml_parser_position(Parser);
	ValueExpr->Source = Source.Name;
	ValueExpr->StartLine = ValueExpr->EndLine = Source.Line;
	ValueExpr->Value = Value;
	return ml_expr_value((mlc_expr_t *)ValueExpr);
}

static int ML_TYPED_FN(ml_value_is_constant, MLUUIDT, ml_value_t *Value) {
	return 1;
}

ML_METHOD(MLUUIDT) {
//>uuid
// Returns a new random UUID.
//$- import: uuid("util/uuid")
//$= uuid()
	ml_uuid_t *UUID = new(ml_uuid_t);
	UUID->Type = MLUUIDT;
	uuid_generate(UUID->Value);
	return (ml_value_t *)UUID;
}

ML_METHOD(MLUUIDT, MLStringT) {
//<String
//>uuid|error
// Parses :mini:`String` as a UUID, returning an error if :mini:`String` does not have the correct format.
//$- import: uuid("util/uuid")
//$= uuid("5fe1af82-02f9-429a-8787-4a7c16628a02")
//$= uuid("test")
	return ml_uuid_parse(ml_string_value(Args[0]), ml_string_length(Args[0]));
}

ML_METHOD(MLUUIDT, MLAddressT) {
//<Address
//>uuid|error
// Returns a new UUID with the bytes of :mini:`Address`.
	if (ml_address_length(Args[0]) < sizeof(uuid_t)) return ml_error("UUIDError", "16 bytes required for UUID");
	ml_uuid_t *UUID = new(ml_uuid_t);
	UUID->Type = MLUUIDT;
	memcpy(UUID->Value, ml_address_value(Args[0]), sizeof(uuid_t));
	return (ml_value_t *)UUID;
}

ML_METHOD(MLAddressT, MLUUIDT) {
//<UUID
//>address
// Returns an address view of :mini:`UUID`.
//$- import: uuid("util/uuid")
//$= address(uuid())
	ml_uuid_t *UUID = (ml_uuid_t *)Args[0];
	ml_address_t *Address = new(ml_address_t);
	Address->Type = MLAddressT;
	Address->Value = (char *)UUID->Value;
	Address->Length = sizeof(uuid_t);
	return (ml_value_t *)Address;
}

ML_METHOD("append", MLStringBufferT, MLUUIDT) {
//<Buffer
//<UUID
// Appends a representation of :mini:`UUID` to :mini:`Buffer`.
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

static void ML_TYPED_FN(ml_cbor_write, MLUUIDT, ml_cbor_writer_t *Writer, ml_uuid_t *UUID) {
	ml_cbor_write_tag(Writer, 37);
	ml_cbor_write_bytes(Writer, 16);
	ml_cbor_write_raw(Writer, UUID->Value, 16);
}

static ml_value_t *ml_cbor_read_uuid_fn(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (!ml_is(Value, MLAddressT)) return ml_error("TagError", "UUID requires string");
	if (ml_string_length(Value) != 16) return ml_error("TagError", "UUID requires 16 bytes");
	return ml_uuid((unsigned char *)ml_string_value(Value));
}

#endif

void ml_uuid_init(stringmap_t *Globals) {
#include "ml_uuid_init.c"
	ml_method_by_value(MLUUIDT->Constructor, NULL, ml_identity, MLUUIDT, NULL);
	uuid_t NilUUID = {0,};
	stringmap_insert(MLUUIDT->Exports, "nil", ml_uuid(NilUUID));
	if (Globals) stringmap_insert(Globals, "uuid", MLUUIDT);
	//ml_string_fn_register("U", ml_uuid_parse);
#ifdef ML_CBOR
	ml_cbor_default_tag(ML_CBOR_TAG_UUID, ml_cbor_read_uuid_fn);
	ml_externals_default_add("uuid", MLUUIDT);
#endif
	ml_parser_add_escape(NULL, "U", ml_parser_escape_uuid);
}

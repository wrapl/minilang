#include "minilang.h"
#include "ml_macros.h"
#include "ml_cbor.h"
#include <gc/gc.h>
#include <string.h>

cbor_item_t *ml_to_cbor_item(ml_value_t *Value) {
	typeof(ml_to_cbor_item) *function = ml_typed_fn_get(Value->Type, ml_to_cbor_item);
	if (!function) return cbor_new_undef();
	return function(Value);
}

ml_cbor_t ml_to_cbor(ml_value_t *Value) {
	cbor_item_t *Item = ml_to_cbor_item(Value);
	size_t Size = 16;
	for (int I = 0; I < 31; ++I) {
		cbor_mutable_data Buffer = GC_malloc_atomic(Size);
		size_t Length = cbor_serialize(Item, Buffer, Size);
		if (Length) return (ml_cbor_t){Buffer, Length};
		Size *= 2;
	}
	return (ml_cbor_t){NULL, 0};
}

typedef struct block_t {
	struct block_t *Prev;
	const void *Data;
	size_t Length;
} block_t;

typedef struct collection_t {
	struct collection_t *Prev;
	struct tag_t *Tags;
	ml_value_t *Key;
	ml_value_t *Collection;
	block_t *Blocks;
	size_t Remaining;
} collection_t;

typedef struct tag_t {
	struct tag_t *Prev;
	ml_tag_t Handler;
	void *Data;
} tag_t;

typedef struct decoder_t {
	collection_t *Collection;
	tag_t *Tags;
	ml_value_t *Value;
	ml_tag_t (*TagFn)(uint64_t Tag, void *TagFnData, void **TagData);
	void *TagFnData;
} decoder_t;

static ml_value_t IsByteString[1];
static ml_value_t IsString[1];
static ml_value_t IsList[1];

static void value_handler(decoder_t *Decoder, ml_value_t *Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	for (tag_t *Tag = Decoder->Tags; Tag; Tag = Tag->Prev) {
		//printf("%s:%d\n", __func__, __LINE__);
		if (Value->Type != MLErrorT) Value = Tag->Handler(Value, Tag->Data);
	}
	Decoder->Tags = 0;
	collection_t *Collection = Decoder->Collection;
	if (!Collection) {
		//printf("%s:%d\n", __func__, __LINE__);
		Decoder->Value = Value;
	} else if (Collection->Key == IsList) {
		//printf("%s:%d\n", __func__, __LINE__);
		ml_list_append(Collection->Collection, Value);
		if (Collection->Remaining && --Collection->Remaining == 0) {
			Decoder->Collection = Collection->Prev;
			Decoder->Tags = Collection->Tags;
			value_handler(Decoder, Collection->Collection);
		}
	} else if (Collection->Key) {
		//printf("%s:%d\n", __func__, __LINE__);
		ml_map_insert(Collection->Collection, Collection->Key, Value);
		if (Collection->Remaining && --Collection->Remaining == 0) {
			Decoder->Collection = Collection->Prev;
			Decoder->Tags = Collection->Tags;
			value_handler(Decoder, Collection->Collection);
		} else {
			Collection->Key = 0;
		}
	} else {
		//printf("%s:%d\n", __func__, __LINE__);
		Collection->Key = Value;
	}
}

static void ml_uint8_cb(decoder_t *Decoder, uint8_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	value_handler(Decoder, ml_integer(Value));
}

static void ml_uint16_cb(decoder_t *Decoder, uint16_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	value_handler(Decoder, ml_integer(Value));
}

static void ml_uint32_cb(decoder_t *Decoder, uint32_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Value <= 0x7FFFFFFF) {
		value_handler(Decoder, ml_integer(Value));
	} else {
		value_handler(Decoder, ml_integer(Value));
		// TODO: Implement large numbers somehow
		// mpz_t Temp;
		// mpz_init_set_ui(Temp, Value);
		// value_handler(Decoder, Std$Integer$new_big(Value));
	}
}

static void ml_uint64_cb(decoder_t *Decoder, uint64_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Value <= 0x7FFFFFFFL) {
		value_handler(Decoder, ml_integer((uint32_t)Value));
	} else {
		// TODO: Implement large numbers somehow
		value_handler(Decoder, ml_integer(Value));
	}
}

static void ml_int8_cb(decoder_t *Decoder, uint8_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	int32_t Value0 = (int32_t)Value;
	value_handler(Decoder, ml_integer(~Value0));
}

static void ml_int16_cb(decoder_t *Decoder, uint16_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	int32_t Value0 = (int32_t)Value;
	value_handler(Decoder, ml_integer(~Value0));
}

static void ml_int32_cb(decoder_t *Decoder, uint32_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	int32_t Value0 = (int32_t)Value;
	value_handler(Decoder, ml_integer(~Value));
}

static void ml_int64_cb(decoder_t *Decoder, uint64_t Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Value <= 0x7FFFFFFFL) {
		value_handler(Decoder, ml_integer(~(uint32_t)Value));
	} else {
		value_handler(Decoder, ml_integer(Value));
		// TODO: Implement large numbers somehow
		// mpz_t Temp;
		// mpz_init_set_ui(Temp, (uint32_t)(Value >> 32));
		// mpz_mul_2exp(Temp, Temp, 32);
		// mpz_add_ui(Temp, Temp, (uint32_t)Value);
		// mpz_com(Temp, Temp);
		// value_handler(Decoder, Std$Integer$new_big(Temp));
	}
}

static void ml_byte_string_start_cb(decoder_t *Decoder) {
	//printf("%s:%d\n", __func__, __LINE__);
	collection_t *Collection = new(collection_t);
	Collection->Prev = Decoder->Collection;
	Collection->Tags = Decoder->Tags;
	Decoder->Tags = 0;
	Collection->Key = IsByteString;
	Collection->Remaining = 0;
	Collection->Blocks = 0;
	Decoder->Collection = Collection;
}

static void ml_byte_string_cb(decoder_t *Decoder, cbor_data Data, size_t Length) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Decoder->Collection && Decoder->Collection->Key == IsByteString) {
		block_t *Block = new(block_t);
		Block->Prev = Decoder->Collection->Blocks;
		Block->Data = Data;
		Block->Length = Length;
		Decoder->Collection->Blocks = Block;
		Decoder->Collection->Remaining += Length;
	} else {
		value_handler(Decoder, ml_string(Data, Length));
	}
}

static void ml_string_start_cb(decoder_t *Decoder) {
	//printf("%s:%d\n", __func__, __LINE__);
	collection_t *Collection = new(collection_t);
	Collection->Prev = Decoder->Collection;
	Collection->Tags = Decoder->Tags;
	Decoder->Tags = 0;
	Collection->Key = IsString;
	Collection->Remaining = 0;
	Collection->Blocks = 0;
	Decoder->Collection = Collection;
}

static void ml_string_cb(decoder_t *Decoder, cbor_data Data, size_t Length) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Decoder->Collection && Decoder->Collection->Key == IsString) {
		block_t *Block = new(block_t);
		Block->Prev = Decoder->Collection->Blocks;
		Block->Data = Data;
		Block->Length = Length;
		Decoder->Collection->Blocks = Block;
		Decoder->Collection->Remaining += Length;
	} else {
		value_handler(Decoder, ml_string(Data, Length));
	}
}

static void ml_indef_array_start_cb(decoder_t *Decoder) {
	//printf("%s:%d\n", __func__, __LINE__);
	collection_t *Collection = new(collection_t);
	Collection->Prev = Decoder->Collection;
	Collection->Tags = Decoder->Tags;
	Decoder->Tags = 0;
	Collection->Remaining = 0;
	Collection->Key = IsList;
	Collection->Collection = ml_list();
	Decoder->Collection = Collection;
}

static void ml_array_start_cb(decoder_t *Decoder, size_t Length) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Length > 0) {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Decoder->Collection;
		Collection->Tags = Decoder->Tags;
		Decoder->Tags = 0;
		Collection->Remaining = Length;
		Collection->Key = IsList;
		Collection->Collection = ml_list();
		Decoder->Collection = Collection;
	} else {
		value_handler(Decoder, ml_list());
	}
}

static void ml_indef_map_start_cb(decoder_t *Decoder) {
	//printf("%s:%d\n", __func__, __LINE__);
	collection_t *Collection = new(collection_t);
	Collection->Prev = Decoder->Collection;
	Collection->Tags = Decoder->Tags;
	Decoder->Tags = 0;
	Collection->Remaining = 0;
	Collection->Key = 0;
	Collection->Collection = ml_map();
	Decoder->Collection = Collection;
}

static void ml_map_start_cb(decoder_t *Decoder, size_t Length) {
	//printf("%s:%d\n", __func__, __LINE__);
	if (Length > 0) {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Decoder->Collection;
		Collection->Tags = Decoder->Tags;
		Decoder->Tags = 0;
		Collection->Remaining = Length;
		Collection->Key = 0;
		Collection->Collection = ml_map();
		Decoder->Collection = Collection;
	} else {
		value_handler(Decoder, ml_map());
	}
}

static void ml_tag_cb(decoder_t *Decoder, uint64_t Tag) {
	//printf("%s:%d\n", __func__, __LINE__);
	void *Data;
	ml_tag_t Handler = Decoder->TagFn(Tag, Decoder->TagFnData, &Data);
	if (Handler) {
		tag_t *Tag = new(tag_t);
		Tag->Prev = Decoder->Tags;
		Tag->Handler = Handler;
		Tag->Data = Data;
		Decoder->Tags = Tag;
	}
}

static void ml_float_cb(decoder_t *Decoder, float Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	value_handler(Decoder, ml_real(Value));
}

static void ml_double_cb(decoder_t *Decoder, double Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	value_handler(Decoder, ml_real(Value));
}

static void ml_null_cb(decoder_t *Decoder) {
	//printf("%s:%d\n", __func__, __LINE__);
	value_handler(Decoder, MLNil);
}

static void ml_boolean_cb(decoder_t *Decoder, bool Value) {
	//printf("%s:%d\n", __func__, __LINE__);
	value_handler(Decoder, ml_method(Value ? "true" : "false"));
}

static void ml_indef_break_cb(decoder_t *Decoder) {
	//printf("%s:%d\n", __func__, __LINE__);
	collection_t *Collection = Decoder->Collection;
	Decoder->Collection = Collection->Prev;
	Decoder->Tags = Collection->Tags;
	if (Collection->Key == IsByteString) {
		char *Buffer = GC_malloc_atomic(Collection->Remaining);
		Buffer += Collection->Remaining;
		for (block_t *B = Collection->Blocks; B; B = B->Prev) {
			Buffer -= B->Length;
			memcpy(Buffer, B->Data, B->Length);
		}
		value_handler(Decoder, ml_string(Buffer, Collection->Remaining));
	} else if (Collection->Key == IsString) {
		char *Buffer = GC_malloc_atomic(Collection->Remaining);
		Buffer += Collection->Remaining;
		for (block_t *B = Collection->Blocks; B; B = B->Prev) {
			Buffer -= B->Length;
			memcpy(Buffer, B->Data, B->Length);
		}
		value_handler(Decoder, ml_string(Buffer, Collection->Remaining));
	} else if (Collection->Key == IsList) {
		value_handler(Decoder, Collection->Collection);
	} else {
		value_handler(Decoder, Collection->Collection);
	}
}

static struct cbor_callbacks Callbacks = {
	.uint8 = (void *)ml_uint8_cb,
	.uint16 = (void *)ml_uint16_cb,
	.uint32 = (void *)ml_uint32_cb,
	.uint64 = (void *)ml_uint64_cb,
	.negint64 = (void *)ml_int64_cb,
	.negint32 = (void *)ml_int32_cb,
	.negint16 = (void *)ml_int16_cb,
	.negint8 = (void *)ml_int8_cb,
	.byte_string_start = (void *)ml_byte_string_start_cb,
	.byte_string = (void *)ml_byte_string_cb,
	.string = (void *)ml_string_cb,
	.string_start = (void *)ml_string_start_cb,
	.indef_array_start = (void *)ml_indef_array_start_cb,
	.array_start = (void *)ml_array_start_cb,
	.indef_map_start = (void *)ml_indef_map_start_cb,
	.map_start = (void *)ml_map_start_cb,
	.tag = (void *)ml_tag_cb,
	.float2 = (void *)ml_float_cb,
	.float4 = (void *)ml_float_cb,
	.float8 = (void *)ml_double_cb,
	.undefined = (void *)ml_null_cb,
	.null = (void *)ml_null_cb,
	.boolean = (void *)ml_boolean_cb,
	.indef_break = (void *)ml_indef_break_cb
};

ml_value_t *ml_from_cbor(ml_cbor_t Cbor, ml_tag_t (*TagFn)(uint64_t, void *, void **), void *TagFnData) {
	decoder_t Decoder;
	Decoder.Collection = 0;
	Decoder.TagFn = TagFn;
	Decoder.TagFnData = TagFnData;
	Decoder.Tags = 0;
	Decoder.Value = MLNil;
	while (Cbor.Length) {
		struct cbor_decoder_result DecoderResult = cbor_stream_decode(
			Cbor.Data, Cbor.Length, &Callbacks, &Decoder
		);
		if (DecoderResult.status != CBOR_DECODER_FINISHED) {
			return ml_error("CborError", "Error decoding cbor");
		}
		Cbor.Length -= DecoderResult.read;
		Cbor.Data += DecoderResult.read;
	}
	return Decoder.Value;
}

static ml_value_t *ml_to_cbor_fn(void *Data, int Count, ml_value_t **Args) {
	ml_cbor_t Cbor = ml_to_cbor(Args[0]);
	if (Cbor.Data) return ml_string(Cbor.Data, Cbor.Length);
	return ml_error("CborError", "Error encoding to cbor");
}

static ml_value_t *ml_value_fn(ml_value_t *Value, ml_value_t *Callback) {
	return ml_inline(Callback, 1, Value);
}

static ml_tag_t ml_value_tag_fn(uint64_t Tag, ml_value_t *Callback, void **Data) {
	Data[0] = ml_inline(Callback, 1, ml_integer(Tag));
	return ml_value_fn;
}

static ml_value_t *ml_from_cbor_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_cbor_t Cbor = {ml_string_value(Args[0]), ml_string_length(Args[0])};
	return ml_from_cbor(Cbor, ml_value_tag_fn, Count > 1 ? Args[1] : MLNil);
}

cbor_item_t *ml_integer_to_cbor_item(ml_value_t *Arg) {
	//printf("%s()\n", __func__);
	int64_t Value = ml_integer_value(Arg);
	cbor_item_t *Item;
	if (Value < 0) {
		Value = ~Value;
		if (Value < 256) {
			Item = cbor_build_uint8(Value);
		} else if (Value < 65536) {
			Item = cbor_build_uint16(Value);
		} else if (Value < 0xFFFFFFFF){
			Item = cbor_build_uint32(Value);
		} else {
			Item = cbor_build_uint64(Value);
		}
		cbor_mark_negint(Item);
	} else {
		if (Value < 256) {
			Item = cbor_build_uint8(Value);
		} else if (Value < 65536) {
			Item = cbor_build_uint16(Value);
		} else if (Value < 0xFFFFFFFF){
			Item = cbor_build_uint32(Value);
		} else {
			Item = cbor_build_uint32(Value);
		}
	}
	return Item;
}

cbor_item_t *ml_string_to_cbor_item(ml_value_t *Arg) {
	//printf("%s()\n", __func__);
	return cbor_build_stringn(ml_string_value(Arg), ml_string_length(Arg));
}

cbor_item_t *ml_list_to_cbor_item(ml_value_t *Arg) {
	cbor_item_t *Item = cbor_new_definite_array(ml_list_length(Arg));
	for (ml_list_node_t *Node = ml_list_head(Arg); Node; Node = Node->Next) {
		cbor_item_t *Child = ml_to_cbor_item(Node->Value);
		cbor_array_push(Item, Child);
	}
	return Item;
}

static int build_map_pair(ml_value_t *Key, ml_value_t *Value, cbor_item_t *Item) {
	struct cbor_pair Pair = {ml_to_cbor_item(Key), ml_to_cbor_item(Value)};
	cbor_map_add(Item, Pair);
	return 0;
}

cbor_item_t *ml_map_to_cbor_item(ml_value_t *Arg) {
	cbor_item_t *Item = cbor_new_definite_map(ml_map_size(Arg));
	ml_map_foreach(Arg, Item, (void *)build_map_pair);
	return Item;
}

cbor_item_t *ml_real_to_cbor_item(ml_value_t *Arg) {
	return cbor_build_float8(ml_real_value(Arg));
}

cbor_item_t *ml_nil_to_cbor_item(ml_value_t *Arg) {
	return cbor_build_ctrl(CBOR_CTRL_NULL);
}

cbor_item_t *ml_symbol_to_cbor_item(ml_value_t *Arg) {
	if (!strcmp(ml_method_name(Arg), "true")) {
		return cbor_build_ctrl(CBOR_CTRL_TRUE);
	} else {
		return cbor_build_ctrl(CBOR_CTRL_FALSE);
	}
}

void ml_cbor_init(stringmap_t *Globals) {
	ml_typed_fn_set(MLIntegerT, ml_to_cbor_item, ml_integer_to_cbor_item);
	ml_typed_fn_set(MLStringT, ml_to_cbor_item, ml_string_to_cbor_item);
	ml_typed_fn_set(MLListT, ml_to_cbor_item, ml_list_to_cbor_item);
	ml_typed_fn_set(MLMapT, ml_to_cbor_item, ml_map_to_cbor_item);
	ml_typed_fn_set(MLRealT, ml_to_cbor_item, ml_real_to_cbor_item);
	ml_typed_fn_set(MLNilT, ml_to_cbor_item, ml_nil_to_cbor_item);
	ml_typed_fn_set(MLMethodT, ml_to_cbor_item, ml_symbol_to_cbor_item);
	if (Globals) {
		stringmap_insert(Globals, "cbor_encode", ml_function(NULL, ml_to_cbor_fn));
		stringmap_insert(Globals, "cbor_decode", ml_function(NULL, ml_from_cbor_fn));
	}
}

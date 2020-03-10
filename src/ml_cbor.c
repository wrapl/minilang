#include "minilang.h"
#include "ml_macros.h"
#include "ml_cbor.h"
#include <gc/gc.h>
#include <string.h>

#include "minicbor/minicbor.h"

void ml_cbor_write(ml_value_t *Value, void *Data, ml_cbor_write_fn WriteFn) {
	typeof(ml_cbor_write) *function = ml_typed_fn_get(Value->Type, ml_cbor_write);
	if (function) {
		function(Value, Data, WriteFn);
	} else {
		ml_cbor_write_simple(Data, WriteFn, CBOR_SIMPLE_UNDEF);
	}
}

static void ml_cbor_size_fn(size_t *Total, unsigned char *Bytes, int Size) {
	*Total += Size;
}

static void ml_cbor_bytes_fn(unsigned char **End, unsigned char *Bytes, int Size) {
	memcpy(*End, Bytes, Size);
	*End += Size;
}

ml_cbor_t ml_to_cbor(ml_value_t *Value) {
	size_t Size = 0;
	ml_cbor_write(Value, &Size, (void *)ml_cbor_size_fn);
	unsigned char *Bytes = GC_MALLOC_ATOMIC(Size), *End = Bytes;
	ml_cbor_write(Value, &End, (void *)ml_cbor_bytes_fn);
	return (ml_cbor_t){Bytes, Size};
}

typedef struct block_t {
	struct block_t *Prev;
	const void *Data;
	int Size;
} block_t;

typedef struct collection_t {
	struct collection_t *Prev;
	struct tag_t *Tags;
	ml_value_t *Key;
	ml_value_t *Collection;
	block_t *Blocks;
	int Remaining;
} collection_t;

typedef struct tag_t {
	struct tag_t *Prev;
	ml_tag_t Handler;
	void *Data;
} tag_t;

typedef struct ml_cbor_reader_t {
	collection_t *Collection;
	tag_t *Tags;
	ml_value_t *Value;
	ml_tag_t (*TagFn)(uint64_t Tag, void *Data, void **TagData);
	void *TagFnData;
	minicbor_reader_t Reader[1];
} ml_cbor_reader_t;

ml_cbor_reader_t *ml_cbor_reader_new(void *TagFnData, ml_tag_t (*TagFn)(uint64_t, void *, void **)) {
	ml_cbor_reader_t *Reader = new(ml_cbor_reader_t);
	Reader->TagFnData = TagFnData;
	Reader->TagFn = TagFn;
	ml_cbor_reader_init(Reader->Reader);
	Reader->Reader->UserData = Reader;
	return Reader;
}

void ml_cbor_reader_read(ml_cbor_reader_t *Reader, unsigned char *Bytes, int Size) {
	ml_cbor_read(Reader->Reader, Bytes, Size);
}

ml_value_t *ml_cbor_reader_get(ml_cbor_reader_t *Reader) {
	if (!Reader->Value) return ml_error("CBORError", "CBOR not completely read");
	return Reader->Value;
}

int ml_cbor_reader_extra(ml_cbor_reader_t *Reader) {
	return ml_cbor_reader_remaining(Reader->Reader);
}

static ml_value_t IsByteString[1];
static ml_value_t IsString[1];
static ml_value_t IsList[1];

static void value_handler(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	for (tag_t *Tag = Reader->Tags; Tag; Tag = Tag->Prev) {
		if (Value->Type != MLErrorT) Value = Tag->Handler(Tag->Data, Value);
	}
	Reader->Tags = 0;
	collection_t *Collection = Reader->Collection;
	if (!Collection) {
		Reader->Value = Value;
		ml_cbor_reader_finish(Reader->Reader);
	} else if (Collection->Key == IsList) {
		ml_list_append(Collection->Collection, Value);
		if (Collection->Remaining && --Collection->Remaining == 0) {
			Reader->Collection = Collection->Prev;
			Reader->Tags = Collection->Tags;
			value_handler(Reader, Collection->Collection);
		}
	} else if (Collection->Key) {
		ml_map_insert(Collection->Collection, Collection->Key, Value);
		if (Collection->Remaining && --Collection->Remaining == 0) {
			Reader->Collection = Collection->Prev;
			Reader->Tags = Collection->Tags;
			value_handler(Reader, Collection->Collection);
		} else {
			Collection->Key = 0;
		}
	} else {
		Collection->Key = Value;
	}
}

void ml_cbor_read_positive_fn(ml_cbor_reader_t *Reader, uint64_t Value) {
	value_handler(Reader, ml_integer(Value));
}

void ml_cbor_read_negative_fn(ml_cbor_reader_t *Reader, uint64_t Value) {
	if (Value <= 0x7FFFFFFFL) {
		value_handler(Reader, ml_integer(~(uint32_t)Value));
	} else {
		value_handler(Reader, ml_integer(Value));
		// TODO: Implement large numbers somehow
		// mpz_t Temp;
		// mpz_init_set_ui(Temp, (uint32_t)(Value >> 32));
		// mpz_mul_2exp(Temp, Temp, 32);
		// mpz_add_ui(Temp, Temp, (uint32_t)Value);
		// mpz_com(Temp, Temp);
		// value_handler(Reader, Std$Integer$new_big(Temp));
	}
}

void ml_cbor_read_bytes_fn(ml_cbor_reader_t *Reader, int Size) {
	if (Size) {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Reader->Collection;
		Collection->Tags = Reader->Tags;
		Reader->Tags = 0;
		Collection->Key = IsByteString;
		Collection->Remaining = 0;
		Collection->Blocks = 0;
		Reader->Collection = Collection;
	} else {
		value_handler(Reader, ml_string("", 0));
	}
}

void ml_cbor_read_bytes_piece_fn(ml_cbor_reader_t *Reader, const void *Bytes, int Size, int Final) {
	collection_t *Collection = Reader->Collection;
	if (Final) {
		Reader->Collection = Collection->Prev;
		Reader->Tags = Collection->Tags;
		int Total = Collection->Remaining + Size;
		char *Buffer = GC_MALLOC_ATOMIC(Total);
		Buffer += Collection->Remaining;
		memcpy(Buffer, Bytes, Size);
		for (block_t *B = Collection->Blocks; B; B = B->Prev) {
			Buffer -= B->Size;
			memcpy(Buffer, B->Data, B->Size);
		}
		value_handler(Reader, ml_string(Buffer, Total));
	} else {
		block_t *Block = new(block_t);
		Block->Prev = Collection->Blocks;
		Block->Data = Bytes;
		Block->Size = Size;
		Collection->Blocks = Block;
		Collection->Remaining += Size;
	}
}

void ml_cbor_read_string_fn(ml_cbor_reader_t *Reader, int Size) {
	if (Size) {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Reader->Collection;
		Collection->Tags = Reader->Tags;
		Reader->Tags = 0;
		Collection->Key = IsString;
		Collection->Remaining = 0;
		Collection->Blocks = 0;
		Reader->Collection = Collection;
	} else {
		value_handler(Reader, ml_string("", 0));
	}
}

void ml_cbor_read_string_piece_fn(ml_cbor_reader_t *Reader, const void *Bytes, int Size, int Final) {
	collection_t *Collection = Reader->Collection;
	if (Final) {
		Reader->Collection = Collection->Prev;
		Reader->Tags = Collection->Tags;
		int Total = Collection->Remaining + Size;
		char *Buffer = GC_MALLOC_ATOMIC(Total);
		Buffer += Collection->Remaining;
		memcpy(Buffer, Bytes, Size);
		for (block_t *B = Collection->Blocks; B; B = B->Prev) {
			Buffer -= B->Size;
			memcpy(Buffer, B->Data, B->Size);
		}
		value_handler(Reader, ml_string(Buffer, Total));
	} else {
		block_t *Block = new(block_t);
		Block->Prev = Collection->Blocks;
		Block->Data = Bytes;
		Block->Size = Size;
		Collection->Blocks = Block;
		Collection->Remaining += Size;
	}
}

void ml_cbor_read_array_fn(ml_cbor_reader_t *Reader, int Size) {
	if (Size) {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Reader->Collection;
		Collection->Tags = Reader->Tags;
		Reader->Tags = 0;
		Collection->Remaining = Size;
		Collection->Key = IsList;
		Collection->Collection = ml_list();
		Reader->Collection = Collection;
	} else {
		value_handler(Reader, ml_list());
	}
}

void ml_cbor_read_map_fn(ml_cbor_reader_t *Reader, int Size) {
	if (Size > 0) {
		collection_t *Collection = new(collection_t);
		Collection->Prev = Reader->Collection;
		Collection->Tags = Reader->Tags;
		Reader->Tags = 0;
		Collection->Remaining = Size;
		Collection->Key = 0;
		Collection->Collection = ml_map();
		Reader->Collection = Collection;
	} else {
		value_handler(Reader, ml_map());
	}
}

void ml_cbor_read_tag_fn(ml_cbor_reader_t *Reader, uint64_t Tag) {
	void *Data;
	ml_tag_t Handler = Reader->TagFn(Tag, Reader->TagFnData, &Data);
	if (Handler) {
		tag_t *Tag = new(tag_t);
		Tag->Prev = Reader->Tags;
		Tag->Handler = Handler;
		Tag->Data = Data;
		Reader->Tags = Tag;
	}
}

void ml_cbor_read_float_fn(ml_cbor_reader_t *Reader, double Value) {
	value_handler(Reader, ml_real(Value));
}

void ml_cbor_read_simple_fn(ml_cbor_reader_t *Reader, int Value) {
	switch (Value) {
	case CBOR_SIMPLE_FALSE:
		value_handler(Reader, ml_method("false"));
		break;
	case CBOR_SIMPLE_TRUE:
		value_handler(Reader, ml_method("true"));
		break;
	case CBOR_SIMPLE_NULL:
		value_handler(Reader, MLNil);
		break;
	default:
		value_handler(Reader, MLNil);
		break;
	}
}

void ml_cbor_read_break_fn(ml_cbor_reader_t *Reader) {
	collection_t *Collection = Reader->Collection;
	Reader->Collection = Collection->Prev;
	Reader->Tags = Collection->Tags;
	value_handler(Reader, Collection->Collection);
}

void ml_cbor_read_error_fn(ml_cbor_reader_t *Reader, int Position, const char *Message) {
	value_handler(Reader, ml_error("CBORError", "Read error: %s at %d", Message, Position));
}

ml_value_t *ml_from_cbor(ml_cbor_t Cbor, void *TagFnData, ml_tag_t (*TagFn)(uint64_t, void *, void **)) {
	ml_cbor_reader_t Reader[1];
	Reader->TagFnData = TagFnData;
	Reader->TagFn = TagFn;
	ml_cbor_reader_init(Reader->Reader);
	Reader->Reader->UserData = Reader;
	Reader->Collection = 0;
	Reader->Tags = 0;
	Reader->Value = 0;
	ml_cbor_read(Reader->Reader, Cbor.Data, Cbor.Length);
	int Extra = ml_cbor_reader_extra(Reader);
	if (Extra) return ml_error("CBORError", "Extra bytes after decoding: %d", Extra);
	return ml_cbor_reader_get(Reader);
}

ml_cbor_result_t ml_from_cbor_extra(ml_cbor_t Cbor, void *TagFnData, ml_tag_t (*TagFn)(uint64_t, void *, void **)) {
	ml_cbor_reader_t Reader[1];
	Reader->TagFnData = TagFnData;
	Reader->TagFn = TagFn;
	ml_cbor_reader_init(Reader->Reader);
	Reader->Reader->UserData = Reader;
	Reader->Collection = 0;
	Reader->Tags = 0;
	Reader->Value = 0;
	ml_cbor_read(Reader->Reader, Cbor.Data, Cbor.Length);
	return (ml_cbor_result_t){ml_cbor_reader_get(Reader), ml_cbor_reader_extra(Reader)};
}

static ml_value_t *ml_to_cbor_fn(void *Data, int Count, ml_value_t **Args) {
	ml_cbor_t Cbor = ml_to_cbor(Args[0]);
	if (Cbor.Data) return ml_string(Cbor.Data, Cbor.Length);
	return ml_error("CborError", "Error encoding to cbor");
}

static ml_value_t *ml_value_fn(ml_value_t *Callback, ml_value_t *Value) {
	return ml_inline(Callback, 1, Value);
}

static ml_tag_t ml_value_tag_fn(uint64_t Tag, ml_value_t *Callback, void **Data) {
	Data[0] = ml_inline(Callback, 1, ml_integer(Tag));
	return (ml_tag_t)ml_value_fn;
}

static ml_value_t *ml_from_cbor_fn(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_cbor_t Cbor = {ml_string_value(Args[0]), ml_string_length(Args[0])};
	return ml_from_cbor(Cbor, Count > 1 ? Args[1] : MLNil, (void *)ml_value_tag_fn);
}

void ml_cbor_write_integer_fn(ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	//printf("%s()\n", __func__);
	int64_t Value = ml_integer_value(Arg);
	if (Value < 0) {
		ml_cbor_write_negative(Data, WriteFn, ~Value);
	} else {
		ml_cbor_write_positive(Data, WriteFn, Value);
	}
}

void ml_cbor_write_string_fn(ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	//printf("%s()\n", __func__);
	ml_cbor_write_string(Data, WriteFn, ml_string_length(Arg));
	WriteFn(Data, (const unsigned char *)ml_string_value(Arg), ml_string_length(Arg));
}

void ml_cbor_write_list_fn(ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_array(Data, WriteFn, ml_list_length(Arg));
	for (ml_list_node_t *Node = ml_list_head(Arg); Node; Node = Node->Next) {
		ml_cbor_write(Node->Value, Data, WriteFn);
	}
}

typedef struct ml_cbor_writer_t {
	void *Data;
	ml_cbor_write_fn WriteFn;
} ml_cbor_writer_t;

static int ml_cbor_write_map_pair(ml_value_t *Key, ml_value_t *Value, ml_cbor_writer_t *Writer) {
	ml_cbor_write(Key, Writer->Data, Writer->WriteFn);
	ml_cbor_write(Value, Writer->Data, Writer->WriteFn);
	return 0;
}

void ml_cbor_write_map_fn(ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_map(Data, WriteFn, ml_map_size(Arg));
	ml_cbor_writer_t Writer[1] = {{Data, WriteFn}};
	ml_map_foreach(Arg, Writer, (void *)ml_cbor_write_map_pair);
}

void ml_cbor_write_real_fn(ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_float8(Data, WriteFn, ml_real_value(Arg));
}

void ml_cbor_write_nil_fn(ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_simple(Data, WriteFn, CBOR_SIMPLE_NULL);
}

void ml_cbor_write_symbol_fn(ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	if (!strcmp(ml_method_name(Arg), "true")) {
		ml_cbor_write_simple(Data, WriteFn, CBOR_SIMPLE_TRUE);
	} else if (!strcmp(ml_method_name(Arg), "false")) {
		ml_cbor_write_simple(Data, WriteFn, CBOR_SIMPLE_FALSE);
	} else {
		ml_cbor_write_simple(Data, WriteFn, CBOR_SIMPLE_UNDEF);
	}
}

void ml_cbor_init(stringmap_t *Globals) {
	ml_typed_fn_set(MLIntegerT, ml_cbor_write, ml_cbor_write_integer_fn);
	ml_typed_fn_set(MLStringT, ml_cbor_write, ml_cbor_write_string_fn);
	ml_typed_fn_set(MLListT, ml_cbor_write, ml_cbor_write_list_fn);
	ml_typed_fn_set(MLMapT, ml_cbor_write, ml_cbor_write_map_fn);
	ml_typed_fn_set(MLRealT, ml_cbor_write, ml_cbor_write_real_fn);
	ml_typed_fn_set(MLNilT, ml_cbor_write, ml_cbor_write_nil_fn);
	ml_typed_fn_set(MLMethodT, ml_cbor_write, ml_cbor_write_symbol_fn);
	if (Globals) {
		ml_value_t *Cbor = ml_map();
		ml_map_insert(Cbor, ml_string("encode", -1), ml_function(NULL, ml_to_cbor_fn));
		ml_map_insert(Cbor, ml_string("decode", -1), ml_function(NULL, ml_from_cbor_fn));
		stringmap_insert(Globals, "cbor", Cbor);
	}
}

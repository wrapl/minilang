#include "minilang.h"
#include "ml_macros.h"
#include "ml_cbor.h"
#ifdef ML_CBOR_BYTECODE
#include "ml_bytecode.h"
#endif
#include <gc/gc.h>
#include <string.h>
#include "ml_object.h"

#include "minicbor/minicbor.h"

ml_value_t *ml_cbor_write(ml_value_t *Value, void *Data, ml_cbor_write_fn WriteFn) {
	typeof(ml_cbor_write) *function = ml_typed_fn_get(ml_typeof(Value), ml_cbor_write);
	if (function) {
		return function(Value, Data, WriteFn);
	} else {
		return ml_error("CBORError", "No method to encode %s to CBOR", ml_typeof(Value)->Name);
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
	ml_value_t *Error = ml_cbor_write(Value, &Size, (void *)ml_cbor_size_fn);
	if (Error) return (ml_cbor_t){{.Error = Error}, 0};
	unsigned char *Bytes = GC_MALLOC_ATOMIC(Size), *End = Bytes;
	ml_cbor_write(Value, &End, (void *)ml_cbor_bytes_fn);
	return (ml_cbor_t){{.Data = Bytes}, Size};
}

typedef struct block_t {
	struct block_t *Prev;
	int Size;
	char Data[];
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
		if (!ml_is_error(Value)) Value = Tag->Handler(Tag->Data, Value);
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
	if (Value <= 0x7FFFFFFFFFFFFFFFL) {
		value_handler(Reader, ml_integer(~(int64_t)Value));
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
		value_handler(Reader, ml_buffer(NULL, 0));
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
		value_handler(Reader, ml_buffer(Buffer, Total));
	} else {
		block_t *Block = xnew(block_t, Size, char);
		Block->Prev = Collection->Blocks;
		Block->Size = Size;
		memcpy(Block->Data, Bytes, Size);
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
		value_handler(Reader, ml_cstring(""));
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
		block_t *Block = xnew(block_t, Size, char);
		Block->Prev = Collection->Blocks;
		Block->Size = Size;
		memcpy(Block->Data, Bytes, Size);
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
		value_handler(Reader, (ml_value_t *)MLFalse);
		break;
	case CBOR_SIMPLE_TRUE:
		value_handler(Reader, (ml_value_t *)MLTrue);
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

static ml_value_t *ml_value_fn(ml_value_t *Callback, ml_value_t *Value) {
	return ml_simple_inline(Callback, 1, Value);
}

static ml_tag_t ml_value_tag_fn(uint64_t Tag, ml_value_t *Callback, void **Data) {
	Data[0] = ml_simple_inline(Callback, 1, ml_integer(Tag));
	return (ml_tag_t)ml_value_fn;
}

ml_value_t *CborDefaultTags = NULL;

void ml_cbor_default_tag(long Tag, void *Data, ml_callback_t Callback) {
	if (!CborDefaultTags) CborDefaultTags = ml_map();
	ml_map_insert(CborDefaultTags, ml_integer(Tag), ml_cfunction(Data, Callback));
}

ML_FUNCTION(DefaultTagFn) {
//!internal
	return ml_map_search(CborDefaultTags, Args[0]);
}

ml_value_t *ml_from_cbor(ml_cbor_t Cbor, void *TagFnData, ml_tag_t (*TagFn)(uint64_t, void *, void **)) {
	ml_cbor_reader_t Reader[1];
	Reader->TagFnData = TagFnData ?: DefaultTagFn;
	Reader->TagFn = TagFn ?: (void *)ml_value_tag_fn;
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
	Reader->TagFnData = TagFnData ?: DefaultTagFn;
	Reader->TagFn = TagFn ?: (void *)ml_value_tag_fn;
	ml_cbor_reader_init(Reader->Reader);
	Reader->Reader->UserData = Reader;
	Reader->Collection = 0;
	Reader->Tags = 0;
	Reader->Value = 0;
	ml_cbor_read(Reader->Reader, Cbor.Data, Cbor.Length);
	return (ml_cbor_result_t){ml_cbor_reader_get(Reader), ml_cbor_reader_extra(Reader)};
}

ML_FUNCTION(MLEncode) {
//@cbor::encode
//<Value
//>string | error
	ML_CHECK_ARG_COUNT(1);
	ml_cbor_t Cbor = ml_to_cbor(Args[0]);
	if (!Cbor.Length) return Cbor.Error;
	if (Cbor.Data) return ml_string(Cbor.Data, Cbor.Length);
	return ml_error("CborError", "Error encoding to cbor");
}

ML_FUNCTION(MLDecode) {
//@cbor::decode
//<Bytes
//>any | error
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLAddressT);
	ml_cbor_t Cbor = {{.Data = ml_address_value(Args[0])}, ml_address_length(Args[0])};
	return ml_from_cbor(Cbor, Count > 1 ? Args[1] : (ml_value_t *)DefaultTagFn, (void *)ml_value_tag_fn);
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLIntegerT, ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	//printf("%s()\n", __func__);
	int64_t Value = ml_integer_value_fast(Arg);
	if (Value < 0) {
		ml_cbor_write_negative(Data, WriteFn, ~Value);
	} else {
		ml_cbor_write_positive(Data, WriteFn, Value);
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLAddressT, ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	//printf("%s()\n", __func__);
	ml_cbor_write_bytes(Data, WriteFn, ml_address_length(Arg));
	WriteFn(Data, (const unsigned char *)ml_address_value(Arg), ml_address_length(Arg));
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLStringT, ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	//printf("%s()\n", __func__);
	ml_cbor_write_string(Data, WriteFn, ml_string_length(Arg));
	WriteFn(Data, (const unsigned char *)ml_string_value(Arg), ml_string_length(Arg));
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLRegexT, ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	//printf("%s()\n", __func__);
	const char *Pattern = ml_regex_pattern(Arg);
	ml_cbor_write_tag(Data, WriteFn, 35);
	ml_cbor_write_string(Data, WriteFn, strlen(Pattern));
	WriteFn(Data, (void *)Pattern, strlen(Pattern));
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLTupleT, ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	int Size = ml_tuple_size(Arg);
	ml_cbor_write_array(Data, WriteFn, Size);
	for (int I = 1; I <= Size; ++I) {
		ml_value_t *Error = ml_cbor_write(ml_tuple_get(Arg, I), Data, WriteFn);
		if (Error) return Error;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLListT, ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_array(Data, WriteFn, ml_list_length(Arg));
	ML_LIST_FOREACH(Arg, Node) {
		ml_value_t *Error = ml_cbor_write(Node->Value, Data, WriteFn);
		if (Error) return Error;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLMapT, ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_map(Data, WriteFn, ml_map_size(Arg));
	ML_MAP_FOREACH(Arg, Node) {
		ml_value_t *Error = ml_cbor_write(Node->Key, Data, WriteFn);
		if (Error) return Error;
		Error = ml_cbor_write(Node->Value, Data, WriteFn);
		if (Error) return Error;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLDoubleT, ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_float8(Data, WriteFn, ml_double_value_fast(Arg));
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLNilT, ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_simple(Data, WriteFn, CBOR_SIMPLE_NULL);
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLBooleanT, ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_simple(Data, WriteFn, ml_boolean_value(Arg) ? CBOR_SIMPLE_TRUE : CBOR_SIMPLE_FALSE);
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLMethodT, ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	const char *Name = ml_method_name(Arg);
	ml_cbor_write_tag(Data, WriteFn, 39);
	ml_cbor_write_string(Data, WriteFn, strlen(Name));
	WriteFn(Data, (void *)Name, strlen(Name));
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLObjectT, ml_value_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_tag(Data, WriteFn, 27);
	int Size = ml_object_size(Arg);
	ml_cbor_write_array(Data, WriteFn, 1 + Size);
	const char *Name = ml_typeof(Arg)->Name;
	ml_cbor_write_string(Data, WriteFn, strlen(Name));
	WriteFn(Data, (void *)Name, strlen(Name));
	for (int I = 0; I < Size; ++I) {
		ml_value_t *Error = ml_cbor_write(ml_object_field(Arg, I), Data, WriteFn);
		if (Error) return Error;
	}
	return NULL;
}

#ifdef ML_TABLES
#include "ml_table.h"



#endif

ml_value_t *ml_cbor_read_regex(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_TYPE(0, MLStringT);
	return ml_regex(ml_string_value(Args[0]), ml_string_length(Args[1]));
}

ml_value_t *ml_cbor_read_method(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_TYPE(0, MLStringT);
	return ml_method(ml_string_value(Args[0]));
}

ml_value_t *CborObjects = NULL;

ml_value_t *ml_cbor_read_object(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLListT);
	ml_list_iter_t Iter[1];
	ml_list_iter_forward(Args[0], Iter);
	if (!ml_list_iter_valid(Iter)) return ml_error("CBORError", "Object tag requires type name");
	ml_value_t *TypeName = Iter->Value;
	if (ml_typeof(TypeName) != MLStringT) return ml_error("CBORError", "Object tag requires type name");
	ml_value_t *Constructor = ml_map_search(CborObjects, TypeName);
	if (Constructor == MLNil) return ml_error("CBORError", "Object %s not found", ml_string_value(TypeName));
	int Count2 = ml_list_length(Args[0]) - 1;
	ml_value_t **Args2 = ml_alloc_args(Count2);
	for (int I = 0; I < Count2; ++I) {
		ml_list_iter_next(Iter);
		Args2[I] = Iter->Value;
	}
	return ml_simple_call(Constructor, Count2, Args2);
}

void ml_cbor_init(stringmap_t *Globals) {
	if (!CborDefaultTags) CborDefaultTags = ml_map();
	if (!CborObjects) CborObjects = ml_map();
	ml_cbor_default_tag(35, NULL, ml_cbor_read_regex);
	ml_cbor_default_tag(39, NULL, ml_cbor_read_method);
	ml_cbor_default_tag(27, NULL, ml_cbor_read_object);
#ifdef ML_CBOR_BYTECODE
	ml_cbor_default_tag(36, NULL, ml_cbor_read_closure);
#endif
#include "ml_cbor_init.c"
	if (Globals) {
		stringmap_insert(Globals, "cbor", ml_module("cbor",
			"encode", MLEncode,
			"decode", MLDecode,
			"Default", CborDefaultTags,
			"Objects", CborObjects,
		NULL));
	}
}

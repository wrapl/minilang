#include "minilang.h"
#include "ml_macros.h"
#include "ml_cbor.h"
#include "ml_stream.h"
#ifdef ML_CBOR_BYTECODE
#include "ml_bytecode.h"
#endif
#include <gc/gc.h>
#include <string.h>
#include "ml_object.h"
#include "ml_bytecode.h"
#include "minicbor/minicbor.h"

#undef ML_CATEGORY
#define ML_CATEGORY "cbor"

struct ml_cbor_tag_fns_t {
	uint64_t *Tags;
	ml_cbor_tag_fn *Fns;
	int Count, Space;
};

ml_cbor_tag_fn ml_cbor_tag_fn_get(ml_cbor_tag_fns_t *TagFns, uint64_t Tag) {
	uint64_t *Tags = TagFns->Tags;
	int Lo = 0, Hi = TagFns->Count - 1;
	while (Lo <= Hi) {
		int Mid = (Lo + Hi) / 2;
		if (Tag < Tags[Mid]) {
			Hi = Mid - 1;
		} else if (Tag > Tags[Mid]) {
			Lo = Mid + 1;
		} else {
			return TagFns->Fns[Mid];
		}
	}
	return NULL;
}

void ml_cbor_tag_fn_set(ml_cbor_tag_fns_t *TagFns, uint64_t Tag, ml_cbor_tag_fn Fn) {
	uint64_t *Tags = TagFns->Tags;
	int Lo = 0, Hi = TagFns->Count - 1;
	while (Lo <= Hi) {
		int Mid = (Lo + Hi) / 2;
		if (Tag < Tags[Mid]) {
			Hi = Mid - 1;
		} else if (Tag > Tags[Mid]) {
			Lo = Mid + 1;
		} else {
			TagFns->Fns[Mid] = Fn;
			return;
		}
	}
	ml_cbor_tag_fn *Fns = TagFns->Fns;
	int Move = TagFns->Count - Lo;
	if (--TagFns->Space >= 0) {
		memmove(Tags + Lo + 1, Tags + Lo, Move * sizeof(uint64_t));
		Tags[Lo] = Tag;
		memmove(Fns + Lo + 1, Fns + Lo, Move * sizeof(ml_cbor_tag_fn));
		Fns[Lo] = Fn;
	} else {
		uint64_t *Tags2 = TagFns->Tags = anew(uint64_t, TagFns->Count + 8);
		memcpy(Tags2, Tags, Lo * sizeof(uint64_t));
		memcpy(Tags2 + Lo + 1, Tags + Lo, Move * sizeof(uint64_t));
		Tags2[Lo] = Tag;
		ml_cbor_tag_fn *Fns2 = TagFns->Fns = anew(ml_cbor_tag_fn, TagFns->Count + 8);
		memcpy(Fns2, Fns, Lo * sizeof(ml_cbor_tag_fn));
		memcpy(Fns2 + Lo + 1, Fns + Lo, Move * sizeof(ml_cbor_tag_fn));
		Fns2[Lo] = Fn;
		TagFns->Space += 8;
	}
	++TagFns->Count;
}

ml_cbor_tag_fns_t *ml_cbor_tag_fns_copy(ml_cbor_tag_fns_t *TagFns) {
	ml_cbor_tag_fns_t *Copy = new(ml_cbor_tag_fns_t);
	int Count = Copy->Count = TagFns->Count;
	int Space = Copy->Space = TagFns->Space;
	int Size = Count + Space;
	uint64_t *Tags = Copy->Tags = anew(uint64_t, Size);
	memcpy(Tags, TagFns->Tags, Count * sizeof(uint64_t));
	ml_cbor_tag_fn *Fns = Copy->Fns = anew(ml_cbor_tag_fn, Size);
	memcpy(Fns, TagFns->Fns, Count * sizeof(ml_cbor_tag_fn));
	return Copy;
}

static ml_cbor_tag_fns_t DefaultTagFns[1] = {{NULL, NULL, 0, 0}};

void ml_cbor_default_tag(uint64_t Tag, ml_cbor_tag_fn TagFn) {
	ml_cbor_tag_fn_set(DefaultTagFns, Tag, TagFn);
}

ml_cbor_tag_fns_t *ml_cbor_tag_fns(int Default) {
	if (Default) {
		return ml_cbor_tag_fns_copy(DefaultTagFns);
	} else {
		return new(ml_cbor_tag_fns_t);
	}
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
	ml_cbor_tag_fn Handler;
	int Index;
} tag_t;

struct ml_cbor_reader_t {
	collection_t *Collection;
	tag_t *Tags;
	ml_value_t *Value;
	ml_cbor_tag_fns_t *TagFns;
	ml_getter_t GlobalGet;
	void *Globals;
	ml_value_t **Reused;
	minicbor_reader_t Reader[1];
	int NumReused, MaxReused;
	int NumSettings;
	void *Settings[];
};

static int NumCborSettings = 0;

int ml_cbor_setting() {
	return NumCborSettings++;
}

ml_cbor_reader_t *ml_cbor_reader(ml_cbor_tag_fns_t *TagFns, ml_getter_t GlobalGet, void *Globals) {
	ml_cbor_reader_t *Reader = xnew(ml_cbor_reader_t, NumCborSettings, void *);
	Reader->TagFns = TagFns ?: DefaultTagFns;
	if (GlobalGet) {
		Reader->GlobalGet = GlobalGet;
		Reader->Globals = Globals;
	} else {
		Reader->GlobalGet = (ml_getter_t)stringmap_search;
		Reader->Globals = MLExternals.Names;
	}
	Reader->NumSettings = NumCborSettings;
	Reader->NumReused = Reader->MaxReused = 0;
	minicbor_reader_init(Reader->Reader);
	Reader->Reader->UserData = Reader;
	return Reader;
}

void ml_cbor_reader_reset(ml_cbor_reader_t *Reader) {
	Reader->Collection = NULL;
	Reader->Tags = NULL;
	Reader->Value = NULL;
	Reader->NumReused = Reader->MaxReused = 0;
	minicbor_reader_init(Reader->Reader);
}

void ml_cbor_reader_set_setting(ml_cbor_reader_t *Reader, int Setting, void *Value) {
	if (Setting < Reader->NumSettings) Reader->Settings[Setting] = Value;
}

void *ml_cbor_reader_get_setting(ml_cbor_reader_t *Reader, int Setting) {
	return Setting < Reader->NumSettings ? Reader->Settings[Setting] : NULL;
}

static int ml_cbor_reader_next_index(ml_cbor_reader_t *Reader) {
	int Index = Reader->NumReused++;
	if (Index == Reader->MaxReused) {
		Reader->MaxReused = Reader->MaxReused + 8;
		Reader->Reused = GC_realloc(Reader->Reused, Reader->MaxReused * sizeof(ml_value_t *));
	}
	return Index;
}

int ml_cbor_reader_read(ml_cbor_reader_t *Reader, unsigned char *Bytes, int Size) {
	return minicbor_read(Reader->Reader, Bytes, Size);
}

ml_value_t *ml_cbor_reader_get(ml_cbor_reader_t *Reader) {
	if (!Reader->Value) return ml_error("CBORError", "CBOR not completely read");
	return Reader->Value;
}

int ml_cbor_reader_extra(ml_cbor_reader_t *Reader) {
	return minicbor_reader_remaining(Reader->Reader);
}

static ml_value_t IsByteString[1];
static ml_value_t IsString[1];
static ml_value_t IsList[1];

ml_value_t *ml_cbor_mark_reused(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	return ml_error("CBORError", "Mark reused should not be called");
}

ml_value_t *ml_cbor_use_previous(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (ml_is(Value, MLIntegerT)) {
		int Index = ml_integer_value(Value);
		if (Index < 0 || Index >= Reader->NumReused) {
			return ml_error("CBORError", "Invalid previous index");
		} else {
			Value = Reader->Reused[Index];
			if (!Value) Value = Reader->Reused[Index] = ml_uninitialized("CBOR");
			return Value;
		}
	} else if (ml_is(Value, MLStringT)) {
		const char *Name = ml_string_value(Value);
		Value = Reader->GlobalGet(Reader->Globals, Name);
		if (!Value) return ml_error("CBORError", "Unknown global %s", Name);
		return Value;
	} else {
		return ml_error("CBORError", "Invalid previous index");
	}
}

static void value_handler(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		Reader->Value = Value;
		return minicbor_reader_finish(Reader->Reader);
	}
	for (tag_t *Tag = Reader->Tags; Tag; Tag = Tag->Prev) {
		if (Tag->Handler == ml_cbor_mark_reused) {
			ml_value_t *Uninitialized = Reader->Reused[Tag->Index];
			if (Uninitialized) ml_uninitialized_set(Uninitialized, Value);
			Reader->Reused[Tag->Index] = Value;
		} else {
			Value = Tag->Handler(Reader, Value);
			if (ml_is_error(Value)) {
				Reader->Value = Value;
				return minicbor_reader_finish(Reader->Reader);
			}
		}
	}
	Reader->Tags = 0;
	collection_t *Collection = Reader->Collection;
	if (!Collection) {
		Reader->Value = Value;
		minicbor_reader_finish(Reader->Reader);
	} else if (Collection->Key == IsList) {
		ml_list_put(Collection->Collection, Value);
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
			Collection->Key = NULL;
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
		value_handler(Reader, ml_address(NULL, 0));
	}
}

void ml_cbor_read_bytes_piece_fn(ml_cbor_reader_t *Reader, const void *Bytes, int Size, int Final) {
	collection_t *Collection = Reader->Collection;
	if (Final) {
		Reader->Collection = Collection->Prev;
		Reader->Tags = Collection->Tags;
		int Total = Collection->Remaining + Size;
		char *Buffer = snew(Total);
		Buffer += Collection->Remaining;
		memcpy(Buffer, Bytes, Size);
		for (block_t *B = Collection->Blocks; B; B = B->Prev) {
			Buffer -= B->Size;
			memcpy(Buffer, B->Data, B->Size);
		}
		value_handler(Reader, ml_address(Buffer, Total));
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
		char *Buffer = snew(Total);
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

void ml_cbor_read_tag_fn(ml_cbor_reader_t *Reader, uint64_t Value) {
	ml_cbor_tag_fn Handler = ml_cbor_tag_fn_get(Reader->TagFns, Value);
	if (Handler) {
		tag_t *Tag = new(tag_t);
		Tag->Prev = Reader->Tags;
		Tag->Handler = Handler;
		// TODO: Reimplement this without hard-coding tag 28
		if (Value == 28) Tag->Index = ml_cbor_reader_next_index(Reader);
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

ml_value_t *ml_from_cbor(ml_cbor_t Cbor, ml_cbor_tag_fns_t *TagFns) {
	ml_cbor_reader_t Reader[1];
	Reader->TagFns = TagFns ?: DefaultTagFns;
	Reader->GlobalGet = (ml_getter_t)stringmap_search;
	Reader->Globals = MLExternals.Names;
	Reader->Reused = NULL;
	Reader->NumReused = Reader->MaxReused = 0;
	Reader->NumSettings = 0;
	minicbor_reader_init(Reader->Reader);
	Reader->Reader->UserData = Reader;
	Reader->Collection = 0;
	Reader->Tags = 0;
	Reader->Value = 0;
	minicbor_read(Reader->Reader, Cbor.Data, Cbor.Length);
	int Extra = ml_cbor_reader_extra(Reader);
	if (Extra) return ml_error("CBORError", "Extra bytes after decoding: %d", Extra);
	return ml_cbor_reader_get(Reader);
}

ml_cbor_result_t ml_from_cbor_extra(ml_cbor_t Cbor, ml_cbor_tag_fns_t *TagFns) {
	ml_cbor_reader_t Reader[1];
	Reader->TagFns = TagFns ?: DefaultTagFns;
	Reader->GlobalGet = (ml_getter_t)stringmap_search;
	Reader->Globals = MLExternals.Names;
	Reader->Reused = NULL;
	Reader->NumReused = Reader->MaxReused = 0;
	Reader->NumSettings = 0;
	minicbor_reader_init(Reader->Reader);
	Reader->Reader->UserData = Reader;
	Reader->Collection = 0;
	Reader->Tags = 0;
	Reader->Value = 0;
	minicbor_read(Reader->Reader, Cbor.Data, Cbor.Length);
	return (ml_cbor_result_t){ml_cbor_reader_get(Reader), ml_cbor_reader_extra(Reader)};
}

ML_METHOD_ANON(CborDecode, "cbor::decode");

ML_METHOD(CborDecode, MLAddressT) {
//@cbor::decode
//<Bytes
//>any|error
// Decode :mini:`Bytes` into a Minilang value, or return an error if :mini:`Bytes` contains invalid CBOR or cannot be decoded into a Minilang value.
	ml_cbor_reader_t Reader[1];
	Reader->TagFns = DefaultTagFns;
	Reader->GlobalGet = (ml_getter_t)stringmap_search;
	Reader->Globals = MLExternals.Names;
	Reader->Reused = NULL;
	Reader->NumReused = Reader->MaxReused = 0;
	Reader->NumSettings = 0;
	minicbor_reader_init(Reader->Reader);
	Reader->Reader->UserData = Reader;
	Reader->Collection = 0;
	Reader->Tags = 0;
	Reader->Value = 0;
	minicbor_read(Reader->Reader, (const unsigned char *)ml_address_value(Args[0]), ml_address_length(Args[0]));
	int Extra = ml_cbor_reader_extra(Reader);
	if (Extra) return ml_error("CBORError", "Extra bytes after decoding: %d", Extra);
	return ml_cbor_reader_get(Reader);
}

static ml_value_t *ml_cbor_global_get_map(ml_value_t *Map, const char *Name) {
	ml_value_t *Value = ml_map_search(Map, ml_string(Name, -1));
	if (Value) return Value;
	return stringmap_search(MLExternals.Names, Name);
}

ML_METHOD(CborDecode, MLAddressT, MLMapT) {
//@cbor::decode
//<Bytes
//<Globals
//>any|error
// Decode :mini:`Bytes` into a Minilang value, or return an error if :mini:`Bytes` contains invalid CBOR or cannot be decoded into a Minilang value.
	ml_cbor_reader_t Reader[1];
	Reader->TagFns = DefaultTagFns;
	Reader->GlobalGet = (ml_getter_t)ml_cbor_global_get_map;
	Reader->Globals = Args[1];
	Reader->Reused = NULL;
	Reader->NumReused = Reader->MaxReused = 0;
	Reader->NumSettings = 0;
	minicbor_reader_init(Reader->Reader);
	Reader->Reader->UserData = Reader;
	Reader->Collection = 0;
	Reader->Tags = 0;
	Reader->Value = 0;
	minicbor_read(Reader->Reader, (const unsigned char *)ml_address_value(Args[0]), ml_address_length(Args[0]));
	int Extra = ml_cbor_reader_extra(Reader);
	if (Extra) return ml_error("CBORError", "Extra bytes after decoding: %d", Extra);
	return ml_cbor_reader_get(Reader);
}

static ml_value_t *ml_cbor_global_get_fn(ml_value_t *Fn, const char *Name) {
	ml_value_t **Args = ml_alloc_args(1);
	Args[0] = ml_string(Name, -1);
	return ml_simple_call(Fn, 1, Args);
}

ML_METHOD(CborDecode, MLAddressT, MLFunctionT) {
//@cbor::decode
//<Bytes
//<Globals
//>any|error
// Decode :mini:`Bytes` into a Minilang value, or return an error if :mini:`Bytes` contains invalid CBOR or cannot be decoded into a Minilang value.
	ml_cbor_reader_t Reader[1];
	Reader->TagFns = DefaultTagFns;
	Reader->GlobalGet = (ml_getter_t)ml_cbor_global_get_fn;
	Reader->Globals = Args[1];
	Reader->Reused = NULL;
	Reader->NumReused = Reader->MaxReused = 0;
	Reader->NumSettings = 0;
	minicbor_reader_init(Reader->Reader);
	Reader->Reader->UserData = Reader;
	Reader->Collection = 0;
	Reader->Tags = 0;
	Reader->Value = 0;
	minicbor_read(Reader->Reader, (const unsigned char *)ml_address_value(Args[0]), ml_address_length(Args[0]));
	int Extra = ml_cbor_reader_extra(Reader);
	if (Extra) return ml_error("CBORError", "Extra bytes after decoding: %d", Extra);
	return ml_cbor_reader_get(Reader);
}

struct ml_cbor_writer_t {
	void *Data;
	ml_cbor_write_fn WriteFn;
	inthash_t *Globals;
	inthash_t References[1];
	inthash_t Reused[1];
	int Index, NumSettings;
	void *Settings[];
};

void ml_cbor_write_integer(ml_cbor_writer_t *Writer, int64_t Number) {
	minicbor_write_integer(Writer->Data, Writer->WriteFn, Number);
}

void ml_cbor_write_positive(ml_cbor_writer_t *Writer, uint64_t Number) {
	minicbor_write_positive(Writer->Data, Writer->WriteFn, Number);
}

void ml_cbor_write_negative(ml_cbor_writer_t *Writer, uint64_t Number) {
	minicbor_write_negative(Writer->Data, Writer->WriteFn, Number);
}

void ml_cbor_write_bytes(ml_cbor_writer_t *Writer, unsigned Size) {
	minicbor_write_bytes(Writer->Data, Writer->WriteFn, Size);
}

void ml_cbor_write_indef_bytes(ml_cbor_writer_t *Writer) {
	minicbor_write_indef_bytes(Writer->Data, Writer->WriteFn);
}

void ml_cbor_write_string(ml_cbor_writer_t *Writer, unsigned Size) {
	minicbor_write_string(Writer->Data, Writer->WriteFn, Size);
}

void ml_cbor_write_indef_string(ml_cbor_writer_t *Writer) {
	minicbor_write_indef_string(Writer->Data, Writer->WriteFn);
}

void ml_cbor_write_array(ml_cbor_writer_t *Writer, unsigned Size) {
	minicbor_write_array(Writer->Data, Writer->WriteFn, Size);
}

void ml_cbor_write_indef_array(ml_cbor_writer_t *Writer) {
	minicbor_write_indef_array(Writer->Data, Writer->WriteFn);
}

void ml_cbor_write_map(ml_cbor_writer_t *Writer, unsigned Size) {
	minicbor_write_map(Writer->Data, Writer->WriteFn, Size);
}

void ml_cbor_write_indef_map(ml_cbor_writer_t *Writer) {
	minicbor_write_indef_map(Writer->Data, Writer->WriteFn);
}

void ml_cbor_write_float2(ml_cbor_writer_t *Writer, double Number) {
	minicbor_write_float2(Writer->Data, Writer->WriteFn, Number);
}

void ml_cbor_write_float4(ml_cbor_writer_t *Writer, double Number) {
	minicbor_write_float4(Writer->Data, Writer->WriteFn, Number);
}

void ml_cbor_write_float8(ml_cbor_writer_t *Writer, double Number) {
	minicbor_write_float8(Writer->Data, Writer->WriteFn, Number);
}

void ml_cbor_write_simple(ml_cbor_writer_t *Writer, unsigned char Simple) {
	minicbor_write_simple(Writer->Data, Writer->WriteFn, Simple);
}

void ml_cbor_write_break(ml_cbor_writer_t *Writer) {
	minicbor_write_break(Writer->Data, Writer->WriteFn);
}

void ml_cbor_write_tag(ml_cbor_writer_t *Writer, uint64_t Tag) {
	minicbor_write_tag(Writer->Data, Writer->WriteFn, Tag);
}

void ml_cbor_write_raw(ml_cbor_writer_t *Writer, const unsigned char *Bytes, size_t Length) {
	Writer->WriteFn(Writer->Data, Bytes, Length);
}

ml_cbor_writer_t *ml_cbor_writer(void *Data, ml_cbor_write_fn WriteFn, inthash_t *Globals) {
	ml_cbor_writer_t *Writer = xnew(ml_cbor_writer_t, NumCborSettings, void *);
	Writer->Data = Data;
	Writer->WriteFn = WriteFn;
	Writer->Globals = Globals ?: MLExternals.Values;
	Writer->References[0] = INTHASH_INIT;
	Writer->Reused[0] = INTHASH_INIT;
	Writer->Index = 0;
	Writer->NumSettings = NumCborSettings;
	return Writer;
}

void ml_cbor_writer_reset(ml_cbor_writer_t *Writer, void *Data) {
	Writer->Data = Data;
	Writer->References[0] = INTHASH_INIT;
	Writer->Reused[0] = INTHASH_INIT;
	Writer->Index = 0;
}

void ml_cbor_writer_set_setting(ml_cbor_writer_t *Writer, int Setting, void *Value) {
	if (Setting < Writer->NumSettings) Writer->Settings[Setting] = Value;
}

void *ml_cbor_writer_get_setting(ml_cbor_writer_t *Writer, int Setting) {
	return Setting < Writer->NumSettings ? Writer->Settings[Setting] : NULL;
}

static int ml_cbor_writer_ref_fn(ml_cbor_writer_t *Writer, ml_value_t *Value) {
	if (inthash_insert(Writer->References, (uintptr_t)Value, Value)) {
		inthash_insert(Writer->Reused, (uintptr_t)Value, (void *)0L);
		return 0;
	}
	return 1;
}

void ml_cbor_writer_find_refs(ml_cbor_writer_t *Writer, ml_value_t *Value) {
	ml_value_find_refs(Value, Writer, (ml_value_ref_fn)ml_cbor_writer_ref_fn);
}

ml_value_t *ml_cbor_write(ml_cbor_writer_t *Writer, ml_value_t *Value) {
	inthash_result_t Result = inthash_search2(Writer->Reused, (uintptr_t)Value);
	if (Result.Present) {
		if (Result.Value) {
			int Index = (uintptr_t)Result.Value - 1;
			minicbor_write_tag(Writer->Data, Writer->WriteFn, 29);
			minicbor_write_integer(Writer->Data, Writer->WriteFn, Index);
			return NULL;
		}
		int Index = ++Writer->Index;
		inthash_insert(Writer->Reused, (uintptr_t)Value, (void *)(uintptr_t)Index);
		minicbor_write_tag(Writer->Data, Writer->WriteFn, 28);
	}
	typeof(ml_cbor_write) *function = ml_typed_fn_get(ml_typeof(Value), ml_cbor_write);
	if (function) return function(Writer, Value);
	const char *Name = (const char *)inthash_search(Writer->Globals, (uintptr_t)Value);
	if (Name) {
		minicbor_write_tag(Writer->Data, Writer->WriteFn, 29);
		size_t Length = strlen(Name);
		minicbor_write_string(Writer->Data, Writer->WriteFn, Length);
		Writer->WriteFn(Writer->Data, (const unsigned char *)Name, Length);
		return NULL;
	}
	return ml_error("CBORError", "No method to encode %s to CBOR", ml_typeof(Value)->Name);
}

ml_cbor_t ml_cbor_writer_encode(ml_value_t *Value) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_cbor_writer_t Writer[1];
	Writer->Data = Buffer;
	Writer->WriteFn = (void *)ml_stringbuffer_write;
	Writer->Globals = MLExternals.Values;
	Writer->References[0] = INTHASH_INIT;
	Writer->Reused[0] = INTHASH_INIT;
	Writer->Index = 0;
	Writer->NumSettings = 0;
	ml_cbor_writer_find_refs(Writer, Value);
	ml_value_t *Error = ml_cbor_write(Writer, Value);
	if (Error) return (ml_cbor_t){{.Error = Error}, 0};
	size_t Size = Buffer->Length;
	if (!Size) return (ml_cbor_t){{.Error = ml_error("CBORError", "Empty CBOR encoding")}, 0};
	return (ml_cbor_t){{.Data = ml_stringbuffer_get_string(Buffer)}, Size};
}

ml_cbor_t ml_to_cbor(ml_value_t *Value) {
	return ml_cbor_writer_encode(Value);
}

static void vlq64_encode(ml_stringbuffer_t *Buffer, int64_t Value) {
	unsigned char Bytes[9];
	uint64_t X = (uint64_t)Value;
	unsigned char Sign = 0;
	if (Value < 0) {
		X = ~X;
		Sign = 128;
	}
	uint64_t Y = X & 63;
	X >>= 6;
	if (!X) {
		Bytes[0] = Sign + Y;
		ml_stringbuffer_write(Buffer, (const char *)Bytes, 1);
	} else {
		Bytes[0] = Sign + 64 + Y;
		unsigned char *Ptr = Bytes + 1;
		for (int I = 1; I < 8; ++I) {
			Y = X & 127;
			X >>= 6;
			if (!X) {
				*Ptr = Y;
				ml_stringbuffer_write(Buffer, (const char *)Bytes, (Ptr - Bytes) + 1);
				return;
			}
			*Ptr++ = 128 + Y;
		}
		*Ptr = (unsigned char)X;
		ml_stringbuffer_write(Buffer, (const char *)Bytes, (Ptr - Bytes) + 1);
	}
}

static void vlq64_encode_string(ml_stringbuffer_t *Buffer, const char *Value) {
	int Length = strlen(Value);
	vlq64_encode(Buffer, Length);
	ml_stringbuffer_write(Buffer, Value, Length);
}

static int ml_closure_info_param_fn(const char *Name, void *Index, const char *Params[]) {
	Params[(intptr_t)Index - 1] = Name;
	return 0;
}

static int ml_closure_find_decl(ml_stringbuffer_t *Buffer, inthash_t *Decls, ml_decl_t *Decl) {
	if (!Decl) return -1;
	inthash_result_t Result = inthash_search2(Decls, (uintptr_t)Decl);
	if (Result.Present) return (uintptr_t)Result.Value;
	int Next = ml_closure_find_decl(Buffer, Decls, Decl->Next);
	int Index = Decls->Size - Decls->Space;
	vlq64_encode_string(Buffer, Decl->Ident);
	vlq64_encode(Buffer, Next);
	vlq64_encode(Buffer, Decl->Source.Line);
	vlq64_encode(Buffer, Decl->Index);
	vlq64_encode(Buffer, Decl->Flags);
	inthash_insert(Decls, (uintptr_t)Decl, (void *)(uintptr_t)Index);
	return Index;
}

static int ml_stringbuffer_copy(ml_stringbuffer_t *Buffer, const char *String, size_t Length) {
	ml_stringbuffer_write(Buffer, String, Length);
	return 0;
}

static int ml_stringbuffer_to_cbor(ml_cbor_writer_t *Writer, const char *String, size_t Length) {
	Writer->WriteFn(Writer->Data, (unsigned char *)String, Length);
	return 0;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLClosureInfoT, ml_cbor_writer_t *Writer, ml_closure_info_t *Info) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	vlq64_encode(Buffer, ML_BYTECODE_VERSION);
	vlq64_encode_string(Buffer, Info->Name ?: "");
	vlq64_encode_string(Buffer, Info->Source ?: "");
	vlq64_encode(Buffer, Info->StartLine);
	vlq64_encode(Buffer, Info->FrameSize);
	vlq64_encode(Buffer, Info->NumParams);
	vlq64_encode(Buffer, Info->NumUpValues);
	vlq64_encode(Buffer, Info->Flags & (ML_CLOSURE_EXTRA_ARGS | ML_CLOSURE_NAMED_ARGS));
	const char *Params[Info->NumParams];
	stringmap_foreach(Info->Params, Params, (void *)ml_closure_info_param_fn);
	for (int I = 0; I < Info->NumParams; ++I) vlq64_encode_string(Buffer, Params[I]);
	inthash_t Labels[1] = {INTHASH_INIT};
	uintptr_t BaseOffset = 0, Return = 0;
	ml_inst_t *Base = Info->Entry;
	ml_closure_info_labels(Info);
	inthash_t Decls[1] = {INTHASH_INIT};
	ml_stringbuffer_t DeclBuffer[1] = {ML_STRINGBUFFER_INIT};
	int DeclsIndex = ml_closure_find_decl(DeclBuffer, Decls, Info->Decls);
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Label) inthash_insert(Labels, Inst->Label, (void *)((Inst - Base) + BaseOffset));
		if (Inst->Opcode == MLI_LINK) {
			BaseOffset += Inst - Base;
			Base = Inst = Inst[1].Inst;
			continue;
		}
		if (Inst == Info->Return) Return = (Inst - Base) + BaseOffset;
		switch (MLInstTypes[Inst->Opcode]) {
		case MLIT_NONE:
			Inst += 1;
			break;
		case MLIT_INST:
			Inst += 2;
			break;
		case MLIT_INST_COUNT_DECL:
			ml_closure_find_decl(DeclBuffer, Decls, Inst[3].Decls);
			Inst += 4;
			break;
		case MLIT_COUNT_COUNT:
			Inst += 3;
			break;
		case MLIT_COUNT:
			Inst += 2;
			break;
		case MLIT_VALUE:
			Inst += 2;
			break;
		case MLIT_VALUE_DATA:
			Inst += 3;
			break;
		case MLIT_VALUE_COUNT:
			Inst += 3;
			break;
		case MLIT_VALUE_COUNT_DATA:
			Inst += 4;
			break;
		case MLIT_COUNT_CHARS:
			Inst += 3;
			break;
		case MLIT_DECL:
			ml_closure_find_decl(DeclBuffer, Decls, Inst[1].Decls);
			Inst += 2;
			break;
		case MLIT_COUNT_DECL:
			ml_closure_find_decl(DeclBuffer, Decls, Inst[2].Decls);
			Inst += 3;
			break;
		case MLIT_COUNT_COUNT_DECL:
			ml_closure_find_decl(DeclBuffer, Decls, Inst[3].Decls);
			Inst += 4;
			break;
		case MLIT_CLOSURE:
			Inst += 2 + Inst[1].ClosureInfo->NumUpValues;
			break;
		case MLIT_SWITCH:
			Inst += 3;
			break;
		default: __builtin_unreachable();
		}
	}
	vlq64_encode(Buffer, Decls->Size - Decls->Space);
	ml_stringbuffer_foreach(DeclBuffer, Buffer, (void *)ml_stringbuffer_copy);
	vlq64_encode(Buffer, DeclsIndex);
	vlq64_encode(Buffer, (Info->Halt - Base) + BaseOffset);
	vlq64_encode(Buffer, Return);
	int Line = Info->Entry->Line;
	vlq64_encode(Buffer, Info->Entry->Line - Info->StartLine);
	ml_value_t *Values = ml_list();
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Opcode == MLI_LINK) {
			Inst = Inst[1].Inst;
			continue;
		}
		if (Inst->Line != Line) {
			vlq64_encode(Buffer, MLI_LINK);
			vlq64_encode(Buffer, Inst->Line - Line);
			Line = Inst->Line;
		}
		vlq64_encode(Buffer, Inst->Opcode);
		switch (MLInstTypes[Inst->Opcode]) {
		case MLIT_NONE:
			Inst += 1;
			break;
		case MLIT_INST:
			vlq64_encode(Buffer, (uintptr_t)inthash_search(Labels, Inst[1].Inst->Label));
			Inst += 2;
			break;
		case MLIT_INST_COUNT_DECL:
			vlq64_encode(Buffer, (uintptr_t)inthash_search(Labels, Inst[1].Inst->Label));
			vlq64_encode(Buffer, Inst[2].Count);
			vlq64_encode(Buffer, (uintptr_t)inthash_search(Decls, (uintptr_t)Inst[3].Decls));
			Inst += 4;
			break;
		case MLIT_COUNT_COUNT:
			vlq64_encode(Buffer, Inst[1].Count);
			vlq64_encode(Buffer, Inst[2].Count);
			Inst += 3;
			break;
		case MLIT_COUNT:
			vlq64_encode(Buffer, Inst[1].Count);
			Inst += 2;
			break;
		case MLIT_VALUE:
			ml_list_put(Values, Inst[1].Value);
			Inst += 2;
			break;
		case MLIT_VALUE_DATA:
			ml_list_put(Values, Inst[1].Value);
			Inst += 3;
			break;
		case MLIT_VALUE_COUNT:
			ml_list_put(Values, Inst[1].Value);
			vlq64_encode(Buffer, Inst[2].Count);
			Inst += 3;
			break;
		case MLIT_VALUE_COUNT_DATA:
			ml_list_put(Values, Inst[1].Value);
			vlq64_encode(Buffer, Inst[2].Count);
			Inst += 4;
			break;
		case MLIT_COUNT_CHARS:
			vlq64_encode(Buffer, Inst[1].Count);
			ml_stringbuffer_write(Buffer, Inst[2].Chars, Inst[1].Count);
			Inst += 3;
			break;
		case MLIT_DECL:
			vlq64_encode(Buffer, (uintptr_t)inthash_search(Decls, (uintptr_t)Inst[1].Decls));
			Inst += 2;
			break;
		case MLIT_COUNT_DECL:
			vlq64_encode(Buffer, Inst[1].Count);
			vlq64_encode(Buffer, (uintptr_t)inthash_search(Decls, (uintptr_t)Inst[2].Decls));
			Inst += 3;
			break;
		case MLIT_COUNT_COUNT_DECL:
			vlq64_encode(Buffer, Inst[1].Count);
			vlq64_encode(Buffer, Inst[2].Count);
			vlq64_encode(Buffer, (uintptr_t)inthash_search(Decls, (uintptr_t)Inst[3].Decls));
			Inst += 4;
			break;
		case MLIT_CLOSURE: {
			ml_closure_info_t *Info = Inst[1].ClosureInfo;
			Info->Type = MLClosureInfoT;
			ml_list_put(Values, (ml_value_t *)Info);
			vlq64_encode(Buffer, Info->NumUpValues);
			for (int N = 0; N < Info->NumUpValues; ++N) vlq64_encode(Buffer, Inst[2 + N].Count);
			Inst += 2 + Info->NumUpValues;
			break;
		}
		case MLIT_SWITCH: {
			vlq64_encode(Buffer, Inst[1].Count);
			for (int N = 0; N < Inst[1].Count; ++N) {
				vlq64_encode(Buffer, (uintptr_t)inthash_search(Labels, Inst[2].Insts[N]->Label));
			}
			Inst += 3;
			break;
		}
		default: __builtin_unreachable();
		}
	}
	minicbor_write_tag(Writer->Data, Writer->WriteFn, 27);
	minicbor_write_array(Writer->Data, Writer->WriteFn, ml_list_length(Values) + 2);
	minicbor_write_string(Writer->Data, Writer->WriteFn, 1);
	Writer->WriteFn(Writer->Data, (unsigned char *)"!", 1);
	minicbor_write_bytes(Writer->Data, Writer->WriteFn, Buffer->Length);
	ml_stringbuffer_foreach(Buffer, Writer, (void *)ml_stringbuffer_to_cbor);
	ML_LIST_FOREACH(Values, Iter) ml_cbor_write(Writer, Iter->Value);
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLClosureT, ml_cbor_writer_t *Writer, ml_closure_t *Closure) {
	minicbor_write_tag(Writer->Data, Writer->WriteFn, 27);
	ml_closure_info_t *Info = Closure->Info;
	Info->Type = MLClosureInfoT;
	minicbor_write_array(Writer->Data, Writer->WriteFn, 2 + Info->NumUpValues);
	minicbor_write_string(Writer->Data, Writer->WriteFn, 1);
	Writer->WriteFn(Writer->Data, (unsigned char *)"*", 1);
	ml_cbor_write(Writer, (ml_value_t *)Info);
	for (int I = 0; I < Info->NumUpValues; ++I) {
		ml_value_t *Error = ml_cbor_write(Writer, Closure->UpValues[I]);
		if (Error) return Error;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLGlobalT, ml_cbor_writer_t *Writer, ml_value_t *Global) {
	return ml_cbor_write(Writer, ml_global_get(Global));
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLIntegerT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	//printf("%s()\n", __func__);
	int64_t Value = ml_integer_value_fast(Arg);
	if (Value < 0) {
		minicbor_write_negative(Writer->Data, Writer->WriteFn, ~Value);
	} else {
		minicbor_write_positive(Writer->Data, Writer->WriteFn, Value);
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLAddressT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	//printf("%s()\n", __func__);
	minicbor_write_bytes(Writer->Data, Writer->WriteFn, ml_address_length(Arg));
	Writer->WriteFn(Writer->Data, (const unsigned char *)ml_address_value(Arg), ml_address_length(Arg));
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLStringT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	//printf("%s()\n", __func__);
	minicbor_write_string(Writer->Data, Writer->WriteFn, ml_string_length(Arg));
	Writer->WriteFn(Writer->Data, (const unsigned char *)ml_string_value(Arg), ml_string_length(Arg));
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLRegexT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	//printf("%s()\n", __func__);
	const char *Pattern = ml_regex_pattern(Arg);
	minicbor_write_tag(Writer->Data, Writer->WriteFn, 35);
	minicbor_write_string(Writer->Data, Writer->WriteFn, strlen(Pattern));
	Writer->WriteFn(Writer->Data, (void *)Pattern, strlen(Pattern));
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLTupleT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_tag(Writer->Data, Writer->WriteFn, 27);
	int Size = ml_tuple_size(Arg);
	minicbor_write_array(Writer->Data, Writer->WriteFn, 1 + Size);
	minicbor_write_string(Writer->Data, Writer->WriteFn, 5);
	Writer->WriteFn(Writer->Data, (void *)"tuple", 5);
	for (int I = 1; I <= Size; ++I) {
		ml_value_t *Error = ml_cbor_write(Writer, ml_tuple_get(Arg, I));
		if (Error) return Error;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLListT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_array(Writer->Data, Writer->WriteFn, ml_list_length(Arg));
	ML_LIST_FOREACH(Arg, Node) {
		ml_value_t *Error = ml_cbor_write(Writer, Node->Value);
		if (Error) return Error;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLMapT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_map(Writer->Data, Writer->WriteFn, ml_map_size(Arg));
	ML_MAP_FOREACH(Arg, Node) {
		ml_value_t *Error = ml_cbor_write(Writer, Node->Key);
		if (Error) return Error;
		Error = ml_cbor_write(Writer, Node->Value);
		if (Error) return Error;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLDoubleT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_float8(Writer->Data, Writer->WriteFn, ml_double_value_fast(Arg));
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLNilT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_simple(Writer->Data, Writer->WriteFn, CBOR_SIMPLE_NULL);
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLBooleanT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_simple(Writer->Data, Writer->WriteFn, ml_boolean_value(Arg) ? CBOR_SIMPLE_TRUE : CBOR_SIMPLE_FALSE);
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLMethodT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	const char *Name = ml_method_name(Arg);
	minicbor_write_tag(Writer->Data, Writer->WriteFn, 39);
	minicbor_write_string(Writer->Data, Writer->WriteFn, strlen(Name));
	Writer->WriteFn(Writer->Data, (void *)Name, strlen(Name));
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLObjectT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_tag(Writer->Data, Writer->WriteFn, 27);
	int Size = ml_object_size(Arg);
	minicbor_write_array(Writer->Data, Writer->WriteFn, 1 + Size);
	const char *Name = ml_typeof(Arg)->Name;
	minicbor_write_string(Writer->Data, Writer->WriteFn, strlen(Name));
	Writer->WriteFn(Writer->Data, (void *)Name, strlen(Name));
	for (int I = 0; I < Size; ++I) {
		ml_value_t *Error = ml_cbor_write(Writer, ml_object_field(Arg, I));
		if (Error) return Error;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLIntegerRangeT, ml_cbor_writer_t *Writer, ml_integer_range_t *Arg) {
	minicbor_write_tag(Writer->Data, Writer->WriteFn, 27);
	if (Arg->Step != 1) {
		minicbor_write_array(Writer->Data, Writer->WriteFn, 4);
		minicbor_write_string(Writer->Data, Writer->WriteFn, 5);
		Writer->WriteFn(Writer->Data, (unsigned const char *)"range", 5);
		minicbor_write_integer(Writer->Data, Writer->WriteFn, Arg->Start);
		minicbor_write_integer(Writer->Data, Writer->WriteFn, Arg->Limit);
		minicbor_write_integer(Writer->Data, Writer->WriteFn, Arg->Step);
	} else {
		minicbor_write_array(Writer->Data, Writer->WriteFn, 3);
		minicbor_write_string(Writer->Data, Writer->WriteFn, 5);
		Writer->WriteFn(Writer->Data, (unsigned const char *)"range", 5);
		minicbor_write_integer(Writer->Data, Writer->WriteFn, Arg->Start);
		minicbor_write_integer(Writer->Data, Writer->WriteFn, Arg->Limit);
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLRealRangeT, ml_cbor_writer_t *Writer, ml_real_range_t *Arg) {
	minicbor_write_tag(Writer->Data, Writer->WriteFn, 27);
	minicbor_write_array(Writer->Data, Writer->WriteFn, 4);
	minicbor_write_string(Writer->Data, Writer->WriteFn, 5);
	Writer->WriteFn(Writer->Data, (unsigned const char *)"range", 5);
	minicbor_write_float8(Writer->Data, Writer->WriteFn, Arg->Start);
	minicbor_write_float8(Writer->Data, Writer->WriteFn, Arg->Limit);
	minicbor_write_float8(Writer->Data, Writer->WriteFn, Arg->Step);
	return NULL;
}

#ifdef ML_COMPLEX

#include <complex.h>
#undef I

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLComplexT, ml_cbor_writer_t *Writer, ml_complex_t *Arg) {
	minicbor_write_tag(Writer->Data, Writer->WriteFn, 27);
	minicbor_write_array(Writer->Data, Writer->WriteFn, 3);
	minicbor_write_string(Writer->Data, Writer->WriteFn, 7);
	Writer->WriteFn(Writer->Data, (unsigned const char *)"complex", 7);
	minicbor_write_float8(Writer->Data, Writer->WriteFn, creal(Arg->Value));
	minicbor_write_float8(Writer->Data, Writer->WriteFn, cimag(Arg->Value));
	return NULL;
}

ML_FUNCTION(DecodeComplex) {
//!internal
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLRealT);
	ML_CHECK_ARG_TYPE(1, MLRealT);
	return ml_complex(ml_real_value(Args[0]) + ml_real_value(Args[1]) * _Complex_I);
}

#endif

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLExternalT, ml_cbor_writer_t *Writer, ml_external_t *Arg) {
	minicbor_write_tag(Writer->Data, Writer->WriteFn, 29);
	minicbor_write_string(Writer->Data, Writer->WriteFn, Arg->Length);
	Writer->WriteFn(Writer->Data, (unsigned const char *)Arg->Name, Arg->Length);
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLSomeT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_tag(Writer->Data, Writer->WriteFn, 27);
	minicbor_write_array(Writer->Data, Writer->WriteFn, 1);
	minicbor_write_string(Writer->Data, Writer->WriteFn, 4);
	Writer->WriteFn(Writer->Data, (unsigned const char *)"some", 4);
	return NULL;
}

ML_METHOD_ANON(CborEncode, "cbor::encode");

ML_METHOD(CborEncode, MLAnyT) {
//@cbor::encode
//<Value
//>address|error
// Encode :mini:`Value` into CBOR or return an error if :mini:`Value` cannot be encoded.
	ml_cbor_t Cbor = ml_cbor_writer_encode(Args[0]);
	if (!Cbor.Length) return Cbor.Error;
	if (Cbor.Data) return ml_address(Cbor.Data, Cbor.Length);
	return ml_error("CborError", "Error encoding to cbor");
}

ML_METHOD(CborEncode, MLStringBufferT, MLAnyT) {
//@cbor::encode
//<Value
//>address|error
// Encode :mini:`Value` into CBOR or return an error if :mini:`Value` cannot be encoded.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_value_t *Value = Args[1];
	ml_cbor_writer_t Writer[1];
	Writer->Data = Buffer;
	Writer->WriteFn = (void *)ml_stringbuffer_write;
	Writer->Globals = MLExternals.Values;
	Writer->References[0] = INTHASH_INIT;
	Writer->Reused[0] = INTHASH_INIT;
	Writer->Index = 0;
	Writer->NumSettings = 0;
	ml_cbor_writer_find_refs(Writer, Value);
	ml_value_t *Error = ml_cbor_write(Writer, Value);
	if (Error) return Error;
	return (ml_value_t *)Buffer;
}

#ifdef ML_TABLES
#include "ml_table.h"



#endif

ml_value_t *ml_cbor_read_regex(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (!ml_is(Value, MLStringT)) return ml_error("TagError", "Regex requires string");
	return ml_regex(ml_string_value(Value), ml_string_length(Value));
}

ml_value_t *ml_cbor_read_method(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (!ml_is(Value, MLStringT)) return ml_error("TagError", "Method requires string");
	return ml_method(ml_string_value(Value));
}

static stringmap_t CborObjectTypes[1] = {STRINGMAP_INIT};

ml_value_t *ml_cbor_read_object(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (!ml_is(Value, MLListT)) return ml_error("TagError", "Object requires list");
	ml_list_iter_t Iter[1];
	ml_list_iter_forward(Value, Iter);
	if (!ml_list_iter_valid(Iter)) return ml_error("CBORError", "Object tag requires type name");
	ml_value_t *TypeName = Iter->Value;
	if (ml_typeof(TypeName) != MLStringT) return ml_error("CBORError", "Object tag requires type name");
	ml_value_t *Constructor = stringmap_search(CborObjectTypes, ml_string_value(TypeName));
	if (!Constructor) return ml_error("CBORError", "Object %s not found", ml_string_value(TypeName));
	int Count2 = ml_list_length(Value) - 1;
	ml_value_t **Args2 = ml_alloc_args(Count2);
	for (int I = 0; I < Count2; ++I) {
		ml_list_iter_next(Iter);
		Args2[I] = Iter->Value;
	}
	return ml_simple_call(Constructor, Count2, Args2);
}

typedef struct {
	int64_t Value;
	int Count;
} vlq_result_t;

static vlq_result_t vlq64_decode(const unsigned char *Bytes) {
	int64_t X = Bytes[0] & 63;
	if (!(Bytes[0] & 64)) {
		if (Bytes[0] & 128) X = ~X;
		return (vlq_result_t){X, 1};
	}
	int Shift = 6;
	for (int I = 1; I < 8; ++I) {
		X += (uint64_t)(Bytes[I] & 127) << Shift;
		Shift += 7;
		if (!(Bytes[I] & 128)) {
			if (Bytes[0] & 128) X = ~X;
			return (vlq_result_t){X, I + 1};
		}
	}
	X += (uint64_t)Bytes[8] << 55;
	return (vlq_result_t){X, 9};
}

#define VLQ64_NEXT() ({ \
	if (Length <= 0) return ml_error("CBORError", "Invalid closure info"); \
	vlq_result_t Result = vlq64_decode(Bytes); \
	Length -= Result.Count; \
	Bytes += Result.Count; \
	Result.Value; \
})

#define VLQ64_NEXT_STRING() ({ \
	if (Length <= 0) return ml_error("CBORError", "Invalid closure info"); \
	vlq_result_t Result = vlq64_decode(Bytes); \
	Length -= Result.Count; \
	if (Length < Result.Value) return ml_error("CBORError", "Invalid closure info"); \
	Bytes += Result.Count; \
	char *String = snew(Length + 1); \
	memcpy(String, Bytes, Result.Value); \
	String[Result.Value] = 0; \
	Length -= Result.Value; \
	Bytes += Result.Value; \
	String; \
})

#define NEXT_VALUE(DEST) { \
	ML_CHECK_ARG_COUNT(Index + 1); \
	ml_value_t *Value = Args[Index++]; \
	if (ml_typeof(Value) == MLUninitializedT) { \
		ml_uninitialized_use(Value, &DEST); \
	} \
	DEST = Value; \
}

ML_FUNCTION(DecodeClosureInfo) {
//!internal
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLAddressT);
	const unsigned char *Bytes = (const unsigned char *)ml_address_value(Args[0]);
	int Length = ml_address_length(Args[0]);
	int Version = VLQ64_NEXT();
	if (Version != ML_BYTECODE_VERSION) return ml_error("CBORError", "Bytecode version mismatch");
	ml_closure_info_t *Info = new(ml_closure_info_t);
	Info->Type = MLClosureInfoT;
	Info->Name = VLQ64_NEXT_STRING();
	Info->Source = VLQ64_NEXT_STRING();
	Info->StartLine = VLQ64_NEXT();
	Info->FrameSize = VLQ64_NEXT();
	Info->NumParams = VLQ64_NEXT();
	Info->NumUpValues = VLQ64_NEXT();
	Info->Flags = VLQ64_NEXT() & (ML_CLOSURE_EXTRA_ARGS | ML_CLOSURE_NAMED_ARGS);
	for (int I = 0; I < Info->NumParams; ++I) {
		const char *Param = VLQ64_NEXT_STRING();
		stringmap_insert(Info->Params, Param, (void *)(uintptr_t)(I + 1));
	}
	int NumDecls = VLQ64_NEXT();
	ml_decl_t *Decls = anew(ml_decl_t, NumDecls);
	for (int I = 0; I < NumDecls; ++I) {
		Decls[I].Ident = VLQ64_NEXT_STRING();
		int Next = VLQ64_NEXT();
		Decls[I].Next = Next >= 0 ? &Decls[Next] : NULL;
		Decls[I].Source.Name = Info->Source;
		Decls[I].Source.Line = VLQ64_NEXT();
		Decls[I].Index = VLQ64_NEXT();
		Decls[I].Flags = VLQ64_NEXT();
	}
	int DeclIndex = VLQ64_NEXT();
	Info->Decls = DeclIndex >= 0 ? &Decls[DeclIndex] : NULL;
	int NumInsts = VLQ64_NEXT();
	ml_inst_t *Code = Info->Entry = anew(ml_inst_t, NumInsts);
	ml_inst_t *Halt = Info->Halt = Code + NumInsts;
	ml_inst_t *Inst = Code;
	Info->Return = Inst + VLQ64_NEXT();
	int Line = VLQ64_NEXT() + Info->StartLine;
	int Index = 1;
	while (Inst < Halt) {
		ml_opcode_t Opcode = (ml_opcode_t)VLQ64_NEXT();
		if (Opcode == MLI_LINK) {
			Line += VLQ64_NEXT();
			continue;
		}
		Inst->Opcode = Opcode;
		Inst->Line = Line;
		switch (MLInstTypes[Opcode]) {
		case MLIT_NONE:
			Inst += 1; break;
		case MLIT_INST:
			Inst[1].Inst = Code + VLQ64_NEXT();
			Inst += 2; break;
		case MLIT_INST_COUNT_DECL:
			Inst[1].Inst = Code + VLQ64_NEXT();
			Inst[2].Count = VLQ64_NEXT();
			Inst[3].Decls = &Decls[VLQ64_NEXT()];
			Inst += 4; break;
		case MLIT_COUNT:
			Inst[1].Count = VLQ64_NEXT();
			Inst += 2; break;
		case MLIT_VALUE:
			NEXT_VALUE(Inst[1].Value);
			Inst += 2; break;
		case MLIT_VALUE_DATA:
			NEXT_VALUE(Inst[1].Value);
			Inst += 3; break;
		case MLIT_COUNT_COUNT:
			Inst[1].Count = VLQ64_NEXT();
			Inst[2].Count = VLQ64_NEXT();
			Inst += 3; break;
		case MLIT_VALUE_COUNT:
			NEXT_VALUE(Inst[1].Value);
			Inst[2].Count = VLQ64_NEXT();
			Inst += 3; break;
		case MLIT_VALUE_COUNT_DATA:
			NEXT_VALUE(Inst[1].Value);
			Inst[2].Count = VLQ64_NEXT();
			Inst += 4; break;
		case MLIT_COUNT_CHARS: {
			int Count2 = Inst[1].Count = VLQ64_NEXT();
			char *Chars = snew(Count2);
			memcpy(Chars, Bytes, Count2);
			Chars[Count2] = 0;
			Inst[2].Chars = Chars;
			Length -= Count2;
			Bytes += Count2;
			Inst += 3; break;
		}
		case MLIT_DECL:
			Inst[1].Decls = &Decls[VLQ64_NEXT()];
			Inst += 2; break;
		case MLIT_COUNT_DECL:
			Inst[1].Count = VLQ64_NEXT();
			Inst[2].Decls = &Decls[VLQ64_NEXT()];
			Inst += 3; break;
		case MLIT_COUNT_COUNT_DECL:
			Inst[1].Count = VLQ64_NEXT();
			Inst[2].Count = VLQ64_NEXT();
			Inst[3].Decls = &Decls[VLQ64_NEXT()];
			Inst += 4; break;
		case MLIT_CLOSURE:
			NEXT_VALUE(Inst[1].Value);
			int NumUpValues = VLQ64_NEXT();
			for (int J = 0; J < NumUpValues; ++J) Inst[J + 2].Count = VLQ64_NEXT();
			Inst += 2 + NumUpValues;
			break;
		case MLIT_SWITCH: {
			int Count2 = Inst[1].Count = VLQ64_NEXT();
			ml_inst_t **Ptr = Inst[2].Insts = anew(ml_inst_t *, Count2);
			for (int J = 0; J < Inst[1].Count; ++J) *Ptr++ = Code + VLQ64_NEXT();
			Inst += 3;
			break;
		}
		}
	}
	return (ml_value_t *)Info;
}

ML_FUNCTION(DecodeClosure) {
//!internal
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLClosureInfoT);
	ml_closure_info_t *Info = (ml_closure_info_t *)Args[0];
	ml_closure_t *Closure = (ml_closure_t *)ml_closure(Info);
	ML_CHECK_ARG_COUNT(Info->NumUpValues + 1);
	for (int I = 0; I < Info->NumUpValues; ++I) Closure->UpValues[I] = Args[I + 1];
	return (ml_value_t *)Closure;
}

extern ml_value_t *RangeMethod;

void ml_cbor_default_object(const char *Name, ml_value_t *Constructor) {
	stringmap_insert(CborObjectTypes, Name, Constructor);
}

void ml_cbor_init(stringmap_t *Globals) {
	ml_cbor_default_object("some", (ml_value_t *)MLSomeT);
	ml_cbor_default_object("tuple", (ml_value_t *)MLTupleT);
	ml_cbor_default_object("range", RangeMethod);
#ifdef ML_COMPLEX
	ml_cbor_default_object("complex", (ml_value_t *)DecodeComplex);
#endif
	ml_cbor_default_object("!", (ml_value_t *)DecodeClosureInfo);
	ml_cbor_default_object("*", (ml_value_t *)DecodeClosure);
	ml_cbor_default_tag(35, ml_cbor_read_regex);
	ml_cbor_default_tag(39, ml_cbor_read_method);
	ml_cbor_default_tag(27, ml_cbor_read_object);
	ml_cbor_default_tag(28, ml_cbor_mark_reused);
	ml_cbor_default_tag(29, ml_cbor_use_previous);
#include "ml_cbor_init.c"
	if (Globals) {
		stringmap_insert(Globals, "cbor", ml_module("cbor",
			"encode", CborEncode,
			"decode", CborDecode,
		NULL));
	}
}

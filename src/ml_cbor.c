#include "minilang.h"
#include "ml_macros.h"
#include "ml_cbor.h"
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

ml_cbor_tag_fns_t *ml_tag_fns_copy(ml_cbor_tag_fns_t *TagFns) {
	ml_cbor_tag_fns_t *Copy = new(ml_cbor_tag_fns_t);
	int Count = Copy->Count = TagFns->Count;
	Copy->Space = TagFns->Space;
	int Size = Count + TagFns->Space;
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

ml_cbor_tag_fns_t *ml_cbor_tag_fns_new(int Default) {
	ml_cbor_tag_fns_t *Copy = new(ml_cbor_tag_fns_t);
	if (Default) {
		int Count = Copy->Count = DefaultTagFns->Count;
		int Space = Copy->Space = DefaultTagFns->Space;
		uint64_t *Tags = Copy->Tags = anew(uint64_t, Count + Space);
		memcpy(Tags, DefaultTagFns->Tags, Count * sizeof(uint64_t));
		ml_cbor_tag_fn *Fns = Copy->Fns = anew(ml_cbor_tag_fn, Count + Space);
		memcpy(Fns, DefaultTagFns->Fns, Count * sizeof(ml_cbor_tag_fn));
	}
	return Copy;
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
	void *Data;
	collection_t *Collection;
	tag_t *Tags;
	ml_value_t *Value;
	ml_cbor_tag_fns_t *TagFns;
	ml_value_t **Reused;
	minicbor_reader_t Reader[1];
	int NumReused, MaxReused;
};

static int ml_cbor_default_tag_fn(uintptr_t Tag, void *Fn, inthash_t *TagFns) {
	return 0;
}

inthash_t *ml_cbor_default_tags() {
	inthash_t *TagFns = inthash_new();
	//inthash_foreach(DefaultTagFns, TagFns, (void *)ml_cbor_default_tag_fn);
	return TagFns;
}

ml_cbor_reader_t *ml_cbor_reader_new(ml_cbor_tag_fns_t *TagFns, void *Data) {
	ml_cbor_reader_t *Reader = new(ml_cbor_reader_t);
	Reader->TagFns = TagFns ?: DefaultTagFns;
	Reader->Data = Data;
	ml_cbor_reader_init(Reader->Reader);
	Reader->Reader->UserData = Reader;
	return Reader;
}

void ml_cbor_reader_set_data(ml_cbor_reader_t *Reader, void *Data) {
	Reader->Data = Data;
}

void *ml_cbor_reader_get_data(ml_cbor_reader_t *Reader) {
	return Reader->Data;
}

static int ml_cbor_reader_next_index(ml_cbor_reader_t *Reader) {
	int Index = Reader->NumReused++;
	if (Index == Reader->MaxReused) {
		Reader->MaxReused = Reader->MaxReused + 8;
		Reader->Reused = GC_realloc(Reader->Reused, Reader->MaxReused * sizeof(ml_value_t *));
	}
	return Index;
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

ml_value_t *ml_cbor_mark_reused(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	return ml_error("CBORError", "Mark reused should not be called");
}

ml_value_t *ml_cbor_use_previous(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	return ml_error("CBORError", "Use previous should not be called");
}

static void value_handler(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	for (tag_t *Tag = Reader->Tags; Tag; Tag = Tag->Prev) {
		if (!ml_is_error(Value)) {
			if (Tag->Handler == ml_cbor_mark_reused) {
				ml_value_t *Uninitialized = Reader->Reused[Tag->Index];
				if (Uninitialized) ml_uninitialized_set(Uninitialized, Value);
				Reader->Reused[Tag->Index] = Value;
			} else if (Tag->Handler == ml_cbor_use_previous) {
				int Index = ml_integer_value(Value);
				if (Index < 0 || Index >= Reader->NumReused) {
					Value = ml_error("CBORError", "Invalid previous index");
				} else {
					Value = Reader->Reused[Index];
					if (!Value) Value = Reader->Reused[Index] = ml_uninitialized("CBOR");
				}
			} else {
				Value = Tag->Handler(Reader, Value);
			}
		}
	}
	Reader->Tags = 0;
	collection_t *Collection = Reader->Collection;
	if (!Collection) {
		Reader->Value = Value;
		ml_cbor_reader_finish(Reader->Reader);
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
		value_handler(Reader, ml_buffer(NULL, 0));
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

static ml_value_t *ml_value_fn(ml_value_t *Callback, ml_value_t *Value) {
	return ml_simple_inline(Callback, 1, Value);
}

static ml_cbor_tag_fn ml_value_tag_fn(uint64_t Tag, ml_value_t *Callback, void **Data) {
	Data[0] = ml_simple_inline(Callback, 1, ml_integer(Tag));
	return (ml_cbor_tag_fn)ml_value_fn;
}

ml_value_t *ml_from_cbor(ml_cbor_t Cbor, ml_cbor_tag_fns_t *TagFns) {
	ml_cbor_reader_t Reader[1];
	Reader->TagFns = TagFns ?: DefaultTagFns;
	Reader->Reused = NULL;
	Reader->NumReused = Reader->MaxReused = 0;
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

ml_cbor_result_t ml_from_cbor_extra(ml_cbor_t Cbor, ml_cbor_tag_fns_t *TagFns) {
	ml_cbor_reader_t Reader[1];
	Reader->TagFns = TagFns ?: DefaultTagFns;
	Reader->Reused = NULL;
	Reader->NumReused = Reader->MaxReused = 0;
	ml_cbor_reader_init(Reader->Reader);
	Reader->Reader->UserData = Reader;
	Reader->Collection = 0;
	Reader->Tags = 0;
	Reader->Value = 0;
	ml_cbor_read(Reader->Reader, Cbor.Data, Cbor.Length);
	return (ml_cbor_result_t){ml_cbor_reader_get(Reader), ml_cbor_reader_extra(Reader)};
}

ML_FUNCTION(MLDecode) {
//@cbor::decode
//<Bytes
//>any | error
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLAddressT);
	ml_cbor_t Cbor = {{.Data = ml_address_value(Args[0])}, ml_address_length(Args[0])};
	return ml_from_cbor(Cbor, NULL);
}

ml_value_t *ml_cbor_writer_write(ml_cbor_writer_t *Writer, ml_value_t *Value) {
	inthash_result_t Result = inthash_search2(Writer->Reused, (uintptr_t)Value);
	if (Result.Present) {
		if (Result.Value) {
			int Index = (uintptr_t)Result.Value - 1;
			ml_cbor_write_tag(Writer->Data, Writer->WriteFn, 29);
			ml_cbor_write_integer(Writer->Data, Writer->WriteFn, Index);
			return NULL;
		}
		int Index = ++Writer->Index;
		inthash_insert(Writer->Reused, (uintptr_t)Value, (void *)(uintptr_t)Index);
		ml_cbor_write_tag(Writer->Data, Writer->WriteFn, 28);
	}
	typeof(ml_cbor_writer_write) *function = ml_typed_fn_get(ml_typeof(Value), ml_cbor_writer_write);
	if (function) return function(Writer, Value);
	typeof(ml_cbor_write) *function2 = ml_typed_fn_get(ml_typeof(Value), ml_cbor_write);
	if (function2) return function2(Value, Writer->Data, Writer->WriteFn);
	return ml_error("CBORError", "No method to encode %s to CBOR", ml_typeof(Value)->Name);
}

static int ml_cbor_writer_ref_fn(ml_cbor_writer_t *Writer, ml_value_t *Value) {
	if (inthash_insert(Writer->References, (uintptr_t)Value, Value)) {
		inthash_insert(Writer->Reused, (uintptr_t)Value, (void *)0L);
		return 0;
	}
	return 1;
}

ml_cbor_t ml_cbor_writer_encode(ml_value_t *Value) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_cbor_writer_t Writer[1];
	ml_cbor_writer_init(Writer, Buffer, (void *)ml_stringbuffer_write);
	ml_value_find_refs(Value, Writer, (ml_value_ref_fn)ml_cbor_writer_ref_fn);
	ml_value_t *Error = ml_cbor_writer_write(Writer, Value);
	if (Error) return (ml_cbor_t){{.Error = Error}, 0};
	size_t Size = Buffer->Length;
	return (ml_cbor_t){{.Data = ml_stringbuffer_get_string(Buffer)}, Size};
}

static ml_value_t *ML_TYPED_FN(ml_cbor_writer_write, MLListT, ml_cbor_writer_t *Writer, ml_value_t *Value) {
	ml_cbor_write_array(Writer->Data, Writer->WriteFn, ml_list_length(Value));
	ML_LIST_FOREACH(Value, Iter) {
		ml_value_t *Error = ml_cbor_writer_write(Writer, Iter->Value);
		if (Error) return Error;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_writer_write, MLMapT, ml_cbor_writer_t *Writer, ml_value_t *Value) {
	ml_cbor_write_map(Writer->Data, Writer->WriteFn, ml_map_size(Value));
	ML_MAP_FOREACH(Value, Iter) {
		ml_value_t *Error = ml_cbor_writer_write(Writer, Iter->Key);
		if (Error) return Error;
		Error = ml_cbor_writer_write(Writer, Iter->Value);
		if (Error) return Error;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_writer_write, MLObjectT, ml_cbor_writer_t *Writer, ml_value_t *Value) {
	ml_cbor_write_tag(Writer->Data, Writer->WriteFn, 27);
	int Size = ml_object_size(Value);
	ml_cbor_write_array(Writer->Data, Writer->WriteFn, 1 + Size);
	const char *Name = ml_typeof(Value)->Name;
	ml_cbor_write_string(Writer->Data, Writer->WriteFn, strlen(Name));
	Writer->WriteFn(Writer->Data, (unsigned char *)Name, strlen(Name));
	for (int I = 0; I < Size; ++I) {
		ml_value_t *Error = ml_cbor_writer_write(Writer, ml_object_field(Value, I));
		if (Error) return Error;
	}
	return NULL;
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
	inthash_result_t Result = inthash_search2(Decls, (uintptr_t)Decl);
	if (Result.Present) return (uintptr_t)Result.Value;
	int Next = Decl->Next ? ml_closure_find_decl(Buffer, Decls, Decl->Next) : -1;
	int Index = Decls->Size;
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

static ml_value_t *ML_TYPED_FN(ml_cbor_writer_write, MLClosureInfoT, ml_cbor_writer_t *Writer, ml_closure_info_t *Info) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	vlq64_encode(Buffer, ML_BYTECODE_VERSION);
	vlq64_encode_string(Buffer, Info->Name ?: "");
	vlq64_encode_string(Buffer, Info->Source ?: "");
	vlq64_encode(Buffer, Info->StartLine);
	vlq64_encode(Buffer, Info->FrameSize);
	vlq64_encode(Buffer, Info->NumParams);
	vlq64_encode(Buffer, Info->NumUpValues);
	vlq64_encode(Buffer, Info->Flags);
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
			ml_closure_find_decl(Buffer, Decls, Inst[3].Decls);
			Inst += 4;
			break;
		case MLIT_INST_TYPES:
			Inst += 3;
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
			ml_closure_find_decl(Buffer, Decls, Inst[1].Decls);
			Inst += 2;
			break;
		case MLIT_COUNT_DECL:
			ml_closure_find_decl(Buffer, Decls, Inst[2].Decls);
			Inst += 3;
			break;
		case MLIT_COUNT_COUNT_DECL:
			ml_closure_find_decl(Buffer, Decls, Inst[3].Decls);
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
		case MLIT_INST_TYPES: {
			vlq64_encode(Buffer, (uintptr_t)inthash_search(Labels, Inst[1].Inst->Label));
			int Count = 0;
			for (const char **Ptr = Inst[2].Ptrs; *Ptr; ++Ptr) ++Count;
			vlq64_encode(Buffer, Count);
			for (const char **Ptr = Inst[2].Ptrs; *Ptr; ++Ptr) vlq64_encode_string(Buffer, *Ptr);
			Inst += 3;
			break;
		}
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
	ml_cbor_write_tag(Writer->Data, Writer->WriteFn, 27);
	ml_cbor_write_array(Writer->Data, Writer->WriteFn, ml_list_length(Values) + 2);
	ml_cbor_write_string(Writer->Data, Writer->WriteFn, 1);
	Writer->WriteFn(Writer->Data, (unsigned char *)"!", 1);
	ml_cbor_write_bytes(Writer->Data, Writer->WriteFn, Buffer->Length);
	ml_stringbuffer_foreach(Buffer, Writer, (void *)ml_stringbuffer_to_cbor);
	ML_LIST_FOREACH(Values, Iter) ml_cbor_writer_write(Writer, Iter->Value);
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_writer_write, MLClosureT, ml_cbor_writer_t *Writer, ml_closure_t *Closure) {
	ml_cbor_write_tag(Writer->Data, Writer->WriteFn, 27);
	ml_closure_info_t *Info = Closure->Info;
	ml_cbor_write_array(Writer->Data, Writer->WriteFn, 2 + Info->NumUpValues);
	ml_cbor_write_string(Writer->Data, Writer->WriteFn, 7);
	Writer->WriteFn(Writer->Data, (unsigned char *)"closure", 7);
	ml_cbor_writer_write(Writer, (ml_value_t *)Info);
	for (int I = 0; I < Info->NumUpValues; ++I) {
		ml_value_t *Error = ml_cbor_writer_write(Writer, Closure->UpValues[I]);
		if (Error) return Error;
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_writer_write, MLGlobalT, ml_cbor_writer_t *Writer, ml_value_t *Global) {
	return ml_cbor_writer_write(Writer, ml_global_get(Global));
}

ml_value_t *ml_cbor_write(ml_value_t *Value, void *Data, ml_cbor_write_fn WriteFn) {
	typeof(ml_cbor_write) *function = ml_typed_fn_get(ml_typeof(Value), ml_cbor_write);
	if (function) {
		return function(Value, Data, WriteFn);
	} else {
		return ml_error("CBORError", "No method to encode %s to CBOR", ml_typeof(Value)->Name);
	}
}

ml_cbor_t ml_to_cbor(ml_value_t *Value) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_value_t *Error = ml_cbor_write(Value, Buffer, (void *)ml_stringbuffer_write);
	if (Error) return (ml_cbor_t){{.Error = Error}, 0};
	size_t Size = Buffer->Length;
	return (ml_cbor_t){{.Data = ml_stringbuffer_get_string(Buffer)}, Size};
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

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLIntegerRangeT, ml_integer_range_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_tag(Data, WriteFn, 27);
	if (Arg->Step != 1) {
		ml_cbor_write_array(Data, WriteFn, 4);
		ml_cbor_write_string(Data, WriteFn, 5);
		WriteFn(Data, (unsigned const char *)"range", 5);
		ml_cbor_write_integer(Data, WriteFn, Arg->Start);
		ml_cbor_write_integer(Data, WriteFn, Arg->Limit);
		ml_cbor_write_integer(Data, WriteFn, Arg->Step);
	} else {
		ml_cbor_write_array(Data, WriteFn, 3);
		ml_cbor_write_string(Data, WriteFn, 5);
		WriteFn(Data, (unsigned const char *)"range", 5);
		ml_cbor_write_integer(Data, WriteFn, Arg->Start);
		ml_cbor_write_integer(Data, WriteFn, Arg->Limit);
	}
	return NULL;
}

static ml_value_t *ML_TYPED_FN(ml_cbor_write, MLRealRangeT, ml_real_range_t *Arg, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_tag(Data, WriteFn, 27);
	ml_cbor_write_array(Data, WriteFn, 4);
	ml_cbor_write_string(Data, WriteFn, 5);
	WriteFn(Data, (unsigned const char *)"range", 5);
	ml_cbor_write_float8(Data, WriteFn, Arg->Start);
	ml_cbor_write_float8(Data, WriteFn, Arg->Limit);
	ml_cbor_write_float8(Data, WriteFn, Arg->Step);
	return NULL;
}

ML_FUNCTION(MLEncode) {
//@cbor::encode
//<Value
//>string | error
	ML_CHECK_ARG_COUNT(1);
	ml_cbor_t Cbor = ml_cbor_writer_encode(Args[0]);
	if (!Cbor.Length) return Cbor.Error;
	if (Cbor.Data) return ml_string(Cbor.Data, Cbor.Length);
	return ml_error("CborError", "Error encoding to cbor");
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

ml_value_t *CborObjects = NULL;

ml_value_t *ml_cbor_read_object(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (!ml_is(Value, MLListT)) return ml_error("TagError", "Object requires list");
	ml_list_iter_t Iter[1];
	ml_list_iter_forward(Value, Iter);
	if (!ml_list_iter_valid(Iter)) return ml_error("CBORError", "Object tag requires type name");
	ml_value_t *TypeName = Iter->Value;
	if (ml_typeof(TypeName) != MLStringT) return ml_error("CBORError", "Object tag requires type name");
	ml_value_t *Constructor = ml_map_search(CborObjects, TypeName);
	if (Constructor == MLNil) return ml_error("CBORError", "Object %s not found", ml_string_value(TypeName));
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
	Info->Flags = VLQ64_NEXT();
	for (int I = 0; I < Info->NumParams; ++I) {
		const char *Param = VLQ64_NEXT_STRING();
		stringmap_insert(Info->Params, Param, (void *)(uintptr_t)(I + 1));
	}
	int NumDecls = VLQ64_NEXT();
	ml_decl_t *Decls = anew(ml_decl_t, NumDecls);
	for (int I = 0; I < NumDecls; ++I) {
		Decls[I].Ident = VLQ64_NEXT_STRING();
		int Next = VLQ64_NEXT();
		Decls[I].Next = Next > 0 ? &Decls[Next] : NULL;
		Decls[I].Source.Name = Info->Source;
		Decls[I].Source.Line = VLQ64_NEXT();
		Decls[I].Index = VLQ64_NEXT();
		Decls[I].Flags = VLQ64_NEXT();
	}
	Info->Decls = &Decls[VLQ64_NEXT()];
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
		case MLIT_INST_TYPES: {
			Inst[1].Inst = Code + VLQ64_NEXT();
			int Count2 = VLQ64_NEXT();
			const char **Ptrs = Inst[2].Ptrs = anew(const char *, Count2 + 1);
			for (int J = 0; J < Count2; ++J) *Ptrs++ = VLQ64_NEXT_STRING();
			Inst += 3; break;
		}
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
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLClosureInfoT);
	ml_closure_info_t *Info = (ml_closure_info_t *)Args[0];
	ml_closure_t *Closure = (ml_closure_t *)ml_closure(Info);
	ML_CHECK_ARG_COUNT(Info->NumUpValues + 1);
	for (int I = 0; I < Info->NumUpValues; ++I) Closure->UpValues[I] = Args[I + 1];
	return (ml_value_t *)Closure;
}

extern ml_value_t *RangeMethod;

void ml_cbor_init(stringmap_t *Globals) {
	if (!CborObjects) CborObjects = ml_map();
	ml_map_insert(CborObjects, ml_cstring("range"), RangeMethod);
	ml_map_insert(CborObjects, ml_cstring("!"), (ml_value_t *)DecodeClosureInfo);
	ml_map_insert(CborObjects, ml_cstring("closure"), (ml_value_t *)DecodeClosure);
	ml_cbor_default_tag(35, ml_cbor_read_regex);
	ml_cbor_default_tag(39, ml_cbor_read_method);
	ml_cbor_default_tag(27, ml_cbor_read_object);
	ml_cbor_default_tag(28, ml_cbor_mark_reused);
	ml_cbor_default_tag(29, ml_cbor_use_previous);
#include "ml_cbor_init.c"
	if (Globals) {
		stringmap_insert(Globals, "cbor", ml_module("cbor",
			"encode", MLEncode,
			"decode", MLDecode,
			"Objects", CborObjects,
		NULL));
	}
}

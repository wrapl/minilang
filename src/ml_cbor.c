#include "minilang.h"
#include "ml_macros.h"
#include "ml_cbor.h"
#include "ml_stream.h"
#ifdef ML_CBOR_BYTECODE
#include "ml_bytecode.h"
#endif
#include <string.h>
#include "ml_object.h"
#include "ml_bytecode.h"

#undef ML_CATEGORY
#define ML_CATEGORY "cbor"

#define MINICBOR_WRITEDATA_TYPE ml_cbor_writer_t *
#define MINICBOR_WRITE_FN ml_cbor_write_raw
#define MINICBOR_WRITE_BUFFER(WRITER) WRITER->Buffer

#include "minicbor/minicbor.h"
#include "minicbor/minicbor_stream.c"

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

typedef struct collection_t {
	struct collection_t *Prev;
	struct tag_t *Tags;
	ml_value_t *Key;
	ml_value_t *Collection;
	int Remaining;
} collection_t;

typedef struct tag_t {
	struct tag_t *Prev;
	ml_cbor_tag_fn Handler;
	int Index;
} tag_t;

struct ml_cbor_reader_t {
	collection_t *Collection, *FreeCollection;
	tag_t *Tags, *FreeTag;
	ml_value_t *Value;
	ml_cbor_tag_fns_t *TagFns;
	ml_external_fn_t GlobalGet;
	void *Globals;
	ml_value_t **Reused;
	minicbor_stream_t Stream[1];
	ml_stringbuffer_t Buffer[1];
	int NumReused, MaxReused;
	int NumSettings;
	void *Settings[];
};

static int NumCborSettings = 0;

int ml_cbor_setting() {
	return NumCborSettings++;
}

ml_cbor_reader_t *ml_cbor_reader(ml_cbor_tag_fns_t *TagFns, ml_external_fn_t GlobalGet, void *Globals) {
	ml_cbor_reader_t *Reader = xnew(ml_cbor_reader_t, NumCborSettings, void *);
	Reader->TagFns = TagFns ?: DefaultTagFns;
	if (GlobalGet) {
		Reader->GlobalGet = GlobalGet;
		Reader->Globals = Globals;
	} else {
		Reader->GlobalGet = (ml_external_fn_t)ml_externals_get_value;
		Reader->Globals = MLExternals;
	}
	Reader->NumSettings = NumCborSettings;
	Reader->NumReused = Reader->MaxReused = 0;
	minicbor_stream_init(Reader->Stream);
	return Reader;
}

void ml_cbor_reader_reset(ml_cbor_reader_t *Reader) {
	Reader->Collection = NULL;
	Reader->Tags = NULL;
	Reader->Value = NULL;
	Reader->NumReused = Reader->MaxReused = 0;
	Reader->Buffer[0] = ML_STRINGBUFFER_INIT;
	minicbor_stream_init(Reader->Stream);
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

int ml_cbor_reader_done(ml_cbor_reader_t *Reader) {
	return Reader->Value != NULL;
}

ml_value_t *ml_cbor_reader_get(ml_cbor_reader_t *Reader) {
	if (!Reader->Value) return ml_error("CBORError", "CBOR not completely read");
	return Reader->Value;
}

int ml_cbor_reader_extra(ml_cbor_reader_t *Reader) {
	return Reader->Stream->Available;
}

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
			if (!Value) Value = Reader->Reused[Index] = ml_uninitialized("CBOR", (ml_source_t){"cbor", 0});
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

static collection_t *collection_push(ml_cbor_reader_t *Reader) {
	collection_t *Collection = Reader->FreeCollection;
	if (Collection) Reader->FreeCollection = Collection->Prev; else Collection = new(collection_t);
	Collection->Prev = Reader->Collection;
	Collection->Tags = Reader->Tags;
	Reader->Tags = NULL;
	Reader->Collection = Collection;
	return Collection;
}

static void value_handler(ml_cbor_reader_t *Reader, ml_value_t *Value);

static void collection_pop(ml_cbor_reader_t *Reader) {
	collection_t *Collection = Reader->Collection;
	Reader->Collection = Collection->Prev;
	Reader->Tags = Collection->Tags;
	ml_value_t *Value = Collection->Collection;
	Collection->Prev = Reader->FreeCollection;
	Collection->Collection = NULL;
	Collection->Key = NULL;
	Reader->FreeCollection = Collection;
	value_handler(Reader, Value);
}

static ml_value_t IsList[1];

static void value_handler(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		Reader->Value = Value;
		Reader->Stream->State = MCS_FINISHED;
		return;
	}
	tag_t *Tags = Reader->Tags;
	if (Tags) {
		tag_t *Tag = Tags;
		for (;;) {
			if (Tag->Handler == ml_cbor_mark_reused) {
				ml_value_t *Uninitialized = Reader->Reused[Tag->Index];
				if (Uninitialized) ml_uninitialized_set(Uninitialized, Value);
				Reader->Reused[Tag->Index] = Value;
			} else {
				Value = Tag->Handler(Reader, Value);
				if (ml_is_error(Value)) {
					Reader->Value = Value;
					Reader->Stream->State = MCS_FINISHED;
					return;
				}
			}
			if (!Tag->Prev) break;
			Tag = Tag->Prev;
		}
		Tag->Prev = Reader->FreeTag;
		Reader->FreeTag = Tags;
		Reader->Tags = NULL;
	}
	collection_t *Collection = Reader->Collection;
	if (!Collection) {
		Reader->Value = Value;
		Reader->Stream->State = MCS_FINISHED;
		return;
	} else if (Collection->Key == IsList) {
		ml_list_put(Collection->Collection, Value);
		if (Collection->Remaining && --Collection->Remaining == 0) {
			collection_pop(Reader);
		}
	} else if (Collection->Key) {
		ml_map_insert(Collection->Collection, Collection->Key, Value);
		if (Collection->Remaining && --Collection->Remaining == 0) {
			collection_pop(Reader);
		} else {
			Collection->Key = NULL;
		}
	} else {
		Collection->Key = Value;
	}
}

int ml_cbor_reader_read(ml_cbor_reader_t *Reader, const unsigned char *Bytes, int Size) {
	minicbor_stream_t *Stream = Reader->Stream;
	Stream->Next = Bytes;
	Stream->Available = Size;
	do {
		switch (minicbor_next(Stream)) {
		case MCE_WAIT: break;
		case MCE_POSITIVE:
			value_handler(Reader, ml_integer(Stream->Integer));
			break;
		case MCE_NEGATIVE:
			value_handler(Reader, ml_integer(~(int64_t)Stream->Integer));
			break;
		case MCE_BYTES:
			if (!Stream->Required) value_handler(Reader, ml_address(NULL, 0));
			break;
		case MCE_BYTES_PIECE:
			ml_stringbuffer_write(Reader->Buffer, (const char *)Stream->Bytes, Stream->Size);
			if (!Stream->Required) value_handler(Reader, ml_stringbuffer_to_address(Reader->Buffer));
			break;
		case MCE_STRING:
			if (!Stream->Required) value_handler(Reader, ml_cstring(""));
			break;
		case MCE_STRING_PIECE:
			ml_stringbuffer_write(Reader->Buffer, (const char *)Stream->Bytes, Stream->Size);
			if (!Stream->Required) value_handler(Reader, ml_stringbuffer_to_string(Reader->Buffer));
			break;
		case MCE_ARRAY:
			if (Stream->Required) {
				collection_t *Collection = collection_push(Reader);
				Collection->Remaining = Stream->Required;
				Collection->Key = IsList;
				Collection->Collection = ml_list();
			} else {
				value_handler(Reader, ml_list());
			}
			break;
		case MCE_MAP:
			if (Stream->Required) {
				collection_t *Collection = collection_push(Reader);
				Collection->Remaining = Stream->Required;
				Collection->Key = NULL;
				Collection->Collection = ml_map();
			} else {
				value_handler(Reader, ml_map());
			}
			break;
		case MCE_TAG: {
			ml_cbor_tag_fn Handler = ml_cbor_tag_fn_get(Reader->TagFns, Stream->Tag);
			if (Handler) {
				tag_t *Tag = Reader->FreeTag;
				if (Tag) Reader->FreeTag = Tag->Prev; else Tag = new(tag_t);
				Tag->Prev = Reader->Tags;
				Tag->Handler = Handler;
				// TODO: Reimplement this without hard-coding tag ML_CBOR_TAG_MARK_REUSED
				if (Stream->Tag == ML_CBOR_TAG_MARK_REUSED) Tag->Index = ml_cbor_reader_next_index(Reader);
				Reader->Tags = Tag;
			}
			break;
		}
		case MCE_SIMPLE:
			switch (Stream->Simple) {
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
			break;
		case MCE_FLOAT:
			value_handler(Reader, ml_real(Stream->Real));
			break;
		case MCE_BREAK:
			collection_pop(Reader);
			break;
		case MCE_ERROR:
			value_handler(Reader, ml_error("CBORError", "Invalid CBOR"));
			break;
		}
	} while (Stream->Available && !Reader->Value);
	return Size - Stream->Available;
}

ml_value_t *ml_from_cbor(ml_cbor_t Cbor, ml_cbor_tag_fns_t *TagFns) {
	ml_cbor_reader_t Reader[1] = {{0,}};
	Reader->TagFns = TagFns ?: DefaultTagFns;
	Reader->GlobalGet = (ml_external_fn_t)ml_externals_get_value;
	Reader->Globals = MLExternals;
	Reader->Reused = NULL;
	minicbor_stream_init(Reader->Stream);
	ml_cbor_reader_read(Reader, Cbor.Data, Cbor.Length);
	int Extra = ml_cbor_reader_extra(Reader);
	if (Extra) return ml_error("CBORError", "Extra bytes after decoding: %d", Extra);
	return ml_cbor_reader_get(Reader);
}

ml_cbor_result_t ml_from_cbor_extra(ml_cbor_t Cbor, ml_cbor_tag_fns_t *TagFns) {
	ml_cbor_reader_t Reader[1] = {{0,}};
	Reader->TagFns = TagFns ?: DefaultTagFns;
	Reader->GlobalGet = (ml_external_fn_t)ml_externals_get_value;
	Reader->Globals = MLExternals;
	Reader->Reused = NULL;
	minicbor_stream_init(Reader->Stream);
	ml_cbor_reader_read(Reader, Cbor.Data, Cbor.Length);
	return (ml_cbor_result_t){ml_cbor_reader_get(Reader), ml_cbor_reader_extra(Reader)};
}

ML_METHOD_ANON(CborDecode, "cbor::decode");

ML_METHOD(CborDecode, MLAddressT) {
//@cbor::decode
//<Bytes
//>any|error
// Decode :mini:`Bytes` into a Minilang value, or return an error if :mini:`Bytes` contains invalid CBOR or cannot be decoded into a Minilang value.
	ml_cbor_reader_t Reader[1] = {{0,}};
	Reader->TagFns = DefaultTagFns;
	Reader->GlobalGet = (ml_external_fn_t)ml_externals_get_value;
	Reader->Globals = MLExternals;
	Reader->Reused = NULL;
	minicbor_stream_init(Reader->Stream);
	ml_cbor_reader_read(Reader, (const unsigned char *)ml_address_value(Args[0]), ml_address_length(Args[0]));
	int Extra = ml_cbor_reader_extra(Reader);
	if (Extra) {
		if (Reader->Value && ml_is_error(Reader->Value)) return Reader->Value;
		return ml_error("CBORError", "Extra bytes after decoding: %d", Extra);
	}
	return ml_cbor_reader_get(Reader);
}

static ml_value_t *ml_cbor_global_get_map(ml_value_t *Map, const char *Name) {
	ml_value_t *Value = ml_map_search(Map, ml_string(Name, -1));
	if (Value) return Value;
	return ml_externals_get_value(MLExternals, Name);
}

ML_METHOD(CborDecode, MLAddressT, MLMapT) {
//@cbor::decode
//<Bytes
//<Globals
//>any|error
// Decode :mini:`Bytes` into a Minilang value, or return an error if :mini:`Bytes` contains invalid CBOR or cannot be decoded into a Minilang value.
	ml_cbor_reader_t Reader[1] = {{0,}};
	Reader->TagFns = DefaultTagFns;
	Reader->GlobalGet = (ml_external_fn_t)ml_cbor_global_get_map;
	Reader->Globals = Args[1];
	Reader->Reused = NULL;
	minicbor_stream_init(Reader->Stream);
	ml_cbor_reader_read(Reader, (const unsigned char *)ml_address_value(Args[0]), ml_address_length(Args[0]));
	int Extra = ml_cbor_reader_extra(Reader);
	if (Extra) {
		if (Reader->Value && ml_is_error(Reader->Value)) return Reader->Value;
		return ml_error("CBORError", "Extra bytes after decoding: %d", Extra);
	}
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
	ml_cbor_reader_t Reader[1] = {{0,}};
	Reader->TagFns = DefaultTagFns;
	Reader->GlobalGet = (ml_external_fn_t)ml_cbor_global_get_fn;
	Reader->Globals = Args[1];
	Reader->Reused = NULL;
	minicbor_stream_init(Reader->Stream);
	ml_cbor_reader_read(Reader, (const unsigned char *)ml_address_value(Args[0]), ml_address_length(Args[0]));
	int Extra = ml_cbor_reader_extra(Reader);
	if (Extra) {
		if (Reader->Value && ml_is_error(Reader->Value)) return Reader->Value;
		return ml_error("CBORError", "Extra bytes after decoding: %d", Extra);
	}
	return ml_cbor_reader_get(Reader);
}

ML_METHOD(CborDecode, MLAddressT, MLExternalSetT) {
//@cbor::decode
//<Bytes
//<Externals
//>any|error
// Decode :mini:`Bytes` into a Minilang value, or return an error if :mini:`Bytes` contains invalid CBOR or cannot be decoded into a Minilang value.
	ml_cbor_reader_t Reader[1] = {{0,}};
	Reader->TagFns = DefaultTagFns;
	Reader->GlobalGet = (ml_external_fn_t)ml_externals_get_value;
	Reader->Globals = Args[1];
	Reader->Reused = NULL;
	minicbor_stream_init(Reader->Stream);
	ml_cbor_reader_read(Reader, (const unsigned char *)ml_address_value(Args[0]), ml_address_length(Args[0]));
	int Extra = ml_cbor_reader_extra(Reader);
	if (Extra) {
		if (Reader->Value && ml_is_error(Reader->Value)) return Reader->Value;
		return ml_error("CBORError", "Extra bytes after decoding: %d", Extra);
	}
	return ml_cbor_reader_get(Reader);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Stream;
	typeof(ml_stream_read) *read;
	ml_cbor_reader_t Reader[1];
	unsigned char Chars[256];
} ml_cbor_decode_stream_t;

static void ml_cbor_decode_stream_run(ml_cbor_decode_stream_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	size_t Length = ml_integer_value(Value);
	if (!Length) ML_ERROR("CBORError", "Incomplete CBOR object");
	ml_cbor_reader_read(State->Reader, State->Chars, Length);
	if (State->Reader->Value) ML_RETURN(State->Reader->Value);
	size_t Required = State->Reader->Stream->Required;
	if (Required > 256) {
		return State->read((ml_state_t *)State, State->Stream, State->Chars, 256);
	} else if (Required > 0) {
		return State->read((ml_state_t *)State, State->Stream, State->Chars, Required);
	} else {
		return State->read((ml_state_t *)State, State->Stream, State->Chars, 1);
	}
}

ML_METHODX(CborDecode, MLStreamT) {
//@cbor::decode
//<Stream
//>any|error
	ml_value_t *Stream = Args[0];
	ml_cbor_decode_stream_t *State = new(ml_cbor_decode_stream_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_cbor_decode_stream_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Reader->TagFns = DefaultTagFns;
	State->Reader->GlobalGet = (ml_external_fn_t)ml_externals_get_value;
	State->Reader->Globals = MLExternals;
	State->Reader->Reused = NULL;
	minicbor_stream_init(State->Reader->Stream);
	return State->read((ml_state_t *)State, State->Stream, State->Chars, 1);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Values, *Callback, *Result;
	ml_value_t *Args[1];
	ml_cbor_reader_t Reader[1];
} ml_cbor_decoder_t;

static void ml_cbor_decoder_run(ml_cbor_decoder_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (!ml_list_length(State->Values)) ML_RETURN(State->Result);
	State->Args[0] = ml_list_pop(State->Values);
	return ml_call(State, State->Callback, 1, State->Args);
}

extern ml_type_t MLCborDecoderT[];

ML_FUNCTION(MLCborDecoder) {
//@cbor::decoder
//<Callback
//>cbor::decoder
// Returns a new CBOR decoder that calls :mini:`Callback(Value)` whenever a complete CBOR value is written to the decoder.
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLFunctionT);
	ml_cbor_decoder_t *Decoder = new(ml_cbor_decoder_t);
	Decoder->Base.Type = MLCborDecoderT;
	Decoder->Base.run = (ml_state_fn)ml_cbor_decoder_run;
	Decoder->Values = ml_list();
	Decoder->Callback = Args[0];
	Decoder->Reader->TagFns = DefaultTagFns;
	Decoder->Reader->GlobalGet = (ml_external_fn_t)ml_externals_get_value;
	Decoder->Reader->Globals = MLExternals;
	Decoder->Reader->Reused = NULL;
	minicbor_stream_init(Decoder->Reader->Stream);
	return (ml_value_t *)Decoder;
}

ML_TYPE(MLCborDecoderT, (MLStreamT), "cbor::decoder",
//@cbor::decoder
// A CBOR decoder that can be written to as a stream and calls a user-supplied callback whenever a complete value is decoded.
	.Constructor = (ml_value_t *)MLCborDecoder
);

static void ML_TYPED_FN(ml_stream_write, MLCborDecoderT, ml_state_t *Caller, ml_cbor_decoder_t *Decoder, const void *Address, int Count) {
	Decoder->Result = ml_integer(Count);
	ml_cbor_reader_t *Reader = Decoder->Reader;
	for (;;) {
		ml_cbor_reader_read(Reader, Address, Count);
		if (!Reader->Value) ML_RETURN(Decoder->Result);
		if (ml_is_error(Reader->Value)) ML_RETURN(Reader->Value);
		ml_list_put(Decoder->Values, Reader->Value);
		Reader->Value = NULL;
		int Extra = ml_cbor_reader_extra(Reader);
		Reader->Reused = NULL;
		minicbor_stream_init(Reader->Stream);
		if (!Extra) break;
		Address += (Count - Extra);
		Count = Extra;
	}
	if (!ml_list_length(Decoder->Values)) ML_RETURN(Decoder->Result);
	Decoder->Base.Caller = Caller;
	Decoder->Base.Context = Caller->Context;
	Decoder->Args[0] = ml_list_pop(Decoder->Values);
	return ml_call(Decoder, Decoder->Callback, 1, Decoder->Args);
}

static void ML_TYPED_FN(ml_stream_flush, MLCborDecoderT, ml_state_t *Caller, ml_cbor_decoder_t *Decoder) {
	int Extra = ml_cbor_reader_extra(Decoder->Reader);
	if (Extra) ML_ERROR("CBORError", "Extra bytes after decoding: %d", Extra);
	Decoder->Base.Caller = Caller;
	Decoder->Base.Context = Caller->Context;
	Decoder->Args[0] = ml_list_pop(Decoder->Values);
	return ml_call(Decoder, Decoder->Callback, 1, Decoder->Args);
}

struct ml_cbor_writer_t {
	void *Data;
	ml_cbor_write_fn WriteFn;
	ml_externals_t *Externals;
	inthash_t References[1];
	inthash_t Reused[1];
	ml_value_t *Error;
	jmp_buf OnError;
	int Index, NumSettings;
	unsigned char Buffer[9];
	void *Settings[];
};

int ml_cbor_write_raw(ml_cbor_writer_t *Writer, const void *Bytes, size_t Length) {
	int Result = Writer->WriteFn(Writer->Data, Bytes, Length);
	if (Result == -1) ML_CBOR_WRITER_ERROR(Writer, "CBORError", "Write error");
	return Result;
}

#include "minicbor/minicbor_writer.c"

void ml_cbor_write_integer(ml_cbor_writer_t *Writer, int64_t Number) {
	minicbor_write_integer(Writer, Number);
}

void ml_cbor_write_positive(ml_cbor_writer_t *Writer, uint64_t Number) {
	minicbor_write_positive(Writer, Number);
}

void ml_cbor_write_negative(ml_cbor_writer_t *Writer, uint64_t Number) {
	minicbor_write_negative(Writer, Number);
}

void ml_cbor_write_bytes(ml_cbor_writer_t *Writer, unsigned Size) {
	minicbor_write_bytes(Writer, Size);
}

void ml_cbor_write_indef_bytes(ml_cbor_writer_t *Writer) {
	minicbor_write_indef_bytes(Writer);
}

void ml_cbor_write_string(ml_cbor_writer_t *Writer, unsigned Size) {
	minicbor_write_string(Writer, Size);
}

void ml_cbor_write_indef_string(ml_cbor_writer_t *Writer) {
	minicbor_write_indef_string(Writer);
}

void ml_cbor_write_array(ml_cbor_writer_t *Writer, unsigned Size) {
	minicbor_write_array(Writer, Size);
}

void ml_cbor_write_indef_array(ml_cbor_writer_t *Writer) {
	minicbor_write_indef_array(Writer);
}

void ml_cbor_write_map(ml_cbor_writer_t *Writer, unsigned Size) {
	minicbor_write_map(Writer, Size);
}

void ml_cbor_write_indef_map(ml_cbor_writer_t *Writer) {
	minicbor_write_indef_map(Writer);
}

void ml_cbor_write_float2(ml_cbor_writer_t *Writer, double Number) {
	minicbor_write_float2(Writer, Number);
}

void ml_cbor_write_float4(ml_cbor_writer_t *Writer, double Number) {
	minicbor_write_float4(Writer, Number);
}

void ml_cbor_write_float8(ml_cbor_writer_t *Writer, double Number) {
	minicbor_write_float8(Writer, Number);
}

void ml_cbor_write_simple(ml_cbor_writer_t *Writer, unsigned char Simple) {
	minicbor_write_simple(Writer, Simple);
}

void ml_cbor_write_break(ml_cbor_writer_t *Writer) {
	minicbor_write_break(Writer);
}

void ml_cbor_write_tag(ml_cbor_writer_t *Writer, uint64_t Tag) {
	minicbor_write_tag(Writer, Tag);
}

ml_cbor_writer_t *ml_cbor_writer(void *Data, ml_cbor_write_fn WriteFn, ml_externals_t *Externals) {
	ml_cbor_writer_t *Writer = xnew(ml_cbor_writer_t, NumCborSettings, void *);
	Writer->Data = Data;
	Writer->WriteFn = WriteFn;
	Writer->Externals = Externals ?: MLExternals;
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

static int ml_cbor_writer_ref_fn(ml_cbor_writer_t *Writer, ml_value_t *Value, int HasRefs) {
	if (!HasRefs) return 0;
	if (inthash_insert(Writer->References, (uintptr_t)Value, Value)) {
		inthash_insert(Writer->Reused, (uintptr_t)Value, (void *)0L);
		return 0;
	}
	return 1;
}

void ml_cbor_writer_find_refs(ml_cbor_writer_t *Writer, ml_value_t *Value) {
	ml_value_find_all(Value, Writer, (ml_value_find_fn)ml_cbor_writer_ref_fn);
}

void ml_cbor_writer_error(ml_cbor_writer_t *Writer, ml_value_t *Error) {
	Writer->Error = Error;
	longjmp(Writer->OnError, 1);
}

void ml_cbor_write(ml_cbor_writer_t *Writer, ml_value_t *Value) {
	/*if (Value == MLNil) {
		minicbor_write_simple(Writer, CBOR_SIMPLE_NULL);
		return;
	}
	if (Value == (ml_value_t *)MLTrue) {
		minicbor_write_simple(Writer, CBOR_SIMPLE_TRUE);
		return;
	}
	if (Value == (ml_value_t *)MLFalse) {
		minicbor_write_simple(Writer, CBOR_SIMPLE_FALSE);
		return;
	}*/
	inthash_result_t Result = inthash_search2(Writer->Reused, (uintptr_t)Value);
	if (Result.Present) {
		if (Result.Value) {
			int Index = (uintptr_t)Result.Value - 1;
			minicbor_write_tag(Writer, ML_CBOR_TAG_USE_PREVIOUS);
			minicbor_write_integer(Writer, Index);
			return;
		}
		int Index = ++Writer->Index;
		inthash_insert(Writer->Reused, (uintptr_t)Value, (void *)(uintptr_t)Index);
		minicbor_write_tag(Writer, ML_CBOR_TAG_MARK_REUSED);
	}
	const char *Name = ml_externals_get_name(Writer->Externals, Value);
	if (Name) {
		minicbor_write_tag(Writer, ML_CBOR_TAG_USE_PREVIOUS);
		size_t Length = strlen(Name);
		minicbor_write_string(Writer, Length);
		Writer->WriteFn(Writer->Data, (const unsigned char *)Name, Length);
		return;
	}
	typeof(ml_cbor_write) *function = ml_typed_fn_get(ml_typeof(Value), ml_cbor_write);
	if (function) return function(Writer, Value);
	ml_value_t *Serialized = ml_serialize(Value);
	if (ml_is_error(Serialized)) {
		ml_value_t *Deref = ml_deref(Value);
		if (Deref != Value) return ml_cbor_write(Writer, Deref);
		ML_CBOR_WRITER_ERROR(Writer, "CBORError", "No method to encode %s to CBOR", ml_typeof(Value)->Name);
	} else {
		minicbor_write_tag(Writer, ML_CBOR_TAG_OBJECT);
		minicbor_write_array(Writer, ml_list_length(Serialized));
		ML_LIST_FOREACH(Serialized, Iter) ml_cbor_write(Writer, Iter->Value);
	}
}

ml_value_t *ml_cbor_try_write(ml_cbor_writer_t *Writer, ml_value_t *Value) {
	if (setjmp(Writer->OnError)) return Writer->Error;
	ml_cbor_write(Writer, Value);
	return NULL;
}

ml_cbor_t ml_cbor_encode(ml_value_t *Value) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_cbor_writer_t Writer[1];
	Writer->Data = Buffer;
	Writer->WriteFn = (void *)ml_stringbuffer_write;
	Writer->Externals = MLExternals;
	Writer->References[0] = INTHASH_INIT;
	Writer->Reused[0] = INTHASH_INIT;
	Writer->Index = 0;
	Writer->NumSettings = 0;
	if (setjmp(Writer->OnError)) return (ml_cbor_t){{.Error = Writer->Error}, 0};
	ml_cbor_writer_find_refs(Writer, Value);
	ml_cbor_write(Writer, Value);
	size_t Size = ml_stringbuffer_length(Buffer);
	if (!Size) return (ml_cbor_t){{.Error = ml_error("CBORError", "Empty CBOR encoding")}, 0};
	return (ml_cbor_t){{.Data = ml_stringbuffer_get_string(Buffer)}, Size};
}

ml_value_t *ml_cbor_encode_to(void *Data, ml_cbor_write_fn WriteFn, ml_externals_t *Externals, ml_value_t *Value) {
	ml_cbor_writer_t Writer[1];
	Writer->Data = Data;
	Writer->WriteFn = WriteFn;
	Writer->Externals = Externals;
	Writer->References[0] = INTHASH_INIT;
	Writer->Reused[0] = INTHASH_INIT;
	Writer->Index = 0;
	Writer->NumSettings = 0;
	if (setjmp(Writer->OnError)) return Writer->Error;
	ml_cbor_writer_find_refs(Writer, Value);
	ml_cbor_write(Writer, Value);
	return NULL;
}

ml_cbor_t ml_to_cbor(ml_value_t *Value) {
	return ml_cbor_encode(Value);
}

static void ML_TYPED_FN(ml_cbor_write, MLSomeT, ml_cbor_writer_t *Writer, ml_value_t *Global) {
	minicbor_write_tag(Writer, ML_CBOR_TAG_OBJECT);
	minicbor_write_array(Writer, 1);
	minicbor_write_string(Writer, 4);
	Writer->WriteFn(Writer->Data, (void *)"some", 4);
}

static void ML_TYPED_FN(ml_cbor_write, MLGlobalT, ml_cbor_writer_t *Writer, ml_value_t *Global) {
	ml_cbor_write(Writer, ml_global_get(Global));
}

static void ML_TYPED_FN(ml_cbor_write, MLIntegerT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	//printf("%s()\n", __func__);
	int64_t Value = ml_integer_value_fast(Arg);
	if (Value < 0) {
		minicbor_write_negative(Writer, ~Value);
	} else {
		minicbor_write_positive(Writer, Value);
	}
}

static void ML_TYPED_FN(ml_cbor_write, MLAddressT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	//printf("%s()\n", __func__);
	minicbor_write_bytes(Writer, ml_address_length(Arg));
	Writer->WriteFn(Writer->Data, (const unsigned char *)ml_address_value(Arg), ml_address_length(Arg));
}

static void ML_TYPED_FN(ml_cbor_write, MLStringT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	//printf("%s()\n", __func__);
	minicbor_write_string(Writer, ml_string_length(Arg));
	Writer->WriteFn(Writer->Data, (const unsigned char *)ml_string_value(Arg), ml_string_length(Arg));
}

static void ML_TYPED_FN(ml_cbor_write, MLRegexT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	//printf("%s()\n", __func__);
	const char *Pattern = ml_regex_pattern(Arg);
	minicbor_write_tag(Writer, ML_CBOR_TAG_REGEX);
	minicbor_write_string(Writer, strlen(Pattern));
	Writer->WriteFn(Writer->Data, (void *)Pattern, strlen(Pattern));
}

static void ML_TYPED_FN(ml_cbor_write, MLTupleT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_tag(Writer, ML_CBOR_TAG_OBJECT);
	int Size = ml_tuple_size(Arg);
	minicbor_write_array(Writer, 1 + Size);
	minicbor_write_string(Writer, 5);
	Writer->WriteFn(Writer->Data, (void *)"tuple", 5);
	for (int I = 1; I <= Size; ++I) ml_cbor_write(Writer, ml_tuple_get(Arg, I));
}

static void ML_TYPED_FN(ml_cbor_write, MLListT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_array(Writer, ml_list_length(Arg));
	ML_LIST_FOREACH(Arg, Node) ml_cbor_write(Writer, Node->Value);
}

static void ML_TYPED_FN(ml_cbor_write, MLMapT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_map(Writer, ml_map_size(Arg));
	ML_MAP_FOREACH(Arg, Node) {
		ml_cbor_write(Writer, Node->Key);
		ml_cbor_write(Writer, Node->Value);
	}
}

static void ML_TYPED_FN(ml_cbor_write, MLDoubleT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_float8(Writer, ml_double_value_fast(Arg));
}

static void ML_TYPED_FN(ml_cbor_write, MLNilT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_simple(Writer, CBOR_SIMPLE_NULL);
}

static void ML_TYPED_FN(ml_cbor_write, MLBooleanT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_simple(Writer, ml_boolean_value(Arg) ? CBOR_SIMPLE_TRUE : CBOR_SIMPLE_FALSE);
}

static void ML_TYPED_FN(ml_cbor_write, MLMethodT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	const char *Name = ml_method_name(Arg);
	minicbor_write_tag(Writer, ML_CBOR_TAG_IDENTIFIER);
	minicbor_write_string(Writer, strlen(Name));
	Writer->WriteFn(Writer->Data, (void *)Name, strlen(Name));
}

static void ML_TYPED_FN(ml_cbor_write, MLObjectT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_tag(Writer, ML_CBOR_TAG_OBJECT);
	int Size = ml_object_size(Arg);
	minicbor_write_array(Writer, 1 + Size);
	const char *Name = ml_typeof(Arg)->Name;
	minicbor_write_string(Writer, strlen(Name));
	Writer->WriteFn(Writer->Data, (void *)Name, strlen(Name));
	for (int I = 0; I < Size; ++I) ml_cbor_write(Writer, ml_object_field(Arg, I));
}

static void ML_TYPED_FN(ml_cbor_write, MLIntegerRangeT, ml_cbor_writer_t *Writer, ml_integer_range_t *Arg) {
	minicbor_write_tag(Writer, ML_CBOR_TAG_OBJECT);
	minicbor_write_array(Writer, 4);
	minicbor_write_string(Writer, 5);
	Writer->WriteFn(Writer->Data, (unsigned const char *)"range", 5);
	minicbor_write_integer(Writer, Arg->Start);
	minicbor_write_integer(Writer, Arg->Limit);
	minicbor_write_integer(Writer, Arg->Step);
}

static void ML_TYPED_FN(ml_cbor_write, MLIntegerIntervalT, ml_cbor_writer_t *Writer, ml_integer_interval_t *Arg) {
	minicbor_write_tag(Writer, ML_CBOR_TAG_OBJECT);
	minicbor_write_array(Writer, 3);
	minicbor_write_string(Writer, 5);
	Writer->WriteFn(Writer->Data, (unsigned const char *)"range", 5);
	minicbor_write_integer(Writer, Arg->Start);
	minicbor_write_integer(Writer, Arg->Limit);
}

static void ML_TYPED_FN(ml_cbor_write, MLRealRangeT, ml_cbor_writer_t *Writer, ml_real_range_t *Arg) {
	minicbor_write_tag(Writer, ML_CBOR_TAG_OBJECT);
	minicbor_write_array(Writer, 4);
	minicbor_write_string(Writer, 5);
	Writer->WriteFn(Writer->Data, (unsigned const char *)"range", 5);
	minicbor_write_float8(Writer, Arg->Start);
	minicbor_write_float8(Writer, Arg->Limit);
	minicbor_write_float8(Writer, Arg->Step);
}

static void ML_TYPED_FN(ml_cbor_write, MLRealIntervalT, ml_cbor_writer_t *Writer, ml_real_interval_t *Arg) {
	minicbor_write_tag(Writer, ML_CBOR_TAG_OBJECT);
	minicbor_write_array(Writer, 3);
	minicbor_write_string(Writer, 5);
	Writer->WriteFn(Writer->Data, (unsigned const char *)"range", 5);
	minicbor_write_float8(Writer, Arg->Start);
	minicbor_write_float8(Writer, Arg->Limit);
}

#ifdef ML_COMPLEX

#include <complex.h>
#undef I

ml_value_t *ml_cbor_read_complex(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (!ml_is(Value, MLListT)) return ml_error("TagError", "Complex requires list");
	if (ml_list_length(Value) != 2) return ml_error("TagError", "Complex requires 2 values");
	return ml_complex(ml_real_value(ml_list_get(Value, 1)) + ml_real_value(ml_list_get(Value, 2)) * _Complex_I);
}

static void ML_TYPED_FN(ml_cbor_write, MLComplexT, ml_cbor_writer_t *Writer, ml_complex_t *Arg) {
	minicbor_write_tag(Writer, ML_CBOR_TAG_COMPLEX);
	minicbor_write_array(Writer, 2);
	minicbor_write_float8(Writer, creal(Arg->Value));
	minicbor_write_float8(Writer, cimag(Arg->Value));
}

ML_FUNCTION(DecodeComplex) {
//!internal
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLRealT);
	ML_CHECK_ARG_TYPE(1, MLRealT);
	return ml_complex(ml_real_value(Args[0]) + ml_real_value(Args[1]) * _Complex_I);
}

#endif

#ifdef ML_DECIMAL

ml_value_t *ml_cbor_read_decimal(ml_cbor_reader_t *Reader, ml_value_t *Value) {
	if (!ml_is(Value, MLListT)) return ml_error("TagError", "Decimal requires list");
	if (ml_list_length(Value) != 2) return ml_error("TagError", "Decimal requires 2 values");
	ml_value_t *Unscaled = ml_list_get(Value, 1);
	if (!ml_is(Unscaled, MLIntegerT)) return ml_error("TagError", "Decimal requires integer unscaled value");
	return ml_decimal(Unscaled, ml_integer_value(ml_list_get(Value, 2)));
}

static void ML_TYPED_FN(ml_cbor_write, MLDecimalT, ml_cbor_writer_t *Writer, ml_decimal_t *Arg) {
	minicbor_write_tag(Writer, ML_CBOR_TAG_DECIMAL_FRACTION);
	minicbor_write_array(Writer, 2);
	ml_cbor_write(Writer, Arg->Unscaled);
	minicbor_write_integer(Writer, Arg->Scale);
}

ML_FUNCTION(DecodeDecimal) {
//!internal
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	ML_CHECK_ARG_TYPE(1, MLIntegerT);
	return ml_decimal(Args[0], ml_integer_value(Args[1]));
}

#endif

static void ML_TYPED_FN(ml_cbor_write, MLExternalT, ml_cbor_writer_t *Writer, ml_external_t *Arg) {
	minicbor_write_tag(Writer, ML_CBOR_TAG_USE_PREVIOUS);
	minicbor_write_string(Writer, Arg->Length);
	Writer->WriteFn(Writer->Data, (unsigned const char *)Arg->Name, Arg->Length);
}

static void ML_TYPED_FN(ml_cbor_write, MLSomeT, ml_cbor_writer_t *Writer, ml_value_t *Arg) {
	minicbor_write_tag(Writer, ML_CBOR_TAG_OBJECT);
	minicbor_write_array(Writer, 1);
	minicbor_write_string(Writer, 4);
	Writer->WriteFn(Writer->Data, (unsigned const char *)"some", 4);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value;
	uint64_t Tag;
} cbor_tag_t;

extern ml_type_t CborTagT[];

ML_FUNCTION(CborTag) {
//@cbor::tag
//<Tag
//>cbor::tag
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	cbor_tag_t *Tag = new(cbor_tag_t);
	Tag->Type = CborTagT;
	Tag->Value = Args[1];
	Tag->Tag = ml_integer_value(Args[0]);
	return (ml_value_t *)Tag;
}

ML_TYPE(CborTagT, (), "cbor::tag",
//@cbor::tag
	.Constructor = (ml_value_t *)CborTag
);

static void ML_TYPED_FN(ml_cbor_write, CborTagT, ml_cbor_writer_t *Writer, cbor_tag_t *Arg) {
	minicbor_write_tag(Writer, Arg->Tag);
	ml_cbor_write(Writer, Arg->Value);
}

#define CBOR_WRITE_NONE(NAME, CNAME) \
\
ML_FUNCTION(CborWrite ## NAME) { \
/*@cbor::write_CNAME
//<Buffer:string::buffer
//>Buffer
*/ \
	ML_CHECK_ARG_COUNT(1); \
	ML_CHECK_ARG_TYPE(0, MLStringBufferT); \
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0]; \
	ml_cbor_writer_t Writer[1] = {{Buffer, (void *)ml_stringbuffer_write}}; \
	minicbor_write_ ## CNAME(Writer); \
	return (ml_value_t *)Buffer; \
}

CBOR_WRITE_NONE(IndefBytes, indef_bytes)
CBOR_WRITE_NONE(IndefString, indef_string)
CBOR_WRITE_NONE(IndefArray, indef_array)
CBOR_WRITE_NONE(IndefMap, indef_map)
CBOR_WRITE_NONE(Break, break)

#define CBOR_WRITE_INTEGER(NAME, CNAME) \
\
ML_FUNCTION(CborWrite ## NAME) { \
/*@cbor::write_CNAME
//<Buffer:string::buffer
//<Value:integer
//>Buffer
*/ \
	ML_CHECK_ARG_COUNT(2); \
	ML_CHECK_ARG_TYPE(0, MLStringBufferT); \
	ML_CHECK_ARG_TYPE(1, MLIntegerT); \
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0]; \
	ml_cbor_writer_t Writer[1] = {{Buffer, (void *)ml_stringbuffer_write}}; \
	minicbor_write_ ## CNAME(Writer, ml_integer_value(Args[1])); \
	return (ml_value_t *)Buffer; \
}

CBOR_WRITE_INTEGER(Integer, integer)
CBOR_WRITE_INTEGER(Positive, positive)
CBOR_WRITE_INTEGER(Negative, negative)
CBOR_WRITE_INTEGER(Bytes, bytes)
CBOR_WRITE_INTEGER(String, string)
CBOR_WRITE_INTEGER(Array, array)
CBOR_WRITE_INTEGER(Map, map)
CBOR_WRITE_INTEGER(Tag, tag)

#define CBOR_WRITE_REAL(NAME, CNAME) \
\
ML_FUNCTION(CborWrite ## NAME) { \
/*@cbor::write_CNAME
//<Buffer:string::buffer
//<Value:integer
//>Buffer
*/ \
	ML_CHECK_ARG_COUNT(2); \
	ML_CHECK_ARG_TYPE(0, MLStringBufferT); \
	ML_CHECK_ARG_TYPE(1, MLRealT); \
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0]; \
	ml_cbor_writer_t Writer[1] = {{Buffer, (void *)ml_stringbuffer_write}}; \
	minicbor_write_ ## CNAME(Writer, ml_real_value(Args[1])); \
	return (ml_value_t *)Buffer; \
}

CBOR_WRITE_REAL(Float2, float2)
CBOR_WRITE_REAL(Float4, float4)
CBOR_WRITE_REAL(Float8, float8)

ML_FUNCTION(CborWriteSimple) {
//@cbor::write_simple
//<Buffer:string::buffer
//<Value:boolean|nil
//>Buffer
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringBufferT);
	unsigned char Simple;
	if (Args[1] == (ml_value_t *)MLFalse) {
		Simple = CBOR_SIMPLE_FALSE;
	} else if (Args[1] == (ml_value_t *)MLTrue) {
		Simple = CBOR_SIMPLE_TRUE;
	} else if (Args[1] == (ml_value_t *)MLNil) {
		Simple = CBOR_SIMPLE_NULL;
	} else {
		return ml_error("CBORError", "Unknown simple value");
	}
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_cbor_writer_t Writer[1] = {{Buffer, (void *)ml_stringbuffer_write}};
	minicbor_write_simple(Writer, Simple);
	return (ml_value_t *)Buffer;
}

ML_METHOD_ANON(CborEncode, "cbor::encode");

ML_METHOD(CborEncode, MLAnyT) {
//@cbor::encode
//<Value
//>address|error
// Encode :mini:`Value` into CBOR or return an error if :mini:`Value` cannot be encoded.
	ml_cbor_t Cbor = ml_cbor_encode(Args[0]);
	if (!Cbor.Length) return Cbor.Error;
	if (Cbor.Data) return ml_address(Cbor.Data, Cbor.Length);
	return ml_error("CborError", "Error encoding to cbor");
}

ML_METHOD(CborEncode, MLAnyT, MLStringBufferT) {
//@cbor::encode
//<Value
//<Buffer
//>address|error
// Encode :mini:`Value` into CBOR or return an error if :mini:`Value` cannot be encoded.
	ml_value_t *Value = Args[0];
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[1];
	ml_cbor_writer_t Writer[1];
	Writer->Data = Buffer;
	Writer->WriteFn = (void *)ml_stringbuffer_write;
	Writer->Externals = MLExternals;
	Writer->References[0] = INTHASH_INIT;
	Writer->Reused[0] = INTHASH_INIT;
	Writer->Index = 0;
	Writer->NumSettings = 0;
	if (setjmp(Writer->OnError)) return Writer->Error;
	ml_cbor_writer_find_refs(Writer, Value);
	ml_cbor_write(Writer, Value);
	return (ml_value_t *)Buffer;
}

ML_METHOD(CborEncode, MLAnyT, MLExternalSetT) {
//@cbor::encode
//<Value
//<Externals
//>address|error
// Encode :mini:`Value` into CBOR or return an error if :mini:`Value` cannot be encoded.
	ml_value_t *Value = Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_cbor_writer_t Writer[1];
	Writer->Data = Buffer;
	Writer->WriteFn = (void *)ml_stringbuffer_write;
	Writer->Externals = (ml_externals_t *)Args[1];
	Writer->References[0] = INTHASH_INIT;
	Writer->Reused[0] = INTHASH_INIT;
	Writer->Index = 0;
	Writer->NumSettings = 0;
	if (setjmp(Writer->OnError)) return Writer->Error;
	ml_cbor_writer_find_refs(Writer, Value);
	ml_cbor_write(Writer, Value);
	int Length = ml_stringbuffer_length(Buffer);
	return ml_address(ml_stringbuffer_get_string(Buffer), Length);
}

ML_METHOD(CborEncode, MLAnyT, MLStringBufferT, MLExternalSetT) {
//@cbor::encode
//<Value
//<Buffer
//<Externals
//>address|error
// Encode :mini:`Value` into CBOR or return an error if :mini:`Value` cannot be encoded.
	ml_value_t *Value = Args[0];
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[1];
	ml_cbor_writer_t Writer[1];
	Writer->Data = Buffer;
	Writer->WriteFn = (void *)ml_stringbuffer_write;
	Writer->Externals = (ml_externals_t *)Args[2];
	Writer->References[0] = INTHASH_INIT;
	Writer->Reused[0] = INTHASH_INIT;
	Writer->Index = 0;
	Writer->NumSettings = 0;
	if (setjmp(Writer->OnError)) return Writer->Error;
	ml_cbor_writer_find_refs(Writer, Value);
	ml_cbor_write(Writer, Value);
	return (ml_value_t *)Buffer;
}

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
	const char *Type = ml_string_value(TypeName);
	int Count = ml_list_length(Value) - 1;
	ml_value_t **Args = ml_alloc_args(Count);
	for (int I = 0; I < Count; ++I) {
		ml_list_iter_next(Iter);
		Args[I] = Iter->Value;
	}
	ml_value_t *Constructor = stringmap_search(CborObjectTypes, Type);
	if (Constructor) return ml_simple_call(Constructor, Count, Args);
	return ml_deserialize(Type, Count, Args);
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
	ml_cbor_default_tag(ML_CBOR_TAG_COMPLEX, ml_cbor_read_complex);
#endif
#ifdef ML_DECIMAL
	ml_cbor_default_object("decimal", (ml_value_t *)DecodeDecimal);
	ml_cbor_default_tag(ML_CBOR_TAG_DECIMAL_FRACTION, ml_cbor_read_decimal);
#endif
	ml_cbor_default_tag(ML_CBOR_TAG_REGEX, ml_cbor_read_regex);
	ml_cbor_default_tag(ML_CBOR_TAG_IDENTIFIER, ml_cbor_read_method);
	ml_cbor_default_tag(ML_CBOR_TAG_OBJECT, ml_cbor_read_object);
	ml_cbor_default_tag(ML_CBOR_TAG_MARK_REUSED, ml_cbor_mark_reused);
	ml_cbor_default_tag(ML_CBOR_TAG_USE_PREVIOUS, ml_cbor_use_previous);
#include "ml_cbor_init.c"
	if (Globals) {
		stringmap_insert(Globals, "cbor", ml_module("cbor",
			"encode", CborEncode,
			"decode", CborDecode,
			"decoder", MLCborDecoderT,
			"tag", CborTagT,
			"write_integer", CborWriteInteger,
			"write_positive", CborWritePositive,
			"write_negative", CborWriteNegative,
			"write_bytes", CborWriteBytes,
			"write_string", CborWriteString,
			"write_array", CborWriteArray,
			"write_map", CborWriteMap,
			"write_tag", CborWriteTag,
			"write_indef_bytes", CborWriteIndefBytes,
			"write_indef_string", CborWriteIndefString,
			"write_indef_array", CborWriteIndefArray,
			"write_indef_map", CborWriteIndefMap,
			"write_break", CborWriteBreak,
			"write_simple", CborWriteSimple,
			"write_float2", CborWriteFloat2,
			"write_float4", CborWriteFloat4,
			"write_float8", CborWriteFloat8,
		NULL));
	}
}

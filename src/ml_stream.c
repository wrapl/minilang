#include "ml_macros.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "ml_stream.h"
#include "ml_object.h"

#undef ML_CATEGORY
#define ML_CATEGORY "stream"

ML_INTERFACE(MLStreamT, (), "stream");
// Base type of readable and writable byte streams.

ML_METHOD_DECL(ModeMethod, "mode");
ML_METHOD_DECL(ReadMethod, "read");
ML_METHOD_DECL(WriteMethod, "write");
ML_METHOD_DECL(FlushMethod, "flush");
ML_METHOD_DECL(SeekMethod, "seek");
ML_METHOD_DECL(TellMethod, "tell");
ML_METHOD_DECL(CloseMethod, "close");

void ml_stream_mode_method(ml_state_t *Caller, ml_value_t *Value) {
	ml_value_t **Args = ml_alloc_args(1);
	Args[0] = Value;
	return ml_call(Caller, ModeMethod, 1, Args);
}

void ml_stream_mode(ml_state_t *Caller, ml_value_t *Value) {
	typeof(ml_stream_mode) *function = ml_typed_fn_get(ml_typeof(Value), ml_stream_mode) ?: ml_stream_mode_method;
	return function(Caller, Value);
}

void ml_stream_read_method(ml_state_t *Caller, ml_value_t *Value, void *Address, int Count) {
	ml_address_t *Buffer = new(ml_address_t);
	Buffer->Type = MLBufferT;
	Buffer->Value = Address;
	Buffer->Length = Count;
	ml_value_t **Args = ml_alloc_args(2);
	Args[0] = Value;
	Args[1] = (ml_value_t *)Buffer;
	return ml_call(Caller, ReadMethod, 2, Args);
}

void ml_stream_read(ml_state_t *Caller, ml_value_t *Value, void *Address, int Count) {
	typeof(ml_stream_read) *function = ml_typed_fn_get(ml_typeof(Value), ml_stream_read) ?: ml_stream_read_method;
	return function(Caller, Value, Address, Count);
}

void ml_stream_write_method(ml_state_t *Caller, ml_value_t *Value, const void *Address, int Count) {
	ml_address_t *Buffer = new(ml_address_t);
	Buffer->Type = MLAddressT;
	Buffer->Value = (void *)Address;
	Buffer->Length = Count;
	ml_value_t **Args = ml_alloc_args(2);
	Args[0] = Value;
	Args[1] = (ml_value_t *)Buffer;
	return ml_call(Caller, WriteMethod, 2, Args);
}

void ml_stream_write(ml_state_t *Caller, ml_value_t *Value, const void *Address, int Count) {
	typeof(ml_stream_write) *function = ml_typed_fn_get(ml_typeof(Value), ml_stream_write) ?: ml_stream_write_method;
	return function(Caller, Value, Address, Count);
}

void ml_stream_flush_method(ml_state_t *Caller, ml_value_t *Value) {
	ml_value_t **Args = ml_alloc_args(1);
	Args[0] = Value;
	return ml_call(Caller, FlushMethod, 1, Args);
}

void ml_stream_flush(ml_state_t *Caller, ml_value_t *Value) {
	typeof(ml_stream_flush) *function = ml_typed_fn_get(ml_typeof(Value), ml_stream_flush) ?: ml_stream_flush_method;
	return function(Caller, Value);
}

ML_ENUM2(MLStreamSeekT, "stream::seek",
	"Set", SEEK_SET,
	"Cur", SEEK_CUR,
	"End", SEEK_END
);

void ml_stream_seek_method(ml_state_t *Caller, ml_value_t *Value, int64_t Offset, int Mode) {
	ml_value_t **Args = ml_alloc_args(3);
	Args[0] = Value;
	Args[1] = ml_integer(Offset);
	ml_value_t *SeekMode = ml_enum_value(MLStreamSeekT, Mode);
	if (ml_is_error(SeekMode)) ML_RETURN(SeekMode);
	Args[2] = SeekMode;
	return ml_call(Caller, SeekMethod, 2, Args);
}

void ml_stream_seek(ml_state_t *Caller, ml_value_t *Value, int64_t Offset, int Mode) {
	typeof(ml_stream_seek) *function = ml_typed_fn_get(ml_typeof(Value), ml_stream_seek) ?: ml_stream_seek_method;
	return function(Caller, Value, Offset, Mode);
}

void ml_stream_tell_method(ml_state_t *Caller, ml_value_t *Value) {
	ml_value_t **Args = ml_alloc_args(1);
	Args[0] = Value;
	return ml_call(Caller, TellMethod, 1, Args);
}

void ml_stream_tell(ml_state_t *Caller, ml_value_t *Value) {
	typeof(ml_stream_tell) *function = ml_typed_fn_get(ml_typeof(Value), ml_stream_tell) ?: ml_stream_tell_method;
	return function(Caller, Value);
}

void ml_stream_close_method(ml_state_t *Caller, ml_value_t *Value) {
	ml_value_t **Args = ml_alloc_args(1);
	Args[0] = Value;
	return ml_call(Caller, CloseMethod, 1, Args);
}

void ml_stream_close(ml_state_t *Caller, ml_value_t *Value) {
	typeof(ml_stream_close) *function = ml_typed_fn_get(ml_typeof(Value), ml_stream_close) ?: ml_stream_close_method;
	return function(Caller, Value);
}

ML_METHODX("mode", MLStreamT) {
//<Stream
//>mode
	ml_value_t *Stream = Args[0];
	typeof(ml_stream_mode) *encoding = ml_typed_fn_get(ml_typeof(Stream), ml_stream_mode);
	if (!encoding) ML_RETURN(MLStringT);
	return encoding(Caller, Stream);
}

ML_METHODX("read", MLStreamT, MLBufferT) {
//<Stream
//<Buffer
//>integer
// Reads bytes from :mini:`Stream` into :mini:`Buffer` to :mini:`Stream`. This method should be overridden for streams defined in Minilang.
	ml_value_t *Stream = Args[0];
	typeof(ml_stream_read) *read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read);
	if (!read) ML_ERROR("StreamError", "No read method defined for %s", ml_typeof(Args[0])->Name);
	return read(Caller, Stream, ml_buffer_value(Args[1]), ml_buffer_length(Args[1]));
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Stream;
	char *Space;
	typeof(ml_stream_read) *read;
	ml_value_t *(*finish)(ml_stringbuffer_t *);
	ml_stringbuffer_t Buffer[1];
	size_t Remaining;
	int UTF8Length;
	unsigned char Chars[];
} ml_read_state_t;

#define UTF8_COUNT(CHARS, LENGTH) { \
	int UTF8Length = State->UTF8Length; \
	size_t Length0 = LENGTH; \
	unsigned char *Bytes = (unsigned char *)CHARS; \
	do { \
		unsigned char Byte = *Bytes++; \
		if (UTF8Length) { \
			if ((Byte & 128) == 128) { \
				if (--UTF8Length == 0) --Remaining; \
			} else { \
				ML_ERROR("ReadError", "Invalid UTF-8 sequence"); \
			} \
		} else if ((Byte & 128) == 0) { \
			--Remaining; \
		} else if ((Byte & 240) == 240) { \
			UTF8Length = 3; \
		} else if ((Byte & 224) == 224) { \
			UTF8Length = 2; \
		} else if ((Byte & 192) == 192) { \
			UTF8Length = 1; \
		} else { \
			ML_ERROR("ReadError", "Invalid UTF-8 sequence"); \
		} \
	} while (--Length0 > 0); \
	State->UTF8Length = UTF8Length; \
}

static void ml_stream_read_run(ml_read_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	size_t Length = ml_integer_value(Value);
	if (!Length) ML_RETURN(State->Buffer->Length ? State->finish(State->Buffer) : MLNil);
	char *Space = ml_stringbuffer_writer(State->Buffer, Length);
	size_t Remaining = State->Remaining;
	if (State->finish == ml_stringbuffer_to_string) {
		UTF8_COUNT(State->Space, Length);
	} else {
		Remaining -= Length;
	}
	if (!Remaining) ML_RETURN(State->finish(State->Buffer));
	State->Space = Space;
	State->Remaining = Remaining;
	if (State->UTF8Length > 1) Remaining += (State->UTF8Length - 1);
	if (Remaining > State->Buffer->Space) {
		return State->read((ml_state_t *)State, State->Stream, Space, State->Buffer->Space);
	} else {
		return State->read((ml_state_t *)State, State->Stream, Space, Remaining);
	}
}

static void ml_stream_mode_read_run(ml_read_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (Value == (ml_value_t *)MLStringT) {
		State->finish = ml_stringbuffer_to_string;
	} else if (Value == (ml_value_t *)MLAddressT) {
		State->finish = ml_stringbuffer_to_address;
	} else if (Value == (ml_value_t *)MLBufferT) {
		State->finish = ml_stringbuffer_to_buffer;
	} else {
		ML_ERROR("StreamError", "Invalid mode for stream");
	}
	State->Base.run = (ml_state_fn)ml_stream_read_run;
	char *Space = State->Space = ml_stringbuffer_writer(State->Buffer, 0);
	if (State->Remaining > State->Buffer->Space) {
		return State->read((ml_state_t *)State, State->Stream, Space, State->Buffer->Space);
	} else {
		return State->read((ml_state_t *)State, State->Stream, Space, State->Remaining);
	}
}

ML_METHODX("read", MLStreamT, MLIntegerT) {
//<Stream
//<Count
//>string|nil
// Returns the next text from :mini:`Stream` upto :mini:`Count` characters. Returns :mini:`nil` if :mini:`Stream` is empty.
	ml_value_t *Stream = Args[0];
	ml_read_state_t *State = xnew(ml_read_state_t, 0, char);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_mode_read_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	State->Remaining = ml_integer_value(Args[1]);
	return ml_stream_mode((ml_state_t *)State, Stream);
}

static void ml_stream_readx_run(ml_read_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	size_t Length = ml_integer_value(Value);
	if (!Length) ML_RETURN(State->Buffer->Length ? State->finish(State->Buffer) : MLNil);
	unsigned char Char = State->Chars[64];
	if (State->Chars[Char / 8] & (1 << (Char % 8))) ML_RETURN(State->finish(State->Buffer));
	ml_stringbuffer_write(State->Buffer, (void *)&State->Chars[64], 1);
	size_t Remaining = State->Remaining;
	if (State->finish == ml_stringbuffer_to_string) {
		UTF8_COUNT(State->Chars + 64, 1);
	} else {
		Remaining -= Length;
	}
	if (!Remaining) ML_RETURN(State->finish(State->Buffer));
	State->Remaining = Remaining;
	ml_value_t *Stream = State->Stream;
	return State->read((ml_state_t *)State, Stream, &State->Chars[64], 1);
}

static void ml_stream_mode_readx_run(ml_read_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (Value == (ml_value_t *)MLStringT) {
		State->finish = ml_stringbuffer_to_string;
	} else if (Value == (ml_value_t *)MLAddressT) {
		State->finish = ml_stringbuffer_to_address;
	} else if (Value == (ml_value_t *)MLBufferT) {
		State->finish = ml_stringbuffer_to_buffer;
	} else {
		ML_ERROR("StreamError", "Invalid mode for stream");
	}
	State->Base.run = (ml_state_fn)ml_stream_readx_run;
	return State->read((ml_state_t *)State, State->Stream, (void *)&State->Chars[64], 1);
}

ML_METHODX("readx", MLStreamT, MLStringT) {
//<Stream
//<Delimiters
//>string|nil
// Returns the next text from :mini:`Stream`, upto but excluding any character in :mini:`Delimiters`. Returns :mini:`nil` if :mini:`Stream` is empty.
	ml_value_t *Stream = Args[0];
	ml_read_state_t *State = xnew(ml_read_state_t, 65, char);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_mode_readx_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	const unsigned char *Terms = (const unsigned char *)ml_string_value(Args[1]);
	int TermsLength = ml_string_length(Args[1]);
	for (int I = 0; I < TermsLength; ++I) {
		unsigned char Char = Terms[I];
		State->Chars[Char / 8] |= 1 << (Char % 8);
	}
	State->Remaining = SIZE_MAX;
	return ml_stream_mode((ml_state_t *)State, Stream);
}

ML_METHODX("readx", MLStreamT, MLStringT, MLIntegerT) {
//<Stream
//<Delimiters
//<Count
//>string|nil
// Returns the next text from :mini:`Stream`, upto but excluding any character in :mini:`Delimiters` or :mini:`Count` characters, whichever comes first. Returns :mini:`nil` if :mini:`Stream` is empty.
	ml_value_t *Stream = Args[0];
	ml_read_state_t *State = xnew(ml_read_state_t, 65, char);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_mode_readx_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	const unsigned char *Terms = (const unsigned char *)ml_string_value(Args[1]);
	int TermsLength = ml_string_length(Args[1]);
	for (int I = 0; I < TermsLength; ++I) {
		unsigned char Char = Terms[I];
		State->Chars[Char / 8] |= 1 << (Char % 8);
	}
	State->Remaining = ml_integer_value(Args[2]);
	return ml_stream_mode((ml_state_t *)State, Stream);
}

static void ml_stream_readi_run(ml_read_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	size_t Length = ml_integer_value(Value);
	if (!Length) ML_RETURN(State->Buffer->Length ? State->finish(State->Buffer) : MLNil);
	ml_stringbuffer_write(State->Buffer, (void *)&State->Chars[64], 1);
	unsigned char Char = State->Chars[64];
	if (State->Chars[Char / 8] & (1 << (Char % 8))) ML_RETURN(State->finish(State->Buffer));
	size_t Remaining = State->Remaining;
	if (State->finish == ml_stringbuffer_to_string) {
		UTF8_COUNT(State->Chars + 64, 1);
	} else {
		Remaining -= Length;
	}
	if (!Remaining) ML_RETURN(State->finish(State->Buffer));
	State->Remaining = Remaining;
	ml_value_t *Stream = State->Stream;
	return State->read((ml_state_t *)State, Stream, &State->Chars[64], 1);
}

static void ml_stream_mode_readi_run(ml_read_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (Value == (ml_value_t *)MLStringT) {
		State->finish = ml_stringbuffer_to_string;
	} else if (Value == (ml_value_t *)MLAddressT) {
		State->finish = ml_stringbuffer_to_address;
	} else if (Value == (ml_value_t *)MLBufferT) {
		State->finish = ml_stringbuffer_to_buffer;
	} else {
		ML_ERROR("StreamError", "Invalid mode for stream");
	}
	State->Base.run = (ml_state_fn)ml_stream_readi_run;
	return State->read((ml_state_t *)State, State->Stream, (void *)&State->Chars[64], 1);
}

ML_METHODX("readi", MLStreamT, MLStringT) {
//<Stream
//<Delimiters
//>string|nil
// Returns the next text from :mini:`Stream`, upto and including any character in :mini:`Delimiters`. Returns :mini:`nil` if :mini:`Stream` is empty.
	ml_value_t *Stream = Args[0];
	ml_read_state_t *State = xnew(ml_read_state_t, 65, char);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_mode_readi_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	const unsigned char *Terms = (const unsigned char *)ml_string_value(Args[1]);
	int TermsLength = ml_string_length(Args[1]);
	for (int I = 0; I < TermsLength; ++I) {
		unsigned char Char = Terms[I];
		State->Chars[Char / 8] |= 1 << (Char % 8);
	}
	State->Remaining = SIZE_MAX;
	return ml_stream_mode((ml_state_t *)State, Stream);
}

ML_METHODX("readi", MLStreamT, MLStringT, MLIntegerT) {
//<Stream
//<Delimiters
//<Count
//>string|nil
// Returns the next text from :mini:`Stream`, upto and including any character in :mini:`Delimiters` or :mini:`Count` characters, whichever comes first. Returns :mini:`nil` if :mini:`Stream` is empty.
	ml_value_t *Stream = Args[0];
	ml_read_state_t *State = xnew(ml_read_state_t, 65, char);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_mode_readi_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	const unsigned char *Terms = (const unsigned char *)ml_string_value(Args[1]);
	int TermsLength = ml_string_length(Args[1]);
	for (int I = 0; I < TermsLength; ++I) {
		unsigned char Char = Terms[I];
		State->Chars[Char / 8] |= 1 << (Char % 8);
	}
	State->Remaining = ml_integer_value(Args[2]);
	return ml_stream_mode((ml_state_t *)State, Stream);
}

ML_METHODX("read", MLStreamT) {
//<Stream
//>string|nil
// Equivalent to :mini:`Stream:readi(SIZE_MAX, '\n')`.
	ml_value_t *Stream = Args[0];
	ml_read_state_t *State = xnew(ml_read_state_t, 65, char);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_mode_readi_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	State->Remaining = SIZE_MAX;
	State->Chars['\n' / 8] |= 1 << ('\n' % 8);
	return ml_stream_mode((ml_state_t *)State, Stream);
}

static void ml_stream_rest_run(ml_read_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	size_t Length = ml_integer_value(Value);
	if (!Length) ML_RETURN(State->Buffer->Length ? State->finish(State->Buffer) : ml_cstring(""));
	char *Space = ml_stringbuffer_writer(State->Buffer, Length);
	ml_value_t *Stream = State->Stream;
	return State->read((ml_state_t *)State, Stream, Space, State->Buffer->Space);
}

static void ml_stream_mode_rest_run(ml_read_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (Value == (ml_value_t *)MLStringT) {
		State->finish = ml_stringbuffer_to_string;
	} else if (Value == (ml_value_t *)MLAddressT) {
		State->finish = ml_stringbuffer_to_address;
	} else if (Value == (ml_value_t *)MLBufferT) {
		State->finish = ml_stringbuffer_to_buffer;
	} else {
		ML_ERROR("StreamError", "Invalid mode for stream");
	}
	State->Base.run = (ml_state_fn)ml_stream_rest_run;
	char *Space = ml_stringbuffer_writer(State->Buffer, 0);
	return State->read((ml_state_t *)State, State->Stream, Space, State->Buffer->Space);
}

ML_METHODX("rest", MLStreamT) {
//<Stream
//>string|nil
// Returns the remainder of :mini:`Stream` or :mini:`nil` if :mini:`Stream` is empty.
	ml_value_t *Stream = Args[0];
	ml_read_state_t *State = xnew(ml_read_state_t, 0, char);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_mode_rest_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	return ml_stream_mode((ml_state_t *)State, Stream);
}

ML_METHODX("write", MLStreamT, MLAddressT) {
//<Stream
//<Address
//>integer
// Writes the bytes at :mini:`Address` to :mini:`Stream`. This method should be overridden for streams defined in Minilang.
	ml_value_t *Stream = Args[0];
	typeof(ml_stream_write) *write = ml_typed_fn_get(ml_typeof(Stream), ml_stream_write);
	if (!write) ML_ERROR("StreamError", "No write method defined for %s", ml_typeof(Args[0])->Name);
	return write(Caller, Stream, ml_address_value(Args[1]), ml_address_length(Args[1]));
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Stream;
	typeof(ml_stream_write) *write;
	ml_stringbuffer_t Buffer[1];
	int Index, Count;
	size_t Length;
	ml_value_t *Args[];
} ml_write_state_t;

static void ml_stream_write_run(ml_write_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_stringbuffer_t *Buffer = State->Buffer;
	size_t Length = ml_integer_value(Value);
	State->Length += Length;
	Length = ml_stringbuffer_reader(Buffer, Length);
	if (!Length) ML_CONTINUE(State->Base.Caller, ml_integer(State->Length));
	return State->write((ml_state_t *)State, State->Stream, Buffer->Head->Chars + Buffer->Start, Length);
}

static void ml_stream_write_append_run(ml_write_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_stringbuffer_t *Buffer = State->Buffer;
	if (++State->Index < State->Count) return ml_stringbuffer_append((ml_state_t *)State, Buffer, State->Args[State->Index]);
	State->Base.run = (ml_state_fn)ml_stream_write_run;
	size_t Length = ml_stringbuffer_reader(Buffer, 0);
	if (!Length) ML_CONTINUE(State->Base.Caller, ml_integer(0));
	return State->write((ml_state_t *)State, State->Stream, Buffer->Head->Chars + Buffer->Start, Length);
}

ML_METHODVX("write", MLStreamT, MLAnyT) {
//<Stream
//<Value/1,...,Value/n
//>integer
// Writes each :mini:`Value/i` in turn to :mini:`Stream`.
	ml_value_t *Stream = Args[0];
	ml_write_state_t *State = xnew(ml_write_state_t, Count - 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_write_append_run;
	State->Stream = Stream;
	State->write = ml_typed_fn_get(ml_typeof(Stream), ml_stream_write) ?: ml_stream_write_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	State->Index = 0;
	State->Count = Count - 1;
	for (int I = 1; I < Count; ++I) State->Args[I - 1] = Args[I];
	return ml_stringbuffer_append((ml_state_t *)State, State->Buffer, State->Args[0]);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Source, *Destination;
	typeof(ml_stream_read) *read;
	typeof(ml_stream_write) *write;
	size_t Length, Start, Total, Remaining;
	char Buffer[ML_STRINGBUFFER_NODE_SIZE];
} ml_stream_copy_state_t;

static void ml_stream_copy_read_run(ml_stream_copy_state_t *State, ml_value_t *Value);

static void ml_stream_copy_write_run(ml_stream_copy_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	size_t Length = ml_integer_value(Value);
	State->Total += Length;
	State->Length -= Length;
	State->Remaining -= Length;
	if (!State->Length) {
		if (!State->Remaining) ML_CONTINUE(State->Base.Caller, ml_integer(State->Total));
		State->Base.run = (ml_state_fn)ml_stream_copy_read_run;
		size_t Read = State->Remaining < ML_STRINGBUFFER_NODE_SIZE ? State->Remaining : ML_STRINGBUFFER_NODE_SIZE;
		return State->read((ml_state_t *)State, State->Source, State->Buffer, Read);
	}
	State->Start += Length;
	return State->write((ml_state_t *)State, State->Destination, State->Buffer + State->Start, State->Length);
}

static void ml_stream_copy_read_run(ml_stream_copy_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	size_t Length = ml_integer_value(Value);
	if (!Length) ML_CONTINUE(State->Base.Caller, ml_integer(State->Total));
	State->Base.run = (ml_state_fn)ml_stream_copy_write_run;
	State->Length = Length;
	State->Start = 0;
	return State->write((ml_state_t *)State, State->Destination, State->Buffer, Length);
}

ML_METHODX("copy", MLStreamT, MLStreamT) {
//<Source
//<Destination
//>integer
// Copies the remaining bytes from :mini:`Source` to :mini:`Destination`.
	ml_stream_copy_state_t *State = new(ml_stream_copy_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	ml_value_t *Source = State->Source = Args[0];
	ml_value_t *Destination = State->Destination = Args[1];
	State->read = ml_typed_fn_get(ml_typeof(Source), ml_stream_read) ?: ml_stream_read_method;
	State->write = ml_typed_fn_get(ml_typeof(Destination), ml_stream_write) ?: ml_stream_write_method;
	State->Base.run = (ml_state_fn)ml_stream_copy_read_run;
	State->Remaining = SIZE_MAX;
	return State->read((ml_state_t *)State, Source, State->Buffer, ML_STRINGBUFFER_NODE_SIZE);
}

ML_METHODX("copy", MLStreamT, MLStreamT, MLIntegerT) {
//<Source
//<Destination
//<Count
//>integer
// Copies upto :mini:`Count` bytes from :mini:`Source` to :mini:`Destination`.
	ml_stream_copy_state_t *State = new(ml_stream_copy_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	ml_value_t *Source = State->Source = Args[0];
	ml_value_t *Destination = State->Destination = Args[1];
	State->read = ml_typed_fn_get(ml_typeof(Source), ml_stream_read) ?: ml_stream_read_method;
	State->write = ml_typed_fn_get(ml_typeof(Destination), ml_stream_write) ?: ml_stream_write_method;
	State->Base.run = (ml_state_fn)ml_stream_copy_read_run;
	State->Remaining = ml_integer_value(Args[2]);
	size_t Read = State->Remaining < ML_STRINGBUFFER_NODE_SIZE ? State->Remaining : ML_STRINGBUFFER_NODE_SIZE;
	return State->read((ml_state_t *)State, Source, State->Buffer, Read);
}

ML_METHODX("flush", MLStreamT) {
//<Stream
// Flushes :mini:`Stream`. This method should be overridden for streams defined in Minilang.
	ml_value_t *Stream = Args[0];
	typeof(ml_stream_flush) *flush = ml_typed_fn_get(ml_typeof(Stream), ml_stream_flush);
	if (!flush) ML_RETURN(Stream);
	return flush(Caller, Stream);
}

ML_METHODX("seek", MLStreamT, MLIntegerT, MLStreamSeekT) {
//<Stream
//<Offset
//<Mode
//>integer
// Sets the position for the next read or write in :mini:`Stream` to :mini:`Offset` using :mini:`Mode`. This method should be overridden for streams defined in Minilang.
	ml_value_t *Stream = Args[0];
	typeof(ml_stream_seek) *seek = ml_typed_fn_get(ml_typeof(Stream), ml_stream_seek);
	if (!seek) ML_ERROR("StreamError", "No seek method defined for %s", ml_typeof(Args[0])->Name);
	return seek(Caller, Stream, ml_integer_value(Args[1]), ml_enum_value_value(Args[2]));
}

ML_METHODX("tell", MLStreamT) {
//<Stream
//>integer
// Gets the position for the next read or write in :mini:`Stream`. This method should be overridden for streams defined in Minilang.
	ml_value_t *Stream = Args[0];
	typeof(ml_stream_tell) *tell = ml_typed_fn_get(ml_typeof(Stream), ml_stream_tell);
	if (!tell) ML_ERROR("StreamError", "No tell method defined for %s", ml_typeof(Args[0])->Name);
	return tell(Caller, Stream);
}

ML_METHODX("close", MLStreamT) {
//<Stream
//>nil
// Closes :mini:`Stream`. This method should be overridden for streams defined in Minilang.
	ml_value_t *Stream = Args[0];
	typeof(ml_stream_close) *close = ml_typed_fn_get(ml_typeof(Stream), ml_stream_close);
	if (!close) ML_ERROR("StreamError", "No close method defined for %s", ml_typeof(Args[0])->Name);
	return close(Caller, Stream);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Stream, *Parser, *Value;
	typeof(ml_stream_read) *read;
	typeof(ml_stream_write) *write;
	typeof(ml_stream_flush) *flush;
	ml_value_t *Values;
	char *Buffer;
	size_t BufferSize, Offset, Available;
	int Index;
} ml_stream_parser_t;

static void ml_stream_parser_call(ml_state_t *Caller, ml_stream_parser_t *Parser, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_list_put(Parser->Values, Args[0]);
	ML_RETURN(MLNil);
}

static void ml_stream_parser_init(ml_stream_parser_t *Parser, ml_value_t *Value) {
	Value = ml_deref(Value);
	ml_state_t *Caller = Parser->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (!ml_is(Value, MLStreamT)) ML_ERROR("TypeError", "Expected stream not %s", ml_typeof(Value)->Name);
	Parser->Parser = Value;
	Parser->write = ml_typed_fn_get(ml_typeof(Value), ml_stream_write);
	Parser->flush = ml_typed_fn_get(ml_typeof(Value), ml_stream_flush);
	Parser->Values = ml_list();
	Parser->Buffer = snew(Parser->BufferSize);
	ML_RETURN(Parser);
}

ML_TYPE(MLStreamParserT, (MLFunctionT, MLSequenceT), "stream::parser",
	.call = (void *)ml_stream_parser_call
);

ML_METHODX("parse", MLStreamT, MLFunctionT) {
	ml_value_t *Stream = Args[0];
	ml_stream_parser_t *Parser = new(ml_stream_parser_t);
	Parser->Base.Type = MLStreamParserT;
	Parser->Base.Caller = Caller;
	Parser->Base.Context = Caller->Context;
	Parser->Base.run = (ml_state_fn)ml_stream_parser_init;
	Parser->Stream = Stream;
	Parser->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read);
	Parser->BufferSize = 256;
	ml_value_t *Constructor = Args[1];
	Args[0] = (ml_value_t *)Parser;
	return ml_call((ml_state_t *)Parser, Constructor, 1, Args);
}

static void ml_stream_parser_read(ml_stream_parser_t *Parser, ml_value_t *Value);

static void ml_stream_parser_flush(ml_stream_parser_t *Parser, ml_value_t *Value) {
	Value = ml_deref(Value);
	ml_state_t *Caller = Parser->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Caller);
	if (!ml_list_length(Parser->Values)) ML_RETURN(MLNil);
	Parser->Value = ml_list_pop(Parser->Values);
	++Parser->Index;
	ML_RETURN(Parser);
}

static void ml_stream_parser_write(ml_stream_parser_t *Parser, ml_value_t *Value) {
	Value = ml_deref(Value);
	ml_state_t *Caller = Parser->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Caller);
	size_t Actual = ml_integer_value(Value);
	if (Actual < Parser->Available) {
		Parser->Available -= Actual;
		Parser->Offset += Actual;
		return Parser->write((ml_state_t *)Parser, Parser->Parser, Parser->Buffer + Parser->Offset, Parser->Available);
	}
	if (!ml_list_length(Parser->Values)) {
		Parser->Base.run = (ml_state_fn)ml_stream_parser_read;
		return Parser->read((ml_state_t *)Parser, Parser->Stream, Parser->Buffer, Parser->BufferSize);
	}
	Parser->Value = ml_list_pop(Parser->Values);
	++Parser->Index;
	ML_RETURN(Parser);
}

static void ml_stream_parser_read(ml_stream_parser_t *Parser, ml_value_t *Value) {
	Value = ml_deref(Value);
	ml_state_t *Caller = Parser->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Caller);
	size_t Actual = ml_integer_value(Value);
	if (!Actual) {
		Parser->Stream = NULL;
		Parser->Base.run = (ml_state_fn)ml_stream_parser_flush;
		return Parser->flush((ml_state_t *)Parser, Parser->Parser);
	}
	Parser->Offset = 0;
	Parser->Available = Actual;
	Parser->Base.run = (ml_state_fn)ml_stream_parser_write;
	return Parser->write((ml_state_t *)Parser, Parser->Parser, Parser->Buffer, Actual);
}

static void ML_TYPED_FN(ml_iterate, MLStreamParserT, ml_state_t *Caller, ml_stream_parser_t *Parser) {
	if (!Parser->Stream) ML_RETURN(MLNil);
	Parser->Base.Caller = Caller;
	Parser->Base.Context = Caller->Context;
	Parser->Base.run = (ml_state_fn)ml_stream_parser_read;
	Parser->Index = 0;
	return Parser->read((ml_state_t *)Parser, Parser->Stream, Parser->Buffer, Parser->BufferSize);
}

static void ML_TYPED_FN(ml_iter_next, MLStreamParserT, ml_state_t *Caller, ml_stream_parser_t *Parser) {
	if (ml_list_length(Parser->Values)) {
		Parser->Value = ml_list_pop(Parser->Values);
		++Parser->Index;
		ML_RETURN(Parser);
	}
	if (!Parser->Stream) ML_RETURN(MLNil);
	Parser->Base.Caller = Caller;
	Parser->Base.Context = Caller->Context;
	Parser->Base.run = (ml_state_fn)ml_stream_parser_read;
	return Parser->read((ml_state_t *)Parser, Parser->Stream, Parser->Buffer, Parser->BufferSize);
}

static void ML_TYPED_FN(ml_iter_key, MLStreamParserT, ml_state_t *Caller, ml_stream_parser_t *Parser) {
	ML_RETURN(ml_integer(Parser->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLStreamParserT, ml_state_t *Caller, ml_stream_parser_t *Parser) {
	ML_RETURN(Parser->Value);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Stream;
	typeof(ml_stream_read) *read;
	void *Next;
	size_t Size, Available;
	struct {
		void *Address;
		size_t Total, Count;
	} Request;
	char Chars[];
} ml_buffered_reader_t;

static void ml_buffered_reader_run0(ml_buffered_reader_t *Reader, ml_value_t *Result) {
	Result = ml_deref(Result);
	ml_state_t *Caller = Reader->Base.Caller;
	Reader->Base.Caller = NULL;
	if (ml_is_error(Result)) ML_RETURN(Result);
	size_t Actual = ml_integer_value(Result);
	ML_RETURN(ml_integer(Reader->Request.Total + Actual));
}

static void ml_buffered_reader_run1(ml_buffered_reader_t *Reader, ml_value_t *Result) {
	Result = ml_deref(Result);
	ml_state_t *Caller = Reader->Base.Caller;
	Reader->Base.Caller = NULL;
	if (ml_is_error(Result)) ML_RETURN(Result);
	size_t Actual = ml_integer_value(Result);
	void *Address = Reader->Request.Address;
	size_t Count = Reader->Request.Count;
	if (Actual > Count) {
		memcpy(Address, Reader->Chars, Count);
		Reader->Available = Actual - Count;
		Reader->Next = Reader->Chars + Count;
		ML_RETURN(ml_integer(Reader->Request.Total + Count));
	} else {
		memcpy(Address, Reader->Chars, Actual);
		Reader->Available = 0;
		ML_RETURN(ml_integer(Reader->Request.Total + Actual));
	}
}

static void ml_buffered_reader_read(ml_state_t *Caller, ml_buffered_reader_t *Reader, void *Address, int Count) {
	if (Reader->Base.Caller) ML_ERROR("StreamError", "Attempting to read from stream before previous read complete");
	if (Reader->Available >= Count) {
		memcpy(Address, Reader->Next, Count);
		Reader->Available -= Count;
		Reader->Next += Count;
		ML_RETURN(ml_integer(Count));
	}
	size_t Total = 0;
	if (Reader->Available) {
		memcpy(Address, Reader->Next, Reader->Available);
		Address += Reader->Available;
		Count -= Reader->Available;
		Total += Reader->Available;
	}
	Reader->Base.Caller = Caller;
	Reader->Base.Context = Caller->Context;
	Reader->Request.Total = Total;
	if (Count >= Reader->Size) {
		Reader->Base.run = (ml_state_fn)ml_buffered_reader_run0;
		return Reader->read((ml_state_t *)Reader, Reader->Stream, Address, Count);
	} else {
		Reader->Base.run = (ml_state_fn)ml_buffered_reader_run1;
		Reader->Request.Address = Address;
		Reader->Request.Count = Count;
		return Reader->read((ml_state_t *)Reader, Reader->Stream, Reader->Chars, Reader->Size);
	}
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Stream;
	typeof(ml_stream_write) *write;
	void *Next;
	size_t Size, Fill, Space;
	struct {
		const void *Address;
		size_t Total, Count;
	} Request;
	char Chars[];
} ml_buffered_writer_t;

static void ml_buffered_writer_run(ml_buffered_writer_t *Writer, ml_value_t *Result) {
	Result = ml_deref(Result);
	ml_state_t *Caller = Writer->Base.Caller;
	if (ml_is_error(Result)) {
		Writer->Base.Caller = NULL;
		ML_RETURN(Result);
	}
	size_t Actual = ml_integer_value(Result);
	if (Writer->Fill -= Actual) {
		return Writer->write((ml_state_t *)Writer, Writer->Stream, Writer->Next - Writer->Fill, Writer->Fill);
	}
	Writer->Base.Caller = NULL;
	const void *Address = Writer->Request.Address;
	size_t Count = Writer->Request.Count;
	if (Count > Writer->Size) {
		Writer->Next = Writer->Chars;
		Writer->Fill = 0;
		Writer->Space = Writer->Size;
		return Writer->write((ml_state_t *)Writer, Writer->Stream, Address, Count);
	}
	memcpy(Writer->Chars, Address, Count);
	Writer->Next = Writer->Chars + Count;
	Writer->Fill = Count;
	Writer->Space = Writer->Size - Count;
	ML_RETURN(ml_integer(Count));
}

static void ml_buffered_writer_write(ml_state_t *Caller, ml_buffered_writer_t *Writer, const void *Address, int Count) {
	if (Writer->Base.Caller) ML_ERROR("StreamError", "Attempting to write from stream before previous write complete");
	if (Count <= Writer->Space) {
		memcpy(Writer->Next, Address, Count);
		Writer->Next += Count;
		Writer->Fill += Count;
		Writer->Space -= Count;
		ML_RETURN(ml_integer(Count));
	} else if (Writer->Fill) {
		Writer->Base.Caller = Caller;
		Writer->Base.Context = Caller->Context;
		Writer->Request.Address = Address;
		Writer->Request.Count = Count;
		return Writer->write((ml_state_t *)Writer, Writer->Stream, Writer->Next - Writer->Fill, Writer->Fill);
	} else {
		return Writer->write(Caller, Writer->Stream, Address, Count);
	}
}

typedef struct {
	ml_type_t *Type;
	ml_buffered_reader_t *Reader;
	ml_buffered_writer_t *Writer;
} ml_buffered_stream_t;

ML_TYPE(MLStreamBufferedT, (MLStreamT), "stream::buffered");
// A stream that buffers reads and writes from another stream.

ml_value_t *ml_stream_buffered(ml_value_t *Stream, size_t Size) {
	ml_buffered_stream_t *Buffered = new(ml_buffered_stream_t);
	Buffered->Type = MLStreamBufferedT;
	ml_buffered_reader_t *Reader = xnew(ml_buffered_reader_t, Size, char);
	Reader->Stream = Stream;
	Reader->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	Reader->Size = Size;
	Buffered->Reader = Reader;
	ml_buffered_writer_t *Writer = xnew(ml_buffered_writer_t, Size, char);
	Writer->Base.run = (ml_state_fn)ml_buffered_writer_run;
	Writer->Stream = Stream;
	Writer->write = ml_typed_fn_get(ml_typeof(Stream), ml_stream_write) ?: ml_stream_write_method;
	Writer->Size = Writer->Space = Size;
	Writer->Next = Writer->Chars;
	Buffered->Writer = Writer;
	return (ml_value_t *)Buffered;
}

ML_METHOD(MLStreamBufferedT, MLStreamT, MLIntegerT) {
//@stream::buffered
//<Stream
//<Size
//>stream::buffered
// Returns a new stream that buffers reads and writes from :mini:`Stream`.
	return ml_stream_buffered(Args[0], ml_integer_value(Args[1]));
}

static void ML_TYPED_FN(ml_stream_read, MLStreamBufferedT, ml_state_t *Caller, ml_buffered_stream_t *Stream, void *Address, int Count) {
	return ml_buffered_reader_read(Caller, Stream->Reader, Address, Count);
}

ML_METHODX("read", MLStreamBufferedT, MLBufferT) {
//!internal
	ml_buffered_stream_t *Stream = (ml_buffered_stream_t *)Args[0];
	void *Address = ml_buffer_value(Args[1]);
	Count = ml_buffer_length(Args[1]);
	return ml_buffered_reader_read(Caller, Stream->Reader, Address, Count);
}

static void ML_TYPED_FN(ml_stream_write, MLStreamBufferedT, ml_state_t *Caller, ml_buffered_stream_t *Stream, const void *Address, int Count) {
	return ml_buffered_writer_write(Caller, Stream->Writer, Address, Count);
}

ML_METHODX("write", MLStreamBufferedT, MLAddressT) {
//!internal
	ml_buffered_stream_t *Stream = (ml_buffered_stream_t *)Args[0];
	const void *Address = ml_address_value(Args[1]);
	Count = ml_address_length(Args[1]);
	return ml_buffered_writer_write(Caller, Stream->Writer, Address, Count);
}

static void ML_TYPED_FN(ml_stream_flush, MLStreamBufferedT, ml_state_t *Caller, ml_buffered_stream_t *Stream) {
	ml_buffered_writer_t *Writer = Stream->Writer;
	if (Writer->Base.Caller) ML_ERROR("StreamError", "Attempting to write from stream before previous write complete");
	Writer->Base.Caller = Caller;
	Writer->Base.Context = Caller->Context;
	Writer->Request.Address = NULL;
	Writer->Request.Count = 0;
	return Writer->write((ml_state_t *)Writer, Writer->Stream, Writer->Next - Writer->Fill, Writer->Fill);
}

ML_METHODX("flush", MLStreamBufferedT) {
//<Stream
// Writes any bytes in the buffer.
	ml_buffered_stream_t *Stream = (ml_buffered_stream_t *)Args[0];
	ml_buffered_writer_t *Writer = Stream->Writer;
	if (Writer->Base.Caller) ML_ERROR("StreamError", "Attempting to write from stream before previous write complete");
	Writer->Base.Caller = Caller;
	Writer->Base.Context = Caller->Context;
	Writer->Request.Address = NULL;
	Writer->Request.Count = 0;
	return Writer->write((ml_state_t *)Writer, Writer->Stream, Writer->Next - Writer->Fill, Writer->Fill);
}

static void ML_TYPED_FN(ml_stream_read, MLStringBufferT, ml_state_t *Caller, ml_stringbuffer_t *Stream, void *Address, int Count) {
	size_t Total = 0, Length = ml_stringbuffer_reader(Stream, 0);
	while (Count) {
		if (!Length) break;
		if (Length >= Count) {
			memcpy(Address, Stream->Head->Chars + Stream->Start, Count);
			ml_stringbuffer_reader(Stream, Count);
			Total += Count;
			break;
		}
		memcpy(Address, Stream->Head->Chars + Stream->Start, Length);
		Total += Length;
		Count -= Length;
		Address += Length;
		Length = ml_stringbuffer_reader(Stream, Length);
	}
	ML_RETURN(ml_integer(Total));
}

ML_METHOD("read", MLStringBufferT, MLBufferT) {
//!internal
	ml_stringbuffer_t *Stream = (ml_stringbuffer_t *)Args[0];
	void *Address = ml_buffer_value(Args[1]);
	Count = ml_buffer_length(Args[1]);
	size_t Length = 0, Total = 0;
	while (Count) {
		Length = ml_stringbuffer_reader(Stream, Length);
		if (!Length) break;
		if (Length > Count) {
			memcpy(Address, Stream->Head->Chars + Stream->Start, Count);
			ml_stringbuffer_reader(Stream, Count);
			Total += Count;
			break;
		}
		memcpy(Address, Stream->Head->Chars + Stream->Start, Length);
		Total += Length;
		Count -= Length;
		Address += Length;
	}
	return ml_integer(Total);
}

static void ML_TYPED_FN(ml_stream_write, MLStringBufferT, ml_state_t *Caller, ml_stringbuffer_t *Buffer, void *Address, int Count) {
	ML_RETURN(ml_integer(ml_stringbuffer_write(Buffer, Address, Count)));
}

typedef struct {
	const ml_type_t *Type;
	int Fd;
} ml_fd_stream_t;

ML_TYPE(MLStreamFdT, (MLStreamT), "fd");
// A file-descriptor based stream.

ml_value_t *ml_fd_stream(ml_type_t *Type, int Fd) {
	if (!ml_is_subtype(Type, MLStreamFdT)) return ml_error("TypeError", "Type must be a subtype of stream::fd");
	ml_fd_stream_t *Stream = new(ml_fd_stream_t);
	Stream->Type = Type;
	Stream->Fd = Fd;
	return (ml_value_t *)Stream;
}

int ml_fd_stream_fd(ml_value_t *Stream) {
	return ((ml_fd_stream_t *)Stream)->Fd;
}

#ifdef ML_THREADS
#include "ml_thread.h"

static ml_value_t *ML_TYPED_FN(ml_is_threadsafe, MLStreamFdT, ml_value_t *Value) {
	return NULL;
}
#endif

static void ML_TYPED_FN(ml_stream_read, MLStreamFdT, ml_state_t *Caller, ml_fd_stream_t *Stream, void *Address, int Count) {
	ssize_t Actual = read(Stream->Fd, Address, Count);
	ml_value_t *Result;
	if (Actual < 0) {
		Result = ml_error("ReadError", "%s", strerror(errno));
	} else {
		Result = ml_integer(Actual);
	}
	ML_CONTINUE(Caller, Result);
}

ML_METHOD("read", MLStreamFdT, MLBufferT) {
//<Stream
//<Dest
//>integer
// Reads from :mini:`Stream` into :mini:`Dest` returning the actual number of bytes read.
	ml_fd_stream_t *Stream = (ml_fd_stream_t *)Args[0];
	ml_address_t *Buffer = (ml_address_t *)Args[1];
	ssize_t Actual = read(Stream->Fd, Buffer->Value, Buffer->Length);
	if (Actual < 0) {
		return ml_error("ReadError", "%s", strerror(errno));
	} else {
		return ml_integer(Actual);
	}
}

static void ML_TYPED_FN(ml_stream_write, MLStreamFdT, ml_state_t *Caller, ml_fd_stream_t *Stream, void *Address, int Count) {
	ssize_t Actual = write(Stream->Fd, Address, Count);
	ml_value_t *Result;
	if (Actual < 0) {
		Result = ml_error("WriteError", "%s", strerror(errno));
	} else {
		Result = ml_integer(Actual);
	}
	ML_RETURN(Result);
}

ML_METHOD("write", MLStreamFdT, MLAddressT) {
//<Stream
//<Source
//>integer
// Writes from :mini:`Source` to :mini:`Stream` returning the actual number of bytes written.
	ml_fd_stream_t *Stream = (ml_fd_stream_t *)Args[0];
	ml_address_t *Buffer = (ml_address_t *)Args[1];
	ssize_t Actual = write(Stream->Fd, Buffer->Value, Buffer->Length);
	if (Actual < 0) {
		return ml_error("WriteError", "%s", strerror(errno));
	} else {
		return ml_integer(Actual);
	}
}

void ml_stream_init(stringmap_t *Globals) {
#include "ml_stream_init.c"
	ml_type_add_parent(MLStringBufferT, MLStreamT);
	stringmap_insert(MLStreamT->Exports, "seek", MLStreamSeekT);
	stringmap_insert(MLStreamT->Exports, "fd", MLStreamFdT);
	stringmap_insert(MLStreamT->Exports, "buffered", MLStreamBufferedT);
	if (Globals) {
		stringmap_insert(Globals, "stream", MLStreamT);
	}
}

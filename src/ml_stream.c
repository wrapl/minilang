#include "ml_macros.h"
#include <gc/gc.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "ml_stream.h"

#undef ML_CATEGORY
#define ML_CATEGORY "stream"

ML_INTERFACE(MLStreamT, (MLAnyT), "stream");
// Base type of readable and writable byte streams.

ML_METHOD_DECL(ReadMethod, "read");
ML_METHOD_DECL(WriteMethod, "write");

static void ml_stream_read_method(ml_state_t *Caller, ml_value_t *Value, void *Address, int Count) {
	ml_address_t *Buffer = new(ml_address_t);
	Buffer->Type = MLAddressT;
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

static void ml_stream_write_method(ml_state_t *Caller, ml_value_t *Value, const void *Address, int Count) {
	ml_address_t *Buffer = new(ml_address_t);
	Buffer->Type = MLAddressT;
	Buffer->Value = (void *)Address;
	Buffer->Length = Count;
	ml_value_t **Args = ml_alloc_args(3);
	Args[0] = Value;
	Args[1] = (ml_value_t *)Buffer;
	return ml_call(Caller, WriteMethod, 2, Args);
}

void ml_stream_write(ml_state_t *Caller, ml_value_t *Value, const void *Address, int Count) {
	typeof(ml_stream_write) *function = ml_typed_fn_get(ml_typeof(Value), ml_stream_write) ?: ml_stream_write_method;
	return function(Caller, Value, Address, Count);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Stream;
	union {
		typeof(ml_stream_read) *read;
		typeof(ml_stream_write) *write;
	};
	ml_stringbuffer_t Buffer[1];
	size_t Max;
	unsigned char Chars[];
} ml_read_state_t;

static void ml_stream_readx_run(ml_read_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (!ml_integer_value(Value)) ML_RETURN(State->Buffer->Length ? ml_stringbuffer_get_value(State->Buffer) : MLNil);
	unsigned char Char = State->Chars[64];
	if (State->Chars[Char / 8] & (1 << (Char % 8))) ML_RETURN(ml_stringbuffer_get_value(State->Buffer));
	ml_stringbuffer_write(State->Buffer, (void *)&State->Chars[64], 1);
	if (State->Buffer->Length == State->Max) ML_RETURN(ml_stringbuffer_get_value(State->Buffer));
	ml_value_t *Stream = State->Stream;
	return State->read((ml_state_t *)State, Stream, &State->Chars[64], 1);
}

ML_METHODX("readx", MLStreamT, MLIntegerT, MLStringT) {
	ml_value_t *Stream = Args[0];
	ml_read_state_t *State = xnew(ml_read_state_t, 65, char);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_readx_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	State->Max = ml_integer_value(Args[1]);
	const unsigned char *Terms = (const unsigned char *)ml_string_value(Args[2]);
	int TermsLength = ml_string_length(Args[2]);
	for (int I = 0; I < TermsLength; ++I) {
		unsigned char Char = Terms[I];
		State->Chars[Char / 8] |= 1 << (Char % 8);
	}
	return State->read((ml_state_t *)State, Stream, (void *)&State->Chars[64], 1);
}

static void ml_stream_readi_run(ml_read_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (!ml_integer_value(Value)) ML_RETURN(State->Buffer->Length ? ml_stringbuffer_get_value(State->Buffer) : MLNil);
	unsigned char Char = State->Chars[64];
	ml_stringbuffer_write(State->Buffer, (void *)&State->Chars[64], 1);
	if (State->Chars[Char / 8] & (1 << (Char % 8))) ML_RETURN(ml_stringbuffer_get_value(State->Buffer));
	if (State->Buffer->Length == State->Max) ML_RETURN(ml_stringbuffer_get_value(State->Buffer));
	ml_value_t *Stream = State->Stream;
	return State->read((ml_state_t *)State, Stream, &State->Chars[64], 1);
}

ML_METHODX("readi", MLStreamT, MLIntegerT, MLStringT) {
	ml_value_t *Stream = Args[0];
	ml_read_state_t *State = xnew(ml_read_state_t, 65, char);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_readx_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	State->Max = ml_integer_value(Args[1]);
	const unsigned char *Terms = (const unsigned char *)ml_string_value(Args[2]);
	int TermsLength = ml_string_length(Args[2]);
	for (int I = 0; I < TermsLength; ++I) {
		unsigned char Char = Terms[I];
		State->Chars[Char / 8] |= 1 << (Char % 8);
	}
	return State->read((ml_state_t *)State, Stream, (void *)&State->Chars[64], 1);
}

typedef struct ml_fd_t {
	const ml_type_t *Type;
	int Fd;
} ml_fd_t;

ML_TYPE(MLStreamFdT, (MLStreamT), "fd");
// A file-descriptor based stream.

ml_value_t *ml_fd_new(int Fd) {
	ml_fd_t *Stream = new(ml_fd_t);
	Stream->Type = MLStreamFdT;
	Stream->Fd = Fd;
	return (ml_value_t *)Stream;
}

static void ML_TYPED_FN(ml_stream_read, MLStreamFdT, ml_state_t *Caller, ml_fd_t *Stream, void *Address, int Count) {
	ssize_t Actual = read(Stream->Fd, Address, Count);
	ml_value_t *Result;
	if (Actual < 0) {
		Result = ml_error("ReadError", strerror(errno));
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
	ml_fd_t *Stream = (ml_fd_t *)Args[0];
	ml_address_t *Buffer = (ml_address_t *)Args[1];
	ssize_t Actual = read(Stream->Fd, Buffer->Value, Buffer->Length);
	if (Actual < 0) {
		return ml_error("ReadError", strerror(errno));
	} else {
		return ml_integer(Actual);
	}
}

static void ML_TYPED_FN(ml_stream_write, MLStreamFdT, ml_state_t *Caller, ml_fd_t *Stream, void *Address, int Count) {
	ssize_t Actual = write(Stream->Fd, Address, Count);
	ml_value_t *Result;
	if (Actual < 0) {
		Result = ml_error("WriteError", strerror(errno));
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
	ml_fd_t *Stream = (ml_fd_t *)Args[0];
	ml_address_t *Buffer = (ml_address_t *)Args[1];
	ssize_t Actual = write(Stream->Fd, Buffer->Value, Buffer->Length);
	if (Actual < 0) {
		return ml_error("WriteError", strerror(errno));
	} else {
		return ml_integer(Actual);
	}
}

void ml_stream_init(stringmap_t *Globals) {
#include "ml_stream_init.c"
	stringmap_insert(MLStreamT->Exports, "fd", MLStreamFdT);
	if (Globals) {
		stringmap_insert(Globals, "stream", MLStreamT);
		stringmap_insert(Globals, "terminal", ml_module("terminal",
			"stdin", ml_fd_new(STDIN_FILENO),
			"stdout", ml_fd_new(STDOUT_FILENO),
			"stderr", ml_fd_new(STDERR_FILENO),
		NULL));
	}
}

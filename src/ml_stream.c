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
	typeof(ml_stream_read) *read;
	char *String, *End;
	size_t Remaining;
} ml_read_state_t;

static void ml_stream_read_run(ml_read_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	size_t Length = ml_integer_value(Value);
	if (!Length) {
		Length = State->End - State->String;
		ML_RETURN(Length ? ml_string(State->String, Length) : MLNil);
	}
	State->End += Length;
	size_t Remaining = State->Remaining - Length;
	if (!Remaining) ML_RETURN(ml_string(State->String, State->End - State->String));
	State->Remaining = Remaining;
	ml_value_t *Stream = State->Stream;
	return State->read((ml_state_t *)State, Stream, State->End, Remaining);
}

ML_METHODX("read", MLStreamT, MLIntegerT) {
	ml_value_t *Stream = Args[0];
	ml_read_state_t *State = new(ml_read_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_read_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	size_t Remaining = State->Remaining = ml_integer_value(Args[1]);
	State->End = State->String = snew(Remaining + 1);
	return State->read((ml_state_t *)State, Stream, State->End, Remaining);
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Stream;
	typeof(ml_stream_read) *read;
	ml_stringbuffer_t Buffer[1];
	size_t Remaining;
	unsigned char Chars[];
} ml_readu_state_t;

static void ml_stream_readx_run(ml_readu_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	size_t Length = ml_integer_value(Value);
	if (!Length) ML_RETURN(State->Buffer->Length ? ml_stringbuffer_get_value(State->Buffer) : MLNil);
	unsigned char Char = State->Chars[64];
	if (State->Chars[Char / 8] & (1 << (Char % 8))) ML_RETURN(ml_stringbuffer_get_value(State->Buffer));
	ml_stringbuffer_write(State->Buffer, (void *)&State->Chars[64], 1);
	size_t Remaining = State->Remaining - Length;
	if (!Remaining) ML_RETURN(ml_stringbuffer_get_value(State->Buffer));
	State->Remaining = Remaining;
	ml_value_t *Stream = State->Stream;
	return State->read((ml_state_t *)State, Stream, &State->Chars[64], 1);
}

ML_METHODX("readx", MLStreamT, MLIntegerT, MLStringT) {
	ml_value_t *Stream = Args[0];
	ml_readu_state_t *State = xnew(ml_readu_state_t, 65, char);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_readx_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	State->Remaining = ml_integer_value(Args[1]);
	const unsigned char *Terms = (const unsigned char *)ml_string_value(Args[2]);
	int TermsLength = ml_string_length(Args[2]);
	for (int I = 0; I < TermsLength; ++I) {
		unsigned char Char = Terms[I];
		State->Chars[Char / 8] |= 1 << (Char % 8);
	}
	return State->read((ml_state_t *)State, Stream, (void *)&State->Chars[64], 1);
}

static void ml_stream_readi_run(ml_readu_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	size_t Length = ml_integer_value(Value);
	if (!Length) ML_RETURN(State->Buffer->Length ? ml_stringbuffer_get_value(State->Buffer) : MLNil);
	ml_stringbuffer_write(State->Buffer, (void *)&State->Chars[64], 1);
	unsigned char Char = State->Chars[64];
	if (State->Chars[Char / 8] & (1 << (Char % 8))) ML_RETURN(ml_stringbuffer_get_value(State->Buffer));
	size_t Remaining = State->Remaining - Length;
	if (!Remaining) ML_RETURN(ml_stringbuffer_get_value(State->Buffer));
	State->Remaining = Remaining;
	ml_value_t *Stream = State->Stream;
	return State->read((ml_state_t *)State, Stream, &State->Chars[64], 1);
}

ML_METHODX("readi", MLStreamT, MLIntegerT, MLStringT) {
	ml_value_t *Stream = Args[0];
	ml_readu_state_t *State = xnew(ml_readu_state_t, 65, char);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_readi_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	State->Remaining = ml_integer_value(Args[1]);
	const unsigned char *Terms = (const unsigned char *)ml_string_value(Args[2]);
	int TermsLength = ml_string_length(Args[2]);
	for (int I = 0; I < TermsLength; ++I) {
		unsigned char Char = Terms[I];
		State->Chars[Char / 8] |= 1 << (Char % 8);
	}
	return State->read((ml_state_t *)State, Stream, (void *)&State->Chars[64], 1);
}

ML_METHODX("read", MLStreamT) {
	ml_value_t *Stream = Args[0];
	ml_readu_state_t *State = xnew(ml_readu_state_t, 65, char);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_readi_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	State->Remaining = SIZE_MAX;
	State->Chars['\n' / 8] |= 1 << ('\n' % 8);
	return State->read((ml_state_t *)State, Stream, (void *)&State->Chars[64], 1);
}

static void ml_stream_rest_run(ml_readu_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	size_t Length = ml_integer_value(Value);
	if (!Length) ML_RETURN(State->Buffer->Length ? ml_stringbuffer_get_value(State->Buffer) : MLNil);
	char *Space = ml_stringbuffer_writer(State->Buffer, Length);
	ml_value_t *Stream = State->Stream;
	return State->read((ml_state_t *)State, Stream, Space, State->Buffer->Space);
}

ML_METHODX("rest", MLStreamT) {
	ml_value_t *Stream = Args[0];
	ml_readu_state_t *State = xnew(ml_readu_state_t, 0, char);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stream_rest_run;
	State->Stream = Stream;
	State->read = ml_typed_fn_get(ml_typeof(Stream), ml_stream_read) ?: ml_stream_read_method;
	State->Buffer[0] = (ml_stringbuffer_t)ML_STRINGBUFFER_INIT;
	char *Space = ml_stringbuffer_writer(State->Buffer, 0);
	return State->read((ml_state_t *)State, Stream, Space, State->Buffer->Space);
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
	ml_fd_t *Stream = (ml_fd_t *)Args[0];
	ml_address_t *Buffer = (ml_address_t *)Args[1];
	ssize_t Actual = read(Stream->Fd, Buffer->Value, Buffer->Length);
	if (Actual < 0) {
		return ml_error("ReadError", "%s", strerror(errno));
	} else {
		return ml_integer(Actual);
	}
}

static void ML_TYPED_FN(ml_stream_write, MLStreamFdT, ml_state_t *Caller, ml_fd_t *Stream, void *Address, int Count) {
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
	ml_fd_t *Stream = (ml_fd_t *)Args[0];
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
	stringmap_insert(MLStreamT->Exports, "fd", MLStreamFdT);
	if (Globals) {
		stringmap_insert(Globals, "stream", MLStreamT);
	}
}

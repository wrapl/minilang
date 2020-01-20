#include "ml_io.h"
#include "ml_macros.h"
#include <gc/gc.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

ml_type_t *MLStreamT;
static ml_value_t *ReadMethod, *WriteMethod;

ml_value_t *ml_io_read(ml_state_t *Caller, ml_value_t *Value, void *Address, int Count) {
	typeof(ml_io_read) *function = ml_typed_fn_get(Value->Type, ml_io_read);
	if (function) return function(Caller, Value, Address, Count);
	ml_buffer_t *Buffer = new(ml_buffer_t);
	Buffer->Type = MLBufferT;
	Buffer->Address = Address;
	Buffer->Size = Count;
	ml_value_t **Args = anew(ml_value_t *, 2);
	Args[0] = Value;
	Args[1] = (ml_value_t *)Buffer;
	return ReadMethod->Type->call(Caller, ReadMethod, 2, Args);
}

ml_value_t *ml_io_write(ml_state_t *Caller, ml_value_t *Value, void *Address, int Count) {
	typeof(ml_io_write) *function = ml_typed_fn_get(Value->Type, ml_io_write);
	if (function) return function(Caller, Value, Address, Count);
	ml_buffer_t *Buffer = new(ml_buffer_t);
	Buffer->Type = MLBufferT;
	Buffer->Address = Address;
	Buffer->Size = Count;
	ml_value_t **Args = anew(ml_value_t *, 3);
	Args[0] = Value;
	Args[1] = (ml_value_t *)Buffer;
	return WriteMethod->Type->call(Caller, WriteMethod, 2, Args);
}

ML_METHODX("write", MLStreamT, MLStringT) {
	return ml_io_write(Caller, Args[0], ml_string_value(Args[1]), ml_string_length(Args[1]));
}

typedef struct ml_fd_t {
	const ml_type_t *Type;
	int Fd;
} ml_fd_t;

ml_type_t *MLFdT;

ml_value_t *ml_fd_new(int Fd) {
	ml_fd_t *Stream = new(ml_fd_t);
	Stream->Type = MLFdT;
	Stream->Fd = Fd;
	return (ml_value_t *)Stream;
}

ml_value_t *ml_fd_read(ml_state_t *Caller, ml_fd_t *Stream, void *Address, int Count) {
	ssize_t Actual = read(Stream->Fd, Address, Count);
	ml_value_t *Result;
	if (Actual < 0) {
		Result = ml_error("ReadError", strerror(errno));
	} else {
		Result = ml_integer(Actual);
	}
	ML_CONTINUE(Caller, Result);
}

ml_value_t *ml_fd_write(ml_state_t *Caller, ml_fd_t *Stream, void *Address, int Count) {
	ssize_t Actual = write(Stream->Fd, Address, Count);
	ml_value_t *Result;
	if (Actual < 0) {
		Result = ml_error("WriteError", strerror(errno));
	} else {
		Result = ml_integer(Actual);
	}
	ML_CONTINUE(Caller, Result);
}

void ml_io_init(stringmap_t *Globals) {
	MLStreamT = ml_type(MLAnyT, "stream");
	MLFdT = ml_type(MLStreamT, "fd");
	ReadMethod = ml_method("read");
	WriteMethod = ml_method("write");
	ml_typed_fn_set(MLFdT, ml_io_read, ml_fd_read);
	ml_typed_fn_set(MLFdT, ml_io_write, ml_fd_write);
	if (Globals) {
		ml_value_t *IO = ml_map();
		ml_map_insert(IO, ml_string("buffer", -1), ml_function(NULL, ml_buffer));
		ml_map_insert(IO, ml_string("stdin", -1), ml_fd_new(STDIN_FILENO));
		ml_map_insert(IO, ml_string("stdout", -1), ml_fd_new(STDOUT_FILENO));
		ml_map_insert(IO, ml_string("stderr", -1), ml_fd_new(STDERR_FILENO));
		stringmap_insert(Globals, "io", IO);
	}
#include "ml_io_init.c"
}

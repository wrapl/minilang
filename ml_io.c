#include "ml_io.h"
#include "ml_macros.h"
#include <gc/gc.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

typedef struct ml_buffer_t {
	const ml_type_t *Type;
	void *Address;
	long Size;
} ml_buffer_t;

ml_type_t *MLBufferT, *MLStreamT;
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

static ml_value_t *ml_buffer(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	long Size = ml_integer_value(Args[0]);
	if (Size < 0) return ml_error("ValueError", "Buffer size must be non-negative");
	ml_buffer_t *Buffer = new(ml_buffer_t);
	Buffer->Type = MLBufferT;
	Buffer->Size = Size;
	Buffer->Address = GC_malloc_atomic(Size);
	return (ml_value_t *)Buffer;
}

ML_METHOD("+", MLBufferT, MLIntegerT) {
	ml_buffer_t *Buffer = (ml_buffer_t *)Args[0];
	long Offset = ml_integer_value(Args[1]);
	if (Offset >= Buffer->Size) return ml_error("ValueError", "Offset larger than buffer");
	ml_buffer_t *Buffer2 = new(ml_buffer_t);
	Buffer2->Type = MLBufferT;
	Buffer2->Address = Buffer->Address + Offset;
	Buffer2->Size = Buffer->Size - Offset;
	return (ml_value_t *)Buffer2;
}

ML_METHOD("-", MLBufferT, MLBufferT) {
	ml_buffer_t *Buffer1 = (ml_buffer_t *)Args[0];
	ml_buffer_t *Buffer2 = (ml_buffer_t *)Args[1];
	return ml_integer(Buffer1->Address - Buffer2->Address);
}

ML_METHOD("string", MLBufferT) {
	ml_buffer_t *Buffer = (ml_buffer_t *)Args[0];
	return ml_string_format("#%" PRIxPTR ":%ld", Buffer->Address, Buffer->Size);
}

typedef struct ml_fd_t {
	const ml_type_t *Type;
	int Fd;
} ml_fd_t;

ml_type_t *MLFdT;

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
		Result = ml_error("ReadError", strerror(errno));
	} else {
		Result = ml_integer(Actual);
	}
	ML_CONTINUE(Caller, Result);
}

void ml_io_init(stringmap_t *Globals) {
	MLBufferT = ml_type(MLAnyT, "buffer");
	MLStreamT = ml_type(MLAnyT, "stream");
	MLFdT = ml_type(MLStreamT, "fd");
	ReadMethod = ml_method("read");
	WriteMethod = ml_method("write");
	ml_typed_fn_set(MLFdT, ml_io_read, ml_fd_read);
	ml_typed_fn_set(MLFdT, ml_io_write, ml_fd_write);
	if (Globals) {
		ml_value_t *IO = ml_map();
		ml_map_insert(IO, ml_string("buffer", -1), ml_function(NULL, ml_buffer));
		stringmap_insert(Globals, "io", IO);
	}
#include "ml_io_init.c"
}

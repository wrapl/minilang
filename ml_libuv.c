#include "ml_libuv.h"
#include "ml_macros.h"
#include <gc/gc.h>
#include <uv.h>
#include <unistd.h>

#define CB(TYPE, BODY) ({ \
	void anonymous(TYPE *Request) BODY \
	&anonymous; \
})

static uv_loop_t *Loop = 0;

static ml_value_t *ml_uv_run(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLFunctionT);
	ml_call(Args[0], 0, NULL);
	uv_run(Loop, UV_RUN_DEFAULT);
	return MLNil;
}

typedef struct ml_uv_file_t {
	const ml_type_t *Type;
	ssize_t Handle;
} ml_uv_file_t;

static ml_type_t *MLUVFileT = 0;

static ml_value_t *ml_uv_file_new(ssize_t Handle) {
	ml_uv_file_t *File = new(ml_uv_file_t);
	File->Type = MLUVFileT;
	File->Handle = Handle;
	return (ml_value_t *)File;
}

static void ml_uv_fs_open_cb(uv_fs_t *Request) {
	ml_state_t *Caller = (ml_state_t *)Request->data;
	ml_value_t *Result;
	if (Request->result >= 0) {
		Result = ml_uv_file_new(Request->result);
	} else {
		Result = ml_error("OpenError", "error opening file %s", Request->path);
	}
	Caller->run(Caller, Result);
	uv_fs_req_cleanup(Request);
}

static ml_value_t *ml_uv_fs_open(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	const char *Path = ml_string_value(Args[0]);
	int Flags = 0;
	for (const char *P = ml_string_value(Args[1]); *P; ++P) {
		switch (*P) {
		case 'r': Flags |= O_RDONLY; break;
		case 'w': Flags |= O_WRONLY | O_CREAT | O_TRUNC; break;
		case 'a': Flags |= O_WRONLY | O_CREAT | O_APPEND; break;
		case '+': Flags |= O_RDWR; break;
		}
	}
	int Mode = ml_integer_value(Args[2]);
	uv_fs_t *Request = new(uv_fs_t);
	Request->data = Caller;
	uv_fs_open(Loop, Request, Path, Flags, Mode, ml_uv_fs_open_cb);
	return MLNil;
}

static void ml_uv_fs_close_cb(uv_fs_t *Request) {
	ml_state_t *Caller = (ml_state_t *)Request->data;
	Caller->run(Caller, MLNil);
	uv_fs_req_cleanup(Request);
}

static ml_value_t *ml_uv_fs_close(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ml_uv_file_t *File = (ml_uv_file_t *)Args[0];
	uv_fs_t *Request = new(uv_fs_t);
	Request->data = Caller;
	uv_fs_close(Loop, Request, File->Handle, ml_uv_fs_close_cb);
	return MLNil;
}

typedef struct ml_uv_fs_buf_t {
	uv_fs_t Base;
	uv_buf_t IOV[];
} ml_uv_fs_buf_t;

static void ml_uv_fs_read_cb(ml_uv_fs_buf_t *Request) {
	ml_state_t *Caller = (ml_state_t *)Request->Base.data;
	ml_value_t *Result;
	if (Request->Base.result > 0) {
		Result = ml_string(Request->IOV[0].base, Request->Base.result);
	} else {
		Result = ml_error("ReadError", "error reading from file");
	}
	Caller->run(Caller, Result);
	uv_fs_req_cleanup((uv_fs_t *)Request);
}

static ml_value_t *ml_uv_fs_read(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ml_uv_file_t *File = (ml_uv_file_t *)Args[0];
	size_t Length = ml_integer_value(Args[1]);
	ml_uv_fs_buf_t *Request = xnew(ml_uv_fs_buf_t, 1, uv_buf_t);
	Request->Base.data = Caller;
	Request->IOV[0].base = GC_malloc_atomic(Length);
	Request->IOV[0].len = Length;
	uv_fs_read(Loop, (uv_fs_t *)Request, File->Handle, Request->IOV, 1, -1, (uv_fs_cb)ml_uv_fs_read_cb);
	return MLNil;
}

static void ml_uv_fs_write_cb(ml_uv_fs_buf_t *Request) {
	ml_state_t *Caller = (ml_state_t *)Request->Base.data;
	ml_value_t *Result;
	if (Request->Base.result > 0) {
		Result = ml_integer(Request->Base.result);
	} else {
		Result = ml_error("WriteError", "error writing to file");
	}
	Caller->run(Caller, Result);
	uv_fs_req_cleanup((uv_fs_t *)Request);
}

static ml_value_t *ml_uv_fs_write(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ml_uv_file_t *File = (ml_uv_file_t *)Args[0];
	ml_uv_fs_buf_t *Request = xnew(ml_uv_fs_buf_t, 1, uv_buf_t);
	Request->Base.data = Caller;
	Request->IOV[0].base = ml_string_value(Args[1]);
	Request->IOV[0].len = ml_string_length(Args[1]);
	uv_fs_write(Loop, (uv_fs_t *)Request, File->Handle, Request->IOV, 1, -1, (uv_fs_cb)ml_uv_fs_write_cb);
	return MLNil;
}

static void ml_uv_sleep_cb(uv_timer_t *Timer) {
	ml_state_t *Caller = (ml_state_t *)Timer->data;
	Caller->run(Caller, MLNil);
}

static ml_value_t *ml_uv_sleep(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIntegerT);
	uv_timer_t *Timer = new(uv_timer_t);
	uv_timer_init(Loop, Timer);
	Timer->data = Caller;
	uv_timer_start(Timer, ml_uv_sleep_cb, ml_integer_value(Args[0]), 0);
	return MLNil;
}

void *ml_calloc(size_t Count, size_t Size) {
	return GC_malloc(Count * Size);
}

void ml_free(void *Ptr) {
}

void ml_uv_init(stringmap_t *Globals) {
	uv_replace_allocator(GC_malloc, GC_realloc, ml_calloc, ml_free);
	Loop = uv_default_loop();
	MLUVFileT = ml_type(MLAnyT, "uv-file");
	ml_methodx_by_name("close", NULL, ml_uv_fs_close, MLUVFileT, NULL);
	ml_methodx_by_name("read", NULL, ml_uv_fs_read, MLUVFileT, MLIntegerT, NULL);
	ml_methodx_by_name("write", NULL, ml_uv_fs_write, MLUVFileT, MLStringT, NULL);
	stringmap_insert(Globals, "uv_fs_open", ml_functionx(NULL, ml_uv_fs_open));
	stringmap_insert(Globals, "uv_run", ml_function(NULL, ml_uv_run));
	stringmap_insert(Globals, "uv_sleep", ml_functionx(NULL, ml_uv_sleep));
}

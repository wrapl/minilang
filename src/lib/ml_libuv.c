#include "../ml_library.h"
#include "../ml_macros.h"
#include <gc/gc.h>
#include <uv.h>
#include <unistd.h>
#include <fcntl.h>

#define CB(TYPE, BODY) ({ \
	void anonymous(TYPE *Request) BODY \
	&anonymous; \
})

static uv_loop_t *Loop;
static uv_idle_t Idle[1];

static unsigned int MLUVCounter = 100;

static void ml_uv_resume(uv_idle_t *Idle) {
	ml_queued_state_t QueuedState = ml_scheduler_queue_next();
	if (QueuedState.State) {
		MLUVCounter = 100;
		QueuedState.State->run(QueuedState.State, QueuedState.Value);
	} else {
		uv_idle_stop(Idle);
	}
}

static void ml_uv_swap(ml_state_t *State, ml_value_t *Value) {
	if (ml_scheduler_queue_add(State, Value) == 1) {
		uv_idle_start(Idle, ml_uv_resume);
	}
}

static ml_schedule_t ml_uv_scheduler(ml_context_t *Context) {
	return (ml_schedule_t){&MLUVCounter, ml_uv_swap};
}

ML_FUNCTIONX(Run) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLFunctionT);
	ml_state_t *State = ml_state_new(Caller);
	ml_context_set(State->Context, ML_SCHEDULER_INDEX, ml_uv_scheduler);
	ml_value_t *Function = Args[0];
	ml_call(State, Function, 0, NULL);
	uv_run(Loop, UV_RUN_DEFAULT);
}

typedef struct ml_uv_file_t {
	const ml_type_t *Type;
	ssize_t Handle;
} ml_uv_file_t;

ML_TYPE(MLUVFileT, (), "uv-file");

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

ML_FUNCTIONX(FSOpen) {
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
}

static void ml_uv_fs_close_cb(uv_fs_t *Request) {
	ml_state_t *Caller = (ml_state_t *)Request->data;
	Caller->run(Caller, MLNil);
	uv_fs_req_cleanup(Request);
}

ML_METHODX("close", MLUVFileT) {
	ml_uv_file_t *File = (ml_uv_file_t *)Args[0];
	uv_fs_t *Request = new(uv_fs_t);
	Request->data = Caller;
	uv_fs_close(Loop, Request, File->Handle, ml_uv_fs_close_cb);
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

ML_METHODX("read", MLUVFileT, MLIntegerT) {
	ml_uv_file_t *File = (ml_uv_file_t *)Args[0];
	size_t Length = ml_integer_value(Args[1]);
	ml_uv_fs_buf_t *Request = xnew(ml_uv_fs_buf_t, 1, uv_buf_t);
	Request->Base.data = Caller;
	Request->IOV[0].base = GC_MALLOC_ATOMIC(Length);
	Request->IOV[0].len = Length;
	uv_fs_read(Loop, (uv_fs_t *)Request, File->Handle, Request->IOV, 1, -1, (uv_fs_cb)ml_uv_fs_read_cb);
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

ML_METHODX("write", MLUVFileT, MLStringT) {
	ml_uv_file_t *File = (ml_uv_file_t *)Args[0];
	ml_uv_fs_buf_t *Request = xnew(ml_uv_fs_buf_t, 1, uv_buf_t);
	Request->Base.data = Caller;
	Request->IOV[0].base = (char *)ml_string_value(Args[1]);
	Request->IOV[0].len = ml_string_length(Args[1]);
	uv_fs_write(Loop, (uv_fs_t *)Request, File->Handle, Request->IOV, 1, -1, (uv_fs_cb)ml_uv_fs_write_cb);
}

static void ml_uv_sleep_cb(uv_timer_t *Timer) {
	ml_state_t *Caller = (ml_state_t *)Timer->data;
	Caller->run(Caller, MLNil);
}

ML_FUNCTIONX(Sleep) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIntegerT);
	uv_timer_t *Timer = new(uv_timer_t);
	uv_timer_init(Loop, Timer);
	Timer->data = Caller;
	uv_timer_start(Timer, ml_uv_sleep_cb, ml_integer_value(Args[0]), 0);
}

void *ml_calloc(size_t Count, size_t Size) {
	return GC_malloc(Count * Size);
}

void ml_free(void *Ptr) {
}

void ml_library_entry0(ml_value_t **Slot) {
	uv_replace_allocator(GC_malloc, GC_realloc, ml_calloc, ml_free);
	Loop = uv_default_loop();
	uv_idle_init(Loop, Idle);
	Idle->data = ml_list();
#include "ml_libuv_init.c"
	Slot[0] = ml_module("libuv",
		"fs_open", FSOpen,
		"run", Run,
		"sleep", Sleep,
	NULL);
}

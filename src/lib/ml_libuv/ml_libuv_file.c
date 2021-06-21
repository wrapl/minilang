#include "ml_libuv.h"

static ml_value_t *ml_uv_file_new(ssize_t Handle);

static void ml_uv_fs_open_cb(uv_fs_t *Request) {
	ml_state_t *Caller = (ml_state_t *)Request->data;
	ml_value_t *Result;
	if (Request->result >= 0) {
		Result = ml_uv_file_new(Request->result);
	} else {
		Result = ml_error("OpenError", "error opening file %s", Request->path);
	}
	ml_scheduler_queue_add(Caller, Result);
	uv_fs_req_cleanup(Request);
}

ML_FUNCTIONX(UVFileOpen) {
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

ML_TYPE(UVFileT, (), "uv-file",
	.Constructor = (ml_value_t *)UVFileOpen
);

static ml_value_t *ml_uv_file_new(ssize_t Handle) {
	ml_uv_file_t *File = new(ml_uv_file_t);
	File->Type = UVFileT;
	File->Handle = Handle;
	return (ml_value_t *)File;
}

static void ml_uv_fs_close_cb(uv_fs_t *Request) {
	ml_state_t *Caller = (ml_state_t *)Request->data;
	ml_scheduler_queue_add(Caller, MLNil);
	uv_fs_req_cleanup(Request);
}

ML_METHODX("close", UVFileT) {
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
	ml_scheduler_queue_add(Caller, Result);
	uv_fs_req_cleanup((uv_fs_t *)Request);
}

ML_METHODX("read", UVFileT, MLIntegerT) {
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
	ml_scheduler_queue_add(Caller, Result);
	uv_fs_req_cleanup((uv_fs_t *)Request);
}

ML_METHODX("write", UVFileT, MLStringT) {
	ml_uv_file_t *File = (ml_uv_file_t *)Args[0];
	ml_uv_fs_buf_t *Request = xnew(ml_uv_fs_buf_t, 1, uv_buf_t);
	Request->Base.data = Caller;
	Request->IOV[0].base = (char *)ml_string_value(Args[1]);
	Request->IOV[0].len = ml_string_length(Args[1]);
	uv_fs_write(Loop, (uv_fs_t *)Request, File->Handle, Request->IOV, 1, -1, (uv_fs_cb)ml_uv_fs_write_cb);
}

void ml_libuv_file_init(stringmap_t *Globals) {
#include "ml_libuv_file_init.c"
	if (Globals) {
		stringmap_insert(Globals, "file", UVFileT);
	}
}

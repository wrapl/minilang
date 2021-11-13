#include "ml_libuv.h"

ML_TYPE(UVPipeT, (UVStreamT), "uv-pipe");

ML_METHOD(UVPipeT) {
	ml_uv_handle_t *S = new(ml_uv_handle_t);
	S->Type = UVPipeT;
	uv_pipe_init(Loop, &S->Handle.pipe, 0);
	S->Handle.pipe.data = S;
	return (ml_value_t *)S;
}

ML_METHOD(UVPipeT, MLBooleanT) {
	ml_uv_handle_t *S = new(ml_uv_handle_t);
	S->Type = UVPipeT;
	uv_pipe_init(Loop, &S->Handle.pipe, Args[0] == (ml_value_t *)MLTrue);
	S->Handle.pipe.data = S;
	return (ml_value_t *)S;
}

ML_METHOD("open", UVPipeT, UVFileT) {
	ml_uv_handle_t *S = (ml_uv_handle_t *)Args[0];
	ml_uv_file_t *File = (ml_uv_file_t *)Args[1];
	uv_pipe_open(&S->Handle.pipe, File->Handle);
	return Args[0];
}

ML_METHOD("bind", UVPipeT, MLStringT) {
	ml_uv_handle_t *S = (ml_uv_handle_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	uv_pipe_bind(&S->Handle.pipe, Name);
	return Args[0];
}

static void ml_uv_pipe_connect_cb(uv_connect_t *Request, int Status) {
	ml_state_t *Caller = (ml_state_t *)Request->data;
	ml_default_queue_add(Caller, MLNil);
}

ML_METHODX("connect", UVPipeT, MLStringT) {
	ml_uv_handle_t *S = (ml_uv_handle_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	uv_connect_t *Request = new(uv_connect_t);
	Request->data = Caller;
	uv_pipe_connect(Request, &S->Handle.pipe, Name, ml_uv_pipe_connect_cb);
}

void ml_libuv_pipe_init(stringmap_t *Globals) {
#include "ml_libuv_pipe_init.c"
	if (Globals) {
		stringmap_insert(Globals, "pipe", UVPipeT);
	}
}

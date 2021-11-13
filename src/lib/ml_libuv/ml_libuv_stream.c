#include "ml_libuv.h"
#include <stdio.h>
#include <string.h>
#include <gc/gc_typed.h>

ML_TYPE(UVStreamT, (UVHandleT), "uv-stream");

static void ml_uv_connection_cb(uv_stream_t *Server, int Status) {
	ml_uv_handle_t *C = new(ml_uv_handle_t);
	switch (Server->type) {
	case UV_NAMED_PIPE: {
		C->Type = UVPipeT;
		uv_pipe_init(Loop, &C->Handle.pipe, ((uv_pipe_t *)Server)->ipc);
		break;
	}
	case UV_TCP: {
		C->Type = UVTcpT;
		uv_tcp_init(Loop, &C->Handle.tcp);
		break;
	}
	default: {
		printf("Unknown stream type for accept: %s\n", uv_handle_type_name(Server->type));
		return;
	}
	}
	uv_accept(Server, &C->Handle.stream);
	ml_value_t **Args = ml_alloc_args(1);
	Args[0] = (ml_value_t *)C;
	ml_uv_handle_t *S = (ml_uv_handle_t *)Server->data;
	ml_state_t *Caller = (ml_state_t *)ml_result_state_new(S->Context);
	ml_call(Caller, S->Callback, 1, Args);
}

ML_METHODX("listen", UVStreamT, MLIntegerT, MLAnyT) {
	ml_uv_handle_t *S = (ml_uv_handle_t *)Args[0];
	int Backlog = ml_integer_value(Args[1]);
	S->Context = Caller->Context;
	S->Callback = Args[2];
	uv_listen(&S->Handle.stream, Backlog, ml_uv_connection_cb);
	ML_RETURN(Args[0]);
}

#define ML_UV_BUFFER_SIZE 384

typedef struct ml_uv_buffer_t ml_uv_buffer_t;

struct ml_uv_buffer_t {
	ml_uv_buffer_t *Next;
	char Buffer[ML_UV_BUFFER_SIZE];
};

static GC_descr BufferDesc = 0;
static ml_uv_buffer_t *CachedBuffers = 0;

static void ml_uv_alloc_cb(uv_handle_t *Handle, size_t Size, uv_buf_t *Buffer) {
	ml_uv_buffer_t *Cached = CachedBuffers;
	if (Cached) {
		CachedBuffers = Cached->Next;
	} else {
		Cached = (ml_uv_buffer_t *)GC_MALLOC_EXPLICITLY_TYPED(sizeof(ml_uv_buffer_t), BufferDesc);
	}
	Buffer->base = Cached->Buffer;
	Buffer->len = ML_UV_BUFFER_SIZE;
}

static void ml_uv_read_cb(uv_stream_t *Stream, ssize_t Count, const uv_buf_t *Buffer) {
	ml_value_t **Args = ml_alloc_args(1);
	if (Count == UV_EOF) {
		Args[0] = MLNil;
	} else if (Count > 0) {
		char *String = GC_malloc_atomic(Count + 1);
		memcpy(String, Buffer->base, Count);
		String[Count] = 0;
		Args[0] = ml_string(String, Count);
	} else {
		Args[0] = ml_error("ReadError", "%s", uv_strerror(Count));
	}
	ml_uv_buffer_t *Cached = (ml_uv_buffer_t *)((void *)Buffer - offsetof(ml_uv_buffer_t, Buffer));
	Cached->Next = CachedBuffers;
	CachedBuffers = Cached;
	ml_uv_handle_t *S = (ml_uv_handle_t *)Stream->data;
	ml_state_t *Caller = (ml_state_t *)ml_result_state_new(S->Context);
	ml_call(Caller, S->Callback, 1, Args);
}

ML_METHODX("read", UVStreamT, MLAnyT) {
	ml_uv_handle_t *S = (ml_uv_handle_t *)Args[0];
	S->Context = Caller->Context;
	S->Callback = Args[1];
	uv_read_start(&S->Handle.stream, ml_uv_alloc_cb, ml_uv_read_cb);
	ML_RETURN(Args[0]);
}

ML_METHOD("stop", UVStreamT) {
	ml_uv_handle_t *S = (ml_uv_handle_t *)Args[0];
	uv_read_stop(&S->Handle.stream);
	return Args[0];
}

typedef struct {
	uv_write_t Base;
	uv_buf_t IOV[];
} ml_uv_write_t;

static void ml_uv_write_cb(ml_uv_write_t *Request, int Status) {
	ml_state_t *Caller = (ml_state_t *)Request->Base.data;
	ml_value_t *Result;
	if (Request->Base.error < 0) {
		Result = ml_error("WriteError", "error writing to file");
	} else {
		Result = MLNil;
	}
	ml_default_queue_add(Caller, Result);
}

ML_METHODX("write", UVStreamT, MLAddressT) {
	ml_uv_handle_t *S = (ml_uv_handle_t *)Args[0];
	ml_uv_write_t *Request = xnew(ml_uv_write_t, 1, uv_buf_t);
	Request->Base.data = Caller;
	Request->IOV[0].base = (char *)ml_address_value(Args[1]);
	Request->IOV[1].len = ml_address_length(Args[1]);
	uv_write((uv_write_t *)Request, &S->Handle.stream, Request->IOV, 1, (uv_write_cb)ml_uv_write_cb);
}

void ml_libuv_stream_init(stringmap_t *Globals) {
	GC_word BufferLayout[] = {1};
	BufferDesc = GC_make_descriptor(BufferLayout, 1);
#include "ml_libuv_stream_init.c"
	if (Globals) {
		stringmap_insert(Globals, "stream", UVStreamT);
	}
}

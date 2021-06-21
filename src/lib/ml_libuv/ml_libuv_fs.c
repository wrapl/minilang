#include "ml_libuv.h"

static void ml_uv_fs_operation_cb(uv_fs_t *Request) {
	ml_state_t *Caller = (ml_state_t *)Request->data;
	ml_value_t *Result = MLNil;
	if (Request->result) {
		Result = ml_error("FSError", "%s", uv_strerror(Request->result));
	}
	ml_scheduler_queue_add(Caller, Result);
	uv_fs_req_cleanup(Request);
}

ML_FUNCTIONX(UVUnlink) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	const char *Path = ml_string_value(Args[0]);
	uv_fs_t *Request = new(uv_fs_t);
	uv_fs_unlink(Loop, Request, Path, ml_uv_fs_operation_cb);
}

ML_FUNCTIONX(UVCopyFile) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	ML_CHECKX_ARG_TYPE(1, MLStringT);
	const char *Path = ml_string_value(Args[0]);
	const char *NewPath = ml_string_value(Args[1]);
	uv_fs_t *Request = new(uv_fs_t);
	uv_fs_unlink(Loop, Request, Path, ml_uv_fs_operation_cb);
}


void ml_libuv_file_init(stringmap_t *Globals) {
#include "ml_libuv_file_init.c"
	if (Globals) {
		//stringmap_insert(Globals, "file", UVFileT);
	}
}

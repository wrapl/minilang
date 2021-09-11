#ifndef ML_LIBUV_H
#define ML_LIBUV_H

#include "../../ml_library.h"
#include "../../ml_macros.h"
#include <uv.h>

typedef struct {
	ml_type_t *Type;
	uv_file Handle;
} ml_uv_file_t;

extern ml_type_t UVFileT[];

typedef struct {
	ml_type_t *Type;
	ml_context_t *Context;
	ml_value_t *Callback;
	union uv_any_handle Handle;
} ml_uv_handle_t;

extern ml_type_t UVHandleT[];
extern ml_type_t UVStreamT[];
extern ml_type_t UVPipeT[];
extern ml_type_t UVTcpT[];

extern uv_loop_t *Loop;

#endif

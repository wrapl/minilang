#ifndef ML_LIBUV_H
#define ML_LIBUV_H

#include "../../ml_library.h"
#include "../../ml_macros.h"
#include <uv.h>

typedef struct ml_uv_file_t {
	const ml_type_t *Type;
	uv_file Handle;
} ml_uv_file_t;

extern ml_type_t UVFileT[];

extern uv_loop_t *Loop;

#endif

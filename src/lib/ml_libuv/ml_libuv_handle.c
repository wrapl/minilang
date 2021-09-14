#include "ml_libuv.h"

ML_TYPE(UVHandleT, (), "uv-handle");

void ml_libuv_handle_init(stringmap_t *Globals) {
#include "ml_libuv_handle_init.c"
	if (Globals) {
		stringmap_insert(Globals, "handle", UVStreamT);
	}
}

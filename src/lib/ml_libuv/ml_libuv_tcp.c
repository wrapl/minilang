#include "ml_libuv.h"

ML_TYPE(UVTcpT, (UVStreamT), "uv-tcp");

ML_METHOD(UVTcpT) {
	ml_uv_handle_t *S = new(ml_uv_handle_t);
	S->Type = UVTcpT;
	uv_tcp_init(Loop, &S->Handle.tcp);
	S->Handle.tcp.data = S;
	return (ml_value_t *)S;
}

void ml_libuv_tcp_init(stringmap_t *Globals) {
#include "ml_libuv_tcp_init.c"
	if (Globals) {
		stringmap_insert(Globals, "tcp", UVTcpT);
	}
}

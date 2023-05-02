#include "ml_socket.h"
#include "ml_stream.h"
#include "ml_macros.h"

ML_TYPE(MLSocketT, (MLStreamFdT), "socket");

void ml_socket_init(stringmap_t *Globals) {
#include "ml_socket_init.c"
}

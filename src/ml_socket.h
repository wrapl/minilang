#ifndef ML_SOCKET_H
#define ML_SOCKET_H

#include <stdio.h>
#include "minilang.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ml_type_t MLSocketT[];

void ml_socket_init(stringmap_t *Globals);

#ifdef __cplusplus
}
#endif

#endif

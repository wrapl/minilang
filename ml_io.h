#ifndef ML_IO_H
#define ML_IO_H

#include "minilang.h"

void ml_io_init(stringmap_t *Globals);

extern ml_type_t *MLStreamT;
extern ml_type_t *MLFdT;
ml_value_t *ml_fd_new(int Fd);

#endif

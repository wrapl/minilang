#ifndef ML_FILE_H
#define ML_FILE_H

#include <stdio.h>
#include "minilang.h"

#ifdef	__cplusplus
extern "C" {
#endif

extern ml_type_t *MLFileT;

void ml_file_init(stringmap_t *Globals);

ml_value_t *ml_file_new(FILE *File);
ml_value_t *ml_file_open(void *Data, int Count, ml_value_t **Args);

FILE *ml_file_handle(ml_value_t *Value);

#ifdef	__cplusplus
}
#endif

#endif
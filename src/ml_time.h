#ifndef ML_TIME_H
#define ML_TIME_H

#include "minilang.h"
#include <time.h>

extern ml_type_t MLTimeT[];

void ml_time_init(stringmap_t *Globals);
ml_value_t *ml_time(time_t Sec, unsigned long NSec);
ml_value_t *ml_time_parse(const char *Value, int Length);

#endif

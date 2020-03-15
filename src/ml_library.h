#ifndef ML_LIBRARY_H
#define ML_LIBRARY_H

#include "minilang.h"

void ml_library_init(stringmap_t *Globals);

void ml_library_entry(ml_value_t *, ml_getter_t, void *);

ml_value_t *ml_library_load_file(ml_state_t *Caller, const char *FileName, ml_getter_t GlobalGet, void *Globals, ml_value_t **Slot);

#endif

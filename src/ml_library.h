#ifndef ML_LIBRARY_H
#define ML_LIBRARY_H

#include "minilang.h"

#ifdef __cplusplus
extern "C" {
#endif

void ml_library_init(stringmap_t *Globals);

void ml_library_path_add(const char *Path);
void ml_library_loader_add(
	const char *Extension, int (*Test)(const char *),
	void (*Load)(ml_state_t *, const char *, ml_value_t **),
	ml_value_t *(*Load0)(const char *, ml_value_t **)
);

void ml_library_load(ml_state_t *Caller, const char *Path, const char *Name);
ml_value_t *ml_library_load0(const char *Path, const char *Name);

ml_module_t *ml_library_internal(const char *Name);

void ml_library_entry(ml_state_t *Caller, ml_value_t **Slot);
void ml_library_entry0(ml_value_t **Slot);

#ifdef __cplusplus
}
#endif

#endif

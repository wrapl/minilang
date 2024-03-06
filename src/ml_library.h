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

void ml_library_register(const char *Name, ml_value_t *Module);
ml_module_t *ml_library_internal(const char *Name);

typedef void (*ml_library_entry_t)(ml_state_t *Caller, ml_value_t **Slot);
typedef void (*ml_library_entry0_t)(ml_value_t **Slot);

#define STRINGIFY(x) #x
#define TOSTRING3(x, y, z) STRINGIFY(x ## y ## z)

#define LIBRARY_ENTRY(NAME) TOSTRING3(ml_, NAME, _entry)
#define LIBRARY_ENTRY0(NAME) TOSTRING3(ml_, NAME, _entry0)

#define ML_LIBRARY_ENTRY(NAME) \
void CONCAT3(ml_, NAME, entry)(ml_state_t *Caller, ml_value_t **Slot); \
void ml_library_entry(ml_state_t *Caller, ml_value_t **Slot) __attribute__ ((weak, alias(LIBRARY_ENTRY(NAME)))); \
void CONCAT3(ml_, NAME, entry)(ml_state_t *Caller, ml_value_t **Slot)

#define ML_LIBRARY_ENTRY0(NAME) \
void CONCAT3(ml_, NAME, entry0)(ml_value_t **Slot); \
void ml_library_entry0(ml_value_t **Slot) __attribute__ ((weak, alias(LIBRARY_ENTRY0(NAME)))); \
void CONCAT3(ml_, NAME, entry0)(ml_value_t **Slot)

typedef void (*ml_main_loop_t)();

#define ML_LIBRARY_SCHEDULER(NAME) \
ml_main_loop_t ml_library_scheduler_init(ml_context_t *Context)

#define ML_LIBRARY_LOGGER(NAME) \
void ml_library_logger_init(ml_context_t *Context)

#ifdef __cplusplus
}
#endif

#endif

#ifndef ML_COMPILER_H
#define ML_COMPILER_H

#include <setjmp.h>

#include "ml_runtime.h"
#include "stringmap.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct mlc_scanner_t mlc_scanner_t;

mlc_scanner_t *ml_scanner(const char *SourceName, void *Data, const char *(*read)(void *), ml_getter_t GlobalGet, void *Globals);
ml_source_t ml_scanner_source(mlc_scanner_t *Scanner, ml_source_t Source);
void ml_scanner_reset(mlc_scanner_t *Scanner);
const char *ml_scanner_clear(mlc_scanner_t *Scanner);
void ml_scanner_error(mlc_scanner_t *Scanner, const char *Error, const char *Format, ...) __attribute__((noreturn));

void ml_function_compile(ml_state_t *Caller, mlc_scanner_t *Scanner, const char **Parameters);
void ml_command_evaluate(ml_state_t *Caller, mlc_scanner_t *Scanner, stringmap_t *Vars);

void ml_load(ml_state_t *Caller, ml_getter_t GlobalGet, void *Globals, const char *FileName, const char *Parameters[]);

void ml_compiler_init();

#ifdef	__cplusplus
}
#endif

#endif

#ifndef ML_COMPILER_H
#define ML_COMPILER_H

#include <setjmp.h>

#include "ml_runtime.h"
#include "stringmap.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct ml_compiler_t ml_compiler_t;

typedef const char *(*ml_reader_t)(void *);

ml_compiler_t *ml_compiler(const char *SourceName, void *Data, ml_reader_t Read, ml_getter_t GlobalGet, void *Globals);
const char *ml_compiler_name(ml_compiler_t *Compiler);
ml_source_t ml_compiler_source(ml_compiler_t *Compiler, ml_source_t Source);
void ml_compiler_reset(ml_compiler_t *Compiler);
const char *ml_compiler_clear(ml_compiler_t *Compiler);
void ml_compiler_error(ml_compiler_t *Compiler, const char *Error, const char *Format, ...) __attribute__((noreturn)) ;

void ml_function_compile(ml_state_t *Caller, ml_compiler_t *Compiler, const char **Parameters);

extern ml_value_t MLEndOfInput[];

void ml_command_evaluate(ml_state_t *Caller, ml_compiler_t *Compiler, stringmap_t *Vars);

void ml_load_file(ml_state_t *Caller, ml_getter_t GlobalGet, void *Globals, const char *FileName, const char *Parameters[]);

void ml_compiler_init();

#ifdef	__cplusplus
}
#endif

#endif

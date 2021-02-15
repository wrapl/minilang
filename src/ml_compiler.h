#ifndef ML_COMPILER_H
#define ML_COMPILER_H

#include <setjmp.h>

#include "ml_runtime.h"
#include "stringmap.h"

#ifdef	__cplusplus
extern "C" {
#endif

extern ml_value_t MLEndOfInput[];
extern ml_value_t MLNotFound[];
extern ml_type_t MLCompilerT[];

typedef struct ml_compiler_t ml_compiler_t;

typedef const char *(*ml_reader_t)(void *);

ml_compiler_t *ml_compiler(ml_getter_t GlobalGet, void *Globals, ml_reader_t Read, void *Data);
void ml_compiler_define(ml_compiler_t *Compiler, const char *Name, ml_value_t *Value);
ml_value_t *ml_compiler_lookup(ml_compiler_t *Compiler, const char *Name);
const char *ml_compiler_name(ml_compiler_t *Compiler);
ml_source_t ml_compiler_source(ml_compiler_t *Compiler, ml_source_t Source);
void ml_compiler_reset(ml_compiler_t *Compiler);
void ml_compiler_input(ml_compiler_t *Compiler, const char *Text);
const char *ml_compiler_clear(ml_compiler_t *Compiler);
void ml_compiler_error(ml_compiler_t *Compiler, const char *Error, const char *Format, ...) __attribute__((noreturn));

void ml_function_compile(ml_state_t *Caller, ml_compiler_t *Compiler, const char **Parameters);
void ml_command_evaluate(ml_state_t *Caller, ml_compiler_t *Compiler);
void ml_load_file(ml_state_t *Caller, ml_getter_t GlobalGet, void *Globals, const char *FileName, const char *Parameters[]);

ml_value_t *ml_stringmap_globals(stringmap_t *Globals);

void ml_compiler_init();

#ifdef	__cplusplus
}
#endif

#endif

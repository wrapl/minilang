#ifndef ML_COMPILER_H
#define ML_COMPILER_H

#include <setjmp.h>

#include "ml_types.h"
#include "ml_runtime.h"
#include "stringmap.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct mlc_expr_t mlc_expr_t;
typedef struct mlc_scanner_t mlc_scanner_t;

typedef struct mlc_context_t {
	ml_getter_t GlobalGet;
	void *Globals;
	ml_value_t *Error;
	jmp_buf OnError;
} mlc_context_t;

#define MLC_ON_ERROR(CONTEXT) if (setjmp(CONTEXT->OnError))

mlc_scanner_t *ml_scanner(const char *SourceName, void *Data, const char *(*read)(void *), mlc_context_t *Context);
ml_source_t ml_scanner_source(mlc_scanner_t *Scanner, ml_source_t Source);
void ml_scanner_reset(mlc_scanner_t *Scanner);
const char *ml_scanner_clear(mlc_scanner_t *Scanner);

ml_value_t *ml_compile(mlc_expr_t *Expr, const char **Parameters, mlc_context_t *Context);

void ml_function_compile(ml_state_t *Caller, mlc_scanner_t *Scanner, const char **Parameters);
void ml_command_evaluate(ml_state_t *Caller, mlc_scanner_t *Scanner, stringmap_t *Vars);

#ifdef	__cplusplus
}
#endif

#endif

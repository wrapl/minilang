#ifndef ML_COMPILER_H
#define ML_COMPILER_H

#include <setjmp.h>

#include "stringmap.h"
#include "ml_types.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct mlc_expr_t mlc_expr_t;
typedef struct mlc_scanner_t mlc_scanner_t;

typedef struct mlc_error_t {
	ml_value_t *Message;
	jmp_buf Handler;
} mlc_error_t;

typedef struct mlc_context_t {
	ml_getter_t GlobalGet;
	void *Globals;
	mlc_error_t Error[1];
} mlc_context_t;

mlc_scanner_t *ml_scanner(const char *SourceName, void *Data, const char *(*read)(void *), mlc_context_t *Context);
ml_source_t ml_scanner_source(mlc_scanner_t *Scanner, ml_source_t Source);
void ml_scanner_reset(mlc_scanner_t *Scanner);
const char *ml_scanner_clear(mlc_scanner_t *Scanner);

void ml_accept_eoi(mlc_scanner_t *Scanner);
mlc_expr_t *ml_accept_block(mlc_scanner_t *Scanner);
mlc_expr_t *ml_accept_command(mlc_scanner_t *Scanner, stringmap_t *Vars);

ml_value_t *ml_compile(mlc_expr_t *Expr, const char **Parameters, mlc_context_t *Context);

extern int MLDebugClosures;

void ml_closure_debug(ml_value_t *Value);

#ifdef	__cplusplus
}
#endif

#endif

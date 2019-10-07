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

ml_value_t *ml_compile(mlc_expr_t *Expr, ml_getter_t GlobalGet, void *Globals, const char **Parameters, mlc_error_t *Error);

mlc_scanner_t *ml_scanner(const char *SourceName, void *Data, const char *(*read)(void *), mlc_error_t *Error);
ml_source_t ml_scanner_source(mlc_scanner_t *Scanner, ml_source_t Source);
void ml_scanner_reset(mlc_scanner_t *Scanner);
const char *ml_scanner_clear(mlc_scanner_t *Scanner);

void ml_accept_eoi(mlc_scanner_t *Scanner);
mlc_expr_t *ml_accept_block(mlc_scanner_t *Scanner);
mlc_expr_t *ml_accept_command(mlc_scanner_t *Scanner, stringmap_t *Vars);

extern int MLDebugClosures;

void ml_closure_debug(ml_value_t *Value);

#ifdef	__cplusplus
}
#endif

#endif

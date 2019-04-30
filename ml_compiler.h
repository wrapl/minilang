#ifndef ML_COMPILER_H
#define ML_COMPILER_H

#include <setjmp.h>

#include "ml_types.h"
#include "ml_runtime.h"
#include "stringmap.h"

typedef struct mlc_expr_t mlc_expr_t;
typedef struct mlc_scanner_t mlc_scanner_t;

typedef struct mlc_error_t {
	ml_value_t *Message;
	jmp_buf Handler;
} mlc_error_t;

ml_value_t *ml_compile(mlc_expr_t *Expr, ml_getter_t GlobalGet, void *Globals, mlc_error_t *Error);

mlc_scanner_t *ml_scanner(const char *SourceName, void *Data, const char *(*read)(void *), mlc_error_t *Error);
void ml_scanner_reset(mlc_scanner_t *Scanner);
const char *ml_scanner_clear(mlc_scanner_t *Scanner);

void ml_accept_eoi(mlc_scanner_t *Scanner);
mlc_expr_t *ml_accept_block(mlc_scanner_t *Scanner);
mlc_expr_t *ml_accept_command(mlc_scanner_t *Scanner, stringmap_t *Vars);

extern int MLDebugClosures;

#endif

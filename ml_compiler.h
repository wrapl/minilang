#ifndef ML_COMPILER_H
#define ML_COMPILER_H

#include <setjmp.h>

#include "ml_types.h"
#include "ml_runtime.h"
#include "stringmap.h"

typedef enum ml_token_t {
	MLT_NONE,
	MLT_EOL,
	MLT_EOI,
	MLT_IF,
	MLT_THEN,
	MLT_ELSEIF,
	MLT_ELSE,
	MLT_END,
	MLT_LOOP,
	MLT_WHILE,
	MLT_UNTIL,
	MLT_EXIT,
	MLT_NEXT,
	MLT_FOR,
	MLT_ALL,
	MLT_IN,
	MLT_IS,
	MLT_FUN,
	MLT_RET,
	MLT_WITH,
	MLT_DO,
	MLT_ON,
	MLT_NIL,
	MLT_AND,
	MLT_OR,
	MLT_NOT,
	MLT_OLD,
	MLT_DEF,
	MLT_VAR,
	MLT_IDENT,
	MLT_LEFT_PAREN,
	MLT_RIGHT_PAREN,
	MLT_LEFT_SQUARE,
	MLT_RIGHT_SQUARE,
	MLT_LEFT_BRACE,
	MLT_RIGHT_BRACE,
	MLT_SEMICOLON,
	MLT_COLON,
	MLT_COMMA,
	MLT_ASSIGN,
	MLT_SYMBOL,
	MLT_VALUE,
	MLT_EXPR,
	MLT_OPERATOR,
	MLT_METHOD
} ml_token_t;

typedef struct mlc_expr_t mlc_expr_t;
typedef struct mlc_scanner_t mlc_scanner_t;
typedef struct mlc_function_t mlc_function_t;
typedef struct mlc_decl_t mlc_decl_t;
typedef struct mlc_loop_t mlc_loop_t;
typedef struct mlc_try_t mlc_try_t;
typedef struct mlc_upvalue_t mlc_upvalue_t;

typedef struct { ml_inst_t *Start, *Exits; } mlc_compiled_t;

typedef struct mlc_error_t {
	ml_value_t *Message;
	jmp_buf Handler;
} mlc_error_t;

struct mlc_function_t {
	mlc_error_t *Error;
	ml_getter_t GlobalGet;
	void *Globals;
	mlc_function_t *Up;
	mlc_decl_t *Decls;
	mlc_loop_t *Loop;
	mlc_try_t *Try;
	mlc_upvalue_t *UpValues;
	int Top, Size, Self;
};

struct mlc_scanner_t {
	mlc_error_t *Error;
	const char *Next;
	ml_source_t Source;
	ml_token_t Token;
	ml_value_t *Value;
	mlc_expr_t *Expr;
	const char *Ident;
	void *Data;
	const char *(*read)(void *);
};

mlc_scanner_t *ml_scanner(const char *SourceName, void *Data, const char *(*read)(void *), mlc_error_t *Error);

void ml_accept(mlc_scanner_t *Scanner, ml_token_t Token);
mlc_expr_t *ml_accept_block(mlc_scanner_t *Scanner);
mlc_expr_t *ml_accept_command(mlc_scanner_t *Scanner, stringmap_t *Vars);

mlc_compiled_t mlc_compile(mlc_function_t *Function, mlc_expr_t *Expr, SHA256_CTX *HashContext);
void mlc_connect(ml_inst_t *Exits, ml_inst_t *Start);

extern int MLDebugClosures;

#endif

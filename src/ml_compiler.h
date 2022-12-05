#ifndef ML_COMPILER_H
#define ML_COMPILER_H

#include <setjmp.h>

#include "ml_runtime.h"
#include "stringmap.h"

#ifdef __cplusplus
extern "C" {
#endif

extern ml_value_t MLEndOfInput[];
extern ml_value_t MLNotFound[];
extern ml_value_t *MLCompilerSwitch;
extern ml_type_t MLCompilerT[];
extern ml_type_t MLMacroT[];
extern ml_type_t MLParserT[];
extern ml_type_t MLGlobalT[];
extern ml_type_t MLExprT[];

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value;
} ml_inline_function_t;

extern ml_type_t MLFunctionInlineT[];

#define ML_FUNCTION_INLINE2(NAME, FUNCTION) static ml_value_t *FUNCTION(void *Data, int Count, ml_value_t **Args); \
\
ml_cfunction_t NAME ## _IMPL[1] = {{MLCFunctionT, FUNCTION, NULL}}; \
\
ml_inline_function_t NAME[1] = {{MLFunctionInlineT, (ml_value_t *)NAME ## _IMPL}}; \
\
static ml_value_t *FUNCTION(void *Data, int Count, ml_value_t **Args)

#define ML_FUNCTION_INLINE(NAME) ML_FUNCTION_INLINE2(NAME, CONCAT3(ml_cfunction_, __LINE__, __COUNTER__))


typedef struct ml_parser_t ml_parser_t;
typedef struct ml_compiler_t ml_compiler_t;

typedef struct mlc_expr_t mlc_expr_t;

typedef const char *(*ml_reader_t)(void *);

ml_parser_t *ml_parser(ml_reader_t Read, void *Data);
void ml_parser_reset(ml_parser_t *Compiler);
void ml_parser_input(ml_parser_t *Compiler, const char *Text);
const char *ml_parser_name(ml_parser_t *Compiler);
ml_source_t ml_parser_source(ml_parser_t *Compiler, ml_source_t Source);
ml_value_t *ml_parser_value(ml_parser_t *Parser);
const char *ml_parser_clear(ml_parser_t *Compiler);
void ml_parse_error(ml_parser_t *Compiler, const char *Error, const char *Format, ...) __attribute__((noreturn));
mlc_expr_t *ml_accept_file(ml_parser_t *Parser);

ml_compiler_t *ml_compiler(ml_getter_t GlobalGet, void *Globals);
void ml_compiler_define(ml_compiler_t *Compiler, const char *Name, ml_value_t *Value);
ml_value_t *ml_compiler_lookup(ml_compiler_t *Compiler, const char *Name, const char *Source, int Line);

void ml_function_compile(ml_state_t *Caller, mlc_expr_t *Expr, ml_compiler_t *Compiler, const char **Parameters);
void ml_command_evaluate(ml_state_t *Caller, ml_parser_t *Parser, ml_compiler_t *Compiler);
void ml_load_file(ml_state_t *Caller, ml_getter_t GlobalGet, void *Globals, const char *FileName, const char *Parameters[]);

ml_value_t *ml_stringmap_globals(stringmap_t *Globals);
ml_value_t *stringmap_global_get(const stringmap_t *Map, const char *Key, const char *Source, int Line);

typedef ml_value_t *(*string_fn_t)(const char *String, int Length);

void ml_string_fn_register(const char *Prefix, string_fn_t Fn);

void ml_compiler_init();

typedef struct ml_scope_macro_t ml_scope_macro_t;
ml_scope_macro_t *ml_scope_macro();
void ml_scope_macro_define(ml_scope_macro_t *Macro, const char *Name, ml_value_t *Value);

ml_value_t *ml_global(const char *Name);
ml_value_t *ml_global_get(ml_value_t *Global);
ml_value_t *ml_global_set(ml_value_t *Global, ml_value_t *Value);

ml_value_t *ml_macro(ml_value_t *Function);
ml_value_t *ml_inline_call_macro(ml_value_t *Value);

ml_value_t *ml_inline_function(ml_value_t *Value);

void ml_expr_evaluate(ml_state_t *Caller, ml_value_t *Expr);

#ifdef __cplusplus
}
#endif

#endif

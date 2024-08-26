#ifndef ML_COMPILER_H
#define ML_COMPILER_H

/// \defgroup compiler
/// @{
///

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

/**
 * Returns a new parser.
 *
 * @param Read Function to call to read the next line of source.
 * @param Data Data passed to \a Read.
 *
 * @return A new parser.
 */
ml_parser_t *ml_parser(ml_reader_t Read, void *Data);

/**
 * Resets the state of a parser.
 *
 * @param Parser Parser.
 */
void ml_parser_reset(ml_parser_t *Parser);

/**
 * Enables or disabled permissive parsing.
 *
 * When permissive parsing is enabled, parsing errors are stored as warnings instead
 * and an invalid expression is substituted. The parser output in this case will
 * cause an error when compiled. The list of warnings can be retrieved with
 * ml_parser_warnings().
 *
 * @param Parser Parser.
 * @param Permissive 0 - disable permissive mode (default), 1 - enable permissive mode.
 *
 * @see ml_parser_warnings
 */
void ml_parser_permissive(ml_parser_t *Parser, int Permissive);

/**
 * Returns a list (\a ml_list_t *) of the warnings generated while parsing. If permissive
 * parsing is disabled, this will always be an empty list.
 *
 * @param Parser Parser.
 *
 * @return List of warnings.
 *
 * @see ml_parser_permissive
 */
ml_value_t *ml_parser_warnings(ml_parser_t *Parser);

/**
 * Sets the next source text for parsing. Any existing unparsed source within parser is
 * discarded. \a Text can be a single line, contain multiple lines of source, or even be
 * the entire contents of a source file.
 *
 * End-of-line characters should *not* be stripped from \a Text. Currently, tokens cannot
 * be split across different calls to ml_parser_input().
 *
 * @param Parser Parser.
 * @param Text Source text.
 */
void ml_parser_input(ml_parser_t *Parser, const char *Text, int Advance);
const char *ml_parser_name(ml_parser_t *Parser);
ml_source_t ml_parser_position(ml_parser_t *Parser);
ml_source_t ml_parser_source(ml_parser_t *Parser, ml_source_t Source);
ml_value_t *ml_parser_value(ml_parser_t *Parser);
const char *ml_parser_clear(ml_parser_t *Parser);
const char *ml_parser_read(ml_parser_t *Parser);
//void ml_parse_error(ml_parser_t *Compiler, const char *Error, const char *Format, ...) __attribute__((noreturn));
void ml_parse_warn(ml_parser_t *Parser, const char *Error, const char *Format, ...);
const mlc_expr_t *ml_accept_file(ml_parser_t *Parser);

void ml_parser_escape(ml_parser_t *Parser, ml_value_t *(*Escape)(void *), void *Data);
void ml_parser_special(ml_parser_t *Parser, ml_value_t *(*Special)(void *), void *Data);

typedef ml_value_t *(*ml_parser_escape_t)(ml_parser_t *Parser);

void ml_parser_add_escape(ml_parser_t *Parser, const char *Prefix, ml_parser_escape_t Fn);

ml_value_t *ml_macro_subst(mlc_expr_t *Child, int Count, const char **Names, ml_value_t **Exprs);

ml_compiler_t *ml_compiler(ml_getter_t GlobalGet, void *Globals);
void ml_compiler_define(ml_compiler_t *Compiler, const char *Name, ml_value_t *Value);
ml_value_t *ml_compiler_lookup(ml_compiler_t *Compiler, const char *Name, const char *Source, int Line, int Eval);

void ml_function_compile(ml_state_t *Caller, const mlc_expr_t *Expr, ml_compiler_t *Compiler, const char **Parameters);
void ml_command_evaluate(ml_state_t *Caller, ml_parser_t *Parser, ml_compiler_t *Compiler);
void ml_load_file(ml_state_t *Caller, ml_getter_t GlobalGet, void *Globals, const char *FileName, const char *Parameters[]);

ml_value_t *ml_stringmap_globals(stringmap_t *Globals);
ml_value_t *ml_stringmap_global_get(const stringmap_t *Map, const char *Key, const char *Source, int Line, int Eval);

//typedef ml_value_t *(*string_fn_t)(const char *String, int Length);

//void ml_string_fn_register(const char *Prefix, string_fn_t Fn);

void ml_compiler_init();

ml_value_t *ml_global(const char *Name);
ml_value_t *ml_global_get(ml_value_t *Global);
ml_value_t *ml_global_set(ml_value_t *Global, ml_value_t *Value);

ml_value_t *ml_macro(ml_value_t *Function);
ml_value_t *ml_inline_call_macro(ml_value_t *Value);

ml_value_t *ml_inline_function(ml_value_t *Value);

#ifdef __cplusplus
}
#endif

/// @}

#endif

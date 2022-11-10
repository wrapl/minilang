#include "ml_compiler2.h"
#include "ml_object.h"
#include <string.h>

ml_value_t *mlc_expr_describe(mlc_expr_t *Expr);

ML_ENUM2(ParamKindT, "param-kind",
	"Default", ML_PARAM_DEFAULT,
	"Extra", ML_PARAM_EXTRA,
	"Named", ML_PARAM_NAMED,
	"ByRef", ML_PARAM_BYREF,
	"AsVar", ML_PARAM_ASVAR
);

extern const char *MLTokens[];

#include "ml_ast_types.c"

ML_METHOD("ast", MLExprT) {
//<Expr
//>ast::expr
// Returns a tuple describing the expression :mini:`Expr`.
	return mlc_expr_describe(((ml_expr_value_t *)Args[0])->Expr);
}

void ml_ast_init(stringmap_t *Globals) {
#include "ml_ast_init.c"
	ml_ast_types_init();
	if (Globals) {
		stringmap_insert(Globals, "ast", Ast);
	}
}

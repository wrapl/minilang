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

ML_TYPE(AstNamesT, (MLListT), "ast::names");

static ml_value_t *ml_ast_names(ml_value_t *Value) {
	if (ml_typeof(Value) == MLNamesT) {
		ml_value_t *List = ml_list();
		ML_NAMES_FOREACH(Value, Iter) ml_list_put(List, Iter->Value);
		List->Type = AstNamesT;
		return List;
	} else {
		return Value;
	}
}

#include "ml_ast_types.c"

ML_METHOD("ast", MLExprT) {
//<Expr
//>ast::expr
// Returns a tuple describing the expression :mini:`Expr`.
	return mlc_expr_describe((mlc_expr_t *)Args[0]);
}

void ml_ast_init(stringmap_t *Globals) {
#include "ml_ast_init.c"
	ml_ast_types_init();
	stringmap_insert(Ast->Exports, "names", AstNamesT);
	if (Globals) {
		stringmap_insert(Globals, "ast", Ast);
	}
}

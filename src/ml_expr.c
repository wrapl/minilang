#include "ml_compiler2.h"
#include "ml_object.h"
#include <string.h>

ml_expr_type_t mlc_expr_type(mlc_expr_t *Expr);

static ml_value_t *ExprNames[ML_EXPR_WITH + 1];

ml_value_t *mlc_expr_describe(mlc_expr_t *Expr);

static ml_value_t *mlc_exprs_describe(mlc_expr_t *Expr) {
	ml_value_t *Exprs = ml_list();
	while (Expr) {
		ml_list_put(Exprs, mlc_expr_describe(Expr));
		Expr = Expr->Next;
	}
	return Exprs;
}

static ml_value_t *mlc_locals_describe(mlc_local_t *Local, int Count) {
	ml_value_t *Locals = ml_map();
	while ((--Count >= 0) && Local) {
		ml_map_insert(Locals,
			ml_string(Local->Ident, -1),
			ml_tuplev(2, ml_integer(Local->Line), ml_integer(Local->Index))
		);
		Local = Local->Next;
	}
	return Locals;
}

ML_FLAGS2(DeclFlagsT, "decl-flags",
	"Constant", MLC_DECL_CONSTANT,
	"Forward", MLC_DECL_FORWARD,
	"Backfill", MLC_DECL_BACKFILL,
	"ByRef", MLC_DECL_BYREF,
	"AsVar", MLC_DECL_ASVAR
);

ML_ENUM2(ParamKindT, "param-kind",
	"Default", ML_PARAM_DEFAULT,
	"Extra", ML_PARAM_EXTRA,
	"Named", ML_PARAM_NAMED,
	"ByRef", ML_PARAM_BYREF,
	"AsVar", ML_PARAM_ASVAR
);

extern const char *MLTokens[];

ml_value_t *mlc_expr_describe(mlc_expr_t *Expr) {
	if (!Expr) return MLNil;
	ml_expr_type_t Type = mlc_expr_type(Expr);
	ml_value_t *ExprName = ExprNames[Type];
	ml_value_t *Source = ml_string(Expr->Source, -1);
	ml_value_t *Start = ml_integer(Expr->StartLine);
	ml_value_t *End = ml_integer(Expr->EndLine);
	switch (Type) {
	case ML_EXPR_UNKNOWN:
	case ML_EXPR_REGISTER:
	case ML_EXPR_BLANK:
	case ML_EXPR_NEXT:
	case ML_EXPR_NIL:
	case ML_EXPR_OLD:
	case ML_EXPR_IT: {
		return ml_tuplev(4, ExprName, Source, Start, End);
	}
	case ML_EXPR_VALUE: {
		mlc_value_expr_t *ValueExpr = (mlc_value_expr_t *)Expr;
		return ml_tuplev(5, ExprName, Source, Start, End, ValueExpr->Value);
	}
	case ML_EXPR_IDENT:
	case ML_EXPR_DEFINE: {
		mlc_ident_expr_t *IdentExpr = (mlc_ident_expr_t *)Expr;
		return ml_tuplev(5, ExprName, Source, Start, End, ml_string(IdentExpr->Ident, -1));
	}
	case ML_EXPR_AND:
	case ML_EXPR_ASSIGN:
	case ML_EXPR_CALL:
	case ML_EXPR_DEBUG:
	case ML_EXPR_DELEGATE:
	case ML_EXPR_EACH:
	case ML_EXPR_EXIT:
	case ML_EXPR_INLINE:
	case ML_EXPR_LIST:
	case ML_EXPR_LOOP:
	case ML_EXPR_MAP:
	case ML_EXPR_NOT:
	case ML_EXPR_OR:
	case ML_EXPR_RETURN:
	case ML_EXPR_SUSPEND:
	case ML_EXPR_SWITCH:
	case ML_EXPR_TUPLE:
	case ML_EXPR_GUARD: {
		mlc_parent_expr_t *ParentExpr = (mlc_parent_expr_t *)Expr;
		return ml_tuplev(5, ExprName, Source, Start, End,
			mlc_exprs_describe(ParentExpr->Child)
		);
	}
	case ML_EXPR_CONST_CALL:
	case ML_EXPR_RESOLVE: {
		mlc_parent_value_expr_t *ParentExpr = (mlc_parent_value_expr_t *)Expr;
		return ml_tuplev(6, ExprName, Source, Start, End,
			ParentExpr->Value,
			mlc_exprs_describe(ParentExpr->Child)
		);
	}
	case ML_EXPR_DEF:
	case ML_EXPR_DEF_IN:
	case ML_EXPR_DEF_UNPACK:
	case ML_EXPR_LET:
	case ML_EXPR_LET_IN:
	case ML_EXPR_LET_UNPACK:
	case ML_EXPR_REF:
	case ML_EXPR_REF_IN:
	case ML_EXPR_REF_UNPACK:
	case ML_EXPR_VAR:
	case ML_EXPR_VAR_TYPE:
	case ML_EXPR_VAR_IN:
	case ML_EXPR_VAR_UNPACK: {
		mlc_local_expr_t *LocalExpr = (mlc_local_expr_t *)Expr;
		return ml_tuplev(6, ExprName, Source, Start, End,
			mlc_locals_describe(LocalExpr->Local, LocalExpr->Count ?: 1),
			mlc_exprs_describe(LocalExpr->Child)
		);
		break;
	}
	case ML_EXPR_WITH: {
		mlc_local_expr_t *LocalExpr = (mlc_local_expr_t *)Expr;
		return ml_tuplev(6, ExprName, Source, Start, End,
			mlc_locals_describe(LocalExpr->Local, INT_MAX),
			mlc_exprs_describe(LocalExpr->Child)
		);
		break;
	}
	case ML_EXPR_IF: {
		mlc_if_expr_t *IfExpr = (mlc_if_expr_t *)Expr;
		ml_value_t *Cases = ml_list();
		for (mlc_if_case_t *Case = IfExpr->Cases; Case; Case = Case->Next) {
			ml_value_t *DeclType = MLNil;
			if (Case->Token == MLT_VAR) {
				DeclType = ml_cstring("var");
			} else if (Case->Token == MLT_LET) {
				DeclType = ml_cstring("let");
			}
			ml_list_put(Cases, ml_tuplev(4,
				DeclType,
				mlc_locals_describe(Case->Local, Case->Local->Index),
				mlc_expr_describe(Case->Condition),
				mlc_expr_describe(Case->Body)
			));
		}
		ml_value_t *Else = IfExpr->Else ? mlc_expr_describe(IfExpr->Else) : MLNil;
		return ml_tuplev(6, ExprName, Source, Start, End, Cases, Else);
		break;
	}
	case ML_EXPR_FOR: {
		mlc_for_expr_t *ForExpr = (mlc_for_expr_t *)Expr;
		return ml_tuplev(8, ExprName, Source, Start, End,
			ForExpr->Key ? ml_string(ForExpr->Key, -1) : MLNil,
			mlc_locals_describe(ForExpr->Local, INT_MAX),
			mlc_exprs_describe(ForExpr->Sequence),
			mlc_exprs_describe(ForExpr->Body)
		);
	}
	case ML_EXPR_FUN: {
		mlc_fun_expr_t *FunExpr = (mlc_fun_expr_t *)Expr;
		ml_value_t *Params = ml_map();
		for (mlc_param_t *Param = FunExpr->Params; Param; Param = Param->Next) {
			ml_value_t *Type = Param->Type ? mlc_expr_describe(Param->Type) : MLNil;
			ml_map_insert(Params, ml_string(Param->Ident, -1), ml_tuplev(3, ml_enum_value(ParamKindT, Param->Kind), ml_integer(Param->Line), Type));
		}
		return ml_tuplev(7, ExprName, Source, Start, End,
			ml_string(FunExpr->Name ?: "", -1),
			Params,
			mlc_expr_describe(FunExpr->Body)
		);
	}
	case ML_EXPR_BLOCK: {
		mlc_block_expr_t *BlockExpr = (mlc_block_expr_t *)Expr;
		return ml_tuplev(10, ExprName, Source, Start, End,
			mlc_locals_describe(BlockExpr->Vars, INT_MAX),
			mlc_locals_describe(BlockExpr->Lets, INT_MAX),
			mlc_locals_describe(BlockExpr->Defs, INT_MAX),
			mlc_exprs_describe(BlockExpr->Child),
			BlockExpr->CatchIdent ? ml_string(BlockExpr->CatchIdent, -1) : MLNil,
			mlc_expr_describe(BlockExpr->CatchBody)
		);
	}
	case ML_EXPR_STRING: {
		mlc_string_expr_t *StringExpr = (mlc_string_expr_t *)Expr;
		ml_value_t *Parts = ml_list();
		for (mlc_string_part_t *Part = StringExpr->Parts; Part; Part = Part->Next) {
			if (Part->Length) {
				ml_list_put(Parts, ml_string(Part->Chars, Part->Length));
			} else {
				ml_list_put(Parts, mlc_exprs_describe(Part->Child));
			}
		}
		return ml_tuplev(5, ExprName, Source, Start, End, Parts);
	}
	case ML_EXPR_SCOPED: {
		return ml_tuplev(4, ExprName, Source, Start, End);
	}
	case ML_EXPR_SUBST: {
		return ml_tuplev(4, ExprName, Source, Start, End);
	}
	case ML_EXPR_DEFAULT: {
		return ml_tuplev(4, ExprName, Source, Start, End);
	}
	}
	__builtin_unreachable();
}

ML_METHOD("describe", MLExprT) {
//<Expr
//>tuple
// Returns a tuple describing the expression :mini:`Expr`.
	return mlc_expr_describe(((ml_expr_value_t *)Args[0])->Expr);
}

void ml_expr_init() {
#include "ml_expr_names.c"
#include "ml_expr_init.c"
}

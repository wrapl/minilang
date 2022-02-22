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

static ml_value_t *mlc_locals_describe(mlc_local_t *Local) {
	ml_value_t *Locals = ml_map();
	while (Local) {
		ml_map_insert(Locals, ml_string(Local->Ident, -1), ml_tuplev(2, ml_integer(Local->Line), ml_integer(Local->Index)));
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
	case ML_EXPR_OLD: {
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
	case ML_EXPR_TUPLE: {
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
	case ML_EXPR_VAR:
	case ML_EXPR_VAR_TYPE:
	case ML_EXPR_VAR_IN:
	case ML_EXPR_VAR_UNPACK:
	case ML_EXPR_WITH: {
		mlc_local_expr_t *LocalExpr = (mlc_local_expr_t *)Expr;
		return ml_tuplev(7, ExprName, Source, Start, End, mlc_locals_describe(LocalExpr->Local), mlc_exprs_describe(LocalExpr->Child), ml_flags_value(DeclFlagsT, LocalExpr->Flags));
		break;
	}
	case ML_EXPR_IF: {
		mlc_if_expr_t *IfExpr = (mlc_if_expr_t *)Expr;
		ml_value_t *Cases = ml_list();
		for (mlc_if_case_t *Case = IfExpr->Cases; Case; Case = Case->Next) {
			ml_value_t *CaseMap = ml_map();
			ml_map_insert(CaseMap, ml_cstring("condition"), mlc_expr_describe(Case->Condition));
			ml_map_insert(CaseMap, ml_cstring("body"), mlc_expr_describe(Case->Body));
			ml_value_t *DeclType = NULL;
			if (Case->Token == MLT_VAR) {
				DeclType = ml_cstring("var");
			} else if (Case->Token == MLT_LET) {
				DeclType = ml_cstring("let");
			}
			if (DeclType) {
				ml_map_insert(CaseMap, DeclType, mlc_locals_describe(Case->Local));
			}
			ml_list_put(Cases, CaseMap);
		}
		ml_value_t *Else = IfExpr->Else ? mlc_expr_describe(IfExpr->Else) : MLNil;
		return ml_tuplev(6, ExprName, Source, Start, End, Cases, Else);
		break;
	}
	case ML_EXPR_FOR: {
		mlc_for_expr_t *ForExpr = (mlc_for_expr_t *)Expr;
		return ml_tuplev(8, ExprName, Source, Start, End,
			ForExpr->Key ? ml_string(ForExpr->Key, -1) : MLNil,
			mlc_locals_describe(ForExpr->Local),
			mlc_expr_describe(ForExpr->Child),
			ForExpr->Child->Next ? mlc_expr_describe(ForExpr->Child->Next) : MLNil
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
		// TODO: Add catches
		return ml_tuplev(8, ExprName, Source, Start, End,
			mlc_locals_describe(BlockExpr->Vars),
			mlc_locals_describe(BlockExpr->Lets),
			mlc_locals_describe(BlockExpr->Defs),
			mlc_exprs_describe(BlockExpr->Child)
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
	default: __builtin_unreachable();
	}
}

extern ml_type_t MLExprT[];

ML_METHOD("describe", MLExprT) {
//<Expr
//>tuple
// Returns a tuple describing the expression :mini:`Expr`.
	return mlc_expr_describe(((ml_expr_value_t *)Args[0])->Expr);
}

void ml_expr_init() {
	ExprNames[ML_EXPR_UNKNOWN] = ml_cstring("unknown");
	ExprNames[ML_EXPR_AND] = ml_cstring("and");
	ExprNames[ML_EXPR_ASSIGN] = ml_cstring("assign");
	ExprNames[ML_EXPR_BLANK] = ml_cstring("blank");
	ExprNames[ML_EXPR_BLOCK] = ml_cstring("block");
	ExprNames[ML_EXPR_CALL] = ml_cstring("call");
	ExprNames[ML_EXPR_CONST_CALL] = ml_cstring("const_call");
	ExprNames[ML_EXPR_DEBUG] = ml_cstring("debug");
	ExprNames[ML_EXPR_DEFAULT] = ml_cstring("default");
	ExprNames[ML_EXPR_DEFINE] = ml_cstring("define");
	ExprNames[ML_EXPR_DEF] = ml_cstring("def");
	ExprNames[ML_EXPR_DEF_IN] = ml_cstring("def_in");
	ExprNames[ML_EXPR_DEF_UNPACK] = ml_cstring("def_unpack");
	ExprNames[ML_EXPR_DELEGATE] = ml_cstring("delegate");
	ExprNames[ML_EXPR_EACH] = ml_cstring("each");
	ExprNames[ML_EXPR_EXIT] = ml_cstring("exit");
	ExprNames[ML_EXPR_FOR] = ml_cstring("for");
	ExprNames[ML_EXPR_FUN] = ml_cstring("fun");
	ExprNames[ML_EXPR_IDENT] = ml_cstring("ident");
	ExprNames[ML_EXPR_IF] = ml_cstring("if");
	ExprNames[ML_EXPR_INLINE] = ml_cstring("inline");
	ExprNames[ML_EXPR_LET] = ml_cstring("let");
	ExprNames[ML_EXPR_LET_IN] = ml_cstring("let_in");
	ExprNames[ML_EXPR_LET_UNPACK] = ml_cstring("let_unpack");
	ExprNames[ML_EXPR_LIST] = ml_cstring("list");
	ExprNames[ML_EXPR_LOOP] = ml_cstring("loop");
	ExprNames[ML_EXPR_MAP] = ml_cstring("map");
	ExprNames[ML_EXPR_NEXT] = ml_cstring("next");
	ExprNames[ML_EXPR_NIL] = ml_cstring("nil");
	ExprNames[ML_EXPR_NOT] = ml_cstring("not");
	ExprNames[ML_EXPR_OLD] = ml_cstring("old");
	ExprNames[ML_EXPR_OR] = ml_cstring("or");
	ExprNames[ML_EXPR_REGISTER] = ml_cstring("register");
	ExprNames[ML_EXPR_RESOLVE] = ml_cstring("resolve");
	ExprNames[ML_EXPR_RETURN] = ml_cstring("return");
	ExprNames[ML_EXPR_SCOPED] = ml_cstring("scoped");
	ExprNames[ML_EXPR_STRING] = ml_cstring("string");
	ExprNames[ML_EXPR_SUBST] = ml_cstring("subst");
	ExprNames[ML_EXPR_SUSPEND] = ml_cstring("suspend");
	ExprNames[ML_EXPR_SWITCH] = ml_cstring("switch");
	ExprNames[ML_EXPR_TUPLE] = ml_cstring("tuple");
	ExprNames[ML_EXPR_VALUE] = ml_cstring("value");
	ExprNames[ML_EXPR_VAR] = ml_cstring("var");
	ExprNames[ML_EXPR_VAR_IN] = ml_cstring("var_in");
	ExprNames[ML_EXPR_VAR_TYPE] = ml_cstring("var_type");
	ExprNames[ML_EXPR_VAR_UNPACK] = ml_cstring("var_unpack");
	ExprNames[ML_EXPR_WITH] = ml_cstring("with");
#include "ml_expr_init.c"
}

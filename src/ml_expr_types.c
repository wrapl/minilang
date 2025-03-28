ml_expr_type_t mlc_expr_type(mlc_expr_t *Expr) {
	if (Expr->compile == (void *)ml_and_expr_compile) return ML_EXPR_AND;
	if (Expr->compile == (void *)ml_args_expr_compile) return ML_EXPR_ARGS;
	if (Expr->compile == (void *)ml_assign_expr_compile) return ML_EXPR_ASSIGN;
	if (Expr->compile == (void *)ml_blank_expr_compile) return ML_EXPR_BLANK;
	if (Expr->compile == (void *)ml_block_expr_compile) return ML_EXPR_BLOCK;
	if (Expr->compile == (void *)ml_call_expr_compile) return ML_EXPR_CALL;
	if (Expr->compile == (void *)ml_const_call_expr_compile) return ML_EXPR_CONST_CALL;
	if (Expr->compile == (void *)ml_debug_expr_compile) return ML_EXPR_DEBUG;
	if (Expr->compile == (void *)ml_def_expr_compile) return ML_EXPR_DEF;
	if (Expr->compile == (void *)ml_def_in_expr_compile) return ML_EXPR_DEF_IN;
	if (Expr->compile == (void *)ml_def_unpack_expr_compile) return ML_EXPR_DEF_UNPACK;
	if (Expr->compile == (void *)ml_default_expr_compile) return ML_EXPR_DEFAULT;
	if (Expr->compile == (void *)ml_define_expr_compile) return ML_EXPR_DEFINE;
	if (Expr->compile == (void *)ml_delegate_expr_compile) return ML_EXPR_DELEGATE;
	if (Expr->compile == (void *)ml_each_expr_compile) return ML_EXPR_EACH;
	if (Expr->compile == (void *)ml_exit_expr_compile) return ML_EXPR_EXIT;
	if (Expr->compile == (void *)ml_for_expr_compile) return ML_EXPR_FOR;
	if (Expr->compile == (void *)ml_fun_expr_compile) return ML_EXPR_FUN;
	if (Expr->compile == (void *)ml_guard_expr_compile) return ML_EXPR_GUARD;
	if (Expr->compile == (void *)ml_ident_expr_compile) return ML_EXPR_IDENT;
	if (Expr->compile == (void *)ml_if_config_expr_compile) return ML_EXPR_IF_CONFIG;
	if (Expr->compile == (void *)ml_if_expr_compile) return ML_EXPR_IF;
	if (Expr->compile == (void *)ml_inline_expr_compile) return ML_EXPR_INLINE;
	if (Expr->compile == (void *)ml_it_expr_compile) return ML_EXPR_IT;
	if (Expr->compile == (void *)ml_let_expr_compile) return ML_EXPR_LET;
	if (Expr->compile == (void *)ml_let_in_expr_compile) return ML_EXPR_LET_IN;
	if (Expr->compile == (void *)ml_let_unpack_expr_compile) return ML_EXPR_LET_UNPACK;
	if (Expr->compile == (void *)ml_list_expr_compile) return ML_EXPR_LIST;
	if (Expr->compile == (void *)ml_loop_expr_compile) return ML_EXPR_LOOP;
	if (Expr->compile == (void *)ml_map_expr_compile) return ML_EXPR_MAP;
	if (Expr->compile == (void *)ml_next_expr_compile) return ML_EXPR_NEXT;
	if (Expr->compile == (void *)ml_nil_expr_compile) return ML_EXPR_NIL;
	if (Expr->compile == (void *)ml_not_expr_compile) return ML_EXPR_NOT;
	if (Expr->compile == (void *)ml_old_expr_compile) return ML_EXPR_OLD;
	if (Expr->compile == (void *)ml_or_expr_compile) return ML_EXPR_OR;
	if (Expr->compile == (void *)ml_recur_expr_compile) return ML_EXPR_RECUR;
	if (Expr->compile == (void *)ml_ref_expr_compile) return ML_EXPR_REF;
	if (Expr->compile == (void *)ml_ref_in_expr_compile) return ML_EXPR_REF_IN;
	if (Expr->compile == (void *)ml_ref_unpack_expr_compile) return ML_EXPR_REF_UNPACK;
	if (Expr->compile == (void *)ml_register_expr_compile) return ML_EXPR_REGISTER;
	if (Expr->compile == (void *)ml_resolve_expr_compile) return ML_EXPR_RESOLVE;
	if (Expr->compile == (void *)ml_return_expr_compile) return ML_EXPR_RETURN;
	if (Expr->compile == (void *)ml_scoped_expr_compile) return ML_EXPR_SCOPED;
	if (Expr->compile == (void *)ml_string_expr_compile) return ML_EXPR_STRING;
	if (Expr->compile == (void *)ml_subst_expr_compile) return ML_EXPR_SUBST;
	if (Expr->compile == (void *)ml_suspend_expr_compile) return ML_EXPR_SUSPEND;
	if (Expr->compile == (void *)ml_switch_expr_compile) return ML_EXPR_SWITCH;
	if (Expr->compile == (void *)ml_tuple_expr_compile) return ML_EXPR_TUPLE;
	if (Expr->compile == (void *)ml_unknown_expr_compile) return ML_EXPR_UNKNOWN;
	if (Expr->compile == (void *)ml_value_expr_compile) return ML_EXPR_VALUE;
	if (Expr->compile == (void *)ml_var_expr_compile) return ML_EXPR_VAR;
	if (Expr->compile == (void *)ml_var_in_expr_compile) return ML_EXPR_VAR_IN;
	if (Expr->compile == (void *)ml_var_type_expr_compile) return ML_EXPR_VAR_TYPE;
	if (Expr->compile == (void *)ml_var_unpack_expr_compile) return ML_EXPR_VAR_UNPACK;
	if (Expr->compile == (void *)ml_with_expr_compile) return ML_EXPR_WITH;
	return ML_EXPR_UNKNOWN;
}

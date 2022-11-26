#include "ml_object.h"

//!ast

static ml_module_t Ast[1] = {{MLModuleT, "ast", {STRINGMAP_INIT}}};

ML_CLASS(AstExprT, (), "ast::expr");
//@ast::expr
// An expression
//
// * :mini:`:source(Value: ast::expr): string`
// * :mini:`:startline(Value: ast::expr): integer`
// * :mini:`:endline(Value: ast::expr): integer`

ML_FIELD("source", AstExprT);

ML_FIELD("startline", AstExprT);

ML_FIELD("endline", AstExprT);

ML_CLASS(AstIfExprT, (AstExprT), "ast::expr::if");
//@ast::expr::if
// An :mini:`if` expression
//
// * :mini:`:cases(Value: ast::expr::if): list[ast::ifcase]`
// * :mini:`:else(Value: ast::expr::if): list[ast::expr]`

ML_FIELD("cases", AstIfExprT);

ML_FIELD("else", AstIfExprT);

ML_CLASS(AstFunExprT, (AstExprT), "ast::expr::fun");
//@ast::expr::fun
// A :mini:`fun` expression
//
// * :mini:`:name(Value: ast::expr::fun): string`
// * :mini:`:params(Value: ast::expr::fun): list[ast::param]`
// * :mini:`:body(Value: ast::expr::fun): list[ast::expr]`
// * :mini:`:returntype(Value: ast::expr::fun): list[ast::expr]`

ML_FIELD("name", AstFunExprT);

ML_FIELD("params", AstFunExprT);

ML_FIELD("body", AstFunExprT);

ML_FIELD("returntype", AstFunExprT);

ML_CLASS(AstForExprT, (AstExprT), "ast::expr::for");
//@ast::expr::for
// A :mini:`for` expression
//
// * :mini:`:key(Value: ast::expr::for): string`
// * :mini:`:local(Value: ast::expr::for): list[ast::local]`
// * :mini:`:sequence(Value: ast::expr::for): list[ast::expr]`
// * :mini:`:body(Value: ast::expr::for): list[ast::expr]`
// * :mini:`:name(Value: ast::expr::for): string`
// * :mini:`:unpack(Value: ast::expr::for): integer`

ML_FIELD("key", AstForExprT);

ML_FIELD("local", AstForExprT);

ML_FIELD("sequence", AstForExprT);

ML_FIELD("body", AstForExprT);

ML_FIELD("name", AstForExprT);

ML_FIELD("unpack", AstForExprT);

ML_CLASS(AstValueExprT, (AstExprT), "ast::expr::value");
//@ast::expr::value
// A :mini:`value` expression
//
// * :mini:`:value(Value: ast::expr::value): any`

ML_FIELD("value", AstValueExprT);

ML_CLASS(AstSubstExprT, (AstExprT), "ast::expr::subst");
//@ast::expr::subst
// A :mini:`subst` expression
//

ML_CLASS(AstIdentExprT, (AstExprT), "ast::expr::ident");
//@ast::expr::ident
// An :mini:`ident` expression
//
// * :mini:`:ident(Value: ast::expr::ident): string`

ML_FIELD("ident", AstIdentExprT);

ML_CLASS(AstLocalExprT, (AstExprT), "ast::expr::local");
//@ast::expr::local
// A :mini:`local` expression
//
// * :mini:`:local(Value: ast::expr::local): list[ast::local]`
// * :mini:`:child(Value: ast::expr::local): list[ast::expr]`
// * :mini:`:count(Value: ast::expr::local): integer`

ML_FIELD("local", AstLocalExprT);

ML_FIELD("child", AstLocalExprT);

ML_FIELD("count", AstLocalExprT);

ML_CLASS(AstBlockExprT, (AstExprT), "ast::expr::block");
//@ast::expr::block
// A :mini:`block` expression
//
// * :mini:`:vars(Value: ast::expr::block): list[ast::local]`
// * :mini:`:lets(Value: ast::expr::block): list[ast::local]`
// * :mini:`:defs(Value: ast::expr::block): list[ast::local]`
// * :mini:`:child(Value: ast::expr::block): list[ast::expr]`
// * :mini:`:catchbody(Value: ast::expr::block): list[ast::expr]`
// * :mini:`:must(Value: ast::expr::block): list[ast::expr]`
// * :mini:`:catchident(Value: ast::expr::block): string`
// * :mini:`:numvars(Value: ast::expr::block): integer`
// * :mini:`:numlets(Value: ast::expr::block): integer`
// * :mini:`:numdefs(Value: ast::expr::block): integer`

ML_FIELD("vars", AstBlockExprT);

ML_FIELD("lets", AstBlockExprT);

ML_FIELD("defs", AstBlockExprT);

ML_FIELD("child", AstBlockExprT);

ML_FIELD("catchbody", AstBlockExprT);

ML_FIELD("must", AstBlockExprT);

ML_FIELD("catchident", AstBlockExprT);

ML_FIELD("numvars", AstBlockExprT);

ML_FIELD("numlets", AstBlockExprT);

ML_FIELD("numdefs", AstBlockExprT);

ML_CLASS(AstStringExprT, (AstExprT), "ast::expr::string");
//@ast::expr::string
// A :mini:`string` expression
//
// * :mini:`:parts(Value: ast::expr::string): list[ast::stringpart]`

ML_FIELD("parts", AstStringExprT);

ML_CLASS(AstScopedExprT, (AstExprT), "ast::expr::scoped");
//@ast::expr::scoped
// A :mini:`scoped` expression
//

ML_CLASS(AstParentExprT, (AstExprT), "ast::expr::parent");
//@ast::expr::parent
// A :mini:`parent` expression
//
// * :mini:`:child(Value: ast::expr::parent): list[ast::expr]`
// * :mini:`:name(Value: ast::expr::parent): string`

ML_FIELD("child", AstParentExprT);

ML_FIELD("name", AstParentExprT);

ML_CLASS(AstDefaultExprT, (AstExprT), "ast::expr::default");
//@ast::expr::default
// A :mini:`default` expression
//
// * :mini:`:child(Value: ast::expr::default): list[ast::expr]`
// * :mini:`:index(Value: ast::expr::default): integer`
// * :mini:`:flags(Value: ast::expr::default): integer`

ML_FIELD("child", AstDefaultExprT);

ML_FIELD("index", AstDefaultExprT);

ML_FIELD("flags", AstDefaultExprT);

ML_CLASS(AstParentValueExprT, (AstExprT), "ast::expr::parentvalue");
//@ast::expr::parentvalue
// A :mini:`parent` :mini:`value` expression
//
// * :mini:`:child(Value: ast::expr::parentvalue): list[ast::expr]`
// * :mini:`:value(Value: ast::expr::parentvalue): any`

ML_FIELD("child", AstParentValueExprT);

ML_FIELD("value", AstParentValueExprT);

ML_CLASS(AstAndExprT, (AstParentExprT), "ast::expr::and");
//@ast::expr::and
// An :mini:`and` expression
//

ML_CLASS(AstAssignExprT, (AstParentExprT), "ast::expr::assign");
//@ast::expr::assign
// An :mini:`assign` expression
//

ML_CLASS(AstBlankExprT, (AstExprT), "ast::expr::blank");
//@ast::expr::blank
// A :mini:`blank` expression
//

ML_CLASS(AstCallExprT, (AstParentExprT), "ast::expr::call");
//@ast::expr::call
// A :mini:`call` expression
//

ML_CLASS(AstConstCallExprT, (AstParentValueExprT), "ast::expr::constcall");
//@ast::expr::constcall
// A :mini:`const` :mini:`call` expression
//

ML_CLASS(AstDebugExprT, (AstParentExprT), "ast::expr::debug");
//@ast::expr::debug
// A :mini:`debug` expression
//

ML_CLASS(AstDefExprT, (AstLocalExprT), "ast::expr::def");
//@ast::expr::def
// A :mini:`def` expression
//

ML_CLASS(AstDefInExprT, (AstLocalExprT), "ast::expr::defin");
//@ast::expr::defin
// A :mini:`def` :mini:`in` expression
//

ML_CLASS(AstDefUnpackExprT, (AstLocalExprT), "ast::expr::defunpack");
//@ast::expr::defunpack
// A :mini:`def` :mini:`unpack` expression
//

ML_CLASS(AstDefineExprT, (AstIdentExprT), "ast::expr::define");
//@ast::expr::define
// A :mini:`define` expression
//

ML_CLASS(AstDelegateExprT, (AstParentExprT), "ast::expr::delegate");
//@ast::expr::delegate
// A :mini:`delegate` expression
//

ML_CLASS(AstEachExprT, (AstParentExprT), "ast::expr::each");
//@ast::expr::each
// An :mini:`each` expression
//

ML_CLASS(AstExitExprT, (AstParentExprT), "ast::expr::exit");
//@ast::expr::exit
// An :mini:`exit` expression
//

ML_CLASS(AstGuardExprT, (AstParentExprT), "ast::expr::guard");
//@ast::expr::guard
// A :mini:`guard` expression
//

ML_CLASS(AstInlineExprT, (AstParentExprT), "ast::expr::inline");
//@ast::expr::inline
// An :mini:`inline` expression
//

ML_CLASS(AstItExprT, (AstExprT), "ast::expr::it");
//@ast::expr::it
// An :mini:`it` expression
//

ML_CLASS(AstLetExprT, (AstLocalExprT), "ast::expr::let");
//@ast::expr::let
// A :mini:`let` expression
//

ML_CLASS(AstLetInExprT, (AstLocalExprT), "ast::expr::letin");
//@ast::expr::letin
// A :mini:`let` :mini:`in` expression
//

ML_CLASS(AstLetUnpackExprT, (AstLocalExprT), "ast::expr::letunpack");
//@ast::expr::letunpack
// A :mini:`let` :mini:`unpack` expression
//

ML_CLASS(AstListExprT, (AstParentExprT), "ast::expr::list");
//@ast::expr::list
// A :mini:`list` expression
//

ML_CLASS(AstLoopExprT, (AstParentExprT), "ast::expr::loop");
//@ast::expr::loop
// A :mini:`loop` expression
//

ML_CLASS(AstMapExprT, (AstParentExprT), "ast::expr::map");
//@ast::expr::map
// A :mini:`map` expression
//

ML_CLASS(AstNextExprT, (AstParentExprT), "ast::expr::next");
//@ast::expr::next
// A :mini:`next` expression
//

ML_CLASS(AstNilExprT, (AstExprT), "ast::expr::nil");
//@ast::expr::nil
// A :mini:`nil` expression
//

ML_CLASS(AstNotExprT, (AstParentExprT), "ast::expr::not");
//@ast::expr::not
// A :mini:`not` expression
//

ML_CLASS(AstOldExprT, (AstExprT), "ast::expr::old");
//@ast::expr::old
// An :mini:`old` expression
//

ML_CLASS(AstOrExprT, (AstParentExprT), "ast::expr::or");
//@ast::expr::or
// An :mini:`or` expression
//

ML_CLASS(AstRefExprT, (AstLocalExprT), "ast::expr::ref");
//@ast::expr::ref
// A :mini:`ref` expression
//

ML_CLASS(AstRefInExprT, (AstLocalExprT), "ast::expr::refin");
//@ast::expr::refin
// A :mini:`ref` :mini:`in` expression
//

ML_CLASS(AstRefUnpackExprT, (AstLocalExprT), "ast::expr::refunpack");
//@ast::expr::refunpack
// A :mini:`ref` :mini:`unpack` expression
//

ML_CLASS(AstRegisterExprT, (AstExprT), "ast::expr::register");
//@ast::expr::register
// A :mini:`register` expression
//

ML_CLASS(AstResolveExprT, (AstParentValueExprT), "ast::expr::resolve");
//@ast::expr::resolve
// A :mini:`resolve` expression
//

ML_CLASS(AstReturnExprT, (AstParentExprT), "ast::expr::return");
//@ast::expr::return
// A :mini:`return` expression
//

ML_CLASS(AstSuspendExprT, (AstParentExprT), "ast::expr::suspend");
//@ast::expr::suspend
// A :mini:`suspend` expression
//

ML_CLASS(AstSwitchExprT, (AstParentExprT), "ast::expr::switch");
//@ast::expr::switch
// A :mini:`switch` expression
//

ML_CLASS(AstTupleExprT, (AstParentExprT), "ast::expr::tuple");
//@ast::expr::tuple
// A :mini:`tuple` expression
//

ML_CLASS(AstUnknownExprT, (AstExprT), "ast::expr::unknown");
//@ast::expr::unknown
// An :mini:`unknown` expression
//

ML_CLASS(AstVarExprT, (AstLocalExprT), "ast::expr::var");
//@ast::expr::var
// A :mini:`var` expression
//

ML_CLASS(AstVarInExprT, (AstLocalExprT), "ast::expr::varin");
//@ast::expr::varin
// A :mini:`var` :mini:`in` expression
//

ML_CLASS(AstVarTypeExprT, (AstLocalExprT), "ast::expr::vartype");
//@ast::expr::vartype
// A :mini:`var` :mini:`type` expression
//

ML_CLASS(AstVarUnpackExprT, (AstLocalExprT), "ast::expr::varunpack");
//@ast::expr::varunpack
// A :mini:`var` :mini:`unpack` expression
//

ML_CLASS(AstWithExprT, (AstLocalExprT), "ast::expr::with");
//@ast::expr::with
// A :mini:`with` expression
//

ML_CLASS(AstLocalT, (), "ast::local");
//@ast::local
// A local
//
// * :mini:`:ident(Value: ast::local): string`
// * :mini:`:line(Value: ast::local): integer`
// * :mini:`:index(Value: ast::local): integer`

ML_FIELD("ident", AstLocalT);

ML_FIELD("line", AstLocalT);

ML_FIELD("index", AstLocalT);

ML_CLASS(AstParamT, (), "ast::param");
//@ast::param
// A param
//
// * :mini:`:ident(Value: ast::param): string`
// * :mini:`:type(Value: ast::param): list[ast::expr]`
// * :mini:`:line(Value: ast::param): integer`
// * :mini:`:kind(Value: ast::param): ast::paramkind`

ML_FIELD("ident", AstParamT);

ML_FIELD("type", AstParamT);

ML_FIELD("line", AstParamT);

ML_FIELD("kind", AstParamT);

ML_CLASS(AstStringPartT, (), "ast::stringpart");
//@ast::stringpart
// A string part
//
// * :mini:`:child(Value: ast::stringpart): list[ast::expr]`
// * :mini:`:chars(Value: ast::stringpart): string`
// * :mini:`:length(Value: ast::stringpart): integer`
// * :mini:`:line(Value: ast::stringpart): integer`

ML_FIELD("child", AstStringPartT);

ML_FIELD("chars", AstStringPartT);

ML_FIELD("length", AstStringPartT);

ML_FIELD("line", AstStringPartT);

ML_CLASS(AstIfCaseT, (), "ast::ifcase");
//@ast::ifcase
// An if case
//
// * :mini:`:condition(Value: ast::ifcase): list[ast::expr]`
// * :mini:`:body(Value: ast::ifcase): list[ast::expr]`
// * :mini:`:local(Value: ast::ifcase): list[ast::local]`
// * :mini:`:token(Value: ast::ifcase): integer`

ML_FIELD("condition", AstIfCaseT);

ML_FIELD("body", AstIfCaseT);

ML_FIELD("local", AstIfCaseT);

ML_FIELD("token", AstIfCaseT);


static void ml_ast_types_init() {
	stringmap_insert(Ast->Exports, "expr", AstExprT);
	stringmap_insert(AstExprT->Exports, "if", AstIfExprT);
	stringmap_insert(AstExprT->Exports, "fun", AstFunExprT);
	stringmap_insert(AstExprT->Exports, "for", AstForExprT);
	stringmap_insert(AstExprT->Exports, "value", AstValueExprT);
	stringmap_insert(AstExprT->Exports, "subst", AstSubstExprT);
	stringmap_insert(AstExprT->Exports, "ident", AstIdentExprT);
	stringmap_insert(AstExprT->Exports, "local", AstLocalExprT);
	stringmap_insert(AstExprT->Exports, "block", AstBlockExprT);
	stringmap_insert(AstExprT->Exports, "string", AstStringExprT);
	stringmap_insert(AstExprT->Exports, "scoped", AstScopedExprT);
	stringmap_insert(AstExprT->Exports, "parent", AstParentExprT);
	stringmap_insert(AstExprT->Exports, "default", AstDefaultExprT);
	stringmap_insert(AstExprT->Exports, "parentvalue", AstParentValueExprT);
	stringmap_insert(AstExprT->Exports, "and", AstAndExprT);
	stringmap_insert(AstExprT->Exports, "assign", AstAssignExprT);
	stringmap_insert(AstExprT->Exports, "blank", AstBlankExprT);
	stringmap_insert(AstExprT->Exports, "call", AstCallExprT);
	stringmap_insert(AstExprT->Exports, "constcall", AstConstCallExprT);
	stringmap_insert(AstExprT->Exports, "debug", AstDebugExprT);
	stringmap_insert(AstExprT->Exports, "def", AstDefExprT);
	stringmap_insert(AstExprT->Exports, "defin", AstDefInExprT);
	stringmap_insert(AstExprT->Exports, "defunpack", AstDefUnpackExprT);
	stringmap_insert(AstExprT->Exports, "define", AstDefineExprT);
	stringmap_insert(AstExprT->Exports, "delegate", AstDelegateExprT);
	stringmap_insert(AstExprT->Exports, "each", AstEachExprT);
	stringmap_insert(AstExprT->Exports, "exit", AstExitExprT);
	stringmap_insert(AstExprT->Exports, "guard", AstGuardExprT);
	stringmap_insert(AstExprT->Exports, "inline", AstInlineExprT);
	stringmap_insert(AstExprT->Exports, "it", AstItExprT);
	stringmap_insert(AstExprT->Exports, "let", AstLetExprT);
	stringmap_insert(AstExprT->Exports, "letin", AstLetInExprT);
	stringmap_insert(AstExprT->Exports, "letunpack", AstLetUnpackExprT);
	stringmap_insert(AstExprT->Exports, "list", AstListExprT);
	stringmap_insert(AstExprT->Exports, "loop", AstLoopExprT);
	stringmap_insert(AstExprT->Exports, "map", AstMapExprT);
	stringmap_insert(AstExprT->Exports, "next", AstNextExprT);
	stringmap_insert(AstExprT->Exports, "nil", AstNilExprT);
	stringmap_insert(AstExprT->Exports, "not", AstNotExprT);
	stringmap_insert(AstExprT->Exports, "old", AstOldExprT);
	stringmap_insert(AstExprT->Exports, "or", AstOrExprT);
	stringmap_insert(AstExprT->Exports, "ref", AstRefExprT);
	stringmap_insert(AstExprT->Exports, "refin", AstRefInExprT);
	stringmap_insert(AstExprT->Exports, "refunpack", AstRefUnpackExprT);
	stringmap_insert(AstExprT->Exports, "register", AstRegisterExprT);
	stringmap_insert(AstExprT->Exports, "resolve", AstResolveExprT);
	stringmap_insert(AstExprT->Exports, "return", AstReturnExprT);
	stringmap_insert(AstExprT->Exports, "suspend", AstSuspendExprT);
	stringmap_insert(AstExprT->Exports, "switch", AstSwitchExprT);
	stringmap_insert(AstExprT->Exports, "tuple", AstTupleExprT);
	stringmap_insert(AstExprT->Exports, "unknown", AstUnknownExprT);
	stringmap_insert(AstExprT->Exports, "var", AstVarExprT);
	stringmap_insert(AstExprT->Exports, "varin", AstVarInExprT);
	stringmap_insert(AstExprT->Exports, "vartype", AstVarTypeExprT);
	stringmap_insert(AstExprT->Exports, "varunpack", AstVarUnpackExprT);
	stringmap_insert(AstExprT->Exports, "with", AstWithExprT);
	stringmap_insert(Ast->Exports, "local", AstLocalT);
	stringmap_insert(Ast->Exports, "param", AstParamT);
	stringmap_insert(Ast->Exports, "stringpart", AstStringPartT);
	stringmap_insert(Ast->Exports, "ifcase", AstIfCaseT);
}

static ml_value_t *a_mlc_local_t(mlc_local_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(AstLocalT,
		"ident", Struct->Ident ? ml_string(Struct->Ident, -1) : MLNil,
		"line", ml_integer(Struct->Line),
		"index", ml_integer(Struct->Index),
	NULL);
}

static ml_value_t *l_mlc_local_t(mlc_local_t *Struct) {
	ml_value_t *List = ml_list();
	while (Struct) {
		ml_list_put(List, a_mlc_local_t(Struct));
		Struct = Struct->Next;
	}
	return List;
}

static ml_value_t *ln_mlc_local_t(mlc_local_t *Struct, int Count) {
	ml_value_t *List = ml_list();
	for (int I = Count ?: 1; --I >= 0;) {
		ml_list_put(List, a_mlc_local_t(Struct));
		Struct = Struct->Next;
	}
	return List;
}

static ml_value_t *a_mlc_expr_t(ml_type_t *Class, mlc_expr_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(Class,
		"source", Struct->Source ? ml_string(Struct->Source, -1) : MLNil,
		"startline", ml_integer(Struct->StartLine),
		"endline", ml_integer(Struct->EndLine),
	NULL);
}

static ml_value_t *l_mlc_expr_t(mlc_expr_t *Struct) {
	ml_value_t *List = ml_list();
	while (Struct) {
		ml_list_put(List, mlc_expr_describe(Struct));
		Struct = Struct->Next;
	}
	return List;
}

static ml_value_t *a_mlc_param_t(mlc_param_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(AstParamT,
		"ident", Struct->Ident ? ml_string(Struct->Ident, -1) : MLNil,
		"type", l_mlc_expr_t(Struct->Type),
		"line", ml_integer(Struct->Line),
		"kind", ml_enum_value(ParamKindT, Struct->Kind),
	NULL);
}

static ml_value_t *l_mlc_param_t(mlc_param_t *Struct) {
	ml_value_t *List = ml_list();
	while (Struct) {
		ml_list_put(List, a_mlc_param_t(Struct));
		Struct = Struct->Next;
	}
	return List;
}

static ml_value_t *a_mlc_string_part_t(mlc_string_part_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(AstStringPartT,
	Struct->Length ? "chars" : "child", Struct->Length ? ml_string(Struct->Chars, -1) : l_mlc_expr_t(Struct->Child),
	NULL);
}

static ml_value_t *l_mlc_string_part_t(mlc_string_part_t *Struct) {
	ml_value_t *List = ml_list();
	while (Struct) {
		ml_list_put(List, a_mlc_string_part_t(Struct));
		Struct = Struct->Next;
	}
	return List;
}

static ml_value_t *a_mlc_if_case_t(mlc_if_case_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(AstIfCaseT,
		"condition", l_mlc_expr_t(Struct->Condition),
		"body", l_mlc_expr_t(Struct->Body),
		"local", l_mlc_local_t(Struct->Local),
		"token", ml_integer(Struct->Token),
	NULL);
}

static ml_value_t *l_mlc_if_case_t(mlc_if_case_t *Struct) {
	ml_value_t *List = ml_list();
	while (Struct) {
		ml_list_put(List, a_mlc_if_case_t(Struct));
		Struct = Struct->Next;
	}
	return List;
}

static ml_value_t *a_mlc_ident_expr_t(ml_type_t *Class, mlc_ident_expr_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(Class,
		"source", ml_string(Struct->Source, -1),
		"startline", ml_integer(Struct->StartLine),
		"endline", ml_integer(Struct->EndLine),
		"ident", Struct->Ident ? ml_string(Struct->Ident, -1) : MLNil,
	NULL);
}

static ml_value_t *a_mlc_fun_expr_t(ml_type_t *Class, mlc_fun_expr_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(Class,
		"source", ml_string(Struct->Source, -1),
		"startline", ml_integer(Struct->StartLine),
		"endline", ml_integer(Struct->EndLine),
		"name", Struct->Name ? ml_string(Struct->Name, -1) : MLNil,
		"params", l_mlc_param_t(Struct->Params),
		"body", l_mlc_expr_t(Struct->Body),
		"returntype", l_mlc_expr_t(Struct->ReturnType),
	NULL);
}

static ml_value_t *a_mlc_default_expr_t(ml_type_t *Class, mlc_default_expr_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(Class,
		"source", ml_string(Struct->Source, -1),
		"startline", ml_integer(Struct->StartLine),
		"endline", ml_integer(Struct->EndLine),
		"child", l_mlc_expr_t(Struct->Child),
		"index", ml_integer(Struct->Index),
		"flags", ml_integer(Struct->Flags),
	NULL);
}

static ml_value_t *a_mlc_string_expr_t(ml_type_t *Class, mlc_string_expr_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(Class,
		"source", ml_string(Struct->Source, -1),
		"startline", ml_integer(Struct->StartLine),
		"endline", ml_integer(Struct->EndLine),
		"parts", l_mlc_string_part_t(Struct->Parts),
	NULL);
}

static ml_value_t *a_mlc_parent_value_expr_t(ml_type_t *Class, mlc_parent_value_expr_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(Class,
		"source", ml_string(Struct->Source, -1),
		"startline", ml_integer(Struct->StartLine),
		"endline", ml_integer(Struct->EndLine),
		"child", l_mlc_expr_t(Struct->Child),
		"value", ml_ast_names(Struct->Value),
	NULL);
}

static ml_value_t *a_mlc_block_expr_t(ml_type_t *Class, mlc_block_expr_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(Class,
		"source", ml_string(Struct->Source, -1),
		"startline", ml_integer(Struct->StartLine),
		"endline", ml_integer(Struct->EndLine),
		"vars", l_mlc_local_t(Struct->Vars),
		"lets", l_mlc_local_t(Struct->Lets),
		"defs", l_mlc_local_t(Struct->Defs),
		"child", l_mlc_expr_t(Struct->Child),
		"catchbody", l_mlc_expr_t(Struct->CatchBody),
		"must", l_mlc_expr_t(Struct->Must),
		"catchident", Struct->CatchIdent ? ml_string(Struct->CatchIdent, -1) : MLNil,
		"numvars", ml_integer(Struct->NumVars),
		"numlets", ml_integer(Struct->NumLets),
		"numdefs", ml_integer(Struct->NumDefs),
	NULL);
}

static ml_value_t *a_mlc_for_expr_t(ml_type_t *Class, mlc_for_expr_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(Class,
		"source", ml_string(Struct->Source, -1),
		"startline", ml_integer(Struct->StartLine),
		"endline", ml_integer(Struct->EndLine),
		"key", Struct->Key ? ml_string(Struct->Key, -1) : MLNil,
		"local", l_mlc_local_t(Struct->Local),
		"sequence", l_mlc_expr_t(Struct->Sequence),
		"body", l_mlc_expr_t(Struct->Body),
		"name", Struct->Name ? ml_string(Struct->Name, -1) : MLNil,
		"unpack", ml_integer(Struct->Unpack),
	NULL);
}

static ml_value_t *a_mlc_local_expr_t(ml_type_t *Class, mlc_local_expr_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(Class,
		"source", ml_string(Struct->Source, -1),
		"startline", ml_integer(Struct->StartLine),
		"endline", ml_integer(Struct->EndLine),
		"local", ln_mlc_local_t(Struct->Local, Struct->Count),
		"child", l_mlc_expr_t(Struct->Child),
		"count", ml_integer(Struct->Count),
	NULL);
}

static ml_value_t *a_mlc_parent_expr_t(ml_type_t *Class, mlc_parent_expr_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(Class,
		"source", ml_string(Struct->Source, -1),
		"startline", ml_integer(Struct->StartLine),
		"endline", ml_integer(Struct->EndLine),
		"child", l_mlc_expr_t(Struct->Child),
		"name", Struct->Name ? ml_string(Struct->Name, -1) : MLNil,
	NULL);
}

static ml_value_t *a_mlc_if_expr_t(ml_type_t *Class, mlc_if_expr_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(Class,
		"source", ml_string(Struct->Source, -1),
		"startline", ml_integer(Struct->StartLine),
		"endline", ml_integer(Struct->EndLine),
		"cases", l_mlc_if_case_t(Struct->Cases),
		"else", l_mlc_expr_t(Struct->Else),
	NULL);
}

static ml_value_t *a_mlc_value_expr_t(ml_type_t *Class, mlc_value_expr_t *Struct) {
	if (!Struct) return MLNil;
	return ml_object(Class,
		"source", ml_string(Struct->Source, -1),
		"startline", ml_integer(Struct->StartLine),
		"endline", ml_integer(Struct->EndLine),
		"value", ml_ast_names(Struct->Value),
	NULL);
}

ml_value_t *mlc_expr_describe(mlc_expr_t *Expr) {
	if (!Expr) return MLNil;
	switch (mlc_expr_type(Expr)) {
		case ML_EXPR_AND: return a_mlc_parent_expr_t(AstAndExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_ASSIGN: return a_mlc_parent_expr_t(AstAssignExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_BLANK: return a_mlc_expr_t(AstBlankExprT, (mlc_expr_t *)Expr);
		case ML_EXPR_BLOCK: return a_mlc_block_expr_t(AstBlockExprT, (mlc_block_expr_t *)Expr);
		case ML_EXPR_CALL: return a_mlc_parent_expr_t(AstCallExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_CONST_CALL: return a_mlc_parent_value_expr_t(AstConstCallExprT, (mlc_parent_value_expr_t *)Expr);
		case ML_EXPR_DEBUG: return a_mlc_parent_expr_t(AstDebugExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_DEF: return a_mlc_local_expr_t(AstDefExprT, (mlc_local_expr_t *)Expr);
		case ML_EXPR_DEF_IN: return a_mlc_local_expr_t(AstDefInExprT, (mlc_local_expr_t *)Expr);
		case ML_EXPR_DEF_UNPACK: return a_mlc_local_expr_t(AstDefUnpackExprT, (mlc_local_expr_t *)Expr);
		case ML_EXPR_DEFAULT: return a_mlc_default_expr_t(AstDefaultExprT, (mlc_default_expr_t *)Expr);
		case ML_EXPR_DEFINE: return a_mlc_ident_expr_t(AstDefineExprT, (mlc_ident_expr_t *)Expr);
		case ML_EXPR_DELEGATE: return a_mlc_parent_expr_t(AstDelegateExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_EACH: return a_mlc_parent_expr_t(AstEachExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_EXIT: return a_mlc_parent_expr_t(AstExitExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_FOR: return a_mlc_for_expr_t(AstForExprT, (mlc_for_expr_t *)Expr);
		case ML_EXPR_FUN: return a_mlc_fun_expr_t(AstFunExprT, (mlc_fun_expr_t *)Expr);
		case ML_EXPR_GUARD: return a_mlc_parent_expr_t(AstGuardExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_IDENT: return a_mlc_ident_expr_t(AstIdentExprT, (mlc_ident_expr_t *)Expr);
		case ML_EXPR_IF: return a_mlc_if_expr_t(AstIfExprT, (mlc_if_expr_t *)Expr);
		case ML_EXPR_INLINE: return a_mlc_parent_expr_t(AstInlineExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_IT: return a_mlc_expr_t(AstItExprT, (mlc_expr_t *)Expr);
		case ML_EXPR_LET: return a_mlc_local_expr_t(AstLetExprT, (mlc_local_expr_t *)Expr);
		case ML_EXPR_LET_IN: return a_mlc_local_expr_t(AstLetInExprT, (mlc_local_expr_t *)Expr);
		case ML_EXPR_LET_UNPACK: return a_mlc_local_expr_t(AstLetUnpackExprT, (mlc_local_expr_t *)Expr);
		case ML_EXPR_LIST: return a_mlc_parent_expr_t(AstListExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_LOOP: return a_mlc_parent_expr_t(AstLoopExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_MAP: return a_mlc_parent_expr_t(AstMapExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_NEXT: return a_mlc_parent_expr_t(AstNextExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_NIL: return a_mlc_expr_t(AstNilExprT, (mlc_expr_t *)Expr);
		case ML_EXPR_NOT: return a_mlc_parent_expr_t(AstNotExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_OLD: return a_mlc_expr_t(AstOldExprT, (mlc_expr_t *)Expr);
		case ML_EXPR_OR: return a_mlc_parent_expr_t(AstOrExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_REF: return a_mlc_local_expr_t(AstRefExprT, (mlc_local_expr_t *)Expr);
		case ML_EXPR_REF_IN: return a_mlc_local_expr_t(AstRefInExprT, (mlc_local_expr_t *)Expr);
		case ML_EXPR_REF_UNPACK: return a_mlc_local_expr_t(AstRefUnpackExprT, (mlc_local_expr_t *)Expr);
		case ML_EXPR_REGISTER: return a_mlc_expr_t(AstRegisterExprT, (mlc_expr_t *)Expr);
		case ML_EXPR_RESOLVE: return a_mlc_parent_value_expr_t(AstResolveExprT, (mlc_parent_value_expr_t *)Expr);
		case ML_EXPR_RETURN: return a_mlc_parent_expr_t(AstReturnExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_SCOPED: return ml_error("TypeError", "Unsupported expression type: scoped");
		case ML_EXPR_STRING: return a_mlc_string_expr_t(AstStringExprT, (mlc_string_expr_t *)Expr);
		case ML_EXPR_SUBST: return ml_error("TypeError", "Unsupported expression type: subst");
		case ML_EXPR_SUSPEND: return a_mlc_parent_expr_t(AstSuspendExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_SWITCH: return a_mlc_parent_expr_t(AstSwitchExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_TUPLE: return a_mlc_parent_expr_t(AstTupleExprT, (mlc_parent_expr_t *)Expr);
		case ML_EXPR_UNKNOWN: return a_mlc_expr_t(AstUnknownExprT, (mlc_expr_t *)Expr);
		case ML_EXPR_VALUE: return a_mlc_value_expr_t(AstValueExprT, (mlc_value_expr_t *)Expr);
		case ML_EXPR_VAR: return a_mlc_local_expr_t(AstVarExprT, (mlc_local_expr_t *)Expr);
		case ML_EXPR_VAR_IN: return a_mlc_local_expr_t(AstVarInExprT, (mlc_local_expr_t *)Expr);
		case ML_EXPR_VAR_TYPE: return a_mlc_local_expr_t(AstVarTypeExprT, (mlc_local_expr_t *)Expr);
		case ML_EXPR_VAR_UNPACK: return a_mlc_local_expr_t(AstVarUnpackExprT, (mlc_local_expr_t *)Expr);
		case ML_EXPR_WITH: return a_mlc_local_expr_t(AstWithExprT, (mlc_local_expr_t *)Expr);
	}
}


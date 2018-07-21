#include "minilang.h"
#include "stringmap.h"
#include <gc.h>
#include <ctype.h>
#include <string.h>

struct mlc_decl_t {
	mlc_decl_t *Next;
	const char *Ident;
	int Index;
};

struct mlc_loop_t {
	mlc_loop_t *Up;
	mlc_try_t *Try;
	ml_inst_t *Next, *Exits;
	int NextTop, ExitTop;
};

struct mlc_try_t {
	mlc_try_t *Up;
	ml_inst_t *CatchInst;
	int CatchTop;
};

struct mlc_upvalue_t {
	mlc_upvalue_t *Next;
	mlc_decl_t *Decl;
	int Index;
};

#define MLC_EXPR_FIELDS(TYPE) \
	mlc_compiled_t (*compile)(mlc_function_t *, mlc_ ## TYPE ## _expr_t *, SHA256_CTX *HashContext); \
	mlc_expr_t *Next; \
	ml_source_t Source

struct mlc_expr_t {
	mlc_compiled_t (*compile)(mlc_function_t *, mlc_expr_t *, SHA256_CTX *HashContext);
	mlc_expr_t *Next;
	ml_source_t Source;
};

static inline ml_inst_t *ml_inst_new(int N, ml_source_t Source, ml_inst_t *(*run)(ml_inst_t *Inst, ml_frame_t *Frame)) {
	ml_inst_t *Inst = xnew(ml_inst_t, N, ml_param_t);
	Inst->Source = Source;
	Inst->run = run;
	return Inst;
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define ML_COMPILE_HASH sha256_update(HashContext, (BYTE *)__FILE__ TOSTRING(__LINE__), strlen(__FILE__ TOSTRING(__LINE__)));

inline mlc_compiled_t ml_compile(mlc_function_t *Function, mlc_expr_t *Expr, SHA256_CTX *HashContext) {
	//static int Indent = NULL;
	if (Expr) {
		//for (int I = Indent; --I >= 0;) printf("\t");
		//printf("before compiling %s:%d Function->Top = %d\n", Expr->Source.Name, Expr->Source.Line, Function->Top);
		//++Indent;
		mlc_compiled_t Compiled = Expr->compile(Function, Expr, HashContext);
		//--Indent;
		//for (int I = Indent; --I >= 0;) printf("\t");
		//printf("after compiling %s:%d Function->Top = %d\n", Expr->Source.Name, Expr->Source.Line, Function->Top);
		//printf("after compiling %s:%d state =", Expr->Source.Name, Expr->Source.Line);
		//for (int I = 0; I < 8; ++I) printf("%08x", HashContext->state[I]);
		//printf("\n");
		return Compiled;
	} else {
		ML_COMPILE_HASH
		ml_inst_t *NilInst = ml_inst_new(2, (ml_source_t){"<internal>", 0}, mli_push_run);
		NilInst->Params[1].Value = MLNil;
		++Function->Top;
		return (mlc_compiled_t){NilInst, NilInst};
	}
}

inline void mlc_connect(ml_inst_t *Exits, ml_inst_t *Start) {
	for (ml_inst_t *Exit = Exits; Exit;) {
		ml_inst_t *NextExit = Exit->Params[0].Inst;
		Exit->Params[0].Inst = Start;
		Exit = NextExit;
	}
}

typedef struct mlc_if_expr_t mlc_if_expr_t;
typedef struct mlc_if_case_t mlc_if_case_t;
typedef struct mlc_parent_expr_t mlc_parent_expr_t;
typedef struct mlc_fun_expr_t mlc_fun_expr_t;
typedef struct mlc_decl_expr_t mlc_decl_expr_t;
typedef struct mlc_dot_expr_t mlc_dot_expr_t;
typedef struct mlc_value_expr_t mlc_value_expr_t;
typedef struct mlc_ident_expr_t mlc_ident_expr_t;
typedef struct mlc_const_call_expr_t mlc_const_call_expr_t;
typedef struct mlc_block_expr_t mlc_block_expr_t;

struct mlc_if_case_t {
	mlc_if_case_t *Next;
	ml_source_t Source;
	mlc_expr_t *Condition;
	mlc_expr_t *Body;
};

struct mlc_if_expr_t {
	MLC_EXPR_FIELDS(if);
	mlc_if_case_t *Cases;
	mlc_expr_t *Else;
};

static mlc_compiled_t ml_if_expr_compile(mlc_function_t *Function, mlc_if_expr_t *Expr, SHA256_CTX *HashContext) {
	int OldTop = Function->Top;
	mlc_if_case_t *Case = Expr->Cases;
	mlc_compiled_t Compiled = ml_compile(Function, Case->Condition, HashContext);
	--Function->Top;
	mlc_compiled_t BodyCompiled = ml_compile(Function, Case->Body, HashContext);
	ML_COMPILE_HASH
	ml_inst_t *IfInst = ml_inst_new(2, Expr->Source, mli_and_run);
	IfInst->Params[0].Inst = BodyCompiled.Exits;
	IfInst->Params[1].Inst = BodyCompiled.Start;
	mlc_connect(Compiled.Exits, IfInst);
	Compiled.Exits = IfInst;
	while ((Case = Case->Next)) {
		Function->Top = OldTop;
		Compiled.Exits = IfInst->Params[0].Inst;
		mlc_compiled_t ConditionCompiled = ml_compile(Function, Case->Condition, HashContext);
		IfInst->run = mli_if_run;
		IfInst->Params[0].Inst = ConditionCompiled.Start;
		--Function->Top;
		BodyCompiled = ml_compile(Function, Case->Body, HashContext);
		ml_inst_t **Slot = &Compiled.Exits;
		while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
		Slot[0] = BodyCompiled.Exits;
		ML_COMPILE_HASH
		IfInst = ml_inst_new(2, Case->Source, mli_and_run);
		IfInst->Params[0].Inst = Compiled.Exits;
		IfInst->Params[1].Inst = BodyCompiled.Start;
		mlc_connect(ConditionCompiled.Exits, IfInst);
		Compiled.Exits = IfInst;
	}
	Function->Top = OldTop;
	if (Expr->Else) {
		Compiled.Exits = IfInst->Params[0].Inst;
		mlc_compiled_t BodyCompiled = ml_compile(Function, Expr->Else, HashContext);
		IfInst->run = mli_if_run;
		IfInst->Params[0].Inst = BodyCompiled.Start;
		ml_inst_t **Slot = &Compiled.Exits;
		while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
		Slot[0] = BodyCompiled.Exits;
	} else {
		++Function->Top;
	}
	return Compiled;
}

struct mlc_parent_expr_t {
	MLC_EXPR_FIELDS(parent);
	mlc_expr_t *Child;
};

static mlc_compiled_t ml_or_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	int OldTop = Function->Top;
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = ml_compile(Function, Child, HashContext);
	ML_COMPILE_HASH
	ml_inst_t *OrInst = ml_inst_new(2, Expr->Source, mli_or_run);
	mlc_connect(Compiled.Exits, OrInst);
	Compiled.Exits = OrInst;
	for (Child = Child->Next; Child->Next; Child = Child->Next) {
		Function->Top = OldTop;
		mlc_compiled_t ChildCompiled = ml_compile(Function, Child, HashContext);
		OrInst->Params[1].Inst = ChildCompiled.Start;
		ML_COMPILE_HASH
		OrInst = ml_inst_new(2, Expr->Source, mli_or_run);
		mlc_connect(ChildCompiled.Exits, OrInst);
		OrInst->Params[0].Inst = Compiled.Exits;
		Compiled.Exits = OrInst;
	}
	Function->Top = OldTop;
	mlc_compiled_t ChildCompiled = ml_compile(Function, Child, HashContext);
	OrInst->Params[1].Inst = ChildCompiled.Start;
	ml_inst_t **Slot = &Compiled.Exits;
	while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
	Slot[0] = ChildCompiled.Exits;
	return Compiled;
}

static mlc_compiled_t ml_and_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	int OldTop = Function->Top;
	ML_COMPILE_HASH
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = ml_compile(Function, Child, HashContext);
	ML_COMPILE_HASH
	ml_inst_t *IfInst = ml_inst_new(2, Expr->Source, mli_and_run);
	mlc_connect(Compiled.Exits, IfInst);
	Compiled.Exits = IfInst;
	for (Child = Child->Next; Child->Next; Child = Child->Next) {
		Function->Top = OldTop;
		mlc_compiled_t ChildCompiled = ml_compile(Function, Child, HashContext);
		IfInst->Params[1].Inst = ChildCompiled.Start;
		ML_COMPILE_HASH
		IfInst = ml_inst_new(2, Expr->Source, mli_and_run);
		mlc_connect(ChildCompiled.Exits, IfInst);
		IfInst->Params[0].Inst = Compiled.Exits;
		Compiled.Exits = IfInst;
	}
	Function->Top = OldTop;
	mlc_compiled_t ChildCompiled = ml_compile(Function, Child, HashContext);
	IfInst->Params[1].Inst = ChildCompiled.Start;
	ml_inst_t **Slot = &Compiled.Exits;
	while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
	Slot[0] = ChildCompiled.Exits;
	return Compiled;
}

static mlc_compiled_t ml_loop_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	int OldTop = Function->Top;
	ML_COMPILE_HASH
	ml_inst_t *LoopInst = ml_inst_new(1, Expr->Source, mli_pop_run);
	mlc_loop_t Loop = {
		Function->Loop, Function->Try,
		LoopInst, NULL,
		Function->Top + 1, Function->Top + 1
	};
	Function->Loop = &Loop;
	mlc_compiled_t Compiled = ml_compile(Function, Expr->Child, HashContext);
	LoopInst->Params[0].Inst = Compiled.Start;
	mlc_connect(Compiled.Exits, LoopInst);
	Function->Loop = Loop.Up;
	Function->Top = OldTop + 1;
	Compiled.Exits = Loop.Exits;
	return Compiled;
}

static mlc_compiled_t ml_next_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr, SHA256_CTX *HashContext) {
	ml_inst_t *NilInst = ml_inst_new(2, (ml_source_t){"<internal>", 0}, mli_push_run);
	NilInst->Params[1].Value = MLNil;
	ml_inst_t *NextInst = Function->Loop->Next;
	Function->Top++;
	if (Function->Try != Function->Loop->Try) {
		ML_COMPILE_HASH
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, mli_try_run);
		TryInst->Params[1].Inst = Function->Try ? Function->Try->CatchInst : NULL;
		TryInst->Params[0].Inst = Function->Loop->Next;
		NextInst = TryInst;
	}
	if (Function->Top > Function->Loop->NextTop) {
		ML_COMPILE_HASH
		ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, mli_exit_run);
		ExitInst->Params[0].Inst = NextInst;
		ExitInst->Params[1].Count = Function->Top - Function->Loop->NextTop;
		NilInst->Params[0].Inst = ExitInst;

	} else {
		ML_COMPILE_HASH
		NilInst->Params[0].Inst = NextInst;
	}
	Function->Top--;
	return (mlc_compiled_t){NilInst, NULL};
}

static mlc_compiled_t ml_exit_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	mlc_loop_t *Loop = Function->Loop;
	mlc_try_t *Try = Function->Try;
	Function->Loop = Loop->Up;
	Function->Try = Loop->Try;
	mlc_compiled_t Compiled = ml_compile(Function, Expr->Child, HashContext);
	if (Function->Try != Try) {
		ML_COMPILE_HASH
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, mli_try_run);
		TryInst->Params[1].Inst = Function->Try ? Function->Try->CatchInst : NULL;
		TryInst->Params[0].Inst = Compiled.Start;
		Compiled.Start = TryInst;
	}
	Function->Loop = Loop;
	Function->Try = Try;
	if (Function->Top > Function->Loop->ExitTop) {
		ML_COMPILE_HASH
		ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, mli_exit_run);
		ExitInst->Params[1].Count = Function->Top - Function->Loop->ExitTop;
		mlc_connect(Compiled.Exits, ExitInst);
		Compiled.Exits = ExitInst;
	}
	if (Compiled.Exits) {
		ml_inst_t **Slot = &Function->Loop->Exits;
		while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
		Slot[0] = Compiled.Exits;
	}
	return (mlc_compiled_t){Compiled.Start, NULL};
}

static mlc_compiled_t ml_not_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	mlc_compiled_t Compiled = ml_compile(Function, Expr->Child, HashContext);
	ML_COMPILE_HASH
	ml_inst_t *NotInst = ml_inst_new(2, Expr->Source, mli_if_run);
	mlc_connect(Compiled.Exits, NotInst);
	ml_inst_t *NilInst = ml_inst_new(2, Expr->Source, mli_push_run);
	NilInst->Params[1].Value = MLNil;
	ml_inst_t *SomeInst = ml_inst_new(2, Expr->Source, mli_push_run);
	SomeInst->Params[1].Value = MLSome;
	NotInst->Params[0].Inst = SomeInst;
	NotInst->Params[1].Inst = NilInst;
	NilInst->Params[0].Inst = SomeInst;
	Compiled.Exits = NilInst;
	return Compiled;
}

static mlc_compiled_t ml_while_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	mlc_compiled_t Compiled = ml_compile(Function, Expr->Child, HashContext);
	ML_COMPILE_HASH
	ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, mli_exit_run);
	ExitInst->Params[1].Count = Function->Top - Function->Loop->ExitTop;
	mlc_loop_t *Loop = Function->Loop;
	if (Function->Try != Loop->Try) {
		ML_COMPILE_HASH
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, mli_try_run);
		TryInst->Params[1].Inst = Loop->Try ? Loop->Try->CatchInst : NULL;
		TryInst->Params[0].Inst = ExitInst;
		ExitInst = TryInst;
	}
	ML_COMPILE_HASH
	ml_inst_t *WhileInst = ml_inst_new(2, Expr->Source, mli_while_run);
	mlc_connect(Compiled.Exits, WhileInst);
	Compiled.Exits = WhileInst;
	WhileInst->Params[1].Inst = ExitInst;
	ExitInst->Params[0].Inst = Function->Loop->Exits;
	Function->Loop->Exits = ExitInst;
	return Compiled;
}

static mlc_compiled_t ml_until_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	mlc_compiled_t Compiled = ml_compile(Function, Expr->Child, HashContext);
	ML_COMPILE_HASH
	ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, mli_exit_run);
	ExitInst->Params[1].Count = Function->Top - Function->Loop->ExitTop;
	mlc_loop_t *Loop = Function->Loop;
	if (Function->Try != Loop->Try) {
		ML_COMPILE_HASH
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, mli_try_run);
		TryInst->Params[1].Inst = Loop->Try ? Loop->Try->CatchInst : NULL;
		TryInst->Params[0].Inst = ExitInst;
		ExitInst = TryInst;
	}
	ML_COMPILE_HASH
	ml_inst_t *UntilInst = ml_inst_new(2, Expr->Source, mli_until_run);
	mlc_connect(Compiled.Exits, UntilInst);
	Compiled.Exits = UntilInst;
	UntilInst->Params[1].Inst = ExitInst;
	ExitInst->Params[0].Inst = Function->Loop->Exits;
	Function->Loop->Exits = ExitInst;
	return Compiled;
}

static mlc_compiled_t ml_return_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	mlc_compiled_t Compiled = ml_compile(Function, Expr->Child, HashContext);
	mlc_connect(Compiled.Exits, NULL);
	Compiled.Exits = NULL;
	return Compiled;
}

struct mlc_decl_expr_t {
	MLC_EXPR_FIELDS(decl);
	mlc_decl_t *Decl;
	mlc_expr_t *Child;
};

static mlc_compiled_t ml_var_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr, SHA256_CTX *HashContext) {
	mlc_compiled_t Compiled = ml_compile(Function, Expr->Child, HashContext);
	ML_COMPILE_HASH
	ml_inst_t *VarInst = ml_inst_new(2, Expr->Source, mli_var_run);
	VarInst->Params[1].Index = Expr->Decl->Index;
	mlc_connect(Compiled.Exits, VarInst);
	Compiled.Exits = VarInst;
	return Compiled;
}

static mlc_compiled_t ml_def_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr, SHA256_CTX *HashContext) {
	mlc_compiled_t Compiled = ml_compile(Function, Expr->Child, HashContext);
	ML_COMPILE_HASH
	ml_inst_t *DefInst = ml_inst_new(1, Expr->Source, mli_def_run);
	mlc_decl_t *Decl = Expr->Decl;
	Decl->Index = Function->Top - 1;
	Decl->Next = Function->Decls;
	Function->Decls = Decl;
	mlc_connect(Compiled.Exits, DefInst);
	Compiled.Exits = DefInst;
	return Compiled;
}

static mlc_compiled_t ml_with_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr, SHA256_CTX *HashContext) {
	int OldTop = Function->Top + 1;
	mlc_decl_t *OldScope = Function->Decls;
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = ml_compile(Function, Child, HashContext);
	mlc_decl_t *Decl = Expr->Decl;
	Decl->Index = Function->Top - 1;
	Child = Child->Next;
	mlc_decl_t *NextDecl = Decl->Next;
	Decl->Next = Function->Decls;
	Function->Decls = Decl;
	while (NextDecl) {
		Decl = NextDecl;
		NextDecl = Decl->Next;
		mlc_compiled_t ChildCompiled = ml_compile(Function, Child, HashContext);
		mlc_connect(Compiled.Exits, ChildCompiled.Start);
		Compiled.Exits = ChildCompiled.Exits;
		Decl->Index = Function->Top - 1;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
		Child = Child->Next;
	}
	mlc_compiled_t ChildCompiled = ml_compile(Function, Child, HashContext);
	mlc_connect(Compiled.Exits, ChildCompiled.Start);
	ML_COMPILE_HASH
	ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, mli_exit_run);
	ExitInst->Params[1].Count = Function->Top - OldTop;
	mlc_connect(ChildCompiled.Exits, ExitInst);
	Compiled.Exits = ExitInst;
	Function->Decls = OldScope;
	Function->Top = OldTop;
	return Compiled;
}

static mlc_compiled_t ml_for_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr, SHA256_CTX *HashContext) {
	int OldTop = Function->Top;
	mlc_decl_t *OldScope = Function->Decls;
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = ml_compile(Function, Child, HashContext);
	ml_inst_t *StartInst = ml_inst_new(2, Expr->Source, mli_until_run);
	mlc_connect(Compiled.Exits, StartInst);
	mlc_decl_t *Decl = Expr->Decl;
	Decl->Index = Function->Top - 1;
	mlc_decl_t *KeyDecl = Decl->Next;
	if (KeyDecl) {
		KeyDecl->Index = (Decl->Index = Function->Top) - 1;
		if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
		KeyDecl->Next = Function->Decls;
	} else {
		Decl->Next = Function->Decls;
	}
	Function->Decls = Decl;
	ML_COMPILE_HASH
	ml_inst_t *NextInst = ml_inst_new(2, Expr->Source, mli_next_run);
	ML_COMPILE_HASH
	ml_inst_t *PopInst = ml_inst_new(1, Expr->Source, mli_pop_run);
	PopInst->Params[0].Inst = NextInst;
	mlc_loop_t Loop = {
		Function->Loop, Function->Try,
		PopInst, NULL,
		Function->Top + 1, OldTop + 1
	};
	Function->Loop = &Loop;
	mlc_compiled_t BodyCompiled = ml_compile(Function, Child->Next, HashContext);
	mlc_connect(BodyCompiled.Exits, PopInst);
	if (KeyDecl) {
		ML_COMPILE_HASH
		ml_inst_t *KeyInst = ml_inst_new(1, Expr->Source, mli_key_run);
		KeyInst->Params[0].Inst = BodyCompiled.Start;
		NextInst->Params[1].Inst = KeyInst;
		StartInst->Params[1].Inst = KeyInst;
		PopInst->run = mli_pop2_run;
	} else {
		NextInst->Params[1].Inst = BodyCompiled.Start;
		StartInst->Params[1].Inst = BodyCompiled.Start;
	}
	Compiled.Exits = Loop.Exits;
	Function->Loop = Loop.Up;
	Function->Top = OldTop;
	if (Child->Next->Next) {
		mlc_compiled_t ElseCompiled = ml_compile(Function, Child->Next->Next, HashContext);
		ML_COMPILE_HASH
		ml_inst_t *PopInst = ml_inst_new(1, Expr->Source, mli_pop_run);
		PopInst->Params[0].Inst = ElseCompiled.Start;
		StartInst->Params[0].Inst = PopInst;
		NextInst->Params[0].Inst = PopInst;
		ml_inst_t **Slot = &Compiled.Exits;
		while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
		Slot[0] = ElseCompiled.Exits;
	} else {
		++Function->Top;
		NextInst->Params[0].Inst = Compiled.Exits;
		StartInst->Params[0].Inst = NextInst;
		Compiled.Exits = StartInst;
	}
	Function->Decls = OldScope;
	return Compiled;
}

static mlc_compiled_t ml_all_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	ml_inst_t *ListInst = ml_inst_new(1, Expr->Source, mli_list_run);
	++Function->Top;
	mlc_compiled_t Compiled = ml_compile(Function, Expr->Child, HashContext);
	ListInst->Params[0].Inst = Compiled.Start;
	ml_inst_t *UntilInst = ml_inst_new(2, Expr->Source, mli_until_run);
	mlc_connect(Compiled.Exits, UntilInst);
	ml_inst_t *AppendInst = ml_inst_new(1, Expr->Source, mli_append_run);
	UntilInst->Params[1].Inst = AppendInst;
	ml_inst_t *NextInst = ml_inst_new(2, Expr->Source, mli_next_run);
	ml_inst_t *PopInst = ml_inst_new(1, Expr->Source, mli_pop_run);
	AppendInst->Params[0].Inst = NextInst;
	UntilInst->Params[0].Inst = PopInst;
	NextInst->Params[0].Inst = PopInst;
	NextInst->Params[1].Inst = AppendInst;
	return (mlc_compiled_t){ListInst, PopInst};
}

struct mlc_block_expr_t {
	MLC_EXPR_FIELDS(block);
	mlc_decl_t *Decl;
	mlc_expr_t *Child;
	mlc_decl_t *CatchDecl;
	mlc_expr_t *Catch;
};

static mlc_compiled_t ml_block_expr_compile(mlc_function_t *Function, mlc_block_expr_t *Expr, SHA256_CTX *HashContext) {
	int OldTop = Function->Top + 1, NumVars = 0, NumDefs = 0;
	mlc_decl_t *OldScope = Function->Decls;
	mlc_try_t Try;
	ml_inst_t *CatchExitInst;
	if (Expr->Catch) {
		ML_COMPILE_HASH
		Expr->CatchDecl->Index = Function->Top++;
		Expr->CatchDecl->Next = Function->Decls;
		Function->Decls = Expr->CatchDecl;
		mlc_compiled_t TryCompiled = ml_compile(Function, Expr->Catch, HashContext);
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, mli_try_run);
		ml_inst_t *CatchInst = ml_inst_new(2, Expr->Source, mli_catch_run);
		TryInst->Params[0].Inst = CatchInst;
		TryInst->Params[1].Inst = Function->Try ? Function->Try->CatchInst : NULL;
		CatchInst->Params[0].Inst = TryCompiled.Start;
		CatchInst->Params[1].Index = OldTop;
		Function->Decls = OldScope;
		Function->Top = OldTop - 1;
		Try.Up = Function->Try;
		Try.CatchInst = TryInst;
		Try.CatchTop = OldTop;
		CatchExitInst = ml_inst_new(2, Expr->Source, mli_exit_run);
		CatchExitInst->Params[1].Count = 1;
		mlc_connect(TryCompiled.Exits, CatchExitInst);
		Function->Try = &Try;
	}
	for (mlc_decl_t *Decl = Expr->Decl; Decl;) {
		Decl->Index = Function->Top++;
		mlc_decl_t *NextDecl = Decl->Next;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
		Decl = NextDecl;
		++NumVars;
	}
	if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = ml_compile(Function, Child, HashContext);
	if (Child) while ((Child = Child->Next)) {
		ML_COMPILE_HASH
		ml_inst_t *PopInst = ml_inst_new(1, Expr->Source, mli_pop_run);
		mlc_connect(Compiled.Exits, PopInst);
		--Function->Top;
		mlc_compiled_t ChildCompiled = ml_compile(Function, Child, HashContext);
		PopInst->Params[0].Inst = ChildCompiled.Start;
		Compiled.Exits = ChildCompiled.Exits;
	}
	if (NumVars > 0) {
		ML_COMPILE_HASH
		ml_inst_t *EnterInst = ml_inst_new(2, Expr->Source, mli_enter_run);
		EnterInst->Params[0].Inst = Compiled.Start;
		EnterInst->Params[1].Count = NumVars;
		Compiled.Start = EnterInst;
	}
	if (NumVars + NumDefs > 0) {
		ML_COMPILE_HASH
		ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, mli_exit_run);
		ExitInst->Params[1].Count = NumVars + NumDefs;
		mlc_connect(Compiled.Exits, ExitInst);
		Compiled.Exits = ExitInst;
	}
	if (Expr->Catch) {
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, mli_try_run);
		TryInst->Params[0].Inst = Compiled.Start;
		TryInst->Params[1].Inst = Try.CatchInst;
		Compiled.Start = TryInst;
		Function->Try = Try.Up;
		TryInst = ml_inst_new(2, Expr->Source, mli_try_run);
		TryInst->Params[1].Inst = Function->Try ? Function->Try->CatchInst : NULL;
		TryInst->Params[0].Inst = CatchExitInst;
		mlc_connect(Compiled.Exits, TryInst);
		Compiled.Exits = TryInst;
	}
	Function->Decls = OldScope;
	Function->Top = OldTop;
	return Compiled;
}

static mlc_compiled_t ml_call_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	int OldTop = Function->Top + 1;
	mlc_compiled_t Compiled = ml_compile(Function, Expr->Child, HashContext);
	int NumArgs = 0;
	for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next) {
		++NumArgs;
		mlc_compiled_t ChildCompiled = ml_compile(Function, Child, HashContext);
		mlc_connect(Compiled.Exits, ChildCompiled.Start);
		Compiled.Exits = ChildCompiled.Exits;
	}
	ML_COMPILE_HASH
	ml_inst_t *CallInst = ml_inst_new(2, Expr->Source, mli_call_run);
	CallInst->Params[1].Count = NumArgs;
	mlc_connect(Compiled.Exits, CallInst);
	Compiled.Exits = CallInst;
	Function->Top = OldTop;
	return Compiled;
}

static mlc_compiled_t ml_assign_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	int OldSelf = Function->Self;
	mlc_compiled_t Compiled = ml_compile(Function, Expr->Child, HashContext);
	Function->Self = Function->Top - 1;
	mlc_compiled_t ValueCompiled = ml_compile(Function, Expr->Child->Next, HashContext);
	mlc_connect(Compiled.Exits, ValueCompiled.Start);
	ML_COMPILE_HASH
	ml_inst_t *AssignInst = ml_inst_new(1, Expr->Source, mli_assign_run);
	mlc_connect(ValueCompiled.Exits, AssignInst);
	Compiled.Exits = AssignInst;
	Function->Top -= 1;
	Function->Self = OldSelf;
	return Compiled;
}

static mlc_compiled_t ml_old_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	ml_inst_t *OldInst = ml_inst_new(2, Expr->Source, mli_local_run);
	OldInst->Params[1].Index = Function->Self;
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	return (mlc_compiled_t){OldInst, OldInst};
}

struct mlc_const_call_expr_t {
	MLC_EXPR_FIELDS(const_call);
	mlc_expr_t *Child;
	ml_value_t *Value;
};

static mlc_compiled_t ml_const_call_expr_compile(mlc_function_t *Function, mlc_const_call_expr_t *Expr, SHA256_CTX *HashContext) {
	int OldTop = Function->Top + 1;
	if (OldTop >= Function->Size) Function->Size = Function->Top + 1;
	long ValueHash = ml_hash(Expr->Value);
	sha256_update(HashContext, (void *)&ValueHash, sizeof(ValueHash));
	ML_COMPILE_HASH
	ml_inst_t *CallInst = ml_inst_new(3, Expr->Source, mli_const_call_run);
	CallInst->Params[2].Value = Expr->Value;
	if (Expr->Child) {
		int NumArgs = 1;
		mlc_compiled_t Compiled = ml_compile(Function, Expr->Child, HashContext);
		for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next) {
			++NumArgs;
			mlc_compiled_t ChildCompiled = ml_compile(Function, Child, HashContext);
			mlc_connect(Compiled.Exits, ChildCompiled.Start);
			Compiled.Exits = ChildCompiled.Exits;
		}
		CallInst->Params[1].Count = NumArgs;
		mlc_connect(Compiled.Exits, CallInst);
		Compiled.Exits = CallInst;
		Function->Top = OldTop;
		return Compiled;
	} else {
		CallInst->Params[1].Count = 0;
		Function->Top = OldTop;
		return (mlc_compiled_t){CallInst, CallInst};
	}
}

struct mlc_fun_expr_t {
	MLC_EXPR_FIELDS(fun);
	mlc_decl_t *Params;
	mlc_expr_t *Body;
};

static mlc_compiled_t ml_fun_expr_compile(mlc_function_t *Function, mlc_fun_expr_t *Expr, SHA256_CTX *HashContext) {
	// closure <entry> <frame_size> <num_params> <num_upvalues> <upvalue_1> ...
	mlc_function_t SubFunction[1] = {{Function->GlobalGet, Function->Globals, NULL,}};
	SubFunction->Up = Function;
	int NumParams = 0;
	mlc_decl_t **ParamSlot = &SubFunction->Decls;
	for (mlc_decl_t *Param = Expr->Params; Param;) {
		mlc_decl_t *NextParam = Param->Next;
		++NumParams;
		if (Param->Index) NumParams = ~NumParams;
		Param->Index = SubFunction->Top++;
		ParamSlot[0] = Param;
		ParamSlot = &Param->Next;
		Param = NextParam;
	}
	SubFunction->Size = SubFunction->Top + 1;
	SHA256_CTX SubHashContext[1];
	sha256_init(SubHashContext);
	mlc_compiled_t Compiled = ml_compile(SubFunction, Expr->Body, SubHashContext);
	mlc_connect(Compiled.Exits, NULL);
	int NumUpValues = 0;
	for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next) ++NumUpValues;
	ML_COMPILE_HASH
	ml_inst_t *ClosureInst = ml_inst_new(2 + NumUpValues, Expr->Source, mli_closure_run);
	ml_param_t *Params = ClosureInst->Params;
	ml_closure_info_t *Info = new(ml_closure_info_t);
	Info->Entry = Compiled.Start;
	Info->FrameSize = SubFunction->Size;
	Info->NumParams = NumParams;
	Info->NumUpValues = NumUpValues;
	sha256_final(SubHashContext, Info->Hash);
	Params[1].ClosureInfo = Info;
	sha256_update(HashContext, Info->Hash, SHA256_BLOCK_SIZE);
	int Index = 2;
	for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next) Params[Index++].Index = UpValue->Index;
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	return (mlc_compiled_t){ClosureInst, ClosureInst};
}

struct mlc_ident_expr_t {
	MLC_EXPR_FIELDS(ident);
	const char *Ident;
};

static int ml_upvalue_find(mlc_function_t *Function, mlc_decl_t *Decl, mlc_function_t *Origin) {
	if (Function == Origin) return Decl->Index;
	mlc_upvalue_t **UpValueSlot = &Function->UpValues;
	int Index = 0;
	while (UpValueSlot[0]) {
		if (UpValueSlot[0]->Decl == Decl) return ~Index;
		UpValueSlot = &UpValueSlot[0]->Next;
		++Index;
	}
	mlc_upvalue_t *UpValue = new(mlc_upvalue_t);
	UpValue->Decl = Decl;
	UpValue->Index = ml_upvalue_find(Function->Up, Decl, Origin);
	UpValueSlot[0] = UpValue;
	return ~Index;
}

static mlc_compiled_t ml_ident_expr_compile(mlc_function_t *Function, mlc_ident_expr_t *Expr, SHA256_CTX *HashContext) {
	for (mlc_function_t *UpFunction = Function; UpFunction; UpFunction = UpFunction->Up) {
		for (mlc_decl_t *Decl = UpFunction->Decls; Decl; Decl = Decl->Next) {
			if (!strcmp(Decl->Ident, Expr->Ident)) {
				int Index = ml_upvalue_find(Function, Decl, UpFunction);
				sha256_update(HashContext, (void *)&Index, sizeof(Index));
				ML_COMPILE_HASH
				ml_inst_t *LocalInst = ml_inst_new(2, Expr->Source, mli_local_run);
				LocalInst->Params[1].Index = Index;
				if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
				return (mlc_compiled_t){LocalInst, LocalInst};
			}
		}
	}
	sha256_update(HashContext, (BYTE *)Expr->Ident, strlen(Expr->Ident));
	ML_COMPILE_HASH
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, mli_push_run);
	ValueInst->Params[1].Value = (Function->GlobalGet)(Function->Globals, Expr->Ident);
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

struct mlc_value_expr_t {
	MLC_EXPR_FIELDS(value);
	ml_value_t *Value;
};

static mlc_compiled_t ml_value_expr_compile(mlc_function_t *Function, mlc_value_expr_t *Expr, SHA256_CTX *HashContext) {
	long ValueHash = ml_hash(Expr->Value);
	sha256_update(HashContext, (void *)&ValueHash, sizeof(ValueHash));
	ML_COMPILE_HASH
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, mli_push_run);
	ValueInst->Params[1].Value = Expr->Value;
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

const char *MLTokens[] = {
	"", // MLT_NONE,
	"<end of line>", // MLT_EOL,
	"<end of input>", // MLT_EOI,
	"if", // MLT_IF,
	"then", // MLT_THEN,
	"elseif", // MLT_ELSEIF,
	"else", // MLT_ELSE,
	"end", // MLT_END,
	"loop", // MLT_LOOP,
	"while", // MLT_WHILE,
	"until", // MLT_UNTIL,
	"exit", // MLT_EXIT,
	"next", // MLT_NEXT,
	"for", // MLT_FOR,
	"all", // MLT_ALL,
	"in", // MLT_IN,
	"is", // MLT_IS,
	"fun", // MLT_FUN,
	"return", // MLT_RETURN,
	"with", // MLT_WITH,
	"do", // MLT_DO,
	"on", // MLT_ON,
	"nil", // MLT_NIL,
	"and", // MLT_AND,
	"or", // MLT_OR,
	"not", // MLT_NOT,
	"old", // MLT_OLD,
	"def", // MLT_DEF,
	"var", // MLT_VAR,
	"<identifier>", // MLT_IDENT,
	"(", // MLT_LEFT_PAREN,
	")", // MLT_RIGHT_PAREN,
	"[", // MLT_LEFT_SQUARE,
	"]", // MLT_RIGHT_SQUARE,
	"{", // MLT_LEFT_BRACE,
	"}", // MLT_RIGHT_BRACE,
	";", // MLT_SEMICOLON,
	":", // MLT_COLON,
	",", // MLT_COMMA,
	":=", // MLT_ASSIGN,
	"::", // MLT_SYMBOL,
	"<value>", // MLT_VALUE,
	"<expr>", // MLT_EXPR,
	"<operator>", // MLT_OPERATOR
	"<method>" // MLT_METHOD
};

mlc_scanner_t *ml_scanner(const char *SourceName, void *Data, const char *(*read)(void *)) {
	mlc_scanner_t *Scanner = new(mlc_scanner_t);
	Scanner->Token = MLT_NONE;
	Scanner->Next = "";
	Scanner->Source.Name = SourceName;
	Scanner->Source.Line = 0;
	Scanner->Data = Data;
	Scanner->read = read;
	return Scanner;
}

typedef enum {EXPR_SIMPLE, EXPR_AND, EXPR_OR, EXPR_DEFAULT} ml_expr_level_t;

static mlc_expr_t *ml_accept_term(mlc_scanner_t *Scanner);
static mlc_expr_t *ml_parse_expression(mlc_scanner_t *Scanner, ml_expr_level_t Level);
static mlc_expr_t *ml_accept_expression(mlc_scanner_t *Scanner, ml_expr_level_t Level);

static mlc_expr_t *ml_accept_string(mlc_scanner_t *Scanner) {
	char Char = Scanner->Next[0];
	if (Char == '\'') {
		++Scanner->Next;
		return NULL;
	}
	int Length = 0;
	const char *End = Scanner->Next;
	while (End[0] && End[0] != '\'' && End[0] != '{') {
		if (End[0] == '\\') {
			if (!*++End) {
				Scanner->Error = ml_error("ParseError", "end of line while parsing string");
				ml_error_trace_add(Scanner->Error, Scanner->Source);
				longjmp(Scanner->OnError, 1);
			}
		}
		++Length;
		++End;
	}
	mlc_expr_t *Expr = NULL;
	if (Length > 0) {
		char *String = snew(Length + 1), *D = String;
		for (const char *S = Scanner->Next; S < End; ++S) {
			if (*S == '\\') {
				++S;
				switch (*S) {
				case 'r': *D++ = '\r'; break;
				case 'n': *D++ = '\n'; break;
				case 't': *D++ = '\t'; break;
				case 'e': *D++ = '\e'; break;
				case '\'': *D++ = '\''; break;
				case '\"': *D++ = '\"'; break;
				case '\\': *D++ = '\\'; break;
				case '{': *D++ = '{'; break;
				}
			} else {
				*D++ = *S;
			}
		}
		*D = 0;
		mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
		ValueExpr->compile = ml_value_expr_compile;
		ValueExpr->Source = Scanner->Source;
		ValueExpr->Value = ml_string(String, Length);
		Expr = (mlc_expr_t *)ValueExpr;
	}
	Scanner->Next = End + 1;
	if (!End[0]) {
		Scanner->Next = (Scanner->read)(Scanner->Data);
		++Scanner->Source.Line;
		if (!Scanner->Next) {
			Scanner->Error = ml_error("ParseError", "end of input while parsing string");
			ml_error_trace_add(Scanner->Error, Scanner->Source);
			longjmp(Scanner->OnError, 1);
		}
		mlc_expr_t *Next = ml_accept_string(Scanner);
		if (Expr) {
			Expr->Next = Next;
		} else {
			Expr = Next;
		}
	} else if (End[0] == '{') {
		mlc_expr_t *Embedded = ml_accept_expression(Scanner, EXPR_DEFAULT);
		ml_accept(Scanner, MLT_RIGHT_BRACE);
		Embedded->Next = ml_accept_string(Scanner);
		if (Expr) {
			Expr->Next = Embedded;
		} else {
			Expr = Embedded;
		}
	}
	return Expr;
}

static ml_function_t StringNew[1] = {{MLFunctionT, ml_string_new, NULL}};
static ml_function_t ListNew[1] = {{MLFunctionT, ml_list_new, NULL}};
static ml_function_t TreeNew[1] = {{MLFunctionT, ml_tree_new, NULL}};

static int ml_parse(mlc_scanner_t *Scanner, ml_token_t Token) {
	static int OperatorChars[] = {
		['!'] = 1,
		['@'] = 1,
		['#'] = 1,
		['$'] = 1,
		['%'] = 1,
		['^'] = 1,
		['&'] = 1,
		['*'] = 1,
		['-'] = 1,
		['+'] = 1,
		['='] = 1,
		['|'] = 1,
		['\\'] = 1,
		['~'] = 1,
		['`'] = 1,
		['/'] = 1,
		['?'] = 1,
		['<'] = 1,
		['>'] = 1,
		['.'] = 1
	};
	if (Scanner->Token == MLT_NONE) for (;;) {
		char Char = Scanner->Next[0];
		if (!Char) {
			Scanner->Next = (Scanner->read)(Scanner->Data);
			++Scanner->Source.Line;
			if (Scanner->Next) continue;
			Scanner->Token = MLT_EOI;
			goto done;
		}
		if (Char == '\n') {
			++Scanner->Next;
			Scanner->Token = MLT_EOL;
			goto done;
		}
		if (Char <= ' ') {
			++Scanner->Next;
			continue;
		}
		if (Char == 'r' && Scanner->Next[1] == '\"') {
			Scanner->Next += 2;
			int Length = 0;
			const char *End = Scanner->Next;
			while (End[0] != '\"') {
				if (!End[0]) {
					Scanner->Error = ml_error("ParseError", "end of input while parsing string");
					ml_error_trace_add(Scanner->Error, Scanner->Source);
					longjmp(Scanner->OnError, 1);
				}
				if (End[0] == '\\') {
					++Length;
					++End;
				}
				++Length;
				++End;
			}
			char *Pattern = snew(Length + 1), *D = Pattern;
			for (const char *S = Scanner->Next; S < End; ++S) {
				if (*S == '\\') {
					++S;
					switch (*S) {
					case 'r': *D++ = '\r'; break;
					case 'n': *D++ = '\n'; break;
					case 't': *D++ = '\t'; break;
					case 'e': *D++ = '\e'; break;
					default: *D++ = '\\'; *D++ = *S; break;
					}
				} else {
					*D++ = *S;
				}
			}
			*D = 0;
			Scanner->Value = ml_regex(Pattern);
			Scanner->Token = MLT_VALUE;
			Scanner->Next = End + 1;
			goto done;
		} else if (isalpha(Char) || Char == '_') {
			const char *End = Scanner->Next + 1;
			for (Char = End[0]; isalnum(Char) || Char == '_'; Char = *++End);
			int Length = End - Scanner->Next;
			for (ml_token_t T = MLT_IF; T <= MLT_VAR; ++T) {
				const char *P = Scanner->Next;
				const char *C = MLTokens[T];
				while (*C && *C == *P) {++C; ++P;}
				if (!*C && P == End) {
					Scanner->Token = T;
					Scanner->Next = End;
					goto done;
				}
			}
			char *Ident = snew(Length + 1);
			memcpy(Ident, Scanner->Next, Length);
			Ident[Length] = 0;
			Scanner->Ident = Ident;
			Scanner->Token = MLT_IDENT;
			Scanner->Next = End;
			goto done;
		}
		if (isdigit(Char) || (Char == '-' && isdigit(Scanner->Next[1]))) {
			char *End;
			double Double = strtod(Scanner->Next, &End);
			for (const char *P = Scanner->Next; P < End; ++P) {
				if (P[0] == '.' || P[0] == 'e' || P[0] == 'E') {
					Scanner->Value = ml_real(Double);
					Scanner->Token = MLT_VALUE;
					Scanner->Next = End;
					goto done;
				}
			}
			long Integer = strtol(Scanner->Next, &End, 10);
			Scanner->Value = ml_integer(Integer);
			Scanner->Token = MLT_VALUE;
			Scanner->Next = End;
			goto done;
		}
		if (Char == '\'') {
			++Scanner->Next;
			mlc_expr_t *Child = ml_accept_string(Scanner);
			if (!Child) {
				Scanner->Token = MLT_VALUE;
				Scanner->Value = ml_string("", 0);
			} else if (!Child->Next && Child->compile == (void *)ml_value_expr_compile && ((mlc_value_expr_t *)Child)->Value->Type == MLStringT) {
				Scanner->Token = MLT_VALUE;
				Scanner->Value = ((mlc_value_expr_t *)Child)->Value;
			} else {
				mlc_const_call_expr_t *CallExpr = new(mlc_const_call_expr_t);
				CallExpr->compile = ml_const_call_expr_compile;
				CallExpr->Source = Scanner->Source;
				CallExpr->Value = (ml_value_t *)StringNew;
				CallExpr->Child = Child;
				Scanner->Token = MLT_EXPR;
				Scanner->Expr = (mlc_expr_t *)CallExpr;
			}
			goto done;
		}
		if (Char == '\"') {
			++Scanner->Next;
			int Length = 0;
			const char *End = Scanner->Next;
			while (End[0] != '\"') {
				if (!End[0]) {
					Scanner->Error = ml_error("ParseError", "end of input while parsing string");
					ml_error_trace_add(Scanner->Error, Scanner->Source);
					longjmp(Scanner->OnError, 1);
				}
				if (End[0] == '\\') ++End;
				++Length;
				++End;
			}
			char *String = snew(Length + 1), *D = String;
			for (const char *S = Scanner->Next; S < End; ++S) {
				if (*S == '\\') {
					++S;
					switch (*S) {
					case 'r': *D++ = '\r'; break;
					case 'n': *D++ = '\n'; break;
					case 't': *D++ = '\t'; break;
					case 'e': *D++ = '\e'; break;
					case '\'': *D++ = '\''; break;
					case '\"': *D++ = '\"'; break;
					case '\\': *D++ = '\\'; break;
					}
				} else {
					*D++ = *S;
				}
			}
			*D = 0;
			Scanner->Value = ml_string(String, Length);
			Scanner->Token = MLT_VALUE;
			Scanner->Next = End + 1;
			goto done;
		}
		if (Char == ':') {
			if (Scanner->Next[1] == '=') {
				Scanner->Token = MLT_ASSIGN;
				Scanner->Next += 2;
				goto done;
			} else if (isalpha(Scanner->Next[1]) || Scanner->Next[1] == '_') {
				const char *End = Scanner->Next + 1;
				for (Char = End[0]; isalnum(Char) || Char == '_'; Char = *++End);
				int Length = End - Scanner->Next - 1;
				char *Ident = snew(Length + 1);
				memcpy(Ident, Scanner->Next + 1, Length);
				Ident[Length] = 0;
				Scanner->Ident = Ident;
				Scanner->Token = MLT_METHOD;
				Scanner->Next = End;
				goto done;
			} else if (Scanner->Next[1] == ':') {
				const char *End = Scanner->Next + 2;
				for (Char = End[0]; OperatorChars[(int)Char]; Char = *++End);
				int Length = End - Scanner->Next - 2;
				char *Operator = snew(Length + 1);
				strncpy(Operator, Scanner->Next + 2, Length);
				Operator[Length] = 0;
				Scanner->Ident = Operator;
				Scanner->Token = MLT_METHOD;
				Scanner->Next = End;
				goto done;
			}
		}
		if (Char == '-' && Scanner->Next[1] == '-') {
			Scanner->Next = "";
			continue;
		}
		for (ml_token_t T = MLT_LEFT_PAREN; T <= MLT_COMMA; ++T) {
			if (Char == MLTokens[T][0]) {
				Scanner->Token = T;
				++Scanner->Next;
				goto done;
			}
		}
		if (OperatorChars[(int)Char]) {
			const char *End = Scanner->Next;
			for (Char = End[0]; OperatorChars[(int)Char]; Char = *++End);
			int Length = End - Scanner->Next;
			char *Operator = snew(Length + 1);
			strncpy(Operator, Scanner->Next, Length);
			Operator[Length] = 0;
			Scanner->Ident = Operator;
			Scanner->Token = MLT_OPERATOR;
			Scanner->Next = End;
			goto done;
		}
		Scanner->Error = ml_error("ParseError", "unexpected character <%c>", Char);
		ml_error_trace_add(Scanner->Error, Scanner->Source);
		longjmp(Scanner->OnError, 1);
	}
	done:
	if (Scanner->Token == Token) {
		Scanner->Token = MLT_NONE;
		return 1;
	} else {
		return 0;
	}
}

void ml_accept(mlc_scanner_t *Scanner, ml_token_t Token) {
	while (ml_parse(Scanner, MLT_EOL));
	if (ml_parse(Scanner, Token)) return;
	if (Scanner->Token == MLT_IDENT) {
		Scanner->Error = ml_error("ParseError", "expected %s not %s (%s)", MLTokens[Token], MLTokens[Scanner->Token], Scanner->Ident);
	} else {
		Scanner->Error = ml_error("ParseError", "expected %s not %s", MLTokens[Token], MLTokens[Scanner->Token]);
	}
	ml_error_trace_add(Scanner->Error, Scanner->Source);
	longjmp(Scanner->OnError, 1);
}

static mlc_expr_t *ml_parse_term(mlc_scanner_t *Scanner) {
	if (ml_parse(Scanner, MLT_DO)) {
		mlc_expr_t *Expr = ml_accept_block(Scanner);
		ml_accept(Scanner, MLT_END);
		return Expr;
	} else if (ml_parse(Scanner, MLT_IF)) {
		mlc_if_expr_t *IfExpr = new(mlc_if_expr_t);
		IfExpr->compile = ml_if_expr_compile;
		IfExpr->Source = Scanner->Source;
		mlc_if_case_t **CaseSlot = &IfExpr->Cases;
		do {
			mlc_if_case_t *Case = CaseSlot[0] = new(mlc_if_case_t);
			CaseSlot = &Case->Next;
			Case->Source = Scanner->Source;
			Case->Condition = ml_accept_expression(Scanner, EXPR_DEFAULT);
			ml_accept(Scanner, MLT_THEN);
			Case->Body = ml_accept_block(Scanner);
		} while (ml_parse(Scanner, MLT_ELSEIF));
		if (ml_parse(Scanner, MLT_ELSE)) IfExpr->Else = ml_accept_block(Scanner);
		ml_accept(Scanner, MLT_END);
		return (mlc_expr_t *)IfExpr;
	} else if (ml_parse(Scanner, MLT_LOOP)) {
		mlc_parent_expr_t *LoopExpr = new(mlc_parent_expr_t);
		LoopExpr->compile = ml_loop_expr_compile;
		LoopExpr->Source = Scanner->Source;
		LoopExpr->Child = ml_accept_block(Scanner);
		ml_accept(Scanner, MLT_END);
		return (mlc_expr_t *)LoopExpr;
	} else if (ml_parse(Scanner, MLT_FOR)) {
		mlc_decl_expr_t *ForExpr = new(mlc_decl_expr_t);
		ForExpr->compile = ml_for_expr_compile;
		ForExpr->Source = Scanner->Source;
		int Deref = ml_parse(Scanner, MLT_VAR);
		mlc_decl_t *Decl = new(mlc_decl_t);
		ml_accept(Scanner, MLT_IDENT);
		Decl->Ident = Scanner->Ident;
		int HasKey = 0;
		if (ml_parse(Scanner, MLT_COMMA)) {
			ml_accept(Scanner, MLT_IDENT);
			mlc_decl_t *KeyDecl = new(mlc_decl_t);
			KeyDecl->Ident = Scanner->Ident;
			Decl->Next = KeyDecl;
			HasKey = 1;
		}
		ForExpr->Decl = Decl;
		if (ml_parse(Scanner, MLT_ASSIGN)) {
			ForExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
		} else {
			ml_accept(Scanner, MLT_IN);
			mlc_const_call_expr_t *CallExpr = new(mlc_const_call_expr_t);
			CallExpr->compile = ml_const_call_expr_compile;
			CallExpr->Source = Scanner->Source;
			CallExpr->Value = ml_method("values");
			CallExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
			ForExpr->Child = (mlc_expr_t *)CallExpr;
		}
		ml_accept(Scanner, MLT_DO);
		ForExpr->Child->Next = ml_accept_block(Scanner);
		if (Deref) {
			mlc_block_expr_t *Block = (mlc_block_expr_t *)ForExpr->Child->Next;
			char *ValueIdent = snew(strlen(Decl->Ident) + 2);
			ValueIdent[0] = '#';
			strcpy(ValueIdent + 1, Decl->Ident);
			mlc_decl_t *ValueDecl = new(mlc_decl_t);
			mlc_ident_expr_t *ValueIdentExpr = new(mlc_ident_expr_t);
			ValueIdentExpr->compile = ml_ident_expr_compile;
			ValueIdentExpr->Source = Scanner->Source;
			ValueIdentExpr->Ident = ValueDecl->Ident = Decl->Ident;
			mlc_ident_expr_t *OldValueIdentExpr = new(mlc_ident_expr_t);
			OldValueIdentExpr->compile = ml_ident_expr_compile;
			OldValueIdentExpr->Source = Scanner->Source;
			OldValueIdentExpr->Ident = ValueIdent;
			mlc_parent_expr_t *ValueAssignExpr = new(mlc_parent_expr_t);
			ValueAssignExpr->compile = ml_assign_expr_compile;
			ValueAssignExpr->Source = Scanner->Source;
			ValueAssignExpr->Child = (mlc_expr_t *)ValueIdentExpr;
			ValueIdentExpr->Next = (mlc_expr_t *)OldValueIdentExpr;
			Decl->Ident = ValueIdent;
			if (HasKey) {
				char *KeyIdent = snew(strlen(Decl->Next->Ident) + 2);
				KeyIdent[0] = '#';
				strcpy(KeyIdent + 1, Decl->Next->Ident);
				mlc_decl_t *KeyDecl = new(mlc_decl_t);
				mlc_ident_expr_t *KeyIdentExpr = new(mlc_ident_expr_t);
				KeyIdentExpr->compile = ml_ident_expr_compile;
				KeyIdentExpr->Source = Scanner->Source;
				KeyIdentExpr->Ident = KeyDecl->Ident = Decl->Next->Ident;
				mlc_ident_expr_t *OldKeyIdentExpr = new(mlc_ident_expr_t);
				OldKeyIdentExpr->compile = ml_ident_expr_compile;
				OldKeyIdentExpr->Source = Scanner->Source;
				OldKeyIdentExpr->Ident = KeyIdent;
				mlc_parent_expr_t *KeyAssignExpr = new(mlc_parent_expr_t);
				KeyAssignExpr->compile = ml_assign_expr_compile;
				KeyAssignExpr->Source = Scanner->Source;
				KeyAssignExpr->Child = (mlc_expr_t *)KeyIdentExpr;
				KeyIdentExpr->Next = (mlc_expr_t *)OldKeyIdentExpr;
				Decl->Next->Ident = KeyIdent;
				ValueDecl->Next = KeyDecl;
				KeyDecl->Next = Block->Decl;
				ValueAssignExpr->Next = (mlc_expr_t *)KeyAssignExpr;
				KeyAssignExpr->Next = Block->Child;
			} else {
				ValueDecl->Next = Block->Decl;
				ValueAssignExpr->Next = Block->Child;
			}
			Block->Decl = ValueDecl;
			Block->Child = (mlc_expr_t *)ValueAssignExpr;
		}
		if (ml_parse(Scanner, MLT_ELSE)) {
			ForExpr->Child->Next->Next = ml_accept_block(Scanner);
		}
		ml_accept(Scanner, MLT_END);
		return (mlc_expr_t *)ForExpr;
	} else if (ml_parse(Scanner, MLT_ALL)) {
		mlc_parent_expr_t *AllExpr = new(mlc_parent_expr_t);
		AllExpr->compile = ml_all_expr_compile;
		AllExpr->Source = Scanner->Source;
		AllExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
		return (mlc_expr_t *)AllExpr;
	} else if (ml_parse(Scanner, MLT_NOT)) {
		mlc_parent_expr_t *NotExpr = new(mlc_parent_expr_t);
		NotExpr->compile = ml_not_expr_compile;
		NotExpr->Source = Scanner->Source;
		NotExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
		return (mlc_expr_t *)NotExpr;
	} else if (ml_parse(Scanner, MLT_WHILE)) {
		mlc_parent_expr_t *WhileExpr = new(mlc_parent_expr_t);
		WhileExpr->compile = ml_while_expr_compile;
		WhileExpr->Source = Scanner->Source;
		WhileExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
		return (mlc_expr_t *)WhileExpr;
	} else if (ml_parse(Scanner, MLT_UNTIL)) {
		mlc_parent_expr_t *UntilExpr = new(mlc_parent_expr_t);
		UntilExpr->compile = ml_until_expr_compile;
		UntilExpr->Source = Scanner->Source;
		UntilExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
		return (mlc_expr_t *)UntilExpr;
	} else if (ml_parse(Scanner, MLT_EXIT)) {
		mlc_parent_expr_t *ExitExpr = new(mlc_parent_expr_t);
		ExitExpr->compile = ml_exit_expr_compile;
		ExitExpr->Source = Scanner->Source;
		ExitExpr->Child = ml_parse_expression(Scanner, EXPR_DEFAULT);
		return (mlc_expr_t *)ExitExpr;
	} else if (ml_parse(Scanner, MLT_NEXT)) {
		mlc_expr_t *NextExpr = new(mlc_expr_t);
		NextExpr->compile = ml_next_expr_compile;
		NextExpr->Source = Scanner->Source;
		return NextExpr;
	} else if (ml_parse(Scanner, MLT_FUN)) {
		mlc_fun_expr_t *FunExpr = new(mlc_fun_expr_t);
		FunExpr->compile = ml_fun_expr_compile;
		FunExpr->Source = Scanner->Source;
		ml_accept(Scanner, MLT_LEFT_PAREN);
		if (!ml_parse(Scanner, MLT_RIGHT_PAREN)) {
			mlc_decl_t **ParamSlot = &FunExpr->Params;
			do {
				ml_accept(Scanner, MLT_IDENT);
				mlc_decl_t *Param = ParamSlot[0] = new(mlc_decl_t);
				ParamSlot = &Param->Next;
				Param->Ident = Scanner->Ident;
				if (ml_parse(Scanner, MLT_LEFT_SQUARE)) {
					ml_accept(Scanner, MLT_RIGHT_SQUARE);
					Param->Index = 1;
					break;
				}
			} while (ml_parse(Scanner, MLT_COMMA));
			ml_accept(Scanner, MLT_RIGHT_PAREN);
		}
		/*if (ml_parse(Scanner, MLT_DO)) {
			FunExpr->Body = ml_accept_block(Scanner);
			ml_accept(Scanner, MLT_END);
		} else {*/
			FunExpr->Body = ml_accept_expression(Scanner, EXPR_DEFAULT);
		//}
		return (mlc_expr_t *)FunExpr;
	} else if (ml_parse(Scanner, MLT_RETURN)) {
		mlc_parent_expr_t *ReturnExpr = new(mlc_parent_expr_t);
		ReturnExpr->compile = ml_return_expr_compile;
		ReturnExpr->Source = Scanner->Source;
		ReturnExpr->Child = ml_parse_expression(Scanner, EXPR_DEFAULT);
		return (mlc_expr_t *)ReturnExpr;
	} else if (ml_parse(Scanner, MLT_WITH)) {
		mlc_decl_expr_t *WithExpr = new(mlc_decl_expr_t);
		WithExpr->compile = ml_with_expr_compile;
		WithExpr->Source = Scanner->Source;
		mlc_decl_t **DeclSlot = &WithExpr->Decl;
		mlc_expr_t **ExprSlot = &WithExpr->Child;
		do {
			ml_accept(Scanner, MLT_IDENT);
			mlc_decl_t *Decl = DeclSlot[0] = new(mlc_decl_t);
			DeclSlot = &Decl->Next;
			Decl->Ident = Scanner->Ident;
			ml_accept(Scanner, MLT_ASSIGN);
			mlc_expr_t *Expr = ExprSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
			ExprSlot = &Expr->Next;
		} while (ml_parse(Scanner, MLT_COMMA));
		ml_accept(Scanner, MLT_DO);
		ExprSlot[0] = ml_accept_block(Scanner);
		ml_accept(Scanner, MLT_END);
		return (mlc_expr_t *)WithExpr;
	} else if (ml_parse(Scanner, MLT_IDENT)) {
		mlc_ident_expr_t *IdentExpr = new(mlc_ident_expr_t);
		IdentExpr->compile = ml_ident_expr_compile;
		IdentExpr->Source = Scanner->Source;
		IdentExpr->Ident = Scanner->Ident;
		return (mlc_expr_t *)IdentExpr;
	} else if (ml_parse(Scanner, MLT_VALUE)) {
		mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
		ValueExpr->compile = ml_value_expr_compile;
		ValueExpr->Source = Scanner->Source;
		ValueExpr->Value = Scanner->Value;
		return (mlc_expr_t *)ValueExpr;
	} else if (ml_parse(Scanner, MLT_EXPR)) {
		return Scanner->Expr;
	} else if (ml_parse(Scanner, MLT_NIL)) {
		mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
		ValueExpr->compile = ml_value_expr_compile;
		ValueExpr->Source = Scanner->Source;
		ValueExpr->Value = MLNil;
		return (mlc_expr_t *)ValueExpr;
	} else if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
		mlc_expr_t *Expr = ml_accept_expression(Scanner, EXPR_DEFAULT);
		ml_accept(Scanner, MLT_RIGHT_PAREN);
		return Expr;
	} else if (ml_parse(Scanner, MLT_LEFT_SQUARE)) {
		mlc_const_call_expr_t *CallExpr = new(mlc_const_call_expr_t);
		CallExpr->compile = ml_const_call_expr_compile;
		CallExpr->Source = Scanner->Source;
		CallExpr->Value = (ml_value_t *)ListNew;
		mlc_expr_t **ArgsSlot = &CallExpr->Child;
		if (!ml_parse(Scanner, MLT_RIGHT_SQUARE)) {
			do {
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
			} while (ml_parse(Scanner, MLT_COMMA));
			ml_accept(Scanner, MLT_RIGHT_SQUARE);
		}
		return (mlc_expr_t *)CallExpr;
	} else if (ml_parse(Scanner, MLT_LEFT_BRACE)) {
		mlc_const_call_expr_t *CallExpr = new(mlc_const_call_expr_t);
		CallExpr->compile = ml_const_call_expr_compile;
		CallExpr->Source = Scanner->Source;
		CallExpr->Value = (ml_value_t *)TreeNew;
		mlc_expr_t **ArgsSlot = &CallExpr->Child;
		if (!ml_parse(Scanner, MLT_RIGHT_BRACE)) {
			do {
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
				if (ml_parse(Scanner, MLT_IS)) {
					mlc_expr_t *ArgExpr = ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
					ArgsSlot = &ArgExpr->Next;
				} else {
					mlc_value_expr_t *ArgExpr = new(mlc_value_expr_t);
					ArgExpr->compile = ml_value_expr_compile;
					ArgExpr->Source = Scanner->Source;
					ArgExpr->Value = MLNil;
					ArgsSlot[0] = (mlc_expr_t *)ArgExpr;
					ArgsSlot = &ArgExpr->Next;
				}
			} while (ml_parse(Scanner, MLT_COMMA));
			ml_accept(Scanner, MLT_RIGHT_BRACE);
		}
		return (mlc_expr_t *)CallExpr;
	} else if (ml_parse(Scanner, MLT_OLD)) {
		mlc_expr_t *OldExpr = new(mlc_expr_t);
		OldExpr->compile = ml_old_expr_compile;
		OldExpr->Source = Scanner->Source;
		return OldExpr;
	} else if (ml_parse(Scanner, MLT_OPERATOR)) {
		mlc_const_call_expr_t *CallExpr = new(mlc_const_call_expr_t);
		CallExpr->compile = ml_const_call_expr_compile;
		CallExpr->Source = Scanner->Source;
		CallExpr->Value = (ml_value_t *)ml_method(Scanner->Ident);
		CallExpr->Child = ml_accept_term(Scanner);
		return (mlc_expr_t *)CallExpr;
	} else if (ml_parse(Scanner, MLT_METHOD)) {
		mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
		ValueExpr->compile = ml_value_expr_compile;
		ValueExpr->Source = Scanner->Source;
		ValueExpr->Value = (ml_value_t *)ml_method(Scanner->Ident);
		return (mlc_expr_t *)ValueExpr;
	}
	return NULL;
}

static mlc_expr_t *ml_accept_term(mlc_scanner_t *Scanner) {
	while (ml_parse(Scanner, MLT_EOL));
	mlc_expr_t *Expr = ml_parse_term(Scanner);
	if (Expr) return Expr;
	Scanner->Error = ml_error("ParseError", "expected <term> not %s", MLTokens[Scanner->Token]);
	ml_error_trace_add(Scanner->Error, Scanner->Source);
	longjmp(Scanner->OnError, 1);
}

static void ml_accept_arguments(mlc_scanner_t *Scanner, mlc_expr_t **ArgsSlot) {
	if (!ml_parse(Scanner, MLT_RIGHT_PAREN)) {
		if (ml_parse(Scanner, MLT_SEMICOLON)) goto has_params;
		do {
			mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
			ArgsSlot = &Arg->Next;
		} while (ml_parse(Scanner, MLT_COMMA));
		if (ml_parse(Scanner, MLT_SEMICOLON)) has_params: {
			mlc_decl_t *Params = 0;
			mlc_decl_t **ParamSlot = &Params;
			if (!ml_parse(Scanner, MLT_RIGHT_PAREN)) {
				do {
					ml_accept(Scanner, MLT_IDENT);
					mlc_decl_t *Param = ParamSlot[0] = new(mlc_decl_t);
					ParamSlot = &Param->Next;
					Param->Ident = Scanner->Ident;
					if (ml_parse(Scanner, MLT_LEFT_SQUARE)) {
						ml_accept(Scanner, MLT_RIGHT_SQUARE);
						Param->Index = 1;
						break;
					}
				} while (ml_parse(Scanner, MLT_COMMA));
				ml_accept(Scanner, MLT_RIGHT_PAREN);
			}
			mlc_fun_expr_t *FunExpr = new(mlc_fun_expr_t);
			FunExpr->compile = ml_fun_expr_compile;
			FunExpr->Source = Scanner->Source;
			FunExpr->Params = Params;
			FunExpr->Body = ml_accept_expression(Scanner, EXPR_DEFAULT);
			ArgsSlot[0] = (mlc_expr_t *)FunExpr;
		} else {
			ml_accept(Scanner, MLT_RIGHT_PAREN);
		}
	}
}

static mlc_expr_t *ml_parse_factor(mlc_scanner_t *Scanner) {
	mlc_expr_t *Expr = ml_parse_term(Scanner);
	if (!Expr) return NULL;
	for (;;) {
		if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
			mlc_parent_expr_t *CallExpr = new(mlc_parent_expr_t);
			CallExpr->compile = ml_call_expr_compile;
			CallExpr->Source = Scanner->Source;
			CallExpr->Child = Expr;
			ml_accept_arguments(Scanner, &Expr->Next);
			Expr = (mlc_expr_t *)CallExpr;
		} else if (ml_parse(Scanner, MLT_LEFT_SQUARE)) {
			mlc_const_call_expr_t *IndexExpr = new(mlc_const_call_expr_t);
			IndexExpr->compile = ml_const_call_expr_compile;
			IndexExpr->Value = ml_method("[]");
			IndexExpr->Source = Scanner->Source;
			IndexExpr->Child = Expr;
			mlc_expr_t **ArgsSlot = &Expr->Next;
			if (!ml_parse(Scanner, MLT_RIGHT_SQUARE)) {
				do {
					mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
					ArgsSlot = &Arg->Next;
				} while (ml_parse(Scanner, MLT_COMMA));
				ml_accept(Scanner, MLT_RIGHT_SQUARE);
			}
			Expr = (mlc_expr_t *)IndexExpr;
		} else if (ml_parse(Scanner, MLT_METHOD)) {
			mlc_const_call_expr_t *CallExpr = new(mlc_const_call_expr_t);
			CallExpr->compile = ml_const_call_expr_compile;
			CallExpr->Source = Scanner->Source;
			CallExpr->Value = (ml_value_t *)ml_method(Scanner->Ident);
			CallExpr->Child = Expr;
			if (ml_parse(Scanner, MLT_LEFT_PAREN)) ml_accept_arguments(Scanner, &Expr->Next);
			Expr = (mlc_expr_t *)CallExpr;
		} else {
			return Expr;
		}
	}
	return NULL; // Unreachable
}

static mlc_expr_t *ml_accept_factor(mlc_scanner_t *Scanner) {
	while (ml_parse(Scanner, MLT_EOL));
	mlc_expr_t *Expr = ml_parse_factor(Scanner);
	if (Expr) return Expr;
	Scanner->Error = ml_error("ParseError", "expected <factor> not %s", MLTokens[Scanner->Token]);
	ml_error_trace_add(Scanner->Error, Scanner->Source);
	longjmp(Scanner->OnError, 1);
}

static mlc_expr_t *ml_parse_expression(mlc_scanner_t *Scanner, ml_expr_level_t Level) {
	mlc_expr_t *Expr = ml_parse_factor(Scanner);
	if (!Expr) return NULL;
	for (;;) if (ml_parse(Scanner, MLT_OPERATOR)) {
		mlc_const_call_expr_t *CallExpr = new(mlc_const_call_expr_t);
		CallExpr->compile = ml_const_call_expr_compile;
		CallExpr->Source = Scanner->Source;
		CallExpr->Value = (ml_value_t *)ml_method(Scanner->Ident);
		CallExpr->Child = Expr;
		Expr->Next = ml_accept_factor(Scanner);
		Expr = (mlc_expr_t *)CallExpr;
	} else if (ml_parse(Scanner, MLT_ASSIGN)) {
		mlc_parent_expr_t *AssignExpr = new(mlc_parent_expr_t);
		AssignExpr->compile = ml_assign_expr_compile;
		AssignExpr->Source = Scanner->Source;
		AssignExpr->Child = Expr;
		Expr->Next = ml_accept_expression(Scanner, EXPR_DEFAULT);
		Expr = (mlc_expr_t *)AssignExpr;
	} else {
		break;
	}
	if (Level >= EXPR_AND && ml_parse(Scanner, MLT_AND)) {
		mlc_parent_expr_t *AndExpr = new(mlc_parent_expr_t);
		AndExpr->compile = ml_and_expr_compile;
		AndExpr->Source = Scanner->Source;
		mlc_expr_t *LastChild = AndExpr->Child = Expr;
		do {
			LastChild = LastChild->Next = ml_accept_expression(Scanner, EXPR_SIMPLE);
		} while (ml_parse(Scanner, MLT_AND));
		Expr = (mlc_expr_t *)AndExpr;
	}
	if (Level >= EXPR_OR && ml_parse(Scanner, MLT_OR)) {
		mlc_parent_expr_t *OrExpr = new(mlc_parent_expr_t);
		OrExpr->compile = ml_or_expr_compile;
		OrExpr->Source = Scanner->Source;
		mlc_expr_t *LastChild = OrExpr->Child = Expr;
		do {
			LastChild = LastChild->Next = ml_accept_expression(Scanner, EXPR_AND);
		} while (ml_parse(Scanner, MLT_OR));
		Expr = (mlc_expr_t *)OrExpr;
	}
	return Expr;
}

static mlc_expr_t *ml_accept_expression(mlc_scanner_t *Scanner, ml_expr_level_t Level) {
	while (ml_parse(Scanner, MLT_EOL));
	mlc_expr_t *Expr = ml_parse_expression(Scanner, Level);
	if (Expr) return Expr;
	Scanner->Error = ml_error("ParseError", "expected <expression> not %s", MLTokens[Scanner->Token]);
	ml_error_trace_add(Scanner->Error, Scanner->Source);
	longjmp(Scanner->OnError, 1);
}

mlc_expr_t *ml_accept_block(mlc_scanner_t *Scanner) {
	mlc_block_expr_t *BlockExpr = new(mlc_block_expr_t);
	BlockExpr->compile = ml_block_expr_compile;
	BlockExpr->Source = Scanner->Source;
	mlc_expr_t **ExprSlot = &BlockExpr->Child;
	mlc_decl_t **DeclSlot = &BlockExpr->Decl;
	for (;;) {
		while (ml_parse(Scanner, MLT_EOL));
		if (ml_parse(Scanner, MLT_VAR)) {
			do {
				ml_accept(Scanner, MLT_IDENT);
				mlc_decl_t *Decl = DeclSlot[0] = new(mlc_decl_t);
				Decl->Ident = Scanner->Ident;
				DeclSlot = &Decl->Next;
				if (ml_parse(Scanner, MLT_ASSIGN)) {
					mlc_decl_expr_t *DeclExpr = new(mlc_decl_expr_t);
					DeclExpr->compile = ml_var_expr_compile;
					DeclExpr->Source = Scanner->Source;
					DeclExpr->Decl = Decl;
					DeclExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
					ExprSlot[0] = (mlc_expr_t *)DeclExpr;
					ExprSlot = &DeclExpr->Next;
				}
			} while (ml_parse(Scanner, MLT_COMMA));
		} else if (ml_parse(Scanner, MLT_DEF)) {
			ml_accept(Scanner, MLT_IDENT);
			mlc_decl_t *Decl = new(mlc_decl_t);
			Decl->Ident = Scanner->Ident;
			ml_accept(Scanner, MLT_ASSIGN);
			mlc_decl_expr_t *DeclExpr = new(mlc_decl_expr_t);
			DeclExpr->compile = ml_def_expr_compile;
			DeclExpr->Source = Scanner->Source;
			DeclExpr->Decl = Decl;
			DeclExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
			ExprSlot[0] = (mlc_expr_t *)DeclExpr;
			ExprSlot = &DeclExpr->Next;
		} else if (ml_parse(Scanner, MLT_ON)) {
			if (BlockExpr->CatchDecl) {
				Scanner->Error = ml_error("ParseError", "no more than one error handler allowed in a block");
				ml_error_trace_add(Scanner->Error, Scanner->Source);
				longjmp(Scanner->OnError, 1);
			}
			ml_accept(Scanner, MLT_IDENT);
			mlc_decl_t *Decl = new(mlc_decl_t);
			Decl->Ident = Scanner->Ident;
			BlockExpr->CatchDecl = Decl;
			ml_accept(Scanner, MLT_DO);
			BlockExpr->Catch = ml_accept_block(Scanner);
			//ml_accept(Scanner, MLT_END);
			return (mlc_expr_t *)BlockExpr;
		} else {
			mlc_expr_t *Expr = ml_parse_expression(Scanner, EXPR_DEFAULT);
			if (!Expr) return (mlc_expr_t *)BlockExpr;
			ExprSlot[0] = Expr;
			ExprSlot = &Expr->Next;
		}
		ml_parse(Scanner, MLT_SEMICOLON);
	}
	return NULL; // Unreachable
}

mlc_expr_t *ml_accept_command(mlc_scanner_t *Scanner, stringmap_t *Vars) {
	while (ml_parse(Scanner, MLT_EOL));
	mlc_block_expr_t *BlockExpr = new(mlc_block_expr_t);
	BlockExpr->compile = ml_block_expr_compile;
	BlockExpr->Source = Scanner->Source;
	mlc_expr_t **ExprSlot = &BlockExpr->Child;
	if (ml_parse(Scanner, MLT_EOI)) {
		return (mlc_expr_t *)-1;
	} else if (ml_parse(Scanner, MLT_VAR)) {
		do {
			ml_accept(Scanner, MLT_IDENT);
			const char *Ident = Scanner->Ident;
			ml_value_t *Ref = ml_reference(NULL);
			stringmap_insert(Vars, Ident, Ref);
			if (ml_parse(Scanner, MLT_ASSIGN)) {
				mlc_value_expr_t *RefExpr = new(mlc_value_expr_t);
				RefExpr->compile = ml_value_expr_compile;
				RefExpr->Source = Scanner->Source;
				RefExpr->Value = Ref;
				mlc_parent_expr_t *AssignExpr = new(mlc_parent_expr_t);
				AssignExpr->compile = ml_assign_expr_compile;
				AssignExpr->Source = Scanner->Source;
				AssignExpr->Child = (mlc_expr_t *)RefExpr;
				RefExpr->Next = ml_accept_expression(Scanner, EXPR_DEFAULT);
				ExprSlot[0] = (mlc_expr_t *)AssignExpr;
				ExprSlot = &AssignExpr->Next;
			}
		} while (ml_parse(Scanner, MLT_COMMA));
	} else {
		mlc_expr_t *Expr = ExprSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
		ExprSlot = &Expr->Next;
	}
	ml_parse(Scanner, MLT_SEMICOLON);
	return (mlc_expr_t *)BlockExpr;
}

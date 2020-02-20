#include "minilang.h"
#include "ml_macros.h"
#include "ml_compiler.h"
#include "stringmap.h"
#include <gc.h>
#include <ctype.h>
#include "ml_bytecode.h"
#include "ml_runtime.h"
#include <string.h>
#include <stdio.h>

typedef struct mlc_function_t mlc_function_t;
typedef struct mlc_decl_t mlc_decl_t;
typedef struct mlc_loop_t mlc_loop_t;
typedef struct mlc_try_t mlc_try_t;
typedef struct mlc_upvalue_t mlc_upvalue_t;

struct mlc_decl_t {
	mlc_decl_t *Next;
	const char *Ident;
	ml_value_t *Value;
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

typedef struct { ml_inst_t *Start, *Exits; } mlc_compiled_t;

#define MLC_EXPR_FIELDS(TYPE) \
	mlc_compiled_t (*compile)(mlc_function_t *, mlc_ ## TYPE ## _expr_t *, SHA256_CTX *HashContext); \
	mlc_expr_t *Next; \
	ml_source_t Source

struct mlc_expr_t {
	mlc_compiled_t (*compile)(mlc_function_t *, mlc_expr_t *, SHA256_CTX *HashContext);
	mlc_expr_t *Next;
	ml_source_t Source;
};

static inline ml_inst_t *ml_inst_new(int N, ml_source_t Source, ml_opcode_t Opcode) {
	ml_inst_t *Inst = xnew(ml_inst_t, N, ml_param_t);
	Inst->Source = Source;
	Inst->Opcode = Opcode;
	return Inst;
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define ML_COMPILE_HASH sha256_update(HashContext, (unsigned char *)__FILE__ TOSTRING(__LINE__), strlen(__FILE__ TOSTRING(__LINE__)));

struct mlc_function_t {
	mlc_context_t *Context;
	ml_inst_t *ReturnInst;
	mlc_function_t *Up;
	mlc_decl_t *Decls;
	mlc_loop_t *Loop;
	mlc_try_t *Try;
	mlc_upvalue_t *UpValues;
	int Top, Size, Self;
};

static inline void mlc_inc_top(mlc_function_t *Function) {
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
}

static inline mlc_compiled_t mlc_compile(mlc_function_t *Function, mlc_expr_t *Expr, SHA256_CTX *HashContext) {
	return Expr->compile(Function, Expr, HashContext);
}

static inline void mlc_connect(ml_inst_t *Exits, ml_inst_t *Start) {
	for (ml_inst_t *Exit = Exits; Exit;) {
		if (Exit->Opcode == MLI_RETURN) return;
		ml_inst_t *NextExit = Exit->Params[0].Inst;
		Exit->Params[0].Inst = Start;
		Exit = NextExit;
	}
}

ml_value_t *ml_compile(mlc_expr_t *Expr, const char **Parameters, mlc_context_t *Context) {
	mlc_function_t Function[1];
	memset(Function, 0, sizeof(mlc_function_t));
	Function->Context = Context;
	Function->ReturnInst = ml_inst_new(0, Expr->Source, MLI_RETURN);
	SHA256_CTX HashContext[1];
	sha256_init(HashContext);
	int NumParams = 0;
	if (Parameters) {
		mlc_decl_t **ParamSlot = &Function->Decls;
		for (const char **P = Parameters; P[0]; ++P) {
			mlc_decl_t *Param = new(mlc_decl_t);
			Param->Ident = P[0];
			Param->Index = Function->Top++;
			ParamSlot[0] = Param;
			ParamSlot = &Param->Next;
		}
		NumParams = Function->Top;
		Function->Size = Function->Top + 1;
	}
	mlc_compiled_t Compiled = mlc_compile(Function, Expr, HashContext);
	mlc_connect(Compiled.Exits, Function->ReturnInst);
	ml_closure_t *Closure = new(ml_closure_t);
	ml_closure_info_t *Info = Closure->Info = new(ml_closure_info_t);
	Closure->Type = MLClosureT;
	Info->Entry = Compiled.Start;
	Info->Return = Function->ReturnInst;
	Info->FrameSize = Function->Size;
	Info->NumParams = NumParams;
	sha256_final(HashContext, Info->Hash);
	return (ml_value_t *)Closure;
}

ml_value_t *ml_expr_evaluate(mlc_expr_t *Expr, mlc_function_t *Function) {
	mlc_function_t SubFunction[1];
	memset(SubFunction, 0, sizeof(SubFunction));
	SubFunction->Context = Function->Context;
	SubFunction->ReturnInst = ml_inst_new(0, Expr->Source, MLI_RETURN);
	SubFunction->Up = Function;
	SubFunction->Size = 1;
	SHA256_CTX SubHashContext[1];
	sha256_init(SubHashContext);
	mlc_compiled_t Compiled = mlc_compile(SubFunction, Expr, SubHashContext);
	mlc_connect(Compiled.Exits, SubFunction->ReturnInst);
	if (SubFunction->UpValues) {
		ml_value_t *Error = ml_error("EvalError", "Use of non-constant value in constant expression");
		ml_error_trace_add(Error, Expr->Source);
		Function->Context->Error = Error;
		longjmp(Function->Context->OnError, 1);
	}
	ml_closure_t *Closure = new(ml_closure_t);
	ml_closure_info_t *Info = Closure->Info = new(ml_closure_info_t);
	Closure->Type = MLClosureT;
	Info->Entry = Compiled.Start;
	Info->Return = SubFunction->ReturnInst;
	Info->FrameSize = SubFunction->Size;
	Info->NumParams = 0;
	if (MLDebugClosures) ml_closure_debug((ml_value_t *)Closure);
	ml_value_t *Result = ml_call((ml_value_t *)Closure, 0, NULL);
	Result = Result->Type->deref(Result);
	if (Result->Type == MLErrorT) {
		ml_error_trace_add(Result, Expr->Source);
		Function->Context->Error = Result;
		longjmp(Function->Context->OnError, 1);
	}
	return Result;
}

typedef struct mlc_if_expr_t mlc_if_expr_t;
typedef struct mlc_if_case_t mlc_if_case_t;
typedef struct mlc_when_expr_t mlc_when_expr_t;
typedef struct mlc_parent_expr_t mlc_parent_expr_t;
typedef struct mlc_fun_expr_t mlc_fun_expr_t;
typedef struct mlc_decl_expr_t mlc_decl_expr_t;
typedef struct mlc_dot_expr_t mlc_dot_expr_t;
typedef struct mlc_value_expr_t mlc_value_expr_t;
typedef struct mlc_ident_expr_t mlc_ident_expr_t;
typedef struct mlc_parent_value_expr_t mlc_parent_value_expr_t;
typedef struct mlc_block_expr_t mlc_block_expr_t;

struct mlc_if_case_t {
	mlc_if_case_t *Next;
	ml_source_t Source;
	mlc_expr_t *Condition;
	mlc_expr_t *Body;
	mlc_decl_t *Decl;
};

struct mlc_if_expr_t {
	MLC_EXPR_FIELDS(if);
	mlc_if_case_t *Cases;
	mlc_expr_t *Else;
};

static mlc_compiled_t ml_if_expr_compile(mlc_function_t *Function, mlc_if_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	mlc_decl_t *OldDecls = Function->Decls;
	mlc_if_case_t *Case = Expr->Cases;
	mlc_compiled_t Compiled = mlc_compile(Function, Case->Condition, HashContext);
	ml_inst_t *IfInst = ml_inst_new(2, Expr->Source, MLI_IF);
	if (Case->Decl) {
		ML_COMPILE_HASH
		IfInst->Opcode = Case->Decl->Index ? MLI_IF_VAR : MLI_IF_LET;
		mlc_inc_top(Function);
		Case->Decl->Index = Function->Top - 1;
		Case->Decl->Next = Function->Decls;
		Function->Decls = Case->Decl;
	}
	mlc_compiled_t BodyCompiled = mlc_compile(Function, Case->Body, HashContext);
	if (Case->Decl) {
		ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, MLI_EXIT);
		ExitInst->Params[1].Count = 1;
		mlc_connect(BodyCompiled.Exits, ExitInst);
		BodyCompiled.Exits = ExitInst;
		Function->Decls = OldDecls;
		--Function->Top;
	}
	mlc_connect(Compiled.Exits, IfInst);
	IfInst->Params[1].Inst = BodyCompiled.Start;
	Compiled.Exits = BodyCompiled.Exits;
	while ((Case = Case->Next)) {
		ML_COMPILE_HASH
		mlc_compiled_t ConditionCompiled = mlc_compile(Function, Case->Condition, HashContext);
		IfInst->Params[0].Inst = ConditionCompiled.Start;
		IfInst = ml_inst_new(2, Case->Source, MLI_IF);
		if (Case->Decl) {
			ML_COMPILE_HASH
			IfInst->Opcode = Case->Decl->Index ? MLI_IF_VAR : MLI_IF_LET;
			mlc_inc_top(Function);
			Case->Decl->Index = Function->Top - 1;
			Function->Decls = Case->Decl;
		}
		BodyCompiled = mlc_compile(Function, Case->Body, HashContext);
		if (Case->Decl) {
			ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, MLI_EXIT);
			ExitInst->Params[1].Count = 1;
			mlc_connect(BodyCompiled.Exits, ExitInst);
			BodyCompiled.Exits = ExitInst;
			Function->Decls = OldDecls;
			--Function->Top;
		}
		ml_inst_t **Slot = &Compiled.Exits;
		while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
		Slot[0] = BodyCompiled.Exits;
		mlc_connect(ConditionCompiled.Exits, IfInst);
		IfInst->Params[1].Inst = BodyCompiled.Start;
	}
	if (Expr->Else) {
		ML_COMPILE_HASH
		mlc_compiled_t BodyCompiled = mlc_compile(Function, Expr->Else, HashContext);
		IfInst->Params[0].Inst = BodyCompiled.Start;
		ml_inst_t **Slot = &Compiled.Exits;
		while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
		Slot[0] = BodyCompiled.Exits;
	} else {
		IfInst->Params[0].Inst = Compiled.Exits;
		Compiled.Exits = IfInst;
	}
	return Compiled;
}

struct mlc_parent_expr_t {
	MLC_EXPR_FIELDS(parent);
	mlc_expr_t *Child;
};

static mlc_compiled_t ml_or_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = mlc_compile(Function, Child, HashContext);
	ml_inst_t *OrInst = ml_inst_new(2, Expr->Source, MLI_ELSE);
	mlc_connect(Compiled.Exits, OrInst);
	Compiled.Exits = OrInst;
	for (Child = Child->Next; Child->Next; Child = Child->Next) {
		ML_COMPILE_HASH
		mlc_compiled_t ChildCompiled = mlc_compile(Function, Child, HashContext);
		OrInst->Params[1].Inst = ChildCompiled.Start;
		OrInst = ml_inst_new(2, Expr->Source, MLI_ELSE);
		mlc_connect(ChildCompiled.Exits, OrInst);
		OrInst->Params[0].Inst = Compiled.Exits;
		Compiled.Exits = OrInst;
	}
	mlc_compiled_t ChildCompiled = mlc_compile(Function, Child, HashContext);
	OrInst->Params[1].Inst = ChildCompiled.Start;
	ml_inst_t **Slot = &Compiled.Exits;
	while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
	Slot[0] = ChildCompiled.Exits;
	return Compiled;
}

static mlc_compiled_t ml_and_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = mlc_compile(Function, Child, HashContext);
	ml_inst_t *IfInst = ml_inst_new(2, Expr->Source, MLI_IF);
	mlc_connect(Compiled.Exits, IfInst);
	Compiled.Exits = IfInst;
	for (Child = Child->Next; Child->Next; Child = Child->Next) {
		ML_COMPILE_HASH
		mlc_compiled_t ChildCompiled = mlc_compile(Function, Child, HashContext);
		IfInst->Params[1].Inst = ChildCompiled.Start;
		IfInst = ml_inst_new(2, Expr->Source, MLI_IF);
		mlc_connect(ChildCompiled.Exits, IfInst);
		IfInst->Params[0].Inst = Compiled.Exits;
		Compiled.Exits = IfInst;
	}
	mlc_compiled_t ChildCompiled = mlc_compile(Function, Child, HashContext);
	IfInst->Params[1].Inst = ChildCompiled.Start;
	ml_inst_t **Slot = &Compiled.Exits;
	while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
	Slot[0] = ChildCompiled.Exits;
	return Compiled;
}

static mlc_compiled_t ml_loop_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	ml_inst_t *LoopInst = ml_inst_new(1, Expr->Source, MLI_LOOP);
	mlc_loop_t Loop = {
		Function->Loop, Function->Try,
		LoopInst, NULL,
		Function->Top, Function->Top
	};
	Function->Loop = &Loop;
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child, HashContext);
	LoopInst->Params[0].Inst = Compiled.Start;
	mlc_connect(Compiled.Exits, LoopInst);
	Function->Loop = Loop.Up;
	Compiled.Exits = Loop.Exits;
	return Compiled;
}

static mlc_compiled_t ml_next_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	if (!Function->Loop) {
		Function->Context->Error = ml_error("CompilerError", "next not in loop");
		ml_error_trace_add(Function->Context->Error, Expr->Source);
		longjmp(Function->Context->OnError, 1);
	}
	ml_inst_t *NextInst = Function->Loop->Next;
	if (Function->Try != Function->Loop->Try) {
		ML_COMPILE_HASH
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, MLI_TRY);
		TryInst->Params[1].Inst = Function->Try ? Function->Try->CatchInst : Function->ReturnInst;
		TryInst->Params[0].Inst = Function->Loop->Next;
		NextInst = TryInst;
	}
	if (Function->Top > Function->Loop->NextTop) {
		ML_COMPILE_HASH
		ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, MLI_EXIT);
		ExitInst->Params[0].Inst = NextInst;
		ExitInst->Params[1].Count = Function->Top - Function->Loop->NextTop;
		NextInst = ExitInst;
	} else {
		ML_COMPILE_HASH
	}
	return (mlc_compiled_t){NextInst, NULL};
}

static mlc_compiled_t ml_exit_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	if (!Function->Loop) {
		Function->Context->Error = ml_error("CompilerError", "exit not in loop");
		ml_error_trace_add(Function->Context->Error, Expr->Source);
		longjmp(Function->Context->OnError, 1);
	}
	mlc_loop_t *Loop = Function->Loop;
	mlc_try_t *Try = Function->Try;
	Function->Loop = Loop->Up;
	Function->Try = Loop->Try;
	mlc_compiled_t Compiled;
	if (Expr->Child) {
		Compiled = mlc_compile(Function, Expr->Child, HashContext);
	} else {
		ml_inst_t *NilInst = ml_inst_new(1, Expr->Source, MLI_NIL);
		Compiled.Start = Compiled.Exits = NilInst;
	}
	if (Function->Try != Try) {
		ML_COMPILE_HASH
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, MLI_TRY);
		TryInst->Params[1].Inst = Function->Try ? Function->Try->CatchInst : Function->ReturnInst;
		TryInst->Params[0].Inst = Compiled.Start;
		Compiled.Start = TryInst;
	}
	Function->Loop = Loop;
	Function->Try = Try;
	if (Function->Top > Function->Loop->ExitTop) {
		ML_COMPILE_HASH
		ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, MLI_EXIT);
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
	ML_COMPILE_HASH
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child, HashContext);
	ml_inst_t *NotInst = ml_inst_new(2, Expr->Source, MLI_IF);
	mlc_connect(Compiled.Exits, NotInst);
	ml_inst_t *NilInst = ml_inst_new(1, Expr->Source, MLI_NIL);
	ml_inst_t *SomeInst = ml_inst_new(1, Expr->Source, MLI_SOME);
	NotInst->Params[0].Inst = SomeInst;
	NotInst->Params[1].Inst = NilInst;
	NilInst->Params[0].Inst = SomeInst;
	Compiled.Exits = NilInst;
	return Compiled;
}

static mlc_compiled_t ml_while_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	if (!Function->Loop) {
		Function->Context->Error = ml_error("CompilerError", "while not in loop");
		ml_error_trace_add(Function->Context->Error, Expr->Source);
		longjmp(Function->Context->OnError, 1);
	}
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child, HashContext);
	ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, MLI_EXIT);
	ExitInst->Params[1].Count = Function->Top - Function->Loop->ExitTop;
	mlc_loop_t *Loop = Function->Loop;
	if (Function->Try != Loop->Try) {
		ML_COMPILE_HASH
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, MLI_TRY);
		TryInst->Params[1].Inst = Loop->Try ? Loop->Try->CatchInst : Function->ReturnInst;
		TryInst->Params[0].Inst = ExitInst;
		ExitInst = TryInst;
	}
	ml_inst_t *WhileInst = ml_inst_new(2, Expr->Source, MLI_ELSE);
	mlc_connect(Compiled.Exits, WhileInst);
	Compiled.Exits = WhileInst;
	WhileInst->Params[1].Inst = ExitInst;
	ExitInst->Params[0].Inst = Function->Loop->Exits;
	Function->Loop->Exits = ExitInst;
	return Compiled;
}

static mlc_compiled_t ml_until_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	if (!Function->Loop) {
		Function->Context->Error = ml_error("CompilerError", "until not in loop");
		ml_error_trace_add(Function->Context->Error, Expr->Source);
		longjmp(Function->Context->OnError, 1);
	}
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child, HashContext);
	ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, MLI_EXIT);
	ExitInst->Params[1].Count = Function->Top - Function->Loop->ExitTop;
	mlc_loop_t *Loop = Function->Loop;
	if (Function->Try != Loop->Try) {
		ML_COMPILE_HASH
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, MLI_TRY);
		TryInst->Params[1].Inst = Loop->Try ? Loop->Try->CatchInst : Function->ReturnInst;
		TryInst->Params[0].Inst = ExitInst;
		ExitInst = TryInst;
	}
	ml_inst_t *UntilInst = ml_inst_new(2, Expr->Source, MLI_IF);
	mlc_connect(Compiled.Exits, UntilInst);
	Compiled.Exits = UntilInst;
	UntilInst->Params[1].Inst = ExitInst;
	ExitInst->Params[0].Inst = Function->Loop->Exits;
	Function->Loop->Exits = ExitInst;
	return Compiled;
}

static mlc_compiled_t ml_return_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	mlc_compiled_t Compiled;
	if (Expr->Child) {
		ML_COMPILE_HASH
		Compiled = mlc_compile(Function, Expr->Child, HashContext);
	} else {
		ML_COMPILE_HASH
		ml_inst_t *NilInst = ml_inst_new(1, Expr->Source, MLI_NIL);
		Compiled.Start = Compiled.Exits = NilInst;
	}
	mlc_connect(Compiled.Exits, Function->ReturnInst);
	Compiled.Exits = NULL;
	return Compiled;
}

static mlc_compiled_t ml_suspend_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	mlc_compiled_t Compiled;
	mlc_expr_t *ValueExpr = Expr->Child;
	if (ValueExpr->Next) {
		ML_COMPILE_HASH
		Compiled = mlc_compile(Function, ValueExpr, HashContext);
		ValueExpr = ValueExpr->Next;
	} else {
		ML_COMPILE_HASH
		ml_inst_t *NilInst = ml_inst_new(1, Expr->Source, MLI_NIL);
		Compiled.Start = Compiled.Exits = NilInst;
	}
	ml_inst_t *KeyPushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	mlc_connect(Compiled.Exits, KeyPushInst);
	mlc_inc_top(Function);
	mlc_compiled_t ValueCompiled = mlc_compile(Function, ValueExpr, HashContext);
	KeyPushInst->Params[0].Inst = ValueCompiled.Start;
	ml_inst_t *ValuePushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	mlc_connect(ValueCompiled.Exits, ValuePushInst);
	ml_inst_t *SuspendInst = ml_inst_new(1, Expr->Source, MLI_SUSPEND);
	ValuePushInst->Params[0].Inst = SuspendInst;
	ml_inst_t *ResumeInst = ml_inst_new(1, Expr->Source, MLI_RESUME);
	SuspendInst->Params[0].Inst = ResumeInst;
	--Function->Top;
	Compiled.Exits = ResumeInst;
	return Compiled;
}

struct mlc_decl_expr_t {
	MLC_EXPR_FIELDS(decl);
	mlc_decl_t *Decl;
	mlc_expr_t *Child;
};

static mlc_compiled_t ml_var_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child, HashContext);
	ml_inst_t *VarInst = ml_inst_new(2, Expr->Source, MLI_VAR);
	VarInst->Params[1].Index = Expr->Decl->Index - Function->Top;
	mlc_connect(Compiled.Exits, VarInst);
	Compiled.Exits = VarInst;
	return Compiled;
}

static mlc_compiled_t ml_let_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child, HashContext);
	ml_inst_t *LetInst = ml_inst_new(2, Expr->Source, MLI_LET);
	LetInst->Params[1].Index = Expr->Decl->Index - Function->Top;
	mlc_connect(Compiled.Exits, LetInst);
	Compiled.Exits = LetInst;
	return Compiled;
}

static mlc_compiled_t ml_def_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Expr->Decl->Value;
	ml_value_t *Result = ml_expr_evaluate(Expr->Child, Function);
	Expr->Decl->Value = Result;
	for (ml_slot_t *Slot = Uninitialized->Slots; Slot; Slot = Slot->Next) Slot->Value[0] = Result;
	long ValueHash = ml_hash(Result);
	sha256_update(HashContext, (void *)&ValueHash, sizeof(ValueHash));
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	ValueInst->Params[1].Value = Result;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

static mlc_compiled_t ml_with_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	int OldTop = Function->Top;
	mlc_decl_t *OldScope = Function->Decls;
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = mlc_compile(Function, Child, HashContext);
	ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	mlc_inc_top(Function);
	mlc_connect(Compiled.Exits, PushInst);
	mlc_decl_t *Decl = Expr->Decl;
	Decl->Index = Function->Top - 1;
	Child = Child->Next;
	mlc_decl_t *NextDecl = Decl->Next;
	Decl->Next = Function->Decls;
	Function->Decls = Decl;
	while (NextDecl) {
		ML_COMPILE_HASH
		Decl = NextDecl;
		NextDecl = Decl->Next;
		mlc_compiled_t ChildCompiled = mlc_compile(Function, Child, HashContext);
		PushInst->Params[0].Inst = ChildCompiled.Start;
		PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
		mlc_inc_top(Function);
		mlc_connect(ChildCompiled.Exits, PushInst);
		Decl->Index = Function->Top - 1;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
		Child = Child->Next;
	}
	mlc_compiled_t ChildCompiled = mlc_compile(Function, Child, HashContext);
	PushInst->Params[0].Inst = ChildCompiled.Start;
	ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, MLI_EXIT);
	ExitInst->Params[1].Count = Function->Top - OldTop;
	mlc_connect(ChildCompiled.Exits, ExitInst);
	Compiled.Exits = ExitInst;
	Function->Decls = OldScope;
	Function->Top = OldTop;
	return Compiled;
}

static mlc_compiled_t ml_for_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	int OldTop = Function->Top;
	mlc_decl_t *OldScope = Function->Decls;
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = mlc_compile(Function, Child, HashContext);
	ml_inst_t *ForInst = ml_inst_new(1, Expr->Source, MLI_FOR);
	ml_inst_t *IfInst = ml_inst_new(2, Expr->Source, MLI_IF);
	ForInst->Params[0].Inst = IfInst;
	mlc_connect(Compiled.Exits, ForInst);
	ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	IfInst->Params[1].Inst = PushInst;
	mlc_inc_top(Function);
	ml_inst_t *NextInst = ml_inst_new(2, Expr->Source, MLI_NEXT);
	NextInst->Params[0].Inst = IfInst;
	NextInst->Params[1].Count = 1;
	mlc_loop_t Loop = {
		Function->Loop, Function->Try,
		NextInst, NULL,
		OldTop + 2, OldTop
	};
	mlc_decl_t *Decl = Expr->Decl;
	int HasKey = Decl->Next != 0;
	if (HasKey) {
		Loop.NextTop += 1;
		Function->Top += 2;
		Decl->Index = Function->Top - 1;
		Decl->Next->Index = Function->Top - 2;
		Decl->Next->Next = Function->Decls;
	} else {
		Function->Top += 1;
		Decl->Index = Function->Top - 1;
		Decl->Next = Function->Decls;
	}
	Function->Decls = Decl;
	if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	Function->Loop = &Loop;
	mlc_compiled_t BodyCompiled = mlc_compile(Function, Child->Next, HashContext);
	mlc_connect(BodyCompiled.Exits, NextInst);
	ml_inst_t *ValueInst = ml_inst_new(1, Expr->Source, MLI_VALUE);
	ml_inst_t *ValueResultInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	ValueInst->Params[0].Inst = ValueResultInst;
	PushInst->Params[0].Inst = ValueInst;
	if (HasKey) {
		ML_COMPILE_HASH
		NextInst->Params[1].Count = 2;
		ml_inst_t *KeyInst = ml_inst_new(1, Expr->Source, MLI_KEY);
		ml_inst_t *KeyResultInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
		KeyInst->Params[0].Inst = KeyResultInst;
		KeyResultInst->Params[0].Inst = BodyCompiled.Start;
		ValueResultInst->Params[0].Inst = KeyInst;
	} else {
		ML_COMPILE_HASH
		ValueResultInst->Params[0].Inst = BodyCompiled.Start;
	}
	Compiled.Exits = Loop.Exits;
	Function->Loop = Loop.Up;
	Function->Top = OldTop;
	if (Child->Next->Next) {
		ML_COMPILE_HASH
		mlc_compiled_t ElseCompiled = mlc_compile(Function, Child->Next->Next, HashContext);
		IfInst->Params[0].Inst = ElseCompiled.Start;;
		ml_inst_t **Slot = &Compiled.Exits;
		while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
		Slot[0] = ElseCompiled.Exits;
	} else {
		ML_COMPILE_HASH
		IfInst->Params[0].Inst = Compiled.Exits;
		Compiled.Exits = IfInst;
	}
	Function->Decls = OldScope;
	return Compiled;
}

static mlc_compiled_t ml_each_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = mlc_compile(Function, Child, HashContext);
	ml_inst_t *EachInst = ml_inst_new(1, Expr->Source, MLI_FOR);
	ml_inst_t *IfInst = ml_inst_new(2, Expr->Source, MLI_IF);
	EachInst->Params[0].Inst = IfInst;
	mlc_connect(Compiled.Exits, EachInst);
	ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	IfInst->Params[1].Inst = PushInst;
	ml_inst_t *ValueInst = ml_inst_new(1, Expr->Source, MLI_VALUE);
	PushInst->Params[0].Inst = ValueInst;
	ml_inst_t *NextInst = ml_inst_new(2, Expr->Source, MLI_NEXT);
	ValueInst->Params[0].Inst = NextInst;
	NextInst->Params[0].Inst = IfInst;
	NextInst->Params[1].Count = 0;
	Compiled.Exits = IfInst;
	return Compiled;
}

struct mlc_block_expr_t {
	MLC_EXPR_FIELDS(block);
	mlc_decl_t *Vars, *Lets, *Defs;
	mlc_expr_t *Child;
	mlc_decl_t *CatchDecl;
	mlc_expr_t *Catch;
};

static mlc_compiled_t ml_block_expr_compile(mlc_function_t *Function, mlc_block_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	int OldTop = Function->Top;
	mlc_decl_t *OldScope = Function->Decls;
	mlc_try_t Try;
	ml_inst_t *CatchExitInst = 0;
	if (Expr->Catch) {
		ML_COMPILE_HASH
		Expr->CatchDecl->Index = Function->Top++;
		Expr->CatchDecl->Next = Function->Decls;
		Function->Decls = Expr->CatchDecl;
		mlc_compiled_t TryCompiled = mlc_compile(Function, Expr->Catch, HashContext);
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, MLI_TRY);
		ml_inst_t *CatchInst = ml_inst_new(2, Expr->Source, MLI_CATCH);
		TryInst->Params[0].Inst = CatchInst;
		TryInst->Params[1].Inst = Function->Try ? Function->Try->CatchInst : Function->ReturnInst;
		CatchInst->Params[0].Inst = TryCompiled.Start;
		CatchInst->Params[1].Index = OldTop;
		Function->Decls = OldScope;
		Function->Top = OldTop;
		Try.Up = Function->Try;
		Try.CatchInst = TryInst;
		Try.CatchTop = OldTop;
		CatchExitInst = ml_inst_new(2, Expr->Source, MLI_EXIT);
		CatchExitInst->Params[1].Count = 1;
		mlc_connect(TryCompiled.Exits, CatchExitInst);
		Function->Try = &Try;
	}
	int NumVars = 0, NumLets = 0;
	for (mlc_decl_t *Decl = Expr->Vars; Decl;) {
		Decl->Index = Function->Top++;
		mlc_decl_t *NextDecl = Decl->Next;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
		Decl = NextDecl;
		++NumVars;
	}
	for (mlc_decl_t *Decl = Expr->Lets; Decl;) {
		Decl->Index = Function->Top++;
		mlc_decl_t *NextDecl = Decl->Next;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
		Decl = NextDecl;
		++NumLets;
	}
	for (mlc_decl_t *Decl = Expr->Defs; Decl;) {
		mlc_decl_t *NextDecl = Decl->Next;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
		Decl = NextDecl;
	}
	if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled;
	if (Child) {
		Compiled = mlc_compile(Function, Child, HashContext);
		while ((Child = Child->Next)) {
			ML_COMPILE_HASH
			mlc_compiled_t ChildCompiled = mlc_compile(Function, Child, HashContext);
			mlc_connect(Compiled.Exits, ChildCompiled.Start);
			Compiled.Exits = ChildCompiled.Exits;
		}
	} else {
		ml_inst_t *NilInst = ml_inst_new(1, Expr->Source, MLI_NIL);
		Compiled.Start = Compiled.Exits = NilInst;
	}
	if (NumVars + NumLets > 0) {
		ML_COMPILE_HASH
		ml_inst_t *EnterInst = ml_inst_new(3, Expr->Source, MLI_ENTER);
		EnterInst->Params[0].Inst = Compiled.Start;
		EnterInst->Params[1].Count = NumVars;
		EnterInst->Params[2].Count = NumLets;
		Compiled.Start = EnterInst;
		ml_inst_t *ExitInst = ml_inst_new(2, Expr->Source, MLI_EXIT);
		ExitInst->Params[1].Count = NumVars + NumLets;
		mlc_connect(Compiled.Exits, ExitInst);
		Compiled.Exits = ExitInst;
	}
	if (Expr->Catch) {
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, MLI_TRY);
		TryInst->Params[0].Inst = Compiled.Start;
		TryInst->Params[1].Inst = Try.CatchInst;
		Compiled.Start = TryInst;
		Function->Try = Try.Up;
		TryInst = ml_inst_new(2, Expr->Source, MLI_TRY);
		TryInst->Params[1].Inst = Function->Try ? Function->Try->CatchInst : Function->ReturnInst;
		TryInst->Params[0].Inst = CatchExitInst;
		mlc_connect(Compiled.Exits, TryInst);
		Compiled.Exits = TryInst;
	}
	Function->Decls = OldScope;
	Function->Top = OldTop;
	return Compiled;
}

static mlc_compiled_t ml_assign_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	int OldSelf = Function->Self;
	mlc_compiled_t Compiled;
	if (Expr->Child) {
		Compiled = mlc_compile(Function, Expr->Child, HashContext);
	} else {
		ml_inst_t *NilInst = ml_inst_new(1, Expr->Source, MLI_NIL);
		Compiled.Start = Compiled.Exits = NilInst;
	}
	ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	mlc_connect(Compiled.Exits, PushInst);
	Function->Self = Function->Top++;
	mlc_compiled_t ValueCompiled = mlc_compile(Function, Expr->Child->Next, HashContext);
	PushInst->Params[0].Inst = ValueCompiled.Start;
	ml_inst_t *AssignInst = ml_inst_new(1, Expr->Source, MLI_ASSIGN);
	mlc_connect(ValueCompiled.Exits, AssignInst);
	Compiled.Exits = AssignInst;
	--Function->Top;
	Function->Self = OldSelf;
	return Compiled;
}

static mlc_compiled_t ml_old_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	ml_inst_t *OldInst = ml_inst_new(2, Expr->Source, MLI_LOCAL);
	OldInst->Params[1].Index = Function->Self;
	return (mlc_compiled_t){OldInst, OldInst};
}

static mlc_compiled_t ml_tuple_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	ml_inst_t *TupleInst = ml_inst_new(2, Expr->Source, MLI_TUPLE_NEW);
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	mlc_compiled_t Compiled = {TupleInst, TupleInst};
	int Count = 0;
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) {
		mlc_compiled_t ChildCompiled = mlc_compile(Function, Child, HashContext);
		Compiled.Exits->Params[0].Inst = ChildCompiled.Start;
		ml_inst_t *SetInst = ml_inst_new(2, Expr->Source, MLI_TUPLE_SET);
		SetInst->Params[1].Index = Count++;
		mlc_connect(ChildCompiled.Exits, SetInst);
		Compiled.Exits = SetInst;
	}
	TupleInst->Params[1].Count = Count;
	ml_inst_t *PopInst = ml_inst_new(1, Expr->Source, MLI_POP);
	Compiled.Exits->Params[0].Inst = PopInst;
	Compiled.Exits = PopInst;
	--Function->Top;
	return Compiled;
}

static mlc_compiled_t ml_list_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	ml_inst_t *ListInst = ml_inst_new(1, Expr->Source, MLI_LIST_NEW);
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	mlc_compiled_t Compiled = {ListInst, ListInst};
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) {
		mlc_compiled_t ChildCompiled = mlc_compile(Function, Child, HashContext);
		Compiled.Exits->Params[0].Inst = ChildCompiled.Start;
		ml_inst_t *AppendInst = ml_inst_new(1, Expr->Source, MLI_LIST_APPEND);
		mlc_connect(ChildCompiled.Exits, AppendInst);
		Compiled.Exits = AppendInst;
	}
	ml_inst_t *PopInst = ml_inst_new(1, Expr->Source, MLI_POP);
	Compiled.Exits->Params[0].Inst = PopInst;
	Compiled.Exits = PopInst;
	--Function->Top;
	return Compiled;
}

static mlc_compiled_t ml_map_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	ml_inst_t *TupleInst = ml_inst_new(1, Expr->Source, MLI_MAP_NEW);
	if (Function->Top + 2 >= Function->Size) Function->Size = Function->Top + 3;
	++Function->Top;
	mlc_compiled_t Compiled = {TupleInst, TupleInst};
	for (mlc_expr_t *Key = Expr->Child; Key; Key = Key->Next->Next) {
		mlc_compiled_t KeyCompiled = mlc_compile(Function, Key, HashContext);
		Compiled.Exits->Params[0].Inst = KeyCompiled.Start;
		ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
		mlc_connect(KeyCompiled.Exits, PushInst);
		++Function->Top;
		mlc_compiled_t ValueCompiled = mlc_compile(Function, Key->Next, HashContext);
		PushInst->Params[0].Inst = ValueCompiled.Start;
		ml_inst_t *InsertInst = ml_inst_new(1, Expr->Source, MLI_MAP_INSERT);
		mlc_connect(ValueCompiled.Exits, InsertInst);
		Compiled.Exits = InsertInst;
		--Function->Top;
	}
	ml_inst_t *PopInst = ml_inst_new(1, Expr->Source, MLI_POP);
	Compiled.Exits->Params[0].Inst = PopInst;
	Compiled.Exits = PopInst;
	--Function->Top;
	return Compiled;
}

static mlc_compiled_t ml_call_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	int OldTop = Function->Top;
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child, HashContext);
	ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	mlc_connect(Compiled.Exits, PushInst);
	++Function->Top;
	int NumArgs = 0;
	for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next) {
		ML_COMPILE_HASH
		++NumArgs;
		mlc_compiled_t ChildCompiled = mlc_compile(Function, Child, HashContext);
		PushInst->Params[0].Inst = ChildCompiled.Start;
		PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
		mlc_connect(ChildCompiled.Exits, PushInst);
		++Function->Top;
	}
	if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	ml_inst_t *CallInst = ml_inst_new(2, Expr->Source, MLI_CALL);
	CallInst->Params[1].Count = NumArgs;
	PushInst->Params[0].Inst = CallInst;
	Compiled.Exits = CallInst;
	Function->Top = OldTop;
	return Compiled;
}

struct mlc_parent_value_expr_t {
	MLC_EXPR_FIELDS(parent_value);
	mlc_expr_t *Child;
	ml_value_t *Value;
};

static mlc_compiled_t ml_const_call_expr_compile(mlc_function_t *Function, mlc_parent_value_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	int OldTop = Function->Top;
	long ValueHash = ml_hash(Expr->Value);
	sha256_update(HashContext, (void *)&ValueHash, sizeof(ValueHash));
	ml_inst_t *CallInst = ml_inst_new(3, Expr->Source, MLI_CONST_CALL);
	CallInst->Params[2].Value = Expr->Value;
	if (Expr->Child) {
		ML_COMPILE_HASH
		int NumArgs = 1;
		mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child, HashContext);
		ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
		mlc_connect(Compiled.Exits, PushInst);
		++Function->Top;
		for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next) {
			ML_COMPILE_HASH
			++NumArgs;
			mlc_compiled_t ChildCompiled = mlc_compile(Function, Child, HashContext);
			PushInst->Params[0].Inst = ChildCompiled.Start;
			PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
			mlc_connect(ChildCompiled.Exits, PushInst);
			++Function->Top;
		}
		if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
		CallInst->Params[1].Count = NumArgs;
		PushInst->Params[0].Inst = CallInst;
		Compiled.Exits = CallInst;
		Function->Top = OldTop;
		return Compiled;
	} else {
		ML_COMPILE_HASH
		CallInst->Params[1].Count = 0;
		Function->Top = OldTop;
		return (mlc_compiled_t){CallInst, CallInst};
	}
}

extern ml_value_t *IndexMethod;
extern ml_value_t *SymbolMethod;

static mlc_compiled_t ml_import_expr_compile(mlc_function_t *Function, mlc_parent_value_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child, HashContext);
	if ((Compiled.Start == Compiled.Exits) && (Compiled.Start->Opcode == MLI_LOAD)) {
		ml_inst_t *ValueInst = Compiled.Start;
		ml_value_t *Args[2] = {ValueInst->Params[1].Value, Expr->Value};
		ml_value_t *Value = SymbolMethod->Type->call(NULL, SymbolMethod, 2, Args);
		if (Value->Type != MLErrorT) {
			if (Value->Type == MLUninitializedT) {
				ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Value;
				ml_slot_t *Slot = new(ml_slot_t);
				Slot->Value = &ValueInst->Params[1].Value;
				Slot->Next = Uninitialized->Slots;
				Uninitialized->Slots = Slot;
			}
			ValueInst->Params[1].Value = Value;
			return Compiled;
		}
	}
	ML_COMPILE_HASH
	ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	mlc_connect(Compiled.Exits, PushInst);
	ml_inst_t *LoadInst = ml_inst_new(1, Expr->Source, MLI_LOAD);
	LoadInst->Params[1].Value = Expr->Value;
	PushInst->Params[0].Inst = LoadInst;
	PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	LoadInst->Params[0].Inst = PushInst;
	Function->Top += 2;
	ml_inst_t *CallInst = ml_inst_new(3, Expr->Source, MLI_CONST_CALL);
	PushInst->Params[0].Inst = CallInst;
	CallInst->Params[1].Count = 2;
	CallInst->Params[2].Value = SymbolMethod;
	Compiled.Exits = CallInst;
	if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	Function->Top -= 2;
	return Compiled;
}

struct mlc_fun_expr_t {
	MLC_EXPR_FIELDS(fun);
	mlc_decl_t *Params;
	mlc_expr_t *Body;
};

int MLDebugClosures = 0;

static mlc_compiled_t ml_fun_expr_compile(mlc_function_t *Function, mlc_fun_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	// closure <entry> <frame_size> <num_params> <num_upvalues> <upvalue_1> ...
	mlc_function_t SubFunction[1];
	memset(SubFunction, 0, sizeof(SubFunction));
	SubFunction->Context = Function->Context;
	SubFunction->ReturnInst = ml_inst_new(0, Expr->Source, MLI_RETURN);
	SubFunction->Up = Function;
	ml_closure_info_t *Info = new(ml_closure_info_t);
	int NumParams = 0;
	mlc_decl_t **ParamSlot = &SubFunction->Decls;
	for (mlc_decl_t *Param = Expr->Params; Param;) {
		mlc_decl_t *NextParam = Param->Next;
		switch (Param->Index) {
		case ML_PARAM_EXTRA:
			Param->Index = NumParams++;
			Info->ExtraArgs = 1;
			break;
		case ML_PARAM_NAMED:
			Param->Index = NumParams++;
			Info->NamedArgs = 1;
			break;
		default:
			Param->Index = NumParams++;
			stringmap_insert(Info->Params, Param->Ident, (void *)(intptr_t)NumParams);
			break;
		}
		ParamSlot[0] = Param;
		ParamSlot = &Param->Next;
		Param = NextParam;
	}
	SubFunction->Top = SubFunction->Size = NumParams;
	SHA256_CTX SubHashContext[1];
	sha256_init(SubHashContext);
	mlc_compiled_t Compiled = mlc_compile(SubFunction, Expr->Body, SubHashContext);
	mlc_connect(Compiled.Exits, SubFunction->ReturnInst);
	int NumUpValues = 0;
	for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next) ++NumUpValues;
	ml_inst_t *ClosureInst = ml_inst_new(2 + NumUpValues, Expr->Source, MLI_CLOSURE);
	ml_param_t *Params = ClosureInst->Params;
	Info->Entry = Compiled.Start;
	Info->Return = SubFunction->ReturnInst;
	Info->FrameSize = SubFunction->Size;
	Info->NumParams = NumParams;
	Info->NumUpValues = NumUpValues;
	sha256_final(SubHashContext, Info->Hash);
	Params[1].ClosureInfo = Info;
	sha256_update(HashContext, Info->Hash, SHA256_BLOCK_SIZE);
	int Index = 2;
	for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next) Params[Index++].Index = UpValue->Index;
	if (MLDebugClosures) ml_closure_info_debug(Info);
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

static mlc_compiled_t ml_ident_expr_finish(mlc_ident_expr_t *Expr, ml_value_t *Value) {
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	if (Value->Type == MLUninitializedT) {
		ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Value;
		ml_slot_t *Slot = new(ml_slot_t);
		Slot->Value = &ValueInst->Params[1].Value;
		Slot->Next = Uninitialized->Slots;
		Uninitialized->Slots = Slot;
	}
	ValueInst->Params[1].Value = Value;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

static mlc_compiled_t ml_ident_expr_compile(mlc_function_t *Function, mlc_ident_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	for (mlc_function_t *UpFunction = Function; UpFunction; UpFunction = UpFunction->Up) {
		for (mlc_decl_t *Decl = UpFunction->Decls; Decl; Decl = Decl->Next) {
			if (!strcmp(Decl->Ident, Expr->Ident)) {
				if (Decl->Value) {
					return ml_ident_expr_finish(Expr, Decl->Value);
				} else {
					int Index = ml_upvalue_find(Function, Decl, UpFunction);
					sha256_update(HashContext, (void *)&Index, sizeof(Index));
					ML_COMPILE_HASH
					ml_inst_t *LocalInst = ml_inst_new(2, Expr->Source, MLI_LOCAL);
					LocalInst->Params[1].Index = Index;
					return (mlc_compiled_t){LocalInst, LocalInst};
				}
			}
		}
	}
	sha256_update(HashContext, (unsigned char *)Expr->Ident, strlen(Expr->Ident));
	ml_value_t *Value = (Function->Context->GlobalGet)(Function->Context->Globals, Expr->Ident);
	if (!Value) {
		Function->Context->Error = ml_error("CompilerError", "identifier %s not declared", Expr->Ident);
		ml_error_trace_add(Function->Context->Error, Expr->Source);
		longjmp(Function->Context->OnError, 1);
	}
	return ml_ident_expr_finish(Expr, Value);
}

struct mlc_value_expr_t {
	MLC_EXPR_FIELDS(value);
	ml_value_t *Value;
};

static mlc_compiled_t ml_value_expr_compile(mlc_function_t *Function, mlc_value_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	long ValueHash = ml_hash(Expr->Value);
	sha256_update(HashContext, (void *)&ValueHash, sizeof(ValueHash));
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	ValueInst->Params[1].Value = Expr->Value;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

static mlc_compiled_t ml_inline_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, SHA256_CTX *HashContext) {
	ML_COMPILE_HASH
	ml_value_t *Value = ml_expr_evaluate(Expr->Child, Function);
	long ValueHash = ml_hash(Value);
	sha256_update(HashContext, (void *)&ValueHash, sizeof(ValueHash));
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	ValueInst->Params[1].Value = Value;
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
	"each", // MLT_EACH,
	"to", // MLT_TO,
	"in", // MLT_IN,
	"is", // MLT_IS,
	"fun", // MLT_FUN,
	"return", // MLT_RETURN,
	"suspend", // MLT_SUSPEND,
	"ret", // MLT_RET,
	"susp", // MLT_SUSP,
	"with", // MLT_WITH,
	"do", // MLT_DO,
	"on", // MLT_ON,
	"nil", // MLT_NIL,
	"and", // MLT_AND,
	"or", // MLT_OR,
	"not", // MLT_NOT,
	"old", // MLT_OLD,
	"def", // MLT_DEF,
	"let", // MLT_LET,
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
	"<inline>", // MLT_INLINE,
	"<operator>", // MLT_OPERATOR
	"<method>" // MLT_METHOD
};

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
	MLT_EACH,
	MLT_TO,
	MLT_IN,
	MLT_IS,
	MLT_FUN,
	MLT_RETURN,
	MLT_SUSPEND,
	MLT_RET,
	MLT_SUSP,
	MLT_WITH,
	MLT_DO,
	MLT_ON,
	MLT_NIL,
	MLT_AND,
	MLT_OR,
	MLT_NOT,
	MLT_OLD,
	MLT_DEF,
	MLT_LET,
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
	MLT_INLINE,
	MLT_OPERATOR,
	MLT_METHOD
} ml_token_t;

struct mlc_scanner_t {
	mlc_context_t *Context;
	const char *Next;
	ml_source_t Source;
	ml_token_t Token;
	ml_value_t *Value;
	mlc_expr_t *Expr;
	mlc_decl_t *Defs;
	const char *Ident;
	void *Data;
	const char *(*read)(void *);
};

mlc_scanner_t *ml_scanner(const char *SourceName, void *Data, const char *(*read)(void *), mlc_context_t *Context) {
	mlc_scanner_t *Scanner = new(mlc_scanner_t);
	Scanner->Context = Context;
	Scanner->Token = MLT_NONE;
	Scanner->Next = "";
	Scanner->Source.Name = SourceName;
	Scanner->Source.Line = 0;
	Scanner->Data = Data;
	Scanner->read = read;
	return Scanner;
}

ml_source_t ml_scanner_source(mlc_scanner_t *Scanner, ml_source_t Source) {
	ml_source_t OldSource = Scanner->Source;
	Scanner->Source = Source;
	return OldSource;
}

void ml_scanner_reset(mlc_scanner_t *Scanner) {
	Scanner->Token = MLT_NONE;
	Scanner->Next = "";
}

const char *ml_scanner_clear(mlc_scanner_t *Scanner) {
	const char *Next = Scanner->Next;
	Scanner->Next = "";
	return Next;
}

typedef enum {EXPR_SIMPLE, EXPR_AND, EXPR_OR, EXPR_FOR, EXPR_DEFAULT} ml_expr_level_t;

static void ml_accept(mlc_scanner_t *Scanner, ml_token_t Token);
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
			++End;
			if (End[0] == '\n') {
				--Length;
			} else if (!End[0]) {
				Scanner->Context->Error = ml_error("ParseError", "end of line while parsing string");
				ml_error_trace_add(Scanner->Context->Error, Scanner->Source);
				longjmp(Scanner->Context->OnError, 1);
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
				case '\n': goto eol;
				}
			} else {
				*D++ = *S;
			}
		}
	eol:
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
			Scanner->Context->Error = ml_error("ParseError", "end of input while parsing string");
			ml_error_trace_add(Scanner->Context->Error, Scanner->Source);
			longjmp(Scanner->Context->OnError, 1);
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

static inline int isidstart(char C) {
	return isalpha(C) || (C == '_') || (C < 0);
}

static inline int isidchar(char C) {
	return isalnum(C) || (C == '_') || (C < 0);
}

static inline int isoperator(char C) {
	switch (C) {
	case '!':
	case '@':
	case '#':
	case '$':
	case '%':
	case '^':
	case '&':
	case '*':
	case '-':
	case '+':
	case '=':
	case '|':
	case '\\':
	case '~':
	case '`':
	case '/':
	case '?':
	case '<':
	case '>':
	case '.':
		return 1;
	default:
		return 0;
	}
}

#include "keywords.c"

static ml_token_t ml_next(mlc_scanner_t *Scanner) {
	if (Scanner->Token == MLT_NONE) for (;;) {
		if (!Scanner->Next || !Scanner->Next[0]) {
			Scanner->Next = (Scanner->read)(Scanner->Data);
			++Scanner->Source.Line;
			if (Scanner->Next) continue;
			Scanner->Token = MLT_EOI;
			return Scanner->Token;
		}
		char Char = Scanner->Next[0];
		if (Char == '\n') {
			++Scanner->Next;
			Scanner->Token = MLT_EOL;
			return Scanner->Token;
		}
		if (0 <= Char && Char <= ' ') {
			++Scanner->Next;
			continue;
		}
		if (Char == 'r' && Scanner->Next[1] == '\"') {
			Scanner->Next += 2;
			int Length = 0;
			const char *End = Scanner->Next;
			while (End[0] != '\"') {
				if (!End[0]) {
					Scanner->Context->Error = ml_error("ParseError", "end of input while parsing string");
					ml_error_trace_add(Scanner->Context->Error, Scanner->Source);
					longjmp(Scanner->Context->OnError, 1);
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
			return Scanner->Token;
		}
		if (isidstart(Char)) {
			const char *End = Scanner->Next + 1;
			for (Char = End[0]; isidchar(Char); Char = *++End);
			int Length = End - Scanner->Next;
			const struct keyword_t *Keyword = lookup(Scanner->Next, Length);
			if (Keyword) {
				Scanner->Token = Keyword->Token;
				Scanner->Next = End;
				return Scanner->Token;
			}
			char *Ident = snew(Length + 1);
			memcpy(Ident, Scanner->Next, Length);
			Ident[Length] = 0;
			Scanner->Ident = Ident;
			Scanner->Token = MLT_IDENT;
			Scanner->Next = End;
			return Scanner->Token;
		}
		if (isdigit(Char) || (Char == '-' && isdigit(Scanner->Next[1])) || (Char == '.' && isdigit(Scanner->Next[1]))) {
			char *End;
			double Double = strtod(Scanner->Next, (char **)&End);
			for (const char *P = Scanner->Next; P < End; ++P) {
				if (P[0] == '.' || P[0] == 'e' || P[0] == 'E') {
					Scanner->Value = ml_real(Double);
					Scanner->Token = MLT_VALUE;
					Scanner->Next = End;
					return Scanner->Token;
				}
			}
			long Integer = strtol(Scanner->Next, (char **)&End, 10);
			Scanner->Value = ml_integer(Integer);
			Scanner->Token = MLT_VALUE;
			Scanner->Next = End;
			return Scanner->Token;
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
				mlc_parent_value_expr_t *CallExpr = new(mlc_parent_value_expr_t);
				CallExpr->compile = ml_const_call_expr_compile;
				CallExpr->Source = Scanner->Source;
				CallExpr->Value = (ml_value_t *)StringNew;
				CallExpr->Child = Child;
				Scanner->Token = MLT_EXPR;
				Scanner->Expr = (mlc_expr_t *)CallExpr;
			}
			return Scanner->Token;
		}
		if (Char == '\"') {
			++Scanner->Next;
			int Length = 0;
			const char *End = Scanner->Next;
			while (End[0] != '\"') {
				if (!End[0]) {
					Scanner->Context->Error = ml_error("ParseError", "end of input while parsing string");
					ml_error_trace_add(Scanner->Context->Error, Scanner->Source);
					longjmp(Scanner->Context->OnError, 1);
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
			return Scanner->Token;
		}
		if (Char == ':') {
			if (Scanner->Next[1] == '=') {
				Scanner->Token = MLT_ASSIGN;
				Scanner->Next += 2;
				return Scanner->Token;
			} else if (Scanner->Next[1] == ':') {
				Scanner->Token = MLT_SYMBOL;
				Scanner->Next += 2;
				return Scanner->Token;
			} else if (isidchar(Scanner->Next[1])) {
				const char *End = Scanner->Next + 1;
				for (Char = End[0]; isidchar(Char); Char = *++End);
				int Length = End - Scanner->Next - 1;
				char *Ident = snew(Length + 1);
				memcpy(Ident, Scanner->Next + 1, Length);
				Ident[Length] = 0;
				Scanner->Ident = Ident;
				Scanner->Token = MLT_METHOD;
				Scanner->Next = End;
				return Scanner->Token;
			} else if (Scanner->Next[1] == '(') {
				Scanner->Token = MLT_INLINE;
				Scanner->Next += 2;
				return Scanner->Token;
			}
		}
		if (Char == '-' && Scanner->Next[1] == '-') {
			Scanner->Next = "\n";
			continue;
		}
		for (ml_token_t T = MLT_LEFT_PAREN; T <= MLT_COMMA; ++T) {
			if (Char == MLTokens[T][0]) {
				Scanner->Token = T;
				++Scanner->Next;
				return Scanner->Token;
			}
		}
		if (isoperator(Char)) {
			const char *End = Scanner->Next;
			for (Char = End[0]; isoperator(Char); Char = *++End);
			int Length = End - Scanner->Next;
			char *Operator = snew(Length + 1);
			memcpy(Operator, Scanner->Next, Length);
			Operator[Length] = 0;
			Scanner->Ident = Operator;
			Scanner->Token = MLT_OPERATOR;
			Scanner->Next = End;
			return Scanner->Token;
		}
		Scanner->Context->Error = ml_error("ParseError", "unexpected character <%c>", Char);
		ml_error_trace_add(Scanner->Context->Error, Scanner->Source);
		longjmp(Scanner->Context->OnError, 1);
	}
	return Scanner->Token;
}

static inline int ml_parse(mlc_scanner_t *Scanner, ml_token_t Token) {
	if (ml_next(Scanner) == Token) {
		Scanner->Token = MLT_NONE;
		return 1;
	} else {
		return 0;
	}
}

static void ml_accept(mlc_scanner_t *Scanner, ml_token_t Token) {
	while (ml_parse(Scanner, MLT_EOL));
	if (ml_parse(Scanner, Token)) return;
	if (Scanner->Token == MLT_IDENT) {
		Scanner->Context->Error = ml_error("ParseError", "expected %s not %s (%s)", MLTokens[Token], MLTokens[Scanner->Token], Scanner->Ident);
	} else {
		Scanner->Context->Error = ml_error("ParseError", "expected %s not %s", MLTokens[Token], MLTokens[Scanner->Token]);
	}
	ml_error_trace_add(Scanner->Context->Error, Scanner->Source);
	longjmp(Scanner->Context->OnError, 1);
}

void ml_accept_eoi(mlc_scanner_t *Scanner) {
	ml_accept(Scanner, MLT_EOI);
}

static mlc_expr_t *ml_parse_term(mlc_scanner_t *Scanner);

mlc_expr_t *ml_accept_fun_decl(mlc_scanner_t *Scanner) {
	mlc_fun_expr_t *FunExpr = new(mlc_fun_expr_t);
	FunExpr->compile = ml_fun_expr_compile;
	FunExpr->Source = Scanner->Source;
	if (!ml_parse(Scanner, MLT_RIGHT_PAREN)) {
		mlc_decl_t **ParamSlot = &FunExpr->Params;
		do {
			mlc_decl_t *Param = ParamSlot[0] = new(mlc_decl_t);
			ParamSlot = &Param->Next;
			if (ml_parse(Scanner, MLT_LEFT_SQUARE)) {
				ml_accept(Scanner, MLT_IDENT);
				Param->Ident = Scanner->Ident;
				Param->Index = ML_PARAM_EXTRA;
				ml_accept(Scanner, MLT_RIGHT_SQUARE);
				if (ml_parse(Scanner, MLT_COMMA)) {
					ml_accept(Scanner, MLT_LEFT_BRACE);
					mlc_decl_t *Param = ParamSlot[0] = new(mlc_decl_t);
					ml_accept(Scanner, MLT_IDENT);
					Param->Ident = Scanner->Ident;
					Param->Index = ML_PARAM_NAMED;
					ml_accept(Scanner, MLT_RIGHT_BRACE);
				}
				break;
			} else if (ml_parse(Scanner, MLT_LEFT_BRACE)) {
				ml_accept(Scanner, MLT_IDENT);
				Param->Ident = Scanner->Ident;
				Param->Index = ML_PARAM_NAMED;
				ml_accept(Scanner, MLT_RIGHT_BRACE);
				break;
			} else {
				ml_accept(Scanner, MLT_IDENT);
				Param->Ident = Scanner->Ident;
			}
		} while (ml_parse(Scanner, MLT_COMMA));
		ml_accept(Scanner, MLT_RIGHT_PAREN);
	}
	FunExpr->Body = ml_accept_expression(Scanner, EXPR_DEFAULT);
	return (mlc_expr_t *)FunExpr;
}

static mlc_expr_t *ml_parse_factor(mlc_scanner_t *Scanner) {
	switch (ml_next(Scanner)) {
	case MLT_DO: {
		Scanner->Token = MLT_NONE;
		mlc_expr_t *Expr = ml_accept_block(Scanner);
		ml_accept(Scanner, MLT_END);
		return Expr;
	}
	case MLT_IF: {
		Scanner->Token = MLT_NONE;
		mlc_if_expr_t *IfExpr = new(mlc_if_expr_t);
		IfExpr->compile = ml_if_expr_compile;
		IfExpr->Source = Scanner->Source;
		mlc_if_case_t **CaseSlot = &IfExpr->Cases;
		do {
			mlc_if_case_t *Case = CaseSlot[0] = new(mlc_if_case_t);
			CaseSlot = &Case->Next;
			Case->Source = Scanner->Source;
			if (ml_parse(Scanner, MLT_VAR)) {
				mlc_decl_t *Decl = new(mlc_decl_t);
				ml_accept(Scanner, MLT_IDENT);
				Decl->Ident = Scanner->Ident;
				Decl->Index = 1;
				ml_accept(Scanner, MLT_ASSIGN);
				Case->Decl = Decl;
			} else if (ml_parse(Scanner, MLT_LET)) {
				mlc_decl_t *Decl = new(mlc_decl_t);
				ml_accept(Scanner, MLT_IDENT);
				Decl->Ident = Scanner->Ident;
				Decl->Index = 0;
				ml_accept(Scanner, MLT_ASSIGN);
				Case->Decl = Decl;
			}
			Case->Condition = ml_accept_expression(Scanner, EXPR_DEFAULT);
			ml_accept(Scanner, MLT_THEN);
			Case->Body = ml_accept_block(Scanner);
		} while (ml_parse(Scanner, MLT_ELSEIF));
		if (ml_parse(Scanner, MLT_ELSE)) IfExpr->Else = ml_accept_block(Scanner);
		ml_accept(Scanner, MLT_END);
		return (mlc_expr_t *)IfExpr;
	}
	case MLT_LOOP: {
		Scanner->Token = MLT_NONE;
		mlc_parent_expr_t *LoopExpr = new(mlc_parent_expr_t);
		LoopExpr->compile = ml_loop_expr_compile;
		LoopExpr->Source = Scanner->Source;
		LoopExpr->Child = ml_accept_block(Scanner);
		ml_accept(Scanner, MLT_END);
		return (mlc_expr_t *)LoopExpr;
	}
	case MLT_FOR: {
		Scanner->Token = MLT_NONE;
		mlc_decl_expr_t *ForExpr = new(mlc_decl_expr_t);
		ForExpr->compile = ml_for_expr_compile;
		ForExpr->Source = Scanner->Source;
		mlc_decl_t *Decl = new(mlc_decl_t);
		ml_accept(Scanner, MLT_IDENT);
		Decl->Ident = Scanner->Ident;
		if (ml_parse(Scanner, MLT_COMMA)) {
			ml_accept(Scanner, MLT_IDENT);
			mlc_decl_t *Decl2 = new(mlc_decl_t);
			Decl2->Ident = Scanner->Ident;
			Decl->Next = Decl2;
		}
		ForExpr->Decl = Decl;
		ml_accept(Scanner, MLT_IN);
		ForExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
		ml_accept(Scanner, MLT_DO);
		ForExpr->Child->Next = ml_accept_block(Scanner);
		if (ml_parse(Scanner, MLT_ELSE)) {
			ForExpr->Child->Next->Next = ml_accept_block(Scanner);
		}
		ml_accept(Scanner, MLT_END);
		return (mlc_expr_t *)ForExpr;
	}
	case MLT_EACH: {
		Scanner->Token = MLT_NONE;
		mlc_parent_expr_t *EachExpr = new(mlc_parent_expr_t);
		EachExpr->compile = ml_each_expr_compile;
		EachExpr->Source = Scanner->Source;
		EachExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
		return (mlc_expr_t *)EachExpr;
	}
	case MLT_NOT: {
		Scanner->Token = MLT_NONE;
		mlc_parent_expr_t *NotExpr = new(mlc_parent_expr_t);
		NotExpr->compile = ml_not_expr_compile;
		NotExpr->Source = Scanner->Source;
		NotExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
		return (mlc_expr_t *)NotExpr;
	}
	case MLT_WHILE: {
		Scanner->Token = MLT_NONE;
		mlc_parent_expr_t *WhileExpr = new(mlc_parent_expr_t);
		WhileExpr->compile = ml_while_expr_compile;
		WhileExpr->Source = Scanner->Source;
		WhileExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
		return (mlc_expr_t *)WhileExpr;
	}
	case MLT_UNTIL: {
		Scanner->Token = MLT_NONE;
		mlc_parent_expr_t *UntilExpr = new(mlc_parent_expr_t);
		UntilExpr->compile = ml_until_expr_compile;
		UntilExpr->Source = Scanner->Source;
		UntilExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
		return (mlc_expr_t *)UntilExpr;
	}
	case MLT_EXIT: {
		Scanner->Token = MLT_NONE;
		mlc_parent_expr_t *ExitExpr = new(mlc_parent_expr_t);
		ExitExpr->compile = ml_exit_expr_compile;
		ExitExpr->Source = Scanner->Source;
		ExitExpr->Child = ml_parse_expression(Scanner, EXPR_DEFAULT);
		return (mlc_expr_t *)ExitExpr;
	}
	case MLT_NEXT: {
		Scanner->Token = MLT_NONE;
		mlc_expr_t *NextExpr = new(mlc_expr_t);
		NextExpr->compile = ml_next_expr_compile;
		NextExpr->Source = Scanner->Source;
		return NextExpr;
	}
	case MLT_FUN: {
		Scanner->Token = MLT_NONE;
		ml_accept(Scanner, MLT_LEFT_PAREN);
		return ml_accept_fun_decl(Scanner);
	}
	case MLT_RETURN: case MLT_RET: {
		Scanner->Token = MLT_NONE;
		mlc_parent_expr_t *ReturnExpr = new(mlc_parent_expr_t);
		ReturnExpr->compile = ml_return_expr_compile;
		ReturnExpr->Source = Scanner->Source;
		ReturnExpr->Child = ml_parse_expression(Scanner, EXPR_DEFAULT);
		return (mlc_expr_t *)ReturnExpr;
	}
	case MLT_SUSPEND: case MLT_SUSP: {
		Scanner->Token = MLT_NONE;
		mlc_parent_expr_t *SuspendExpr = new(mlc_parent_expr_t);
		SuspendExpr->compile = ml_suspend_expr_compile;
		SuspendExpr->Source = Scanner->Source;
		SuspendExpr->Child = ml_parse_expression(Scanner, EXPR_DEFAULT);
		if (ml_parse(Scanner, MLT_COMMA)) {
			SuspendExpr->Child->Next = ml_accept_expression(Scanner, EXPR_DEFAULT);
		}
		return (mlc_expr_t *)SuspendExpr;
	}
	case MLT_WITH: {
		Scanner->Token = MLT_NONE;
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
	}
	case MLT_IDENT: {
		Scanner->Token = MLT_NONE;
		mlc_ident_expr_t *IdentExpr = new(mlc_ident_expr_t);
		IdentExpr->compile = ml_ident_expr_compile;
		IdentExpr->Source = Scanner->Source;
		IdentExpr->Ident = Scanner->Ident;
		return (mlc_expr_t *)IdentExpr;
	}
	case MLT_VALUE: {
		Scanner->Token = MLT_NONE;
		mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
		ValueExpr->compile = ml_value_expr_compile;
		ValueExpr->Source = Scanner->Source;
		ValueExpr->Value = Scanner->Value;
		return (mlc_expr_t *)ValueExpr;
	}
	case MLT_EXPR: {
		Scanner->Token = MLT_NONE;
		return Scanner->Expr;
	}
	case MLT_INLINE: {
		Scanner->Token = MLT_NONE;
		mlc_parent_expr_t *InlineExpr = new(mlc_parent_expr_t);
		InlineExpr->compile = ml_inline_expr_compile;
		InlineExpr->Source = Scanner->Source;
		InlineExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
		ml_accept(Scanner, MLT_RIGHT_PAREN);
		return (mlc_expr_t *)InlineExpr;
	}
	case MLT_NIL: {
		Scanner->Token = MLT_NONE;
		mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
		ValueExpr->compile = ml_value_expr_compile;
		ValueExpr->Source = Scanner->Source;
		ValueExpr->Value = MLNil;
		return (mlc_expr_t *)ValueExpr;
	}
	case MLT_LEFT_PAREN: {
		Scanner->Token = MLT_NONE;
		mlc_expr_t *Expr = ml_accept_expression(Scanner, EXPR_DEFAULT);
		if (ml_parse(Scanner, MLT_COMMA)) {
			mlc_parent_expr_t *TupleExpr = new(mlc_parent_expr_t);
			TupleExpr->compile = ml_tuple_expr_compile;
			TupleExpr->Source = Expr->Source;
			TupleExpr->Child = Expr;
			mlc_expr_t **Slot = &Expr->Next;
			do {
				mlc_expr_t *Next = Slot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
				Slot = &Next->Next;
			} while (ml_parse(Scanner, MLT_COMMA));
			Expr = (mlc_expr_t *)TupleExpr;
		}
		ml_accept(Scanner, MLT_RIGHT_PAREN);
		return Expr;
	}
	case MLT_LEFT_SQUARE: {
		Scanner->Token = MLT_NONE;
		while (ml_parse(Scanner, MLT_EOL));
		mlc_parent_expr_t *ListExpr = new(mlc_parent_expr_t);
		ListExpr->compile = ml_list_expr_compile;
		ListExpr->Source = Scanner->Source;
		mlc_expr_t **ArgsSlot = &ListExpr->Child;
		if (!ml_parse(Scanner, MLT_RIGHT_SQUARE)) {
			do {
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
			} while (ml_parse(Scanner, MLT_COMMA));
			ml_accept(Scanner, MLT_RIGHT_SQUARE);
		}
		return (mlc_expr_t *)ListExpr;
	}
	case MLT_LEFT_BRACE: {
		Scanner->Token = MLT_NONE;
		while (ml_parse(Scanner, MLT_EOL));
		mlc_parent_expr_t *MapExpr = new(mlc_parent_expr_t);
		MapExpr->compile = ml_map_expr_compile;
		MapExpr->Source = Scanner->Source;
		mlc_expr_t **ArgsSlot = &MapExpr->Child;
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
					ArgExpr->Value = MLSome;
					ArgsSlot[0] = (mlc_expr_t *)ArgExpr;
					ArgsSlot = &ArgExpr->Next;
				}
			} while (ml_parse(Scanner, MLT_COMMA));
			ml_accept(Scanner, MLT_RIGHT_BRACE);
		}
		return (mlc_expr_t *)MapExpr;
	}
	case MLT_OLD: {
		Scanner->Token = MLT_NONE;
		mlc_expr_t *OldExpr = new(mlc_expr_t);
		OldExpr->compile = ml_old_expr_compile;
		OldExpr->Source = Scanner->Source;
		return OldExpr;
	}
	case MLT_OPERATOR: {
		Scanner->Token = MLT_NONE;
		ml_value_t *Operator = ml_method(Scanner->Ident);
		mlc_expr_t *Child = ml_parse_term(Scanner);
		if (Child) {
			mlc_parent_value_expr_t *CallExpr = new(mlc_parent_value_expr_t);
			CallExpr->compile = ml_const_call_expr_compile;
			CallExpr->Source = Scanner->Source;
			CallExpr->Value = Operator;
			if (Child->compile == (void *)ml_tuple_expr_compile) {
				CallExpr->Child = ((mlc_parent_expr_t *)Child)->Child;
			} else {
				CallExpr->Child = Child;
			}
			return (mlc_expr_t *)CallExpr;
		} else {
			mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
			ValueExpr->compile = ml_value_expr_compile;
			ValueExpr->Source = Scanner->Source;
			ValueExpr->Value = Operator;
			return (mlc_expr_t *)ValueExpr;
		}
	}
	case MLT_METHOD: {
		Scanner->Token = MLT_NONE;
		mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
		ValueExpr->compile = ml_value_expr_compile;
		ValueExpr->Source = Scanner->Source;
		ValueExpr->Value = ml_method(Scanner->Ident);
		return (mlc_expr_t *)ValueExpr;
	}
	default: return NULL;
	}
}

static void ml_accept_arguments(mlc_scanner_t *Scanner, mlc_expr_t **ArgsSlot) {
	while (ml_parse(Scanner, MLT_EOL));
	if (!ml_parse(Scanner, MLT_RIGHT_PAREN)) {
		ml_value_t *Names = NULL;
		if (ml_parse(Scanner, MLT_SEMICOLON)) goto has_params;
		do {
			if (Names) {
				ml_accept(Scanner, MLT_IDENT);
				ml_list_append(Names, ml_method(Scanner->Ident));
				ml_accept(Scanner, MLT_IS);
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
			} else {
				mlc_expr_t *Arg = ml_accept_expression(Scanner, EXPR_DEFAULT);
				if ((Arg->compile == (void *)ml_ident_expr_compile) && ml_parse(Scanner, MLT_IS)) {
					Names = ml_list();
					Names->Type = MLNamesT;
					ml_list_append(Names, ml_method(((mlc_ident_expr_t *)Arg)->Ident));
					mlc_value_expr_t *NamesArg = new(mlc_value_expr_t);
					NamesArg->compile = ml_value_expr_compile;
					NamesArg->Source = Scanner->Source;
					NamesArg->Value = Names;
					ArgsSlot[0] = (mlc_expr_t *)NamesArg;
					Arg = NamesArg->Next = ml_accept_expression(Scanner, EXPR_DEFAULT);
					ArgsSlot = &Arg->Next;
				} else {
					ArgsSlot[0] = Arg;
					ArgsSlot = &Arg->Next;
				}
			}
		} while (ml_parse(Scanner, MLT_COMMA));
		if (ml_parse(Scanner, MLT_SEMICOLON)) has_params: {
			ArgsSlot[0] = ml_accept_fun_decl(Scanner);
		} else {
			ml_accept(Scanner, MLT_RIGHT_PAREN);
		}
	}
}

static mlc_expr_t *ml_parse_term(mlc_scanner_t *Scanner) {
	mlc_expr_t *Expr = ml_parse_factor(Scanner);
	if (!Expr) return NULL;
	for (;;) {
		switch (ml_next(Scanner)) {
		case MLT_LEFT_PAREN: {
			Scanner->Token = MLT_NONE;
			mlc_parent_expr_t *CallExpr = new(mlc_parent_expr_t);
			CallExpr->compile = ml_call_expr_compile;
			CallExpr->Source = Scanner->Source;
			CallExpr->Child = Expr;
			ml_accept_arguments(Scanner, &Expr->Next);
			Expr = (mlc_expr_t *)CallExpr;
			break;
		}
		case MLT_LEFT_SQUARE: {
			Scanner->Token = MLT_NONE;
			while (ml_parse(Scanner, MLT_EOL));
			mlc_parent_value_expr_t *IndexExpr = new(mlc_parent_value_expr_t);
			IndexExpr->compile = ml_const_call_expr_compile;
			IndexExpr->Value = IndexMethod;
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
			break;
		}
		case MLT_METHOD: {
			Scanner->Token = MLT_NONE;
			mlc_parent_value_expr_t *CallExpr = new(mlc_parent_value_expr_t);
			CallExpr->compile = ml_const_call_expr_compile;
			CallExpr->Source = Scanner->Source;
			CallExpr->Value = ml_method(Scanner->Ident);
			CallExpr->Child = Expr;
			if (ml_parse(Scanner, MLT_LEFT_PAREN)) ml_accept_arguments(Scanner, &Expr->Next);
			Expr = (mlc_expr_t *)CallExpr;
			break;
		}
		case MLT_SYMBOL: {
			Scanner->Token = MLT_NONE;
			if (!ml_parse(Scanner, MLT_OPERATOR) && !ml_parse(Scanner, MLT_IDENT)) {
				ml_accept(Scanner, MLT_VALUE);
				if (Scanner->Value->Type != MLStringT) {
					Scanner->Context->Error = ml_error("ParseError", "expected import not %s", MLTokens[Scanner->Token]);
					ml_error_trace_add(Scanner->Context->Error, Scanner->Source);
					longjmp(Scanner->Context->OnError, 1);
				}
				Scanner->Ident = ml_string_value(Scanner->Value);
			}
			mlc_parent_value_expr_t *ImportExpr = new(mlc_parent_value_expr_t);
			ImportExpr->compile = ml_import_expr_compile;
			ImportExpr->Source = Scanner->Source;
			ImportExpr->Value = ml_string(Scanner->Ident, -1);
			ImportExpr->Child = Expr;
			Expr = (mlc_expr_t *)ImportExpr;
			break;
		}
		default: return Expr;
		}
	}
	return NULL; // Unreachable
}

static mlc_expr_t *ml_accept_term(mlc_scanner_t *Scanner) {
	while (ml_parse(Scanner, MLT_EOL));
	mlc_expr_t *Expr = ml_parse_term(Scanner);
	if (Expr) return Expr;
	Scanner->Context->Error = ml_error("ParseError", "expected <factor> not %s", MLTokens[Scanner->Token]);
	ml_error_trace_add(Scanner->Context->Error, Scanner->Source);
	longjmp(Scanner->Context->OnError, 1);
}

static mlc_expr_t *ml_parse_expression(mlc_scanner_t *Scanner, ml_expr_level_t Level) {
	mlc_expr_t *Expr = ml_parse_term(Scanner);
	if (!Expr) return NULL;
	for (;;) if (ml_parse(Scanner, MLT_OPERATOR) || ml_parse(Scanner, MLT_IDENT)) {
		mlc_parent_value_expr_t *CallExpr = new(mlc_parent_value_expr_t);
		CallExpr->compile = ml_const_call_expr_compile;
		CallExpr->Source = Scanner->Source;
		CallExpr->Value = ml_method(Scanner->Ident);
		CallExpr->Child = Expr;
		mlc_expr_t *Next = ml_accept_term(Scanner);
		if (Next->compile == (void *)ml_tuple_expr_compile) Next = ((mlc_parent_expr_t *)Next)->Child;
		Expr->Next = Next;
		Expr = (mlc_expr_t *)CallExpr;
	} else if (ml_parse(Scanner, MLT_ASSIGN)) {
		mlc_parent_expr_t *AssignExpr = new(mlc_parent_expr_t);
		AssignExpr->compile = ml_assign_expr_compile;
		AssignExpr->Source = Scanner->Source;
		AssignExpr->Child = Expr;
		Expr->Next = ml_accept_expression(Scanner, EXPR_DEFAULT);
		Expr = (mlc_expr_t *)AssignExpr;
	} else if (ml_parse(Scanner, MLT_IN)) {
		mlc_parent_value_expr_t *CallExpr = new(mlc_parent_value_expr_t);
		CallExpr->compile = ml_const_call_expr_compile;
		CallExpr->Source = Scanner->Source;
		CallExpr->Value = ml_method("in");
		CallExpr->Child = Expr;
		Expr->Next = ml_accept_expression(Scanner, EXPR_SIMPLE);
		Expr = (mlc_expr_t *)CallExpr;
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
	if (Level >= EXPR_FOR) {
		int IsComprehension = 0;
		if (ml_parse(Scanner, MLT_TO)) {
			Expr->Next = ml_accept_expression(Scanner, EXPR_OR);
			ml_accept(Scanner, MLT_FOR);
			IsComprehension = 1;
		} else {
			IsComprehension = ml_parse(Scanner, MLT_FOR);
		}
		if (IsComprehension) {
			mlc_fun_expr_t *FunExpr = new(mlc_fun_expr_t);
			FunExpr->compile = ml_fun_expr_compile;
			FunExpr->Source = Scanner->Source;
			mlc_parent_expr_t *SuspendExpr = new(mlc_parent_expr_t);
			SuspendExpr->compile = ml_suspend_expr_compile;
			SuspendExpr->Source = Scanner->Source;
			SuspendExpr->Child = Expr;
			mlc_expr_t *Body = (mlc_expr_t *)SuspendExpr;
			do {
				mlc_decl_expr_t *ForExpr = new(mlc_decl_expr_t);
				ForExpr->compile = ml_for_expr_compile;
				ForExpr->Source = Scanner->Source;
				mlc_decl_t *Decl = new(mlc_decl_t);
				ml_accept(Scanner, MLT_IDENT);
				Decl->Ident = Scanner->Ident;
				if (ml_parse(Scanner, MLT_COMMA)) {
					ml_accept(Scanner, MLT_IDENT);
					mlc_decl_t *Decl2 = new(mlc_decl_t);
					Decl2->Ident = Scanner->Ident;
					Decl->Next = Decl2;
				}
				ForExpr->Decl = Decl;
				ml_accept(Scanner, MLT_IN);
				ForExpr->Child = ml_accept_expression(Scanner, EXPR_OR);
				for (;;) {
					if (ml_parse(Scanner, MLT_IF)) {
						mlc_if_expr_t *IfExpr = new(mlc_if_expr_t);
						IfExpr->compile = ml_if_expr_compile;
						IfExpr->Source = Scanner->Source;
						mlc_if_case_t *IfCase = IfExpr->Cases = new(mlc_if_case_t);
						IfCase->Condition = ml_accept_expression(Scanner, EXPR_OR);
						IfCase->Body = Body;
						Body = (mlc_expr_t *)IfExpr;
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
						ExprSlot[0] = Body;
						Body = (mlc_expr_t *)WithExpr;
					} else {
						break;
					}
				}
				ForExpr->Child->Next = Body;
				Body = (mlc_expr_t *)ForExpr;
			} while (ml_parse(Scanner, MLT_FOR));
			FunExpr->Body = Body;
			Expr = (mlc_expr_t *)FunExpr;
		}
	}
	return Expr;
}

static mlc_expr_t *ml_accept_expression(mlc_scanner_t *Scanner, ml_expr_level_t Level) {
	while (ml_parse(Scanner, MLT_EOL));
	mlc_expr_t *Expr = ml_parse_expression(Scanner, Level);
	if (Expr) return Expr;
	Scanner->Context->Error = ml_error("ParseError", "expected <expression> not %s", MLTokens[Scanner->Token]);
	ml_error_trace_add(Scanner->Context->Error, Scanner->Source);
	longjmp(Scanner->Context->OnError, 1);
}

mlc_expr_t *ml_accept_block(mlc_scanner_t *Scanner) {
	mlc_block_expr_t *BlockExpr = new(mlc_block_expr_t);
	BlockExpr->compile = ml_block_expr_compile;
	BlockExpr->Source = Scanner->Source;
	mlc_expr_t **ExprSlot = &BlockExpr->Child;
	mlc_decl_t **VarsSlot = &BlockExpr->Vars;
	mlc_decl_t **LetsSlot = &BlockExpr->Lets;
	mlc_decl_t **DefsSlot = &BlockExpr->Defs;
	for (;;) {
		while (ml_parse(Scanner, MLT_EOL));
		if (ml_parse(Scanner, MLT_VAR)) {
			do {
				ml_accept(Scanner, MLT_IDENT);
				mlc_decl_t *Decl = VarsSlot[0] = new(mlc_decl_t);
				Decl->Ident = Scanner->Ident;
				VarsSlot = &Decl->Next;
				if (ml_parse(Scanner, MLT_ASSIGN)) {
					mlc_decl_expr_t *DeclExpr = new(mlc_decl_expr_t);
					DeclExpr->compile = ml_var_expr_compile;
					DeclExpr->Source = Scanner->Source;
					DeclExpr->Decl = Decl;
					DeclExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
					ExprSlot[0] = (mlc_expr_t *)DeclExpr;
					ExprSlot = &DeclExpr->Next;
				} else if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
					mlc_decl_expr_t *DeclExpr = new(mlc_decl_expr_t);
					DeclExpr->compile = ml_var_expr_compile;
					DeclExpr->Source = Scanner->Source;
					DeclExpr->Decl = Decl;
					DeclExpr->Child = ml_accept_fun_decl(Scanner);
					ExprSlot[0] = (mlc_expr_t *)DeclExpr;
					ExprSlot = &DeclExpr->Next;
				}
			} while (ml_parse(Scanner, MLT_COMMA));
		} else if (ml_parse(Scanner, MLT_LET)) {
			do {
				ml_accept(Scanner, MLT_IDENT);
				mlc_decl_t *Decl = LetsSlot[0] = new(mlc_decl_t);
				Decl->Ident = Scanner->Ident;
				LetsSlot = &Decl->Next;
				mlc_decl_expr_t *DeclExpr = new(mlc_decl_expr_t);
				DeclExpr->compile = ml_let_expr_compile;
				DeclExpr->Source = Scanner->Source;
				DeclExpr->Decl = Decl;
				if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
					DeclExpr->Child = ml_accept_fun_decl(Scanner);
				} else {
					ml_accept(Scanner, MLT_ASSIGN);
					DeclExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
				}
				ExprSlot[0] = (mlc_expr_t *)DeclExpr;
				ExprSlot = &DeclExpr->Next;
			} while (ml_parse(Scanner, MLT_COMMA));
		} else if (ml_parse(Scanner, MLT_DEF)) {
			do {
				ml_accept(Scanner, MLT_IDENT);
				mlc_decl_t *Decl = DefsSlot[0] = new(mlc_decl_t);
				Decl->Ident = Scanner->Ident;
				ml_uninitialized_t *Uninitialized = new(ml_uninitialized_t);
				Uninitialized->Type = MLUninitializedT;
				Decl->Value = (ml_value_t *)Uninitialized;
				DefsSlot = &Decl->Next;
				mlc_decl_expr_t *DeclExpr = new(mlc_decl_expr_t);
				DeclExpr->compile = ml_def_expr_compile;
				DeclExpr->Source = Scanner->Source;
				DeclExpr->Decl = Decl;
				if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
					DeclExpr->Child = ml_accept_fun_decl(Scanner);
				} else {
					ml_accept(Scanner, MLT_ASSIGN);
					DeclExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
				}
				ExprSlot[0] = (mlc_expr_t *)DeclExpr;
				ExprSlot = &DeclExpr->Next;
			} while (ml_parse(Scanner, MLT_COMMA));
		} else if (ml_parse(Scanner, MLT_FUN)) {
			if (ml_parse(Scanner, MLT_IDENT)) {
				mlc_decl_t *Decl = LetsSlot[0] = new(mlc_decl_t);
				Decl->Ident = Scanner->Ident;
				LetsSlot = &Decl->Next;
				ml_accept(Scanner, MLT_LEFT_PAREN);
				mlc_decl_expr_t *DeclExpr = new(mlc_decl_expr_t);
				DeclExpr->compile = ml_let_expr_compile;
				DeclExpr->Source = Scanner->Source;
				DeclExpr->Decl = Decl;
				DeclExpr->Child = ml_accept_fun_decl(Scanner);
				ExprSlot[0] = (mlc_expr_t *)DeclExpr;
				ExprSlot = &DeclExpr->Next;
			} else {
				ml_accept(Scanner, MLT_LEFT_PAREN);
				mlc_expr_t *Expr = ml_accept_fun_decl(Scanner);
				ExprSlot[0] = Expr;
				ExprSlot = &Expr->Next;
			}
		} else if (ml_parse(Scanner, MLT_ON)) {
			if (BlockExpr->CatchDecl) {
				Scanner->Context->Error = ml_error("ParseError", "no more than one error handler allowed in a block");
				ml_error_trace_add(Scanner->Context->Error, Scanner->Source);
				longjmp(Scanner->Context->OnError, 1);
			}
			ml_accept(Scanner, MLT_IDENT);
			mlc_decl_t *Decl = new(mlc_decl_t);
			Decl->Ident = Scanner->Ident;
			BlockExpr->CatchDecl = Decl;
			ml_accept(Scanner, MLT_DO);
			BlockExpr->Catch = ml_accept_block(Scanner);
			break;
		} else {
			mlc_expr_t *Expr = ml_parse_expression(Scanner, EXPR_DEFAULT);
			if (!Expr) break;
			ExprSlot[0] = Expr;
			ExprSlot = &Expr->Next;
		}
		ml_parse(Scanner, MLT_SEMICOLON);
	}
	return (mlc_expr_t *)BlockExpr;
}

ml_value_t *ml_command_evaluate(mlc_scanner_t *Scanner, stringmap_t *Vars, mlc_context_t *Context) {
	while (ml_parse(Scanner, MLT_EOL));
	if (ml_parse(Scanner, MLT_EOI)) return NULL;
	ml_value_t *Result = NULL;
	if (ml_parse(Scanner, MLT_VAR)) {

		do {
			ml_accept(Scanner, MLT_IDENT);
			const char *Ident = Scanner->Ident;
			ml_reference_t *Ref = (ml_reference_t *)ml_reference(NULL);
			ml_value_t **Slot = (ml_value_t **)stringmap_slot(Vars, Ident);
			ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Slot[0];
			if (Uninitialized && Uninitialized->Type == MLUninitializedT) {
				for (ml_slot_t *Slot = Uninitialized->Slots; Slot; Slot = Slot->Next) Slot->Value[0] = (ml_value_t *)Ref;
			}
			Slot[0] = (ml_value_t *)Ref;
			if (ml_parse(Scanner, MLT_ASSIGN)) {
				mlc_expr_t *Expr = ml_accept_expression(Scanner, EXPR_DEFAULT);
				Result = ml_compile(Expr, NULL, Context);
				if (Result->Type == MLErrorT) return Result;
				Result = ml_call(Result, 0, NULL);
				if (Result->Type == MLErrorT) return Result;
				Ref->Address[0] = Result;
			} else {
				Result = MLNil;
			}
		} while (ml_parse(Scanner, MLT_COMMA));
	} else if (ml_parse(Scanner, MLT_LET) || ml_parse(Scanner, MLT_DEF)) {
		do {
			ml_accept(Scanner, MLT_IDENT);
			const char *Ident = Scanner->Ident;
			ml_value_t **Slot = (ml_value_t **)stringmap_slot(Vars, Ident);
			ml_uninitialized_t *Ref = (ml_uninitialized_t *)Slot[0];
			if (!Ref || Ref->Type != MLUninitializedT) {
				Ref = new(ml_uninitialized_t);
				Ref->Type = MLUninitializedT;
				Slot[0] = (ml_value_t *)Ref;
			}
			mlc_expr_t *Expr;
			if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
				Expr = ml_accept_fun_decl(Scanner);
			} else {
				ml_accept(Scanner, MLT_ASSIGN);
				Expr = ml_accept_expression(Scanner, EXPR_DEFAULT);
			}
			Result = ml_compile(Expr, NULL, Context);
			if (Result->Type == MLErrorT) return Result;
			Result = ml_call(Result, 0, NULL);
			if (Result->Type == MLErrorT) return Result;
			for (ml_slot_t *Slot = Ref->Slots; Slot; Slot = Slot->Next) Slot->Value[0] = Result;
			Slot[0] = Result;
		} while (ml_parse(Scanner, MLT_COMMA));
	} else if (ml_parse(Scanner, MLT_FUN)) {
		if (ml_parse(Scanner, MLT_IDENT)) {
			const char *Ident = Scanner->Ident;
			ml_value_t **Slot = (ml_value_t **)stringmap_slot(Vars, Ident);
			ml_uninitialized_t *Ref = (ml_uninitialized_t *)Slot[0];
			if (!Ref || Ref->Type != MLUninitializedT) {
				Ref = new(ml_uninitialized_t);
				Ref->Type = MLUninitializedT;
				Slot[0] = (ml_value_t *)Ref;
			}
			ml_accept(Scanner, MLT_LEFT_PAREN);
			mlc_expr_t *Expr = ml_accept_fun_decl(Scanner);
			Result = ml_compile(Expr, NULL, Context);
			if (Result->Type == MLErrorT) return Result;
			Result = ml_call(Result, 0, NULL);
			if (Result->Type == MLErrorT) return Result;
			for (ml_slot_t *Slot = Ref->Slots; Slot; Slot = Slot->Next) Slot->Value[0] = Result;
			Slot[0] = Result;
		} else {
			ml_accept(Scanner, MLT_LEFT_PAREN);
			mlc_expr_t *Expr = ml_accept_fun_decl(Scanner);
			Result = ml_compile(Expr, NULL, Context);
			if (Result->Type == MLErrorT) return Result;
			Result = ml_call(Result, 0, NULL);
			if (Result->Type == MLErrorT) return Result;
		}
	} else {
		mlc_expr_t *Expr = ml_accept_expression(Scanner, EXPR_DEFAULT);
		Result = ml_compile(Expr, NULL, Context);
		if (Result->Type == MLErrorT) return Result;
		Result = ml_call(Result, 0, NULL);
		if (Result->Type == MLErrorT) return Result;
	}
	ml_parse(Scanner, MLT_SEMICOLON);
	return Result;
}
#include "minilang.h"
#include "ml_macros.h"
#include "ml_compiler.h"
#include "stringmap.h"
#include "sha256.h"
#include <gc/gc.h>
#include <ctype.h>
#include "ml_bytecode.h"
#include "ml_runtime.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>

typedef struct mlc_context_t {
	ml_getter_t GlobalGet;
	void *Globals;
	ml_value_t *Error;
	jmp_buf OnError;
} mlc_context_t;

#define MLC_ON_ERROR(CONTEXT) if (setjmp(CONTEXT->OnError))

typedef struct mlc_expr_t mlc_expr_t;

typedef struct mlc_function_t mlc_function_t;
typedef struct mlc_loop_t mlc_loop_t;
typedef struct mlc_try_t mlc_try_t;
typedef struct mlc_upvalue_t mlc_upvalue_t;

struct mlc_upvalue_t {
	mlc_upvalue_t *Next;
	ml_decl_t *Decl;
	int Index;
};

#define MLC_DECL_CONSTANT 1
#define MLC_DECL_FORWARD 2
#define MLC_DECL_BACKFILL 4

struct mlc_loop_t {
	mlc_loop_t *Up;
	mlc_try_t *Try;
	ml_inst_t *Next, *Exits;
	ml_decl_t *NextDecls, *ExitDecls;
	int NextTop, ExitTop;
};

struct mlc_try_t {
	mlc_try_t *Up;
	ml_inst_t *CatchInst;
	int CatchTop;
};

typedef struct { ml_inst_t *Start, *Exits; } mlc_compiled_t;

struct mlc_expr_t {
	mlc_compiled_t (*compile)(mlc_function_t *, mlc_expr_t *);
	mlc_expr_t *Next;
	ml_source_t Source;
	int End;
};

#define MLC_EXPR_FIELDS(name) \
	mlc_compiled_t (*compile)(mlc_function_t *, mlc_## name ## _expr_t *); \
	mlc_expr_t *Next; \
	ml_source_t Source; \
	int End

static inline ml_inst_t *ml_inst_new(int N, ml_source_t Source, ml_opcode_t Opcode) {
	ml_inst_t *Inst = xnew(ml_inst_t, N, ml_param_t);
	Inst->LineNo = Source.Line;
	Inst->Opcode = Opcode;
	return Inst;
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define MLCF_POTENTIAL_ASSIGNMENT 1

struct mlc_function_t {
	mlc_context_t *Context;
	ml_inst_t *ReturnInst;
	mlc_function_t *Up;
	ml_decl_t *Decls;
	mlc_loop_t *Loop;
	mlc_try_t *Try;
	mlc_upvalue_t *UpValues;
	int Top, Size, Self;
};

static inline void mlc_inc_top(mlc_function_t *Function) {
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
}

static inline mlc_compiled_t mlc_compile(mlc_function_t *Function, mlc_expr_t *Expr) {
	return Expr->compile(Function, Expr);
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
		ml_decl_t **ParamSlot = &Function->Decls;
		for (const char **P = Parameters; P[0]; ++P) {
			ml_decl_t *Param = new(ml_decl_t);
			Param->Source = Expr->Source;
			Param->Ident = P[0];
			Param->Index = Function->Top++;
			ParamSlot[0] = Param;
			ParamSlot = &Param->Next;
		}
		NumParams = Function->Top;
		Function->Size = Function->Top + 1;
	}
	mlc_compiled_t Compiled = mlc_compile(Function, Expr);
	mlc_connect(Compiled.Exits, Function->ReturnInst);
	ml_closure_t *Closure = new(ml_closure_t);
	ml_closure_info_t *Info = Closure->Info = new(ml_closure_info_t);
	Closure->Type = MLClosureT;
	Info->Entry = Compiled.Start;
	Info->Return = Function->ReturnInst;
	Info->Source = Expr->Source.Name;
	Info->End = Expr->End;
	Info->FrameSize = Function->Size;
	Info->NumParams = NumParams;
	ml_closure_info_finish(Info);
	return (ml_value_t *)Closure;
}

#define ml_expr_error(EXPR, ERROR) { \
	ml_error_trace_add(ERROR, EXPR->Source); \
	Function->Context->Error = ERROR; \
	longjmp(Function->Context->OnError, 1); \
}

static ml_value_t *ml_expr_evaluate(mlc_expr_t *Expr, mlc_function_t *Function) {
	mlc_function_t SubFunction[1];
	memset(SubFunction, 0, sizeof(SubFunction));
	SubFunction->Context = Function->Context;
	SubFunction->ReturnInst = ml_inst_new(0, Expr->Source, MLI_RETURN);
	SubFunction->Up = Function;
	SubFunction->Size = 1;
	mlc_compiled_t Compiled = mlc_compile(SubFunction, Expr);
	mlc_connect(Compiled.Exits, SubFunction->ReturnInst);
	if (SubFunction->UpValues) {
		ml_expr_error(Expr, ml_error("EvalError", "Use of non-constant value in constant expression"));
	}
	ml_closure_t *Closure = new(ml_closure_t);
	ml_closure_info_t *Info = Closure->Info = new(ml_closure_info_t);
	Closure->Type = MLClosureT;
	Info->Entry = Compiled.Start;
	Info->Return = SubFunction->ReturnInst;
	Info->Source = Expr->Source.Name;
	Info->FrameSize = SubFunction->Size;
	Info->NumParams = 0;
	ml_closure_info_finish(Info);
	ml_value_t *Result = ml_simple_call((ml_value_t *)Closure, 0, NULL);
	Result = ml_typeof(Result)->deref(Result);
	if (ml_is_error(Result)) ml_expr_error(Expr, Result);
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
typedef struct mlc_string_expr_t mlc_string_expr_t;
typedef struct mlc_block_expr_t mlc_block_expr_t;

extern ml_value_t MLBlank[];

static mlc_compiled_t ml_blank_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr) {
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	ValueInst->Params[1].Value = MLBlank;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

static mlc_compiled_t ml_nil_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr) {
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	ValueInst->Params[1].Value = MLNil;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

struct mlc_value_expr_t {
	MLC_EXPR_FIELDS(value);
	ml_value_t *Value;
};

static mlc_compiled_t ml_value_expr_compile(mlc_function_t *Function, mlc_value_expr_t *Expr) {
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	ValueInst->Params[1].Value = Expr->Value;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

struct mlc_if_case_t {
	mlc_if_case_t *Next;
	ml_source_t Source;
	mlc_expr_t *Condition;
	mlc_expr_t *Body;
	ml_decl_t *Decl;
};

struct mlc_if_expr_t {
	MLC_EXPR_FIELDS(if);
	mlc_if_case_t *Cases;
	mlc_expr_t *Else;
};

static mlc_compiled_t ml_if_expr_compile(mlc_function_t *Function, mlc_if_expr_t *Expr) {
	ml_decl_t *OldDecls = Function->Decls;
	mlc_if_case_t *Case = Expr->Cases;
	mlc_compiled_t Compiled = mlc_compile(Function, Case->Condition);
	ml_inst_t *IfInst = ml_inst_new(2, Case->Source, MLI_IF);
	ml_inst_t *ThenInst = NULL;
	if (Case->Decl) {
		ThenInst = ml_inst_new(2, Case->Source, Case->Decl->Index ? MLI_WITH_VAR : MLI_WITH);
		mlc_inc_top(Function);
		Case->Decl->Index = Function->Top - 1;
		Case->Decl->Next = Function->Decls;
		Function->Decls = Case->Decl;
		ThenInst->Params[1].Decls = Function->Decls;
		IfInst->Params[1].Inst = ThenInst;
	}
	mlc_compiled_t BodyCompiled = mlc_compile(Function, Case->Body);
	if (Case->Decl) {
		ml_inst_t *ExitInst = ml_inst_new(3, Expr->Source, MLI_EXIT);
		ExitInst->Params[1].Count = 1;
		mlc_connect(BodyCompiled.Exits, ExitInst);
		BodyCompiled.Exits = ExitInst;
		Function->Decls = OldDecls;
		--Function->Top;
		ExitInst->Params[2].Decls = Function->Decls;
	}
	mlc_connect(Compiled.Exits, IfInst);
	if (ThenInst) {
		ThenInst->Params[0].Inst = BodyCompiled.Start;
	} else {
		IfInst->Params[1].Inst = BodyCompiled.Start;
	}
	Compiled.Exits = BodyCompiled.Exits;
	while ((Case = Case->Next)) {
		mlc_compiled_t ConditionCompiled = mlc_compile(Function, Case->Condition);
		IfInst->Params[0].Inst = ConditionCompiled.Start;
		IfInst = ml_inst_new(2, Case->Source, MLI_IF);
		ThenInst = NULL;
		if (Case->Decl) {
			ThenInst = ml_inst_new(2, Case->Source, Case->Decl->Index ? MLI_WITH_VAR : MLI_WITH);
			mlc_inc_top(Function);
			Case->Decl->Index = Function->Top - 1;
			Case->Decl->Next = Function->Decls;
			Function->Decls = Case->Decl;
			ThenInst->Params[1].Decls = Function->Decls;
			IfInst->Params[1].Inst = ThenInst;
		}
		BodyCompiled = mlc_compile(Function, Case->Body);
		if (Case->Decl) {
			ml_inst_t *ExitInst = ml_inst_new(3, Expr->Source, MLI_EXIT);
			ExitInst->Params[1].Count = 1;
			mlc_connect(BodyCompiled.Exits, ExitInst);
			BodyCompiled.Exits = ExitInst;
			Function->Decls = OldDecls;
			--Function->Top;
			ExitInst->Params[2].Decls = Function->Decls;
		}
		ml_inst_t **Slot = &Compiled.Exits;
		while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
		Slot[0] = BodyCompiled.Exits;
		mlc_connect(ConditionCompiled.Exits, IfInst);
		if (ThenInst) {
			ThenInst->Params[0].Inst = BodyCompiled.Start;
		} else {
			IfInst->Params[1].Inst = BodyCompiled.Start;
		}
	}
	if (Expr->Else) {
		mlc_compiled_t BodyCompiled = mlc_compile(Function, Expr->Else);
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

static mlc_compiled_t ml_or_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = mlc_compile(Function, Child);
	ml_inst_t *OrInst = ml_inst_new(2, Expr->Source, MLI_ELSE);
	mlc_connect(Compiled.Exits, OrInst);
	Compiled.Exits = OrInst;
	for (Child = Child->Next; Child->Next; Child = Child->Next) {
		mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
		OrInst->Params[1].Inst = ChildCompiled.Start;
		OrInst = ml_inst_new(2, Expr->Source, MLI_ELSE);
		mlc_connect(ChildCompiled.Exits, OrInst);
		OrInst->Params[0].Inst = Compiled.Exits;
		Compiled.Exits = OrInst;
	}
	mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
	OrInst->Params[1].Inst = ChildCompiled.Start;
	ml_inst_t **Slot = &Compiled.Exits;
	while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
	Slot[0] = ChildCompiled.Exits;
	return Compiled;
}

static mlc_compiled_t ml_and_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = mlc_compile(Function, Child);
	ml_inst_t *IfInst = ml_inst_new(2, Expr->Source, MLI_IF);
	mlc_connect(Compiled.Exits, IfInst);
	Compiled.Exits = IfInst;
	for (Child = Child->Next; Child->Next; Child = Child->Next) {
		mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
		IfInst->Params[1].Inst = ChildCompiled.Start;
		IfInst = ml_inst_new(2, Expr->Source, MLI_IF);
		mlc_connect(ChildCompiled.Exits, IfInst);
		IfInst->Params[0].Inst = Compiled.Exits;
		Compiled.Exits = IfInst;
	}
	mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
	IfInst->Params[1].Inst = ChildCompiled.Start;
	ml_inst_t **Slot = &Compiled.Exits;
	while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
	Slot[0] = ChildCompiled.Exits;
	return Compiled;
}

static mlc_compiled_t ml_loop_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	ml_inst_t *LoopInst = ml_inst_new(1, Expr->Source, MLI_LOOP);
	mlc_loop_t Loop = {
		Function->Loop, Function->Try,
		LoopInst, NULL,
		Function->Decls, Function->Decls,
		Function->Top, Function->Top
	};
	Function->Loop = &Loop;
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
	LoopInst->Params[0].Inst = Compiled.Start;
	mlc_connect(Compiled.Exits, LoopInst);
	Function->Loop = Loop.Up;
	Compiled.Exits = Loop.Exits;
	return Compiled;
}

static mlc_compiled_t ml_next_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr) {
	if (!Function->Loop) {
		ml_expr_error(Expr, ml_error("CompilerError", "next not in loop"));
	}
	ml_inst_t *NextInst = Function->Loop->Next;
	if (Function->Try != Function->Loop->Try) {
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, MLI_TRY);
		TryInst->Params[1].Inst = Function->Try ? Function->Try->CatchInst : Function->ReturnInst;
		TryInst->Params[0].Inst = Function->Loop->Next;
		NextInst = TryInst;
	}
	if (Function->Top > Function->Loop->NextTop) {
		ml_inst_t *ExitInst = ml_inst_new(3, Expr->Source, MLI_EXIT);
		ExitInst->Params[0].Inst = NextInst;
		ExitInst->Params[1].Count = Function->Top - Function->Loop->NextTop;
		ExitInst->Params[2].Decls = Function->Loop->NextDecls;
		NextInst = ExitInst;
	} else {
	}
	return (mlc_compiled_t){NextInst, NULL};
}

static mlc_compiled_t ml_exit_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	if (!Function->Loop) {
		ml_expr_error(Expr, ml_error("CompilerError", "exit not in loop"));
	}
	mlc_loop_t *Loop = Function->Loop;
	mlc_try_t *Try = Function->Try;
	Function->Loop = Loop->Up;
	Function->Try = Loop->Try;
	mlc_compiled_t Compiled;
	if (Expr->Child) {
		Compiled = mlc_compile(Function, Expr->Child);
	} else {
		ml_inst_t *NilInst = ml_inst_new(1, Expr->Source, MLI_NIL);
		Compiled.Start = Compiled.Exits = NilInst;
	}
	if (Function->Try != Try) {
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, MLI_TRY);
		TryInst->Params[1].Inst = Function->Try ? Function->Try->CatchInst : Function->ReturnInst;
		TryInst->Params[0].Inst = Compiled.Start;
		Compiled.Start = TryInst;
	}
	Function->Loop = Loop;
	Function->Try = Try;
	if (Function->Top > Function->Loop->ExitTop) {
		ml_inst_t *ExitInst = ml_inst_new(3, Expr->Source, MLI_EXIT);
		ExitInst->Params[1].Count = Function->Top - Function->Loop->ExitTop;
		ExitInst->Params[2].Decls = Function->Loop->ExitDecls;
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

static mlc_compiled_t ml_not_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
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

static mlc_compiled_t ml_while_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	if (!Function->Loop) {
		ml_expr_error(Expr, ml_error("CompilerError", "while not in loop"));
	}
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
	ml_inst_t *ExitInst = ml_inst_new(3, Expr->Source, MLI_EXIT);
	ExitInst->Params[1].Count = Function->Top - Function->Loop->ExitTop;
	ExitInst->Params[2].Decls = Function->Loop->ExitDecls;
	mlc_loop_t *Loop = Function->Loop;
	if (Function->Try != Loop->Try) {
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

static mlc_compiled_t ml_until_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	if (!Function->Loop) {
		ml_expr_error(Expr, ml_error("CompilerError", "until not in loop"));
	}
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
	ml_inst_t *ExitInst = ml_inst_new(3, Expr->Source, MLI_EXIT);
	ExitInst->Params[1].Count = Function->Top - Function->Loop->ExitTop;
	ExitInst->Params[2].Decls = Function->Loop->ExitDecls;
	mlc_loop_t *Loop = Function->Loop;
	if (Function->Try != Loop->Try) {
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

static mlc_compiled_t ml_return_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	mlc_compiled_t Compiled;
	if (Expr->Child) {
		Compiled = mlc_compile(Function, Expr->Child);
	} else {
		ml_inst_t *NilInst = ml_inst_new(1, Expr->Source, MLI_NIL);
		Compiled.Start = Compiled.Exits = NilInst;
	}
	mlc_connect(Compiled.Exits, Function->ReturnInst);
	Compiled.Exits = NULL;
	return Compiled;
}

static mlc_compiled_t ml_suspend_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	mlc_compiled_t Compiled;
	mlc_expr_t *ValueExpr = Expr->Child;
	if (ValueExpr->Next) {
		Compiled = mlc_compile(Function, ValueExpr);
		ValueExpr = ValueExpr->Next;
	} else {
		ml_inst_t *NilInst = ml_inst_new(1, Expr->Source, MLI_NIL);
		Compiled.Start = Compiled.Exits = NilInst;
	}
	ml_inst_t *KeyPushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	mlc_connect(Compiled.Exits, KeyPushInst);
	mlc_inc_top(Function);
	mlc_compiled_t ValueCompiled = mlc_compile(Function, ValueExpr);
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
	ml_decl_t *Decl;
	mlc_expr_t *Child;
	int Count;
};

static mlc_compiled_t ml_var_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
	ml_inst_t *VarInst = ml_inst_new(2, Expr->Source, MLI_VAR);
	VarInst->Params[1].Index = Expr->Decl->Index - Function->Top;
	mlc_connect(Compiled.Exits, VarInst);
	Compiled.Exits = VarInst;
	return Compiled;
}

static mlc_compiled_t ml_varx_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
	ml_inst_t *LetInst = ml_inst_new(3, Expr->Source, MLI_VARX);
	LetInst->Params[1].Index = (Expr->Decl->Index - Function->Top) - (Expr->Count - 1);
	LetInst->Params[2].Count = Expr->Count;
	mlc_connect(Compiled.Exits, LetInst);
	Compiled.Exits = LetInst;
	return Compiled;
}

static mlc_compiled_t ml_let_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
	ml_inst_t *LetInst = ml_inst_new(2, Expr->Source, (Expr->Decl->Flags & MLC_DECL_BACKFILL) ? MLI_LETI : MLI_LET);
	LetInst->Params[1].Index = Expr->Decl->Index - Function->Top;
	Expr->Decl->Flags = 0;
	mlc_connect(Compiled.Exits, LetInst);
	Compiled.Exits = LetInst;
	return Compiled;
}

static mlc_compiled_t ml_letx_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
	ml_inst_t *LetInst = ml_inst_new(3, Expr->Source, MLI_LETX);
	ml_decl_t *Decl = Expr->Decl;
	LetInst->Params[1].Index = (Decl->Index - Function->Top) - (Expr->Count - 1);
	LetInst->Params[2].Count = Expr->Count;
	for (int I = 0; I < Expr->Count; ++I) {
		Decl->Flags = 0;
		Decl = Decl->Next;
	}
	mlc_connect(Compiled.Exits, LetInst);
	Compiled.Exits = LetInst;
	return Compiled;
}

static inline void ml_decl_set_value(ml_decl_t *Decl, ml_value_t *Value) {
	if (Decl->Value) ml_uninitialized_set(Decl->Value, Value);
	Decl->Value = Value;
}

extern ml_value_t *IndexMethod;
extern ml_value_t *SymbolMethod;

static mlc_compiled_t ml_def_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
	ml_value_t *Result = ml_expr_evaluate(Expr->Child, Function);
	if (ml_is_error(Result)) ml_expr_error(Expr, Result);
	ml_decl_t *Decl = Expr->Decl;
	if (Expr->Count) {
		ml_value_t *Args[2] = {Result, MLNil};
		for (int I = Expr->Count; --I >= 0;) {
			Args[1] = ml_string(Decl->Ident, -1);
			ml_value_t *Value = ml_simple_call(SymbolMethod, 2, Args);
			if (ml_is_error(Value)) ml_expr_error(Expr, Value);
			ml_decl_set_value(Decl, Value);
			Decl = Decl->Next;
		}
	} else {
		ml_decl_set_value(Decl, Result);
	}
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	ValueInst->Params[1].Value = Result;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

static mlc_compiled_t ml_defx_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
	ml_value_t *Result = ml_expr_evaluate(Expr->Child, Function);
	if (ml_is_error(Result)) ml_expr_error(Expr, Result);
	ml_decl_t *Decl = Expr->Decl;
	for (int I = Expr->Count; --I >= 0;) {
		ml_value_t *Value = ml_unpack(Result, I);
		if (!Value) ml_expr_error(Expr, ml_error("ValueError", "Not enough values to unpack (%d < %d)", I, Expr->Count));
		if (ml_is_error(Value)) ml_expr_error(Expr, Value);
		ml_decl_set_value(Decl, Value);
		Decl = Decl->Next;
	}
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	ValueInst->Params[1].Value = Result;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

static mlc_compiled_t ml_with_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
	int OldTop = Function->Top;
	ml_decl_t *OldDecls = Function->Decls;
	mlc_expr_t *Child = Expr->Child;
	ml_inst_t *Start = NULL, *PushInst = NULL;
	for (ml_decl_t *Decl = Expr->Decl; Decl;) {
		ml_decl_t *NextDecl = Decl->Next;
		int Count = Decl->Index;
		int Top = Function->Top;
		Decl->Index = Top++;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
		for (int I = 1; I < Count; ++I) {
			Decl = NextDecl;
			NextDecl = Decl->Next;
			Decl->Index = Top++;
			Decl->Next = Function->Decls;
			Function->Decls = Decl;
		}
		mlc_compiled_t Compiled = mlc_compile(Function, Child);
		if (PushInst) {
			PushInst->Params[0].Inst = Compiled.Start;
		} else {
			Start = Compiled.Start;
		}
		if (Count == 1) {
			PushInst = ml_inst_new(2, Expr->Source, MLI_WITH);
			PushInst->Params[1].Decls = Function->Decls;
			mlc_inc_top(Function);
		} else {
			PushInst = ml_inst_new(3, Expr->Source, MLI_WITHX);
			PushInst->Params[1].Count = Count;
			PushInst->Params[2].Decls = Function->Decls;
			for (int I = 0; I < Count; ++I) mlc_inc_top(Function);
		}
		mlc_connect(Compiled.Exits, PushInst);
		Child = Child->Next;
		Decl = NextDecl;
	}
	mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
	PushInst->Params[0].Inst = ChildCompiled.Start;
	ml_inst_t *ExitInst = ml_inst_new(3, Expr->Source, MLI_EXIT);
	ExitInst->Params[1].Count = Function->Top - OldTop;
	ExitInst->Params[2].Decls = OldDecls;
	mlc_connect(ChildCompiled.Exits, ExitInst);
	Function->Decls = OldDecls;
	Function->Top = OldTop;
	return (mlc_compiled_t){Start, ExitInst};
}

static mlc_compiled_t ml_for_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
	int OldTop = Function->Top;
	ml_decl_t *OldDecls = Function->Decls;
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = mlc_compile(Function, Child);
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
	ml_decl_t *Decl = Expr->Decl;
	int HasKey = Decl->Next != 0;
	if (HasKey) {
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
	mlc_loop_t Loop = {
		Function->Loop, Function->Try,
		NextInst, NULL,
		Function->Decls, OldDecls,
		Function->Top, OldTop
	};
	if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	Function->Loop = &Loop;
	mlc_compiled_t BodyCompiled = mlc_compile(Function, Child->Next);
	mlc_connect(BodyCompiled.Exits, NextInst);
	ml_inst_t *ValueInst = ml_inst_new(1, Expr->Source, MLI_VALUE);
	ml_inst_t *ValueResultInst = ml_inst_new(2, Expr->Source, MLI_WITH);
	ValueInst->Params[0].Inst = ValueResultInst;
	ValueResultInst->Params[1].Decls = Function->Decls;
	PushInst->Params[0].Inst = ValueInst;
	if (HasKey) {
		NextInst->Params[1].Count = 2;
		ml_inst_t *KeyInst = ml_inst_new(1, Expr->Source, MLI_KEY);
		ml_inst_t *KeyResultInst = ml_inst_new(2, Expr->Source, MLI_WITH);
		KeyInst->Params[0].Inst = KeyResultInst;
		KeyResultInst->Params[0].Inst = BodyCompiled.Start;
		KeyResultInst->Params[1].Decls = Function->Decls;
		ValueResultInst->Params[0].Inst = KeyInst;
		ValueResultInst->Params[1].Decls = Function->Decls->Next;
	} else {
		ValueResultInst->Params[0].Inst = BodyCompiled.Start;
	}
	Compiled.Exits = Loop.Exits;
	Function->Loop = Loop.Up;
	Function->Top = OldTop;
	if (Child->Next->Next) {
		mlc_compiled_t ElseCompiled = mlc_compile(Function, Child->Next->Next);
		IfInst->Params[0].Inst = ElseCompiled.Start;;
		ml_inst_t **Slot = &Compiled.Exits;
		while (Slot[0]) Slot = &Slot[0]->Params[0].Inst;
		Slot[0] = ElseCompiled.Exits;
	} else {
		IfInst->Params[0].Inst = Compiled.Exits;
		Compiled.Exits = IfInst;
	}
	Function->Decls = OldDecls;
	return Compiled;
}

static mlc_compiled_t ml_each_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled = mlc_compile(Function, Child);
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
	ml_decl_t *Vars, *Lets, *Defs;
	mlc_expr_t *Child;
	ml_decl_t *CatchDecl;
	mlc_expr_t *Catch;
};

static mlc_compiled_t ml_block_expr_compile(mlc_function_t *Function, mlc_block_expr_t *Expr) {
	int OldTop = Function->Top;
	ml_decl_t *OldDecls = Function->Decls;
	mlc_try_t Try;
	ml_inst_t *CatchExitInst = 0;
	if (Expr->Catch) {
		Expr->CatchDecl->Index = Function->Top++;
		Expr->CatchDecl->Next = Function->Decls;
		Function->Decls = Expr->CatchDecl;
		mlc_compiled_t TryCompiled = mlc_compile(Function, Expr->Catch);
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, MLI_TRY);
		ml_inst_t *CatchInst = ml_inst_new(3, Expr->Source, MLI_CATCH);
		TryInst->Params[0].Inst = CatchInst;
		TryInst->Params[1].Inst = Function->Try ? Function->Try->CatchInst : Function->ReturnInst;
		CatchInst->Params[0].Inst = TryCompiled.Start;
		CatchInst->Params[1].Index = OldTop;
		CatchInst->Params[2].Decls = Function->Decls;
		Function->Decls = OldDecls;
		Function->Top = OldTop;
		Try.Up = Function->Try;
		Try.CatchInst = TryInst;
		Try.CatchTop = OldTop;
		CatchExitInst = ml_inst_new(3, Expr->Source, MLI_EXIT);
		CatchExitInst->Params[1].Count = 1;
		CatchExitInst->Params[2].Decls = OldDecls;
		mlc_connect(TryCompiled.Exits, CatchExitInst);
		Function->Try = &Try;
	}
	int NumVars = 0, NumLets = 0;
	stringmap_t Decls[1] = {STRINGMAP_INIT};
	for (ml_decl_t *Decl = Expr->Vars; Decl;) {
		ml_decl_t **Slot = (ml_decl_t **)stringmap_slot(Decls, Decl->Ident);
		if (Slot[0]) ml_expr_error(Expr, ml_error("NameError", "Identifier %s already defined in line %d", Decl->Ident, Slot[0]->Source.Line));
		Slot[0] = Decl;
		Decl->Index = Function->Top++;
		ml_decl_t *NextDecl = Decl->Next;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
		Decl = NextDecl;
		++NumVars;
	}
	for (ml_decl_t *Decl = Expr->Lets; Decl;) {
		ml_decl_t **Slot = (ml_decl_t **)stringmap_slot(Decls, Decl->Ident);
		if (Slot[0]) ml_expr_error(Expr, ml_error("NameError", "Identifier %s already defined in line %d", Decl->Ident, Slot[0]->Source.Line));
		Slot[0] = Decl;
		Decl->Index = Function->Top++;
		ml_decl_t *NextDecl = Decl->Next;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
		Decl = NextDecl;
		++NumLets;
	}
	for (ml_decl_t *Decl = Expr->Defs; Decl;) {
		ml_decl_t **Slot = (ml_decl_t **)stringmap_slot(Decls, Decl->Ident);
		if (Slot[0]) ml_expr_error(Expr, ml_error("NameError", "Identifier %s already defined in line %d", Decl->Ident, Slot[0]->Source.Line));
		Slot[0] = Decl;
		Decl->Flags = MLC_DECL_CONSTANT;
		ml_decl_t *NextDecl = Decl->Next;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
		Decl = NextDecl;
	}
	if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	mlc_expr_t *Child = Expr->Child;
	mlc_compiled_t Compiled;
	if (Child) {
		Compiled = mlc_compile(Function, Child);
		while ((Child = Child->Next)) {
			mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
			mlc_connect(Compiled.Exits, ChildCompiled.Start);
			Compiled.Exits = ChildCompiled.Exits;
		}
	} else {
		ml_inst_t *NilInst = ml_inst_new(1, Expr->Source, MLI_NIL);
		Compiled.Start = Compiled.Exits = NilInst;
	}
	if (NumVars + NumLets > 0) {
		ml_inst_t *EnterInst = ml_inst_new(4, Expr->Source, MLI_ENTER);
		EnterInst->Params[0].Inst = Compiled.Start;
		EnterInst->Params[1].Count = NumVars;
		EnterInst->Params[2].Count = NumLets;
		EnterInst->Params[3].Decls = Function->Decls;
		Compiled.Start = EnterInst;
		ml_inst_t *ExitInst = ml_inst_new(3, Expr->Source, MLI_EXIT);
		ExitInst->Params[1].Count = NumVars + NumLets;
		ExitInst->Params[2].Decls = OldDecls;
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
	Function->Decls = OldDecls;
	Function->Top = OldTop;
	return Compiled;
}

static mlc_compiled_t ml_assign_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	int OldSelf = Function->Self;
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
	ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	mlc_connect(Compiled.Exits, PushInst);
	Function->Self = Function->Top++;
	mlc_compiled_t ValueCompiled = mlc_compile(Function, Expr->Child->Next);
	PushInst->Params[0].Inst = ValueCompiled.Start;
	ml_inst_t *AssignInst = ml_inst_new(1, Expr->Source, MLI_ASSIGN);
	mlc_connect(ValueCompiled.Exits, AssignInst);
	Compiled.Exits = AssignInst;
	--Function->Top;
	Function->Self = OldSelf;
	return Compiled;
}

static mlc_compiled_t ml_old_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr) {
	ml_inst_t *OldInst = ml_inst_new(2, Expr->Source, MLI_LOCAL);
	OldInst->Params[1].Index = Function->Self;
	return (mlc_compiled_t){OldInst, OldInst};
}

static mlc_compiled_t ml_tuple_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	ml_inst_t *TupleInst = ml_inst_new(2, Expr->Source, MLI_TUPLE_NEW);
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	mlc_compiled_t Compiled = {TupleInst, TupleInst};
	int Count = 0;
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) {
		mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
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

static mlc_compiled_t ml_list_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	ml_inst_t *ListInst = ml_inst_new(1, Expr->Source, MLI_LIST_NEW);
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	mlc_compiled_t Compiled = {ListInst, ListInst};
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) {
		mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
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

static mlc_compiled_t ml_map_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	ml_inst_t *TupleInst = ml_inst_new(1, Expr->Source, MLI_MAP_NEW);
	if (Function->Top + 2 >= Function->Size) Function->Size = Function->Top + 3;
	++Function->Top;
	mlc_compiled_t Compiled = {TupleInst, TupleInst};
	for (mlc_expr_t *Key = Expr->Child; Key; Key = Key->Next->Next) {
		mlc_compiled_t KeyCompiled = mlc_compile(Function, Key);
		Compiled.Exits->Params[0].Inst = KeyCompiled.Start;
		ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
		mlc_connect(KeyCompiled.Exits, PushInst);
		++Function->Top;
		mlc_compiled_t ValueCompiled = mlc_compile(Function, Key->Next);
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

static mlc_compiled_t ml_call_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next) {
		if (Child->compile == (void *)ml_blank_expr_compile) {
			mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
			ml_inst_t *PartialInst = ml_inst_new(2, Expr->Source, MLI_PARTIAL_NEW);
			mlc_connect(Compiled.Exits, PartialInst);
			int NumArgs = 0;
			ml_inst_t *LastInst = PartialInst;
			++Function->Top;
			for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next, ++NumArgs) {
				if (Child->compile != (void *)ml_blank_expr_compile) {
					mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
					ml_inst_t *SetInst = ml_inst_new(2, Expr->Source, MLI_PARTIAL_SET);
					SetInst->Params[1].Count = NumArgs;
					LastInst->Params[0].Inst = ChildCompiled.Start;
					mlc_connect(ChildCompiled.Exits, SetInst);
					LastInst = SetInst;
				}
			}
			PartialInst->Params[1].Count = NumArgs;
			ml_inst_t *PopInst = ml_inst_new(1, Expr->Source, MLI_POP);
			LastInst->Params[0].Inst = PopInst;
			--Function->Top;
			Compiled.Exits = PopInst;
			return Compiled;
		}
	}
	int OldTop = Function->Top;
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
	ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	mlc_connect(Compiled.Exits, PushInst);
	++Function->Top;
	int NumArgs = 0;
	for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next) {
		++NumArgs;
		mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
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

static mlc_compiled_t ml_const_call_expr_compile(mlc_function_t *Function, mlc_parent_value_expr_t *Expr) {
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) {
		if (Child->compile == (void *)ml_blank_expr_compile) {
			ml_inst_t *LoadInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
			LoadInst->Params[1].Value = Expr->Value;
			mlc_compiled_t Compiled = {LoadInst, NULL};
			ml_inst_t *PartialInst = ml_inst_new(2, Expr->Source, MLI_PARTIAL_NEW);
			LoadInst->Params[0].Inst = PartialInst;
			int NumArgs = 0;
			ml_inst_t *LastInst = PartialInst;
			++Function->Top;
			for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next, ++NumArgs) {
				if (Child->compile != (void *)ml_blank_expr_compile) {
					mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
					ml_inst_t *SetInst = ml_inst_new(2, Expr->Source, MLI_PARTIAL_SET);
					SetInst->Params[1].Count = NumArgs;
					LastInst->Params[0].Inst = ChildCompiled.Start;
					mlc_connect(ChildCompiled.Exits, SetInst);
					LastInst = SetInst;
				}
			}
			PartialInst->Params[1].Count = NumArgs;
			ml_inst_t *PopInst = ml_inst_new(1, Expr->Source, MLI_POP);
			LastInst->Params[0].Inst = PopInst;
			--Function->Top;
			Compiled.Exits = PopInst;
			return Compiled;
		}
	}
	int OldTop = Function->Top;
	if (Expr->Child) {
		int NumArgs = 1;
		mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
		ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
		mlc_connect(Compiled.Exits, PushInst);
		++Function->Top;
		for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next) {
			++NumArgs;
			mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
			PushInst->Params[0].Inst = ChildCompiled.Start;
			PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
			mlc_connect(ChildCompiled.Exits, PushInst);
			++Function->Top;
		}
		if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
		ml_inst_t *CallInst = ml_inst_new(3, Expr->Source, MLI_CONST_CALL);
		CallInst->Params[2].Value = Expr->Value;
		CallInst->Params[1].Count = NumArgs;
		PushInst->Params[0].Inst = CallInst;
		Compiled.Exits = CallInst;
		Function->Top = OldTop;
		return Compiled;
	} else {
		ml_inst_t *CallInst = ml_inst_new(3, Expr->Source, MLI_CONST_CALL);
		CallInst->Params[2].Value = Expr->Value;
		CallInst->Params[1].Count = 0;
		Function->Top = OldTop;
		return (mlc_compiled_t){CallInst, CallInst};
	}
}

static mlc_compiled_t ml_import_expr_compile(mlc_function_t *Function, mlc_parent_value_expr_t *Expr) {
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
	if ((Compiled.Start == Compiled.Exits) && (Compiled.Start->Opcode == MLI_LOAD)) {
		ml_inst_t *ValueInst = Compiled.Start;
		ml_value_t *Args[2] = {ValueInst->Params[1].Value, Expr->Value};
		ml_value_t *Value = ml_simple_call(SymbolMethod, 2, Args);
		if (!ml_is_error(Value)) {
			if (ml_typeof(Value) == MLUninitializedT) {
				ml_uninitialized_use(Value, &ValueInst->Params[1].Value);
			}
			ValueInst->Params[1].Value = Value;
			return Compiled;
		}
	}
	ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	mlc_connect(Compiled.Exits, PushInst);
	ml_inst_t *LoadInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
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

typedef struct mlc_string_part_t mlc_string_part_t;

struct mlc_string_expr_t {
	MLC_EXPR_FIELDS(string);
	mlc_string_part_t *Parts;
};

struct mlc_string_part_t {
	mlc_string_part_t *Next;
	union {
		mlc_expr_t *Child;
		const char *Chars;
	};
	int Length;
};

static mlc_compiled_t ml_string_expr_compile(mlc_function_t *Function, mlc_string_expr_t *Expr) {
	ml_inst_t *StringInst = ml_inst_new(1, Expr->Source, MLI_STRING_NEW);
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	mlc_compiled_t Compiled = {StringInst, StringInst};
	for (mlc_string_part_t *Part = Expr->Parts; Part; Part = Part->Next) {
		if (Part->Length) {
			ml_inst_t *AddInst = ml_inst_new(3, Expr->Source, MLI_STRING_ADDS);
			AddInst->Params[1].Count = Part->Length;
			AddInst->Params[2].Ptr = Part->Chars;
			Compiled.Exits->Params[0].Inst = AddInst;
			Compiled.Exits = AddInst;
		} else {
			int OldTop = Function->Top;
			int NumArgs = 0;
			for (mlc_expr_t *Child = Part->Child; Child; Child = Child->Next) {
				++NumArgs;
				mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
				Compiled.Exits->Params[0].Inst = ChildCompiled.Start;
				ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
				mlc_connect(ChildCompiled.Exits, PushInst);
				Compiled.Exits = PushInst;
				++Function->Top;
			}
			if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
			ml_inst_t *AddInst = ml_inst_new(3, Expr->Source, MLI_STRING_ADD);
			AddInst->Params[1].Count = NumArgs;
			Compiled.Exits->Params[0].Inst = AddInst;
			Compiled.Exits = AddInst;
			Function->Top = OldTop;
		}
	}
	ml_inst_t *EndInst = ml_inst_new(1, Expr->Source, MLI_STRING_END);
	Compiled.Exits->Params[0].Inst = EndInst;
	Compiled.Exits = EndInst;
	--Function->Top;
	return Compiled;
}

struct mlc_fun_expr_t {
	MLC_EXPR_FIELDS(fun);
	ml_decl_t *Params;
	mlc_expr_t *Body;
};

static mlc_compiled_t ml_fun_expr_compile(mlc_function_t *Function, mlc_fun_expr_t *Expr) {
	// closure <entry> <frame_size> <num_params> <num_upvalues> <upvalue_1> ...
	mlc_function_t SubFunction[1];
	memset(SubFunction, 0, sizeof(SubFunction));
	SubFunction->Context = Function->Context;
	SubFunction->ReturnInst = ml_inst_new(0, Expr->Source, MLI_RETURN);
	SubFunction->Up = Function;
	ml_closure_info_t *Info = new(ml_closure_info_t);
	Info->Source = Expr->Source.Name;
	Info->End = Expr->End;
	int NumParams = 0;
	ml_decl_t **ParamSlot = &SubFunction->Decls;
	for (ml_decl_t *Param = Expr->Params; Param;) {
		ml_decl_t *NextParam = Param->Next;
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
	Info->Decls = SubFunction->Decls;
	mlc_compiled_t Compiled = mlc_compile(SubFunction, Expr->Body);
	ml_decl_t **UpValueSlot = &SubFunction->Decls;
	while (UpValueSlot[0]) UpValueSlot = &UpValueSlot[0]->Next;
	for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next) {
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source = Expr->Source;
		Decl->Ident = UpValue->Decl->Ident;
		Decl->Value = UpValue->Decl->Value;
		Decl->Index = ~UpValue->Index;
		UpValueSlot[0] = Decl;
		UpValueSlot = &Decl->Next;
	}
	mlc_connect(Compiled.Exits, SubFunction->ReturnInst);
	Info->Entry = Compiled.Start;
	Info->Return = SubFunction->ReturnInst;
	Info->FrameSize = SubFunction->Size;
	Info->NumParams = NumParams;
	ml_closure_info_finish(Info);
	if (SubFunction->UpValues) {
		int NumUpValues = 0;
		for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next) ++NumUpValues;
		ml_inst_t *ClosureInst = ml_inst_new(2 + NumUpValues, Expr->Source, MLI_CLOSURE);
		ml_param_t *Params = ClosureInst->Params;
		Info->NumUpValues = NumUpValues;
		Params[1].ClosureInfo = Info;
		int Index = 2;
		for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next) Params[Index++].Index = UpValue->Index;
		return (mlc_compiled_t){ClosureInst, ClosureInst};
	} else {
		Info->NumUpValues = 0;
		ml_closure_t *Closure = xnew(ml_closure_t, 0, ml_value_t *);
		Closure->Type = MLClosureT;
		Closure->Info = Info;
		ml_inst_t *LoadInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
		LoadInst->Params[1].Value = (ml_value_t *)Closure;
		return (mlc_compiled_t){LoadInst, LoadInst};
	}
}

struct mlc_ident_expr_t {
	MLC_EXPR_FIELDS(ident);
	const char *Ident;
};

static int ml_upvalue_find(mlc_function_t *Function, ml_decl_t *Decl, mlc_function_t *Origin) {
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
	if (ml_typeof(Value) == MLUninitializedT) {
		ml_uninitialized_use(Value, &ValueInst->Params[1].Value);
	}
	ValueInst->Params[1].Value = Value;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

static mlc_compiled_t ml_ident_expr_compile(mlc_function_t *Function, mlc_ident_expr_t *Expr) {
	for (mlc_function_t *UpFunction = Function; UpFunction; UpFunction = UpFunction->Up) {
		for (ml_decl_t *Decl = UpFunction->Decls; Decl; Decl = Decl->Next) {
			if (!strcmp(Decl->Ident, Expr->Ident)) {
				if (Decl->Flags == MLC_DECL_CONSTANT) {
					if (!Decl->Value) {
						Decl->Value = ml_uninitialized();
					}
					return ml_ident_expr_finish(Expr, Decl->Value);
				} else {
					int Index = ml_upvalue_find(Function, Decl, UpFunction);
					ml_inst_t *LocalInst;
					if (Decl->Flags & MLC_DECL_FORWARD) Decl->Flags |= MLC_DECL_BACKFILL;
					if ((Index >= 0) && (Decl->Flags & MLC_DECL_FORWARD)) {
						LocalInst = ml_inst_new(2, Expr->Source, MLI_LOCALX);
						LocalInst->Params[1].Index = Index;
					} else if (Index >= 0) {
						LocalInst = ml_inst_new(2, Expr->Source, MLI_LOCAL);
						LocalInst->Params[1].Index = Index;
					} else {
						LocalInst = ml_inst_new(2, Expr->Source, MLI_UPVALUE);
						LocalInst->Params[1].Index = ~Index;
					}
					return (mlc_compiled_t){LocalInst, LocalInst};
				}
			}
		}
	}
	ml_value_t *Value = (Function->Context->GlobalGet)(Function->Context->Globals, Expr->Ident);
	if (!Value) {
		ml_expr_error(Expr, ml_error("CompilerError", "identifier %s not declared", Expr->Ident));
	}
	if (ml_is_error(Value)) ml_expr_error(Expr, Value);
	return ml_ident_expr_finish(Expr, Value);
}

static mlc_compiled_t ml_inline_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	ml_value_t *Value = ml_expr_evaluate(Expr->Child, Function);
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	ValueInst->Params[1].Value = Value;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

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
	MLT_RET,
	MLT_SUSP,
	MLT_METH,
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
	MLT_BLANK,
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
	MLT_IMPORT,
	MLT_VALUE,
	MLT_EXPR,
	MLT_INLINE,
	MLT_OPERATOR,
	MLT_METHOD
} ml_token_t;

#define MLT_DELIM_FIRST MLT_LEFT_PAREN
#define MLT_DELIM_LAST MLT_COMMA

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
	"ret", // MLT_RET,
	"susp", // MLT_SUSP,
	"meth", // MLT_METH,
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
	"_", // MLT_BLANK,
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

struct mlc_scanner_t {
	mlc_context_t *Context;
	const char *Next;
	ml_source_t Source;
	ml_token_t Token;
	ml_value_t *Value;
	mlc_expr_t *Expr;
	ml_decl_t *Defs;
	const char *Ident;
	void *Data;
	const char *(*read)(void *);
};

mlc_scanner_t *ml_scanner(const char *SourceName, void *Data, const char *(*read)(void *), ml_getter_t GlobalGet, void *Globals) {
	mlc_context_t *Context = new(mlc_context_t);
	Context->GlobalGet = GlobalGet;
	Context->Globals = Globals;
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

__attribute__((noreturn)) void ml_scanner_error(mlc_scanner_t *Scanner, const char *Error, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	ml_value_t *Value = ml_errorv(Error, Format, Args);
	va_end(Args);
	Scanner->Context->Error = (ml_value_t *)Value;
	ml_error_trace_add(Scanner->Context->Error, Scanner->Source);
	longjmp(Scanner->Context->OnError, 1);
}

#define ML_EXPR(EXPR, TYPE, COMP) \
	mlc_ ## TYPE ## _expr_t *EXPR = new(mlc_ ## TYPE ## _expr_t); \
	EXPR->compile = ml_ ## COMP ## _expr_compile; \
	EXPR->Source = Scanner->Source

typedef enum {
	EXPR_SIMPLE,
	EXPR_AND,
	EXPR_OR,
	EXPR_FOR,
	EXPR_DEFAULT
} ml_expr_level_t;

static int ml_parse(mlc_scanner_t *Scanner, ml_token_t Token);
static void ml_accept(mlc_scanner_t *Scanner, ml_token_t Token);
static mlc_expr_t *ml_parse_expression(mlc_scanner_t *Scanner, ml_expr_level_t Level);
static mlc_expr_t *ml_accept_expression(mlc_scanner_t *Scanner, ml_expr_level_t Level);
static void ml_accept_arguments(mlc_scanner_t *Scanner, ml_token_t EndToken, mlc_expr_t **ArgsSlot);

static ml_token_t ml_accept_string(mlc_scanner_t *Scanner) {
	mlc_string_part_t *Parts = NULL, **Slot = &Parts;
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *End = Scanner->Next;
	for (;;) {
		char C = *End++;
		if (!C) {
			Scanner->Next = (Scanner->read)(Scanner->Data);
			++Scanner->Source.Line;
			if (!Scanner->Next) {
				ml_scanner_error(Scanner, "ParseError", "end of input while parsing string");
			}
			End = Scanner->Next;
		} else if (C == '\'') {
			Scanner->Next = End;
			break;
		} else if (C == '{') {
			if (Buffer->Length) {
				mlc_string_part_t *Part = new(mlc_string_part_t);
				Part->Length = Buffer->Length;
				Part->Chars = ml_stringbuffer_get(Buffer);
				Slot[0] = Part;
				Slot = &Part->Next;
			}
			Scanner->Next = End;
			mlc_string_part_t *Part = new(mlc_string_part_t);
			ml_accept_arguments(Scanner, MLT_RIGHT_BRACE, &Part->Child);
			End = Scanner->Next;
			Slot[0] = Part;
			Slot = &Part->Next;
		} else if (C == '\\') {
			C = *End++;
			switch (C) {
			case 'r': ml_stringbuffer_add(Buffer, "\r", 1); break;
			case 'n': ml_stringbuffer_add(Buffer, "\n", 1); break;
			case 't': ml_stringbuffer_add(Buffer, "\t", 1); break;
			case 'e': ml_stringbuffer_add(Buffer, "\e", 1); break;
			case '\'': ml_stringbuffer_add(Buffer, "\'", 1); break;
			case '\"': ml_stringbuffer_add(Buffer, "\"", 1); break;
			case '\\': ml_stringbuffer_add(Buffer, "\\", 1); break;
			case '{': ml_stringbuffer_add(Buffer, "{", 1); break;
			case '\n': break;
			case 0: ml_scanner_error(Scanner, "ParseError", "end of line while parsing string");
			}
		} else {
			ml_stringbuffer_add(Buffer, End - 1, 1);
		}
	}
	if (!Parts) {
		Scanner->Value = ml_stringbuffer_value(Buffer);
		return (Scanner->Token = MLT_VALUE);
	} else {
		if (Buffer->Length) {
			mlc_string_part_t *Part = new(mlc_string_part_t);
			Part->Length = Buffer->Length;
			Part->Chars = ml_stringbuffer_get(Buffer);
			Slot[0] = Part;
		}
		ML_EXPR(Expr, string, string);
		Expr->Parts = Parts;
		Expr->End = Scanner->Source.Line;
		Scanner->Expr = (mlc_expr_t *)Expr;
		return (Scanner->Token = MLT_EXPR);
	}
}

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

static stringmap_t StringFns[1] = {STRINGMAP_INIT};

typedef ml_value_t *(*string_fn_t)(const char *String);

static ml_token_t ml_advance(mlc_scanner_t *Scanner) {
	for (;;) {
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
			Scanner->Next = End;
			if (End[0] == '\"') {
				string_fn_t StringFn = stringmap_search(StringFns, Ident);
				if (!StringFn) ml_scanner_error(Scanner, "ParseError", "Unknown string prefix: %s", Ident);
				Scanner->Next += 1;
				const char *End = Scanner->Next;
				while (End[0] != '\"') {
					if (!End[0]) {
						ml_scanner_error(Scanner, "ParseError", "end of input while parsing string");
					}
					if (End[0] == '\\') ++End;
					++End;
				}
				int Length = End - Scanner->Next;
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
				Scanner->Value = StringFn(Pattern);
				Scanner->Token = MLT_VALUE;
				Scanner->Next = End + 1;
				return Scanner->Token;
			}
			Scanner->Ident = Ident;
			Scanner->Token = MLT_IDENT;
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
			return ml_accept_string(Scanner);
		}
		if (Char == '\"') {
			++Scanner->Next;
			int Length = 0;
			const char *End = Scanner->Next;
			while (End[0] != '\"') {
				if (!End[0]) {
					ml_scanner_error(Scanner, "ParseError", "end of input while parsing string");
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
				Scanner->Token = MLT_IMPORT;
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
			} else if (Scanner->Next[1] == '\"') {
				Scanner->Next += 2;
				int Length = 0;
				const char *End = Scanner->Next;
				while (End[0] != '\"') {
					if (!End[0]) {
						ml_scanner_error(Scanner, "ParseError", "end of input while parsing string");
					}
					if (End[0] == '\\') ++End;
					++Length;
					++End;
				}
				char *Ident = snew(Length + 1), *D = Ident;
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
				Scanner->Ident = Ident;
				Scanner->Token = MLT_METHOD;
				Scanner->Next = End + 1;
				return Scanner->Token;
			} else if (Scanner->Next[1] == '>') {
				Scanner->Next = "\n";
				continue;
			} else if (Scanner->Next[1] == '<') {
				Scanner->Next += 2;
				int Level = 1;
				do {
					if (Scanner->Next[0] == 0) {
						Scanner->Next = (Scanner->read)(Scanner->Data);
						++Scanner->Source.Line;
						if (!Scanner->Next) ml_scanner_error(Scanner, "ParseError", "End of input in comment");
					} else if (Scanner->Next[0] == '>' && Scanner->Next[1] == ':') {
						Scanner->Next += 2;
						--Level;
					} else if (Scanner->Next[0] == ':' && Scanner->Next[1] == '<') {
						Scanner->Next = "";
					} else {
						++Scanner->Next;
					}
				} while (Level);
				continue;
			} else if (Scanner->Next[1] == '(') {
				Scanner->Token = MLT_INLINE;
				Scanner->Next += 2;
				return Scanner->Token;
			} else {
				Scanner->Token = MLT_COLON;
				Scanner->Next += 1;
				return Scanner->Token;
			}
		}
		for (ml_token_t T = MLT_DELIM_FIRST; T <= MLT_DELIM_LAST; ++T) {
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
		ml_scanner_error(Scanner, "ParseError", "unexpected character <%c>", Char);
	}
	return Scanner->Token;
}

static inline ml_token_t ml_next(mlc_scanner_t *Scanner) {
	if (Scanner->Token == MLT_NONE) ml_advance(Scanner);
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
		ml_scanner_error(Scanner, "ParseError", "expected %s not %s (%s)", MLTokens[Token], MLTokens[Scanner->Token], Scanner->Ident);
	} else {
		ml_scanner_error(Scanner, "ParseError", "expected %s not %s", MLTokens[Token], MLTokens[Scanner->Token]);
	}
}

static void ml_accept_eoi(mlc_scanner_t *Scanner) {
	ml_accept(Scanner, MLT_EOI);
}

static mlc_expr_t *ml_parse_factor(mlc_scanner_t *Scanner, int MethDecl);
static mlc_expr_t *ml_parse_term(mlc_scanner_t *Scanner);
static mlc_expr_t *ml_accept_block(mlc_scanner_t *Scanner);

static mlc_expr_t *ml_accept_fun_expr(mlc_scanner_t *Scanner, ml_token_t EndToken) {
	ML_EXPR(FunExpr, fun, fun);
	if (!ml_parse(Scanner, EndToken)) {
		ml_decl_t **ParamSlot = &FunExpr->Params;
		do {
			ml_decl_t *Param = ParamSlot[0] = new(ml_decl_t);
			Param->Source = Scanner->Source;
			ParamSlot = &Param->Next;
			if (ml_parse(Scanner, MLT_LEFT_SQUARE)) {
				ml_accept(Scanner, MLT_IDENT);
				Param->Ident = Scanner->Ident;
				Param->Index = ML_PARAM_EXTRA;
				ml_accept(Scanner, MLT_RIGHT_SQUARE);
				if (ml_parse(Scanner, MLT_COMMA)) {
					ml_accept(Scanner, MLT_LEFT_BRACE);
					ml_decl_t *Param = ParamSlot[0] = new(ml_decl_t);
					Param->Source = Scanner->Source;
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
				if (ml_parse(Scanner, MLT_COLON)) {
					// Parse type specifications but ignore for now
					ml_parse_expression(Scanner, EXPR_DEFAULT);
				}
			}
		} while (ml_parse(Scanner, MLT_COMMA));
		ml_accept(Scanner, EndToken);
	}
	FunExpr->Body = ml_accept_expression(Scanner, EXPR_DEFAULT);
	FunExpr->End = Scanner->Source.Line;
	return (mlc_expr_t *)FunExpr;
}

extern ml_value_t MLMethodSet[];

static mlc_expr_t *ml_accept_meth_expr(mlc_scanner_t *Scanner) {
	ML_EXPR(MethodExpr, parent_value, const_call);
	MethodExpr->Value = MLMethodSet;
	mlc_expr_t *Method = ml_parse_factor(Scanner, 1);
	if (!Method) ml_scanner_error(Scanner, "ParseError", "expected <factor> not <%s>", MLTokens[Scanner->Token]);
	MethodExpr->Child = Method;
	mlc_expr_t **ArgsSlot = &Method->Next;
	ml_accept(Scanner, MLT_LEFT_PAREN);
	ML_EXPR(FunExpr, fun, fun);
	if (!ml_parse(Scanner, MLT_RIGHT_PAREN)) {
		ml_decl_t **ParamSlot = &FunExpr->Params;
		do {
			ml_decl_t *Param = ParamSlot[0] = new(ml_decl_t);
			Param->Source = Scanner->Source;
			ParamSlot = &Param->Next;
			ml_accept(Scanner, MLT_IDENT);
			Param->Ident = Scanner->Ident;
			ml_accept(Scanner, MLT_COLON);
			mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
			ArgsSlot = &Arg->Next;
		} while (ml_parse(Scanner, MLT_COMMA));
		ml_accept(Scanner, MLT_RIGHT_PAREN);
	}
	if (ml_parse(Scanner, MLT_ASSIGN)) {
		ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
	} else {
		FunExpr->Body = ml_accept_expression(Scanner, EXPR_DEFAULT);
		FunExpr->End = Scanner->Source.Line;
		ArgsSlot[0] = (mlc_expr_t *)FunExpr;
	}
	return (mlc_expr_t *)MethodExpr;
}

static void ml_accept_named_arguments(mlc_scanner_t *Scanner, ml_token_t EndToken, mlc_expr_t **ArgsSlot, ml_value_t *Names) {
	mlc_expr_t **NamesSlot = ArgsSlot;
	mlc_expr_t *Arg = ArgsSlot[0];
	ArgsSlot = &Arg->Next;
	if (ml_parse(Scanner, MLT_SEMICOLON)) {
		ArgsSlot[0] = ml_accept_fun_expr(Scanner, EndToken);
		return;
	}
	Arg = ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
	ArgsSlot = &Arg->Next;
	while (ml_parse(Scanner, MLT_COMMA)) {
		if (ml_parse(Scanner, MLT_IDENT) || ml_parse(Scanner, MLT_METHOD)) {
			ml_names_add(Names, ml_method(Scanner->Ident));
		} else {
			ml_scanner_error(Scanner, "ParseError", "Argument names must be identifiers or methods");
		}
		ml_accept(Scanner, MLT_IS);
		if (ml_parse(Scanner, MLT_SEMICOLON)) {
			ArgsSlot[0] = ml_accept_fun_expr(Scanner, EndToken);
			return;
		}
		Arg = ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
		ArgsSlot = &Arg->Next;
	}
	if (ml_parse(Scanner, MLT_SEMICOLON)) {
		mlc_expr_t *FunExpr = ml_accept_fun_expr(Scanner, EndToken);
		FunExpr->Next = NamesSlot[0];
		NamesSlot[0] = FunExpr;
	} else {
		ml_accept(Scanner, EndToken);
	}
}

static void ml_accept_arguments(mlc_scanner_t *Scanner, ml_token_t EndToken, mlc_expr_t **ArgsSlot) {
	while (ml_parse(Scanner, MLT_EOL));
	if (ml_parse(Scanner, MLT_SEMICOLON)) {
		ArgsSlot[0] = ml_accept_fun_expr(Scanner, EndToken);
	} else if (!ml_parse(Scanner, EndToken)) {
		do {
			mlc_expr_t *Arg = ml_accept_expression(Scanner, EXPR_DEFAULT);
			if (ml_parse(Scanner, MLT_IS)) {
				ml_value_t *Names = ml_names();
				if (Arg->compile == (void *)ml_ident_expr_compile) {
					ml_names_add(Names, ml_method(((mlc_ident_expr_t *)Arg)->Ident));
				} else if (Arg->compile == (void *)ml_value_expr_compile) {
					ml_value_t *Name = ((mlc_value_expr_t *)Arg)->Value;
					if (ml_typeof(Name) != MLMethodT) {
						ml_scanner_error(Scanner, "ParseError", "Argument names must be identifiers or methods");
					}
					ml_names_add(Names, Name);
				} else {
					ml_scanner_error(Scanner, "ParseError", "Argument names must be identifiers or methods");
				}
				ML_EXPR(NamesArg, value, value);
				NamesArg->Value = Names;
				ArgsSlot[0] = (mlc_expr_t *)NamesArg;
				return ml_accept_named_arguments(Scanner, EndToken, ArgsSlot, Names);
			} else {
				ArgsSlot[0] = Arg;
				ArgsSlot = &Arg->Next;
			}
		} while (ml_parse(Scanner, MLT_COMMA));
		if (ml_parse(Scanner, MLT_SEMICOLON)) {
			ArgsSlot[0] = ml_accept_fun_expr(Scanner, EndToken);
		} else {
			ml_accept(Scanner, EndToken);
		}
		return;
	}
}

static mlc_expr_t *ml_accept_with_expr(mlc_scanner_t *Scanner, mlc_expr_t *Child) {
	ML_EXPR(WithExpr, decl, with);
	ml_decl_t **DeclSlot = &WithExpr->Decl;
	mlc_expr_t **ExprSlot = &WithExpr->Child;
	do {
		if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
			int Count = 0;
			ml_decl_t **First = DeclSlot;
			do {
				ml_accept(Scanner, MLT_IDENT);
				++Count;
				ml_decl_t *Decl = DeclSlot[0] = new(ml_decl_t);
				Decl->Source = Scanner->Source;
				Decl->Ident = Scanner->Ident;
				DeclSlot = &Decl->Next;
			} while (ml_parse(Scanner, MLT_COMMA));
			ml_accept(Scanner, MLT_RIGHT_PAREN);
			First[0]->Index = Count;
		} else {
			ml_accept(Scanner, MLT_IDENT);
			ml_decl_t *Decl = DeclSlot[0] = new(ml_decl_t);
			Decl->Source = Scanner->Source;
			DeclSlot = &Decl->Next;
			Decl->Ident = Scanner->Ident;
			Decl->Index = 1;
		}
		ml_accept(Scanner, MLT_ASSIGN);
		mlc_expr_t *Expr = ExprSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
		ExprSlot = &Expr->Next;
	} while (ml_parse(Scanner, MLT_COMMA));
	if (Child) {
		ExprSlot[0] = Child;
	} else {
		ml_accept(Scanner, MLT_DO);
		ExprSlot[0] = ml_accept_block(Scanner);
		ml_accept(Scanner, MLT_END);
		WithExpr->End = Scanner->Source.Line;
	}
	return (mlc_expr_t *)WithExpr;
}

static mlc_expr_t *ml_parse_factor(mlc_scanner_t *Scanner, int MethDecl) {
	static void *CompileFns[] = {
		[MLT_EACH] = ml_each_expr_compile,
		[MLT_NOT] = ml_not_expr_compile,
		[MLT_WHILE] = ml_while_expr_compile,
		[MLT_UNTIL] = ml_until_expr_compile,
		[MLT_EXIT] = ml_exit_expr_compile,
		[MLT_RET] = ml_return_expr_compile,
		[MLT_NEXT] = ml_next_expr_compile,
		[MLT_NIL] = ml_nil_expr_compile,
		[MLT_BLANK] = ml_blank_expr_compile,
		[MLT_OLD] = ml_old_expr_compile
	};
	switch (ml_next(Scanner)) {
	case MLT_EACH:
	case MLT_NOT:
	case MLT_WHILE:
	case MLT_UNTIL:
	{
		mlc_parent_expr_t *ParentExpr = new(mlc_parent_expr_t);
		ParentExpr->compile = CompileFns[Scanner->Token];
		ParentExpr->Source = Scanner->Source;
		Scanner->Token = MLT_NONE;
		ParentExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
		ParentExpr->End = Scanner->Source.Line;
		return (mlc_expr_t *)ParentExpr;
	}
	case MLT_EXIT:
	case MLT_RET:
	{
		mlc_parent_expr_t *ParentExpr = new(mlc_parent_expr_t);
		ParentExpr->compile = CompileFns[Scanner->Token];
		ParentExpr->Source = Scanner->Source;
		Scanner->Token = MLT_NONE;
		ParentExpr->Child = ml_parse_expression(Scanner, EXPR_DEFAULT);
		ParentExpr->End = Scanner->Source.Line;
		return (mlc_expr_t *)ParentExpr;
	}
	case MLT_NEXT:
	case MLT_NIL:
	case MLT_BLANK:
	case MLT_OLD:
	{
		mlc_expr_t *Expr = new(mlc_expr_t);
		Expr->compile = CompileFns[Scanner->Token];
		Expr->Source = Scanner->Source;
		Scanner->Token = MLT_NONE;
		Expr->End = Scanner->Source.Line;
		return Expr;
	}
	case MLT_DO: {
		Scanner->Token = MLT_NONE;
		mlc_expr_t *BlockExpr = ml_accept_block(Scanner);
		ml_accept(Scanner, MLT_END);
		BlockExpr->End = Scanner->Source.Line;
		return BlockExpr;
	}
	case MLT_IF: {
		Scanner->Token = MLT_NONE;
		ML_EXPR(IfExpr, if, if);
		mlc_if_case_t **CaseSlot = &IfExpr->Cases;
		do {
			mlc_if_case_t *Case = CaseSlot[0] = new(mlc_if_case_t);
			CaseSlot = &Case->Next;
			Case->Source = Scanner->Source;
			if (ml_parse(Scanner, MLT_VAR)) {
				ml_decl_t *Decl = new(ml_decl_t);
				Decl->Source = Scanner->Source;
				ml_accept(Scanner, MLT_IDENT);
				Decl->Ident = Scanner->Ident;
				Decl->Index = 1;
				ml_accept(Scanner, MLT_ASSIGN);
				Case->Decl = Decl;
			} else if (ml_parse(Scanner, MLT_LET)) {
				ml_decl_t *Decl = new(ml_decl_t);
				Decl->Source = Scanner->Source;
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
		IfExpr->End = Scanner->Source.Line;
		return (mlc_expr_t *)IfExpr;
	}
	case MLT_LOOP: {
		Scanner->Token = MLT_NONE;
		ML_EXPR(LoopExpr, parent, loop);
		LoopExpr->Child = ml_accept_block(Scanner);
		ml_accept(Scanner, MLT_END);
		LoopExpr->End = Scanner->Source.Line;
		return (mlc_expr_t *)LoopExpr;
	}
	case MLT_FOR: {
		Scanner->Token = MLT_NONE;
		ML_EXPR(ForExpr, decl, for);
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source = Scanner->Source;
		ml_accept(Scanner, MLT_IDENT);
		Decl->Ident = Scanner->Ident;
		if (ml_parse(Scanner, MLT_COMMA)) {
			ml_accept(Scanner, MLT_IDENT);
			ml_decl_t *Decl2 = new(ml_decl_t);
			Decl2->Source = Scanner->Source;
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
		ForExpr->End = Scanner->Source.Line;
		return (mlc_expr_t *)ForExpr;
	}
	case MLT_FUN: {
		Scanner->Token = MLT_NONE;
		if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
			return ml_accept_fun_expr(Scanner, MLT_RIGHT_PAREN);
		} else {
			ML_EXPR(FunExpr, fun, fun);
			FunExpr->Body = ml_accept_expression(Scanner, EXPR_DEFAULT);
			FunExpr->End = Scanner->Source.Line;
			return (mlc_expr_t *)FunExpr;
		}
	}
	case MLT_METH: {
		Scanner->Token = MLT_NONE;
		return ml_accept_meth_expr(Scanner);
	}
	case MLT_SUSP: {
		Scanner->Token = MLT_NONE;
		ML_EXPR(SuspendExpr, parent, suspend);
		SuspendExpr->Child = ml_parse_expression(Scanner, EXPR_DEFAULT);
		if (ml_parse(Scanner, MLT_COMMA)) {
			SuspendExpr->Child->Next = ml_accept_expression(Scanner, EXPR_DEFAULT);
		}
		SuspendExpr->End = Scanner->Source.Line;
		return (mlc_expr_t *)SuspendExpr;
	}
	case MLT_WITH: {
		Scanner->Token = MLT_NONE;
		return ml_accept_with_expr(Scanner, NULL);
	}
	case MLT_IDENT: {
		Scanner->Token = MLT_NONE;
		ML_EXPR(IdentExpr, ident, ident);
		IdentExpr->Ident = Scanner->Ident;
		IdentExpr->End = Scanner->Source.Line;
		return (mlc_expr_t *)IdentExpr;
	}
	case MLT_VALUE: {
		Scanner->Token = MLT_NONE;
		ML_EXPR(ValueExpr, value, value);
		ValueExpr->Value = Scanner->Value;
		ValueExpr->End = Scanner->Source.Line;
		return (mlc_expr_t *)ValueExpr;
	}
	case MLT_EXPR: {
		Scanner->Token = MLT_NONE;
		return Scanner->Expr;
	}
	case MLT_INLINE: {
		Scanner->Token = MLT_NONE;
		ML_EXPR(InlineExpr, parent, inline);
		InlineExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
		ml_accept(Scanner, MLT_RIGHT_PAREN);
		InlineExpr->End = Scanner->Source.Line;
		return (mlc_expr_t *)InlineExpr;
	}
	case MLT_LEFT_PAREN: {
		Scanner->Token = MLT_NONE;
		if (ml_parse(Scanner, MLT_SEMICOLON)) {
			ML_EXPR(TupleExpr, parent, tuple);
			TupleExpr->Child = ml_accept_fun_expr(Scanner, MLT_RIGHT_PAREN);
			TupleExpr->End = Scanner->Source.Line;
			return (mlc_expr_t *)TupleExpr;
		}
		mlc_expr_t *Expr = ml_accept_expression(Scanner, EXPR_DEFAULT);
		if (ml_parse(Scanner, MLT_COMMA)) {
			ML_EXPR(TupleExpr, parent, tuple);
			TupleExpr->Child = Expr;
			ml_accept_arguments(Scanner, MLT_RIGHT_PAREN, &Expr->Next);
			Expr = (mlc_expr_t *)TupleExpr;
		} else if (ml_parse(Scanner, MLT_SEMICOLON)) {
			ML_EXPR(TupleExpr, parent, tuple);
			TupleExpr->Child = Expr;
			Expr->Next = ml_accept_fun_expr(Scanner, MLT_RIGHT_PAREN);
			Expr = (mlc_expr_t *)TupleExpr;
		} else {
			ml_accept(Scanner, MLT_RIGHT_PAREN);
		}
		Expr->End = Scanner->Source.Line;
		return Expr;
	}
	case MLT_LEFT_SQUARE: {
		Scanner->Token = MLT_NONE;
		while (ml_parse(Scanner, MLT_EOL));
		ML_EXPR(ListExpr, parent, list);
		mlc_expr_t **ArgsSlot = &ListExpr->Child;
		if (!ml_parse(Scanner, MLT_RIGHT_SQUARE)) {
			do {
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
			} while (ml_parse(Scanner, MLT_COMMA));
			ml_accept(Scanner, MLT_RIGHT_SQUARE);
		}
		ListExpr->End = Scanner->Source.Line;
		return (mlc_expr_t *)ListExpr;
	}
	case MLT_LEFT_BRACE: {
		Scanner->Token = MLT_NONE;
		while (ml_parse(Scanner, MLT_EOL));
		ML_EXPR(MapExpr, parent, map);
		mlc_expr_t **ArgsSlot = &MapExpr->Child;
		if (!ml_parse(Scanner, MLT_RIGHT_BRACE)) {
			do {
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
				if (ml_parse(Scanner, MLT_IS)) {
					mlc_expr_t *ArgExpr = ArgsSlot[0] = ml_accept_expression(Scanner, EXPR_DEFAULT);
					ArgsSlot = &ArgExpr->Next;
				} else {
					ML_EXPR(ArgExpr, value, value);
					ArgExpr->Value = MLSome;
					ArgsSlot[0] = (mlc_expr_t *)ArgExpr;
					ArgsSlot = &ArgExpr->Next;
				}
			} while (ml_parse(Scanner, MLT_COMMA));
			ml_accept(Scanner, MLT_RIGHT_BRACE);
		}
		MapExpr->End = Scanner->Source.Line;
		return (mlc_expr_t *)MapExpr;
	}
	case MLT_OPERATOR: {
		Scanner->Token = MLT_NONE;
		ml_value_t *Operator = ml_method(Scanner->Ident);
		if (MethDecl) {
			ML_EXPR(ValueExpr, value, value);
			ValueExpr->Value = Operator;
			ValueExpr->End = Scanner->Source.Line;
			return (mlc_expr_t *)ValueExpr;
		} else if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
			ML_EXPR(CallExpr, parent_value, const_call);
			CallExpr->Value = Operator;
			ml_accept_arguments(Scanner, MLT_RIGHT_PAREN, &CallExpr->Child);
			return (mlc_expr_t *)CallExpr;
		} else {
			mlc_expr_t *Child = ml_parse_term(Scanner);
			if (Child) {
				ML_EXPR(CallExpr, parent_value, const_call);
				CallExpr->Value = Operator;
				CallExpr->Child = Child;
				return (mlc_expr_t *)CallExpr;
			} else {
				ML_EXPR(ValueExpr, value, value);
				ValueExpr->Value = Operator;
				ValueExpr->End = Scanner->Source.Line;
				return (mlc_expr_t *)ValueExpr;
			}
		}
	}
	case MLT_METHOD: {
		Scanner->Token = MLT_NONE;
		ML_EXPR(ValueExpr, value, value);
		ValueExpr->Value = ml_method(Scanner->Ident);
		ValueExpr->End = Scanner->Source.Line;
		return (mlc_expr_t *)ValueExpr;
	}
	default: return NULL;
	}
}

static mlc_expr_t *ml_parse_term(mlc_scanner_t *Scanner) {
	mlc_expr_t *Expr = ml_parse_factor(Scanner, 0);
	if (!Expr) return NULL;
	for (;;) {
		switch (ml_next(Scanner)) {
		case MLT_LEFT_PAREN: {
			Scanner->Token = MLT_NONE;
			ML_EXPR(CallExpr, parent, call);
			CallExpr->Child = Expr;
			ml_accept_arguments(Scanner, MLT_RIGHT_PAREN, &Expr->Next);
			Expr = (mlc_expr_t *)CallExpr;
			break;
		}
		case MLT_LEFT_SQUARE: {
			Scanner->Token = MLT_NONE;
			while (ml_parse(Scanner, MLT_EOL));
			ML_EXPR(IndexExpr, parent_value, const_call);
			IndexExpr->Value = IndexMethod;
			IndexExpr->Child = Expr;
			ml_accept_arguments(Scanner, MLT_RIGHT_SQUARE, &Expr->Next);
			Expr = (mlc_expr_t *)IndexExpr;
			break;
		}
		case MLT_METHOD: {
			Scanner->Token = MLT_NONE;
			ML_EXPR(CallExpr, parent_value, const_call);
			CallExpr->Value = ml_method(Scanner->Ident);
			CallExpr->Child = Expr;
			if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
				ml_accept_arguments(Scanner, MLT_RIGHT_PAREN, &Expr->Next);
			}
			Expr = (mlc_expr_t *)CallExpr;
			break;
		}
		case MLT_IMPORT: {
			Scanner->Token = MLT_NONE;
			if (!ml_parse(Scanner, MLT_OPERATOR) && !ml_parse(Scanner, MLT_IDENT)) {
				ml_accept(Scanner, MLT_VALUE);
				if (!ml_is(Scanner->Value, MLStringT)) {
					ml_scanner_error(Scanner, "ParseError", "expected import not %s", MLTokens[Scanner->Token]);
				}
				Scanner->Ident = ml_string_value(Scanner->Value);
			}
			ML_EXPR(ImportExpr, parent_value, import);
			ImportExpr->Value = ml_string(Scanner->Ident, -1);
			ImportExpr->Child = Expr;
			Expr = (mlc_expr_t *)ImportExpr;
			break;
		}
		default: {
			Expr->End = Scanner->Source.Line;
			return Expr;
		}
		}
	}
	return NULL; // Unreachable
}

static mlc_expr_t *ml_accept_term(mlc_scanner_t *Scanner) {
	while (ml_parse(Scanner, MLT_EOL));
	mlc_expr_t *Expr = ml_parse_term(Scanner);
	if (Expr) return Expr;
	ml_scanner_error(Scanner, "ParseError", "expected <expression> not %s", MLTokens[Scanner->Token]);
}

static mlc_expr_t *ml_parse_expression(mlc_scanner_t *Scanner, ml_expr_level_t Level) {
	mlc_expr_t *Expr = ml_parse_term(Scanner);
	if (!Expr) return NULL;
	for (;;) switch (ml_next(Scanner)) {
	case MLT_OPERATOR: case MLT_IDENT: {
		Scanner->Token = MLT_NONE;
		ML_EXPR(CallExpr, parent_value, const_call);
		CallExpr->Value = ml_method(Scanner->Ident);
		CallExpr->Child = Expr;
		if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
			ml_accept_arguments(Scanner, MLT_RIGHT_PAREN, &Expr->Next);
		} else {
			Expr->Next = ml_accept_term(Scanner);
		}
		Expr = (mlc_expr_t *)CallExpr;
		break;
	}
	case MLT_ASSIGN: {
		Scanner->Token = MLT_NONE;
		ML_EXPR(AssignExpr, parent, assign);
		AssignExpr->Child = Expr;
		Expr->Next = ml_accept_expression(Scanner, EXPR_DEFAULT);
		Expr = (mlc_expr_t *)AssignExpr;
		break;
	}
	case MLT_IN: {
		Scanner->Token = MLT_NONE;
		ML_EXPR(CallExpr, parent_value, const_call);
		CallExpr->Value = ml_method("in");
		CallExpr->Child = Expr;
		Expr->Next = ml_accept_expression(Scanner, EXPR_SIMPLE);
		Expr = (mlc_expr_t *)CallExpr;
		break;
	}
	default: goto done;
	}
done:
	if (Level >= EXPR_AND && ml_parse(Scanner, MLT_AND)) {
		ML_EXPR(AndExpr, parent, and);
		mlc_expr_t *LastChild = AndExpr->Child = Expr;
		do {
			LastChild = LastChild->Next = ml_accept_expression(Scanner, EXPR_SIMPLE);
		} while (ml_parse(Scanner, MLT_AND));
		Expr = (mlc_expr_t *)AndExpr;
	}
	if (Level >= EXPR_OR && ml_parse(Scanner, MLT_OR)) {
		ML_EXPR(OrExpr, parent, or);
		mlc_expr_t *LastChild = OrExpr->Child = Expr;
		do {
			LastChild = LastChild->Next = ml_accept_expression(Scanner, EXPR_AND);
		} while (ml_parse(Scanner, MLT_OR));
		Expr = (mlc_expr_t *)OrExpr;
	}
	if (Level >= EXPR_FOR) {
		if (ml_parse(Scanner, MLT_WITH)) {
			Expr = ml_accept_with_expr(Scanner, Expr);
		}
		int IsComprehension = 0;
		if (ml_parse(Scanner, MLT_TO)) {
			Expr->Next = ml_accept_expression(Scanner, EXPR_OR);
			ml_accept(Scanner, MLT_FOR);
			IsComprehension = 1;
		} else {
			IsComprehension = ml_parse(Scanner, MLT_FOR);
		}
		if (IsComprehension) {
			ML_EXPR(FunExpr, fun, fun);
			ML_EXPR(SuspendExpr, parent, suspend);
			SuspendExpr->Child = Expr;
			mlc_expr_t *Body = (mlc_expr_t *)SuspendExpr;
			do {
				ML_EXPR(ForExpr, decl, for);
				ml_decl_t *Decl = new(ml_decl_t);
				Decl->Source = Scanner->Source;
				ml_accept(Scanner, MLT_IDENT);
				Decl->Ident = Scanner->Ident;
				if (ml_parse(Scanner, MLT_COMMA)) {
					ml_accept(Scanner, MLT_IDENT);
					ml_decl_t *Decl2 = new(ml_decl_t);
					Decl2->Source = Scanner->Source;
					Decl2->Ident = Scanner->Ident;
					Decl->Next = Decl2;
				}
				ForExpr->Decl = Decl;
				ml_accept(Scanner, MLT_IN);
				ForExpr->Child = ml_accept_expression(Scanner, EXPR_OR);
				for (;;) {
					if (ml_parse(Scanner, MLT_IF)) {
						ML_EXPR(IfExpr, if, if);
						mlc_if_case_t *IfCase = IfExpr->Cases = new(mlc_if_case_t);
						IfCase->Condition = ml_accept_expression(Scanner, EXPR_OR);
						IfCase->Body = Body;
						Body = (mlc_expr_t *)IfExpr;
					} else if (ml_parse(Scanner, MLT_WITH)) {
						Body = ml_accept_with_expr(Scanner, Body);
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
	Expr->End = Scanner->Source.Line;
	return Expr;
}

static mlc_expr_t *ml_accept_expression(mlc_scanner_t *Scanner, ml_expr_level_t Level) {
	while (ml_parse(Scanner, MLT_EOL));
	mlc_expr_t *Expr = ml_parse_expression(Scanner, Level);
	if (Expr) return Expr;
	ml_scanner_error(Scanner, "ParseError", "expected <expression> not %s", MLTokens[Scanner->Token]);
}

static mlc_expr_t *ml_accept_block(mlc_scanner_t *Scanner) {
	ML_EXPR(BlockExpr, block, block);
	mlc_expr_t **ExprSlot = &BlockExpr->Child;
	ml_decl_t **VarsSlot = &BlockExpr->Vars;
	ml_decl_t **LetsSlot = &BlockExpr->Lets;
	ml_decl_t **DefsSlot = &BlockExpr->Defs;
	for (;;) {
		while (ml_parse(Scanner, MLT_EOL));
		switch (ml_next(Scanner)) {
		case MLT_VAR: {
			Scanner->Token = MLT_NONE;
			do {
				if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
					int Count = 0;
					ml_decl_t *Decl;
					do {
						ml_accept(Scanner, MLT_IDENT);
						++Count;
						Decl = VarsSlot[0] = new(ml_decl_t);
						Decl->Source = Scanner->Source;
						Decl->Ident = Scanner->Ident;
						VarsSlot = &Decl->Next;
					} while (ml_parse(Scanner, MLT_COMMA));
					ml_accept(Scanner, MLT_RIGHT_PAREN);
					ML_EXPR(DeclExpr, decl, varx);
					DeclExpr->Decl = Decl;
					DeclExpr->Count = Count;
					ml_accept(Scanner, MLT_ASSIGN);
					DeclExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
					ExprSlot[0] = (mlc_expr_t *)DeclExpr;
					ExprSlot = &DeclExpr->Next;
				} else {
					ml_accept(Scanner, MLT_IDENT);
					ml_decl_t *Decl = VarsSlot[0] = new(ml_decl_t);
					Decl->Source = Scanner->Source;
					Decl->Ident = Scanner->Ident;
					VarsSlot = &Decl->Next;
					mlc_expr_t *Child = NULL;
					if (ml_parse(Scanner, MLT_ASSIGN)) {
						Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
					} else if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
						Child = ml_accept_fun_expr(Scanner, MLT_RIGHT_PAREN);
					}
					if (Child) {
						ML_EXPR(DeclExpr, decl, var);
						DeclExpr->Decl = Decl;
						DeclExpr->Child = Child;
						ExprSlot[0] = (mlc_expr_t *)DeclExpr;
						ExprSlot = &DeclExpr->Next;
					}
				}
			} while (ml_parse(Scanner, MLT_COMMA));
			break;
		}
		case MLT_LET: {
			Scanner->Token = MLT_NONE;
			do {
				if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
					int Count = 0;
					ml_decl_t *Decl;
					do {
						ml_accept(Scanner, MLT_IDENT);
						++Count;
						Decl = LetsSlot[0] = new(ml_decl_t);
						Decl->Source = Scanner->Source;
						Decl->Ident = Scanner->Ident;
						Decl->Flags = MLC_DECL_FORWARD;
						LetsSlot = &Decl->Next;
					} while (ml_parse(Scanner, MLT_COMMA));
					ml_accept(Scanner, MLT_RIGHT_PAREN);
					ML_EXPR(DeclExpr, decl, letx);
					DeclExpr->Decl = Decl;
					DeclExpr->Count = Count;
					ml_accept(Scanner, MLT_ASSIGN);
					DeclExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
					ExprSlot[0] = (mlc_expr_t *)DeclExpr;
					ExprSlot = &DeclExpr->Next;
				} else {
					ml_accept(Scanner, MLT_IDENT);
					ml_decl_t *Decl = LetsSlot[0] = new(ml_decl_t);
					Decl->Source = Scanner->Source;
					Decl->Ident = Scanner->Ident;
					Decl->Flags = MLC_DECL_FORWARD;
					LetsSlot = &Decl->Next;
					ML_EXPR(DeclExpr, decl, let);
					DeclExpr->Decl = Decl;
					if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
						DeclExpr->Child = ml_accept_fun_expr(Scanner, MLT_RIGHT_PAREN);
					} else {
						ml_accept(Scanner, MLT_ASSIGN);
						DeclExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
					}
					ExprSlot[0] = (mlc_expr_t *)DeclExpr;
					ExprSlot = &DeclExpr->Next;
				}
			} while (ml_parse(Scanner, MLT_COMMA));
			break;
		}
		case MLT_DEF: {
			Scanner->Token = MLT_NONE;
			do {
				if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
					int Count = 0;
					ml_decl_t *Decl;
					do {
						ml_accept(Scanner, MLT_IDENT);
						++Count;
						Decl = DefsSlot[0] = new(ml_decl_t);
						Decl->Source = Scanner->Source;
						Decl->Ident = Scanner->Ident;
						DefsSlot = &Decl->Next;
					} while (ml_parse(Scanner, MLT_COMMA));
					ml_accept(Scanner, MLT_RIGHT_PAREN);
					ML_EXPR(DeclExpr, decl, defx);
					DeclExpr->Decl = Decl;
					DeclExpr->Count = Count;
					ml_accept(Scanner, MLT_ASSIGN);
					DeclExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
					ExprSlot[0] = (mlc_expr_t *)DeclExpr;
					ExprSlot = &DeclExpr->Next;
				} else {
					ml_accept(Scanner, MLT_IDENT);
					ml_decl_t *Decl = DefsSlot[0] = new(ml_decl_t);
					Decl->Source = Scanner->Source;
					Decl->Ident = Scanner->Ident;
					DefsSlot = &Decl->Next;
					ML_EXPR(DeclExpr, decl, def);
					DeclExpr->Count = 0;
					if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
						DeclExpr->Child = ml_accept_fun_expr(Scanner, MLT_RIGHT_PAREN);
					} else if (ml_parse(Scanner, MLT_ASSIGN)) {
						DeclExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
					} else {
						DeclExpr->Count = 1;
						while (ml_parse(Scanner, MLT_COMMA)) {
							ml_accept(Scanner, MLT_IDENT);
							Decl = DefsSlot[0] = new(ml_decl_t);
							Decl->Source = Scanner->Source;
							Decl->Ident = Scanner->Ident;
							DefsSlot = &Decl->Next;
							++DeclExpr->Count;
						}
						ml_accept(Scanner, MLT_IN);
						DeclExpr->Child = ml_accept_expression(Scanner, EXPR_DEFAULT);
					}
					DeclExpr->Decl = Decl;
					ExprSlot[0] = (mlc_expr_t *)DeclExpr;
					ExprSlot = &DeclExpr->Next;
				}
			} while (ml_parse(Scanner, MLT_COMMA));
			break;
		}
		case MLT_FUN: {
			Scanner->Token = MLT_NONE;
			if (ml_parse(Scanner, MLT_IDENT)) {
				ml_decl_t *Decl = LetsSlot[0] = new(ml_decl_t);
				Decl->Source = Scanner->Source;
				Decl->Ident = Scanner->Ident;
				Decl->Flags = MLC_DECL_FORWARD;
				LetsSlot = &Decl->Next;
				ml_accept(Scanner, MLT_LEFT_PAREN);
				ML_EXPR(DeclExpr, decl, let);
				DeclExpr->Decl = Decl;
				DeclExpr->Child = ml_accept_fun_expr(Scanner, MLT_RIGHT_PAREN);
				ExprSlot[0] = (mlc_expr_t *)DeclExpr;
				ExprSlot = &DeclExpr->Next;
			} else {
				ml_accept(Scanner, MLT_LEFT_PAREN);
				mlc_expr_t *Expr = ml_accept_fun_expr(Scanner, MLT_RIGHT_PAREN);
				ExprSlot[0] = Expr;
				ExprSlot = &Expr->Next;
			}
			break;
		}
		case MLT_ON: {
			Scanner->Token = MLT_NONE;
			if (BlockExpr->CatchDecl) {
				ml_scanner_error(Scanner, "ParseError", "no more than one error handler allowed in a block");
			}
			ml_accept(Scanner, MLT_IDENT);
			ml_decl_t *Decl = new(ml_decl_t);
			Decl->Source = Scanner->Source;
			Decl->Ident = Scanner->Ident;
			BlockExpr->CatchDecl = Decl;
			ml_accept(Scanner, MLT_DO);
			BlockExpr->Catch = ml_accept_block(Scanner);
			goto end;
		}
		default: {
			mlc_expr_t *Expr = ml_parse_expression(Scanner, EXPR_DEFAULT);
			if (!Expr) goto end;
			if (ml_parse(Scanner, MLT_COLON)) {
				ml_accept(Scanner, MLT_IDENT);
				ml_decl_t *Decl = DefsSlot[0] = new(ml_decl_t);
				Decl->Source = Scanner->Source;
				Decl->Ident = Scanner->Ident;
				Decl->Flags = MLC_DECL_CONSTANT;
				DefsSlot = &Decl->Next;
				ML_EXPR(DeclExpr, decl, def);
				DeclExpr->Decl = Decl;
				ml_accept(Scanner, MLT_LEFT_PAREN);
				ml_accept_arguments(Scanner, MLT_RIGHT_PAREN, &Expr->Next);
				ML_EXPR(CallExpr, parent, call);
				CallExpr->Child = Expr;
				DeclExpr->Child = (mlc_expr_t *)CallExpr;
				Expr = (mlc_expr_t *)DeclExpr;
			}
			ExprSlot[0] = Expr;
			ExprSlot = &Expr->Next;
			break;
		}
		}
		if (ml_parse(Scanner, MLT_SEMICOLON)) continue;
		if (ml_parse(Scanner, MLT_EOL)) continue;
		break;
	}
end:
	BlockExpr->End = Scanner->Source.Line;
	return (mlc_expr_t *)BlockExpr;
}

void ml_function_compile(ml_state_t *Caller, mlc_scanner_t *Scanner, const char **Parameters) {
	MLC_ON_ERROR(Scanner->Context) ML_RETURN(Scanner->Context->Error);
	mlc_expr_t *Block = ml_accept_block(Scanner);
	ml_accept_eoi(Scanner);
	ml_value_t *Function = ml_compile(Block, Parameters, Scanner->Context);
	ML_RETURN(Function);
}

void ml_command_evaluate(ml_state_t *Caller, mlc_scanner_t *Scanner, stringmap_t *Vars) {
	MLC_ON_ERROR(Scanner->Context) {
		ml_scanner_reset(Scanner);
		ML_RETURN(Scanner->Context->Error);
	}
	while (ml_parse(Scanner, MLT_EOL));
	if (ml_parse(Scanner, MLT_EOI)) ML_RETURN(NULL);
	ml_value_t *Result = NULL;
	if (ml_parse(Scanner, MLT_VAR)) {
		do {
			ml_accept(Scanner, MLT_IDENT);
			const char *Ident = Scanner->Ident;
			ml_variable_t *Var = new(ml_variable_t);
			Var->Type = MLVariableT;
			Var->Value = MLNil;
			ml_value_t **Slot = (ml_value_t **)stringmap_slot(Vars, Ident);
			if (Slot[0] && ml_typeof(Slot[0]) == MLUninitializedT) {
				ml_uninitialized_set(Slot[0], (ml_value_t *)Var);
			}
			Slot[0] = (ml_value_t *)Var;
			if (ml_parse(Scanner, MLT_ASSIGN)) {
				mlc_expr_t *Expr = ml_accept_expression(Scanner, EXPR_DEFAULT);
				Result = ml_compile(Expr, NULL, Scanner->Context);
				if (ml_is_error(Result)) ML_RETURN(Result);
				Result = ml_simple_call(Result, 0, NULL);
				if (ml_is_error(Result)) ML_RETURN(Result);
				Var->Value = Result;
			} else {
				Result = MLNil;
			}
		} while (ml_parse(Scanner, MLT_COMMA));
	} else if (ml_parse(Scanner, MLT_LET) || ml_parse(Scanner, MLT_DEF)) {
		do {
			ml_accept(Scanner, MLT_IDENT);
			const char *Ident = Scanner->Ident;
			mlc_expr_t *Expr;
			ml_decl_t *Import = NULL;
			if (ml_parse(Scanner, MLT_LEFT_PAREN)) {
				Expr = ml_accept_fun_expr(Scanner, MLT_RIGHT_PAREN);
			} else if (ml_parse(Scanner, MLT_ASSIGN)) {
				Expr = ml_accept_expression(Scanner, EXPR_DEFAULT);
			} else {
				Import = new(ml_decl_t);
				Import->Source = Scanner->Source;
				Import->Ident = Scanner->Ident;
				while (ml_parse(Scanner, MLT_COMMA)) {
					ml_accept(Scanner, MLT_IDENT);
					ml_decl_t *Decl = new(ml_decl_t);
					Decl->Source = Scanner->Source;
					Decl->Ident = Scanner->Ident;
					Decl->Next = Import;
					Import = Decl;
				}
				ml_accept(Scanner, MLT_IN);
				Expr = ml_accept_expression(Scanner, EXPR_DEFAULT);
			}
			if (!Import) {
				ml_value_t **Slot = (ml_value_t **)stringmap_slot(Vars, Ident);
				if (!Slot[0] || ml_typeof(Slot[0]) != MLUninitializedT) {
					Slot[0] = ml_uninitialized();
				}
				Result = ml_compile(Expr, NULL, Scanner->Context);
				if (ml_is_error(Result)) ML_RETURN(Result);
				Result = ml_simple_call(Result, 0, NULL);
				if (ml_is_error(Result)) ML_RETURN(Result);
				ml_uninitialized_set(Slot[0],  Result);
				Slot[0] = Result;
			} else {
				Result = ml_compile(Expr, NULL, Scanner->Context);
				if (ml_is_error(Result)) ML_RETURN(Result);
				Result = ml_simple_call(Result, 0, NULL);
				if (ml_is_error(Result)) ML_RETURN(Result);
				do {
					ml_value_t **Slot = (ml_value_t **)stringmap_slot(Vars, Import->Ident);
					if (!Slot[0] || ml_typeof(Slot[0]) != MLUninitializedT) {
						Slot[0] = ml_uninitialized();
					}
					ml_value_t *Args[2] = {Result, ml_string(Import->Ident, -1)};
					ml_value_t *Value = ml_simple_call(SymbolMethod, 2, Args);
					if (ml_is_error(Value)) ML_RETURN(Value);
					ml_uninitialized_set(Slot[0],  Value);
					Slot[0] = Value;
					Import = Import->Next;
				} while (Import);
			}
		} while (ml_parse(Scanner, MLT_COMMA));
	} else if (ml_parse(Scanner, MLT_FUN)) {
		if (ml_parse(Scanner, MLT_IDENT)) {
			const char *Ident = Scanner->Ident;
			ml_value_t **Slot = (ml_value_t **)stringmap_slot(Vars, Ident);
			if (!Slot[0] || ml_typeof(Slot[0]) != MLUninitializedT) {
				Slot[0] = ml_uninitialized();
			}
			ml_accept(Scanner, MLT_LEFT_PAREN);
			mlc_expr_t *Expr = ml_accept_fun_expr(Scanner, MLT_RIGHT_PAREN);
			Result = ml_compile(Expr, NULL, Scanner->Context);
			if (ml_is_error(Result)) ML_RETURN(Result);
			Result = ml_simple_call(Result, 0, NULL);
			if (ml_is_error(Result)) ML_RETURN(Result);
			ml_uninitialized_set(Slot[0], Result);
			Slot[0] = Result;
		} else {
			ml_accept(Scanner, MLT_LEFT_PAREN);
			mlc_expr_t *Expr = ml_accept_fun_expr(Scanner, MLT_RIGHT_PAREN);
			Result = ml_compile(Expr, NULL, Scanner->Context);
			if (ml_is_error(Result)) ML_RETURN(Result);
			Result = ml_simple_call(Result, 0, NULL);
			if (ml_is_error(Result)) ML_RETURN(Result);
		}
	} else if (ml_parse(Scanner, MLT_METH)) {
		mlc_expr_t *Expr = ml_accept_meth_expr(Scanner);
		Result = ml_compile(Expr, NULL, Scanner->Context);
		if (ml_is_error(Result)) ML_RETURN(Result);
		Result = ml_simple_call(Result, 0, NULL);
		if (ml_is_error(Result)) ML_RETURN(Result);
	} else {
		mlc_expr_t *Expr = ml_accept_expression(Scanner, EXPR_DEFAULT);
		if (ml_parse(Scanner, MLT_COLON)) {
			ml_accept(Scanner, MLT_IDENT);
			const char *Ident = Scanner->Ident;
			ML_EXPR(CallExpr, parent, call);
			CallExpr->Child = Expr;
			ml_accept(Scanner, MLT_LEFT_PAREN);
			ml_accept_arguments(Scanner, MLT_RIGHT_PAREN, &Expr->Next);
			ml_value_t **Slot = (ml_value_t **)stringmap_slot(Vars, Ident);
			if (!Slot[0] || ml_typeof(Slot[0]) != MLUninitializedT) {
				Slot[0] = ml_uninitialized();
			}
			Result = ml_compile((mlc_expr_t *)CallExpr, NULL, Scanner->Context);
			if (ml_is_error(Result)) ML_RETURN(Result);
			Result = ml_simple_call(Result, 0, NULL);
			if (ml_is_error(Result)) ML_RETURN(Result);
			ml_uninitialized_set(Slot[0],  Result);
			Slot[0] = Result;
		} else {
			Result = ml_compile(Expr, NULL, Scanner->Context);
			if (ml_is_error(Result)) ML_RETURN(Result);
			Result = ml_simple_call(Result, 0, NULL);
			if (ml_is_error(Result)) ML_RETURN(Result);
		}
	}
	ml_parse(Scanner, MLT_SEMICOLON);
	ML_RETURN(Result);
}

#ifdef __MINGW32__
static ssize_t ml_read_line(FILE *File, ssize_t Offset, char **Result) {
	char Buffer[129];
	if (fgets(Buffer, 129, File) == NULL) return -1;
	int Length = strlen(Buffer);
	if (Length == 128) {
		ssize_t Total = ml_read_line(File, Offset + 128, Result);
		memcpy(*Result + Offset, Buffer, 128);
		return Total;
	} else {
		*Result = GC_MALLOC_ATOMIC(Offset + Length + 1);
		strcpy(*Result + Offset, Buffer);
		return Offset + Length;
	}
}
#endif

static const char *ml_file_read(void *Data) {
	FILE *File = (FILE *)Data;
	char *Line = NULL;
	size_t Length = 0;
#ifdef __MINGW32__
	Length = ml_read_line(File, 0, &Line);
	if (Length < 0) return NULL;
#else
	if (getline(&Line, &Length, File) < 0) return NULL;
#endif
	return Line;
}

void ml_load(ml_state_t *Caller, ml_getter_t GlobalGet, void *Globals, const char *FileName, const char *Parameters[]) {
	FILE *File = fopen(FileName, "r");
	if (!File) ML_RETURN(ml_error("LoadError", "error opening %s", FileName));
	mlc_scanner_t *Scanner = ml_scanner(FileName, File, ml_file_read, GlobalGet, Globals);
	if (setjmp(Scanner->Context->OnError)) ML_RETURN(Scanner->Context->Error);
	mlc_expr_t *Expr = ml_accept_block(Scanner);
	ml_accept_eoi(Scanner);
	fclose(File);
	ml_value_t *Closure = ml_compile(Expr, Parameters, Scanner->Context);
	ML_RETURN(Closure);
}

void ml_compiler_init() {
	stringmap_insert(StringFns, "r", ml_regex);
}

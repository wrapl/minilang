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
	MLT_WHEN,
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

typedef struct ml_compiler_task_t ml_compiler_task_t;

struct ml_compiler_t {
	ml_state_t Base;
	ml_compiler_task_t *Tasks, **TaskSlot;
	ml_context_t *Context;
	const char *Next;
	void *Data;
	const char *(*Read)(void *);
	union {
		ml_value_t *Value;
		mlc_expr_t *Expr;
		const char *Ident;
	};
	ml_getter_t GlobalGet;
	void *Globals;
	ml_value_t *Error;
	ml_source_t Source;
	ml_token_t Token;
	jmp_buf OnError;
};

#define MLC_EXPR_FIELDS(name) \
	mlc_compiled_t (*compile)(mlc_function_t *, mlc_## name ## _expr_t *); \
	mlc_expr_t *Next; \
	ml_source_t Source;

static inline ml_inst_t *ml_inst_new(int N, ml_source_t Source, ml_opcode_t Opcode) {
	ml_inst_t *Inst = xnew(ml_inst_t, N, ml_param_t);
	Inst->LineNo = Source.Line;
	Inst->Opcode = Opcode;
	return Inst;
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

extern ml_value_t *IndexMethod;
extern ml_value_t *SymbolMethod;

struct ml_compiler_task_t {
	void (*start)(ml_compiler_task_t *, ml_compiler_t *);
	ml_value_t *(*finish)(ml_compiler_task_t *, ml_value_t *);
	ml_value_t *(*error)(ml_compiler_task_t *, ml_value_t *);
	ml_compiler_task_t *Next;
	ml_value_t *Closure;
	ml_source_t Source;
};

static void ml_task_default_start(ml_compiler_task_t *Task, ml_compiler_t *Compiler) {
	ml_call((ml_state_t *)Compiler, Task->Closure, 0, NULL);
}

static ml_value_t *ml_task_default_finish(ml_compiler_task_t *Task, ml_value_t *Value) {
	return NULL;
}

static void ml_task_queue(ml_compiler_t *Compiler, ml_compiler_task_t *Task) {
	Compiler->TaskSlot[0] = Task;
	Compiler->TaskSlot = &Task->Next;
}

static void ml_tasks_state_run(ml_compiler_t *Compiler, ml_value_t *Value) {
	ml_state_t *Caller = Compiler->Base.Caller;
	ml_compiler_task_t *Task = Compiler->Tasks;
	ml_value_t *Error;
	if (ml_is_error(Value)) {
		if (!Task->error) {
			ml_error_trace_add(Value, Task->Source);
			ML_RETURN(Value);
		}
		Error = Task->error(Task, Value);
	} else {
		Error = Task->finish(Task, Value);
	}
	if (Error) ML_RETURN(Error);
	Task = Compiler->Tasks = Task->Next;
	if (Task) {
		Task->start(Task, Compiler);
	} else {
		Compiler->TaskSlot = &Compiler->Tasks;
		ML_RETURN(Value);
	}
}

struct mlc_function_t {
	ml_compiler_t *Compiler;
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

typedef struct {
	ml_compiler_task_t Base;
	ml_closure_info_t *Info;
} ml_task_closure_info_t;

static void ml_task_closure_info_start(ml_task_closure_info_t *Task, ml_compiler_t *Compiler) {
	ml_closure_info_finish(Task->Info);
	Compiler->Base.run((ml_state_t *)Compiler, MLNil);
}

#define ml_expr_error(EXPR, ERROR) { \
	ml_error_trace_add(ERROR, EXPR->Source); \
	Function->Compiler->Error = ERROR; \
	longjmp(Function->Compiler->OnError, 1); \
}

static ml_value_t *ml_expr_compile(mlc_expr_t *Expr, mlc_function_t *Function) {
	mlc_function_t SubFunction[1];
	memset(SubFunction, 0, sizeof(SubFunction));
	SubFunction->Compiler = Function->Compiler;
	SubFunction->ReturnInst = ml_inst_new(0, Expr->Source, MLI_RETURN);
	SubFunction->Up = Function;
	SubFunction->Size = 1;
	mlc_compiled_t Compiled = mlc_compile(SubFunction, Expr);
	mlc_connect(Compiled.Exits, SubFunction->ReturnInst);
	if (SubFunction->UpValues) {
		ml_expr_error(Expr, ml_error("EvalError", "Use of non-constant value in constant expression"));
	}
	ml_closure_info_t *Info = new(ml_closure_info_t);
	Info->Entry = Compiled.Start;
	Info->Return = SubFunction->ReturnInst;
	Info->Source = Expr->Source.Name;
	Info->LineNo = Expr->Source.Line;
	Info->FrameSize = SubFunction->Size;
	Info->NumParams = 0;
	ml_closure_t *Closure = new(ml_closure_t);
	Closure->Type = MLClosureT;
	Closure->Info = Info;
	ml_task_closure_info_t *Task = new(ml_task_closure_info_t);
	Task->Base.start = (void *)ml_task_closure_info_start;
	Task->Base.finish = (void *)ml_task_default_finish;
	Task->Base.Source = Expr->Source;
	Task->Info = Info;
	ml_task_queue(Function->Compiler, (ml_compiler_task_t *)Task);
	return (ml_value_t *)Closure;
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
typedef struct mlc_catch_expr_t mlc_catch_expr_t;
typedef struct mlc_catch_type_t mlc_catch_type_t;

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
	ml_inst_t *IfInst = ml_inst_new(2, Case->Source, MLI_AND);
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
		IfInst = ml_inst_new(2, Case->Source, MLI_AND);
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
	ml_inst_t *OrInst = ml_inst_new(2, Expr->Source, MLI_OR);
	mlc_connect(Compiled.Exits, OrInst);
	Compiled.Exits = OrInst;
	for (Child = Child->Next; Child->Next; Child = Child->Next) {
		mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
		OrInst->Params[1].Inst = ChildCompiled.Start;
		OrInst = ml_inst_new(2, Expr->Source, MLI_OR);
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
	ml_inst_t *IfInst = ml_inst_new(2, Expr->Source, MLI_AND);
	mlc_connect(Compiled.Exits, IfInst);
	Compiled.Exits = IfInst;
	for (Child = Child->Next; Child->Next; Child = Child->Next) {
		mlc_compiled_t ChildCompiled = mlc_compile(Function, Child);
		IfInst->Params[1].Inst = ChildCompiled.Start;
		IfInst = ml_inst_new(2, Expr->Source, MLI_AND);
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
		TryInst->Params[0].Inst = NextInst;
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
	ml_inst_t *NotInst = ml_inst_new(2, Expr->Source, MLI_AND);
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
	ml_inst_t *WhileInst = ml_inst_new(2, Expr->Source, MLI_OR);
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
	ml_inst_t *UntilInst = ml_inst_new(2, Expr->Source, MLI_AND);
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

static mlc_compiled_t ml_var_in_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
	ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	mlc_connect(Compiled.Exits, PushInst);
	mlc_inc_top(Function);
	ml_inst_t *VarInst = PushInst;
	ml_decl_t *Decl = Expr->Decl;
	for (int I = 0; I < Expr->Count; ++I) {
		ml_inst_t *PushInst = ml_inst_new(2, Expr->Source, MLI_LOCAL_PUSH);
		PushInst->Params[1].Index = Function->Top - 1;
		VarInst->Params[0].Inst = PushInst;
		mlc_inc_top(Function);
		ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD_PUSH);
		ValueInst->Params[1].Value = ml_cstring(Decl->Ident);
		PushInst->Params[0].Inst = ValueInst;
		mlc_inc_top(Function);
		ml_inst_t *CallInst = ml_inst_new(3, Expr->Source, MLI_CONST_CALL);
		CallInst->Params[2].Value = SymbolMethod;
		CallInst->Params[1].Count = 2;
		ValueInst->Params[0].Inst = CallInst;
		Function->Top -= 2;
		VarInst = ml_inst_new(2, Expr->Source, MLI_VAR);
		VarInst->Params[1].Index = Decl->Index - Function->Top;
		CallInst->Params[0].Inst = VarInst;
		Decl = Decl->Next;
	}
	ml_inst_t *PopInst = ml_inst_new(1, Expr->Source, MLI_POP);
	VarInst->Params[0].Inst = PopInst;
	--Function->Top;
	Compiled.Exits = PopInst;
	return Compiled;
}

static mlc_compiled_t ml_var_unpack_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
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

static mlc_compiled_t ml_let_in_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
	ml_inst_t *PushInst = ml_inst_new(1, Expr->Source, MLI_PUSH);
	mlc_connect(Compiled.Exits, PushInst);
	mlc_inc_top(Function);
	ml_inst_t *LetInst = PushInst;
	ml_decl_t *Decl = Expr->Decl;
	for (int I = 0; I < Expr->Count; ++I) {
		ml_inst_t *PushInst = ml_inst_new(2, Expr->Source, MLI_LOCAL_PUSH);
		PushInst->Params[1].Index = Function->Top - 1;
		LetInst->Params[0].Inst = PushInst;
		mlc_inc_top(Function);
		ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD_PUSH);
		ValueInst->Params[1].Value = ml_cstring(Decl->Ident);
		PushInst->Params[0].Inst = ValueInst;
		mlc_inc_top(Function);
		ml_inst_t *CallInst = ml_inst_new(3, Expr->Source, MLI_CONST_CALL);
		CallInst->Params[2].Value = SymbolMethod;
		CallInst->Params[1].Count = 2;
		ValueInst->Params[0].Inst = CallInst;
		Function->Top -= 2;
		LetInst = ml_inst_new(2, Expr->Source, (Decl->Flags & MLC_DECL_BACKFILL) ? MLI_LETI : MLI_LET);
		LetInst->Params[1].Index = Decl->Index - Function->Top;
		CallInst->Params[0].Inst = LetInst;
		Decl->Flags = 0;
		Decl = Decl->Next;
	}
	ml_inst_t *PopInst = ml_inst_new(1, Expr->Source, MLI_POP);
	LetInst->Params[0].Inst = PopInst;
	--Function->Top;
	Compiled.Exits = PopInst;
	return Compiled;
}

static mlc_compiled_t ml_let_unpack_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
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

typedef struct {
	ml_compiler_task_t Base;
	ml_decl_t *Decl;
	ml_inst_t *Inst;
	int NumImports, NumUnpack;
} ml_task_def_t;

typedef struct {
	ml_compiler_task_t Base;
	ml_decl_t *Decl;
	ml_value_t *Args[2];
} ml_task_def_import_t;

static ml_value_t *ml_task_def_finish(ml_task_def_t *Task, ml_value_t *Value) {
	Task->Inst->Params[1].Value = Value;
	ml_decl_t *Decl = Task->Decl;
	if (Task->NumUnpack) {
			for (int I = Task->NumUnpack; --I >= 0; Decl = Decl->Next) {
			ml_value_t *Unpacked = ml_unpack(Value, I);
			if (!Unpacked) {
				ml_value_t *Error = ml_error("ValueError", "Not enough values to unpack (%d < %d)", I, Task->NumUnpack);
				ml_error_trace_add(Error, Task->Base.Source);
				return Error;
			}
			if (Decl->Value) ml_uninitialized_set(Decl->Value, Unpacked);
			Decl->Value = Unpacked;
		}
	} else if (Task->NumImports) {
		ml_task_def_import_t *Import = (ml_task_def_import_t *)Task->Base.Next;
		for (int I = 0; I < Task->NumImports; ++I) {
			Import->Args[0] = Value;
			Import = (ml_task_def_import_t *)Import->Base.Next;
		}
	} else {
		if (Decl->Value) ml_uninitialized_set(Decl->Value, Value);
		Decl->Value = Value;
	}
	return NULL;
}

static void ml_task_def_import_start(ml_task_def_import_t *Task, ml_compiler_t *Compiler) {
	ml_call((ml_state_t *)Compiler, SymbolMethod, 2, Task->Args);
}

static ml_value_t *ml_task_def_import_finish(ml_task_def_import_t *Task, ml_value_t *Value) {
	ml_decl_t *Decl = Task->Decl;
	if (Decl->Value) ml_uninitialized_set(Decl->Value, Value);
	Decl->Value = Value;
	return NULL;
}

static mlc_compiled_t ml_def_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
	ml_decl_t *Decl = Expr->Decl;
	ml_task_def_t *Task = new(ml_task_def_t);
	Task->Base.Closure = ml_expr_compile(Expr->Child, Function);
	Task->Base.start = (void *)ml_task_default_start;
	Task->Base.finish = (void *)ml_task_def_finish;
	Task->Base.Source = Expr->Source;
	ml_task_queue(Function->Compiler, (ml_compiler_task_t *)Task);
	Task->Decl = Decl;
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	Task->Inst = ValueInst;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

static mlc_compiled_t ml_def_in_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
	ml_decl_t *Decl = Expr->Decl;
	ml_task_def_t *Task = new(ml_task_def_t);
	Task->Base.Closure = ml_expr_compile(Expr->Child, Function);
	Task->Base.start = (void *)ml_task_default_start;
	Task->Base.finish = (void *)ml_task_def_finish;
	Task->Base.Source = Expr->Source;
	ml_task_queue(Function->Compiler, (ml_compiler_task_t *)Task);
	for (int I = Expr->Count; --I >= 0; Decl = Decl->Next) {
		++Task->NumImports;
		ml_task_def_import_t *ImportCommand = new(ml_task_def_import_t);
		ImportCommand->Base.start = (void *)ml_task_def_import_start;
		ImportCommand->Base.finish = (void *)ml_task_def_import_finish;
		ImportCommand->Base.Source = Expr->Source;
		ImportCommand->Decl = Decl;
		ImportCommand->Args[1] = ml_cstring(Decl->Ident);
		ml_task_queue(Function->Compiler, (ml_compiler_task_t *)ImportCommand);
	}
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	Task->Inst = ValueInst;
	return (mlc_compiled_t){ValueInst, ValueInst};
}

static mlc_compiled_t ml_def_unpack_expr_compile(mlc_function_t *Function, mlc_decl_expr_t *Expr) {
	ml_task_def_t *Task = new(ml_task_def_t);
	Task->Base.Closure = ml_expr_compile(Expr->Child, Function);
	Task->Base.start = (void *)ml_task_default_start;
	Task->Base.finish = (void *)ml_task_def_finish;
	Task->Base.Source = Expr->Source;
	Task->Decl = Expr->Decl;
	Task->NumUnpack = Expr->Count;
	ml_task_queue(Function->Compiler, (ml_compiler_task_t *)Task);
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	Task->Inst = ValueInst;
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
	mlc_catch_expr_t *Catches;
};

struct mlc_catch_expr_t {
	mlc_catch_expr_t *Next;
	ml_decl_t *Decl;
	mlc_catch_type_t *Types;
	mlc_expr_t *Body;
};

struct mlc_catch_type_t {
	mlc_catch_type_t *Next;
	const char *Type;
};

static mlc_compiled_t ml_block_expr_compile(mlc_function_t *Function, mlc_block_expr_t *Expr) {
	int OldTop = Function->Top;
	ml_decl_t *OldDecls = Function->Decls;
	mlc_try_t Try;
	ml_inst_t *CatchExitInst = NULL;
	if (Expr->Catches) {
		CatchExitInst = ml_inst_new(3, Expr->Source, MLI_EXIT);
		CatchExitInst->Params[1].Count = 1;
		CatchExitInst->Params[2].Decls = OldDecls;
		ml_inst_t *TryInst = ml_inst_new(2, Expr->Source, MLI_TRY);
		TryInst->Params[1].Inst = Function->Try ? Function->Try->CatchInst : Function->ReturnInst;
		ml_inst_t *Last = TryInst;

		for (mlc_catch_expr_t *CatchExpr = Expr->Catches; CatchExpr; CatchExpr = CatchExpr->Next) {
			CatchExpr->Decl->Index = Function->Top++;
			CatchExpr->Decl->Next = Function->Decls;
			Function->Decls = CatchExpr->Decl;
			mlc_compiled_t TryCompiled = mlc_compile(Function, CatchExpr->Body);
			if (CatchExpr->Types) {
				for (mlc_catch_type_t *Type = CatchExpr->Types; Type; Type = Type->Next) {
					ml_inst_t *CatchInst = ml_inst_new(5, CatchExpr->Decl->Source, MLI_CATCH);
					Last->Params[0].Inst = CatchInst;
					Last = CatchInst;
					CatchInst->Params[1].Inst = TryCompiled.Start;
					CatchInst->Params[2].Index = OldTop;
					CatchInst->Params[3].Ptr = Type->Type;
					CatchInst->Params[4].Decls = Function->Decls;
				}
			} else {
				ml_inst_t *CatchInst = ml_inst_new(5, Expr->Source, MLI_CATCH);
				Last->Params[0].Inst = CatchInst;
				Last = CatchInst;
				CatchInst->Params[1].Inst = TryCompiled.Start;
				CatchInst->Params[2].Index = OldTop;
				CatchInst->Params[3].Ptr = NULL;
			}
			mlc_connect(TryCompiled.Exits, CatchExitInst);
			Function->Decls = OldDecls;
			Function->Top = OldTop;
		}
		Last->Params[0].Inst = TryInst->Params[1].Inst;
		Try.Up = Function->Try;
		Try.CatchInst = TryInst;
		Try.CatchTop = OldTop;
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
	if (Expr->Catches) {
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

typedef struct {
	ml_compiler_task_t Base;
	ml_value_t **ModuleParam;
	ml_inst_t *Inst;
	ml_value_t *Args[2];
} ml_task_resolve_t;

static void ml_task_resolve_start(ml_task_resolve_t *Task, ml_compiler_t *Compiler) {
	Task->Args[0] = Task->ModuleParam[0];
	ml_call((ml_state_t *)Compiler, SymbolMethod, 2, Task->Args);
}

static ml_value_t *ml_task_resolve_finish(ml_task_resolve_t *Task, ml_value_t *Value) {
	Task->Inst->Params[1].Value = Value;
	if (ml_typeof(Value) == MLUninitializedT) ml_uninitialized_use(Value, &Task->Inst->Params[1].Value);
	return NULL;
}

static ml_value_t *ml_task_resolve_error(ml_task_resolve_t *Task, ml_value_t *Value) {
	Task->Inst->Opcode = MLI_RESOLVE;
	Task->Inst->Params[1].Value = Task->ModuleParam[0];
	Task->Inst->Params[2].Value = Task->Args[1];
	return NULL;
}

static mlc_compiled_t ml_resolve_expr_compile(mlc_function_t *Function, mlc_parent_value_expr_t *Expr) {
	mlc_compiled_t Compiled = mlc_compile(Function, Expr->Child);
	if ((Compiled.Start == Compiled.Exits) && (Compiled.Start->Opcode == MLI_LOAD)) {
		ml_inst_t *ValueInst = Compiled.Start;
		ml_task_resolve_t *Task = new(ml_task_resolve_t);
		Task->Base.start = (void *)ml_task_resolve_start;
		Task->Base.finish = (void *)ml_task_resolve_finish;
		Task->Base.error = (void *)ml_task_resolve_error;
		Task->Base.Source = Expr->Source;
		Task->ModuleParam = &ValueInst->Params[1].Value;
		Task->Args[1] = Expr->Value;
		ml_inst_t *ImportInst = ml_inst_new(3, Expr->Source, MLI_LOAD);
		Task->Inst = ImportInst;
		ml_task_queue(Function->Compiler, (ml_compiler_task_t *)Task);
		if (Function->Top + 2 >= Function->Size) Function->Size = Function->Top + 3;
		return (mlc_compiled_t){ImportInst, ImportInst};
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
	//ml_source_t End;
};

static mlc_compiled_t ml_fun_expr_compile(mlc_function_t *Function, mlc_fun_expr_t *Expr) {
	// closure <entry> <frame_size> <num_params> <num_upvalues> <upvalue_1> ...
	mlc_function_t SubFunction[1];
	memset(SubFunction, 0, sizeof(SubFunction));
	SubFunction->Compiler = Function->Compiler;
	SubFunction->ReturnInst = ml_inst_new(0, Expr->Source, MLI_RETURN);
	SubFunction->Up = Function;
	ml_closure_info_t *Info = new(ml_closure_info_t);
	Info->Source = Expr->Source.Name;
	Info->LineNo = Expr->Source.Line;
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
	int Index = 0;
	for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next, ++Index) {
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source = Expr->Source;
		Decl->Ident = UpValue->Decl->Ident;
		Decl->Value = UpValue->Decl->Value;
		Decl->Index = ~Index;
		UpValueSlot[0] = Decl;
		UpValueSlot = &Decl->Next;
	}
	mlc_connect(Compiled.Exits, SubFunction->ReturnInst);
	Info->Entry = Compiled.Start;
	Info->Return = SubFunction->ReturnInst;
	Info->FrameSize = SubFunction->Size;
	Info->NumParams = NumParams;
	ml_task_closure_info_t *Task = new(ml_task_closure_info_t);
	Task->Base.start = (void *)ml_task_closure_info_start;
	Task->Base.finish = (void *)ml_task_default_finish;
	Task->Base.Source = Expr->Source;
	Task->Info = Info;
	ml_task_queue(Function->Compiler, (ml_compiler_task_t *)Task);
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
					if (!Decl->Value) Decl->Value = ml_uninitialized(Decl->Ident);
					return ml_ident_expr_finish(Expr, Decl->Value);
				} else {
					int Index = ml_upvalue_find(Function, Decl, UpFunction);
					ml_inst_t *LocalInst;
					if (Decl->Flags & MLC_DECL_FORWARD) Decl->Flags |= MLC_DECL_BACKFILL;
					if ((Index >= 0) && (Decl->Flags & MLC_DECL_FORWARD)) {
						LocalInst = ml_inst_new(3, Expr->Source, MLI_LOCALX);
						LocalInst->Params[1].Index = Index;
						LocalInst->Params[2].Ptr = Decl->Ident;
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
	ml_value_t *Value = (Function->Compiler->GlobalGet)(Function->Compiler->Globals, Expr->Ident);
	if (!Value) {
		ml_expr_error(Expr, ml_error("CompilerError", "identifier %s not declared", Expr->Ident));
	}
	if (ml_is_error(Value)) ml_expr_error(Expr, Value);
	return ml_ident_expr_finish(Expr, Value);
}

typedef struct {
	ml_compiler_task_t Base;
	ml_inst_t *Inst;
} ml_command_inline_t;

static ml_value_t *ml_command_inline_finish(ml_command_inline_t *Task, ml_value_t *Value) {
	Task->Inst->Params[1].Value = Value;
	return NULL;
}

static mlc_compiled_t ml_inline_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr) {
	ml_command_inline_t *Task = new(ml_command_inline_t);
	Task->Base.Closure = ml_expr_compile(Expr->Child, Function);
	Task->Base.start = (void *)ml_task_default_start;
	Task->Base.finish = (void *)ml_command_inline_finish;
	Task->Base.Source = Expr->Source;
	ml_inst_t *ValueInst = ml_inst_new(2, Expr->Source, MLI_LOAD);
	Task->Inst = ValueInst;
	ml_task_queue(Function->Compiler, (ml_compiler_task_t *)Task);
	return (mlc_compiled_t){ValueInst, ValueInst};
}

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
	"when", // MLT_WHEN,
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

static const char *ml_compiler_no_input(void *Data) {
	return NULL;
}

ml_compiler_t *ml_compiler(ml_reader_t Read, void *Data, ml_getter_t GlobalGet, void *Globals) {
	ml_compiler_t *Compiler = new(ml_compiler_t);
	Compiler->Base.run = (ml_state_fn)ml_tasks_state_run;
	Compiler->TaskSlot = &Compiler->Tasks;
	Compiler->GlobalGet = GlobalGet;
	Compiler->Globals = Globals;
	Compiler->Token = MLT_NONE;
	Compiler->Next = "";
	Compiler->Source.Name = "";
	Compiler->Source.Line = 0;
	Compiler->Data = Data;
	Compiler->Read = Read ?: ml_compiler_no_input;
	return Compiler;
}

const char *ml_compiler_name(ml_compiler_t *Compiler) {
	return Compiler->Source.Name;
}

ml_source_t ml_compiler_source(ml_compiler_t *Compiler, ml_source_t Source) {
	ml_source_t OldSource = Compiler->Source;
	Compiler->Source = Source;
	return OldSource;
}

void ml_compiler_reset(ml_compiler_t *Compiler) {
	Compiler->Token = MLT_NONE;
	Compiler->Next = "";
	Compiler->Tasks = NULL;
	Compiler->TaskSlot = &Compiler->Tasks;
}

void ml_compiler_input(ml_compiler_t *Compiler, const char *Text) {
	Compiler->Next = Text;
	++Compiler->Source.Line;
}

const char *ml_compiler_clear(ml_compiler_t *Compiler) {
	const char *Next = Compiler->Next;
	Compiler->Next = "";
	return Next;
}

void ml_compiler_error(ml_compiler_t *Compiler, const char *Error, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	ml_value_t *Value = ml_errorv(Error, Format, Args);
	va_end(Args);
	Compiler->Error = (ml_value_t *)Value;
	ml_error_trace_add(Compiler->Error, Compiler->Source);
	longjmp(Compiler->OnError, 1);
}

#define ML_EXPR(EXPR, TYPE, COMP) \
	mlc_ ## TYPE ## _expr_t *EXPR = new(mlc_ ## TYPE ## _expr_t); \
	EXPR->compile = ml_ ## COMP ## _expr_compile; \
	EXPR->Source = Compiler->Source

typedef enum {
	EXPR_SIMPLE,
	EXPR_AND,
	EXPR_OR,
	EXPR_FOR,
	EXPR_DEFAULT
} ml_expr_level_t;

static int ml_parse(ml_compiler_t *Compiler, ml_token_t Token);
static void ml_accept(ml_compiler_t *Compiler, ml_token_t Token);
static mlc_expr_t *ml_parse_expression(ml_compiler_t *Compiler, ml_expr_level_t Level);
static mlc_expr_t *ml_accept_expression(ml_compiler_t *Compiler, ml_expr_level_t Level);
static void ml_accept_arguments(ml_compiler_t *Compiler, ml_token_t EndToken, mlc_expr_t **ArgsSlot);

static ml_token_t ml_accept_string(ml_compiler_t *Compiler) {
	mlc_string_part_t *Parts = NULL, **Slot = &Parts;
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *End = Compiler->Next;
	for (;;) {
		char C = *End++;
		if (!C) {
			Compiler->Next = (Compiler->Read)(Compiler->Data);
			++Compiler->Source.Line;
			if (!Compiler->Next) {
				ml_compiler_error(Compiler, "ParseError", "end of input while parsing string");
			}
			End = Compiler->Next;
		} else if (C == '\'') {
			Compiler->Next = End;
			break;
		} else if (C == '{') {
			if (Buffer->Length) {
				mlc_string_part_t *Part = new(mlc_string_part_t);
				Part->Length = Buffer->Length;
				Part->Chars = ml_stringbuffer_get(Buffer);
				Slot[0] = Part;
				Slot = &Part->Next;
			}
			Compiler->Next = End;
			mlc_string_part_t *Part = new(mlc_string_part_t);
			ml_accept_arguments(Compiler, MLT_RIGHT_BRACE, &Part->Child);
			End = Compiler->Next;
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
			case 0: ml_compiler_error(Compiler, "ParseError", "end of line while parsing string");
			}
		} else {
			ml_stringbuffer_add(Buffer, End - 1, 1);
		}
	}
	if (!Parts) {
		Compiler->Value = ml_stringbuffer_value(Buffer);
		return (Compiler->Token = MLT_VALUE);
	} else {
		if (Buffer->Length) {
			mlc_string_part_t *Part = new(mlc_string_part_t);
			Part->Length = Buffer->Length;
			Part->Chars = ml_stringbuffer_get(Buffer);
			Slot[0] = Part;
		}
		ML_EXPR(Expr, string, string);
		Expr->Parts = Parts;
		Compiler->Expr = (mlc_expr_t *)Expr;
		return (Compiler->Token = MLT_EXPR);
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

typedef ml_value_t *(*string_fn_t)(const char *String, int Length);

static ml_token_t ml_advance(ml_compiler_t *Compiler) {
	for (;;) {
		if (!Compiler->Next || !Compiler->Next[0]) {
			Compiler->Next = (Compiler->Read)(Compiler->Data);
			if (Compiler->Next) continue;
			Compiler->Token = MLT_EOI;
			return Compiler->Token;
		}
		char Char = Compiler->Next[0];
		if (Char == '\n') {
			++Compiler->Next;
			++Compiler->Source.Line;
			Compiler->Token = MLT_EOL;
			return Compiler->Token;
		}
		if (0 <= Char && Char <= ' ') {
			++Compiler->Next;
			continue;
		}
		if (isidstart(Char)) {
			const char *End = Compiler->Next + 1;
			for (Char = End[0]; isidchar(Char); Char = *++End);
			int Length = End - Compiler->Next;
			const struct keyword_t *Keyword = lookup(Compiler->Next, Length);
			if (Keyword) {
				Compiler->Token = Keyword->Token;
				Compiler->Next = End;
				return Compiler->Token;
			}
			char *Ident = snew(Length + 1);
			memcpy(Ident, Compiler->Next, Length);
			Ident[Length] = 0;
			Compiler->Next = End;
			if (End[0] == '\"') {
				string_fn_t StringFn = stringmap_search(StringFns, Ident);
				if (!StringFn) ml_compiler_error(Compiler, "ParseError", "Unknown string prefix: %s", Ident);
				Compiler->Next += 1;
				const char *End = Compiler->Next;
				while (End[0] != '\"') {
					if (!End[0]) {
						ml_compiler_error(Compiler, "ParseError", "End of input while parsing string");
					}
					if (End[0] == '\\') ++End;
					++End;
				}
				int Length = End - Compiler->Next;
				char *Pattern = snew(Length + 1), *D = Pattern;
				for (const char *S = Compiler->Next; S < End; ++S) {
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
				ml_value_t *Value = StringFn(Pattern, D - Pattern);
				if (ml_is_error(Value)) {
					ml_error_trace_add(Value, Compiler->Source);
					Compiler->Error = Value;
					longjmp(Compiler->OnError, 1);
				}
				Compiler->Value = Value;
				Compiler->Token = MLT_VALUE;
				Compiler->Next = End + 1;
				return Compiler->Token;
			}
			Compiler->Ident = Ident;
			Compiler->Token = MLT_IDENT;
			return Compiler->Token;
		}
		if (isdigit(Char) || (Char == '-' && isdigit(Compiler->Next[1])) || (Char == '.' && isdigit(Compiler->Next[1]))) {
			char *End;
			double Double = strtod(Compiler->Next, (char **)&End);
			for (const char *P = Compiler->Next; P < End; ++P) {
				if (P[0] == '.' || P[0] == 'e' || P[0] == 'E') {
					Compiler->Value = ml_real(Double);
					Compiler->Token = MLT_VALUE;
					Compiler->Next = End;
					return Compiler->Token;
				}
			}
			long Integer = strtol(Compiler->Next, (char **)&End, 10);
			Compiler->Value = ml_integer(Integer);
			Compiler->Token = MLT_VALUE;
			Compiler->Next = End;
			return Compiler->Token;
		}
		if (Char == '\'') {
			++Compiler->Next;
			return ml_accept_string(Compiler);
		}
		if (Char == '\"') {
			++Compiler->Next;
			int Length = 0;
			const char *End = Compiler->Next;
			while (End[0] != '\"') {
				if (!End[0]) {
					ml_compiler_error(Compiler, "ParseError", "end of input while parsing string");
				}
				if (End[0] == '\\') ++End;
				++Length;
				++End;
			}
			char *String = snew(Length + 1), *D = String;
			for (const char *S = Compiler->Next; S < End; ++S) {
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
			Compiler->Value = ml_string(String, Length);
			Compiler->Token = MLT_VALUE;
			Compiler->Next = End + 1;
			return Compiler->Token;
		}
		if (Char == ':') {
			if (Compiler->Next[1] == '=') {
				Compiler->Token = MLT_ASSIGN;
				Compiler->Next += 2;
				return Compiler->Token;
			} else if (Compiler->Next[1] == ':') {
				Compiler->Token = MLT_IMPORT;
				Compiler->Next += 2;
				return Compiler->Token;
			} else if (isidchar(Compiler->Next[1])) {
				const char *End = Compiler->Next + 1;
				for (Char = End[0]; isidchar(Char); Char = *++End);
				int Length = End - Compiler->Next - 1;
				char *Ident = snew(Length + 1);
				memcpy(Ident, Compiler->Next + 1, Length);
				Ident[Length] = 0;
				Compiler->Ident = Ident;
				Compiler->Token = MLT_METHOD;
				Compiler->Next = End;
				return Compiler->Token;
			} else if (Compiler->Next[1] == '\"') {
				Compiler->Next += 2;
				int Length = 0;
				const char *End = Compiler->Next;
				while (End[0] != '\"') {
					if (!End[0]) {
						ml_compiler_error(Compiler, "ParseError", "end of input while parsing string");
					}
					if (End[0] == '\\') ++End;
					++Length;
					++End;
				}
				char *Ident = snew(Length + 1), *D = Ident;
				for (const char *S = Compiler->Next; S < End; ++S) {
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
				Compiler->Ident = Ident;
				Compiler->Token = MLT_METHOD;
				Compiler->Next = End + 1;
				return Compiler->Token;
			} else if (Compiler->Next[1] == '>') {
				const char *End = Compiler->Next + 2;
				while (End[0] && End[0] != '\n') ++End;
				Compiler->Next = End;
				continue;
			} else if (Compiler->Next[1] == '<') {
				Compiler->Next += 2;
				int Level = 1;
				do {
					if (Compiler->Next[0] == 0) {
						Compiler->Next = (Compiler->Read)(Compiler->Data);
						++Compiler->Source.Line;
						if (!Compiler->Next) ml_compiler_error(Compiler, "ParseError", "End of input in comment");
					} else if (Compiler->Next[0] == '>' && Compiler->Next[1] == ':') {
						Compiler->Next += 2;
						--Level;
					} else if (Compiler->Next[0] == ':' && Compiler->Next[1] == '<') {
						Compiler->Next = "";
					} else {
						++Compiler->Next;
					}
				} while (Level);
				continue;
			} else if (Compiler->Next[1] == '(') {
				Compiler->Token = MLT_INLINE;
				Compiler->Next += 2;
				return Compiler->Token;
			} else {
				Compiler->Token = MLT_COLON;
				Compiler->Next += 1;
				return Compiler->Token;
			}
		}
		for (ml_token_t T = MLT_DELIM_FIRST; T <= MLT_DELIM_LAST; ++T) {
			if (Char == MLTokens[T][0]) {
				Compiler->Token = T;
				++Compiler->Next;
				return Compiler->Token;
			}
		}
		if (isoperator(Char)) {
			const char *End = Compiler->Next;
			for (Char = End[0]; isoperator(Char); Char = *++End);
			int Length = End - Compiler->Next;
			char *Operator = snew(Length + 1);
			memcpy(Operator, Compiler->Next, Length);
			Operator[Length] = 0;
			Compiler->Ident = Operator;
			Compiler->Token = MLT_OPERATOR;
			Compiler->Next = End;
			return Compiler->Token;
		}
		ml_compiler_error(Compiler, "ParseError", "unexpected character <%c>", Char);
	}
	return Compiler->Token;
}

static inline ml_token_t ml_next(ml_compiler_t *Compiler) {
	if (Compiler->Token == MLT_NONE) ml_advance(Compiler);
	return Compiler->Token;
}

static inline int ml_parse(ml_compiler_t *Compiler, ml_token_t Token) {
	if (ml_next(Compiler) == Token) {
		Compiler->Token = MLT_NONE;
		return 1;
	} else {
		return 0;
	}
}

static void ml_accept(ml_compiler_t *Compiler, ml_token_t Token) {
	while (ml_parse(Compiler, MLT_EOL));
	if (ml_parse(Compiler, Token)) return;
	if (Compiler->Token == MLT_IDENT) {
		ml_compiler_error(Compiler, "ParseError", "expected %s not %s (%s)", MLTokens[Token], MLTokens[Compiler->Token], Compiler->Ident);
	} else {
		ml_compiler_error(Compiler, "ParseError", "expected %s not %s", MLTokens[Token], MLTokens[Compiler->Token]);
	}
}

static void ml_accept_eoi(ml_compiler_t *Compiler) {
	ml_accept(Compiler, MLT_EOI);
}

static mlc_expr_t *ml_parse_factor(ml_compiler_t *Compiler, int MethDecl);
static mlc_expr_t *ml_parse_term(ml_compiler_t *Compiler);
static mlc_expr_t *ml_accept_block(ml_compiler_t *Compiler, int NoCatches);

static mlc_expr_t *ml_accept_fun_expr(ml_compiler_t *Compiler, ml_token_t EndToken) {
	ML_EXPR(FunExpr, fun, fun);
	if (!ml_parse(Compiler, EndToken)) {
		ml_decl_t **ParamSlot = &FunExpr->Params;
		do {
			ml_decl_t *Param = ParamSlot[0] = new(ml_decl_t);
			Param->Source = Compiler->Source;
			ParamSlot = &Param->Next;
			if (ml_parse(Compiler, MLT_LEFT_SQUARE)) {
				ml_accept(Compiler, MLT_IDENT);
				Param->Ident = Compiler->Ident;
				Param->Index = ML_PARAM_EXTRA;
				ml_accept(Compiler, MLT_RIGHT_SQUARE);
				if (ml_parse(Compiler, MLT_COMMA)) {
					ml_accept(Compiler, MLT_LEFT_BRACE);
					ml_decl_t *Param = ParamSlot[0] = new(ml_decl_t);
					Param->Source = Compiler->Source;
					ml_accept(Compiler, MLT_IDENT);
					Param->Ident = Compiler->Ident;
					Param->Index = ML_PARAM_NAMED;
					ml_accept(Compiler, MLT_RIGHT_BRACE);
				}
				break;
			} else if (ml_parse(Compiler, MLT_LEFT_BRACE)) {
				ml_accept(Compiler, MLT_IDENT);
				Param->Ident = Compiler->Ident;
				Param->Index = ML_PARAM_NAMED;
				ml_accept(Compiler, MLT_RIGHT_BRACE);
				break;
			} else {
				ml_accept(Compiler, MLT_IDENT);
				Param->Ident = Compiler->Ident;
				if (ml_parse(Compiler, MLT_COLON)) {
					// Parse type specifications but ignore for now
					ml_parse_expression(Compiler, EXPR_DEFAULT);
				}
			}
		} while (ml_parse(Compiler, MLT_COMMA));
		ml_accept(Compiler, EndToken);
	}
	FunExpr->Body = ml_accept_expression(Compiler, EXPR_DEFAULT);
	FunExpr->Source = FunExpr->Body->Source;
	//FunExpr->End = Compiler->Source;
	return (mlc_expr_t *)FunExpr;
}

extern ml_value_t MLMethodSet[];

static mlc_expr_t *ml_accept_meth_expr(ml_compiler_t *Compiler) {
	ML_EXPR(MethodExpr, parent_value, const_call);
	MethodExpr->Value = MLMethodSet;
	mlc_expr_t *Method = ml_parse_factor(Compiler, 1);
	if (!Method) ml_compiler_error(Compiler, "ParseError", "expected <factor> not <%s>", MLTokens[Compiler->Token]);
	MethodExpr->Child = Method;
	mlc_expr_t **ArgsSlot = &Method->Next;
	ml_accept(Compiler, MLT_LEFT_PAREN);
	ML_EXPR(FunExpr, fun, fun);
	if (!ml_parse(Compiler, MLT_RIGHT_PAREN)) {
		ml_decl_t **ParamSlot = &FunExpr->Params;
		do {
			ml_decl_t *Param = ParamSlot[0] = new(ml_decl_t);
			Param->Source = Compiler->Source;
			ParamSlot = &Param->Next;
			ml_accept(Compiler, MLT_IDENT);
			Param->Ident = Compiler->Ident;
			ml_accept(Compiler, MLT_COLON);
			mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
			ArgsSlot = &Arg->Next;
		} while (ml_parse(Compiler, MLT_COMMA));
		ml_accept(Compiler, MLT_RIGHT_PAREN);
	}
	if (ml_parse(Compiler, MLT_ASSIGN)) {
		ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
	} else {
		FunExpr->Body = ml_accept_expression(Compiler, EXPR_DEFAULT);
		//FunExpr->End = Compiler->Source;
		ArgsSlot[0] = (mlc_expr_t *)FunExpr;
	}
	return (mlc_expr_t *)MethodExpr;
}

static void ml_accept_named_arguments(ml_compiler_t *Compiler, ml_token_t EndToken, mlc_expr_t **ArgsSlot, ml_value_t *Names) {
	mlc_expr_t **NamesSlot = ArgsSlot;
	mlc_expr_t *Arg = ArgsSlot[0];
	ArgsSlot = &Arg->Next;
	if (ml_parse(Compiler, MLT_SEMICOLON)) {
		ArgsSlot[0] = ml_accept_fun_expr(Compiler, EndToken);
		return;
	}
	Arg = ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
	ArgsSlot = &Arg->Next;
	while (ml_parse(Compiler, MLT_COMMA)) {
		if (ml_parse(Compiler, MLT_IDENT) || ml_parse(Compiler, MLT_METHOD)) {
			ml_names_add(Names, ml_method(Compiler->Ident));
		} else {
			ml_compiler_error(Compiler, "ParseError", "Argument names must be identifiers or methods");
		}
		ml_accept(Compiler, MLT_IS);
		if (ml_parse(Compiler, MLT_SEMICOLON)) {
			ArgsSlot[0] = ml_accept_fun_expr(Compiler, EndToken);
			return;
		}
		Arg = ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
		ArgsSlot = &Arg->Next;
	}
	if (ml_parse(Compiler, MLT_SEMICOLON)) {
		mlc_expr_t *FunExpr = ml_accept_fun_expr(Compiler, EndToken);
		FunExpr->Next = NamesSlot[0];
		NamesSlot[0] = FunExpr;
	} else {
		ml_accept(Compiler, EndToken);
	}
}

static void ml_accept_arguments(ml_compiler_t *Compiler, ml_token_t EndToken, mlc_expr_t **ArgsSlot) {
	while (ml_parse(Compiler, MLT_EOL));
	if (ml_parse(Compiler, MLT_SEMICOLON)) {
		ArgsSlot[0] = ml_accept_fun_expr(Compiler, EndToken);
	} else if (!ml_parse(Compiler, EndToken)) {
		do {
			mlc_expr_t *Arg = ml_accept_expression(Compiler, EXPR_DEFAULT);
			if (ml_parse(Compiler, MLT_IS)) {
				ml_value_t *Names = ml_names();
				if (Arg->compile == (void *)ml_ident_expr_compile) {
					ml_names_add(Names, ml_method(((mlc_ident_expr_t *)Arg)->Ident));
				} else if (Arg->compile == (void *)ml_value_expr_compile) {
					ml_value_t *Name = ((mlc_value_expr_t *)Arg)->Value;
					if (ml_typeof(Name) != MLMethodT) {
						ml_compiler_error(Compiler, "ParseError", "Argument names must be identifiers or methods");
					}
					ml_names_add(Names, Name);
				} else {
					ml_compiler_error(Compiler, "ParseError", "Argument names must be identifiers or methods");
				}
				ML_EXPR(NamesArg, value, value);
				NamesArg->Value = Names;
				ArgsSlot[0] = (mlc_expr_t *)NamesArg;
				return ml_accept_named_arguments(Compiler, EndToken, ArgsSlot, Names);
			} else {
				ArgsSlot[0] = Arg;
				ArgsSlot = &Arg->Next;
			}
		} while (ml_parse(Compiler, MLT_COMMA));
		if (ml_parse(Compiler, MLT_SEMICOLON)) {
			ArgsSlot[0] = ml_accept_fun_expr(Compiler, EndToken);
		} else {
			ml_accept(Compiler, EndToken);
		}
		return;
	}
}

static mlc_expr_t *ml_accept_with_expr(ml_compiler_t *Compiler, mlc_expr_t *Child) {
	ML_EXPR(WithExpr, decl, with);
	ml_decl_t **DeclSlot = &WithExpr->Decl;
	mlc_expr_t **ExprSlot = &WithExpr->Child;
	do {
		if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
			int Count = 0;
			ml_decl_t **First = DeclSlot;
			do {
				ml_accept(Compiler, MLT_IDENT);
				++Count;
				ml_decl_t *Decl = DeclSlot[0] = new(ml_decl_t);
				Decl->Source = Compiler->Source;
				Decl->Ident = Compiler->Ident;
				DeclSlot = &Decl->Next;
			} while (ml_parse(Compiler, MLT_COMMA));
			ml_accept(Compiler, MLT_RIGHT_PAREN);
			First[0]->Index = Count;
		} else {
			ml_accept(Compiler, MLT_IDENT);
			ml_decl_t *Decl = DeclSlot[0] = new(ml_decl_t);
			Decl->Source = Compiler->Source;
			DeclSlot = &Decl->Next;
			Decl->Ident = Compiler->Ident;
			Decl->Index = 1;
		}
		ml_accept(Compiler, MLT_ASSIGN);
		mlc_expr_t *Expr = ExprSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
		ExprSlot = &Expr->Next;
	} while (ml_parse(Compiler, MLT_COMMA));
	if (Child) {
		ExprSlot[0] = Child;
	} else {
		ml_accept(Compiler, MLT_DO);
		ExprSlot[0] = ml_accept_block(Compiler, 0);
		ml_accept(Compiler, MLT_END);
	}
	return (mlc_expr_t *)WithExpr;
}

static ML_METHOD_DECL(MLIn, "in");
static ML_METHOD_DECL(MLIs, "=");

static mlc_expr_t *ml_parse_factor(ml_compiler_t *Compiler, int MethDecl) {
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
	switch (ml_next(Compiler)) {
	case MLT_EACH:
	case MLT_NOT:
	case MLT_WHILE:
	case MLT_UNTIL:
	{
		mlc_parent_expr_t *ParentExpr = new(mlc_parent_expr_t);
		ParentExpr->compile = CompileFns[Compiler->Token];
		ParentExpr->Source = Compiler->Source;
		Compiler->Token = MLT_NONE;
		ParentExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
		return (mlc_expr_t *)ParentExpr;
	}
	case MLT_EXIT:
	case MLT_RET:
	{
		mlc_parent_expr_t *ParentExpr = new(mlc_parent_expr_t);
		ParentExpr->compile = CompileFns[Compiler->Token];
		ParentExpr->Source = Compiler->Source;
		Compiler->Token = MLT_NONE;
		ParentExpr->Child = ml_parse_expression(Compiler, EXPR_DEFAULT);
		return (mlc_expr_t *)ParentExpr;
	}
	case MLT_NEXT:
	case MLT_NIL:
	case MLT_BLANK:
	case MLT_OLD:
	{
		mlc_expr_t *Expr = new(mlc_expr_t);
		Expr->compile = CompileFns[Compiler->Token];
		Expr->Source = Compiler->Source;
		Compiler->Token = MLT_NONE;
		return Expr;
	}
	case MLT_DO: {
		Compiler->Token = MLT_NONE;
		mlc_expr_t *BlockExpr = ml_accept_block(Compiler, 0);
		ml_accept(Compiler, MLT_END);
		return BlockExpr;
	}
	case MLT_IF: {
		Compiler->Token = MLT_NONE;
		ML_EXPR(IfExpr, if, if);
		mlc_if_case_t **CaseSlot = &IfExpr->Cases;
		do {
			mlc_if_case_t *Case = CaseSlot[0] = new(mlc_if_case_t);
			CaseSlot = &Case->Next;
			Case->Source = Compiler->Source;
			if (ml_parse(Compiler, MLT_VAR)) {
				ml_decl_t *Decl = new(ml_decl_t);
				Decl->Source = Compiler->Source;
				ml_accept(Compiler, MLT_IDENT);
				Decl->Ident = Compiler->Ident;
				Decl->Index = 1;
				ml_accept(Compiler, MLT_ASSIGN);
				Case->Decl = Decl;
			} else if (ml_parse(Compiler, MLT_LET)) {
				ml_decl_t *Decl = new(ml_decl_t);
				Decl->Source = Compiler->Source;
				ml_accept(Compiler, MLT_IDENT);
				Decl->Ident = Compiler->Ident;
				Decl->Index = 0;
				ml_accept(Compiler, MLT_ASSIGN);
				Case->Decl = Decl;
			}
			Case->Condition = ml_accept_expression(Compiler, EXPR_DEFAULT);
			ml_accept(Compiler, MLT_THEN);
			Case->Body = ml_accept_block(Compiler, 0);
		} while (ml_parse(Compiler, MLT_ELSEIF));
		if (ml_parse(Compiler, MLT_ELSE)) IfExpr->Else = ml_accept_block(Compiler, 0);
		ml_accept(Compiler, MLT_END);
		return (mlc_expr_t *)IfExpr;
	}
	case MLT_WHEN: {
		Compiler->Token = MLT_NONE;
		ML_EXPR(WhenExpr, decl, with);
		char *Ident;
		asprintf(&Ident, "when:%d", Compiler->Source.Line);
		WhenExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
		ml_decl_t *Decl = WhenExpr->Decl = new(ml_decl_t);
		Decl->Source = Compiler->Source;
		Decl->Ident = Ident;
		Decl->Index = 1;
		ML_EXPR(IfExpr, if, if);
		mlc_if_case_t **CaseSlot = &IfExpr->Cases;
		do {
			mlc_if_case_t *Case = CaseSlot[0] = new(mlc_if_case_t);
			CaseSlot = &Case->Next;
			Case->Source = Compiler->Source;
			mlc_expr_t **ConditionSlot = &Case->Condition;
			ml_accept(Compiler, MLT_IS);
			ml_value_t *Method = MLIsMethod;
			do {
				ML_EXPR(IdentExpr, ident, ident);
				IdentExpr->Ident = Ident;
				if (ml_parse(Compiler, MLT_NIL)) {
					ML_EXPR(NotExpr, parent, not);
					NotExpr->Child = (mlc_expr_t *)IdentExpr;
					ConditionSlot[0] = (mlc_expr_t *)NotExpr;
					ConditionSlot = &NotExpr->Next;
					Method = MLIsMethod;
				} else {
					if (ml_parse(Compiler, MLT_IN)) {
						Method = MLInMethod;
					} else if (ml_parse(Compiler, MLT_OPERATOR)) {
						Method = ml_method(Compiler->Ident);
					}
					if (!Method) ml_compiler_error(Compiler, "ParseError", "Expected operator not %s", MLTokens[Compiler->Token]);
					IdentExpr->Next = ml_accept_expression(Compiler, EXPR_DEFAULT);
					ML_EXPR(CallExpr, parent_value, const_call);
					CallExpr->Value = Method;
					CallExpr->Child = (mlc_expr_t *)IdentExpr;
					ConditionSlot[0] = (mlc_expr_t *)CallExpr;
					ConditionSlot = &CallExpr->Next;
				}
			} while (ml_parse(Compiler, MLT_COMMA));
			if (Case->Condition->Next) {
				ML_EXPR(OrExpr, parent, or);
				OrExpr->Child = Case->Condition;
				Case->Condition = (mlc_expr_t *)OrExpr;
			}
			ml_accept(Compiler, MLT_DO);
			Case->Body = ml_accept_block(Compiler, 0);
			if (ml_parse(Compiler, MLT_ELSE)) {
				IfExpr->Else = ml_accept_block(Compiler, 0);
				ml_accept(Compiler, MLT_END);
				break;
			}
		} while (!ml_parse(Compiler, MLT_END));
		WhenExpr->Child->Next = (mlc_expr_t *)IfExpr;
		return (mlc_expr_t *)WhenExpr;
	}
	case MLT_LOOP: {
		Compiler->Token = MLT_NONE;
		ML_EXPR(LoopExpr, parent, loop);
		LoopExpr->Child = ml_accept_block(Compiler, 0);
		ml_accept(Compiler, MLT_END);
		return (mlc_expr_t *)LoopExpr;
	}
	case MLT_FOR: {
		Compiler->Token = MLT_NONE;
		ML_EXPR(ForExpr, decl, for);
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source = Compiler->Source;
		ml_accept(Compiler, MLT_IDENT);
		Decl->Ident = Compiler->Ident;
		if (ml_parse(Compiler, MLT_COMMA)) {
			ml_accept(Compiler, MLT_IDENT);
			ml_decl_t *Decl2 = new(ml_decl_t);
			Decl2->Source = Compiler->Source;
			Decl2->Ident = Compiler->Ident;
			Decl->Next = Decl2;
		}
		ForExpr->Decl = Decl;
		ml_accept(Compiler, MLT_IN);
		ForExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
		ml_accept(Compiler, MLT_DO);
		ForExpr->Child->Next = ml_accept_block(Compiler, 0);
		if (ml_parse(Compiler, MLT_ELSE)) {
			ForExpr->Child->Next->Next = ml_accept_block(Compiler, 0);
		}
		ml_accept(Compiler, MLT_END);
		return (mlc_expr_t *)ForExpr;
	}
	case MLT_FUN: {
		Compiler->Token = MLT_NONE;
		if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
			return ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
		} else {
			ML_EXPR(FunExpr, fun, fun);
			FunExpr->Body = ml_accept_expression(Compiler, EXPR_DEFAULT);
			//FunExpr->End = Compiler->Source;
			return (mlc_expr_t *)FunExpr;
		}
	}
	case MLT_METH: {
		Compiler->Token = MLT_NONE;
		return ml_accept_meth_expr(Compiler);
	}
	case MLT_SUSP: {
		Compiler->Token = MLT_NONE;
		ML_EXPR(SuspendExpr, parent, suspend);
		SuspendExpr->Child = ml_parse_expression(Compiler, EXPR_DEFAULT);
		if (ml_parse(Compiler, MLT_COMMA)) {
			SuspendExpr->Child->Next = ml_accept_expression(Compiler, EXPR_DEFAULT);
		}
		return (mlc_expr_t *)SuspendExpr;
	}
	case MLT_WITH: {
		Compiler->Token = MLT_NONE;
		return ml_accept_with_expr(Compiler, NULL);
	}
	case MLT_IDENT: {
		Compiler->Token = MLT_NONE;
		ML_EXPR(IdentExpr, ident, ident);
		IdentExpr->Ident = Compiler->Ident;
		return (mlc_expr_t *)IdentExpr;
	}
	case MLT_VALUE: {
		Compiler->Token = MLT_NONE;
		ML_EXPR(ValueExpr, value, value);
		ValueExpr->Value = Compiler->Value;
		return (mlc_expr_t *)ValueExpr;
	}
	case MLT_EXPR: {
		Compiler->Token = MLT_NONE;
		return Compiler->Expr;
	}
	case MLT_INLINE: {
		Compiler->Token = MLT_NONE;
		ML_EXPR(InlineExpr, parent, inline);
		InlineExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
		ml_accept(Compiler, MLT_RIGHT_PAREN);
		return (mlc_expr_t *)InlineExpr;
	}
	case MLT_LEFT_PAREN: {
		Compiler->Token = MLT_NONE;
		if (ml_parse(Compiler, MLT_SEMICOLON)) {
			ML_EXPR(TupleExpr, parent, tuple);
			TupleExpr->Child = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			return (mlc_expr_t *)TupleExpr;
		}
		mlc_expr_t *Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
		if (ml_parse(Compiler, MLT_COMMA)) {
			ML_EXPR(TupleExpr, parent, tuple);
			TupleExpr->Child = Expr;
			ml_accept_arguments(Compiler, MLT_RIGHT_PAREN, &Expr->Next);
			Expr = (mlc_expr_t *)TupleExpr;
		} else if (ml_parse(Compiler, MLT_SEMICOLON)) {
			ML_EXPR(TupleExpr, parent, tuple);
			TupleExpr->Child = Expr;
			Expr->Next = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			Expr = (mlc_expr_t *)TupleExpr;
		} else {
			ml_accept(Compiler, MLT_RIGHT_PAREN);
		}
		return Expr;
	}
	case MLT_LEFT_SQUARE: {
		Compiler->Token = MLT_NONE;
		while (ml_parse(Compiler, MLT_EOL));
		ML_EXPR(ListExpr, parent, list);
		mlc_expr_t **ArgsSlot = &ListExpr->Child;
		if (!ml_parse(Compiler, MLT_RIGHT_SQUARE)) {
			do {
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
			} while (ml_parse(Compiler, MLT_COMMA));
			ml_accept(Compiler, MLT_RIGHT_SQUARE);
		}
		return (mlc_expr_t *)ListExpr;
	}
	case MLT_LEFT_BRACE: {
		Compiler->Token = MLT_NONE;
		while (ml_parse(Compiler, MLT_EOL));
		ML_EXPR(MapExpr, parent, map);
		mlc_expr_t **ArgsSlot = &MapExpr->Child;
		if (!ml_parse(Compiler, MLT_RIGHT_BRACE)) {
			do {
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
				if (ml_parse(Compiler, MLT_IS)) {
					mlc_expr_t *ArgExpr = ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
					ArgsSlot = &ArgExpr->Next;
				} else {
					ML_EXPR(ArgExpr, value, value);
					ArgExpr->Value = MLSome;
					ArgsSlot[0] = (mlc_expr_t *)ArgExpr;
					ArgsSlot = &ArgExpr->Next;
				}
			} while (ml_parse(Compiler, MLT_COMMA));
			ml_accept(Compiler, MLT_RIGHT_BRACE);
		}
		return (mlc_expr_t *)MapExpr;
	}
	case MLT_OPERATOR: {
		Compiler->Token = MLT_NONE;
		ml_value_t *Operator = ml_method(Compiler->Ident);
		if (MethDecl) {
			ML_EXPR(ValueExpr, value, value);
			ValueExpr->Value = Operator;
			return (mlc_expr_t *)ValueExpr;
		} else if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
			ML_EXPR(CallExpr, parent_value, const_call);
			CallExpr->Value = Operator;
			ml_accept_arguments(Compiler, MLT_RIGHT_PAREN, &CallExpr->Child);
			return (mlc_expr_t *)CallExpr;
		} else {
			mlc_expr_t *Child = ml_parse_term(Compiler);
			if (Child) {
				ML_EXPR(CallExpr, parent_value, const_call);
				CallExpr->Value = Operator;
				CallExpr->Child = Child;
				return (mlc_expr_t *)CallExpr;
			} else {
				ML_EXPR(ValueExpr, value, value);
				ValueExpr->Value = Operator;
				return (mlc_expr_t *)ValueExpr;
			}
		}
	}
	case MLT_METHOD: {
		Compiler->Token = MLT_NONE;
		ML_EXPR(ValueExpr, value, value);
		ValueExpr->Value = ml_method(Compiler->Ident);
		return (mlc_expr_t *)ValueExpr;
	}
	default: return NULL;
	}
}

static mlc_expr_t *ml_parse_term(ml_compiler_t *Compiler) {
	mlc_expr_t *Expr = ml_parse_factor(Compiler, 0);
	if (!Expr) return NULL;
	for (;;) {
		switch (ml_next(Compiler)) {
		case MLT_LEFT_PAREN: {
			Compiler->Token = MLT_NONE;
			ML_EXPR(CallExpr, parent, call);
			CallExpr->Child = Expr;
			ml_accept_arguments(Compiler, MLT_RIGHT_PAREN, &Expr->Next);
			Expr = (mlc_expr_t *)CallExpr;
			break;
		}
		case MLT_LEFT_SQUARE: {
			Compiler->Token = MLT_NONE;
			while (ml_parse(Compiler, MLT_EOL));
			ML_EXPR(IndexExpr, parent_value, const_call);
			IndexExpr->Value = IndexMethod;
			IndexExpr->Child = Expr;
			ml_accept_arguments(Compiler, MLT_RIGHT_SQUARE, &Expr->Next);
			Expr = (mlc_expr_t *)IndexExpr;
			break;
		}
		case MLT_METHOD: {
			Compiler->Token = MLT_NONE;
			ML_EXPR(CallExpr, parent_value, const_call);
			CallExpr->Value = ml_method(Compiler->Ident);
			CallExpr->Child = Expr;
			if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
				ml_accept_arguments(Compiler, MLT_RIGHT_PAREN, &Expr->Next);
			}
			Expr = (mlc_expr_t *)CallExpr;
			break;
		}
		case MLT_IMPORT: {
			Compiler->Token = MLT_NONE;
			if (!ml_parse(Compiler, MLT_OPERATOR) && !ml_parse(Compiler, MLT_IDENT)) {
				ml_accept(Compiler, MLT_VALUE);
				if (!ml_is(Compiler->Value, MLStringT)) {
					ml_compiler_error(Compiler, "ParseError", "expected import not %s", MLTokens[Compiler->Token]);
				}
				Compiler->Ident = ml_string_value(Compiler->Value);
			}
			ML_EXPR(ResolveExpr, parent_value, resolve);
			ResolveExpr->Value = ml_string(Compiler->Ident, -1);
			ResolveExpr->Child = Expr;
			Expr = (mlc_expr_t *)ResolveExpr;
			break;
		}
		default: {
			return Expr;
		}
		}
	}
	return NULL; // Unreachable
}

static mlc_expr_t *ml_accept_term(ml_compiler_t *Compiler) {
	while (ml_parse(Compiler, MLT_EOL));
	mlc_expr_t *Expr = ml_parse_term(Compiler);
	if (Expr) return Expr;
	ml_compiler_error(Compiler, "ParseError", "expected <expression> not %s", MLTokens[Compiler->Token]);
}

static mlc_expr_t *ml_parse_expression(ml_compiler_t *Compiler, ml_expr_level_t Level) {
	mlc_expr_t *Expr = ml_parse_term(Compiler);
	if (!Expr) return NULL;
	for (;;) switch (ml_next(Compiler)) {
	case MLT_OPERATOR: case MLT_IDENT: {
		Compiler->Token = MLT_NONE;
		ML_EXPR(CallExpr, parent_value, const_call);
		CallExpr->Value = ml_method(Compiler->Ident);
		CallExpr->Child = Expr;
		if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
			ml_accept_arguments(Compiler, MLT_RIGHT_PAREN, &Expr->Next);
		} else {
			Expr->Next = ml_accept_term(Compiler);
		}
		Expr = (mlc_expr_t *)CallExpr;
		break;
	}
	case MLT_ASSIGN: {
		Compiler->Token = MLT_NONE;
		ML_EXPR(AssignExpr, parent, assign);
		AssignExpr->Child = Expr;
		Expr->Next = ml_accept_expression(Compiler, EXPR_DEFAULT);
		Expr = (mlc_expr_t *)AssignExpr;
		break;
	}
	case MLT_IN: {
		Compiler->Token = MLT_NONE;
		ML_EXPR(CallExpr, parent_value, const_call);
		CallExpr->Value = MLInMethod;
		CallExpr->Child = Expr;
		Expr->Next = ml_accept_expression(Compiler, EXPR_SIMPLE);
		Expr = (mlc_expr_t *)CallExpr;
		break;
	}
	default: goto done;
	}
done:
	if (Level >= EXPR_AND && ml_parse(Compiler, MLT_AND)) {
		ML_EXPR(AndExpr, parent, and);
		mlc_expr_t *LastChild = AndExpr->Child = Expr;
		do {
			LastChild = LastChild->Next = ml_accept_expression(Compiler, EXPR_SIMPLE);
		} while (ml_parse(Compiler, MLT_AND));
		Expr = (mlc_expr_t *)AndExpr;
	}
	if (Level >= EXPR_OR && ml_parse(Compiler, MLT_OR)) {
		ML_EXPR(OrExpr, parent, or);
		mlc_expr_t *LastChild = OrExpr->Child = Expr;
		do {
			LastChild = LastChild->Next = ml_accept_expression(Compiler, EXPR_AND);
		} while (ml_parse(Compiler, MLT_OR));
		Expr = (mlc_expr_t *)OrExpr;
	}
	if (Level >= EXPR_FOR) {
		if (ml_parse(Compiler, MLT_WITH)) {
			Expr = ml_accept_with_expr(Compiler, Expr);
		}
		int IsComprehension = 0;
		if (ml_parse(Compiler, MLT_TO)) {
			Expr->Next = ml_accept_expression(Compiler, EXPR_OR);
			ml_accept(Compiler, MLT_FOR);
			IsComprehension = 1;
		} else {
			IsComprehension = ml_parse(Compiler, MLT_FOR);
		}
		if (IsComprehension) {
			ML_EXPR(FunExpr, fun, fun);
			ML_EXPR(SuspendExpr, parent, suspend);
			SuspendExpr->Child = Expr;
			mlc_expr_t *Body = (mlc_expr_t *)SuspendExpr;
			do {
				ML_EXPR(ForExpr, decl, for);
				ml_decl_t *Decl = new(ml_decl_t);
				Decl->Source = Compiler->Source;
				ml_accept(Compiler, MLT_IDENT);
				Decl->Ident = Compiler->Ident;
				if (ml_parse(Compiler, MLT_COMMA)) {
					ml_accept(Compiler, MLT_IDENT);
					ml_decl_t *Decl2 = new(ml_decl_t);
					Decl2->Source = Compiler->Source;
					Decl2->Ident = Compiler->Ident;
					Decl->Next = Decl2;
				}
				ForExpr->Decl = Decl;
				ml_accept(Compiler, MLT_IN);
				ForExpr->Child = ml_accept_expression(Compiler, EXPR_OR);
				for (;;) {
					if (ml_parse(Compiler, MLT_IF)) {
						ML_EXPR(IfExpr, if, if);
						mlc_if_case_t *IfCase = IfExpr->Cases = new(mlc_if_case_t);
						IfCase->Condition = ml_accept_expression(Compiler, EXPR_OR);
						IfCase->Body = Body;
						Body = (mlc_expr_t *)IfExpr;
					} else if (ml_parse(Compiler, MLT_WITH)) {
						Body = ml_accept_with_expr(Compiler, Body);
					} else {
						break;
					}
				}
				ForExpr->Child->Next = Body;
				Body = (mlc_expr_t *)ForExpr;
			} while (ml_parse(Compiler, MLT_FOR));
			FunExpr->Body = Body;
			//FunExpr->End = Compiler->Source;
			Expr = (mlc_expr_t *)FunExpr;
		}
	}
	return Expr;
}

static mlc_expr_t *ml_accept_expression(ml_compiler_t *Compiler, ml_expr_level_t Level) {
	while (ml_parse(Compiler, MLT_EOL));
	mlc_expr_t *Expr = ml_parse_expression(Compiler, Level);
	if (Expr) return Expr;
	ml_compiler_error(Compiler, "ParseError", "expected <expression> not %s", MLTokens[Compiler->Token]);
}

typedef struct {
	mlc_expr_t **ExprSlot;
	ml_decl_t **VarsSlot;
	ml_decl_t **LetsSlot;
	ml_decl_t **DefsSlot;
} ml_accept_block_t;

static void ml_accept_block_var(ml_compiler_t *Compiler, ml_accept_block_t *Accept) {
	do {
		if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
			int Count = 0;
			ml_decl_t *Decl;
			do {
				ml_accept(Compiler, MLT_IDENT);
				++Count;
				Decl = Accept->VarsSlot[0] = new(ml_decl_t);
				Decl->Source = Compiler->Source;
				Decl->Ident = Compiler->Ident;
				Accept->VarsSlot = &Decl->Next;
			} while (ml_parse(Compiler, MLT_COMMA));
			ml_accept(Compiler, MLT_RIGHT_PAREN);
			if (ml_parse(Compiler, MLT_IN)) {
				ML_EXPR(DeclExpr, decl, var_in);
				DeclExpr->Decl = Decl;
				DeclExpr->Count = Count;
				DeclExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
				Accept->ExprSlot[0] = (mlc_expr_t *)DeclExpr;
				Accept->ExprSlot = &DeclExpr->Next;
			} else {
				ml_accept(Compiler, MLT_ASSIGN);
				ML_EXPR(DeclExpr, decl, var_unpack);
				DeclExpr->Decl = Decl;
				DeclExpr->Count = Count;
				DeclExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
				Accept->ExprSlot[0] = (mlc_expr_t *)DeclExpr;
				Accept->ExprSlot = &DeclExpr->Next;
			}
		} else {
			ml_accept(Compiler, MLT_IDENT);
			ml_decl_t *Decl = Accept->VarsSlot[0] = new(ml_decl_t);
			Decl->Source = Compiler->Source;
			Decl->Ident = Compiler->Ident;
			Accept->VarsSlot = &Decl->Next;
			mlc_expr_t *Child = NULL;
			if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
				Child = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			} else if (ml_parse(Compiler, MLT_ASSIGN)) {
				Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
			}
			if (Child) {
				ML_EXPR(DeclExpr, decl, var);
				DeclExpr->Decl = Decl;
				DeclExpr->Child = Child;
				Accept->ExprSlot[0] = (mlc_expr_t *)DeclExpr;
				Accept->ExprSlot = &DeclExpr->Next;
			}
		}
	} while (ml_parse(Compiler, MLT_COMMA));
}

static void ml_accept_block_let(ml_compiler_t *Compiler, ml_accept_block_t *Accept) {
	do {
		if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
			int Count = 0;
			ml_decl_t *Decl;
			do {
				ml_accept(Compiler, MLT_IDENT);
				++Count;
				Decl = Accept->LetsSlot[0] = new(ml_decl_t);
				Decl->Source = Compiler->Source;
				Decl->Ident = Compiler->Ident;
				Decl->Flags = MLC_DECL_FORWARD;
				Accept->LetsSlot = &Decl->Next;
			} while (ml_parse(Compiler, MLT_COMMA));
			ml_accept(Compiler, MLT_RIGHT_PAREN);
			if (ml_parse(Compiler, MLT_IN)) {
				ML_EXPR(DeclExpr, decl, let_in);
				DeclExpr->Decl = Decl;
				DeclExpr->Count = Count;
				DeclExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
				Accept->ExprSlot[0] = (mlc_expr_t *)DeclExpr;
				Accept->ExprSlot = &DeclExpr->Next;
			} else {
				ml_accept(Compiler, MLT_ASSIGN);
				ML_EXPR(DeclExpr, decl, let_unpack);
				DeclExpr->Decl = Decl;
				DeclExpr->Count = Count;
				DeclExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
				Accept->ExprSlot[0] = (mlc_expr_t *)DeclExpr;
				Accept->ExprSlot = &DeclExpr->Next;
			}
		} else {
			ml_accept(Compiler, MLT_IDENT);
			ml_decl_t *Decl = Accept->LetsSlot[0] = new(ml_decl_t);
			Decl->Source = Compiler->Source;
			Decl->Ident = Compiler->Ident;
			Decl->Flags = MLC_DECL_FORWARD;
			Accept->LetsSlot = &Decl->Next;
			ML_EXPR(DeclExpr, decl, let);
			DeclExpr->Decl = Decl;
			if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
				DeclExpr->Child = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			} else {
				ml_accept(Compiler, MLT_ASSIGN);
				DeclExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
			}
			Accept->ExprSlot[0] = (mlc_expr_t *)DeclExpr;
			Accept->ExprSlot = &DeclExpr->Next;
		}
	} while (ml_parse(Compiler, MLT_COMMA));
}

static void ml_accept_block_def(ml_compiler_t *Compiler, ml_accept_block_t *Accept) {
	do {
		if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
			int Count = 0;
			ml_decl_t *Decl;
			do {
				ml_accept(Compiler, MLT_IDENT);
				++Count;
				Decl = Accept->DefsSlot[0] = new(ml_decl_t);
				Decl->Source = Compiler->Source;
				Decl->Ident = Compiler->Ident;
				Accept->DefsSlot = &Decl->Next;
			} while (ml_parse(Compiler, MLT_COMMA));
			ml_accept(Compiler, MLT_RIGHT_PAREN);
			if (ml_parse(Compiler, MLT_IN)) {
				ML_EXPR(DeclExpr, decl, def_in);
				DeclExpr->Decl = Decl;
				DeclExpr->Count = Count;
				DeclExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
				Accept->ExprSlot[0] = (mlc_expr_t *)DeclExpr;
				Accept->ExprSlot = &DeclExpr->Next;
			} else {
				ml_accept(Compiler, MLT_ASSIGN);
				ML_EXPR(DeclExpr, decl, def_unpack);
				DeclExpr->Decl = Decl;
				DeclExpr->Count = Count;
				DeclExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
				Accept->ExprSlot[0] = (mlc_expr_t *)DeclExpr;
				Accept->ExprSlot = &DeclExpr->Next;
			}
		} else {
			ml_accept(Compiler, MLT_IDENT);
			ml_decl_t *Decl = Accept->DefsSlot[0] = new(ml_decl_t);
			Decl->Source = Compiler->Source;
			Decl->Ident = Compiler->Ident;
			Accept->DefsSlot = &Decl->Next;
			ML_EXPR(DeclExpr, decl, def);
			DeclExpr->Count = 0;
			if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
				DeclExpr->Child = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			} else {
				ml_accept(Compiler, MLT_ASSIGN);
				DeclExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
			}
			DeclExpr->Decl = Decl;
			Accept->ExprSlot[0] = (mlc_expr_t *)DeclExpr;
			Accept->ExprSlot = &DeclExpr->Next;
		}
	} while (ml_parse(Compiler, MLT_COMMA));
}

static void ml_accept_block_fun(ml_compiler_t *Compiler, ml_accept_block_t *Accept) {
	if (ml_parse(Compiler, MLT_IDENT)) {
		ml_decl_t *Decl = Accept->LetsSlot[0] = new(ml_decl_t);
		Decl->Source = Compiler->Source;
		Decl->Ident = Compiler->Ident;
		Decl->Flags = MLC_DECL_FORWARD;
		Accept->LetsSlot = &Decl->Next;
		ml_accept(Compiler, MLT_LEFT_PAREN);
		ML_EXPR(DeclExpr, decl, let);
		DeclExpr->Decl = Decl;
		DeclExpr->Child = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
		Accept->ExprSlot[0] = (mlc_expr_t *)DeclExpr;
		Accept->ExprSlot = &DeclExpr->Next;
	} else {
		ml_accept(Compiler, MLT_LEFT_PAREN);
		mlc_expr_t *Expr = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
		Accept->ExprSlot[0] = Expr;
		Accept->ExprSlot = &Expr->Next;
	}
}

static mlc_expr_t *ml_accept_block_export(ml_compiler_t *Compiler, mlc_expr_t *Expr, ml_decl_t *Export) {
	ML_EXPR(CallExpr, parent, call);
	CallExpr->Child = Expr;
	ml_value_t *Names = ml_names();
	ML_EXPR(NamesExpr, value, value);
	NamesExpr->Value = Names;
	Expr->Next = (mlc_expr_t *)NamesExpr;
	mlc_expr_t **ArgsSlot = &NamesExpr->Next;
	while (Export) {
		ml_names_add(Names, ml_method(Export->Ident));
		ML_EXPR(IdentExpr, ident, ident);
		IdentExpr->Ident = Export->Ident;
		ArgsSlot[0] = (mlc_expr_t *)IdentExpr;
		ArgsSlot = &IdentExpr->Next;
		Export = Export->Next;
	}
	return (mlc_expr_t *)CallExpr;
}

static mlc_expr_t *ml_accept_block(ml_compiler_t *Compiler, int NoCatches) {
	//if (Compiler->Token == MLT_NONE) ml_advance(Compiler);
	ML_EXPR(BlockExpr, block, block);
	ml_accept_block_t Accept[1];
	Accept->ExprSlot = &BlockExpr->Child;
	Accept->VarsSlot = &BlockExpr->Vars;
	Accept->LetsSlot = &BlockExpr->Lets;
	Accept->DefsSlot = &BlockExpr->Defs;
	for (;;) {
		while (ml_parse(Compiler, MLT_EOL));
		switch (ml_next(Compiler)) {
		case MLT_VAR: {
			Compiler->Token = MLT_NONE;
			ml_accept_block_var(Compiler, Accept);
			break;
		}
		case MLT_LET: {
			Compiler->Token = MLT_NONE;
			ml_accept_block_let(Compiler, Accept);
			break;
		}
		case MLT_DEF: {
			Compiler->Token = MLT_NONE;
			ml_accept_block_def(Compiler, Accept);
			break;
		}
		case MLT_FUN: {
			Compiler->Token = MLT_NONE;
			ml_accept_block_fun(Compiler, Accept);
			break;
		}
		case MLT_ON: {
			if (NoCatches) goto end;
			Compiler->Token = MLT_NONE;
			mlc_catch_expr_t **CatchSlot = &BlockExpr->Catches;
			do {
				mlc_catch_expr_t *CatchExpr = CatchSlot[0] = new(mlc_catch_expr_t);
				CatchSlot = &CatchExpr->Next;
				ml_accept(Compiler, MLT_IDENT);
				ml_decl_t *Decl = CatchExpr->Decl = new(ml_decl_t);
				Decl->Source = Compiler->Source;
				Decl->Ident = Compiler->Ident;
				if (ml_parse(Compiler, MLT_COLON)) {
					mlc_catch_type_t **TypeSlot = &CatchExpr->Types;
					do {
						ml_accept(Compiler, MLT_VALUE);
						ml_value_t *Value = Compiler->Value;
						if (!ml_is(Value, MLStringT)) {
							ml_compiler_error(Compiler, "ParseError", "Expected <string> not <%s>", ml_typeof(Value)->Name);
						}
						mlc_catch_type_t *Type = TypeSlot[0] = new(mlc_catch_type_t);
						TypeSlot = &Type->Next;
						Type->Type = ml_string_value(Value);
					} while (ml_parse(Compiler, MLT_COMMA));
				}
				ml_accept(Compiler, MLT_DO);
				CatchExpr->Body = ml_accept_block(Compiler, 1);
			} while (ml_parse(Compiler, MLT_ON));
			goto end;
		}
		default: {
			mlc_expr_t *Expr = ml_parse_expression(Compiler, EXPR_DEFAULT);
			if (!Expr) goto end;
			if (ml_parse(Compiler, MLT_COLON)) {
				if (ml_parse(Compiler, MLT_VAR)) {
					ml_decl_t **Exports = Accept->VarsSlot;
					ml_accept_block_var(Compiler, Accept);
					Expr = ml_accept_block_export(Compiler, Expr, Exports[0]);
				} else if (ml_parse(Compiler, MLT_LET)) {
					ml_decl_t **Exports = Accept->LetsSlot;
					ml_accept_block_let(Compiler, Accept);
					Expr = ml_accept_block_export(Compiler, Expr, Exports[0]);
				} else if (ml_parse(Compiler, MLT_DEF)) {
					ml_decl_t **Exports = Accept->DefsSlot;
					ml_accept_block_def(Compiler, Accept);
					Expr = ml_accept_block_export(Compiler, Expr, Exports[0]);
				} else if (ml_parse(Compiler, MLT_FUN)) {
					ml_decl_t **Exports = Accept->LetsSlot;
					ml_accept_block_fun(Compiler, Accept);
					Expr = ml_accept_block_export(Compiler, Expr, Exports[0]);
				} else {
					ml_accept(Compiler, MLT_IDENT);
					ml_decl_t *Decl = Accept->DefsSlot[0] = new(ml_decl_t);
					Decl->Source = Compiler->Source;
					Decl->Ident = Compiler->Ident;
					Accept->DefsSlot = &Decl->Next;
					ML_EXPR(DeclExpr, decl, def);
					DeclExpr->Decl = Decl;
					ml_accept(Compiler, MLT_LEFT_PAREN);
					ml_accept_arguments(Compiler, MLT_RIGHT_PAREN, &Expr->Next);
					ML_EXPR(CallExpr, parent, call);
					CallExpr->Child = Expr;
					DeclExpr->Child = (mlc_expr_t *)CallExpr;
					Expr = (mlc_expr_t *)DeclExpr;
				}
			}
			Accept->ExprSlot[0] = Expr;
			Accept->ExprSlot = &Expr->Next;
			break;
		}
		}
		if (ml_parse(Compiler, MLT_SEMICOLON)) continue;
		if (ml_parse(Compiler, MLT_EOL)) continue;
		break;
	}
end:
	BlockExpr->Source = Compiler->Source;
	return (mlc_expr_t *)BlockExpr;
}

ml_value_t *ml_compile(mlc_expr_t *Expr, const char **Parameters, ml_compiler_t *Compiler) {
	mlc_function_t Function[1];
	memset(Function, 0, sizeof(mlc_function_t));
	Function->Compiler = Compiler;
	Function->ReturnInst = ml_inst_new(0, Compiler->Source, MLI_RETURN);
	SHA256_CTX HashCompiler[1];
	sha256_init(HashCompiler);
	ml_closure_info_t *Info = new(ml_closure_info_t);
	int NumParams = 0;
	if (Parameters) {
		ml_decl_t **ParamSlot = &Function->Decls;
		for (const char **P = Parameters; P[0]; ++P) {
			ml_decl_t *Param = new(ml_decl_t);
			Param->Source = Expr->Source;
			Param->Ident = P[0];
			Param->Index = Function->Top++;
			stringmap_insert(Info->Params, Param->Ident, (void *)(intptr_t)Function->Top);
			ParamSlot[0] = Param;
			ParamSlot = &Param->Next;
		}
		NumParams = Function->Top;
		Function->Size = Function->Top + 1;
	}
	mlc_compiled_t Compiled = mlc_compile(Function, Expr);
	mlc_connect(Compiled.Exits, Function->ReturnInst);
	Info->Entry = Compiled.Start;
	Info->Return = Function->ReturnInst;
	Info->Source = Expr->Source.Name;
	Info->LineNo = Expr->Source.Line;
	Info->FrameSize = Function->Size;
	Info->NumParams = NumParams;
	ml_closure_t *Closure = new(ml_closure_t);
	Closure->Info = Info;
	Closure->Type = MLClosureT;
	ml_task_closure_info_t *Task = new(ml_task_closure_info_t);
	Task->Base.start = (void *)ml_task_closure_info_start;
	Task->Base.finish = (void *)ml_task_default_finish;
	Task->Base.Source = Expr->Source;
	Task->Info = Info;
	ml_task_queue(Function->Compiler, (ml_compiler_task_t *)Task);
	return (ml_value_t *)Closure;
}

static void ml_task_closure_start(ml_compiler_task_t *Task, ml_compiler_t *Compiler) {
	ml_tasks_state_run(Compiler, Task->Closure);
}

void ml_function_compile(ml_state_t *Caller, ml_compiler_t *Compiler, const char **Parameters) {
	MLC_ON_ERROR(Compiler) ML_RETURN(Compiler->Error);
	Compiler->Context = Caller->Context;
	Compiler->Base.Caller = Caller;
	Compiler->Base.Context = Caller->Context;
	mlc_expr_t *Block = ml_accept_block(Compiler, 0);
	ml_accept_eoi(Compiler);
	ml_compiler_task_t *Task = new(ml_compiler_task_t);
	Task->Closure = ml_compile(Block, Parameters, Compiler);
	Task->start = (void *)ml_task_closure_start;
	Task->finish = (void *)ml_task_default_finish;
	Task->Source = Block->Source;
	ml_task_queue(Compiler, Task);
	Compiler->Tasks->start(Compiler->Tasks, Compiler);
}

ml_value_t MLEndOfInput[1] = {{MLAnyT}};

typedef struct {
	ml_compiler_task_t Base;
	ml_variable_t *Var;
} ml_command_var_t;

static ml_value_t *ml_command_var_finish(ml_command_var_t *Task, ml_value_t *Value) {
	Task->Var->Value = Value;
	return NULL;
}

typedef struct {
	ml_compiler_task_t Base;
	ml_value_t **Slot;
	ml_value_t *Args[2];
} ml_command_import_t;

typedef struct {
	ml_compiler_task_t Base;
	ml_value_t **Slot;
	ml_command_import_t *Unpacks;
	int NumImports, NumUnpack;
} ml_command_def_t;

static ml_value_t *ml_command_def_finish(ml_command_def_t *Task, ml_value_t *Value) {
	if (Task->Slot) {
		ml_uninitialized_set(Task->Slot[0], Value);
		Task->Slot[0] = Value;
	}
	if (Task->NumUnpack) {
		int Index = 0;
		for (ml_command_import_t *Unpack = Task->Unpacks; Unpack; Unpack = (ml_command_import_t *)Unpack->Base.Next, ++Index) {
			ml_value_t *Unpacked = ml_unpack(Value, Index);
			if (!Unpacked) return ml_error("ValueError", "Not enough values to unpack (%d < %d)", Index, Task->NumUnpack);
			ml_uninitialized_set(Unpack->Slot[0], Unpacked);
			Unpack->Slot[0] = Unpacked;
		}
	} else if (Task->NumImports) {
		ml_command_import_t *Import = (ml_command_import_t *)Task->Base.Next;
		for (int I = 0; I < Task->NumImports; ++I) {
			Import->Args[0] = Value;
			Import = (ml_command_import_t *)Import->Base.Next;
		}
	}
	return NULL;
}

static void ml_command_import_start(ml_command_import_t *Task, ml_compiler_t *Compiler) {
	ml_call((ml_state_t *)Compiler, SymbolMethod, 2, Task->Args);
}

static ml_value_t *ml_command_import_finish(ml_command_import_t *Task, ml_value_t *Value) {
	ml_uninitialized_set(Task->Slot[0], Value);
	Task->Slot[0] = Value;
	return NULL;
}

void ml_command_evaluate(ml_state_t *Caller, ml_compiler_t *Compiler, stringmap_t *Vars) {
	MLC_ON_ERROR(Compiler) {
		ml_compiler_reset(Compiler);
		ML_RETURN(Compiler->Error);
	}
	while (ml_parse(Compiler, MLT_EOL));
	if (ml_parse(Compiler, MLT_EOI)) ML_RETURN(MLEndOfInput);
	Compiler->Context = Caller->Context;
	Compiler->Base.Caller = Caller;
	Compiler->Base.Context = Caller->Context;
	if (ml_parse(Compiler, MLT_VAR)) {
		do {
			ml_accept(Compiler, MLT_IDENT);
			const char *Ident = Compiler->Ident;
			ml_variable_t *Var = new(ml_variable_t);
			Var->Type = MLVariableT;
			Var->Value = MLNil;
			ml_value_t **Slot = (ml_value_t **)stringmap_slot(Vars, Ident);
			if (Slot[0] && ml_typeof(Slot[0]) == MLUninitializedT) {
				ml_uninitialized_set(Slot[0], (ml_value_t *)Var);
			}
			Slot[0] = (ml_value_t *)Var;
			if (ml_parse(Compiler, MLT_ASSIGN)) {
				mlc_expr_t *Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
				ml_command_var_t *Task = new(ml_command_var_t);
				Task->Base.start = ml_task_default_start;
				Task->Base.finish = (void *)ml_command_var_finish;
				Task->Base.Closure = ml_compile(Expr, NULL, Compiler);
				Task->Base.Source = Expr->Source;
				Task->Var = Var;
				ml_task_queue(Compiler, (ml_compiler_task_t *)Task);
			}
		} while (ml_parse(Compiler, MLT_COMMA));
	} else if (ml_parse(Compiler, MLT_LET) || ml_parse(Compiler, MLT_DEF)) {
		do {
			ml_command_def_t *Task = new(ml_command_def_t);
			Task->Base.start = ml_task_default_start;
			Task->Base.finish = (void *)ml_command_def_finish;
			ml_task_queue(Compiler, (ml_compiler_task_t *)Task);
			if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
				ml_command_import_t **Unpacks = &Task->Unpacks;
				do {
					ml_accept(Compiler, MLT_IDENT);
					const char *Ident = Compiler->Ident;
					ml_value_t **Slot = (ml_value_t **)stringmap_slot(Vars, Ident);
					if (!Slot[0] || ml_typeof(Slot[0]) != MLUninitializedT) {
						Slot[0] = ml_uninitialized(Ident);
					}
					++Task->NumUnpack;
					ml_command_import_t *Unpack = Unpacks[0] = new(ml_command_import_t);
					Unpacks = (ml_command_import_t **)&Unpack->Base.Next;
					Unpack->Slot = Slot;
					Unpack->Args[1] = ml_cstring(Ident);
				} while (ml_parse(Compiler, MLT_COMMA));
				ml_accept(Compiler, MLT_RIGHT_PAREN);
				mlc_expr_t *Expr;
				if (ml_parse(Compiler, MLT_IN)) {
					Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
					Task->NumImports = Task->NumUnpack;
					Task->NumUnpack = 0;
					ml_command_import_t *Import = Task->Unpacks;
					while (Import) {
						ml_command_import_t *Next = (ml_command_import_t *)Import->Base.Next;
						Import->Base.start = (void *)ml_command_import_start;
						Import->Base.finish = (void *)ml_command_import_finish;
						Import->Base.Source = Expr->Source;
						ml_task_queue(Compiler, (ml_compiler_task_t *)Import);
						Import = Next;
					}
				} else {
					ml_accept(Compiler, MLT_ASSIGN);
					Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
				}
				Task->Base.Closure = ml_compile(Expr, NULL, Compiler);
				Task->Base.Source = Expr->Source;
			} else {
				ml_accept(Compiler, MLT_IDENT);
				const char *Ident = Compiler->Ident;
				mlc_expr_t *Expr;
				if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
					Expr = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
				} else {
					ml_accept(Compiler, MLT_ASSIGN);
					Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
				}
				Task->Base.Closure = ml_compile(Expr, NULL, Compiler);
				Task->Base.Source = Expr->Source;
				ml_value_t **Slot = (ml_value_t **)stringmap_slot(Vars, Ident);
				if (!Slot[0] || ml_typeof(Slot[0]) != MLUninitializedT) {
					Slot[0] = ml_uninitialized(Ident);
				}
				Task->Slot = Slot;
			}
		} while (ml_parse(Compiler, MLT_COMMA));
	} else if (ml_parse(Compiler, MLT_FUN)) {
		if (ml_parse(Compiler, MLT_IDENT)) {
			const char *Ident = Compiler->Ident;
			ml_value_t **Slot = (ml_value_t **)stringmap_slot(Vars, Ident);
			if (!Slot[0] || ml_typeof(Slot[0]) != MLUninitializedT) {
				Slot[0] = ml_uninitialized(Ident);
			}
			ml_accept(Compiler, MLT_LEFT_PAREN);
			mlc_expr_t *Expr = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			ml_command_def_t *Task = new(ml_command_def_t);
			Task->Base.start = ml_task_default_start;
			Task->Base.finish = (void *)ml_command_def_finish;
			Task->Base.Closure = ml_compile(Expr, NULL, Compiler);
			Task->Base.Source = Expr->Source;
			Task->Slot = Slot;
			ml_task_queue(Compiler, (ml_compiler_task_t *)Task);
		} else {
			ml_accept(Compiler, MLT_LEFT_PAREN);
			mlc_expr_t *Expr = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			ml_compiler_task_t *Task = new(ml_compiler_task_t);
			Task->start = ml_task_default_start;
			Task->finish = ml_task_default_finish;
			Task->Closure = ml_compile(Expr, NULL, Compiler);
			Task->Source = Expr->Source;
			ml_task_queue(Compiler, (ml_compiler_task_t *)Task);
		}
	} else {
		mlc_expr_t *Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
		if (ml_parse(Compiler, MLT_COLON)) {
			ml_accept(Compiler, MLT_IDENT);
			const char *Ident = Compiler->Ident;
			ML_EXPR(CallExpr, parent, call);
			CallExpr->Child = Expr;
			ml_accept(Compiler, MLT_LEFT_PAREN);
			ml_accept_arguments(Compiler, MLT_RIGHT_PAREN, &Expr->Next);
			ml_value_t **Slot = (ml_value_t **)stringmap_slot(Vars, Ident);
			if (!Slot[0] || ml_typeof(Slot[0]) != MLUninitializedT) {
				Slot[0] = ml_uninitialized(Ident);
			}
			ml_command_def_t *Task = new(ml_command_def_t);
			Task->Base.start = ml_task_default_start;
			Task->Base.finish = (void *)ml_command_def_finish;
			Task->Base.Closure = ml_compile((mlc_expr_t *)CallExpr, NULL, Compiler);
			Task->Base.Source = Expr->Source;
			Task->Slot = Slot;
			ml_task_queue(Compiler, (ml_compiler_task_t *)Task);
		} else {
			ml_compiler_task_t *Task = new(ml_compiler_task_t);
			Task->start = ml_task_default_start;
			Task->finish = ml_task_default_finish;
			Task->Closure = ml_compile(Expr, NULL, Compiler);
			Task->Source = Expr->Source;
			ml_task_queue(Compiler, (ml_compiler_task_t *)Task);
		}
	}
	ml_parse(Compiler, MLT_SEMICOLON);
	if (Compiler->Tasks) {
		Compiler->Tasks->start(Compiler->Tasks, Compiler);
	} else {
		ML_RETURN(MLNil);
	}
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

typedef struct {
	ml_state_t Base;
	FILE *File;
} ml_load_file_state_t;

static void ml_load_file_state_run(ml_load_file_state_t *State, ml_value_t *Value) {
	fclose(State->File);
	ml_state_t *Caller = State->Base.Caller;
	ML_RETURN(Value);
}

void ml_load_file(ml_state_t *Caller, ml_getter_t GlobalGet, void *Globals, const char *FileName, const char *Parameters[]) {
	FILE *File = fopen(FileName, "r");
	if (!File) ML_RETURN(ml_error("LoadError", "error opening %s", FileName));
	ml_compiler_t *Compiler = ml_compiler(ml_file_read, File, GlobalGet, Globals);
	ml_compiler_source(Compiler, (ml_source_t){FileName, 1});
	ml_load_file_state_t *State = new(ml_load_file_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_load_file_state_run;
	State->File = File;
	return ml_function_compile((ml_state_t *)State, Compiler, Parameters);
}

void ml_compiler_init() {
#include "ml_compiler_init.c"
	stringmap_insert(StringFns, "r", ml_regex);
}

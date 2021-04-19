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

#define MINICORO_IMPL
#define MCO_NO_DEFAULT_ALLOCATORS
#define MCO_API static
#include "minicoro.h"

typedef struct mlc_expr_t mlc_expr_t;

typedef struct mlc_function_t mlc_function_t;
typedef struct mlc_loop_t mlc_loop_t;
typedef struct mlc_block_t mlc_block_t;
typedef struct mlc_try_t mlc_try_t;
typedef struct mlc_upvalue_t mlc_upvalue_t;

struct mlc_upvalue_t {
	mlc_upvalue_t *Next;
	ml_decl_t *Decl;
	int Index;
};

struct mlc_loop_t {
	mlc_loop_t *Up;
	mlc_try_t *Try;
	ml_decl_t *NextDecls, *ExitDecls;
	ml_inst_t *Nexts, *Exits;
	int NextTop, ExitTop;
};

struct mlc_block_t {
	mlc_block_t *Up;
	ml_decl_t **Decls;
	int Top;
};

struct mlc_try_t {
	mlc_try_t *Up;
	ml_inst_t *Retries;
	int Top;
};

struct mlc_expr_t {
	int (*compile)(mlc_function_t *, mlc_expr_t *, int);
	mlc_expr_t *Next;
	int StartLine, EndLine;
};

#define MLC_EXPR_FIELDS(name) \
	int (*compile)(mlc_function_t *, mlc_## name ## _expr_t *, int); \
	mlc_expr_t *Next; \
	int StartLine, EndLine;

#define ML_EXPR(EXPR, TYPE, COMP) \
	mlc_ ## TYPE ## _expr_t *EXPR = new(mlc_ ## TYPE ## _expr_t); \
	EXPR->compile = ml_ ## COMP ## _expr_compile; \
	EXPR->StartLine = EXPR->EndLine = Compiler->Source.Line

#define ML_EXPR_END(EXPR) (((mlc_expr_t *)EXPR)->EndLine = Compiler->Source.Line, (mlc_expr_t *)EXPR)

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
	MLT_MACRO,
	MLT_RET,
	MLT_SUSP,
	MLT_DEBUG,
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
	MLT_REF,
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
	MLT_EXPAND,
	MLT_OPERATOR,
	MLT_METHOD
} ml_token_t;

typedef enum {
	MLCS_RETURN,
	MLCS_CALL,
	MLCS_FUNCTION,
	MLCS_COMMAND
} ml_compiler_state_t;

struct ml_compiler_t {
	ml_state_t Base;
	mco_coro *Coroutine;
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
	union { ml_value_t **Args; const char **Params; };
	ml_source_t Source;
	stringmap_t Vars[1];
	int Line;
	ml_token_t Token;
	volatile ml_compiler_state_t State;
	int Count;
	jmp_buf OnError;
};

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

extern ml_value_t *IndexMethod;
extern ml_value_t *SymbolMethod;

long ml_ident_hash(const char *Ident) {
	long Hash = 5381;
	while (*Ident) Hash = ((Hash << 5) + Hash) + *Ident++;
	return Hash;
}

typedef struct mlc_define_t mlc_define_t;

struct mlc_define_t {
	mlc_define_t *Next;
	const char *Ident;
	mlc_expr_t *Expr;
};

typedef struct ml_macro_t ml_macro_t;

struct ml_macro_t {
	ml_type_t *Type;
	int (*apply)(mlc_function_t *, ml_macro_t *, mlc_expr_t *, mlc_expr_t *, int);
};

typedef struct ml_define_t ml_define_t;

struct ml_define_t {
	ml_define_t *Next;
	const char *Ident;
	mlc_expr_t *Expr;
	long Hash;
	ml_source_t Source;
};

ML_TYPE(MLMacroT, (), "macro");

struct mlc_function_t {
	ml_compiler_t *Compiler;
	const char *Source;
	mlc_function_t *Up;
	mlc_block_t *Block;
	ml_decl_t *Decls;
	ml_define_t *Defines;
	mlc_loop_t *Loop;
	mlc_try_t *Try;
	mlc_upvalue_t *UpValues;
	ml_inst_t *Next, *Returns;
	ml_value_t *Constant;
	int Top, Size, Self, Space;
	int Local, BlockTop;
};

static ml_inst_t *ml_inst_alloc(mlc_function_t *Function, int Line, ml_opcode_t Opcode, int N) {
	int Count = N + 1;
	if (Function->Space < Count) {
		ml_inst_t *GotoInst = Function->Next;
		GotoInst->Opcode = MLI_LINK;
		GotoInst->Line = Line;
		GotoInst[1].Inst = Function->Next = anew(ml_inst_t, 128);
		Function->Space = 126;
	}
	ml_inst_t *Inst = Function->Next;
	Function->Next += Count;
	Function->Space -= Count;
	Inst->Opcode = Opcode;
	Inst->Line = Line;
	return Inst;
}

static inline void mlc_fix_links(ml_inst_t *Start, ml_inst_t *Target) {
	while (Start) {
		ml_inst_t *Next = Start->Inst;
		Start->Inst = Target;
		Start = Next;
	}
}

#define mlc_emit(LINE, OPCODE, N) ml_inst_alloc(Function, LINE, OPCODE, N)
#define mlc_link(START, TARGET) mlc_fix_links(START, TARGET)

static inline void mlc_inc_top(mlc_function_t *Function) {
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
}

#define MLC_JUMP 1 << 31
#define MLC_PUSH 1 << 30
#define MLC_LOCAL 1 << 29
#define MLC_CONSTANT 1 << 28

static inline int mlc_compile(mlc_function_t *Function, mlc_expr_t *Expr, int Flags) {
	int Result = Expr->compile(Function, Expr, Flags);
	if ((Flags & MLC_PUSH) && !(Result & MLC_PUSH)) {
		mlc_emit(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	return Result;
}

#define ml_expr_error(EXPR, ERROR) { \
	ml_error_trace_add(ERROR, (ml_source_t){Function->Source, EXPR->StartLine}); \
	Function->Compiler->Value = ERROR; \
	longjmp(Function->Compiler->OnError, 1); \
}

static ml_value_t *mlc_yield_call(ml_compiler_t *Compiler, ml_value_t *Value, int Count, ml_value_t **Args) {
	Compiler->Value = Value;
	Compiler->Count = Count;
	Compiler->Args = Args;
	Compiler->State = MLCS_CALL;
	mco_yield(Compiler->Coroutine);
	return Compiler->Value;
}

static ml_value_t *ml_compile(ml_compiler_t *Compiler, mlc_expr_t *Expr, const char **Parameters);

static ml_value_t *mlc_yield_compile_call(ml_compiler_t *Compiler, mlc_expr_t *Expr) {
	Compiler->Value = ml_compile(Compiler, Expr, NULL);
	Compiler->Count = 0;
	Compiler->Args = NULL;
	Compiler->State = MLCS_CALL;
	mco_yield(Compiler->Coroutine);
	ml_value_t *Value = Compiler->Value;
	if (ml_is_error(Value)) {
		ml_error_trace_add(Value, (ml_source_t){Compiler->Source.Name, Expr->EndLine});
		Compiler->Value = Value;
		longjmp(Compiler->OnError, 1);
	}
	return Value;
}

static ml_value_t *ml_expr_compile(mlc_expr_t *Expr, mlc_function_t *Parent) {
	mlc_function_t Function[1];
	memset(Function, 0, sizeof(Function));
	Function->Compiler = Parent->Compiler;
	Function->Source = Parent->Source;
	Function->Up = Parent;
	Function->Size = 1;
	Function->Next = anew(ml_inst_t, 128);
	Function->Space = 126;
	Function->Returns = NULL;
	ml_closure_info_t *Info = new(ml_closure_info_t);
	Info->Entry = Function->Next;
	mlc_compile(Function, Expr, 0);
	if (Function->UpValues) {
		ml_expr_error(Expr, ml_error("EvalError", "Use of non-constant value in constant expression"));
	}
	Info->Return = mlc_emit(Expr->EndLine, MLI_RETURN, 0);
	mlc_link(Function->Returns, Info->Return);
	Info->Halt = Function->Next;
	Info->Source = Function->Source;
	Info->StartLine = Expr->StartLine;
	Info->EndLine = Expr->EndLine;
	Info->FrameSize = Function->Size;
	Info->NumParams = 0;
	ml_closure_t *Closure = new(ml_closure_t);
	Closure->Type = MLClosureT;
	Closure->Info = Info;
	ml_closure_info_finish(Info);
	return (ml_value_t *)Closure;
}

static ml_value_t *mlc_yield_expr_call(mlc_function_t *Function, mlc_expr_t *Expr) {
	ml_value_t *Value = mlc_yield_call(Function->Compiler, ml_expr_compile(Expr, Function), 0, NULL);
	if (ml_is_error(Value)) {
		ml_error_trace_add(Value, (ml_source_t){Function->Source, Expr->EndLine});
		Function->Compiler->Value = Value;
		longjmp(Function->Compiler->OnError, 1);
	}
	return Value;
}

extern ml_value_t MLBlank[];

static int ml_blank_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr, int Flags) {
	ml_inst_t *Inst = mlc_emit(Expr->StartLine, MLI_LOAD, 1);
	Inst[1].Value = MLBlank;
	if (Flags & MLC_PUSH) {
		Inst->Opcode = MLI_LOAD_PUSH;
		mlc_inc_top(Function);
		return MLC_PUSH;
	}
	return 0;
}

static int ml_nil_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr, int Flags) {
	ml_inst_t *Inst = mlc_emit(Expr->StartLine, MLI_NIL, 0);
	if (Flags & MLC_PUSH) {
		Inst->Opcode = MLI_NIL_PUSH;
		mlc_inc_top(Function);
		return MLC_PUSH;
	}
	return 0;
}

typedef struct mlc_value_expr_t mlc_value_expr_t;

struct mlc_value_expr_t {
	MLC_EXPR_FIELDS(value);
	ml_value_t *Value;
};

static int ml_value_expr_compile(mlc_function_t *Function, mlc_value_expr_t *Expr, int Flags) {
	if (Flags & MLC_CONSTANT) {
		Function->Constant = Expr->Value;
		return MLC_CONSTANT | MLC_PUSH;
	}
	ml_inst_t *Inst = mlc_emit(Expr->StartLine, MLI_LOAD, 1);
	Inst[1].Value = Expr->Value;
	if (Flags & MLC_PUSH) {
		Inst->Opcode = MLI_LOAD_PUSH;
		mlc_inc_top(Function);
		return MLC_PUSH;
	}
	return 0;
}

typedef struct mlc_if_expr_t mlc_if_expr_t;
typedef struct mlc_if_case_t mlc_if_case_t;

struct mlc_if_case_t {
	mlc_if_case_t *Next;
	mlc_expr_t *Condition;
	mlc_expr_t *Body;
	const char *Ident;
	int Line;
	ml_token_t Token;
};

struct mlc_if_expr_t {
	MLC_EXPR_FIELDS(if);
	mlc_if_case_t *Cases;
	mlc_expr_t *Else;
};

static int ml_if_expr_compile(mlc_function_t *Function, mlc_if_expr_t *Expr, int Flags) {
	ml_decl_t *OldDecls = Function->Decls;
	mlc_if_case_t *Case = Expr->Cases;
	mlc_compile(Function, Case->Condition, 0);
	int CaseLine = Case->Condition->EndLine;
	ml_inst_t *IfInst = mlc_emit(CaseLine, MLI_AND, 1);
	if (Case->Ident) {
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source.Name = Function->Source;
		Decl->Source.Line = Case->Line;
		Decl->Ident = Case->Ident;
		Decl->Hash = ml_ident_hash(Case->Ident);
		Decl->Index = Function->Top;
		mlc_inc_top(Function);
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
		ml_inst_t *WithInst = mlc_emit(CaseLine, Case->Token == MLT_VAR ? MLI_WITH_VAR : MLI_WITH, 1);
		WithInst[1].Decls = Function->Decls;
	}
	int Result = mlc_compile(Function, Case->Body, 0);
	if (Case->Ident) {
		Function->Decls = OldDecls;
		--Function->Top;
		ml_inst_t *ExitInst = mlc_emit(Expr->EndLine, MLI_EXIT, 2);
		ExitInst[1].Count = 1;
		ExitInst[2].Decls = Function->Decls;
	}
	ml_inst_t *Exits = NULL;
	while (Case->Next) {
		if (!(Result & MLC_JUMP)) {
			ml_inst_t *GotoInst = mlc_emit(Case->Body->EndLine, MLI_GOTO, 1);
			GotoInst[1].Inst = Exits;
			Exits = GotoInst + 1;
		}
		Case = Case->Next;
		CaseLine = Case->Condition->EndLine;
		IfInst[1].Inst = Function->Next;
		mlc_compile(Function, Case->Condition, 0);
		IfInst = mlc_emit(CaseLine, MLI_AND, 1);
		if (Case->Ident) {
			ml_decl_t *Decl = new(ml_decl_t);
			Decl->Source.Name = Function->Source;
			Decl->Source.Line = Case->Line;
			Decl->Ident = Case->Ident;
			Decl->Hash = ml_ident_hash(Case->Ident);
			Decl->Index = Function->Top;
			mlc_inc_top(Function);
			Decl->Next = Function->Decls;
			Function->Decls = Decl;
			ml_inst_t *WithInst = mlc_emit(CaseLine, Case->Token == MLT_VAR ? MLI_WITH_VAR : MLI_WITH, 1);
			WithInst[1].Decls = Function->Decls;
		}
		Result = mlc_compile(Function, Case->Body, 0);
		if (Case->Ident) {
			Function->Decls = OldDecls;
			--Function->Top;
			ml_inst_t *ExitInst = mlc_emit(Expr->EndLine, MLI_EXIT, 2);
			ExitInst[1].Count = 1;
			ExitInst[2].Decls = Function->Decls;
		}
	}
	if (Expr->Else) {
		if (!(Result & MLC_JUMP)) {
			ml_inst_t *GotoInst = mlc_emit(Case->Body->EndLine, MLI_GOTO, 1);
			GotoInst[1].Inst = Exits;
			Exits = GotoInst + 1;
		}
		IfInst[1].Inst = Function->Next;
		mlc_compile(Function, Expr->Else, 0);
	} else {
		IfInst[1].Inst = Function->Next;
	}
	mlc_link(Exits, Function->Next);
	return 0;
}

typedef struct mlc_parent_expr_t mlc_parent_expr_t;

struct mlc_parent_expr_t {
	MLC_EXPR_FIELDS(parent);
	mlc_expr_t *Child;
};

static int ml_or_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	mlc_expr_t *Child = Expr->Child;
	int EndLine = Child->EndLine;
	mlc_compile(Function, Child, 0);
	ml_inst_t *Exits = 0;
	while ((Child = Child->Next)) {
		ml_inst_t *OrInst = mlc_emit(EndLine, MLI_OR, 1);
		EndLine = Child->EndLine;
		OrInst[1].Inst = Exits;
		Exits = OrInst + 1;
		mlc_compile(Function, Child, 0);
	}
	mlc_link(Exits, Function->Next);
	return 0;
}

static int ml_and_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	mlc_expr_t *Child = Expr->Child;
	int EndLine = Child->EndLine;
	mlc_compile(Function, Child, 0);
	ml_inst_t *Exits = 0;
	while ((Child = Child->Next)) {
		ml_inst_t *AndInst = mlc_emit(EndLine, MLI_AND, 1);
		EndLine = Child->EndLine;
		AndInst[1].Inst = Exits;
		Exits = AndInst + 1;
		mlc_compile(Function, Child, 0);
	}
	mlc_link(Exits, Function->Next);
	return 0;
}

static int ml_not_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	mlc_compile(Function, Expr->Child, 0);
	mlc_emit(Expr->EndLine, MLI_NOT, 0);
	return 0;
}

static int ml_loop_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	mlc_loop_t Loop = {
		Function->Loop, Function->Try,
		Function->Decls, Function->Decls,
		NULL, NULL,
		Function->Top, Function->Top
	};
	Function->Loop = &Loop;
	ml_inst_t *Next = Function->Next;
	mlc_compile(Function, Expr->Child, 0);
	mlc_link(Loop.Nexts, Next);
	ml_inst_t *GotoInst = mlc_emit(Expr->EndLine, MLI_GOTO, 1);
	GotoInst[1].Inst = Next;
	mlc_link(Loop.Exits, Function->Next);
	Function->Loop = Loop.Up;
	return 0;
}

static int ml_next_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr, int Flags) {
	mlc_loop_t *Loop = Function->Loop;
	if (!Loop) ml_expr_error(Expr, ml_error("CompilerError", "next not in loop"));
	if (Function->Try != Loop->Try) {
		ml_inst_t *TryInst = mlc_emit(Expr->StartLine, MLI_TRY, 1);
		if (Loop->Try) {
			TryInst[1].Inst = Loop->Try->Retries;
			Loop->Try->Retries = TryInst + 1;
		} else {
			TryInst[1].Inst = Function->Returns;
			Function->Returns = TryInst + 1;
		}
	}
	if (Function->Top > Loop->NextTop) {
		ml_inst_t *ExitInst = mlc_emit(Expr->EndLine, MLI_EXIT, 2);
		ExitInst[1].Count = Function->Top - Loop->NextTop;
		ExitInst[2].Decls = Loop->NextDecls;
	}
	ml_inst_t *GotoInst = mlc_emit(Expr->EndLine, MLI_GOTO, 1);
	GotoInst[1].Inst = Loop->Nexts;
	Loop->Nexts = GotoInst + 1;
	return MLC_JUMP;
}

static int ml_exit_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	mlc_loop_t *Loop = Function->Loop;
	if (!Loop) ml_expr_error(Expr, ml_error("CompilerError", "exit not in loop"));
	if (Function->Try != Loop->Try) {
		ml_inst_t *TryInst = mlc_emit(Expr->StartLine, MLI_TRY, 1);
		if (Loop->Try) {
			TryInst[1].Inst = Loop->Try->Retries;
			Loop->Try->Retries = TryInst + 1;
		} else {
			TryInst[1].Inst = Function->Returns;
			Function->Returns = TryInst + 1;
		}
	}
	int Jumps = 0;
	if (Expr->Child) {
		mlc_try_t *Try = Function->Try;
		Function->Loop = Loop->Up;
		Function->Try = Loop->Try;
		Jumps = mlc_compile(Function, Expr->Child, 0) & MLC_JUMP;
		Function->Loop = Loop;
		Function->Try = Try;
	} else {
		mlc_emit(Expr->StartLine, MLI_NIL, 0);
	}
	if (!Jumps) {
		if (Function->Top > Loop->ExitTop) {
			ml_inst_t *ExitInst = mlc_emit(Expr->EndLine, MLI_EXIT, 2);
			ExitInst[1].Count = Function->Top - Loop->ExitTop;
			ExitInst[2].Decls = Loop->ExitDecls;
		}
		ml_inst_t *GotoInst = mlc_emit(Expr->EndLine, MLI_GOTO, 1);
		GotoInst[1].Inst = Loop->Exits;
		Loop->Exits = GotoInst + 1;
	}
	return MLC_JUMP;
}

static int ml_while_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	mlc_loop_t *Loop = Function->Loop;
	if (!Loop) ml_expr_error(Expr, ml_error("CompilerError", "exit not in loop"));
	mlc_compile(Function, Expr->Child, 0);
	ml_inst_t *OrInst = mlc_emit(Expr->Child->EndLine, MLI_OR, 1);
	if (Function->Try != Loop->Try) {
		ml_inst_t *TryInst = mlc_emit(Expr->Child->EndLine, MLI_TRY, 1);
		if (Loop->Try) {
			TryInst[1].Inst = Loop->Try->Retries;
			Loop->Try->Retries = TryInst + 1;
		} else {
			TryInst[1].Inst = Function->Returns;
			Function->Returns = TryInst + 1;
		}
	}
	if (Expr->Child->Next) {
		mlc_try_t *Try = Function->Try;
		Function->Loop = Loop->Up;
		Function->Try = Loop->Try;
		mlc_compile(Function, Expr->Child->Next, 0);
		Function->Loop = Loop;
		Function->Try = Try;
	}
	if (Function->Top > Loop->ExitTop) {
		ml_inst_t *ExitInst = mlc_emit(Expr->EndLine, MLI_EXIT, 2);
		ExitInst[1].Count = Function->Top - Loop->ExitTop;
		ExitInst[2].Decls = Loop->ExitDecls;
	}
	ml_inst_t *GotoInst = mlc_emit(Expr->EndLine, MLI_GOTO, 1);
	GotoInst[1].Inst = Loop->Exits;
	Loop->Exits = GotoInst + 1;
	OrInst[1].Inst = Function->Next;
	return 0;
}

static int ml_until_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	mlc_loop_t *Loop = Function->Loop;
	if (!Loop) ml_expr_error(Expr, ml_error("CompilerError", "exit not in loop"));
	mlc_compile(Function, Expr->Child, 0);
	ml_inst_t *AndInst = mlc_emit(Expr->Child->EndLine, MLI_AND, 1);
	if (Function->Try != Loop->Try) {
		ml_inst_t *TryInst = mlc_emit(Expr->Child->EndLine, MLI_TRY, 1);
		if (Loop->Try) {
			TryInst[1].Inst = Loop->Try->Retries;
			Loop->Try->Retries = TryInst + 1;
		} else {
			TryInst[1].Inst = Function->Returns;
			Function->Returns = TryInst + 1;
		}
	}
	if (Expr->Child->Next) {
		mlc_try_t *Try = Function->Try;
		Function->Loop = Loop->Up;
		Function->Try = Loop->Try;
		mlc_compile(Function, Expr->Child->Next, 0);
		Function->Loop = Loop;
		Function->Try = Try;
	}
	if (Function->Top > Loop->ExitTop) {
		ml_inst_t *ExitInst = mlc_emit(Expr->EndLine, MLI_EXIT, 2);
		ExitInst[1].Count = Function->Top - Loop->ExitTop;
		ExitInst[2].Decls = Loop->ExitDecls;
	}
	ml_inst_t *GotoInst = mlc_emit(Expr->EndLine, MLI_GOTO, 1);
	GotoInst[1].Inst = Loop->Exits;
	Loop->Exits = GotoInst + 1;
	AndInst[1].Inst = Function->Next;
	return 0;
}

static int ml_return_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	if (Expr->Child) {
		mlc_compile(Function, Expr->Child, 0);
	} else {
		mlc_emit(Expr->StartLine, MLI_NIL, 0);
	}
	mlc_emit(Expr->EndLine, MLI_RETURN, 0);
	return MLC_JUMP;
}

static int ml_suspend_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	mlc_expr_t *ValueExpr = Expr->Child;
	if (ValueExpr->Next) {
		mlc_compile(Function, ValueExpr, MLC_PUSH);
		ValueExpr = ValueExpr->Next;
	} else {
		mlc_emit(Expr->StartLine, MLI_NIL_PUSH, 0);
		mlc_inc_top(Function);
	}
	mlc_compile(Function, ValueExpr, MLC_PUSH);
	mlc_emit(Expr->EndLine, MLI_SUSPEND, 0);
	mlc_emit(Expr->EndLine, MLI_RESUME, 0);
	Function->Top -= 2;
	return 0;
}

static int ml_debug_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	ml_inst_t *DebugInst = mlc_emit(Expr->StartLine, MLI_IF_DEBUG, 1);
	mlc_compile(Function, Expr->Child, 0);
	DebugInst[1].Inst = Function->Next;
	return 0;
}

typedef struct mlc_local_expr_t mlc_local_expr_t;
typedef struct mlc_local_t mlc_local_t;

struct mlc_local_t {
	mlc_local_t *Next;
	const char *Ident;
	int Line, Index;
};

struct mlc_local_expr_t {
	MLC_EXPR_FIELDS(local);
	mlc_local_t *Local;
	mlc_expr_t *Child;
	int Count, Flags;
};

static int ml_var_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	mlc_compile(Function, Expr->Child, 0);
	mlc_local_t *Local = Expr->Local;
	ml_decl_t *Decl = Function->Block->Decls[Local->Index];
	ml_inst_t *VarInst = mlc_emit(Expr->EndLine, MLI_VAR, 1);
	VarInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
	Decl->Flags = 0;
	return 0;
}

static int ml_var_type_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	mlc_compile(Function, Expr->Child, 0);
	mlc_local_t *Local = Expr->Local;
	ml_inst_t *TypeInst = mlc_emit(Expr->EndLine, MLI_VAR_TYPE, 1);
	TypeInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
	return 0;
}

static int ml_var_in_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	mlc_compile(Function, Expr->Child, MLC_PUSH);
	mlc_local_t *Local = Expr->Local;
	ml_decl_t **Decls = Function->Block->Decls + Local->Index;
	for (int I = 0; I < Expr->Count; ++I, Local = Local->Next) {
		ml_inst_t *PushInst = mlc_emit(Expr->EndLine, MLI_LOCAL_PUSH, 1);
		PushInst[1].Index = Function->Top - 1;
		mlc_inc_top(Function);
		ml_inst_t *ValueInst = mlc_emit(Expr->EndLine, MLI_LOAD_PUSH, 1);
		ValueInst[1].Value = ml_cstring(Local->Ident);
		mlc_inc_top(Function);
		ml_inst_t *CallInst = mlc_emit(Expr->EndLine, MLI_CONST_CALL, 2);
		CallInst[1].Count = 2;
		CallInst[2].Value = SymbolMethod;
		Function->Top -= 2;
		ml_decl_t *Decl = Decls[I];
		ml_inst_t *VarInst = mlc_emit(Expr->EndLine, MLI_VAR, 1);
		VarInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
		Decl->Flags = 0;
	}
	mlc_emit(Expr->EndLine, MLI_POP, 0);
	--Function->Top;
	return 0;
}

static int ml_var_unpack_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	mlc_compile(Function, Expr->Child, 0);
	mlc_local_t *Local = Expr->Local;
	ml_inst_t *VarInst = mlc_emit(Expr->EndLine, MLI_VARX, 2);
	VarInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
	VarInst[2].Count = Expr->Count;
	return 0;
}

static int ml_let_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	mlc_compile(Function, Expr->Child, 0);
	mlc_local_t *Local = Expr->Local;
	ml_decl_t *Decl = Function->Block->Decls[Local->Index];
	ml_inst_t *LetInst;
	if (Expr->Flags == MLT_REF) {
		if (Decl->Flags & MLC_DECL_BACKFILL) {
			LetInst = mlc_emit(Expr->EndLine, MLI_REFI, 1);
		} else {
			LetInst = mlc_emit(Expr->EndLine, MLI_REF, 1);
		}
	} else {
		if (Decl->Flags & MLC_DECL_BACKFILL) {
			LetInst = mlc_emit(Expr->EndLine, MLI_LETI, 1);
		} else {
			LetInst = mlc_emit(Expr->EndLine, MLI_LET, 1);
		}
	}
	LetInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
	Decl->Flags = 0;
	return 0;
}

static int ml_let_in_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	mlc_compile(Function, Expr->Child, MLC_PUSH);
	mlc_local_t *Local = Expr->Local;
	ml_decl_t **Decls = Function->Block->Decls + Local->Index;
	for (int I = 0; I < Expr->Count; ++I, Local = Local->Next) {
		ml_inst_t *PushInst = mlc_emit(Expr->EndLine, MLI_LOCAL_PUSH, 1);
		PushInst[1].Index = Function->Top - 1;
		mlc_inc_top(Function);
		ml_inst_t *ValueInst = mlc_emit(Expr->EndLine, MLI_LOAD_PUSH, 1);
		ValueInst[1].Value = ml_cstring(Local->Ident);
		mlc_inc_top(Function);
		ml_inst_t *CallInst = mlc_emit(Expr->EndLine, MLI_CONST_CALL, 2);
		CallInst[1].Count = 2;
		CallInst[2].Value = SymbolMethod;
		Function->Top -= 2;
		ml_decl_t *Decl = Decls[I];
		ml_inst_t *LetInst;
		if (Expr->Flags == MLT_REF) {
			if (Decl->Flags & MLC_DECL_BACKFILL) {
				LetInst = mlc_emit(Expr->EndLine, MLI_REFI, 1);
			} else {
				LetInst = mlc_emit(Expr->EndLine, MLI_REF, 1);
			}
		} else {
			if (Decl->Flags & MLC_DECL_BACKFILL) {
				LetInst = mlc_emit(Expr->EndLine, MLI_LETI, 1);
			} else {
				LetInst = mlc_emit(Expr->EndLine, MLI_LET, 1);
			}
		}
		LetInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
		Decl->Flags = 0;
	}
	mlc_emit(Expr->EndLine, MLI_POP, 0);
	--Function->Top;
	return 0;
}

static int ml_let_unpack_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	mlc_compile(Function, Expr->Child, 0);
	mlc_local_t *Local = Expr->Local;
	ml_decl_t **Decls = Function->Block->Decls + Local->Index;
	ml_inst_t *LetInst = mlc_emit(Expr->EndLine, Expr->Flags == MLT_REF ? MLI_REFX : MLI_LETX, 2);
	LetInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
	LetInst[2].Count = Expr->Count;
	for (int I = 0; I < Expr->Count; ++I) Decls[I]->Flags = 0;
	return 0;
}

static int ml_def_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	ml_value_t *Value = mlc_yield_expr_call(Function, Expr->Child);
	mlc_local_t *Local = Expr->Local;
	ml_decl_t *Decl = Function->Block->Decls[Local->Index];
	if (Decl->Value) ml_uninitialized_set(Decl->Value, Value);
	Decl->Value = Value;
	return 0;
}

static int ml_def_in_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	ml_value_t *Args[2];
	Args[0] = mlc_yield_expr_call(Function, Expr->Child);
	mlc_local_t *Local = Expr->Local;
	ml_decl_t **Decls = Function->Block->Decls + Local->Index;
	for (int I = 0; I < Expr->Count; ++I, Local = Local->Next) {
		Args[1] = ml_cstring(Local->Ident);
		ml_value_t *Value = mlc_yield_call(Function->Compiler, SymbolMethod, 2, Args);
		if (ml_is_error(Value)) {
			ml_error_trace_add(Value, (ml_source_t){Function->Source, Expr->EndLine});
			Function->Compiler->Value = Value;
			longjmp(Function->Compiler->OnError, 1);
		}
		ml_decl_t *Decl = Decls[I];
		if (Decl->Value) ml_uninitialized_set(Decl->Value, Value);
		Decl->Value = Value;
	}
	return 0;
}

static int ml_def_unpack_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	ml_value_t *Packed = mlc_yield_expr_call(Function, Expr->Child);
	mlc_local_t *Local = Expr->Local;
	ml_decl_t **Decls = Function->Block->Decls + Local->Index;
	for (int I = 0; I < Expr->Count; ++I) {
		ml_value_t *Value = ml_unpack(Packed, I + 1);
		ml_decl_t *Decl = Decls[I];
		if (Decl->Value) ml_uninitialized_set(Decl->Value, Value);
		Decl->Value = Value;
	}
	return 0;
}

static int ml_with_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	int OldTop = Function->Top;
	ml_decl_t *OldDecls = Function->Decls;
	mlc_expr_t *Child = Expr->Child;
	for (mlc_local_t *Local = Expr->Local; Local; Local = Local->Next) {
		int Count = Local->Index;
		if (Count) {
			ml_decl_t *Decl = new(ml_decl_t);
			Decl->Source.Name = Function->Source;
			Decl->Source.Line = Local->Line;
			Decl->Ident = Local->Ident;
			Decl->Hash = ml_ident_hash(Local->Ident);
			Decl->Index = Function->Top;
			mlc_inc_top(Function);
			Decl->Next = Function->Decls;
			Function->Decls = Decl;
			for (int I = Count; --I > 0;) {
				Local = Local->Next;
				ml_decl_t *Decl = new(ml_decl_t);
				Decl->Source.Name = Function->Source;
				Decl->Source.Line = Local->Line;
				Decl->Ident = Local->Ident;
				Decl->Hash = ml_ident_hash(Local->Ident);
				Decl->Index = Function->Top;
				mlc_inc_top(Function);
				Decl->Next = Function->Decls;
				Function->Decls = Decl;
			}
			mlc_compile(Function, Child, 0);
			ml_inst_t *PushInst = mlc_emit(Child->EndLine, MLI_WITHX, 2);
			PushInst[1].Count = Count;
			PushInst[2].Decls = Function->Decls;
		} else {
			ml_decl_t *Decl = new(ml_decl_t);
			Decl->Source.Name = Function->Source;
			Decl->Source.Line = Local->Line;
			Decl->Ident = Local->Ident;
			Decl->Hash = ml_ident_hash(Local->Ident);
			Decl->Index = Function->Top;
			mlc_inc_top(Function);
			Decl->Next = Function->Decls;
			Function->Decls = Decl;
			mlc_compile(Function, Child, 0);
			ml_inst_t *PushInst = mlc_emit(Child->EndLine, MLI_WITH, 1);
			PushInst[1].Decls = Function->Decls;
		}
	}
	mlc_compile(Function, Child->Next, 0);
	ml_inst_t *ExitInst = mlc_emit(Expr->EndLine, MLI_EXIT, 2);
	ExitInst[1].Count = Function->Top - OldTop;
	ExitInst[2].Decls = OldDecls;
	Function->Decls = OldDecls;
	Function->Top = OldTop;
	return 0;
}

typedef struct mlc_for_expr_t mlc_for_expr_t;

struct mlc_for_expr_t {
	MLC_EXPR_FIELDS(for);
	const char *Key;
	mlc_local_t *Local;
	mlc_expr_t *Child;
	int Unpack;
};

static int ml_for_expr_compile(mlc_function_t *Function, mlc_for_expr_t *Expr, int Flags) {
	int OldTop = Function->Top;
	ml_decl_t *OldDecls = Function->Decls;
	mlc_expr_t *Child = Expr->Child;
	mlc_compile(Function, Child, 0);
	mlc_emit(Child->EndLine, MLI_FOR, 0);
	ml_inst_t *IterInst = mlc_emit(Child->EndLine, MLI_ITER, 1);
	mlc_inc_top(Function);
	if (Expr->Key) {
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source.Name = Function->Source;
		Decl->Source.Line = Expr->StartLine;
		Decl->Ident = Expr->Key;
		Decl->Hash = ml_ident_hash(Decl->Ident);
		Decl->Index = Function->Top++;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
		ml_inst_t *KeyInst = mlc_emit(Child->EndLine, MLI_KEY, 1);
		KeyInst[1].Index = -1;
		ml_inst_t *WithInst = mlc_emit(Child->EndLine, MLI_WITH, 1);
		WithInst[1].Decls = Decl;
	}
	for (mlc_local_t *Local = Expr->Local; Local; Local = Local->Next) {
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source.Name = Function->Source;
		Decl->Source.Line = Local->Line;
		Decl->Ident = Local->Ident;
		Decl->Hash = ml_ident_hash(Local->Ident);
		Decl->Index = Function->Top++;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
	}
	ml_inst_t *ValueInst = mlc_emit(Child->EndLine, MLI_VALUE, 1);
	ValueInst[1].Index = Expr->Key ? -2 : -1;
	if (Expr->Unpack) {
		ml_inst_t *WithInst = mlc_emit(Child->EndLine, MLI_WITHX, 2);
		WithInst[1].Count = Expr->Unpack;
		WithInst[2].Decls = Function->Decls;
	} else {
		ml_inst_t *WithInst = mlc_emit(Child->EndLine, MLI_WITH, 1);
		WithInst[1].Decls = Function->Decls;
	}
	mlc_loop_t Loop = {
		Function->Loop, Function->Try,
		OldDecls, OldDecls,
		NULL, NULL,
		OldTop + 1, OldTop
	};
	if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	Function->Loop = &Loop;
	mlc_compile(Function, Child->Next, 0);
	ml_inst_t *ExitInst = mlc_emit(Expr->EndLine, MLI_EXIT, 2);
	ExitInst[1].Count = Expr->Key ? 2 : 1;
	ExitInst[2].Decls = OldDecls;
	ml_inst_t *NextInst = mlc_emit(Expr->StartLine, MLI_NEXT, 1);
	NextInst[1].Inst = IterInst;
	mlc_link(Loop.Nexts, NextInst);
	IterInst[1].Inst = Function->Next;
	Function->Loop = Loop.Up;
	Function->Top = OldTop;
	Function->Decls = OldDecls;
	if (Child->Next->Next) {
		mlc_compile(Function, Child->Next->Next, 0);
	}
	mlc_link(Loop.Exits, Function->Next);
	return 0;
}

static int ml_each_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	mlc_expr_t *Child = Expr->Child;
	mlc_compile(Function, Child, 0);
	mlc_emit(Child->EndLine, MLI_FOR, 0);
	ml_inst_t *AndInst = mlc_emit(Child->EndLine, MLI_ITER, 1);
	mlc_inc_top(Function);
	ml_inst_t *ValueInst = mlc_emit(Child->EndLine, MLI_VALUE, 1);
	ValueInst[1].Index = -1;
	ml_inst_t *NextInst = mlc_emit(Expr->StartLine, MLI_NEXT, 1);
	NextInst[1].Inst = AndInst;
	AndInst[1].Inst = Function->Next;
	return 0;
}

typedef struct mlc_block_expr_t mlc_block_expr_t;
typedef struct mlc_catch_expr_t mlc_catch_expr_t;
typedef struct mlc_catch_type_t mlc_catch_type_t;

struct mlc_block_expr_t {
	MLC_EXPR_FIELDS(block);
	mlc_local_t *Vars, *Lets, *Defs;
	mlc_expr_t *Child;
	mlc_catch_expr_t *Catches;
	int NumVars, NumLets, NumDefs;
};

struct mlc_catch_expr_t {
	mlc_catch_expr_t *Next;
	const char *Ident;
	mlc_catch_type_t *Types;
	mlc_expr_t *Body;
	int Line;
};

struct mlc_catch_type_t {
	mlc_catch_type_t *Next;
	const char *Type;
};

static int ml_block_expr_compile(mlc_function_t *Function, mlc_block_expr_t *Expr, int Flags) {
	int OldTop = Function->Top;
	ml_decl_t *OldDecls = Function->Decls;
	mlc_try_t Try;
	ml_inst_t *MainTryInst = NULL;
	if (Expr->Catches) {
		MainTryInst = mlc_emit(Expr->StartLine, MLI_TRY, 1);
		Try.Up = Function->Try;
		Try.Retries = NULL;
		Try.Top = OldTop;
		Function->Try = &Try;
	}
	int Top = Function->Top;
	ml_decl_t *Last = Function->Decls, *Decls = Last;
	inthash_t DeclHashes[1] = {INTHASH_INIT};
	int NumDecls = Expr->NumVars + Expr->NumLets + Expr->NumDefs;
	mlc_block_t *OldBlock = Function->Block;
	mlc_block_t Block = {Function->Block, anew(ml_decl_t *, NumDecls), OldTop};
	Function->Block = &Block;
	for (mlc_local_t *Local = Expr->Vars; Local; Local = Local->Next) {
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source.Name = Function->Source;
		Decl->Source.Line = Local->Line;
		Decl->Ident = Local->Ident;
		Decl->Hash = ml_ident_hash(Local->Ident);
		Decl->Index = Top++;
		Block.Decls[Local->Index] = Decl;
		if (inthash_insert(DeclHashes, (uintptr_t)Decl->Hash, Decl)) {
			for (ml_decl_t *Prev = Decls; Prev != Last; Prev = Prev->Next) {
				if (!strcmp(Prev->Ident, Decl->Ident)) {
					ml_expr_error(Expr, ml_error("NameError", "Identifier %s redefined in line %d, previously declared on line %d", Decl->Ident, Decl->Source.Line, Prev->Source.Line));
				}
			}
		}
		Decl->Next = Decls;
		Decls = Decl;
	}
	for (mlc_local_t *Local = Expr->Lets; Local; Local = Local->Next) {
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source.Name = Function->Source;
		Decl->Source.Line = Local->Line;
		Decl->Ident = Local->Ident;
		Decl->Hash = ml_ident_hash(Local->Ident);
		Decl->Index = Top++;
		Decl->Flags = MLC_DECL_FORWARD;
		Block.Decls[Local->Index] = Decl;
		if (inthash_insert(DeclHashes, (uintptr_t)Decl->Hash, Decl)) {
			for (ml_decl_t *Prev = Decls; Prev != Last; Prev = Prev->Next) {
				if (!strcmp(Prev->Ident, Decl->Ident)) {
					ml_expr_error(Expr, ml_error("NameError", "Identifier %s redefined in line %d, previously declared on line %d", Decl->Ident, Decl->Source.Line, Prev->Source.Line));
				}
			}
		}
		Decl->Next = Decls;
		Decls = Decl;
	}
	for (mlc_local_t *Local = Expr->Defs; Local; Local = Local->Next) {
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source.Name = Function->Source;
		Decl->Source.Line = Local->Line;
		Decl->Ident = Local->Ident;
		Decl->Hash = ml_ident_hash(Local->Ident);
		Decl->Flags = MLC_DECL_CONSTANT;
		Block.Decls[Local->Index] = Decl;
		if (inthash_insert(DeclHashes, (uintptr_t)Decl->Hash, Decl)) {
			for (ml_decl_t *Prev = Decls; Prev != Last; Prev = Prev->Next) {
				if (!strcmp(Prev->Ident, Decl->Ident)) {
					ml_expr_error(Expr, ml_error("NameError", "Identifier %s redefined in line %d, previously declared on line %d", Decl->Ident, Decl->Source.Line, Prev->Source.Line));
				}
			}
		}
		Decl->Next = Decls;
		Decls = Decl;
	}
	if (Top >= Function->Size) Function->Size = Top + 1;
	Function->Top = Top;
	Function->Decls = Decls;
	if (Expr->NumVars + Expr->NumLets) {
		ml_inst_t *EnterInst = mlc_emit(Expr->StartLine, MLI_ENTER, 3);
		EnterInst[1].Count = Expr->NumVars;
		EnterInst[2].Count = Expr->NumLets;
		EnterInst[3].Decls = Function->Decls;
	}
	mlc_expr_t *Child = Expr->Child;
	int Result = 0;
	if (Child) {
		do Result = mlc_compile(Function, Child, 0); while ((Child = Child->Next));
	} else {
		mlc_emit(Expr->StartLine, MLI_NIL, 0);
	}
	if (Expr->NumVars + Expr->NumLets) {
		ml_inst_t *ExitInst = mlc_emit(Expr->EndLine, MLI_EXIT, 2);
		ExitInst[1].Count = Expr->NumVars + Expr->NumLets;
		ExitInst[2].Decls = OldDecls;
	}
	Function->Decls = OldDecls;
	Function->Block = OldBlock;
	Function->Top = OldTop;
	ml_inst_t *Exits = NULL;
	if (Expr->Catches) {
		Result = 0;
		Function->Try = Function->Try->Up;
		ml_inst_t *TryInst = mlc_emit(Expr->EndLine, MLI_TRY, 1);
		if (Function->Try) {
			TryInst[1].Inst = Function->Try->Retries;
			Function->Try->Retries = TryInst + 1;
		} else {
			TryInst[1].Inst = Function->Returns;
			Function->Returns = TryInst + 1;
		}
		ml_inst_t *GotoInst = mlc_emit(Expr->EndLine, MLI_GOTO, 1);
		GotoInst[1].Inst = Exits;
		Exits = GotoInst + 1;
		TryInst = mlc_emit(Expr->Catches->Line, MLI_TRY, 1);
		mlc_link(Try.Retries, TryInst);
		if (Function->Try) {
			TryInst[1].Inst = Function->Try->Retries;
			Function->Try->Retries = TryInst + 1;
		} else {
			TryInst[1].Inst = Function->Returns;
			Function->Returns = TryInst + 1;
		}
		MainTryInst[1].Inst = TryInst;
		mlc_catch_expr_t *CatchExpr = Expr->Catches;
		ml_inst_t *CatchTypeInst = NULL;
		do {
			if (CatchTypeInst) {
				CatchTypeInst[1].Inst = Function->Next;
				CatchTypeInst = NULL;
			}
			if (CatchExpr->Types) {
				int NumTypes = 0;
				for (mlc_catch_type_t *Type = CatchExpr->Types; Type; Type = Type->Next) ++NumTypes;
				CatchTypeInst = mlc_emit(CatchExpr->Line, MLI_CATCH_TYPE, 2);
				const char **Ptrs = CatchTypeInst[2].Ptrs = anew(const char *, NumTypes + 1);
				for (mlc_catch_type_t *Type = CatchExpr->Types; Type; Type = Type->Next) *Ptrs++ = Type->Type;
			}
			ml_decl_t *Decl = new(ml_decl_t);
			Decl->Source.Name = Function->Source;
			Decl->Source.Line = CatchExpr->Line;
			Decl->Ident = CatchExpr->Ident;
			Decl->Hash = ml_ident_hash(CatchExpr->Ident);
			Decl->Index = Function->Top;
			Decl->Next = Function->Decls;
			Function->Decls = Decl;
			mlc_inc_top(Function);
			ml_inst_t *CatchInst = mlc_emit(CatchExpr->Line, MLI_CATCH, 2);
			CatchInst[1].Index = OldTop;
			CatchInst[2].Decls = Function->Decls;
			mlc_compile(Function, CatchExpr->Body, 0);
			ml_inst_t *ExitInst = mlc_emit(Expr->EndLine, MLI_EXIT, 2);
			ExitInst[1].Count = 1;
			ExitInst[2].Decls = OldDecls;
			Function->Decls = OldDecls;
			Function->Top = OldTop;
			ml_inst_t *GotoInst = mlc_emit(Expr->EndLine, MLI_GOTO, 1);
			GotoInst[1].Inst = Exits;
			Exits = GotoInst + 1;
		} while ((CatchExpr = CatchExpr->Next));
		if (CatchTypeInst) {
			CatchTypeInst[1].Inst = Function->Next;
			mlc_emit(Expr->EndLine, MLI_RETRY, 0);
		}
	}
	mlc_link(Exits, Function->Next);
	return Result;
}

static int ml_assign_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	int Result = mlc_compile(Function, Expr->Child, MLC_LOCAL | MLC_PUSH);
	int OldSelf = Function->Self;
	if (Result & MLC_LOCAL) {
		Function->Self = Function->Local;
	} else {
		Function->Self = Function->Top - 1;
	}
	mlc_compile(Function, Expr->Child->Next, 0);
	if (Result & MLC_LOCAL) {
		ml_inst_t *AssignInst = mlc_emit(Expr->Child->EndLine, MLI_ASSIGN_LOCAL, 1);
		AssignInst[1].Index = Function->Self;
	} else {
		mlc_emit(Expr->Child->EndLine, MLI_ASSIGN, 0);
		--Function->Top;
	}
	Function->Self = OldSelf;
	return 0;
}

static int ml_old_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr, int Flags) {
	if (Flags & MLC_PUSH) {
		ml_inst_t *OldInst = mlc_emit(Expr->StartLine, MLI_LOCAL_PUSH, 1);
		OldInst[1].Index = Function->Self;
		return MLC_PUSH;
	}
	ml_inst_t *OldInst = mlc_emit(Expr->StartLine, MLI_LOCAL, 1);
	OldInst[1].Index = Function->Self;
	return 0;
}

static int ml_tuple_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	int Count = 0;
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) {
		mlc_compile(Function, Child, MLC_PUSH);
		++Count;
	}
	ml_inst_t *TupleInst = mlc_emit(Expr->StartLine, MLI_TUPLE_NEW, 1);
	TupleInst[1].Count = Count;
	Function->Top -= Count;
	return 0;
}

static int ml_list_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	mlc_emit(Expr->StartLine, MLI_LIST_NEW, 0);
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) {
		mlc_compile(Function, Child, 0);
		mlc_emit(Child->EndLine, MLI_LIST_APPEND, 0);
	}
	if (Flags & MLC_PUSH) return MLC_PUSH;
	mlc_emit(Expr->EndLine, MLI_POP, 0);
	--Function->Top;
	return 0;
}

static int ml_map_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	mlc_emit(Expr->StartLine, MLI_MAP_NEW, 0);
	if (Function->Top + 2 >= Function->Size) Function->Size = Function->Top + 3;
	++Function->Top;
	for (mlc_expr_t *Key = Expr->Child; Key; Key = Key->Next->Next) {
		mlc_compile(Function, Key, MLC_PUSH);
		mlc_compile(Function, Key->Next, 0);
		mlc_emit(Key->Next->EndLine, MLI_MAP_INSERT, 0);
		--Function->Top;
	}
	if (Flags & MLC_PUSH) return MLC_PUSH;
	mlc_emit(Expr->EndLine, MLI_POP, 0);
	--Function->Top;
	return 0;
}

typedef struct {
	ml_macro_t Base;
	ml_decl_t *Params;
	mlc_expr_t *Expr;
} ml_template_macro_t;

static int ml_template_macro_apply(mlc_function_t *Function, ml_template_macro_t *Macro, mlc_expr_t *Expr, mlc_expr_t *Child, int Flags) {
	ml_define_t *OldDefines = Function->Defines;
	for (ml_decl_t *Param = Macro->Params; Param; Param = Param->Next) {
		if (!Child) ml_expr_error(Expr, ml_error("MacroError", "Insufficient arguments to macro"));
		ml_define_t *Define = new(ml_define_t);
		Define->Ident = Param->Ident;
		Define->Hash = Param->Hash;
		Define->Expr = Child;
		Define->Next = Function->Defines;
		Function->Defines = Define;
		Child = Child->Next;
	}
	int Result = mlc_compile(Function, Macro->Expr, Flags);
	Function->Defines = OldDefines;
	return Result;
}

typedef struct ml_scope_macro_t ml_scope_macro_t;

struct ml_scope_macro_t {
	ml_macro_t Base;
	stringmap_t Names[1];
};

static int ml_scope_macro_fn(const char *Name, ml_value_t *Value, mlc_function_t *Function) {
	ml_decl_t *Decl = new(ml_decl_t);
	Decl->Ident = Name;
	Decl->Hash = ml_ident_hash(Name);
	Decl->Value = Value;
	Decl->Next = Function->Decls;
	Function->Decls = Decl;
	return 0;
}

static int ml_scope_macro_apply(mlc_function_t *Function, ml_scope_macro_t *Macro, mlc_expr_t *Expr, mlc_expr_t *Child, int Flags) {
	if (!Child) ml_expr_error(Expr, ml_error("MacroError", "Insufficient arguments to macro"));
	ml_decl_t *OldDecls = Function->Decls;
	stringmap_foreach(Macro->Names, Function, (void *)ml_scope_macro_fn);
	int Result = mlc_compile(Function, Child, Flags);
	Function->Decls = OldDecls;
	return Result;
}

ml_scope_macro_t *ml_scope_macro_new() {
	ml_scope_macro_t *Macro = new(ml_scope_macro_t);
	Macro->Base.Type = MLMacroT;
	Macro->Base.apply = (void *)ml_scope_macro_apply;
	return Macro;
}

void ml_scope_macro_define(ml_scope_macro_t *Macro, const char *Name, ml_value_t *Value) {
	stringmap_insert(Macro->Names, Name, Value);
}

static int ml_stringify_macro_apply(mlc_function_t *Function, ml_macro_t *Macro, mlc_expr_t *Expr, mlc_expr_t *Child, int Flags) {
	if (!Child) ml_expr_error(Expr, ml_error("MacroError", "Insufficient arguments to macro"));
	return 0;
}

static int ml_call_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next) {
		if (Child->compile == (void *)ml_blank_expr_compile) {
			mlc_compile(Function, Expr->Child, 0);
			ml_inst_t *PartialInst = mlc_emit(Expr->StartLine, MLI_PARTIAL_NEW, 1);
			int NumArgs = 0;
			++Function->Top;
			for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next, ++NumArgs) {
				if (Child->compile != (void *)ml_blank_expr_compile) {
					mlc_compile(Function, Child, 0);
					ml_inst_t *SetInst = mlc_emit(Expr->EndLine, MLI_PARTIAL_SET, 1);
					SetInst[1].Index = NumArgs;
				}
			}
			PartialInst[1].Count = NumArgs;
			mlc_emit(Expr->EndLine, MLI_POP, 0);
			--Function->Top;
			return 0;
		}
	}
	int OldTop = Function->Top;
	mlc_expr_t *Child = Expr->Child;
	int Result = mlc_compile(Function, Child, MLC_CONSTANT | MLC_PUSH);
	ml_value_t *Value = NULL;
	if (Result & MLC_CONSTANT) {
		Value = Function->Constant;
		if (ml_typeof(Value) == MLMacroT) {
			ml_macro_t *Macro = (ml_macro_t *)Value;
			return Macro->apply(Function, Macro, (mlc_expr_t *)Expr, Child->Next, Flags);
		}
	}
	int NumArgs = 0;
	Child = Child->Next;
	while (Child) {
		++NumArgs;
		mlc_compile(Function, Child, MLC_PUSH);
		Child = Child->Next;
	}
	if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	if (Value) {
		ml_inst_t *CallInst = mlc_emit(Expr->EndLine, MLI_CONST_CALL, 2);
		CallInst[1].Count = NumArgs;
		CallInst[2].Value = Value;
		if (ml_typeof(Value) == MLUninitializedT) ml_uninitialized_use(Value, &CallInst[2].Value);
	} else {
		ml_inst_t *CallInst = mlc_emit(Expr->EndLine, MLI_CALL, 1);
		CallInst[1].Count = NumArgs;
	}
	Function->Top = OldTop;
	return 0;
}

typedef struct mlc_parent_value_expr_t mlc_parent_value_expr_t;

struct mlc_parent_value_expr_t {
	MLC_EXPR_FIELDS(parent_value);
	mlc_expr_t *Child;
	ml_value_t *Value;
};

static int ml_const_call_expr_compile(mlc_function_t *Function, mlc_parent_value_expr_t *Expr, int Flags) {
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) {
		if (Child->compile == (void *)ml_blank_expr_compile) {
			ml_inst_t *LoadInst = mlc_emit(Expr->StartLine, MLI_LOAD, 1);
			LoadInst[1].Value = Expr->Value;
			ml_inst_t *PartialInst = mlc_emit(Expr->StartLine, MLI_PARTIAL_NEW, 1);
			int NumArgs = 0;
			++Function->Top;
			for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next, ++NumArgs) {
				if (Child->compile != (void *)ml_blank_expr_compile) {
					mlc_compile(Function, Child, 0);
					ml_inst_t *SetInst = mlc_emit(Child->EndLine, MLI_PARTIAL_SET, 1);
					SetInst[1].Count = NumArgs;
				}
			}
			PartialInst[1].Count = NumArgs;
			mlc_emit(Expr->StartLine, MLI_POP, 0);
			--Function->Top;
			return 0;
		}
	}
	int OldTop = Function->Top;
	int NumArgs = 0;
	mlc_expr_t *Child = Expr->Child;
	while (Child) {
		++NumArgs;
		mlc_compile(Function, Child, MLC_PUSH);
		Child = Child->Next;
	}
	if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	ml_inst_t *CallInst = mlc_emit(Expr->EndLine, MLI_CONST_CALL, 2);
	CallInst[1].Count = NumArgs;
	CallInst[2].Value = Expr->Value;
	Function->Top = OldTop;
	return 0;
}

static int ml_resolve_expr_compile(mlc_function_t *Function, mlc_parent_value_expr_t *Expr, int Flags) {
	int Result = mlc_compile(Function, Expr->Child, MLC_CONSTANT);
	if (Result & MLC_CONSTANT) {
		ml_value_t *Args[2] = {Function->Constant, Expr->Value};
		ml_value_t *Value = mlc_yield_call(Function->Compiler, SymbolMethod, 2, Args);
		if (ml_is_error(Value)) {
			ml_error_trace_add(Value, (ml_source_t){Function->Source, Expr->EndLine});
			Function->Compiler->Value = Value;
			longjmp(Function->Compiler->OnError, 1);
		}
		if (Flags & MLC_CONSTANT) {
			Function->Constant = Value;
			return MLC_CONSTANT & MLC_PUSH;
		} else if (Flags & MLC_PUSH) {
			ml_inst_t *ValueInst = mlc_emit(Expr->EndLine, MLI_LOAD_PUSH, 1);
			ValueInst[1].Value = Value;
			mlc_inc_top(Function);
			return MLC_PUSH;
		} else {
			ml_inst_t *ValueInst = mlc_emit(Expr->EndLine, MLI_LOAD, 1);
			ValueInst[1].Value = Value;
			return 0;
		}
	} else {
		ml_inst_t *ResolveInst = mlc_emit(Expr->EndLine, MLI_RESOLVE, 1);
		ResolveInst[1].Value = Expr->Value;
	}
	return 0;
}


typedef struct mlc_string_expr_t mlc_string_expr_t;
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
	int Length, Line;
};

static int ml_string_expr_compile(mlc_function_t *Function, mlc_string_expr_t *Expr, int Flags) {
	mlc_emit(Expr->StartLine, MLI_STRING_NEW, 0);
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	for (mlc_string_part_t *Part = Expr->Parts; Part; Part = Part->Next) {
		if (Part->Length) {
			ml_inst_t *AddInst = mlc_emit(Part->Line, MLI_STRING_ADDS, 2);
			AddInst[1].Count = Part->Length;
			AddInst[2].Ptr = Part->Chars;
		} else {
			int OldTop = Function->Top;
			int NumArgs = 0;
			for (mlc_expr_t *Child = Part->Child; Child; Child = Child->Next) {
				++NumArgs;
				mlc_compile(Function, Child, MLC_PUSH);
			}
			if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
			ml_inst_t *AddInst = mlc_emit(Part->Line, MLI_STRING_ADD, 1);
			AddInst[1].Count = NumArgs;
			Function->Top = OldTop;
		}
	}
	mlc_emit(Expr->StartLine, MLI_STRING_END, 0);
	--Function->Top;
	return 0;
}

typedef struct mlc_fun_expr_t mlc_fun_expr_t;
typedef struct mlc_param_t mlc_param_t;

struct mlc_param_t {
	mlc_param_t *Next;
	const char *Ident;
	mlc_expr_t *Type;
	int Line, Flags;
};

struct mlc_fun_expr_t {
	MLC_EXPR_FIELDS(fun);
	mlc_param_t *Params;
	mlc_expr_t *Body;
	mlc_expr_t *ReturnType;
	const char *Source;
};

#define ML_PARAM_EXTRA 1
#define ML_PARAM_NAMED 2
#define ML_PARAM_BYREF 3

static int ml_fun_expr_compile(mlc_function_t *Function, mlc_fun_expr_t *Expr, int Flags) {
	// closure <entry> <frame_size> <num_params> <num_upvalues> <upvalue_1> ...
	mlc_function_t SubFunction[1];
	memset(SubFunction, 0, sizeof(SubFunction));
	SubFunction->Compiler = Function->Compiler;
	SubFunction->Up = Function;
	SubFunction->Source = Expr->Source;
	ml_closure_info_t *Info = new(ml_closure_info_t);
	Info->Source = Expr->Source;
	Info->StartLine = Expr->StartLine;
	Info->EndLine = Expr->EndLine;
	int NumParams = 0, HasParamTypes = 0;
	ml_decl_t **DeclSlot = &SubFunction->Decls;
	for (mlc_param_t *Param = Expr->Params; Param; Param = Param->Next) {
		ml_decl_t *Decl = DeclSlot[0] = new(ml_decl_t);
		Decl->Source.Name = Function->Source;
		Decl->Source.Line = Param->Line;
		Decl->Ident = Param->Ident;
		Decl->Hash = ml_ident_hash(Param->Ident);
		Decl->Index = NumParams++;
		switch (Param->Flags) {
		case ML_PARAM_EXTRA:
			Info->ExtraArgs = 1;
			break;
		case ML_PARAM_NAMED:
			Info->NamedArgs = 1;
			break;
		case ML_PARAM_BYREF:
			Decl->Flags |= MLC_DECL_BYREF;
		default:
			stringmap_insert(Info->Params, Param->Ident, (void *)(intptr_t)NumParams);
			break;
		}
		if (Param->Type) HasParamTypes = 1;
		DeclSlot = &Decl->Next;
	}
	SubFunction->Top = SubFunction->Size = NumParams;
	SubFunction->Next = anew(ml_inst_t, 128);
	SubFunction->Space = 126;
	SubFunction->Returns = NULL;
	Info->Decls = SubFunction->Decls;
	Info->Entry = SubFunction->Next;
	mlc_compile(SubFunction, Expr->Body, 0);
	Info->Return = ml_inst_alloc(SubFunction, Expr->EndLine, MLI_RETURN, 0);
	mlc_link(SubFunction->Returns, Info->Return);
	Info->Halt = SubFunction->Next;
	ml_decl_t **UpValueSlot = &SubFunction->Decls;
	while (UpValueSlot[0]) UpValueSlot = &UpValueSlot[0]->Next;
	int Index = 0;
	for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next, ++Index) {
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source.Name = Function->Source;
		Decl->Source.Line = Expr->StartLine;
		Decl->Ident = UpValue->Decl->Ident;
		Decl->Hash = UpValue->Decl->Hash;
		Decl->Value = UpValue->Decl->Value;
		Decl->Index = ~Index;
		UpValueSlot[0] = Decl;
		UpValueSlot = &Decl->Next;
	}
	Info->FrameSize = SubFunction->Size;
	Info->NumParams = NumParams;
	ml_closure_info_finish(Info);
	if (SubFunction->UpValues || HasParamTypes || Expr->ReturnType) {
		int NumUpValues = 0;
		for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next) ++NumUpValues;
		ml_inst_t *ClosureInst;
#ifdef ML_GENERICS
		if (Expr->ReturnType) {
			mlc_compile(Function, Expr->ReturnType, 0);
			ClosureInst = mlc_emit(Expr->StartLine, MLI_CLOSURE_TYPED, NumUpValues + 1);
		} else {
#endif
			 ClosureInst = mlc_emit(Expr->StartLine, MLI_CLOSURE, NumUpValues + 1);
#ifdef ML_GENERICS
		}
#endif
		Info->NumUpValues = NumUpValues;
		ClosureInst[1].ClosureInfo = Info;
		int Index = 1;
		for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next) ClosureInst[++Index].Index = UpValue->Index;
		if (HasParamTypes) {
			mlc_emit(Expr->StartLine, MLI_PUSH, 0);
			mlc_inc_top(Function);
			int Index = 0;
			for (mlc_param_t *Param = Expr->Params; Param; Param = Param->Next, ++Index) {
				if (Param->Type) {
					mlc_compile(Function, Param->Type, 0);
					ml_inst_t *TypeInst = mlc_emit(Param->Line, MLI_PARAM_TYPE, 1);
					TypeInst[1].Index = Index;
				}
			}
			mlc_emit(Expr->StartLine, MLI_POP, 0);
			--Function->Top;
		}
	} else {
		Info->NumUpValues = 0;
		ml_closure_t *Closure = xnew(ml_closure_t, 0, ml_value_t *);
		Closure->Type = MLClosureT;
		Closure->Info = Info;
		ml_inst_t *LoadInst = mlc_emit(Expr->StartLine, MLI_LOAD, 1);
		LoadInst[1].Value = (ml_value_t *)Closure;
	}
	return 0;
}

typedef struct mlc_ident_expr_t mlc_ident_expr_t;

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

static int ml_ident_expr_finish(mlc_function_t *Function, mlc_ident_expr_t *Expr, ml_value_t *Value, int Flags) {
	if (Flags & MLC_CONSTANT) {
		Function->Constant = Value;
		return MLC_CONSTANT | MLC_PUSH;
	}
	ml_inst_t *ValueInst = mlc_emit(Expr->StartLine, MLI_LOAD, 1);
	if (ml_typeof(Value) == MLUninitializedT) {
		ml_uninitialized_use(Value, &ValueInst[1].Value);
	}
	ValueInst[1].Value = Value;
	if (Flags & MLC_PUSH) {
		ValueInst->Opcode = MLI_LOAD_PUSH;
		mlc_inc_top(Function);
		return MLC_PUSH;
	}
	return 0;
}

static int ml_ident_expr_compile(mlc_function_t *Function, mlc_ident_expr_t *Expr, int Flags) {
	long Hash = ml_ident_hash(Expr->Ident);
	//printf("#<%s> -> %ld\n", Expr->Ident, Hash);
	for (mlc_function_t *UpFunction = Function; UpFunction; UpFunction = UpFunction->Up) {
		for (ml_decl_t *Decl = UpFunction->Decls; Decl; Decl = Decl->Next) {
			if (Hash == Decl->Hash) {
				//printf("\tTesting <%s>\n", Decl->Ident);
				if (!strcmp(Decl->Ident, Expr->Ident)) {
					if (Decl->Flags == MLC_DECL_CONSTANT) {
						if (!Decl->Value) Decl->Value = ml_uninitialized(Decl->Ident);
						return ml_ident_expr_finish(Function, Expr, Decl->Value, Flags);
					} else {
						int Index = ml_upvalue_find(Function, Decl, UpFunction);
						if (Decl->Flags & MLC_DECL_FORWARD) Decl->Flags |= MLC_DECL_BACKFILL;
						if ((Index >= 0) && (Decl->Flags & MLC_DECL_FORWARD)) {
							ml_inst_t *LocalInst = mlc_emit(Expr->StartLine, MLI_LOCALX, 2);
							LocalInst[1].Index = Index;
							LocalInst[2].Ptr = Decl->Ident;
						} else if (Index >= 0) {
							if (Flags & MLC_LOCAL) {
								Function->Local = Index;
								return MLC_PUSH | MLC_LOCAL;
							} else if (Flags & MLC_PUSH) {
								ml_inst_t *LocalInst = mlc_emit(Expr->StartLine, MLI_LOCAL_PUSH, 1);
								LocalInst[1].Index = Index;
								mlc_inc_top(Function);
								return MLC_PUSH;
							} else {
								ml_inst_t *LocalInst = mlc_emit(Expr->StartLine, MLI_LOCAL, 1);
								LocalInst[1].Index = Index;
							}
						} else {
							ml_inst_t *LocalInst = mlc_emit(Expr->StartLine, MLI_UPVALUE, 1);
							LocalInst[1].Index = ~Index;
						}
						return 0;
					}
				}
			}
		}
	}
	ml_value_t *Value = (ml_value_t *)stringmap_search(Function->Compiler->Vars, Expr->Ident);
	if (!Value) Value = Function->Compiler->GlobalGet(Function->Compiler->Globals, Expr->Ident);
	if (!Value) {
		ml_expr_error(Expr, ml_error("CompilerError", "identifier %s not declared", Expr->Ident));
	}
	if (ml_is_error(Value)) ml_expr_error(Expr, Value);
	return ml_ident_expr_finish(Function, Expr, Value, Flags);
}

static int ml_define_expr_compile(mlc_function_t *Function, mlc_ident_expr_t *Expr, int Flags) {
	long Hash = ml_ident_hash(Expr->Ident);
	for (mlc_function_t *UpFunction = Function; UpFunction; UpFunction = UpFunction->Up) {
		for (ml_define_t *Define = UpFunction->Defines; Define; Define = Define->Next) {
			if (Hash == Define->Hash) {
				//printf("\tTesting <%s>\n", Decl->Ident);
				if (!strcmp(Define->Ident, Expr->Ident)) {
					return mlc_compile(Function, Define->Expr, Flags);
				}
			}
		}
	}
	ml_expr_error(Expr, ml_error("CompilerError", "identifier %s not defined", Expr->Ident));
}

static int ml_inline_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	ml_value_t *Value = mlc_yield_expr_call(Function, Expr->Child);
	if (Flags & MLC_CONSTANT) {
		Function->Constant = Value;
		return MLC_CONSTANT & MLC_PUSH;
	} else if (Flags & MLC_PUSH) {
		ml_inst_t *ValueInst = mlc_emit(Expr->EndLine, MLI_LOAD_PUSH, 1);
		ValueInst[1].Value = Value;
		mlc_inc_top(Function);
		return MLC_PUSH;
	} else {
		ml_inst_t *ValueInst = mlc_emit(Expr->EndLine, MLI_LOAD, 1);
		ValueInst[1].Value = Value;
		return 0;
	}
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
	"macro", // MLT_MACRO,
	"ret", // MLT_RET,
	"susp", // MLT_SUSP,
	"debug", // MLT_DEBUG,
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
	"ref", // MLT_REF,
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
	"<expand>", // MLT_EXPAND,
	"<operator>", // MLT_OPERATOR
	"<method>" // MLT_METHOD
};

static const char *ml_compiler_no_input(void *Data) {
	return NULL;
}

static void ml_compiler_call(ml_state_t *Caller, ml_compiler_t *Compiler, int Count, ml_value_t **Args) {
	ML_RETURN(MLNil);
}

static const char *ml_function_read(ml_value_t *Function) {
	ml_value_t *Result = ml_simple_call(Function, 0, NULL);
	if (!ml_is(Result, MLStringT)) return NULL;
	return ml_string_value(Result);
}

static ml_value_t *ml_function_global_get(ml_value_t *Function, const char *Name) {
	ml_value_t *Value = ml_simple_inline(Function, 1, ml_cstring(Name));
	return (Value != MLNotFound) ? Value : NULL;
}

static ml_value_t *ml_map_global_get(ml_value_t *Map, const char *Name) {
	return ml_map_search0(Map, ml_cstring(Name));
}

ML_FUNCTION(MLCompiler) {
//@compiler
//<Global:function|map
//<?Read:function
//>compiler
	ML_CHECK_ARG_COUNT(1);
	ml_getter_t GlobalGet = (ml_getter_t)ml_function_global_get;
	if (ml_is(Args[0], MLMapT)) GlobalGet = (ml_getter_t)ml_map_global_get;
	void *Input = NULL;
	ml_reader_t Reader = ml_compiler_no_input;
	if (Count > 1) {
		Input = Args[1];
		Reader = (ml_reader_t)ml_function_read;
	}
	return (ml_value_t *)ml_compiler(GlobalGet, Args[0], Reader, Input);
}

ML_TYPE(MLCompilerT, (MLStateT), "compiler",
	.call = (void *)ml_compiler_call,
	.Constructor = (ml_value_t *)MLCompiler
);

static void *MainThread;
static struct GC_stack_base MainStackBase;

static void ml_compiler_set_stackbottom(mco_coro *Coroutine) {
	if (Coroutine) {
		struct GC_stack_base StackBase;
		StackBase.mem_base = Coroutine->stack_base + Coroutine->stack_size;
		GC_set_stackbottom(MainThread, &StackBase);
	} else {
		GC_set_stackbottom(MainThread, &MainStackBase);
	}
}

static void ml_compiler_run(ml_compiler_t *Compiler, ml_value_t *Value) {
	Compiler->Value = Value;
	mco_resume(Compiler->Coroutine);
	switch (Compiler->State) {
	case MLCS_RETURN: {
		ml_state_t *Caller = Compiler->Base.Caller;
		ML_RETURN(Compiler->Value);
	}
	case MLCS_CALL: {
		return ml_call((ml_state_t *)Compiler, Compiler->Value, Compiler->Count, Compiler->Args);
	}
	default: {
		ml_state_t *Caller = Compiler->Base.Caller;
		ML_RETURN(ml_error("CompilerError", "Invalid compiler state"));
	}
	}
}

static mlc_expr_t *ml_accept_block(ml_compiler_t *Compiler);
static void ml_accept_eoi(ml_compiler_t *Compiler);
static ml_value_t *ml_accept_command(ml_compiler_t *Compiler);

static void ml_compiler_start(mco_coro *Coroutine) {
	GC_call_with_alloc_lock((void *)ml_compiler_set_stackbottom, Coroutine);
	ml_compiler_t *Compiler = (ml_compiler_t *)Coroutine->user_data;
	Coroutine->user_data = NULL;
	GC_call_with_alloc_lock((void *)ml_compiler_set_stackbottom, NULL);
	mco_yield(Coroutine);
	for (;;) {
		GC_call_with_alloc_lock((void *)ml_compiler_set_stackbottom, Coroutine);
		if (!setjmp(Compiler->OnError)) {
			switch (Compiler->State) {
			case MLCS_FUNCTION: {
				mlc_expr_t *Block = ml_accept_block(Compiler);
				ml_accept_eoi(Compiler);
				Compiler->Value = ml_compile(Compiler, Block, Compiler->Params);
				break;
			}
			case MLCS_COMMAND: {
				Compiler->Value = ml_accept_command(Compiler);
				break;
			}
			default: {
				Compiler->Value = ml_error("CompilerError", "Invalid compiler state");
				break;
			}
			}
		}
		Compiler->State = MLCS_RETURN;
		GC_call_with_alloc_lock((void *)ml_compiler_set_stackbottom, NULL);
		mco_yield(Compiler->Coroutine);
	}
}

static void ml_compiler_finalize(ml_compiler_t *Compiler, void *Data) {
	mco_destroy(Compiler->Coroutine);
}

ml_compiler_t *ml_compiler(ml_getter_t GlobalGet, void *Globals, ml_reader_t Read, void *Data) {
	ml_compiler_t *Compiler = new(ml_compiler_t);
	Compiler->Base.Type = MLCompilerT;
	Compiler->Base.run = (ml_state_fn)ml_compiler_run;
	Compiler->GlobalGet = GlobalGet;
	Compiler->Globals = Globals;
	Compiler->Token = MLT_NONE;
	Compiler->Next = "";
	Compiler->Source.Name = "";
	Compiler->Source.Line = 0;
	Compiler->Line = 0;
	Compiler->Data = Data;
	Compiler->Read = Read ?: ml_compiler_no_input;
	mco_desc Desc = mco_desc_init(ml_compiler_start, 0);
	Desc.user_data = Compiler;
	Desc.free_cb = (void *)GC_free;
	Desc.malloc_cb = (void *)GC_malloc;
	mco_create(&Compiler->Coroutine, &Desc);
	GC_register_finalizer_no_order(Compiler, (GC_finalization_proc)ml_compiler_finalize, NULL, NULL, NULL);
	mco_resume(Compiler->Coroutine);
	return Compiler;
}

void ml_compiler_define(ml_compiler_t *Compiler, const char *Name, ml_value_t *Value) {
	stringmap_insert(Compiler->Vars, Name, Value);
}

ml_value_t *ml_compiler_lookup(ml_compiler_t *Compiler, const char *Name) {
	ml_value_t *Value = (ml_value_t *)stringmap_search(Compiler->Vars, Name);
	if (!Value) Value = Compiler->GlobalGet(Compiler->Globals, Name);
	return Value;
}

const char *ml_compiler_name(ml_compiler_t *Compiler) {
	return Compiler->Source.Name;
}

ml_source_t ml_compiler_source(ml_compiler_t *Compiler, ml_source_t Source) {
	ml_source_t OldSource = Compiler->Source;
	Compiler->Source = Source;
	Compiler->Line = Source.Line;
	return OldSource;
}

void ml_compiler_reset(ml_compiler_t *Compiler) {
	Compiler->Token = MLT_NONE;
	Compiler->Next = "";
}

void ml_compiler_input(ml_compiler_t *Compiler, const char *Text) {
	Compiler->Next = Text;
	++Compiler->Line;
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
	ml_error_trace_add(Value, Compiler->Source);
	Compiler->Value = Value;
	longjmp(Compiler->OnError, 1);
}

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
static mlc_expr_t *ml_accept_term(ml_compiler_t *Compiler);
static mlc_expr_t *ml_accept_expression(ml_compiler_t *Compiler, ml_expr_level_t Level);
static void ml_accept_arguments(ml_compiler_t *Compiler, ml_token_t EndToken, mlc_expr_t **ArgsSlot);

static ml_token_t ml_accept_string(ml_compiler_t *Compiler) {
	mlc_string_part_t *Parts = NULL, **Slot = &Parts;
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *End = Compiler->Next;
	for (;;) {
		char C = *End++;
		if (!C) {
			Compiler->Next = Compiler->Read(Compiler->Data);
			++Compiler->Line;
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
				Part->Line = Compiler->Source.Line;
				Slot[0] = Part;
				Slot = &Part->Next;
			}
			Compiler->Next = End;
			mlc_string_part_t *Part = new(mlc_string_part_t);
			ml_accept_arguments(Compiler, MLT_RIGHT_BRACE, &Part->Child);
			Part->Line = Compiler->Source.Line;
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
			Part->Line = Compiler->Source.Line;
			Slot[0] = Part;
		}
		ML_EXPR(Expr, string, string);
		Expr->Parts = Parts;
		Compiler->Expr = ML_EXPR_END(Expr);
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

void ml_string_fn_register(const char *Prefix, string_fn_t Fn) {
	stringmap_insert(StringFns, Prefix, Fn);
}

static ml_token_t ml_scan(ml_compiler_t *Compiler) {
	for (;;) {
		//GC_gcollect();
		if (!Compiler->Next || !Compiler->Next[0]) {
			Compiler->Next = Compiler->Read(Compiler->Data);
			if (Compiler->Next) continue;
			Compiler->Token = MLT_EOI;
			return Compiler->Token;
		}
		char Char = Compiler->Next[0];
		if (Char == '\n') {
			++Compiler->Next;
			++Compiler->Line;
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
					Compiler->Value = Value;
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
#ifdef ML_COMPLEX
			if (*End == 'i') {
				Compiler->Value = ml_complex(Double * 1i);
				Compiler->Token = MLT_VALUE;
				Compiler->Next = End + 1;
				return Compiler->Token;
			}
#endif
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
					if (Compiler->Next[0] == '\n') {
						++Compiler->Next;
						++Compiler->Line;
					} else if (Compiler->Next[0] == 0) {
						Compiler->Next = Compiler->Read(Compiler->Data);
						if (!Compiler->Next) ml_compiler_error(Compiler, "ParseError", "End of input in comment");
					} else if (Compiler->Next[0] == '>' && Compiler->Next[1] == ':') {
						Compiler->Next += 2;
						--Level;
					} else if (Compiler->Next[0] == ':' && Compiler->Next[1] == '<') {
						Compiler->Next += 2;
						++Level;
					} else {
						++Compiler->Next;
					}
				} while (Level);
				continue;
			} else if (Compiler->Next[1] == '(') {
				Compiler->Token = MLT_INLINE;
				Compiler->Next += 2;
				return Compiler->Token;
			} else if (Compiler->Next[1] == '$') {
				Compiler->Token = MLT_EXPAND;
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

static inline ml_token_t ml_current(ml_compiler_t *Compiler) {
	if (Compiler->Token == MLT_NONE) ml_scan(Compiler);
	return Compiler->Token;
}

static inline void ml_next(ml_compiler_t *Compiler) {
	Compiler->Token = MLT_NONE;
	Compiler->Source.Line = Compiler->Line;
}

static inline int ml_parse(ml_compiler_t *Compiler, ml_token_t Token) {
	if (Compiler->Token == MLT_NONE) ml_scan(Compiler);
	if (Compiler->Token == Token) {
		Compiler->Token = MLT_NONE;
		Compiler->Source.Line = Compiler->Line;
		return 1;
	} else {
		return 0;
	}
}

static inline void ml_skip_eol(ml_compiler_t *Compiler) {
	if (Compiler->Token == MLT_NONE) ml_scan(Compiler);
	while (Compiler->Token == MLT_EOL) ml_scan(Compiler);
}

static inline int ml_parse2(ml_compiler_t *Compiler, ml_token_t Token) {
	if (Compiler->Token == MLT_NONE) ml_scan(Compiler);
	while (Compiler->Token == MLT_EOL) ml_scan(Compiler);
	if (Compiler->Token == Token) {
		Compiler->Token = MLT_NONE;
		Compiler->Source.Line = Compiler->Line;
		return 1;
	} else {
		return 0;
	}
}

static void ml_accept(ml_compiler_t *Compiler, ml_token_t Token) {
	if (ml_parse2(Compiler, Token)) return;
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
static mlc_expr_t *ml_parse_term(ml_compiler_t *Compiler, int MethDecl);
static mlc_expr_t *ml_accept_block(ml_compiler_t *Compiler);

static mlc_expr_t *ml_accept_fun_expr(ml_compiler_t *Compiler, ml_token_t EndToken) {
	ML_EXPR(FunExpr, fun, fun);
	FunExpr->Source = Compiler->Source.Name;
	if (!ml_parse2(Compiler, EndToken)) {
		mlc_param_t **ParamSlot = &FunExpr->Params;
		do {
			mlc_param_t *Param = ParamSlot[0] = new(mlc_param_t);
			Param->Line = Compiler->Source.Line;
			ParamSlot = &Param->Next;
			if (ml_parse2(Compiler, MLT_LEFT_SQUARE)) {
				ml_accept(Compiler, MLT_IDENT);
				Param->Ident = Compiler->Ident;
				Param->Flags = ML_PARAM_EXTRA;
				ml_accept(Compiler, MLT_RIGHT_SQUARE);
				if (ml_parse2(Compiler, MLT_COMMA)) {
					ml_accept(Compiler, MLT_LEFT_BRACE);
					mlc_param_t *Param = ParamSlot[0] = new(mlc_param_t);
					Param->Line = Compiler->Source.Line;
					ml_accept(Compiler, MLT_IDENT);
					Param->Ident = Compiler->Ident;
					Param->Flags = ML_PARAM_NAMED;
					ml_accept(Compiler, MLT_RIGHT_BRACE);
				}
				break;
			} else if (ml_parse2(Compiler, MLT_LEFT_BRACE)) {
				ml_accept(Compiler, MLT_IDENT);
				Param->Ident = Compiler->Ident;
				Param->Flags = ML_PARAM_NAMED;
				ml_accept(Compiler, MLT_RIGHT_BRACE);
				break;
			} else {
				if (ml_parse2(Compiler, MLT_BLANK)) {
					Param->Ident = "_";
				} else {
					if (ml_parse2(Compiler, MLT_REF)) Param->Flags = ML_PARAM_BYREF;
					ml_accept(Compiler, MLT_IDENT);
					Param->Ident = Compiler->Ident;
				}
				if (ml_parse2(Compiler, MLT_COLON)) {
					Param->Type = ml_accept_term(Compiler);
				}
			}
		} while (ml_parse2(Compiler, MLT_COMMA));
		ml_accept(Compiler, EndToken);
	}
	if (ml_parse2(Compiler, MLT_COLON)) {
		FunExpr->ReturnType = ml_parse_term(Compiler, 0);
	}
	FunExpr->Body = ml_accept_expression(Compiler, EXPR_DEFAULT);
	FunExpr->StartLine = FunExpr->Body->StartLine;
	return ML_EXPR_END(FunExpr);
}

extern ml_cfunctionx_t MLMethodSet[];

static mlc_expr_t *ml_accept_meth_expr(ml_compiler_t *Compiler) {
	ML_EXPR(MethodExpr, parent_value, const_call);
	MethodExpr->Value = (ml_value_t *)MLMethodSet;
	mlc_expr_t *Method = ml_parse_term(Compiler, 1);
	if (!Method) ml_compiler_error(Compiler, "ParseError", "expected <factor> not <%s>", MLTokens[Compiler->Token]);
	MethodExpr->Child = Method;
	mlc_expr_t **ArgsSlot = &Method->Next;
	ml_accept(Compiler, MLT_LEFT_PAREN);
	ML_EXPR(FunExpr, fun, fun);
	if (!ml_parse2(Compiler, MLT_RIGHT_PAREN)) {
		mlc_param_t **ParamSlot = &FunExpr->Params;
		do {
			if (ml_parse2(Compiler, MLT_OPERATOR)) {
				if (!strcmp(Compiler->Ident, "..")) {
					ML_EXPR(ValueExpr, value, value);
					ValueExpr->Value = ml_method("..");
					mlc_expr_t *Arg = ArgsSlot[0] = ML_EXPR_END(ValueExpr);
					ArgsSlot = &Arg->Next;
					break;
				} else {
					ml_compiler_error(Compiler, "ParseError", "expected <identfier> not %s (%s)", MLTokens[Compiler->Token], Compiler->Ident);
				}
			}
			mlc_param_t *Param = ParamSlot[0] = new(mlc_param_t);
			Param->Line = Compiler->Source.Line;
			ParamSlot = &Param->Next;
			if (ml_parse2(Compiler, MLT_LEFT_SQUARE)) {
				ml_accept(Compiler, MLT_IDENT);
				Param->Ident = Compiler->Ident;
				Param->Flags = ML_PARAM_EXTRA;
				ml_accept(Compiler, MLT_RIGHT_SQUARE);
				if (ml_parse2(Compiler, MLT_COMMA)) {
					ml_accept(Compiler, MLT_LEFT_BRACE);
					mlc_param_t *Param = ParamSlot[0] = new(mlc_param_t);
					Param->Line = Compiler->Source.Line;
					ml_accept(Compiler, MLT_IDENT);
					Param->Ident = Compiler->Ident;
					Param->Flags = ML_PARAM_NAMED;
					ml_accept(Compiler, MLT_RIGHT_BRACE);
				}
				ML_EXPR(ValueExpr, value, value);
				ValueExpr->Value = ml_method("..");
				mlc_expr_t *Arg = ArgsSlot[0] = ML_EXPR_END(ValueExpr);
				ArgsSlot = &Arg->Next;
				break;
			} else if (ml_parse2(Compiler, MLT_LEFT_BRACE)) {
				ml_accept(Compiler, MLT_IDENT);
				Param->Ident = Compiler->Ident;
				Param->Flags = ML_PARAM_NAMED;
				ml_accept(Compiler, MLT_RIGHT_BRACE);
				ML_EXPR(ValueExpr, value, value);
				ValueExpr->Value = ml_method("..");
				mlc_expr_t *Arg = ArgsSlot[0] = ML_EXPR_END(ValueExpr);
				ArgsSlot = &Arg->Next;
				break;
			} else {
				if (ml_parse2(Compiler, MLT_BLANK)) {
					Param->Ident = "_";
				} else {
					ml_accept(Compiler, MLT_IDENT);
					Param->Ident = Compiler->Ident;
				}
				ml_accept(Compiler, MLT_COLON);
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
			}
		} while (ml_parse2(Compiler, MLT_COMMA));
		ml_accept(Compiler, MLT_RIGHT_PAREN);
	}
	if (ml_parse2(Compiler, MLT_ASSIGN)) {
		ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
	} else {
		FunExpr->Body = ml_accept_expression(Compiler, EXPR_DEFAULT);
		ArgsSlot[0] = ML_EXPR_END(FunExpr);
	}
	return ML_EXPR_END(MethodExpr);
}

static void ml_accept_named_arguments(ml_compiler_t *Compiler, ml_token_t EndToken, mlc_expr_t **ArgsSlot, ml_value_t *Names) {
	mlc_expr_t **NamesSlot = ArgsSlot;
	mlc_expr_t *Arg = ArgsSlot[0];
	ArgsSlot = &Arg->Next;
	if (ml_parse2(Compiler, MLT_SEMICOLON)) {
		ArgsSlot[0] = ml_accept_fun_expr(Compiler, EndToken);
		return;
	}
	Arg = ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
	ArgsSlot = &Arg->Next;
	while (ml_parse2(Compiler, MLT_COMMA)) {
		if (ml_parse2(Compiler, MLT_IDENT)) {
			ml_names_add(Names, ml_cstring(Compiler->Ident));
		} else if (ml_parse2(Compiler, MLT_VALUE)) {
			if (ml_typeof(Compiler->Value) != MLStringT) {
				ml_compiler_error(Compiler, "ParseError", "Argument names must be identifiers or string");
			}
			ml_names_add(Names, Compiler->Value);
		} else {
			ml_compiler_error(Compiler, "ParseError", "Argument names must be identifiers or string");
		}
		ml_accept(Compiler, MLT_IS);
		if (ml_parse2(Compiler, MLT_SEMICOLON)) {
			ArgsSlot[0] = ml_accept_fun_expr(Compiler, EndToken);
			return;
		}
		Arg = ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
		ArgsSlot = &Arg->Next;
	}
	if (ml_parse2(Compiler, MLT_SEMICOLON)) {
		mlc_expr_t *FunExpr = ml_accept_fun_expr(Compiler, EndToken);
		FunExpr->Next = NamesSlot[0];
		NamesSlot[0] = FunExpr;
	} else {
		ml_accept(Compiler, EndToken);
	}
}

static void ml_accept_arguments(ml_compiler_t *Compiler, ml_token_t EndToken, mlc_expr_t **ArgsSlot) {
	if (ml_parse2(Compiler, MLT_SEMICOLON)) {
		ArgsSlot[0] = ml_accept_fun_expr(Compiler, EndToken);
	} else if (!ml_parse2(Compiler, EndToken)) {
		do {
			mlc_expr_t *Arg = ml_accept_expression(Compiler, EXPR_DEFAULT);
			if (ml_parse2(Compiler, MLT_IS)) {
				ml_value_t *Names = ml_names();
				if (Arg->compile == (void *)ml_ident_expr_compile) {
					ml_names_add(Names, ml_cstring(((mlc_ident_expr_t *)Arg)->Ident));
				} else if (Arg->compile == (void *)ml_value_expr_compile) {
					ml_value_t *Name = ((mlc_value_expr_t *)Arg)->Value;
					if (ml_typeof(Name) != MLStringT) {
						ml_compiler_error(Compiler, "ParseError", "Argument names must be identifiers or strings");
					}
					ml_names_add(Names, Name);
				} else {
					ml_compiler_error(Compiler, "ParseError", "Argument names must be identifiers or strings");
				}
				ML_EXPR(NamesArg, value, value);
				NamesArg->Value = Names;
				ArgsSlot[0] = ML_EXPR_END(NamesArg);
				return ml_accept_named_arguments(Compiler, EndToken, ArgsSlot, Names);
			} else {
				ArgsSlot[0] = Arg;
				ArgsSlot = &Arg->Next;
			}
		} while (ml_parse2(Compiler, MLT_COMMA));
		if (ml_parse2(Compiler, MLT_SEMICOLON)) {
			ArgsSlot[0] = ml_accept_fun_expr(Compiler, EndToken);
		} else {
			ml_accept(Compiler, EndToken);
		}
		return;
	}
}

static mlc_expr_t *ml_accept_with_expr(ml_compiler_t *Compiler, mlc_expr_t *Child) {
	ML_EXPR(WithExpr, local, with);
	mlc_local_t **LocalSlot = &WithExpr->Local;
	mlc_expr_t **ExprSlot = &WithExpr->Child;
	do {
		if (ml_parse2(Compiler, MLT_LEFT_PAREN)) {
			int Count = 0;
			mlc_local_t **First = LocalSlot;
			do {
				ml_accept(Compiler, MLT_IDENT);
				++Count;
				mlc_local_t *Local = LocalSlot[0] = new(mlc_local_t);
				Local->Line = Compiler->Source.Line;
				Local->Ident = Compiler->Ident;
				LocalSlot = &Local->Next;
			} while (ml_parse2(Compiler, MLT_COMMA));
			ml_accept(Compiler, MLT_RIGHT_PAREN);
			First[0]->Index = Count;
		} else {
			ml_accept(Compiler, MLT_IDENT);
			mlc_local_t *Local = LocalSlot[0] = new(mlc_local_t);
			Local->Line = Compiler->Source.Line;
			LocalSlot = &Local->Next;
			Local->Ident = Compiler->Ident;
			Local->Index = 1;
		}
		ml_accept(Compiler, MLT_ASSIGN);
		mlc_expr_t *Expr = ExprSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
		ExprSlot = &Expr->Next;
	} while (ml_parse2(Compiler, MLT_COMMA));
	if (Child) {
		ExprSlot[0] = Child;
	} else {
		ml_accept(Compiler, MLT_DO);
		ExprSlot[0] = ml_accept_block(Compiler);
		ml_accept(Compiler, MLT_END);
	}
	return ML_EXPR_END(WithExpr);
}

static void ml_accept_for_decl(ml_compiler_t *Compiler, mlc_for_expr_t *Expr) {
	if (ml_parse2(Compiler, MLT_IDENT)) {
		const char *Ident = Compiler->Ident;
		if (ml_parse2(Compiler, MLT_COMMA)) {
			Expr->Key = Ident;
		} else {
			mlc_local_t *Local = Expr->Local = new(mlc_local_t);
			Local->Line = Compiler->Source.Line;
			Local->Ident = Ident;
			return;
		}
	}
	if (ml_parse2(Compiler, MLT_LEFT_PAREN)) {
		int Count = 0;
		mlc_local_t **Slot = &Expr->Local;
		do {
			ml_accept(Compiler, MLT_IDENT);
			++Count;
			mlc_local_t *Local = Slot[0] = new(mlc_local_t);
			Local->Line = Compiler->Source.Line;
			Local->Ident = Compiler->Ident;
			Slot = &Local->Next;
		} while (ml_parse2(Compiler, MLT_COMMA));
		ml_accept(Compiler, MLT_RIGHT_PAREN);
		Expr->Unpack = Count;
	} else {
		ml_accept(Compiler, MLT_IDENT);
		mlc_local_t *Local = Expr->Local = new(mlc_local_t);
		Local->Line = Compiler->Source.Line;
		Local->Ident = Compiler->Ident;
	}
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
		[MLT_OLD] = ml_old_expr_compile,
		[MLT_DEBUG] = ml_debug_expr_compile
	};
	switch (ml_current(Compiler)) {
	case MLT_EACH:
	case MLT_NOT:
	case MLT_DEBUG:
	{
		mlc_parent_expr_t *ParentExpr = new(mlc_parent_expr_t);
		ParentExpr->compile = CompileFns[Compiler->Token];
		ml_next(Compiler);
		ParentExpr->StartLine = Compiler->Source.Line;
		ParentExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
		return ML_EXPR_END(ParentExpr);
	}
	case MLT_WHILE:
	case MLT_UNTIL:
	{
		mlc_parent_expr_t *ParentExpr = new(mlc_parent_expr_t);
		ParentExpr->compile = CompileFns[Compiler->Token];
		ml_next(Compiler);
		ParentExpr->StartLine = Compiler->Source.Line;
		ParentExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
		if (ml_parse(Compiler, MLT_COMMA)) {
			ParentExpr->Child->Next = ml_accept_expression(Compiler, EXPR_DEFAULT);
		}
		return ML_EXPR_END(ParentExpr);
	}
	case MLT_EXIT:
	case MLT_RET:
	{
		mlc_parent_expr_t *ParentExpr = new(mlc_parent_expr_t);
		ParentExpr->compile = CompileFns[Compiler->Token];
		ml_next(Compiler);
		ParentExpr->StartLine = Compiler->Source.Line;
		ParentExpr->Child = ml_parse_expression(Compiler, EXPR_DEFAULT);
		return ML_EXPR_END(ParentExpr);
	}
	case MLT_NEXT:
	case MLT_NIL:
	case MLT_BLANK:
	case MLT_OLD:
	{
		mlc_expr_t *Expr = new(mlc_expr_t);
		Expr->compile = CompileFns[Compiler->Token];
		ml_next(Compiler);
		Expr->StartLine = Expr->EndLine = Compiler->Source.Line;
		return Expr;
	}
	case MLT_DO: {
		ml_next(Compiler);
		mlc_expr_t *BlockExpr = ml_accept_block(Compiler);
		ml_accept(Compiler, MLT_END);
		return BlockExpr;
	}
	case MLT_IF: {
		ml_next(Compiler);
		ML_EXPR(IfExpr, if, if);
		mlc_if_case_t **CaseSlot = &IfExpr->Cases;
		do {
			mlc_if_case_t *Case = CaseSlot[0] = new(mlc_if_case_t);
			Case->Line = Compiler->Source.Line;
			CaseSlot = &Case->Next;
			if (ml_parse2(Compiler, MLT_VAR)) {
				ml_accept(Compiler, MLT_IDENT);
				Case->Ident = Compiler->Ident;
				Case->Token = MLT_VAR;
				ml_accept(Compiler, MLT_ASSIGN);
			} else if (ml_parse2(Compiler, MLT_LET)) {
				ml_accept(Compiler, MLT_IDENT);
				Case->Ident = Compiler->Ident;
				Case->Token = MLT_LET;
				ml_accept(Compiler, MLT_ASSIGN);
			}
			Case->Condition = ml_accept_expression(Compiler, EXPR_DEFAULT);
			ml_accept(Compiler, MLT_THEN);
			Case->Body = ml_accept_block(Compiler);
		} while (ml_parse2(Compiler, MLT_ELSEIF));
		if (ml_parse2(Compiler, MLT_ELSE)) IfExpr->Else = ml_accept_block(Compiler);
		ml_accept(Compiler, MLT_END);
		return ML_EXPR_END(IfExpr);
	}
	case MLT_WHEN: {
		ml_next(Compiler);
		ML_EXPR(WhenExpr, local, with);
		char *Ident;
		asprintf(&Ident, "when:%d", Compiler->Source.Line);
		WhenExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
		mlc_local_t *Local = WhenExpr->Local = new(mlc_local_t);
		Local->Line = Compiler->Source.Line;
		Local->Ident = Ident;
		ML_EXPR(IfExpr, if, if);
		mlc_if_case_t **CaseSlot = &IfExpr->Cases;
		do {
			mlc_if_case_t *Case = CaseSlot[0] = new(mlc_if_case_t);
			CaseSlot = &Case->Next;
			mlc_expr_t **ConditionSlot = &Case->Condition;
			ml_accept(Compiler, MLT_IS);
			ml_value_t *Method = MLIsMethod;
			do {
				ML_EXPR(IdentExpr, ident, ident);
				IdentExpr->Ident = Ident;
				if (ml_parse2(Compiler, MLT_NIL)) {
					ML_EXPR(NotExpr, parent, not);
					NotExpr->Child = ML_EXPR_END(IdentExpr);
					ConditionSlot[0] = ML_EXPR_END(NotExpr);
					ConditionSlot = &NotExpr->Next;
					Method = MLIsMethod;
				} else {
					if (ml_parse2(Compiler, MLT_IN)) {
						Method = MLInMethod;
					} else if (ml_parse2(Compiler, MLT_OPERATOR)) {
						Method = ml_method(Compiler->Ident);
					}
					if (!Method) ml_compiler_error(Compiler, "ParseError", "Expected operator not %s", MLTokens[Compiler->Token]);
					IdentExpr->Next = ml_accept_expression(Compiler, EXPR_DEFAULT);
					ML_EXPR(CallExpr, parent_value, const_call);
					CallExpr->Value = Method;
					CallExpr->Child = ML_EXPR_END(IdentExpr);
					ConditionSlot[0] = ML_EXPR_END(CallExpr);
					ConditionSlot = &CallExpr->Next;
				}
			} while (ml_parse2(Compiler, MLT_COMMA));
			if (Case->Condition->Next) {
				ML_EXPR(OrExpr, parent, or);
				OrExpr->Child = Case->Condition;
				Case->Condition = ML_EXPR_END(OrExpr);
			}
			ml_accept(Compiler, MLT_DO);
			Case->Body = ml_accept_block(Compiler);
			if (ml_parse2(Compiler, MLT_ELSE)) {
				IfExpr->Else = ml_accept_block(Compiler);
				ml_accept(Compiler, MLT_END);
				break;
			}
		} while (!ml_parse2(Compiler, MLT_END));
		WhenExpr->Child->Next = ML_EXPR_END(IfExpr);
		return ML_EXPR_END(WhenExpr);
	}
	case MLT_LOOP: {
		ml_next(Compiler);
		ML_EXPR(LoopExpr, parent, loop);
		LoopExpr->Child = ml_accept_block(Compiler);
		ml_accept(Compiler, MLT_END);
		return ML_EXPR_END(LoopExpr);
	}
	case MLT_FOR: {
		ml_next(Compiler);
		ML_EXPR(ForExpr, for, for);
		ml_accept_for_decl(Compiler, ForExpr);
		ml_accept(Compiler, MLT_IN);
		ForExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
		ml_accept(Compiler, MLT_DO);
		ForExpr->Child->Next = ml_accept_block(Compiler);
		if (ml_parse2(Compiler, MLT_ELSE)) {
			ForExpr->Child->Next->Next = ml_accept_block(Compiler);
		}
		ml_accept(Compiler, MLT_END);
		return ML_EXPR_END(ForExpr);
	}
	case MLT_FUN: {
		ml_next(Compiler);
		if (ml_parse2(Compiler, MLT_LEFT_PAREN)) {
			return ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
		} else {
			ML_EXPR(FunExpr, fun, fun);
			FunExpr->Body = ml_accept_expression(Compiler, EXPR_DEFAULT);
			return ML_EXPR_END(FunExpr);
		}
	}
	case MLT_METH: {
		ml_next(Compiler);
		return ml_accept_meth_expr(Compiler);
	}
	case MLT_SUSP: {
		ml_next(Compiler);
		ML_EXPR(SuspendExpr, parent, suspend);
		SuspendExpr->Child = ml_parse_expression(Compiler, EXPR_DEFAULT);
		if (ml_parse(Compiler, MLT_COMMA)) {
			SuspendExpr->Child->Next = ml_accept_expression(Compiler, EXPR_DEFAULT);
		}
		return ML_EXPR_END(SuspendExpr);
	}
	case MLT_WITH: {
		ml_next(Compiler);
		return ml_accept_with_expr(Compiler, NULL);
	}
	case MLT_IDENT: {
		ml_next(Compiler);
		ML_EXPR(IdentExpr, ident, ident);
		IdentExpr->Ident = Compiler->Ident;
		return ML_EXPR_END(IdentExpr);
	}
	case MLT_VALUE: {
		ml_next(Compiler);
		ML_EXPR(ValueExpr, value, value);
		ValueExpr->Value = Compiler->Value;
		return ML_EXPR_END(ValueExpr);
	}
	case MLT_EXPR: {
		ml_next(Compiler);
		return Compiler->Expr;
	}
	case MLT_INLINE: {
		ml_next(Compiler);
		ML_EXPR(InlineExpr, parent, inline);
		InlineExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
		ml_accept(Compiler, MLT_RIGHT_PAREN);
		return ML_EXPR_END(InlineExpr);
	}
	case MLT_EXPAND: {
		ml_next(Compiler);
		ml_accept(Compiler, MLT_IDENT);
		ML_EXPR(DefineExpr, ident, define);
		DefineExpr->Ident = Compiler->Ident;
		return ML_EXPR_END(DefineExpr);
	}
	case MLT_LEFT_PAREN: {
		ml_next(Compiler);
		if (ml_parse2(Compiler, MLT_SEMICOLON)) {
			ML_EXPR(TupleExpr, parent, tuple);
			TupleExpr->Child = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			return ML_EXPR_END(TupleExpr);
		}
		mlc_expr_t *Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
		if (ml_parse2(Compiler, MLT_COMMA)) {
			ML_EXPR(TupleExpr, parent, tuple);
			TupleExpr->Child = Expr;
			ml_accept_arguments(Compiler, MLT_RIGHT_PAREN, &Expr->Next);
			Expr = ML_EXPR_END(TupleExpr);
		} else if (ml_parse2(Compiler, MLT_SEMICOLON)) {
			ML_EXPR(TupleExpr, parent, tuple);
			TupleExpr->Child = Expr;
			Expr->Next = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			Expr = ML_EXPR_END(TupleExpr);
		} else {
			ml_accept(Compiler, MLT_RIGHT_PAREN);
		}
		return Expr;
	}
	case MLT_LEFT_SQUARE: {
		ml_next(Compiler);
		ML_EXPR(ListExpr, parent, list);
		mlc_expr_t **ArgsSlot = &ListExpr->Child;
		if (!ml_parse2(Compiler, MLT_RIGHT_SQUARE)) {
			do {
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
			} while (ml_parse2(Compiler, MLT_COMMA));
			ml_accept(Compiler, MLT_RIGHT_SQUARE);
		}
		return ML_EXPR_END(ListExpr);
	}
	case MLT_LEFT_BRACE: {
		ml_next(Compiler);
		ML_EXPR(MapExpr, parent, map);
		mlc_expr_t **ArgsSlot = &MapExpr->Child;
		if (!ml_parse2(Compiler, MLT_RIGHT_BRACE)) {
			do {
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
				if (ml_parse2(Compiler, MLT_IS)) {
					mlc_expr_t *ArgExpr = ArgsSlot[0] = ml_accept_expression(Compiler, EXPR_DEFAULT);
					ArgsSlot = &ArgExpr->Next;
				} else {
					ML_EXPR(ArgExpr, value, value);
					ArgExpr->Value = MLSome;
					ArgsSlot[0] = ML_EXPR_END(ArgExpr);
					ArgsSlot = &ArgExpr->Next;
				}
			} while (ml_parse2(Compiler, MLT_COMMA));
			ml_accept(Compiler, MLT_RIGHT_BRACE);
		}
		return ML_EXPR_END(MapExpr);
	}
	case MLT_OPERATOR: {
		ml_next(Compiler);
		ml_value_t *Operator = ml_method(Compiler->Ident);
		if (MethDecl) {
			ML_EXPR(ValueExpr, value, value);
			ValueExpr->Value = Operator;
			return ML_EXPR_END(ValueExpr);
		} else if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
			ML_EXPR(CallExpr, parent_value, const_call);
			CallExpr->Value = Operator;
			ml_accept_arguments(Compiler, MLT_RIGHT_PAREN, &CallExpr->Child);
			return ML_EXPR_END(CallExpr);
		} else {
			mlc_expr_t *Child = ml_parse_term(Compiler, 0);
			if (Child) {
				ML_EXPR(CallExpr, parent_value, const_call);
				CallExpr->Value = Operator;
				CallExpr->Child = Child;
				return ML_EXPR_END(CallExpr);
			} else {
				ML_EXPR(ValueExpr, value, value);
				ValueExpr->Value = Operator;
				return ML_EXPR_END(ValueExpr);
			}
		}
	}
	case MLT_METHOD: {
		ml_next(Compiler);
		ML_EXPR(ValueExpr, value, value);
		ValueExpr->Value = ml_method(Compiler->Ident);
		return ML_EXPR_END(ValueExpr);
	}
	default: return NULL;
	}
}

static mlc_expr_t *ml_parse_term(ml_compiler_t *Compiler, int MethDecl) {
	mlc_expr_t *Expr = ml_parse_factor(Compiler, MethDecl);
	if (!Expr) return NULL;
	for (;;) {
		switch (ml_current(Compiler)) {
		case MLT_LEFT_PAREN: {
			if (MethDecl) return Expr;
			ml_next(Compiler);
			ML_EXPR(CallExpr, parent, call);
			CallExpr->Child = Expr;
			ml_accept_arguments(Compiler, MLT_RIGHT_PAREN, &Expr->Next);
			Expr = ML_EXPR_END(CallExpr);
			break;
		}
		case MLT_LEFT_SQUARE: {
			ml_next(Compiler);
			ML_EXPR(IndexExpr, parent_value, const_call);
			IndexExpr->Value = IndexMethod;
			IndexExpr->Child = Expr;
			ml_accept_arguments(Compiler, MLT_RIGHT_SQUARE, &Expr->Next);
			Expr = ML_EXPR_END(IndexExpr);
			break;
		}
		case MLT_METHOD: {
			ml_next(Compiler);
			ML_EXPR(CallExpr, parent_value, const_call);
			CallExpr->Value = ml_method(Compiler->Ident);
			CallExpr->Child = Expr;
			if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
				ml_accept_arguments(Compiler, MLT_RIGHT_PAREN, &Expr->Next);
			}
			Expr = ML_EXPR_END(CallExpr);
			break;
		}
		case MLT_IMPORT: {
			ml_next(Compiler);
			if (!ml_parse2(Compiler, MLT_OPERATOR) && !ml_parse2(Compiler, MLT_IDENT)) {
				ml_accept(Compiler, MLT_VALUE);
				if (!ml_is(Compiler->Value, MLStringT)) {
					ml_compiler_error(Compiler, "ParseError", "expected import not %s", MLTokens[Compiler->Token]);
				}
				Compiler->Ident = ml_string_value(Compiler->Value);
			}
			ML_EXPR(ResolveExpr, parent_value, resolve);
			ResolveExpr->Value = ml_string(Compiler->Ident, -1);
			ResolveExpr->Child = Expr;
			Expr = ML_EXPR_END(ResolveExpr);
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
	ml_skip_eol(Compiler);
	mlc_expr_t *Expr = ml_parse_term(Compiler, 0);
	if (!Expr) ml_compiler_error(Compiler, "ParseError", "expected <expression> not %s", MLTokens[Compiler->Token]);
	return Expr;
}

static mlc_expr_t *ml_parse_expression(ml_compiler_t *Compiler, ml_expr_level_t Level) {
	mlc_expr_t *Expr = ml_parse_term(Compiler, 0);
	if (!Expr) return NULL;
	for (;;) switch (ml_current(Compiler)) {
	case MLT_OPERATOR: case MLT_IDENT: {
		ml_next(Compiler);
		ML_EXPR(CallExpr, parent_value, const_call);
		CallExpr->Value = ml_method(Compiler->Ident);
		CallExpr->Child = Expr;
		if (ml_parse2(Compiler, MLT_LEFT_PAREN)) {
			ml_accept_arguments(Compiler, MLT_RIGHT_PAREN, &Expr->Next);
		} else {
			Expr->Next = ml_accept_term(Compiler);
		}
		Expr = ML_EXPR_END(CallExpr);
		break;
	}
	case MLT_ASSIGN: {
		ml_next(Compiler);
		ML_EXPR(AssignExpr, parent, assign);
		AssignExpr->Child = Expr;
		Expr->Next = ml_accept_expression(Compiler, EXPR_DEFAULT);
		Expr = ML_EXPR_END(AssignExpr);
		break;
	}
	case MLT_IN: {
		ml_next(Compiler);
		ML_EXPR(CallExpr, parent_value, const_call);
		CallExpr->Value = MLInMethod;
		CallExpr->Child = Expr;
		Expr->Next = ml_accept_expression(Compiler, EXPR_SIMPLE);
		Expr = ML_EXPR_END(CallExpr);
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
		Expr = ML_EXPR_END(AndExpr);
	}
	if (Level >= EXPR_OR && ml_parse(Compiler, MLT_OR)) {
		ML_EXPR(OrExpr, parent, or);
		mlc_expr_t *LastChild = OrExpr->Child = Expr;
		do {
			LastChild = LastChild->Next = ml_accept_expression(Compiler, EXPR_AND);
		} while (ml_parse(Compiler, MLT_OR));
		Expr = ML_EXPR_END(OrExpr);
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
			mlc_expr_t *Body = ML_EXPR_END(SuspendExpr);
			do {
				ML_EXPR(ForExpr, for, for);
				ml_accept_for_decl(Compiler, ForExpr);
				ml_accept(Compiler, MLT_IN);
				ForExpr->Child = ml_accept_expression(Compiler, EXPR_OR);
				for (;;) {
					if (ml_parse2(Compiler, MLT_IF)) {
						ML_EXPR(IfExpr, if, if);
						mlc_if_case_t *IfCase = IfExpr->Cases = new(mlc_if_case_t);
						IfCase->Condition = ml_accept_expression(Compiler, EXPR_OR);
						IfCase->Body = Body;
						Body = ML_EXPR_END(IfExpr);
					} else if (ml_parse2(Compiler, MLT_WITH)) {
						Body = ml_accept_with_expr(Compiler, Body);
					} else {
						break;
					}
				}
				ForExpr->Child->Next = Body;
				Body = ML_EXPR_END(ForExpr);
			} while (ml_parse2(Compiler, MLT_FOR));
			FunExpr->Body = Body;
			FunExpr->StartLine = FunExpr->Body->StartLine;
			Expr = ML_EXPR_END(FunExpr);
		}
	}
	return Expr;
}

static mlc_expr_t *ml_accept_expression(ml_compiler_t *Compiler, ml_expr_level_t Level) {
	ml_skip_eol(Compiler);
	mlc_expr_t *Expr = ml_parse_expression(Compiler, Level);
	if (!Expr) ml_compiler_error(Compiler, "ParseError", "expected <expression> not %s", MLTokens[Compiler->Token]);
	return Expr;
}

typedef struct {
	mlc_expr_t **ExprSlot;
	mlc_local_t **VarsSlot;
	mlc_local_t **LetsSlot;
	mlc_local_t **DefsSlot;
} ml_accept_block_t;

static void ml_accept_block_var(ml_compiler_t *Compiler, ml_accept_block_t *Accept) {
	do {
		if (ml_parse2(Compiler, MLT_LEFT_PAREN)) {
			int Count = 0;
			mlc_local_t *Locals, **Slot = &Locals;
			do {
				ml_accept(Compiler, MLT_IDENT);
				++Count;
				mlc_local_t *Local = Slot[0] = new(mlc_local_t);
				Local->Line = Compiler->Source.Line;
				Local->Ident = Compiler->Ident;
				Slot = &Local->Next;
			} while (ml_parse2(Compiler, MLT_COMMA));
			Accept->VarsSlot[0] = Locals;
			Accept->VarsSlot = Slot;
			ml_accept(Compiler, MLT_RIGHT_PAREN);
			if (ml_parse2(Compiler, MLT_IN)) {
				ML_EXPR(LocalExpr, local, var_in);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			} else {
				ml_accept(Compiler, MLT_ASSIGN);
				ML_EXPR(LocalExpr, local, var_unpack);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			}
		} else {
			ml_accept(Compiler, MLT_IDENT);
			mlc_local_t *Local = Accept->VarsSlot[0] = new(mlc_local_t);
			Local->Line = Compiler->Source.Line;
			Local->Ident = Compiler->Ident;
			Accept->VarsSlot = &Local->Next;
			if (ml_parse(Compiler, MLT_COLON)) {
				ML_EXPR(TypeExpr, local, var_type);
				TypeExpr->Local = Local;
				TypeExpr->Child = ml_accept_term(Compiler);
				Accept->ExprSlot[0] = ML_EXPR_END(TypeExpr);
				Accept->ExprSlot = &TypeExpr->Next;
			}
			mlc_expr_t *Child = NULL;
			if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
				Child = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			} else if (ml_parse(Compiler, MLT_ASSIGN)) {
				Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
			}
			if (Child) {
				ML_EXPR(LocalExpr, local, var);
				LocalExpr->Local = Local;
				LocalExpr->Child = Child;
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			}
		}
	} while (ml_parse(Compiler, MLT_COMMA));
}

static void ml_accept_block_let(ml_compiler_t *Compiler, ml_accept_block_t *Accept, int Flags) {
	do {
		if (ml_parse2(Compiler, MLT_LEFT_PAREN)) {
			int Count = 0;
			mlc_local_t *Locals, **Slot = &Locals;
			do {
				ml_accept(Compiler, MLT_IDENT);
				++Count;
				mlc_local_t *Local = Slot[0] = new(mlc_local_t);
				Local->Line = Compiler->Source.Line;
				Local->Ident = Compiler->Ident;
				Slot = &Local->Next;
			} while (ml_parse2(Compiler, MLT_COMMA));
			Accept->LetsSlot[0] = Locals;
			Accept->LetsSlot = Slot;
			ml_accept(Compiler, MLT_RIGHT_PAREN);
			if (ml_parse2(Compiler, MLT_IN)) {
				ML_EXPR(LocalExpr, local, let_in);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
				LocalExpr->Flags = Flags;
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			} else {
				ml_accept(Compiler, MLT_ASSIGN);
				ML_EXPR(LocalExpr, local, let_unpack);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
				LocalExpr->Flags = Flags;
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			}
		} else {
			ml_accept(Compiler, MLT_IDENT);
			mlc_local_t *Local = Accept->LetsSlot[0] = new(mlc_local_t);
			Local->Line = Compiler->Source.Line;
			Local->Ident = Compiler->Ident;
			Accept->LetsSlot = &Local->Next;
			ML_EXPR(LocalExpr, local, let);
			if (ml_parse2(Compiler, MLT_LEFT_PAREN)) {
				LocalExpr->Child = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			} else {
				ml_accept(Compiler, MLT_ASSIGN);
				LocalExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
			}
			LocalExpr->Local = Local;
			LocalExpr->Flags = Flags;
			Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
			Accept->ExprSlot = &LocalExpr->Next;
		}
	} while (ml_parse(Compiler, MLT_COMMA));
}

static void ml_accept_block_def(ml_compiler_t *Compiler, ml_accept_block_t *Accept) {
	do {
		if (ml_parse2(Compiler, MLT_LEFT_PAREN)) {
			int Count = 0;
			mlc_local_t *Locals, **Slot = &Locals;
			do {
				ml_accept(Compiler, MLT_IDENT);
				++Count;
				mlc_local_t *Local = Slot[0] = new(mlc_local_t);
				Local->Line = Compiler->Source.Line;
				Local->Ident = Compiler->Ident;
				Slot = &Local->Next;
			} while (ml_parse2(Compiler, MLT_COMMA));
			Accept->DefsSlot[0] = Locals;
			Accept->DefsSlot = Slot;
			ml_accept(Compiler, MLT_RIGHT_PAREN);
			if (ml_parse2(Compiler, MLT_IN)) {
				ML_EXPR(LocalExpr, local, def_in);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			} else {
				ml_accept(Compiler, MLT_ASSIGN);
				ML_EXPR(LocalExpr, local, def_unpack);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			}
		} else {
			ml_accept(Compiler, MLT_IDENT);
			mlc_local_t *Local = Accept->DefsSlot[0] = new(mlc_local_t);
			Local->Line = Compiler->Source.Line;
			Local->Ident = Compiler->Ident;
			Accept->DefsSlot = &Local->Next;
			ML_EXPR(LocalExpr, local, def);
			if (ml_parse2(Compiler, MLT_LEFT_PAREN)) {
				LocalExpr->Child = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			} else {
				ml_accept(Compiler, MLT_ASSIGN);
				LocalExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
			}
			LocalExpr->Local = Local;
			Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
			Accept->ExprSlot = &LocalExpr->Next;
		}
	} while (ml_parse(Compiler, MLT_COMMA));
}

static void ml_accept_block_fun(ml_compiler_t *Compiler, ml_accept_block_t *Accept) {
	if (ml_parse2(Compiler, MLT_IDENT)) {
		mlc_local_t *Local = Accept->LetsSlot[0] = new(mlc_local_t);
		Local->Line = Compiler->Source.Line;
		Local->Ident = Compiler->Ident;
		Accept->LetsSlot = &Local->Next;
		ml_accept(Compiler, MLT_LEFT_PAREN);
		ML_EXPR(LocalExpr, local, let);
		LocalExpr->Local = Local;
		LocalExpr->Child = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
		Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
		Accept->ExprSlot = &LocalExpr->Next;
	} else {
		ml_accept(Compiler, MLT_LEFT_PAREN);
		mlc_expr_t *Expr = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
		Accept->ExprSlot[0] = Expr;
		Accept->ExprSlot = &Expr->Next;
	}
}

static void ml_accept_block_macro(ml_compiler_t *Compiler, ml_accept_block_t *Accept) {
	ml_accept(Compiler, MLT_IDENT);
	ML_EXPR(ValueExpr, value, value);
	ValueExpr->StartLine = Compiler->Source.Line;
	mlc_local_t *Local = Accept->DefsSlot[0] = new(mlc_local_t);
	Local->Line = Compiler->Source.Line;
	Local->Ident = Compiler->Ident;
	Accept->DefsSlot = &Local->Next;
	ml_accept(Compiler, MLT_LEFT_PAREN);
	ml_template_macro_t *Macro = new(ml_template_macro_t);
	Macro->Base.Type = MLMacroT;
	Macro->Base.apply = (void *)ml_template_macro_apply;
	ml_decl_t **ParamSlot = &Macro->Params;
	if (!ml_parse2(Compiler, MLT_RIGHT_PAREN)) {
		do {
			ml_accept(Compiler, MLT_IDENT);
			ml_decl_t *Param = ParamSlot[0] = new(ml_decl_t);
			Param->Ident = Compiler->Ident;
			Param->Hash = ml_ident_hash(Compiler->Ident);
			ParamSlot = &Param->Next;
		} while (ml_parse2(Compiler, MLT_COMMA));
		ml_accept(Compiler, MLT_RIGHT_PAREN);
	}
	Macro->Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
	ValueExpr->Value = (ml_value_t *)Macro;
	ML_EXPR_END(ValueExpr);
	ML_EXPR(LocalExpr, local, def);
	LocalExpr->Local = Local;
	LocalExpr->Child = (mlc_expr_t *)ValueExpr;
	Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
	Accept->ExprSlot = &LocalExpr->Next;
}

static mlc_expr_t *ml_accept_block_export(ml_compiler_t *Compiler, mlc_expr_t *Expr, mlc_local_t *Export) {
	ML_EXPR(CallExpr, parent, call);
	CallExpr->Child = Expr;
	ml_value_t *Names = ml_names();
	ML_EXPR(NamesExpr, value, value);
	NamesExpr->Value = Names;
	Expr->Next = ML_EXPR_END(NamesExpr);
	mlc_expr_t **ArgsSlot = &NamesExpr->Next;
	while (Export) {
		ml_names_add(Names, ml_cstring(Export->Ident));
		ML_EXPR(IdentExpr, ident, ident);
		IdentExpr->Ident = Export->Ident;
		ArgsSlot[0] = ML_EXPR_END(IdentExpr);
		ArgsSlot = &IdentExpr->Next;
		Export = Export->Next;
	}
	return ML_EXPR_END(CallExpr);
}

static mlc_expr_t *ml_parse_block_expr(ml_compiler_t *Compiler, ml_accept_block_t *Accept) {
	mlc_expr_t *Expr = ml_parse_expression(Compiler, EXPR_DEFAULT);
	if (!Expr) return NULL;
	if (ml_parse(Compiler, MLT_COLON)) {
		if (ml_parse2(Compiler, MLT_VAR)) {
			mlc_local_t **Exports = Accept->VarsSlot;
			ml_accept_block_var(Compiler, Accept);
			Expr = ml_accept_block_export(Compiler, Expr, Exports[0]);
		} else if (ml_parse2(Compiler, MLT_LET)) {
			mlc_local_t **Exports = Accept->LetsSlot;
			ml_accept_block_let(Compiler, Accept, MLT_LET);
			Expr = ml_accept_block_export(Compiler, Expr, Exports[0]);
		} else if (ml_parse2(Compiler, MLT_REF)) {
			mlc_local_t **Exports = Accept->LetsSlot;
			ml_accept_block_let(Compiler, Accept, MLT_REF);
			Expr = ml_accept_block_export(Compiler, Expr, Exports[0]);
		} else if (ml_parse2(Compiler, MLT_DEF)) {
			mlc_local_t **Exports = Accept->DefsSlot;
			ml_accept_block_def(Compiler, Accept);
			Expr = ml_accept_block_export(Compiler, Expr, Exports[0]);
		} else if (ml_parse2(Compiler, MLT_FUN)) {
			mlc_local_t **Exports = Accept->LetsSlot;
			ml_accept_block_fun(Compiler, Accept);
			Expr = ml_accept_block_export(Compiler, Expr, Exports[0]);
		} else {
			ml_accept_block_t Previous = *Accept;
			mlc_expr_t *Child = ml_parse_block_expr(Compiler, Accept);
			if (!Child) ml_compiler_error(Compiler, "ParseError", "Expected expression");
			if (Accept->VarsSlot != Previous.VarsSlot) {
				Accept->ExprSlot[0] = Child;
				Accept->ExprSlot = &Child->Next;
				Expr = ml_accept_block_export(Compiler, Expr, Previous.VarsSlot[0]);
			} else if (Accept->LetsSlot != Previous.LetsSlot) {
				Accept->ExprSlot[0] = Child;
				Accept->ExprSlot = &Child->Next;
				Expr = ml_accept_block_export(Compiler, Expr, Previous.LetsSlot[0]);
			} else if (Accept->DefsSlot != Previous.DefsSlot) {
				Accept->ExprSlot[0] = Child;
				Accept->ExprSlot = &Child->Next;
				Expr = ml_accept_block_export(Compiler, Expr, Previous.DefsSlot[0]);
			} else {
				mlc_parent_expr_t *CallExpr = (mlc_parent_expr_t *)Child;
				if (CallExpr->compile != ml_call_expr_compile) {
					ml_compiler_error(Compiler, "ParseError", "Invalid declaration");
				}
				mlc_ident_expr_t *IdentExpr = (mlc_ident_expr_t *)CallExpr->Child;
				if (!IdentExpr || IdentExpr->compile != ml_ident_expr_compile) {
					ml_compiler_error(Compiler, "ParseError", "Invalid declaration");
				}
				mlc_local_t *Local = Accept->DefsSlot[0] = new(mlc_local_t);
				Local->Line = IdentExpr->StartLine;
				Local->Ident = IdentExpr->Ident;
				Accept->DefsSlot = &Local->Next;
				ML_EXPR(LocalExpr, local, def);
				LocalExpr->Local = Local;
				Expr->Next = IdentExpr->Next;
				CallExpr->Child = Expr;
				LocalExpr->Child = ML_EXPR_END(CallExpr);
				Expr = ML_EXPR_END(LocalExpr);
			}
		}
	}
	return Expr;
}

static mlc_block_expr_t *ml_accept_block_body(ml_compiler_t *Compiler) {
	ML_EXPR(BlockExpr, block, block);
	ml_accept_block_t Accept[1];
	Accept->ExprSlot = &BlockExpr->Child;
	Accept->VarsSlot = &BlockExpr->Vars;
	Accept->LetsSlot = &BlockExpr->Lets;
	Accept->DefsSlot = &BlockExpr->Defs;
	do {
		ml_skip_eol(Compiler);
		switch (ml_current(Compiler)) {
		case MLT_VAR: {
			ml_next(Compiler);
			ml_accept_block_var(Compiler, Accept);
			break;
		}
		case MLT_LET: {
			ml_next(Compiler);
			ml_accept_block_let(Compiler, Accept, MLT_LET);
			break;
		}
		case MLT_REF: {
			ml_next(Compiler);
			ml_accept_block_let(Compiler, Accept, MLT_REF);
			break;
		}
		case MLT_DEF: {
			ml_next(Compiler);
			ml_accept_block_def(Compiler, Accept);
			break;
		}
		case MLT_FUN: {
			ml_next(Compiler);
			ml_accept_block_fun(Compiler, Accept);
			break;
		}
		case MLT_MACRO: {
			ml_next(Compiler);
			ml_accept_block_macro(Compiler, Accept);
			break;
		}
		default: {
			mlc_expr_t *Expr = ml_parse_block_expr(Compiler, Accept);
			if (!Expr) goto finish;
			Accept->ExprSlot[0] = Expr;
			Accept->ExprSlot = &Expr->Next;
			break;
		}
		}
	} while (ml_parse(Compiler, MLT_SEMICOLON) || ml_parse(Compiler, MLT_EOL));
	finish: {
		int Index = 0, First = 0;
		for (mlc_local_t *Local = BlockExpr->Vars; Local; Local = Local->Next) {
			Local->Index = Index++;
		}
		BlockExpr->NumVars = Index;
		First = Index;
		for (mlc_local_t *Local = BlockExpr->Lets; Local; Local = Local->Next) {
			Local->Index = Index++;
		}
		BlockExpr->NumLets = Index - First;
		First = Index;
		for (mlc_local_t *Local = BlockExpr->Defs; Local; Local = Local->Next) {
			Local->Index = Index++;
		}
		BlockExpr->NumDefs = Index - First;
	}
	return BlockExpr;
}

static mlc_expr_t *ml_accept_block(ml_compiler_t *Compiler) {
	mlc_block_expr_t *BlockExpr = ml_accept_block_body(Compiler);
	if (ml_parse(Compiler, MLT_ON)) {
		mlc_catch_expr_t **CatchSlot = &BlockExpr->Catches;
		do {
			mlc_catch_expr_t *CatchExpr = CatchSlot[0] = new(mlc_catch_expr_t);
			CatchExpr->Line = Compiler->Source.Line;
			CatchSlot = &CatchExpr->Next;
			ml_accept(Compiler, MLT_IDENT);
			CatchExpr->Ident = Compiler->Ident;
			if (ml_parse2(Compiler, MLT_COLON)) {
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
				} while (ml_parse2(Compiler, MLT_COMMA));
			}
			ml_accept(Compiler, MLT_DO);
			mlc_block_expr_t *Body = ml_accept_block_body(Compiler);
			CatchExpr->Body = ML_EXPR_END(Body);
		} while (ml_parse(Compiler, MLT_ON));
	}
	return ML_EXPR_END(BlockExpr);
}

static ml_value_t *ml_compile(ml_compiler_t *Compiler, mlc_expr_t *Expr, const char **Parameters) {
	mlc_function_t Function[1];
	memset(Function, 0, sizeof(mlc_function_t));
	Function->Compiler = Compiler;
	Function->Source = Compiler->Source.Name;
	SHA256_CTX HashCompiler[1];
	sha256_init(HashCompiler);
	ml_closure_info_t *Info = new(ml_closure_info_t);
	int NumParams = 0;
	if (Parameters) {
		ml_decl_t **ParamSlot = &Function->Decls;
		for (const char **P = Parameters; P[0]; ++P) {
			ml_decl_t *Param = new(ml_decl_t);
			Param->Source.Name = Function->Source;
			Param->Source.Line = Expr->StartLine;
			Param->Ident = P[0];
			Param->Hash = ml_ident_hash(P[0]);
			Param->Index = Function->Top++;
			stringmap_insert(Info->Params, Param->Ident, (void *)(intptr_t)Function->Top);
			ParamSlot[0] = Param;
			ParamSlot = &Param->Next;
		}
		NumParams = Function->Top;
		Function->Size = Function->Top + 1;
	}
	Function->Next = anew(ml_inst_t, 128);
	Function->Space = 126;
	Function->Returns = NULL;
	Info->Entry = Function->Next;
	mlc_compile(Function, Expr, 0);
	Info->Return = mlc_emit(Expr->EndLine, MLI_RETURN, 0);
	mlc_link(Function->Returns, Info->Return);
	Info->Halt = Function->Next;
	Info->Source = Function->Source;
	Info->StartLine = Expr->StartLine;
	Info->EndLine = Expr->EndLine;
	Info->FrameSize = Function->Size;
	Info->NumParams = NumParams;
	Info->Decls = Function->Decls;
	ml_closure_t *Closure = new(ml_closure_t);
	Closure->Info = Info;
	Closure->Type = MLClosureT;
	ml_closure_info_finish(Info);
	return (ml_value_t *)Closure;
}

void ml_function_compile(ml_state_t *Caller, ml_compiler_t *Compiler, const char **Params) {
	Compiler->Base.Caller = Caller;
	Compiler->Base.Context = Caller->Context;
	Compiler->State = MLCS_FUNCTION;
	Compiler->Params = Params;
	return ml_compiler_run(Compiler, MLNil);
}

ML_METHODX("compile", MLCompilerT) {
//<Compiler
//>any
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	return ml_function_compile(Caller, Compiler, NULL);
}

ML_METHODX("compile", MLCompilerT, MLListT) {
//<Compiler
//<Parameters
//>any
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	const char **Parameters = anew(const char *, ml_list_length(Args[1]));
	int I = 0;
	ML_LIST_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Value, MLStringT)) ML_ERROR("TypeError", "Parameter name must be a string");
		Parameters[I++] = ml_string_value(Iter->Value);
	}
	return ml_function_compile(Caller, Compiler, Parameters);
}

ML_METHOD("source", MLCompilerT, MLStringT, MLIntegerT) {
//<Compiler
//<Source
//<Line
//>tuple
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	ml_source_t Source = {ml_string_value(Args[1]), ml_integer_value(Args[2])};
	Source = ml_compiler_source(Compiler, Source);
	ml_value_t *Tuple = ml_tuple(2);
	ml_tuple_set(Tuple, 1, ml_cstring(Source.Name));
	ml_tuple_set(Tuple, 2, ml_integer(Source.Line));
	return Tuple;
}

ML_METHOD("reset", MLCompilerT) {
//<Compiler
//>compiler
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	ml_compiler_reset(Compiler);
	return Args[0];
}

ML_METHOD("input", MLCompilerT, MLStringT) {
//<Compiler
//<String
//>compiler
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	ml_compiler_input(Compiler, ml_string_value(Args[1]));
	return Args[0];
}

ML_METHOD("clear", MLCompilerT) {
//<Compiler
//>string
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	return ml_cstring(ml_compiler_clear(Compiler));
}

ML_METHODX("evaluate", MLCompilerT) {
//<Compiler
//>any
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	return ml_command_evaluate(Caller, Compiler);
}

typedef struct {
	ml_state_t Base;
	ml_compiler_t *Compiler;
} ml_evaluate_state_t;

static void ml_evaluate_state_run(ml_evaluate_state_t *State, ml_value_t *Value) {
	if (Value == MLEndOfInput) ML_CONTINUE(State->Base.Caller, MLNil);
	return ml_command_evaluate((ml_state_t *)State, State->Compiler);
}

ML_METHODX("run", MLCompilerT) {
//<Compiler
//>any
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	ml_evaluate_state_t *State = new(ml_evaluate_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_evaluate_state_run;
	State->Compiler = Compiler;
	return ml_command_evaluate((ml_state_t *)State, Compiler);
}

ML_METHOD("[]", MLCompilerT, MLStringT) {
//<Compiler
//<Name
//>any
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	return (ml_value_t *)stringmap_search(Compiler->Vars, ml_string_value(Args[1])) ?: MLNil;
}

ml_value_t MLEndOfInput[1] = {{MLAnyT}};
ml_value_t MLNotFound[1] = {{MLAnyT}};

static ml_value_t *ml_stringmap_global(stringmap_t *Globals, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_value_t *Value = (ml_value_t *)stringmap_search(Globals, ml_string_value(Args[0]));
	return Value ?: MLNotFound;
}

ml_value_t *ml_stringmap_globals(stringmap_t *Globals) {
	return ml_cfunction(Globals, (ml_callback_t)ml_stringmap_global);
}

static ml_value_t *ml_global_deref(ml_global_t *Global) {
	if (!Global->Value) return ml_error("NameError", "identifier %s not declared", Global->Name);
	return ml_deref(Global->Value);
}

static ml_value_t *ml_global_assign(ml_global_t *Global, ml_value_t *Value) {
	if (!Global->Value) return ml_error("NameError", "identifier %s not declared", Global->Name);
	return ml_assign(Global->Value, Value);
}

static void ml_global_call(ml_state_t *Caller, ml_global_t *Global, int Count, ml_value_t **Args) {
	return ml_call(Caller, Global->Value, Count, Args);
}

ML_TYPE(MLGlobalT, (), "global",
//!compiler
	.deref = (void *)ml_global_deref,
	.assign = (void *)ml_global_assign,
	.call = (void *)ml_global_call
);

static ml_value_t *ML_TYPED_FN(ml_unpack, MLGlobalT, ml_global_t *Global, int Index) {
	return ml_unpack(Global->Value, Index);
}

ML_METHOD("var", MLCompilerT, MLStringT) {
//<Compiler
//<Name
//>global
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_variable_t *Var = new(ml_variable_t);
	Var->Type = MLVariableT;
	Var->Value = MLNil;
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Compiler->Vars, Name);
	ml_global_t *Global;
	if (!Slot[0] || ml_typeof(Slot[0]) != MLGlobalT) {
		Global = new(ml_global_t);
		Global->Type = MLGlobalT;
		Global->Name = Name;
		Slot[0] = (ml_value_t *)Global;
	} else {
		Global = (ml_global_t *)Slot[0];
	}
	return (Global->Value = (ml_value_t *)Var);
}

ML_METHOD("var", MLCompilerT, MLStringT, MLTypeT) {
//<Compiler
//<Name
//<Type
//>global
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_variable_t *Var = new(ml_variable_t);
	Var->Type = MLVariableT;
	Var->Value = MLNil;
	Var->VarType = (ml_type_t *)Args[2];
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Compiler->Vars, Name);
	ml_global_t *Global;
	if (!Slot[0] || ml_typeof(Slot[0]) != MLGlobalT) {
		Global = new(ml_global_t);
		Global->Type = MLGlobalT;
		Global->Name = Name;
		Slot[0] = (ml_value_t *)Global;
	} else {
		Global = (ml_global_t *)Slot[0];
	}
	return (Global->Value = (ml_value_t *)Var);
}

ML_METHOD("let", MLCompilerT, MLStringT, MLAnyT) {
//<Compiler
//<Name
//<Value
//>global
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Compiler->Vars, Name);
	ml_global_t *Global;
	if (!Slot[0] || ml_typeof(Slot[0]) != MLGlobalT) {
		Global = new(ml_global_t);
		Global->Type = MLGlobalT;
		Global->Name = Name;
		Slot[0] = (ml_value_t *)Global;
	} else {
		Global = (ml_global_t *)Slot[0];
	}
	return (Global->Value = Args[2]);
}

ML_METHOD("def", MLCompilerT, MLStringT, MLAnyT) {
//<Compiler
//<Name
//<Value
//>global
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Compiler->Vars, Name);
	ml_global_t *Global;
	if (!Slot[0] || ml_typeof(Slot[0]) != MLGlobalT) {
		Global = new(ml_global_t);
		Global->Type = MLGlobalT;
		Global->Name = Name;
		Slot[0] = (ml_value_t *)Global;
	} else {
		Global = (ml_global_t *)Slot[0];
	}
	return (Global->Value = Args[2]);
}

static int ml_compiler_var_fn(const char *Name, ml_value_t *Value, ml_value_t *Vars) {
	ml_map_insert(Vars, ml_cstring(Name), ml_deref(Value));
	return 0;
}

ML_METHOD("vars", MLCompilerT) {
//<Compiler
//>map
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	ml_value_t *Vars = ml_map();
	stringmap_foreach(Compiler->Vars, Vars, (void *)ml_compiler_var_fn);
	return Vars;
}

static void ml_command_global(stringmap_t *Globals, ml_token_t Type, const char *Name, ml_value_t *Value, ml_type_t *VarType) {
	ml_global_t *Global;
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Globals, Name);
	if (!Slot[0]) {
		Global = new(ml_global_t);
		Global->Type = MLGlobalT;
		Global->Name = Name;
		Slot[0] = (ml_value_t *)Global;
	} else if (ml_typeof(Slot[0]) == MLGlobalT) {
		Global = (ml_global_t *)Slot[0];
	} else if (ml_typeof(Slot[0]) == MLUninitializedT) {
		Global = new(ml_global_t);
		Global->Type = MLGlobalT;
		Global->Name = Name;
		ml_uninitialized_set(Slot[0], (ml_value_t *)Global);
		Slot[0] = (ml_value_t *)Global;
	} else {
		Global = new(ml_global_t);
		Global->Type = MLGlobalT;
		Global->Name = Name;
		Slot[0] = (ml_value_t *)Global;
	}
	if (Type != MLT_REF) Value = ml_deref(Value);
	if (Type == MLT_VAR) {
		ml_variable_t *Var = new(ml_variable_t);
		Var->Type = MLVariableT;
		Var->Value = Value;
		if (VarType) Var->VarType = VarType;
		Value = (ml_value_t *)Var;
	}
	Global->Value = Value;
}

typedef struct ml_command_decl_t ml_command_decl_t;

struct ml_command_decl_t {
	ml_command_decl_t *Next;
	const char *Ident;
};

static ml_value_t *ml_accept_command_decl(ml_token_t Type, ml_compiler_t *Compiler) {
	ml_value_t *Value = MLNil;
	do {
		if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
			ml_command_decl_t *Decls = NULL;
			ml_command_decl_t **Slot = &Decls;
			do {
				ml_accept(Compiler, MLT_IDENT);
				ml_command_decl_t *Decl = Slot[0] = new(ml_command_decl_t);
				Decl->Ident = Compiler->Ident;
				Slot = &Decl->Next;
			} while (ml_parse(Compiler, MLT_COMMA));
			ml_accept(Compiler, MLT_RIGHT_PAREN);
			if (ml_parse(Compiler, MLT_IN)) {
				mlc_expr_t *Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
				ml_value_t *Args[2];
				Args[0] = Value = mlc_yield_compile_call(Compiler, Expr);
				do {
					Args[1] = ml_cstring(Decls->Ident);
					ml_value_t *Import = mlc_yield_call(Compiler, SymbolMethod, 2, Args);
					if (ml_is_error(Value)) {
						ml_error_trace_add(Value, (ml_source_t){Compiler->Source.Name, Expr->EndLine});
						Compiler->Value = Value;
						longjmp(Compiler->OnError, 1);
					}
					ml_command_global(Compiler->Vars, Type, Decls->Ident, Import, NULL);
				} while ((Decls = Decls->Next));
			} else {
				ml_accept(Compiler, MLT_ASSIGN);
				mlc_expr_t *Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
				Value = mlc_yield_compile_call(Compiler, Expr);
				int Index = 0;
				do {
					ml_value_t *Unpacked = ml_unpack(Value, ++Index);
					ml_command_global(Compiler->Vars, Type, Decls->Ident, Unpacked, NULL);
				} while ((Decls = Decls->Next));
			}
		} else {
			ml_accept(Compiler, MLT_IDENT);
			const char *Ident = Compiler->Ident;
			ml_value_t *VarType = NULL;
			if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
				mlc_expr_t *Expr = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
				Value = mlc_yield_compile_call(Compiler, Expr);
			} else {
				if (ml_parse(Compiler, MLT_COLON)) {
					mlc_expr_t *Expr = ml_accept_term(Compiler);
					VarType = mlc_yield_compile_call(Compiler, Expr);
					if (!ml_is(VarType, MLTypeT)) {
						ml_compiler_error(Compiler, "TypeError", "Expected <type> not <%s>", ml_typeof(VarType)->Name);
					}
				}
				if (Type == MLT_VAR) {
					if (ml_parse(Compiler, MLT_ASSIGN)) {
						mlc_expr_t *Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
						Value = mlc_yield_compile_call(Compiler, Expr);
					} else {
						Value = MLNil;
					}
				} else {
					ml_accept(Compiler, MLT_ASSIGN);
					mlc_expr_t *Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
					Value = mlc_yield_compile_call(Compiler, Expr);
				}
			}
			ml_command_global(Compiler->Vars, Type, Ident, Value, (ml_type_t *)VarType);
		}
	} while (ml_parse(Compiler, MLT_COMMA));
	return Value;
}

static ml_value_t *ml_accept_command(ml_compiler_t *Compiler) {
	ml_skip_eol(Compiler);
	ml_value_t *Value = MLNil;
	if (ml_parse(Compiler, MLT_EOI)) return MLEndOfInput;
	if (ml_parse(Compiler, MLT_VAR)) {
		Value = ml_accept_command_decl(MLT_VAR, Compiler);
		ml_parse(Compiler, MLT_SEMICOLON);
	} else if (ml_parse(Compiler, MLT_LET)) {
		Value = ml_accept_command_decl(MLT_LET, Compiler);
		ml_parse(Compiler, MLT_SEMICOLON);
	} else if (ml_parse(Compiler, MLT_REF)) {
		Value = ml_accept_command_decl(MLT_REF, Compiler);
		ml_parse(Compiler, MLT_SEMICOLON);
	} else if (ml_parse(Compiler, MLT_DEF)) {
		Value = ml_accept_command_decl(MLT_DEF, Compiler);
		ml_parse(Compiler, MLT_SEMICOLON);
	} else if (ml_parse(Compiler, MLT_FUN)) {
		if (ml_parse(Compiler, MLT_IDENT)) {
			const char *Ident = Compiler->Ident;
			ml_accept(Compiler, MLT_LEFT_PAREN);
			mlc_expr_t *Expr = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			ml_parse(Compiler, MLT_SEMICOLON);
			Value = mlc_yield_compile_call(Compiler, Expr);
			ml_command_global(Compiler->Vars, MLT_LET, Ident, Value, NULL);
		} else {
			ml_accept(Compiler, MLT_LEFT_PAREN);
			mlc_expr_t *Expr = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			ml_parse(Compiler, MLT_SEMICOLON);
			Value = mlc_yield_compile_call(Compiler, Expr);
		}
	} else if (ml_parse(Compiler, MLT_MACRO)) {
		ml_accept(Compiler, MLT_IDENT);
		const char *Name = Compiler->Ident;
		ml_accept(Compiler, MLT_LEFT_PAREN);
		ml_template_macro_t *Macro = new(ml_template_macro_t);
		Macro->Base.Type = MLMacroT;
		Macro->Base.apply = (void *)ml_template_macro_apply;
		ml_decl_t **ParamSlot = &Macro->Params;
		if (!ml_parse2(Compiler, MLT_RIGHT_PAREN)) {
			do {
				ml_accept(Compiler, MLT_IDENT);
				ml_decl_t *Param = ParamSlot[0] = new(ml_decl_t);
				Param->Ident = Compiler->Ident;
				Param->Hash = ml_ident_hash(Compiler->Ident);
				ParamSlot = &Param->Next;
			} while (ml_parse2(Compiler, MLT_COMMA));
			ml_accept(Compiler, MLT_RIGHT_PAREN);
		}
		Macro->Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
		ml_parse(Compiler, MLT_SEMICOLON);
		stringmap_insert(Compiler->Vars, Name, Macro);
		Value = (ml_value_t *)Macro;
	} else {
		mlc_expr_t *Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
		if (ml_parse(Compiler, MLT_COLON)) {
			ml_accept(Compiler, MLT_IDENT);
			const char *Ident = Compiler->Ident;
			ML_EXPR(CallExpr, parent, call);
			CallExpr->Child = Expr;
			ml_accept(Compiler, MLT_LEFT_PAREN);
			ml_accept_arguments(Compiler, MLT_RIGHT_PAREN, &Expr->Next);
			ml_parse(Compiler, MLT_SEMICOLON);
			Value = mlc_yield_compile_call(Compiler, ML_EXPR_END(CallExpr));
			ml_command_global(Compiler->Vars, MLT_LET, Ident, Value, NULL);
		} else {
			ml_parse(Compiler, MLT_SEMICOLON);
			Value = mlc_yield_compile_call(Compiler, Expr);
		}
	}
	return Value;
}

void ml_command_evaluate(ml_state_t *Caller, ml_compiler_t *Compiler) {
	Compiler->Base.Caller = Caller;
	Compiler->Base.Context = Caller->Context;
	Compiler->State = MLCS_COMMAND;
	return ml_compiler_run(Compiler, MLNil);
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
	ml_compiler_t *Compiler = ml_compiler(GlobalGet, Globals, ml_file_read, File);
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
	MainThread = GC_get_my_stackbottom(&MainStackBase);
	stringmap_insert(MLCompilerT->Exports, "EOI", MLEndOfInput);
	stringmap_insert(MLCompilerT->Exports, "NotFound", MLNotFound);
	stringmap_insert(StringFns, "r", ml_regex);
	stringmap_insert(StringFns, "ri", ml_regexi);
}

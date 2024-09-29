#include "minilang.h"
#include "ml_macros.h"
#include "ml_compiler2.h"
#include "stringmap.h"
#include "ml_runtime.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include "ml_compiler2.h"

#undef ML_CATEGORY
#define ML_CATEGORY "compiler"

struct mlc_upvalue_t {
	mlc_upvalue_t *Next;
	ml_decl_t *Decl;
	int Index, Line;
};

struct mlc_try_t {
	mlc_try_t *Up;
	ml_inst_t *Retries;
	int Top;
};

typedef struct mlc_token_t mlc_token_t;

struct mlc_token_t {
	mlc_token_t *Next;
	void *General;
	ml_source_t Source;
	ml_token_t Token;
};

typedef struct mlc_expected_delimiter_t mlc_expected_delimiter_t;

struct mlc_expected_delimiter_t {
	mlc_expected_delimiter_t *Prev;
	ml_token_t Token;
};

#ifdef ML_ASYNC_PARSER

#include "coro.h"

typedef struct ml_parser_coro_t ml_parser_coro_t;

struct ml_parser_coro_t {
	coro_context Context;
	ml_parser_coro_t *Next;
};

#ifdef ML_THREADSAFE

static ml_parser_coro_t * _Atomic CoroutineCache = NULL;

#else

static ml_parser_coro_t *CoroutineCache = NULL;

#endif

#endif

struct ml_parser_t {
	ml_type_t *Type;
	const char *Next;
	void *ReadData, *SpecialData, *EscapeData;
	const char *(*Read)(void *);
	ml_value_t *(*Escape)(void *);
	ml_value_t *(*Special)(void *);
	union {
		ml_value_t *Value;
		mlc_expr_t *Expr;
		const char *Ident;
	};
	ml_value_t *Warnings;
	mlc_expected_delimiter_t *ExpectedDelimiter;
	stringmap_t *EscapeFns;
	ml_source_t Source;
	int Line;
	jmp_buf OnError;
	ml_token_t Token;
#ifdef ML_ASYNC_PARSER
	ml_parser_coro_t *Coroutine;
#endif
};

struct ml_compiler_t {
	ml_type_t *Type;
	ml_getter_t GlobalGet;
	void *Globals;
	stringmap_t Vars[1];
};

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

extern ml_value_t *IndexMethod;
extern ml_value_t *SymbolMethod;

inline long ml_ident_hash(const char *Ident) {
	long Hash = 5381;
	while (*Ident) Hash = ((Hash << 5) + Hash) + *Ident++;
	return Hash;
}

static void mlc_function_run(mlc_function_t *Function, ml_value_t *Value) {
	if (ml_is_error(Value) && !Function->Frame->AllowErrors) {
		ml_state_t *Caller = Function->Base.Caller;
		ml_error_trace_add(Value, (ml_source_t){Function->Source, Function->Frame->Line});
		ML_RETURN(Value);
	}
	Function->Frame->run(Function, Value, Function->Frame->Data);
}

ML_TYPE(MLCompilerFunctionT, (MLStateT), "compiler-function");

static ml_source_t ML_TYPED_FN(ml_debugger_source, MLCompilerFunctionT, mlc_function_t *Function) {
	return (ml_source_t){Function->Source, Function->Frame ? Function->Frame->Line : 0};
}

#define FRAME_BLOCK_SIZE 2000

inline void mlc_return(mlc_function_t *Function, ml_value_t *Value) {
	mlc_frame_t *Frame = Function->Frame;
	return Frame->run(Function, Value, Frame->Data);
}

inline void mlc_pop(mlc_function_t *Function) {
	Function->Frame = Function->Frame->Next;
}

static void mlc_link_frame_run(mlc_function_t *Function, ml_value_t *Value, void **Limit) {
	Function->Limit = *Limit;
	Function->Frame = Function->Frame->Next;
	Function->Frame->run(Function, Value, Function->Frame->Data);
}

static __attribute__ ((noinline)) void *mlc_frame_alloc(mlc_function_t *Function, size_t Size, mlc_frame_fn run) {
	size_t FrameSize = sizeof(mlc_frame_t) + Size;
	FrameSize = (FrameSize + 7) & ~7;
	mlc_frame_t *Frame = (mlc_frame_t *)((void *)Function->Frame - FrameSize);
	if (!Function->Limit || (void *)Frame < Function->Limit) {
		size_t BlockSize = FrameSize + sizeof(void *);
		if (BlockSize < FRAME_BLOCK_SIZE) BlockSize = FRAME_BLOCK_SIZE;
		void *Limit = bnew(BlockSize);
		size_t LinkFrameSize = sizeof(mlc_frame_t) + sizeof(void *);
		mlc_frame_t *LinkFrame = (mlc_frame_t *)((Limit + BlockSize) - LinkFrameSize);
		LinkFrame->Next = Function->Frame;
		LinkFrame->run = (mlc_frame_fn)mlc_link_frame_run;
		LinkFrame->Data[0] = Function->Limit;
		Function->Limit = Limit;
		Frame = (mlc_frame_t *)((void *)LinkFrame - FrameSize);
		Frame->Next = LinkFrame;
	} else {
		Frame->Next = Function->Frame;
	}
	Frame->AllowErrors = 0;
	Frame->run = run;
	Function->Frame = Frame;
	return (void *)Frame->Data;
}

ml_inst_t *ml_inst_alloc(mlc_function_t *Function, int Line, ml_opcode_t Opcode, int N) {
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

inline void mlc_fix_links(ml_inst_t *Start, ml_inst_t *Target) {
	while (Start) {
		ml_inst_t *Next = Start->Inst;
		Start->Inst = Target;
		Start = Next;
	}
}

inline void mlc_inc_top(mlc_function_t *Function) {
	if (++Function->Top >= Function->Size) Function->Size = Function->Top + 1;
}

inline void mlc_compile(mlc_function_t *Function, const mlc_expr_t *Expr, int Flags) {
	Expr->compile(Function, Expr, Flags);
}

inline void mlc_expr_error(mlc_function_t *Function, const mlc_expr_t *Expr, ml_value_t *Error) {
	ml_error_trace_add(Error, (ml_source_t){Function->Source, Expr->StartLine});
	ML_CONTINUE(Function->Base.Caller, Error);
}

static mlc_local_t *mlc_local_new(const char *Ident, int Line) {
	mlc_local_t *Local = new(mlc_local_t);
	Local->Ident = Ident;
	Local->Line = Line;
	return Local;
}

typedef struct {
	const mlc_expr_t *Expr;
	ml_closure_info_t *Info;
} mlc_compile_frame_t;

static void mlc_expr_call2(mlc_function_t *Function, ml_value_t *Value, mlc_compile_frame_t *Frame) {
	ml_closure_info_t *Info = Frame->Info;
	const mlc_expr_t *Expr = Frame->Expr;
	ml_state_t *Caller = Function->Base.Caller;
	if (Function->UpValues) {
		ml_value_t *Error = ml_error("EvalError", "Use of non-constant value %s in constant expression", Function->UpValues->Decl->Ident);
		ml_error_trace_add(Error, (ml_source_t){Function->Source, Expr->EndLine});
		ML_RETURN(Error);
	}
	Info->Return = MLC_EMIT(Expr->EndLine, MLI_RETURN, 0);
	MLC_LINK(Function->Returns, Info->Return);
	Info->Halt = Function->Next;
	Info->Source = Function->Source;
	Info->StartLine = Expr->StartLine;
	Info->EndLine = Expr->EndLine;
	GC_asprintf((char **)&Info->Name, "@%s:%d", Info->Source, Info->StartLine);
	Info->FrameSize = Function->Size;
	Info->NumParams = 0;
	MLC_POP();
	return ml_call(Caller, ml_closure(Info), 0, NULL);
}

static void mlc_expr_call(mlc_function_t *Parent, const mlc_expr_t *Expr) {
	Parent->Frame->Line = Expr->EndLine;
	mlc_function_t *Function = new(mlc_function_t);
	Function->Base.Type = MLCompilerFunctionT;
	Function->Base.Caller = (ml_state_t *)Parent;
	Function->Base.Context = Parent->Base.Context;
	Function->Base.run = (ml_state_fn)mlc_function_run;
	Function->Compiler = Parent->Compiler;
	Function->Eval = 1;
	Function->Source = Parent->Source;
	Function->Old = -1;
	Function->It = -1;
	Function->Up = Parent;
	Function->Size = 1;
	Function->Next = anew(ml_inst_t, 128);
	Function->Space = 126;
	Function->Returns = NULL;
	MLC_FRAME(mlc_compile_frame_t, mlc_expr_call2);
	Frame->Expr = Expr;
	Frame->Info = new(ml_closure_info_t);
	Frame->Info->Entry = Function->Next;
	mlc_compile(Function, Expr, MLCF_RETURN);
}

ML_TYPE(MLExprGotoT, (), "expr::goto");
//!internal

ML_VALUE(MLExprGoto, MLExprGotoT);
//!internal

void ml_unknown_expr_compile(mlc_function_t *Function, const mlc_expr_t *Expr, int Flags) {
	MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Unknown expression cannot be compiled"));
}

void ml_register_expr_compile(mlc_function_t *Function, const mlc_expr_t *Expr, int Flags) {
	if (Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->StartLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_RETURN(NULL);
}

extern ml_value_t MLBlank[];

void ml_blank_expr_compile(mlc_function_t *Function, const mlc_expr_t *Expr, int Flags) {
	ml_inst_t *Inst = MLC_EMIT(Expr->StartLine, MLI_LOAD, 1);
	Inst[1].Value = MLBlank;
	if (Flags & MLCF_PUSH) {
		Inst->Opcode = MLI_LOAD_PUSH;
		mlc_inc_top(Function);
	}
	MLC_RETURN(NULL);
}

void ml_nil_expr_compile(mlc_function_t *Function, const mlc_expr_t *Expr, int Flags) {
	ml_inst_t *Inst = MLC_EMIT(Expr->StartLine, MLI_NIL, 0);
	if (Flags & MLCF_PUSH) {
		Inst->Opcode = MLI_NIL_PUSH;
		mlc_inc_top(Function);
	}
	MLC_RETURN(NULL);
}

void ml_value_expr_compile(mlc_function_t *Function, mlc_value_expr_t *Expr, int Flags) {
	if (Flags & MLCF_CONSTANT) MLC_RETURN(Expr->Value);
	ml_inst_t *Inst = MLC_EMIT(Expr->StartLine, MLI_LOAD, 1);
	Inst[1].Value = Expr->Value;
	if (Flags & MLCF_PUSH) {
		Inst->Opcode = MLI_LOAD_PUSH;
		mlc_inc_top(Function);
	}
	MLC_RETURN(NULL);
}

typedef struct {
	mlc_if_expr_t *Expr;
	mlc_if_case_t *Case;
	ml_decl_t *Decls;
	ml_inst_t *Exits, *IfInst;
	int Flags, Goto;
} mlc_if_expr_frame_t;

static void ml_if_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_if_expr_frame_t *Frame);

static void ml_if_expr_compile4(mlc_function_t *Function, ml_value_t *Value, mlc_if_expr_frame_t *Frame) {
	mlc_if_expr_t *Expr = Frame->Expr;
	MLC_LINK(Frame->Exits, Function->Next);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	int Goto = Frame->Goto && (Value == MLExprGoto);
	MLC_POP();
	MLC_RETURN(Goto ? MLExprGoto : NULL);
}

static void ml_if_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_if_expr_frame_t *Frame) {
	mlc_if_case_t *Case = Frame->Case;
	if (Case->Local->Ident) {
		Function->Decls = Frame->Decls;
		ml_inst_t *ExitInst = MLC_EMIT(Case->Body->EndLine, MLI_EXIT, 2);
		if (Case->Local->Index) {
			Function->Top -= Case->Local->Index;
			ExitInst[1].Count = Case->Local->Index;
		} else {
			--Function->Top;
			ExitInst[1].Count = 1;
		}
		ExitInst[2].Decls = Function->Decls;
	}
	mlc_if_expr_t *Expr = Frame->Expr;
	if (Case->Next || Expr->Else) {
		if (Value != MLExprGoto) {
			Frame->Goto = 0;
			ml_inst_t *GotoInst = MLC_EMIT(Case->Body->EndLine, MLI_GOTO, 1);
			GotoInst[1].Inst = Frame->Exits;
			Frame->Exits = GotoInst + 1;
		}
	}
	Frame->IfInst[1].Inst = Function->Next;
	if (Case->Next) {
		Frame->Case = Case = Case->Next;
		Function->Frame->run = (mlc_frame_fn)ml_if_expr_compile2;
		return mlc_compile(Function, Case->Condition, 0);
	}
	if (Expr->Else) {
		Function->Frame->run = (mlc_frame_fn)ml_if_expr_compile4;
		return mlc_compile(Function, Expr->Else, Frame->Flags & MLCF_RETURN);
	}
	MLC_LINK(Frame->Exits, Function->Next);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	int Goto = Frame->Goto;
	MLC_POP();
	MLC_RETURN(Goto ? MLExprGoto : NULL);
}

static void ml_if_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_if_expr_frame_t *Frame) {
	mlc_if_case_t *Case = Frame->Case;
	int CaseLine = Case->Condition->EndLine;
	Frame->IfInst = MLC_EMIT(CaseLine, MLI_AND, 1);
	if (Case->Local->Ident) {
		mlc_local_t *Local = Case->Local;
		do {
			ml_decl_t *Decl = new(ml_decl_t);
			Decl->Source.Name = Function->Source;
			Decl->Source.Line = Local->Line;
			Decl->Ident = Local->Ident;
			Decl->Hash = ml_ident_hash(Local->Ident);
			Decl->Index = Function->Top;
			mlc_inc_top(Function);
			Decl->Next = Function->Decls;
			Function->Decls = Decl;
			Local = Local->Next;
		} while (Local);
		ml_inst_t *EnterInst = MLC_EMIT(CaseLine, MLI_ENTER, 3);
		if (Case->Local->Index) {
			if (Case->Token == MLT_VAR) {
				EnterInst[1].Count = Case->Local->Index;
				EnterInst[2].Count = 0;
			} else {
				EnterInst[1].Count = 0;
				EnterInst[2].Count = Case->Local->Index;
			}
		} else {
			if (Case->Token == MLT_VAR) {
				EnterInst[1].Count = 1;
				EnterInst[2].Count = 0;
			} else {
				EnterInst[1].Count = 0;
				EnterInst[2].Count = 1;
			}
		}
		EnterInst[3].Decls = Function->Decls;
		if (Case->Local->Index) {
			ml_inst_t *Inst = MLC_EMIT(CaseLine, Case->Token == MLT_VAR ? MLI_VARX : MLI_LETX, 2);
			Inst[1].Count = -Case->Local->Index;
			Inst[2].Count = Case->Local->Index;
		} else {
			ml_inst_t *Inst = MLC_EMIT(CaseLine, Case->Token == MLT_VAR ? MLI_VAR : MLI_LET, 1);
			Inst[1].Count = -1;
		}
	}
	Function->Frame->run = (mlc_frame_fn)ml_if_expr_compile3;
	return mlc_compile(Function, Case->Body, Frame->Flags & MLCF_RETURN);
}

void ml_if_expr_compile(mlc_function_t *Function, mlc_if_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_if_expr_frame_t, ml_if_expr_compile2);
	Frame->Expr = Expr;
	Frame->Decls = Function->Decls;
	Frame->Flags = Flags;
	Frame->Goto = !!Expr->Else;
	Frame->Exits = NULL;
	Frame->IfInst = NULL;
	mlc_if_case_t *Case = Frame->Case = Expr->Cases;
	return mlc_compile(Function, Case->Condition, 0);
}

typedef struct {
	mlc_parent_expr_t *Expr;
	mlc_expr_t *Child;
	int Flags, Count;
} mlc_parent_expr_frame_t;

typedef struct {
	mlc_parent_expr_t *Expr;
	mlc_expr_t *Child;
	ml_inst_t *Exits;
	int Flags;
} mlc_link_expr_frame_t;

static void ml_or_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_link_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	if (Child->Next) {
		ml_inst_t *AndInst = MLC_EMIT(Child->EndLine, MLI_OR, 1);
		Frame->Child = Child = Child->Next;
		AndInst[1].Inst = Frame->Exits;
		Frame->Exits = AndInst + 1;
		return mlc_compile(Function, Child, 0);
	}
	MLC_LINK(Frame->Exits, Function->Next);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Child->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_or_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_link_expr_frame_t, ml_or_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Frame->Exits = NULL;
	mlc_expr_t *Child = Frame->Child = Expr->Child;
	return mlc_compile(Function, Child, 0);
}

static void ml_and_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_link_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	if (Child->Next) {
		ml_inst_t *AndInst = MLC_EMIT(Child->EndLine, MLI_AND, 1);
		Frame->Child = Child = Child->Next;
		AndInst[1].Inst = Frame->Exits;
		Frame->Exits = AndInst + 1;
		return mlc_compile(Function, Child, 0);
	}
	MLC_LINK(Frame->Exits, Function->Next);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Child->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_and_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_link_expr_frame_t, ml_and_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Frame->Exits = NULL;
	mlc_expr_t *Child = Frame->Child = Expr->Child;
	return mlc_compile(Function, Child, 0);
}

static void ml_debug_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_link_expr_frame_t *Frame) {
	Frame->Exits[1].Inst = Function->Next;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Frame->Expr->EndLine, MLI_NIL_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_debug_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_link_expr_frame_t, ml_debug_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Frame->Exits = MLC_EMIT(Expr->StartLine, MLI_IF_CONFIG, 1);
	return mlc_compile(Function, Expr->Child, 0);
}

typedef struct {
	mlc_if_config_expr_t *Expr;
	mlc_expr_t *Child;
	ml_inst_t *Exits;
	int Flags;
} mlc_config_expr_frame_t;

static void ml_if_config_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_config_expr_frame_t *Frame) {
	Frame->Exits[1].Inst = Function->Next;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Frame->Expr->EndLine, MLI_NIL_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_if_config_expr_compile(mlc_function_t *Function, mlc_if_config_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_config_expr_frame_t, ml_if_config_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	ml_inst_t *ConfigInst = Frame->Exits = MLC_EMIT(Expr->StartLine, MLI_IF_CONFIG, 2);
	ml_config_fn Fn = ml_config_lookup(Expr->Config);
	if (!Fn) MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Unknown config name %s", Expr->Config));
	ConfigInst[2].Data = Fn;
	return mlc_compile(Function, Expr->Child, 0);
}

static void ml_not_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	mlc_parent_expr_t *Expr = Frame->Expr;
	MLC_EMIT(Expr->EndLine, MLI_NOT, 0);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Frame->Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_not_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_parent_expr_frame_t, ml_not_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, 0);
}

typedef struct {
	mlc_parent_expr_t *Expr;
	mlc_expr_t *Child;
	ml_inst_t *Exits;
	ml_inst_t **Insts;
	int Flags;
} mlc_switch_expr_frame_t;

static void ml_switch_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_switch_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	if (Child->Next) {
		ml_inst_t *GotoInst = MLC_EMIT(Child->EndLine, MLI_GOTO, 1);
		Frame->Child = Child = Child->Next;
		GotoInst[1].Inst = Frame->Exits;
		Frame->Exits = GotoInst + 1;
		*Frame->Insts++ = Function->Next;
		return mlc_compile(Function, Child, Frame->Flags & MLCF_RETURN);
	}
	MLC_LINK(Frame->Exits, Function->Next);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Child->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_switch_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_switch_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	int Count = 0;
	for (mlc_expr_t *Next = Child->Next; Next; Next = Next->Next) ++Count;
	ml_inst_t *SwitchInst = MLC_EMIT(Child->EndLine, MLI_SWITCH, 2);
	SwitchInst[1].Count = Count;
	Frame->Insts = SwitchInst[2].Insts = anew(ml_inst_t *, Count);
	Frame->Child = Child = Child->Next;
	Function->Frame->run = (mlc_frame_fn)ml_switch_expr_compile3;
	*Frame->Insts++ = Function->Next;
	return mlc_compile(Function, Child, Frame->Flags & MLCF_RETURN);
}

void ml_switch_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_switch_expr_frame_t, ml_switch_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Frame->Exits = NULL;
	mlc_expr_t *Child = Frame->Child = Expr->Child;
	return mlc_compile(Function, Child, 0);
}

struct mlc_must_t {
	mlc_must_t *Next;
	mlc_expr_t *Expr;
	ml_decl_t *Decls;
};

typedef struct {
	mlc_must_t *Must, *End;
	ml_decl_t *Decls;
	int Line;
} mlc_must_frame_t;

static void ml_must_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_must_frame_t *Frame) {
	mlc_must_t *Must = Frame->Must;
	if (Must != Frame->End) {
		Frame->Must = Must->Next;
		Frame->Line = Must->Expr->EndLine;
		Function->Decls = Must->Decls;
		return mlc_compile(Function, Must->Expr, 0);
	}
	MLC_EMIT(Frame->Line, MLI_POP, 0);
	--Function->Top;
	Function->Decls = Frame->Decls;
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_must_compile(mlc_function_t *Function, mlc_must_t *Must, mlc_must_t *End) {
	MLC_FRAME(mlc_must_frame_t, ml_must_compile2);
	Frame->Decls = Function->Decls;
	Frame->Must = Must->Next;
	Frame->End = End;
	Frame->Line = Must->Expr->EndLine;
	MLC_EMIT(Must->Expr->StartLine, MLI_PUSH, 0);
	mlc_inc_top(Function);
	Function->Decls = Must->Decls;
	return mlc_compile(Function, Must->Expr, 0);
}

struct mlc_loop_t {
	mlc_loop_t *Up;
	const char *Name;
	mlc_try_t *Try;
	mlc_must_t *Must;
	ml_decl_t *Decls;
	ml_inst_t *Nexts, *Exits;
	int NextTop, ExitTop;
};

typedef struct {
	mlc_parent_expr_t *Expr;
	ml_inst_t *Next;
	mlc_loop_t Loop[1];
	int Flags;
} mlc_loop_frame_t;

static void ml_loop_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_loop_frame_t *Frame) {
	mlc_parent_expr_t *Expr = Frame->Expr;
	MLC_LINK(Frame->Loop->Nexts, Frame->Next);
	if (Value != MLExprGoto) {
		ml_inst_t *GotoInst = MLC_EMIT(Expr->EndLine, MLI_GOTO, 1);
		GotoInst[1].Inst = Frame->Next;
	}
	MLC_LINK(Frame->Loop->Exits, Function->Next);
	Function->Loop = Frame->Loop->Up;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_loop_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_loop_frame_t, ml_loop_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Frame->Loop->Up = Function->Loop;
	Frame->Loop->Name = Expr->Name;
	Frame->Loop->Try = Function->Try;
	Frame->Loop->Must = Function->Must;
	Frame->Loop->Decls = Function->Decls;
	Frame->Loop->Exits = NULL;
	Frame->Loop->Nexts = NULL;
	Frame->Loop->ExitTop = Function->Top;
	Frame->Loop->NextTop = Function->Top;
	Function->Loop = Frame->Loop;
	Frame->Next = Function->Next;
	return mlc_compile(Function, Expr->Child, 0);
}

static void ml_next_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_t **Frame) {
	mlc_parent_expr_t *Expr = Frame[0];
	mlc_loop_t *Loop = Function->Loop;
	if (Expr->Name) {
		while (Loop) {
			if (Loop->Name && !strcmp(Loop->Name, Expr->Name)) break;
			Loop = Loop->Up;
		}
		if (!Loop) MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Exit not in loop named %s", Expr->Name));
	}
	if (Function->Top > Loop->NextTop) {
		ml_inst_t *ExitInst = MLC_EMIT(Expr->EndLine, MLI_EXIT, 2);
		ExitInst[1].Count = Function->Top - Loop->NextTop;
		ExitInst[2].Decls = Loop->Decls;
	}
	ml_inst_t *GotoInst = MLC_EMIT(Expr->EndLine, MLI_GOTO, 1);
	GotoInst[1].Inst = Loop->Nexts;
	Loop->Nexts = GotoInst + 1;
	MLC_POP();
	MLC_RETURN(MLExprGoto);
}

void ml_next_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	mlc_loop_t *Loop = Function->Loop;
	if (!Loop) MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Next not in loop"));
	if (Expr->Name) {
		while (Loop) {
			if (Loop->Name && !strcmp(Loop->Name, Expr->Name)) break;
			Loop = Loop->Up;
		}
		if (!Loop) MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Exit not in loop named %s", Expr->Name));
	}
	if (Function->Try != Loop->Try) {
		ml_inst_t *TryInst = MLC_EMIT(Expr->StartLine, MLI_TRY, 1);
		if (Loop->Try) {
			TryInst[1].Inst = Loop->Try->Retries;
			Loop->Try->Retries = TryInst + 1;
		} else {
			TryInst[1].Inst = Function->Returns;
			Function->Returns = TryInst + 1;
		}
	}
	if (Function->Must != Loop->Must) {
		MLC_FRAME(mlc_parent_expr_t *, ml_next_expr_compile2);
		Frame[0] = Expr;
		return ml_must_compile(Function, Function->Must, Loop->Must);
	}
	if (Function->Top > Loop->NextTop) {
		ml_inst_t *ExitInst = MLC_EMIT(Expr->EndLine, MLI_EXIT, 2);
		ExitInst[1].Count = Function->Top - Loop->NextTop;
		ExitInst[2].Decls = Loop->Decls;
	}
	ml_inst_t *GotoInst = MLC_EMIT(Expr->EndLine, MLI_GOTO, 1);
	GotoInst[1].Inst = Loop->Nexts;
	Loop->Nexts = GotoInst + 1;
	MLC_RETURN(MLExprGoto);
}

typedef struct {
	mlc_parent_expr_t *Expr;
	mlc_loop_t *OldLoop, *Target;
	mlc_try_t *OldTry;
	int Flags;
} mlc_exit_expr_frame_t;

static void ml_exit_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_exit_expr_frame_t *Frame) {
	mlc_parent_expr_t *Expr = Frame->Expr;
	mlc_loop_t *Target = Frame->Target;
	Function->Loop = Frame->OldLoop;
	Function->Try = Frame->OldTry;
	if (Function->Top > Target->ExitTop) {
		ml_inst_t *ExitInst = MLC_EMIT(Expr->EndLine, MLI_EXIT, 2);
		ExitInst[1].Count = Function->Top - Target->ExitTop;
		ExitInst[2].Decls = Target->Decls;
	}
	ml_inst_t *GotoInst = MLC_EMIT(Expr->EndLine, MLI_GOTO, 1);
	GotoInst[1].Inst = Target->Exits;
	Target->Exits = GotoInst + 1;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(MLExprGoto);
}

static void ml_exit_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_exit_expr_frame_t *Frame) {
	mlc_loop_t *Target = Frame->Target;
	if (Value == MLExprGoto) {
		Function->Loop = Frame->OldLoop;
		Function->Try = Frame->OldTry;
		MLC_POP();
		MLC_RETURN(Value);
	}
	if (Function->Must != Target->Must) {
		Function->Frame->run = (mlc_frame_fn)ml_exit_expr_compile3;
		return ml_must_compile(Function, Function->Must, Target->Must);
	} else {
		return ml_exit_expr_compile3(Function, Value, Frame);
	}
}

void ml_exit_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_exit_expr_frame_t, ml_exit_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	mlc_loop_t *Loop = Function->Loop;
	if (!Loop) MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Exit not in loop"));
	if (Expr->Name) {
		while (Loop) {
			if (Loop->Name && !strcmp(Loop->Name, Expr->Name)) break;
			Loop = Loop->Up;
		}
		if (!Loop) MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Exit not in loop named %s", Expr->Name));
	}
	if (Function->Try != Loop->Try) {
		ml_inst_t *TryInst = MLC_EMIT(Expr->StartLine, MLI_TRY, 1);
		if (Loop->Try) {
			TryInst[1].Inst = Loop->Try->Retries;
			Loop->Try->Retries = TryInst + 1;
		} else {
			TryInst[1].Inst = Function->Returns;
			Function->Returns = TryInst + 1;
		}
	}
	Frame->Target = Loop;
	Frame->OldLoop = Function->Loop;
	Frame->OldTry = Function->Try;
	Function->Loop = Loop->Up;
	Function->Try = Loop->Try;
	if (Expr->Child) {
		return mlc_compile(Function, Expr->Child, 0);
	} else {
		MLC_EMIT(Expr->StartLine, MLI_NIL, 0);
		return ml_exit_expr_compile2(Function, NULL, Frame);
	}
}

static void ml_return_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	mlc_parent_expr_t *Expr = Frame->Expr;
	MLC_EMIT(Expr->EndLine, MLI_RETURN, 0);
	MLC_POP();
	MLC_RETURN(MLExprGoto);
}

static void ml_return_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	if (Function->Must) {
		Function->Frame->run = (mlc_frame_fn)ml_return_expr_compile3;
		return ml_must_compile(Function, Function->Must, NULL);
	}
	if (Value != MLExprGoto) {
		mlc_parent_expr_t *Expr = Frame->Expr;
		MLC_EMIT(Expr->EndLine, MLI_RETURN, 0);
	}
	MLC_POP();
	MLC_RETURN(MLExprGoto);
}

void ml_return_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	if (Expr->Child) {
		MLC_FRAME(mlc_parent_expr_frame_t, ml_return_expr_compile2);
		Frame->Expr = Expr;
		Frame->Flags = Flags;
		return mlc_compile(Function, Expr->Child, Function->Try ? 0 : MLCF_RETURN);
	} else {
		MLC_EMIT(Expr->StartLine, MLI_NIL, 0);
		MLC_EMIT(Expr->EndLine, MLI_RETURN, 0);
		MLC_RETURN(MLExprGoto);
	}
}

static void ml_suspend_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	mlc_parent_expr_t *Expr = Frame->Expr;
	MLC_EMIT(Expr->EndLine, MLI_SUSPEND, 0);
	MLC_EMIT(Expr->EndLine, MLI_RESUME, 0);
	Function->Top -= 2;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_suspend_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	Function->Frame->run = (mlc_frame_fn)ml_suspend_expr_compile3;
	mlc_compile(Function, Frame->Child, MLCF_PUSH);
}

void ml_suspend_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_parent_expr_frame_t, ml_suspend_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	mlc_expr_t *Child = Expr->Child;
	if (!Child) {
		MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Empty susp expression"));
	}
	if (Child->Next) {
		Frame->Child = Child->Next;
		return mlc_compile(Function, Child, MLCF_PUSH);
	} else {
		Frame->Child = Child;
		MLC_EMIT(Expr->StartLine, MLI_NIL_PUSH, 0);
		mlc_inc_top(Function);
		return ml_suspend_expr_compile2(Function, NULL, Frame);
	}
}

typedef struct {
	mlc_local_expr_t *Expr;
	ml_decl_t *Decls;
	mlc_local_t *Local;
	mlc_expr_t *Child;
	int Flags, Top;
} mlc_with_expr_frame_t;

static void ml_with_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_with_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	ml_inst_t *ExitInst = MLC_EMIT(Expr->EndLine, MLI_EXIT, 2);
	ExitInst[1].Count = Function->Top - Frame->Top;
	ExitInst[2].Decls = Frame->Decls;
	Function->Decls = Frame->Decls;
	Function->Top = Frame->Top;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_with_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_with_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	mlc_local_t *Local = Frame->Local;
	int Count = Local->Index;
	if (Count) {
		int I = Count;
		do {
			ml_decl_t *Decl = new(ml_decl_t);
			Decl->Source.Name = Function->Source;
			Decl->Source.Line = Local->Line;
			Decl->Ident = Local->Ident;
			Decl->Hash = ml_ident_hash(Local->Ident);
			Decl->Index = Function->Top;
			mlc_inc_top(Function);
			Decl->Next = Function->Decls;
			Function->Decls = Decl;
			Local = Local->Next;
		} while (--I > 0);
		ml_inst_t *PushInst = MLC_EMIT(Child->EndLine, MLI_WITHX, 2);
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
		Local = Local->Next;
		ml_inst_t *PushInst = MLC_EMIT(Child->EndLine, MLI_WITH, 1);
		PushInst[1].Decls = Function->Decls;
	}
	Child = Frame->Child = Child->Next;
	if (Local) {
		Frame->Local = Local;
		return mlc_compile(Function, Child, 0);
	}
	Function->Frame->run = (mlc_frame_fn)ml_with_expr_compile3;
	return mlc_compile(Function, Child, 0);
}

void ml_with_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_with_expr_frame_t, ml_with_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Frame->Top = Function->Top;
	Frame->Decls = Function->Decls;
	mlc_expr_t *Child = Frame->Child = Expr->Child;
	Frame->Local = Expr->Local;
	return mlc_compile(Function, Child, 0);
}

typedef struct {
	mlc_for_expr_t *Expr;
	ml_inst_t *IterInst;
	mlc_loop_t Loop[1];
	int Flags;
} mlc_for_expr_frame_t;

static void ml_for_expr_compile4(mlc_function_t *Function, ml_value_t *Value, mlc_for_expr_frame_t *Frame) {
	mlc_for_expr_t *Expr = Frame->Expr;
	MLC_LINK(Frame->Loop->Exits, Function->Next);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_for_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_for_expr_frame_t *Frame) {
	mlc_for_expr_t *Expr = Frame->Expr;
	ml_inst_t *ExitInst = MLC_EMIT(Expr->EndLine, MLI_EXIT, 2);
	if (Expr->Unpack) {
		ExitInst[1].Count = Expr->Unpack + !!Expr->Key;
	} else {
		ExitInst[1].Count = 1 + !!Expr->Key;
	}
	ExitInst[2].Decls = Frame->Loop->Decls;
	ml_inst_t *NextInst = MLC_EMIT(Expr->StartLine, MLI_NEXT, 1);
	NextInst[1].Inst = Frame->IterInst;
	MLC_LINK(Frame->Loop->Nexts, NextInst);
	Frame->IterInst[1].Inst = Function->Next;
	Function->Loop = Frame->Loop->Up;
	Function->Top = Frame->Loop->ExitTop;
	Function->Decls = Frame->Loop->Decls;
	if (Expr->Body->Next) {
		Function->Frame->run = (mlc_frame_fn)ml_for_expr_compile4;
		return mlc_compile(Function, Expr->Body->Next, 0);
	}
	MLC_LINK(Frame->Loop->Exits, Function->Next);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_for_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_for_expr_frame_t *Frame) {
	mlc_for_expr_t *Expr = Frame->Expr;
	mlc_expr_t *Sequence = Expr->Sequence;
	MLC_EMIT(Sequence->EndLine, MLI_FOR, 0);
	Frame->IterInst = MLC_EMIT(Sequence->EndLine, MLI_ITER, 1);
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
		MLC_EMIT(Sequence->EndLine, MLI_KEY, 0);
		ml_inst_t *WithInst = MLC_EMIT(Sequence->EndLine, MLI_WITH, 1);
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
	MLC_EMIT(Sequence->EndLine, Expr->Key ? MLI_VALUE_2 : MLI_VALUE_1, 0);
	if (Expr->Unpack) {
		ml_inst_t *WithInst = MLC_EMIT(Sequence->EndLine, MLI_WITHX, 2);
		WithInst[1].Count = Expr->Unpack;
		WithInst[2].Decls = Function->Decls;
	} else {
		ml_inst_t *WithInst = MLC_EMIT(Sequence->EndLine, MLI_WITH, 1);
		WithInst[1].Decls = Function->Decls;
	}
	Frame->Loop->Up = Function->Loop;
	Frame->Loop->Name = Expr->Name;
	Frame->Loop->Try = Function->Try;
	Frame->Loop->Must = Function->Must;
	Frame->Loop->Exits = NULL;
	Frame->Loop->Nexts = NULL;
	if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	Function->Loop = Frame->Loop;
	Function->Frame->run = (mlc_frame_fn)ml_for_expr_compile3;
	return mlc_compile(Function, Expr->Body, 0);
}

void ml_for_expr_compile(mlc_function_t *Function, mlc_for_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_for_expr_frame_t, ml_for_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Frame->Loop->ExitTop = Function->Top;
	Frame->Loop->NextTop = Function->Top + 1;
	Frame->Loop->Decls = Function->Decls;
	return mlc_compile(Function, Expr->Sequence, 0);
}

static void ml_each_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	mlc_parent_expr_t *Expr = Frame->Expr;
	mlc_expr_t *Child = Expr->Child;
	MLC_EMIT(Child->EndLine, MLI_FOR, 0);
	ml_inst_t *AndInst = MLC_EMIT(Child->EndLine, MLI_ITER, 1);
	mlc_inc_top(Function);
	MLC_EMIT(Child->EndLine, MLI_VALUE_1, 0);
	ml_inst_t *NextInst = MLC_EMIT(Expr->StartLine, MLI_NEXT, 1);
	NextInst[1].Inst = AndInst;
	AndInst[1].Inst = Function->Next;
	--Function->Top;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_each_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_parent_expr_frame_t, ml_each_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, 0);
}

struct mlc_block_t {
	mlc_block_t *Up;
	ml_decl_t *OldDecls;
	mlc_block_expr_t *Expr;
	mlc_expr_t *Child;
	ml_inst_t *Exits;
	inthash_t DeclHashes;
	mlc_try_t Try;
	mlc_must_t Must, *OldMust;
	int Flags, Size, Top;
	ml_decl_t *Decls[];
};

typedef struct {
	mlc_local_expr_t *Expr;
	int Flags;
} mlc_local_expr_frame_t;

static void ml_var_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_decl_t *Decl = Function->Block->Decls[Local->Index];
	if (Value) {
		ml_inst_t *VarInst = MLC_EMIT(Expr->EndLine, MLI_LOAD_VAR, 2);
		VarInst[1].Value = Value;
		VarInst[2].Count = Function->Block->Top + Local->Index - Function->Top;
	} else {
		ml_inst_t *VarInst = MLC_EMIT(Expr->EndLine, MLI_VAR, 1);
		VarInst[1].Count = Function->Block->Top + Local->Index - Function->Top;
	}
	Decl->Flags = 0;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_var_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_var_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, MLCF_CONSTANT);
}

static void ml_var_type_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_inst_t *TypeInst = MLC_EMIT(Expr->EndLine, MLI_VAR_TYPE, 1);
	TypeInst[1].Count = Function->Block->Top + Local->Index - Function->Top;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_var_type_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_var_type_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, 0);
}

static void ml_var_in_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_decl_t **Decls = Function->Block->Decls + Local->Index;
	for (int I = 0; I < Expr->Count; ++I, Local = Local->Next) {
		ml_inst_t *PushInst = MLC_EMIT(Expr->EndLine, MLI_LOCAL_PUSH, 1);
		PushInst[1].Count = -1;
		mlc_inc_top(Function);
		ml_inst_t *ValueInst = MLC_EMIT(Expr->EndLine, MLI_LOAD_PUSH, 1);
		ValueInst[1].Value = ml_string(Local->Ident, -1);
		mlc_inc_top(Function);
		ml_inst_t *CallInst = MLC_EMIT(Expr->EndLine, MLI_CALL_CONST, 2);
		CallInst[1].Value = SymbolMethod;
		CallInst[2].Count = 2;
		Function->Top -= 2;
		ml_decl_t *Decl = Decls[I];
		ml_inst_t *VarInst = MLC_EMIT(Expr->EndLine, MLI_VAR, 1);
		VarInst[1].Count = Function->Block->Top + Local->Index - Function->Top;
		Decl->Flags = 0;
	}
	if (!(Frame->Flags & MLCF_PUSH)) {
		MLC_EMIT(Expr->EndLine, MLI_POP, 0);
		--Function->Top;
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_var_in_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_var_in_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, MLCF_PUSH);
}

static void ml_var_unpack_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_inst_t *VarInst = MLC_EMIT(Expr->EndLine, MLI_VARX, 2);
	VarInst[1].Count = Function->Block->Top + Local->Index - Function->Top;
	VarInst[2].Count = Expr->Count;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_var_unpack_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_var_unpack_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, 0);
}

static void ml_let_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_decl_t *Decl = Function->Block->Decls[Local->Index];
	ml_inst_t *LetInst;
	if (Decl->Flags & MLC_DECL_BACKFILL) {
		LetInst = MLC_EMIT(Expr->EndLine, MLI_LETI, 1);
	} else {
		LetInst = MLC_EMIT(Expr->EndLine, MLI_LET, 1);
	}
	LetInst[1].Count = Function->Block->Top + Local->Index - Function->Top;
	Decl->Flags = 0;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_let_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_let_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, 0);
}

static void ml_let_in_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_decl_t **Decls = Function->Block->Decls + Local->Index;
	for (int I = 0; I < Expr->Count; ++I, Local = Local->Next) {
		ml_inst_t *PushInst = MLC_EMIT(Expr->EndLine, MLI_LOCAL_PUSH, 1);
		PushInst[1].Count = -1;
		mlc_inc_top(Function);
		ml_inst_t *ValueInst = MLC_EMIT(Expr->EndLine, MLI_LOAD_PUSH, 1);
		ValueInst[1].Value = ml_string(Local->Ident, -1);
		mlc_inc_top(Function);
		ml_inst_t *CallInst = MLC_EMIT(Expr->EndLine, MLI_CALL_CONST, 2);
		CallInst[1].Value = SymbolMethod;
		CallInst[2].Count = 2;
		Function->Top -= 2;
		ml_decl_t *Decl = Decls[I];
		ml_inst_t *LetInst;
		if (Decl->Flags & MLC_DECL_BACKFILL) {
			LetInst = MLC_EMIT(Expr->EndLine, MLI_LETI, 1);
		} else {
			LetInst = MLC_EMIT(Expr->EndLine, MLI_LET, 1);
		}
		LetInst[1].Count = Function->Block->Top + Local->Index - Function->Top;
		Decl->Flags = 0;
	}
	if (!(Frame->Flags & MLCF_PUSH)) {
		MLC_EMIT(Expr->EndLine, MLI_POP, 0);
		--Function->Top;
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_let_in_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_let_in_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, MLCF_PUSH);
}

static void ml_let_unpack_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_decl_t **Decls = Function->Block->Decls + Local->Index;
	ml_inst_t *LetInst = MLC_EMIT(Expr->EndLine, MLI_LETX, 2);
	LetInst[1].Count = Function->Block->Top + Local->Index - Function->Top;
	LetInst[2].Count = Expr->Count;
	for (int I = 0; I < Expr->Count; ++I) Decls[I]->Flags = 0;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_let_unpack_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_let_unpack_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, 0);
}

static void ml_ref_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_decl_t *Decl = Function->Block->Decls[Local->Index];
	ml_inst_t *LetInst;
	if (Decl->Flags & MLC_DECL_BACKFILL) {
		LetInst = MLC_EMIT(Expr->EndLine, MLI_REFI, 1);
	} else {
		LetInst = MLC_EMIT(Expr->EndLine, MLI_REF, 1);
	}
	LetInst[1].Count = Function->Block->Top + Local->Index - Function->Top;
	Decl->Flags = 0;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_ref_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_ref_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, 0);
}

static void ml_ref_in_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_decl_t **Decls = Function->Block->Decls + Local->Index;
	for (int I = 0; I < Expr->Count; ++I, Local = Local->Next) {
		ml_inst_t *PushInst = MLC_EMIT(Expr->EndLine, MLI_LOCAL_PUSH, 1);
		PushInst[1].Count = -1;
		mlc_inc_top(Function);
		ml_inst_t *ValueInst = MLC_EMIT(Expr->EndLine, MLI_LOAD_PUSH, 1);
		ValueInst[1].Value = ml_string(Local->Ident, -1);
		mlc_inc_top(Function);
		ml_inst_t *CallInst = MLC_EMIT(Expr->EndLine, MLI_CALL_CONST, 2);
		CallInst[1].Value = SymbolMethod;
		CallInst[2].Count = 2;
		Function->Top -= 2;
		ml_decl_t *Decl = Decls[I];
		ml_inst_t *LetInst;
		if (Decl->Flags & MLC_DECL_BACKFILL) {
			LetInst = MLC_EMIT(Expr->EndLine, MLI_REFI, 1);
		} else {
			LetInst = MLC_EMIT(Expr->EndLine, MLI_REF, 1);
		}
		LetInst[1].Count = Function->Block->Top + Local->Index - Function->Top;
		Decl->Flags = 0;
	}
	if (!(Frame->Flags & MLCF_PUSH)) {
		MLC_EMIT(Expr->EndLine, MLI_POP, 0);
		--Function->Top;
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_ref_in_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_ref_in_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, MLCF_PUSH);
}

static void ml_ref_unpack_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_decl_t **Decls = Function->Block->Decls + Local->Index;
	ml_inst_t *LetInst = MLC_EMIT(Expr->EndLine, MLI_REFX, 2);
	LetInst[1].Count = Function->Block->Top + Local->Index - Function->Top;
	LetInst[2].Count = Expr->Count;
	for (int I = 0; I < Expr->Count; ++I) Decls[I]->Flags = 0;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_ref_unpack_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_ref_unpack_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, 0);
}

static void ml_def_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	if (ml_is_error(Value)) {
		ml_state_t *Caller = Function->Base.Caller;
		ML_RETURN(Value);
	}
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_decl_t *Decl = Function->Block->Decls[Local->Index];
	if (Decl->Value) ml_uninitialized_set(Decl->Value, Value);
	Decl->Value = Value;
	ml_value_set_name(Value, Local->Ident);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_NIL_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_def_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_def_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_expr_call(Function, Expr->Child);
}

typedef struct {
	mlc_local_expr_t *Expr;
	mlc_local_t *Local;
	ml_value_t *Args[2];
	ml_decl_t **Decls;
	int Flags, Index;
} mlc_def_in_expr_frame_t;

static void ml_def_in_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_def_in_expr_frame_t *Frame) {
	if (ml_is_error(Value)) {
		ml_state_t *Caller = Function->Base.Caller;
		ML_RETURN(Value);
	}
	mlc_local_expr_t *Expr = Frame->Expr;
	int Index = Frame->Index;
	mlc_local_t *Local = Frame->Local;
	ml_decl_t *Decl = Frame->Decls[Index];
	if (Decl->Value) ml_uninitialized_set(Decl->Value, Value);
	Decl->Value = Value;
	if (++Index < Expr->Count) {
		Frame->Index = Index;
		Local = Frame->Local = Local->Next;
		Frame->Args[1] = ml_string(Local->Ident, -1);
		return ml_call(Function, SymbolMethod, 2, Frame->Args);
	}
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_NIL_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_def_in_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_def_in_expr_frame_t *Frame) {
	if (ml_is_error(Value)) {
		ml_state_t *Caller = Function->Base.Caller;
		ML_RETURN(Value);
	}
	mlc_local_expr_t *Expr = Frame->Expr;
	Frame->Args[0] = Value;
	mlc_local_t *Local = Frame->Local = Expr->Local;
	Frame->Index = 0;
	Frame->Decls = Function->Block->Decls + Local->Index;
	Frame->Args[1] = ml_string(Local->Ident, -1);
	Function->Frame->run = (mlc_frame_fn)ml_def_in_expr_compile3;
	return ml_call(Function, SymbolMethod, 2, Frame->Args);
}

void ml_def_in_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_def_in_expr_frame_t, ml_def_in_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_expr_call(Function, Expr->Child);
}

static void ml_def_unpack_expr_compile2(mlc_function_t *Function, ml_value_t *Packed, mlc_local_expr_frame_t *Frame) {
	if (ml_is_error(Packed)) {
		ml_state_t *Caller = Function->Base.Caller;
		ML_RETURN(Packed);
	}
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_decl_t **Decls = Function->Block->Decls + Local->Index;
	for (int I = 0; I < Expr->Count; ++I) {
		ml_value_t *Value = ml_unpack(Packed, I + 1);
		ml_decl_t *Decl = Decls[I];
		if (Decl->Value) ml_uninitialized_set(Decl->Value, Value);
		Decl->Value = Value;
	}
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_NIL_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_def_unpack_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_def_unpack_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_expr_call(Function, Expr->Child);
}

static void ml_block_expr_compile5(mlc_function_t *Function, ml_value_t *Value, mlc_block_t *Frame) {
	mlc_block_expr_t *Expr = Frame->Expr;
	MLC_EMIT(Expr->EndLine, MLI_RETRY, 0);
	MLC_LINK(Frame->Exits, Function->Next);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_block_expr_compile4(mlc_function_t *Function, ml_value_t *Value, mlc_block_t *Frame) {
	mlc_block_expr_t *Expr = Frame->Expr;
	Function->Must = Frame->OldMust;
	Frame->Exits = NULL;
	Function->Try = Function->Try->Up;
	ml_inst_t *TryInst = MLC_EMIT(Expr->EndLine, MLI_TRY, 1);
	if (Function->Try) {
		TryInst[1].Inst = Function->Try->Retries;
		Function->Try->Retries = TryInst + 1;
	} else {
		TryInst[1].Inst = Function->Returns;
		Function->Returns = TryInst + 1;
	}
	ml_inst_t *GotoInst = MLC_EMIT(Expr->EndLine, MLI_GOTO, 1);
	GotoInst[1].Inst = Frame->Exits;
	Frame->Exits = GotoInst + 1;
	MLC_LINK(Frame->Try.Retries, Function->Next);
	ml_inst_t *PopInst = MLC_EMIT(Expr->Must->StartLine, MLI_CATCHX, 3);
	if (Function->Try) {
		PopInst[1].Inst = Function->Try->Retries;
		Function->Try->Retries = PopInst + 1;
	} else {
		PopInst[1].Inst = Function->Returns;
		Function->Returns = PopInst + 1;
	}
	PopInst[2].Count = Frame->Top;
	PopInst[3].Decls = Function->Decls;
	Function->Frame->run = (mlc_frame_fn)ml_block_expr_compile5;
	return ml_must_compile(Function, &Frame->Must, Frame->OldMust);
}

static void ml_block_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_block_t *Frame) {
	mlc_block_expr_t *Expr = Frame->Expr;
	ml_inst_t *ExitInst = MLC_EMIT(Expr->CatchBody->EndLine, MLI_EXIT, 2);
	ExitInst[1].Count = 1;
	ExitInst[2].Decls = Frame->OldDecls;
	Function->Decls = Frame->OldDecls;
	Function->Top = Frame->Top;
	ml_inst_t *GotoInst = MLC_EMIT(Expr->CatchBody->EndLine, MLI_GOTO, 1);
	GotoInst[1].Inst = Frame->Exits;
	Frame->Exits = GotoInst + 1;
	MLC_LINK(Frame->Exits, Function->Next);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_block_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_block_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	if (Child) {
		Frame->Child = Child->Next;
		if ((Frame->Flags & MLCF_RETURN) && !Child->Next && !Function->Try) {
			return mlc_compile(Function, Child, MLCF_RETURN);
		} else {
			return mlc_compile(Function, Child, 0);
		}
	}
	mlc_block_expr_t *Expr = Frame->Expr;
	if (Expr->NumVars + Expr->NumLets) {
		ml_inst_t *ExitInst = MLC_EMIT(Expr->EndLine, MLI_EXIT, 2);
		ExitInst[1].Count = Expr->NumVars + Expr->NumLets;
		ExitInst[2].Decls = Frame->OldDecls;
	}
	Function->Decls = Frame->OldDecls;
	Function->Block = Frame->Up;
	Function->Top = Frame->Top;
	if (Expr->CatchBody) {
		Frame->Exits = NULL;
		Function->Try = Function->Try->Up;
		ml_inst_t *TryInst = MLC_EMIT(Expr->EndLine, MLI_TRY, 1);
		if (Function->Try) {
			TryInst[1].Inst = Function->Try->Retries;
			Function->Try->Retries = TryInst + 1;
		} else {
			TryInst[1].Inst = Function->Returns;
			Function->Returns = TryInst + 1;
		}
		ml_inst_t *GotoInst = MLC_EMIT(Expr->EndLine, MLI_GOTO, 1);
		GotoInst[1].Inst = Frame->Exits;
		Frame->Exits = GotoInst + 1;
		MLC_LINK(Frame->Try.Retries, Function->Next);
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source.Name = Function->Source;
		Decl->Source.Line = Expr->CatchBody->StartLine;
		Decl->Ident = Expr->CatchIdent;
		Decl->Hash = ml_ident_hash(Expr->CatchIdent);
		Decl->Index = Function->Top;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
		mlc_inc_top(Function);
		ml_inst_t *CatchInst = MLC_EMIT(Expr->CatchBody->StartLine, MLI_CATCH, 3);
		if (Function->Try) {
			CatchInst[1].Inst = Function->Try->Retries;
			Function->Try->Retries = CatchInst + 1;
		} else {
			CatchInst[1].Inst = Function->Returns;
			Function->Returns = CatchInst + 1;
		}
		CatchInst[2].Count = Frame->Top;
		CatchInst[3].Decls = Function->Decls;
		Function->Frame->run = (mlc_frame_fn)ml_block_expr_compile3;
		return mlc_compile(Function, Expr->CatchBody, 0);
	} else if (Expr->Must) {
		Function->Frame->run = (mlc_frame_fn)ml_block_expr_compile4;
		return ml_must_compile(Function, &Frame->Must, Frame->OldMust);
	}
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(Value);
}

void ml_block_expr_compile(mlc_function_t *Function, mlc_block_expr_t *Expr, int Flags) {
	int NumDecls = Expr->NumVars + Expr->NumLets + Expr->NumDefs;
	MLC_XFRAME(mlc_block_t, NumDecls, ml_decl_t *, ml_block_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Frame->Top = Function->Top;
	Frame->OldDecls = Function->Decls;
	if (Expr->CatchBody) {
		ml_inst_t *TryInst = MLC_EMIT(Expr->StartLine, MLI_TRY, 1);
		TryInst[1].Inst = NULL;
		Frame->Try.Up = Function->Try;
		Frame->Try.Retries = TryInst + 1;
		Frame->Try.Top = Function->Top;
		Function->Try = &Frame->Try;
	} else if (Expr->Must) {
		ml_inst_t *TryInst = MLC_EMIT(Expr->StartLine, MLI_TRY, 1);
		TryInst[1].Inst = NULL;
		Frame->Try.Up = Function->Try;
		Frame->Try.Retries = TryInst + 1;
		Frame->Try.Top = Function->Top;
		Function->Try = &Frame->Try;
		Frame->OldMust = Function->Must;
		Frame->Must.Next = Function->Must;
		Frame->Must.Expr = Expr->Must;
		Frame->Must.Decls = Function->Decls;
		Function->Must = &Frame->Must;
	}
	int Top = Function->Top;
	ml_decl_t *Last = Function->Decls, *Decls = Last;
	Frame->DeclHashes = (inthash_t)INTHASH_INIT;
	inthash_t *DeclHashes = &Frame->DeclHashes;
	Frame->Up = Function->Block;
	Function->Block = Frame;
	for (mlc_local_t *Local = Expr->Vars; Local; Local = Local->Next) {
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source.Name = Function->Source;
		Decl->Source.Line = Local->Line;
		Decl->Ident = Local->Ident;
		Decl->Hash = ml_ident_hash(Local->Ident);
		Decl->Index = Top++;
		Frame->Decls[Local->Index] = Decl;
		if (Local->Ident[0] && inthash_insert(DeclHashes, (uintptr_t)Decl->Hash, Decl)) {
			for (ml_decl_t *Prev = Decls; Prev != Last; Prev = Prev->Next) {
				if (!strcmp(Prev->Ident, Decl->Ident)) {
					MLC_EXPR_ERROR(Expr, ml_error("NameError", "Identifier %s redefined in line %d, previously declared on line %d", Decl->Ident, Decl->Source.Line, Prev->Source.Line));
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
		Frame->Decls[Local->Index] = Decl;
		if (Local->Ident[0] && inthash_insert(DeclHashes, (uintptr_t)Decl->Hash, Decl)) {
			for (ml_decl_t *Prev = Decls; Prev != Last; Prev = Prev->Next) {
				if (!strcmp(Prev->Ident, Decl->Ident)) {
					MLC_EXPR_ERROR(Expr, ml_error("NameError", "Identifier %s redefined in line %d, previously declared on line %d", Decl->Ident, Decl->Source.Line, Prev->Source.Line));
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
		Frame->Decls[Local->Index] = Decl;
		if (Local->Ident[0] && inthash_insert(DeclHashes, (uintptr_t)Decl->Hash, Decl)) {
			for (ml_decl_t *Prev = Decls; Prev != Last; Prev = Prev->Next) {
				if (!strcmp(Prev->Ident, Decl->Ident)) {
					MLC_EXPR_ERROR(Expr, ml_error("NameError", "Identifier %s redefined in line %d, previously declared on line %d", Decl->Ident, Decl->Source.Line, Prev->Source.Line));
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
		ml_inst_t *EnterInst = MLC_EMIT(Expr->StartLine, MLI_ENTER, 3);
		EnterInst[1].Count = Expr->NumVars;
		EnterInst[2].Count = Expr->NumLets;
		EnterInst[3].Decls = Function->Decls;
	}
	mlc_expr_t *Child = Expr->Child;
	if (Child) {
		Frame->Child = Child->Next;
		if ((Frame->Flags & MLCF_RETURN) && !Child->Next && !Function->Try) {
			return mlc_compile(Function, Child, MLCF_RETURN);
		} else {
			return mlc_compile(Function, Child, 0);
		}
	} else {
		Frame->Child = NULL;
		MLC_EMIT(Expr->StartLine, MLI_NIL, 0);
		return ml_block_expr_compile2(Function, NULL, Frame);
	}
}

static void ml_assign_expr_compile4(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	mlc_parent_expr_t *Expr = Frame->Expr;
	MLC_EMIT(Expr->EndLine, MLI_ASSIGN, 0);
	--Function->Top;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	Function->Old = Frame->Count;
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_assign_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	mlc_parent_expr_t *Expr = Frame->Expr;
	ml_inst_t *AssignInst = MLC_EMIT(Expr->EndLine, MLI_ASSIGN_LOCAL, 1);
	AssignInst[1].Count = Function->Old - Function->Top;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	Function->Old = Frame->Count;
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_assign_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	Frame->Count = Function->Old;
	if (Value) {
		Function->Old = ml_integer_value_fast(Value);
		Function->Frame->run = (mlc_frame_fn)ml_assign_expr_compile3;
	} else {
		Function->Old = Function->Top - 1;
		Function->Frame->run = (mlc_frame_fn)ml_assign_expr_compile4;
	}
	return mlc_compile(Function, Frame->Child, 0);
}

void ml_assign_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_parent_expr_frame_t, ml_assign_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	mlc_expr_t *Child = Expr->Child;
	Frame->Child = Child->Next;
	return mlc_compile(Function, Child, MLCF_LOCAL | MLCF_PUSH);
}

void ml_old_expr_compile(mlc_function_t *Function, const mlc_expr_t *Expr, int Flags) {
	if (Function->Old < 0) MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Old must be used in assigment expression"));
	if (Flags & MLCF_PUSH) {
		ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_LOCAL_PUSH, 1);
		LocalInst[1].Count = Function->Old - Function->Top;
		mlc_inc_top(Function);
	} else {
		ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_LOCAL, 1);
		LocalInst[1].Count = Function->Old - Function->Top;
	}
	MLC_RETURN(NULL);
}

void ml_recur_expr_compile(mlc_function_t *Function, const mlc_expr_t *Expr, int Flags) {
	ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_UPVALUE, 1);
	LocalInst[1].Count = -1;
	if (Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->StartLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_RETURN(NULL);
}

void ml_it_expr_compile(mlc_function_t *Function, const mlc_expr_t *Expr, int Flags) {
	if (Function->It < 0) MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "It must be used in guard expression"));
	if (Flags & MLCF_PUSH) {
		ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_LOCAL_PUSH, 1);
		LocalInst[1].Count = Function->It - Function->Top;
		mlc_inc_top(Function);
	} else {
		ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_LOCAL, 1);
		LocalInst[1].Count = Function->It - Function->Top;
	}
	MLC_RETURN(NULL);
}

ML_TYPE(MLExprT, (), "expr");
//!macro
// An expression value used by the compiler to implement macros.

ml_value_t *ml_expr_value(mlc_expr_t *Expr) {
	Expr->Type = MLExprT;
	return (ml_value_t *)Expr;
}

ML_METHODX("$", MLExprT) {
	mlc_expr_t *Expr = (mlc_expr_t *)Args[0];
	mlc_function_t *Parent = NULL;
	for (ml_state_t *State = Caller; State; State = State->Caller) {
		if (State->Type == MLCompilerFunctionT) {
			Parent = (mlc_function_t *)State;
			break;
		}
	}
	if (!Parent) ML_ERROR("MacroError", "Expression has no function for evaluation");
	mlc_function_t *Function = new(mlc_function_t);
	Function->Base.Type = MLCompilerFunctionT;
	Function->Base.Caller = Caller;
	Function->Base.Context = Caller->Context;
	Function->Base.run = (ml_state_fn)mlc_function_run;
	Function->Compiler = Parent->Compiler;
	Function->Eval = 1;
	Function->Source = Parent->Source;
	Function->Old = -1;
	Function->It = -1;
	Function->Up = Parent;
	Function->Size = 1;
	Function->Next = anew(ml_inst_t, 128);
	Function->Space = 126;
	Function->Returns = NULL;
	MLC_FRAME(mlc_compile_frame_t, mlc_expr_call2);
	Frame->Expr = Expr;
	Frame->Info = new(ml_closure_info_t);
	Frame->Info->Entry = Function->Next;
	return mlc_compile(Function, Expr, 0);
}

void ml_delegate_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	return mlc_compile(Function, Expr->Child, Flags);
}

static mlc_expr_t *ml_delegate_expr(ml_value_t *Value) {
	mlc_expr_t *Child = (mlc_expr_t *)Value;
	mlc_parent_expr_t *Expr = new(mlc_parent_expr_t);
	Expr->compile = ml_delegate_expr_compile;
	Expr->Source = Child->Source;
	Expr->StartLine = Child->StartLine;
	Expr->EndLine = Child->EndLine;
	Expr->Child = Child;
	return (mlc_expr_t *)Expr;
}

ML_METHOD("source", MLExprT) {
	mlc_expr_t *Child = (mlc_expr_t *)Args[0];
	return ml_string(Child->Source, -1);
}

ML_METHOD("start", MLExprT) {
	mlc_expr_t *Child = (mlc_expr_t *)Args[0];
	return ml_integer(Child->StartLine);
}

ML_METHOD("end", MLExprT) {
	mlc_expr_t *Child = (mlc_expr_t *)Args[0];
	return ml_integer(Child->EndLine);
}

ML_FUNCTION(MLMacro) {
//!macro
//@macro
//<Function:function
//>macro
// Returns a new macro which applies :mini:`Function` when compiled.
// :mini:`Function` should have the following signature: :mini:`Function(Expr/1: expr, Expr/2: expr, ...): expr`.
	ML_CHECK_ARG_COUNT(1);
	ml_macro_t *Macro = new(ml_macro_t);
	Macro->Type = MLMacroT;
	Macro->Function = Args[0];
	return (ml_value_t *)Macro;
}

ML_TYPE(MLMacroT, (), "macro",
//!macro
// A macro.
	.Constructor = (ml_value_t *)MLMacro
);

ml_value_t *ml_macro(ml_value_t *Function) {
	ml_macro_t *Macro = new(ml_macro_t);
	Macro->Type = MLMacroT;
	Macro->Function = Function;
	return (ml_value_t *)Macro;
}

ml_value_t *ml_macrox(const mlc_expr_t *(*function)(const mlc_expr_t *, const mlc_expr_t *, void *), void *Data) {
	ml_macro_t *Macro = new(ml_macro_t);
	Macro->Type = MLMacroT;
	Macro->function = function;
	Macro->Data = Data;
	return (ml_value_t *)Macro;
}

typedef struct {
	const mlc_expr_t *Expr;
	int Flags;
} ml_inline_expr_frame_t;

static void ml_inline_expr_compile2(mlc_function_t *Function, ml_value_t *Value, ml_inline_expr_frame_t *Frame) {
	if (ml_is_error(Value)) {
		ml_state_t *Caller = Function->Base.Caller;
		ML_RETURN(Value);
	}
	const mlc_expr_t *Expr = Frame->Expr;
	int Flags = Frame->Flags;
	if (Flags & MLCF_CONSTANT) {
		MLC_POP();
		MLC_RETURN(Value);
	}
	ml_inst_t *ValueInst = MLC_EMIT(Expr->StartLine, MLI_LOAD, 1);
	if (ml_typeof(Value) == MLUninitializedT) {
		ml_uninitialized_use(Value, &ValueInst[1].Value);
	}
	ValueInst[1].Value = Value;
	if (Flags & MLCF_PUSH) {
		ValueInst->Opcode = MLI_LOAD_PUSH;
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_inline_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(ml_inline_expr_frame_t, ml_inline_expr_compile2);
	Frame->Expr = (mlc_expr_t *)Expr;
	Frame->Flags = Flags;
	mlc_expr_call(Function, Expr->Child);
}

typedef struct {
	mlc_expr_t *Expr;
	mlc_expr_t *Child;
	ml_value_t *Value;
	ml_inst_t *NilInst;
	int Count, Index, Flags;
} ml_call_expr_frame_t;

static void ml_call_expr_compile3(mlc_function_t *Function, ml_value_t *Value, ml_call_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	ml_inst_t *SetInst = MLC_EMIT(Child->EndLine, MLI_PARTIAL_SET, 1);
	int Index = SetInst[1].Count = Frame->Index;
	while ((Child = Child->Next)) {
		++Index;
		if (Child->compile != (void *)ml_blank_expr_compile) {
			Frame->Index = Index;
			Frame->Child = Child;
			return mlc_compile(Function, Child, 0);
		}
	}
	mlc_expr_t *Expr = Frame->Expr;
	if (!(Frame->Flags & MLCF_PUSH)) {
		MLC_EMIT(Expr->EndLine, MLI_POP, 0);
		--Function->Top;
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_call_expr_compile2(mlc_function_t *Function, ml_value_t *Value, ml_call_expr_frame_t *Frame) {
	mlc_expr_t *Expr = Frame->Expr;
	ml_inst_t *PartialInst = MLC_EMIT(Expr->StartLine, MLI_PARTIAL_NEW, 1);
	PartialInst[1].Count = Frame->Count;
	mlc_inc_top(Function);
	int Index = 0;
	for (mlc_expr_t *Child = Frame->Child; Child; Child = Child->Next) {
		if (Child->compile != (void *)ml_blank_expr_compile) {
			Function->Frame->run = (mlc_frame_fn)ml_call_expr_compile3;
			Frame->Index = Index;
			Frame->Child = Child;
			return mlc_compile(Function, Child, 0);
		}
		++Index;
	}
	if (!(Frame->Flags & MLCF_PUSH)) {
		MLC_EMIT(Expr->EndLine, MLI_POP, 0);
		--Function->Top;
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_call_expr_compile5(mlc_function_t *Function, ml_value_t *Value, ml_call_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	if (Child) {
		++Frame->Index;
		if ((Child = Child->Next)) {
			Frame->Child = Child;
			return mlc_compile(Function, Child, MLCF_PUSH);
		}
	}
	mlc_expr_t *Expr = Frame->Expr;
	int TailCall = Frame->Flags & MLCF_RETURN;
	//if (TailCall) fprintf(stderr, "Found a tail call at %s:%d!\n", Expr->Source, Expr->EndLine);
	if (Frame->Value) {
		int Count = Frame->Count;
		if (ml_typeof(Frame->Value) == MLMethodT) {
			ml_inst_t *CallInst;
			CallInst = MLC_EMIT(Expr->EndLine, (TailCall ? MLI_TAIL_CALL_METHOD : MLI_CALL_METHOD), 3);
			CallInst[2].Count = Count;
			CallInst[1].Value = Frame->Value;
		} else {
			ml_inst_t *CallInst;
			CallInst = MLC_EMIT(Expr->EndLine, (TailCall ? MLI_TAIL_CALL_CONST : MLI_CALL_CONST), 2);
			CallInst[2].Count = Count;
			ml_value_t *Value = Frame->Value;
			CallInst[1].Value = Value;
			if (ml_typeof(Value) == MLUninitializedT) ml_uninitialized_use(Value, &CallInst[1].Value);
		}
		Function->Top -= Count;
	} else {
		ml_inst_t *CallInst = MLC_EMIT(Expr->EndLine, (TailCall ? MLI_TAIL_CALL : MLI_CALL), 1);
		CallInst[1].Count = Frame->Count;
		Function->Top -= Frame->Count + 1;
	}
	if (Frame->NilInst) TailCall = 0;
	MLC_LINK(Frame->NilInst, Function->Next);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(TailCall ? MLExprGoto : NULL);
}

typedef struct {
	const mlc_expr_t *Expr;
	int Flags;
} ml_macro_frame_t;

static void ml_call_macro_compile2(mlc_function_t *Function, ml_value_t *Value, ml_macro_frame_t *Frame) {
	Value = ml_deref(Value);
	if (!ml_is(Value, MLExprT)) MLC_EXPR_ERROR(Frame->Expr, ml_error("MacroError", "Macro returned %s instead of expr", ml_typeof(Value)->Name));
	MLC_POP();
	mlc_expr_t *Expr = (mlc_expr_t *)Value;
	return mlc_compile(Function, Expr, Frame->Flags);
}

static void ml_call_macro_compile(mlc_function_t *Function, ml_macro_t *Macro, const mlc_expr_t *Expr, mlc_expr_t *Child, int Flags) {
	if (Macro->function) return mlc_compile(Function, Macro->function(Expr, Child, Macro->Data), Flags);
	int Count = 0;
	for (mlc_expr_t *E = Child; E; E = E->Next) ++Count;
	ml_value_t **Args = ml_alloc_args(Count);
	Count = 0;
	for (mlc_expr_t *E = Child; E; E = E->Next) {
		if (E->compile == (void *)ml_value_expr_compile) {
			mlc_value_expr_t *ValueExpr = (mlc_value_expr_t *)E;
			if (ml_typeof(ValueExpr->Value) == MLNamesT) {
				Args[Count++] = ValueExpr->Value;
				continue;
			}
		}
		Args[Count++] = ml_expr_value(E);
	}
	MLC_FRAME(ml_macro_frame_t, ml_call_macro_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Function->Frame->run = (mlc_frame_fn)ml_call_macro_compile2;
	Function->Frame->Line = Expr->StartLine;
	return ml_call(Function, Macro->Function, Count, Args);
}

typedef struct {
	ml_value_t *Value;
	mlc_expr_t *Child;
	ml_closure_info_t *Info;
	int Count, Line;
} mlc_inline_call_frame_t;

static void mlc_inline_call_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_inline_call_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	if (Child) {
		Frame->Child = Child->Next;
		++Frame->Count;
		return mlc_compile(Function, Child, MLCF_PUSH);
	}
	int Count = Frame->Count;
	ml_inst_t *CallInst;
	CallInst = MLC_EMIT(Frame->Line, MLI_TAIL_CALL_CONST, 2);
	CallInst[2].Count = Count;
	CallInst[1].Value = Frame->Value;
	ml_closure_info_t *Info = Frame->Info;
	ml_state_t *Caller = Function->Base.Caller;
	if (Function->UpValues) {
		ml_value_t *Error = ml_error("EvalError", "Use of non-constant value %s in constant expression", Function->UpValues->Decl->Ident);
		ml_error_trace_add(Error, (ml_source_t){Function->Source, Frame->Line});
		ML_RETURN(Error);
	}
	Info->Return = MLC_EMIT(Frame->Line, MLI_RETURN, 0);
	MLC_LINK(Function->Returns, Info->Return);
	Info->Halt = Function->Next;
	Info->Source = Function->Source;
	Info->StartLine = Frame->Line;
	Info->EndLine = Frame->Line;
	GC_asprintf((char **)&Info->Name, "@%s:%d", Info->Source, Info->StartLine);
	Info->FrameSize = Function->Size;
	Info->NumParams = 0;
	MLC_POP();
	return ml_call(Caller, ml_closure(Info), 0, NULL);
}

static void mlc_inline_call_expr_compile2(mlc_function_t *Parent, ml_value_t *Value, const mlc_expr_t *Expr, mlc_expr_t *Child) {
	Parent->Frame->Line = Expr->EndLine;
	mlc_function_t *Function = new(mlc_function_t);
	Function->Base.Type = MLCompilerFunctionT;
	Function->Base.Caller = (ml_state_t *)Parent;
	Function->Base.Context = Parent->Base.Context;
	Function->Base.run = (ml_state_fn)mlc_function_run;
	Function->Compiler = Parent->Compiler;
	Function->Eval = 1;
	Function->Source = Parent->Source;
	Function->Old = -1;
	Function->It = -1;
	Function->Up = Parent;
	Function->Size = 1;
	Function->Next = anew(ml_inst_t, 128);
	Function->Space = 126;
	Function->Returns = NULL;
	MLC_FRAME(mlc_inline_call_frame_t, mlc_inline_call_expr_compile3);
	Frame->Line = Expr->EndLine;
	Frame->Value = Value;
	Frame->Child = Child;
	Frame->Info = new(ml_closure_info_t);
	Frame->Info->Entry = Function->Next;
	return mlc_inline_call_expr_compile3(Function, NULL, Frame);
}

void mlc_inline_call_expr_compile(mlc_function_t *Function, ml_value_t *Value, const mlc_expr_t *Expr, mlc_expr_t *Child, int Flags) {
	MLC_FRAME(ml_inline_expr_frame_t, ml_inline_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_inline_call_expr_compile2(Function, Value, Expr, Child);
}

static void ml_inline_call(ml_state_t *Caller, ml_inline_function_t *Inline, int Count, ml_value_t **Args) {
	return ml_call(Caller, Inline->Value, Count, Args);
}

ML_TYPE(MLFunctionInlineT, (MLFunctionT), "inline",
	.call = (void *)ml_inline_call
);

ml_value_t *ml_inline_function(ml_value_t *Value) {
	ml_inline_function_t *Inline = new(ml_inline_function_t);
	Inline->Type = MLFunctionInlineT;
	Inline->Value = Value;
	return (ml_value_t *)Inline;
}

static void ml_call_expr_compile4(mlc_function_t *Function, ml_value_t *Value, ml_call_expr_frame_t *Frame) {
	mlc_expr_t *Expr = Frame->Expr;
	if (Value) {
		ml_value_t *Deref = ml_deref(Value);
		if (ml_typeof(Deref) == MLMacroT) {
			MLC_POP();
			ml_macro_t *Macro = (ml_macro_t *)Deref;
			return ml_call_macro_compile(Function, Macro, Expr, Frame->Child, Frame->Flags);
		} else if (ml_typeof(Deref) == MLFunctionInlineT) {
			MLC_POP();
			ml_inline_function_t *Inline = (ml_inline_function_t *)Deref;
			return mlc_inline_call_expr_compile(Function, Inline->Value, Expr, Frame->Child, Frame->Flags);
		}
	}
	mlc_expr_t *Child = Frame->Child;
	Function->Frame->run = (mlc_frame_fn)ml_call_expr_compile5;
	Function->Frame->Line = Expr->StartLine;
	Frame->Value = Value;
	Frame->Index = !Value;
	if (Child) {
		return mlc_compile(Function, Child, MLCF_PUSH);
	} else {
		return ml_call_expr_compile5(Function, NULL, Frame);
	}
}

extern ml_cfunctionx_t MLCall[];

void ml_call_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(ml_call_expr_frame_t, ml_call_expr_compile4);
	Frame->Expr = (mlc_expr_t *)Expr;
	Frame->Child = Expr->Child->Next;
	int Count = 0;
	for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next) ++Count;
	Frame->Count = Count;
	Frame->NilInst = NULL;
	Frame->Flags = Flags;
	if (Expr->Child->compile == (void *)ml_blank_expr_compile) {
		ml_inst_t *LoadInst = MLC_EMIT(Expr->StartLine, MLI_LOAD, 1);
		LoadInst[1].Value = (ml_value_t *)MLCall;
		ml_inst_t *PartialInst = MLC_EMIT(Expr->StartLine, MLI_PARTIAL_NEW, 1);
		PartialInst[1].Count = Frame->Count + 1;
		mlc_inc_top(Function);
		Function->Frame->run = (mlc_frame_fn)ml_call_expr_compile3;
		int Index = 0;
		for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next) {
			++Index;
			if (Child->compile != (void *)ml_blank_expr_compile) {
				Frame->Index = Index;
				Frame->Child = Child;
				return mlc_compile(Function, Child, 0);
			}
		}
		if (!(Flags & MLCF_PUSH)) {
			MLC_EMIT(Expr->EndLine, MLI_POP, 0);
			--Function->Top;
		}
		MLC_POP();
		MLC_RETURN(NULL);
	}
	for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next) {
		if (Child->compile == (void *)ml_blank_expr_compile) {
			Function->Frame->run = (mlc_frame_fn)ml_call_expr_compile2;
			return mlc_compile(Function, Expr->Child, 0);
		}
	}
	return mlc_compile(Function, Expr->Child, MLCF_CONSTANT | MLCF_PUSH);
}

void ml_const_call_expr_compile(mlc_function_t *Function, mlc_parent_value_expr_t *Expr, int Flags) {
	MLC_FRAME(ml_call_expr_frame_t, ml_call_expr_compile4);
	Frame->Expr = (mlc_expr_t *)Expr;
	Frame->Child = Expr->Child;
	int Count = 0;
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) ++Count;
	Frame->Count = Count;
	Frame->NilInst = NULL;
	Frame->Flags = Flags;
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) {
		if (Child->compile == (void *)ml_blank_expr_compile) {
			ml_inst_t *LoadInst = MLC_EMIT(Expr->StartLine, MLI_LOAD, 1);
			LoadInst[1].Value = Expr->Value;
			return ml_call_expr_compile2(Function, Expr->Value, Frame);
		}
	}
	return ml_call_expr_compile4(Function, Expr->Value, Frame);
}

typedef struct {
	mlc_expr_t *Guard;
	ml_call_expr_frame_t *Caller;
	int OldIt;
} ml_guard_expr_frame_t;

static void ml_guard_expr_compile3(mlc_function_t *Function, ml_value_t *Value, ml_guard_expr_frame_t *Frame) {
	Function->It = Frame->OldIt;
	MLC_POP();
	ml_inst_t *CheckInst = MLC_EMIT(Frame->Guard->EndLine, MLI_AND_POP, 2);
	CheckInst[2].Count = Frame->Caller->Index + 1;
	CheckInst[1].Inst = Frame->Caller->NilInst;
	Frame->Caller->NilInst = CheckInst + 1;
	MLC_RETURN(NULL);
}

static void ml_guard_expr_compile2(mlc_function_t *Function, ml_value_t *Value, ml_guard_expr_frame_t *Frame) {
	Frame->OldIt = Function->It;
	Function->It = Function->Top - 1;
	Function->Frame->run = (mlc_frame_fn)ml_guard_expr_compile3;
	return mlc_compile(Function, Frame->Guard, 0);
}

void ml_guard_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	if ((Function->Frame->run != (mlc_frame_fn)ml_call_expr_compile4) &&
		(Function->Frame->run != (mlc_frame_fn)ml_call_expr_compile5)) {
		MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Guard expression used outside of function call"));
	}
	ml_call_expr_frame_t *Caller = (ml_call_expr_frame_t *)Function->Frame->Data;
	MLC_FRAME(ml_guard_expr_frame_t, ml_guard_expr_compile2);
	Frame->Guard = Expr->Child->Next;
	Frame->Caller = Caller;
	return mlc_compile(Function, Expr->Child, MLCF_PUSH);
}

static void ml_tuple_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	if ((Child = Child->Next)) {
		Frame->Child = Child;
		++Frame->Count;
		return mlc_compile(Function, Child, MLCF_PUSH);
	}
	mlc_parent_expr_t *Expr = Frame->Expr;
	ml_inst_t *TupleInst = MLC_EMIT(Expr->StartLine, MLI_TUPLE_NEW, 1);
	TupleInst[1].Count = Frame->Count;
	Function->Top -= Frame->Count;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_tuple_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) {
		if (Child->compile == (void *)ml_blank_expr_compile) {
			MLC_FRAME(ml_call_expr_frame_t, ml_call_expr_compile4);
			Frame->Expr = (mlc_expr_t *)Expr;
			Frame->Child = Expr->Child;
			int Count = 0;
			for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) ++Count;
			Frame->Count = Count;
			Frame->NilInst = NULL;
			Frame->Flags = Flags;
			ml_inst_t *LoadInst = MLC_EMIT(Expr->StartLine, MLI_LOAD, 1);
			LoadInst[1].Value = (ml_value_t *)MLTupleT;
			return ml_call_expr_compile2(Function, (ml_value_t *)MLTupleT, Frame);
		}
	}
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) {
		if (Child->compile == (void *)ml_guard_expr_compile) {
			MLC_FRAME(ml_call_expr_frame_t, ml_call_expr_compile4);
			Frame->Expr = (mlc_expr_t *)Expr;
			Frame->Child = Expr->Child;
			int Count = 0;
			for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) ++Count;
			Frame->Count = Count;
			Frame->NilInst = NULL;
			Frame->Flags = Flags;
			return ml_call_expr_compile4(Function, (ml_value_t *)MLTupleT, Frame);
		}
	}
	mlc_expr_t *Child = Expr->Child;
	if (Child) {
		MLC_FRAME(mlc_parent_expr_frame_t, ml_tuple_expr_compile2);
		Frame->Expr = Expr;
		Frame->Child = Child;
		Frame->Flags = Flags;
		Frame->Count = 1;
		return mlc_compile(Function, Child, MLCF_PUSH);
	}
	ml_inst_t *TupleInst = MLC_EMIT(Expr->StartLine, MLI_TUPLE_NEW, 1);
	TupleInst[1].Count = 0;
	if (Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_RETURN(NULL);
}

static void ml_list_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	MLC_EMIT(Child->EndLine, MLI_LIST_APPEND, 0);
	if ((Child = Child->Next)) {
		Frame->Child = Child;
		return mlc_compile(Function, Child, 0);
	}
	mlc_parent_expr_t *Expr = Frame->Expr;
	if (!(Frame->Flags & MLCF_PUSH)) {
		MLC_EMIT(Expr->EndLine, MLI_POP, 0);
		--Function->Top;
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

ML_FUNCTION(MLListOfArgs) {
//!internal
	ml_value_t *List = ml_list();
	for (int I = 0; I < Count; ++I) ml_list_put(List, Args[I]);
	return List;
}

void ml_list_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) {
		if (Child->compile == (void *)ml_blank_expr_compile) {
			MLC_FRAME(ml_call_expr_frame_t, ml_call_expr_compile4);
			Frame->Expr = (mlc_expr_t *)Expr;
			Frame->Child = Expr->Child;
			int Count = 0;
			for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) ++Count;
			Frame->Count = Count;
			Frame->NilInst = NULL;
			Frame->Flags = Flags;
			ml_inst_t *LoadInst = MLC_EMIT(Expr->StartLine, MLI_LOAD, 1);
			LoadInst[1].Value = (ml_value_t *)MLListOfArgs;
			return ml_call_expr_compile2(Function, (ml_value_t *)MLListOfArgs, Frame);
		}
	}
	MLC_EMIT(Expr->StartLine, MLI_LIST_NEW, 0);
	mlc_inc_top(Function);
	mlc_expr_t *Child = Expr->Child;
	if (Child) {
		MLC_FRAME(mlc_parent_expr_frame_t, ml_list_expr_compile2);
		Frame->Expr = Expr;
		Frame->Child = Child;
		Frame->Flags = Flags;
		return mlc_compile(Function, Child, 0);
	}
	if (!(Flags & MLCF_PUSH)) {
		MLC_EMIT(Expr->EndLine, MLI_POP, 0);
		--Function->Top;
	}
	MLC_RETURN(NULL);
}

static void ml_map_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame);

static void ml_map_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	MLC_EMIT(Child->EndLine, MLI_MAP_INSERT, 0);
	--Function->Top;
	if ((Child = Child->Next)) {
		Function->Frame->run = (mlc_frame_fn)ml_map_expr_compile2;
		Frame->Child = Child;
		return mlc_compile(Function, Child, MLCF_PUSH);
	}
	mlc_parent_expr_t *Expr = Frame->Expr;
	if (!(Frame->Flags & MLCF_PUSH)) {
		MLC_EMIT(Expr->EndLine, MLI_POP, 0);
		--Function->Top;
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_map_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	Function->Frame->run = (mlc_frame_fn)ml_map_expr_compile3;
	mlc_expr_t *Child = Frame->Child = Frame->Child->Next;
	mlc_compile(Function, Child, 0);
}

ML_FUNCTION(MLMapOfArgs) {
//!internal
	ml_value_t *Map = ml_map();
	for (int I = 1; I < Count; I += 2) ml_map_insert(Map, Args[I - 1], Args[I]);
	return Map;
}

void ml_map_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) {
		if (Child->compile == (void *)ml_blank_expr_compile) {
			MLC_FRAME(ml_call_expr_frame_t, ml_call_expr_compile4);
			Frame->Expr = (mlc_expr_t *)Expr;
			Frame->Child = Expr->Child;
			int Count = 0;
			for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) ++Count;
			Frame->Count = Count;
			Frame->NilInst = NULL;
			Frame->Flags = Flags;
			ml_inst_t *LoadInst = MLC_EMIT(Expr->StartLine, MLI_LOAD, 1);
			LoadInst[1].Value = (ml_value_t *)MLMapOfArgs;
			return ml_call_expr_compile2(Function, (ml_value_t *)MLMapOfArgs, Frame);
		}
	}
	MLC_EMIT(Expr->StartLine, MLI_MAP_NEW, 0);
	mlc_inc_top(Function);
	mlc_expr_t *Child = Expr->Child;
	if (Child) {
		MLC_FRAME(mlc_parent_expr_frame_t, ml_map_expr_compile2);
		Frame->Expr = Expr;
		Frame->Child = Child;
		Frame->Flags = Flags;
		return mlc_compile(Function, Child, MLCF_PUSH);
	}
	if (!(Flags & MLCF_PUSH)) {
		MLC_EMIT(Expr->EndLine, MLI_POP, 0);
		--Function->Top;
	}
	MLC_RETURN(NULL);
}

typedef struct mlc_scoped_expr_t mlc_scoped_expr_t;

typedef struct {
	const char *Name;
	ml_value_t *Value;
} mlc_scoped_decl_t;

struct mlc_scoped_expr_t {
	MLC_EXPR_FIELDS(scoped);
	mlc_expr_t *Child;
	mlc_scoped_decl_t Decls[];
};

static void ml_scoped_expr_compile2(mlc_function_t *Function, ml_value_t *Value, ml_decl_t **Frame) {
	Function->Decls = Frame[0];
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_scoped_expr_compile(mlc_function_t *Function, mlc_scoped_expr_t *Expr, int Flags) {
	MLC_FRAME(ml_decl_t *, ml_scoped_expr_compile2);
	Frame[0] = Function->Decls;
	for (mlc_scoped_decl_t *Scoped = Expr->Decls; Scoped->Name; ++Scoped) {
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Ident = Scoped->Name;
		Decl->Hash = ml_ident_hash(Scoped->Name);
		Decl->Value = Scoped->Value;
		Decl->Flags = MLC_DECL_CONSTANT;
		Decl->Next = Function->Decls;
		Function->Decls = Decl;
	}
	return mlc_compile(Function, Expr->Child, Flags);
}

ML_METHODV("scoped", MLExprT, MLNamesT) {
//!macro
//<Expr
//<Name,Value
//>expr
// Returns a new expression which wraps :mini:`Expr` with the constant definitions from :mini:`Names` and :mini:`Values`.
	ML_NAMES_CHECK_ARG_COUNT(1);
	mlc_expr_t *Child = (mlc_expr_t *)Args[0];
	mlc_scoped_expr_t *Expr = xnew(mlc_scoped_expr_t, ml_names_length(Args[1]) + 1, mlc_scoped_decl_t);
	Expr->Source = Child->Source;
	Expr->StartLine = Child->StartLine;
	Expr->EndLine = Child->EndLine;
	Expr->compile = ml_scoped_expr_compile;
	Expr->Child = Child;
	int I = 2;
	mlc_scoped_decl_t *Decl = Expr->Decls;
	ML_NAMES_FOREACH(Args[1], Iter) {
		Decl->Name = ml_string_value(Iter->Value);
		Decl->Value = Args[I++];
		++Decl;
	}
	return ml_expr_value((mlc_expr_t *)Expr);
}

ML_METHOD("scoped", MLExprT, MLMapT) {
//!macro
//<Expr
//<Definitions
//>expr
// Returns a new expression which wraps :mini:`Expr` with the constant definitions from :mini:`Definitions`.
	mlc_expr_t *Child = (mlc_expr_t *)Args[0];
	mlc_scoped_expr_t *Expr = xnew(mlc_scoped_expr_t, ml_map_size(Args[1]) + 1, mlc_scoped_decl_t);
	Expr->Source = Child->Source;
	Expr->StartLine = Child->StartLine;
	Expr->EndLine = Child->EndLine;
	Expr->compile = ml_scoped_expr_compile;
	Expr->Child = Child;
	mlc_scoped_decl_t *Decl = Expr->Decls;
	ML_MAP_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Key, MLStringT)) return ml_error("MacroError", "Invalid definition");
		Decl->Name = ml_string_value(Iter->Key);
		Decl->Value = Iter->Value;
		++Decl;
	}
	return ml_expr_value((mlc_expr_t *)Expr);
}

static int ml_scoped_decl_add(const char *Name, ml_value_t *Value, mlc_scoped_decl_t **Decls) {
	Decls[0]->Name = Name;
	Decls[0]->Value = Value;
	++Decls[0];
	return 0;
}

ML_METHOD("scoped", MLExprT, MLModuleT) {
//!macro
//<Expr
//<Module
//>expr
// Returns a new expression which wraps :mini:`Expr` with the exports from :mini:`Module`.
	mlc_expr_t *Child = (mlc_expr_t *)Args[0];
	ml_module_t *Module = (ml_module_t *)Args[1];
	mlc_scoped_expr_t *Expr = xnew(mlc_scoped_expr_t, Module->Exports->Size + 1, mlc_scoped_decl_t);
	Expr->Source = Child->Source;
	Expr->StartLine = Child->StartLine;
	Expr->EndLine = Child->EndLine;
	Expr->compile = ml_scoped_expr_compile;
	Expr->Child = Child;
	mlc_scoped_decl_t *Decl = Expr->Decls;
	stringmap_foreach(Module->Exports, &Decl, (void *)ml_scoped_decl_add);
	return ml_expr_value((mlc_expr_t *)Expr);
}

ML_METHOD("scoped", MLExprT, MLTypeT) {
//!macro
//<Expr
//<Module
//>expr
// Returns a new expression which wraps :mini:`Expr` with the exports from :mini:`Module`.
	mlc_expr_t *Child = (mlc_expr_t *)Args[0];
	ml_type_t *Type = (ml_type_t *)Args[1];
	mlc_scoped_expr_t *Expr = xnew(mlc_scoped_expr_t, Type->Exports->Size + 1, mlc_scoped_decl_t);
	Expr->Source = Child->Source;
	Expr->StartLine = Child->StartLine;
	Expr->EndLine = Child->EndLine;
	Expr->compile = ml_scoped_expr_compile;
	Expr->Child = Child;
	mlc_scoped_decl_t *Decl = Expr->Decls;
	stringmap_foreach(Type->Exports, &Decl, (void *)ml_scoped_decl_add);
	return ml_expr_value((mlc_expr_t *)Expr);
}

typedef struct mlc_subst_expr_t mlc_subst_expr_t;

struct mlc_subst_expr_t {
	MLC_EXPR_FIELDS(subst);
	mlc_expr_t *Child;
	stringmap_t Subst[1];
};

static void ml_subst_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_define_t **Frame) {
	Function->Defines = Frame[0];
	MLC_POP();
	MLC_RETURN(NULL);
}

static int ml_subst_define_fn(const char *Ident, const mlc_expr_t *Expr, mlc_function_t *Function) {
	mlc_define_t *Define = new(mlc_define_t);
	Define->Ident = Ident;
	Define->Hash = ml_ident_hash(Ident);
	Define->Expr = Expr;
	Define->Next = Function->Defines;
	Function->Defines = Define;
	return 0;
}

static void ml_subst_expr_compile(mlc_function_t *Function, mlc_subst_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_define_t *, ml_subst_expr_compile2);
	Frame[0] = Function->Defines;
	stringmap_foreach(Expr->Subst, Function, (void *)ml_subst_define_fn);
	return mlc_compile(Function, Expr->Child, Flags);
}

typedef struct mlc_args_expr_t mlc_args_expr_t;

struct mlc_args_expr_t {
	MLC_EXPR_FIELDS(args);
	mlc_expr_t *Args[];
};

typedef struct {
	mlc_args_expr_t *Expr;
	ml_call_expr_frame_t *Parent;
	mlc_expr_t **Arg;
} mlc_args_expr_frame_t;

static void ml_args_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_args_expr_frame_t *Frame) {
	++Frame->Parent->Index;
	++Frame->Parent->Count;
	mlc_expr_t **Arg = Frame->Arg + 1;
	if (Arg[0]) {
		Frame->Arg = Arg;
		return mlc_compile(Function, Arg[0], MLCF_PUSH);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_args_expr_compile(mlc_function_t *Function, mlc_args_expr_t *Expr, int Flags) {
	if (!Function->Frame) MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Invalid use of expression list"));
	if ((void *)Function->Frame->run == (void *)ml_call_expr_compile5) {
		ml_call_expr_frame_t *Parent = (ml_call_expr_frame_t *)Function->Frame->Data;
		--Parent->Index;
		--Parent->Count;
		if (!Expr->Args[0]) MLC_RETURN(NULL);
		MLC_FRAME(mlc_args_expr_frame_t, ml_args_expr_compile2);
		Frame->Expr = Expr;
		Frame->Parent = Parent;
		Frame->Arg = Expr->Args;
		return mlc_compile(Function, Expr->Args[0], MLCF_PUSH);
	} else {
		MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Invalid use of expression list"));
	}
}

ML_METHODV("subst", MLExprT, MLNamesT) {
//!macro
//<Expr
//<Name,Sub
//>expr
// Returns a new expression which substitutes macro references to :mini:`:$Name/i` with the corresponding expression :mini:`Sub/i`.
	ML_NAMES_CHECK_ARG_COUNT(1);
	mlc_subst_expr_t *Expr = new(mlc_subst_expr_t);
	mlc_expr_t *Child = (mlc_expr_t *)Args[0];
	Expr->Source = Child->Source;
	Expr->StartLine = Child->StartLine;
	Expr->EndLine = Child->EndLine;
	Expr->compile = ml_subst_expr_compile;
	Expr->Child = Child;
	int I = 2;
	ML_NAMES_FOREACH(Args[1], Iter) {
		ml_value_t *Arg = Args[I];
		if (ml_is(Arg, MLListT)) {
			mlc_args_expr_t *ArgsExpr = xnew(mlc_args_expr_t, ml_list_length(Arg) + 1, mlc_expr_t *);
			ArgsExpr->Source = Child->Source;
			ArgsExpr->StartLine = Child->StartLine;
			ArgsExpr->EndLine = Child->EndLine;
			ArgsExpr->compile = ml_args_expr_compile;
			int J = 0;
			ML_LIST_FOREACH(Arg, Iter2) {
				if (!ml_is(Iter2->Value, MLExprT)) {
					return ml_error("CompilerError", "Expected expression not %s", ml_typeof(Iter2->Value)->Name);
				}
				ArgsExpr->Args[J++] = (mlc_expr_t *)Iter2->Value;
			}
			Arg = ml_expr_value((mlc_expr_t *)ArgsExpr);
		} else {
			ML_CHECK_ARG_TYPE(I, MLExprT);
		}
		stringmap_insert(Expr->Subst, ml_string_value(Iter->Value), Arg);
		++I;
	}
	return ml_expr_value((mlc_expr_t *)Expr);
}

ML_METHOD("subst", MLExprT, MLListT, MLListT) {
//!macro
//<Expr
//<Names
//<Subs
//>expr
// Returns a new expression which substitutes macro references to :mini:`:$Name/i` with the corresponding expressions :mini:`Sub/i`.
	if (ml_list_length(Args[2]) < ml_list_length(Args[1])) return ml_error("MacroError", "Insufficient arguments to macro");
	mlc_subst_expr_t *Expr = new(mlc_subst_expr_t);
	mlc_expr_t *Child = (mlc_expr_t *)Args[0];
	Expr->Source = Child->Source;
	Expr->StartLine = Child->StartLine;
	Expr->EndLine = Child->EndLine;
	Expr->compile = ml_subst_expr_compile;
	Expr->Child = Child;
	ml_list_node_t *Node = ((ml_list_t *)Args[2])->Head;
	ML_LIST_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Value, MLStringT)) return ml_error("MacroError", "Substitution name must be string");
		if (!ml_is(Node->Value, MLExprT)) return ml_error("MacroError", "Substitution value must be expr");
		stringmap_insert(Expr->Subst, ml_string_value(Iter->Value), Node->Value);
		Node = Node->Next;
	}
	return ml_expr_value((mlc_expr_t *)Expr);
}

ML_METHODV("subst", MLExprT, MLMapT) {
//!macro
//<Expr
//<Subs
//>expr
// Returns a new expression which substitutes macro references to :mini:`:$Name/i` with the corresponding expression :mini:`Sub/i`.
	mlc_subst_expr_t *Expr = new(mlc_subst_expr_t);
	mlc_expr_t *Child = (mlc_expr_t *)Args[0];
	Expr->Source = Child->Source;
	Expr->StartLine = Child->StartLine;
	Expr->EndLine = Child->EndLine;
	Expr->compile = ml_subst_expr_compile;
	Expr->Child = Child;
	ML_MAP_FOREACH(Args[1], Iter) {
		if (!ml_is(Iter->Key, MLStringT)) return ml_error("MacroError", "Substitution name must be a string");
		if (!ml_is(Iter->Value, MLExprT)) return ml_error("MacroError", "Substitution value must be expr");
		stringmap_insert(Expr->Subst, ml_string_value(Iter->Key), Iter->Value);
	}
	return ml_expr_value((mlc_expr_t *)Expr);
}

typedef struct {
	mlc_parent_value_expr_t *Expr;
	ml_value_t *Args[2];
	int Flags;
} ml_resolve_expr_frame_t;

static void ml_resolve_expr_compile3(mlc_function_t *Function, ml_value_t *Value, ml_resolve_expr_frame_t *Frame) {
	mlc_parent_value_expr_t *Expr = Frame->Expr;
	if (ml_is_error(Value)) {
		ml_inst_t *LoadInst = MLC_EMIT(Expr->EndLine, MLI_LOAD, 1);
		LoadInst[1].Value = Frame->Args[0];
		ml_inst_t *ResolveInst = MLC_EMIT(Expr->EndLine, MLI_RESOLVE, 1);
		ResolveInst[1].Value = Expr->Value;
		if (Frame->Flags & MLCF_PUSH) {
			MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
			mlc_inc_top(Function);
		}
		MLC_POP();
		MLC_RETURN(NULL);
	}
	int Flags = Frame->Flags;
	if (Flags & MLCF_CONSTANT) {
		MLC_POP();
		MLC_RETURN(Value);
	} else if (Flags & MLCF_PUSH) {
		ml_inst_t *ValueInst = MLC_EMIT(Expr->EndLine, MLI_LOAD_PUSH, 1);
		if (ml_typeof(Value) == MLUninitializedT) {
			ml_uninitialized_use(Value, &ValueInst[1].Value);
		}
		ValueInst[1].Value = Value;
		mlc_inc_top(Function);
		MLC_POP();
		MLC_RETURN(NULL);
	} else {
		ml_inst_t *ValueInst = MLC_EMIT(Expr->EndLine, MLI_LOAD, 1);
		if (ml_typeof(Value) == MLUninitializedT) {
			ml_uninitialized_use(Value, &ValueInst[1].Value);
		}
		ValueInst[1].Value = Value;
		MLC_POP();
		MLC_RETURN(NULL);
	}
}

static void ml_resolve_expr_compile2(mlc_function_t *Function, ml_value_t *Value, ml_resolve_expr_frame_t *Frame) {
	mlc_parent_value_expr_t *Expr = Frame->Expr;
	if (Value) {
		Frame->Args[0] = Value;
		Frame->Args[1] = Expr->Value;
		Function->Frame->AllowErrors = 1;
		Function->Frame->run = (mlc_frame_fn)ml_resolve_expr_compile3;
		return ml_call(Function, SymbolMethod, 2, Frame->Args);
	} else {
		ml_inst_t *ResolveInst = MLC_EMIT(Expr->EndLine, MLI_RESOLVE, 1);
		ResolveInst[1].Value = Expr->Value;
		if (Frame->Flags & MLCF_PUSH) {
			MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
			mlc_inc_top(Function);
		}
		MLC_POP();
		MLC_RETURN(NULL);
	}
}

void ml_resolve_expr_compile(mlc_function_t *Function, mlc_parent_value_expr_t *Expr, int Flags) {
	MLC_FRAME(ml_resolve_expr_frame_t, ml_resolve_expr_compile2);
	Function->Frame->Line = Expr->StartLine;
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	mlc_compile(Function, Expr->Child, MLCF_CONSTANT);
}

typedef struct {
	mlc_string_expr_t *Expr;
	mlc_string_part_t *Part;
	mlc_expr_t *Child;
	int NumArgs, Flags;
} ml_string_expr_frame_t;

static void ml_string_expr_compile2(mlc_function_t *Function, ml_value_t *Value, ml_string_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	if (Child) {
		++Frame->NumArgs;
		Frame->Child = Child->Next;
		return mlc_compile(Function, Child, MLCF_PUSH);
	}
	mlc_string_part_t *Part = Frame->Part;
	if (Frame->NumArgs > 1) {
		ml_inst_t *AddInst = MLC_EMIT(Part->Line, MLI_STRING_ADD, 1);
		AddInst[1].Count = Frame->NumArgs;
	} else {
		MLC_EMIT(Part->Line, MLI_STRING_ADD_1, 0);
	}
	Function->Top -= Frame->NumArgs;
	while ((Part = Part->Next)) {
		if (Part->Length) {
			ml_inst_t *AddInst = MLC_EMIT(Part->Line, MLI_STRING_ADDS, 2);
			AddInst[1].Count = Part->Length;
			AddInst[2].Chars = Part->Chars;
		} else {
			Frame->Part = Part;
			mlc_expr_t *Child = Part->Child;
			Frame->Child = Child->Next;
			Frame->NumArgs = 1;
			return mlc_compile(Function, Child, MLCF_PUSH);
		}
	}
	mlc_string_expr_t *Expr = Frame->Expr;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_STRING_END, 0);
	} else {
		MLC_EMIT(Expr->EndLine, MLI_STRING_POP, 0);
		--Function->Top;
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

void ml_string_expr_compile(mlc_function_t *Function, mlc_string_expr_t *Expr, int Flags) {
	MLC_EMIT(Expr->StartLine, MLI_STRING_NEW, 0);
	mlc_inc_top(Function);
	for (mlc_string_part_t *Part = Expr->Parts; Part; Part = Part->Next) {
		if (Part->Length) {
			ml_inst_t *AddInst = MLC_EMIT(Part->Line, MLI_STRING_ADDS, 2);
			AddInst[1].Count = Part->Length;
			AddInst[2].Chars = Part->Chars;
		} else {
			MLC_FRAME(ml_string_expr_frame_t, ml_string_expr_compile2);
			Frame->Expr = Expr;
			Frame->Part = Part;
			mlc_expr_t *Child = Part->Child;
			Frame->Child = Child->Next;
			Frame->NumArgs = 1;
			Frame->Flags = Flags;
			return mlc_compile(Function, Child, MLCF_PUSH);
		}
	}
	if (Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_STRING_END, 0);
	} else {
		MLC_EMIT(Expr->EndLine, MLI_STRING_POP, 0);
		--Function->Top;
	}
	MLC_RETURN(NULL);
}

typedef struct {
	mlc_fun_expr_t *Expr;
	ml_closure_info_t *Info;
	mlc_function_t *SubFunction;
	mlc_param_t *Param;
	int HasParamTypes, Index, Flags;
	ml_opcode_t OpCode;
} mlc_fun_expr_frame_t;

static void ml_fun_expr_compile4(mlc_function_t *Function, ml_value_t *Value, mlc_fun_expr_frame_t *Frame) {
	mlc_fun_expr_t *Expr = Frame->Expr;
	mlc_param_t *Param = Frame->Param;
	int Index = Frame->Index;
	ml_inst_t *TypeInst = MLC_EMIT(Param->Line, MLI_PARAM_TYPE, 1);
	TypeInst[1].Count = Index;
	while ((Param = Param->Next)) {
		++Index;
		if (Param->Type) {
			Frame->Param = Param;
			Frame->Index = Index;
			return mlc_compile(Function, Param->Type, 0);
		}
	}
	if (!(Frame->Flags & MLCF_PUSH)) {
		MLC_EMIT(Expr->EndLine, MLI_POP, 0);
		--Function->Top;
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_fun_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_fun_expr_frame_t *Frame) {
	mlc_fun_expr_t *Expr = Frame->Expr;
	ml_closure_info_t *Info = Frame->Info;
	mlc_function_t *SubFunction = Frame->SubFunction;
	int NumUpValues = 0;
	for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next) ++NumUpValues;
	ml_inst_t *ClosureInst = MLC_EMIT(Expr->StartLine, Frame->OpCode, NumUpValues + 1);
	Info->NumUpValues = NumUpValues;
	ClosureInst[1].ClosureInfo = Info;
	int Index = 1;
	for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next) ClosureInst[++Index].Count = UpValue->Index;
	if (Frame->HasParamTypes) {
		MLC_EMIT(Expr->StartLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
		mlc_param_t *Param = Expr->Params;
		int Index = 0;
		while (!Param->Type) {
			Param = Param->Next;
			++Index;
		}
		Frame->Param = Param;
		Frame->Index = Index;
		Function->Frame->run = (mlc_frame_fn)ml_fun_expr_compile4;
		return mlc_compile(Function, Param->Type, 0);
	}
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_fun_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_fun_expr_frame_t *Frame) {
	mlc_fun_expr_t *Expr = Frame->Expr;
	ml_closure_info_t *Info = Frame->Info;
	mlc_function_t *SubFunction = Frame->SubFunction;
	Info->Return = ml_inst_alloc(SubFunction, Expr->EndLine, MLI_RETURN, 0);
	MLC_LINK(SubFunction->Returns, Info->Return);
	Info->Halt = SubFunction->Next;
	ml_decl_t **UpValueSlot = &SubFunction->Decls;
	while (UpValueSlot[0]) UpValueSlot = &UpValueSlot[0]->Next;
	int Index = 0;
	for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next, ++Index) {
		ml_decl_t *Decl = new(ml_decl_t);
		Decl->Source.Name = Function->Source;
		Decl->Source.Line = UpValue->Line;
		Decl->Ident = UpValue->Decl->Ident;
		Decl->Hash = UpValue->Decl->Hash;
		Decl->Value = UpValue->Decl->Value;
		Decl->Index = ~Index;
		UpValueSlot[0] = Decl;
		UpValueSlot = &Decl->Next;
	}
	Info->FrameSize = SubFunction->Size;
	if (SubFunction->UpValues || Frame->HasParamTypes || Expr->ReturnType) {
#ifdef ML_GENERICS
		if (Expr->ReturnType) {
			Frame->OpCode = MLI_CLOSURE_TYPED;
			Function->Frame->run = (mlc_frame_fn)ml_fun_expr_compile3;
			return mlc_compile(Function, Expr->ReturnType, 0);
		} else {
#endif
			Frame->OpCode = MLI_CLOSURE;
			return ml_fun_expr_compile3(Function, NULL, Frame);
#ifdef ML_GENERICS
		}
#endif
	} else {
		Info->NumUpValues = 0;
		ml_value_t *Closure = ml_closure(Info);
		if (Frame->Flags & MLCF_PUSH) {
			ml_inst_t *LoadInst = MLC_EMIT(Expr->StartLine, MLI_LOAD_PUSH, 1);
			LoadInst[1].Value = Closure;
			mlc_inc_top(Function);
		} else {
			ml_inst_t *LoadInst = MLC_EMIT(Expr->StartLine, MLI_LOAD, 1);
			LoadInst[1].Value = Closure;
		}
		MLC_POP();
		MLC_RETURN(NULL);
	}
}

static void ml_subfunction_run(mlc_function_t *SubFunction, ml_value_t *Value, void *Frame) {
	mlc_function_t *Function = SubFunction->Up;
	MLC_RETURN(NULL);
}

void ml_fun_expr_compile(mlc_function_t *Function, mlc_fun_expr_t *Expr, int Flags) {
	mlc_function_t *SubFunction = new(mlc_function_t);
	SubFunction->Base.Type = MLCompilerFunctionT;
	SubFunction->Base.Caller = (ml_state_t *)Function;
	SubFunction->Base.Context = Function->Base.Context;
	SubFunction->Base.run = (ml_state_fn)mlc_function_run;
	SubFunction->Compiler = Function->Compiler;
	SubFunction->Eval = 0;
	SubFunction->Up = Function;
	SubFunction->Source = Expr->Source;
	SubFunction->Old = -1;
	SubFunction->It = -1;
	ml_closure_info_t *Info = new(ml_closure_info_t);
	Info->Source = Expr->Source;
	Info->StartLine = Expr->StartLine;
	Info->EndLine = Expr->EndLine;
	if (Expr->Name) {
		Info->Name = Expr->Name;
	} else {
		GC_asprintf((char **)&Info->Name, "@%s:%d", Info->Source, Info->StartLine);
	}
	int NumParams = 0, HasParamTypes = 0;
	ml_decl_t **DeclSlot = &SubFunction->Decls;
	for (mlc_param_t *Param = Expr->Params; Param; Param = Param->Next) {
		ml_decl_t *Decl = DeclSlot[0] = new(ml_decl_t);
		Decl->Source.Name = Function->Source;
		Decl->Source.Line = Param->Line;
		Decl->Ident = Param->Ident;
		Decl->Hash = ml_ident_hash(Param->Ident);
		Decl->Index = NumParams++;
		switch (Param->Kind) {
		case ML_PARAM_EXTRA:
			Info->Flags |= ML_CLOSURE_EXTRA_ARGS;
			break;
		case ML_PARAM_NAMED:
			Info->Flags |= ML_CLOSURE_NAMED_ARGS;
			break;
		case ML_PARAM_BYREF:
			Decl->Flags |= MLC_DECL_BYREF;
			stringmap_insert(Info->Params, Param->Ident, (void *)(intptr_t)NumParams);
			break;
		case ML_PARAM_ASVAR:
			Decl->Flags |= MLC_DECL_ASVAR;
			stringmap_insert(Info->Params, Param->Ident, (void *)(intptr_t)NumParams);
			break;
		default:
			stringmap_insert(Info->Params, Param->Ident, (void *)(intptr_t)NumParams);
			break;
		}
		if (Param->Type) HasParamTypes = 1;
		DeclSlot = &Decl->Next;
	}
	Info->NumParams = NumParams;
	SubFunction->Top = SubFunction->Size = NumParams;
	SubFunction->Next = anew(ml_inst_t, 128);
	SubFunction->Space = 126;
	SubFunction->Returns = NULL;
	mlc_frame_alloc(SubFunction, 0, ml_subfunction_run);
	Info->Decls = SubFunction->Decls;
	Info->Entry = SubFunction->Next;
	MLC_FRAME(mlc_fun_expr_frame_t, ml_fun_expr_compile2);
	Frame->Expr = Expr;
	Frame->Info = Info;
	Frame->HasParamTypes = HasParamTypes;
	Frame->SubFunction = SubFunction;
	Frame->Flags = Flags;
	mlc_compile(SubFunction, Expr->Body, MLCF_RETURN);
}

typedef struct {
	mlc_default_expr_t *Expr;
	ml_inst_t *AndInst;
	int Flags;
} mlc_default_expr_frame_t;

static void ml_default_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_default_expr_frame_t *Frame) {
	mlc_default_expr_t *Expr = Frame->Expr;
	int Flags = Frame->Flags;
	ml_inst_t *AssignInst = MLC_EMIT(Expr->StartLine, Expr->Flags & ML_PARAM_ASVAR ? MLI_VAR : MLI_LET, 1);
	AssignInst[1].Count = Expr->Index - Function->Top;
	Frame->AndInst[1].Inst = Function->Next;
	MLC_POP();
	return mlc_compile(Function, Expr->Next, Flags);
}

void ml_default_expr_compile(mlc_function_t *Function, mlc_default_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_default_expr_frame_t, ml_default_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_LOCAL, 1);
	LocalInst[1].Count = Expr->Index - Function->Top;
	Frame->AndInst = MLC_EMIT(Expr->StartLine, MLI_OR, 1);
	return mlc_compile(Function, Expr->Child, 0);
}

static int ml_upvalue_find(mlc_function_t *Function, ml_decl_t *Decl, mlc_function_t *Origin, int Line) {
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
	UpValue->Index = ml_upvalue_find(Function->Up, Decl, Origin, Line);
	UpValue->Line = Line;
	UpValueSlot[0] = UpValue;
	return ~Index;
}

static void ml_ident_expr_finish(mlc_function_t *Function, mlc_ident_expr_t *Expr, ml_value_t *Value, int Flags) {
	if (Flags & MLCF_CONSTANT) MLC_RETURN(Value);
	ml_inst_t *ValueInst = MLC_EMIT(Expr->StartLine, MLI_LOAD, 1);
	if (ml_typeof(Value) == MLUninitializedT) {
		ml_uninitialized_use(Value, &ValueInst[1].Value);
	}
	ValueInst[1].Value = Value;
	if (Flags & MLCF_PUSH) {
		ValueInst->Opcode = MLI_LOAD_PUSH;
		mlc_inc_top(Function);
	}
	MLC_RETURN(NULL);
}

void ml_ident_expr_compile(mlc_function_t *Function, mlc_ident_expr_t *Expr, int Flags) {
	long Hash = ml_ident_hash(Expr->Ident);
	//printf("#<%s> -> %ld\n", Expr->Ident, Hash);
	for (mlc_function_t *UpFunction = Function; UpFunction; UpFunction = UpFunction->Up) {
		for (ml_decl_t *Decl = UpFunction->Decls; Decl; Decl = Decl->Next) {
			if (Hash == Decl->Hash && !strcmp(Decl->Ident, Expr->Ident)) {
				if (Decl->Flags == MLC_DECL_CONSTANT) {
					if (!Decl->Value) Decl->Value = ml_uninitialized(Decl->Ident, (ml_source_t){Expr->Source, Expr->StartLine});
					return ml_ident_expr_finish(Function, Expr, Decl->Value, Flags);
				} else {
					int Index = ml_upvalue_find(Function, Decl, UpFunction, Expr->StartLine);
					if (Decl->Flags & MLC_DECL_FORWARD) Decl->Flags |= MLC_DECL_BACKFILL;
					if (Index < 0) {
						ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_UPVALUE, 1);
						LocalInst[1].Count = ~Index;
					} else if (Decl->Flags & MLC_DECL_FORWARD) {
						ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_LOCALI, 2);
						LocalInst[1].Count = Index - Function->Top;
						LocalInst[2].Decls = Decl;
					} else {
						if (Flags & MLCF_LOCAL) {
							MLC_RETURN(ml_integer(Index));
						} else if (Flags & MLCF_PUSH) {
							ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_LOCAL_PUSH, 1);
							LocalInst[1].Count = Index - Function->Top;
							mlc_inc_top(Function);
							MLC_RETURN(NULL);
						} else {
							ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_LOCAL, 1);
							LocalInst[1].Count = Index - Function->Top;
						}
					}
					if (Flags & MLCF_PUSH) {
						MLC_EMIT(Expr->StartLine, MLI_PUSH, 0);
						mlc_inc_top(Function);
					}
					MLC_RETURN(NULL);
				}
			}
		}
	}
	ml_value_t *Value = (ml_value_t *)stringmap_search(Function->Compiler->Vars, Expr->Ident);
	if (!Value) Value = Function->Compiler->GlobalGet(Function->Compiler->Globals, Expr->Ident, Expr->Source, Expr->StartLine, Function->Eval);
	if (!Value) {
		MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Identifier %s not declared", Expr->Ident));
	}
	if (ml_is_error(Value)) MLC_EXPR_ERROR(Expr, Value);
	return ml_ident_expr_finish(Function, Expr, Value, Flags);
}

ML_FUNCTION(MLIdentExpr) {
//!macro
//@macro::ident
//<Name:string
//>expr
// Returns a new identifier expression.
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	mlc_ident_expr_t *Expr = new(mlc_ident_expr_t);
	Expr->compile = ml_ident_expr_compile;
	Expr->Source = "<macro>";
	Expr->StartLine = 1;
	Expr->EndLine = 1;
	Expr->Ident = ml_string_value(Args[0]);
	return ml_expr_value((mlc_expr_t *)Expr);
}

ML_FUNCTION(MLValueExpr) {
//!macro
//@macro::value
//<Value:any
//>expr
// Returns a new value expression.
	ML_CHECK_ARG_COUNT(1);
	if (Args[0] == MLNil) {
		mlc_expr_t *Expr = new(mlc_expr_t);
		Expr->compile = ml_nil_expr_compile;
		Expr->Source = "<macro>";
		Expr->StartLine = 1;
		Expr->EndLine = 1;
		return ml_expr_value(Expr);
	} else {
		mlc_value_expr_t *Expr = new(mlc_value_expr_t);
		Expr->compile = ml_value_expr_compile;
		Expr->Source = "<macro>";
		Expr->StartLine = 1;
		Expr->EndLine = 1;
		Expr->Value = Args[0];
		return ml_expr_value((mlc_expr_t *)Expr);
	}
}

ml_value_t *ml_macro_subst(mlc_expr_t *Child, int Count, const char **Names, ml_value_t **Exprs) {
	mlc_subst_expr_t *Expr = new(mlc_subst_expr_t);
	Expr->Source = Child->Source;
	Expr->StartLine = Child->StartLine;
	Expr->EndLine = Child->EndLine;
	Expr->compile = ml_subst_expr_compile;
	Expr->Child = Child;
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Exprs[I];
		if (ml_is(Arg, MLListT)) {
			mlc_args_expr_t *ArgsExpr = xnew(mlc_args_expr_t, ml_list_length(Arg) + 1, mlc_expr_t *);
			ArgsExpr->Source = Child->Source;
			ArgsExpr->StartLine = Child->StartLine;
			ArgsExpr->EndLine = Child->EndLine;
			ArgsExpr->compile = ml_args_expr_compile;
			int J = 0;
			ML_LIST_FOREACH(Arg, Iter2) {
				if (!ml_is(Iter2->Value, MLExprT)) {
					return ml_error("CompilerError", "Expected expression not %s", ml_typeof(Iter2->Value)->Name);
				}
				ArgsExpr->Args[J++] = (mlc_expr_t *)Iter2->Value;
			}
			Arg = ml_expr_value((mlc_expr_t *)ArgsExpr);
		}
		stringmap_insert(Expr->Subst, Names[I], Arg);
	}
	return ml_expr_value((mlc_expr_t *)Expr);
}

typedef struct {
	ml_type_t *Type;
	mlc_expr_t *Expr;
	mlc_param_t *Params;
} ml_macro_subst_t;

static void ml_macro_subst_call(ml_state_t *Caller, ml_macro_subst_t *Subst, int Count, ml_value_t **Args) {
	ml_source_t Source = ml_debugger_source(Caller);
	mlc_subst_expr_t *Expr = new(mlc_subst_expr_t);
	mlc_expr_t *Child = Subst->Expr;
	Expr->Source = Source.Name;
	Expr->StartLine = Source.Line;
	Expr->EndLine = Source.Line;
	Expr->compile = ml_subst_expr_compile;
	Expr->Child = Child;
	int I = 0;
	for (mlc_param_t *Param = Subst->Params; Param; Param = Param->Next) {
		ML_CHECKX_ARG_COUNT(I + 1);
		ML_CHECKX_ARG_TYPE(I, MLExprT);
		stringmap_insert(Expr->Subst, Param->Ident, Args[I]);
		++I;
	}
	ML_RETURN(ml_expr_value((mlc_expr_t *)Expr));
}

ML_TYPE(MLMacroSubstT, (MLFunctionT), "macro::subst",
	.call = (void *)ml_macro_subst_call
);

ML_FUNCTION(MLMacroSubst) {
//!macro
//@macro::subst
//<Expr:expr
//>macro
// Returns a new macro which substitutes its arguments into :mini:`Expr`.
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLExprT);
	mlc_fun_expr_t *FunExpr = (mlc_fun_expr_t *)Args[0];
	if (FunExpr->compile != (void *)ml_fun_expr_compile) {
		return ml_error("MacroError", "Function expression required for substitution");
	}
	ml_macro_subst_t *Subst = new(ml_macro_subst_t);
	Subst->Type = MLMacroSubstT;
	Subst->Expr = FunExpr->Body;
	Subst->Params = FunExpr->Params;
	mlc_value_expr_t *Expr = new(mlc_value_expr_t);
	Expr->compile = ml_value_expr_compile;
	Expr->Source = FunExpr->Source;
	Expr->StartLine = FunExpr->StartLine;
	Expr->EndLine = FunExpr->EndLine;
	Expr->Value = ml_macro((ml_value_t *)Subst);
	return ml_expr_value((mlc_expr_t *)Expr);
}

ML_METHOD_DECL(VarMethod, "var");
ML_METHOD_DECL(RefMethod, "ref");
ML_METHOD_DECL(LetMethod, "let");

ML_FUNCTION(MLFunExpr) {
//!macro
//@macro::fun
//<Params:map[string,method|nil]
//>expr
// Returns a new function expression.
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLMapT);
	ML_CHECK_ARG_TYPE(1, MLExprT);
	mlc_param_t *Params = NULL, **Slot = &Params;
	ML_MAP_FOREACH(Args[0], Iter) {
		mlc_param_t *Param = Slot[0] = new(mlc_param_t);
		Slot = &Param->Next;
		if (!ml_is(Iter->Key, MLStringT)) return ml_error("TypeError", "Parameter name must be a string");
		Param->Ident = ml_string_value(Iter->Key);
		if (Iter->Value == (ml_value_t *)MLListT) {
			Param->Kind = ML_PARAM_EXTRA;
		} else if (Iter->Value == (ml_value_t *)MLMapT) {
			Param->Kind = ML_PARAM_NAMED;
		} else if (Iter->Value == RefMethod) {
			Param->Kind = ML_PARAM_BYREF;
		} else if (Iter->Value == VarMethod) {
			Param->Kind = ML_PARAM_ASVAR;
		}
	}
	mlc_fun_expr_t *Expr = new(mlc_fun_expr_t);
	Expr->compile = ml_fun_expr_compile;
	Expr->Source = "<macro>";
	Expr->StartLine = 1;
	Expr->EndLine = 1;
	Expr->Params = Params;
	Expr->Body = ml_delegate_expr(Args[1]);
	return ml_expr_value((mlc_expr_t *)Expr);
}

typedef struct {
	ml_type_t *Type;
	mlc_block_expr_t *Expr;
	mlc_expr_t **ExprSlot;
	mlc_local_t **VarsSlot;
	mlc_local_t **LetsSlot;
	mlc_local_t **DefsSlot;
} mlc_block_builder_t;

ML_TYPE(MLBlockBuilderT, (), "block-builder");
//!macro
// Utility object for building a block expression.

ML_METHOD("var", MLBlockBuilderT, MLStringT) {
//!macro
//<Builder
//<Name
//>blockbuilder
// Adds a :mini:`var`-declaration to a block.
	mlc_block_builder_t *Builder = (mlc_block_builder_t *)Args[0];
	mlc_local_t *Local = Builder->VarsSlot[0] = mlc_local_new(ml_string_value(Args[1]), 1);
	Builder->VarsSlot = &Local->Next;
	//TODO: add support for types
	return Args[0];
}

ML_METHOD("var", MLBlockBuilderT, MLStringT, MLExprT) {
//!macro
//<Builder
//<Name
//<Expr
//>blockbuilder
// Adds a :mini:`var`-declaration to a block with initializer :mini:`Expr`.
	mlc_block_builder_t *Builder = (mlc_block_builder_t *)Args[0];
	mlc_local_t *Local = Builder->VarsSlot[0] = mlc_local_new(ml_string_value(Args[1]), 1);
	Builder->VarsSlot = &Local->Next;
	//TODO: add support for types
	mlc_local_expr_t *LocalExpr = new(mlc_local_expr_t);
	LocalExpr->compile = ml_var_expr_compile;
	LocalExpr->Source = "<macro>";
	LocalExpr->StartLine = 1;
	LocalExpr->EndLine = 1;
	LocalExpr->Local = Local;
	LocalExpr->Child = ml_delegate_expr(Args[2]);
	Builder->ExprSlot[0] = (mlc_expr_t *)LocalExpr;
	Builder->ExprSlot = &LocalExpr->Next;
	return Args[0];
}

ML_METHOD("let", MLBlockBuilderT, MLStringT, MLExprT) {
//!macro
//<Builder
//<Name
//<Expr
//>blockbuilder
// Adds a :mini:`let`-declaration to a block with initializer :mini:`Expr`.
	mlc_block_builder_t *Builder = (mlc_block_builder_t *)Args[0];
	mlc_local_t *Local = Builder->LetsSlot[0] = mlc_local_new(ml_string_value(Args[1]), 1);
	Builder->LetsSlot = &Local->Next;
	//TODO: add support for types
	mlc_local_expr_t *LocalExpr = new(mlc_local_expr_t);
	LocalExpr->compile = ml_let_expr_compile;
	LocalExpr->Source = "<macro>";
	LocalExpr->StartLine = 1;
	LocalExpr->EndLine = 1;
	LocalExpr->Local = Local;
	LocalExpr->Child = ml_delegate_expr(Args[2]);
	Builder->ExprSlot[0] = (mlc_expr_t *)LocalExpr;
	Builder->ExprSlot = &LocalExpr->Next;
	return Args[0];
}

ML_METHODV("do", MLBlockBuilderT, MLExprT) {
//!macro
//<Builder
//<Expr/i...
//>blockbuilder
// Adds each expression :mini:`Expr/i` to a block.
	mlc_block_builder_t *Builder = (mlc_block_builder_t *)Args[0];
	for (int I = 1; I < Count; ++I) {
		mlc_expr_t *Delegate = ml_delegate_expr(Args[I]);
		Builder->ExprSlot[0] = Delegate;
		Builder->ExprSlot = &Delegate->Next;
	}
	return Args[0];
}

ML_METHOD("end", MLBlockBuilderT) {
//!macro
//<Builder
//>expr
// Finishes a block and returns it as an expression.
	mlc_block_builder_t *Builder = (mlc_block_builder_t *)Args[0];
	mlc_block_expr_t *Expr = Builder->Expr;
	int Index = 0, First = 0;
	for (mlc_local_t *Local = Expr->Vars; Local; Local = Local->Next) {
		Local->Index = Index++;
	}
	Expr->NumVars = Index;
	First = Index;
	for (mlc_local_t *Local = Expr->Lets; Local; Local = Local->Next) {
		Local->Index = Index++;
	}
	Expr->NumLets = Index - First;
	First = Index;
	for (mlc_local_t *Local = Expr->Defs; Local; Local = Local->Next) {
		Local->Index = Index++;
	}
	Expr->NumDefs = Index - First;
	return ml_expr_value((mlc_expr_t *)Expr);
}

ML_FUNCTION(MLBlockBuilder) {
//!macro
//@macro::block
//>blockbuilder
// Returns a new block builder.
	mlc_block_expr_t *Expr = new(mlc_block_expr_t);
	Expr->compile = ml_block_expr_compile;
	Expr->Source = "<macro>";
	Expr->StartLine = 1;
	Expr->EndLine = 1;
	mlc_block_builder_t *Builder = new(mlc_block_builder_t);
	Builder->Type = MLBlockBuilderT;
	Builder->Expr = Expr;
	Builder->ExprSlot = &Expr->Child;
	Builder->VarsSlot = &Expr->Vars;
	Builder->LetsSlot = &Expr->Lets;
	Builder->DefsSlot = &Expr->Defs;
	return (ml_value_t *)Builder;
}

typedef struct {
	ml_type_t *Type;
	mlc_parent_expr_t *Expr;
	mlc_expr_t **ExprSlot;
} mlc_expr_builder_t;

ML_TYPE(MLExprBuilderT, (), "expr-builder");
//!macro
// Utility object for building a block expression.

ML_FUNCTION(MLTupleBuilder) {
//!macro
//@macro::tuple
//>exprbuilder
// Returns a new list builder.
	mlc_parent_expr_t *Expr = new(mlc_parent_expr_t);
	Expr->compile = ml_tuple_expr_compile;
	Expr->Source = "<macro>";
	Expr->StartLine = 1;
	Expr->EndLine = 1;
	mlc_expr_builder_t *Builder = new(mlc_expr_builder_t);
	Builder->Type = MLExprBuilderT;
	Builder->Expr = Expr;
	Builder->ExprSlot = &Expr->Child;
	return (ml_value_t *)Builder;
}

ML_FUNCTION(MLListBuilder) {
//!macro
//@macro::list
//>exprbuilder
// Returns a new list builder.
	mlc_parent_expr_t *Expr = new(mlc_parent_expr_t);
	Expr->compile = ml_list_expr_compile;
	Expr->Source = "<macro>";
	Expr->StartLine = 1;
	Expr->EndLine = 1;
	mlc_expr_builder_t *Builder = new(mlc_expr_builder_t);
	Builder->Type = MLExprBuilderT;
	Builder->Expr = Expr;
	Builder->ExprSlot = &Expr->Child;
	return (ml_value_t *)Builder;
}

ML_FUNCTION(MLMapBuilder) {
//!macro
//@macro::map
//>exprbuilder
// Returns a new list builder.
	mlc_parent_expr_t *Expr = new(mlc_parent_expr_t);
	Expr->compile = ml_map_expr_compile;
	Expr->Source = "<macro>";
	Expr->StartLine = 1;
	Expr->EndLine = 1;
	mlc_expr_builder_t *Builder = new(mlc_expr_builder_t);
	Builder->Type = MLExprBuilderT;
	Builder->Expr = Expr;
	Builder->ExprSlot = &Expr->Child;
	return (ml_value_t *)Builder;
}

ML_FUNCTION(MLCallBuilder) {
//!macro
//@macro::call
//>exprbuilder
// Returns a new call builder.
	mlc_parent_expr_t *Expr = new(mlc_parent_expr_t);
	Expr->compile = ml_call_expr_compile;
	Expr->Source = "<macro>";
	Expr->StartLine = 1;
	Expr->EndLine = 1;
	mlc_expr_builder_t *Builder = new(mlc_expr_builder_t);
	Builder->Type = MLExprBuilderT;
	Builder->Expr = Expr;
	Builder->ExprSlot = &Expr->Child;
	return (ml_value_t *)Builder;
}

ML_METHODV("add", MLExprBuilderT, MLExprT) {
//!macro
//<Builder
//<Expr...
//>blockbuilder
// Adds the expression :mini:`Expr` to a block.
	mlc_block_builder_t *Builder = (mlc_block_builder_t *)Args[0];
	for (int I = 1; I < Count; ++I) {
		mlc_expr_t *Delegate = ml_delegate_expr(Args[I]);
		Builder->ExprSlot[0] = Delegate;
		Builder->ExprSlot = &Delegate->Next;
	}
	return Args[0];
}

ML_METHOD("end", MLExprBuilderT) {
//!macro
//<Builder
//>expr
// Finishes a block and returns it as an expression.
	mlc_expr_builder_t *Builder = (mlc_expr_builder_t *)Args[0];
	return ml_expr_value((mlc_expr_t *)Builder->Expr);
}

void ml_define_expr_compile(mlc_function_t *Function, mlc_ident_expr_t *Expr, int Flags) {
	long Hash = ml_ident_hash(Expr->Ident);
	for (mlc_function_t *UpFunction = Function; UpFunction; UpFunction = UpFunction->Up) {
		for (mlc_define_t *Define = UpFunction->Defines; Define; Define = Define->Next) {
			if (Hash == Define->Hash) {
				//printf("\tTesting <%s>\n", Decl->Ident);
				if (!strcmp(Define->Ident, Expr->Ident)) {
					return mlc_compile(Function, Define->Expr, Flags);
				}
			}
		}
	}
	MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "Identifier %s not defined", Expr->Ident));
}

#include "ml_expr_types.c"

const char *MLTokens[] = {
	"", // MLT_NONE,
	"<end of line>", // MLT_EOL,
	"<end of input>", // MLT_EOI,
	"and", // MLT_AND,
	"case", // MLT_CASE,
	"def", // MLT_DEF,
	"do", // MLT_DO,
	"each", // MLT_EACH,
	"else", // MLT_ELSE,
	"elseif", // MLT_ELSEIF,
	"end", // MLT_END,
	"exit", // MLT_EXIT,
	"for", // MLT_FOR,
	"fun", // MLT_FUN,
	"if", // MLT_IF,
	"ifConfig", // MLT_IF_CONFIG
	"in", // MLT_IN,
	"is", // MLT_IS,
	"it", // MLT_IT,
	"let", // MLT_LET,
	"loop", // MLT_LOOP,
	"meth", // MLT_METH,
	"must", // MLT_MUST,
	"next", // MLT_NEXT,
	"nil", // MLT_NIL,
	"not", // MLT_NOT,
	"old", // MLT_OLD,
	"on", // MLT_ON,
	"or", // MLT_OR,
	"recur", // MLT_RECUR,
	"ref", // MLT_REF,
	"ret", // MLT_RET,
	"seq", // MLT_SEQ,
	"susp", // MLT_SUSP,
	"switch", // MLT_SWITCH,
	"then", // MLT_THEN,
	"to", // MLT_TO,
	"until", // MLT_UNTIL,
	"var", // MLT_VAR,
	"when", // MLT_WHEN,
	"while", // MLT_WHILE,
	"with", // MLT_WITH,
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
	":-", // MLT_NAMED,
	"<escape>", // MLT_ESCAPE,
	"<import>", // MLT_IMPORT,
	"<value>", // MLT_VALUE,
	"<expr>", // MLT_EXPR,
	"<inline>", // MLT_INLINE,
	"<expand>", // MLT_EXPAND,
	"<expr_value>", // MLT_EXPR_VALUE,
	"<operator>", // MLT_OPERATOR,
	"<method>" // MLT_METHOD
};

static void ml_compiler_call(ml_state_t *Caller, ml_compiler_t *Compiler, int Count, ml_value_t **Args) {
	ML_RETURN(MLNil);
}

static ml_value_t *ml_function_global_get(ml_value_t *Function, const char *Name, const char *Source, int Line, int Eval) {
	ml_value_t *Value = ml_simple_inline(Function, 3, ml_string(Name, -1), ml_string(Source, -1), ml_integer(Line), ml_integer(Eval));
	return (Value != MLNotFound) ? Value : NULL;
}

static ml_value_t *ml_map_global_get(ml_value_t *Map, const char *Name, const char *Source, int Line, int Eval) {
	return ml_map_search0(Map, ml_string(Name, -1));
}

ML_FUNCTION(MLCompiler) {
//@compiler
//<Global:function|map
//>compiler
	ML_CHECK_ARG_COUNT(1);
	ml_getter_t GlobalGet = (ml_getter_t)ml_function_global_get;
	if (ml_is(Args[0], MLMapT)) GlobalGet = (ml_getter_t)ml_map_global_get;
	return (ml_value_t *)ml_compiler(GlobalGet, Args[0]);
}

ML_TYPE(MLCompilerT, (MLStateT), "compiler",
	.call = (void *)ml_compiler_call,
	.Constructor = (ml_value_t *)MLCompiler
);

ML_FUNCTIONX(MLSource) {
//@source
//>tuple[string,integer]
// Returns the caller source location. Evaluated at compile time if possible.
	ml_source_t Source = ml_debugger_source(Caller);
	ML_RETURN(ml_tuplev(2, ml_string(Source.Name, -1), ml_integer(Source.Line)));
}

static ml_inline_function_t MLSourceInline[1] = {{MLFunctionInlineT, (ml_value_t *)MLSource}};

ml_compiler_t *ml_compiler(ml_getter_t GlobalGet, void *Globals) {
	ml_compiler_t *Compiler = new(ml_compiler_t);
	Compiler->Type = MLCompilerT;
	Compiler->GlobalGet = GlobalGet;
	Compiler->Globals = Globals;
	return Compiler;
}

void ml_compiler_define(ml_compiler_t *Compiler, const char *Name, ml_value_t *Value) {
	stringmap_insert(Compiler->Vars, Name, Value);
}

ml_value_t *ml_compiler_lookup(ml_compiler_t *Compiler, const char *Name, const char *Source, int Line, int Eval) {
	ml_value_t *Value = (ml_value_t *)stringmap_search(Compiler->Vars, Name);
	if (!Value) Value = Compiler->GlobalGet(Compiler->Globals, Name, Source, Line, Eval);
	return Value;
}

static const char *ml_parser_no_input(void *Data) {
	return NULL;
}

static const char *ml_function_read(ml_value_t *Function) {
	ml_value_t *Result = ml_simple_call(Function, 0, NULL);
	if (!ml_is(Result, MLStringT)) return NULL;
	return ml_string_value(Result);
}

ML_FUNCTION(MLParser) {
//@parser
//<Read?:function
//>parser
	void *Input = NULL;
	ml_reader_t Reader = ml_parser_no_input;
	if (Count > 0) {
		Input = Args[0];
		Reader = (ml_reader_t)ml_function_read;
	}
	return (ml_value_t *)ml_parser(Reader, Input);
}

ML_TYPE(MLParserT, (), "parser",
	.Constructor = (ml_value_t *)MLParser
);

static ml_value_t *ml_parser_default_escape(void *Data) {
	return ml_error("ParseError", "Parser does support escaping");
}

static ml_value_t *ml_parser_default_special(void *Data) {
	return ml_error("ParseError", "Parser does support special values");
}

static stringmap_t MLEscapeFns[1] = {STRINGMAP_INIT};

ml_parser_t *ml_parser(ml_reader_t Read, void *Data) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLParserT;
	Parser->Token = MLT_NONE;
	Parser->Next = "";
	Parser->Source.Name = "";
	Parser->Source.Line = 1;
	Parser->Line = 1;
	Parser->ReadData = Data;
	Parser->Read = Read ?: ml_parser_no_input;
	Parser->Escape = ml_parser_default_escape;
	Parser->Special = ml_parser_default_special;
	Parser->EscapeFns = MLEscapeFns;
	return Parser;
}

static inline const char *ml_parser_do_read(ml_parser_t *Parser) {
#ifdef ML_ASYNC_PARSER

#else
	return Parser->Read(Parser->ReadData);
#endif
}

static mlc_expr_t *ml_accept_block(ml_parser_t *Parser);
static void ml_accept_eoi(ml_parser_t *Parser);

const char *ml_parser_name(ml_parser_t *Parser) {
	return Parser->Source.Name;
}

inline ml_source_t ml_parser_position(ml_parser_t *Parser) {
	return Parser->Source;
}

ml_source_t ml_parser_source(ml_parser_t *Parser, ml_source_t Source) {
	ml_source_t OldSource = Parser->Source;
	Parser->Source = Source;
	Parser->Line = Source.Line;
	return OldSource;
}

ml_value_t *ml_parser_value(ml_parser_t *Parser) {
	return Parser->Value;
}

void ml_parser_reset(ml_parser_t *Parser) {
	Parser->Token = MLT_NONE;
	Parser->Next = "";
}

void ml_parser_permissive(ml_parser_t *Parser, int Permissive) {
	Parser->Warnings = Permissive ? ml_list() : NULL;
}

ml_value_t *ml_parser_warnings(ml_parser_t *Parser) {
	return Parser->Warnings;
}

void ml_parser_input(ml_parser_t *Parser, const char *Text, int Advance) {
	Parser->Next = Text;
	Parser->Line += Advance;
}

const char *ml_parser_clear(ml_parser_t *Parser) {
	const char *Next = Parser->Next;
	Parser->Next = "";
	return Next;
}

const char *ml_parser_read(ml_parser_t *Parser) {
	return Parser->Read(Parser->ReadData);
}

/*void ml_parse_error(ml_parser_t *Parser, const char *Error, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	ml_value_t *Value = ml_errorv(Error, Format, Args);
	va_end(Args);
	ml_error_trace_add(Value, Parser->Source);
	Parser->Value = Value;
	longjmp(Parser->OnError, 1);
}*/

void ml_parse_warn(ml_parser_t *Parser, const char *Error, const char *Format, ...) {
	if (!Parser->Warnings) {
		va_list Args;
		va_start(Args, Format);
		ml_value_t *Value = ml_errorv(Error, Format, Args);
		va_end(Args);
		ml_error_trace_add(Value, Parser->Source);
		Parser->Value = Value;
		longjmp(Parser->OnError, 1);
	}
	char *Message;
	va_list Args;
	va_start(Args, Format);
	int Length = GC_vasprintf(&Message, Format, Args);
	va_end(Args);
	ml_list_put(Parser->Warnings, ml_tuplev(3,
		ml_string(Parser->Source.Name, -1),
		ml_integer(Parser->Line),
		ml_string(Message, Length)
	));
}

static ml_value_t *ml_parser_escape_other(ml_parser_t *Parser) {
	return Parser->Escape(Parser->EscapeData);
}

void ml_parser_escape(ml_parser_t *Parser, ml_value_t *(*Escape)(void *), void *Data) {
	Parser->Escape = Escape;
	Parser->EscapeData = Data;
	ml_parser_add_escape(Parser, "", ml_parser_escape_other);
}

void ml_parser_special(ml_parser_t *Parser, ml_value_t *(*Special)(void *), void *Data) {
	Parser->Special = Special;
	Parser->SpecialData = Data;
}

static int ml_parse(ml_parser_t *Parser, ml_token_t Token);
static mlc_expr_t *ml_parse_expression(ml_parser_t *Parser, ml_expr_level_t Level);
static mlc_expr_t *ml_accept_term(ml_parser_t *Parser, int MethDecl);
static void ml_accept_arguments(ml_parser_t *Parser, ml_token_t EndToken, mlc_expr_t **ArgsSlot);

static inline uint8_t ml_nibble(ml_parser_t *Parser, char C) {
	switch (C) {
	case '0' ... '9': return C - '0';
	case 'A' ... 'F': return (C - 'A') + 10;
	case 'a' ... 'f': return (C - 'a') + 10;
	default:
		ml_parse_warn(Parser, "ParseError", "Invalid character in escape sequence");
		return 0;
	}
}

static ml_token_t ml_accept_string(ml_parser_t *Parser) {
	mlc_string_part_t *Parts = NULL, **Slot = &Parts;
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *End = Parser->Next;
	for (;;) {
		char C = *End++;
		if (!C) {
			End = ml_parser_do_read(Parser);
			if (!End) {
				ml_parse_warn(Parser, "ParseError", "End of input while parsing string");
				Parser->Next = "";
				goto eoi;
			}
		} else if (C == '\'') {
			Parser->Next = End;
			break;
		} else if (C == '{') {
			if (ml_stringbuffer_length(Buffer)) {
				mlc_string_part_t *Part = new(mlc_string_part_t);
				Part->Length = ml_stringbuffer_length(Buffer);
				Part->Chars = ml_stringbuffer_get_string(Buffer);
				Part->Line = Parser->Source.Line;
				Slot[0] = Part;
				Slot = &Part->Next;
			}
			Parser->Next = End;
			mlc_string_part_t *Part = new(mlc_string_part_t);
			ml_accept_arguments(Parser, MLT_RIGHT_BRACE, &Part->Child);
			if (!Part->Child) {
				ml_parse_warn(Parser, "ParserError", "Empty string expression");
			}
			Part->Line = Parser->Source.Line;
			End = Parser->Next;
			Slot[0] = Part;
			Slot = &Part->Next;
		} else if (C == '\\') {
			C = *End++;
			switch (C) {
			case 'r': ml_stringbuffer_put(Buffer, '\r'); break;
			case 'n': ml_stringbuffer_put(Buffer, '\n'); break;
			case 't': ml_stringbuffer_put(Buffer, '\t'); break;
			case 'e': ml_stringbuffer_put(Buffer, '\e'); break;
			case 'x': {
				char Char = ml_nibble(Parser, *End++) << 4;
				Char += ml_nibble(Parser, *End++);
				ml_stringbuffer_put(Buffer, Char);
				break;
			}
			case 'u': {
				uint32_t Code = ml_nibble(Parser, *End++) << 12;
				Code += ml_nibble(Parser, *End++) << 8;
				Code += ml_nibble(Parser, *End++) << 4;
				Code += ml_nibble(Parser, *End++);
				ml_stringbuffer_put32(Buffer, Code);
				break;
			}
			case 'U': {
				uint32_t Code = ml_nibble(Parser, *End++) << 28;
				Code += ml_nibble(Parser, *End++) << 24;
				Code += ml_nibble(Parser, *End++) << 20;
				Code += ml_nibble(Parser, *End++) << 16;
				Code += ml_nibble(Parser, *End++) << 12;
				Code += ml_nibble(Parser, *End++) << 8;
				Code += ml_nibble(Parser, *End++) << 4;
				Code += ml_nibble(Parser, *End++);
				ml_stringbuffer_put32(Buffer, Code);
				break;
			}
			case '\'': ml_stringbuffer_put(Buffer, '\''); break;
			case '\"': ml_stringbuffer_put(Buffer, '\"'); break;
			case '\\': ml_stringbuffer_put(Buffer, '\\'); break;
			case '0': ml_stringbuffer_put(Buffer, '\0'); break;
			case '{': ml_stringbuffer_put(Buffer, '{'); break;
			case '\n': ++Parser->Line; break;
			case 0:
				ml_parse_warn(Parser, "ParseError", "End of line while parsing string");
				Parser->Next = "";
				goto eoi;
			}
		} else if (C == '\n') {
			++Parser->Line;
			ml_stringbuffer_write(Buffer, End - 1, 1);
		} else {
			ml_stringbuffer_write(Buffer, End - 1, 1);
		}
	}
eoi:
	if (!Parts) {
		Parser->Value = ml_stringbuffer_get_value(Buffer);
		return (Parser->Token = MLT_VALUE);
	} else {
		if (ml_stringbuffer_length(Buffer)) {
			mlc_string_part_t *Part = new(mlc_string_part_t);
			Part->Length = ml_stringbuffer_length(Buffer);
			Part->Chars = ml_stringbuffer_get_string(Buffer);
			Part->Line = Parser->Source.Line;
			Slot[0] = Part;
		}
		ML_EXPR(Expr, string, string);
		Expr->Parts = Parts;
		Parser->Expr = ML_EXPR_END(Expr);
		return (Parser->Token = MLT_EXPR);
	}
}

typedef enum {
	ML_CHAR_OTHER,
	ML_CHAR_EOI,
	ML_CHAR_SPACE,
	ML_CHAR_LINE,
	ML_CHAR_ALPHA,
	ML_CHAR_DIGIT,
	ML_CHAR_OPER,
	ML_CHAR_DELIM,
	ML_CHAR_COLON,
	ML_CHAR_SQUOTE,
	ML_CHAR_DQUOTE,
	ML_CHAR_SPECIAL
} ml_char_type_t;

static const unsigned char CharTypes[256] = {
	ML_CHAR_OTHER,
	[0] = ML_CHAR_EOI,
	[1 ... ' '] = ML_CHAR_SPACE,
	['\n'] = ML_CHAR_LINE,
	['0' ... '9'] = ML_CHAR_DIGIT,
	['_'] = ML_CHAR_ALPHA,
	['A' ... 'Z'] = ML_CHAR_ALPHA,
	['a' ... 'z'] = ML_CHAR_ALPHA,
	['!'] = ML_CHAR_OPER,
	['@'] = ML_CHAR_OPER,
	['#'] = ML_CHAR_OPER,
	['$'] = ML_CHAR_OPER,
	['%'] = ML_CHAR_OPER,
	['^'] = ML_CHAR_OPER,
	['&'] = ML_CHAR_OPER,
	['*'] = ML_CHAR_OPER,
	['-'] = ML_CHAR_OPER,
	['+'] = ML_CHAR_OPER,
	['='] = ML_CHAR_OPER,
	['|'] = ML_CHAR_OPER,
	['\\'] = ML_CHAR_OPER,
	['~'] = ML_CHAR_OPER,
	['`'] = ML_CHAR_OPER,
	['/'] = ML_CHAR_OPER,
	['?'] = ML_CHAR_OPER,
	['<'] = ML_CHAR_OPER,
	['>'] = ML_CHAR_OPER,
	['.'] = ML_CHAR_OPER,
	[':'] = ML_CHAR_COLON,
	['('] = ML_CHAR_DELIM,
	[')'] = ML_CHAR_DELIM,
	['['] = ML_CHAR_DELIM,
	[']'] = ML_CHAR_DELIM,
	['{'] = ML_CHAR_DELIM,
	['}'] = ML_CHAR_DELIM,
	[';'] = ML_CHAR_DELIM,
	[','] = ML_CHAR_DELIM,
	['\''] = ML_CHAR_SQUOTE,
	['\"'] = ML_CHAR_DQUOTE,
	[128 ... 238] = ML_CHAR_ALPHA,
	[239] = ML_CHAR_SPECIAL,
	[240 ... 253] = ML_CHAR_ALPHA
};

static const ml_token_t CharTokens[256] = {
	0,
	['('] = MLT_LEFT_PAREN,
	[')'] = MLT_RIGHT_PAREN,
	['['] = MLT_LEFT_SQUARE,
	[']'] = MLT_RIGHT_SQUARE,
	['{'] = MLT_LEFT_BRACE,
	['}'] = MLT_RIGHT_BRACE,
	[';'] = MLT_SEMICOLON,
	[':'] = MLT_COLON,
	[','] = MLT_COMMA
};

static inline int ml_isidstart(char C) {
	return CharTypes[(unsigned char)C] == ML_CHAR_ALPHA;
}

static inline int ml_isidchar(char C) {
	return CharTypes[(unsigned char)C] == ML_CHAR_ALPHA || CharTypes[(unsigned char)C] == ML_CHAR_DIGIT;
}

static inline int ml_isoperator(char C) {
	return CharTypes[(unsigned char)C] == ML_CHAR_OPER;
}

static inline int ml_isdigit(char C) {
	return CharTypes[(unsigned char)C] == ML_CHAR_DIGIT;
}

#include "keywords.c"

void ml_parser_add_escape(ml_parser_t *Parser, const char *Prefix, ml_parser_escape_t Fn) {
	if (!Parser) {
		stringmap_insert(MLEscapeFns, Prefix, Fn);
	} else {
		if (Parser->EscapeFns == MLEscapeFns) Parser->EscapeFns = stringmap_copy(MLEscapeFns);
		stringmap_insert(Parser->EscapeFns, Prefix, Fn);
	}
}

//static stringmap_t StringFns[1] = {STRINGMAP_INIT};

//void ml_string_fn_register(const char *Prefix, string_fn_t Fn) {
//	stringmap_insert(StringFns, Prefix, Fn);
//}

static inline char *ml_scan_utf8(char *D, uint32_t Code) {
	char Val[8];
	uint32_t LeadByteMax = 0x7F;
	int I = 0;
	while (Code > LeadByteMax) {
		Val[I++] = (Code & 0x3F) | 0x80;
		Code >>= 6;
		LeadByteMax >>= (I == 1 ? 2 : 1);
	}
	Val[I++] = (Code & LeadByteMax) | (~LeadByteMax << 1);
	while (I--) *D++ = Val[I];
	return D;
}

static int ml_scan_string(ml_parser_t *Parser) {
	const char *End = Parser->Next;
	int Closed = 1;
	while (End[0] != '\"') {
		if (!End[0]) {
			ml_parse_warn(Parser, "ParseError", "End of input while parsing string");
			Closed = 0;
			break;
		}
		if (End[0] == '\\') {
			++End;
			if (!End[0]) {
				ml_parse_warn(Parser, "ParseError", "End of input while parsing string");
				Closed = 0;
				break;
			}
		}
		++End;
	}
	int Length = End - Parser->Next;
	char *Quoted = snew(Length + 1), *D = Quoted;
	for (const char *S = Parser->Next; S < End; ++S) {
		if (*S == '\\') {
			switch (*++S) {
			case 'r': *D++ = '\r'; break;
			case 'n': *D++ = '\n'; break;
			case 't': *D++ = '\t'; break;
			case 'e': *D++ = '\e'; break;
			case 'x': {
				char Char = ml_nibble(Parser, *++S) << 4;
				Char += ml_nibble(Parser, *++S);
				*D++ = Char;
				break;
			}
			case 'u': {
				uint32_t Code = ml_nibble(Parser, *++S) << 12;
				Code += ml_nibble(Parser, *++S) << 8;
				Code += ml_nibble(Parser, *++S) << 4;
				Code += ml_nibble(Parser, *++S);
				D = ml_scan_utf8(D, Code);
				break;
			}
			case 'U': {
				uint32_t Code = ml_nibble(Parser, *++S) << 28;
				Code += ml_nibble(Parser, *++S) << 24;
				Code += ml_nibble(Parser, *++S) << 20;
				Code += ml_nibble(Parser, *++S) << 16;
				Code += ml_nibble(Parser, *++S) << 12;
				Code += ml_nibble(Parser, *++S) << 8;
				Code += ml_nibble(Parser, *++S) << 4;
				Code += ml_nibble(Parser, *++S);
				D = ml_scan_utf8(D, Code);
				break;
			}
			case '\'': *D++ = '\''; break;
			case '\"': *D++ = '\"'; break;
			case '\\': *D++ = '\\'; break;
			case '0': *D++ = '\0'; break;
			case 0: goto eoi;
			default: *D++ = '\\'; *D++ = *S; break;
			}
		} else {
			*D++ = *S;
		}
	}
eoi:
	*D = 0;
	Parser->Ident = Quoted;
	Parser->Next = End + Closed;
	return D - Quoted;
}

static int ml_scan_raw_string(ml_parser_t *Parser) {
	const char *End = Parser->Next;
	while (End[0] != '\"') {
		if (!End[0]) {
			ml_parse_warn(Parser, "ParseError", "End of input while parsing string");
			break;
		}
		if (End[0] == '\\') ++End;
		++End;
	}
	int Length = End - Parser->Next;
	char *Raw = snew(Length + 1);
	memcpy(Raw, Parser->Next, Length);
	Raw[Length] = 0;
	Parser->Ident = Raw;
	Parser->Next = End + 1;
	return Length;
}

static inthash_t IdentCache[1] = {INTHASH_INIT};

static const char *ml_ident(const char *Next, int Length) {
	uintptr_t Key = 0;
	switch (Length) {
	case 0: return "";
	case 1 ... sizeof(uintptr_t): memcpy(&Key, Next, Length); break;
	default: {
		char *Ident = snew(Length + 1);
		memcpy(Ident, Next, Length);
		Ident[Length] = 0;
		return Ident;
	}
	}
	char *Ident = inthash_search_inline(IdentCache, Key);
	if (!Ident) {
		Ident = snew(Length + 1);
		memcpy(Ident, Next, Length);
		Ident[Length] = 0;
		inthash_insert(IdentCache, Key, Ident);
	}
	//fprintf(stderr, "%s -> 0x%lx\n", Ident, Ident);
	return Ident;
}

int ml_ident_cache_check() {
	inthash_t Copy = IdentCache[0];
	for (int I = 0; I < Copy.Size; ++I) {
		if (Copy.Values[I]) {
			char Key[sizeof(uintptr_t) + 1] = {0,};
			memcpy(Key, Copy.Keys + I, sizeof(uintptr_t));
			if (strcmp(Key, (char *)Copy.Values[I])) {
				return 1;
			}
		}
	}
	return 0;
}

static ml_token_t ml_scan(ml_parser_t *Parser) {
	const char *Next = Parser->Next;
	for (;;) {
		char Char = Next[0];
		static const void *Labels[] = {
			&&DO_CHAR_OTHER,
			&&DO_CHAR_EOI,
			&&DO_CHAR_SPACE,
			&&DO_CHAR_LINE,
			&&DO_CHAR_ALPHA,
			&&DO_CHAR_DIGIT,
			&&DO_CHAR_OPER,
			&&DO_CHAR_DELIM,
			&&DO_CHAR_COLON,
			&&DO_CHAR_SQUOTE,
			&&DO_CHAR_DQUOTE,
			&&DO_CHAR_SPECIAL
		};
		goto *Labels[CharTypes[(unsigned char)Char]];
		DO_CHAR_EOI:
			Next = ml_parser_do_read(Parser);
			if (Next) continue;
			Parser->Next = "";
			Parser->Token = MLT_EOI;
			return Parser->Token;
		DO_CHAR_LINE:
			Parser->Next = Next + 1;
			++Parser->Line;
			Parser->Token = MLT_EOL;
			return Parser->Token;
		DO_CHAR_SPACE:
			++Next;
			continue;
		DO_CHAR_SPECIAL: {
			if ((unsigned char)Next[1] == 0xBF && (unsigned char)Next[2] == 0xBC) {
				Parser->Next = Next + 3;
				Parser->Value = Parser->Special(Parser->SpecialData);
				Parser->Token = MLT_VALUE;
				return Parser->Token;
			}
			// no break;
		}
		DO_CHAR_ALPHA: {
			const char *End = Next + 1;
			while (ml_isidchar(*End)) ++End;
			int Length = End - Next;
			const struct keyword_t *Keyword = lookup(Next, Length);
			if (Keyword) {
				Parser->Token = Keyword->Token;
				Parser->Ident = MLTokens[Parser->Token];
				Parser->Next = End;
				return Parser->Token;
			}
			const char *Ident = ml_ident(Next, Length);
			if (End[0] == '\"' || End[0] == '\'') {
				Parser->Ident = Ident;
				Parser->Token = MLT_ESCAPE;
				Parser->Next = End;
				return Parser->Token;
			}
			Parser->Next = End;
			Parser->Ident = Ident;
			Parser->Token = MLT_IDENT;
			return Parser->Token;
		}
		DO_CHAR_DIGIT: {
			char *End;
			double Double = strtod(Next, (char **)&End);
#ifdef ML_COMPLEX
			if (*End == 'i') {
				Parser->Value = ml_complex(Double * 1i);
				Parser->Token = MLT_VALUE;
				Parser->Next = End + 1;
				return Parser->Token;
			}
#endif
			for (const char *P = Next; P < End; ++P) {
				if (P[0] == '.' || P[0] == 'e' || P[0] == 'E') {
					Parser->Value = ml_real(Double);
					Parser->Token = MLT_VALUE;
					Parser->Next = End;
					return Parser->Token;
				}
			}
			long Integer = strtol(Next, (char **)&End, 10);
			Parser->Value = ml_integer(Integer);
			Parser->Token = MLT_VALUE;
			Parser->Next = End;
			return Parser->Token;
		}
		DO_CHAR_SQUOTE:
			Parser->Next = Next + 1;
			return ml_accept_string(Parser);
		DO_CHAR_DQUOTE: {
			Parser->Next = Next + 1;
			int Length = ml_scan_string(Parser);
			Parser->Value = ml_string(Parser->Ident, Length);
			Parser->Token = MLT_VALUE;
			return Parser->Token;
		}
		DO_CHAR_COLON: {
			Char = *++Next;
			if (Char == '=') {
				Parser->Token = MLT_ASSIGN;
				Parser->Next = Next + 1;
				return Parser->Token;
			} else if (Char == ':') {
				Parser->Token = MLT_IMPORT;
				Char = *++Next;
				if (ml_isidchar(Char)) {
					const char *End = Next + 1;
					while (ml_isidchar(*End)) ++End;
					int Length = End - Next;
					//char *Ident = snew(Length + 1);
					//memcpy(Ident, Next, Length);
					//Ident[Length] = 0;
					//Parser->Ident = Ident;
					Parser->Ident = ml_ident(Next, Length);
					Parser->Next = End;
				} else if (Char == '\"') {
					Parser->Next = Next + 1;
					ml_scan_string(Parser);
				} else if (ml_isoperator(Char)) {
					const char *End = Next + 1;
					while (ml_isoperator(*End)) ++End;
					int Length = End - Next;
					//char *Operator = snew(Length + 1);
					//memcpy(Operator, Next, Length);
					//Operator[Length] = 0;
					//Parser->Ident = Operator;
					Parser->Ident = ml_ident(Next, Length);
					Parser->Next = End;
				} else {
					Parser->Next = Next;
					Parser->Ident = "::";
					Parser->Token = MLT_OPERATOR;
				}
				return Parser->Token;
			} else if (ml_isidchar(Char)) {
				const char *End = Next + 1;
				while (ml_isidchar(*End)) ++End;
				int Length = End - Next;
				//char *Ident = snew(Length + 1);
				//memcpy(Ident, Next, Length);
				//Ident[Length] = 0;
				//Parser->Ident = Ident;
				Parser->Ident = ml_ident(Next, Length);
				Parser->Token = MLT_METHOD;
				Parser->Next = End;
				return Parser->Token;
			} else if (Char == '\"') {
				Parser->Next = Next + 1;
				ml_scan_string(Parser);
				Parser->Token = MLT_METHOD;
				return Parser->Token;
			} else if (Char == '-') {
				Parser->Token = MLT_NAMED;
				Parser->Next = Next + 1;
				return Parser->Token;
			} else if (Char == '\\') {
				Parser->Ident = "";
				Parser->Token = MLT_ESCAPE;
				Parser->Next = Next + 1;
				return Parser->Token;
			} else if (Char == '>') {
				const char *End = Next + 1;
				while (End[0] && End[0] != '\n') ++End;
				Next = End;
				continue;
			} else if (Char == '<') {
				++Next;
				int Level = 1;
				do {
					switch (*Next++) {
					case '\n':
						++Parser->Line;
						break;
					case 0:
						Next = ml_parser_do_read(Parser);
						if (!Next) {
							Parser->Next = Next = "";
							ml_parse_warn(Parser, "ParseError", "End of input in comment");
							Level = 0;
						}
						break;
					case '>':
						if (Next[0] == ':') {
							++Next;
							--Level;
						}
						break;
					case ':':
						if (Next[0] == '<') {
							++Next;
							++Level;
						}
						break;
					}
				} while (Level);
				continue;
			} else if (Char == '(') {
				Parser->Token = MLT_INLINE;
				Parser->Next = Next + 1;
				return Parser->Token;
			} else if (Char == '$') {
				Parser->Token = MLT_EXPAND;
				Parser->Next = Next + 1;
				return Parser->Token;
			} else if (Char == '{') {
				Parser->Token = MLT_EXPR_VALUE;
				Parser->Next = Next + 1;
				return Parser->Token;
			} else {
				Parser->Token = MLT_COLON;
				Parser->Next = Next;
				return Parser->Token;
			}
		}
		DO_CHAR_DELIM:
			Parser->Next = Next + 1;
			Parser->Token = CharTokens[(unsigned char)Char];
			return Parser->Token;
		DO_CHAR_OPER: {
			if (Char == '-' || Char == '.') {
				if (ml_isdigit(Next[1])) goto DO_CHAR_DIGIT;
			}
			const char *End = Next + 1;
			while (ml_isoperator(*End)) ++End;
			int Length = End - Next;
			//char *Operator = snew(Length + 1);
			//memcpy(Operator, Next, Length);
			//Operator[Length] = 0;
			//Parser->Ident = Operator;
			Parser->Ident = ml_ident(Next, Length);
			Parser->Token = MLT_OPERATOR;
			Parser->Next = End;
			return Parser->Token;
		}
		DO_CHAR_OTHER: {
			ml_parse_warn(Parser, "ParseError", "Unexpected character <%c>", Char);
			++Next;
		}
	}
	return Parser->Token;
}

static inline ml_token_t ml_current(ml_parser_t *Parser) {
	if (Parser->Token == MLT_NONE) ml_scan(Parser);
	return Parser->Token;
}

static inline ml_token_t ml_current2(ml_parser_t *Parser) {
	if (Parser->Token == MLT_NONE) ml_scan(Parser);
	while (Parser->Token == MLT_EOL) ml_scan(Parser);
	return Parser->Token;
}

static inline void ml_next(ml_parser_t *Parser) {
	Parser->Token = MLT_NONE;
	Parser->Source.Line = Parser->Line;
}

static inline int ml_parse(ml_parser_t *Parser, ml_token_t Token) {
	if (Parser->Token == MLT_NONE) ml_scan(Parser);
	if (Parser->Token == Token) {
		Parser->Token = MLT_NONE;
		Parser->Source.Line = Parser->Line;
		return 1;
	} else {
		return 0;
	}
}

static inline void ml_skip_eol(ml_parser_t *Parser) {
	if (Parser->Token == MLT_NONE) ml_scan(Parser);
	while (Parser->Token == MLT_EOL) ml_scan(Parser);
}

static inline int ml_parse2(ml_parser_t *Parser, ml_token_t Token) {
	if (Parser->Token == MLT_NONE) ml_scan(Parser);
	while (Parser->Token == MLT_EOL) ml_scan(Parser);
	if (Parser->Token == Token) {
		Parser->Token = MLT_NONE;
		Parser->Source.Line = Parser->Line;
		return 1;
	} else {
		return 0;
	}
}

void ml_accept(ml_parser_t *Parser, ml_token_t Token) {
	if (ml_parse2(Parser, Token)) return;
	if (Parser->Token == MLT_IDENT) {
		ml_parse_warn(Parser, "ParseError", "Expected %s not %s (%s)", MLTokens[Token], MLTokens[Parser->Token], Parser->Ident);
	} else {
		ml_parse_warn(Parser, "ParseError", "Expected %s not %s", MLTokens[Token], MLTokens[Parser->Token]);
	}
	if (Token == MLT_IDENT) {
		Parser->Token = MLT_NONE;
		Parser->Ident = "?";
	}
}

static void ml_accept_eoi(ml_parser_t *Parser) {
	ml_accept(Parser, MLT_EOI);
}

static mlc_expr_t *ml_parse_factor(ml_parser_t *Parser, int MethDecl);
static mlc_expr_t *ml_parse_term(ml_parser_t *Parser, int MethDecl);

static mlc_expr_t *ml_accept_fun_expr(ml_parser_t *Parser, const char *Name, ml_token_t EndToken) {
	ML_EXPR(FunExpr, fun, fun);
	FunExpr->Name = Name;
	FunExpr->Source = Parser->Source.Name;
	mlc_expr_t **BodySlot = &FunExpr->Body;
	if (!ml_parse2(Parser, EndToken)) {
		mlc_param_t **ParamSlot = &FunExpr->Params;
		int Index = 0;
		do {
			mlc_param_t *Param = ParamSlot[0] = new(mlc_param_t);
			Param->Line = Parser->Source.Line;
			ParamSlot = &Param->Next;
			if (ml_parse2(Parser, MLT_LEFT_SQUARE)) {
				ml_accept(Parser, MLT_IDENT);
				Param->Ident = Parser->Ident;
				Param->Kind = ML_PARAM_EXTRA;
				ml_accept(Parser, MLT_RIGHT_SQUARE);
				if (ml_parse2(Parser, MLT_COMMA)) {
					ml_accept(Parser, MLT_LEFT_BRACE);
					mlc_param_t *Param = ParamSlot[0] = new(mlc_param_t);
					Param->Line = Parser->Source.Line;
					ml_accept(Parser, MLT_IDENT);
					Param->Ident = Parser->Ident;
					Param->Kind = ML_PARAM_NAMED;
					ml_accept(Parser, MLT_RIGHT_BRACE);
				}
				break;
			} else if (ml_parse2(Parser, MLT_LEFT_BRACE)) {
				ml_accept(Parser, MLT_IDENT);
				Param->Ident = Parser->Ident;
				Param->Kind = ML_PARAM_NAMED;
				ml_accept(Parser, MLT_RIGHT_BRACE);
				break;
			} else {
				if (ml_parse2(Parser, MLT_BLANK)) {
					Param->Ident = "_";
				} else {
					if (ml_parse2(Parser, MLT_REF)) {
						Param->Kind = ML_PARAM_BYREF;
					} else if (ml_parse2(Parser, MLT_VAR)) {
						Param->Kind = ML_PARAM_ASVAR;
					} else if (ml_parse2(Parser, MLT_LET)) {
					}
					if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
						GC_asprintf((char **)&Param->Ident, "(%d)", Index);
						ML_EXPR(UnpackExpr, block, block);
						int Count = 1;
						ml_accept(Parser, MLT_IDENT);
						mlc_local_t *Local = UnpackExpr->Lets = mlc_local_new(Parser->Ident, Parser->Source.Line);
						while (ml_parse2(Parser, MLT_COMMA)) {
							ml_accept(Parser, MLT_IDENT);
							Local = Local->Next = mlc_local_new(Parser->Ident, Parser->Source.Line);
							Local->Index = Count++;
						}
						ml_accept(Parser, MLT_RIGHT_PAREN);
						ML_EXPR(IdentExpr, ident, ident);
						IdentExpr->Ident = Param->Ident;
						ML_EXPR(LocalExpr, local, let_unpack);
						if (Param->Kind == ML_PARAM_BYREF) {
							LocalExpr->compile = ml_ref_unpack_expr_compile;
						} else if (Param->Kind == ML_PARAM_ASVAR) {
							LocalExpr->compile = ml_var_unpack_expr_compile;
						}
						Param->Kind = 0;
						LocalExpr->Local = UnpackExpr->Lets;
						LocalExpr->Count = Count;
						LocalExpr->Child = ML_EXPR_END(IdentExpr);
						UnpackExpr->NumLets = Count;
						UnpackExpr->Child = ML_EXPR_END(LocalExpr);
						BodySlot[0] = ML_EXPR_END(UnpackExpr);
						BodySlot = &LocalExpr->Next;
					} else {
						ml_accept(Parser, MLT_IDENT);
						Param->Ident = Parser->Ident;
					}
				}
				if (ml_parse2(Parser, MLT_COLON)) Param->Type = ml_accept_term(Parser, 0);
				if (ml_parse2(Parser, MLT_ASSIGN)) {
					ML_EXPR(DefaultExpr, default, default);
					DefaultExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
					DefaultExpr->Index = Index;
					DefaultExpr->Flags = Param->Kind;
					BodySlot[0] = ML_EXPR_END(DefaultExpr);
					BodySlot = &DefaultExpr->Next;
				}
			}
			++Index;
		} while (ml_parse2(Parser, MLT_COMMA));
		ml_accept(Parser, EndToken);
	}
	mlc_expected_delimiter_t *Expected = Parser->ExpectedDelimiter;
	while (Expected && ml_current2(Parser) == Expected->Token) {
		ml_next(Parser);
		Expected->Token = MLT_NONE;
		Expected = Expected->Prev;
	}
	if (ml_parse2(Parser, MLT_COLON)) FunExpr->ReturnType = ml_accept_term(Parser, 0);
	mlc_expr_t *Body = BodySlot[0] = ml_accept_expression(Parser, EXPR_DEFAULT);
	FunExpr->StartLine = Body->StartLine;
	return ML_EXPR_END(FunExpr);
}

//extern ml_cfunctionx_t MLMethodSet[];

static mlc_expr_t *ml_accept_meth_expr(ml_parser_t *Parser) {
	ML_EXPR(MethodExpr, parent_value, const_call);
	//MethodExpr->Value = (ml_value_t *)MLMethodSet;
	MethodExpr->Value = MLMethodDefine;
	mlc_expr_t *Method = ml_accept_term(Parser, 1);
	if (!Method) {
		ml_parse_warn(Parser, "ParseError", "Expected <factor> not <%s>", MLTokens[Parser->Token]);
		Method = new(mlc_expr_t);
		Method->Source = Parser->Source.Name;
		Method->StartLine = Method->EndLine = Parser->Source.Line;
		Method->compile = ml_unknown_expr_compile;
	}
	MethodExpr->Child = Method;
	mlc_expr_t **ArgsSlot = &Method->Next;
	ml_accept(Parser, MLT_LEFT_PAREN);
	ML_EXPR(FunExpr, fun, fun);
	FunExpr->Source = Parser->Source.Name;
	if (!ml_parse2(Parser, MLT_RIGHT_PAREN)) {
		mlc_param_t **ParamSlot = &FunExpr->Params;
		do {
			if (ml_parse2(Parser, MLT_OPERATOR)) {
				if (!strcmp(Parser->Ident, "..")) {
					ML_EXPR(ValueExpr, value, value);
					ValueExpr->Value = ml_list();
					mlc_expr_t *Arg = ArgsSlot[0] = ML_EXPR_END(ValueExpr);
					ArgsSlot = &Arg->Next;
					break;
				} else {
					ml_parse_warn(Parser, "ParseError", "Expected <identfier> not %s (%s)", MLTokens[Parser->Token], Parser->Ident);
				}
			}
			mlc_param_t *Param = ParamSlot[0] = new(mlc_param_t);
			Param->Line = Parser->Source.Line;
			ParamSlot = &Param->Next;
			if (ml_parse2(Parser, MLT_LEFT_SQUARE)) {
				ml_accept(Parser, MLT_IDENT);
				Param->Ident = Parser->Ident;
				Param->Kind = ML_PARAM_EXTRA;
				mlc_expr_t *Arg;
				if (ml_parse2(Parser, MLT_COLON)) {
					ML_EXPR(ListExpr, parent, list);
					ListExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
					Arg = ML_EXPR_END(ListExpr);
				} else {
					ML_EXPR(ValueExpr, value, value);
					ValueExpr->Value = ml_list();
					Arg = ML_EXPR_END(ValueExpr);
				}
				ArgsSlot[0] = Arg;
				ArgsSlot = &Arg->Next;
				ml_accept(Parser, MLT_RIGHT_SQUARE);
				if (ml_parse2(Parser, MLT_COMMA)) {
					ml_accept(Parser, MLT_LEFT_BRACE);
					mlc_param_t *Param = ParamSlot[0] = new(mlc_param_t);
					Param->Line = Parser->Source.Line;
					ml_accept(Parser, MLT_IDENT);
					Param->Ident = Parser->Ident;
					Param->Kind = ML_PARAM_NAMED;
					ml_accept(Parser, MLT_RIGHT_BRACE);
				}
				break;
			} else if (ml_parse2(Parser, MLT_LEFT_BRACE)) {
				ml_accept(Parser, MLT_IDENT);
				Param->Ident = Parser->Ident;
				Param->Kind = ML_PARAM_NAMED;
				ml_accept(Parser, MLT_RIGHT_BRACE);
				ML_EXPR(ValueExpr, value, value);
				ValueExpr->Value = ml_list();
				mlc_expr_t *Arg = ArgsSlot[0] = ML_EXPR_END(ValueExpr);
				ArgsSlot = &Arg->Next;
				break;
			} else {
				if (ml_parse2(Parser, MLT_BLANK)) {
					Param->Ident = "_";
				} else {
					if (ml_parse2(Parser, MLT_REF)) {
						Param->Kind = ML_PARAM_BYREF;
					} else if (ml_parse2(Parser, MLT_VAR)) {
						Param->Kind = ML_PARAM_ASVAR;
					} else if (ml_parse2(Parser, MLT_LET)) {
					}
					ml_accept(Parser, MLT_IDENT);
					Param->Ident = Parser->Ident;
				}
				ml_accept(Parser, MLT_COLON);
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Parser, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
			}
		} while (ml_parse2(Parser, MLT_COMMA));
		ml_accept(Parser, MLT_RIGHT_PAREN);
	}
	if (ml_parse2(Parser, MLT_ASSIGN)) {
		ArgsSlot[0] = ml_accept_expression(Parser, EXPR_DEFAULT);
	} else {
		if (ml_parse2(Parser, MLT_COLON)) FunExpr->ReturnType = ml_accept_term(Parser, 0);
		FunExpr->Body = ml_accept_expression(Parser, EXPR_DEFAULT);
		ArgsSlot[0] = ML_EXPR_END(FunExpr);
	}
	return ML_EXPR_END(MethodExpr);
}

static void ml_accept_named_arguments(ml_parser_t *Parser, ml_token_t EndToken, mlc_expr_t **ArgsSlot, ml_value_t *Names) {
	mlc_expr_t **NamesSlot = ArgsSlot;
	mlc_expr_t *Arg = ArgsSlot[0];
	ArgsSlot = &Arg->Next;
	if (ml_parse2(Parser, MLT_SEMICOLON)) {
		ArgsSlot[0] = ml_accept_fun_expr(Parser, NULL, EndToken);
		return;
	}
	mlc_expected_delimiter_t Expected = {Parser->ExpectedDelimiter, EndToken};
	Parser->ExpectedDelimiter = &Expected;
	Arg = ArgsSlot[0] = ml_accept_expression(Parser, EXPR_DEFAULT);
	if (Expected.Token != EndToken) {
		Parser->ExpectedDelimiter = Expected.Prev;
		return;
	}
	ArgsSlot = &Arg->Next;
	while (ml_parse2(Parser, MLT_COMMA)) {
		ml_token_t Token = ml_current2(Parser);
		if (Token == MLT_IDENT) {
			ml_next(Parser);
			ml_names_add(Names, ml_string(Parser->Ident, -1));
		} else if (Token == MLT_VALUE && ml_typeof(Parser->Value) == MLStringT) {
			ml_next(Parser);
			ml_names_add(Names, Parser->Value);
		} else {
			ml_parse_warn(Parser, "ParseError", "Argument names must be identifiers or string");
			goto resume;
		}
		if (!ml_parse2(Parser, MLT_COLON)) ml_accept(Parser, MLT_IS);
		if (ml_parse2(Parser, MLT_SEMICOLON)) {
			Parser->ExpectedDelimiter = Expected.Prev;
			ArgsSlot[0] = ml_accept_fun_expr(Parser, NULL, EndToken);
			return;
		}
		Arg = ArgsSlot[0] = ml_accept_expression(Parser, EXPR_DEFAULT);
		if (Expected.Token != EndToken) {
			Parser->ExpectedDelimiter = Expected.Prev;
			return;
		}
		ArgsSlot = &Arg->Next;
	}
resume:
	Parser->ExpectedDelimiter = Expected.Prev;
	if (ml_parse2(Parser, MLT_SEMICOLON)) {
		mlc_expr_t *FunExpr = ml_accept_fun_expr(Parser, NULL, EndToken);
		FunExpr->Next = NamesSlot[0];
		NamesSlot[0] = FunExpr;
	} else {
		ml_accept(Parser, EndToken);
	}
}

static void ml_accept_arguments(ml_parser_t *Parser, ml_token_t EndToken, mlc_expr_t **ArgsSlot) {
	if (ml_parse2(Parser, MLT_SEMICOLON)) {
		ArgsSlot[0] = ml_accept_fun_expr(Parser, NULL, EndToken);
	} else if (!ml_parse2(Parser, EndToken)) {
		mlc_expected_delimiter_t Expected = {Parser->ExpectedDelimiter, EndToken};
		Parser->ExpectedDelimiter = &Expected;
		do {
			mlc_expr_t *Arg = ml_accept_expression(Parser, EXPR_DEFAULT);
			if (Expected.Token != EndToken) {
				ArgsSlot[0] = Arg;
				Parser->ExpectedDelimiter = Expected.Prev;
				return;
			}
			if (ml_parse2(Parser, MLT_COLON) || ml_parse2(Parser, MLT_IS)) {
				ml_value_t *Names = ml_names();
				if (Arg->compile == (void *)ml_ident_expr_compile) {
					ml_names_add(Names, ml_string(((mlc_ident_expr_t *)Arg)->Ident, -1));
				} else if (Arg->compile == (void *)ml_value_expr_compile) {
					ml_value_t *Name = ((mlc_value_expr_t *)Arg)->Value;
					if (ml_typeof(Name) != MLStringT) {
						ml_parse_warn(Parser, "ParseError", "Argument names must be identifiers or strings");
					}
					ml_names_add(Names, Name);
				} else {
					ml_parse_warn(Parser, "ParseError", "Argument names must be identifiers or strings");
				}
				ML_EXPR(NamesArg, value, value);
				NamesArg->Value = Names;
				ArgsSlot[0] = ML_EXPR_END(NamesArg);
				Parser->ExpectedDelimiter = Expected.Prev;
				return ml_accept_named_arguments(Parser, EndToken, ArgsSlot, Names);
			}
			ArgsSlot[0] = Arg;
			ArgsSlot = &Arg->Next;
		} while (ml_parse2(Parser, MLT_COMMA));
		Parser->ExpectedDelimiter = Expected.Prev;
		if (ml_parse2(Parser, MLT_SEMICOLON)) {
			ArgsSlot[0] = ml_accept_fun_expr(Parser, NULL, EndToken);
		} else {
			ml_accept(Parser, EndToken);
		}
	}
}

static mlc_if_case_t *ml_accept_if_case(ml_parser_t *Parser) {
	mlc_if_case_t *Case = new(mlc_if_case_t);
	Case->Local->Line = Parser->Source.Line;
	if (ml_parse2(Parser, MLT_VAR)) {
		Case->Token = MLT_VAR;
	} else if (ml_parse2(Parser, MLT_LET)) {
		Case->Token = MLT_LET;
	}
	if (Case->Token) {
		if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
			int Count = 1;
			if (!ml_parse2(Parser, MLT_BLANK)) ml_accept(Parser, MLT_IDENT);
			Case->Local->Line = Parser->Source.Line;
			Case->Local->Ident = Parser->Ident;
			mlc_local_t **LocalSlot = &Case->Local->Next;
			while (ml_parse2(Parser, MLT_COMMA)) {
				if (!ml_parse2(Parser, MLT_BLANK)) ml_accept(Parser, MLT_IDENT);
				++Count;
				mlc_local_t *Local = LocalSlot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
				LocalSlot = &Local->Next;
			}
			ml_accept(Parser, MLT_RIGHT_PAREN);
			Case->Local->Index = Count;
		} else {
			ml_accept(Parser, MLT_IDENT);
			Case->Local->Ident = Parser->Ident;
		}

		ml_accept(Parser, MLT_ASSIGN);
	}
	Case->Condition = ml_accept_expression(Parser, EXPR_DEFAULT);
	ml_accept(Parser, MLT_THEN);
	Case->Body = ml_accept_block(Parser);
	return Case;
}

static mlc_expr_t *ml_accept_with_expr(ml_parser_t *Parser, mlc_expr_t *Child) {
	ML_EXPR(WithExpr, local, with);
	mlc_local_t **LocalSlot = &WithExpr->Local;
	mlc_expr_t **ExprSlot = &WithExpr->Child;
	do {
		if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
			int Count = 0;
			mlc_local_t **First = LocalSlot;
			do {
				if (!ml_parse2(Parser, MLT_BLANK)) ml_accept(Parser, MLT_IDENT);
				++Count;
				mlc_local_t *Local = LocalSlot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
				LocalSlot = &Local->Next;
			} while (ml_parse2(Parser, MLT_COMMA));
			ml_accept(Parser, MLT_RIGHT_PAREN);
			First[0]->Index = Count;
		} else {
			ml_accept(Parser, MLT_IDENT);
			mlc_local_t *Local = LocalSlot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
			LocalSlot = &Local->Next;
			Local->Index = 0;
		}
		ml_accept(Parser, MLT_ASSIGN);
		mlc_expr_t *Expr = ExprSlot[0] = ml_accept_expression(Parser, EXPR_OR);
		ExprSlot = &Expr->Next;
	} while (ml_parse2(Parser, MLT_COMMA));
	if (Child) {
		ExprSlot[0] = Child;
	} else {
		ml_accept(Parser, MLT_DO);
		ExprSlot[0] = ml_accept_block(Parser);
		ml_accept(Parser, MLT_END);
	}
	return ML_EXPR_END(WithExpr);
}

static void ml_accept_for_decl(ml_parser_t *Parser, mlc_for_expr_t *Expr) {
	if (ml_parse2(Parser, MLT_IDENT)) {
		const char *Ident = Parser->Ident;
		if (ml_parse2(Parser, MLT_COMMA)) {
			Expr->Key = Ident;
		} else {
			Expr->Local = mlc_local_new(Ident, Parser->Source.Line);
			return;
		}
	}
	if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
		int Count = 0;
		mlc_local_t **Slot = &Expr->Local;
		do {
			if (!ml_parse2(Parser, MLT_BLANK)) ml_accept(Parser, MLT_IDENT);
			++Count;
			mlc_local_t *Local = Slot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
			Slot = &Local->Next;
		} while (ml_parse2(Parser, MLT_COMMA));
		ml_accept(Parser, MLT_RIGHT_PAREN);
		Expr->Unpack = Count;
	} else {
		ml_accept(Parser, MLT_IDENT);
		Expr->Local = mlc_local_new(Parser->Ident, Parser->Source.Line);
	}
}

typedef struct {
	ml_type_t *Type;
	ml_value_t **Iters;
	int *Keys, *Vars;
	int NumIters, NumVars;
} ml_compiler_zip_t;

ML_TYPE(MLCompilerZipT, (MLSequenceT), "compiler::zip");
//!internal

ML_FUNCTION(MLCompilerZip) {
//!internal
	int NumIters = Count / 2;
	ml_compiler_zip_t *Zip = new(ml_compiler_zip_t);
	Zip->Type = MLCompilerZipT;
	Zip->NumIters = NumIters;
	ml_value_t **Iters = Zip->Iters = anew(ml_value_t *, NumIters);
	int NumVars = 0;
	int *Keys = Zip->Keys = anew(int, NumIters);
	for (int I = 0; I < Count; I += 2) {
		int Flag = Keys[I / 2] = ml_integer_value(Args[I]);
		if (Flag & 1) NumVars += 1;
		if (Flag >> 1) {
			NumVars += Flag >> 1;
		} else {
			NumVars += 1;
		}
		Iters[I / 2] = Args[I + 1];
	}
	Zip->NumVars = NumVars;
	int *Vars = Zip->Vars = anew(int, 2 * NumVars);
	for (int I = 0; I < NumIters; ++I) {
		int Flag = Keys[I];
		Keys[I] = Flag & 1;
		int Unpack = Flag >> 1;
		if (Flag & 1) {
			*Vars++ = I;
			*Vars++ = -1;
		}
		if (Unpack) {
			for (int K = 1; K <= Unpack; ++K) {
				*Vars++ = I;
				*Vars++ = K;
			}
		} else {
			*Vars++ = I;
			*Vars++ = 0;
		}
	}
	return (ml_value_t *)Zip;
}

typedef struct {
	ml_state_t Base;
	int *Keys, *Vars;
	int Index, NumIters, NumVars;
	ml_value_t *Args[];
} ml_compiler_zip_state_t;

ML_TYPE(MLCompilerZipStateT, (), "compiler::zip::state");
//!internal

static void ml_compiler_zip_iter_key(ml_compiler_zip_state_t *State, ml_value_t *Result);

static void ml_compiler_zip_iter_value(ml_compiler_zip_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	int Index = State->Index;
	State->Args[3 * Index + 2] = Result;
	if (++Index < State->NumIters) {
		State->Index = Index;
		if (State->Keys[Index]) {
			State->Base.run = (ml_state_fn)ml_compiler_zip_iter_key;
			return ml_iter_key((ml_state_t *)State, State->Args[3 * Index]);
		} else {
			State->Base.run = (ml_state_fn)ml_compiler_zip_iter_value;
			return ml_iter_value((ml_state_t *)State, State->Args[3 * Index]);
		}
	}
	ML_RETURN(State);
}

static void ml_compiler_zip_iter_key(ml_compiler_zip_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	int Index = State->Index;
	State->Args[3 * Index + 1] = Result;
	State->Base.run = (ml_state_fn)ml_compiler_zip_iter_value;
	return ml_iter_value((ml_state_t *)State, State->Args[3 * Index]);
}

static void ml_compiler_zip_iter_next(ml_compiler_zip_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (Result == MLNil) ML_RETURN(Result);
	int Index = State->Index;
	State->Args[3 * Index] = Result;
	if (++Index < State->NumIters) {
		State->Index = Index;
		return ml_iter_next((ml_state_t *)State, State->Args[3 * Index]);
	}
	State->Index = 0;
	if (State->Keys[0]) {
		State->Base.run = (ml_state_fn)ml_compiler_zip_iter_key;
		return ml_iter_key((ml_state_t *)State, State->Args[0]);
	} else {
		State->Base.run = (ml_state_fn)ml_compiler_zip_iter_value;
		return ml_iter_value((ml_state_t *)State, State->Args[0]);
	}
}

static void ML_TYPED_FN(ml_iter_next, MLCompilerZipStateT, ml_state_t *Caller, ml_compiler_zip_state_t *State) {
	State->Index = 0;
	State->Base.run = (ml_state_fn)ml_compiler_zip_iter_next;
	return ml_iter_next((ml_state_t *)State, State->Args[0]);
}

static void ML_TYPED_FN(ml_iter_key, MLCompilerZipStateT, ml_state_t *Caller, ml_compiler_zip_state_t *State) {
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_value, MLCompilerZipStateT, ml_state_t *Caller, ml_compiler_zip_state_t *State) {
	ML_RETURN(State);
}

static ml_value_t *ML_TYPED_FN(ml_unpack, MLCompilerZipStateT, ml_compiler_zip_state_t *State, int Index) {
	if (--Index >= State->NumVars) return MLNil;
	int Iter = State->Vars[2 * Index];
	int Unpack = State->Vars[2 * Index + 1];
	if (!Unpack) {
		return State->Args[3 * Iter + 2];
	} else if (Unpack == -1) {
		return State->Args[3 * Iter + 1] ?: MLNil;
	} else {
		return ml_unpack(State->Args[3 * Iter + 2], Unpack);
	}
}

static void ml_compiler_zip_iterate(ml_compiler_zip_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (Result == MLNil) ML_RETURN(Result);
	int Index = State->Index;
	State->Args[3 * Index] = Result;
	if (++Index < State->NumIters) {
		State->Index = Index;
		return ml_iterate((ml_state_t *)State, State->Args[3 * Index]);
	}
	State->Index = 0;
	if (State->Keys[0]) {
		State->Base.run = (ml_state_fn)ml_compiler_zip_iter_key;
		return ml_iter_key((ml_state_t *)State, State->Args[0]);
	} else {
		State->Base.run = (ml_state_fn)ml_compiler_zip_iter_value;
		return ml_iter_value((ml_state_t *)State, State->Args[0]);
	}
}

static void ML_TYPED_FN(ml_iterate, MLCompilerZipT, ml_state_t *Caller, ml_compiler_zip_t *Zip) {
	ml_compiler_zip_state_t *State = xnew(ml_compiler_zip_state_t, Zip->NumIters * 3, ml_value_t *);
	State->Base.Type = MLCompilerZipStateT;
	State->Keys = Zip->Keys;
	State->Vars = Zip->Vars;
	int NumIters = State->NumIters = Zip->NumIters;
	State->NumVars = Zip->NumVars;
	State->Index = 0;
	for (int I = 0; I < NumIters; ++I) State->Args[3 * I] = Zip->Iters[I];
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_compiler_zip_iterate;
	return ml_iterate((ml_state_t *)State, State->Args[0]);
}

static void ml_accept_for_decls(ml_parser_t *Parser, mlc_for_expr_t *Expr) {
	ML_EXPR(CallExpr, parent_value, const_call);
	CallExpr->Value = (ml_value_t *)MLCompilerZip;
	if (Expr->Key) {
		mlc_local_t *Local = mlc_local_new(Expr->Key, Expr->StartLine);
		Local->Next = Expr->Local;
		Expr->Local = Local;
	}
	int Total = 0;
	mlc_local_t **LocalSlot = &Expr->Local;
	while (LocalSlot[0]) {
		++Total;
		LocalSlot = &LocalSlot[0]->Next;
	}
	ML_EXPR(FlagExpr, value, value);
	FlagExpr->Value = ml_integer(!!Expr->Key + (Expr->Unpack << 1));
	FlagExpr->Next = Expr->Sequence;
	CallExpr->Child = ML_EXPR_END(FlagExpr);
	mlc_expr_t **ArgSlot = &FlagExpr->Next->Next;
	Expr->Sequence = ML_EXPR_END(CallExpr);
	do {
		int Flag = 0, Done = 0;
		if (ml_parse2(Parser, MLT_IDENT)) {
			++Total;
			mlc_local_t *Local = mlc_local_new(Parser->Ident, Parser->Source.Line);
			LocalSlot[0] = Local;
			LocalSlot = &Local->Next;
			if (ml_parse2(Parser, MLT_COMMA)) {
				Flag = 1;
			} else {
				Done = 1;
			}
		}
		if (Done) {
		} else if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
			do {
				++Total;
				if (!ml_parse2(Parser, MLT_BLANK)) ml_accept(Parser, MLT_IDENT);
				Flag += 2;
				mlc_local_t *Local = LocalSlot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
				LocalSlot = &Local->Next;
			} while (ml_parse2(Parser, MLT_COMMA));
			ml_accept(Parser, MLT_RIGHT_PAREN);
		} else {
			++Total;
			ml_accept(Parser, MLT_IDENT);
			mlc_local_t *Local = mlc_local_new(Parser->Ident, Parser->Source.Line);
			LocalSlot[0] = Local;
			LocalSlot = &Local->Next;
		}
		ML_EXPR(FlagExpr, value, value);
		FlagExpr->Value = ml_integer(Flag);
		FlagExpr->Next = Expr->Sequence;
		ArgSlot[0] = ML_EXPR_END(FlagExpr);
		ml_accept(Parser, MLT_IN);
		FlagExpr->Next = ml_accept_expression(Parser, EXPR_DEFAULT);
		ArgSlot = &FlagExpr->Next->Next;
	} while (ml_parse2(Parser, MLT_SEMICOLON));
	Expr->Key = NULL;
	Expr->Unpack = Total;
	Expr->Sequence = ML_EXPR_END(CallExpr);
}

static ML_METHOD_DECL(MLInMethod, "in");
static ML_METHOD_DECL(MLIsMethod, "=");
extern ml_type_t MLFunctionSequenceT[];

ML_FUNCTION(MLNot) {
//@not
	ML_CHECK_ARG_COUNT(1);
	if (Args[0] == MLNil) return MLSome;
	return MLNil;
}

static mlc_expr_t *ml_parse_factor(ml_parser_t *Parser, int MethDecl) {
	static void *CompileFns[] = {
		[MLT_EACH] = ml_each_expr_compile,
		[MLT_NOT] = ml_not_expr_compile,
		[MLT_WHILE] = ml_or_expr_compile,
		[MLT_UNTIL] = ml_and_expr_compile,
		[MLT_EXIT] = ml_exit_expr_compile,
		[MLT_RET] = ml_return_expr_compile,
		[MLT_NEXT] = ml_next_expr_compile,
		[MLT_NIL] = ml_nil_expr_compile,
		[MLT_BLANK] = ml_blank_expr_compile,
		[MLT_OLD] = ml_old_expr_compile,
		[MLT_RECUR] = ml_recur_expr_compile,
		[MLT_IT] = ml_it_expr_compile
	};
	const char *ExprName = NULL;
with_name:
	switch (ml_current(Parser)) {
	case MLT_NOT: {
		ml_next(Parser);
		mlc_expr_t *Child = ml_parse_expression(Parser, EXPR_DEFAULT);
		if (Child) {
			ML_EXPR(ParentExpr, parent, not);
			ParentExpr->Child = Child;
			return ML_EXPR_END(ParentExpr);
		} else {
			ML_EXPR(ValueExpr, value, value);
			ValueExpr->Value = (ml_value_t *)MLNot;
			return ML_EXPR_END(ValueExpr);
		}
	}
	case MLT_EACH:
	{
		mlc_parent_expr_t *ParentExpr = new(mlc_parent_expr_t);
		ParentExpr->compile = CompileFns[Parser->Token];
		ml_next(Parser);
		ParentExpr->StartLine = Parser->Source.Line;
		ParentExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
		return ML_EXPR_END(ParentExpr);
	}
	case MLT_WHILE:
	case MLT_UNTIL:
	{
		mlc_parent_expr_t *ParentExpr = new(mlc_parent_expr_t);
		ParentExpr->compile = CompileFns[Parser->Token];
		ml_next(Parser);
		ParentExpr->StartLine = Parser->Source.Line;
		ParentExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
		ML_EXPR(ExitExpr, parent, exit);
		ExitExpr->Name = ExprName;
		if (ml_parse(Parser, MLT_COMMA)) {
			ExitExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
		} else {
			mlc_expr_t *RegisterExpr = new(mlc_expr_t);
			RegisterExpr->compile = ml_register_expr_compile;
			RegisterExpr->StartLine = RegisterExpr->EndLine = Parser->Source.Line;
			ExitExpr->Child = RegisterExpr;
		}
		ParentExpr->Child->Next = ML_EXPR_END(ExitExpr);
		return ML_EXPR_END(ParentExpr);
	}
	case MLT_EXIT:
	case MLT_RET:
	{
		mlc_parent_expr_t *ParentExpr = new(mlc_parent_expr_t);
		ParentExpr->compile = CompileFns[Parser->Token];
		ml_next(Parser);
		ParentExpr->StartLine = Parser->Source.Line;
		ParentExpr->Name = ExprName;
		ParentExpr->Child = ml_parse_expression(Parser, EXPR_DEFAULT);
		return ML_EXPR_END(ParentExpr);
	}
	case MLT_NEXT: {
		mlc_parent_expr_t *ParentExpr = new(mlc_parent_expr_t);
		ParentExpr->compile = CompileFns[Parser->Token];
		ml_next(Parser);
		ParentExpr->StartLine = Parser->Source.Line;
		ParentExpr->Name = ExprName;
		return ML_EXPR_END(ParentExpr);
	}
	case MLT_NIL:
	case MLT_BLANK:
	case MLT_OLD:
	case MLT_IT:
	case MLT_RECUR:
	{
		mlc_expr_t *Expr = new(mlc_expr_t);
		Expr->compile = CompileFns[Parser->Token];
		ml_next(Parser);
		Expr->Source = Parser->Source.Name;
		Expr->StartLine = Expr->EndLine = Parser->Source.Line;
		return Expr;
	}
	case MLT_DO: {
		ml_next(Parser);
		mlc_expr_t *BlockExpr = ml_accept_block(Parser);
		ml_accept(Parser, MLT_END);
		return BlockExpr;
	}
	case MLT_IF: {
		ml_next(Parser);
		ML_EXPR(IfExpr, if, if);
		mlc_if_case_t **CaseSlot = &IfExpr->Cases;
		do {
			mlc_if_case_t *Case = CaseSlot[0] = ml_accept_if_case(Parser);
			CaseSlot = &Case->Next;
		} while (ml_parse2(Parser, MLT_ELSEIF));
		if (ml_parse2(Parser, MLT_ELSE)) IfExpr->Else = ml_accept_block(Parser);
		ml_accept(Parser, MLT_END);
		return ML_EXPR_END(IfExpr);
	}
	case MLT_SWITCH: {
		ml_next(Parser);
		ML_EXPR(CaseExpr, parent, switch);
		mlc_expr_t *Child = ml_accept_expression(Parser, EXPR_DEFAULT);
		mlc_expr_t **CaseExprs = NULL;
		if (ml_parse(Parser, MLT_COLON)) {
			ML_EXPR(ProviderExpr, parent_value, const_call);
			ProviderExpr->Value = MLCompilerSwitch;
			ProviderExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
			ML_EXPR(InlineExpr, parent, inline);
			InlineExpr->Child = ML_EXPR_END(ProviderExpr);
			CaseExprs = &InlineExpr->Next;
			ML_EXPR(SwitchExpr, parent, call);
			SwitchExpr->Child = ML_EXPR_END(InlineExpr);
			SwitchExpr->Next = Child;
			ML_EXPR(CallExpr, parent, call);
			CallExpr->Child = ML_EXPR_END(SwitchExpr);
			Child = ML_EXPR_END(CallExpr);
		}
		CaseExpr->Child = Child;
		while (ml_parse2(Parser, MLT_CASE)) {
			if (CaseExprs) {
				ML_EXPR(ListExpr, parent, list);
				mlc_expr_t *ListChild = ListExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
				while (ml_parse(Parser, MLT_COMMA)) {
					ListChild = ListChild->Next = ml_accept_expression(Parser, EXPR_DEFAULT);
				}
				CaseExprs[0] = ML_EXPR_END(ListExpr);
				CaseExprs = &ListExpr->Next;
				ml_accept(Parser, MLT_DO);
			}
			Child = Child->Next = ml_accept_block(Parser);
		}
		if (ml_parse2(Parser, MLT_ELSE)) {
			Child->Next = ml_accept_block(Parser);
		} else {
			mlc_expr_t *NilExpr = new(mlc_expr_t);
			NilExpr->compile = ml_nil_expr_compile;
			NilExpr->StartLine = NilExpr->EndLine = Parser->Source.Line;
			Child->Next = NilExpr;
		}
		ml_accept(Parser, MLT_END);
		return ML_EXPR_END(CaseExpr);
	}
	case MLT_LOOP: {
		ml_next(Parser);
		ML_EXPR(LoopExpr, parent, loop);
		LoopExpr->Name = ExprName;
		LoopExpr->Child = ml_accept_block(Parser);
		ml_accept(Parser, MLT_END);
		return ML_EXPR_END(LoopExpr);
	}
	case MLT_FOR: {
		ml_next(Parser);
		ML_EXPR(ForExpr, for, for);
		ForExpr->Name = ExprName;
		ml_accept_for_decl(Parser, ForExpr);
		ml_accept(Parser, MLT_IN);
		ForExpr->Sequence = ml_accept_expression(Parser, EXPR_DEFAULT);
		if (ml_parse2(Parser, MLT_SEMICOLON)) ml_accept_for_decls(Parser, ForExpr);
		ml_accept(Parser, MLT_DO);
		ForExpr->Body = ml_accept_block(Parser);
		if (ml_parse2(Parser, MLT_ELSE)) {
			ForExpr->Body->Next = ml_accept_block(Parser);
		}
		ml_accept(Parser, MLT_END);
		return ML_EXPR_END(ForExpr);
	}
	case MLT_FUN: {
		ml_next(Parser);
		if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
			return ml_accept_fun_expr(Parser, NULL, MLT_RIGHT_PAREN);
		} else {
			ML_EXPR(FunExpr, fun, fun);
			FunExpr->Source = Parser->Source.Name;
			FunExpr->Body = ml_accept_expression(Parser, EXPR_DEFAULT);
			return ML_EXPR_END(FunExpr);
		}
	}
	case MLT_METH: {
		ml_next(Parser);
		return ml_accept_meth_expr(Parser);
	}
	case MLT_SEQ: {
		ml_next(Parser);
		ML_EXPR(FunExpr, fun, fun);
		FunExpr->Source = Parser->Source.Name;
		FunExpr->Body = ml_accept_expression(Parser, EXPR_DEFAULT);
		ML_EXPR(SequenceExpr, parent_value, const_call);
		SequenceExpr->Value = (ml_value_t *)MLFunctionSequenceT;
		SequenceExpr->Child = ML_EXPR_END(FunExpr);
		return ML_EXPR_END(SequenceExpr);
	}
	case MLT_SUSP: {
		ml_next(Parser);
		ML_EXPR(SuspendExpr, parent, suspend);
		SuspendExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
		if (ml_parse(Parser, MLT_COMMA)) {
			SuspendExpr->Child->Next = ml_accept_expression(Parser, EXPR_DEFAULT);
		}
		return ML_EXPR_END(SuspendExpr);
	}
	case MLT_WITH: {
		ml_next(Parser);
		return ml_accept_with_expr(Parser, NULL);
	}
	case MLT_IF_CONFIG: {
		ml_next(Parser);
		ml_accept(Parser, MLT_VALUE);
		const char *Config = "";
		if (ml_is(Parser->Value, MLStringT)) {
			Config = ml_string_value(Parser->Value);
		} else {
			ml_parse_warn(Parser, "ParserError", "Expected string not %s", MLTokens[Parser->Token]);
		}
		ML_EXPR(IfConfigExpr, if_config, if_config);
		IfConfigExpr->Config = Config;
		IfConfigExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
		return ML_EXPR_END(IfConfigExpr);
	}
	case MLT_IDENT: {
		ml_next(Parser);
		const char *Ident = Parser->Ident;
		if (ml_parse(Parser, MLT_NAMED)) {
			ExprName = Ident;
			goto with_name;
		}
		ML_EXPR(IdentExpr, ident, ident);
		IdentExpr->Ident = Ident;
		return ML_EXPR_END(IdentExpr);
	}
	case MLT_IMPORT: {
		ml_next(Parser);
		ML_EXPR(ValueExpr, value, value);
		ValueExpr->Value = ml_symbol(Parser->Ident);
		return ML_EXPR_END(ValueExpr);
	}
	case MLT_VALUE: {
		ml_next(Parser);
		ML_EXPR(ValueExpr, value, value);
		ValueExpr->Value = Parser->Value;
		return ML_EXPR_END(ValueExpr);
	}
	case MLT_EXPR: {
		ml_next(Parser);
		return Parser->Expr;
	}
	case MLT_ESCAPE: {
		ml_next(Parser);
		ml_parser_escape_t Escape = stringmap_search(Parser->EscapeFns, Parser->Ident);
		if (!Escape) {
			ml_parse_warn(Parser, "ParseError", "Unknown string prefix: %s", Parser->Ident);
		} else {
			ml_value_t *Value = Escape(Parser);
			if (ml_is(Value, MLExprT)) return (mlc_expr_t *)Value;
			if (ml_is_error(Value)) {
				ml_parse_warn(Parser, ml_error_type(Value), "%s", ml_error_message(Value));
			} else {
				ml_parse_warn(Parser, "ParseError", "Expected expression not %s", ml_typeof(Value)->Name);
			}
		}
		mlc_expr_t *Expr = new(mlc_expr_t);
		Expr->Source = Parser->Source.Name;
		Expr->StartLine = Expr->EndLine = Parser->Source.Line;
		Expr->compile = ml_unknown_expr_compile;
		return Expr;
	}
	case MLT_INLINE: {
		ml_next(Parser);
		ML_EXPR(InlineExpr, parent, inline);
		InlineExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
		ml_accept(Parser, MLT_RIGHT_PAREN);
		return ML_EXPR_END(InlineExpr);
	}
	case MLT_EXPAND: {
		ml_next(Parser);
		if (ml_parse(Parser, MLT_VALUE)) {
			ml_value_t *Value = Parser->Value;
			if (ml_is(Value, MLExprT)) return (mlc_expr_t *)Value;
			ml_parse_warn(Parser, "ParseError", "Expected expression not %s", ml_typeof(Value)->Name);
			mlc_expr_t *Expr = new(mlc_expr_t);
			Expr->Source = Parser->Source.Name;
			Expr->StartLine = Expr->EndLine = Parser->Source.Line;
			Expr->compile = ml_unknown_expr_compile;
			return Expr;
		}
		ml_accept(Parser, MLT_IDENT);
		ML_EXPR(DefineExpr, ident, define);
		DefineExpr->Ident = Parser->Ident;
		return ML_EXPR_END(DefineExpr);
	}
	case MLT_EXPR_VALUE: {
		ml_next(Parser);
		ML_EXPR(ValueExpr, value, value);
		ValueExpr->Value = ml_expr_value(ml_accept_expression(Parser, EXPR_DEFAULT));
		mlc_expr_t *Expr = ML_EXPR_END(ValueExpr);
		if (ml_parse(Parser, MLT_COMMA)) {
			ML_EXPR(CallExpr, parent_value, const_call);
			CallExpr->Value = ml_method("subst");
			CallExpr->Child = Expr;
			ml_accept_arguments(Parser, MLT_RIGHT_BRACE, &Expr->Next);
			Expr = ML_EXPR_END(CallExpr);
		} else {
			ml_accept(Parser, MLT_RIGHT_BRACE);
		}
		return Expr;
	}
	case MLT_LEFT_PAREN: {
		ml_next(Parser);
		if (ml_parse2(Parser, MLT_SEMICOLON)) {
			ML_EXPR(TupleExpr, parent, tuple);
			TupleExpr->Child = ml_accept_fun_expr(Parser, NULL, MLT_RIGHT_PAREN);
			return ML_EXPR_END(TupleExpr);
		}
		mlc_expr_t *Expr = ml_accept_expression(Parser, EXPR_DEFAULT);
		if (ml_parse2(Parser, MLT_COMMA)) {
			ML_EXPR(TupleExpr, parent, tuple);
			TupleExpr->Child = Expr;
			ml_accept_arguments(Parser, MLT_RIGHT_PAREN, &Expr->Next);
			Expr = ML_EXPR_END(TupleExpr);
		} else if (ml_parse2(Parser, MLT_SEMICOLON)) {
			ML_EXPR(TupleExpr, parent, tuple);
			TupleExpr->Child = Expr;
			Expr->Next = ml_accept_fun_expr(Parser, NULL, MLT_RIGHT_PAREN);
			Expr = ML_EXPR_END(TupleExpr);
		} else {
			ml_accept(Parser, MLT_RIGHT_PAREN);
		}
		return Expr;
	}
	case MLT_LEFT_SQUARE: {
		ml_next(Parser);
		ML_EXPR(ListExpr, parent, list);
		mlc_expr_t **ArgsSlot = &ListExpr->Child;
		if (!ml_parse2(Parser, MLT_RIGHT_SQUARE)) {
			do {
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Parser, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
			} while (ml_parse2(Parser, MLT_COMMA));
			ml_accept(Parser, MLT_RIGHT_SQUARE);
		}
		return ML_EXPR_END(ListExpr);
	}
	case MLT_LEFT_BRACE: {
		ml_next(Parser);
		ML_EXPR(MapExpr, parent, map);
		mlc_expr_t **ArgsSlot = &MapExpr->Child;
		if (!ml_parse2(Parser, MLT_RIGHT_BRACE)) {
			do {
				mlc_expr_t *Arg = ArgsSlot[0] = ml_accept_expression(Parser, EXPR_DEFAULT);
				ArgsSlot = &Arg->Next;
				if (ml_parse2(Parser, MLT_COLON) || ml_parse2(Parser, MLT_IS)) {
					mlc_expr_t *ArgExpr = ArgsSlot[0] = ml_accept_expression(Parser, EXPR_DEFAULT);
					ArgsSlot = &ArgExpr->Next;
				} else {
					ML_EXPR(ArgExpr, value, value);
					ArgExpr->Value = MLSome;
					ArgsSlot[0] = ML_EXPR_END(ArgExpr);
					ArgsSlot = &ArgExpr->Next;
				}
			} while (ml_parse2(Parser, MLT_COMMA));
			ml_accept(Parser, MLT_RIGHT_BRACE);
		}
		return ML_EXPR_END(MapExpr);
	}
	case MLT_OPERATOR: {
		ml_next(Parser);
		ml_value_t *Operator = ml_method(Parser->Ident);
		if (MethDecl) {
			ML_EXPR(ValueExpr, value, value);
			ValueExpr->Value = Operator;
			return ML_EXPR_END(ValueExpr);
		} else if (ml_parse(Parser, MLT_LEFT_PAREN)) {
			ML_EXPR(CallExpr, parent_value, const_call);
			CallExpr->Value = Operator;
			ml_accept_arguments(Parser, MLT_RIGHT_PAREN, &CallExpr->Child);
			return ML_EXPR_END(CallExpr);
		} else {
			mlc_expr_t *Child = ml_parse_term(Parser, 0);
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
		ml_next(Parser);
		ML_EXPR(ValueExpr, value, value);
		ValueExpr->Value = ml_method(Parser->Ident);
		return ML_EXPR_END(ValueExpr);
	}
	default: return NULL;
	}
}

static mlc_expr_t *ml_parse_term_postfix(ml_parser_t *Parser, int MethDecl, mlc_expr_t *Expr) {
	for (;;) {
		switch (ml_current(Parser)) {
		case MLT_LEFT_PAREN: {
			if (MethDecl) return Expr;
			ml_next(Parser);
			ML_EXPR(CallExpr, parent, call);
			CallExpr->Child = Expr;
			ml_accept_arguments(Parser, MLT_RIGHT_PAREN, &Expr->Next);
			Expr = ML_EXPR_END(CallExpr);
			break;
		}
		case MLT_LEFT_SQUARE: {
			ml_next(Parser);
			ML_EXPR(IndexExpr, parent_value, const_call);
			IndexExpr->Value = IndexMethod;
			IndexExpr->Child = Expr;
			ml_accept_arguments(Parser, MLT_RIGHT_SQUARE, &Expr->Next);
			Expr = ML_EXPR_END(IndexExpr);
			break;
		}
		case MLT_METHOD: {
			ml_next(Parser);
			ML_EXPR(CallExpr, parent_value, const_call);
			CallExpr->Value = ml_method(Parser->Ident);
			CallExpr->Child = Expr;
			if (ml_parse(Parser, MLT_LEFT_PAREN)) {
				ml_accept_arguments(Parser, MLT_RIGHT_PAREN, &Expr->Next);
			}
			Expr = ML_EXPR_END(CallExpr);
			break;
		}
		case MLT_IMPORT: {
			ml_next(Parser);
			ML_EXPR(ResolveExpr, parent_value, resolve);
			ResolveExpr->Value = ml_string(Parser->Ident, -1);
			ResolveExpr->Child = Expr;
			Expr = ML_EXPR_END(ResolveExpr);
			break;
		}
		case MLT_LEFT_BRACE: {
			ml_next(Parser);
			ML_EXPR(GuardExpr, parent, guard);
			GuardExpr->Child = Expr;
			mlc_expr_t *Guard = ml_parse_expression(Parser, EXPR_DEFAULT);
			if (!Guard) {
				Guard = new(mlc_expr_t);
				Guard->compile = ml_it_expr_compile;
				Guard->StartLine = Guard->EndLine = Parser->Source.Line;
			}
			Expr->Next = Guard;
			ml_accept(Parser, MLT_RIGHT_BRACE);
			Expr = ML_EXPR_END(GuardExpr);
			break;
		}
		default: {
			return Expr;
		}
		}
	}
	return NULL; // Unreachable
}

static mlc_expr_t *ml_parse_term(ml_parser_t *Parser, int MethDecl) {
	mlc_expr_t *Expr = ml_parse_factor(Parser, MethDecl);
	if (!Expr) return NULL;
	return ml_parse_term_postfix(Parser, MethDecl, Expr);
}

static mlc_expr_t *ml_accept_term(ml_parser_t *Parser, int MethDecl) {
	ml_skip_eol(Parser);
	mlc_expr_t *Expr = ml_parse_term(Parser, MethDecl);
	if (!Expr) {
		ml_parse_warn(Parser, "ParseError", "Expected <expression> not %s", MLTokens[Parser->Token]);
		Expr = new(mlc_expr_t);
		Expr->Source = Parser->Source.Name;
		Expr->StartLine = Expr->EndLine = Parser->Source.Line;
		Expr->compile = ml_unknown_expr_compile;
	}
	return Expr;
}

static mlc_expr_t *ml_parse_expression(ml_parser_t *Parser, ml_expr_level_t Level) {
	mlc_expr_t *Expr = ml_parse_term(Parser, 0);
	if (!Expr) return NULL;
	for (;;) switch (ml_current(Parser)) {
	case MLT_OPERATOR: case MLT_IDENT: case MLT_IN: {
		ml_next(Parser);
		ML_EXPR(CallExpr, parent_value, const_call);
		CallExpr->Value = ml_method(Parser->Ident);
		CallExpr->Child = Expr;
		if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
			//ml_accept_arguments(Parser, MLT_RIGHT_PAREN, &Expr->Next);
			mlc_expr_t *Expr2 = NULL;
			ml_accept_arguments(Parser, MLT_RIGHT_PAREN, &Expr2);
			if (Expr2) {
				if (Expr2->Next) {
					Expr->Next = Expr2;
				} else {
					Expr->Next = ml_parse_term_postfix(Parser, 0, Expr2);
				}
			}
		} else {
			Expr->Next = ml_accept_term(Parser, 0);
		}
		Expr = ML_EXPR_END(CallExpr);
		break;
	}
	case MLT_ASSIGN: {
		ml_next(Parser);
		ML_EXPR(AssignExpr, parent, assign);
		AssignExpr->Child = Expr;
		Expr->Next = ml_accept_expression(Parser, EXPR_DEFAULT);
		Expr = ML_EXPR_END(AssignExpr);
		break;
	}
	default: goto done;
	}
done:
	if (Level >= EXPR_AND && ml_parse(Parser, MLT_AND)) {
		ML_EXPR(AndExpr, parent, and);
		mlc_expr_t *LastChild = AndExpr->Child = Expr;
		do {
			LastChild = LastChild->Next = ml_accept_expression(Parser, EXPR_SIMPLE);
		} while (ml_parse(Parser, MLT_AND));
		Expr = ML_EXPR_END(AndExpr);
	}
	if (Level >= EXPR_OR && ml_parse(Parser, MLT_OR)) {
		ML_EXPR(OrExpr, parent, or);
		mlc_expr_t *LastChild = OrExpr->Child = Expr;
		do {
			LastChild = LastChild->Next = ml_accept_expression(Parser, EXPR_AND);
		} while (ml_parse(Parser, MLT_OR));
		Expr = ML_EXPR_END(OrExpr);
	}
	if (Level >= EXPR_FOR) {
		if (ml_parse(Parser, MLT_WITH)) {
			Expr = ml_accept_with_expr(Parser, Expr);
		}
		int IsComprehension = 0;
		if (ml_parse(Parser, MLT_TO)) {
			Expr->Next = ml_accept_expression(Parser, EXPR_OR);
			ml_accept(Parser, MLT_FOR);
			IsComprehension = 1;
		} else {
			IsComprehension = ml_parse(Parser, MLT_FOR);
		}
		if (IsComprehension) {
			ML_EXPR(FunExpr, fun, fun);
			FunExpr->Source = Parser->Source.Name;
			ML_EXPR(SuspendExpr, parent, suspend);
			SuspendExpr->Child = Expr;
			mlc_expr_t *Body = ML_EXPR_END(SuspendExpr);
			do {
				ML_EXPR(ForExpr, for, for);
				ml_accept_for_decl(Parser, ForExpr);
				ml_accept(Parser, MLT_IN);
				ForExpr->Sequence = ml_accept_expression(Parser, EXPR_OR);
				for (;;) {
					if (ml_parse2(Parser, MLT_IF)) {
						ML_EXPR(IfExpr, if, if);
						mlc_if_case_t *IfCase = IfExpr->Cases = new(mlc_if_case_t);
						IfCase->Condition = ml_accept_expression(Parser, EXPR_OR);
						IfCase->Body = Body;
						Body = ML_EXPR_END(IfExpr);
					} else if (ml_parse2(Parser, MLT_WITH)) {
						Body = ml_accept_with_expr(Parser, Body);
					} else {
						break;
					}
				}
				ForExpr->Body = Body;
				Body = ML_EXPR_END(ForExpr);
			} while (ml_parse2(Parser, MLT_FOR));
			FunExpr->Body = Body;
			FunExpr->StartLine = FunExpr->Body->StartLine;
			Expr = ML_EXPR_END(FunExpr);
		}
	}
	return Expr;
}

mlc_expr_t *ml_accept_expression(ml_parser_t *Parser, ml_expr_level_t Level) {
	ml_skip_eol(Parser);
	mlc_expr_t *Expr = ml_parse_expression(Parser, Level);
	if (!Expr) {
		ml_parse_warn(Parser, "ParseError", "Expected <expression> not %s", MLTokens[Parser->Token]);
		Expr = new(mlc_expr_t);
		Expr->Source = Parser->Source.Name;
		Expr->StartLine = Expr->EndLine = Parser->Source.Line;
		Expr->compile = ml_unknown_expr_compile;
	}
	return Expr;
}

typedef struct {
	mlc_expr_t **ExprSlot;
	mlc_local_t **VarsSlot;
	mlc_local_t **LetsSlot;
	mlc_local_t **DefsSlot;
} ml_accept_block_t;

static void ml_accept_block_var(ml_parser_t *Parser, ml_accept_block_t *Accept) {
	do {
		if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
			int Count = 0;
			mlc_local_t *Locals, **Slot = &Locals;
			do {
				if (!ml_parse2(Parser, MLT_BLANK)) ml_accept(Parser, MLT_IDENT);
				++Count;
				mlc_local_t *Local = Slot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
				Slot = &Local->Next;
			} while (ml_parse2(Parser, MLT_COMMA));
			Accept->VarsSlot[0] = Locals;
			Accept->VarsSlot = Slot;
			ml_accept(Parser, MLT_RIGHT_PAREN);
			if (ml_parse2(Parser, MLT_IN)) {
				ML_EXPR(LocalExpr, local, var_in);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			} else {
				ml_accept(Parser, MLT_ASSIGN);
				ML_EXPR(LocalExpr, local, var_unpack);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			}
		} else {
			ml_accept(Parser, MLT_IDENT);
			mlc_local_t *Local = Accept->VarsSlot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
			Accept->VarsSlot = &Local->Next;
			if (ml_parse(Parser, MLT_COLON)) {
				ML_EXPR(TypeExpr, local, var_type);
				TypeExpr->Local = Local;
				TypeExpr->Child = ml_accept_term(Parser, 0);
				Accept->ExprSlot[0] = ML_EXPR_END(TypeExpr);
				Accept->ExprSlot = &TypeExpr->Next;
			}
			mlc_expr_t *Child = NULL;
			if (ml_parse(Parser, MLT_LEFT_PAREN)) {
				Child = ml_accept_fun_expr(Parser, Local->Ident, MLT_RIGHT_PAREN);
			} else if (ml_parse(Parser, MLT_ASSIGN)) {
				Child = ml_accept_expression(Parser, EXPR_DEFAULT);
			}
			if (Child) {
				ML_EXPR(LocalExpr, local, var);
				LocalExpr->Local = Local;
				LocalExpr->Child = Child;
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			}
		}
	} while (ml_parse(Parser, MLT_COMMA));
}

static void ml_accept_block_let(ml_parser_t *Parser, ml_accept_block_t *Accept) {
	do {
		if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
			int Count = 0;
			mlc_local_t *Locals, **Slot = &Locals;
			do {
				if (!ml_parse2(Parser, MLT_BLANK)) ml_accept(Parser, MLT_IDENT);
				++Count;
				mlc_local_t *Local = Slot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
				Slot = &Local->Next;
			} while (ml_parse2(Parser, MLT_COMMA));
			Accept->LetsSlot[0] = Locals;
			Accept->LetsSlot = Slot;
			ml_accept(Parser, MLT_RIGHT_PAREN);
			if (ml_parse2(Parser, MLT_IN)) {
				ML_EXPR(LocalExpr, local, let_in);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			} else {
				ml_accept(Parser, MLT_ASSIGN);
				ML_EXPR(LocalExpr, local, let_unpack);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			}
		} else {
			ml_accept(Parser, MLT_IDENT);
			mlc_local_t *Local = Accept->LetsSlot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
			Accept->LetsSlot = &Local->Next;
			ML_EXPR(LocalExpr, local, let);
			if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
				LocalExpr->Child = ml_accept_fun_expr(Parser, Local->Ident, MLT_RIGHT_PAREN);
			} else {
				ml_accept(Parser, MLT_ASSIGN);
				LocalExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
			}
			LocalExpr->Local = Local;
			Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
			Accept->ExprSlot = &LocalExpr->Next;
		}
	} while (ml_parse(Parser, MLT_COMMA));
}

static void ml_accept_block_ref(ml_parser_t *Parser, ml_accept_block_t *Accept) {
	do {
		if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
			int Count = 0;
			mlc_local_t *Locals, **Slot = &Locals;
			do {
				if (!ml_parse2(Parser, MLT_BLANK)) ml_accept(Parser, MLT_IDENT);
				++Count;
				mlc_local_t *Local = Slot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
				Slot = &Local->Next;
			} while (ml_parse2(Parser, MLT_COMMA));
			Accept->LetsSlot[0] = Locals;
			Accept->LetsSlot = Slot;
			ml_accept(Parser, MLT_RIGHT_PAREN);
			if (ml_parse2(Parser, MLT_IN)) {
				ML_EXPR(LocalExpr, local, ref_in);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			} else {
				ml_accept(Parser, MLT_ASSIGN);
				ML_EXPR(LocalExpr, local, ref_unpack);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			}
		} else {
			ml_accept(Parser, MLT_IDENT);
			mlc_local_t *Local = Accept->LetsSlot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
			Accept->LetsSlot = &Local->Next;
			ML_EXPR(LocalExpr, local, ref);
			if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
				LocalExpr->Child = ml_accept_fun_expr(Parser, Local->Ident, MLT_RIGHT_PAREN);
			} else {
				ml_accept(Parser, MLT_ASSIGN);
				LocalExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
			}
			LocalExpr->Local = Local;
			Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
			Accept->ExprSlot = &LocalExpr->Next;
		}
	} while (ml_parse(Parser, MLT_COMMA));
}

static void ml_accept_block_def(ml_parser_t *Parser, ml_accept_block_t *Accept) {
	do {
		if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
			int Count = 0;
			mlc_local_t *Locals, **Slot = &Locals;
			do {
				if (!ml_parse2(Parser, MLT_BLANK)) ml_accept(Parser, MLT_IDENT);
				++Count;
				mlc_local_t *Local = Slot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
				Slot = &Local->Next;
			} while (ml_parse2(Parser, MLT_COMMA));
			Accept->DefsSlot[0] = Locals;
			Accept->DefsSlot = Slot;
			ml_accept(Parser, MLT_RIGHT_PAREN);
			if (ml_parse2(Parser, MLT_IN)) {
				ML_EXPR(LocalExpr, local, def_in);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			} else {
				ml_accept(Parser, MLT_ASSIGN);
				ML_EXPR(LocalExpr, local, def_unpack);
				LocalExpr->Local = Locals;
				LocalExpr->Count = Count;
				LocalExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
				Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
				Accept->ExprSlot = &LocalExpr->Next;
			}
		} else if (ml_parse2(Parser, MLT_VAR)) {
			ml_accept(Parser, MLT_IDENT);
			mlc_local_t *Local = Accept->DefsSlot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
			Accept->DefsSlot = &Local->Next;
			ML_EXPR(LocalExpr, local, def);
			ML_EXPR(CallExpr, parent_value, const_call);
			CallExpr->Value = (ml_value_t *)MLVariableT;
			mlc_expr_t *TypeExpr = ml_parse(Parser, MLT_COLON) ? ml_accept_term(Parser, 0) : NULL;
			if (ml_parse(Parser, MLT_ASSIGN)) {
				CallExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
			} else {
				mlc_expr_t *NilExpr = new(mlc_expr_t);
				NilExpr->compile = ml_nil_expr_compile;
				NilExpr->Source = Parser->Source.Name;
				NilExpr->StartLine = NilExpr->EndLine = Parser->Source.Line;
				CallExpr->Child = NilExpr;
			}
			CallExpr->Child->Next = TypeExpr;
			LocalExpr->Child = ML_EXPR_END(CallExpr);
			LocalExpr->Local = Local;
			Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
			Accept->ExprSlot = &LocalExpr->Next;
		} else {
			ml_accept(Parser, MLT_IDENT);
			mlc_local_t *Local = Accept->DefsSlot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
			Accept->DefsSlot = &Local->Next;
			ML_EXPR(LocalExpr, local, def);
			if (ml_parse2(Parser, MLT_LEFT_PAREN)) {
				LocalExpr->Child = ml_accept_fun_expr(Parser, Local->Ident, MLT_RIGHT_PAREN);
			} else {
				ml_accept(Parser, MLT_ASSIGN);
				LocalExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
			}
			LocalExpr->Local = Local;
			Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
			Accept->ExprSlot = &LocalExpr->Next;
		}
	} while (ml_parse(Parser, MLT_COMMA));
}

static void ml_accept_block_fun(ml_parser_t *Parser, ml_accept_block_t *Accept) {
	if (ml_parse2(Parser, MLT_IDENT)) {
		mlc_local_t *Local = Accept->LetsSlot[0] = mlc_local_new(Parser->Ident, Parser->Source.Line);
		Accept->LetsSlot = &Local->Next;
		ml_accept(Parser, MLT_LEFT_PAREN);
		ML_EXPR(LocalExpr, local, let);
		LocalExpr->Local = Local;
		LocalExpr->Child = ml_accept_fun_expr(Parser, Local->Ident, MLT_RIGHT_PAREN);
		Accept->ExprSlot[0] = ML_EXPR_END(LocalExpr);
		Accept->ExprSlot = &LocalExpr->Next;
	} else {
		ml_accept(Parser, MLT_LEFT_PAREN);
		mlc_expr_t *Expr = ml_accept_fun_expr(Parser, NULL, MLT_RIGHT_PAREN);
		Accept->ExprSlot[0] = Expr;
		Accept->ExprSlot = &Expr->Next;
	}
}

static mlc_expr_t *ml_accept_block_export(ml_parser_t *Parser, mlc_expr_t *Expr, mlc_local_t *Export) {
	ML_EXPR(CallExpr, parent, call);
	CallExpr->Child = Expr;
	ml_value_t *Names = ml_names();
	ML_EXPR(NamesExpr, value, value);
	NamesExpr->Value = Names;
	Expr->Next = ML_EXPR_END(NamesExpr);
	mlc_expr_t **ArgsSlot = &NamesExpr->Next;
	while (Export) {
		ml_names_add(Names, ml_string(Export->Ident, -1));
		ML_EXPR(IdentExpr, ident, ident);
		IdentExpr->Ident = Export->Ident;
		ArgsSlot[0] = ML_EXPR_END(IdentExpr);
		ArgsSlot = &IdentExpr->Next;
		Export = Export->Next;
	}
	return ML_EXPR_END(CallExpr);
}

static mlc_expr_t *ml_parse_block_expr(ml_parser_t *Parser, ml_accept_block_t *Accept) {
	mlc_expr_t *Expr = ml_parse_expression(Parser, EXPR_DEFAULT);
	if (!Expr) return NULL;
	if (ml_parse(Parser, MLT_COLON)) {
		if (ml_parse2(Parser, MLT_VAR)) {
			mlc_local_t **Exports = Accept->VarsSlot;
			ml_accept_block_var(Parser, Accept);
			Expr = ml_accept_block_export(Parser, Expr, Exports[0]);
		} else if (ml_parse2(Parser, MLT_LET)) {
			mlc_local_t **Exports = Accept->LetsSlot;
			ml_accept_block_let(Parser, Accept);
			Expr = ml_accept_block_export(Parser, Expr, Exports[0]);
		} else if (ml_parse2(Parser, MLT_REF)) {
			mlc_local_t **Exports = Accept->LetsSlot;
			ml_accept_block_ref(Parser, Accept);
			Expr = ml_accept_block_export(Parser, Expr, Exports[0]);
		} else if (ml_parse2(Parser, MLT_DEF)) {
			mlc_local_t **Exports = Accept->DefsSlot;
			ml_accept_block_def(Parser, Accept);
			Expr = ml_accept_block_export(Parser, Expr, Exports[0]);
		} else if (ml_parse2(Parser, MLT_FUN)) {
			mlc_local_t **Exports = Accept->LetsSlot;
			ml_accept_block_fun(Parser, Accept);
			Expr = ml_accept_block_export(Parser, Expr, Exports[0]);
		} else {
			ml_accept_block_t Previous = *Accept;
			mlc_expr_t *Child = ml_parse_block_expr(Parser, Accept);
			if (!Child) {
				ml_parse_warn(Parser, "ParseError", "Expected expression");
				return Expr;
			}
			if (Accept->VarsSlot != Previous.VarsSlot) {
				Accept->ExprSlot[0] = Child;
				Accept->ExprSlot = &Child->Next;
				Expr = ml_accept_block_export(Parser, Expr, Previous.VarsSlot[0]);
			} else if (Accept->LetsSlot != Previous.LetsSlot) {
				Accept->ExprSlot[0] = Child;
				Accept->ExprSlot = &Child->Next;
				Expr = ml_accept_block_export(Parser, Expr, Previous.LetsSlot[0]);
			} else if (Accept->DefsSlot != Previous.DefsSlot) {
				Accept->ExprSlot[0] = Child;
				Accept->ExprSlot = &Child->Next;
				Expr = ml_accept_block_export(Parser, Expr, Previous.DefsSlot[0]);
			} else {
				mlc_parent_expr_t *CallExpr = (mlc_parent_expr_t *)Child;
				if (CallExpr->compile != ml_call_expr_compile) {
					ml_parse_warn(Parser, "ParseError", "Invalid declaration");
					return Expr;
				}
				mlc_ident_expr_t *IdentExpr = (mlc_ident_expr_t *)CallExpr->Child;
				if (!IdentExpr || IdentExpr->compile != ml_ident_expr_compile) {
					ml_parse_warn(Parser, "ParseError", "Invalid declaration");
					return Expr;
				}
				mlc_local_t *Local = Accept->DefsSlot[0] = mlc_local_new(IdentExpr->Ident, IdentExpr->StartLine);
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

static mlc_block_expr_t *ml_accept_block_body(ml_parser_t *Parser) {
	ML_EXPR(BlockExpr, block, block);
	ml_accept_block_t Accept[1];
	Accept->ExprSlot = &BlockExpr->Child;
	Accept->VarsSlot = &BlockExpr->Vars;
	Accept->LetsSlot = &BlockExpr->Lets;
	Accept->DefsSlot = &BlockExpr->Defs;
	do {
		ml_skip_eol(Parser);
		switch (ml_current(Parser)) {
		case MLT_VAR: {
			ml_next(Parser);
			ml_accept_block_var(Parser, Accept);
			break;
		}
		case MLT_LET: {
			ml_next(Parser);
			ml_accept_block_let(Parser, Accept);
			break;
		}
		case MLT_REF: {
			ml_next(Parser);
			ml_accept_block_ref(Parser, Accept);
			break;
		}
		case MLT_DEF: {
			ml_next(Parser);
			ml_accept_block_def(Parser, Accept);
			break;
		}
		case MLT_FUN: {
			ml_next(Parser);
			ml_accept_block_fun(Parser, Accept);
			break;
		}
		case MLT_MUST: {
			ml_next(Parser);
			mlc_expr_t *Must = ml_accept_expression(Parser, EXPR_DEFAULT);
			mlc_block_expr_t *MustExpr = ml_accept_block_body(Parser);
			MustExpr->Must = Must;
			Accept->ExprSlot[0] = ML_EXPR_END(MustExpr);
			Accept->ExprSlot = &MustExpr->Next;
			goto finish;
		}
		default: {
			mlc_expr_t *Expr = ml_parse_block_expr(Parser, Accept);
			if (!Expr) goto finish;
			Accept->ExprSlot[0] = Expr;
			Accept->ExprSlot = &Expr->Next;
			break;
		}
		}
	} while (ml_parse(Parser, MLT_SEMICOLON) || ml_parse(Parser, MLT_EOL));
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

static mlc_expr_t *ml_accept_block(ml_parser_t *Parser) {
	mlc_block_expr_t *BlockExpr = ml_accept_block_body(Parser);
	if (ml_parse(Parser, MLT_ON)) {
		ml_accept(Parser, MLT_IDENT);
		BlockExpr->CatchIdent = Parser->Ident;
		ml_accept(Parser, MLT_DO);
		BlockExpr->CatchBody = ml_accept_block(Parser);
	}
	return ML_EXPR_END(BlockExpr);
}

const mlc_expr_t *ml_accept_file(ml_parser_t *Parser) {
	if (setjmp(Parser->OnError)) return NULL;
	const mlc_expr_t *Expr = ml_accept_block(Parser);
	ml_accept_eoi(Parser);
	return Expr;
}

mlc_expr_t *ml_accept_expr(ml_parser_t *Parser) {
	if (setjmp(Parser->OnError)) return NULL;
	ml_skip_eol(Parser);
	return ml_accept_expression(Parser, EXPR_DEFAULT);
}

static void ml_function_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_compile_frame_t *Frame) {
	ml_closure_info_t *Info = Frame->Info;
	const mlc_expr_t *Expr = Frame->Expr;
	Info->Return = MLC_EMIT(Expr->EndLine, MLI_RETURN, 0);
	MLC_LINK(Function->Returns, Info->Return);
	Info->Halt = Function->Next;
	Info->StartLine = Expr->StartLine;
	Info->EndLine = Expr->EndLine;
	Info->FrameSize = Function->Size;
	Info->Decls = Function->Decls;
	ml_state_t *Caller = Function->Base.Caller;
	ML_RETURN(ml_closure(Info));
}

void ml_function_compile(ml_state_t *Caller, const mlc_expr_t *Expr, ml_compiler_t *Compiler, const char **Parameters) {
	mlc_function_t *Function = new(mlc_function_t);
	Function->Base.Type = MLCompilerFunctionT;
	Function->Base.Caller = Caller;
	Function->Base.Context = Caller->Context;
	Function->Base.run = (ml_state_fn)mlc_function_run;
	Function->Compiler = Compiler;
	Function->Eval = 0;
	Function->Source = Expr->Source;
	Function->Old = -1;
	Function->It = -1;
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
	Info->NumParams = NumParams;
	Info->Source = Function->Source;
	Info->StartLine = Expr->StartLine;
	Info->EndLine = Expr->EndLine;
	GC_asprintf((char **)&Info->Name, "@%s:%d", Info->Source, Info->StartLine);
	Function->Next = Info->Entry = anew(ml_inst_t, 128);
	Function->Space = 126;
	Function->Returns = NULL;
	MLC_FRAME(mlc_compile_frame_t, ml_function_compile2);
	Frame->Info = Info;
	Frame->Expr = Expr;
	mlc_compile(Function, Expr, MLCF_RETURN);
}

ML_METHOD("permissive", MLParserT, MLBooleanT) {
//<Parser
//<Permissive
//>parser
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	if (ml_boolean_value(Args[1])) {
		Parser->Warnings = ml_list();
	} else {
		Parser->Warnings = NULL;
	}
	return (ml_value_t *)Parser;
}

ML_METHOD("warnings", MLParserT) {
//<Parser
//>parser
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	return Parser->Warnings ?: MLNil;
}

ML_METHOD("parse", MLParserT) {
//<Parser
//>expr
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	if (setjmp(Parser->OnError)) return Parser->Value;
	return ml_expr_value(ml_accept_expression(Parser, EXPR_DEFAULT));
}

ML_METHODX("compile", MLParserT, MLCompilerT) {
//<Parser
//<Compiler
//>any
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[1];
	const mlc_expr_t *Expr = ml_accept_file(Parser);
	if (!Expr) ML_RETURN(Parser->Value);
	return ml_function_compile(Caller, Expr, Compiler, NULL);
}

ML_METHODX("compile", MLParserT, MLCompilerT, MLListT) {
//<Parser
//<Compiler
//<Parameters
//>any
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[1];
	const mlc_expr_t *Expr = ml_accept_file(Parser);
	if (!Expr) ML_RETURN(Parser->Value);
	const char **Parameters = anew(const char *, ml_list_length(Args[2]) + 1);
	int I = 0;
	ML_LIST_FOREACH(Args[2], Iter) {
		if (!ml_is(Iter->Value, MLStringT)) ML_ERROR("TypeError", "Parameter name must be a string");
		Parameters[I++] = ml_string_value(Iter->Value);
	}
	return ml_function_compile(Caller, Expr, Compiler, Parameters);
}

ML_METHOD("source", MLParserT, MLStringT, MLIntegerT) {
//<Parser
//<Source
//<Line
//>tuple
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	ml_source_t Source = {ml_string_value(Args[1]), ml_integer_value(Args[2])};
	Source = ml_parser_source(Parser, Source);
	return ml_tuplev(2, ml_string(Source.Name, -1), ml_integer(Source.Line));
}

ML_METHOD("reset", MLParserT) {
//<Parser
//>parser
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	ml_parser_reset(Parser);
	return Args[0];
}

ML_METHOD("input", MLParserT, MLStringT) {
//<Parser
//<String
//>compiler
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	ml_parser_input(Parser, ml_string_value(Args[1]), 0);
	return Args[0];
}

ML_METHOD("input", MLParserT, MLStringT, MLIntegerT) {
//<Parser
//<String
//>compiler
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	ml_parser_input(Parser, ml_string_value(Args[1]), ml_integer_value(Args[2]));
	return Args[0];
}

static ml_value_t *ml_parser_escape_fn(ml_value_t *Callback) {
	return ml_simple_call(Callback, 0, NULL);
}

ML_METHOD("escape", MLParserT, MLFunctionT) {
//<Parser
//<Callback
//>parser
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	ml_parser_escape(Parser, (void *)ml_parser_escape_fn, Args[1]);
	return Args[0];
}

static ml_value_t *ml_parser_special_fn(ml_value_t *Callback) {
	return ml_simple_call(Callback, 0, NULL);
}

ML_METHOD("special", MLParserT, MLFunctionT) {
//<Parser
//<Callback
//>parser
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	ml_parser_special(Parser, (void *)ml_parser_special_fn, Args[1]);
	return Args[0];
}

ML_METHOD("special", MLParserT, MLListT) {
//<Parser
//<Callback
//>parser
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	ml_parser_special(Parser, (void *)ml_list_pop, Args[1]);
	return Args[0];
}

ML_METHOD("clear", MLParserT) {
//<Parser
//>string
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	return ml_string(ml_parser_clear(Parser), -1);
}

ML_METHODX("evaluate", MLParserT, MLCompilerT) {
//<Parser
//<Compiler
//>any
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[1];
	return ml_command_evaluate(Caller, Parser, Compiler);
}

typedef struct {
	ml_state_t Base;
	ml_parser_t *Parser;
	ml_compiler_t *Compiler;
} ml_evaluate_state_t;

static void ml_evaluate_state_run(ml_evaluate_state_t *State, ml_value_t *Value) {
	if (Value == MLEndOfInput) ML_CONTINUE(State->Base.Caller, MLNil);
	return ml_command_evaluate((ml_state_t *)State, State->Parser, State->Compiler);
}

ML_METHODX("run", MLParserT, MLCompilerT) {
//<Parser
//<Compiler
//>any
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[1];
	ml_evaluate_state_t *State = new(ml_evaluate_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_evaluate_state_run;
	State->Parser = Parser;
	State->Compiler = Compiler;
	return ml_command_evaluate((ml_state_t *)State, Parser, Compiler);
}

ML_METHODX("compile", MLExprT, MLCompilerT) {
//<Expr
//<Compiler
//>any
	mlc_expr_t *Expr = (mlc_expr_t *)Args[0];
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[1];
	return ml_function_compile(Caller, Expr, Compiler, NULL);
}

ML_METHODX("compile", MLExprT, MLCompilerT, MLListT) {
//<Expr
//<Compiler
//>any
	mlc_expr_t *Expr = (mlc_expr_t *)Args[0];
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[1];
	const char **Parameters = anew(const char *, ml_list_length(Args[2]) + 1);
	int I = 0;
	ML_LIST_FOREACH(Args[2], Iter) {
		if (!ml_is(Iter->Value, MLStringT)) ML_ERROR("TypeError", "Parameter name must be a string");
		Parameters[I++] = ml_string_value(Iter->Value);
	}
	return ml_function_compile(Caller, Expr, Compiler, Parameters);
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

ML_METHOD("var", MLCompilerT, MLStringT) {
//<Compiler
//<Name
//>variable
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t *Var = ml_variable(MLNil, NULL);
	stringmap_insert(Compiler->Vars, Name, Var);
	return Var;
}

ML_METHOD("var", MLCompilerT, MLStringT, MLTypeT) {
//<Compiler
//<Name
//<Type
//>variable
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t *Var = ml_variable(MLNil, (ml_type_t *)Args[2]);
	stringmap_insert(Compiler->Vars, Name, Var);
	return Var;
}

ML_METHOD("let", MLCompilerT, MLStringT, MLAnyT) {
//<Compiler
//<Name
//<Value
//>any
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	stringmap_insert(Compiler->Vars, Name, Args[2]);
	return Args[2];
}

ML_METHOD("def", MLCompilerT, MLStringT, MLAnyT) {
//<Compiler
//<Name
//<Value
//>any
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	stringmap_insert(Compiler->Vars, Name, Args[2]);
	return Args[2];
}

static int ml_compiler_var_fn(const char *Name, ml_value_t *Value, ml_value_t *Vars) {
	ml_map_insert(Vars, ml_string(Name, -1), ml_deref(Value));
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

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value;
	const char *Name;
} ml_global_t;

static ml_value_t *ml_global_deref(ml_global_t *Global) {
	//if (!Global->Value) return ml_error("NameError", "Identifier %s not declared", Global->Name);
	if (!Global->Value) return Global->Value = ml_uninitialized(Global->Name, (ml_source_t){"global", 0});
	return ml_deref(Global->Value);
}

static void ml_global_assign(ml_state_t *Caller, ml_global_t *Global, ml_value_t *Value) {
	if (!Global->Value) ML_ERROR("NameError", "Identifier %s not declared", Global->Name);
	return ml_assign(Caller, Global->Value, Value);
}

static void ml_global_call(ml_state_t *Caller, ml_global_t *Global, int Count, ml_value_t **Args) {
	if (!Global->Value) ML_ERROR("NameError", "Identifier %s not declared", Global->Name);
	return ml_call(Caller, Global->Value, Count, Args);
}

ML_TYPE(MLGlobalT, (), "global",
//!compiler
	.deref = (void *)ml_global_deref,
	.assign = (void *)ml_global_assign,
	.call = (void *)ml_global_call
);

static void ML_TYPED_FN(ml_value_find_all, MLGlobalT, ml_global_t *Global, void *Data, ml_value_find_fn RefFn) {
	if (!RefFn(Data, (ml_value_t *)Global, 1)) return;
	ml_value_find_all(Global->Value, Data, RefFn);
}

ml_value_t *ml_global(const char *Name) {
	ml_global_t *Global = new(ml_global_t);
	Global->Type =  MLGlobalT;
	Global->Name = Name;
	return (ml_value_t *)Global;
}

ml_value_t *ml_global_get(ml_value_t *Global) {
	return ((ml_global_t *)Global)->Value;
}

ml_value_t *ml_global_set(ml_value_t *Global, ml_value_t *Value) {
	return ((ml_global_t *)Global)->Value = Value;
}

static ml_value_t *ML_TYPED_FN(ml_unpack, MLGlobalT, ml_global_t *Global, int Index) {
	return ml_unpack(Global->Value, Index);
}

ML_METHOD("command_var", MLCompilerT, MLStringT) {
//<Compiler
//<Name
//>variable
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Compiler->Vars, Name);
	if (!Slot[0] || ml_typeof(Slot[0]) != MLGlobalT) {
		Slot[0] = ml_global(Name);
	}
	return ml_global_set(Slot[0], ml_variable(MLNil, NULL));
}

ML_METHOD("command_var", MLCompilerT, MLStringT, MLTypeT) {
//<Compiler
//<Name
//<Type
//>variable
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Compiler->Vars, Name);
	if (!Slot[0] || ml_typeof(Slot[0]) != MLGlobalT) {
		Slot[0] = ml_global(Name);
	}
	return ml_global_set(Slot[0], ml_variable(MLNil, (ml_type_t *)Args[2]));
}

ML_METHOD("command_let", MLCompilerT, MLStringT, MLAnyT) {
//<Compiler
//<Name
//<Value
//>any
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Compiler->Vars, Name);
	if (!Slot[0] || ml_typeof(Slot[0]) != MLGlobalT) {
		Slot[0] = ml_global(Name);
	}
	return ml_global_set(Slot[0], Args[2]);
}

ML_METHOD("command_def", MLCompilerT, MLStringT, MLAnyT) {
//<Compiler
//<Name
//<Value
//>any
	// TODO: Use a non-deref method to preserve reference values.
	ml_compiler_t *Compiler = (ml_compiler_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Compiler->Vars, Name);
	if (!Slot[0] || ml_typeof(Slot[0]) != MLGlobalT) {
		Slot[0] = ml_global(Name);
	}
	return ml_global_set(Slot[0], Args[2]);
}

static ml_global_t *ml_command_global(stringmap_t *Globals, const char *Name) {
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Globals, Name);
	if (!Slot[0]) {
		Slot[0] = ml_global(Name);
	} else if (ml_typeof(Slot[0]) == MLGlobalT) {
	} else if (ml_typeof(Slot[0]) == MLUninitializedT) {
		ml_value_t *Global = ml_global(Name);
		ml_uninitialized_set(Slot[0], Global);
		Slot[0] = Global;
	} else {
		Slot[0] = ml_global(Name);
	}
	return (ml_global_t *)Slot[0];
}

typedef struct {
	ml_value_t *Args[2];
	int Index;
	ml_token_t Type;
	ml_global_t *Globals[];
} ml_command_idents_frame_t;

static void ml_command_idents_in2(mlc_function_t *Function, ml_value_t *Value, ml_command_idents_frame_t *Frame) {
	if (ml_is_error(Value)) {
		ml_state_t *Caller = Function->Base.Caller;
		ML_RETURN(Value);
	}
	ml_global_t *Global = Frame->Globals[Frame->Index];
	if (Frame->Type == MLT_LET || Frame->Type == MLT_VAR) Value = ml_deref(Value);
	if (Frame->Type == MLT_VAR) {
		Value = ml_variable(Value, NULL);
	}
	Global->Value = Value;
	if (Frame->Index) {
		int Index = --Frame->Index;
		Frame->Args[1] = ml_string(Frame->Globals[Index]->Name, -1);
		return ml_call(Function, SymbolMethod, 2, Frame->Args);
	} else {
		MLC_POP();
		MLC_RETURN(Frame->Args[0]);
	}
}

static void ml_command_idents_in(mlc_function_t *Function, ml_value_t *Value, ml_command_idents_frame_t *Frame) {
	Frame->Args[0] = Value;
	Frame->Args[1] = ml_string(Frame->Globals[Frame->Index]->Name, -1);
	Function->Frame->run = (mlc_frame_fn)ml_command_idents_in2;
	return ml_call(Function, SymbolMethod, 2, Frame->Args);
}

static void ml_command_idents_unpack(mlc_function_t *Function, ml_value_t *Packed, ml_command_idents_frame_t *Frame) {
	for (int Index = 0; Index <= Frame->Index; ++Index) {
		ml_value_t *Value = ml_unpack(Packed, Index + 1);
		if (ml_is(Value, MLErrorT)) {
			MLC_POP();
			MLC_RETURN(Value);
		}
		ml_global_t *Global = Frame->Globals[Index];
		if (Frame->Type == MLT_LET || Frame->Type == MLT_VAR) Value = ml_deref(Value);
		if (Frame->Type == MLT_VAR) {
			Value = ml_variable(Value, NULL);
		}
		Global->Value = Value;
	}
	MLC_POP();
	MLC_RETURN(Packed);
}

static ml_command_idents_frame_t *ml_command_evaluate_idents(mlc_function_t *Function, ml_parser_t *Parser, int Index) {
	if (!ml_parse2(Parser, MLT_BLANK)) ml_accept(Parser, MLT_IDENT);
	const char *Ident = Parser->Ident;
	if (ml_parse(Parser, MLT_COMMA)) {
		ml_command_idents_frame_t *Frame = ml_command_evaluate_idents(Function, Parser, Index + 1);
		Frame->Globals[Index] = ml_command_global(Function->Compiler->Vars, Ident);
		return Frame;
	}
	ml_accept(Parser, MLT_RIGHT_PAREN);
	mlc_frame_fn FrameFn;
	if (ml_parse(Parser, MLT_IN)) {
		FrameFn = (mlc_frame_fn)ml_command_idents_in;
	} else {
		ml_accept(Parser, MLT_ASSIGN);
		FrameFn = (mlc_frame_fn)ml_command_idents_unpack;
	}
	int Count = Index + 1;
	MLC_XFRAME(ml_command_idents_frame_t, Count, const char *, FrameFn);
	Frame->Index = Index;
	Frame->Globals[Index] = ml_command_global(Function->Compiler->Vars, Ident);
	return Frame;
}

typedef struct {
	ml_global_t *Global;
	mlc_expr_t *VarType;
	ml_value_t *Value;
	ml_token_t Type;
} ml_command_ident_frame_t;

static void ml_command_var_type_run(mlc_function_t *Function, ml_value_t *Value, ml_command_ident_frame_t *Frame) {
	ml_state_t *Caller = Function->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (!ml_is(Value, MLTypeT)) ML_ERROR("TypeError", "Expected type not %s", ml_typeof(Value)->Name);
	ml_type_t *Type = (ml_type_t *)Value;
	if (!ml_is(Frame->Value, Type)) ML_ERROR("TypeError", "Cannot assign %s to variable of type %s", ml_typeof(Frame->Value)->Name, ml_type_name(Type));
	Value = ml_variable(Frame->Value, Type);
	ml_global_t *Global = Frame->Global;
	if (Global->Value && ml_typeof(Global->Value) == MLUninitializedT) {
		ml_uninitialized_set(Global->Value, Value);
	}
	Global->Value = Value;
	MLC_POP();
	MLC_RETURN(Value);
}

static void ml_command_ident_run(mlc_function_t *Function, ml_value_t *Value, ml_command_ident_frame_t *Frame) {
	if (ml_is_error(Value)) {
		ml_state_t *Caller = Function->Base.Caller;
		ML_RETURN(Value);
	}
	/*ml_compiler_t *Compiler = Function->Compiler;
	if (!ml_is(VarType, MLTypeT)) {
		ml_parse_error(Compiler, "TypeError", "Expected <type> not <%s>", ml_typeof(VarType)->Name);
	}*/
	ml_global_t *Global = Frame->Global;
	if (Frame->Type == MLT_LET || Frame->Type == MLT_VAR) Value = ml_deref(Value);
	switch (Frame->Type) {
	case MLT_VAR:
		if (Frame->VarType) {
			Function->Frame->run = (mlc_frame_fn)ml_command_var_type_run;
			Frame->Value = Value;
			return mlc_expr_call(Function, Frame->VarType);
		}
		Value = ml_variable(Value, NULL);
		break;
	case MLT_LET:
	case MLT_DEF:
		ml_value_set_name(Value, Global->Name);
		break;
	default:
		break;
	}
	if (Global->Value && ml_typeof(Global->Value) == MLUninitializedT) {
		ml_uninitialized_set(Global->Value, Value);
	}
	Global->Value = Value;
	MLC_POP();
	MLC_RETURN(Value);
}

static void ml_command_evaluate_decl2(mlc_function_t *Function, ml_parser_t *Parser, ml_token_t Type) {
	if (ml_parse(Parser, MLT_LEFT_PAREN)) {
		ml_command_idents_frame_t *Frame = ml_command_evaluate_idents(Function, Parser, 0);
		Frame->Type = Type;
		mlc_expr_t *Expr = ml_accept_expression(Parser, EXPR_DEFAULT);
		return mlc_expr_call(Function, Expr);
	} else {
		MLC_FRAME(ml_command_ident_frame_t, ml_command_ident_run);
		ml_accept(Parser, MLT_IDENT);
		Frame->Global = ml_command_global(Function->Compiler->Vars, Parser->Ident);
		Frame->VarType = NULL;
		Frame->Type = Type;
		if (ml_parse(Parser, MLT_LEFT_PAREN)) {
			mlc_expr_t *Expr = ml_accept_fun_expr(Parser, Frame->Global->Name, MLT_RIGHT_PAREN);
			return mlc_expr_call(Function, Expr);
		} else {
			if (ml_parse(Parser, MLT_COLON)) Frame->VarType = ml_accept_term(Parser, 0);
			if (Type == MLT_VAR) {
				if (ml_parse(Parser, MLT_ASSIGN)) {
					mlc_expr_t *Expr = ml_accept_expression(Parser, EXPR_DEFAULT);
					return mlc_expr_call(Function, Expr);
				} else {
					return ml_command_ident_run(Function, MLNil, Frame);
				}
			} else {
				ml_accept(Parser, MLT_ASSIGN);
				mlc_expr_t *Expr = ml_accept_expression(Parser, EXPR_DEFAULT);
				return mlc_expr_call(Function, Expr);
			}
		}
	}
}

typedef struct {
	ml_parser_t *Parser;
	ml_token_t Type;
} ml_command_decl_frame_t;

static void ml_command_decl_run(mlc_function_t *Function, ml_value_t *Value, ml_command_decl_frame_t *Frame) {
	ml_parser_t *Parser = Frame->Parser;
	if (setjmp(Parser->OnError)) MLC_RETURN(Parser->Value);
	if (ml_parse(Parser, MLT_COMMA)) {
		return ml_command_evaluate_decl2(Function, Parser, Frame->Type);
	}
	ml_parse(Parser, MLT_SEMICOLON);
	MLC_POP();
	MLC_RETURN(Value);
}

static void ml_command_evaluate_decl(mlc_function_t *Function, ml_parser_t *Parser, ml_token_t Type) {
	MLC_FRAME(ml_command_decl_frame_t, ml_command_decl_run);
	Frame->Parser = Parser;
	Frame->Type = Type;
	return ml_command_evaluate_decl2(Function, Parser, Type);
}

static void ml_command_evaluate_fun(mlc_function_t *Function, ml_parser_t *Parser) {
	ml_compiler_t *Compiler = Function->Compiler;
	if (ml_parse(Parser, MLT_IDENT)) {
		while (ml_parse(Parser, MLT_COMMA)) {
			ml_command_global(Function->Compiler->Vars, Parser->Ident);
			ml_accept(Parser, MLT_IDENT);
		}
		if (ml_parse(Parser, MLT_SEMICOLON)) {
			ml_command_global(Function->Compiler->Vars, Parser->Ident);
			ML_CONTINUE(Function, MLNil);
		}
		MLC_FRAME(ml_command_ident_frame_t, ml_command_ident_run);
		Frame->Global = ml_command_global(Compiler->Vars, Parser->Ident);
		Frame->VarType = NULL;
		Frame->Type = MLT_DEF;
		ml_accept(Parser, MLT_LEFT_PAREN);
		mlc_expr_t *Expr = ml_accept_fun_expr(Parser, Frame->Global->Name, MLT_RIGHT_PAREN);
		ml_parse(Parser, MLT_SEMICOLON);
		return mlc_expr_call(Function, Expr);
	} else {
		ml_accept(Parser, MLT_LEFT_PAREN);
		mlc_expr_t *Expr = ml_accept_fun_expr(Parser, NULL, MLT_RIGHT_PAREN);
		ml_parse(Parser, MLT_SEMICOLON);
		return mlc_expr_call(Function, Expr);
	}
}

static void ml_command_evaluate_expr(mlc_function_t *Function, ml_parser_t *Parser) {
	ml_compiler_t *Compiler = Function->Compiler;
	mlc_expr_t *Expr = ml_accept_expression(Parser, EXPR_DEFAULT);
	if (ml_parse(Parser, MLT_COLON)) {
		ml_accept(Parser, MLT_IDENT);
		const char *Ident = Parser->Ident;
		ML_EXPR(CallExpr, parent, call);
		CallExpr->Child = Expr;
		ml_accept(Parser, MLT_LEFT_PAREN);
		ml_accept_arguments(Parser, MLT_RIGHT_PAREN, &Expr->Next);
		ml_parse(Parser, MLT_SEMICOLON);
		MLC_FRAME(ml_command_ident_frame_t, ml_command_ident_run);
		Frame->Global = ml_command_global(Compiler->Vars, Ident);
		Frame->VarType = NULL;
		Frame->Type = MLT_DEF;
		return mlc_expr_call(Function, ML_EXPR_END(CallExpr));
	} else {
		ml_parse(Parser, MLT_SEMICOLON);
		return mlc_expr_call(Function, Expr);
	}
}

static void ml_command_evaluate2(mlc_function_t *Function, ml_value_t *Value, void *Data) {
	ml_state_t *Caller = Function->Base.Caller;
	ML_RETURN(ml_deref(Value));
}

void ml_command_evaluate(ml_state_t *Caller, ml_parser_t *Parser, ml_compiler_t *Compiler) {
	mlc_function_t *Function = new(mlc_function_t);
	Function->Base.Type = MLCompilerFunctionT;
	Function->Base.Caller = (ml_state_t *)Caller;
	Function->Base.Context = Caller->Context;
	Function->Base.run = (ml_state_fn)mlc_function_run;
	Function->Compiler = Compiler;
	Function->Eval = 0;
	Function->Source = Parser->Source.Name;
	Function->Old = -1;
	Function->It = -1;
	Function->Up = NULL;
	__attribute__((unused)) MLC_FRAME(void, ml_command_evaluate2);
	if (setjmp(Parser->OnError)) MLC_RETURN(Parser->Value);
	ml_skip_eol(Parser);
	if (ml_parse(Parser, MLT_EOI)) {
		MLC_RETURN(MLEndOfInput);
	} else if (ml_parse(Parser, MLT_VAR)) {
		return ml_command_evaluate_decl(Function, Parser, MLT_VAR);
	} else if (ml_parse(Parser, MLT_LET)) {
		return ml_command_evaluate_decl(Function, Parser, MLT_LET);
	} else if (ml_parse(Parser, MLT_REF)) {
		return ml_command_evaluate_decl(Function, Parser, MLT_REF);
	} else if (ml_parse(Parser, MLT_DEF)) {
		return ml_command_evaluate_decl(Function, Parser, MLT_DEF);
	} else if (ml_parse(Parser, MLT_FUN)) {
		return ml_command_evaluate_fun(Function, Parser);
	} else {
		return ml_command_evaluate_expr(Function, Parser);
	}
}

/*
typedef struct {
	ml_type_t *Type;
	mlc_expr_t *Expr;
	mlc_expr_t *VarType;
	int NumIdents, Flags;
	const char *Idents[];
} ml_command_decl_t;

static ml_command_decl_t ml_accept_command_decls(ml_parser_t *Parser, int Index) {
	if (!ml_parse2(Parser, MLT_BLANK)) ml_accept(Parser, MLT_IDENT);
	const char *Ident = Parser->Ident;
	ml_command_decl_t *Decls;
	if (ml_parse(Parser, MLT_COMMA)) {
		Decls = ml_accept_command_decls(Parser, Index + 1);
	} else {
		ml_accept(Parser, MLT_RIGHT_PAREN);
		Decls = xnew(ml_command_decl_t, Index + 1, const char *);
		Decls->NumIdents = Index + 1;
	}
	Decls->Idents[Index] = Ident;
	return Decls;
}

#define COMMAND_DECL_VAR 1
#define COMMAND_DECL_LET 2
#define COMMAND_DECL_DEF 4
#define COMMAND_DECL_REF 8
#define COMMAND_DECL_UNPACK 16
#define COMMAND_DECL_IN 32

ML_TYPE(MLCommandDeclT, (), "command::decl");

static void ml_accept_command_decl(ml_state_t *Caller, ml_parser_t *Parser, ml_token_t Type) {
	int Flags;
	switch (Type) {
	case MLT_VAR: Flags = COMMAND_DECL_VAR; break;
	case MLT_LET: Flags = COMMAND_DECL_LET; break;
	case MLT_DEF: Flags = COMMAND_DECL_DEF; break;
	case MLT_REF: Flags = COMMAND_DECL_REF; break;
	}
	ml_command_decl_t *Decl;
	if (ml_parse(Parser, MLT_LEFT_PAREN)) {
		Decl = ml_accept_command_decls(Parser, 0);
		Decl->Type = MLCommandDeclT;
		if (ml_parse(Parser, MLT_IN)) {
			Decl->Flags = Flags | COMMAND_DECL_IN;
		} else {
			ml_accept(Parser, MLT_ASSIGN);
			Decl->Flags = Flags | COMMAND_DECL_UNPACK;
		}
		Decl->Expr = ml_accept_expression(Parser, EXPR_DEFAULT);
	} else {
		ml_accept(Parser, MLT_IDENT);
		Decl = xnew(ml_command_decl_t, 1, const char *);
		Decl->Type = MLCommandDeclT;
		Decl->Idents[0] = Parser->Ident;
		Decl->Flags = Flags;
		if (ml_parse(Parser, MLT_LEFT_PAREN)) {
			Decl->Expr = ml_accept_fun_expr(Parser, Decl->Idents[0], MLT_RIGHT_PAREN);
		} else {
			if (ml_parse(Parser, MLT_COLON)) Decl->VarType = ml_accept_term(Parser, 0);
			if (Type == MLT_VAR) {
				if (ml_parse(Parser, MLT_ASSIGN)) {
					Decl->Expr = ml_accept_expression(Parser, EXPR_DEFAULT);
				}
			} else {
				ml_accept(Parser, MLT_ASSIGN);
				Decl->Expr = ml_accept_expression(Parser, EXPR_DEFAULT);
			}
		}
	}
	if (ml_parse(Parser, MLT_COMMA)) {
		Parser->Token = Type;
	} else {
		ml_parse(Parser, MLT_SEMICOLON);
	}
	ML_RETURN(Decl);
}

void ml_accept_command_fun(Caller, Parser) {
	if (ml_parse(Parser, MLT_IDENT)) {
		while (ml_parse(Parser, MLT_COMMA)) {
			ml_command_global(Function->Compiler->Vars, Parser->Ident);
			ml_accept(Parser, MLT_IDENT);
		}
		if (ml_parse(Parser, MLT_SEMICOLON)) {
			ml_command_global(Function->Compiler->Vars, Parser->Ident);
			ML_CONTINUE(Function, MLNil);
		}
		MLC_FRAME(ml_command_ident_frame_t, ml_command_ident_run);
		Frame->Global = ml_command_global(Compiler->Vars, Parser->Ident);
		Frame->VarType = NULL;
		Frame->Type = MLT_DEF;
		ml_accept(Parser, MLT_LEFT_PAREN);
		mlc_expr_t *Expr = ml_accept_fun_expr(Parser, Frame->Global->Name, MLT_RIGHT_PAREN);
		ml_parse(Parser, MLT_SEMICOLON);
		return mlc_expr_call(Function, Expr);
	} else {
		ml_accept(Parser, MLT_LEFT_PAREN);
		mlc_expr_t *Expr = ml_accept_fun_expr(Parser, NULL, MLT_RIGHT_PAREN);
		ml_parse(Parser, MLT_SEMICOLON);
		return mlc_expr_call(Function, Expr);
	}
}

void ml_accept_command(ml_state_t *Caller, ml_parser_t *Parser) {
	if (setjmp(Parser->OnError)) ML_RETURN(Parser->Value);
	ml_skip_eol(Parser);
	if (ml_parse(Parser, MLT_EOI)) {
		ML_RETURN(MLEndOfInput);
	} else if (ml_parse(Parser, MLT_VAR)) {
		return ml_accept_command_decl(Caller, Parser, MLT_VAR);
	} else if (ml_parse(Parser, MLT_LET)) {
		return ml_accept_command_decl(Caller, Parser, MLT_LET);
	} else if (ml_parse(Parser, MLT_REF)) {
		return ml_accept_command_decl(Caller, Parser, MLT_REF);
	} else if (ml_parse(Parser, MLT_DEF)) {
		return ml_accept_command_decl(Caller, Parser, MLT_DEF);
	} else if (ml_parse(Parser, MLT_FUN)) {
		return ml_accept_command_fun(Caller, Parser);
	} else {
		return ml_accept_command_expr(Caller, Parser);
	}
}
*/

ml_value_t *ml_stringmap_global_get(const stringmap_t *Map, const char *Key, const char *Source, int Line, int Eval) {
	return (ml_value_t *)stringmap_search(Map, Key);
}

static ssize_t ml_read_line(FILE *File, ssize_t Offset, char **Result) {
	char Buffer[129];
	if (fgets(Buffer, 129, File) == NULL) return -1;
	int Length = strlen(Buffer);
	if (Length == 128) {
		ssize_t Total = ml_read_line(File, Offset + 128, Result);
		memcpy(*Result + Offset, Buffer, 128);
		return Total;
	} else {
		*Result = snew(Offset + Length + 1);
		strcpy(*Result + Offset, Buffer);
		return Offset + Length;
	}
}

const char *ml_load_file_read(void *Data) {
	FILE *File = (FILE *)Data;
	char *Line = NULL;
	size_t Length = 0;
	Length = ml_read_line(File, 0, &Line);
	if (Length < 0) return NULL;
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
	static const char *DefaultParameters[] = {"Args", NULL};
	if (!Parameters) Parameters = DefaultParameters;
	FILE *File = fopen(FileName, "r");
	if (!File) ML_ERROR("LoadError", "Error opening %s", FileName);
	ml_parser_t *Parser = ml_parser(ml_load_file_read, File);
	Parser->Source.Name = FileName;
	const char *Line = ml_load_file_read(File);
	if (!Line) ML_ERROR("LoadError", "Empty file %s", FileName);
	if (Line[0] == '#' && Line[1] == '!') {
		Parser->Line = 2;
		Line = ml_load_file_read(File);
		if (!Line) ML_ERROR("LoadError", "Empty file %s", FileName);
	} else {
		Parser->Line = 1;
	}
	Parser->Next = Line;
	const mlc_expr_t *Expr = ml_accept_file(Parser);
	if (!Expr) ML_RETURN(Parser->Value);
	ml_compiler_t *Compiler = ml_compiler(GlobalGet, Globals);
	ml_load_file_state_t *State = new(ml_load_file_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_load_file_state_run;
	State->File = File;
	return ml_function_compile((ml_state_t *)State, Expr, Compiler, Parameters);
}

static void ml_inline_call_macro_fn(ml_state_t *Caller, void *Value, int Count, ml_value_t **Args) {
	struct { ml_source_t Source; } Parser[1];
	if (Count) {
		mlc_expr_t *Expr = (mlc_expr_t *)Args[0];
		Parser->Source.Name = Expr->Source;
		Parser->Source.Line = Expr->StartLine;
	} else {
		Parser->Source.Name = "<macro>";
		Parser->Source.Line = 1;
	}
	ML_EXPR(CallExpr, parent_value, const_call);
	CallExpr->Value = (ml_value_t *)Value;
	mlc_expr_t **Slot = &CallExpr->Child;
	for (int I = 0; I < Count; ++I) {
		mlc_expr_t *Child = Slot[0] = ml_delegate_expr(Args[I]);
		Slot = &Child->Next;
	}
	ML_EXPR(InlineExpr, parent, inline);
	InlineExpr->Child = ML_EXPR_END(CallExpr);
	ML_RETURN(ml_expr_value(ML_EXPR_END(InlineExpr)));
}

ml_value_t *ml_inline_call_macro(ml_value_t *Value) {
	return ml_macro(ml_cfunctionx(Value, ml_inline_call_macro_fn));
}

ML_FUNCTION(MLIdentCacheCheck) {
	if (ml_ident_cache_check()) return ml_error("InternalError", "Compiler ident cache is corrupt");
	return MLNil;
}

void ml_compiler_init() {
#include "ml_compiler_init.c"
	stringmap_insert(MLParserT->Exports, "expr", MLExprT);
	stringmap_insert(MLCompilerT->Exports, "EOI", MLEndOfInput);
	stringmap_insert(MLCompilerT->Exports, "NotFound", MLNotFound);
	stringmap_insert(MLCompilerT->Exports, "switch", MLCompilerSwitch);
	stringmap_insert(MLCompilerT->Exports, "source", MLSourceInline);
	stringmap_insert(MLCompilerT->Exports, "_check_ident_cache", MLIdentCacheCheck);
	stringmap_insert(MLMacroT->Exports, "expr", ml_macro((ml_value_t *)MLValueExpr));
	stringmap_insert(MLMacroT->Exports, "subst", ml_macro((ml_value_t *)MLMacroSubst));
	stringmap_insert(MLMacroT->Exports, "ident", MLIdentExpr);
	stringmap_insert(MLMacroT->Exports, "value", MLValueExpr);
	stringmap_insert(MLMacroT->Exports, "fun", MLFunExpr);
	stringmap_insert(MLMacroT->Exports, "block", MLBlockBuilder);
	stringmap_insert(MLMacroT->Exports, "tuple", MLTupleBuilder);
	stringmap_insert(MLMacroT->Exports, "list", MLListBuilder);
	stringmap_insert(MLMacroT->Exports, "map", MLMapBuilder);
	stringmap_insert(MLMacroT->Exports, "call", MLCallBuilder);
	//ml_string_fn_register("r", ml_regex);
	//ml_string_fn_register("ri", ml_regexi);
}

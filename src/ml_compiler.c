#include "minilang.h"
#include "ml_macros.h"
#include "ml_compiler.h"
#include "stringmap.h"
#include "sha256.h"
#include <gc/gc.h>
#include "ml_runtime.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include "ml_compiler2.h"

struct mlc_upvalue_t {
	mlc_upvalue_t *Next;
	ml_decl_t *Decl;
	int Index;
};

struct mlc_try_t {
	mlc_try_t *Up;
	ml_inst_t *Retries;
	int Top;
};

#define ML_EXPR(EXPR, TYPE, COMP) \
	mlc_ ## TYPE ## _expr_t *EXPR = new(mlc_ ## TYPE ## _expr_t); \
	EXPR->compile = ml_ ## COMP ## _expr_compile; \
	EXPR->StartLine = EXPR->EndLine = Compiler->Source.Line

#define ML_EXPR_END(EXPR) (((mlc_expr_t *)EXPR)->EndLine = Compiler->Source.Line, (mlc_expr_t *)EXPR)

struct ml_compiler_t {
	ml_type_t *Type;
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
	ml_source_t Source;
	stringmap_t Vars[1];
	int Line;
	ml_token_t Token;
	jmp_buf OnError;
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

ML_TYPE(MLMacroT, (), "macro");

static void mlc_function_run(mlc_function_t *Function, ml_value_t *Value) {
	if (ml_is_error(Value) && !Function->Frame->AllowErrors) {
		ml_state_t *Caller = Function->Base.Caller;
		ML_RETURN(Value);
	}
	Function->Frame->run(Function, Value, Function->Frame->Data);
}

#define FRAME_BLOCK_SIZE 384

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

void *mlc_frame_alloc(mlc_function_t *Function, size_t Size, mlc_frame_fn run) {
	size_t FrameSize = sizeof(mlc_frame_t) + Size;
	FrameSize = (FrameSize + 7) & ~7;
	mlc_frame_t *Frame = (mlc_frame_t *)((void *)Function->Frame - FrameSize);
	if (!Function->Limit || (void *)Frame < Function->Limit) {
		size_t BlockSize = Size + sizeof(mlc_frame_t) + sizeof(void *);
		if (BlockSize < FRAME_BLOCK_SIZE) BlockSize = FRAME_BLOCK_SIZE;
		void *Limit = GC_malloc(BlockSize);
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

inline void mlc_compile(mlc_function_t *Function, mlc_expr_t *Expr, int Flags) {
	Expr->compile(Function, Expr, Flags);
}

inline void mlc_expr_error(mlc_function_t *Function, mlc_expr_t *Expr, ml_value_t *Error) {
	ml_error_trace_add(Error, (ml_source_t){Function->Source, Expr->StartLine});
	ML_CONTINUE(Function->Base.Caller, Error);
}

typedef struct {
	mlc_expr_t *Expr;
	ml_closure_info_t *Info;
} mlc_compile_frame_t;

static void mlc_expr_call2(mlc_function_t *Function, ml_value_t *Value, mlc_compile_frame_t *Frame) {
	ml_closure_info_t *Info = Frame->Info;
	mlc_expr_t *Expr = Frame->Expr;
	ml_state_t *Caller = Function->Base.Caller;
	if (Function->UpValues) {
		ml_value_t *Error = ml_error("EvalError", "Use of non-constant value in constant expression");
		ml_error_trace_add(Error, (ml_source_t){Function->Source, Expr->EndLine});
		ML_RETURN(Error);
	}
	Info->Return = MLC_EMIT(Expr->EndLine, MLI_RETURN, 0);
	MLC_LINK(Function->Returns, Info->Return);
	Info->Halt = Function->Next;
	Info->Source = Function->Source;
	Info->StartLine = Expr->StartLine;
	Info->EndLine = Expr->EndLine;
	Info->FrameSize = Function->Size;
	Info->NumParams = 0;
	ml_closure_t *Closure = new(ml_closure_t);
	Closure->Type = MLClosureT;
	Closure->Info = Info;
	MLC_POP();
	ml_call(Caller, (ml_value_t *)Closure, 0, NULL);
}

static void mlc_expr_call(mlc_function_t *Parent, mlc_expr_t *Expr) {
	mlc_function_t *Function = new(mlc_function_t);
	Function->Base.Caller = (ml_state_t *)Parent;
	Function->Base.Context = Parent->Base.Context;
	Function->Base.run = (ml_state_fn)mlc_function_run;
	Function->Compiler = Parent->Compiler;
	Function->Source = Parent->Source;
	Function->Up = Parent;
	Function->Size = 1;
	Function->Next = anew(ml_inst_t, 128);
	Function->Space = 126;
	Function->Returns = NULL;
	MLC_FRAME(mlc_compile_frame_t, mlc_expr_call2);
	Frame->Expr = Expr;
	Frame->Info = new(ml_closure_info_t);
	Frame->Info->Entry = Function->Next;
	mlc_compile(Function, Expr, 0);
}

static void ml_register_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr, int Flags) {
	if (Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->StartLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_RETURN(NULL);
}

extern ml_value_t MLBlank[];

static void ml_blank_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr, int Flags) {
	ml_inst_t *Inst = MLC_EMIT(Expr->StartLine, MLI_LOAD, 1);
	Inst[1].Value = MLBlank;
	if (Flags & MLCF_PUSH) {
		Inst->Opcode = MLI_LOAD_PUSH;
		mlc_inc_top(Function);
	}
	MLC_RETURN(NULL);
}

static void ml_nil_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr, int Flags) {
	ml_inst_t *Inst = MLC_EMIT(Expr->StartLine, MLI_NIL, 0);
	if (Flags & MLCF_PUSH) {
		Inst->Opcode = MLI_NIL_PUSH;
		mlc_inc_top(Function);
	}
	MLC_RETURN(NULL);
}

static void ml_value_expr_compile(mlc_function_t *Function, mlc_value_expr_t *Expr, int Flags) {
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
	int Flags;
} mlc_if_expr_frame_t;

static void ml_if_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_if_expr_frame_t *Frame);

static void ml_if_expr_compile4(mlc_function_t *Function, ml_value_t *Value, mlc_if_expr_frame_t *Frame) {
	mlc_if_expr_t *Expr = Frame->Expr;
	MLC_LINK(Frame->Exits, Function->Next);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_if_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_if_expr_frame_t *Frame) {
	mlc_if_case_t *Case = Frame->Case;
	if (Case->Ident) {
		Function->Decls = Frame->Decls;
		--Function->Top;
		ml_inst_t *ExitInst = MLC_EMIT(Case->Body->EndLine, MLI_EXIT, 2);
		ExitInst[1].Count = 1;
		ExitInst[2].Decls = Function->Decls;
	}
	mlc_if_expr_t *Expr = Frame->Expr;
	if (Case->Next || Expr->Else) {
		//if (!(Result & MLCF_RETURN)) {
		ml_inst_t *GotoInst = MLC_EMIT(Case->Body->EndLine, MLI_GOTO, 1);
		GotoInst[1].Inst = Frame->Exits;
		Frame->Exits = GotoInst + 1;
		//}
	}
	Frame->IfInst[1].Inst = Function->Next;
	if (Case->Next) {
		Frame->Case = Case = Case->Next;
		Function->Frame->run = (mlc_frame_fn)ml_if_expr_compile2;
		return mlc_compile(Function, Case->Condition, 0);
	}
	if (Expr->Else) {
		Function->Frame->run = (mlc_frame_fn)ml_if_expr_compile4;
		return mlc_compile(Function, Expr->Else, 0);
	}
	MLC_LINK(Frame->Exits, Function->Next);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_if_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_if_expr_frame_t *Frame) {
	mlc_if_case_t *Case = Frame->Case;
	int CaseLine = Case->Condition->EndLine;
	Frame->IfInst = MLC_EMIT(CaseLine, MLI_AND, 1);
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
		ml_inst_t *WithInst = MLC_EMIT(CaseLine, Case->Token == MLT_VAR ? MLI_WITH_VAR : MLI_WITH, 1);
		WithInst[1].Decls = Function->Decls;
	}
	Function->Frame->run = (mlc_frame_fn)ml_if_expr_compile3;
	return mlc_compile(Function, Case->Body, 0);
}

static void ml_if_expr_compile(mlc_function_t *Function, mlc_if_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_if_expr_frame_t, ml_if_expr_compile2);
	Frame->Expr = Expr;
	Frame->Decls = Function->Decls;
	Frame->Flags = Flags;
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

static void ml_or_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
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

static void ml_and_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
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

static void ml_debug_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_link_expr_frame_t, ml_debug_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Frame->Exits = MLC_EMIT(Expr->StartLine, MLI_IF_DEBUG, 1);
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

static void ml_not_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
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
} mlc_case_expr_frame_t;

static void ml_case_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_case_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	if (Child->Next) {
		ml_inst_t *GotoInst = MLC_EMIT(Child->EndLine, MLI_GOTO, 1);
		Frame->Child = Child = Child->Next;
		GotoInst[1].Inst = Frame->Exits;
		Frame->Exits = GotoInst + 1;
		*Frame->Insts++ = Function->Next;
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

static void ml_case_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_case_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	int Count = 0;
	for (mlc_expr_t *Next = Child->Next; Next; Next = Next->Next) ++Count;
	ml_inst_t *SwitchInst = MLC_EMIT(Child->EndLine, MLI_SWITCH, 2);
	SwitchInst[1].Count = Count;
	Frame->Insts = SwitchInst[2].Insts = anew(ml_inst_t *, Count);
	Frame->Child = Child = Child->Next;
	Function->Frame->run = (mlc_frame_fn)ml_case_expr_compile3;
	*Frame->Insts++ = Function->Next;
	return mlc_compile(Function, Child, 0);
}

static void ml_case_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_case_expr_frame_t, ml_case_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Frame->Exits = NULL;
	mlc_expr_t *Child = Frame->Child = Expr->Child;
	return mlc_compile(Function, Child, 0);
}

struct mlc_loop_t {
	mlc_loop_t *Up;
	mlc_try_t *Try;
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
	ml_inst_t *GotoInst = MLC_EMIT(Expr->EndLine, MLI_GOTO, 1);
	GotoInst[1].Inst = Frame->Next;
	MLC_LINK(Frame->Loop->Exits, Function->Next);
	Function->Loop = Frame->Loop->Up;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_loop_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_loop_frame_t, ml_loop_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Frame->Loop->Up = Function->Loop;
	Frame->Loop->Try = Function->Try;
	Frame->Loop->Decls = Function->Decls;
	Frame->Loop->Exits = NULL;
	Frame->Loop->Nexts = NULL;
	Frame->Loop->ExitTop = Function->Top;
	Frame->Loop->NextTop = Function->Top;
	Function->Loop = Frame->Loop;
	Frame->Next = Function->Next;
	return mlc_compile(Function, Expr->Child, 0);
}

static void ml_next_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr, int Flags) {
	mlc_loop_t *Loop = Function->Loop;
	if (!Loop) MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "next not in loop"));
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
	if (Function->Top > Loop->NextTop) {
		ml_inst_t *ExitInst = MLC_EMIT(Expr->EndLine, MLI_EXIT, 2);
		ExitInst[1].Count = Function->Top - Loop->NextTop;
		ExitInst[2].Decls = Loop->Decls;
	}
	ml_inst_t *GotoInst = MLC_EMIT(Expr->EndLine, MLI_GOTO, 1);
	GotoInst[1].Inst = Loop->Nexts;
	Loop->Nexts = GotoInst + 1;
	MLC_RETURN(NULL);
}

typedef struct {
	mlc_parent_expr_t *Expr;
	mlc_loop_t *Loop;
	mlc_try_t *Try;
	int Flags;
} mlc_exit_expr_frame_t;

static void ml_exit_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_exit_expr_frame_t *Frame) {
	mlc_parent_expr_t *Expr = Frame->Expr;
	mlc_loop_t *Loop = Frame->Loop;
	Function->Loop = Loop;
	Function->Try = Frame->Try;
	if (Function->Top > Loop->ExitTop) {
		ml_inst_t *ExitInst = MLC_EMIT(Expr->EndLine, MLI_EXIT, 2);
		ExitInst[1].Count = Function->Top - Loop->ExitTop;
		ExitInst[2].Decls = Loop->Decls;
	}
	ml_inst_t *GotoInst = MLC_EMIT(Expr->EndLine, MLI_GOTO, 1);
	GotoInst[1].Inst = Loop->Exits;
	Loop->Exits = GotoInst + 1;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_exit_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_exit_expr_frame_t, ml_exit_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	mlc_loop_t *Loop = Frame->Loop = Function->Loop;
	if (!Loop) MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "exit not in loop"));
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
	Frame->Try = Function->Try;
	Function->Loop = Loop->Up;
	Function->Try = Loop->Try;
	if (Expr->Child) {
		return mlc_compile(Function, Expr->Child, 0);
	} else {
		MLC_EMIT(Expr->StartLine, MLI_NIL, 0);
		return ml_exit_expr_compile2(Function, NULL, Frame);
	}
}

static void ml_return_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	mlc_parent_expr_t *Expr = Frame->Expr;
	MLC_EMIT(Expr->EndLine, MLI_RETURN, 0);
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_return_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	if (Expr->Child) {
		MLC_FRAME(mlc_parent_expr_frame_t, ml_return_expr_compile2);
		Frame->Expr = Expr;
		Frame->Flags = Flags;
		return mlc_compile(Function, Expr->Child, 0);
	} else {
		MLC_EMIT(Expr->StartLine, MLI_NIL, 0);
		MLC_EMIT(Expr->EndLine, MLI_RETURN, 0);
		MLC_RETURN(NULL);
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

static void ml_suspend_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_parent_expr_frame_t, ml_suspend_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	mlc_expr_t *Child = Expr->Child;
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

static void ml_with_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
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
	if (Expr->Child->Next->Next) {
		Function->Frame->run = (mlc_frame_fn)ml_for_expr_compile4;
		return mlc_compile(Function, Expr->Child->Next->Next, 0);
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
	mlc_expr_t *Child = Expr->Child;
	MLC_EMIT(Child->EndLine, MLI_FOR, 0);
	Frame->IterInst = MLC_EMIT(Child->EndLine, MLI_ITER, 1);
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
		ml_inst_t *KeyInst = MLC_EMIT(Child->EndLine, MLI_KEY, 1);
		KeyInst[1].Index = -1;
		ml_inst_t *WithInst = MLC_EMIT(Child->EndLine, MLI_WITH, 1);
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
	ml_inst_t *ValueInst = MLC_EMIT(Child->EndLine, MLI_VALUE, 1);
	ValueInst[1].Index = Expr->Key ? -2 : -1;
	if (Expr->Unpack) {
		ml_inst_t *WithInst = MLC_EMIT(Child->EndLine, MLI_WITHX, 2);
		WithInst[1].Count = Expr->Unpack;
		WithInst[2].Decls = Function->Decls;
	} else {
		ml_inst_t *WithInst = MLC_EMIT(Child->EndLine, MLI_WITH, 1);
		WithInst[1].Decls = Function->Decls;
	}
	Frame->Loop->Up = Function->Loop;
	Frame->Loop->Try = Function->Try;
	Frame->Loop->Exits = NULL;
	Frame->Loop->Nexts = NULL;
	if (Function->Top >= Function->Size) Function->Size = Function->Top + 1;
	Function->Loop = Frame->Loop;
	Function->Frame->run = (mlc_frame_fn)ml_for_expr_compile3;
	return mlc_compile(Function, Child->Next, 0);
}

static void ml_for_expr_compile(mlc_function_t *Function, mlc_for_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_for_expr_frame_t, ml_for_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Frame->Loop->ExitTop = Function->Top;
	Frame->Loop->NextTop = Function->Top + 1;
	Frame->Loop->Decls = Function->Decls;
	return mlc_compile(Function, Expr->Child, 0);
}

static void ml_each_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	mlc_parent_expr_t *Expr = Frame->Expr;
	mlc_expr_t *Child = Expr->Child;
	MLC_EMIT(Child->EndLine, MLI_FOR, 0);
	ml_inst_t *AndInst = MLC_EMIT(Child->EndLine, MLI_ITER, 1);
	mlc_inc_top(Function);
	ml_inst_t *ValueInst = MLC_EMIT(Child->EndLine, MLI_VALUE, 1);
	ValueInst[1].Index = -1;
	ml_inst_t *NextInst = MLC_EMIT(Expr->StartLine, MLI_NEXT, 1);
	NextInst[1].Inst = AndInst;
	AndInst[1].Inst = Function->Next;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_each_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
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
	mlc_catch_expr_t *CatchExpr;
	ml_inst_t *TryInst, *CatchInst, *Exits;
	inthash_t DeclHashes;
	mlc_try_t Try;
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
	ml_inst_t *VarInst = MLC_EMIT(Expr->EndLine, MLI_VAR, 1);
	VarInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
	Decl->Flags = 0;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_var_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_var_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, 0);
}

static void ml_var_type_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_inst_t *TypeInst = MLC_EMIT(Expr->EndLine, MLI_VAR_TYPE, 1);
	TypeInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_var_type_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
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
		PushInst[1].Index = Function->Top - 1;
		mlc_inc_top(Function);
		ml_inst_t *ValueInst = MLC_EMIT(Expr->EndLine, MLI_LOAD_PUSH, 1);
		ValueInst[1].Value = ml_cstring(Local->Ident);
		mlc_inc_top(Function);
		ml_inst_t *CallInst = MLC_EMIT(Expr->EndLine, MLI_CONST_CALL, 2);
		CallInst[1].Count = 2;
		CallInst[2].Value = SymbolMethod;
		Function->Top -= 2;
		ml_decl_t *Decl = Decls[I];
		ml_inst_t *VarInst = MLC_EMIT(Expr->EndLine, MLI_VAR, 1);
		VarInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
		Decl->Flags = 0;
	}
	if (!(Frame->Flags & MLCF_PUSH)) {
		MLC_EMIT(Expr->EndLine, MLI_POP, 0);
		--Function->Top;
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_var_in_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_var_in_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, MLCF_PUSH);
}

static void ml_var_unpack_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_inst_t *VarInst = MLC_EMIT(Expr->EndLine, MLI_VARX, 2);
	VarInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
	VarInst[2].Count = Expr->Count;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_var_unpack_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
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
	if (Expr->Flags == MLT_REF) {
		if (Decl->Flags & MLC_DECL_BACKFILL) {
			LetInst = MLC_EMIT(Expr->EndLine, MLI_REFI, 1);
		} else {
			LetInst = MLC_EMIT(Expr->EndLine, MLI_REF, 1);
		}
	} else {
		if (Decl->Flags & MLC_DECL_BACKFILL) {
			LetInst = MLC_EMIT(Expr->EndLine, MLI_LETI, 1);
		} else {
			LetInst = MLC_EMIT(Expr->EndLine, MLI_LET, 1);
		}
	}
	LetInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
	Decl->Flags = 0;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_let_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
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
		PushInst[1].Index = Function->Top - 1;
		mlc_inc_top(Function);
		ml_inst_t *ValueInst = MLC_EMIT(Expr->EndLine, MLI_LOAD_PUSH, 1);
		ValueInst[1].Value = ml_cstring(Local->Ident);
		mlc_inc_top(Function);
		ml_inst_t *CallInst = MLC_EMIT(Expr->EndLine, MLI_CONST_CALL, 2);
		CallInst[1].Count = 2;
		CallInst[2].Value = SymbolMethod;
		Function->Top -= 2;
		ml_decl_t *Decl = Decls[I];
		ml_inst_t *LetInst;
		if (Expr->Flags == MLT_REF) {
			if (Decl->Flags & MLC_DECL_BACKFILL) {
				LetInst = MLC_EMIT(Expr->EndLine, MLI_REFI, 1);
			} else {
				LetInst = MLC_EMIT(Expr->EndLine, MLI_REF, 1);
			}
		} else {
			if (Decl->Flags & MLC_DECL_BACKFILL) {
				LetInst = MLC_EMIT(Expr->EndLine, MLI_LETI, 1);
			} else {
				LetInst = MLC_EMIT(Expr->EndLine, MLI_LET, 1);
			}
		}
		LetInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
		Decl->Flags = 0;
	}
	if (!(Frame->Flags & MLCF_PUSH)) {
		MLC_EMIT(Expr->EndLine, MLI_POP, 0);
		--Function->Top;
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_let_in_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_let_in_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_compile(Function, Expr->Child, MLCF_PUSH);
}

static void ml_let_unpack_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_local_expr_frame_t *Frame) {
	mlc_local_expr_t *Expr = Frame->Expr;
	mlc_local_t *Local = Expr->Local;
	ml_decl_t **Decls = Function->Block->Decls + Local->Index;
	ml_inst_t *LetInst = MLC_EMIT(Expr->EndLine, Expr->Flags == MLT_REF ? MLI_REFX : MLI_LETX, 2);
	LetInst[1].Index = Function->Block->Top + Local->Index - Function->Top;
	LetInst[2].Count = Expr->Count;
	for (int I = 0; I < Expr->Count; ++I) Decls[I]->Flags = 0;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_let_unpack_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_let_unpack_expr_compile2);
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
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_NIL_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_def_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
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
		Frame->Args[1] = ml_cstring(Local->Ident);
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
	Frame->Args[1] = ml_cstring(Local->Ident);
	Function->Frame->run = (mlc_frame_fn)ml_def_in_expr_compile3;
	return ml_call(Function, SymbolMethod, 2, Frame->Args);
}

static void ml_def_in_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
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

static void ml_def_unpack_expr_compile(mlc_function_t *Function, mlc_local_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_local_expr_frame_t, ml_def_unpack_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	return mlc_expr_call(Function, Expr->Child);
}

static void ml_block_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_block_t *Frame) {
	mlc_catch_expr_t *CatchExpr = Frame->CatchExpr;
	ml_inst_t *ExitInst = MLC_EMIT(CatchExpr->Body->EndLine, MLI_EXIT, 2);
	ExitInst[1].Count = 1;
	ExitInst[2].Decls = Frame->OldDecls;
	Function->Decls = Frame->OldDecls;
	Function->Top = Frame->Top;
	ml_inst_t *GotoInst = MLC_EMIT(CatchExpr->Body->EndLine, MLI_GOTO, 1);
	GotoInst[1].Inst = Frame->Exits;
	Frame->Exits = GotoInst + 1;
	if ((CatchExpr = CatchExpr->Next)) {
		if (Frame->CatchInst) {
			Frame->CatchInst[1].Inst = Function->Next;
			Frame->CatchInst = NULL;
		}
		if (CatchExpr->Types) {
			int NumTypes = 0;
			for (mlc_catch_type_t *Type = CatchExpr->Types; Type; Type = Type->Next) ++NumTypes;
			Frame->CatchInst = MLC_EMIT(CatchExpr->Line, MLI_CATCH_TYPE, 2);
			const char **Ptrs = Frame->CatchInst[2].Ptrs = anew(const char *, NumTypes + 1);
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
		ml_inst_t *CatchInst = MLC_EMIT(CatchExpr->Line, MLI_CATCH, 2);
		CatchInst[1].Index = Frame->Top;
		CatchInst[2].Decls = Function->Decls;
		return mlc_compile(Function, CatchExpr->Body, 0);
	}
	mlc_block_expr_t *Expr = Frame->Expr;
	if (Frame->CatchInst) {
		Frame->CatchInst[1].Inst = Function->Next;
		MLC_EMIT(Expr->EndLine, MLI_RETRY, 0);
	}
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
		return mlc_compile(Function, Child, 0);
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
	if (Expr->Catches) {
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
		TryInst = MLC_EMIT(Expr->Catches->Line, MLI_TRY, 1);
		MLC_LINK(Frame->Try.Retries, TryInst);
		if (Function->Try) {
			TryInst[1].Inst = Function->Try->Retries;
			Function->Try->Retries = TryInst + 1;
		} else {
			TryInst[1].Inst = Function->Returns;
			Function->Returns = TryInst + 1;
		}
		Frame->TryInst[1].Inst = TryInst;
		mlc_catch_expr_t *CatchExpr = Frame->CatchExpr = Expr->Catches;
		Function->Frame->run = (mlc_frame_fn)ml_block_expr_compile3;
		if (CatchExpr->Types) {
			int NumTypes = 0;
			for (mlc_catch_type_t *Type = CatchExpr->Types; Type; Type = Type->Next) ++NumTypes;
			Frame->CatchInst = MLC_EMIT(CatchExpr->Line, MLI_CATCH_TYPE, 2);
			const char **Ptrs = Frame->CatchInst[2].Ptrs = anew(const char *, NumTypes + 1);
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
		ml_inst_t *CatchInst = MLC_EMIT(CatchExpr->Line, MLI_CATCH, 2);
		CatchInst[1].Index = Frame->Top;
		CatchInst[2].Decls = Function->Decls;
		return mlc_compile(Function, CatchExpr->Body, 0);
	}
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_block_expr_compile(mlc_function_t *Function, mlc_block_expr_t *Expr, int Flags) {
	int NumDecls = Expr->NumVars + Expr->NumLets + Expr->NumDefs;
	MLC_XFRAME(mlc_block_t, NumDecls, ml_decl_t *, ml_block_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	Frame->Top = Function->Top;
	Frame->OldDecls = Function->Decls;
	Frame->CatchInst = NULL;
	if (Expr->Catches) {
		Frame->TryInst = MLC_EMIT(Expr->StartLine, MLI_TRY, 1);
		Frame->Try.Up = Function->Try;
		Frame->Try.Retries = NULL;
		Frame->Try.Top = Function->Top;
		Function->Try = &Frame->Try;
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
		if (inthash_insert(DeclHashes, (uintptr_t)Decl->Hash, Decl)) {
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
		if (inthash_insert(DeclHashes, (uintptr_t)Decl->Hash, Decl)) {
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
		if (inthash_insert(DeclHashes, (uintptr_t)Decl->Hash, Decl)) {
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
		return mlc_compile(Function, Child, 0);
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
	Function->Self = Frame->Count;
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_assign_expr_compile3(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	mlc_parent_expr_t *Expr = Frame->Expr;
	ml_inst_t *AssignInst = MLC_EMIT(Expr->EndLine, MLI_ASSIGN_LOCAL, 1);
	AssignInst[1].Index = Function->Self;
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	Function->Self = Frame->Count;
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_assign_expr_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_parent_expr_frame_t *Frame) {
	Frame->Count = Function->Self;
	if (Value) {
		Function->Self = ml_integer_value_fast(Value);
		Function->Frame->run = (mlc_frame_fn)ml_assign_expr_compile3;
	} else {
		Function->Self = Function->Top - 1;
		Function->Frame->run = (mlc_frame_fn)ml_assign_expr_compile4;
	}
	return mlc_compile(Function, Frame->Child, 0);
}

static void ml_assign_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(mlc_parent_expr_frame_t, ml_assign_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	mlc_expr_t *Child = Expr->Child;
	Frame->Child = Child->Next;
	return mlc_compile(Function, Child, MLCF_LOCAL | MLCF_PUSH);
}

static void ml_old_expr_compile(mlc_function_t *Function, mlc_expr_t *Expr, int Flags) {
	if (Flags & MLCF_PUSH) {
		ml_inst_t *OldInst = MLC_EMIT(Expr->StartLine, MLI_LOCAL_PUSH, 1);
		OldInst[1].Index = Function->Self;
		mlc_inc_top(Function);
	} else {
		ml_inst_t *OldInst = MLC_EMIT(Expr->StartLine, MLI_LOCAL, 1);
		OldInst[1].Index = Function->Self;
	}
	MLC_RETURN(NULL);
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

static void ml_tuple_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
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

static void ml_list_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
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

static void ml_map_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
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

typedef struct {
	ml_macro_t Base;
	ml_decl_t *Params;
	mlc_expr_t *Expr;
} ml_template_macro_t;

static void ml_template_macro_apply2(mlc_function_t *Function, ml_value_t *Value, mlc_define_t **Frame) {
	Function->Defines = Frame[0];
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_template_macro_apply(mlc_function_t *Function, ml_template_macro_t *Macro, mlc_expr_t *Expr, mlc_expr_t *Child, int Flags) {
	MLC_FRAME(mlc_define_t *, ml_template_macro_apply2);
	Frame[0] = Function->Defines;
	for (ml_decl_t *Param = Macro->Params; Param; Param = Param->Next) {
		if (!Child) MLC_EXPR_ERROR(Expr, ml_error("MacroError", "Insufficient arguments to macro"));
		mlc_define_t *Define = new(mlc_define_t);
		Define->Ident = Param->Ident;
		Define->Hash = Param->Hash;
		Define->Expr = Child;
		Define->Next = Function->Defines;
		Function->Defines = Define;
		Child = Child->Next;
	}
	return mlc_compile(Function, Macro->Expr, Flags);
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

static void ml_scope_macro_apply2(mlc_function_t *Function, ml_value_t *Value, ml_decl_t **Frame) {
	Function->Decls = Frame[0];
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_scope_macro_apply(mlc_function_t *Function, ml_scope_macro_t *Macro, mlc_expr_t *Expr, mlc_expr_t *Child, int Flags) {
	if (!Child) MLC_EXPR_ERROR(Expr, ml_error("MacroError", "Insufficient arguments to macro"));
	MLC_FRAME(ml_decl_t *, ml_scope_macro_apply2);
	Frame[0] = Function->Decls;
	stringmap_foreach(Macro->Names, Function, (void *)ml_scope_macro_fn);
	return mlc_compile(Function, Child, Flags);
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

static void ml_stringify_macro_apply(mlc_function_t *Function, ml_macro_t *Macro, mlc_expr_t *Expr, mlc_expr_t *Child, int Flags) {
}

typedef struct {
	mlc_expr_t *Expr;
	mlc_expr_t *Child;
	ml_value_t *Value;
	int Count, Index, Flags;
} ml_call_expr_frame_t;

static void ml_call_expr_compile3(mlc_function_t *Function, ml_value_t *Value, ml_call_expr_frame_t *Frame) {
	mlc_expr_t *Child = Frame->Child;
	ml_inst_t *SetInst = MLC_EMIT(Child->EndLine, MLI_PARTIAL_SET, 1);
	int Index = SetInst[1].Index = Frame->Index;
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
	PartialInst[1].Count = Frame->Count;
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
		Frame->Child = Child->Next;
		return mlc_compile(Function, Child, MLCF_PUSH);
	}
	mlc_expr_t *Expr = Frame->Expr;
	if (Frame->Value) {
		ml_inst_t *CallInst = MLC_EMIT(Expr->EndLine, MLI_CONST_CALL, 2);
		CallInst[1].Count = Frame->Count;
		ml_value_t *Value = Frame->Value;
		CallInst[2].Value = Value;
		if (ml_typeof(Value) == MLUninitializedT) ml_uninitialized_use(Value, &CallInst[2].Value);
		Function->Top -= Frame->Count;
	} else {
		ml_inst_t *CallInst = MLC_EMIT(Expr->EndLine, MLI_CALL, 1);
		CallInst[1].Count = Frame->Count;
		Function->Top -= Frame->Count + 1;
	}
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
		mlc_inc_top(Function);
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_call_expr_compile4(mlc_function_t *Function, ml_value_t *Value, ml_call_expr_frame_t *Frame) {
	mlc_expr_t *Expr = Frame->Expr;
	if (Value) {
		if (ml_typeof(Value) == MLMacroT) {
			MLC_POP();
			ml_macro_t *Macro = (ml_macro_t *)Value;
			return Macro->apply(Function, Macro, Expr, Frame->Child, Frame->Flags);
		}
	}
	mlc_expr_t *Child = Frame->Child;
	Function->Frame->run = (mlc_frame_fn)ml_call_expr_compile5;
	Frame->Value = Value;
	if (Child) {
		Frame->Child = Child->Next;
		return mlc_compile(Function, Child, MLCF_PUSH);
	} else {
		Frame->Child = NULL;
		return ml_call_expr_compile5(Function, NULL, Frame);
	}
}

static void ml_call_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(ml_call_expr_frame_t, ml_call_expr_compile4);
	Frame->Expr = (mlc_expr_t *)Expr;
	Frame->Child = Expr->Child->Next;
	int Count = 0;
	for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next) ++Count;
	Frame->Count = Count;
	Frame->Flags = Flags;
	for (mlc_expr_t *Child = Expr->Child->Next; Child; Child = Child->Next) {
		if (Child->compile == (void *)ml_blank_expr_compile) {
			Function->Frame->run = (mlc_frame_fn)ml_call_expr_compile2;
			return mlc_compile(Function, Expr->Child, 0);
		}
	}
	return mlc_compile(Function, Expr->Child, MLCF_CONSTANT | MLCF_PUSH);
}

static void ml_const_call_expr_compile(mlc_function_t *Function, mlc_parent_value_expr_t *Expr, int Flags) {
	MLC_FRAME(ml_call_expr_frame_t, ml_call_expr_compile4);
	Frame->Expr = (mlc_expr_t *)Expr;
	Frame->Child = Expr->Child;
	int Count = 0;
	for (mlc_expr_t *Child = Expr->Child; Child; Child = Child->Next) ++Count;
	Frame->Count = Count;
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
		ValueInst[1].Value = Value;
		mlc_inc_top(Function);
		MLC_POP();
		MLC_RETURN(NULL);
	} else {
		ml_inst_t *ValueInst = MLC_EMIT(Expr->EndLine, MLI_LOAD, 1);
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

static void ml_resolve_expr_compile(mlc_function_t *Function, mlc_parent_value_expr_t *Expr, int Flags) {
	MLC_FRAME(ml_resolve_expr_frame_t, ml_resolve_expr_compile2);
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
	ml_inst_t *AddInst = MLC_EMIT(Part->Line, MLI_STRING_ADD, 1);
	AddInst[1].Count = Frame->NumArgs;
	Function->Top -= Frame->NumArgs;
	while ((Part = Part->Next)) {
		if (Part->Length) {
			ml_inst_t *AddInst = MLC_EMIT(Part->Line, MLI_STRING_ADDS, 2);
			AddInst[1].Count = Part->Length;
			AddInst[2].Ptr = Part->Chars;
		} else {
			Frame->Part = Part;
			mlc_expr_t *Child = Part->Child;
			Frame->Child = Child->Next;
			Frame->NumArgs = 1;
			return mlc_compile(Function, Child, MLCF_PUSH);
		}
	}
	mlc_string_expr_t *Expr = Frame->Expr;
	MLC_EMIT(Expr->StartLine, MLI_STRING_END, 0);
	if (Frame->Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
	} else {
		--Function->Top;
	}
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_string_expr_compile(mlc_function_t *Function, mlc_string_expr_t *Expr, int Flags) {
	MLC_EMIT(Expr->StartLine, MLI_STRING_NEW, 0);
	mlc_inc_top(Function);
	for (mlc_string_part_t *Part = Expr->Parts; Part; Part = Part->Next) {
		if (Part->Length) {
			ml_inst_t *AddInst = MLC_EMIT(Part->Line, MLI_STRING_ADDS, 2);
			AddInst[1].Count = Part->Length;
			AddInst[2].Ptr = Part->Chars;
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
	MLC_EMIT(Expr->EndLine, MLI_STRING_END, 0);
	if (Flags & MLCF_PUSH) {
		MLC_EMIT(Expr->EndLine, MLI_PUSH, 0);
	} else {
		--Function->Top;
	}
	MLC_RETURN(NULL);
}

#define ML_PARAM_EXTRA 1
#define ML_PARAM_NAMED 2
#define ML_PARAM_BYREF 3

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
	TypeInst[1].Index = Index;
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
	for (mlc_upvalue_t *UpValue = SubFunction->UpValues; UpValue; UpValue = UpValue->Next) ClosureInst[++Index].Index = UpValue->Index;
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
		Decl->Source.Line = Expr->StartLine;
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
		ml_closure_t *Closure = xnew(ml_closure_t, 0, ml_value_t *);
		Closure->Type = MLClosureT;
		Closure->Info = Info;
		if (Frame->Flags & MLCF_PUSH) {
			ml_inst_t *LoadInst = MLC_EMIT(Expr->StartLine, MLI_LOAD_PUSH, 1);
			LoadInst[1].Value = (ml_value_t *)Closure;
			mlc_inc_top(Function);
		} else {
			ml_inst_t *LoadInst = MLC_EMIT(Expr->StartLine, MLI_LOAD, 1);
			LoadInst[1].Value = (ml_value_t *)Closure;
		}
		MLC_POP();
		MLC_RETURN(NULL);
	}
}

static void ml_subfunction_run(mlc_function_t *SubFunction, ml_value_t *Value, void *Frame) {
	mlc_function_t *Function = SubFunction->Up;
	MLC_RETURN(NULL);
}

static void ml_fun_expr_compile(mlc_function_t *Function, mlc_fun_expr_t *Expr, int Flags) {
	// closure <entry> <frame_size> <num_params> <num_upvalues> <upvalue_1> ...
	mlc_function_t *SubFunction = new(mlc_function_t);
	SubFunction->Base.Caller = (ml_state_t *)Function;
	SubFunction->Base.Context = Function->Base.Context;
	SubFunction->Base.run = (ml_state_fn)mlc_function_run;
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
			/* no break */
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
	mlc_compile(SubFunction, Expr->Body, 0);
}

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

static void ml_ident_expr_compile(mlc_function_t *Function, mlc_ident_expr_t *Expr, int Flags) {
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
							ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_LOCALX, 2);
							LocalInst[1].Index = Index;
							LocalInst[2].Ptr = Decl->Ident;
						} else if (Index >= 0) {
							if (Flags & MLCF_LOCAL) {
								MLC_RETURN(ml_integer(Index));
							} else if (Flags & MLCF_PUSH) {
								ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_LOCAL_PUSH, 1);
								LocalInst[1].Index = Index;
								mlc_inc_top(Function);
								MLC_RETURN(NULL);
							} else {
								ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_LOCAL, 1);
								LocalInst[1].Index = Index;
							}
						} else {
							ml_inst_t *LocalInst = MLC_EMIT(Expr->StartLine, MLI_UPVALUE, 1);
							LocalInst[1].Index = ~Index;
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
	}
	ml_value_t *Value = (ml_value_t *)stringmap_search(Function->Compiler->Vars, Expr->Ident);
	if (!Value) Value = Function->Compiler->GlobalGet(Function->Compiler->Globals, Expr->Ident);
	if (!Value) {
		MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "identifier %s not declared", Expr->Ident));
	}
	if (ml_is_error(Value)) MLC_EXPR_ERROR(Expr, Value);
	return ml_ident_expr_finish(Function, Expr, Value, Flags);
}

static void ml_define_expr_compile(mlc_function_t *Function, mlc_ident_expr_t *Expr, int Flags) {
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
	MLC_EXPR_ERROR(Expr, ml_error("CompilerError", "identifier %s not defined", Expr->Ident));
}

typedef struct {
	mlc_parent_expr_t *Expr;
	int Flags;
} ml_inline_expr_frame_t;

static void ml_inline_expr_compile2(mlc_function_t *Function, ml_value_t *Value, ml_inline_expr_frame_t *Frame) {
	if (ml_is_error(Value)) {
		ml_state_t *Caller = Function->Base.Caller;
		ML_RETURN(Value);
	}
	mlc_parent_expr_t *Expr = Frame->Expr;
	int Flags = Frame->Flags;
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
	MLC_POP();
	MLC_RETURN(NULL);
}

static void ml_inline_expr_compile(mlc_function_t *Function, mlc_parent_expr_t *Expr, int Flags) {
	MLC_FRAME(ml_inline_expr_frame_t, ml_inline_expr_compile2);
	Frame->Expr = Expr;
	Frame->Flags = Flags;
	mlc_expr_call(Function, Expr->Child);
}

ml_expr_type_t mlc_expr_type(mlc_expr_t *Expr) {
	if (Expr->compile == (void *)ml_register_expr_compile) {
		return ML_EXPR_REGISTER;
	} else if (Expr->compile == (void *)ml_blank_expr_compile) {
		return ML_EXPR_BLANK;
	} else if (Expr->compile == (void *)ml_nil_expr_compile) {
		return ML_EXPR_NIL;
	} else if (Expr->compile == (void *)ml_value_expr_compile) {
		return ML_EXPR_VALUE;
	} else if (Expr->compile == (void *)ml_if_expr_compile) {
		return ML_EXPR_IF;
	} else if (Expr->compile == (void *)ml_or_expr_compile) {
		return ML_EXPR_OR;
	} else if (Expr->compile == (void *)ml_and_expr_compile) {
		return ML_EXPR_AND;
	} else if (Expr->compile == (void *)ml_debug_expr_compile) {
		return ML_EXPR_DEBUG;
	} else if (Expr->compile == (void *)ml_not_expr_compile) {
		return ML_EXPR_NOT;
	} else if (Expr->compile == (void *)ml_loop_expr_compile) {
		return ML_EXPR_LOOP;
	} else if (Expr->compile == (void *)ml_next_expr_compile) {
		return ML_EXPR_NEXT;
	} else if (Expr->compile == (void *)ml_exit_expr_compile) {
		return ML_EXPR_EXIT;
	} else if (Expr->compile == (void *)ml_return_expr_compile) {
		return ML_EXPR_RETURN;
	} else if (Expr->compile == (void *)ml_suspend_expr_compile) {
		return ML_EXPR_SUSPEND;
	} else if (Expr->compile == (void *)ml_with_expr_compile) {
		return ML_EXPR_WITH;
	} else if (Expr->compile == (void *)ml_for_expr_compile) {
		return ML_EXPR_FOR;
	} else if (Expr->compile == (void *)ml_each_expr_compile) {
		return ML_EXPR_EACH;
	} else if (Expr->compile == (void *)ml_var_expr_compile) {
		return ML_EXPR_VAR;
	} else if (Expr->compile == (void *)ml_var_type_expr_compile) {
		return ML_EXPR_VAR_TYPE;
	} else if (Expr->compile == (void *)ml_var_in_expr_compile) {
		return ML_EXPR_VAR_IN;
	} else if (Expr->compile == (void *)ml_var_unpack_expr_compile) {
		return ML_EXPR_VAR_UNPACK;
	} else if (Expr->compile == (void *)ml_let_expr_compile) {
		return ML_EXPR_LET;
	} else if (Expr->compile == (void *)ml_let_in_expr_compile) {
		return ML_EXPR_LET_IN;
	} else if (Expr->compile == (void *)ml_let_unpack_expr_compile) {
		return ML_EXPR_LET_UNPACK;
	} else if (Expr->compile == (void *)ml_def_expr_compile) {
		return ML_EXPR_DEF;
	} else if (Expr->compile == (void *)ml_def_in_expr_compile) {
		return ML_EXPR_DEF_IN;
	} else if (Expr->compile == (void *)ml_def_unpack_expr_compile) {
		return ML_EXPR_DEF_UNPACK;
	} else if (Expr->compile == (void *)ml_block_expr_compile) {
		return ML_EXPR_BLOCK;
	} else if (Expr->compile == (void *)ml_assign_expr_compile) {
		return ML_EXPR_ASSIGN;
	} else if (Expr->compile == (void *)ml_old_expr_compile) {
		return ML_EXPR_OLD;
	} else if (Expr->compile == (void *)ml_tuple_expr_compile) {
		return ML_EXPR_TUPLE;
	} else if (Expr->compile == (void *)ml_list_expr_compile) {
		return ML_EXPR_LIST;
	} else if (Expr->compile == (void *)ml_map_expr_compile) {
		return ML_EXPR_MAP;
	} else if (Expr->compile == (void *)ml_call_expr_compile) {
		return ML_EXPR_CALL;
	} else if (Expr->compile == (void *)ml_const_call_expr_compile) {
		return ML_EXPR_CONST_CALL;
	} else if (Expr->compile == (void *)ml_resolve_expr_compile) {
		return ML_EXPR_RESOLVE;
	} else if (Expr->compile == (void *)ml_string_expr_compile) {
		return ML_EXPR_STRING;
	} else if (Expr->compile == (void *)ml_fun_expr_compile) {
		return ML_EXPR_FUN;
	} else if (Expr->compile == (void *)ml_ident_expr_compile) {
		return ML_EXPR_IDENT;
	} else if (Expr->compile == (void *)ml_define_expr_compile) {
		return ML_EXPR_DEFINE;
	} else if (Expr->compile == (void *)ml_inline_expr_compile) {
		return ML_EXPR_INLINE;
	} else {
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
	"switch", // MLT_SWITCH,
	"case", // MLT_CASE,
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

static mlc_expr_t *ml_accept_block(ml_compiler_t *Compiler);
static void ml_accept_eoi(ml_compiler_t *Compiler);

ml_compiler_t *ml_compiler(ml_getter_t GlobalGet, void *Globals, ml_reader_t Read, void *Data) {
	ml_compiler_t *Compiler = new(ml_compiler_t);
	Compiler->Type = MLCompilerT;
	Compiler->GlobalGet = GlobalGet;
	Compiler->Globals = Globals;
	Compiler->Token = MLT_NONE;
	Compiler->Next = "";
	Compiler->Source.Name = "";
	Compiler->Source.Line = 0;
	Compiler->Line = 0;
	Compiler->Data = Data;
	Compiler->Read = Read ?: ml_compiler_no_input;
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

void ml_parse_error(ml_compiler_t *Compiler, const char *Error, const char *Format, ...) {
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
			End = Compiler->Read(Compiler->Data);
			if (!End) {
				ml_parse_error(Compiler, "ParseError", "end of input while parsing string");
			}
			++Compiler->Line;
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
			case 0: ml_parse_error(Compiler, "ParseError", "end of line while parsing string");
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
	ML_CHAR_DQUOTE
} ml_char_type_t;

static const ml_char_type_t CharTypes[256] = {
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
	[128 ... 255] = ML_CHAR_ALPHA
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

static stringmap_t StringFns[1] = {STRINGMAP_INIT};

void ml_string_fn_register(const char *Prefix, string_fn_t Fn) {
	stringmap_insert(StringFns, Prefix, Fn);
}

static int ml_scan_string(ml_compiler_t *Compiler) {
	const char *End = Compiler->Next;
	while (End[0] != '\"') {
		if (!End[0]) {
			ml_parse_error(Compiler, "ParseError", "End of input while parsing string");
		}
		if (End[0] == '\\') ++End;
		++End;
	}
	int Length = End - Compiler->Next;
	char *Quoted = snew(Length + 1), *D = Quoted;
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
			case '0': *D++ = '\0'; break;
			default: *D++ = '\\'; *D++ = *S; break;
			}
		} else {
			*D++ = *S;
		}
	}
	*D = 0;
	Compiler->Ident = Quoted;
	Compiler->Next = End + 1;
	return D - Quoted;
}

static int ml_scan_raw_string(ml_compiler_t *Compiler) {
	const char *End = Compiler->Next;
	while (End[0] != '\"') {
		if (!End[0]) {
			ml_parse_error(Compiler, "ParseError", "End of input while parsing string");
		}
		if (End[0] == '\\') ++End;
		++End;
	}
	int Length = End - Compiler->Next;
	char *Raw = snew(Length + 1);
	memcpy(Raw, Compiler->Next, Length);
	Raw[Length] = 0;
	Compiler->Ident = Raw;
	Compiler->Next = End + 1;
	return Length;
}

static ml_token_t ml_scan(ml_compiler_t *Compiler) {
	const char *Next = Compiler->Next;
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
			&&DO_CHAR_DQUOTE
		};
		goto *Labels[CharTypes[(unsigned char)Char]];
		DO_CHAR_EOI:
			Next = Compiler->Read(Compiler->Data);
			if (Next) continue;
			Compiler->Next = "";
			Compiler->Token = MLT_EOI;
			return Compiler->Token;
		DO_CHAR_LINE:
			Compiler->Next = Next + 1;
			++Compiler->Line;
			Compiler->Token = MLT_EOL;
			return Compiler->Token;
		DO_CHAR_SPACE:
			++Next;
			continue;
		DO_CHAR_ALPHA: {
			const char *End = Next + 1;
			while (ml_isidchar(*End)) ++End;
			int Length = End - Next;
			const struct keyword_t *Keyword = lookup(Next, Length);
			if (Keyword) {
				Compiler->Token = Keyword->Token;
				Compiler->Next = End;
				return Compiler->Token;
			}
			char *Ident = snew(Length + 1);
			memcpy(Ident, Next, Length);
			Ident[Length] = 0;
			if (End[0] == '\"') {
				string_fn_t StringFn = stringmap_search(StringFns, Ident);
				if (!StringFn) ml_parse_error(Compiler, "ParseError", "Unknown string prefix: %s", Ident);
				Compiler->Next = End + 1;
				int Length = ml_scan_raw_string(Compiler);
				ml_value_t *Value = StringFn(Compiler->Ident, Length);
				if (ml_is_error(Value)) {
					ml_error_trace_add(Value, Compiler->Source);
					Compiler->Value = Value;
					longjmp(Compiler->OnError, 1);
				}
				Compiler->Value = Value;
				Compiler->Token = MLT_VALUE;
				return Compiler->Token;
			}
			Compiler->Next = End;
			Compiler->Ident = Ident;
			Compiler->Token = MLT_IDENT;
			return Compiler->Token;
		}
		DO_CHAR_DIGIT: {
			char *End;
			double Double = strtod(Next, (char **)&End);
#ifdef ML_COMPLEX
			if (*End == 'i') {
				Compiler->Value = ml_complex(Double * 1i);
				Compiler->Token = MLT_VALUE;
				Compiler->Next = End + 1;
				return Compiler->Token;
			}
#endif
			for (const char *P = Next; P < End; ++P) {
				if (P[0] == '.' || P[0] == 'e' || P[0] == 'E') {
					Compiler->Value = ml_real(Double);
					Compiler->Token = MLT_VALUE;
					Compiler->Next = End;
					return Compiler->Token;
				}
			}
			long Integer = strtol(Next, (char **)&End, 10);
			Compiler->Value = ml_integer(Integer);
			Compiler->Token = MLT_VALUE;
			Compiler->Next = End;
			return Compiler->Token;
		}
		DO_CHAR_SQUOTE:
			Compiler->Next = Next + 1;
			return ml_accept_string(Compiler);
		DO_CHAR_DQUOTE:
			Compiler->Next = Next + 1;
			int Length = ml_scan_string(Compiler);;
			Compiler->Value = ml_string(Compiler->Ident, Length);
			Compiler->Token = MLT_VALUE;
			return Compiler->Token;
		DO_CHAR_COLON: {
			Char = *++Next;
			if (Char == '=') {
				Compiler->Token = MLT_ASSIGN;
				Compiler->Next = Next + 1;
				return Compiler->Token;
			} else if (Char == ':') {
				Compiler->Token = MLT_IMPORT;
				Char = *++Next;
				if (ml_isidchar(Char)) {
					const char *End = Next + 1;
					while (ml_isidchar(*End)) ++End;
					int Length = End - Next;
					char *Ident = snew(Length + 1);
					memcpy(Ident, Next, Length);
					Ident[Length] = 0;
					Compiler->Ident = Ident;
					Compiler->Next = End;
				} else if (Char == '\"') {
					Compiler->Next = Next + 1;
					ml_scan_string(Compiler);
				} else if (ml_isoperator(Char)) {
					const char *End = Next + 1;
					while (ml_isoperator(*End)) ++End;
					int Length = End - Next;
					char *Operator = snew(Length + 1);
					memcpy(Operator, Next, Length);
					Operator[Length] = 0;
					Compiler->Ident = Operator;
					Compiler->Next = End;
				} else {
					Compiler->Next = Next;
					Compiler->Ident = "::";
					Compiler->Token = MLT_OPERATOR;
				}
				return Compiler->Token;
			} else if (ml_isidchar(Char)) {
				const char *End = Next + 1;
				while (ml_isidchar(*End)) ++End;
				int Length = End - Next;
				char *Ident = snew(Length + 1);
				memcpy(Ident, Next, Length);
				Ident[Length] = 0;
				Compiler->Ident = Ident;
				Compiler->Token = MLT_METHOD;
				Compiler->Next = End;
				return Compiler->Token;
			} else if (Char == '\"') {
				Compiler->Next = Next + 1;
				ml_scan_string(Compiler);
				Compiler->Token = MLT_METHOD;
				return Compiler->Token;
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
						++Compiler->Line;
						break;
					case 0:
						Next = Compiler->Read(Compiler->Data);
						if (!Next) {
							Compiler->Next = "";
							ml_parse_error(Compiler, "ParseError", "End of input in comment");
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
				Compiler->Token = MLT_INLINE;
				Compiler->Next = Next + 1;
				return Compiler->Token;
			} else if (Char == '$') {
				Compiler->Token = MLT_EXPAND;
				Compiler->Next = Next + 1;
				return Compiler->Token;
			} else {
				Compiler->Token = MLT_COLON;
				Compiler->Next = Next;
				return Compiler->Token;
			}
		}
		DO_CHAR_DELIM:
			Compiler->Next = Next + 1;
			Compiler->Token = CharTokens[(unsigned char)Char];
			return Compiler->Token;
		DO_CHAR_OPER: {
			if (Char == '-' || Char == '.') {
				if (ml_isdigit(Next[1])) goto DO_CHAR_DIGIT;
			}
			const char *End = Next + 1;
			while (ml_isoperator(*End)) ++End;
			int Length = End - Next;
			char *Operator = snew(Length + 1);
			memcpy(Operator, Next, Length);
			Operator[Length] = 0;
			Compiler->Ident = Operator;
			Compiler->Token = MLT_OPERATOR;
			Compiler->Next = End;
			return Compiler->Token;
		}
		DO_CHAR_OTHER:
			ml_parse_error(Compiler, "ParseError", "unexpected character <%c>", Char);
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
		ml_parse_error(Compiler, "ParseError", "expected %s not %s (%s)", MLTokens[Token], MLTokens[Compiler->Token], Compiler->Ident);
	} else {
		ml_parse_error(Compiler, "ParseError", "expected %s not %s", MLTokens[Token], MLTokens[Compiler->Token]);
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
	if (!Method) ml_parse_error(Compiler, "ParseError", "expected <factor> not <%s>", MLTokens[Compiler->Token]);
	MethodExpr->Child = Method;
	mlc_expr_t **ArgsSlot = &Method->Next;
	ml_accept(Compiler, MLT_LEFT_PAREN);
	ML_EXPR(FunExpr, fun, fun);
	FunExpr->Source = Compiler->Source.Name;
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
					ml_parse_error(Compiler, "ParseError", "expected <identfier> not %s (%s)", MLTokens[Compiler->Token], Compiler->Ident);
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
				ml_parse_error(Compiler, "ParseError", "Argument names must be identifiers or string");
			}
			ml_names_add(Names, Compiler->Value);
		} else {
			ml_parse_error(Compiler, "ParseError", "Argument names must be identifiers or string");
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
						ml_parse_error(Compiler, "ParseError", "Argument names must be identifiers or strings");
					}
					ml_names_add(Names, Name);
				} else {
					ml_parse_error(Compiler, "ParseError", "Argument names must be identifiers or strings");
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
			Local->Index = 0;
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
		[MLT_WHILE] = ml_or_expr_compile,
		[MLT_UNTIL] = ml_and_expr_compile,
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
		ML_EXPR(ExitExpr, parent, exit);
		if (ml_parse(Compiler, MLT_COMMA)) {
			ExitExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
		} else {
			mlc_expr_t *RegisterExpr = new(mlc_expr_t);
			RegisterExpr->compile = ml_register_expr_compile;
			RegisterExpr->StartLine = RegisterExpr->EndLine = Compiler->Source.Line;
			ExitExpr->Child = RegisterExpr;
		}
		ParentExpr->Child->Next = ML_EXPR_END(ExitExpr);
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
	case MLT_SWITCH: {
		ml_next(Compiler);
		ML_EXPR(CaseExpr, parent, case);
		mlc_expr_t *Child = CaseExpr->Child = ml_accept_expression(Compiler, EXPR_DEFAULT);
		while (ml_parse2(Compiler, MLT_CASE)) {
			Child = Child->Next = ml_accept_block(Compiler);
		}
		ml_accept(Compiler, MLT_END);
		return ML_EXPR_END(CaseExpr);
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
					if (!Method) ml_parse_error(Compiler, "ParseError", "Expected operator not %s", MLTokens[Compiler->Token]);
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
			FunExpr->Source = Compiler->Source.Name;
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
	if (!Expr) ml_parse_error(Compiler, "ParseError", "expected <expression> not %s", MLTokens[Compiler->Token]);
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
			FunExpr->Source = Compiler->Source.Name;
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
	if (!Expr) ml_parse_error(Compiler, "ParseError", "expected <expression> not %s", MLTokens[Compiler->Token]);
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
			if (!Child) ml_parse_error(Compiler, "ParseError", "Expected expression");
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
					ml_parse_error(Compiler, "ParseError", "Invalid declaration");
				}
				mlc_ident_expr_t *IdentExpr = (mlc_ident_expr_t *)CallExpr->Child;
				if (!IdentExpr || IdentExpr->compile != ml_ident_expr_compile) {
					ml_parse_error(Compiler, "ParseError", "Invalid declaration");
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
						ml_parse_error(Compiler, "ParseError", "Expected <string> not <%s>", ml_typeof(Value)->Name);
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

static void ml_function_compile2(mlc_function_t *Function, ml_value_t *Value, mlc_compile_frame_t *Frame) {
	ml_closure_info_t *Info = Frame->Info;
	mlc_expr_t *Expr = Frame->Expr;
	Info->Return = MLC_EMIT(Expr->EndLine, MLI_RETURN, 0);
	MLC_LINK(Function->Returns, Info->Return);
	Info->Halt = Function->Next;
	Info->StartLine = Expr->StartLine;
	Info->EndLine = Expr->EndLine;
	Info->FrameSize = Function->Size;
	Info->Decls = Function->Decls;
	ml_closure_t *Closure = new(ml_closure_t);
	Closure->Info = Info;
	Closure->Type = MLClosureT;
	ml_state_t *Caller = Function->Base.Caller;
	ML_RETURN(Closure);
}

void ml_function_compile(ml_state_t *Caller, ml_compiler_t *Compiler, const char **Parameters) {
	if (setjmp(Compiler->OnError)) ML_RETURN(Compiler->Value);
	mlc_expr_t *Expr = ml_accept_block(Compiler);
	ml_accept_eoi(Compiler);
	mlc_function_t *Function = new(mlc_function_t);
	Function->Base.Caller = Caller;
	Function->Base.Context = Caller->Context;
	Function->Base.run = (ml_state_fn)mlc_function_run;
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
	Info->NumParams = NumParams;
	Info->Source = Function->Source;
	Function->Next = Info->Entry = anew(ml_inst_t, 128);
	Function->Space = 126;
	Function->Returns = NULL;
	MLC_FRAME(mlc_compile_frame_t, ml_function_compile2);
	Frame->Info = Info;
	Frame->Expr = Expr;
	mlc_compile(Function, Expr, 0);
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

static ml_global_t *ml_command_global(stringmap_t *Globals, const char *Name) {
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
	return Global;
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
	if (Frame->Type != MLT_REF) Value = ml_deref(Value);
	if (Frame->Type == MLT_VAR) {
		ml_variable_t *Var = new(ml_variable_t);
		Var->Type = MLVariableT;
		Var->Value = Value;
		Value = (ml_value_t *)Var;
	}
	Global->Value = Value;
	if (Frame->Index) {
		int Index = --Frame->Index;
		Frame->Args[1] = ml_cstring(Frame->Globals[Index]->Name);
		return ml_call(Function, SymbolMethod, 2, Frame->Args);
	} else {
		MLC_POP();
		MLC_RETURN(Frame->Args[0]);
	}
}

static void ml_command_idents_in(mlc_function_t *Function, ml_value_t *Value, ml_command_idents_frame_t *Frame) {
	Frame->Args[0] = Value;
	Frame->Args[1] = ml_cstring(Frame->Globals[Frame->Index]->Name);
	Function->Frame->run = (mlc_frame_fn)ml_command_idents_in2;
	return ml_call(Function, SymbolMethod, 2, Frame->Args);
}

static void ml_command_idents_unpack(mlc_function_t *Function, ml_value_t *Packed, ml_command_idents_frame_t *Frame) {
	for (int Index = 0; Index <= Frame->Index; ++Index) {
		ml_value_t *Value = ml_unpack(Packed, Index + 1);
		ml_global_t *Global = Frame->Globals[Index];
		if (Frame->Type != MLT_REF) Value = ml_deref(Value);
		if (Frame->Type == MLT_VAR) {
			ml_variable_t *Var = new(ml_variable_t);
			Var->Type = MLVariableT;
			Var->Value = Value;
			Value = (ml_value_t *)Var;
		}
		Global->Value = Value;
	}
	MLC_POP();
	MLC_RETURN(Packed);
}

static ml_command_idents_frame_t *ml_accept_command_idents(mlc_function_t *Function, ml_compiler_t *Compiler, int Index) {
	ml_accept(Compiler, MLT_IDENT);
	const char *Ident = Compiler->Ident;
	if (ml_parse(Compiler, MLT_COMMA)) {
		ml_command_idents_frame_t *Frame = ml_accept_command_idents(Function, Compiler, Index + 1);
		Frame->Globals[Index] = ml_command_global(Compiler->Vars, Ident);
		return Frame;
	}
	ml_accept(Compiler, MLT_RIGHT_PAREN);
	mlc_frame_fn FrameFn;
	if (ml_parse(Compiler, MLT_IN)) {
		FrameFn = (mlc_frame_fn)ml_command_idents_in;
	} else {
		ml_accept(Compiler, MLT_ASSIGN);
		FrameFn = (mlc_frame_fn)ml_command_idents_unpack;
	}
	int Count = Index + 1;
	MLC_XFRAME(ml_command_idents_frame_t, Count, const char *, FrameFn);
	Frame->Index = Index;
	Frame->Globals[Index] = ml_command_global(Compiler->Vars, Ident);
	return Frame;
}

typedef struct {
	ml_global_t *Global;
	mlc_expr_t *VarType;
	ml_token_t Type;
} ml_command_ident_frame_t;

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
	if (Frame->Type != MLT_REF) Value = ml_deref(Value);
	if (Frame->Type == MLT_VAR) {
		ml_variable_t *Var = new(ml_variable_t);
		Var->Type = MLVariableT;
		Var->Value = Value;
		//if (VarType) Var->VarType = VarType;
		Value = (ml_value_t *)Var;
	}
	Global->Value = Value;
	MLC_POP();
	MLC_RETURN(Value);
}

static void ml_accept_command_decl2(mlc_function_t *Function, ml_compiler_t *Compiler, ml_token_t Type) {
	if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
		ml_command_idents_frame_t *Frame = ml_accept_command_idents(Function, Compiler, 0);
		Frame->Type = Type;
		mlc_expr_t *Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
		return mlc_expr_call(Function, Expr);
	} else {
		MLC_FRAME(ml_command_ident_frame_t, ml_command_ident_run);
		ml_accept(Compiler, MLT_IDENT);
		Frame->Global = ml_command_global(Compiler->Vars, Compiler->Ident);
		Frame->VarType = NULL;
		Frame->Type = Type;
		if (ml_parse(Compiler, MLT_LEFT_PAREN)) {
			mlc_expr_t *Expr = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			return mlc_expr_call(Function, Expr);
		} else {
			if (ml_parse(Compiler, MLT_COLON)) Frame->VarType = ml_accept_term(Compiler);
			if (Type == MLT_VAR) {
				if (ml_parse(Compiler, MLT_ASSIGN)) {
					mlc_expr_t *Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
					return mlc_expr_call(Function, Expr);
				} else {
					return ml_command_ident_run(Function, MLNil, Frame);
				}
			} else {
				ml_accept(Compiler, MLT_ASSIGN);
				mlc_expr_t *Expr = ml_accept_expression(Compiler, EXPR_DEFAULT);
				return mlc_expr_call(Function, Expr);
			}
		}
	}
}

typedef struct {
	ml_token_t Type;
} ml_command_decl_frame_t;

static void ml_command_decl_run(mlc_function_t *Function, ml_value_t *Value, ml_command_decl_frame_t *Frame) {
	ml_compiler_t *Compiler = Function->Compiler;
	if (setjmp(Compiler->OnError)) MLC_RETURN(Compiler->Value);
	if (ml_parse(Compiler, MLT_COMMA)) {
		return ml_accept_command_decl2(Function, Compiler, Frame->Type);
	}
	ml_parse(Compiler, MLT_SEMICOLON);
	MLC_POP();
	MLC_RETURN(Value);
}

static void ml_accept_command_decl(mlc_function_t *Function, ml_token_t Type) {
	MLC_FRAME(ml_command_decl_frame_t, ml_command_decl_run);
	Frame->Type = Type;
	return ml_accept_command_decl2(Function, Function->Compiler, Type);
}

static void ml_command_evaluate2(mlc_function_t *Function, ml_value_t *Value, void *Data) {
	ml_state_t *Caller = Function->Base.Caller;
	ML_RETURN(Value);
}

void ml_command_evaluate(ml_state_t *Caller, ml_compiler_t *Compiler) {
	mlc_function_t *Function = new(mlc_function_t);
	Function->Base.Caller = (ml_state_t *)Caller;
	Function->Base.Context = Caller->Context;
	Function->Base.run = (ml_state_fn)mlc_function_run;
	Function->Compiler = Compiler;
	Function->Source = Compiler->Source.Name;
	Function->Up = NULL;
	__attribute__((unused)) MLC_FRAME(void, ml_command_evaluate2);
	if (setjmp(Compiler->OnError)) MLC_RETURN(Compiler->Value);
	ml_skip_eol(Compiler);
	if (ml_parse(Compiler, MLT_EOI)) MLC_RETURN(MLEndOfInput);
	if (ml_parse(Compiler, MLT_VAR)) {
		return ml_accept_command_decl(Function, MLT_VAR);
	} else if (ml_parse(Compiler, MLT_LET)) {
		return ml_accept_command_decl(Function, MLT_LET);
	} else if (ml_parse(Compiler, MLT_REF)) {
		return ml_accept_command_decl(Function, MLT_REF);
	} else if (ml_parse(Compiler, MLT_DEF)) {
		return ml_accept_command_decl(Function, MLT_DEF);
	} else if (ml_parse(Compiler, MLT_FUN)) {
		if (ml_parse(Compiler, MLT_IDENT)) {
			MLC_FRAME(ml_command_ident_frame_t, ml_command_ident_run);
			Frame->Global = ml_command_global(Compiler->Vars, Compiler->Ident);
			Frame->VarType = NULL;
			Frame->Type = MLT_LET;
			ml_accept(Compiler, MLT_LEFT_PAREN);
			mlc_expr_t *Expr = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			ml_parse(Compiler, MLT_SEMICOLON);
			return mlc_expr_call(Function, Expr);
		} else {
			ml_accept(Compiler, MLT_LEFT_PAREN);
			mlc_expr_t *Expr = ml_accept_fun_expr(Compiler, MLT_RIGHT_PAREN);
			ml_parse(Compiler, MLT_SEMICOLON);
			return mlc_expr_call(Function, Expr);
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
		MLC_RETURN((ml_value_t *)Macro);
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
			MLC_FRAME(ml_command_ident_frame_t, ml_command_ident_run);
			Frame->Global = ml_command_global(Compiler->Vars, Ident);
			Frame->VarType = NULL;
			Frame->Type = MLT_LET;
			return mlc_expr_call(Function, ML_EXPR_END(CallExpr));
		} else {
			ml_parse(Compiler, MLT_SEMICOLON);
			return mlc_expr_call(Function, Expr);
		}
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
	static const char *DefaultParameters[] = {"Args", NULL};
	if (!Parameters) Parameters = DefaultParameters;
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
	stringmap_insert(MLCompilerT->Exports, "EOI", MLEndOfInput);
	stringmap_insert(MLCompilerT->Exports, "NotFound", MLNotFound);
	stringmap_insert(StringFns, "r", ml_regex);
	stringmap_insert(StringFns, "ri", ml_regexi);
}

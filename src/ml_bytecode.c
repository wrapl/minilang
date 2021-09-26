#include "ml_macros.h"
#include "stringmap.h"
#include <gc/gc.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include "ml_bytecode.h"
#include "ml_debugger.h"

#ifndef DEBUG_VERSION

typedef struct {
	const ml_type_t *Type;
	ml_value_t *Value;
	ml_type_t *VarType;
} ml_variable_t;

static long ml_variable_hash(ml_variable_t *Variable, ml_hash_chain_t *Chain) {
	ml_value_t *Value = Variable->Value;
	return ml_typeof(Value)->hash(Value, Chain);
}

static ml_value_t *ml_variable_deref(ml_variable_t *Variable) {
	return Variable->Value;
}

static ml_value_t *ml_variable_assign(ml_variable_t *Variable, ml_value_t *Value) {
	if (Variable->VarType && !ml_is(Value, Variable->VarType)) {
		return ml_error("TypeError", "Cannot assign %s to variable of type %s", ml_typeof(Value)->Name, Variable->VarType->Name);
	}
	return (Variable->Value = Value);
}

static void ml_variable_call(ml_state_t *Caller, ml_variable_t *Variable, int Count, ml_value_t **Args) {
	return ml_call(Caller, Variable->Value, Count, Args);
}

ML_TYPE(MLVariableT, (), "variable",
	.hash = (void *)ml_variable_hash,
	.deref = (void *)ml_variable_deref,
	.assign = (void *)ml_variable_assign,
	.call = (void *)ml_variable_call
);

ml_value_t *ml_variable(ml_value_t *Value, ml_type_t *Type) {
	ml_variable_t *Variable = new(ml_variable_t);
	Variable->Type = MLVariableT;
	Variable->Value = Value;
	Variable->VarType = Type;
	return (ml_value_t *)Variable;
}

#endif

#ifdef ML_JIT
#include "ml_bytecode_jit.h"
#endif

#ifdef DEBUG_VERSION

#undef DEBUG_STRUCT
#undef DEBUG_FUNC
#undef DEBUG_TYPE
#undef DEBUG_VAR
#define DEBUG_STRUCT(X) ml_ ## X ## _debug_t
#define DEBUG_FUNC(X) ml_ ## X ## _debug
#define DEBUG_TYPE(X) ML ## X ## DebugT
#define DEBUG_VAR(X) ML ## X ## Debug

typedef struct DEBUG_STRUCT(frame) DEBUG_STRUCT(frame);

#else

#define DEBUG_STRUCT(X) ml_ ## X ## _t
#define DEBUG_FUNC(X) ml_ ## X
#define DEBUG_TYPE(X) ML ## X ## T
#define DEBUG_VAR(X) ML ## X

#endif

struct DEBUG_STRUCT(frame) {
	ml_state_t Base;
	union {
		void *Next;
		ml_inst_t *Inst;
	};
	ml_value_t **Top;
	const char *Source;
	ml_inst_t *OnError;
	ml_value_t **UpValues;
#ifdef ML_SCHEDULER
	ml_schedule_t Schedule;
#endif
	unsigned int Line;
	char Continue, Reentry, Suspend;
	/*unsigned int Continue:1;
	unsigned int Reentry:1;
	unsigned int Suspend:1;*/
#ifdef DEBUG_VERSION
	unsigned int StepOver:1;
	unsigned int StepOut:1;
	ml_debugger_t *Debugger;
	size_t *Breakpoints;
	ml_decl_t *Decls;
	size_t Revision;
#endif
	ml_value_t *Stack[];
};

static void DEBUG_FUNC(continuation_call)(ml_state_t *Caller, DEBUG_STRUCT(frame) *Frame, int Count, ml_value_t **Args) {
	if (Frame->Suspend) ML_ERROR("StateError", "Cannot call suspended function");
	Frame->Continue = 1;
	Frame->Base.Caller = Caller;
	Frame->Base.Context = Caller->Context;
	return Frame->Base.run((ml_state_t *)Frame, Count ? Args[0] : MLNil);
}

ML_TYPE(DEBUG_TYPE(Continuation), (MLStateT, MLSequenceT), "continuation",
//!internal
	.call = (void *)DEBUG_FUNC(continuation_call)
);

static int ML_TYPED_FN(ml_debugger_check, DEBUG_TYPE(Continuation), DEBUG_STRUCT(frame) *Frame) {
#ifdef DEBUG_VERSION
	return 1;
#else
	return 0;
#endif
}

static void ML_TYPED_FN(ml_debugger_step_mode, DEBUG_TYPE(Continuation), DEBUG_STRUCT(frame) *Frame, int StepOver, int StepOut) {
#ifdef DEBUG_VERSION
	Frame->StepOver = StepOver;
	Frame->StepOut = StepOut;
#else
#endif
}

static ml_source_t ML_TYPED_FN(ml_debugger_source, DEBUG_TYPE(Continuation), DEBUG_STRUCT(frame) *Frame) {
	return (ml_source_t){Frame->Source, Frame->Line};
}

static ml_decl_t *ML_TYPED_FN(ml_debugger_decls, DEBUG_TYPE(Continuation), DEBUG_STRUCT(frame) *Frame) {
#ifdef DEBUG_VERSION
	return Frame->Decls;
#else
	return NULL;
#endif
}

static ml_value_t *ML_TYPED_FN(ml_debugger_local, DEBUG_TYPE(Continuation), DEBUG_STRUCT(frame) *Frame, int Index) {
	if (Index < 0) return Frame->UpValues[~Index];
	return Frame->Stack[Index];
}

static void ML_TYPED_FN(ml_iter_value, DEBUG_TYPE(Continuation), ml_state_t *Caller, DEBUG_STRUCT(frame) *Suspension) {
	if (!Suspension->Suspend) ML_ERROR("StateError", "Function did not suspend");
	ML_RETURN(Suspension->Top[-1]);
}

static void ML_TYPED_FN(ml_iter_key, DEBUG_TYPE(Continuation), ml_state_t *Caller, DEBUG_STRUCT(frame) *Suspension) {
	if (!Suspension->Suspend) ML_ERROR("StateError", "Function did not suspend");
	ML_RETURN(Suspension->Top[-2]);
}

static void ML_TYPED_FN(ml_iter_next, DEBUG_TYPE(Continuation), ml_state_t *Caller, DEBUG_STRUCT(frame) *Suspension) {
	if (!Suspension->Suspend) ML_CONTINUE(Caller, MLNil);
	//Suspension->Top[-2] = Suspension->Top[-1];
	//--Suspension->Top;
	Suspension->Base.Caller = Caller;
	Suspension->Base.Context = Caller->Context;
	ML_CONTINUE(Suspension, MLNil);
}

static void ML_TYPED_FN(ml_iterate, DEBUG_TYPE(Continuation), ml_state_t *Caller, DEBUG_STRUCT(frame) *Suspension) {
	if (!Suspension->Suspend) ML_ERROR("StateError", "Function did not suspend");
	ML_RETURN(Suspension);
}

#ifndef DEBUG_VERSION

#ifdef ML_SCHEDULER
#define CHECK_COUNTER if (__builtin_expect(--Counter == 0, 0)) goto DO_SWAP;
#else
#define CHECK_COUNTER
#endif

#define ERROR() { \
	Inst = Frame->OnError; \
	CHECK_COUNTER \
	goto *Labels[Inst->Opcode]; \
}

#define ADVANCE(NEXT) { \
	Inst = NEXT; \
	CHECK_COUNTER \
	goto *Labels[Inst->Opcode]; \
}

#define ERROR_CHECK(VALUE) if (ml_is_error(VALUE)) { \
	ml_error_trace_add(VALUE, (ml_source_t){Frame->Source, Inst->Line}); \
	Result = VALUE; \
	ERROR(); \
}

#else

#undef ERROR
#undef ADVANCE

#define ERROR() { \
	Inst = Frame->OnError; \
	CHECK_COUNTER \
	goto DO_DEBUG_ERROR; \
}

#define ADVANCE(NEXT) { \
	Inst = NEXT; \
	CHECK_COUNTER \
	goto DO_DEBUG_ADVANCE; \
}

#endif

#ifndef DEBUG_VERSION

static ml_frame_t *MLCachedFrame = NULL;

#ifdef ML_THREADSAFE

#include <stdatomic.h>

static volatile atomic_flag MLCachedFrameLock[1] = {ATOMIC_FLAG_INIT};

#define ML_CACHED_FRAME_LOCK() while (atomic_flag_test_and_set(MLCachedFrameLock))
#define ML_CACHED_FRAME_UNLOCK() atomic_flag_clear(MLCachedFrameLock)

#else

#define ML_CACHED_FRAME_LOCK() {}
#define ML_CACHED_FRAME_UNLOCK() {}

#endif

extern ml_value_t *SymbolMethod;

#define ML_FRAME_REUSE_SIZE 384

#endif

static ML_METHOD_DECL(AppendMethod, "append");

static void DEBUG_FUNC(frame_run)(DEBUG_STRUCT(frame) *Frame, ml_value_t *Result) {
	static const void *Labels[] = {
		[MLI_LINK] = &&DO_LINK,
		[MLI_RETURN] = &&DO_RETURN,
		[MLI_SUSPEND] = &&DO_SUSPEND,
		[MLI_RESUME] = &&DO_RESUME,
		[MLI_NIL] = &&DO_NIL,
		[MLI_NIL_PUSH] = &&DO_NIL_PUSH,
		[MLI_SOME] = &&DO_SOME,
		[MLI_AND] = &&DO_AND,
		[MLI_OR] = &&DO_OR,
		[MLI_NOT] = &&DO_NOT,
		[MLI_PUSH] = &&DO_PUSH,
		[MLI_WITH] = &&DO_WITH,
		[MLI_WITH_VAR] = &&DO_WITH_VAR,
		[MLI_WITHX] = &&DO_WITHX,
		[MLI_POP] = &&DO_POP,
		[MLI_ENTER] = &&DO_ENTER,
		[MLI_EXIT] = &&DO_EXIT,
		[MLI_GOTO] = &&DO_GOTO,
		[MLI_TRY] = &&DO_TRY,
		[MLI_CATCH_TYPE] = &&DO_CATCH_TYPE,
		[MLI_CATCH] = &&DO_CATCH,
		[MLI_RETRY] = &&DO_RETRY,
		[MLI_LOAD] = &&DO_LOAD,
		[MLI_LOAD_PUSH] = &&DO_LOAD_PUSH,
		[MLI_VAR] = &&DO_VAR,
		[MLI_VAR_TYPE] = &&DO_VAR_TYPE,
		[MLI_VARX] = &&DO_VARX,
		[MLI_LET] = &&DO_LET,
		[MLI_LETI] = &&DO_LETI,
		[MLI_LETX] = &&DO_LETX,
		[MLI_REF] = &&DO_REF,
		[MLI_REFI] = &&DO_REFI,
		[MLI_REFX] = &&DO_REFX,
		[MLI_FOR] = &&DO_FOR,
		[MLI_ITER] = &&DO_ITER,
		[MLI_NEXT] = &&DO_NEXT,
		[MLI_VALUE] = &&DO_VALUE,
		[MLI_KEY] = &&DO_KEY,
		[MLI_CALL] = &&DO_CALL,
		[MLI_CONST_CALL] = &&DO_CONST_CALL,
		[MLI_ASSIGN] = &&DO_ASSIGN,
		[MLI_LOCAL] = &&DO_LOCAL,
		[MLI_LOCAL_PUSH] = &&DO_LOCAL_PUSH,
		[MLI_UPVALUE] = &&DO_UPVALUE,
		[MLI_LOCALX] = &&DO_LOCALX,
		[MLI_TUPLE_NEW] = &&DO_TUPLE_NEW,
		[MLI_UNPACK] = &&DO_UNPACK,
		[MLI_LIST_NEW] = &&DO_LIST_NEW,
		[MLI_LIST_APPEND] = &&DO_LIST_APPEND,
		[MLI_MAP_NEW] = &&DO_MAP_NEW,
		[MLI_MAP_INSERT] = &&DO_MAP_INSERT,
		[MLI_CLOSURE] = &&DO_CLOSURE,
		[MLI_CLOSURE_TYPED] = &&DO_CLOSURE_TYPED,
		[MLI_PARAM_TYPE] = &&DO_PARAM_TYPE,
		[MLI_PARTIAL_NEW] = &&DO_PARTIAL_NEW,
		[MLI_PARTIAL_SET] = &&DO_PARTIAL_SET,
		[MLI_STRING_NEW] = &&DO_STRING_NEW,
		[MLI_STRING_ADD] = &&DO_STRING_ADD,
		[MLI_STRING_ADDS] = &&DO_STRING_ADDS,
		[MLI_STRING_END] = &&DO_STRING_END,
		[MLI_RESOLVE] = &&DO_RESOLVE,
		[MLI_IF_DEBUG] = &&DO_IF_DEBUG,
		[MLI_ASSIGN_LOCAL] = &&DO_ASSIGN_LOCAL,
		[MLI_SWITCH] = &&DO_SWITCH
	};
	if (!Result) {
		ml_value_t *Error = ml_error("RuntimeError", "NULL value passed to continuation");
		ml_error_trace_add(Error, (ml_source_t){Frame->Source, Frame->Inst->Line});
		ML_CONTINUE(Frame->Base.Caller, Error);
	}
#ifdef ML_SCHEDULER
	uint64_t Counter = Frame->Schedule.Counter[0];
#endif
	ml_inst_t *Inst = Frame->Inst;
	ml_value_t **Top = Frame->Top;
#ifdef DEBUG_VERSION
	int Line = Frame->Line;
	if (Frame->Reentry) {
		Frame->Reentry = 0;
		goto *Labels[Inst->Opcode];
	}
#endif
	if (ml_is_error(Result)) {
		ml_error_trace_add(Result, (ml_source_t){Frame->Source, Frame->Line});
		ERROR();
	}
#ifdef DEBUG_VERSION
	goto DO_DEBUG_ADVANCE;
#else
	goto *Labels[Inst->Opcode];
#endif

	DO_LINK: {
		Inst = Inst[1].Inst;
		goto *Labels[Inst->Opcode];
	}
	DO_RETURN: {
#ifdef ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		ml_state_t *Caller = Frame->Base.Caller;
		if (!Frame->Continue) {
			//memset(Frame, 0, ML_FRAME_REUSE_SIZE);
			//while (Top > Frame->Stack) *--Top = NULL;
			memset(Frame->Stack, 0, (Top - Frame->Stack) * sizeof(ml_value_t *));
			ML_CACHED_FRAME_LOCK();
			Frame->Next = MLCachedFrame;
			MLCachedFrame = (ml_frame_t *)Frame;
			ML_CACHED_FRAME_UNLOCK();
		} else {
			Frame->Line = Inst->Line;
			Frame->Inst = Inst;
		}
		ML_CONTINUE(Caller, Result);
	}
	DO_SUSPEND: {
		Frame->Line = Inst->Line;
		Frame->Inst = Inst + 1;
		Frame->Top = Top;
#ifdef ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		Frame->Suspend = 1;
		ML_CONTINUE(Frame->Base.Caller, (ml_value_t *)Frame);
	}
	DO_RESUME: {
		Frame->Suspend = 0;
		*--Top = NULL;
		*--Top = NULL;
		ADVANCE(Inst + 1);
	}
	DO_NIL: {
		Result = MLNil;
		ADVANCE(Inst + 1);
	}
	DO_NIL_PUSH: {
		Result = MLNil;
		*Top++ = Result;
		ADVANCE(Inst + 1);
	}
	DO_SOME: {
		Result = MLSome;
		ADVANCE(Inst + 1);
	}
	DO_AND: {
		ADVANCE((ml_deref(Result) == MLNil) ? Inst[1].Inst : (Inst + 2));
	}
	DO_OR: {
		ADVANCE((ml_deref(Result) != MLNil) ? Inst[1].Inst : (Inst + 2));
	}
	DO_NOT: {
		Result = (ml_deref(Result) == MLNil) ? MLSome : MLNil;
		ADVANCE(Inst + 1);
	}
	DO_PUSH: {
		*Top++ = Result;
		ADVANCE(Inst + 1);
	}
	DO_WITH: {
		*Top++ = Result;
#ifdef DEBUG_VERSION
		Frame->Decls = Inst[1].Decls;
#endif
		ADVANCE(Inst + 2);
	}
	DO_WITH_VAR: {
		ml_variable_t *Local = new(ml_variable_t);
		Local->Type = MLVariableT;
		Local->Value = Result;
		*Top++ = (ml_value_t *)Local;
#ifdef DEBUG_VERSION
		Frame->Decls = Inst[1].Decls;
#endif
		ADVANCE(Inst + 2);
	}
	DO_WITHX: {
		ml_value_t *Packed = Result;
		int Count = Inst[1].Count;
		for (int I = 0; I < Count; ++I) {
			Result = ml_unpack(Packed, I + 1);
			*Top++ = Result;
		}
#ifdef DEBUG_VERSION
		Frame->Decls = Inst[2].Decls;
#endif
		ADVANCE(Inst + 3);
	}
	DO_POP: {
		Result = *--Top;
		*Top = NULL;
		ADVANCE(Inst + 1);
	}
	DO_ENTER: {
		for (int I = Inst[1].Count; --I >= 0;) {
			ml_variable_t *Local = new(ml_variable_t);
			Local->Type = MLVariableT;
			Local->Value = MLNil;
			*Top++ = (ml_value_t *)Local;
		}
		for (int I = Inst[2].Count; --I >= 0;) {
			*Top++ = NULL;
		}
#ifdef DEBUG_VERSION
		Frame->Decls = Inst[3].Decls;
#endif
		ADVANCE(Inst + 4);
	}
	DO_EXIT: {
		for (int I = Inst[1].Count; --I >= 0;) *--Top = NULL;
#ifdef DEBUG_VERSION
		Frame->Decls = Inst[2].Decls;
#endif
		ADVANCE(Inst + 3);
	}
	DO_GOTO: {
		ADVANCE(Inst[1].Inst);
	}
	DO_TRY: {
		Frame->OnError = Inst[1].Inst;
		ADVANCE(Inst + 2);
	}
	DO_CATCH_TYPE: {
		if (!ml_is_error(Result)) {
			Result = ml_error("InternalError", "expected error value, not %s", ml_typeof(Result)->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->Line});
			ERROR();
		}
		const char *Type = ml_error_type(Result);
		for (const char **Ptr = Inst[2].Ptrs; *Ptr; ++Ptr) {
			if (!Ptr) break;
			if (!strcmp(*Ptr, Type)) goto match;
		}
		ADVANCE(Inst[1].Inst);
	match:
		ADVANCE(Inst + 3);
	}
	DO_CATCH: {
		if (!ml_is_error(Result)) {
			Result = ml_error("InternalError", "expected error value, not %s", ml_typeof(Result)->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->Line});
			ERROR();
		}
		Result = ml_error_value(Result);
		ml_value_t **Old = Frame->Stack + Inst[1].Index;
		while (Top > Old) *--Top = NULL;
		*Top++ = Result;
#ifdef DEBUG_VERSION
		Frame->Decls = Inst[2].Decls;
#endif
		ADVANCE(Inst + 3);
	}
	DO_RETRY: {
		ERROR();
	}
	DO_LOAD: {
		Result = Inst[1].Value;
		ADVANCE(Inst + 2);
	}
	DO_LOAD_PUSH: {
		Result = Inst[1].Value;
		*Top++ = Result;
		ADVANCE(Inst + 2);
	}
	DO_VAR: {
		Result = ml_deref(Result);
		ml_variable_t *Variable = (ml_variable_t *)Top[Inst[1].Index];
		if (Variable->VarType && !ml_is(Result, Variable->VarType)) {
			Result = ml_error("TypeError", "Cannot assign %s to variable of type %s", ml_typeof(Result)->Name, Variable->VarType->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->Line});
			ERROR();
		}
		Variable->Value = Result;
		ADVANCE(Inst + 2);
	}
	DO_VAR_TYPE: {
		Result = ml_deref(Result);
		if (!ml_is(Result, MLTypeT)) {
			Result = ml_error("TypeError", "expected type, not %s", ml_typeof(Result)->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->Line});
			ERROR();
		}
		ml_variable_t *Local = (ml_variable_t *)Top[Inst[1].Index];
		Local->VarType = (ml_type_t *)Result;
		ADVANCE(Inst + 2);
	}
	DO_VARX: {
		ml_value_t *Packed = ml_deref(Result);
		int Count = Inst[2].Count;
		ml_value_t **Base = Top + Inst[1].Index;
		for (int I = 0; I < Count; ++I) {
			Result = ml_unpack(Packed, I + 1);
			ERROR_CHECK(Result);
			Result = ml_deref(Result);
			ml_variable_t *Local = (ml_variable_t *)Base[I];
			Local->Value = Result;
		}
		ADVANCE(Inst + 3);
	}
	DO_LET: {
		Result = ml_deref(Result);
		Top[Inst[1].Index] = Result;
		//ml_value_set_name(Result, Inst[2].Chars);
		ADVANCE(Inst + 2);
	}
	DO_LETI: {
		Result = ml_deref(Result);
		ml_value_t *Uninitialized = Top[Inst[1].Index];
		Top[Inst[1].Index] = Result;
		if (Uninitialized) ml_uninitialized_set(Uninitialized, Result);
		ADVANCE(Inst + 2);
	}
	DO_LETX: {
		ml_value_t *Packed = ml_deref(Result);
		int Count = Inst[2].Count;
		ml_value_t **Base = Top + Inst[1].Index;
		for (int I = 0; I < Count; ++I) {
			Result = ml_unpack(Packed, I + 1);
			ERROR_CHECK(Result);
			Result = ml_deref(Result);
			ml_value_t *Uninitialized = Base[I];
			Base[I] = Result;
			if (Uninitialized) ml_uninitialized_set(Uninitialized, Result);
		}
		ADVANCE(Inst + 3);
	}
	DO_REF: {
		Top[Inst[1].Index] = Result;
		ADVANCE(Inst + 2);
	}
	DO_REFI: {
		ml_value_t *Uninitialized = Top[Inst[1].Index];
		Top[Inst[1].Index] = Result;
		if (Uninitialized) ml_uninitialized_set(Uninitialized, Result);
		ADVANCE(Inst + 2);
	}
	DO_REFX: {
		ml_value_t *Packed = ml_deref(Result);
		int Count = Inst[2].Count;
		ml_value_t **Base = Top + Inst[1].Index;
		for (int I = 0; I < Count; ++I) {
			Result = ml_unpack(Packed, I + 1);
			ERROR_CHECK(Result);
			ml_value_t *Uninitialized = Base[I];
			Base[I] = Result;
			if (Uninitialized) ml_uninitialized_set(Uninitialized, Result);
		}
		ADVANCE(Inst + 3);
	}
	DO_FOR: {
		Result = ml_deref(Result);
		Frame->Line = Inst->Line;
		Frame->Inst = Inst + 1;
		Frame->Top = Top;
#ifdef ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		return ml_iterate((ml_state_t *)Frame, Result);
	}
	DO_ITER: {
		if (Result == MLNil) {
			ADVANCE(Inst[1].Inst);
		} else {
			*Top++ = Result;
			ADVANCE(Inst + 2);
		}
	}
	DO_NEXT: {
		Result = *--Top;
		*Top = NULL;
		Frame->Line = Inst->Line;
		Frame->Inst = Inst[1].Inst;
		Frame->Top = Top;
#ifdef ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		return ml_iter_next((ml_state_t *)Frame, Result);
	}
	DO_VALUE: {
		Result = Top[Inst[1].Index];
		Frame->Line = Inst->Line;
		Frame->Inst = Inst + 2;
		Frame->Top = Top;
#ifdef ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		return ml_iter_value((ml_state_t *)Frame, Result);
	}
	DO_KEY: {
		Result = Top[Inst[1].Index];
		Frame->Line = Inst->Line;
		Frame->Inst = Inst + 2;
		Frame->Top = Top;
#ifdef ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		return ml_iter_key((ml_state_t *)Frame, Result);
	}
	DO_CALL: {
		int Count = Inst[1].Count;
		ml_value_t *Function = Top[~Count];
		ml_value_t **Args = Top - Count;
		ml_inst_t *Next = Inst + 2;
#ifdef ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		if (Next->Opcode == MLI_RETURN && !Frame->Continue) {
			// Ensure at least one other cached frame is available to prevent this frame being used immediately which may result in arguments being overwritten.
			ML_CACHED_FRAME_LOCK();
			if (!MLCachedFrame) {
				MLCachedFrame = GC_MALLOC(ML_FRAME_REUSE_SIZE);
			}
			Frame->Next = MLCachedFrame->Next;
			MLCachedFrame->Next = Frame;
			ML_CACHED_FRAME_UNLOCK();
			return ml_call(Frame->Base.Caller, Function, Count, Args);
		} else {
			Frame->Inst = Next;
			Frame->Line = Inst->Line;
			Frame->Top = Top - (Count + 1);
			return ml_call(Frame, Function, Count, Args);
		}
	}
	DO_CONST_CALL: {
		int Count = Inst[1].Count;
		ml_value_t *Function = Inst[2].Value;
		ml_value_t **Args = Top - Count;
		ml_inst_t *Next = Inst + 3;
#ifdef ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		if (Next->Opcode == MLI_RETURN && !Frame->Continue) {
			// Ensure at least one other cached frame is available to prevent this frame being used immediately which may result in arguments being overwritten.
			ML_CACHED_FRAME_LOCK();
			if (!MLCachedFrame) {
				MLCachedFrame = GC_MALLOC(ML_FRAME_REUSE_SIZE);
			}
			Frame->Next = MLCachedFrame->Next;
			MLCachedFrame->Next = Frame;
			ML_CACHED_FRAME_UNLOCK();
			return ml_call(Frame->Base.Caller, Function, Count, Args);
		} else {
			Frame->Inst = Next;
			Frame->Line = Inst->Line;
			Frame->Top = Top - Count;
			return ml_call(Frame, Function, Count, Args);
		}
	}
	DO_ASSIGN: {
		Result = ml_deref(Result);
		ml_value_t *Ref = Top[-1];
		*--Top = NULL;
		Result = ml_assign(Ref, Result);
		ERROR_CHECK(Result);
		ADVANCE(Inst + 1);
	}
	DO_LOCAL: {
		int Index = Inst[1].Index;
		Result = Frame->Stack[Index];
		ADVANCE(Inst + 2);
	}
	DO_ASSIGN_LOCAL: {
		Result = ml_deref(Result);
		int Index = Inst[1].Index;
		ml_value_t *Ref = Frame->Stack[Index];
		Result = ml_assign(Ref, Result);
		ERROR_CHECK(Result);
		ADVANCE(Inst + 2);
	}
	DO_LOCAL_PUSH: {
		int Index = Inst[1].Index;
		Result = Frame->Stack[Index];
		*Top++ = Result;
		ADVANCE(Inst + 2);
	}
	DO_UPVALUE: {
		int Index = Inst[1].Index;
		Result = Frame->UpValues[Index];
		ADVANCE(Inst + 2);
	}
	DO_LOCALX: {
		int Index = Inst[1].Index;
		ml_value_t **Slot = &Frame->Stack[Index];
		Result = Slot[0];
		if (!Result) Result = Slot[0] = ml_uninitialized(Inst[2].Chars);
		ADVANCE(Inst + 3);
	}
	DO_TUPLE_NEW: {
		int Count = Inst[1].Count;
		Result = ml_tuple(Count);
#ifdef ML_GENERICS
		for (int I = Count; --I > 0;) {
			((ml_tuple_t *)Result)->Values[I] = Top[-1];
			*--Top = NULL;
		}
		ml_tuple_set(Result, 1, Top[-1]);
		*--Top = NULL;
#else
		for (int I = Count; --I >= 0;) {
			((ml_tuple_t *)Result)->Values[I] = Top[-1];
			*--Top = NULL;
		}
#endif
		ADVANCE(Inst + 2);
	}
	DO_UNPACK: {
		ADVANCE(Inst + 2);
	}
	DO_LIST_NEW: {
		*Top++ = ml_list();
		ADVANCE(Inst + 1);
	}
	DO_LIST_APPEND: {
		Result = ml_deref(Result);
		ml_list_put(Top[-1], Result);
		ADVANCE(Inst + 1);
	}
	DO_MAP_NEW: {
		*Top++ = ml_map();
		ADVANCE(Inst + 1);
	}
	DO_MAP_INSERT: {
		ml_value_t *Key = ml_deref(Top[-1]);
		Result = ml_deref(Result);
		ml_map_insert(Top[-2], Key, Result);
		*--Top = NULL;
		ADVANCE(Inst + 1);
	}
	DO_CLOSURE: {
		// closure <entry> <frame_size> <num_params> <num_upvalues> <upvalue_1> ...
		ml_closure_info_t *Info = Inst[1].ClosureInfo;
		ml_closure_t *Closure = xnew(ml_closure_t, Info->NumUpValues, ml_value_t *);
		Closure->Type = MLClosureT;
		Closure->Info = Info;
		for (int I = 0; I < Info->NumUpValues; ++I) {
			int Index = Inst[2 + I].Index;
			ml_value_t **Slot = (Index < 0) ? &Frame->UpValues[~Index] : &Frame->Stack[Index];
			ml_value_t *Value = Slot[0];
			if (!Value) Value = Slot[0] = ml_uninitialized("<upvalue>");
			if (ml_typeof(Value) == MLUninitializedT) {
				ml_uninitialized_use(Value, &Closure->UpValues[I]);
			}
			Closure->UpValues[I] = Value;
		}
		Result = (ml_value_t *)Closure;
		ADVANCE(Inst + Info->NumUpValues + 2);
	}
	DO_CLOSURE_TYPED: {
#ifdef ML_GENERICS
		// closure <entry> <frame_size> <num_params> <num_upvalues> <upvalue_1> ...
		if (!ml_is(Result, MLTypeT)) {
			Result = ml_error("InternalError", "expected type, not %s", ml_typeof(Result)->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->Line});
			ERROR();
		}
		ml_closure_info_t *Info = Inst[1].ClosureInfo;
		ml_closure_t *Closure = xnew(ml_closure_t, Info->NumUpValues, ml_value_t *);
		// A new block is necessary here to allow GCC to perform TCO in this function
		Closure->Type = ({ml_generic_type(2, (ml_type_t *[]){MLClosureT, (ml_type_t *)Result});});
		Closure->Info = Info;
		for (int I = 0; I < Info->NumUpValues; ++I) {
			int Index = Inst[2 + I].Index;
			ml_value_t **Slot = (Index < 0) ? &Frame->UpValues[~Index] : &Frame->Stack[Index];
			ml_value_t *Value = Slot[0];
			if (!Value) Value = Slot[0] = ml_uninitialized("<upvalue>");
			if (ml_typeof(Value) == MLUninitializedT) {
				ml_uninitialized_use(Value, &Closure->UpValues[I]);
			}
			Closure->UpValues[I] = Value;
		}
		Result = (ml_value_t *)Closure;
		ADVANCE(Inst + Info->NumUpValues + 2);
#else
		goto DO_CLOSURE;
#endif
	}
	DO_PARAM_TYPE: {
		Result = ml_deref(Result);
		if (!ml_is(Result, MLTypeT)) {
			Result = ml_error("TypeError", "expected type, not %s", ml_typeof(Result)->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->Line});
			ERROR();
		}
		ml_closure_t *Closure = (ml_closure_t *)Top[-1];
		ml_param_type_t *Type = new(ml_param_type_t);
		Type->Next = Closure->ParamTypes;
		Type->Index = Inst[1].Index;
		Type->Type = (ml_type_t *)Result;
		Closure->ParamTypes = Type;
		ADVANCE(Inst + 2);
	}
	DO_PARTIAL_NEW: {
		Result = ml_deref(Result);
		*Top++ = ml_partial_function_new(Result, Inst[1].Count);
		ADVANCE(Inst + 2);
	}
	DO_PARTIAL_SET: {
		Result = ml_deref(Result);
		ml_partial_function_set(Top[-1], Inst[1].Index, Result);
		ADVANCE(Inst + 2);
	}
	DO_STRING_NEW: {
		*Top++ = ml_stringbuffer();
		ADVANCE(Inst + 1);
	}
	DO_STRING_ADD: {
		int Count = Inst[1].Count;
		ml_value_t **Args = Top - (Count + 1);
		ml_inst_t *Next = Inst + 2;
#ifdef ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		Frame->Line = Inst->Line;
		Frame->Inst = Next;
		Frame->Top = Top - Count;
		return ml_call(Frame, AppendMethod, Count + 1, Args);
	}
	DO_STRING_ADDS: {
		ml_stringbuffer_add((ml_stringbuffer_t *)Top[-1], Inst[2].Chars, Inst[1].Count);
		ADVANCE(Inst + 3);
	}
	DO_STRING_END: {
		Result = *--Top;
		*Top = NULL;
		Result = ml_stringbuffer_value((ml_stringbuffer_t *)Result);
		ADVANCE(Inst + 1);
	}
	DO_RESOLVE: {
		ml_value_t **Args = ml_alloc_args(2);
		Args[0] = Result;
		Args[1] = Inst[1].Value;
		ml_inst_t *Next = Inst + 2;
#ifdef ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		Frame->Line = Inst->Line;
		Frame->Inst = Next;
		Frame->Top = Top;
		return ml_call(Frame, SymbolMethod, 2, Args);
	}
	DO_SWITCH: {
		if (!ml_is(Result, MLIntegerT)) {
			Result = ml_error("TypeError", "expected integer, not %s", ml_typeof(Result)->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->Line});
			ERROR();
		}
		int Count = Inst[1].Count;
		int Index = ml_integer_value_fast(Result);
		if (Index < 0 || Index >= Count) Index = Count - 1;
		ADVANCE(Inst[2].Insts[Index]);
	}
	DO_IF_DEBUG: {
#ifdef DEBUG_VERSION
		ADVANCE(Inst[1].Inst);
#else
		ADVANCE(Inst + 2);
#endif
	}
#ifdef DEBUG_VERSION
	DO_DEBUG_ERROR: {
		ml_debugger_t *Debugger = Frame->Debugger;
		if (Debugger->BreakOnError) {
			Frame->Inst = Inst;
			Frame->Top = Top;
			Frame->Reentry = 1;
			return Debugger->run(Debugger, (ml_state_t *)Frame, Result);
		}
	}
	DO_DEBUG_ADVANCE: {
		ml_debugger_t *Debugger = Frame->Debugger;
		unsigned int Revision = Debugger->Revision;
		if (Frame->Revision != Revision) {
			Frame->Revision = Revision;
			Frame->Breakpoints = Debugger->breakpoints(Debugger, Frame->Source, 0);
		}
		if (Inst->Line != Line) {
			if (Debugger->StepIn) goto DO_BREAKPOINT;
			if (Frame->StepOver) goto DO_BREAKPOINT;
			if (Inst->Opcode == MLI_RETURN && Frame->StepOut) goto DO_BREAKPOINT;
			int Line = Inst->Line;
			if (Frame->Breakpoints[Line / SIZE_BITS] & (1L << Line % SIZE_BITS)) goto DO_BREAKPOINT;
		}
		CHECK_COUNTER
		Line = Inst->Line;
		goto *Labels[Inst->Opcode];
	}
	DO_BREAKPOINT: {
		ml_debugger_t *Debugger = Frame->Debugger;
		Frame->Line = Inst->Line;
		Frame->Inst = Inst;
		Frame->Top = Top;
		Frame->Reentry = 1;
		return Debugger->run(Debugger, (ml_state_t *)Frame, Result);
	}
#endif
#ifdef ML_SCHEDULER
	DO_SWAP: {
		Frame->Line = Inst->Line;
		Frame->Inst = Inst;
		Frame->Top = Top;
		return Frame->Schedule.swap((ml_state_t *)Frame, Result);
	}
#endif
}

static void ml_closure_call_debug(ml_state_t *Caller, ml_closure_t *Closure, int Count, ml_value_t **Args);

static void DEBUG_FUNC(closure_call)(ml_state_t *Caller, ml_closure_t *Closure, int Count, ml_value_t **Args) {
	ml_closure_info_t *Info = Closure->Info;
	ml_debugger_t *Debugger = (ml_debugger_t *)Caller->Context->Values[ML_DEBUGGER_INDEX];
#ifndef DEBUG_VERSION
	if (Debugger) return ml_closure_call_debug(Caller, Closure, Count, Args);
#endif
	size_t Size = sizeof(DEBUG_STRUCT(frame)) + Info->FrameSize * sizeof(ml_value_t *);
	DEBUG_STRUCT(frame) *Frame;
	if (Size <= ML_FRAME_REUSE_SIZE) {
		ML_CACHED_FRAME_LOCK();
		if ((Frame = (DEBUG_STRUCT(frame) *)MLCachedFrame)) {
			MLCachedFrame = Frame->Next;
			ML_CACHED_FRAME_UNLOCK();
		} else {
			ML_CACHED_FRAME_UNLOCK();
			Frame = GC_MALLOC(ML_FRAME_REUSE_SIZE);
		}
		Frame->Continue = 0;
	} else {
		Frame = GC_MALLOC(Size);
	}
	Frame->Base.Type = DEBUG_TYPE(Continuation);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)DEBUG_FUNC(frame_run);
	Frame->Base.Context = Caller->Context;
	Frame->Source = Info->Source;
	int NumParams = Info->NumParams;
	int Flags = Info->Flags;
	if (Flags & ML_CLOSURE_EXTRA_ARGS) --NumParams;
	if (Flags & ML_CLOSURE_NAMED_ARGS) --NumParams;
	int Min = (Count < NumParams) ? Count : NumParams;
	int I = 0;
	ml_decl_t *Decl = Info->Decls;
	for (; I < Min; ++I) {
		ml_value_t *Arg = Args[I];
		if (!(Decl->Flags & MLC_DECL_BYREF)) Arg = ml_deref(Arg);
		if (ml_is_error(Arg)) ML_RETURN(Arg);
#ifdef ML_NANBOXING
		if (!ml_tag(Arg) && Arg->Type == MLNamesT) break;
#else
		if (Arg->Type == MLNamesT) break;
#endif
		if (Decl->Flags & MLC_DECL_ASVAR) Arg = ml_variable(Arg, NULL);
		Decl = Decl->Next;
		Frame->Stack[I] = Arg;
	}
	for (int J = I; J < NumParams; ++J) {
		Frame->Stack[J] = MLNil;
	}
	if (Flags & ML_CLOSURE_EXTRA_ARGS) {
		ml_value_t *Rest = ml_list();
		for (; I < Count; ++I) {
			ml_value_t *Arg = ml_deref(Args[I]);
			if (ml_is_error(Arg)) ML_RETURN(Arg);
#ifdef ML_NANBOXING
			if (!ml_tag(Arg) && Arg->Type == MLNamesT) break;
#else
			if (Arg->Type == MLNamesT) break;
#endif
			ml_list_put(Rest, Arg);
		}
		Frame->Stack[NumParams] = Rest;
		++NumParams;
	}
	if (Flags & ML_CLOSURE_NAMED_ARGS) {
		ml_value_t *Options = ml_map();
		for (; I < Count; ++I) {
			ml_value_t *Arg = Args[I];
			if (ml_is_error(Arg)) ML_RETURN(Arg);
#ifdef ML_NANBOXING
			if (!ml_tag(Arg) && Arg->Type == MLNamesT) {
#else
			if (Arg->Type == MLNamesT) {
#endif
				ML_NAMES_FOREACH(Arg, Node) {
					const char *Name = ml_string_value(Node->Value);
					int Index = (intptr_t)stringmap_search(Info->Params, Name);
					if (Index) {
						Frame->Stack[Index - 1] = ml_deref(Args[++I]);
					} else {
						ml_map_insert(Options, Node->Value, ml_deref(Args[++I]));
					}
				}
				break;
			}
		}
		Frame->Stack[NumParams] = Options;
		++NumParams;
	} else {
		for (; I < Count; ++I) {
			ml_value_t *Arg = Args[I];
			if (ml_is_error(Arg)) ML_RETURN(Arg);
#ifdef ML_NANBOXING
			if (!ml_tag(Arg) && Arg->Type == MLNamesT) {
#else
			if (Arg->Type == MLNamesT) {
#endif
				ML_NAMES_FOREACH(Arg, Node) {
					const char *Name = ml_string_value(Node->Value);
					int Index = (intptr_t)stringmap_search(Info->Params, Name);
					if (Index) {
						Frame->Stack[Index - 1] = ml_deref(Args[++I]);
					} else {
						ML_ERROR("NameError", "Unknown named parameters %s", Name);
					}
				}
				break;
			}
		}
	}
	for (ml_param_type_t *Type = Closure->ParamTypes; Type; Type = Type->Next) {
		ml_value_t *Value = Frame->Stack[Type->Index];
		if (!ml_is(Value, Type->Type)) {
			ML_ERROR("TypeError", "Expected %s not %s for argument %d", Type->Type->Name, ml_typeof(Value)->Name, Type->Index);
		}
	}
	Frame->Top = Frame->Stack + NumParams;
	Frame->OnError = Info->Return;
	Frame->UpValues = Closure->UpValues;
	Frame->Inst = Info->Entry;
	Frame->Line = Info->Entry->Line - 1;
#ifdef ML_SCHEDULER
	ml_scheduler_t scheduler = (ml_scheduler_t)Caller->Context->Values[ML_SCHEDULER_INDEX];
	Frame->Schedule = scheduler(Caller->Context);
#endif
#ifdef DEBUG_VERSION
	Frame->Debugger = Debugger;
	Frame->Revision = Debugger->Revision;
	size_t *Breakpoints = Frame->Breakpoints = Debugger->breakpoints(Debugger, Frame->Source, Info->EndLine);
	Frame->Decls = Info->Decls;
	int Line = Frame->Inst->Line;
	if (Breakpoints[Line / SIZE_BITS] & (1L << Line % SIZE_BITS)) {
		return Debugger->run(Debugger, (ml_state_t *)Frame, MLNil);
	}
#else
#ifdef ML_JIT
	if (Info->JITStart) {
		Frame->Base.run = Info->JITStart;
		Frame->Inst = Info->JITEntry;
		Frame->Line = Info->JITEntry->Line;
		Frame->OnError = Info->JITReturn;
	}
#endif
#endif
	ML_CONTINUE(Frame, MLNil);
}

#ifndef DEBUG_VERSION
const char *MLInstNames[] = {
	"link", // MLI_LINK,
	"return", // MLI_RETURN,
	"suspend", // MLI_SUSPEND,
	"resume", // MLI_RESUME,
	"nil", // MLI_NIL,
	"push_nil", // MLI_NIL_PUSH,
	"some", // MLI_SOME,
	"and", // MLI_AND,
	"or", // MLI_OR,
	"not", // MLI_NOT,
	"push", // MLI_PUSH,
	"with", // MLI_WITH,
	"with_var", // MLI_WITH_VAR,
	"withx", // MLI_WITHX,
	"pop", // MLI_POP,
	"enter", // MLI_ENTER,
	"exit", // MLI_EXIT,
	"goto", // MLI_GOTO,
	"try", // MLI_TRY,
	"catch_type", // MLI_CATCH_TYPE,
	"catch", // MLI_CATCH,
	"retry", // MLI_RETRY,
	"load", // MLI_LOAD,
	"push", // MLI_LOAD_PUSH,
	"var", // MLI_VAR,
	"var_type", // MLI_VAR_TYPE,
	"varx", // MLI_VARX,
	"let", // MLI_LET,
	"leti", // MLI_LETI,
	"letx", // MLI_LETX,
	"ref", // MLI_REF,
	"refi", // MLI_REFI,
	"refx", // MLI_REFX,
	"for", // MLI_FOR,
	"iter", // MLI_ITER,
	"next", // MLI_NEXT,
	"value", // MLI_VALUE,
	"key", // MLI_KEY,
	"call", // MLI_CALL,
	"const_call", // MLI_CONST_CALL,
	"assign", // MLI_ASSIGN,
	"local", // MLI_LOCAL,
	"push_local", // MLI_LOCAL_PUSH,
	"upvalue", // MLI_UPVALUE,
	"localx", // MLI_LOCALX,
	"tuple_new", // MLI_TUPLE_NEW,
	"unpack", // MLI_UNPACK,
	"list_new", // MLI_LIST_NEW,
	"list_append", // MLI_LIST_APPEND,
	"map_new", // MLI_MAP_NEW,
	"map_insert", // MLI_MAP_INSERT,
	"closure", // MLI_CLOSURE,
	"typed_closure", // MLI_CLOSURE_TYPED,
	"param_type", // MLI_PARAM_TYPE,
	"partial_new", // MLI_PARTIAL_NEW,
	"partial_set", // MLI_PARTIAL_SET,
	"string_new", // MLI_STRING_NEW,
	"string_add", // MLI_STRING_ADD,
	"string_adds", // MLI_STRING_ADDS,
	"string_end", // MLI_STRING_END
	"resolve", // MLI_RESOLVE,
	"debug", // MLI_IF_DEBUG,
	"assign_local", // MLI_ASSIGN_LOCAL,
	"switch" // MLI_SWITCH
};

const ml_inst_type_t MLInstTypes[] = {
	MLIT_INST, // MLI_LINK,
	MLIT_NONE, // MLI_RETURN,
	MLIT_NONE, // MLI_SUSPEND,
	MLIT_NONE, // MLI_RESUME,
	MLIT_NONE, // MLI_NIL,
	MLIT_NONE, // MLI_NIL_PUSH,
	MLIT_NONE, // MLI_SOME,
	MLIT_INST, // MLI_AND,
	MLIT_INST, // MLI_OR,
	MLIT_NONE, // MLI_NOT,
	MLIT_NONE, // MLI_PUSH,
	MLIT_DECL, // MLI_WITH,
	MLIT_DECL, // MLI_WITH_VAR,
	MLIT_COUNT_DECL, // MLI_WITHX,
	MLIT_NONE, // MLI_POP,
	MLIT_COUNT_COUNT_DECL, // MLI_ENTER,
	MLIT_COUNT_DECL, // MLI_EXIT,
	MLIT_INST, // MLI_GOTO,
	MLIT_INST, // MLI_TRY,
	MLIT_INST_TYPES, // MLI_CATCH_TYPE,
	MLIT_INDEX_DECL, // MLI_CATCH,
	MLIT_NONE, // MLI_RETRY,
	MLIT_VALUE, // MLI_LOAD,
	MLIT_VALUE, // MLI_LOAD_PUSH,
	MLIT_INDEX, // MLI_VAR,
	MLIT_INDEX, // MLI_VAR_TYPE,
	MLIT_INDEX_COUNT, // MLI_VARX,
	MLIT_INDEX, // MLI_LET,
	MLIT_INDEX, // MLI_LETI,
	MLIT_INDEX_COUNT, // MLI_LETX,
	MLIT_INDEX, // MLI_REF,
	MLIT_INDEX, // MLI_REFI,
	MLIT_INDEX_COUNT, // MLI_REFX,
	MLIT_NONE, // MLI_FOR,
	MLIT_INST, // MLI_ITER,
	MLIT_INST, // MLI_NEXT,
	MLIT_INDEX, // MLI_VALUE,
	MLIT_INDEX, // MLI_KEY,
	MLIT_COUNT, // MLI_CALL,
	MLIT_COUNT_VALUE, // MLI_CONST_CALL,
	MLIT_NONE, // MLI_ASSIGN,
	MLIT_INDEX, // MLI_LOCAL,
	MLIT_INDEX, // MLI_PUSH_LOCAL,
	MLIT_INDEX, // MLI_UPVALUE,
	MLIT_INDEX_CHARS, // MLI_LOCALX,
	MLIT_COUNT, // MLI_TUPLE_NEW,
	MLIT_INDEX, // MLI_UNPACK,
	MLIT_NONE, // MLI_LIST_NEW,
	MLIT_NONE, // MLI_LIST_APPEND,
	MLIT_NONE, // MLI_MAP_NEW,
	MLIT_NONE, // MLI_MAP_INSERT,
	MLIT_CLOSURE, // MLI_CLOSURE,
	MLIT_CLOSURE, // MLI_CLOSURE_TYPED,
	MLIT_INDEX, // MLI_PARAM_TYPE,
	MLIT_COUNT, // MLI_PARTIAL_NEW,
	MLIT_INDEX, // MLI_PARTIAL_SET,
	MLIT_NONE, // MLI_STRING_NEW,
	MLIT_COUNT, // MLI_STRING_ADD,
	MLIT_COUNT_CHARS, // MLI_STRING_ADDS,
	MLIT_NONE, // MLI_STRING_END
	MLIT_VALUE, // MLI_RESOLVE,
	MLIT_INST, // MLI_IF_DEBUG,
	MLIT_INDEX, // MLI_ASSIGN_LOCAL
	MLIT_SWITCH // MLI_SWITCH
};

static int ml_closure_find_labels(ml_inst_t *Inst, unsigned int *Labels) {
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_NONE: return 1;
	case MLIT_INST:
		if (!Inst[1].Inst->Label) Inst[1].Inst->Label = ++*Labels;
		return 2;
	case MLIT_INST_TYPES:
		if (!Inst[1].Inst->Label) Inst[1].Inst->Label = ++*Labels;
		return 3;
	case MLIT_COUNT_COUNT: return 3;
	case MLIT_COUNT: return 2;
	case MLIT_INDEX: return 2;
	case MLIT_VALUE: return 2;
	case MLIT_VALUE_VALUE: return 3;
	case MLIT_INDEX_COUNT: return 3;
	case MLIT_INDEX_CHARS: return 3;
	case MLIT_COUNT_VALUE: return 3;
	case MLIT_COUNT_CHARS: return 3;
	case MLIT_DECL: return 2;
	case MLIT_INDEX_DECL: return 3;
	case MLIT_COUNT_DECL: return 3;
	case MLIT_COUNT_COUNT_DECL: return 4;
	case MLIT_CLOSURE: return 2 + Inst[1].ClosureInfo->NumUpValues;
	case MLIT_SWITCH: {
		int Count = Inst[1].Count;
		ml_inst_t **Insts = Inst[2].Insts;
		for (int I = 0; I < Count; ++I) {
			ml_inst_t *Next = Insts[I];
			if (!Next->Label) Next->Label = ++*Labels;
		}
		return 3;
	}
	default: return 0;
	}
}

void ml_closure_info_labels(ml_closure_info_t *Info) {
	if (Info->Flags & ML_CLOSURE_LABELLED) return;
	Info->Flags |= ML_CLOSURE_LABELLED;
	unsigned int Labels = 0;
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Opcode == MLI_LINK) {
			Inst = Inst[1].Inst;
		} else {
			Inst += ml_closure_find_labels(Inst, &Labels);
		}
	}
}

static int ml_inst_hash(ml_inst_t *Inst, ml_closure_info_t *Info, int I, int J) {
	Info->Hash[I] ^= Inst->Opcode;
	Info->Hash[J] ^= (Inst->Opcode << 4);
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_NONE: return 1;
	case MLIT_INST:
		*(int *)(Info->Hash + I) ^= Inst[1].Inst->Label;
		return 2;
	case MLIT_INST_TYPES:
		for (const char **Ptr = Inst[2].Ptrs; *Ptr; ++Ptr) {
			*(long *)(Info->Hash + J) ^= stringmap_hash(*Ptr);
		}
		return 3;
	case MLIT_COUNT_COUNT:
		*(int *)(Info->Hash + I) ^= Inst[1].Count;
		*(int *)(Info->Hash + J) ^= Inst[2].Count;
		return 3;
	case MLIT_COUNT:
		*(int *)(Info->Hash + I) ^= Inst[1].Count;
		return 2;
	case MLIT_INDEX:
		*(int *)(Info->Hash + I) ^= Inst[1].Index;
		return 2;
	case MLIT_VALUE:
		*(long *)(Info->Hash + J) ^= ml_hash(Inst[1].Value);
		return 2;
	case MLIT_VALUE_VALUE:
		*(long *)(Info->Hash + J) ^= ml_hash(Inst[1].Value);
		*(long *)(Info->Hash + J) ^= ml_hash(Inst[2].Value);
		return 3;
	case MLIT_INDEX_COUNT:
		*(int *)(Info->Hash + I) ^= Inst[1].Index;
		*(int *)(Info->Hash + J) ^= Inst[2].Count;
		return 3;
	case MLIT_INDEX_CHARS:
		*(int *)(Info->Hash + I) ^= Inst[1].Index;
		*(long *)(Info->Hash + J) ^= stringmap_hash(Inst[2].Chars);
		return 3;
	case MLIT_COUNT_VALUE:
		*(int *)(Info->Hash + I) ^= Inst[1].Count;
		*(long *)(Info->Hash + J) ^= ml_hash(Inst[2].Value);
		return 3;
	case MLIT_COUNT_CHARS:
		*(int *)(Info->Hash + I) ^= Inst[1].Count;
		*(long *)(Info->Hash + J) ^= stringmap_hash(Inst[2].Chars);
		return 3;
	case MLIT_DECL: return 2;
	case MLIT_INDEX_DECL:
		*(int *)(Info->Hash + I) ^= Inst[1].Index;
		return 3;
	case MLIT_COUNT_DECL:
		*(int *)(Info->Hash + I) ^= Inst[1].Count;
		return 3;
	case MLIT_COUNT_COUNT_DECL:
		*(int *)(Info->Hash + I) ^= Inst[1].Count;
		*(int *)(Info->Hash + J) ^= Inst[2].Count;
		return 4;
	case MLIT_CLOSURE: {
		ml_closure_info_t *Info = Inst[1].ClosureInfo;
		*(long *)(Info->Hash + J) ^= *(long *)(Info->Hash + J);
		for (int N = 0; N < Info->NumUpValues; ++N) {
			int Index = Inst[2 + N].Index;
			*(int *)(Info->Hash + I) ^= (Index << N);
		}
		return 2 + Inst[1].ClosureInfo->NumUpValues;
	}
	case MLIT_SWITCH:
		return 3;
	default:
		return 0;
	}
}

static void ml_closure_info_hash(ml_closure_info_t *Info) {
	if (Info->Flags & ML_CLOSURE_HASHED) return;
	Info->Flags |= ML_CLOSURE_HASHED;
	int I = 0, J = 0;
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Opcode == MLI_LINK) {
			Inst = Inst[1].Inst;
		} else {
			Inst += ml_inst_hash(Inst, Info, I, J);
		}
		I = (I + 3) % (SHA256_BLOCK_SIZE - 4);
		J = (J + 7) % (SHA256_BLOCK_SIZE - 8);
	}
}

void ml_closure_sha256(ml_value_t *Value, unsigned char Hash[SHA256_BLOCK_SIZE]) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	ml_closure_info_t *Info = Closure->Info;
	ml_closure_info_hash(Info);
	memcpy(Hash, Info->Hash, SHA256_BLOCK_SIZE);
	for (int I = 0; I < Info->NumUpValues; ++I) {
		long ValueHash = ml_hash(Closure->UpValues[I]);
		*(long *)(Hash + (I % 16)) ^= ValueHash;
	}
}

static long ml_closure_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	ml_closure_info_t *Info = Closure->Info;
	ml_closure_info_hash(Info);
	long Hash = 0;
	long *P = (long *)Info->Hash;
	long *Q = (long *)(Info->Hash + SHA256_BLOCK_SIZE);
	while (P < Q) Hash ^= *P++;
	for (int I = 0; I < Info->NumUpValues; ++I) {
		Hash ^= ml_hash_chain(Closure->UpValues[I], Chain) << I;
	}
	return Hash;
}

static void ML_TYPED_FN(ml_iterate, DEBUG_TYPE(Closure), ml_state_t *Frame, ml_closure_t *Closure) {
	return ml_closure_call(Frame, Closure, 0, NULL);
}

ML_TYPE(MLClosureT, (MLFunctionT, MLSequenceT), "closure",
	.hash = ml_closure_hash,
	.call = (void *)ml_closure_call
);

ml_value_t *ml_closure(ml_closure_info_t *Info) {
	ml_closure_t *Closure = xnew(ml_closure_t, Info->NumUpValues, ml_value_t *);
	Closure->Type = MLClosureT;
	Closure->Info = Info;
	return (ml_value_t *)Closure;
}

ML_METHOD(MLStringT, MLClosureT) {
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	return ml_cstring(Closure->Info->Name);
}

static int ml_closure_parameter_fn(const char *Name, void *Value, ml_value_t *Parameters) {
	ml_list_set(Parameters, (intptr_t)Value, ml_string(Name, -1));
	return 0;
}

ML_METHOD("parameters", MLClosureT) {
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	ml_value_t *Parameters = ml_list();
	ml_list_grow(Parameters, Closure->Info->Params->Size);
	stringmap_foreach(Closure->Info->Params, Parameters, (void *)ml_closure_parameter_fn);
	return Parameters;
}

static void ml_closure_value_list(ml_value_t *Value, ml_stringbuffer_t *Buffer) {
	if (ml_is(Value, MLStringT)) {
		ml_stringbuffer_add(Buffer, " \"", 2);
		int Length = ml_string_length(Value);
		const char *String = ml_string_value(Value);
		for (int I = 0; I < Length; ++I) switch (String[I]) {
			case 0: ml_stringbuffer_add(Buffer, "\\0", 2); break;
			case '\t': ml_stringbuffer_add(Buffer, "\\t", 2); break;
			case '\r': ml_stringbuffer_add(Buffer, "\\r", 2); break;
			case '\n': ml_stringbuffer_add(Buffer, "\\n", 2); break;
			case '\'': ml_stringbuffer_add(Buffer, "\\\'", 2); break;
			case '\"': ml_stringbuffer_add(Buffer, "\\\"", 2); break;
			case '\\': ml_stringbuffer_add(Buffer, "\\\\", 2); break;
			default: ml_stringbuffer_add(Buffer, String + I, 1); break;
		}
		ml_stringbuffer_add(Buffer, "\"", 1);
	} else if (ml_is(Value, MLNumberT)) {
		ml_stringbuffer_add(Buffer, " ", 1);
		ml_stringbuffer_append(Buffer, Value);
	} else if (ml_typeof(Value) == MLMethodT) {
		ml_stringbuffer_addf(Buffer, " :%s", ml_method_name(Value));
	} else if (ml_typeof(Value) == MLTypeT) {
		ml_stringbuffer_addf(Buffer, " <%s>", ml_type_name(Value));
	} else {
		ml_stringbuffer_addf(Buffer, " %s", ml_typeof(Value)->Name);
	}
	long Hash = ml_hash(Value);
	ml_stringbuffer_addf(Buffer, "[%ld]", Hash);
}

static int ml_closure_inst_list(ml_inst_t *Inst, ml_stringbuffer_t *Buffer) {
	if (Inst->Label) ml_stringbuffer_addf(Buffer, "L%d:", Inst->Label);
	ml_stringbuffer_addf(Buffer, "\t%s%3d %s", Inst->PotentialBreakpoint ? "*" : " ", Inst->Line, MLInstNames[Inst->Opcode]);
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_NONE: return 1;
	case MLIT_INST:
		ml_stringbuffer_addf(Buffer, " ->L%d", Inst[1].Inst->Label);
		return 2;
	case MLIT_INST_TYPES: {
		ml_stringbuffer_addf(Buffer, " ->L%d", Inst[1].Inst->Label);
		for (const char **Ptr = Inst[2].Ptrs; *Ptr; ++Ptr) ml_stringbuffer_addf(Buffer, " %s", *Ptr);
		return 3;
	}
	case MLIT_COUNT_COUNT:
		ml_stringbuffer_addf(Buffer, " %d, %d", Inst[1].Count, Inst[2].Count);
		return 3;
	case MLIT_COUNT:
		ml_stringbuffer_addf(Buffer, " %d", Inst[1].Count);
		return 2;
	case MLIT_INDEX:
		ml_stringbuffer_addf(Buffer, " %d", Inst[1].Index);
		return 2;
	case MLIT_VALUE: {
		ml_closure_value_list(Inst[1].Value, Buffer);
		return 2;
	}
	case MLIT_VALUE_VALUE: {
		ml_closure_value_list(Inst[1].Value, Buffer);
		ml_stringbuffer_add(Buffer, ",", 2);
		ml_closure_value_list(Inst[2].Value, Buffer);
		return 3;
	}
	case MLIT_INDEX_COUNT:
		ml_stringbuffer_addf(Buffer, " %d, %d", Inst[1].Index, Inst[2].Count);
		return 3;
	case MLIT_INDEX_CHARS:
		ml_stringbuffer_addf(Buffer, " %d, %s", Inst[1].Index, Inst[2].Chars);
		return 3;
	case MLIT_COUNT_VALUE: {
		ml_stringbuffer_addf(Buffer, " %d,", Inst[1].Count);
		ml_closure_value_list(Inst[2].Value, Buffer);
		return 3;
	}
	case MLIT_COUNT_CHARS:
		ml_stringbuffer_addf(Buffer, " %d, \"", Inst[1].Count);
		for (const char *P = Inst[2].Chars; *P; ++P) switch(*P) {
			case 0: ml_stringbuffer_add(Buffer, "\\0", 2); break;
			case '\e': ml_stringbuffer_add(Buffer, "\\e", 2); break;
			case '\t': ml_stringbuffer_add(Buffer, "\\t", 2); break;
			case '\r': ml_stringbuffer_add(Buffer, "\\r", 2); break;
			case '\n': ml_stringbuffer_add(Buffer, "\\n", 2); break;
			case '\'': ml_stringbuffer_add(Buffer, "\\\'", 2); break;
			case '\"': ml_stringbuffer_add(Buffer, "\\\"", 2); break;
			case '\\': ml_stringbuffer_add(Buffer, "\\\\", 2); break;
			default: ml_stringbuffer_add(Buffer, P, 1); break;
		}
		ml_stringbuffer_add(Buffer, "\"", 1);
		return 3;
	case MLIT_DECL:
		if (Inst[1].Decls) {
			ml_stringbuffer_addf(Buffer, " <%s>", Inst[1].Decls->Ident);
		} else {
			ml_stringbuffer_addf(Buffer, " -");
		}
		return 2;
	case MLIT_INDEX_DECL:
		if (Inst[2].Decls) {
			ml_stringbuffer_addf(Buffer, " %d <%s>", Inst[1].Index, Inst[2].Decls->Ident);
		} else {
			ml_stringbuffer_addf(Buffer, " %d -", Inst[1].Index);
		}
		return 3;
	case MLIT_COUNT_DECL:
		if (Inst[2].Decls) {
			ml_stringbuffer_addf(Buffer, " %d <%s>", Inst[1].Count, Inst[2].Decls->Ident);
		} else {
			ml_stringbuffer_addf(Buffer, " %d -", Inst[1].Count);
		}
		return 3;
	case MLIT_COUNT_COUNT_DECL:
		if (Inst[3].Decls) {
			ml_stringbuffer_addf(Buffer, " %d, %d <%s>", Inst[1].Count, Inst[2].Count, Inst[3].Decls->Ident);
		} else {
			ml_stringbuffer_addf(Buffer, " %d, %d -", Inst[1].Count, Inst[2].Count);
		}
		return 4;
	case MLIT_CLOSURE: {
		ml_closure_info_t *Info = Inst[1].ClosureInfo;
		ml_stringbuffer_addf(Buffer, " %s:%d", Info->Source, Info->StartLine);
		for (int N = 0; N < Info->NumUpValues; ++N) {
			ml_stringbuffer_addf(Buffer, ", %d", Inst[2 + N].Index);
		}
		return 2 + Info->NumUpValues;
	}
	case MLIT_SWITCH: {
		int Count = Inst[1].Count;
		if (Count) {
			ml_inst_t **Insts = Inst[2].Insts;
			ml_stringbuffer_addf(Buffer, " L%d", Insts[0]->Label);
			for (int I = 1; I < Count; ++I) {
				ml_stringbuffer_addf(Buffer, ", L%d", Insts[I]->Label);
			}
		}
		return 3;
	}
	default: return 0;
	}
}

ML_METHOD("info", MLClosureT) {
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	ml_closure_info_t *Info = Closure->Info;
	ml_value_t *Result = ml_map();
	ml_map_insert(Result, ml_cstring("Source"), ml_cstring(Info->Source));
	ml_map_insert(Result, ml_cstring("Start"), ml_integer(Info->StartLine));
	ml_map_insert(Result, ml_cstring("End"), ml_integer(Info->EndLine));
	ml_map_insert(Result, ml_cstring("Size"), ml_integer(Info->FrameSize));
	ml_map_insert(Result, ml_cstring("Flags"), ml_integer(Info->Flags));
	return Result;
}

ML_METHOD("list", MLClosureT) {
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	ml_closure_info_t *Info = Closure->Info;
	ml_closure_info_labels(Info);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Opcode == MLI_LINK) {
			Inst = Inst[1].Inst;
		} else {
			Inst += ml_closure_inst_list(Inst, Buffer);
		}
		ml_stringbuffer_add(Buffer, "\n", 1);
	}
	return ml_stringbuffer_value(Buffer);
}

void ml_closure_list(ml_value_t *Value) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	ml_closure_info_t *Info = Closure->Info;
	ml_closure_info_labels(Info);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_addf(Buffer, "<%s:%d>\n", Info->Source, Info->StartLine);
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Opcode == MLI_LINK) {
			Inst = Inst[1].Inst;
		} else {
			Inst += ml_closure_inst_list(Inst, Buffer);
		}
		ml_stringbuffer_add(Buffer, "\n", 1);
	}
	ml_stringbuffer_add(Buffer, "\n", 1);
	for (int I = 0; I < Info->NumUpValues; ++I) {
		ml_value_t *UpValue = Closure->UpValues[I];
		ml_stringbuffer_addf(Buffer, "Upvalues %d:", I);
		ml_closure_value_list(UpValue, Buffer);
		ml_stringbuffer_add(Buffer, "\n", 1);
	}
	puts(ml_stringbuffer_get(Buffer));
}

#ifdef ML_JIT

ML_METHOD("jit", MLClosureT) {
//!internal
	ml_closure_info_t *Info = ((ml_closure_t *)Args[0])->Info;
	if (!Info->JITEntry) ml_bytecode_jit(Info);
	return Args[0];
}

#endif

#define DEBUG_VERSION
#include "ml_bytecode.c"
#undef DEBUG_VERSION

void ml_bytecode_init() {
#ifdef ML_GENERICS
	ml_type_add_rule(MLClosureT, MLFunctionT, ML_TYPE_ARG(1), NULL);
#endif
#include "ml_bytecode_init.c"
}
#endif

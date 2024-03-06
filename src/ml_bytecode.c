#include "ml_macros.h"
#include "stringmap.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <inttypes.h>
#include "ml_bytecode.h"
#include "ml_debugger.h"
#include "ml_method.h"

#ifndef DEBUG_VERSION

#undef ML_CATEGORY
#define ML_CATEGORY "bytecode"

// Overview
// This is a mostly internal module, subject to change.

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

static void ml_variable_assign(ml_state_t *Caller, ml_variable_t *Variable, ml_value_t *Value) {
	if (Variable->VarType && !ml_is(Value, Variable->VarType)) {
		ML_ERROR("TypeError", "Cannot assign %s to variable of type %s", ml_typeof(Value)->Name, Variable->VarType->Name);
	}
	Variable->Value = Value;
	ML_RETURN(Value);
}

ML_TYPE(MLVariableT, (), "variable",
// A variable, which can hold another value (returned when dereferenced) and assigned a new value.
// Variables may optionally be typed, assigning a value that is not an instance of the specified type (or a subtype) will raise an error.
	.hash = (void *)ml_variable_hash,
	.deref = (void *)ml_variable_deref,
	.assign = (void *)ml_variable_assign
);

ml_value_t *ml_variable(ml_value_t *Value, ml_type_t *Type) {
	ml_variable_t *Variable = new(ml_variable_t);
	Variable->Type = MLVariableT;
	Variable->Value = Value;
	Variable->VarType = Type;
	return (ml_value_t *)Variable;
}

ml_value_t *ml_variable_set(ml_value_t *_Variable, ml_value_t *Value) {
	ml_variable_t *Variable = (ml_variable_t *)_Variable;
	if (Variable->VarType && !ml_is(Value, Variable->VarType)) {
		return ml_error("TypeError", "Cannot assign %s to variable of type %s", ml_typeof(Value)->Name, Variable->VarType->Name);
	}
	return Variable->Value = Value;
}

ML_METHOD(MLVariableT) {
//>variable
// Return a new untyped variable with current value :mini:`nil`.
	return ml_variable(MLNil, NULL);
}

ML_METHOD(MLVariableT, MLAnyT) {
//<Value
//>variable
// Return a new untyped variable with current value :mini:`Value`.
	return ml_variable(Args[0], NULL);
}

ML_METHOD(MLVariableT, MLAnyT, MLTypeT) {
//<Value
//<Type
//>variable
// Return a new typed variable with type :mini:`Type` and current value :mini:`Value`.
	return ml_variable(Args[0], (ml_type_t *)Args[1]);
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
		ml_frame_t *Next;
		ml_inst_t *Inst;
	};
	ml_value_t **Top;
	const char *Source;
	ml_inst_t *OnError;
	ml_value_t **UpValues;
#ifdef ML_SCHEDULER
	uint64_t *Counter;
#endif
	unsigned int Line;
	unsigned int Reentry:1;
	unsigned int Suspend:1;
	unsigned int Reuse:1;
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
	Frame->Base.Context = Caller->Context;
#ifdef ML_SCHEDULER
	Frame->Counter = (uint64_t *)Caller->Context->Values[ML_COUNTER_INDEX];
#endif
	Frame->Reuse = 0;
	ml_state_schedule((ml_state_t *)Frame, Count ? Args[0] : MLNil);
	ML_RETURN(MLNil);
}

ML_TYPE(DEBUG_TYPE(Continuation), (MLStateT, MLSequenceT), "continuation",
//@continuation
// A bytecode function frame which can be resumed.
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

#define FRAME_DECLS(DECLS)

#else

#undef ERROR
#undef ADVANCE
#undef FRAME_DECLS

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

#define FRAME_DECLS(DECLS) Frame->Decls = DECLS;

#endif

#ifndef DEBUG_VERSION

#ifdef ML_THREADSAFE
static ml_frame_t * _Atomic FrameCache = NULL;
#else
static ml_frame_t *FrameCache = NULL;
#endif

#define ML_FRAME_REUSE_SIZE 384

static ml_frame_t *ml_frame() {
#ifdef ML_THREADSAFE
	ml_frame_t *Next = FrameCache, *CacheNext;
	do {
		if (!Next) {
			Next = bnew(ML_FRAME_REUSE_SIZE);
			Next->Reuse = 1;
			break;
		}
		CacheNext = Next->Next;
	} while (!atomic_compare_exchange_weak(&FrameCache, &Next, CacheNext));
#else
	ml_frame_t *Next = FrameCache;
	if (!Next) {
		Next = bnew(ML_FRAME_REUSE_SIZE);
		Next->Reuse = 1;
	} else {
		FrameCache = Next->Next;
	}
#endif
	Next->Next = NULL;
	return Next;
}

static void ml_frame_reuse(ml_frame_t *Frame) {
	Frame->Base.Caller = NULL;
#ifdef ML_THREADSAFE
	ml_frame_t *CacheNext = FrameCache;
	do {
		Frame->Next = CacheNext;
	} while (!atomic_compare_exchange_weak(&FrameCache, &CacheNext, Frame));
#else
	Frame->Next = FrameCache;
	FrameCache = Frame;
#endif
}

size_t ml_count_cached_frames() {
	size_t Count = 0;
	for (ml_frame_t *Frame = FrameCache; Frame; Frame = Frame->Next) ++Count;
	return Count;
}

extern ml_value_t *SymbolMethod;

#ifdef ML_SCHEDULER
#define ML_STORE_COUNTER() Frame->Counter[0] = Counter
#else
#define ML_STORE_COUNTER() {}

#endif

static ml_inst_t ReturnInst[1] = {{.Opcode = MLI_RETURN, .Line = 0}};

#define TAIL_CALL(COUNT) \
	ml_state_t *Caller = Frame->Base.Caller; \
	ml_value_t **Args2 = ml_alloc_args(COUNT); \
	memcpy(Args2, Top - COUNT, COUNT * sizeof(ml_value_t *)); \
	if (Frame->Reuse) { \
		while (Top > Frame->Stack) *--Top = NULL; \
		ml_frame_reuse((ml_frame_t *)Frame); \
	} else { \
		Frame->Inst = ReturnInst; \
	} \
	return ml_call(Caller, Function, COUNT, Args2)

#define DO_CALL_COUNT(COUNT) \
	DO_CALL_ ## COUNT: { \
		ml_value_t **Args = Top - COUNT; \
		ml_value_t *Function = Args[-1]; \
		ml_inst_t *Next = Inst + 2; \
		ML_STORE_COUNTER(); \
		Frame->Inst = Next; \
		Frame->Line = Inst->Line; \
		Frame->Top = Top - (COUNT + 1); \
		return ml_call(Frame, Function, COUNT, Args); \
	} \
	DO_TAIL_CALL_ ## COUNT: { \
		ml_value_t *Function = Top[~COUNT]; \
		ML_STORE_COUNTER(); \
		TAIL_CALL(COUNT); \
	} \
	DO_CALL_CONST_ ## COUNT: { \
		ml_value_t **Args = Top - COUNT; \
		ml_value_t *Function = Inst[1].Value; \
		ml_inst_t *Next = Inst + 3; \
		ML_STORE_COUNTER(); \
		Frame->Inst = Next; \
		Frame->Line = Inst->Line; \
		Frame->Top = Args; \
		return ml_call(Frame, Function, COUNT, Args); \
	} \
	DO_TAIL_CALL_CONST_ ## COUNT: { \
		ml_value_t *Function = Inst[1].Value; \
		ML_STORE_COUNTER(); \
		TAIL_CALL(COUNT); \
	} \
	DO_CALL_METHOD_ ## COUNT: { \
		ml_value_t **Args = Top - COUNT; \
		ml_method_t *Method = (ml_method_t *)Inst[1].Value; \
		ml_methods_t *Methods = Frame->Base.Context->Values[ML_METHODS_INDEX]; \
		ml_method_cached_t *Cached = ml_method_check_cached(Methods, Method, Inst[3].Data, COUNT, Args); \
		if (!Cached) { \
			ml_value_t **Args2 = ml_alloc_args(COUNT + 1); \
			Args2[0] = (ml_value_t *)Method; \
			memcpy(Args2 + 1, Args, (COUNT) * sizeof(ml_value_t *)); \
			ml_inst_t *Next = Inst + 4; \
			ML_STORE_COUNTER(); \
			Frame->Inst = Next; \
			Frame->Line = Inst->Line; \
			Frame->Top = Args; \
			return ml_call(Frame, MLMethodDefault, COUNT + 1, Args2); \
		} \
		Inst[3].Data = Cached; \
		ml_value_t *Function = Cached->Callback; \
		ml_inst_t *Next = Inst + 4; \
		ML_STORE_COUNTER(); \
		Frame->Inst = Next; \
		Frame->Line = Inst->Line; \
		Frame->Top = Args; \
		return ml_call(Frame, Function, COUNT, Args); \
	} \
	DO_TAIL_CALL_METHOD_ ## COUNT: { \
		ml_value_t **Args = Top - COUNT; \
		ml_method_t *Method = (ml_method_t *)Inst[1].Value; \
		ml_methods_t *Methods = Frame->Base.Context->Values[ML_METHODS_INDEX]; \
		ml_method_cached_t *Cached = ml_method_check_cached(Methods, Method, Inst[3].Data, COUNT, Args); \
		if (!Cached) { \
			ml_value_t **Args2 = ml_alloc_args(COUNT + 1); \
			Args2[0] = (ml_value_t *)Method; \
			memcpy(Args2 + 1, Args, (COUNT) * sizeof(ml_value_t *)); \
			ml_inst_t *Next = Inst + 4; \
			ML_STORE_COUNTER(); \
			Frame->Inst = Next; \
			Frame->Line = Inst->Line; \
			Frame->Top = Args; \
			return ml_call(Frame, MLMethodDefault, COUNT + 1, Args2); \
		} \
		Inst[3].Data = Cached; \
		ml_value_t *Function = Cached->Callback; \
		ML_STORE_COUNTER(); \
		TAIL_CALL(COUNT); \
	}

#define CALL_LABELS(PREFIX) \
	static const void *PREFIX ##_Labels[] = { \
		&&DO_ ## PREFIX ## _0, \
		&&DO_ ## PREFIX ## _1, \
		&&DO_ ## PREFIX ## _2, \
		&&DO_ ## PREFIX ## _3, \
		&&DO_ ## PREFIX ## _4, \
		&&DO_ ## PREFIX ## _5, \
		&&DO_ ## PREFIX ## _6, \
		&&DO_ ## PREFIX ## _7, \
		&&DO_ ## PREFIX ## _8, \
		&&DO_ ## PREFIX ## _9 \
	}

#endif

extern ml_value_t *AppendMethod;

static void DEBUG_FUNC(frame_run)(DEBUG_STRUCT(frame) *Frame, ml_value_t *Result) {
	static const void *Labels[] = {
		[MLI_AND] = &&DO_AND,
		[MLI_ASSIGN] = &&DO_ASSIGN,
		[MLI_ASSIGN_LOCAL] = &&DO_ASSIGN_LOCAL,
		[MLI_CALL] = &&DO_CALL,
		[MLI_TAIL_CALL] = &&DO_TAIL_CALL,
		[MLI_CATCH] = &&DO_CATCH,
		[MLI_CATCHX] = &&DO_CATCHX,
		[MLI_CLOSURE] = &&DO_CLOSURE,
		[MLI_CLOSURE_TYPED] = &&DO_CLOSURE_TYPED,
		[MLI_CALL_CONST] = &&DO_CALL_CONST,
		[MLI_TAIL_CALL_CONST] = &&DO_TAIL_CALL_CONST,
		[MLI_ENTER] = &&DO_ENTER,
		[MLI_EXIT] = &&DO_EXIT,
		[MLI_FOR] = &&DO_FOR,
		[MLI_GOTO] = &&DO_GOTO,
		[MLI_IF_CONFIG] = &&DO_IF_CONFIG,
		[MLI_ITER] = &&DO_ITER,
		[MLI_KEY] = &&DO_KEY,
		[MLI_LET] = &&DO_LET,
		[MLI_LETI] = &&DO_LETI,
		[MLI_LETX] = &&DO_LETX,
		[MLI_LINK] = &&DO_LINK,
		[MLI_LIST_APPEND] = &&DO_LIST_APPEND,
		[MLI_LIST_NEW] = &&DO_LIST_NEW,
		[MLI_LOAD] = &&DO_LOAD,
		[MLI_LOAD_PUSH] = &&DO_LOAD_PUSH,
		[MLI_LOAD_VAR] = &&DO_LOAD_VAR,
		[MLI_LOCAL] = &&DO_LOCAL,
		[MLI_LOCALI] = &&DO_LOCALI,
		[MLI_LOCAL_PUSH] = &&DO_LOCAL_PUSH,
		[MLI_MAP_INSERT] = &&DO_MAP_INSERT,
		[MLI_MAP_NEW] = &&DO_MAP_NEW,
		[MLI_CALL_METHOD] = &&DO_CALL_METHOD,
		[MLI_TAIL_CALL_METHOD] = &&DO_TAIL_CALL_METHOD,
		[MLI_NEXT] = &&DO_NEXT,
		[MLI_NIL] = &&DO_NIL,
		[MLI_AND_POP] = &&DO_AND_POP,
		[MLI_NIL_PUSH] = &&DO_NIL_PUSH,
		[MLI_NOT] = &&DO_NOT,
		[MLI_OR] = &&DO_OR,
		[MLI_PARAM_TYPE] = &&DO_PARAM_TYPE,
		[MLI_PARTIAL_NEW] = &&DO_PARTIAL_NEW,
		[MLI_PARTIAL_SET] = &&DO_PARTIAL_SET,
		[MLI_POP] = &&DO_POP,
		[MLI_PUSH] = &&DO_PUSH,
		[MLI_REF] = &&DO_REF,
		[MLI_REFI] = &&DO_REFI,
		[MLI_REFX] = &&DO_REFX,
		[MLI_RESOLVE] = &&DO_RESOLVE,
		[MLI_RESUME] = &&DO_RESUME,
		[MLI_RETRY] = &&DO_RETRY,
		[MLI_RETURN] = &&DO_RETURN,
		[MLI_STRING_ADD] = &&DO_STRING_ADD,
		[MLI_STRING_ADD_1] = &&DO_STRING_ADD_1,
		[MLI_STRING_ADDS] = &&DO_STRING_ADDS,
		[MLI_STRING_POP] = &&DO_STRING_POP,
		[MLI_STRING_END] = &&DO_STRING_END,
		[MLI_STRING_NEW] = &&DO_STRING_NEW,
		[MLI_SUSPEND] = &&DO_SUSPEND,
		[MLI_SWITCH] = &&DO_SWITCH,
		[MLI_TRY] = &&DO_TRY,
		[MLI_TUPLE_NEW] = &&DO_TUPLE_NEW,
		[MLI_UPVALUE] = &&DO_UPVALUE,
		[MLI_VALUE_1] = &&DO_VALUE_1,
		[MLI_VALUE_2] = &&DO_VALUE_2,
		[MLI_VAR] = &&DO_VAR,
		[MLI_VAR_TYPE] = &&DO_VAR_TYPE,
		[MLI_VARX] = &&DO_VARX,
		[MLI_WITH] = &&DO_WITH,
		[MLI_WITHX] = &&DO_WITHX,
	};
	CALL_LABELS(CALL);
	CALL_LABELS(TAIL_CALL);
	CALL_LABELS(CALL_CONST);
	CALL_LABELS(TAIL_CALL_CONST);
	CALL_LABELS(CALL_METHOD);
	CALL_LABELS(TAIL_CALL_METHOD);
#ifdef ML_SCHEDULER
	uint64_t Counter = Frame->Counter[0];
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
		ML_STORE_COUNTER();
		ml_state_t *Caller = Frame->Base.Caller;
		if (Frame->Reuse) {
			//memset(Frame, 0, ML_FRAME_REUSE_SIZE);
			while (Top > Frame->Stack) *--Top = NULL;
			//memset(Frame->Stack, 0, (Top - Frame->Stack) * sizeof(ml_value_t *));
			ml_frame_reuse((ml_frame_t *)Frame);
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
		ML_STORE_COUNTER();
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
		Result = *Top++ = MLNil;
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
		FRAME_DECLS(Inst[1].Decls);
		ADVANCE(Inst + 2);
	}
	DO_WITHX: {
		ml_value_t *Packed = Result;
		int Count = Inst[1].Count;
		for (int I = 0; I < Count; ++I) {
			Result = ml_unpack(Packed, I + 1);
			ERROR_CHECK(Result);
			*Top++ = Result;
		}
		FRAME_DECLS(Inst[2].Decls);
		ADVANCE(Inst + 3);
	}
	DO_POP: {
		Result = *--Top;
		*Top = NULL;
		ADVANCE(Inst + 1);
	}
	DO_AND_POP: {
		if (ml_deref(Result) == MLNil) {
			for (int I = Inst[2].Count; --I >= 0;) *--Top = NULL;
			Result = MLNil;
			ADVANCE(Inst[1].Inst);
		} else {
			ADVANCE(Inst + 3);
		}
	}
	DO_ENTER: {
		for (int I = Inst[1].Count; --I >= 0;) {
			ml_variable_t *Local = new(ml_variable_t);
			Local->Type = MLVariableT;
			Local->Value = MLNil;
			*Top++ = (ml_value_t *)Local;
		}
		for (int I = Inst[2].Count; --I >= 0;) *Top++ = NULL;
		FRAME_DECLS(Inst[3].Decls);
		ADVANCE(Inst + 4);
	}
	DO_EXIT: {
		for (int I = Inst[1].Count; --I >= 0;) *--Top = NULL;
		FRAME_DECLS(Inst[2].Decls);
		ADVANCE(Inst + 3);
	}
	DO_GOTO: {
		ADVANCE(Inst[1].Inst);
	}
	DO_TRY: {
		Frame->OnError = Inst[1].Inst;
		ADVANCE(Inst + 2);
	}
	DO_CATCH: {
		Frame->OnError = Inst[1].Inst;
		if (!ml_is_error(Result)) {
			Result = ml_error("InternalError", "expected error value, not %s", ml_typeof(Result)->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->Line});
			ERROR();
		}
		Result = ml_error_unwrap(Result);
		ml_value_t **Old = Frame->Stack + Inst[2].Count;
		while (Top > Old) *--Top = NULL;
		*Top++ = Result;
		FRAME_DECLS(Inst[3].Decls);
		ADVANCE(Inst + 4);
	}
	DO_CATCHX: {
		Frame->OnError = Inst[1].Inst;
		if (!ml_is_error(Result)) {
			Result = ml_error("InternalError", "expected error value, not %s", ml_typeof(Result)->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->Line});
			ERROR();
		}
		ml_value_t **Old = Frame->Stack + Inst[2].Count;
		while (Top > Old) *--Top = NULL;
		FRAME_DECLS(Inst[3].Decls);
		ADVANCE(Inst + 4);
	}
	DO_RETRY: {
		ERROR();
	}
	DO_LOAD: {
		Result = Inst[1].Value;
		ADVANCE(Inst + 2);
	}
	DO_LOAD_PUSH: {
		Result = *Top++ = Inst[1].Value;
		ADVANCE(Inst + 2);
	}
	DO_LOAD_VAR: {
		Result = ml_deref(Inst[1].Value);
		ml_variable_t *Variable = (ml_variable_t *)Top[Inst[2].Count];
		if (Variable->VarType && !ml_is(Result, Variable->VarType)) {
			Result = ml_error("TypeError", "Cannot assign %s to variable of type %s", ml_typeof(Result)->Name, Variable->VarType->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->Line});
			ERROR();
		}
		Variable->Value = Result;
		ADVANCE(Inst + 3);
	}
	DO_VAR: {
		Result = ml_deref(Result);
		ml_variable_t *Variable = (ml_variable_t *)Top[Inst[1].Count];
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
		ml_variable_t *Local = (ml_variable_t *)Top[Inst[1].Count];
		Local->VarType = (ml_type_t *)Result;
		ADVANCE(Inst + 2);
	}
	DO_VARX: {
		ml_value_t *Packed = ml_deref(Result);
		int Count = Inst[2].Count;
		ml_value_t **Base = Top + Inst[1].Count;
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
		Top[Inst[1].Count] = Result;
		ADVANCE(Inst + 2);
	}
	DO_LETI: {
		Result = ml_deref(Result);
		ml_value_t *Uninitialized = Top[Inst[1].Count];
		Top[Inst[1].Count] = Result;
		if (Uninitialized) ml_uninitialized_set(Uninitialized, Result);
		ADVANCE(Inst + 2);
	}
	DO_LETX: {
		ml_value_t *Packed = ml_deref(Result);
		int Count = Inst[2].Count;
		ml_value_t **Base = Top + Inst[1].Count;
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
		Top[Inst[1].Count] = Result;
		ADVANCE(Inst + 2);
	}
	DO_REFI: {
		ml_value_t *Uninitialized = Top[Inst[1].Count];
		Top[Inst[1].Count] = Result;
		if (Uninitialized) ml_uninitialized_set(Uninitialized, Result);
		ADVANCE(Inst + 2);
	}
	DO_REFX: {
		ml_value_t *Packed = ml_deref(Result);
		int Count = Inst[2].Count;
		ml_value_t **Base = Top + Inst[1].Count;
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
		ML_STORE_COUNTER();
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
		ML_STORE_COUNTER();
		return ml_iter_next((ml_state_t *)Frame, Result);
	}
	DO_VALUE_1: {
		Result = Top[-1];
		Frame->Line = Inst->Line;
		Frame->Inst = Inst + 1;
		Frame->Top = Top;
		ML_STORE_COUNTER();
		return ml_iter_value((ml_state_t *)Frame, Result);
	}
	DO_VALUE_2: {
		Result = Top[-2];
		Frame->Line = Inst->Line;
		Frame->Inst = Inst + 1;
		Frame->Top = Top;
		ML_STORE_COUNTER();
		return ml_iter_value((ml_state_t *)Frame, Result);
	}
	DO_KEY: {
		Result = Top[-1];
		Frame->Line = Inst->Line;
		Frame->Inst = Inst + 1;
		Frame->Top = Top;
		ML_STORE_COUNTER();
		return ml_iter_key((ml_state_t *)Frame, Result);
	}
	DO_CALL: {
		int Count = Inst[1].Count;
		if (__builtin_expect(Count < 10, 1)) goto *CALL_Labels[Count];
		ml_value_t **Args = Top - Count;
		ml_value_t *Function = Args[-1];
		ml_inst_t *Next = Inst + 2;
		ML_STORE_COUNTER();
		Frame->Inst = Next;
		Frame->Line = Inst->Line;
		Frame->Top = Top - (Count + 1);
		return ml_call(Frame, Function, Count, Args);
	}
	DO_TAIL_CALL: {
		int Count = Inst[1].Count;
		if (__builtin_expect(Count < 10, 1)) goto *TAIL_CALL_Labels[Count];
		ml_value_t *Function = Top[~Count];
		ML_STORE_COUNTER();
		TAIL_CALL(Count);
	}
	DO_CALL_CONST: {
		int Count = Inst[2].Count;
		if (__builtin_expect(Count < 10, 1)) goto *CALL_CONST_Labels[Count];
		ml_value_t **Args = Top - Count;
		ml_value_t *Function = Inst[1].Value;
		ml_inst_t *Next = Inst + 3;
		ML_STORE_COUNTER();
		Frame->Inst = Next;
		Frame->Line = Inst->Line;
		Frame->Top = Args;
		return ml_call(Frame, Function, Count, Args);
	}
	DO_TAIL_CALL_CONST: {
		int Count = Inst[2].Count;
		if (__builtin_expect(Count < 10, 1)) goto *TAIL_CALL_CONST_Labels[Count];
		ml_value_t *Function = Inst[1].Value;
		ML_STORE_COUNTER();
		TAIL_CALL(Count);
	}
	DO_CALL_METHOD: {
		int Count = Inst[2].Count;
		if (__builtin_expect(Count < 10, 1)) goto *CALL_METHOD_Labels[Count];
		ml_value_t **Args = Top - Count;
		ml_value_t *Function = Inst[1].Value;
		ml_inst_t *Next = Inst + 4;
		ML_STORE_COUNTER();
		Frame->Inst = Next;
		Frame->Line = Inst->Line;
		Frame->Top = Args;
		return ml_call(Frame, Function, Count, Args);
	}
	DO_TAIL_CALL_METHOD: {
		int Count = Inst[2].Count;
		if (__builtin_expect(Count < 10, 1)) goto *TAIL_CALL_METHOD_Labels[Count];
		ml_value_t *Function = Inst[1].Value;
		ML_STORE_COUNTER();
		TAIL_CALL(Count);
	}
	DO_ASSIGN: {
		Result = ml_deref(Result);
		ml_value_t *Ref = Top[-1];
		*--Top = NULL;
		Frame->Inst = Inst + 1;
		Frame->Line = Inst->Line;
		Frame->Top = Top;
		return ml_assign((ml_state_t *)Frame, Ref, Result);
	}
	DO_LOCAL: {
		Result = Top[Inst[1].Count];
		ADVANCE(Inst + 2);
	}
	DO_ASSIGN_LOCAL: {
		Result = ml_deref(Result);
		ml_value_t *Ref = Top[Inst[1].Count];
		Frame->Inst = Inst + 2;
		Frame->Line = Inst->Line;
		Frame->Top = Top;
		return ml_assign((ml_state_t *)Frame, Ref, Result);
	}
	DO_LOCAL_PUSH: {
		Result = *Top = Top[Inst[1].Count];
		++Top;
		ADVANCE(Inst + 2);
	}
	DO_UPVALUE: {
		int Index = Inst[1].Count;
		Result = Frame->UpValues[Index];
		ADVANCE(Inst + 2);
	}
	DO_LOCALI: {
		ml_value_t **Slot = &Top[Inst[1].Count];
		Result = Slot[0];
		if (!Result) Result = Slot[0] = ml_uninitialized(Inst[2].Chars, (ml_source_t){Frame->Source, Inst->Line});
		ADVANCE(Inst + 3);
	}
	DO_TUPLE_NEW: {
		int Count = Inst[1].Count;
		Result = ml_tuplen(Count, Top - Count);
		for (int I = Count; --I >= 0;) *--Top = NULL;
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
		// closure <info> <upvalue_1> ...
		ml_closure_info_t *Info = Inst[1].ClosureInfo;
		ml_closure_t *Closure = xnew(ml_closure_t, Info->NumUpValues, ml_value_t *);
		Closure->Type = MLClosureT;
		Closure->Info = Info;
		for (int I = 0; I < Info->NumUpValues; ++I) {
			int Index = Inst[2 + I].Count;
			ml_value_t **Slot = (Index < 0) ? &Frame->UpValues[~Index] : &Frame->Stack[Index];
			ml_value_t *Value = Slot[0];
			if (!Value) Value = Slot[0] = ml_uninitialized("<upvalue>", (ml_source_t){Frame->Source, Inst->Line});
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
		// closure <info> <upvalue_1> ...
		if (!ml_is(Result, MLTypeT)) {
			Result = ml_error("TypeError", "expected type, not %s", ml_typeof(Result)->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->Line});
			ERROR();
		}
		ml_closure_info_t *Info = Inst[1].ClosureInfo;
		ml_closure_t *Closure = xnew(ml_closure_t, Info->NumUpValues, ml_value_t *);
		Closure->Type = ({ml_generic_type(2, (ml_type_t *[]){MLClosureT, (ml_type_t *)Result});});
		Closure->Info = Info;
		for (int I = 0; I < Info->NumUpValues; ++I) {
			int Index = Inst[2 + I].Count;
			ml_value_t **Slot = (Index < 0) ? &Frame->UpValues[~Index] : &Frame->Stack[Index];
			ml_value_t *Value = Slot[0];
			if (!Value) Value = Slot[0] = ml_uninitialized("<upvalue>", (ml_source_t){Frame->Source, Inst->Line});
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
		Type->Index = Inst[1].Count;
		Type->Type = (ml_type_t *)Result;
		Closure->ParamTypes = Type;
		ADVANCE(Inst + 2);
	}
	DO_PARTIAL_NEW: {
		Result = ml_deref(Result);
		*Top++ = ml_partial_function(Result, Inst[1].Count);
		ADVANCE(Inst + 2);
	}
	DO_PARTIAL_SET: {
		Result = ml_deref(Result);
		ml_partial_function_set(Top[-1], Inst[1].Count, Result);
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
		ML_STORE_COUNTER();
		Frame->Line = Inst->Line;
		Frame->Inst = Next;
		Frame->Top = Top - Count;
		return ml_call(Frame, AppendMethod, Count + 1, Args);
	}
	DO_STRING_ADD_1: {
		ml_value_t **Args = Top - 2;
		ml_inst_t *Next = Inst + 1;
		ML_STORE_COUNTER();
		Frame->Line = Inst->Line;
		Frame->Inst = Next;
		Frame->Top = Top - 1;
		return ml_call(Frame, AppendMethod, 2, Args);
	}
	DO_STRING_ADDS: {
		ml_stringbuffer_write((ml_stringbuffer_t *)Top[-1], Inst[2].Chars, Inst[1].Count);
		ADVANCE(Inst + 3);
	}
	DO_STRING_POP: {
		Result = *--Top;
		*Top = NULL;
		Result = ml_stringbuffer_get_value((ml_stringbuffer_t *)Result);
		ADVANCE(Inst + 1);
	}
	DO_STRING_END: {
		Result = Top[-1];
		Result = ml_stringbuffer_get_value((ml_stringbuffer_t *)Result);
		Top[-1] = Result;
		ADVANCE(Inst + 1);
	}
	DO_RESOLVE: {
		ml_value_t **Args = ml_alloc_args(2);
		Args[0] = Result;
		Args[1] = Inst[1].Value;
		ml_inst_t *Next = Inst + 2;
		ML_STORE_COUNTER();
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
	DO_IF_CONFIG: {
		if (((ml_config_fn)Inst[2].Data)(Frame->Base.Context)) {
			ADVANCE(Inst + 3);
		} else {
			ADVANCE(Inst[1].Inst);
		}
	}
	DO_CALL_COUNT(0);
	DO_CALL_COUNT(1);
	DO_CALL_COUNT(2);
	DO_CALL_COUNT(3);
	DO_CALL_COUNT(4);
	DO_CALL_COUNT(5);
	DO_CALL_COUNT(6);
	DO_CALL_COUNT(7);
	DO_CALL_COUNT(8);
	DO_CALL_COUNT(9);
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
		ml_state_schedule((ml_state_t *)Frame, Result);
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
		Frame = (DEBUG_STRUCT(frame) *)ml_frame();
	} else {
		Frame = bnew(Size);
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
		Frame->Stack[J] = Decl->Flags & MLC_DECL_ASVAR ? ml_variable(MLNil, NULL) : MLNil;
		Decl = Decl->Next;
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
			ml_value_t *Arg = ml_deref(Args[I]);
			if (ml_is_error(Arg)) ML_RETURN(Arg);
#ifdef ML_NANBOXING
			if (!ml_tag(Arg) && Arg->Type == MLNamesT) {
#else
			if (Arg->Type == MLNamesT) {
#endif
				ML_NAMES_CHECKX_ARG_COUNT(I);
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
	} else if (Flags & ML_CLOSURE_RELAX_NAMES) {
		for (; I < Count; ++I) {
			ml_value_t *Arg = ml_deref(Args[I]);
			if (ml_is_error(Arg)) ML_RETURN(Arg);
#ifdef ML_NANBOXING
			if (!ml_tag(Arg) && Arg->Type == MLNamesT) {
#else
			if (Arg->Type == MLNamesT) {
#endif
				ML_NAMES_CHECKX_ARG_COUNT(I);
				ML_NAMES_FOREACH(Arg, Node) {
					const char *Name = ml_string_value(Node->Value);
					int Index = (intptr_t)stringmap_search(Info->Params, Name);
					if (Index) {
						Frame->Stack[Index - 1] = ml_deref(Args[++I]);
					} else {
						++I;
					}
				}
				break;
			}
		}
	} else {
		for (; I < Count; ++I) {
			ml_value_t *Arg = ml_deref(Args[I]);
			if (ml_is_error(Arg)) ML_RETURN(Arg);
#ifdef ML_NANBOXING
			if (!ml_tag(Arg) && Arg->Type == MLNamesT) {
#else
			if (Arg->Type == MLNamesT) {
#endif
				ML_NAMES_CHECKX_ARG_COUNT(I);
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
	for (ml_param_type_t *Param = Closure->ParamTypes; Param; Param = Param->Next) {
		ml_value_t *Value = Frame->Stack[Param->Index];
		ml_type_t *Type = ml_typeof(Value);
		if (!ml_is_subtype(Type, Param->Type)) {
			ML_ERROR("TypeError", "Expected %s not %s for argument %d", Param->Type->Name, Type->Name, Param->Index + 1);
		}
	}
	Frame->Top = Frame->Stack + NumParams;
	Frame->OnError = Info->Return;
	Frame->UpValues = Closure->UpValues;
	Frame->Inst = Info->Entry;
	Frame->Line = Info->Entry->Line - 1;
#ifdef ML_SCHEDULER
	Frame->Counter = (uint64_t *)Caller->Context->Values[ML_COUNTER_INDEX];
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

static int ml_closure_find_labels(ml_inst_t *Inst, unsigned int *Labels) {
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_NONE: return 1;
	case MLIT_INST:
		if (!Inst[1].Inst->Label) Inst[1].Inst->Label = ++*Labels;
		return 2;
	case MLIT_INST_CONFIG:
		if (!Inst[1].Inst->Label) Inst[1].Inst->Label = ++*Labels;
	case MLIT_INST_COUNT:
		if (!Inst[1].Inst->Label) Inst[1].Inst->Label = ++*Labels;
		return 3;
	case MLIT_INST_COUNT_DECL:
		if (!Inst[1].Inst->Label) Inst[1].Inst->Label = ++*Labels;
		return 4;
	case MLIT_COUNT_COUNT: return 3;
	case MLIT_COUNT: return 2;
	case MLIT_VALUE: return 2;
	case MLIT_VALUE_COUNT: return 3;
	case MLIT_VALUE_COUNT_DATA: return 4;
	case MLIT_COUNT_CHARS: return 3;
	case MLIT_DECL: return 2;
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
	}
	return 0;
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
	case MLIT_INST_CONFIG:
		*(int *)(Info->Hash + I) ^= Inst[1].Inst->Label;
		*(int *)(Info->Hash + J) ^= Inst[2].Count;
		return 3;
	case MLIT_INST_COUNT:
		*(int *)(Info->Hash + I) ^= Inst[1].Inst->Label;
		*(int *)(Info->Hash + J) ^= Inst[2].Count;
		return 3;
	case MLIT_INST_COUNT_DECL:
		*(int *)(Info->Hash + I) ^= Inst[1].Inst->Label;
		*(int *)(Info->Hash + J) ^= Inst[2].Count;
		return 4;
	case MLIT_COUNT_COUNT:
		*(int *)(Info->Hash + I) ^= Inst[1].Count;
		*(int *)(Info->Hash + J) ^= Inst[2].Count;
		return 3;
	case MLIT_COUNT:
		*(int *)(Info->Hash + I) ^= Inst[1].Count;
		return 2;
	case MLIT_VALUE:
		*(long *)(Info->Hash + J) ^= ml_hash(Inst[1].Value);
		return 2;
	case MLIT_VALUE_COUNT:
		*(long *)(Info->Hash + J) ^= ml_hash(Inst[1].Value);
		*(int *)(Info->Hash + I) ^= Inst[2].Count;
		return 3;
	case MLIT_VALUE_COUNT_DATA:
		*(long *)(Info->Hash + J) ^= ml_hash(Inst[1].Value);
		*(int *)(Info->Hash + I) ^= Inst[2].Count;
		return 4;
	case MLIT_COUNT_CHARS:
		*(int *)(Info->Hash + I) ^= Inst[1].Count;
		*(long *)(Info->Hash + J) ^= stringmap_hash(Inst[2].Chars);
		return 3;
	case MLIT_DECL: return 2;
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
			int Index = Inst[2 + N].Count;
			*(int *)(Info->Hash + I) ^= (Index << N);
		}
		return 2 + Inst[1].ClosureInfo->NumUpValues;
	}
	case MLIT_SWITCH:
		return 3;
	}
	return 0;
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

ML_FUNCTION(MLClosure) {
//@closure
//<Original
//>closure
// Returns a copy of :mini:`Closure`.
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLClosureT);
	ml_closure_t *Original = (ml_closure_t *)Args[0];
	ml_closure_info_t *Info = Original->Info;
	ml_closure_t *Closure = xnew(ml_closure_t, Info->NumUpValues, ml_value_t *);
	Closure->Type = MLClosureT;
	Closure->Info = Info;
	Closure->ParamTypes = Original->ParamTypes;
	memcpy(Closure->UpValues, Original->UpValues, Info->NumUpValues * sizeof(ml_value_t *));
	return (ml_value_t *)Closure;
}

ML_TYPE(MLClosureT, (MLFunctionT, MLSequenceT), "closure",
// A Minilang function.
	.hash = ml_closure_hash,
	.call = (void *)ml_closure_call,
	.Constructor = (ml_value_t *)MLClosure
);

static void ML_TYPED_FN(ml_iterate, MLClosureT, ml_state_t *Frame, ml_closure_t *Closure) {
	return ml_closure_call(Frame, Closure, 0, NULL);
}

static void ML_TYPED_FN(ml_value_find_all, MLClosureT, ml_closure_t *Closure, void *Data, ml_value_find_fn RefFn) {
	if (!RefFn(Data, (ml_value_t *)Closure, 1)) return;
	ml_closure_info_t *Info = Closure->Info;
	Info->Type = MLClosureInfoT;
	ml_value_find_all((ml_value_t *)Info, Data, RefFn);
	for (int I = 0; I < Info->NumUpValues; ++I) ml_value_find_all(Closure->UpValues[I], Data, RefFn);
}

static int ML_TYPED_FN(ml_value_is_constant, MLClosureT, ml_closure_t *Closure) {
	ml_closure_info_t *Info = Closure->Info;
	for (int I = 0; I < Info->NumUpValues; ++I) {
		if (!ml_value_is_constant(Closure->UpValues[I])) return 0;
	}
	return 1;
}

static ml_value_t * ML_TYPED_FN(ml_method_wrap, MLClosureT, ml_value_t *Closure, int Count, ml_type_t **Types) {
	return Closure;
}

ML_TYPE(MLClosureInfoT, (), "closure::info");
// Information about a closure.

static void ML_TYPED_FN(ml_value_find_all, MLClosureInfoT, ml_closure_info_t *Info, void *Data, ml_value_find_fn RefFn) {
	if (!RefFn(Data, (ml_value_t *)Info, 1)) return;
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Opcode == MLI_LINK) {
			Inst = Inst[1].Inst;
			continue;
		}
		switch (MLInstTypes[Inst->Opcode]) {
		case MLIT_NONE:
			Inst += 1;
			break;
		case MLIT_INST:
			Inst += 2;
			break;
		case MLIT_INST_COUNT_DECL:
			Inst += 4;
			break;
		case MLIT_COUNT_COUNT:
			Inst += 3;
			break;
		case MLIT_COUNT:
			Inst += 2;
			break;
		case MLIT_VALUE:
			ml_value_find_all(Inst[1].Value, Data, RefFn);
			Inst += 2;
			break;
		case MLIT_VALUE_COUNT:
			ml_value_find_all(Inst[1].Value, Data, RefFn);
			Inst += 3;
			break;
		case MLIT_VALUE_COUNT_DATA:
			ml_value_find_all(Inst[1].Value, Data, RefFn);
			Inst += 4;
			break;
		case MLIT_COUNT_CHARS:
			Inst += 3;
			break;
		case MLIT_DECL:
			Inst += 2;
			break;
		case MLIT_COUNT_DECL:
			Inst += 3;
			break;
		case MLIT_COUNT_COUNT_DECL:
			Inst += 4;
			break;
		case MLIT_CLOSURE: {
			ml_closure_info_t *Info = Inst[1].ClosureInfo;
			if (!Info->Type) Info->Type = MLClosureInfoT;
			ml_value_find_all((ml_value_t *)Info, Data, RefFn);
			Inst += 2 + Info->NumUpValues;
			break;
		}
		case MLIT_SWITCH: {
			Inst += 3;
			break;
		}
		default: __builtin_unreachable();
		}
	}
}

ml_value_t *ml_closure(ml_closure_info_t *Info) {
	ml_closure_t *Closure = xnew(ml_closure_t, Info->NumUpValues, ml_value_t *);
	Closure->Type = MLClosureT;
	Closure->Info = Info;
	return (ml_value_t *)Closure;
}

void ml_closure_relax_names(ml_value_t *Value) {
	if (ml_is(Value, MLClosureT)) {
		ml_closure_t *Closure = (ml_closure_t *)Value;
		Closure->Info->Flags |= ML_CLOSURE_RELAX_NAMES;
	}
}

static void ML_TYPED_FN(ml_value_set_name, MLClosureT, ml_closure_t *Closure, const char *Name) {
	Closure->Name = Name;
}

ML_METHOD("append", MLStringBufferT, MLClosureT) {
//<Buffer
//<Closure
// Appends a representation of :mini:`Closure` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_closure_t *Closure = (ml_closure_t *)Args[1];
	const char *Name = Closure->Name ?: Closure->Info->Name;
	ml_stringbuffer_write(Buffer, Name, strlen(Name));
	return MLSome;
}

static int ml_closure_parameter_fn(const char *Name, void *Value, ml_value_t *Parameters[]) {
	Parameters[(intptr_t)Value * 2 - 2] = ml_string(Name, -1);
	return 0;
}

ML_METHOD("parameters", MLClosureT) {
//<Closure
//>list
// Returns the list of parameter names of :mini:`Closure`.
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	ml_value_t *Parameters[Closure->Info->Params->Size * 2];
	for (int I = 0; I < Closure->Info->Params->Size; ++I) {
		Parameters[I * 2 + 1] = MLNil;
	}
	stringmap_foreach(Closure->Info->Params, Parameters, (void *)ml_closure_parameter_fn);
	for (ml_param_type_t *Param = Closure->ParamTypes; Param; Param = Param->Next) {
		Parameters[Param->Index * 2 + 1] = (ml_value_t *)Param->Type;
	}
	ml_value_t *Map = ml_map();
	for (int I = 0; I < Closure->Info->Params->Size; ++I) {
		ml_map_insert(Map, Parameters[I * 2], Parameters[I * 2 + 1]);
	}
	return Map;
}

ML_METHOD("sha256", MLClosureT) {
//<Closure
//>address
// Returns the SHA256 hash of :mini:`Closure`.
	char *Hash = snew(SHA256_BLOCK_SIZE);
	ml_closure_sha256(Args[0], (unsigned char *)Hash);
	return ml_address(Hash, SHA256_BLOCK_SIZE);
}

static void ml_closure_value_list(ml_value_t *Value, ml_stringbuffer_t *Buffer) {
	if (ml_is(Value, MLStringT)) {
		ml_stringbuffer_write(Buffer, " \"", 2);
		int Length = ml_string_length(Value);
		const char *String = ml_string_value(Value);
		for (int I = 0; I < Length; ++I) switch (String[I]) {
			case 0: ml_stringbuffer_write(Buffer, "\\0", 2); break;
			case '\t': ml_stringbuffer_write(Buffer, "\\t", 2); break;
			case '\r': ml_stringbuffer_write(Buffer, "\\r", 2); break;
			case '\n': ml_stringbuffer_write(Buffer, "\\n", 2); break;
			case '\'': ml_stringbuffer_write(Buffer, "\\\'", 2); break;
			case '\"': ml_stringbuffer_write(Buffer, "\\\"", 2); break;
			case '\\': ml_stringbuffer_write(Buffer, "\\\\", 2); break;
			default: ml_stringbuffer_write(Buffer, String + I, 1); break;
		}
		ml_stringbuffer_put(Buffer, '\"');
	} else if (ml_is(Value, MLNumberT)) {
		ml_stringbuffer_put(Buffer, ' ');
		ml_stringbuffer_simple_append(Buffer, Value);
	} else if (ml_typeof(Value) == MLMethodT) {
		ml_stringbuffer_printf(Buffer, " :%s", ml_method_name(Value));
	} else if (ml_typeof(Value) == MLTypeT) {
		ml_stringbuffer_printf(Buffer, " <%s>", ml_type_name((ml_type_t *)Value));
	} else {
		ml_stringbuffer_printf(Buffer, " %s", ml_typeof(Value)->Name);
	}
	long Hash = ml_hash(Value);
	ml_stringbuffer_printf(Buffer, "[%ld]", Hash);
}

static int ml_closure_inst_list(ml_inst_t *Inst, ml_stringbuffer_t *Buffer) {
	if (Inst->Label) ml_stringbuffer_printf(Buffer, "L%d:", Inst->Label);
	ml_stringbuffer_printf(Buffer, "\t %3d %s", Inst->Line, MLInstNames[Inst->Opcode]);
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_NONE: return 1;
	case MLIT_INST:
		ml_stringbuffer_printf(Buffer, " ->L%d", Inst[1].Inst->Label);
		return 2;
	case MLIT_INST_CONFIG:
		// TODO: Implement this!
		ml_stringbuffer_printf(Buffer, " ->L%d", Inst[1].Inst->Label);
		ml_stringbuffer_printf(Buffer, ", %d", Inst[2].Count);
		return 3;
	case MLIT_INST_COUNT:
		ml_stringbuffer_printf(Buffer, " ->L%d", Inst[1].Inst->Label);
		ml_stringbuffer_printf(Buffer, ", %d", Inst[2].Count);
		return 3;
	case MLIT_INST_COUNT_DECL:
		ml_stringbuffer_printf(Buffer, " ->L%d", Inst[1].Inst->Label);
		ml_stringbuffer_printf(Buffer, ", %d", Inst[2].Count);
		if (Inst[3].Decls) {
			ml_stringbuffer_printf(Buffer, " <%s>", Inst[3].Decls->Ident);
		} else {
			ml_stringbuffer_printf(Buffer, " -");
		}
		return 4;
	case MLIT_COUNT_COUNT:
		ml_stringbuffer_printf(Buffer, " %d, %d", Inst[1].Count, Inst[2].Count);
		return 3;
	case MLIT_COUNT:
		ml_stringbuffer_printf(Buffer, " %d", Inst[1].Count);
		return 2;
	case MLIT_VALUE:
		ml_closure_value_list(Inst[1].Value, Buffer);
		return 2;
	case MLIT_VALUE_COUNT:
		ml_closure_value_list(Inst[1].Value, Buffer);
		ml_stringbuffer_printf(Buffer, ", %d", Inst[2].Count);
		return 3;
	case MLIT_VALUE_COUNT_DATA:
		ml_closure_value_list(Inst[1].Value, Buffer);
		ml_stringbuffer_printf(Buffer, ", %d", Inst[2].Count);
		return 4;
	case MLIT_COUNT_CHARS:
		ml_stringbuffer_printf(Buffer, " %d, \"", Inst[1].Count);
		for (const char *P = Inst[2].Chars; *P; ++P) switch(*P) {
			case 0: ml_stringbuffer_write(Buffer, "\\0", 2); break;
			case '\e': ml_stringbuffer_write(Buffer, "\\e", 2); break;
			case '\t': ml_stringbuffer_write(Buffer, "\\t", 2); break;
			case '\r': ml_stringbuffer_write(Buffer, "\\r", 2); break;
			case '\n': ml_stringbuffer_write(Buffer, "\\n", 2); break;
			case '\'': ml_stringbuffer_write(Buffer, "\\\'", 2); break;
			case '\"': ml_stringbuffer_write(Buffer, "\\\"", 2); break;
			case '\\': ml_stringbuffer_write(Buffer, "\\\\", 2); break;
			default: ml_stringbuffer_write(Buffer, P, 1); break;
		}
		ml_stringbuffer_put(Buffer, '\"');
		return 3;
	case MLIT_DECL:
		if (Inst[1].Decls) {
			ml_stringbuffer_printf(Buffer, " <%s>", Inst[1].Decls->Ident);
		} else {
			ml_stringbuffer_printf(Buffer, " -");
		}
		return 2;
	case MLIT_COUNT_DECL:
		if (Inst[2].Decls) {
			ml_stringbuffer_printf(Buffer, " %d <%s>", Inst[1].Count, Inst[2].Decls->Ident);
		} else {
			ml_stringbuffer_printf(Buffer, " %d -", Inst[1].Count);
		}
		return 3;
	case MLIT_COUNT_COUNT_DECL:
		if (Inst[3].Decls) {
			ml_stringbuffer_printf(Buffer, " %d, %d <%s>", Inst[1].Count, Inst[2].Count, Inst[3].Decls->Ident);
		} else {
			ml_stringbuffer_printf(Buffer, " %d, %d -", Inst[1].Count, Inst[2].Count);
		}
		return 4;
	case MLIT_CLOSURE: {
		ml_closure_info_t *Info = Inst[1].ClosureInfo;
		ml_stringbuffer_printf(Buffer, " %s:%d", Info->Source, Info->StartLine);
		for (int N = 0; N < Info->NumUpValues; ++N) {
			ml_stringbuffer_printf(Buffer, ", %d", Inst[2 + N].Count);
		}
		ml_stringbuffer_write(Buffer, "\n\t    /--------\\\n", strlen("\n\t    /--------\\\n"));
		ml_closure_info_list(Buffer, Info);
		ml_stringbuffer_write(Buffer, "\t    \\--------/", strlen("\t    \\--------/"));
		return 2 + Info->NumUpValues;
	}
	case MLIT_SWITCH: {
		int Count = Inst[1].Count;
		if (Count) {
			ml_inst_t **Insts = Inst[2].Insts;
			ml_stringbuffer_printf(Buffer, " L%d", Insts[0]->Label);
			for (int I = 1; I < Count; ++I) {
				ml_stringbuffer_printf(Buffer, ", L%d", Insts[I]->Label);
			}
		}
		return 3;
	}
	default: __builtin_unreachable();
	}
}

static int ML_TYPED_FN(ml_function_source, MLClosureT, ml_closure_t *Closure, const char **Source, int *Line) {
	*Source = Closure->Info->Source;
	*Line = Closure->Info->StartLine;
	return 1;
}

ML_METHOD("info", MLClosureT) {
//<Closure
//>map
// Returns some information about :mini:`Closure`.
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	ml_closure_info_t *Info = Closure->Info;
	ml_value_t *Result = ml_map();
	ml_map_insert(Result, ml_cstring("Source"), ml_string(Info->Source, -1));
	ml_map_insert(Result, ml_cstring("Start"), ml_integer(Info->StartLine));
	ml_map_insert(Result, ml_cstring("End"), ml_integer(Info->EndLine));
	ml_map_insert(Result, ml_cstring("Size"), ml_integer(Info->FrameSize));
	ml_map_insert(Result, ml_cstring("Flags"), ml_integer(Info->Flags));
	return Result;
}

ML_METHOD("values", MLClosureT) {
//<Closure
//>map
// Returns some information about :mini:`Closure`.
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	ml_closure_info_t *Info = Closure->Info;
	ml_value_t *Result = ml_list();
	for (int I = 0; I < Info->NumUpValues; ++I) ml_list_put(Result, Closure->UpValues[I]);
	return Result;
}

void ml_closure_info_list(ml_stringbuffer_t *Buffer, ml_closure_info_t *Info) {
	ml_closure_info_labels(Info);
	for (ml_inst_t *Inst = Info->Entry; Inst != Info->Halt;) {
		if (Inst->Opcode == MLI_LINK) {
			Inst = Inst[1].Inst;
		} else {
			Inst += ml_closure_inst_list(Inst, Buffer);
		}
		ml_stringbuffer_put(Buffer, '\n');
	}
}

ML_METHOD("list", MLClosureT) {
//<Closure
//>string
// Returns a listing of the bytecode of :mini:`Closure`.
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_closure_info_t *Info = Closure->Info;
	ml_stringbuffer_printf(Buffer, "@%s:%d\n", Info->Source, Info->StartLine);
	ml_closure_info_list(Buffer, Info);
	ml_stringbuffer_put(Buffer, '\n');
	for (int I = 0; I < Info->NumUpValues; ++I) {
		ml_value_t *UpValue = Closure->UpValues[I];
		ml_stringbuffer_printf(Buffer, "Upvalues %d:", I);
		ml_closure_value_list(UpValue, Buffer);
		ml_stringbuffer_put(Buffer, '\n');
	}
	return ml_stringbuffer_get_value(Buffer);
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

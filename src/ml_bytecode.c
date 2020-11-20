#ifndef DEBUG_VERSION
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

ML_TYPE(MLVariableT, (), "variable",
	.hash = (void *)ml_variable_hash,
	.deref = (void *)ml_variable_deref,
	.assign = (void *)ml_variable_assign
);

#endif

#ifdef USE_ML_JIT
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
	const char *Source;
	ml_inst_t *Inst;
	ml_value_t **Top;
	ml_inst_t *OnError;
	ml_value_t **UpValues;
#ifdef USE_ML_SCHEDULER
	ml_schedule_t Schedule;
#endif
	unsigned int Reuse:1;
	unsigned int Suspend:1;
	unsigned int Reentry:1;
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
	Frame->Reuse = 0;
	return Frame->Base.run((ml_state_t *)Frame, Count ? Args[0] : MLNil);
}

ML_TYPE(DEBUG_TYPE(Continuation), (MLStateT, MLIteratableT), "continuation",
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
	return (ml_source_t){Frame->Source, Frame->Inst->LineNo};
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
	if (!Suspension->Suspend) ML_ERROR("StateError", "Function did not suspend");
	Suspension->Base.Type = DEBUG_TYPE(Continuation);
	Suspension->Top[-2] = Suspension->Top[-1];
	--Suspension->Top;
	Suspension->Base.Caller = Caller;
	Suspension->Base.Context = Caller->Context;
	ML_CONTINUE(Suspension, MLNil);
}

static void ML_TYPED_FN(ml_iterate, DEBUG_TYPE(Continuation), ml_state_t *Caller, DEBUG_STRUCT(frame) *Suspension) {
	if (!Suspension->Suspend) ML_ERROR("StateError", "Function did not suspend");
	ML_RETURN(Suspension);
}

#ifndef DEBUG_VERSION

#ifdef USE_ML_SCHEDULER
#define CHECK_COUNTER if (--Counter == 0) goto DO_SWAP;
#else
#define CHECK_COUNTER
#endif

#define ERROR() { \
	Inst = Frame->OnError; \
	CHECK_COUNTER \
	goto *Labels[Inst->Opcode]; \
}

#define ADVANCE(N) { \
	Inst = Inst->Params[N].Inst; \
	CHECK_COUNTER \
	goto *Labels[Inst->Opcode]; \
}

#define ERROR_CHECK(VALUE) if (ml_is_error(VALUE)) { \
	ml_error_trace_add(VALUE, (ml_source_t){Frame->Source, Inst->LineNo}); \
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

#define ADVANCE(N) { \
	Inst = Inst->Params[N].Inst; \
	CHECK_COUNTER \
	goto DO_DEBUG_ADVANCE; \
}

#endif

#ifndef DEBUG_VERSION
void *MLCachedFrame = NULL;

extern ml_value_t *SymbolMethod;
#endif

static ML_METHOD_DECL(Append, "append");

static void DEBUG_FUNC(frame_run)(DEBUG_STRUCT(frame) *Frame, ml_value_t *Result) {
	if (!Result) {
		ml_value_t *Error = ml_error("RuntimeError", "NULL value passed to continuation");
		ml_error_trace_add(Error, (ml_source_t){Frame->Source, Frame->Inst->LineNo});
		ML_CONTINUE(Frame->Base.Caller, Error);
	}
#ifdef USE_ML_SCHEDULER
	int Counter = Frame->Schedule.Counter[0];
#endif
#ifdef ML_USE_INST_FNS
#ifndef DEBUG_VERSION
	if (ml_is_error(Result)) {
		ml_error_trace_add(Result, (ml_source_t){Frame->Source, Frame->Inst->LineNo});
		return Frame->OnError->run(Frame, Result, Frame->Top, Frame->OnError);
	} else {
		return Frame->Inst->run(Frame, Result, Frame->Top, Frame->Inst);
	}
#endif
#endif
	static void *Labels[] = {
		[MLI_RETURN] = &&DO_RETURN,
		[MLI_SUSPEND] = &&DO_SUSPEND,
		[MLI_RESUME] = &&DO_RESUME,
		[MLI_NIL] = &&DO_NIL,
		[MLI_NIL_PUSH] = &&DO_NIL_PUSH,
		[MLI_SOME] = &&DO_SOME,
		[MLI_AND] = &&DO_AND,
		[MLI_OR] = &&DO_OR,
		[MLI_PUSH] = &&DO_PUSH,
		[MLI_WITH] = &&DO_WITH,
		[MLI_WITH_VAR] = &&DO_WITH_VAR,
		[MLI_WITHX] = &&DO_WITHX,
		[MLI_POP] = &&DO_POP,
		[MLI_ENTER] = &&DO_ENTER,
		[MLI_EXIT] = &&DO_EXIT,
		[MLI_LOOP] = &&DO_LOOP,
		[MLI_TRY] = &&DO_TRY,
		[MLI_CATCH] = &&DO_CATCH,
		[MLI_LOAD] = &&DO_LOAD,
		[MLI_LOAD_PUSH] = &&DO_LOAD_PUSH,
		[MLI_VAR] = &&DO_VAR,
		[MLI_VAR_TYPE] = &&DO_VAR_TYPE,
		[MLI_VARX] = &&DO_VARX,
		[MLI_LET] = &&DO_LET,
		[MLI_LETI] = &&DO_LETI,
		[MLI_LETX] = &&DO_LETX,
		[MLI_FOR] = &&DO_FOR,
		[MLI_IF] = &&DO_IF,
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
		[MLI_TUPLE_SET] = &&DO_TUPLE_SET,
		[MLI_LIST_NEW] = &&DO_LIST_NEW,
		[MLI_LIST_APPEND] = &&DO_LIST_APPEND,
		[MLI_MAP_NEW] = &&DO_MAP_NEW,
		[MLI_MAP_INSERT] = &&DO_MAP_INSERT,
		[MLI_CLOSURE] = &&DO_CLOSURE,
#ifdef USE_GENERICS
		[MLI_CLOSURE_TYPED] = &&DO_CLOSURE_TYPED,
#endif
		[MLI_PARAM_TYPE] = &&DO_PARAM_TYPE,
		[MLI_PARTIAL_NEW] = &&DO_PARTIAL_NEW,
		[MLI_PARTIAL_SET] = &&DO_PARTIAL_SET,
		[MLI_STRING_NEW] = &&DO_STRING_NEW,
		[MLI_STRING_ADD] = &&DO_STRING_ADD,
		[MLI_STRING_ADDS] = &&DO_STRING_ADDS,
		[MLI_STRING_END] = &&DO_STRING_END,
		[MLI_RESOLVE] = &&DO_RESOLVE
	};
	ml_inst_t *Inst = Frame->Inst;
	ml_value_t **Top = Frame->Top;
#ifdef DEBUG_VERSION
	if (Frame->Reentry) {
		Frame->Reentry = 0;
		goto *Labels[Inst->Opcode];
	}
#endif
	ERROR_CHECK(Result);
#ifdef DEBUG_VERSION
	goto DO_DEBUG_ADVANCE;
#else
	goto *Labels[Inst->Opcode];
#endif

	DO_RETURN: {
#ifdef USE_ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		ml_state_t *Caller = Frame->Base.Caller;
		if (Frame->Reuse) {
			memset(Frame, 0, ML_FRAME_REUSE_SIZE);
			*(ml_frame_t **)Frame = MLCachedFrame;
			MLCachedFrame = Frame;
		} else {
			Frame->Inst = Inst;
		}
		ML_CONTINUE(Caller, Result);
	}
	DO_SUSPEND: {
		Frame->Inst = Inst->Params[0].Inst;
		Frame->Top = Top;
#ifdef USE_ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		Frame->Suspend = 1;
		ML_CONTINUE(Frame->Base.Caller, (ml_value_t *)Frame);
	}
	DO_RESUME: {
		Frame->Suspend = 0;
		*--Top = 0;
		ADVANCE(0);
	}
	DO_NIL: {
		Result = MLNil;
		ADVANCE(0);
	}
	DO_NIL_PUSH: {
		Result = MLNil;
		*Top++ = Result;
		ADVANCE(0);
	}
	DO_SOME: {
		Result = MLSome;
		ADVANCE(0);
	}
	DO_AND: {
		if (ml_deref(Result) == MLNil) {
			ADVANCE(0);
		} else {
			ADVANCE(1);
		}
	}
	DO_OR: {
		if (ml_deref(Result) != MLNil) {
			ADVANCE(0);
		} else {
			ADVANCE(1);
		}
	}
	DO_PUSH: {
		*Top++ = Result;
		ADVANCE(0);
	}
	DO_WITH: {
		*Top++ = Result;
#ifdef DEBUG_VERSION
		Frame->Decls = Inst->Params[1].Decls;
#endif
		ADVANCE(0);
	}
	DO_WITH_VAR: {
		ml_variable_t *Local = new(ml_variable_t);
		Local->Type = MLVariableT;
		Local->Value = Result;
		*Top++ = (ml_value_t *)Local;
#ifdef DEBUG_VERSION
		Frame->Decls = Inst->Params[1].Decls;
#endif
		ADVANCE(0);
	}
	DO_WITHX: {
		ml_value_t *Packed = Result;
		int Count = Inst->Params[1].Count;
		for (int I = 0; I < Count; ++I) {
			Result = ml_unpack(Packed, I + 1);
			*Top++ = Result;
		}
#ifdef DEBUG_VERSION
		Frame->Decls = Inst->Params[2].Decls;
#endif
		ADVANCE(0);
	}
	DO_POP: {
		Result = *--Top;
		*Top = 0;
		ADVANCE(0);
	}
	DO_ENTER: {
		for (int I = Inst->Params[1].Count; --I >= 0;) {
			ml_variable_t *Local = new(ml_variable_t);
			Local->Type = MLVariableT;
			Local->Value = MLNil;
			*Top++ = (ml_value_t *)Local;
		}
		for (int I = Inst->Params[2].Count; --I >= 0;) {
			*Top++ = NULL;
		}
#ifdef DEBUG_VERSION
		Frame->Decls = Inst->Params[3].Decls;
#endif
		ADVANCE(0);
	}
	DO_EXIT: {
		for (int I = Inst->Params[1].Count; --I >= 0;) *--Top = 0;
#ifdef DEBUG_VERSION
		Frame->Decls = Inst->Params[2].Decls;
#endif
		ADVANCE(0);
	}
	DO_LOOP: {
		ADVANCE(0);
	}
	DO_TRY: {
		Frame->OnError = Inst->Params[1].Inst;
		ADVANCE(0);
	}
	DO_CATCH: {
		if (!ml_is_error(Result)) {
			Result = ml_error("InternalError", "expected error value, not %s", ml_typeof(Result)->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		}
		if (Inst->Params[3].Ptr && strcmp(ml_error_type(Result), Inst->Params[3].Ptr)) {
			ADVANCE(0);
		}
		Result = ml_error_value(Result);
		ml_value_t **Old = Frame->Stack + Inst->Params[2].Index;
		while (Top > Old) *--Top = 0;
		*Top++ = Result;
#ifdef DEBUG_VERSION
		Frame->Decls = Inst->Params[4].Decls;
#endif
		ADVANCE(1);
	}
	DO_LOAD: {
		Result = Inst->Params[1].Value;
		ADVANCE(0);
	}
	DO_LOAD_PUSH: {
		Result = Inst->Params[1].Value;
		*Top++ = Result;
		ADVANCE(0);
	}
	DO_VAR: {
		Result = ml_deref(Result);
		//ERROR_CHECK(Result);
		ml_variable_t *Variable = (ml_variable_t *)Top[Inst->Params[1].Index];
		if (Variable->VarType && !ml_is(Result, Variable->VarType)) {
			Result = ml_error("TypeError", "Cannot assign %s to variable of type %s", ml_typeof(Result)->Name, Variable->VarType->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		}
		Variable->Value = Result;
		ADVANCE(0);
	}
	DO_VAR_TYPE: {
		Result = ml_deref(Result);
		if (!ml_is(Result, MLTypeT)) {
			Result = ml_error("TypeError", "expected type, not %s", ml_typeof(Result)->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		}
		ml_variable_t *Local = (ml_variable_t *)Top[Inst->Params[1].Index];
		Local->VarType = (ml_type_t *)Result;
		ADVANCE(0);
	}
	DO_VARX: {
		ml_value_t *Packed = ml_deref(Result);
		//ERROR_CHECK(Packed);
		int Count = Inst->Params[2].Count;
		ml_value_t **Base = Top + Inst->Params[1].Index;
		for (int I = 0; I < Count; ++I) {
			Result = ml_unpack(Packed, I + 1);
			Result = ml_deref(Result);
			//ERROR_CHECK(Result);
			ml_variable_t *Local = (ml_variable_t *)Base[I];
			Local->Value = Result;
		}
		ADVANCE(0);
	}
	DO_LET: {
		Result = ml_deref(Result);
		//ERROR_CHECK(Result);
		Top[Inst->Params[1].Index] = Result;
		ADVANCE(0);
	}
	DO_LETI: {
		Result = ml_deref(Result);
		//ERROR_CHECK(Result);
		ml_value_t *Uninitialized = Top[Inst->Params[1].Index];
		Top[Inst->Params[1].Index] = Result;
		if (Uninitialized) ml_uninitialized_set(Uninitialized, Result);
		ADVANCE(0);
	}
	DO_LETX: {
		ml_value_t *Packed = ml_deref(Result);
		//ERROR_CHECK(Packed);
		int Count = Inst->Params[2].Count;
		ml_value_t **Base = Top + Inst->Params[1].Index;
		for (int I = 0; I < Count; ++I) {
			Result = ml_unpack(Packed, I + 1);
			Result = ml_deref(Result);
			//ERROR_CHECK(Result);
			ml_value_t *Uninitialized = Base[I];
			Base[I] = Result;
			if (Uninitialized) ml_uninitialized_set(Uninitialized, Result);
		}
		ADVANCE(0);
	}
	DO_FOR: {
		Result = ml_deref(Result);
		//ERROR_CHECK(Result);
		Frame->Inst = Inst->Params[0].Inst;
		Frame->Top = Top;
#ifdef USE_ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		return ml_iterate((ml_state_t *)Frame, Result);
	}
	DO_IF: {
		if (Result == MLNil) {
			ADVANCE(0);
		} else {
			ADVANCE(1);
		}
	}
	DO_NEXT: {
		for (int I = Inst->Params[1].Count; --I >= 0;) *--Top = 0;
		Result = Top[-1];
		*--Top = 0;
		Frame->Inst = Inst->Params[0].Inst;
		Frame->Top = Top;
#ifdef USE_ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		return ml_iter_next((ml_state_t *)Frame, Result);
	}
	DO_VALUE: {
		Result = Top[Inst->Params[1].Index];
		Frame->Inst = Inst->Params[0].Inst;
		Frame->Top = Top;
#ifdef USE_ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		return ml_iter_value((ml_state_t *)Frame, Result);
	}
	DO_KEY: {
		Result = Top[Inst->Params[1].Index];
		Frame->Inst = Inst->Params[0].Inst;
		Frame->Top = Top;
#ifdef USE_ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		return ml_iter_key((ml_state_t *)Frame, Result);
	}
	DO_CALL: {
		int Count = Inst->Params[1].Count;
		ml_value_t *Function = Top[~Count];
		Function = ml_deref(Function);
		//ERROR_CHECK(Function);
		ml_value_t **Args = Top - Count;
		ml_inst_t *Next = Inst->Params[0].Inst;
#ifdef USE_ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		if (Inst->Opcode == MLI_RETURN) {
			return ml_call(Frame->Base.Caller, Function, Count, Args);
		} else {
			Frame->Inst = Next;
			Frame->Top = Top - (Count + 1);
			return ml_call(Frame, Function, Count, Args);
		}
	}
	DO_CONST_CALL: {
		int Count = Inst->Params[1].Count;
		ml_value_t *Function = Inst->Params[2].Value;
		ml_value_t **Args = Top - Count;
		ml_inst_t *Next = Inst->Params[0].Inst;
#ifdef USE_ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		if (Inst->Opcode == MLI_RETURN) {
			return ml_call(Frame->Base.Caller, Function, Count, Args);
		} else {
			Frame->Inst = Next;
			Frame->Top = Top - Count;
			return ml_call(Frame, Function, Count, Args);
		}
	}
	DO_ASSIGN: {
		Result = ml_deref(Result);
		//ERROR_CHECK(Result);
		ml_value_t *Ref = Top[-1];
		*--Top = 0;
		Result = ml_typeof(Ref)->assign(Ref, Result);
		ERROR_CHECK(Result);
		ADVANCE(0);
	}
	DO_LOCAL: {
		int Index = Inst->Params[1].Index;
		Result = Frame->Stack[Index];
		ADVANCE(0);
	}
	DO_LOCAL_PUSH: {
		int Index = Inst->Params[1].Index;
		Result = Frame->Stack[Index];
		*Top++ = Result;
		ADVANCE(0);
	}
	DO_UPVALUE: {
		int Index = Inst->Params[1].Index;
		Result = Frame->UpValues[Index];
		ADVANCE(0);
	}
	DO_LOCALX: {
		int Index = Inst->Params[1].Index;
		ml_value_t **Slot = &Frame->Stack[Index];
		Result = Slot[0];
		if (!Result) Result = Slot[0] = ml_uninitialized(Inst->Params[2].Ptr);
		ADVANCE(0);
	}
	DO_TUPLE_NEW: {
		*Top++ = ml_tuple(Inst->Params[1].Count);
		ADVANCE(0);
	}
	DO_TUPLE_SET: {
		int Index = Inst->Params[1].Index;
		ml_tuple_t *Tuple = (ml_tuple_t *)Top[-1];
		Tuple->Values[Index] = Result;
		ADVANCE(0);
	}
	DO_LIST_NEW: {
		*Top++ = ml_list();
		ADVANCE(0);
	}
	DO_LIST_APPEND: {
		Result = ml_deref(Result);
		//ERROR_CHECK(Result);
		ml_list_put(Top[-1], Result);
		ADVANCE(0);
	}
	DO_MAP_NEW: {
		*Top++ = ml_map();
		ADVANCE(0);
	}
	DO_MAP_INSERT: {
		ml_value_t *Key = ml_deref(Top[-1]);
		//ERROR_CHECK(Key);
		Result = ml_deref(Result);
		//ERROR_CHECK(Result);
		ml_map_insert(Top[-2], Key, Result);
		*--Top = 0;
		ADVANCE(0);
	}
	DO_CLOSURE: {
		// closure <entry> <frame_size> <num_params> <num_upvalues> <upvalue_1> ...
		ml_closure_info_t *Info = Inst->Params[1].ClosureInfo;
		ml_closure_t *Closure = xnew(ml_closure_t, Info->NumUpValues, ml_value_t *);
		Closure->Type = MLClosureT;
		Closure->Info = Info;
		for (int I = 0; I < Info->NumUpValues; ++I) {
			int Index = Inst->Params[2 + I].Index;
			ml_value_t **Slot = (Index < 0) ? &Frame->UpValues[~Index] : &Frame->Stack[Index];
			ml_value_t *Value = Slot[0];
			if (!Value) Value = Slot[0] = ml_uninitialized("<upvalue>");
			if (ml_typeof(Value) == MLUninitializedT) {
				ml_uninitialized_use(Value, &Closure->UpValues[I]);
			}
			Closure->UpValues[I] = Value;
		}
		Result = (ml_value_t *)Closure;
		ADVANCE(0);
	}
#ifdef USE_GENERICS
	DO_CLOSURE_TYPED: {
		// closure <entry> <frame_size> <num_params> <num_upvalues> <upvalue_1> ...
		if (!ml_is(Result, MLTypeT)) {
			Result = ml_error("InternalError", "expected type, not %s", ml_typeof(Result)->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		}
		ml_closure_info_t *Info = Inst->Params[1].ClosureInfo;
		ml_closure_t *Closure = xnew(ml_closure_t, Info->NumUpValues, ml_value_t *);
		const ml_type_t *Type = (ml_type_t *)Result;
		Closure->Type = ml_type_generic(MLClosureT, 1, &Type);
		Closure->Info = Info;
		for (int I = 0; I < Info->NumUpValues; ++I) {
			int Index = Inst->Params[2 + I].Index;
			ml_value_t **Slot = (Index < 0) ? &Frame->UpValues[~Index] : &Frame->Stack[Index];
			ml_value_t *Value = Slot[0];
			if (!Value) Value = Slot[0] = ml_uninitialized("<upvalue>");
			if (ml_typeof(Value) == MLUninitializedT) {
				ml_uninitialized_use(Value, &Closure->UpValues[I]);
			}
			Closure->UpValues[I] = Value;
		}
		Result = (ml_value_t *)Closure;
		ADVANCE(0);
	}
#endif
	DO_PARAM_TYPE: {
		Result = ml_deref(Result);
		if (!ml_is(Result, MLTypeT)) {
			Result = ml_error("TypeError", "expected type, not %s", ml_typeof(Result)->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		}
		ml_closure_t *Closure = (ml_closure_t *)Top[-1];
		ml_param_type_t *Type = new(ml_param_type_t);
		Type->Next = Closure->ParamTypes;
		Type->Index = Inst->Params[1].Index;
		Type->Type = (ml_type_t *)Result;
		Closure->ParamTypes = Type;
		ADVANCE(0);
	}
	DO_PARTIAL_NEW: {
		Result = ml_deref(Result);
		//ERROR_CHECK(Result);
		*Top++ = ml_partial_function_new(Result, Inst->Params[1].Count);
		ADVANCE(0);
	}
	DO_PARTIAL_SET: {
		Result = ml_deref(Result);
		//ERROR_CHECK(Result);
		ml_partial_function_set(Top[-1], Inst->Params[1].Index, Result);
		ADVANCE(0);
	}
	DO_STRING_NEW: {
		*Top++ = ml_stringbuffer();
		ADVANCE(0);
	}
	DO_STRING_ADD: {
		int Count = Inst->Params[1].Count;
		ml_value_t **Args = Top - (Count + 1);
		ml_inst_t *Next = Inst->Params[0].Inst;
#ifdef USE_ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		Frame->Inst = Next;
		Frame->Top = Top - Count;
		return ml_call(Frame, AppendMethod, Count + 1, Args);
	}
	DO_STRING_ADDS: {
		ml_stringbuffer_add((ml_stringbuffer_t *)Top[-1], Inst->Params[2].Ptr, Inst->Params[1].Count);
		ADVANCE(0);
	}
	DO_STRING_END: {
		Result = *--Top;
		*Top = 0;
		Result = ml_stringbuffer_value((ml_stringbuffer_t *)Result);
		ADVANCE(0);
	}
	DO_RESOLVE: {
		*Top++ = Inst->Params[1].Value;
		*Top++ = Inst->Params[2].Value;
		ml_value_t **Args = Top - 2;
		ml_inst_t *Next = Inst->Params[0].Inst;
#ifdef USE_ML_SCHEDULER
		Frame->Schedule.Counter[0] = Counter;
#endif
		Frame->Inst = Next;
		Frame->Top = Top - 2;
		return ml_call(Frame, SymbolMethod, 2, Args);
		ADVANCE(0);
	}
#ifdef DEBUG_VERSION
	DO_DEBUG_ERROR: {
		ml_debugger_t *Debugger = Frame->Debugger;
		if (Debugger->BreakOnError) goto DO_BREAKPOINT;
	}
	DO_DEBUG_ADVANCE: {
		ml_debugger_t *Debugger = Frame->Debugger;
		if (Inst->PotentialBreakpoint) {
			if (Debugger->StepIn) goto DO_BREAKPOINT;
			if (Frame->StepOver) goto DO_BREAKPOINT;
			if (Inst->Opcode == MLI_RETURN && Frame->StepOut) goto DO_BREAKPOINT;
			size_t *Breakpoints;
			unsigned int Revision = Debugger->Revision;
			if (Frame->Revision != Revision) {
				Frame->Revision = Revision;
				Breakpoints = Frame->Breakpoints = Debugger->breakpoints(Debugger, Frame->Source, 0);
			} else {
				Breakpoints = Frame->Breakpoints;
			}
			int LineNo = Inst->LineNo;
			if (Breakpoints[LineNo / SIZE_BITS] & (1 << LineNo % SIZE_BITS)) goto DO_BREAKPOINT;
		}
		CHECK_COUNTER
		goto *Labels[Inst->Opcode];
	}
	DO_BREAKPOINT: {
		ml_debugger_t *Debugger = Frame->Debugger;
		Frame->Inst = Inst;
		Frame->Top = Top;
		Frame->Reentry = 1;
		return Debugger->run(Debugger, (ml_state_t *)Frame, Result);
	}
#endif
#ifdef USE_ML_SCHEDULER
	DO_SWAP: {
		Frame->Inst = Inst;
		Frame->Top = Top;
		return Frame->Schedule.swap((ml_state_t *)Frame, Result);
	}
#endif
}

static void ml_closure_call_debug(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args);

static void DEBUG_FUNC(closure_call)(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	ml_closure_info_t *Info = Closure->Info;
	ml_debugger_t *Debugger = (ml_debugger_t *)Caller->Context->Values[ML_DEBUGGER_INDEX];
#ifndef DEBUG_VERSION
	if (Debugger) return ml_closure_call_debug(Caller, Value, Count, Args);
#endif
	size_t Size = sizeof(DEBUG_STRUCT(frame)) + Info->FrameSize * sizeof(ml_value_t *);
	DEBUG_STRUCT(frame) *Frame;
	if (Size <= ML_FRAME_REUSE_SIZE) {
		if ((Frame = MLCachedFrame)) {
			MLCachedFrame = *(ml_frame_t **)Frame;
		} else {
			Frame = GC_MALLOC(ML_FRAME_REUSE_SIZE);
		}
		Frame->Reuse = 1;
	} else {
		Frame = GC_MALLOC(Size);
	}
	Frame->Base.Type = DEBUG_TYPE(Continuation);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)DEBUG_FUNC(frame_run);
	Frame->Base.Context = Caller->Context;
	Frame->Source = Info->Source;
	int NumParams = Info->NumParams;
	if (Closure->PartialCount) {
		int CombinedCount = Count + Closure->PartialCount;
		ml_value_t **CombinedArgs = anew(ml_value_t *, CombinedCount);
		memcpy(CombinedArgs, Closure->UpValues + Info->NumUpValues, Closure->PartialCount * sizeof(ml_value_t *));
		memcpy(CombinedArgs + Closure->PartialCount, Args, Count * sizeof(ml_value_t *));
		Count = CombinedCount;
		Args = CombinedArgs;
	}
	if (Info->ExtraArgs) --NumParams;
	if (Info->NamedArgs) --NumParams;
	int Min = (Count < NumParams) ? Count : NumParams;
	int I = 0;
	for (; I < Min; ++I) {
		ml_value_t *Arg = ml_deref(Args[I]);
		if (ml_is_error(Arg)) ML_RETURN(Arg);
		if (ml_typeof(Arg) == MLNamesT) break;
		Frame->Stack[I] = Arg;
	}
	for (int J = I; J < NumParams; ++J) {
		Frame->Stack[J] = MLNil;
	}
	if (Info->ExtraArgs) {
		ml_value_t *Rest = ml_list();
		for (; I < Count; ++I) {
			ml_value_t *Arg = ml_deref(Args[I]);
			if (ml_is_error(Arg)) ML_RETURN(Arg);
			if (ml_typeof(Arg) == MLNamesT) break;
			ml_list_put(Rest, Arg);
		}
		Frame->Stack[NumParams] = Rest;
		++NumParams;
	}
	if (Info->NamedArgs) {
		ml_value_t *Options = ml_map();
		for (; I < Count; ++I) {
			if (ml_typeof(Args[I]) == MLNamesT) {
				ML_NAMES_FOREACH(Args[I], Node) {
					const char *Name = ml_method_name(Node->Value);
					int Index = (intptr_t)stringmap_search(Info->Params, Name);
					if (Index) {
						Frame->Stack[Index - 1] = Args[++I];
					} else {
						ml_map_insert(Options, Node->Value, Args[++I]);
					}
				}
				break;
			}
		}
		Frame->Stack[NumParams] = Options;
		++NumParams;
	} else {
		for (; I < Count; ++I) {
			if (ml_typeof(Args[I]) == MLNamesT) {
				ML_NAMES_FOREACH(Args[I], Node) {
					const char *Name = ml_method_name(Node->Value);
					int Index = (intptr_t)stringmap_search(Info->Params, Name);
					if (Index) {
						Frame->Stack[Index - 1] = Args[++I];
					} else {
						ML_RETURN(ml_error("NameError", "Unknown named parameters %s", Name));
					}
				}
				break;
			}
		}
	}
	for (ml_param_type_t *Type = Closure->ParamTypes; Type; Type = Type->Next) {
		ml_value_t *Value = Frame->Stack[Type->Index];
		if (!ml_is(Value, Type->Type)) {
			ML_RETURN(ml_error("TypeError", "Expected %s not %s", Type->Type->Name, ml_typeof(Value)->Name));
		}
	}
	Frame->Top = Frame->Stack + NumParams;
	Frame->OnError = Info->Return;
	Frame->UpValues = Closure->UpValues;
	Frame->Inst = Info->Entry;
#ifdef USE_ML_SCHEDULER
	ml_scheduler_t scheduler = (ml_scheduler_t)Caller->Context->Values[ML_SCHEDULER_INDEX];
	Frame->Schedule = scheduler(Caller->Context);
#endif
#ifdef DEBUG_VERSION
	Frame->Debugger = Debugger;
	Frame->Revision = Debugger->Revision;
	size_t *Breakpoints = Frame->Breakpoints = Debugger->breakpoints(Debugger, Frame->Source, Info->End);
	Frame->Decls = Info->Decls;
	int LineNo = Frame->Inst->LineNo;
	if (Breakpoints[LineNo / SIZE_BITS] & (1 << LineNo % SIZE_BITS)) {
		return Debugger->run(Debugger, (ml_state_t *)Frame, MLNil);
	}
#else
#ifdef USE_ML_JIT
	if (Info->JITStart) {
		Frame->Base.run = Info->JITStart;
		Frame->Inst = Info->JITEntry;
		Frame->OnError = Info->JITReturn;
	}
#endif
#endif
	ML_CONTINUE(Frame, MLNil);
}

#ifndef DEBUG_VERSION
const char *MLInsts[] = {
	"return", // MLI_RETURN,
	"suspend", // MLI_SUSPEND,
	"resume", // MLI_RESUME,
	"nil", // MLI_NIL,
	"push_nil", // MLI_NIL_PUSH,
	"some", // MLI_SOME,
	"and", // MLI_AND,
	"or", // MLI_OR,
	"push", // MLI_PUSH,
	"with", // MLI_WITH,
	"with_var", // MLI_WITH_VAR,
	"withx", // MLI_WITHX,
	"pop", // MLI_POP,
	"enter", // MLI_ENTER,
	"exit", // MLI_EXIT,
	"loop", // MLI_LOOP,
	"try", // MLI_TRY,
	"catch", // MLI_CATCH,
	"load", // MLI_LOAD,
	"push", // MLI_LOAD_PUSH,
	"var", // MLI_VAR,
	"var_type", // MLI_VAR_TYPE,
	"varx", // MLI_VARX,
	"let", // MLI_LET,
	"leti", // MLI_LETI,
	"letx", // MLI_LETX,
	"for", // MLI_FOR,
	"if", // MLI_IF,
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
	"tuple_set", // MLI_TUPLE_SET,
	"list_new", // MLI_LIST_NEW,
	"list_append", // MLI_LIST_APPEND,
	"map_new", // MLI_MAP_NEW,
	"map_insert", // MLI_MAP_INSERT,
	"closure", // MLI_CLOSURE,
#ifdef USE_GENERICS
	"typed_closure", // MLI_CLOSURE_TYPED,
#endif
	"param_type", // MLI_PARAM_TYPE,
	"partial_new", // MLI_PARTIAL_NEW,
	"partial_set", // MLI_PARTIAL_SET,
	"string_new", // MLI_STRING_NEW,
	"string_add", // MLI_STRING_ADD,
	"string_adds", // MLI_STRING_ADDS,
	"string_end", // MLI_STRING_END
	"resolve" // MLI_RESOLVE
};

const ml_inst_type_t MLInstTypes[] = {
	MLIT_NONE, // MLI_RETURN,
	MLIT_INST, // MLI_SUSPEND,
	MLIT_INST, // MLI_RESUME,
	MLIT_INST, // MLI_NIL,
	MLIT_INST, // MLI_NIL_PUSH,
	MLIT_INST, // MLI_SOME,
	MLIT_INST_INST, // MLI_AND,
	MLIT_INST_INST, // MLI_OR,
	MLIT_INST, // MLI_PUSH,
	MLIT_INST, // MLI_WITH,
	MLIT_INST, // MLI_WITH_VAR,
	MLIT_INST_COUNT, // MLI_WITHX,
	MLIT_INST, // MLI_POP,
	MLIT_INST_COUNT_COUNT, // MLI_ENTER,
	MLIT_INST_COUNT, // MLI_EXIT,
	MLIT_INST, // MLI_LOOP,
	MLIT_INST_INST, // MLI_TRY,
	MLIT_INST_INST_INDEX_CHARS, // MLI_CATCH,
	MLIT_INST_VALUE, // MLI_LOAD,
	MLIT_INST_VALUE, // MLI_LOAD_PUSH,
	MLIT_INST_INDEX, // MLI_VAR,
	MLIT_INST_INDEX, // MLI_VAR_TYPE,
	MLIT_INST_INDEX_COUNT, // MLI_VARX,
	MLIT_INST_INDEX, // MLI_LET,
	MLIT_INST_INDEX, // MLI_LETI,
	MLIT_INST_INDEX_COUNT, // MLI_LETX,
	MLIT_INST, // MLI_FOR,
	MLIT_INST_INST, // MLI_IF,
	MLIT_INST_COUNT, // MLI_NEXT,
	MLIT_INST_INDEX, // MLI_VALUE,
	MLIT_INST_INDEX, // MLI_KEY,
	MLIT_INST_COUNT, // MLI_CALL,
	MLIT_INST_COUNT_VALUE, // MLI_CONST_CALL,
	MLIT_INST, // MLI_ASSIGN,
	MLIT_INST_INDEX, // MLI_LOCAL,
	MLIT_INST_INDEX, // MLI_PUSH_LOCAL,
	MLIT_INST_INDEX, // MLI_UPVALUE,
	MLIT_INST_INDEX_CHARS, // MLI_LOCALX,
	MLIT_INST_COUNT, // MLI_TUPLE_NEW,
	MLIT_INST_INDEX, // MLI_TUPLE_SET,
	MLIT_INST, // MLI_LIST_NEW,
	MLIT_INST, // MLI_LIST_APPEND,
	MLIT_INST, // MLI_MAP_NEW,
	MLIT_INST, // MLI_MAP_INSERT,
	MLIT_INST_CLOSURE, // MLI_CLOSURE,
#ifdef USE_GENERICS
	MLIT_INST_CLOSURE, // MLI_CLOSURE_TYPED,
#endif
	MLIT_INST_INDEX, // MLI_PARAM_TYPE,
	MLIT_INST_COUNT, // MLI_PARTIAL_NEW,
	MLIT_INST_INDEX, // MLI_PARTIAL_SET,
	MLIT_INST, // MLI_STRING_NEW,
	MLIT_INST_COUNT, // MLI_STRING_ADD,
	MLIT_INST_COUNT_CHARS, // MLI_STRING_ADDS,
	MLIT_INST, // MLI_STRING_END
	MLIT_INST_VALUE_VALUE // MLI_RESOLVE
};

#ifdef ML_USE_INST_FNS
extern ml_inst_fn_t MLInstFns[];
#endif

static void ml_inst_process(int Process, ml_inst_t *Source, ml_inst_t *Inst, ml_closure_info_t *Info, int I, int J) {
	if (!Source || (Source->LineNo != Inst->LineNo)) Inst->PotentialBreakpoint = 1;
	if (Inst->LineNo > Info->End) Info->End = Inst->LineNo;
	if (Inst->Processed == Process) return;
	Inst->Processed = Process;
	if (Inst->Opcode == MLI_LOAD && Inst->Params[0].Inst->Opcode == MLI_PUSH) {
		Inst->Opcode = MLI_LOAD_PUSH;
		Inst->Params[0].Inst = Inst->Params[0].Inst->Params[0].Inst;
	} else if (Inst->Opcode == MLI_LOCAL && Inst->Params[0].Inst->Opcode == MLI_PUSH) {
		Inst->Opcode = MLI_LOCAL_PUSH;
		Inst->Params[0].Inst = Inst->Params[0].Inst->Params[0].Inst;
	} else if (Inst->Opcode == MLI_NIL && Inst->Params[0].Inst->Opcode == MLI_PUSH) {
		Inst->Opcode = MLI_NIL_PUSH;
		Inst->Params[0].Inst = Inst->Params[0].Inst->Params[0].Inst;
	}
#ifdef ML_USE_INST_FNS
	Inst->run = MLInstFns[Inst->Opcode];
#endif
	Info->Hash[I] ^= Inst->Opcode;
	Info->Hash[J] ^= (Inst->Opcode << 4);
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_INST_INST:
		ml_inst_process(Process, Inst, Inst->Params[1].Inst, Info, (I + 11) % (SHA256_BLOCK_SIZE - 4), (J + 13) % (SHA256_BLOCK_SIZE - 8));
		break;
	case MLIT_INST_INST_INDEX_CHARS:
		ml_inst_process(Process, Inst, Inst->Params[1].Inst, Info, (I + 11) % (SHA256_BLOCK_SIZE - 4), (J + 13) % (SHA256_BLOCK_SIZE - 8));
		*(int *)(Info->Hash + I) ^= Inst->Params[2].Index;
		if (Inst->Params[3].Ptr) {
			*(long *)(Info->Hash + J) ^= stringmap_hash(Inst->Params[3].Ptr);
		}
		break;
	case MLIT_INST_COUNT_COUNT:
		*(int *)(Info->Hash + I) ^= Inst->Params[1].Count;
		*(int *)(Info->Hash + J) ^= Inst->Params[2].Count;
		break;
	case MLIT_INST_COUNT:
		*(int *)(Info->Hash + I) ^= Inst->Params[1].Count;
		break;
	case MLIT_INST_INDEX:
		*(int *)(Info->Hash + I) ^= Inst->Params[1].Index;
		break;
	case MLIT_INST_VALUE:
		*(long *)(Info->Hash + J) ^= ml_hash(Inst->Params[1].Value);
		break;
	case MLIT_INST_VALUE_VALUE:
		*(long *)(Info->Hash + J) ^= ml_hash(Inst->Params[1].Value);
		*(long *)(Info->Hash + J) ^= ml_hash(Inst->Params[2].Value);
		break;
	case MLIT_INST_INDEX_COUNT:
		*(int *)(Info->Hash + I) ^= Inst->Params[1].Index;
		*(int *)(Info->Hash + J) ^= Inst->Params[2].Count;
		break;
	case MLIT_INST_INDEX_CHARS:
		*(int *)(Info->Hash + I) ^= Inst->Params[1].Index;
		*(long *)(Info->Hash + J) ^= stringmap_hash(Inst->Params[2].Ptr);
		break;
	case MLIT_INST_COUNT_VALUE:
		*(int *)(Info->Hash + I) ^= Inst->Params[1].Count;
		*(long *)(Info->Hash + J) ^= ml_hash(Inst->Params[2].Value);
		break;
	case MLIT_INST_COUNT_CHARS:
		*(int *)(Info->Hash + I) ^= Inst->Params[1].Count;
		*(long *)(Info->Hash + J) ^= stringmap_hash(Inst->Params[2].Ptr);
		break;
	case MLIT_INST_CLOSURE: {
		ml_closure_info_t *Info = Inst->Params[1].ClosureInfo;
		*(long *)(Info->Hash + J) ^= *(long *)(Info->Hash + J);
		for (int N = 0; N < Info->NumUpValues; ++N) {
			int Index = Inst->Params[2 + N].Index;
			*(int *)(Info->Hash + I) ^= (Index << N);
		}
		break;
	default:
		break;
	}
	}
	if (Inst->Opcode != MLI_RETURN) {
		ml_inst_process(Process, Inst, Inst->Params[0].Inst, Info, (I + 3) % (SHA256_BLOCK_SIZE - 4), (J + 7) % (SHA256_BLOCK_SIZE - 8));
	}
}

void ml_closure_info_finish(ml_closure_info_t *Info) {
	ml_inst_process(!Info->Entry->Processed, NULL, Info->Entry, Info, 0, 0);
#ifdef USE_ML_JIT
	ml_bytecode_jit(Info);
#endif
}

void ml_closure_sha256(ml_value_t *Value, unsigned char Hash[SHA256_BLOCK_SIZE]) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	memcpy(Hash, Closure->Info->Hash, SHA256_BLOCK_SIZE);
	for (int I = 0; I < Closure->Info->NumUpValues; ++I) {
		long ValueHash = ml_hash(Closure->UpValues[I]);
		*(long *)(Hash + (I % 16)) ^= ValueHash;
	}
}

static long ml_closure_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	long Hash = 0;
	long *P = (long *)Closure->Info->Hash;
	long *Q = (long *)(Closure->Info->Hash + SHA256_BLOCK_SIZE);
	while (P < Q) Hash ^= *P++;
	for (int I = 0; I < Closure->Info->NumUpValues; ++I) {
		Hash ^= ml_hash_chain(Closure->UpValues[I], Chain) << I;
	}
	return Hash;
}

static void ML_TYPED_FN(ml_iterate, DEBUG_TYPE(Closure), ml_state_t *Frame, ml_value_t *Closure) {
	return ml_closure_call(Frame, Closure, 0, NULL);
}

ML_TYPE(MLClosureT, (MLFunctionT, MLIteratableT), "closure",
	.hash = ml_closure_hash,
	.call = ml_closure_call
);

ML_METHOD(MLStringOfMethod, MLClosureT) {
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	return ml_string_format("<%s:%d>", Closure->Info->Source, Closure->Info->LineNo);
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

static void ml_closure_find_labels(int Process, ml_inst_t *Inst, unsigned int *Labels, int NonLinear) {
	if (Inst->Processed == Process) {
		Inst->Label = ++*Labels;
		return;
	}
	if (NonLinear) Inst->Label = ++*Labels;
	Inst->Processed = Process;
	if (MLInstTypes[Inst->Opcode] != MLIT_NONE) {
		ml_closure_find_labels(Process, Inst->Params[0].Inst, Labels, 0);
	}
	if (MLInstTypes[Inst->Opcode] == MLIT_INST_INST) {
		ml_closure_find_labels(Process, Inst->Params[1].Inst, Labels, 1);
	}
	if (MLInstTypes[Inst->Opcode] == MLIT_INST_INST_INDEX_CHARS) {
		ml_closure_find_labels(Process, Inst->Params[1].Inst, Labels, 1);
	}
}

static void ml_closure_inst_list(int Process, ml_inst_t *Inst, ml_stringbuffer_t *Buffer) {
	if (Inst->Processed == Process) {
		ml_stringbuffer_addf(Buffer, "\t %3d jump L%d\n", Inst->LineNo, Inst->Label);
		return;
	}
	if (Inst->Label) ml_stringbuffer_addf(Buffer, "L%d:\n", Inst->Label);
	Inst->Processed = Process;
	ml_stringbuffer_addf(Buffer, "\t%s%3d %s", Inst->PotentialBreakpoint ? "*" : " ", Inst->LineNo, MLInsts[Inst->Opcode]);
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_INST_INST:
		ml_stringbuffer_addf(Buffer, " ->L%d", Inst->Params[1].Inst->Label);
		break;
	case MLIT_INST_INST_INDEX_CHARS:
		ml_stringbuffer_addf(Buffer, " %s ->L%d, %d", Inst->Params[3].Ptr, Inst->Params[1].Inst->Label, Inst->Params[2].Index);
		break;
	case MLIT_INST_COUNT_COUNT:
		ml_stringbuffer_addf(Buffer, " %d, %d", Inst->Params[1].Count, Inst->Params[2].Count);
		break;
	case MLIT_INST_COUNT:
		ml_stringbuffer_addf(Buffer, " %d", Inst->Params[1].Count);
		break;
	case MLIT_INST_INDEX:
		ml_stringbuffer_addf(Buffer, " %d", Inst->Params[1].Index);
		break;
	case MLIT_INST_VALUE: {
		ml_value_t *Value = Inst->Params[1].Value;
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
		} else {
			ml_stringbuffer_addf(Buffer, " %s", ml_typeof(Value)->Name);
		}
		break;
	}
	case MLIT_INST_VALUE_VALUE: {
		ml_value_t *Value = Inst->Params[1].Value;
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
		} else {
			ml_stringbuffer_addf(Buffer, " %s", ml_typeof(Value)->Name);
		}
		Value = Inst->Params[2].Value;
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
		} else {
			ml_stringbuffer_addf(Buffer, " %s", ml_typeof(Value)->Name);
		}
		break;
	}
	case MLIT_INST_INDEX_COUNT:
		ml_stringbuffer_addf(Buffer, " %d, %d", Inst->Params[1].Index, Inst->Params[2].Count);
		break;
	case MLIT_INST_INDEX_CHARS:
		ml_stringbuffer_addf(Buffer, " %d, %s", Inst->Params[1].Index, Inst->Params[2].Ptr);
		break;
	case MLIT_INST_COUNT_VALUE: {
		ml_stringbuffer_addf(Buffer, " %d, ", Inst->Params[1].Count);
		ml_value_t *Value = Inst->Params[2].Value;
		if (ml_is(Value, MLStringT)) {
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
		} else if (ml_is(Value, MLNumberT)) {
			ml_stringbuffer_append(Buffer, Value);
		} else if (ml_typeof(Value) == MLMethodT) {
			ml_stringbuffer_addf(Buffer, ":%s", ml_method_name(Value));
		} else {
			ml_stringbuffer_addf(Buffer, "%s", ml_typeof(Value)->Name);
		}
		break;
	}
	case MLIT_INST_COUNT_CHARS:
		ml_stringbuffer_addf(Buffer, " %d, %s", Inst->Params[1].Count, Inst->Params[2].Ptr);
		break;
	case MLIT_INST_CLOSURE: {
		ml_stringbuffer_addf(Buffer, " closure");
		ml_closure_info_t *Info = Inst->Params[1].ClosureInfo;
		for (int N = 0; N < Info->NumUpValues; ++N) {
			ml_stringbuffer_addf(Buffer, ", %d", Inst->Params[2 + N].Index);
		}
		break;
	default:
		break;
	}
	}
	ml_stringbuffer_add(Buffer, "\n", 1);
	if (Inst->Opcode != MLI_RETURN) {
		ml_closure_inst_list(Process, Inst->Params[0].Inst, Buffer);
	}
	if (MLInstTypes[Inst->Opcode] == MLIT_INST_INST) {
		ml_closure_inst_list(Process, Inst->Params[1].Inst, Buffer);
	}
	if (MLInstTypes[Inst->Opcode] == MLIT_INST_INST_INDEX_CHARS) {
		ml_closure_inst_list(Process, Inst->Params[1].Inst, Buffer);
	}
}

ML_METHOD("info", MLClosureT) {
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	ml_closure_info_t *Info = Closure->Info;
	ml_value_t *Result = ml_map();
	ml_map_insert(Result, ml_cstring("Source"), ml_cstring(Info->Source));
	ml_map_insert(Result, ml_cstring("Start"), ml_integer(Info->LineNo));
	ml_map_insert(Result, ml_cstring("End"), ml_integer(Info->End));
	ml_map_insert(Result, ml_cstring("Size"), ml_integer(Info->FrameSize));
	return Result;
}

ML_METHOD("list", MLClosureT) {
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	ml_closure_info_t *Info = Closure->Info;
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	unsigned int Labels = 0;
	ml_closure_find_labels(!Info->Entry->Processed, Info->Entry, &Labels, 0);
	ml_closure_inst_list(!Info->Entry->Processed, Info->Entry, Buffer);
	return ml_stringbuffer_value(Buffer);
}

ML_METHOD("!!", MLClosureT, MLListT) {
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	ml_list_t *ArgsList = (ml_list_t *)Args[1];
	int NumUpValues = Closure->Info->NumUpValues + Closure->PartialCount;
	ml_closure_t *Partial = xnew(ml_closure_t, NumUpValues + ArgsList->Length, ml_value_t *);
	memcpy(Partial, Closure, sizeof(ml_closure_t) + NumUpValues * sizeof(ml_value_t *));
	Partial->PartialCount += ArgsList->Length;
	ml_value_t **Arg = Partial->UpValues + NumUpValues;
	ML_LIST_FOREACH(ArgsList, Node) *Arg++ = Node->Value;
	return (ml_value_t *)Partial;
}

#ifdef USE_ML_JIT

ML_METHOD("jit", MLClosureT) {
	ml_closure_info_t *Info = ((ml_closure_t *)Args[0])->Info;
	if (!Info->JITEntry) ml_bytecode_jit(Info);
	return Args[0];
}

#endif

#ifdef USE_ML_CBOR_BYTECODE

static void ml_cbor_closure_write_int(int Value0, ml_stringbuffer_t *Buffer) {
	unsigned int Value = (unsigned int)Value0;
	uint8_t Bytes[(sizeof(unsigned int) * 8 + 7) / 7];
	for (int I = 0;; ++I) {
		if (Value < 0x80) {
			Bytes[I] = Value;
			ml_stringbuffer_add(Buffer, Bytes, I + 1);
			break;
		} else {
			Bytes[I] = Value | 0x80;
			Value >>= 7;
		}
	}
}

static void ml_cbor_closure_write_string(const char *Value, ml_stringbuffer_t *Buffer) {
	int Length = strlen(Value);
	ml_cbor_closure_write_int(Length, Buffer);
	ml_stringbuffer_add(Buffer, (void *)Value, Length);
}

static void ml_cbor_closure_find_refs(int Process, ml_inst_t *Inst, stringmap_t *Refs) {
	if (Inst->Processed == Process) {
		char *InstName = GC_MALLOC_ATOMIC(3 * sizeof(uintptr_t) * CHAR_BIT / 8 + 2);
		sprintf(InstName, "%" PRIxPTR, (uintptr_t)Inst);
		char **Slot = (char **)stringmap_slot(Refs, InstName);
		if (!Slot[0]) Slot[0] = (char *)NULL + Refs->Size;
		return;
	}
	Inst->Processed = Process;
	if (MLInstTypes[Inst->Opcode] == MLIT_INST_INST) {
		ml_cbor_closure_find_refs(Process, Inst->Params[1].Inst, Refs);
	}
	if (MLInstTypes[Inst->Opcode] != MLIT_NONE) {
		ml_cbor_closure_find_refs(Process, Inst->Params[0].Inst, Refs);
	}
}

#define MLI_REFER 0xFF
#define MLI_LABEL 0xFE

static void ml_cbor_closure_write_inst(int Process, ml_inst_t *Inst, stringmap_t *Refs, ml_stringbuffer_t *Buffer) {
	char InstName[3 * sizeof(uintptr_t) * CHAR_BIT / 8 + 2];
	sprintf(InstName, "%" PRIxPTR, (uintptr_t)Inst);
	int Index = (char *)stringmap_search(Refs, InstName) - (char *)NULL;
	if (Inst->Processed == Process) {
		ml_stringbuffer_add(Buffer, (char[]){MLI_REFER}, 1);
		ml_cbor_closure_write_int(Index - 1, Buffer);
		return;
	}
	Inst->Processed = Process;
	if (Index) {
		ml_stringbuffer_add(Buffer, (char[]){MLI_LABEL}, 1);
		ml_cbor_closure_write_int(Index - 1, Buffer);
	}
	ml_stringbuffer_add(Buffer, (char[]){Inst->Opcode}, 1);
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_INST_INST:
		ml_cbor_closure_write_inst(Process, Inst->Params[1].Inst, Refs, Buffer);
		break;
	case MLIT_INST_COUNT_COUNT:
		ml_cbor_closure_write_int(Inst->Params[1].Count, Buffer);
		ml_cbor_closure_write_int(Inst->Params[2].Count, Buffer);
		break;
	case MLIT_INST_COUNT:
		ml_cbor_closure_write_int(Inst->Params[1].Count, Buffer);
		break;
	case MLIT_INST_INDEX:
		ml_cbor_closure_write_int(Inst->Params[1].Index, Buffer);
		break;
	case MLIT_INST_VALUE:
		ml_cbor_write(Inst->Params[1].Value, Buffer, ml_stringbuffer_add);
		break;
	case MLIT_INST_INDEX_COUNT:
		ml_cbor_closure_write_int(Inst->Params[1].Index, Buffer);
		ml_cbor_closure_write_int(Inst->Params[2].Count, Buffer);
		break;
	case MLIT_INST_COUNT_VALUE:
		ml_cbor_closure_write_int(Inst->Params[1].Count, Buffer);
		ml_cbor_write(Inst->Params[2].Value, Buffer, ml_stringbuffer_add);
		break;
	case MLIT_INST_CLOSURE: {
		ml_closure_info_t *Info = Inst->Params[1].ClosureInfo;
		for (int N = 0; N < Info->NumUpValues; ++N) {
			Inst->Params[2 + N].Index;
		}
		break;
	default:
		break;
	}
	}
	if (MLInstTypes[Inst->Opcode] != MLIT_NONE) {
		ml_cbor_closure_write_inst(Process, Inst->Params[0].Inst, Refs, Buffer);
	}
}

/*
struct ml_closure_info_t {
	ml_inst_t *Entry, *Return;
	const char *Source;
	ml_decl_t *Decls;
	stringmap_t Params[1];
	int End, FrameSize;
	int NumParams, NumUpValues;
	int ExtraArgs, NamedArgs;
	unsigned char Hash[SHA256_BLOCK_SIZE];
};
 */

static int ml_cbor_write_closure_param(const char *Name, void *Value, ml_stringbuffer_t *Buffer) {
	ml_cbor_closure_write_string(Name, Buffer);
	ml_cbor_closure_write_int((intptr_t)Value, Buffer);
	return 0;
}

void ml_cbor_write_closure(ml_closure_t *Closure, ml_stringbuffer_t *Buffer) {
	ml_closure_info_t *Info = Closure->Info;
	stringmap_t Refs[1] = {STRINGMAP_INIT};
	char *InstName = GC_MALLOC_ATOMIC(3 * sizeof(uintptr_t) * CHAR_BIT / 8 + 2);
	sprintf(InstName, "%" PRIxPTR, (uintptr_t)Info->Return);
	char **Slot = (char **)stringmap_slot(Refs, InstName);
	Slot[0] = (char *)NULL + Refs->Size;
	Info->Return->Processed = !Info->Entry->Processed;
	ml_cbor_closure_find_refs(!Info->Entry->Processed, Info->Entry, Refs);
	ml_cbor_closure_write_int(Refs->Size, Buffer);
	ml_cbor_closure_write_inst(!Info->Entry->Processed, Info->Entry, Refs, Buffer);
	ml_cbor_closure_write_string(Info->Source, Buffer);
	// TODO: Write Info->Decls
	ml_cbor_closure_write_int(Info->Params->Size, Buffer);
	stringmap_foreach(Info->Params, Buffer, ml_cbor_write_closure_param);
	ml_cbor_closure_write_int(Info->End, Buffer);
	ml_cbor_closure_write_int(Info->FrameSize, Buffer);
	ml_cbor_closure_write_int(Info->NumParams, Buffer);
	ml_cbor_closure_write_int(Info->NumUpValues, Buffer);
	ml_cbor_closure_write_int(Info->ExtraArgs, Buffer);
	ml_cbor_closure_write_int(Info->NamedArgs, Buffer);
	ml_cbor_closure_write_int(Closure->PartialCount, Buffer);
	for (int I = 0; I < Info->NumUpValues; ++I) {
		ml_value_t *UpValue = Closure->UpValues[I];
		UpValue = ml_deref(UpValue);
		ml_cbor_write(UpValue, Buffer, ml_stringbuffer_add);
	}
}

ml_value_t *ML_TYPED_FN(ml_cbor_write, MLClosureT, ml_closure_t *Closure, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_tag(Data, WriteFn, 36); // TODO: Pick correct tag
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_cbor_write_closure(Closure, Buffer);
	ml_cbor_write_bytes(Data, WriteFn, Buffer->Length);
	ml_stringbuffer_foreach(Buffer, Data, WriteFn);
	return NULL;
}

typedef struct {
	ml_value_t *Error;
	const uint8_t *Data;
	ml_inst_t **Refs;
	size_t Length, NumRefs;
	jmp_buf OnError;
} ml_cbor_closure_reader_t;

#define ML_CBOR_CLOSURE_ERROR { \
	Reader->Error = ml_error("CborError", "Invalid closure data (%d)", __LINE__); \
	longjmp(Reader->OnError, 1); \
}

static int ml_cbor_closure_read_int(ml_cbor_closure_reader_t *Reader) {
	unsigned int Value = 0;
	int Shift = 0;
	const uint8_t *Data = Reader->Data;
	size_t Length = Reader->Length;
	for (;;) {
		if (!Length) ML_CBOR_CLOSURE_ERROR;
		uint8_t Byte = *Data++;
		--Length;
		Value += (Byte & 0x7F) << Shift;
		if (Byte < 0x80) break;
		Shift += 7;
	}
	Reader->Data = Data;
	Reader->Length = Length;
	return (int)Value;
}

static const char *ml_cbor_closure_read_string(ml_cbor_closure_reader_t *Reader) {
	int Length = ml_cbor_closure_read_int(Reader);
	if (Reader->Length < Length) ML_CBOR_CLOSURE_ERROR;
	char *Value = GC_MALLOC_ATOMIC(Length + 1);
	memcpy(Value, Reader->Data, Length);
	Value[Length] = 0;
	Reader->Data += Length;
	Reader->Length -= Length;
	return Value;
}

static ml_value_t *ml_cbor_closure_read_value(ml_cbor_closure_reader_t *Reader) {
	ml_cbor_t Cbor = {Reader->Data, Reader->Length};
	ml_cbor_result_t Result = ml_from_cbor_extra(Cbor, NULL, NULL);
	Reader->Data += (Reader->Length - Result.Extra);
	Reader->Length = Result.Extra;
	return Result.Value;
}

static void ml_cbor_closure_read_inst(ml_cbor_closure_reader_t *Reader, ml_inst_t **Slot) {
	if (!Reader->Length) ML_CBOR_CLOSURE_ERROR;
	ml_opcode_t Opcode = *Reader->Data++;
	--Reader->Length;
	if (Opcode == MLI_REFER) {
		int Index = ml_cbor_closure_read_int(Reader);
		if (Index < 0 || Index >= Reader->NumRefs) ML_CBOR_CLOSURE_ERROR;
		ml_inst_t *Inst = Reader->Refs[Index];
		if (!Inst) ML_CBOR_CLOSURE_ERROR;
		Slot[0] = Inst;
	} else if (Opcode == MLI_LABEL) {
		int Index = ml_cbor_closure_read_int(Reader);
		if (Index < 0 || Index >= Reader->NumRefs) ML_CBOR_CLOSURE_ERROR;
		if (Reader->Refs[Index]) ML_CBOR_CLOSURE_ERROR;
		ml_cbor_closure_read_inst(Reader, &Reader->Refs[Index]);
		Slot[0] = Reader->Refs[Index];
	} else if ((Opcode >= MLI_RETURN) && (Opcode <= MLI_PARTIAL_SET)) {
		switch (MLInstTypes[Opcode]) {
		case MLIT_NONE: {
			ml_inst_t *Inst = Slot[0] = xnew(ml_inst_t, 0, ml_param_t);
			Inst->Opcode = Opcode;
			break;
		}
		case MLIT_INST: {
			ml_inst_t *Inst = Slot[0] = xnew(ml_inst_t, 1, ml_param_t);
			Inst->Opcode = Opcode;
			ml_cbor_closure_read_inst(Reader, &Inst->Params[0].Inst);
			break;
		}
		case MLIT_INST_INST: {
			ml_inst_t *Inst = Slot[0] = xnew(ml_inst_t, 2, ml_param_t);
			Inst->Opcode = Opcode;
			ml_cbor_closure_read_inst(Reader, &Inst->Params[1].Inst);
			ml_cbor_closure_read_inst(Reader, &Inst->Params[0].Inst);
			break;
		}
		case MLIT_INST_COUNT_COUNT: {
			ml_inst_t *Inst = Slot[0] = xnew(ml_inst_t, 3, ml_param_t);
			Inst->Opcode = Opcode;
			Inst->Params[1].Count = ml_cbor_closure_read_int(Reader);
			Inst->Params[2].Count = ml_cbor_closure_read_int(Reader);
			ml_cbor_closure_read_inst(Reader, &Inst->Params[0].Inst);
			break;
		}
		case MLIT_INST_COUNT: {
			ml_inst_t *Inst = Slot[0] = xnew(ml_inst_t, 2, ml_param_t);
			Inst->Opcode = Opcode;
			Inst->Params[1].Count = ml_cbor_closure_read_int(Reader);
			ml_cbor_closure_read_inst(Reader, &Inst->Params[0].Inst);
			break;
		}
		case MLIT_INST_INDEX: {
			ml_inst_t *Inst = Slot[0] = xnew(ml_inst_t, 2, ml_param_t);
			Inst->Opcode = Opcode;
			Inst->Params[1].Index = ml_cbor_closure_read_int(Reader);
			ml_cbor_closure_read_inst(Reader, &Inst->Params[0].Inst);
			break;
		}
		case MLIT_INST_VALUE: {
			ml_inst_t *Inst = Slot[0] = xnew(ml_inst_t, 2, ml_param_t);
			Inst->Opcode = Opcode;
			Inst->Params[1].Value = ml_cbor_closure_read_value(Reader);
			ml_cbor_closure_read_inst(Reader, &Inst->Params[0].Inst);
			break;
		}
		case MLIT_INST_INDEX_COUNT: {
			ml_inst_t *Inst = Slot[0] = xnew(ml_inst_t, 3, ml_param_t);
			Inst->Opcode = Opcode;
			Inst->Params[1].Index = ml_cbor_closure_read_int(Reader);
			Inst->Params[2].Count = ml_cbor_closure_read_int(Reader);
			ml_cbor_closure_read_inst(Reader, &Inst->Params[0].Inst);
			break;
		}
		case MLIT_INST_COUNT_VALUE: {
			ml_inst_t *Inst = Slot[0] = xnew(ml_inst_t, 3, ml_param_t);
			Inst->Opcode = Opcode;
			Inst->Params[1].Count = ml_cbor_closure_read_int(Reader);
			Inst->Params[2].Value = ml_cbor_closure_read_value(Reader);
			ml_cbor_closure_read_inst(Reader, &Inst->Params[0].Inst);
			break;
		}
		case MLIT_INST_CLOSURE:  {
			ML_CBOR_CLOSURE_ERROR;
		}
		}
	}
}

ml_value_t *ml_cbor_read_closure(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_TYPE(0, MLBufferT);
	ml_buffer_t *Buffer = (ml_buffer_t *)Args[0];
	ml_cbor_closure_reader_t Reader[1];
	Reader->Data = (uint8_t *)Buffer->Address;
	Reader->Length = Buffer->Size;
	if (setjmp(Reader->OnError)) return Reader->Error;
	Reader->NumRefs = ml_cbor_closure_read_int(Reader);
	if (Reader->NumRefs < 1) return ml_error("CborError", "Invalid closure data (%d)", __LINE__);
	Reader->Refs = anew(ml_inst_t *, Reader->NumRefs);
	ml_closure_info_t *Info = new(ml_closure_info_t);
	ml_cbor_closure_read_inst(Reader, &Info->Entry);
	Info->Return = Reader->Refs[0];
	Info->Source = ml_cbor_closure_read_string(Reader);
	// TODO: Read Info->Decls
	int NumParams = ml_cbor_closure_read_int(Reader);
	for (int I = 0; I < NumParams; ++I) {
		const char *Name = ml_cbor_closure_read_string(Reader);
		intptr_t Index = ml_cbor_closure_read_int(Reader);
		stringmap_insert(Info->Params, Name, (void *)Index);
	}
	Info->End = ml_cbor_closure_read_int(Reader);
	Info->FrameSize = ml_cbor_closure_read_int(Reader);
	Info->NumParams = ml_cbor_closure_read_int(Reader);
	Info->NumUpValues = ml_cbor_closure_read_int(Reader);
	Info->ExtraArgs = ml_cbor_closure_read_int(Reader);
	Info->NamedArgs = ml_cbor_closure_read_int(Reader);
	ml_closure_t *Closure = new(ml_closure_t);
	Closure->Type = MLClosureT;
	Closure->Info = Info;
	Closure->PartialCount = ml_cbor_closure_read_int(Reader);
	for (int I = 0; I < Closure->Info->NumUpValues; ++I) {
		Closure->UpValues[I] = ml_cbor_closure_read_value(Reader);
	}
	return (ml_value_t *)Closure;
}

#endif

#define DEBUG_VERSION
#include "ml_bytecode.c"
#undef DEBUG_VERSION

void ml_bytecode_init() {
#ifdef USE_GENERICS
	stringmap_insert(MLClosureT->Exports, "T", MLAnyT);
	stringmap_insert(MLClosureT->Exports, "[]", ml_cfunction(MLClosureT, ml_type_generic_fn));
#endif
#include "ml_bytecode_init.c"
}
#endif

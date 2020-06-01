#ifndef DEBUG_VERSION
#include "ml_macros.h"
#include "stringmap.h"
#include <gc.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include "ml_bytecode.h"
#include "ml_debugger.h"
#endif

#ifdef DEBUG_VERSION
#undef DEBUG_STRUCT
#undef DEBUG_FUNC
#undef DEBUG_TYPE
#define DEBUG_STRUCT(X) ml_ ## X ## _debug_t
#define DEBUG_FUNC(X) ml_ ## X ## _debug
#define DEBUG_TYPE(X) ML ## X ## DebugT
#else
#define DEBUG_STRUCT(X) ml_ ## X ## _t
#define DEBUG_FUNC(X) ml_ ## X
#define DEBUG_TYPE(X) ML ## X ## T
#endif

typedef struct DEBUG_STRUCT(frame) DEBUG_STRUCT(frame);

struct DEBUG_STRUCT(frame) {
	ml_state_t Base;
	const char *Source;
	ml_inst_t *Inst;
	ml_value_t **Top;
	ml_inst_t *OnError;
	ml_value_t **UpValues;
#ifdef DEBUG_VERSION
	ml_debugger_t *Debugger;
	size_t *Breakpoints;
	ml_decl_t *Decls;
	size_t Revision;
	size_t Reentry;
#endif
	ml_value_t *Stack[];
};

static void DEBUG_FUNC(continuation_call)(ml_state_t *Caller, ml_state_t *State, int Count, ml_value_t **Args) {
	return State->run(State, Count ? Args[0] : MLNil);
}

ml_type_t DEBUG_TYPE(Continuation)[1] = {{
	MLTypeT,
	MLStateT, "continuation",
	ml_default_hash,
	(void *)DEBUG_FUNC(continuation_call),
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

static int ML_TYPED_FN(ml_debugger_check, DEBUG_TYPE(Continuation), DEBUG_STRUCT(frame) *Frame) {
	return 1;
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

static void ML_TYPED_FN(ml_iter_value, DEBUG_TYPE(Suspension), ml_state_t *Caller, DEBUG_STRUCT(frame) *Suspension) {
	ML_RETURN(Suspension->Top[-1]);
}

static void ML_TYPED_FN(ml_iter_key, DEBUG_TYPE(Suspension), ml_state_t *Caller, DEBUG_STRUCT(frame) *Suspension) {
	ML_RETURN(Suspension->Top[-2]);
}

static void ML_TYPED_FN(ml_iter_next, DEBUG_TYPE(Suspension), ml_state_t *Caller, DEBUG_STRUCT(frame) *Suspension) {
	Suspension->Base.Type = DEBUG_TYPE(Continuation);
	Suspension->Top[-2] = Suspension->Top[-1];
	--Suspension->Top;
	Suspension->Base.Caller = Caller;
	Suspension->Base.Context = Caller->Context;
	ML_CONTINUE(Suspension, MLNil);
}

static void DEBUG_FUNC(suspension_call)(ml_state_t *Caller, ml_state_t *State, int Count, ml_value_t **Args) {
	State->Caller = Caller;
	return State->run(State, Count ? Args[0] : MLNil);
}

ml_type_t DEBUG_TYPE(Suspension)[1] = {{
	MLTypeT,
	MLFunctionT, "suspension",
	ml_default_hash,
	(void *)DEBUG_FUNC(suspension_call),
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

#ifndef DEBUG_VERSION

#define ERROR() \
	Inst = Frame->OnError; \
	goto *Labels[Inst->Opcode]

#define ADVANCE(N) \
	Inst = Inst->Params[N].Inst; \
	goto *Labels[Inst->Opcode]

#define ERROR_CHECK(VALUE) if (VALUE->Type == MLErrorT) { \
	ml_error_trace_add(VALUE, (ml_source_t){Frame->Source, Inst->LineNo}); \
	Result = VALUE; \
	ERROR(); \
}

static ml_value_t ClosureEntry[1] = {{MLAnyT}};

#else

#undef ERROR
#undef ADVANCE

#define ERROR() \
	Inst = Frame->OnError; \
	goto DO_DEBUG_ERROR

#define ADVANCE(N) \
	Inst = Inst->Params[N].Inst; \
	goto DO_DEBUG_ADVANCE

#endif

static void DEBUG_FUNC(frame_run)(DEBUG_STRUCT(frame) *Frame, ml_value_t *Result) {
	static void *Labels[] = {
		[MLI_RETURN] = &&DO_RETURN,
		[MLI_SUSPEND] = &&DO_SUSPEND,
		[MLI_RESUME] = &&DO_RESUME,
		[MLI_NIL] = &&DO_NIL,
		[MLI_SOME] = &&DO_SOME,
		[MLI_IF] = &&DO_IF,
		[MLI_IF_VAR] = &&DO_IF_VAR,
		[MLI_IF_LET] = &&DO_IF_LET,
		[MLI_ELSE] = &&DO_ELSE,
		[MLI_PUSH] = &&DO_PUSH,
		[MLI_WITH] = &&DO_WITH,
		[MLI_WITHX] = &&DO_WITHX,
		[MLI_POP] = &&DO_POP,
		[MLI_ENTER] = &&DO_ENTER,
		[MLI_EXIT] = &&DO_EXIT,
		[MLI_LOOP] = &&DO_LOOP,
		[MLI_TRY] = &&DO_TRY,
		[MLI_CATCH] = &&DO_CATCH,
		[MLI_LOAD] = &&DO_LOAD,
		[MLI_VAR] = &&DO_VAR,
		[MLI_VARX] = &&DO_VARX,
		[MLI_LET] = &&DO_LET,
		[MLI_LETX] = &&DO_LETX,
		[MLI_FOR] = &&DO_FOR,
		[MLI_NEXT] = &&DO_NEXT,
		[MLI_VALUE] = &&DO_VALUE,
		[MLI_KEY] = &&DO_KEY,
		[MLI_CALL] = &&DO_CALL,
		[MLI_CONST_CALL] = &&DO_CONST_CALL,
		[MLI_ASSIGN] = &&DO_ASSIGN,
		[MLI_LOCAL] = &&DO_LOCAL,
		[MLI_TUPLE_NEW] = &&DO_TUPLE_NEW,
		[MLI_TUPLE_SET] = &&DO_TUPLE_SET,
		[MLI_LIST_NEW] = &&DO_LIST_NEW,
		[MLI_LIST_APPEND] = &&DO_LIST_APPEND,
		[MLI_MAP_NEW] = &&DO_MAP_NEW,
		[MLI_MAP_INSERT] = &&DO_MAP_INSERT,
		[MLI_CLOSURE] = &&DO_CLOSURE,
		[MLI_PARTIAL_NEW] = &&DO_PARTIAL_NEW,
		[MLI_PARTIAL_SET] = &&DO_PARTIAL_SET
	};
	ml_inst_t *Inst = Frame->Inst;
	ml_value_t **Top = Frame->Top;
	if (Result->Type == MLErrorT) {
		ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
		Inst = Frame->OnError;
#ifdef DEBUG_VERSION
		ml_debugger_t *Debugger = Frame->Debugger;
		if (Debugger->BreakOnError && --Frame->Reentry) goto DO_BREAKPOINT;
	} else {
		goto DO_DEBUG_ADVANCE;
#endif
	}
	goto *Labels[Inst->Opcode];

	DO_RETURN: {
		ML_CONTINUE(Frame->Base.Caller, Result);
	}
	DO_SUSPEND: {
		Frame->Base.Type = DEBUG_TYPE(Suspension);
		Frame->Inst = Inst->Params[0].Inst;
		Frame->Top = Top;
		ML_CONTINUE(Frame->Base.Caller, (ml_value_t *)Frame);
	}
	DO_RESUME: {
		*--Top = 0;
		ADVANCE(0);
	}
	DO_NIL: {
		Result = MLNil;
		ADVANCE(0);
	}
	DO_SOME: {
		Result = MLSome;
		ADVANCE(0);
	}
	DO_IF: {
		Result = Result->Type->deref(Result);
		if (Result->Type == MLErrorT) {
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		} else if (Result == MLNil) {
			ADVANCE(0);
		} else {
			ADVANCE(1);
		}
	}
	DO_IF_VAR: {
		Result = Result->Type->deref(Result);
		if (Result->Type == MLErrorT) {
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		} else if (Result == MLNil) {
			ADVANCE(0);
		} else {
			ml_reference_t *Local = xnew(ml_reference_t, 1, ml_value_t *);
			Local->Type = MLReferenceT;
			Local->Address = Local->Value;
			Local->Value[0] = Result;
			*Top++ = (ml_value_t *)Local;
#ifdef DEBUG_VERSION
			Frame->Decls = Inst->Params[2].Decls;
#endif
			ADVANCE(1);
		}
	}
	DO_IF_LET: {
		Result = Result->Type->deref(Result);
		if (Result->Type == MLErrorT) {
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		} else if (Result == MLNil) {
			ADVANCE(0);
		} else {
			*Top++ = Result;
#ifdef DEBUG_VERSION
			Frame->Decls = Inst->Params[2].Decls;
#endif
			ADVANCE(1);
		}
	}
	DO_ELSE: {
		Result = Result->Type->deref(Result);
		if (Result->Type == MLErrorT) {
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		} else if (Result != MLNil) {
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
	DO_WITHX: {
		if (Result->Type != MLTupleT) {
			Result = ml_error("TypeError", "Can only unpack tuples");
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		}
		ml_tuple_t *Tuple = (ml_tuple_t *)Result;
		if (Tuple->Size < Inst->Params[2].Count) {
			Result = ml_error("ValueError", "Tuple has too few values (%d < %d)", Tuple->Size, Inst->Params[2].Count);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		}
		for (int I = 0; I < Inst->Params[1].Count; ++I) {
			*Top++ = Tuple->Values[I];
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
			ml_reference_t *Local = xnew(ml_reference_t, 1, ml_value_t *);
			Local->Type = MLReferenceT;
			Local->Address = Local->Value;
			Local->Value[0] = MLNil;
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
		if (Result->Type != MLErrorT) {
			Result = ml_error("InternalError", "expected error value, not %s", Result->Type->Name);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		}
		Result->Type = MLErrorValueT;
		ml_value_t **Old = Frame->Stack + Inst->Params[1].Index;
		while (Top > Old) *--Top = 0;
		*Top++ = Result;
#ifdef DEBUG_VERSION
		Frame->Decls = Inst->Params[2].Decls;
#endif
		ADVANCE(0);
	}
	DO_LOAD: {
		Result = Inst->Params[1].Value;
		ADVANCE(0);
	}
	DO_VAR: {
		Result = Result->Type->deref(Result);
		ERROR_CHECK(Result);
		ml_reference_t *Local = (ml_reference_t *)Top[Inst->Params[1].Index];
		Local->Value[0] = Result;
		ADVANCE(0);
	}
	DO_VARX: {
		Result = Result->Type->deref(Result);
		ERROR_CHECK(Result);
		if (Result->Type != MLTupleT) {
			Result = ml_error("TypeError", "Can only unpack tuples");
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		}
		ml_tuple_t *Tuple = (ml_tuple_t *)Result;
		if (Tuple->Size < Inst->Params[2].Count) {
			Result = ml_error("ValueError", "Tuple has too few values (%d < %d)", Tuple->Size, Inst->Params[2].Count);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		}
		ml_value_t **Base = Top + Inst->Params[1].Index;
		for (int I = 0; I < Inst->Params[2].Count; ++I) {
			Result = Tuple->Values[I]->Type->deref(Tuple->Values[I]);
			ERROR_CHECK(Result);
			ml_reference_t *Local = (ml_reference_t *)Base[I];
			Local->Value[0] = Result;
		}
		ADVANCE(0);
	}
	DO_LET: {
		Result = Result->Type->deref(Result);
		ERROR_CHECK(Result);
		ml_value_t *Uninitialized = Top[Inst->Params[1].Index];
		if (Uninitialized) ml_uninitialized_set(Uninitialized, Result);
		Top[Inst->Params[1].Index] = Result;
		ADVANCE(0);
	}
	DO_LETX: {
		Result = Result->Type->deref(Result);
		ERROR_CHECK(Result);
		if (Result->Type != MLTupleT) {
			Result = ml_error("TypeError", "Can only unpack tuples");
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		}
		ml_tuple_t *Tuple = (ml_tuple_t *)Result;
		if (Tuple->Size < Inst->Params[2].Count) {
			Result = ml_error("ValueError", "Tuple has too few values (%d < %d)", Tuple->Size, Inst->Params[2].Count);
			ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
			ERROR();
		}
		ml_value_t **Base = Top + Inst->Params[1].Index;
		for (int I = 0; I < Inst->Params[2].Count; ++I) {
			Result = Tuple->Values[I]->Type->deref(Tuple->Values[I]);
			ERROR_CHECK(Result);
			ml_value_t *Uninitialized = Base[I];
			if (Uninitialized) ml_uninitialized_set(Uninitialized, Result);
			Base[I] = Result;
		}
		ADVANCE(0);
	}
	DO_FOR: {
		Result = Result->Type->deref(Result);
		ERROR_CHECK(Result);
		Frame->Inst = Inst->Params[0].Inst;
		Frame->Top = Top;
		return ml_iterate((ml_state_t *)Frame, Result);
	}
	DO_NEXT: {
		for (int I = Inst->Params[1].Count; --I >= 0;) *--Top = 0;
		Result = Top[-1];
		*--Top = 0;
		Frame->Inst = Inst->Params[0].Inst;
		Frame->Top = Top;
		return ml_iter_next((ml_state_t *)Frame, Result);
	}
	DO_VALUE: {
		Result = Top[-1];
		Frame->Inst = Inst->Params[0].Inst;
		Frame->Top = Top;
		return ml_iter_value((ml_state_t *)Frame, Result);
	}
	DO_KEY: {
		Result = Top[-2];
		Frame->Inst = Inst->Params[0].Inst;
		Frame->Top = Top;
		return ml_iter_key((ml_state_t *)Frame, Result);
	}
	DO_CALL: {
		int Count = Inst->Params[1].Count;
		ml_value_t *Function = Top[~Count];
		Function = Function->Type->deref(Function);
		ERROR_CHECK(Function);
		ml_value_t **Args = Top - Count;
		ml_inst_t *Next = Inst->Params[0].Inst;
		if (Next->Opcode == MLI_RETURN) {
			return Function->Type->call(Frame->Base.Caller, Function, Count, Args);
		} else {
			Frame->Inst = Next;
			Frame->Top = Top - (Count + 1);
			return Function->Type->call((ml_state_t *)Frame, Function, Count, Args);
		}
	}
	DO_CONST_CALL: {
		int Count = Inst->Params[1].Count;
		ml_value_t *Function = Inst->Params[2].Value;
		ml_value_t **Args = Top - Count;
		ml_inst_t *Next = Inst->Params[0].Inst;
		if (Next->Opcode == MLI_RETURN) {
			return Function->Type->call(Frame->Base.Caller, Function, Count, Args);
		} else {
			Frame->Inst = Inst->Params[0].Inst;
			Frame->Top = Top - Count;
			return Function->Type->call((ml_state_t *)Frame, Function, Count, Args);
		}
	}
	DO_ASSIGN: {
		Result = Result->Type->deref(Result);
		ERROR_CHECK(Result);
		ml_value_t *Ref = Top[-1];
		*--Top = 0;
		Result = Ref->Type->assign(Ref, Result);
		ERROR_CHECK(Result);
		ADVANCE(0);
	}
	DO_LOCAL: {
		int Index = Inst->Params[1].Index;
		ml_value_t **Slot = (Index < 0) ? &Frame->UpValues[~Index] : &Frame->Stack[Index];
		Result = Slot[0];
		if (!Result) {
			Result = Slot[0] = ml_uninitialized();
		}
		ADVANCE(0);
	}
	DO_TUPLE_NEW: {
		int Size = Inst->Params[1].Count;
		ml_tuple_t *Tuple = xnew(ml_tuple_t, Size, ml_value_t *);
		Tuple->Type = MLTupleT;
		Tuple->Size = Size;
		*Top++ = (ml_value_t *)Tuple;
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
		Result = Result->Type->deref(Result);
		ERROR_CHECK(Result);
		ml_list_put(Top[-1], Result);
		ADVANCE(0);
	}
	DO_MAP_NEW: {
		*Top++ = ml_map();
		ADVANCE(0);
	}
	DO_MAP_INSERT: {
		ml_value_t *Key = Top[-1]->Type->deref(Top[-1]);
		ERROR_CHECK(Key);
		Result = Result->Type->deref(Result);
		ERROR_CHECK(Result);
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
			if (!Value) {
				Value = Slot[0] = ml_uninitialized();
			}
			if (Value->Type == MLUninitializedT) {
				ml_uninitialized_use(Value, &Closure->UpValues[I]);
			}
			Closure->UpValues[I] = Value;
		}
		Result = (ml_value_t *)Closure;
		ADVANCE(0);
	}
	DO_PARTIAL_NEW: {
		Result = Result->Type->deref(Result);
		ERROR_CHECK(Result);
		*Top++ = ml_partial_function_new(Result, Inst->Params[1].Count);
		ADVANCE(0);
	}
	DO_PARTIAL_SET: {
		Result = Result->Type->deref(Result);
		ERROR_CHECK(Result);
		ml_partial_function_set(Top[-1], Inst->Params[1].Index, Result);
		ADVANCE(0);
	}
#ifdef DEBUG_VERSION
	DO_DEBUG_ADVANCE: {
		ml_debugger_t *Debugger = Frame->Debugger;
		if (Result == ClosureEntry || Inst->PotentialBreakpoint) {
			size_t *Breakpoints;
			size_t Revision = Debugger->Revision;
			if (Frame->Revision != Revision) {
				Frame->Revision = Revision;
				Breakpoints = Frame->Breakpoints = Debugger->breakpoints(Debugger, Frame->Source, 0);
			} else {
				Breakpoints = Frame->Breakpoints;
			}
			int LineNo = Inst->LineNo;
			if (Breakpoints[LineNo / SIZE_BITS] & (1 << LineNo % SIZE_BITS)) goto DO_BREAKPOINT;
			if (Debugger->StepIn) goto DO_BREAKPOINT;
			if (Debugger->StepOverFrame == (ml_state_t *)Frame) goto DO_BREAKPOINT;
			if (Inst->Opcode == MLI_RETURN && Debugger->StepOutFrame == (ml_state_t *)Frame) goto DO_BREAKPOINT;
		}
		goto *Labels[Inst->Opcode];
	}
	DO_DEBUG_ERROR: {
		ml_debugger_t *Debugger = Frame->Debugger;
		if (Debugger->BreakOnError) goto DO_BREAKPOINT;
		goto *Labels[Inst->Opcode];
	}
	DO_BREAKPOINT: {
		if (--Frame->Reentry) {
			ml_debugger_t *Debugger = Frame->Debugger;
			Frame->Inst = Inst;
			Frame->Top = Top;
			Frame->Reentry = 1;
			return Debugger->run(Debugger, (ml_state_t *)Frame, Result);
		} else {
			goto *Labels[Inst->Opcode];
		}
	}
#endif
}

static void ml_closure_call_debug(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args);

static void DEBUG_FUNC(closure_call)(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	ml_closure_info_t *Info = Closure->Info;
	ml_debugger_t *Debugger = (ml_debugger_t *)ml_context_get(Caller->Context, ML_DEBUGGER_INDEX);
#ifndef DEBUG_VERSION
	if (Debugger) return ml_closure_call_debug(Caller, Value, Count, Args);
#endif
	DEBUG_STRUCT(frame) *Frame = xnew(DEBUG_STRUCT(frame), Info->FrameSize, ml_value_t *);
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
		ml_reference_t *Local = (ml_reference_t *)ml_reference(NULL);
		ml_value_t *Arg = Args[I]->Type->deref(Args[I]);
		if (Arg->Type == MLErrorT) ML_RETURN(Arg);
		if (Arg->Type == MLNamesT) break;
		Local->Value[0] = Arg;
		Frame->Stack[I] = (ml_value_t *)Local;
	}
	for (int J = I; J < NumParams; ++J) {
		ml_reference_t *Local = (ml_reference_t *)ml_reference(NULL);
		Local->Value[0] = MLNil;
		Frame->Stack[J] = (ml_value_t *)Local;
	}
	if (Info->ExtraArgs) {
		ml_reference_t *Local = (ml_reference_t *)ml_reference(NULL);
		ml_value_t *Rest = ml_list();
		for (; I < Count; ++I) {
			ml_value_t *Arg = Args[I]->Type->deref(Args[I]);
			if (Arg->Type == MLErrorT) ML_RETURN(Arg);
			if (Arg->Type == MLNamesT) break;
			ml_list_put(Rest, Arg);
		}
		Local->Value[0] = Rest;
		Frame->Stack[NumParams] = (ml_value_t *)Local;
		++NumParams;
	}
	if (Info->NamedArgs) {
		ml_reference_t *Local = (ml_reference_t *)ml_reference(NULL);
		ml_value_t *Options = ml_map();
		for (; I < Count; ++I) {
			if (Args[I]->Type == MLNamesT) {
				ML_NAMES_FOREACH(Args[I], Node) {
					const char *Name = ml_method_name(Node->Value);
					int Index = (intptr_t)stringmap_search(Info->Params, Name);
					if (Index) {
						ml_reference_t *Local = (ml_reference_t *)Frame->Stack[Index - 1];
						Local->Value[0] = Args[++I];
					} else {
						ml_map_insert(Options, Node->Value, Args[++I]);
					}
				}
				break;
			}
		}
		Local->Value[0] = Options;
		Frame->Stack[NumParams] = (ml_value_t *)Local;
		++NumParams;
	} else {
		for (; I < Count; ++I) {
			if (Args[I]->Type == MLNamesT) {
				ML_NAMES_FOREACH(Args[I], Node) {
					const char *Name = ml_method_name(Node->Value);
					int Index = (intptr_t)stringmap_search(Info->Params, Name);
					if (Index) {
						ml_reference_t *Local = (ml_reference_t *)Frame->Stack[Index - 1];
						Local->Value[0] = Args[++I];
					} else {
						ML_RETURN(ml_error("NameError", "Unknown named parameters %s", Name));
					}
				}
				break;
			}
		}
	}
	Frame->Top = Frame->Stack + NumParams;
	Frame->OnError = Info->Return;
	Frame->UpValues = Closure->UpValues;
	Frame->Inst = Info->Entry;
#ifdef DEBUG_VERSION
	Frame->Debugger = Debugger;
	Frame->Revision = Debugger->Revision;
	Frame->Breakpoints = Debugger->breakpoints(Debugger, Frame->Source, Info->End);
	Frame->Decls = Info->Decls;
#endif
	ML_CONTINUE(Frame, ClosureEntry);
}

#ifndef DEBUG_VERSION
const char *MLInsts[] = {
	"return", // MLI_RETURN,
	"suspend", // MLI_SUSPEND,
	"resume", // MLI_RESUME,
	"nil", // MLI_NIL,
	"some", // MLI_SOME,
	"if", // MLI_IF,
	"if_var", // MLI_IF_VAR,
	"if_let", // MLI_IF_LET,
	"else", // MLI_ELSE,
	"push", // MLI_PUSH,
	"with", // MLI_WITH,
	"withx", // MLI_WITHX,
	"pop", // MLI_POP,
	"enter", // MLI_ENTER,
	"exit", // MLI_EXIT,
	"loop", // MLI_LOOP,
	"try", // MLI_TRY,
	"catch", // MLI_CATCH,
	"load", // MLI_LOAD,
	"var", // MLI_VAR,
	"varx", // MLI_VARX,
	"let", // MLI_LET,
	"letx", // MLI_LETX,
	"for", // MLI_FOR,
	"next", // MLI_NEXT,
	"value", // MLI_VALUE,
	"key", // MLI_KEY,
	"call", // MLI_CALL,
	"const_call", // MLI_CONST_CALL,
	"assign", // MLI_ASSIGN,
	"local", // MLI_LOCAL,
	"tuple_new", // MLI_TUPLE_NEW,
	"tuple_set", // MLI_TUPLE_SET,
	"list_new", // MLI_LIST_NEW,
	"list_append", // MLI_LIST_APPEND,
	"map_new", // MLI_MAP_NEW,
	"map_insert", // MLI_MAP_INSERT,
	"closure", // MLI_CLOSURE,
	"partial_new", // MLI_PARTIAL_NEW,
	"partial_set" // MLI_PARTIAL_SET
};

typedef enum {
	MLIT_NONE,
	MLIT_INST,
	MLIT_INST_INST,
	MLIT_INST_INDEX,
	MLIT_INST_INDEX_COUNT,
	MLIT_INST_COUNT,
	MLIT_INST_COUNT_COUNT,
	MLIT_INST_COUNT_VALUE,
	MLIT_INST_VALUE,
	MLIT_INST_CLOSURE
} ml_inst_type_t;

const ml_inst_type_t MLInstTypes[] = {
	MLIT_NONE, // MLI_RETURN,
	MLIT_INST, // MLI_SUSPEND,
	MLIT_INST, // MLI_RESUME,
	MLIT_INST, // MLI_NIL,
	MLIT_INST, // MLI_SOME,
	MLIT_INST_INST, // MLI_IF,
	MLIT_INST_INST, // MLI_IF_VAR,
	MLIT_INST_INST, // MLI_IF_LET,
	MLIT_INST_INST, // MLI_ELSE,
	MLIT_INST, // MLI_PUSH,
	MLIT_INST, // MLI_WITH,
	MLIT_INST_COUNT, // MLI_WITHX,
	MLIT_INST, // MLI_POP,
	MLIT_INST_COUNT_COUNT, // MLI_ENTER,
	MLIT_INST_COUNT, // MLI_EXIT,
	MLIT_INST, // MLI_LOOP,
	MLIT_INST_INST, // MLI_TRY,
	MLIT_INST_INDEX, // MLI_CATCH,
	MLIT_INST_VALUE, // MLI_LOAD,
	MLIT_INST_INDEX, // MLI_VAR,
	MLIT_INST_INDEX_COUNT, // MLI_VARX,
	MLIT_INST_INDEX, // MLI_LET,
	MLIT_INST_INDEX_COUNT, // MLI_LETX,
	MLIT_INST, // MLI_FOR,
	MLIT_INST_COUNT, // MLI_NEXT,
	MLIT_INST, // MLI_VALUE,
	MLIT_INST, // MLI_KEY,
	MLIT_INST_COUNT, // MLI_CALL,
	MLIT_INST_COUNT_VALUE, // MLI_CONST_CALL,
	MLIT_INST, // MLI_ASSIGN,
	MLIT_INST_INDEX, // MLI_LOCAL,
	MLIT_INST_COUNT, // MLI_TUPLE_NEW,
	MLIT_INST_INDEX, // MLI_TUPLE_SET,
	MLIT_INST, // MLI_LIST_NEW,
	MLIT_INST, // MLI_LIST_APPEND,
	MLIT_INST, // MLI_MAP_NEW,
	MLIT_INST, // MLI_MAP_INSERT,
	MLIT_INST_CLOSURE, // MLI_CLOSURE,
	MLIT_INST_COUNT, // MLI_PARTIAL_NEW,
	MLIT_INST_INDEX, // MLI_PARTIAL_SET
};

static void ml_inst_process(int Process, ml_inst_t *Source, ml_inst_t *Inst, unsigned char Hash[SHA256_BLOCK_SIZE], int I, int J) {
	if (!Source || (Source->LineNo != Inst->LineNo)) Inst->PotentialBreakpoint = 1;
	if (Inst->Processed == Process) return;
	Inst->Processed = Process;
	Hash[I] ^= Inst->Opcode;
	Hash[J] ^= (Inst->Opcode << 4);
	switch (MLInstTypes[Inst->Opcode]) {
	case MLIT_INST_INST:
		ml_inst_process(Process, Inst, Inst->Params[1].Inst, Hash, (I + 11) % (SHA256_BLOCK_SIZE - 4), (J + 13) % (SHA256_BLOCK_SIZE - 8));
		break;
	case MLIT_INST_COUNT_COUNT:
		*(int *)(Hash + I) ^= Inst->Params[1].Count;
		*(int *)(Hash + J) ^= Inst->Params[2].Count;
		break;
	case MLIT_INST_COUNT:
		*(int *)(Hash + I) ^= Inst->Params[1].Count;
		break;
	case MLIT_INST_INDEX:
		*(int *)(Hash + I) ^= Inst->Params[1].Index;
		break;
	case MLIT_INST_VALUE:
		*(long *)(Hash + J) ^= ml_hash(Inst->Params[1].Value);
		break;
	case MLIT_INST_INDEX_COUNT:
		*(int *)(Hash + I) ^= Inst->Params[1].Index;
		*(int *)(Hash + J) ^= Inst->Params[2].Count;
		break;
	case MLIT_INST_COUNT_VALUE:
		*(int *)(Hash + I) ^= Inst->Params[1].Count;
		*(long *)(Hash + J) ^= ml_hash(Inst->Params[2].Value);
		break;
	case MLIT_INST_CLOSURE: {
		ml_closure_info_t *Info = Inst->Params[1].ClosureInfo;
		*(long *)(Hash + J) ^= *(long *)(Info->Hash + J);
		for (int N = 0; N < Info->NumUpValues; ++N) {
			int Index = Inst->Params[2 + N].Index;
			*(int *)(Hash + I) ^= (Index << N);
		}
		break;
	default:
		break;
	}
	}
	if (Inst->Opcode != MLI_RETURN) {
		ml_inst_process(Process, Inst, Inst->Params[0].Inst, Hash, (I + 3) % (SHA256_BLOCK_SIZE - 4), (J + 7) % (SHA256_BLOCK_SIZE - 8));
	}
}

void ml_closure_info_finish(ml_closure_info_t *Info) {
	ml_inst_process(Info->Entry->Processed + 1, NULL, Info->Entry, Info->Hash, 0, 0);
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

ML_TYPE(MLClosureT, MLFunctionT, "closure",
	.hash = ml_closure_hash,
	.call = ml_closure_call
);

ML_METHOD(MLStringOfMethod, MLClosureT) {
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	return ml_string_format("<closure:%s@%d>", Closure->Info->Source, Closure->Info->Entry->LineNo);
}

static void ml_inst_escape_string(FILE *Graph, const char *String, size_t Length) {
	for (int I = 0; I < Length; ++I) switch (String[I]) {
		case 0: fputs("\\\\0", Graph); break;
		case '\t': fputs("\\\\t", Graph); break;
		case '\r': fputs("\\\\r", Graph); break;
		case '\n': fputs("\\\\n", Graph); break;
		case '\'': fputs("\\\'", Graph); break;
		case '\"': fputs("\\\"", Graph); break;
		case '\\': fputs("\\\\", Graph); break;
		default: fputc(String[I], Graph); break;
	}
}

static void ml_inst_graph(int Process, FILE *Graph, ml_inst_t *Inst) {
	if (Inst->Processed == Process) return;
	Inst->Processed = Process;
	fprintf(Graph, "\tI%" PRIxPTR " [label=\"%d: %s(", (uintptr_t)Inst, Inst->LineNo, MLInsts[Inst->Opcode]);
	ml_inst_t *Next = NULL, *NotNil = NULL;
	if (Inst->Opcode != MLI_RETURN) Next = Inst->Params[0].Inst;
	switch (Inst->Opcode) {
	case MLI_IF:
	case MLI_IF_VAR:
	case MLI_IF_LET:
		NotNil = Inst->Params[1].Inst;
		break;
	case MLI_ELSE:
	case MLI_TRY:
		NotNil = Next;
		Next = Inst->Params[0].Inst;
		break;
	case MLI_NEXT:
	case MLI_WITHX:
	case MLI_EXIT:
	case MLI_CALL:
	case MLI_TUPLE_NEW:
	case MLI_PARTIAL_NEW:
		fprintf(Graph, "%d", Inst->Params[1].Count);
		break;
	case MLI_CATCH:
	case MLI_VAR:
	case MLI_LET:
	case MLI_LOCAL:
	case MLI_TUPLE_SET:
	case MLI_PARTIAL_SET:
		fprintf(Graph, "%d", Inst->Params[1].Index);
		break;
	case MLI_ENTER:
		fprintf(Graph, "%d, %d", Inst->Params[1].Count, Inst->Params[2].Count);
		break;
	case MLI_VARX:
	case MLI_LETX:
		fprintf(Graph, "%d, %d", Inst->Params[1].Index, Inst->Params[2].Count);
		break;
	case MLI_LOAD: {
		ml_value_t *Value = Inst->Params[1].Value;
		if (Value->Type == MLStringT) {
			ml_inst_escape_string(Graph, ml_string_value(Value), ml_string_length(Value));
		} else if (Value->Type == MLIntegerT) {
			fprintf(Graph, "%ld", ml_integer_value(Value));
		} else if (Value->Type == MLRealT) {
			fprintf(Graph, "%f", ml_real_value(Value));
		} else {
			fprintf(Graph, "%s", Inst->Params[1].Value->Type->Name);
		}
		break;
	}
	case MLI_CONST_CALL: {
		fprintf(Graph, "%d, ", Inst->Params[1].Count);
		ml_value_t *Value = Inst->Params[2].Value;
		if (Value->Type == MLMethodT) {
			fprintf(Graph, ":%s", ml_method_name(Value));
		} else {
			Value = ml_inline(MLStringOfMethod, 1, Value);
			if (Value->Type == MLStringT) {
				fprintf(Graph, "%s", ml_string_value(Value));
			} else {
				fprintf(Graph, "%s", Inst->Params[2].Value->Type->Name);
			}
		}
		break;
	}
	case MLI_CLOSURE:
		fprintf(Graph, "C%" PRIxPTR, (uintptr_t)Inst->Params[1].ClosureInfo);
		break;
	default: break;
	}
	fprintf(Graph, ")\"];\n");
	if (Next) {
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Next);
		ml_inst_graph(Process, Graph, Next);
	}
	if (NotNil) {
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR " [label=\"not nil\" style=dashed color=blue];\n", (uintptr_t)Inst, (uintptr_t)NotNil);
		ml_inst_graph(Process, Graph, NotNil);
	}
}

const char *ml_closure_info_debug(ml_closure_info_t *Info) {
	char *FileName;
	asprintf(&FileName, "C%" PRIxPTR ".dot", (uintptr_t)Info);
	FILE *File = fopen(FileName, "w");
	fprintf(File, "digraph C%" PRIxPTR " {\n", (uintptr_t)Info);
	fprintf(File, "\tgraph [compound=true,ranksep=0.3];\n");
	fprintf(File, "\tnode [shape=plaintext,margin=0.1,height=0];\n");
	ml_inst_graph(Info->Entry->Processed + 1, File, Info->Entry);
	fprintf(File, "}\n");
	fclose(File);
	printf("Wrote closure to %s\n", FileName);
	return FileName;
}

const char *ml_closure_debug(ml_value_t *Value) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	return ml_closure_info_debug(Closure->Info);
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
	int Process = Info->Entry->Processed + 1;
	Info->Return->Processed = Process;
	ml_cbor_closure_find_refs(Process, Info->Entry, Refs);
	ml_cbor_closure_write_int(Refs->Size, Buffer);
	ml_cbor_closure_write_inst(Info->Entry->Processed + 1, Info->Entry, Refs, Buffer);
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
		UpValue = UpValue->Type->deref(UpValue);
		ml_cbor_write(UpValue, Buffer, ml_stringbuffer_add);
	}
}

void ML_TYPED_FN(ml_cbor_write, MLClosureT, ml_closure_t *Closure, void *Data, ml_cbor_write_fn WriteFn) {
	ml_cbor_write_tag(Data, WriteFn, 36); // TODO: Pick correct tag
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_cbor_write_closure(Closure, Buffer);
	ml_cbor_write_bytes(Data, WriteFn, Buffer->Length);
	ml_stringbuffer_foreach(Buffer, Data, WriteFn);
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
#include "ml_bytecode_init.c"
}
#endif

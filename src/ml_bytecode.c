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
	debug_function_t *Debug;
	DEBUG_STRUCT(frame) *UpState;
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

#define ERROR_CHECK(VALUE) if (VALUE->Type == MLErrorT) { \
	ml_error_trace_add(VALUE, (ml_source_t){Frame->Source, Inst->LineNo}); \
	Result = VALUE; \
	ERROR(); \
}

#define ADVANCE(N) \
	Inst = Inst->Params[N].Inst; \
	goto *Labels[Inst->Opcode]

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
		[MLI_PUSHX] = &&DO_PUSHX,
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
		ERROR();
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
	DO_PUSHX: {
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
		ADVANCE(0);
	}
	DO_EXIT: {
		for (int I = Inst->Params[1].Count; --I >= 0;) *--Top = 0;
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
		ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Top[Inst->Params[1].Index];
		if (Uninitialized) {
			for (ml_slot_t *Slot = Uninitialized->Slots; Slot; Slot = Slot->Next) Slot->Value[0] = Result;
		}
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
			ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Base[I];
			if (Uninitialized) {
				for (ml_slot_t *Slot = Uninitialized->Slots; Slot; Slot = Slot->Next) Slot->Value[0] = Result;
			}
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
			ml_uninitialized_t *Uninitialized = new(ml_uninitialized_t);
			Uninitialized->Type = MLUninitializedT;
			Result = Slot[0] = (ml_value_t *)Uninitialized;
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
		ml_list_append(Top[-1], Result);
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
				ml_uninitialized_t *Uninitialized = new(ml_uninitialized_t);
				Uninitialized->Type = MLUninitializedT;
				Slot[0] = Value = (ml_value_t *)Uninitialized;
			}
			if (Value->Type == MLUninitializedT) {
				ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Value;
				ml_slot_t *Slot = new(ml_slot_t);
				Slot->Value = &Closure->UpValues[I];
				Slot->Next = Uninitialized->Slots;
				Uninitialized->Slots = Slot;
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
}

#ifndef DEBUG_VERSION
static void ml_closure_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	ml_closure_info_t *Info = Closure->Info;
	DEBUG_STRUCT(frame) *Frame = xnew(DEBUG_STRUCT(frame), Info->FrameSize, ml_value_t *);
	Frame->Base.Type = DEBUG_TYPE(Continuation);
	Frame->Base.Caller = Caller;
	Frame->Base.run = (void *)DEBUG_FUNC(frame_run);
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
		ml_list_t *Rest = new(ml_list_t);
		Rest->Type = MLListT;
		int Length = 0;
		ml_list_node_t **Next = &Rest->Head;
		ml_list_node_t *Prev = 0;
		for (; I < Count; ++I) {
			ml_value_t *Arg = Args[I]->Type->deref(Args[I]);
			if (Arg->Type == MLErrorT) ML_RETURN(Arg);
			if (Arg->Type == MLNamesT) break;
			ml_list_node_t *Node = new(ml_list_node_t);
			Node->Value = Arg;
			Node->Prev = Prev;
			Next[0] = Prev = Node;
			Next = &Node->Next;
			++Length;
		}
		Rest->Tail = Prev;
		Rest->Length = Length;
		Local->Value[0] = (ml_value_t *)Rest;
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
	ML_CONTINUE(Frame, MLNil);
}

static void ml_inst_sha256_pass1(ml_inst_t *Inst, unsigned char Hash[SHA256_BLOCK_SIZE], int I, int J) {
	if (Inst->Flags) return;
	Inst->Flags = 1;
	Hash[I] ^= Inst->Opcode;
	Hash[J] ^= (Inst->Opcode << 4);
	switch (Inst->Opcode) {
		case MLI_IF:
	case MLI_IF_VAR:
	case MLI_IF_LET:
	case MLI_ELSE:
	case MLI_TRY:
		ml_inst_sha256_pass1(Inst->Params[1].Inst, Hash, (I + 11) % (SHA256_BLOCK_SIZE - 4), (J + 13) % (SHA256_BLOCK_SIZE - 8));
		break;
	case MLI_ENTER:
		*(int *)(Hash + I) ^= Inst->Params[1].Count;
		*(int *)(Hash + J) ^= Inst->Params[2].Count;
		break;
	case MLI_EXIT:
	case MLI_NEXT:
	case MLI_CALL:
	case MLI_TUPLE_NEW:
	case MLI_PARTIAL_NEW:
		*(int *)(Hash + I) ^= Inst->Params[1].Count;
		break;
	case MLI_CATCH:
	case MLI_VAR:
	case MLI_LET:
	case MLI_LOCAL:
	case MLI_TUPLE_SET:
	case MLI_PARTIAL_SET:
		*(int *)(Hash + I) ^= Inst->Params[1].Index;
		break;
	case MLI_LOAD:
		*(long *)(Hash + J) ^= ml_hash(Inst->Params[1].Value);
		break;
	case MLI_VARX:
	case MLI_LETX:
		*(int *)(Hash + I) ^= Inst->Params[1].Index;
		*(int *)(Hash + J) ^= Inst->Params[2].Count;
		break;
	case MLI_CONST_CALL:
		*(int *)(Hash + I) ^= Inst->Params[1].Count;
		*(long *)(Hash + J) ^= ml_hash(Inst->Params[2].Value);
		break;
	case MLI_CLOSURE: {
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
		ml_inst_sha256_pass1(Inst->Params[0].Inst, Hash, (I + 3) % (SHA256_BLOCK_SIZE - 4), (J + 7) % (SHA256_BLOCK_SIZE - 8));
	}
}

static void ml_inst_sha256_pass2(ml_inst_t *Inst) {
	if (!Inst->Flags) return;
	Inst->Flags = 0;
	if (Inst->Opcode == MLI_RETURN) return;
	ml_inst_sha256_pass2(Inst->Params[0].Inst);
	switch (Inst->Opcode) {
	case MLI_IF:
	case MLI_IF_VAR:
	case MLI_IF_LET:
	case MLI_ELSE:
	case MLI_TRY:
		ml_inst_sha256_pass2(Inst->Params[1].Inst);
		break;
	default:
		break;
	}
}

void ml_closure_info_sha256(ml_closure_info_t *Info) {
	ml_inst_sha256_pass1(Info->Entry, Info->Hash, 0, 0);
	ml_inst_sha256_pass2(Info->Entry);
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
	/*printf("Closure->Info->Hash =");
	for (int I = 0; I < SHA256_BLOCK_SIZE; ++I) printf(" %02x", Closure->Info->Hash[I]);
	printf("\n");*/
	long Hash = 0;
	long *P = (long *)Closure->Info->Hash;
	while (P < (Closure->Info->Hash + SHA256_BLOCK_SIZE)) Hash ^= *P++;
	for (int I = 0; I < Closure->Info->NumUpValues; ++I) {
		Hash ^= ml_hash_chain(Closure->UpValues[I], Chain) << I;
	}
	return Hash;
}

static void ML_TYPED_FN(ml_iterate, DEBUG_TYPE(Closure), ml_state_t *Frame, ml_value_t *Closure) {
	return ml_closure_call(Frame, Closure, 0, NULL);
}

ml_type_t MLClosureT[1] = {{
	MLTypeT,
	MLFunctionT, "closure",
	ml_closure_hash,
	ml_closure_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

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

static void ml_inst_graph(FILE *Graph, ml_inst_t *Inst, stringmap_t *Done, const char *Colour) {
	char InstName[16];
	sprintf(InstName, "I%" PRIxPTR "", (uintptr_t)Inst);
	if (stringmap_search(Done, InstName)) return;
	stringmap_insert(Done, strdup(InstName), Inst);
	switch (Inst->Opcode) {
	case MLI_RETURN: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: return()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		break;
	}
	case MLI_SUSPEND: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: suspend()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_RESUME: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: resume()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_NIL: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: nil()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_SOME: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: some()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_IF: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: if()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR " [label=\"not nil\" style=dashed color=blue];\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[1].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_IF_VAR: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: if_var()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR " [label=\"not nil\" style=dashed color=blue];\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[1].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_IF_LET: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: if_def()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR " [label=\"not nil\" style=dashed color=blue];\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[1].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_ELSE: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: else()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR " [label=\"not nil\" style=dashed color=blue];\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[1].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_PUSH: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: push()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_PUSHX: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: pushx(%d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Count);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_POP: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: pop()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_LOAD: {
		ml_value_t *Value = Inst->Params[1].Value;
		if (Value->Type == MLStringT) {
			fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: load(\'", (uintptr_t)Inst, Colour, Inst->LineNo);
			ml_inst_escape_string(Graph, ml_string_value(Value), ml_string_length(Value));
			fprintf(Graph, "\')\"];\n");
		} else if (Value->Type == MLIntegerT) {
			fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: load(%ld)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, ml_integer_value(Value));
		} else if (Value->Type == MLRealT) {
			fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: load(%f)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, ml_real_value(Value));
		} else {
			fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: load(%s)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Value->Type->Name);
		}
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_ENTER: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: enter(%d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Count);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_EXIT: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: exit(%d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Count);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_LOOP: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: loop()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_TRY: {
		intptr_t Handler = (intptr_t)Inst->Params[1].Inst;
		unsigned char *X = (unsigned char *)&Handler;
		unsigned int Hash = 0;
		for (int I = 0; I < sizeof(intptr_t); ++I) Hash ^= X[I];
		double Hue = Hash / 255.0;
		char *TryColour;
		asprintf(&TryColour, "%f 0.9 0.5", Hue);
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: try()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[1].Inst);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR " [label=\"error\" style=dashed color=red];\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, TryColour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_CATCH: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: catch(%d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Index);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_VAR: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: var(%d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Index);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_VARX: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: varx(%d, %d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Index, Inst->Params[2].Count);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_LET: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: let(%d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Index);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_LETX: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: ltex(%d, %d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Index, Inst->Params[2].Count);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_FOR: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: for()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_NEXT: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: next()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_VALUE: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: value()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_KEY: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: key()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_CALL: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: call(%d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Count);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_CONST_CALL: {
		ml_value_t *StringMethod = ml_method("string");
		ml_value_t *Value = Inst->Params[2].Value;
		if (Value->Type == MLMethodT) {
			fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: const_call(%d, :%s)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Count, ml_method_name(Value));
		} else {
			Value = ml_inline(StringMethod, 1, Value);
			if (Value->Type == MLStringT) {
				fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: const_call(%d, %s)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Count, ml_string_value(Value));
			} else {
				fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: const_call(%d, %s)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Count, Inst->Params[2].Value->Type->Name);
			}
		}
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_ASSIGN: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: assign()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_LOCAL: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: local(%d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Index);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_TUPLE_NEW: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: tuple_new(%d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Count);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_TUPLE_SET: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: tuple_set(%d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Index);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_LIST_NEW: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: list_new()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_LIST_APPEND: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: list_append()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_MAP_NEW: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: map_new()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_MAP_INSERT: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: map_insert()\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_CLOSURE: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: closure(C%" PRIxPTR ")\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, (uintptr_t)Inst->Params[1].ClosureInfo);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_PARTIAL_NEW: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: partial_new(%d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Count);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_PARTIAL_SET: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: partial_set(%d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Params[1].Index);
		fprintf(Graph, "\tI%" PRIxPTR " -> I%" PRIxPTR ";\n", (uintptr_t)Inst, (uintptr_t)Inst->Params[0].Inst);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	default: {
		fprintf(Graph, "\tI%" PRIxPTR " [fontcolor=\"%s\" label=\"%d: unknown(%d)\"];\n", (uintptr_t)Inst, Colour, Inst->LineNo, Inst->Opcode);
		break;
	}
	}
}

const char *ml_closure_info_debug(ml_closure_info_t *Info) {
	stringmap_t Done[1] = {STRINGMAP_INIT};
	char *FileName;
	asprintf(&FileName, "C%" PRIxPTR ".dot", (uintptr_t)Info);
	FILE *File = fopen(FileName, "w");
	fprintf(File, "digraph C%" PRIxPTR " {\n", (uintptr_t)Info);
	fprintf(File, "\tgraph [compound=true,ranksep=0.3];\n");
	fprintf(File, "\tnode [shape=plaintext,margin=0.1,height=0];\n");
	ml_inst_graph(File, Info->Entry, Done, "black");
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
	for (ml_list_node_t *Node = ArgsList->Head; Node; Node = Node->Next) *Arg++ = Node->Value;
	return (ml_value_t *)Partial;
}

#define DEBUG_VERSION
#include "ml_bytecode.c"
#undef DEBUG_VERSION

void ml_bytecode_init() {
#include "ml_bytecode_init.c"
}
#endif

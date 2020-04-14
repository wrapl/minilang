#include "ml_bytecode_threaded.h"
#include "ml_macros.h"

struct ml_frame_t {
	ml_state_t Base;
	const char *Source;
	ml_inst_t *Inst;
	ml_value_t **Top;
	ml_inst_t *OnError;
	ml_value_t **UpValues;
	ml_value_t *Stack[];
};

static ml_value_t *frame_run_threaded(ml_frame_t *Frame, ml_value_t *Result) {
	ml_inst_t *Inst = Frame->Inst;
	if (Result->Type == MLErrorT) {
		ml_error_trace_add(Result, (ml_source_t){Frame->Source, Inst->LineNo});
		Inst = Frame->OnError;
	}
	return Inst->run(Inst, Frame, Result, Frame->Top);
}

#define ADVANCE(N) \
	Inst = Inst->Params[N].Inst; \
	return Inst->run(Inst, Frame, Result, Top)

#define ERROR() \
	Inst = Frame->OnError; \
	return Inst->run(Inst, Frame, Result, Top)

#define ERROR_CHECK(VALUE) if (VALUE->Type == MLErrorT) { \
	ml_error_trace_add(VALUE, (ml_source_t){Frame->Source, Inst->LineNo}); \
	Result = VALUE; \
	ERROR(); \
}

ml_value_t *ml_inst_RETURN(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	ML_CONTINUE(Frame->Base.Caller, Result);
}

extern ml_type_t MLSuspensionT[];

ml_value_t *ml_inst_SUSPEND(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Frame->Base.Type = MLSuspensionT;
	Frame->Inst = Inst->Params[0].Inst;
	Frame->Top = Top;
	ML_CONTINUE(Frame->Base.Caller, (ml_value_t *)Frame);
}

ml_value_t *ml_inst_RESUME(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	*--Top = 0;
	ADVANCE(0);
}

ml_value_t *ml_inst_NIL(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Result = MLNil;
	ADVANCE(0);
}

ml_value_t *ml_inst_SOME(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Result = MLSome;
	ADVANCE(0);
}

ml_value_t *ml_inst_IF(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
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

ml_value_t *ml_inst_IF_VAR(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
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

ml_value_t *ml_inst_IF_LET(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
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

ml_value_t *ml_inst_ELSE(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
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

ml_value_t *ml_inst_PUSH(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	*Top++ = Result;
	ADVANCE(0);
}

ml_value_t *ml_inst_POP(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Result = *--Top;
	*Top = 0;
	ADVANCE(0);
}

ml_value_t *ml_inst_ENTER(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
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

ml_value_t *ml_inst_EXIT(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	for (int I = Inst->Params[1].Count; --I >= 0;) *--Top = 0;
	ADVANCE(0);
}

ml_value_t *ml_inst_LOOP(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	ADVANCE(0);
}

ml_value_t *ml_inst_TRY(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Frame->OnError = Inst->Params[1].Inst;
	ADVANCE(0);
}

ml_value_t *ml_inst_CATCH(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
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

ml_value_t *ml_inst_LOAD(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Result = Inst->Params[1].Value;
	ADVANCE(0);
}

ml_value_t *ml_inst_VAR(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	ml_reference_t *Local = (ml_reference_t *)Top[Inst->Params[1].Index];
	Local->Value[0] = Result;
	ADVANCE(0);
}

ml_value_t *ml_inst_VARX(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
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

ml_value_t *ml_inst_LET(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Top[Inst->Params[1].Index];
	if (Uninitialized) {
		for (ml_slot_t *Slot = Uninitialized->Slots; Slot; Slot = Slot->Next) Slot->Value[0] = Result;
	}
	Top[Inst->Params[1].Index] = Result;
	ADVANCE(0);
}

ml_value_t *ml_inst_LETX(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
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

ml_value_t *ml_inst_FOR(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	Frame->Inst = Inst->Params[0].Inst;
	Frame->Top = Top;
	return ml_iterate((ml_state_t *)Frame, Result);
}

ml_value_t *ml_inst_NEXT(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	for (int I = Inst->Params[1].Count; --I >= 0;) *--Top = 0;
	Result = Top[-1];
	*--Top = 0;
	Frame->Inst = Inst->Params[0].Inst;
	Frame->Top = Top;
	return ml_iter_next((ml_state_t *)Frame, Result);
}

ml_value_t *ml_inst_VALUE(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Result = Top[-1];
	Frame->Inst = Inst->Params[0].Inst;
	Frame->Top = Top;
	return ml_iter_value((ml_state_t *)Frame, Result);
}

ml_value_t *ml_inst_KEY(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Result = Top[-2];
	Frame->Inst = Inst->Params[0].Inst;
	Frame->Top = Top;
	return ml_iter_key((ml_state_t *)Frame, Result);
}

ml_value_t *ml_inst_CALL(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	int Count = Inst->Params[1].Count;
	ml_value_t *Function = Top[~Count];
	Function = Function->Type->deref(Function);
	ERROR_CHECK(Function);
	ml_value_t **Args = Top - Count;
	ml_inst_t *Next = Inst->Params[0].Inst;
	if (Next->run == ml_inst_RETURN) {
		return Function->Type->call(Frame->Base.Caller, Function, Count, Args);
	} else {
		Frame->Inst = Next;
		Frame->Top = Top - (Count + 1);
		return Function->Type->call((ml_state_t *)Frame, Function, Count, Args);
	}
}

ml_value_t *ml_inst_CONST_CALL(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	int Count = Inst->Params[1].Count;
	ml_value_t *Function = Inst->Params[2].Value;
	ml_value_t **Args = Top - Count;
	ml_inst_t *Next = Inst->Params[0].Inst;
	if (Next->run == ml_inst_RETURN) {
		return Function->Type->call(Frame->Base.Caller, Function, Count, Args);
	} else {
		Frame->Inst = Inst->Params[0].Inst;
		Frame->Top = Top - Count;
		return Function->Type->call((ml_state_t *)Frame, Function, Count, Args);
	}
}

ml_value_t *ml_inst_ASSIGN(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	ml_value_t *Ref = Top[-1];
	*--Top = 0;
	Result = Ref->Type->assign(Ref, Result);
	ERROR_CHECK(Result);
	ADVANCE(0);
}

ml_value_t *ml_inst_LOCAL(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
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

ml_value_t *ml_inst_TUPLE_NEW(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	int Size = Inst->Params[1].Count;
	ml_tuple_t *Tuple = xnew(ml_tuple_t, Size, ml_value_t *);
	Tuple->Type = MLTupleT;
	Tuple->Size = Size;
	*Top++ = (ml_value_t *)Tuple;
	ADVANCE(0);
}

ml_value_t *ml_inst_TUPLE_SET(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	int Index = Inst->Params[1].Index;
	ml_tuple_t *Tuple = (ml_tuple_t *)Top[-1];
	Tuple->Values[Index] = Result;
	ADVANCE(0);
}

ml_value_t *ml_inst_LIST_NEW(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	*Top++ = ml_list();
	ADVANCE(0);
}

ml_value_t *ml_inst_LIST_APPEND(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	ml_list_append(Top[-1], Result);
	ADVANCE(0);
}

ml_value_t *ml_inst_MAP_NEW(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	*Top++ = ml_map();
	ADVANCE(0);
}

ml_value_t *ml_inst_MAP_INSERT(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	ml_value_t *Key = Top[-1]->Type->deref(Top[-1]);
	ERROR_CHECK(Key);
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	ml_map_insert(Top[-2], Key, Result);
	*--Top = 0;
	ADVANCE(0);
}

ml_value_t *ml_inst_CLOSURE(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
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

ml_value_t *ml_inst_PARTIAL_NEW(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	*Top++ = ml_partial_function_new(Result, Inst->Params[1].Count);
	ADVANCE(0);
}

ml_value_t *ml_inst_PARTIAL_SET(ml_inst_t *Inst, ml_frame_t *Frame, ml_value_t *Result, ml_value_t **Top) {
	Result = Result->Type->deref(Result);
	ERROR_CHECK(Result);
	ml_partial_function_set(Top[-1], Inst->Params[1].Index, Result);
	ADVANCE(0);
}

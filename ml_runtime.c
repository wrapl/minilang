#include "minilang.h"
#include <gc.h>
#include <string.h>

struct ml_frame_t {
	ml_inst_t *OnError;
	ml_value_t **UpValues;
	ml_value_t **Top;
	ml_value_t *Stack[];
};

void ml_error_trace_add(ml_value_t *Value, ml_source_t Source) {
	ml_error_t *Error = (ml_error_t *)Value;
	for (int I = 0; I < MAX_TRACE; ++I) if (!Error->Trace[I].Name) {
		Error->Trace[I] = Source;
		return;
	}
}

ml_inst_t *mli_push_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	(++Frame->Top)[-1] = Inst->Params[1].Value;
	return Inst->Params[0].Inst;
}

ml_inst_t *mli_pop_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	(--Frame->Top)[0] = 0;
	return Inst->Params[0].Inst;
}

ml_inst_t *mli_pop2_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	(--Frame->Top)[0] = 0;
	(--Frame->Top)[0] = 0;
	return Inst->Params[0].Inst;
}

ml_inst_t *mli_enter_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	for (int I = Inst->Params[1].Count; --I >= 0;) {
		ml_reference_t *Local = xnew(ml_reference_t, 1, ml_value_t *);
		Local->Type = MLReferenceT;
		Local->Address = Local->Value;
		Local->Value[0] = MLNil;
		(++Frame->Top)[-1] = (ml_value_t *)Local;
	}
	return Inst->Params[0].Inst;
}

ml_inst_t *mli_var_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_reference_t *Local = (ml_reference_t *)Frame->Stack[Inst->Params[1].Index];
	ml_value_t *Value = Frame->Top[-1];
	Value = Value->Type->deref(Value);
	if (Value->Type == MLErrorT) {
		ml_error_trace_add(Value, Inst->Source);
		(Frame->Top++)[0] = Value;
		return Frame->OnError;
	}
	Local->Value[0] = Value;
	return Inst->Params[0].Inst;
}

ml_inst_t *mli_def_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Value = Frame->Top[-1];
	Frame->Top[-1] = Value->Type->deref(Value);
	return Inst->Params[0].Inst;
}

ml_inst_t *mli_exit_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Value = Frame->Top[-1];
	for (int I = Inst->Params[1].Count; --I >= 0;) (--Frame->Top)[0] = 0;
	Frame->Top[-1] = Value;
	return Inst->Params[0].Inst;
}

ml_inst_t *mli_try_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	Frame->OnError = Inst->Params[1].Inst;
	return Inst->Params[0].Inst;
}

ml_inst_t *mli_catch_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Error= Frame->Top[-1];
	if (Error->Type != MLErrorT) {
		Frame->Top[-1] = ml_error("InternalError", "expected error value, not %s", Error->Type->Name);
		return Frame->OnError;
	}
	ml_value_t *Value = (ml_value_t *)new(ml_error_t);
	memcpy(Value, Error, sizeof(ml_error_t));
	Value->Type = MLErrorValueT;
	ml_value_t **Top = Frame->Stack + Inst->Params[1].Index;
	while (Frame->Top > Top) (--Frame->Top)[0] = 0;
	Frame->Top[-1] = Value;
	return Inst->Params[0].Inst;
}

ml_inst_t *mli_call_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	int Count = Inst->Params[1].Count;
	ml_value_t *Function = Frame->Top[~Count];
	Function = Function->Type->deref(Function);
	if (Function->Type == MLErrorT) {
		ml_error_trace_add(Function, Inst->Source);
		(Frame->Top++)[0] = Function;
		return Frame->OnError;
	}
	ml_value_t **Args = Frame->Top - Count;
	for (int I = 0; I < Count; ++I) {
		Args[I] = Args[I]->Type->deref(Args[I]);
		if (Args[I]->Type == MLErrorT) {
			ml_error_trace_add(Args[I], Inst->Source);
			(Frame->Top++)[0] = Args[I];
			return Frame->OnError;
		}
	}
	ml_value_t *Result = ml_call(Function, Count, Args);
	for (int I = Count; --I >= 0;) (--Frame->Top)[0] = 0;
	Frame->Top[-1] = Result;
	if (Result->Type == MLErrorT) {
		ml_error_trace_add(Result, Inst->Source);
		return Frame->OnError;
	} else {
		return Inst->Params[0].Inst;
	}
}

ml_inst_t *mli_const_call_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	int Count = Inst->Params[1].Count;
	ml_value_t *Function = Inst->Params[2].Value;
	ml_value_t **Args = Frame->Top - Count;
	for (int I = 0; I < Count; ++I) {
		Args[I] = Args[I]->Type->deref(Args[I]);
		if (Args[I]->Type == MLErrorT) {
			ml_error_trace_add(Args[I], Inst->Source);
			(Frame->Top++)[0] = Args[I];
			return Frame->OnError;
		}
	}
	ml_value_t *Result = ml_call(Function, Count, Args);
	if (Count == 0) {
		++Frame->Top;
	} else {
		for (int I = Count - 1; --I >= 0;) (--Frame->Top)[0] = 0;
	}
	Frame->Top[-1] = Result;
	if (Result->Type == MLErrorT) {
		ml_error_trace_add(Result, Inst->Source);
		return Frame->OnError;
	} else {
		return Inst->Params[0].Inst;
	}
}

ml_inst_t *mli_assign_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Value = Frame->Top[-1];
	(--Frame->Top)[0] = 0;
	Value = Value->Type->deref(Value);
	if (Value->Type == MLErrorT) {
		ml_error_trace_add(Value, Inst->Source);
		(Frame->Top++)[0] = Value;
		return Frame->OnError;
	}
	ml_value_t *Ref = Frame->Top[-1];
	ml_value_t *Result = Frame->Top[-1] = Ref->Type->assign(Ref, Value);
	if (Result->Type == MLErrorT) {
		ml_error_trace_add(Result, Inst->Source);
		return Frame->OnError;
	} else {
		return Inst->Params[0].Inst;
	}
}

ml_inst_t *mli_jump_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	return Inst->Params[0].Inst;
}

ml_inst_t *mli_if_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Value = Frame->Top[-1];
	Value = Value->Type->deref(Value);
	if (Value->Type == MLErrorT) {
		ml_error_trace_add(Value, Inst->Source);
		Frame->Top[-1] = Value;
		return Frame->OnError;
	}
	(--Frame->Top)[0] = 0;
	if (Value == MLNil) {
		return Inst->Params[0].Inst;
	} else {
		return Inst->Params[1].Inst;
	}
}

ml_inst_t *mli_for_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Value = Frame->Top[-1];
	Value = Value->Type->deref(Value);
	if (Value->Type == MLErrorT) {
		ml_error_trace_add(Value, Inst->Source);
		Frame->Top[-1] = Value;
		return Frame->OnError;
	}
	Value = Value->Type->iterate(Value);
	if (Value->Type == MLErrorT) {
		ml_error_trace_add(Value, Inst->Source);
		Frame->Top[-1] = Value;
		return Frame->OnError;
	} else if (Value == MLNil) {
		Frame->Top[-1] = Value;
		return Inst->Params[0].Inst;
	} else {
		Frame->Top[-1] = Value;
		return Inst->Params[1].Inst;
	}
}

ml_inst_t *mli_until_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Value = Frame->Top[-1];
	if (Value == MLNil) {
		return Inst->Params[0].Inst;
	} else {
		return Inst->Params[1].Inst;
	}
}

ml_inst_t *mli_while_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Value = Frame->Top[-1];
	if (Value != MLNil) {
		return Inst->Params[0].Inst;
	} else {
		return Inst->Params[1].Inst;
	}
}

ml_inst_t *mli_and_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Value = Frame->Top[-1];
	Value = Value->Type->deref(Value);
	if (Value->Type == MLErrorT) {
		ml_error_trace_add(Value, Inst->Source);
		Frame->Top[-1] = Value;
		return Frame->OnError;
	} else if (Value == MLNil) {
		return Inst->Params[0].Inst;
	} else {
		(--Frame->Top)[0] = 0;
		return Inst->Params[1].Inst;
	}
}

ml_inst_t *mli_or_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Value = Frame->Top[-1];
	Value = Value->Type->deref(Value);
	if (Value->Type == MLErrorT) {
		ml_error_trace_add(Value, Inst->Source);
		Frame->Top[-1] = Value;
		return Frame->OnError;
	} else if (Value != MLNil) {
		return Inst->Params[0].Inst;
	} else {
		(--Frame->Top)[0] = 0;
		return Inst->Params[1].Inst;
	}
}

ml_inst_t *mli_exists_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Value = Frame->Top[-1];
	if (Value == MLNil) {
		(--Frame->Top)[0] = 0;
		return Inst->Params[0].Inst;
	} else {
		return Inst->Params[1].Inst;
	}
}

ml_inst_t *mli_next_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Iter = Frame->Top[-1];
	Frame->Top[-1] = Iter = Iter->Type->next(Iter);
	if (Iter->Type == MLErrorT) {
		ml_error_trace_add(Iter, Inst->Source);
		return Frame->OnError;
	} else if (Iter == MLNil) {
		return Inst->Params[0].Inst;
	} else {
		return Inst->Params[1].Inst;
	}
}

ml_inst_t *mli_key_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Iter = Frame->Top[-1];
	ml_value_t *Key = (++Frame->Top)[-1] = Iter->Type->key(Iter);
	if (Key->Type == MLErrorT) {
		ml_error_trace_add(Key, Inst->Source);
		return Frame->OnError;
	} else {
		return Inst->Params[0].Inst;
	}
}

ml_inst_t *mli_local_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	int Index = Inst->Params[1].Index;
	if (Index < 0) {
		(++Frame->Top)[-1] = Frame->UpValues[~Index];
	} else {
		(++Frame->Top)[-1] = Frame->Stack[Index];
	}
	return Inst->Params[0].Inst;
}

ml_inst_t *mli_list_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	(++Frame->Top)[-1] = ml_list();
	return Inst->Params[0].Inst;
}

ml_inst_t *mli_append_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	ml_value_t *Value = Frame->Top[-1];
	Value = Value->Type->deref(Value);
	if (Value->Type == MLErrorT) {
		Frame->Top[-1] = Value;
		return Frame->OnError;
	}
	ml_value_t *List = Frame->Top[-2];
	ml_list_append(List, Value);
	return Inst->Params[0].Inst;
}

ml_inst_t *mli_closure_run(ml_inst_t *Inst, ml_frame_t *Frame) {
	// closure <entry> <frame_size> <num_params> <num_upvalues> <upvalue_1> ...
	// TODO: incorporate UpValues into Closure instance hash
	ml_closure_info_t *Info = Inst->Params[1].ClosureInfo;
	ml_closure_t *Closure = xnew(ml_closure_t, Info->NumUpValues, ml_value_t *);
	Closure->Type = MLClosureT;
	Closure->Info = Info;
	for (int I = 0; I < Info->NumUpValues; ++I) {
		int Index = Inst->Params[2 + I].Index;
		if (Index < 0) {
			Closure->UpValues[I] = Frame->UpValues[~Index];
		} else {
			Closure->UpValues[I] = Frame->Stack[Index];
		}
	}
	(++Frame->Top)[-1] = (ml_value_t *)Closure;
	return Inst->Params[0].Inst;
}

static long ml_closure_hash(ml_value_t *Value) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	long Hash = *(long *)Closure->Info->Hash;
	for (int I = 0; I < Closure->Info->NumUpValues; ++I) {
		Hash ^= ml_hash(Closure->UpValues[I]) << I;
	}
	return Hash;
}

ml_value_t *ml_closure_call(ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	ml_closure_info_t *Info = Closure->Info;
	ml_frame_t *Frame = xnew(ml_frame_t, Info->FrameSize, ml_value_t *);
	int NumParams = Info->NumParams;
	int VarArgs = 0;
	if (NumParams < 0) {
		VarArgs = 1;
		NumParams = ~NumParams;
	}
	if (Count > NumParams) Count = NumParams;
	for (int I = 0; I < Count; ++I) {
		ml_reference_t *Local = xnew(ml_reference_t, 1, ml_value_t *);
		Local->Type = MLReferenceT;
		Local->Address = Local->Value;
		ml_value_t *Value = Args[I];
		Local->Value[0] = Value->Type->deref(Value);
		Frame->Stack[I] = (ml_value_t *)Local;
	}
	for (int I = Count; I < NumParams; ++I) {
		ml_reference_t *Local = xnew(ml_reference_t, 1, ml_value_t *);
		Local->Type = MLReferenceT;
		Local->Address = Local->Value;
		Local->Value[0] = MLNil;
		Frame->Stack[I] = (ml_value_t *)Local;
	}
	if (VarArgs) {
		ml_reference_t *Local = xnew(ml_reference_t, 1, ml_value_t *);
		Local->Type = MLReferenceT;
		Local->Address = Local->Value;
		ml_list_t *Rest = new(ml_list_t);
		int Length = 0;
		ml_list_node_t **Next = &Rest->Head;
		ml_list_node_t *Prev = Next[0] = 0;
		for (int I = NumParams; I < Count; ++I) {
			ml_list_node_t *Node = new(ml_list_node_t);
			Node->Prev = Prev;
			Next[0] = Prev = Node;
			Next = &Node->Next;
			++Length;
		}
		Rest->Tail = Prev;
		Rest->Length = Length;
		Local->Value[0] = (ml_value_t *)Rest;
		Frame->Stack[NumParams] = (ml_value_t *)Local;
	}
	Frame->Top = Frame->Stack + NumParams + VarArgs;
	Frame->OnError = NULL;
	Frame->UpValues = Closure->UpValues;
	ml_inst_t *Inst = Closure->Info->Entry;
	while (Inst) Inst = Inst->run(Inst, Frame);
	ml_value_t *Result = Frame->Top[-1];
	return Result->Type->deref(Result);
}

ml_type_t MLClosureT[1] = {{
	MLFunctionT, "closure",
	ml_closure_hash,
	ml_closure_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

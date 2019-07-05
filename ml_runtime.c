#include "minilang.h"
#include "ml_internal.h"
#include "ml_macros.h"
#include "stringmap.h"
#include <gc.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <alloca.h>

struct ml_frame_t {
	ml_inst_t *OnError;
	ml_value_t **UpValues;
	ml_value_t **Top;
	ml_value_t *Stack[];
};

static ml_value_t *ml_frame_run(ml_frame_t *Frame, ml_inst_t *Inst);

struct ml_reference_t {
	const ml_type_t *Type;
	ml_value_t **Address;
	ml_value_t *Value[];
};

static ml_value_t *ml_reference_deref(ml_value_t *Ref) {
	ml_reference_t *Reference = (ml_reference_t *)Ref;
	return Reference->Address[0];
}

static ml_value_t *ml_reference_assign(ml_value_t *Ref, ml_value_t *Value) {
	ml_reference_t *Reference = (ml_reference_t *)Ref;
	return Reference->Address[0] = Value;
}

ml_type_t MLReferenceT[1] = {{
	MLTypeT,
	MLAnyT, "reference",
	ml_default_hash,
	ml_default_call,
	ml_reference_deref,
	ml_reference_assign,
	ml_default_iterate,
	ml_default_current,
	ml_default_next,
	ml_default_key
}};

ml_value_t *ml_reference(ml_value_t **Address) {
	ml_reference_t *Reference;
	if (Address == 0) {
		Reference = xnew(ml_reference_t, 1, ml_value_t *);
		Reference->Address = Reference->Value;
		Reference->Value[0] = MLNil;
	} else {
		Reference = new(ml_reference_t);
		Reference->Address = Address;
	}
	Reference->Type = MLReferenceT;
	return (ml_value_t *)Reference;
}

typedef struct ml_suspend_t {
	const ml_type_t *Type;
	ml_value_t *Key;
	ml_value_t *Value;
	ml_frame_t *Frame;
	ml_inst_t *Inst;
} ml_suspend_t;

static ml_value_t *ml_suspend_current(ml_suspend_t *Suspend) {
	return Suspend->Value;
}

static ml_value_t *ml_suspend_key(ml_suspend_t *Suspend) {
	return Suspend->Key ?: MLSome;
}

static ml_value_t *ml_suspend_next(ml_suspend_t *Suspend) {
	ml_frame_t *Frame = Suspend->Frame;
	ml_inst_t *Inst = Suspend->Inst;
	Frame->Top[-1] = Suspend->Value;
	return ml_frame_run(Frame, Inst);
}

ml_type_t MLSuspendT[1] = {{
	MLTypeT,
	MLAnyT, "suspend",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_iterate,
	(void *)ml_suspend_current,
	(void *)ml_suspend_next,
	(void *)ml_suspend_key
}};

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
	long Hash = *(long *)Closure->Info->Hash;
	for (int I = 0; I < Closure->Info->NumUpValues; ++I) {
		Hash ^= ml_hash_chain(Closure->UpValues[I], Chain) << I;
	}
	return Hash;
}

static ml_value_t *ml_closure_call(ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	ml_closure_info_t *Info = Closure->Info;
	ml_frame_t *Frame = Info->CanSuspend
		? xnew(ml_frame_t, Info->FrameSize, ml_value_t *)
		: (ml_frame_t *)alloca(sizeof(ml_frame_t) + Info->FrameSize * sizeof(ml_value_t *));
	int NumParams = Info->NumParams;
	int VarArgs = 0;
	if (NumParams < 0) {
		VarArgs = 1;
		NumParams = ~NumParams;
	}
	if (Closure->PartialCount) {
		int CombinedCount = Count + Closure->PartialCount;
		ml_value_t **CombinedArgs = anew(ml_value_t *, CombinedCount);
		memcpy(CombinedArgs, Closure->UpValues + Info->NumUpValues, Closure->PartialCount * sizeof(ml_value_t *));
		memcpy(CombinedArgs + Closure->PartialCount, Args, Count * sizeof(ml_value_t *));
		Count = CombinedCount;
		Args = CombinedArgs;
	}
	int Min = (Count < NumParams) ? Count : NumParams;
	for (int I = 0; I < Min; ++I) {
		ml_reference_t *Local = xnew(ml_reference_t, 1, ml_value_t *);
		Local->Type = MLReferenceT;
		Local->Address = Local->Value;
		Local->Value[0] = Args[I];
		Frame->Stack[I] = (ml_value_t *)Local;
	}
	for (int I = Min; I < NumParams; ++I) {
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
		Rest->Type = MLListT;
		int Length = 0;
		ml_list_node_t **Next = &Rest->Head;
		ml_list_node_t *Prev = 0;
		for (int I = NumParams; I < Count; ++I) {
			ml_list_node_t *Node = new(ml_list_node_t);
			Node->Value = Args[I];
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
	Frame->OnError = Info->Return;
	Frame->UpValues = Closure->UpValues;
	return ml_frame_run(Frame, Closure->Info->Entry);
}

static ml_value_t *ml_closure_iterate(ml_value_t *Closure) {
	return ml_closure_call(Closure, 0, NULL);
}

ml_type_t MLClosureT[1] = {{
	MLTypeT,
	MLFunctionT, "closure",
	ml_closure_hash,
	ml_closure_call,
	ml_default_deref,
	ml_default_assign,
	ml_closure_iterate,
	ml_default_current,
	ml_default_next,
	ml_default_key
}};

#define ERROR_CHECK(VALUE) if (VALUE->Type == MLErrorT) { \
	ml_error_trace_add(VALUE, Inst->Source); \
	(Top++)[0] = VALUE; \
	Inst = Frame->OnError; \
	goto *Labels[Inst->Opcode]; \
}

#define ADVANCE(N) { \
	Inst = Inst->Params[N].Inst; \
	goto *Labels[Inst->Opcode]; \
}

#define ERROR() { \
	Inst = Frame->OnError; \
	goto *Labels[Inst->Opcode]; \
}

static ml_value_t *ml_frame_run(ml_frame_t *Frame, ml_inst_t *Inst) {
	static void *Labels[] = {
		[MLI_RETURN] = &&DO_RETURN,
		[MLI_SUSPEND] = &&DO_SUSPEND,
		[MLI_SUSPEND2] = &&DO_SUSPEND2,
		[MLI_PUSH] = &&DO_PUSH,
		[MLI_POP] = &&DO_POP,
		[MLI_POP2] = &&DO_POP2,
		[MLI_POP3] = &&DO_POP3,
		[MLI_ENTER] = &&DO_ENTER,
		[MLI_VAR] = &&DO_VAR,
		[MLI_DEF] = &&DO_DEF,
		[MLI_EXIT] = &&DO_EXIT,
		[MLI_TRY] = &&DO_TRY,
		[MLI_CATCH] = &&DO_CATCH,
		[MLI_CALL] = &&DO_CALL,
		[MLI_CONST_CALL] = &&DO_CONST_CALL,
		[MLI_ASSIGN] = &&DO_ASSIGN,
		[MLI_JUMP] = &&DO_JUMP,
		[MLI_IF] = &&DO_IF,
		[MLI_IF_VAR] = &&DO_IF_VAR,
		[MLI_IF_DEF] = &&DO_IF_DEF,
		[MLI_FOR] = &&DO_FOR,
		[MLI_UNTIL] = &&DO_UNTIL,
		[MLI_WHILE] = &&DO_WHILE,
		[MLI_AND] = &&DO_AND,
		[MLI_AND_VAR] = &&DO_AND_VAR,
		[MLI_AND_DEF] = &&DO_AND_DEF,
		[MLI_OR] = &&DO_OR,
		[MLI_EXISTS] = &&DO_EXISTS,
		[MLI_NEXT] = &&DO_NEXT,
		[MLI_CURRENT] = &&DO_CURRENT,
		[MLI_KEY] = &&DO_KEY,
		[MLI_LOCAL] = &&DO_LOCAL,
		[MLI_LIST] = &&DO_LIST,
		[MLI_APPEND] = &&DO_APPEND,
		[MLI_MAP] = &&DO_MAP,
		[MLI_INSERT] = &&DO_INSERT,
		[MLI_UNIQUE] = &&DO_UNIQUE,
		[MLI_CLOSURE] = &&DO_CLOSURE
	};
	ml_value_t **Stack = Frame->Stack;
	ml_value_t **Top = Frame->Top;
	goto *Labels[Inst->Opcode];

	DO_RETURN: return Top[-1];
	DO_SUSPEND: {
		Frame->Top = Top;
		ml_suspend_t *Suspend = new(ml_suspend_t);
		Suspend->Type = MLSuspendT;
		Suspend->Value = Top[-1];
		Suspend->Key = 0;
		Suspend->Frame = Frame;
		Suspend->Inst = Inst->Params[0].Inst;
		Top[-1] = (ml_value_t *)Suspend;
		return (ml_value_t *)Suspend;
	}
	DO_SUSPEND2: {
		Frame->Top = Top - 1;
		ml_suspend_t *Suspend = new(ml_suspend_t);
		Suspend->Type = MLSuspendT;
		Suspend->Value = Top[-1];
		Suspend->Key = Top[-2];
		Suspend->Frame = Frame;
		Suspend->Inst = Inst->Params[0].Inst;
		Top[-1] = 0;
		Top[-2] = (ml_value_t *)Suspend;
		return (ml_value_t *)Suspend;
	}
	DO_PUSH: {
		*Top++ = Inst->Params[1].Value;
		ADVANCE(0);
	}
	DO_POP: {
		*--Top = 0;
		ADVANCE(0);
	}
	DO_POP2: {
		*--Top = 0;
		*--Top = 0;
		ADVANCE(0);
	}
	DO_POP3: {
		*--Top = 0;
		*--Top = 0;
		*--Top = 0;
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
		ADVANCE(0);
	}
	DO_VAR: {
		ml_reference_t *Local = (ml_reference_t *)Stack[Inst->Params[1].Index];
		ml_value_t *Value = Top[-1];
		Value = Value->Type->deref(Value);
		ERROR_CHECK(Value);
		Local->Value[0] = Value;
		ADVANCE(0);
	}
	DO_DEF: {
		ml_value_t *Value = Top[-1];
		Value = Value->Type->deref(Value);
		ERROR_CHECK(Value);
		Stack[Inst->Params[1].Index] = Value;
		ADVANCE(0);
	}
	DO_EXIT: {
		ml_value_t *Value = Top[-1];
		for (int I = Inst->Params[1].Count; --I >= 0;) *--Top = 0;
		Top[-1] = Value;
		ADVANCE(0);
	}
	DO_TRY: {
		Frame->OnError = Inst->Params[1].Inst;
		ADVANCE(0);
	}
	DO_CATCH: {
		ml_value_t *Error= Top[-1];
		if (Error->Type != MLErrorT) {
			return ml_error("InternalError", "expected error value, not %s", Error->Type->Name);
		}
		Error->Type = MLErrorValueT;
		ml_value_t **Old = Stack + Inst->Params[1].Index;
		while (Top > Old) *--Top = 0;
		Top[-1] = Error;
		ADVANCE(0);
	}
	DO_CALL: {
		int Count = Inst->Params[1].Count;
		ml_value_t *Function = Top[~Count];
		Function = Function->Type->deref(Function);
		ERROR_CHECK(Function);
		ml_value_t **Args = Top - Count;
		for (int I = 0; I < Count; ++I) {
			Args[I] = Args[I]->Type->deref(Args[I]);
			ERROR_CHECK(Args[I]);
		}
		ml_value_t *Result = Function->Type->call(Function, Count, Args);
		for (int I = Count; --I >= 0;) *--Top = 0;
		Top[-1] = Result;
		if (Result->Type == MLErrorT) {
			ml_error_trace_add(Result, Inst->Source);
			ERROR();
		} else {
			ADVANCE(0);
		}
	}
	DO_CONST_CALL: {
		int Count = Inst->Params[1].Count;
		ml_value_t *Function = Inst->Params[2].Value;
		ml_value_t **Args = Top - Count;
		for (int I = 0; I < Count; ++I) {
			Args[I] = Args[I]->Type->deref(Args[I]);
			ERROR_CHECK(Args[I]);
		}
		ml_value_t *Result = Function->Type->call(Function, Count, Args);
		if (Count == 0) {
			++Top;
		} else {
			for (int I = Count - 1; --I >= 0;) *--Top = 0;
		}
		Top[-1] = Result;
		if (Result->Type == MLErrorT) {
			ml_error_trace_add(Result, Inst->Source);
			ERROR();
		} else {
			ADVANCE(0);
		}
	}
	DO_ASSIGN: {
		ml_value_t *Value = Top[-1];
		*--Top = 0;
		Value = Value->Type->deref(Value);
		ERROR_CHECK(Value);
		ml_value_t *Ref = Top[-1];
		ml_value_t *Result = Top[-1] = Ref->Type->assign(Ref, Value);
		if (Result->Type == MLErrorT) {
			ml_error_trace_add(Result, Inst->Source);
			ERROR();
		} else {
			ADVANCE(0);
		}
	}
	DO_JUMP: {
		ADVANCE(0);
	}
	DO_IF: {
		ml_value_t *Value = Top[-1];
		Value = Value->Type->deref(Value);
		ERROR_CHECK(Value);
		*--Top = 0;
		if (Value == MLNil) {
			ADVANCE(0);
		} else {
			ADVANCE(1);
		}
	}
	DO_IF_VAR: {
		ml_value_t *Value = Top[-1];
		Value = Value->Type->deref(Value);
		if (Value->Type == MLErrorT) {
			ml_error_trace_add(Value, Inst->Source);
			Top[-1] = Value;
			ERROR();
		} else if (Value == MLNil) {
			*--Top = 0;
			ADVANCE(0);
		} else {
			ml_reference_t *Local = xnew(ml_reference_t, 1, ml_value_t *);
			Local->Type = MLReferenceT;
			Local->Address = Local->Value;
			Local->Value[0] = Value;
			Top[-1] = (ml_value_t *)Local;
			ADVANCE(1);
		}
	}
	DO_IF_DEF: {
		ml_value_t *Value = Top[-1];
		Value = Value->Type->deref(Value);
		if (Value->Type == MLErrorT) {
			ml_error_trace_add(Value, Inst->Source);
			Top[-1] = Value;
			ERROR();
		} else if (Value == MLNil) {
			*--Top = 0;
			ADVANCE(0);
		} else {
			ADVANCE(1);
		}
	}
	DO_FOR: {
		ml_value_t *Value = Top[-1];
		Value = Value->Type->deref(Value);
		ERROR_CHECK(Value);
		Value = Value->Type->iterate(Value);
		if (Value->Type == MLErrorT) {
			ml_error_trace_add(Value, Inst->Source);
			Top[-1] = Value;
			ERROR();
		} else if (Value == MLNil) {
			Top[-1] = Value;
			ADVANCE(0);
		} else {
			Top[-1] = Value;
			ADVANCE(1);
		}
	}
	DO_UNTIL: {
		ml_value_t *Value = Top[-1];
		if (Value == MLNil) {
			ADVANCE(0);
		} else {
			ADVANCE(1);
		}
	}
	DO_WHILE: {
		ml_value_t *Value = Top[-1];
		if (Value != MLNil) {
			ADVANCE(0);
		} else {
			ADVANCE(1);
		}
	}
	DO_AND: {
		ml_value_t *Value = Top[-1];
		Value = Value->Type->deref(Value);
		if (Value->Type == MLErrorT) {
			ml_error_trace_add(Value, Inst->Source);
			Top[-1] = Value;
			ERROR();
		} else if (Value == MLNil) {
			ADVANCE(0);
		} else {
			*--Top = 0;
			ADVANCE(1);
		}
	}
	DO_AND_VAR: {
		ml_value_t *Value = Top[-1];
		Value = Value->Type->deref(Value);
		if (Value->Type == MLErrorT) {
			ml_error_trace_add(Value, Inst->Source);
			Top[-1] = Value;
			ERROR();
		} else if (Value == MLNil) {
			ADVANCE(0);
		} else {
			ml_reference_t *Local = xnew(ml_reference_t, 1, ml_value_t *);
			Local->Type = MLReferenceT;
			Local->Address = Local->Value;
			Local->Value[0] = Value;
			Top[-1] = (ml_value_t *)Local;
			ADVANCE(1);
		}
	}
	DO_AND_DEF: {
		ml_value_t *Value = Top[-1];
		Value = Value->Type->deref(Value);
		if (Value->Type == MLErrorT) {
			ml_error_trace_add(Value, Inst->Source);
			Top[-1] = Value;
			ERROR();
		} else if (Value == MLNil) {
			ADVANCE(0);
		} else {
			ADVANCE(1);
		}
	}
	DO_OR: {
		ml_value_t *Value = Top[-1];
		Value = Value->Type->deref(Value);
		if (Value->Type == MLErrorT) {
			ml_error_trace_add(Value, Inst->Source);
			Top[-1] = Value;
			ERROR();
		} else if (Value != MLNil) {
			ADVANCE(0);
		} else {
			*--Top = 0;
			ADVANCE(1);
		}
	}
	DO_EXISTS: {
		ml_value_t *Value = Top[-1];
		if (Value == MLNil) {
			*--Top = 0;
			ADVANCE(0);
		} else {
			ADVANCE(1);
		}
	}
	DO_NEXT: {
		ml_value_t *Iter = Top[-1];
		Top[-1] = Iter = Iter->Type->next(Iter);
		if (Iter->Type == MLErrorT) {
			ml_error_trace_add(Iter, Inst->Source);
			ERROR();
		} else if (Iter == MLNil) {
			ADVANCE(0);
		} else {
			ADVANCE(1);
		}
	}
	DO_CURRENT: {
		ml_value_t *Iter = Top[-1];
		ml_value_t *Current = *Top++ = Iter->Type->current(Iter);
		if (Current->Type == MLErrorT) {
			ml_error_trace_add(Current, Inst->Source);
			ERROR();
		} else {
			ADVANCE(0);
		}
	}
	DO_KEY: {
		ml_value_t *Iter = Top[-1];
		ml_value_t *Key = *Top++ = Iter->Type->key(Iter);
		ERROR_CHECK(Key);
		ml_value_t *Current = *Top++ = Iter->Type->current(Iter);
		if (Current->Type == MLErrorT) {
			ml_error_trace_add(Current, Inst->Source);
			ERROR();
		} else {
			ADVANCE(0);
		}
	}
	DO_LOCAL: {
		int Index = Inst->Params[1].Index;
		if (Index < 0) {
			*Top++ = Frame->UpValues[~Index];
		} else {
			*Top++ = Stack[Index];
		}
		ADVANCE(0);
	}
	DO_LIST: {
		*Top++ = ml_list();
		ADVANCE(0);
	}
	DO_APPEND: {
		ml_value_t *Iter = Top[-1];
		ml_value_t *Value = Iter->Type->current(Iter);
		Value = Value->Type->deref(Value);
		ERROR_CHECK(Value);
		ml_value_t *List = Top[-2];
		ml_list_append(List, Value);
		ADVANCE(0);
	}
	DO_MAP: {
		*Top++ = ml_map();
		ADVANCE(0);
	}
	DO_INSERT: {
		ml_value_t *Iter= Top[-1];
		ml_value_t *Key = Iter->Type->key(Iter);
		Key = Key->Type->deref(Key);
		ERROR_CHECK(Key);
		ml_value_t *Value = Iter->Type->current(Iter);
		Value = Value->Type->deref(Value);
		ERROR_CHECK(Value);
		ml_value_t *Map = Top[-2];
		ml_map_insert(Map, Key, Value);
		ADVANCE(0);
	}
	DO_UNIQUE: {
		ml_value_t *Iter = Top[-1];
		ml_value_t *Value = Iter->Type->current(Iter);
		Value = Value->Type->deref(Value);
		ERROR_CHECK(Value);
		ml_value_t *Map = Top[-2];
		ml_map_insert(Map, Value, MLNil);
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
			if (Index < 0) {
				Closure->UpValues[I] = Frame->UpValues[~Index];
			} else {
				Closure->UpValues[I] = Stack[Index];
			}
		}
		*Top++ = (ml_value_t *)Closure;
		ADVANCE(0);
	}
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

static void ml_inst_graph(FILE *Graph, ml_inst_t *Inst, stringmap_t *Done, const char *Colour) {
	char InstName[16];
	sprintf(InstName, "I%x", Inst);
	if (stringmap_search(Done, InstName)) return;
	stringmap_insert(Done, strdup(InstName), Inst);
	switch (Inst->Opcode) {
	case MLI_RETURN: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: return()\"];\n", Inst, Colour, Inst->Source.Line);
		break;
	}
	case MLI_SUSPEND: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: suspend()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_SUSPEND2: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: suspend2()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_PUSH: {
		ml_value_t *StringMethod = ml_method("string");
		ml_value_t *Value = Inst->Params[1].Value;
		if (Value->Type == MLStringT) {
			fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: push(\'", Inst, Colour, Inst->Source.Line);
			ml_inst_escape_string(Graph, ml_string_value(Value), ml_string_length(Value));
			fprintf(Graph, "\')\"];\n");
		} else {
			Value = ml_inline(StringMethod, 1, Value);
			if (Value->Type == MLStringT) {
				fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: push(%s)\"];\n", Inst, Colour, Inst->Source.Line, ml_string_value(Value));
			} else {
				fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: push(%s)\"];\n", Inst, Colour, Inst->Source.Line, Inst->Params[1].Value->Type->Name);
			}
		}
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_POP: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: pop()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_POP2: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: pop2()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_POP3: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: pop3()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_ENTER: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: enter(%d)\"];\n", Inst, Colour, Inst->Source.Line, Inst->Params[1].Count);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_VAR: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: var(%d)\"];\n", Inst, Colour, Inst->Source.Line, Inst->Params[1].Index);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_DEF: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: def(%d)\"];\n", Inst, Colour, Inst->Source.Line, Inst->Params[1].Index);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_EXIT: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: exit(%d)\"];\n", Inst, Colour, Inst->Source.Line, Inst->Params[1].Count);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
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
		asprintf(&TryColour, "%f 0.3 0.9", Hue);
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: try()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		fprintf(Graph, "\tI%x -> I%x [label=\"error\" style=dashed color=red];\n", Inst, Inst->Params[1]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, TryColour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_CATCH: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: catch(%d)\"];\n", Inst, Colour, Inst->Source.Line, Inst->Params[1].Index);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_CALL: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: call(%d)\"];\n", Inst, Colour, Inst->Source.Line, Inst->Params[1].Count);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_CONST_CALL: {
		ml_value_t *StringMethod = ml_method("string");
		ml_value_t *Value = Inst->Params[2].Value;
		if (Value->Type == MLMethodT) {
			fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: const_call(%d, :%s)\"];\n", Inst, Colour, Inst->Source.Line, Inst->Params[1].Count, ml_method_name(Value));
		} else {
			Value = ml_inline(StringMethod, 1, Value);
			if (Value->Type == MLStringT) {
				fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: const_call(%d, %s)\"];\n", Inst, Colour, Inst->Source.Line, Inst->Params[1].Count, ml_string_value(Value));
			} else {
				fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: const_call(%d, %s)\"];\n", Inst, Colour, Inst->Source.Line, Inst->Params[1].Count, Inst->Params[2].Value->Type->Name);
			}
		}
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_ASSIGN: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: assign()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_JUMP: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: jump()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_IF: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: if()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		fprintf(Graph, "\tI%x -> I%x [label=\"not nil\" style=dashed color=blue];\n", Inst, Inst->Params[1]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_IF_VAR: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: if_var()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		fprintf(Graph, "\tI%x -> I%x [label=\"not nil\" style=dashed color=blue];\n", Inst, Inst->Params[1]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_IF_DEF: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: if_def()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		fprintf(Graph, "\tI%x -> I%x [label=\"not nil\" style=dashed color=blue];\n", Inst, Inst->Params[1]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_FOR: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: for()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		fprintf(Graph, "\tI%x -> I%x [label=\"not nil\" style=dashed color=blue];\n", Inst, Inst->Params[1]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_UNTIL: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: until()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		fprintf(Graph, "\tI%x -> I%x [label=\"not nil\" style=dashed color=blue];\n", Inst, Inst->Params[1]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_WHILE: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: while()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		fprintf(Graph, "\tI%x -> I%x [label=\"nil\" style=dashed color=blue];\n", Inst, Inst->Params[1]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_AND: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: and()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		fprintf(Graph, "\tI%x -> I%x [label=\"not nil\" style=dashed color=blue];\n", Inst, Inst->Params[1]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_AND_VAR: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: and_var()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		fprintf(Graph, "\tI%x -> I%x [label=\"not nil\" style=dashed color=blue];\n", Inst, Inst->Params[1]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_AND_DEF: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: and_def()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		fprintf(Graph, "\tI%x -> I%x [label=\"not nil\" style=dashed color=blue];\n", Inst, Inst->Params[1]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_OR: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: or()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		fprintf(Graph, "\tI%x -> I%x [label=\"nil\" style=dashed color=blue];\n", Inst, Inst->Params[1]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_EXISTS: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: exists()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		fprintf(Graph, "\tI%x -> I%x [label=\"not nil\" style=dashed color=blue];\n", Inst, Inst->Params[1]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_NEXT: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: next()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		fprintf(Graph, "\tI%x -> I%x [label=\"not nil\" style=dashed color=blue];\n", Inst, Inst->Params[1]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		ml_inst_graph(Graph, Inst->Params[1].Inst, Done, Colour);
		break;
	}
	case MLI_CURRENT: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: current()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_KEY: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: key()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_LOCAL: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: local(%d)\"];\n", Inst, Colour, Inst->Source.Line, Inst->Params[1].Index);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_LIST: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: list()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_APPEND: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: append()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_MAP: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: map()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_INSERT: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: inesrt()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_UNIQUE: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: UNIQUE()\"];\n", Inst, Colour, Inst->Source.Line);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	case MLI_CLOSURE: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: closure(C%x)\"];\n", Inst, Colour, Inst->Source.Line, Inst->Params[1].ClosureInfo);
		fprintf(Graph, "\tI%x -> I%x;\n", Inst, Inst->Params[0]);
		ml_inst_graph(Graph, Inst->Params[0].Inst, Done, Colour);
		break;
	}
	default: {
		fprintf(Graph, "\tI%x [fillcolor=\"%s\" label=\"%d: unknown(%d)\"];\n", Inst, Colour, Inst->Source.Line, Inst->Opcode);
		break;
	}
	}
}

void ml_closure_info_debug(ml_closure_info_t *Info) {
	stringmap_t Done[1] = {STRINGMAP_INIT};
	char ClosureName[20];
	sprintf(ClosureName, "C%x.dot", Info);
	FILE *Graph = fopen(ClosureName, "w");
	fprintf(Graph, "digraph C%x {\n", Info);
	fprintf(Graph, "\tgraph [compound=true];\n");
	fprintf(Graph, "\tnode [shape=box style=filled];\n");
	ml_inst_graph(Graph, Info->Entry, Done, "lightgray");
	fprintf(Graph, "}\n");
	fclose(Graph);
	printf("Wrote closure to %s\n", ClosureName);
}

void ml_closure_debug(ml_value_t *Value) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	ml_closure_info_debug(Closure->Info);
}

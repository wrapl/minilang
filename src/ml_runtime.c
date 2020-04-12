#include "minilang.h"
#include "ml_macros.h"
#include "stringmap.h"
#include <gc.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include "ml_types.h"

/****************************** Runtime ******************************/

static int MLContextSize = 2;
// Reserved context slots:
//  0: Method Table
//  1: Debugger

ml_context_t MLRootContext = {&MLRootContext, 2, {NULL, NULL}};

ml_context_t *ml_context_new(ml_context_t *Parent) {
	ml_context_t *Context = xnew(ml_context_t, MLContextSize, void *);
	Context->Parent = Parent;
	for (int I = 0; I < Parent->Size; ++I) Context->Values[I] = Parent->Values[I];
	return Context;
}

int ml_context_index_new() {
	return MLContextSize++;
}

void ml_context_set(ml_context_t *Context, int Index, void *Value) {
	if (Context->Size <= Index) return;
	Context->Values[Index] = Value;
}

static void ml_state_call(ml_state_t *Caller, ml_state_t *State, int Count, ml_value_t **Args) {
	return State->run(State, Count ? Args[0] : MLNil);
}

ml_type_t MLStateT[1] = {{
	MLTypeT,
	MLFunctionT, "state",
	ml_default_hash,
	(void *)ml_state_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

static void ml_end_state_run(ml_state_t *State, ml_value_t *Value) {
}

inline ml_value_t *ml_call(ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_value_state_t State[1] = {ML_EVAL_STATE_INIT};
	Value->Type->call(State, Value, Count, Args);
	return State->Value->Type->deref(State->Value);
}

void ml_eval_state_run(ml_value_state_t *State, ml_value_t *Value) {
	State->Value = Value;
}

void ml_call_state_run(ml_value_state_t *State, ml_value_t *Value) {
	if (Value->Type == MLErrorT) {
		State->Value = Value;
	} else {
		State->Base.run = ml_eval_state_run;
		Value->Type->call(State, Value, 0, NULL);
	}
}

typedef struct ml_resumable_state_t {
	ml_state_t Base;
	ml_state_t *Last;
} ml_resumable_state_t;

static void ml_resumable_state_call(ml_state_t *Caller, ml_resumable_state_t *State, int Count, ml_value_t **Args) {
	State->Last->Caller = Caller;
	ML_CONTINUE(State->Base.Caller, Count ? Args[0] : MLNil);
}

ml_type_t MLResumableStateT[1] = {{
	MLTypeT,
	MLStateT, "resumable-state",
	ml_default_hash,
	(void *)ml_resumable_state_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

static void ml_resumable_state_run(ml_resumable_state_t *State, ml_value_t *Value) {
	ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Invalid use of resumable state"));
}

static void ml_callcc_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	if (Count > 1) {
		ML_CHECKX_ARG_TYPE(0, MLStateT);
		ml_state_t *State = (ml_state_t *)Args[0];
		ml_state_t *Last = Caller;
		while (Last && Last->Caller != State) Last = Last->Caller;
		if (!Last) ML_RETURN(ml_error("StateError", "State not in current call chain"));
		Last->Caller = NULL;
		ml_resumable_state_t *Resumable = new(ml_resumable_state_t);
		Resumable->Base.Type = MLResumableStateT;
		Resumable->Base.Caller = Caller;
		Resumable->Base.run = (void *)ml_resumable_state_run;
		Resumable->Base.Context = Caller->Context;
		Resumable->Last = Last;
		ml_value_t *Function = Args[1];
		ml_value_t **Args2 = anew(ml_value_t *, 1);
		Args2[0] = (ml_value_t *)Resumable;
		return Function->Type->call(State, Function, 1, Args2);
	} else {
		ML_CHECKX_ARG_COUNT(1);
		ml_value_t *Function = Args[0];
		ml_value_t **Args2 = anew(ml_value_t *, 1);
		Args2[0] = (ml_value_t *)Caller;
		ml_state_t *State = new(ml_state_t);
		State->run = ml_end_state_run;
		State->Context = Caller->Context;
		return Function->Type->call(State, Function, 1, Args2);
	}
}

static void ml_spawn_state_fn(ml_state_t *State, ml_value_t *Value) {
	ML_CONTINUE(State->Caller, Value);
}

static void ml_spawn_fnx(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_state_t *State = new(ml_state_t);
	State->Type = MLStateT;
	State->Caller = Caller;
	State->run = ml_spawn_state_fn;
	State->Context = Caller->Context;
	ml_value_t *Func = Args[0];
	ml_value_t **Args2 = anew(ml_value_t *, 1);
	Args2[0] = (ml_value_t *)State;
	return Func->Type->call(State, Func, 1, Args2);
}

ml_functionx_t MLCallCC[1] = {{MLFunctionXT, ml_callcc_fnx, NULL}};
ml_functionx_t MLSpawn[1] = {{MLFunctionXT, ml_spawn_fnx, NULL}};

/****************************** Functions ******************************/

static void ml_function_call(ml_state_t *Caller, ml_function_t *Function, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I] = Args[I]->Type->deref(Args[I]);
		if (Arg->Type == MLErrorT) ML_RETURN(Arg);
	}
	ML_RETURN((Function->Callback)(Function->Data, Count, Args));
}

ml_type_t MLFunctionT[1] = {{
	MLTypeT,
	MLIteratableT, "function",
	ml_default_hash,
	(void *)ml_function_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_function(void *Data, ml_callback_t Callback) {
	ml_function_t *Function = fnew(ml_function_t);
	Function->Type = MLFunctionT;
	Function->Data = Data;
	Function->Callback = Callback;
	GC_end_stubborn_change(Function);
	return (ml_value_t *)Function;
}

static void ml_functionx_call(ml_state_t *Caller, ml_functionx_t *Function, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I] = Args[I]->Type->deref(Args[I]);
		if (Arg->Type == MLErrorT) ML_RETURN(Arg);
	}
	return (Function->Callback)(Caller, Function->Data, Count, Args);
}

ml_type_t MLFunctionXT[1] = {{
	MLTypeT,
	MLFunctionT, "functionx",
	ml_default_hash,
	(void *)ml_functionx_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_functionx(void *Data, ml_callbackx_t Callback) {
	ml_functionx_t *Function = fnew(ml_functionx_t);
	Function->Type = MLFunctionXT;
	Function->Data = Data;
	Function->Callback = Callback;
	GC_end_stubborn_change(Function);
	return (ml_value_t *)Function;
}

ML_METHODX("!", MLFunctionT, MLListT) {
	ml_list_t *List = (ml_list_t *)Args[1];
	ml_value_t **ListArgs = anew(ml_value_t *, List->Length);
	ml_value_t **Arg = ListArgs;
	ML_LIST_FOREACH(List, Node) *(Arg++) = Node->Value;
	ml_value_t *Function = Args[0];
	return Function->Type->call(Caller, Function, Arg - ListArgs, ListArgs);
}

ML_METHODX("!", MLFunctionT, MLMapT) {
	ml_map_t *Map = (ml_map_t *)Args[1];
	ml_value_t **ListArgs = anew(ml_value_t *, Map->Size + 1);
	ml_value_t *Names = ml_map();
	Names->Type = MLNamesT;
	ml_value_t **Arg = ListArgs;
	*(Arg++) = Names;
	ML_MAP_FOREACH(Map, Node) {
		ml_value_t *Name = Node->Key;
		if (Name->Type == MLMethodT) {
			ml_map_insert(Names, Name, ml_integer(Arg - ListArgs));
		} else if (Name->Type == MLStringT) {
			ml_map_insert(Names, ml_method(ml_string_value(Name)), ml_integer(Arg - ListArgs));
		} else {
			ML_RETURN(ml_error("TypeError", "Parameter names must be strings or methods"));
		}
		*(Arg++) = Node->Value;
	}
	ml_value_t *Function = Args[0];
	return Function->Type->call(Caller, Function, Arg - ListArgs, ListArgs);
}

ML_METHODX("!", MLFunctionT, MLListT, MLMapT) {
	ml_list_t *List = (ml_list_t *)Args[1];
	ml_map_t *Map = (ml_map_t *)Args[2];
	ml_value_t **ListArgs = anew(ml_value_t *, List->Length + Map->Size + 1);
	ml_value_t **Arg = ListArgs;
	ML_LIST_FOREACH(List, Node) *(Arg++) = Node->Value;
	ml_value_t *Names = ml_map();
	Names->Type = MLNamesT;
	*(Arg++) = Names;
	ML_MAP_FOREACH(Map, Node) {
		ml_value_t *Name = Node->Key;
		if (Name->Type == MLMethodT) {
			ml_map_insert(Names, Name, ml_integer(Arg - ListArgs));
		} else if (Name->Type == MLStringT) {
			ml_map_insert(Names, ml_method(ml_string_value(Name)), ml_integer(Arg - ListArgs));
		} else {
			ML_RETURN(ml_error("TypeError", "Parameter names must be strings or methods"));
		}
		*(Arg++) = Node->Value;
	}
	ml_value_t *Function = Args[0];
	return Function->Type->call(Caller, Function, Arg - ListArgs, ListArgs);
}

ml_value_t *ml_return_nil(void *Data, int Count, ml_value_t **Args) {
	return MLNil;
}

ml_value_t *ml_identity(void *Data, int Count, ml_value_t **Args) {
	return Args[0];
}

typedef struct ml_partial_function_t {
	const ml_type_t *Type;
	ml_value_t *Function;
	int Count, Set;
	ml_value_t *Args[];
} ml_partial_function_t;

static void ml_partial_function_call(ml_state_t *Caller, ml_partial_function_t *Partial, int Count, ml_value_t **Args) {
	int CombinedCount = Count + Partial->Set;
	ml_value_t **CombinedArgs = anew(ml_value_t *, CombinedCount);
	int J = 0;
	for (int I = 0; I < Partial->Count; ++I) {
		ml_value_t *Arg = Partial->Args[I];
		if (Arg) {
			CombinedArgs[I] = Arg;
		} else if (J < Count) {
			CombinedArgs[I] = Args[J++];
		} else {
			CombinedArgs[I] = MLNil;
		}
	}
	memcpy(CombinedArgs + Partial->Count, Args + J, (Count - J) * sizeof(ml_value_t *));
	return Partial->Function->Type->call(Caller, Partial->Function, CombinedCount, CombinedArgs);
}

ml_type_t MLPartialFunctionT[1] = {{
	MLTypeT,
	MLFunctionT, "partial-function",
	ml_default_hash,
	(void *)ml_partial_function_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_partial_function_new(ml_value_t *Function, int Count) {
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, Count, ml_value_t *);
	Partial->Type = MLPartialFunctionT;
	Partial->Function = Function;
	Partial->Count = Count;
	Partial->Set = 0;
	return (ml_value_t *)Partial;
}

ml_value_t *ml_partial_function_set(ml_value_t *Partial, size_t Index, ml_value_t *Value) {
	++((ml_partial_function_t *)Partial)->Set;
	return ((ml_partial_function_t *)Partial)->Args[Index] = Value;
}

ML_METHOD("!!", MLFunctionT, MLListT) {
	ml_list_t *ArgsList = (ml_list_t *)Args[1];
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, ArgsList->Length, ml_value_t *);
	Partial->Type = MLPartialFunctionT;
	Partial->Function = Args[0];
	Partial->Count = Partial->Set = ArgsList->Length;
	ml_value_t **Arg = Partial->Args;
	ML_LIST_FOREACH(ArgsList, Node) *Arg++ = Node->Value;
	return (ml_value_t *)Partial;
}

ML_METHOD("$", MLFunctionT, MLAnyT) {
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, 1, ml_value_t *);
	Partial->Type = MLPartialFunctionT;
	Partial->Function = Args[0];
	Partial->Count = 1;
	Partial->Args[0] = Args[1];
	return (ml_value_t *)Partial;
}

ML_METHOD("$", MLPartialFunctionT, MLAnyT) {
	ml_partial_function_t *Old = (ml_partial_function_t *)Args[0];
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, Old->Count + 1, ml_value_t *);
	Partial->Type = MLPartialFunctionT;
	Partial->Function = Old->Function;
	Partial->Count = Partial->Set = Old->Count + 1;
	memcpy(Partial->Args, Old->Args, Old->Count * sizeof(ml_value_t *));
	Partial->Args[Old->Count] = Args[1];
	return (ml_value_t *)Partial;
}

static void ML_TYPED_FN(ml_iterate, MLPartialFunctionT, ml_state_t *Caller, ml_partial_function_t *Partial) {
	return Partial->Function->Type->call(Caller, Partial->Function, Partial->Count, Partial->Args);
}

/****************************** References ******************************/

static long ml_reference_hash(ml_value_t *Ref, ml_hash_chain_t *Chain) {
	ml_reference_t *Reference = (ml_reference_t *)Ref;
	ml_value_t *Value = Reference->Address[0];
	return Value->Type->hash(Value, Chain);
}

static ml_value_t *ml_reference_deref(ml_value_t *Ref) {
	ml_reference_t *Reference = (ml_reference_t *)Ref;
	return Reference->Address[0];
}

static ml_value_t *ml_reference_assign(ml_value_t *Ref, ml_value_t *Value) {
	Value = Value->Type->deref(Value);
	if (Value->Type == MLErrorT) return Value;
	ml_reference_t *Reference = (ml_reference_t *)Ref;
	return Reference->Address[0] = Value;
}

ml_type_t MLReferenceT[1] = {{
	MLTypeT,
	MLAnyT, "reference",
	ml_reference_hash,
	ml_default_call,
	ml_reference_deref,
	ml_reference_assign,
	NULL, 0, 0
}};

inline ml_value_t *ml_reference(ml_value_t **Address) {
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

ml_type_t MLUninitializedT[] = {{
	MLTypeT,
	MLAnyT, "uninitialized",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

/****************************** Errors ******************************/

#define MAX_TRACE 16

struct ml_error_t {
	const ml_type_t *Type;
	const char *Error;
	const char *Message;
	ml_source_t Trace[MAX_TRACE];
};

static void ml_error_call(ml_state_t *Caller, ml_error_t *Error, int Count, ml_value_t **Args) {
	ml_error_trace_add(Error, (ml_source_t){__FILE__, __LINE__});
	ML_RETURN(Error);
}

ml_type_t MLErrorT[1] = {{
	MLTypeT,
	MLAnyT, "error",
	ml_default_hash,
	ml_error_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_type_t MLErrorValueT[1] = {{
	MLTypeT,
	MLErrorT, "error_value",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_error(const char *Error, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Message;
	vasprintf(&Message, Format, Args);
	va_end(Args);
	ml_error_t *Value = new(ml_error_t);
	Value->Type = MLErrorT;
	Value->Error = Error;
	Value->Message = Message;
	memset(Value->Trace, 0, sizeof(Value->Trace));
	return (ml_value_t *)Value;
}

const char *ml_error_type(ml_value_t *Value) {
	return ((ml_error_t *)Value)->Error;
}

const char *ml_error_message(ml_value_t *Value) {
	return ((ml_error_t *)Value)->Message;
}

int ml_error_trace(ml_value_t *Value, int Level, const char **Source, int *Line) {
	ml_error_t *Error = (ml_error_t *)Value;
	if (Level >= MAX_TRACE) return 0;
	if (!Error->Trace[Level].Name) return 0;
	Source[0] = Error->Trace[Level].Name;
	Line[0] = Error->Trace[Level].Line;
	return 1;
}

void ml_error_trace_add(ml_value_t *Value, ml_source_t Source) {
	ml_error_t *Error = (ml_error_t *)Value;
	for (int I = 0; I < MAX_TRACE; ++I) if (!Error->Trace[I].Name) {
		Error->Trace[I] = Source;
		return;
	}
}

void ml_error_print(ml_value_t *Value) {
	ml_error_t *Error = (ml_error_t *)Value;
	printf("Error: %s\n", Error->Message);
	for (int I = 0; (I < MAX_TRACE) && Error->Trace[I].Name; ++I) {
		printf("\t%s:%d\n", Error->Trace[I].Name, Error->Trace[I].Line);
	}
}

ML_METHOD("type", MLErrorT) {
	return ml_string(((ml_error_t *)Args[0])->Error, -1);
}

ML_METHOD("message", MLErrorT) {
	return ml_string(((ml_error_t *)Args[0])->Message, -1);
}

void ml_runtime_init() {
#include "ml_runtime_init.c"
}

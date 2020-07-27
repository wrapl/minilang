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

static int MLContextSize = 4;
// Reserved context slots:
//  0: Method Table
//  1: Context variables
//  2: Debugger
//	3: Scheduler

static unsigned int DefaultCounter = UINT_MAX;

static void default_swap(ml_state_t *State, ml_value_t *Value) {
	DefaultCounter = UINT_MAX;
	return State->run(State, Value);
}

static ml_schedule_t default_scheduler(ml_context_t *Context) {
	return (ml_schedule_t){&DefaultCounter, default_swap};
}

ml_context_t MLRootContext = {&MLRootContext, 4, {
	NULL,
	NULL,
	NULL,
	default_scheduler
}};

ml_context_t *ml_context_new(ml_context_t *Parent) {
	ml_context_t *Context = xnew(ml_context_t, MLContextSize, void *);
	Context->Parent = Parent;
	Context->Size = MLContextSize;
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

#define ML_VARIABLES_INDEX 1

typedef struct  {
	const ml_type_t *Type;
} ml_context_key_t;

typedef struct ml_context_value_t ml_context_value_t;

struct ml_context_value_t {
	ml_context_value_t *Prev;
	ml_context_key_t *Key;
	ml_value_t *Value;
};

static void ml_context_key_call(ml_state_t *Caller, ml_context_key_t *Key, int Count, ml_value_t **Args) {
	ml_context_value_t *Values = Caller->Context->Values[ML_VARIABLES_INDEX];
	if (Count == 0) {
		while (Values) {
			if (Values->Key == Key) ML_RETURN(Values->Value);
		}
		ML_RETURN(MLNil);
	} else if (Count == 1) {
		ML_RETURN(ml_error("CallError", "Context key requires exactly 0 or >2 arguments"));
	} else {
		ml_context_value_t *Value = new(ml_context_value_t);
		Value->Prev = Values;
		Value->Key = Key;
		Value->Value = Args[0];
		ml_context_t *Context = ml_context_new(Caller->Context);
		ml_context_set(Context, ML_VARIABLES_INDEX, Value);
		ml_state_t *State = new(ml_state_t);
		State->Caller = Caller;
		State->run = ml_default_state_run;
		State->Context = Context;
		ml_value_t *Function = Args[1];
		Function = ml_deref(Function);
		return ml_typeof(Function)->call(State, Function, Count - 2, Args + 2);
	}
}

ML_TYPE(MLContextKeyT, (MLCFunctionT), "context-key",
	.call = (void *)ml_context_key_call
);

ml_value_t *ml_context_key() {
	ml_context_key_t *Key = new(ml_context_key_t);
	Key->Type = MLContextKeyT;
	return (ml_value_t *)Key;
}

ML_FUNCTION(MLContextKey) {
	return ml_context_key();
}

static void ml_state_call(ml_state_t *Caller, ml_state_t *State, int Count, ml_value_t **Args) {
	return State->run(State, Count ? Args[0] : MLNil);
}

ML_TYPE(MLStateT, (MLFunctionT), "state",
	.call = (void *)ml_state_call
);

static void ml_end_state_run(ml_state_t *State, ml_value_t *Value) {
}

inline ml_value_t *ml_call(ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_value_state_t State[1] = {ML_EVAL_STATE_INIT};
	ml_typeof(Value)->call((ml_state_t *)State, Value, Count, Args);
	return ml_typeof(State->Value)->deref(State->Value);
}

void ml_default_state_run(ml_state_t *State, ml_value_t *Value) {
	ML_CONTINUE(State->Caller, Value);
}

void ml_eval_state_run(ml_value_state_t *State, ml_value_t *Value) {
	State->Value = Value;
}

void ml_call_state_run(ml_value_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		State->Value = Value;
	} else {
		State->Base.run = (ml_state_fn)ml_eval_state_run;
		ml_typeof(Value)->call((ml_state_t *)State, Value, 0, NULL);
	}
}

ml_state_t *ml_state_new(ml_state_t *Caller) {
	ml_state_t *State = new(ml_state_t);
	State->Type = MLStateT;
	State->Context = ml_context_new(Caller->Context);
	State->Caller = Caller;
	State->run = ml_default_state_run;
	return State;
}

typedef struct ml_resumable_state_t {
	ml_state_t Base;
	ml_state_t *Last;
} ml_resumable_state_t;

static void ml_resumable_state_call(ml_state_t *Caller, ml_resumable_state_t *State, int Count, ml_value_t **Args) {
	State->Last->Caller = Caller;
	ML_CONTINUE(State->Base.Caller, Count ? Args[0] : MLNil);
}

ML_TYPE(MLResumableStateT, (MLStateT), "resumable-state",
	.call = (void *)ml_resumable_state_call
);

static void ml_resumable_state_run(ml_resumable_state_t *State, ml_value_t *Value) {
	ML_CONTINUE(State->Base.Caller, ml_error("StateError", "Invalid use of resumable state"));
}

ML_FUNCTIONX(MLCallCC) {
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
		return ml_typeof(Function)->call(State, Function, 1, Args2);
	} else {
		ML_CHECKX_ARG_COUNT(1);
		ml_value_t *Function = Args[0];
		ml_value_t **Args2 = anew(ml_value_t *, 1);
		Args2[0] = (ml_value_t *)Caller;
		ml_state_t *State = new(ml_state_t);
		State->run = ml_end_state_run;
		State->Context = Caller->Context;
		return ml_typeof(Function)->call(State, Function, 1, Args2);
	}
}

ML_FUNCTIONX(MLMark) {
	ML_CHECKX_ARG_COUNT(1);
	ml_state_t *State = new(ml_state_t);
	State->Type = MLStateT;
	State->Caller = Caller;
	State->run = ml_default_state_run;
	State->Context = Caller->Context;
	ml_value_t *Func = Args[0];
	ml_value_t **Args2 = anew(ml_value_t *, 1);
	Args2[0] = (ml_value_t *)State;
	return ml_typeof(Func)->call(State, Func, 1, Args2);
}

/****************************** References ******************************/

static long ml_reference_hash(ml_value_t *Ref, ml_hash_chain_t *Chain) {
	ml_reference_t *Reference = (ml_reference_t *)Ref;
	ml_value_t *Value = Reference->Address[0];
	return ml_typeof(Value)->hash(Value, Chain);
}

static ml_value_t *ml_reference_deref(ml_value_t *Ref) {
	ml_reference_t *Reference = (ml_reference_t *)Ref;
	return Reference->Address[0];
}

static ml_value_t *ml_reference_assign(ml_value_t *Ref, ml_value_t *Value) {
	ml_reference_t *Reference = (ml_reference_t *)Ref;
	return Reference->Address[0] = Value;
}

ML_TYPE(MLReferenceT, (), "reference",
	.hash = ml_reference_hash,
	.deref = ml_reference_deref,
	.assign = ml_reference_assign
);

inline ml_value_t *ml_reference(ml_value_t **Address) {
	ml_reference_t *Reference;
	Reference = new(ml_reference_t);
	Reference->Address = Address;
	Reference->Type = MLReferenceT;
	return (ml_value_t *)Reference;
}

typedef struct ml_uninitialized_slot_t ml_uninitialized_slot_t;

struct ml_uninitialized_slot_t {
	ml_uninitialized_slot_t *Next;
	ml_value_t **Value;
};

typedef struct ml_uninitialized_t {
	const ml_type_t *Type;
	ml_uninitialized_slot_t *Slots;
	stringmap_t Unresolved[1];
} ml_uninitialized_t;

ML_TYPE(MLUninitializedT, (), "uninitialized");

ml_value_t *ml_uninitialized() {
	ml_uninitialized_t *Uninitialized = new(ml_uninitialized_t);
	Uninitialized->Type = MLUninitializedT;
	return (ml_value_t *)Uninitialized;
}

void ml_uninitialized_use(ml_value_t *Uninitialized0, ml_value_t **Value) {
	ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Uninitialized0;
	ml_uninitialized_slot_t *Slot = new(ml_uninitialized_slot_t);
	Slot->Value = Value;
	Slot->Next = Uninitialized->Slots;
	Uninitialized->Slots = Slot;
}

static ML_METHOD_DECL(Symbol, "::");

static int ml_uninitialized_resolve(const char *Name, ml_uninitialized_t *Unitialized, ml_value_t *Value) {
	ml_value_t *Args[2] = {Value, ml_string(Name, -1)};
	ml_value_t *Resolved = ml_call(SymbolMethod, 2, Args);
	ml_uninitialized_set((ml_value_t *)Unitialized, Resolved);
	return 0;
}

void ml_uninitialized_set(ml_value_t *Uninitialized0, ml_value_t *Value) {
	ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Uninitialized0;
	for (ml_uninitialized_slot_t *Slot = Uninitialized->Slots; Slot; Slot = Slot->Next) Slot->Value[0] = Value;
	stringmap_foreach(Uninitialized->Unresolved, Value, (void *)ml_uninitialized_resolve);
}

ML_METHOD("::", MLUninitializedT, MLStringT) {
	ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Uninitialized->Unresolved, Name);
	if (!Slot[0]) Slot[0] = ml_uninitialized();
	return Slot[0];
}

/****************************** Errors ******************************/

#define MAX_TRACE 16

struct ml_error_t {
	const ml_type_t *Type;
	const char *Error;
	const char *Message;
	ml_source_t Trace[MAX_TRACE];
};

static ml_value_t *ml_error_assign(ml_value_t *Error, ml_value_t *Value) {
	return Error;
}

static void ml_error_call(ml_state_t *Caller, ml_value_t *Error, int Count, ml_value_t **Args) {
	ML_RETURN(Error);
}

ML_TYPE(MLErrorT, (), "error",
	.assign = ml_error_assign,
	.call = ml_error_call
);

ML_TYPE(MLErrorValueT, (MLErrorT), "error_value");

ml_value_t *ml_errorv(const char *Error, const char *Format, va_list Args) {
	char *Message;
	vasprintf(&Message, Format, Args);
	ml_error_t *Value = new(ml_error_t);
	Value->Type = MLErrorT;
	Value->Error = Error;
	Value->Message = Message;
	memset(Value->Trace, 0, sizeof(Value->Trace));
	return (ml_value_t *)Value;
}

ml_value_t *ml_error(const char *Error, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	ml_value_t *Value = ml_errorv(Error, Format, Args);
	va_end(Args);
	return Value;
}

const char *ml_error_type(ml_value_t *Value) {
	return ((ml_error_t *)Value)->Error;
}

const char *ml_error_message(ml_value_t *Value) {
	return ((ml_error_t *)Value)->Message;
}

int ml_error_source(ml_value_t *Value, int Level, ml_source_t *Source) {
	ml_error_t *Error = (ml_error_t *)Value;
	if (Level >= MAX_TRACE) return 0;
	if (!Error->Trace[Level].Name) return 0;
	Source[0] = Error->Trace[Level];
	return 1;
}

ml_value_t *ml_error_trace_add(ml_value_t *Value, ml_source_t Source) {
	ml_error_t *Error = (ml_error_t *)Value;
	for (int I = 0; I < MAX_TRACE; ++I) if (!Error->Trace[I].Name) {
		Error->Trace[I] = Source;
		break;
	}
	return Value;
}

void ml_error_print(ml_value_t *Value) {
	ml_error_t *Error = (ml_error_t *)Value;
	printf("Error: %s\n", Error->Message);
	for (int I = 0; (I < MAX_TRACE) && Error->Trace[I].Name; ++I) {
		printf("\t%s:%d\n", Error->Trace[I].Name, Error->Trace[I].Line);
	}
}

void ml_error_fprint(FILE *File, ml_value_t *Value) {
	ml_error_t *Error = (ml_error_t *)Value;
	fprintf(File, "Error: %s\n", Error->Message);
	for (int I = 0; (I < MAX_TRACE) && Error->Trace[I].Name; ++I) {
		fprintf(File, "\t%s:%d\n", Error->Trace[I].Name, Error->Trace[I].Line);
	}
}

ML_METHOD("type", MLErrorT) {
	return ml_string(((ml_error_t *)Args[0])->Error, -1);
}

ML_METHOD("message", MLErrorT) {
	return ml_string(((ml_error_t *)Args[0])->Message, -1);
}

ML_METHOD("trace", MLErrorT) {
	ml_value_t *Trace = ml_list();
	ml_source_t Source;
	for (int I = 0; ml_error_source(Args[0], I, &Source); ++I) {
		ml_value_t *Tuple = ml_tuple(2);
		ml_tuple_set(Tuple, 1, ml_string(Source.Name, -1));
		ml_tuple_set(Tuple, 2, ml_integer(Source.Line));
		ml_list_put(Trace, Tuple);
	}
	return Trace;
}

/****************************** Debugging ******************************/

int ml_debugger_check(ml_state_t *State) {
	if (!State || !State->Type) return 0;
	typeof(ml_debugger_check) *function = ml_typed_fn_get(State->Type, ml_debugger_check);
	if (function) return function(State);
	return 0;
}

ml_source_t ml_debugger_source(ml_state_t *State) {
	if (!State || !State->Type) return (ml_source_t){"<unknown>", 0};
	typeof(ml_debugger_source) *function = ml_typed_fn_get(State->Type, ml_debugger_source);
	if (function) return function(State);
	return (ml_source_t){"<unknown>", 0};
}

ml_decl_t *ml_debugger_decls(ml_state_t *State) {
	if (!State || !State->Type) return NULL;
	typeof(ml_debugger_decls) *function = ml_typed_fn_get(State->Type, ml_debugger_decls);
	if (function) return function(State);
	return NULL;
}

ml_value_t *ml_debugger_local(ml_state_t *State, int Index) {
	if (!State || !State->Type) return ml_error("DebugError", "Locals not available");
	typeof(ml_debugger_local) *function = ml_typed_fn_get(State->Type, ml_debugger_local);
	if (function) return function(State, Index);
	return ml_error("DebugError", "Locals not available");
}

void ml_runtime_init() {
#include "ml_runtime_init.c"
}

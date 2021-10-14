#include "minilang.h"
#include "ml_macros.h"
#include "stringmap.h"
#include <gc.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "ml_types.h"

#undef ML_CATEGORY
#define ML_CATEGORY "runtime"

// Runtime //

#ifndef ML_THREADSAFE

ml_value_t *MLArgCache[ML_ARG_CACHE_SIZE];

#endif

static int MLContextSize = 5;
// Reserved context slots:
//  0: Method Table
//  1: Context variables
//  2: Debugger
//	3: Scheduler
//	4: Module Path

static uint64_t DefaultCounter = UINT_MAX;

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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
	Context->Values[Index] = Value;
#pragma GCC diagnostic pop
}

#define ML_VARIABLES_INDEX 1

typedef struct  {
	ml_type_t *Type;
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
		ML_ERROR("CallError", "Context key requires exactly 0 or >2 arguments");
	} else {
		ml_context_value_t *Value = new(ml_context_value_t);
		Value->Prev = Values;
		Value->Key = Key;
		Value->Value = Args[0];
		ml_state_t *State = ml_state_new(Caller);
		ml_context_set(State->Context, ML_VARIABLES_INDEX, Value);
		ml_value_t *Function = Args[1];
		Function = ml_deref(Function);
		return ml_call(State, Function, Count - 2, Args + 2);
	}
}

ML_FUNCTION(MLContextKey) {
//!context
//@context
//>context
// Creates a new context specific key.
	ml_context_key_t *Key = new(ml_context_key_t);
	Key->Type = MLContextKeyT;
	return (ml_value_t *)Key;
}

ML_TYPE(MLContextKeyT, (MLCFunctionT), "context",
//!context
//@context
// A context key can be used to create context specific values.
// If :mini:`key` is a context key, then calling :mini:`key()` no arguments returns the value associated with the key in the current context, or :mini:`nil` is no value is associated.
// Calling :mini:`key(Value, Function)` will invoke :mini:`Function` in a new context where :mini:`key` is associated with :mini:`Value`.
	.call = (void *)ml_context_key_call,
	.Constructor = (ml_value_t *)MLContextKey
);

static void ml_state_call(ml_state_t *Caller, ml_state_t *State, int Count, ml_value_t **Args) {
	State->run(State, Count ? Args[0] : MLNil);
	ML_RETURN(MLNil);
}

ML_TYPE(MLStateT, (MLFunctionT), "state",
	.call = (void *)ml_state_call
);

static void ml_end_state_run(ml_state_t *State, ml_value_t *Value) {
}

void ml_default_state_run(ml_state_t *State, ml_value_t *Value) {
	ML_CONTINUE(State->Caller, Value);
}

static void ml_main_state_run(ml_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) {
		printf("%s: %s\n", ml_error_type(Result), ml_error_message(Result));
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Result, Level++, &Source)) {
			printf("\t%s:%d\n", Source.Name, Source.Line);
		}
		exit(1);
	}
}

ml_state_t MLMain[1] = {{MLStateT, NULL, ml_main_state_run, &MLRootContext}};

static void ml_call_state_run(ml_call_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		ML_CONTINUE(State->Base.Caller, Value);
	} else {
		ml_call(State->Base.Caller, Value, State->Count, State->Args);
	}
}

ml_call_state_t *ml_call_state_new(ml_state_t *Caller, int Count) {
	ml_call_state_t *State = xnew(ml_call_state_t, Count, ml_value_t *);
	State->Base.Type = MLStateT;
	State->Base.run = (ml_state_fn)ml_call_state_run;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Count = Count;
	return State;
}

void ml_result_state_run(ml_result_state_t *State, ml_value_t *Value) {
	State->Value = ml_deref(Value);
}

ml_result_state_t *ml_result_state_new(ml_context_t *Context) {
	ml_result_state_t *State = new(ml_result_state_t);
	State->Base.Context = Context ?: &MLRootContext;
	State->Value = MLNil;
	State->Base.Type = MLStateT;
	State->Base.run = (ml_state_fn)ml_result_state_run;
	return State;
}

ml_value_t *ml_simple_call(ml_value_t *Value, int Count, ml_value_t **Args) {
	static ml_result_state_t State = {{MLStateT, NULL, (void *)ml_result_state_run, &MLRootContext}, MLNil};
	ml_call(&State, Value, Count, Args);
	ml_value_t *Result = State.Value;
	State.Value = MLNil;
	return Result;
}

typedef struct {
	ml_state_t Base;
	ml_context_t Context[1];
} ml_context_state_t;

ml_state_t *ml_state_new(ml_state_t *Caller) {
	ml_context_state_t *State = xnew(ml_context_state_t, MLContextSize, void *);
	ml_context_t *Parent = Caller->Context;
	State->Context->Parent = Parent;
	State->Context->Size = MLContextSize;
	for (int I = 0; I < Parent->Size; ++I) State->Context->Values[I] = Parent->Values[I];
	State->Base.Caller = Caller;
	State->Base.run = ml_default_state_run;
	State->Base.Context = State->Context;
	return (ml_state_t *)State;
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
//@callcc
	if (!Caller->Type) Caller->Type = MLStateT;
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Function = Args[0];
	Args[0] = (ml_value_t *)Caller;
	ml_state_t *State = new(ml_state_t);
	State->run = ml_end_state_run;
	State->Context = Caller->Context;
	return ml_call(State, Function, Count, Args);
}

ML_FUNCTIONX(MLMarkCC) {
//@markcc
	ML_CHECKX_ARG_COUNT(1);
	ml_state_t *State = new(ml_state_t);
	State->Type = MLStateT;
	State->Caller = Caller;
	State->run = ml_default_state_run;
	State->Context = Caller->Context;
	ml_value_t *Func = Args[0];
	Args[0] = (ml_value_t *)State;
	return ml_call(State, Func, 1, Args);
}

ML_FUNCTIONX(MLCallDC) {
//@calldc
	if (!Caller->Type) Caller->Type = MLStateT;
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLStateT);
	ml_state_t *State = (ml_state_t *)Args[0];
	ml_state_t *Last = Caller;
	while (Last && Last->Caller != State) Last = Last->Caller;
	if (!Last) ML_ERROR("StateError", "State not in current call chain");
	Last->Caller = NULL;
	ml_resumable_state_t *Resumable = new(ml_resumable_state_t);
	Resumable->Base.Type = MLResumableStateT;
	Resumable->Base.Caller = Caller;
	Resumable->Base.run = (void *)ml_resumable_state_run;
	Resumable->Base.Context = Caller->Context;
	Resumable->Last = Last;
	ml_value_t *Function = Args[1];
	Args[0] = (ml_value_t *)Resumable;
	return ml_call(State, Function, 1, Args);
}

ML_FUNCTIONX(MLSwapCC) {
//@swapcc
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLStateT);
	ml_state_t *State = (ml_state_t *)Args[0];
	return State->run(State, Count > 1 ? Args[1] : MLNil);
}

// References //

static long ml_reference_hash(ml_reference_t *Reference, ml_hash_chain_t *Chain) {
	return ml_hash_chain(Reference->Address[0], Chain);
}

static ml_value_t *ml_reference_deref(ml_reference_t *Reference) {
	return Reference->Address[0];
}

static ml_value_t *ml_reference_assign(ml_reference_t *Reference, ml_value_t *Value) {
	return Reference->Address[0] = Value;
}

static void ml_reference_call(ml_state_t *Caller, ml_reference_t *Reference, int Count, ml_value_t **Args) {
	return ml_call(Caller, Reference->Address[0], Count, Args);
}

ML_TYPE(MLReferenceT, (), "reference",
	.hash = (void *)ml_reference_hash,
	.deref = (void *)ml_reference_deref,
	.assign = (void *)ml_reference_assign,
	.call = (void *)ml_reference_call
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
	ml_type_t *Type;
	const char *Name;
	ml_uninitialized_slot_t *Slots;
	stringmap_t Unresolved[1];
} ml_uninitialized_t;

static void ml_uninitialized_call(ml_state_t *Caller, ml_uninitialized_t *Uninitialized, int Count, ml_value_t **Args) {
	ML_ERROR("ValueError", "%s is uninitialized", Uninitialized->Name);
}

static ml_value_t *ml_unitialized_assign(ml_uninitialized_t *Uninitialized, ml_value_t *Value) {
	return ml_error("ValueError", "%s is uninitialized", Uninitialized->Name);
}

ML_TYPE(MLUninitializedT, (), "uninitialized",
	.call = (void *)ml_uninitialized_call,
	.assign = (void *)ml_unitialized_assign
);

ml_value_t *ml_uninitialized(const char *Name) {
	ml_uninitialized_t *Uninitialized = new(ml_uninitialized_t);
	Uninitialized->Type = MLUninitializedT;
	Uninitialized->Name = Name;
	return (ml_value_t *)Uninitialized;
}

const char *ml_uninitialized_name(ml_value_t *Uninitialized) {
	return ((ml_uninitialized_t *)Uninitialized)->Name;
}

void ml_uninitialized_use(ml_value_t *Uninitialized0, ml_value_t **Value) {
	ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Uninitialized0;
	ml_uninitialized_slot_t *Slot = new(ml_uninitialized_slot_t);
	Slot->Value = Value;
	Slot->Next = Uninitialized->Slots;
	Uninitialized->Slots = Slot;
}

static ML_METHOD_DECL(SymbolMethod, "::");

static int ml_uninitialized_resolve(const char *Name, ml_uninitialized_t *Unitialized, ml_value_t *Value) {
	ml_value_t *Result = ml_simple_inline(SymbolMethod, 2, Value, ml_string(Name, -1));
	ml_uninitialized_set((ml_value_t *)Unitialized, Result);
	return 0;
}

static void ml_uninitialized_transfer(ml_uninitialized_t *Uninitialized, ml_uninitialized_t *Uninitialized2);

static int ml_uninitialized_transfer_import(const char *Name, ml_uninitialized_t *Uninitialized, ml_uninitialized_t *Uninitialized2) {
	ml_uninitialized_t **Slot = (ml_uninitialized_t **)stringmap_slot(Uninitialized2->Unresolved, Name);
	if (Slot[0]) {
		ml_uninitialized_transfer(Uninitialized, Slot[0]);
	} else {
		Slot[0] = Uninitialized;
	}
	return 0;
}

static void ml_uninitialized_transfer(ml_uninitialized_t *Uninitialized, ml_uninitialized_t *Uninitialized2) {
	ml_uninitialized_slot_t *Slot = Uninitialized2->Slots;
	if (Slot) {
		while (Slot->Next) Slot = Slot->Next;
		Slot->Next = Uninitialized->Slots;
	} else {
		Uninitialized2->Slots = Uninitialized->Slots;
	}
	stringmap_foreach(Uninitialized->Unresolved, Uninitialized2, (void *)ml_uninitialized_transfer_import);
}

void ml_uninitialized_set(ml_value_t *Uninitialized0, ml_value_t *Value) {
	ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Uninitialized0;
	if (ml_typeof(Value) == MLUninitializedT) {
		ml_uninitialized_transfer(Uninitialized, (ml_uninitialized_t *)Value);
	} else {
		for (ml_uninitialized_slot_t *Slot = Uninitialized->Slots; Slot; Slot = Slot->Next) Slot->Value[0] = Value;
		stringmap_foreach(Uninitialized->Unresolved, Value, (void *)ml_uninitialized_resolve);
	}
}

ML_METHOD("::", MLUninitializedT, MLStringT) {
	ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Uninitialized->Unresolved, Name);
	if (!Slot[0]) Slot[0] = ml_uninitialized(Name);
	return Slot[0];
}

// Errors //

#define MAX_TRACE 16

typedef struct {
	ml_type_t *Type;
	const char *Error;
	const char *Message;
	ml_source_t Trace[MAX_TRACE];
} ml_error_value_t;

typedef struct {
	ml_type_t *Type;
	ml_value_t *Value;
	ml_error_value_t Error[];
} ml_error_t;

static ml_value_t *ml_error_assign(ml_value_t *Error, ml_value_t *Value) {
	return Error;
}

static void ml_error_call(ml_state_t *Caller, ml_value_t *Error, int Count, ml_value_t **Args) {
	ML_RETURN(Error);
}

ML_FUNCTION(MLError) {
//!error
//@error
//<Type
//<Message
//>error
// Creates an error exception with type :mini:`Type` and message :mini:`Message`. Since this creates an exception, it will trigger the current exception handler.
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLStringT);
	ml_error_t *Error = xnew(ml_error_t, 1, ml_error_value_t);
	Error->Type = MLErrorT;
	Error->Error->Type = MLErrorValueT;
	Error->Error->Error = ml_string_value(Args[0]);
	Error->Error->Message = ml_string_value(Args[1]);
	Error->Value = (ml_value_t *)Error->Error;
	return (ml_value_t *)Error;
}

ML_FUNCTION(MLRaise) {
//!error
//@raise
//<Type
//<Value
//>error
// Creates a general exception with type :mini:`Type` and value :mini:`Value`. Since this creates an exception, it will trigger the current exception handler.
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_error_t *Error = new(ml_error_t);
	Error->Type = MLErrorT;
	Error->Error->Type = MLErrorValueT;
	Error->Error->Error = ml_string_value(Args[0]);
	Error->Error->Message = ml_typeof(Args[1])->Name;
	Error->Value = Args[1];
	return (ml_value_t *)Error;
}

ML_TYPE(MLErrorT, (), "error",
//!error
// An error. Values of this type are not accessible from Minilang code since they are caught by the runtime. Each error contains an *error value* which contains the details of the error.
	.assign = ml_error_assign,
	.call = ml_error_call
);

ML_TYPE(MLErrorValueT, (), "error",
//!error
// An error value. Error values contain the details of an error but are not themselves errors (since errors are caught by the runtime).
	.Constructor = (ml_value_t *)MLError
);

ml_value_t *ml_errorv(const char *Error, const char *Format, va_list Args) {
	char *Message;
	vasprintf(&Message, Format, Args);
	ml_error_t *Value = xnew(ml_error_t, 1, ml_error_value_t);
	Value->Type = MLErrorT;
	Value->Error->Type = MLErrorValueT;
	Value->Error->Error = Error;
	Value->Error->Message = Message;
	Value->Value = (ml_value_t *)Value->Error;
	return (ml_value_t *)Value;
}

ml_value_t *ml_error(const char *Error, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	ml_value_t *Value = ml_errorv(Error, Format, Args);
	va_end(Args);
	return Value;
}

const char *ml_error_type(const ml_value_t *Value) {
	return ((ml_error_t *)Value)->Error->Error;
}

const char *ml_error_message(const ml_value_t *Value) {
	return ((ml_error_t *)Value)->Error->Message;
}

ml_value_t *ml_error_value(const ml_value_t *Value) {
	return ((ml_error_t *)Value)->Value;
}

int ml_error_source(const ml_value_t *Value, int Level, ml_source_t *Source) {
	ml_error_t *Error = (ml_error_t *)Value;
	if (Level >= MAX_TRACE) return 0;
	if (!Error->Error->Trace[Level].Name) return 0;
	Source[0] = Error->Error->Trace[Level];
	return 1;
}

ml_value_t *ml_error_trace_add(ml_value_t *Value, ml_source_t Source) {
	ml_error_t *Error = (ml_error_t *)Value;
	for (int I = 0; I < MAX_TRACE; ++I) if (!Error->Error->Trace[I].Name) {
		Error->Error->Trace[I] = Source;
		break;
	}
	return Value;
}

void ml_error_print(const ml_value_t *Value) {
	ml_error_t *Error = (ml_error_t *)Value;
	printf("Error: %s\n", Error->Error->Message);
	for (int I = 0; (I < MAX_TRACE) && Error->Error->Trace[I].Name; ++I) {
		printf("\t%s:%d\n", Error->Error->Trace[I].Name, Error->Error->Trace[I].Line);
	}
}

void ml_error_fprint(FILE *File, const ml_value_t *Value) {
	ml_error_t *Error = (ml_error_t *)Value;
	fprintf(File, "Error: %s\n", Error->Error->Message);
	for (int I = 0; (I < MAX_TRACE) && Error->Error->Trace[I].Name; ++I) {
		fprintf(File, "\t%s:%d\n", Error->Error->Trace[I].Name, Error->Error->Trace[I].Line);
	}
}

const char *ml_error_value_type(const ml_value_t *Value) {
	return ((ml_error_value_t *)Value)->Error;
}

const char *ml_error_value_message(const ml_value_t *Value) {
	return ((ml_error_value_t *)Value)->Message;
}

int ml_error_value_source(const ml_value_t *Value, int Level, ml_source_t *Source) {
	ml_error_value_t *Error = (ml_error_value_t *)Value;
	if (Level >= MAX_TRACE) return 0;
	if (!Error->Trace[Level].Name) return 0;
	Source[0] = Error->Trace[Level];
	return 1;
}

ML_METHOD("type", MLErrorValueT) {
//!error
//<Error
//>string
// Returns the type of :mini:`Error`.
	return ml_string(((ml_error_value_t *)Args[0])->Error, -1);
}

ML_METHOD("message", MLErrorValueT) {
//!error
//<Error
//>string
// Returns the message of :mini:`Error`.
	return ml_string(((ml_error_value_t *)Args[0])->Message, -1);
}

ML_METHOD("trace", MLErrorValueT) {
//!error
//<Error
//>list
// Returns the stack trace of :mini:`Error` as a list of tuples.
	ml_error_value_t *Value = (ml_error_value_t *)Args[0];
	ml_value_t *Trace = ml_list();
	ml_source_t *Source = Value->Trace;
	for (int I = MAX_TRACE; --I >= 0 && Source->Name; ++Source) {
		ml_value_t *Tuple = ml_tuple(2);
		ml_tuple_set(Tuple, 1, ml_string(Source->Name, -1));
		ml_tuple_set(Tuple, 2, ml_integer(Source->Line));
		ml_list_put(Trace, Tuple);
	}
	return Trace;
}

ML_METHOD("raise", MLErrorValueT) {
//!error
//<Error
//>error
// Returns :mini:`Error` as an error (i.e. rethrows the error).
	ml_error_t *Error = xnew(ml_error_t, 1, ml_error_value_t);
	Error->Type = MLErrorT;
	Error->Error->Type = MLErrorValueT;
	Error->Error->Error = ml_error_value_type(Args[0]);
	Error->Error->Message = ml_error_value_message(Args[0]);
	int Level = 0;
	while (ml_error_value_source(Args[0], Level, Error->Error->Trace + Level)) ++Level;
	Error->Value = (ml_value_t *)Error->Error;
	return (ml_value_t *)Error;
}

ML_METHOD("append", MLStringBufferT, MLErrorValueT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_error_value_t *Value = (ml_error_value_t *)Args[1];
	ml_stringbuffer_add(Buffer, Value->Error, strlen(Value->Error));
	ml_stringbuffer_add(Buffer, ": ", 2);
	ml_stringbuffer_add(Buffer, Value->Message, strlen(Value->Message));
	ml_source_t *Source = Value->Trace;
	for (int I = MAX_TRACE; --I >= 0 && Source->Name; ++Source) {
		ml_stringbuffer_addf(Buffer, "\n\t%s:%d", Source->Name, Source->Line);
	}
	return Args[0];
}

// Debugging //

int ml_debugger_check(ml_state_t *State) {
	if (!State || !State->Type) return 0;
	typeof(ml_debugger_check) *function = ml_typed_fn_get(State->Type, ml_debugger_check);
	if (function) return function(State);
	return 0;
}

void ml_debugger_step_mode(ml_state_t *State, int StepOver, int StepOut) {
	if (!State || !State->Type) return;
	typeof(ml_debugger_step_mode) *function = ml_typed_fn_get(State->Type, ml_debugger_step_mode);
	if (function) return function(State, StepOver, StepOut);
}

ml_source_t ml_debugger_source(ml_state_t *State) {
	if (!State || !State->Type) return (ml_source_t){"<unknown>", 0};
	typeof(ml_debugger_source) *function = ml_typed_fn_get(State->Type, ml_debugger_source);
	if (function) return function(State);
	if (State->Caller) return ml_debugger_source(State->Caller);
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

ML_FUNCTIONX(MLBreak) {
//@break
//<Condition?
// If a debugger is present and :mini:`Condition` is omitted or not :mini:`nil` then triggers a breakpoint.
	ml_debugger_t *Debugger = Caller->Context->Values[ML_DEBUGGER_INDEX];
	if (!Debugger) ML_RETURN(MLNil);
	if (Count && Args[0] == MLNil) ML_RETURN(MLNil);
	return Debugger->run(Debugger, Caller, MLNil);
}

typedef struct {
	ml_type_t *Type;
	ml_debugger_t Base;
	ml_value_t *Run;
	stringmap_t Modules[1];
} ml_mini_debugger_t;

static void ml_mini_debugger_call(ml_state_t *Caller, ml_mini_debugger_t *Debugger, int Count, ml_value_t **Args) {
	ml_state_t *State = ml_state_new(Caller);
	ml_context_set(State->Context, ML_DEBUGGER_INDEX, &Debugger->Base);
	ml_value_t *Function = Args[0];
	return ml_call(State, Function, Count - 1, Args + 1);
}

ML_TYPE(MLMiniDebuggerT, (), "mini-debugger",
	.call = (void *)ml_mini_debugger_call
);

static void mini_debugger_run(ml_debugger_t *Base, ml_state_t *State, ml_value_t *Value) {
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)((void *)Base - offsetof(ml_mini_debugger_t, Base));
	ml_value_t **Args = ml_alloc_args(2);
	if (State->Type == NULL) State->Type = MLStateT;
	Args[0] = (ml_value_t *)State;
	Args[1] = Value;
	static ml_result_state_t Caller = {{MLStateT, NULL, (void *)ml_result_state_run, &MLRootContext}, MLNil};
	return ml_call(&Caller, Debugger->Run, 2, Args);
}

typedef struct {
	size_t Count;
	size_t Bits[];
} breakpoints_t;

static size_t *mini_debugger_breakpoints(ml_debugger_t *Base, const char *Source, int Max) {
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)((void *)Base - offsetof(ml_mini_debugger_t, Base));
	breakpoints_t **Slot = (breakpoints_t **)stringmap_slot(Debugger->Modules, Source);
	size_t Count = (Max + SIZE_BITS) / SIZE_BITS;
	if (!Slot[0]) {
		breakpoints_t *New = (breakpoints_t *)GC_MALLOC_ATOMIC(sizeof(breakpoints_t) + Count * sizeof(size_t));
		memset(New->Bits, 0, Count * sizeof(size_t));
		New->Count = Count;
		Slot[0] = New;
	} else if (Count > Slot[0]->Count) {
		breakpoints_t *Old = Slot[0];
		breakpoints_t *New = (breakpoints_t *)GC_MALLOC_ATOMIC(sizeof(breakpoints_t) + Count * sizeof(size_t));
		memset(New->Bits, 0, Count * sizeof(size_t));
		memcpy(New->Bits, Old->Bits, Old->Count * sizeof(size_t));
		New->Count = Count;
		Slot[0] = New;
	}
	return Slot[0]->Bits;
}

ML_FUNCTION(MLDebugger) {
	ML_CHECK_ARG_COUNT(1);
	ml_mini_debugger_t *Debugger = new(ml_mini_debugger_t);
	Debugger->Type = MLMiniDebuggerT;
	Debugger->Base.run = mini_debugger_run;
	Debugger->Base.breakpoints = mini_debugger_breakpoints;
	Debugger->Base.StepIn = 1;
	Debugger->Base.BreakOnError = 1;
	Debugger->Run = Args[0];
	return (ml_value_t *)Debugger;
}

ML_METHOD("breakpoint_set", MLMiniDebuggerT, MLStringT, MLIntegerT) {
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	const char *Source = ml_string_value(Args[1]);
	int LineNo = ml_integer_value(Args[2]);
	size_t *Breakpoints = mini_debugger_breakpoints(&Debugger->Base, Source, LineNo);
	Breakpoints[LineNo / SIZE_BITS] |= 1L << (LineNo % SIZE_BITS);
	++Debugger->Base.Revision;
	return Args[0];
}

ML_METHOD("breakpoint_clear", MLMiniDebuggerT, MLStringT, MLIntegerT) {
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	const char *Source = ml_string_value(Args[1]);
	int LineNo = ml_integer_value(Args[2]);
	size_t *Breakpoints = mini_debugger_breakpoints(&Debugger->Base, Source, LineNo);
	Breakpoints[LineNo / SIZE_BITS] &= ~(1L << (LineNo % SIZE_BITS));
	++Debugger->Base.Revision;
	return Args[0];
}

ML_METHOD("error_mode", MLMiniDebuggerT, MLAnyT) {
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	Debugger->Base.BreakOnError = Args[1] != MLNil;
	return Args[0];
}

ML_METHOD("step_mode", MLMiniDebuggerT, MLAnyT) {
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	Debugger->Base.StepIn = Args[1] != MLNil;
	return Args[0];
}

ML_METHODX("step_in", MLMiniDebuggerT, MLStateT, MLAnyT) {
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	ml_state_t *State = (ml_state_t *)Args[1];
	Debugger->Base.StepIn = 1;
	ml_debugger_step_mode(State, 0, 0);
	return State->run(State, Args[2]);
}

ML_METHODX("step_over", MLMiniDebuggerT, MLStateT, MLAnyT) {
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	ml_state_t *State = (ml_state_t *)Args[1];
	Debugger->Base.StepIn = 0;
	ml_debugger_step_mode(State, 1, 0);
	return State->run(State, Args[2]);
}

ML_METHODX("step_out", MLMiniDebuggerT, MLStateT, MLAnyT) {
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	ml_state_t *State = (ml_state_t *)Args[1];
	Debugger->Base.StepIn = 0;
	ml_debugger_step_mode(State, 0, 1);
	return State->run(State, Args[2]);
}

ML_METHODX("continue", MLMiniDebuggerT, MLStateT, MLAnyT) {
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	ml_state_t *State = (ml_state_t *)Args[1];
	Debugger->Base.StepIn = 0;
	ml_debugger_step_mode(State, 0, 0);
	return State->run(State, Args[2]);
}

ML_METHOD("locals", MLStateT) {
	ml_state_t *State = (ml_state_t *)Args[0];
	ml_value_t *Locals = ml_list();
	for (ml_decl_t *Decl = ml_debugger_decls(State); Decl; Decl = Decl->Next) {
		ml_list_put(Locals, ml_string(Decl->Ident, -1));
	}
	return Locals;
}

ML_METHOD("trace", MLStateT) {
	ml_state_t *State = (ml_state_t *)Args[0];
	ml_value_t *Trace = ml_list();
	while (State) {
		if (ml_debugger_check(State)) {
			if (State->Type == NULL) State->Type = MLStateT;
			ml_list_put(Trace, (ml_value_t *)State);
		}
		State = State->Caller;
	}
	return Trace;
}

ML_METHOD("source", MLStateT) {
	ml_state_t *State = (ml_state_t *)Args[0];
	ml_source_t Source = ml_debugger_source(State);
	ml_value_t *Location = ml_tuple(2);
	ml_tuple_set(Location, 1, ml_string(Source.Name, -1));
	ml_tuple_set(Location, 2, ml_integer(Source.Line));
	return Location;
}

// Schedulers //

#ifdef ML_SCHEDULER

static struct {
	ml_queued_state_t *States;
	int Size, Fill, Write, Read;
} SchedulerQueue;

void ml_scheduler_queue_init(int Size) {
	SchedulerQueue.Size = Size;
	SchedulerQueue.States = anew(ml_queued_state_t, Size);
}

ml_queued_state_t ml_scheduler_queue_next() {
	if (SchedulerQueue.Fill) {
		ml_queued_state_t *States = SchedulerQueue.States;
		int Read = SchedulerQueue.Read;
		ml_queued_state_t QueuedState = States[Read];
		States[Read] = (ml_queued_state_t){NULL, NULL};
		--SchedulerQueue.Fill;
		SchedulerQueue.Read = (Read + 1) % SchedulerQueue.Size;
		return QueuedState;
	} else {
		return (ml_queued_state_t){NULL, NULL};
	}
}

int ml_scheduler_queue_add(ml_state_t *State, ml_value_t *Value) {
	if (++SchedulerQueue.Fill > SchedulerQueue.Size) {
		int NewQueueSize = SchedulerQueue.Size * 2;
		ml_queued_state_t *NewQueuedStates = anew(ml_queued_state_t, NewQueueSize);
		memcpy(NewQueuedStates, SchedulerQueue.States, SchedulerQueue.Size * sizeof(ml_queued_state_t));
		SchedulerQueue.Read = 0;
		SchedulerQueue.Write = SchedulerQueue.Size;
		SchedulerQueue.States = NewQueuedStates;
		SchedulerQueue.Size = NewQueueSize;
	}
	int Write = SchedulerQueue.Write;
	SchedulerQueue.States[Write] = (ml_queued_state_t){State, Value};
	SchedulerQueue.Write = (Write + 1) % SchedulerQueue.Size;
	return SchedulerQueue.Fill;
}

#endif

// Semaphore //

typedef struct {
	ml_type_t *Type;
	ml_state_t **States;
	int64_t Value;
	int Size, Fill, Write, Read;
} ml_semaphore_t;

ML_FUNCTION(MLSemaphore) {
//!semaphore
//@semaphore
//<Initial? : integer
	if (Count > 0) ML_CHECK_ARG_TYPE(0, MLIntegerT);
	ml_semaphore_t *Semaphore = new(ml_semaphore_t);
	Semaphore->Type = MLSemaphoreT;
	Semaphore->Value = (Count > 0) ? ml_integer_value(Args[0]) : 1;
	Semaphore->Fill = 0;
	Semaphore->Size = 4;
	Semaphore->Read = Semaphore->Write = 0;
	Semaphore->States = anew(ml_state_t *, 4);
	return (ml_value_t *)Semaphore;
}

ML_TYPE(MLSemaphoreT, (), "semaphore",
//!semaphore
	.Constructor = (ml_value_t *)MLSemaphore
);

ML_METHODX("wait", MLSemaphoreT) {
//!semaphore
//<Semaphore
	ml_semaphore_t *Semaphore = (ml_semaphore_t *)Args[0];
	int64_t Value = Semaphore->Value;
	if (Value) {
		Semaphore->Value = Value - 1;
		ML_RETURN(Args[0]);
	}
	++Semaphore->Fill;
	if (Semaphore->Fill > Semaphore->Size) {
		int NewSize = Semaphore->Size * 2;
		ml_state_t **NewStates = anew(ml_state_t *, NewSize);
		memcpy(NewStates, Semaphore->States, Semaphore->Size * sizeof(ml_state_t *));
		Semaphore->Read = 0;
		Semaphore->Write = Semaphore->Size;
		Semaphore->States = NewStates;
		Semaphore->Size = NewSize;
	}
	Semaphore->States[Semaphore->Write] = Caller;
	Semaphore->Write = (Semaphore->Write + 1) % Semaphore->Size;
}

ML_METHOD("signal", MLSemaphoreT) {
//!semaphore
//<Semaphore
	ml_semaphore_t *Semaphore = (ml_semaphore_t *)Args[0];
	int Fill = Semaphore->Fill;
	if (Fill) {
		Semaphore->Fill = Fill - 1;
		ml_state_t *State = Semaphore->States[Semaphore->Read];
		Semaphore->States[Semaphore->Read] = NULL;
		Semaphore->Read = (Semaphore->Read + 1) % Semaphore->Size;
		State->run(State, Args[0]);
	} else {
		++Semaphore->Value;
	}
	return Args[0];
}

ML_METHOD("value", MLSemaphoreT) {
//!semaphore
//<Semaphore
	ml_semaphore_t *Semaphore = (ml_semaphore_t *)Args[0];
	return ml_integer(Semaphore->Value);
}

// Channels

typedef struct ml_channel_message_t ml_channel_message_t;

struct ml_channel_message_t {
	ml_channel_message_t *Next;
	ml_state_t *Sender;
	ml_value_t *Value;
};

typedef struct {
	ml_state_t Base;
	ml_state_t *Sender;
	ml_channel_message_t *Head, **Tail;
	int Open;
} ml_channel_t;

static void ml_channel_run(ml_channel_t *Channel, ml_value_t *Value) {
	Channel->Open = 0;
	ML_CONTINUE(Channel->Base.Caller, Value);
}

ML_FUNCTION(MLChannel) {
//!channel
//@channel
	ml_channel_t *Channel = new(ml_channel_t);
	Channel->Base.Type = MLChannelT;
	Channel->Base.run = (ml_state_fn)ml_channel_run;
	Channel->Tail = &Channel->Head;
	return (ml_value_t *)Channel;
}

ML_TYPE(MLChannelT, (), "channel",
//!channel
	.Constructor = (ml_value_t *)MLChannel
);

ML_METHODVX("start", MLChannelT, MLFunctionT) {
//!channel
//<Channel
//<Function
//>any
	ml_channel_t *Channel = (ml_channel_t *)Args[0];
	Channel->Base.Caller = Caller;
	Channel->Base.Context = Caller->Context;
	Channel->Open = 1;
	ml_value_t *Function = Args[1];
	Args[1] = Args[0];
	return ml_call(Channel, Function, Count - 1, Args + 1);
}

ML_METHOD("open", MLChannelT) {
//!channel
//<Channel
//>channel | nil
	ml_channel_t *Channel = (ml_channel_t *)Args[0];
	return Channel->Open ? Args[0] : MLNil;
}

static inline void ml_channel_next(ml_state_t *Caller, ml_channel_t *Channel, ml_value_t *Value) {
	if (!Channel->Open) ML_ERROR("ChannelError", "Channel is not open");
	ml_channel_message_t *Message = Channel->Head;
	ml_channel_message_t *Next = Message->Next;
	Channel->Head = Next;
	Channel->Base.Caller = Caller;
	if (Next) {
		Caller->run(Caller, Next->Value);
	} else {
		Channel->Tail = &Channel->Head;
	}
	ML_CONTINUE(Message->Sender, Value);
}

ML_METHODX("next", MLChannelT) {
//!channel
//<Channel
//>any
	return ml_channel_next(Caller, (ml_channel_t *)Args[0], MLNil);
}

ML_METHODX("next", MLChannelT, MLAnyT) {
//!channel
//<Channel
//<Reply
//>any
	return ml_channel_next(Caller, (ml_channel_t *)Args[0], Args[1]);
}

ML_METHODX("send", MLChannelT, MLAnyT) {
//!channel
//<Channel
//<Message
//>any
	ml_channel_t *Channel = (ml_channel_t *)Args[0];
	if (!Channel->Open) ML_ERROR("ChannelError", "Channel is not open");
	ml_channel_message_t *Message = new(ml_channel_message_t);
	Message->Sender = Caller;
	Message->Value = Args[1];
	Channel->Tail[0] = Message;
	Channel->Tail = &Message->Next;
	if (Message == Channel->Head) {
		ML_CONTINUE(Channel->Base.Caller, Channel->Head->Value);
	}
}

ML_METHODVX("close", MLChannelT, MLFunctionT) {
//!channel
//<Channel
//<Function
	ml_channel_t *Channel = (ml_channel_t *)Args[0];
	return ml_call((ml_state_t *)Channel, Args[1], Count - 2, Args + 2);
}

/*
ML_METHODX("error", MLChannelT, MLStringT, MLStringT) {
	ml_state_t *Channel = (ml_state_t *)Args[0];
	ml_state_t *Receiver = Channel->Caller;
	if (!Receiver) ML_ERROR("ChannelError", "Channel is not open");
	Channel->Caller = Caller;
	Channel->Context = Caller->Context;
	ml_error_t *Error = xnew(ml_error_t, 1, ml_error_value_t);
	Error->Type = MLErrorT;
	Error->Error->Type = MLErrorValueT;
	Error->Error->Error = ml_string_value(Args[1]);
	Error->Error->Message = ml_string_value(Args[2]);
	Error->Value = (ml_value_t *)Error->Error;
	ML_CONTINUE(Receiver, Error);
}

ML_METHODX("raise", MLChannelT, MLStringT, MLAnyT) {
	ml_state_t *Channel = (ml_state_t *)Args[0];
	ml_state_t *Receiver = Channel->Caller;
	if (!Receiver) ML_ERROR("ChannelError", "Channel is not open");
	Channel->Caller = Caller;
	Channel->Context = Caller->Context;
	ml_error_t *Error = xnew(ml_error_t, 1, ml_error_value_t);
	Error->Type = MLErrorT;
	Error->Error->Type = MLErrorValueT;
	Error->Error->Error = ml_string_value(Args[1]);
	Error->Error->Message = ml_typeof(Args[2])->Name;
	Error->Value = Args[2];
	ML_CONTINUE(Receiver, Error);
}

ML_METHODX("raise", MLChannelT, MLErrorValueT) {
	ml_state_t *Channel = (ml_state_t *)Args[0];
	ml_state_t *Receiver = Channel->Caller;
	if (!Receiver) ML_ERROR("ChannelError", "Channel is not open");
	Channel->Caller = Caller;
	Channel->Context = Caller->Context;
	ml_error_t *Error = xnew(ml_error_t, 1, ml_error_value_t);
	Error->Type = MLErrorT;
	Error->Error[0] = *(ml_error_value_t *)Args[1];
	ML_CONTINUE(Receiver, Error);
}
*/

void ml_runtime_init() {
#include "ml_runtime_init.c"
}

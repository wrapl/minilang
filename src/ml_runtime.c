#include "minilang.h"
#include "ml_macros.h"
#include "stringmap.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "ml_types.h"
#include "ml_logging.h"
#include <time.h>

#ifdef ML_TIMESCHED
#include <sys/time.h>
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "runtime"

// Runtime //

#ifdef ML_THREADSAFE
__thread
#endif
ml_value_t *MLArgCache[ML_ARG_CACHE_SIZE];

static uint64_t DefaultCounter = UINT_MAX;

static int default_swap(ml_scheduler_t *Queue, ml_state_t *State, ml_value_t *Value) {
	DefaultCounter = UINT_MAX;
	MLPreempt = 1;
	State->run(State, Value);
	return 0;
}

static ml_scheduler_t DefaultScheduler = {default_swap};

ml_context_t *MLRootContext;

#ifdef ML_CONTEXT_SECTION

__attribute__ ((section("ml_context_section"))) void *ML_METHODS_INDEX[1];
__attribute__ ((section("ml_context_section"))) void *ML_VARIABLES_INDEX[1];
__attribute__ ((section("ml_context_section"))) void *ML_DEBUGGER_INDEX[1];
__attribute__ ((section("ml_context_section"))) void *ML_SCHEDULER_INDEX[1];
__attribute__ ((section("ml_context_section"))) void *ML_COUNTER_INDEX[1];
__attribute__ ((section("ml_context_section"))) void *ML_THREAD_INDEX[1];

static int MLContextSize = 0;
static uint64_t MLContextReserved = 0;

#else

static int MLContextSize = ML_CONTEXT_SIZE;
static uint64_t MLContextReserved = (1 << ML_CONTEXT_SIZE) - 1;

#endif

ml_context_t *ml_context(ml_context_t *Parent) {
	ml_context_t *Context = xnew(ml_context_t, MLContextSize, void *);
	Context->Parent = Parent;
	Context->Size = MLContextSize;
	for (int I = 0; I < Parent->Size; ++I) Context->Values[I] = Parent->Values[I];
	return Context;
}

int ml_context_index() {
	int Index = MLContextSize++;
	MLContextReserved |= (1 << Index);
	return Index;
}

void ml_context_reserve(int Index) {
	if (MLContextReserved & (1 << Index)) {
		ML_LOG_FATAL(NULL, "Context index %d already reserved", Index);
		abort();
	}
	MLContextReserved |= (1 << Index);
	if (MLContextSize < Index + 1) MLContextSize = Index + 1;
}

/*void ml_context_set(ml_context_t *Context, int Index, void *Value) {
	if (Context->Size <= Index) return;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
	Context->Values[Index] = Value;
#pragma GCC diagnostic pop
}*/

static stringmap_t MLConfigs[1] = {STRINGMAP_INIT};
static inthash_t MLConfigFns[1] = {INTHASH_INIT};

void ml_config_register(const char *Name, ml_config_fn Fn) {
	stringmap_insert(MLConfigs, Name, Fn);
	inthash_insert(MLConfigFns, (uintptr_t)Fn, (void *)Name);
}

ml_config_fn ml_config_lookup(const char *Name) {
	return (ml_config_fn)stringmap_search(MLConfigs, Name);
}

const char *ml_config_name(void *Fn) {
	return (const char *)inthash_search(MLConfigFns, (uintptr_t)Fn);
}

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
	ml_context_value_t *Values = ml_context_get_static(Caller->Context, ML_VARIABLES_INDEX);
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
		ml_state_t *State = ml_state(Caller);
		ml_context_set_static(State->Context, ML_VARIABLES_INDEX, Value);
		ml_value_t *Function = ml_deref(Args[Count - 1]);
		return ml_call(State, Function, Count - 2, Args + 1);
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

ML_TYPE(MLContextKeyT, (MLFunctionT), "context",
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

ml_state_t MLEndState[1] = {{MLStateT, NULL, ml_end_state_run, NULL}};

void ml_default_state_run(ml_state_t *State, ml_value_t *Value) {
	ML_CONTINUE(State->Caller, Value);
}

static void ml_call_state_run(ml_call_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		ML_CONTINUE(State->Base.Caller, Value);
	} else {
		return ml_call(State->Base.Caller, Value, State->Count, State->Args);
	}
}

ml_call_state_t *ml_call_state(ml_state_t *Caller, int Count) {
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

ml_result_state_t *ml_result_state(ml_context_t *Context) {
	ml_result_state_t *State = new(ml_result_state_t);
	State->Base.Context = Context ?: MLRootContext;
	//State->Value = MLNil;
	State->Base.Type = MLStateT;
	State->Base.run = (ml_state_fn)ml_result_state_run;
	return State;
}

#ifdef ML_TRAMPOLINE
static ml_scheduler_queue_t *MLRootQueue;
#endif

ml_value_t *ml_simple_call(ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_result_state_t State = {{MLStateT, NULL, (void *)ml_result_state_run, MLRootContext}, NULL};
	ml_call(&State, Value, Count, Args);
#ifdef ML_TRAMPOLINE
	while (!State.Value) {
		ml_queued_state_t Queued = ml_scheduler_queue_next(MLRootQueue);
		Queued.State->run(Queued.State, Queued.Value);
	}
#endif
	return State.Value;
}

ml_value_t *ml_simple_assign(ml_value_t *Value, ml_value_t *Value2) {
	ml_result_state_t State = {{MLStateT, NULL, (void *)ml_result_state_run, MLRootContext}, NULL};
	ml_assign(&State, Value, Value2);
#ifdef ML_TRAMPOLINE
	while (!State.Value) {
		ml_queued_state_t Queued = ml_scheduler_queue_next(MLRootQueue);
		Queued.State->run(Queued.State, Queued.Value);
	}
#endif
	ml_value_t *Result = State.Value;
	State.Value = MLNil;
	return Result;
}

typedef struct {
	ml_state_t Base;
	ml_context_t Context[1];
} ml_context_state_t;

ml_state_t *ml_state(ml_state_t *Caller) {
	ml_context_state_t *State = xnew(ml_context_state_t, MLContextSize, void *);
	ml_context_t *Parent = Caller ? Caller->Context : MLRootContext;
	State->Context->Parent = Parent;
	State->Context->Size = MLContextSize;
	for (int I = 0; I < Parent->Size; ++I) State->Context->Values[I] = Parent->Values[I];
	State->Base.Caller = Caller;
	State->Base.run = ml_default_state_run;
	State->Base.Context = State->Context;
	return (ml_state_t *)State;
}

void ml_state_continue(ml_state_t *State, ml_value_t *Value) {
#ifdef ML_TIMESCHED
	if (MLPreempt < 0) {
		return ml_state_schedule(State, Value);
	} else {
		return State->run(State, Value);
	}
#else
	return State->run(State, Value);
#endif
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

typedef struct ml_cache_info_t ml_cache_info_t;

struct ml_cache_info_t {
	ml_cache_info_t *Next;
	void *Data;
	const char *Name;
	ml_cache_usage_fn Usage;
	ml_cache_clear_fn Clear;
};

static ml_cache_info_t *CacheInfos = NULL;

void ml_cache_register(const char *Name, ml_cache_usage_fn Usage, ml_cache_clear_fn Clear, void *Data) {
	ml_cache_info_t *Info = new(ml_cache_info_t);
	Info->Data = Data;
	Info->Name = Name;
	Info->Usage = Usage;
	Info->Clear = Clear;
	Info->Next = CacheInfos;
	CacheInfos = Info;
}

void ml_cache_usage(ml_cache_usage_callback_fn Callback, void *Arg) {
	for (ml_cache_info_t *Info = CacheInfos; Info; Info = Info->Next) {
		size_t Usage = Info->Usage(Info->Data);
		Callback(Info->Name, Usage, Arg);
	}
}

void ml_cache_clear(const char *Name) {
	for (ml_cache_info_t *Info = CacheInfos; Info; Info = Info->Next) {
		if (!strcmp(Name, Info->Name)) Info->Clear(Info->Data);
	}
}

void ml_cache_clear_all() {
	for (ml_cache_info_t *Info = CacheInfos; Info; Info = Info->Next) {
		Info->Clear(Info->Data);
	}
}

ML_FUNCTIONX(MLCallCC) {
//!runtime
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
//!runtime
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
//!runtime
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
//!runtime
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

static void ml_reference_assign(ml_state_t *Caller, ml_reference_t *Reference, ml_value_t *Value) {
	Reference->Address[0] = Value;
	ML_RETURN(Value);
}

ML_TYPE(MLReferenceT, (), "reference",
//!internal
	.hash = (void *)ml_reference_hash,
	.deref = (void *)ml_reference_deref,
	.assign = (void *)ml_reference_assign
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
	ml_source_t Source;
} ml_uninitialized_t;

static void ml_uninitialized_call(ml_state_t *Caller, ml_uninitialized_t *Uninitialized, int Count, ml_value_t **Args) {
	ML_ERROR("ValueError", "%s is uninitialized", Uninitialized->Name);
}

static void ml_unitialized_assign(ml_state_t *Caller, ml_uninitialized_t *Uninitialized, ml_value_t *Value) {
	ML_ERROR("ValueError", "%s is uninitialized", Uninitialized->Name);
}

ML_TYPE(MLUninitializedT, (), "uninitialized",
// An uninitialized value. Used for forward declarations.
	.call = (void *)ml_uninitialized_call,
	.assign = (void *)ml_unitialized_assign
);

ml_value_t *ml_uninitialized(const char *Name, ml_source_t Source) {
	ml_uninitialized_t *Uninitialized = new(ml_uninitialized_t);
	Uninitialized->Type = MLUninitializedT;
	Uninitialized->Name = Name;
	Uninitialized->Source = Source;
	return (ml_value_t *)Uninitialized;
}

const char *ml_uninitialized_name(ml_value_t *Uninitialized) {
	return ((ml_uninitialized_t *)Uninitialized)->Name;
}

ml_source_t ml_uninitialized_source(ml_value_t *Uninitialized) {
	return ((ml_uninitialized_t *)Uninitialized)->Source;
}

void ml_uninitialized_use(ml_value_t *Uninitialized0, ml_value_t **Value) {
	ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Uninitialized0;
	ml_uninitialized_slot_t *Slot = new(ml_uninitialized_slot_t);
	Slot->Value = Value;
	Slot->Next = Uninitialized->Slots;
	Uninitialized->Slots = Slot;
}

static ML_METHOD_DECL(SymbolMethod, "::");

static int ml_uninitialized_resolve(const char *Name, ml_uninitialized_t *Uninitialized, ml_value_t *Value) {
	ml_value_t *Result = ml_simple_inline(SymbolMethod, 2, Value, ml_string(Name, -1));
	ml_uninitialized_set((ml_value_t *)Uninitialized, Result);
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

ML_METHODX("::", MLUninitializedT, MLStringT) {
	ml_uninitialized_t *Uninitialized = (ml_uninitialized_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Uninitialized->Unresolved, Name);
	if (!Slot[0]) Slot[0] = ml_uninitialized(Name, ml_debugger_source(Caller));
	ML_RETURN(Slot[0]);
}

// Errors //

#define MAX_TRACE 16

typedef struct {
	ml_type_t *Type;
	const char *Error;
	const char *Message;
	ml_value_t *Value;
	ml_source_t Trace[MAX_TRACE];
} ml_error_value_t;

typedef struct {
	ml_type_t *Type;
	ml_error_value_t Error[1];
} ml_error_t;

static void ml_error_assign(ml_state_t *Caller, ml_value_t *Error, ml_value_t *Value) {
	ML_RETURN(Error);
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
	ml_error_t *Error = new(ml_error_t);
	Error->Type = MLErrorT;
	Error->Error->Type = MLErrorValueT;
	Error->Error->Error = ml_string_value(Args[0]);
	Error->Error->Message = ml_string_value(Args[1]);
	Error->Error->Value = MLNil;
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
	Error->Error->Value = Args[1];
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
	GC_vasprintf(&Message, Format, Args);
	ml_error_t *Value = new(ml_error_t);
	Value->Type = MLErrorT;
	Value->Error->Type = MLErrorValueT;
	Value->Error->Error = Error;
	Value->Error->Message = Message;
	Value->Error->Value = MLNil;
	return (ml_value_t *)Value;
}

ml_value_t *ml_error(const char *Error, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	ml_value_t *Value = ml_errorv(Error, Format, Args);
	va_end(Args);
	return Value;
}

ml_value_t *ml_error_unwrap(const ml_value_t *Value) {
	return (ml_value_t *)((ml_error_t *)Value)->Error;
}

const char *ml_error_type(const ml_value_t *Value) {
	return ((ml_error_t *)Value)->Error->Error;
}

const char *ml_error_message(const ml_value_t *Value) {
	return ((ml_error_t *)Value)->Error->Message;
}

ml_value_t *ml_error_value(const ml_value_t *Value) {
	return ((ml_error_t *)Value)->Error->Value;
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

void ml_error_value_set(ml_value_t *Error, ml_value_t *Value) {
	((ml_error_t *)Error)->Error->Value = Value;
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

ml_value_t *ml_error_value_error(ml_value_t *Value) {
	return (ml_value_t *)((void *)Value - offsetof(ml_error_t, Error));
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
	ml_error_value_t *Error = (ml_error_value_t *)Args[0];
	ml_value_t *Trace = ml_list();
	ml_source_t *Source = Error->Trace;
	for (int I = MAX_TRACE; --I >= 0 && Source->Name; ++Source) {
		ml_value_t *Tuple = ml_tuplev(2, ml_string(Source->Name, -1), ml_integer(Source->Line));
		ml_list_put(Trace, Tuple);
	}
	return Trace;
}

ML_METHOD("value", MLErrorValueT) {
	ml_error_value_t *Error = (ml_error_value_t *)Args[0];
	return Error->Value;
}

ML_METHOD("raise", MLErrorValueT) {
//!error
//<Error
//>error
// Returns :mini:`Error` as an error (i.e. rethrows the error).
	ml_error_t *Error = new(ml_error_t);
	Error->Type = MLErrorT;
	Error->Error->Type = MLErrorValueT;
	Error->Error->Error = ml_error_value_type(Args[0]);
	Error->Error->Message = ml_error_value_message(Args[0]);
	int Level = 0;
	while (ml_error_value_source(Args[0], Level, Error->Error->Trace + Level)) ++Level;
	Error->Error->Value = (ml_value_t *)Error->Error;
	return (ml_value_t *)Error;
}

ML_METHOD("append", MLStringBufferT, MLErrorValueT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_error_value_t *Value = (ml_error_value_t *)Args[1];
	ml_stringbuffer_write(Buffer, Value->Error, strlen(Value->Error));
	ml_stringbuffer_write(Buffer, ": ", 2);
	ml_stringbuffer_write(Buffer, Value->Message, strlen(Value->Message));
	ml_source_t *Source = Value->Trace;
	for (int I = MAX_TRACE; --I >= 0 && Source->Name; ++Source) {
		ml_stringbuffer_printf(Buffer, "\n\t%s:%d", Source->Name, Source->Line);
	}
	return Args[0];
}

static ml_value_t *ML_TYPED_FN(ml_serialize, MLErrorValueT, ml_error_value_t *Value) {
	ml_value_t *Serialized = ml_list();
	ml_list_put(Serialized, ml_cstring("error"));
	ml_list_put(Serialized, ml_string(Value->Error, -1));
	ml_list_put(Serialized, ml_string(Value->Message, -1));
	ml_source_t *Source = Value->Trace;
	for (int I = MAX_TRACE; --I >= 0 && Source->Name; ++Source) {
		ml_list_put(Serialized, ml_string(Source->Name, -1));
		ml_list_put(Serialized, ml_integer(Source->Line));
	}
	return Serialized;
}

ML_DESERIALIZER("error") {
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLStringT);
	ml_error_value_t *Value = new(ml_error_value_t);
	Value->Type = MLErrorValueT;
	Value->Error = ml_string_value(Args[0]);
	Value->Message = ml_string_value(Args[1]);
	ml_source_t *Source = Value->Trace;
	if (Count > (2 + MAX_TRACE * 2)) Count = 2 + MAX_TRACE + 2;
	for (int I = 2; I < Count; I += 2, ++Source) {
		ML_CHECK_ARG_TYPE(I, MLStringT);
		ML_CHECK_ARG_TYPE(I + 1, MLIntegerT);
		Source->Name = ml_string_value(Args[I]);
		Source->Line = ml_integer_value(Args[I + 1]);
	}
	Value->Value = MLNil;
	return (ml_value_t *)Value;
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
//!runtime
//@break
//<Condition?
// If a debugger is present and :mini:`Condition` is omitted or not :mini:`nil` then triggers a breakpoint.
	ml_debugger_t *Debugger = ml_context_get_static(Caller->Context, ML_DEBUGGER_INDEX);
	if (!Debugger) ML_RETURN(MLNil);
	if (Count && Args[0] == MLNil) ML_RETURN(MLNil);
	return Debugger->run(Debugger, Caller, MLNil);
}

typedef struct {
	ml_state_t Base;
	ml_debugger_t Debug;
	ml_value_t *Run;
	stringmap_t Modules[1];
} ml_mini_debugger_t;

static void ml_mini_debugger_call(ml_state_t *Caller, ml_mini_debugger_t *Debugger, int Count, ml_value_t **Args) {
	ml_state_t *State = ml_state(Caller);
	ml_context_set_static(State->Context, ML_DEBUGGER_INDEX, &Debugger->Debug);
	ml_value_t *Function = Args[0];
	return ml_call(State, Function, Count - 1, Args + 1);
}

ML_TYPE(MLDebuggerT, (), "mini-debugger",
	.call = (void *)ml_mini_debugger_call
);

static void mini_debugger_debug(ml_debugger_t *Debug, ml_state_t *State, ml_value_t *Value) {
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)((void *)Debug - offsetof(ml_mini_debugger_t, Debug));
	ml_value_t **Args = ml_alloc_args(2);
	if (State->Type == NULL) State->Type = MLStateT;
	Args[0] = (ml_value_t *)State;
	Args[1] = Value;
	return ml_call(Debugger, Debugger->Run, 2, Args);
}

typedef struct {
	size_t Count;
	size_t Bits[];
} breakpoints_t;

static size_t *mini_debugger_breakpoints(ml_debugger_t *Debug, const char *Source, int Max) {
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)((void *)Debug - offsetof(ml_mini_debugger_t, Debug));
	breakpoints_t **Slot = (breakpoints_t **)stringmap_slot(Debugger->Modules, Source);
	size_t Count = (Max + SIZE_BITS) / SIZE_BITS;
	if (!Slot[0]) {
		breakpoints_t *New = (breakpoints_t *)snew(sizeof(breakpoints_t) + Count * sizeof(size_t));
		memset(New->Bits, 0, Count * sizeof(size_t));
		New->Count = Count;
		Slot[0] = New;
	} else if (Count > Slot[0]->Count) {
		breakpoints_t *Old = Slot[0];
		breakpoints_t *New = (breakpoints_t *)snew(sizeof(breakpoints_t) + Count * sizeof(size_t));
		memset(New->Bits, 0, Count * sizeof(size_t));
		memcpy(New->Bits, Old->Bits, Old->Count * sizeof(size_t));
		New->Count = Count;
		Slot[0] = New;
	}
	return Slot[0]->Bits;
}

static void mini_debugger_run(ml_state_t *State, ml_value_t *Value) {
}

ML_FUNCTIONX(MLDebugger) {
//!debugger
//@debugger
//<Function
//>debugger
// Returns a new debugger for :mini:`Function()`.
	ML_CHECKX_ARG_COUNT(1);
	ml_mini_debugger_t *Debugger = new(ml_mini_debugger_t);
	Debugger->Base.Type = MLDebuggerT;
	Debugger->Base.Caller = Caller;
	Debugger->Base.Context = Caller->Context;
	Debugger->Base.run = mini_debugger_run;
	Debugger->Debug.run = mini_debugger_debug;
	Debugger->Debug.breakpoints = mini_debugger_breakpoints;
	Debugger->Debug.StepIn = 1;
	Debugger->Debug.BreakOnError = 1;
	Debugger->Run = Args[0];
	ML_RETURN(Debugger);
}

ML_METHOD("breakpoint_set", MLDebuggerT, MLStringT, MLIntegerT) {
//!debugger
//<Debugger
//<Source
//<Line
// Sets a breakpoint in :mini:`Source` at line :mini:`Line`.
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	const char *Source = ml_string_value(Args[1]);
	int LineNo = ml_integer_value(Args[2]);
	size_t *Breakpoints = mini_debugger_breakpoints(&Debugger->Debug, Source, LineNo);
	Breakpoints[LineNo / SIZE_BITS] |= 1L << (LineNo % SIZE_BITS);
	++Debugger->Debug.Revision;
	return Args[0];
}

ML_METHOD("breakpoint_clear", MLDebuggerT, MLStringT, MLIntegerT) {
//!debugger
//<Debugger
//<Source
//<Line
// Clears any breakpoints from :mini:`Source` at line :mini:`Line`.
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	const char *Source = ml_string_value(Args[1]);
	int LineNo = ml_integer_value(Args[2]);
	size_t *Breakpoints = mini_debugger_breakpoints(&Debugger->Debug, Source, LineNo);
	Breakpoints[LineNo / SIZE_BITS] &= ~(1L << (LineNo % SIZE_BITS));
	++Debugger->Debug.Revision;
	return Args[0];
}

ML_METHOD("error_mode", MLDebuggerT, MLAnyT) {
//!debugger
//<Debugger
//<Set
// If :mini:`Set` is not :mini:`nil` then :mini:`Debugger` will stop on errors.
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	Debugger->Debug.BreakOnError = Args[1] != MLNil;
	return Args[0];
}

ML_METHOD("step_mode", MLDebuggerT, MLAnyT) {
//!debugger
//<Debugger
//<Set
// If :mini:`Set` is not :mini:`nil` then :mini:`Debugger` will stop on after each line.
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	Debugger->Debug.StepIn = Args[1] != MLNil;
	return Args[0];
}

ML_METHODX("step_in", MLDebuggerT, MLStateT, MLAnyT) {
//!debugger
//<Debugger
//<State
//<Value
// Resume :mini:`State` with :mini:`Value` in the debugger, stopping after the next line.
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	ml_state_t *State = (ml_state_t *)Args[1];
	Debugger->Debug.StepIn = 1;
	ml_debugger_step_mode(State, 0, 0);
	return State->run(State, Args[2]);
}

ML_METHODX("step_over", MLDebuggerT, MLStateT, MLAnyT) {
//!debugger
//<Debugger
//<State
//<Value
// Resume :mini:`State` with :mini:`Value` in the debugger, stopping after the next line in the same function (i.e. stepping over function calls).
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	ml_state_t *State = (ml_state_t *)Args[1];
	Debugger->Debug.StepIn = 0;
	ml_debugger_step_mode(State, 1, 0);
	return State->run(State, Args[2]);
}

ML_METHODX("step_out", MLDebuggerT, MLStateT, MLAnyT) {
//!debugger
//<Debugger
//<State
//<Value
// Resume :mini:`State` with :mini:`Value` in the debugger, stopping at the end of the current function.
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	ml_state_t *State = (ml_state_t *)Args[1];
	Debugger->Debug.StepIn = 0;
	ml_debugger_step_mode(State, 0, 1);
	return State->run(State, Args[2]);
}

ML_METHODX("continue", MLDebuggerT, MLStateT, MLAnyT) {
//!debugger
//<Debugger
//<State
//<Value
// Resume :mini:`State` with :mini:`Value` in the debugger.
	ml_mini_debugger_t *Debugger = (ml_mini_debugger_t *)Args[0];
	ml_state_t *State = (ml_state_t *)Args[1];
	Debugger->Debug.StepIn = 0;
	ml_debugger_step_mode(State, 0, 0);
	return State->run(State, Args[2]);
}

ML_METHOD("locals", MLStateT) {
//!debugger
//<State
//>list[string]
// Returns the list of locals in :mini:`State`. Returns an empty list if :mini:`State` does not have any debugging information.
	ml_state_t *State = (ml_state_t *)Args[0];
	ml_value_t *Locals = ml_list();
	for (ml_decl_t *Decl = ml_debugger_decls(State); Decl; Decl = Decl->Next) {
		ml_list_put(Locals, ml_string(Decl->Ident, -1));
	}
	return Locals;
}

ML_METHOD("trace", MLStateT) {
//!debugger
//<State
//>list[state]
// Returns the call trace from :mini:`State`, excluding states that do not have debugging information.
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
//!debugger
//<State
//>tuple[string, integer]
// Returns the source location for :mini:`State`.
	ml_state_t *State = (ml_state_t *)Args[0];
	ml_source_t Source = ml_debugger_source(State);
	return ml_tuplev(2, ml_string(Source.Name, -1), ml_integer(Source.Line));
}

ML_FUNCTIONX(MLTrace) {
//@trace
//>list[tuple[string,integer]]
// Returns the call stack trace (source locations).
	ml_value_t *Trace = ml_list();
	for (ml_state_t *State = Caller; State; State = State->Caller) {
		ml_source_t Source = ml_debugger_source(State);
		if (strcmp(Source.Name, "<unknown>")) {
			ml_list_put(Trace, ml_tuplev(2, ml_string(Source.Name, -1), ml_integer(Source.Line)));
		}
	}
	ML_RETURN(Trace);
}

static int ml_config_debugger(ml_context_t *Context) {
	return !!ml_context_get_static(Context, ML_DEBUGGER_INDEX);
}

// Schedulers //

volatile int MLPreempt = 0;

#ifdef ML_SCHEDULER

typedef struct ml_queue_block_t ml_queue_block_t;

#define QUEUE_BLOCK_SIZE 128

struct ml_queue_block_t {
	ml_queued_state_t States[QUEUE_BLOCK_SIZE];
	ml_queue_block_t *Next;
};

struct ml_scheduler_queue_t {
	ml_scheduler_t Base;
	ml_queue_block_t *WriteBlock, *ReadBlock;
#ifdef ML_THREADS
	pthread_mutex_t Lock[1];
	pthread_cond_t Available[1];
#endif
	uint64_t Counter, Slice;
	int WriteIndex, ReadIndex, Fill, Space;
};

static ml_queued_state_t ml_scheduler_queue_read(ml_scheduler_queue_t *Queue) {
	ml_queue_block_t *Block = Queue->ReadBlock;
	int Index = Queue->ReadIndex;
	ml_queued_state_t Next = Block->States[Index];
	Block->States[Index] = (ml_queued_state_t){NULL, NULL};
	if (Index == 0) {
		Queue->ReadIndex = QUEUE_BLOCK_SIZE - 1;
		Queue->ReadBlock = Block->Next;
	} else {
		Queue->ReadIndex = Index - 1;
	}
	++Queue->Space;
	--Queue->Fill;
#ifdef ML_TIMESCHED
	MLPreempt = 1;
#else
	Queue->Counter = Queue->Slice;
#endif
	return Next;
}

int ml_scheduler_queue_size(ml_scheduler_queue_t *Queue) {
	return Queue->Space + Queue->Fill;
}

int ml_scheduler_queue_fill(ml_scheduler_queue_t *Queue) {
	return Queue->Fill;
}

ml_queued_state_t ml_scheduler_queue_next(ml_scheduler_queue_t *Queue) {
	ml_queued_state_t Next = {NULL, NULL};
#ifdef ML_THREADS
	pthread_mutex_lock(Queue->Lock);
#endif
	if (Queue->Fill) Next = ml_scheduler_queue_read(Queue);
#ifdef ML_THREADS
	pthread_mutex_unlock(Queue->Lock);
#endif
	return Next;
}

static int ml_scheduler_queue_write(ml_scheduler_queue_t *Queue, ml_state_t *State, ml_value_t *Value) {
	ml_queue_block_t *Block = Queue->WriteBlock;
	if (Queue->Space == 0) {
		// implies ReadBlock == WriteBlock and ReadIndex == WriteIndex
		ml_queue_block_t *ReadBlock = new(ml_queue_block_t);
		int ReadIndex = Queue->ReadIndex;
		memcpy(ReadBlock->States, Block->States, (ReadIndex + 1) * sizeof(ml_queued_state_t));
		memset(Block->States, 0, (ReadIndex + 1) * sizeof(ml_queued_state_t));
		ReadBlock->Next = Block->Next;
		Queue->ReadBlock = Block->Next = ReadBlock;
		Queue->Space += QUEUE_BLOCK_SIZE;
	}
	int Index = Queue->WriteIndex;
	Block->States[Index] = (ml_queued_state_t){State, Value};;
	if (Index == 0) {
		Queue->WriteIndex = QUEUE_BLOCK_SIZE - 1;
		Queue->WriteBlock = Block->Next;
	} else {
		Queue->WriteIndex = Index - 1;
	}
	--Queue->Space;
	return ++Queue->Fill;
}

int ml_scheduler_queue_add(ml_scheduler_queue_t *Queue, ml_state_t *State, ml_value_t *Value) {
#ifdef ML_THREADS
	pthread_mutex_lock(Queue->Lock);
#endif
	int Fill = ml_scheduler_queue_write(Queue, State, Value);
#ifdef ML_THREADS
	pthread_mutex_unlock(Queue->Lock);
#endif
	return Fill;
}

#ifdef ML_THREADS

ml_queued_state_t ml_scheduler_queue_next_wait(ml_scheduler_queue_t *Queue) {
	pthread_mutex_lock(Queue->Lock);
	while (!Queue->Fill) pthread_cond_wait(Queue->Available, Queue->Lock);
	ml_queued_state_t Next = ml_scheduler_queue_read(Queue);
	pthread_mutex_unlock(Queue->Lock);
	return Next;
}

int ml_scheduler_queue_add_signal(ml_scheduler_queue_t *Queue, ml_state_t *State, ml_value_t *Value) {
	int Fill = ml_scheduler_queue_add(Queue, State, Value);
	if (Fill == 1) pthread_cond_signal(Queue->Available);
	return Fill;
}

#else

#define ml_scheduler_queue_next_wait ml_scheduler_queue_next
#define ml_scheduler_queue_add_signal ml_scheduler_queue_add

#endif

void ml_scheduler_queue_run(ml_scheduler_queue_t *Queue) {
	ml_queued_state_t Queued = ml_scheduler_queue_next_wait(Queue);
	Queued.State->run(Queued.State, Queued.Value);
}

/*static
#ifdef ML_THREADS
__thread
#endif
ml_scheduler_t *CurrentScheduler = NULL;*/

ml_scheduler_queue_t *ml_scheduler_queue(int Slice) {
	ml_scheduler_queue_t *Queue = new(ml_scheduler_queue_t);
	Queue->Base.add = (ml_scheduler_add_fn)ml_scheduler_queue_add_signal;
	Queue->Base.run = (ml_scheduler_run_fn)ml_scheduler_queue_run;
	Queue->Base.fill = (ml_scheduler_fill_fn)ml_scheduler_queue_fill;
	ml_queue_block_t *Block = new(ml_queue_block_t);
	Block->Next = Block;
	Queue->WriteBlock = Queue->ReadBlock = Block;
	Queue->WriteIndex = Queue->ReadIndex = QUEUE_BLOCK_SIZE - 1;
	Queue->Space = QUEUE_BLOCK_SIZE;
	Queue->Fill = 0;
#ifdef ML_THREADS
	pthread_mutex_init(Queue->Lock, NULL);
	pthread_cond_init(Queue->Available, NULL);
#endif
	Queue->Slice = Slice;
#ifdef ML_TIMESCHED
	if (Slice) {
		struct itimerval Interval = {0,};
		Interval.it_interval.tv_usec = Slice;
		Interval.it_value.tv_usec = Slice;
		setitimer(ITIMER_REAL, &Interval, NULL);
	}
#else
	Queue->Counter = Slice;
#endif
	return Queue;
}

uint64_t *ml_scheduler_queue_counter(ml_scheduler_queue_t *Queue) {
	return &Queue->Counter;
}

ml_scheduler_queue_t *ml_default_queue_init(ml_context_t *Context, int Slice) {
	ml_scheduler_queue_t *Queue = ml_scheduler_queue(Slice);
	ml_context_set_static(Context, ML_SCHEDULER_INDEX, Queue);
#ifndef ML_TIMESCHED
	ml_context_set_static(Context, ML_COUNTER_INDEX, &Queue->Counter);
#endif
	//CurrentScheduler = (ml_scheduler_t *)Queue;
	return Queue;
}

/*static ml_queued_state_t ml_default_queue_read() {
	return ml_scheduler_queue_read(DefaultQueue);
}

ml_queued_state_t ml_default_queue_next() {
	return ml_scheduler_queue_next(DefaultQueue);
}

static int ml_default_queue_write(ml_state_t *State, ml_value_t *Value) {
	return ml_scheduler_queue_write(DefaultQueue, State, Value);
}

int ml_default_queue_add(ml_state_t *State, ml_value_t *Value) {
	return ml_scheduler_queue_add(DefaultQueue, State, Value);
}*/

#ifdef ML_THREADS

/*ml_queued_state_t ml_default_queue_next_wait() {
	return ml_scheduler_queue_next_wait(DefaultQueue);
}

int ml_default_queue_add_signal(ml_state_t *State, ml_value_t *Value) {
	return ml_scheduler_queue_add_signal(DefaultQueue, State, Value);
}*/

typedef struct ml_scheduler_thread_t ml_scheduler_thread_t;

struct ml_scheduler_thread_t {
	ml_scheduler_thread_t *Next;
	ml_scheduler_t *Scheduler;
	pthread_cond_t Resume[1];
};

struct ml_scheduler_block_t {
	ml_state_t Base;
	ml_scheduler_t *Scheduler;
	pthread_mutex_t Lock[1];
	pthread_cond_t Resume[1];
};

static ml_scheduler_thread_t *NextThread = NULL;
static int MaxIdle = 8;
static pthread_mutex_t ThreadLock[1] = {PTHREAD_MUTEX_INITIALIZER};

static void ml_scheduler_thread_resume(ml_state_t *State, ml_value_t *Value) {
	ml_scheduler_block_t *Block = (ml_scheduler_block_t *)State;
	Block->Scheduler->Resume = Block;
}

static void *ml_scheduler_thread_fn(void *Data) {
	static int NumIdle = 0;
	ml_scheduler_t *Scheduler = (ml_scheduler_t *)Data;
	GC_add_roots(MLArgCache, MLArgCache + ML_ARG_CACHE_SIZE);
	for (;;) {
		Scheduler->run(Scheduler);
		if (Scheduler->Resume) {
			ml_scheduler_block_t *Block = Scheduler->Resume;
			pthread_mutex_lock(Block->Lock);
			pthread_cond_signal(Block->Resume);
			pthread_mutex_unlock(Block->Lock);

			pthread_mutex_lock(ThreadLock);
			if (NumIdle >= MaxIdle) {
				pthread_mutex_unlock(ThreadLock);
				GC_remove_roots(MLArgCache, MLArgCache + ML_ARG_CACHE_SIZE);
				return NULL;
			} else {
				memset(MLArgCache, 0, ML_ARG_CACHE_SIZE * sizeof(ml_value_t *));
			}
			ml_scheduler_thread_t Thread = {NextThread, NULL, {PTHREAD_COND_INITIALIZER}};
			NextThread = &Thread;
			++NumIdle;
			pthread_cond_wait(Thread.Resume, ThreadLock);
			--NumIdle;
			pthread_mutex_unlock(ThreadLock);
			Scheduler = Thread.Scheduler;
		}
	}
	return NULL;
}

void ml_threads_set_max_count(int Max) {
	MaxIdle = Max;
}

void ml_scheduler_split(ml_scheduler_t *Scheduler) {
	pthread_mutex_lock(ThreadLock);
	ml_scheduler_thread_t *Thread = NextThread;
	if (Thread) {
		NextThread = Thread->Next;
		Thread->Scheduler = Scheduler;
		pthread_cond_signal(Thread->Resume);
	} else {
		pthread_attr_t Attr;
		pthread_attr_init(&Attr);
		pthread_attr_setdetachstate(&Attr, PTHREAD_CREATE_DETACHED);
		pthread_t Thread;
		GC_pthread_create(&Thread, &Attr, ml_scheduler_thread_fn, Scheduler);
		pthread_setname_np(Thread, "minilang");
	}
	pthread_mutex_unlock(ThreadLock);
}

void ml_scheduler_join(ml_scheduler_t *Scheduler) {
	ml_scheduler_block_t Block = {
		{NULL, NULL, ml_scheduler_thread_resume},
		Scheduler,
		{PTHREAD_MUTEX_INITIALIZER},
		{PTHREAD_COND_INITIALIZER}
	};
	pthread_mutex_lock(Block.Lock);
	Scheduler->add(Scheduler, (ml_state_t *)&Block, MLNil);
	pthread_cond_wait(Block.Resume, Block.Lock);
	pthread_mutex_unlock(Block.Lock);
	Scheduler->Resume = NULL;
}

#endif

ML_FUNCTIONX(MLAtomic) {
//@atomic
//<Args...:any
//<Fn:function
//>any
// Calls :mini:`Fn(Args)` in a new context without a scheduler and returns the result.
	ML_CHECKX_ARG_COUNT(1);
	ml_state_t *State = ml_state(Caller);
	ml_context_set_static(State->Context, ML_SCHEDULER_INDEX, &DefaultScheduler);
	ml_context_set_static(State->Context, ML_COUNTER_INDEX, &DefaultCounter);
	return ml_call(State, Args[0], Count - 1, Args + 1);
}

#endif

typedef struct {
	ml_state_t *Caller;
	ml_value_t *Fn;
} ml_finalizer_t;

static void ml_finalize(void *Value, ml_finalizer_t *Finalizer) {
	ml_state_t *State = ml_state(Finalizer->Caller);
	State->run = ml_end_state_run;
	return ml_call(State, Finalizer->Fn, 0, NULL);
}

ML_FUNCTIONX(MLFinalizer) {
//@finalize
//<Value:any
//<Fn:function
// Registers :mini:`Fn` as the finalizer for :mini:`Value`.
	ML_CHECKX_ARG_COUNT(2);
	ml_value_t *Value = Args[0];
	ml_finalizer_t *Finalizer = new(ml_finalizer_t);
	Finalizer->Caller = Caller;
	Finalizer->Fn = Args[1];
	GC_register_finalizer_no_order(Value, (GC_finalization_proc)ml_finalize, Finalizer, NULL, NULL);
	ML_RETURN(Value);
}

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
//>semaphore
// Returns a new semaphore with initial value :mini:`Initial` or :mini:`1` if no initial value is specified.
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
// A semaphore for synchronizing concurrent code.
	.Constructor = (ml_value_t *)MLSemaphore
);

static void ml_semaphore_wait(ml_state_t *Caller, ml_semaphore_t *Semaphore) {
	int64_t Value = Semaphore->Value;
	if (Value) {
		Semaphore->Value = Value - 1;
		return ml_state_schedule(Caller, ml_integer(Semaphore->Value));
	}
	if (++Semaphore->Fill > Semaphore->Size) {
		int Size = Semaphore->Size * 2;
		ml_state_t **States = anew(ml_state_t *, Size);
		memcpy(States, Semaphore->States, Semaphore->Size * sizeof(ml_state_t *));
		Semaphore->Read = 0;
		Semaphore->Write = Semaphore->Size;
		Semaphore->States = States;
		Semaphore->Size = Size;
	}
	Semaphore->States[Semaphore->Write] = Caller;
	Semaphore->Write = (Semaphore->Write + 1) % Semaphore->Size;
}

static void ml_semaphore_signal(ml_semaphore_t *Semaphore) {
	int Fill = Semaphore->Fill;
	if (Fill) {
		Semaphore->Fill = Fill - 1;
		ml_state_t *State = Semaphore->States[Semaphore->Read];
		Semaphore->States[Semaphore->Read] = NULL;
		Semaphore->Read = (Semaphore->Read + 1) % Semaphore->Size;
		ml_state_schedule(State, ml_integer(Semaphore->Value));
	} else {
		++Semaphore->Value;
	}
}

ML_METHODX("wait", MLSemaphoreT) {
//!semaphore
//<Semaphore
//>integer
// Waits until the internal value in :mini:`Semaphore` is postive, then decrements it and returns the new value.
	ml_semaphore_t *Semaphore = (ml_semaphore_t *)Args[0];
	int64_t Value = Semaphore->Value;
	if (Value) {
		Semaphore->Value = Value - 1;
		ML_RETURN(ml_integer(Semaphore->Value));
	}
	if (++Semaphore->Fill > Semaphore->Size) {
		int Size = Semaphore->Size * 2;
		ml_state_t **States = anew(ml_state_t *, Size);
		memcpy(States, Semaphore->States, Semaphore->Size * sizeof(ml_state_t *));
		Semaphore->Read = 0;
		Semaphore->Write = Semaphore->Size;
		Semaphore->States = States;
		Semaphore->Size = Size;
	}
	Semaphore->States[Semaphore->Write] = Caller;
	Semaphore->Write = (Semaphore->Write + 1) % Semaphore->Size;
}

ML_METHOD("signal", MLSemaphoreT) {
//!semaphore
//<Semaphore
//>integer
// Increments the internal value in :mini:`Semaphore`, resuming any waiters. Returns the new value.
	ml_semaphore_t *Semaphore = (ml_semaphore_t *)Args[0];
	ml_semaphore_signal(Semaphore);
	return ml_integer(Semaphore->Value);
}

ML_METHOD("value", MLSemaphoreT) {
//!semaphore
//<Semaphore
//>integer
// Returns the internal value in :mini:`Semaphore`.
	ml_semaphore_t *Semaphore = (ml_semaphore_t *)Args[0];
	return ml_integer(Semaphore->Value);
}

typedef struct ml_cond_waiter_t ml_cond_waiter_t;

struct ml_cond_waiter_t {
	ml_cond_waiter_t *Next;
	ml_state_t *State;
	ml_semaphore_t *Semaphore;
};

typedef struct {
	ml_type_t *Type;
	ml_cond_waiter_t *Waiters;
} ml_condition_t;

ML_FUNCTION(MLCondition) {
//!condition
//@condition
//>condition
// Returns a new condition.
	ml_condition_t *Condition = new(ml_condition_t);
	Condition->Type = MLConditionT;
	return (ml_value_t *)Condition;
}

ML_TYPE(MLConditionT, (), "condition",
//!condition
// A condition for synchronizing concurrent code.
	.Constructor = (ml_value_t *)MLCondition
);

ML_METHODX("wait", MLConditionT, MLSemaphoreT) {
//!condition
//<Condition
//<Semaphore
//>integer
// Increments :mini:`Semaphore`, waits until :mini:`Condition` is signalled, then decrements :mini:`Semaphore` (waiting if necessary) and returns its value.
	ml_condition_t *Condition = (ml_condition_t *)Args[0];
	ml_semaphore_t *Semaphore = (ml_semaphore_t *)Args[1];
	ml_cond_waiter_t *Waiter = new(ml_cond_waiter_t);
	Waiter->State = Caller;
	Waiter->Semaphore = Semaphore;
	Waiter->Next = Condition->Waiters;
	Condition->Waiters = Waiter;
	ml_semaphore_signal(Semaphore);
}

ML_METHOD("signal", MLConditionT) {
//!condition
//<Condition
// Signals :mini:`Condition`, resuming a single waiter.
	ml_condition_t *Condition = (ml_condition_t *)Args[0];
	ml_cond_waiter_t *Waiter = Condition->Waiters;
	if (Waiter) {
		Condition->Waiters = Waiter->Next;
		ml_semaphore_wait(Waiter->State, Waiter->Semaphore);
	}
	return MLNil;
}

ML_METHOD("broadcast", MLConditionT) {
//!condition
//<Condition
// Signals :mini:`Condition`, resuming all waiters.
	ml_condition_t *Condition = (ml_condition_t *)Args[0];
	ml_cond_waiter_t *Waiter = Condition->Waiters;
	Condition->Waiters = NULL;
	while (Waiter) {
		ml_semaphore_wait(Waiter->State, Waiter->Semaphore);
		Waiter = Waiter->Next;
	}
	return MLNil;
}

typedef struct {
	ml_state_t *State;
	int Writer;
} ml_rw_waiter_t;

typedef struct {
	ml_type_t *Type;
	ml_rw_waiter_t *Waiters;
	int Readers, Writers;
	int Size, Fill, Write, Read;
} ml_rw_lock_t;

ML_FUNCTION(MLRWLock) {
//!rwlock
//@rwlock
//>rwlock
// Returns a new read-write lock.
	ml_rw_lock_t *RWLock = new(ml_rw_lock_t);
	RWLock->Type = MLRWLockT;
	RWLock->Readers = RWLock->Writers = 0;
	RWLock->Fill = 0;
	RWLock->Size = 4;
	RWLock->Read = RWLock->Write = 0;
	RWLock->Waiters = anew(ml_rw_waiter_t, 4);
	return (ml_value_t *)RWLock;
}

ML_TYPE(MLRWLockT, (), "rwlock",
//!rwlock
// A read-write lock for synchronizing concurrent code.
	.Constructor = (ml_value_t *)MLRWLock
);

ML_METHODX("rdlock", MLRWLockT) {
//!rwlock
//<Lock
// Locks :mini:`Lock` for reading, waiting if there are any writers using or waiting to use :mini:`Lock`.
	ml_rw_lock_t *RWLock = (ml_rw_lock_t *)Args[0];
	if (!RWLock->Writers) {
		++RWLock->Readers;
		ML_RETURN(RWLock);
	}
	if (++RWLock->Fill > RWLock->Size) {
		int Size = RWLock->Size * 2;
		ml_rw_waiter_t *Waiters = anew(ml_rw_waiter_t, Size);
		memcpy(Waiters, RWLock->Waiters, RWLock->Size * sizeof(ml_rw_waiter_t));
		RWLock->Read = 0;
		RWLock->Write = RWLock->Size;
		RWLock->Waiters = Waiters;
		RWLock->Size = Size;
	}
	RWLock->Waiters[RWLock->Write].State = Caller;
	RWLock->Waiters[RWLock->Write].Writer = 0;
	RWLock->Write = (RWLock->Write + 1) % RWLock->Size;
}

ML_METHODX("wrlock", MLRWLockT) {
//!rwlock
//<Lock
// Locks :mini:`Lock` for reading, waiting if there are any readers or other writers using :mini:`Lock`.
	ml_rw_lock_t *RWLock = (ml_rw_lock_t *)Args[0];
	if (!RWLock->Writers && !RWLock->Readers) {
		RWLock->Writers = 1;
		ML_RETURN(RWLock);
	}
	if (++RWLock->Fill > RWLock->Size) {
		int Size = RWLock->Size * 2;
		ml_rw_waiter_t *Waiters = anew(ml_rw_waiter_t, Size);
		memcpy(Waiters, RWLock->Waiters, RWLock->Size * sizeof(ml_rw_waiter_t));
		RWLock->Read = 0;
		RWLock->Write = RWLock->Size;
		RWLock->Waiters = Waiters;
		RWLock->Size = Size;
	}
	RWLock->Writers = 1;
	RWLock->Waiters[RWLock->Write].State = Caller;
	RWLock->Waiters[RWLock->Write].Writer = 1;
	RWLock->Write = (RWLock->Write + 1) % RWLock->Size;
}

ML_METHOD("unlock", MLRWLockT) {
//!rwlock
//<Lock
// Unlocks :mini:`Lock`, resuming any waiting writers or readers (giving preference to writers).
	ml_rw_lock_t *RWLock = (ml_rw_lock_t *)Args[0];
	if (RWLock->Readers) --RWLock->Readers;
	RWLock->Writers = 0;
	while (RWLock->Fill) {
		ml_rw_waiter_t Waiter = RWLock->Waiters[RWLock->Read];
		if (Waiter.Writer) {
			if (!RWLock->Writers && !RWLock->Readers) {
				RWLock->Writers = 1;
				--RWLock->Fill;
				RWLock->Waiters[RWLock->Read].State = NULL;
				RWLock->Read = (RWLock->Read + 1) % RWLock->Size;
				Waiter.State->run(Waiter.State, (ml_value_t *)RWLock);
			} else {
				RWLock->Writers = 1;
			}
			break;
		} else if (!RWLock->Writers) {
			++RWLock->Readers;
			--RWLock->Fill;
			RWLock->Waiters[RWLock->Read].State = NULL;
			RWLock->Read = (RWLock->Read + 1) % RWLock->Size;
			Waiter.State->run(Waiter.State, (ml_value_t *)RWLock);
		} else {
			break;
		}
	}
	return (ml_value_t *)RWLock;
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
	ml_error_t *Error = new(ml_error_t);
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
	ml_error_t *Error = new(ml_error_t);
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
	ml_error_t *Error = new(ml_error_t);
	Error->Type = MLErrorT;
	Error->Error[0] = *(ml_error_value_t *)Args[1];
	ML_CONTINUE(Receiver, Error);
}
*/

#ifdef ML_UNWIND

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <unistd.h>

static void error_handler(int Signal) {
	//backtrace_print(BacktraceState, 0, stderr);
	unw_context_t Context;
	unw_getcontext(&Context);
	unw_cursor_t Cursor;
	unw_init_local2(&Cursor, &Context, UNW_INIT_SIGNAL_FRAME);
	while (unw_step(&Cursor) > 0) {
		unw_word_t Offset;
		char Line[256];
		int Length;
		if (unw_get_proc_name(&Cursor, Line, 240, &Offset)) {
			Length = sprintf(Line, "<unknown>\n");
		} else {
			Length = strlen(Line);
			char *End = Line + Length;
			Length += sprintf(End, "+%lu\n", Offset);
		}
		write(STDERR_FILENO, Line, Length);
	}
	exit(0);
}

#endif

#ifdef ML_BACKTRACE
#include <backtrace.h>

struct backtrace_state *BacktraceState = NULL;

static int ml_backtrace_write(void *Data, uintptr_t PC, const char *Filename, int Lineno, const char *Function) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Data;
	ml_stringbuffer_printf(Buffer, "%08x: %s: %s:%d\n", (unsigned int)PC, Function, Filename, Lineno);
	return 0;
}

int ml_backtrace_full(ml_backtrace_fn Callback, void *Data) {
	return backtrace_full(BacktraceState, 0, Callback, NULL, Data);
}

#endif

ML_FUNCTION(MLBacktrace) {
//@backtrace
#ifdef ML_BACKTRACE
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	backtrace_full(BacktraceState, 0, ml_backtrace_write, NULL, Buffer);
	return ml_stringbuffer_to_string(Buffer);
#endif
	return ml_cstring("");
}

#ifdef ML_BACKTRACE

static int ml_backtrace_to_error(void *Data, uintptr_t PC, const char *Filename, int Lineno, const char *Function) {
	ml_error_t *Error = (ml_error_t *)Data;
	for (int I = 0; I < MAX_TRACE; ++I) if (!Error->Error->Trace[I].Name) {
		Error->Error->Trace[I].Name = Filename;
		Error->Error->Trace[I].Line = Lineno;
		return 0;
	}
	return 1;
}

#endif

static void ml_gc_warn_fn(char *Format, GC_word Arg) {
	ml_error_t Error[1] = {0,};
	Error->Type = MLErrorT;
	Error->Error->Type = MLErrorValueT;
	Error->Error->Error = "";
	Error->Error->Message = "";
	Error->Error->Value = MLNil;
#ifdef ML_BACKTRACE
	backtrace_full(BacktraceState, 1, ml_backtrace_to_error, NULL, Error);
#endif
	ml_log(MLLoggerDefault, ML_LOG_LEVEL_WARN, NULL, "", 0, Format, Arg);
}

static void ml_preempt(int Signal) {
	--MLPreempt;
}

void ml_runtime_init(const char *ExecName) {
#ifdef ML_CONTEXT_SECTION
	MLContextSize = __stop_ml_context_section - __start_ml_context_section;
	MLContextReserved = (1 << MLContextSize) - 1;
	//fprintf(stderr, "Context section size = %d\n", MLContextSize);
#endif
	MLRootContext = xnew(ml_context_t, MLContextSize, void *);
	MLRootContext->Parent = MLRootContext;
	MLRootContext->Size = MLContextSize;
#ifdef ML_TRAMPOLINE
	MLRootQueue = ml_default_queue_init(MLRootContext, 250);
#else
	ml_context_set_static(MLRootContext, ML_SCHEDULER_INDEX, &DefaultScheduler);
	ml_context_set_static(MLRootContext, ML_COUNTER_INDEX, &DefaultCounter);
#endif
	MLEndState->Context = MLRootContext;
#ifdef ML_TIMESCHED
	struct sigaction Action = {0,};
	Action.sa_handler = ml_preempt;
	Action.sa_flags = SA_RESTART;
	sigaction(SIGALRM, &Action, NULL);
#endif
#ifdef ML_UNWIND
	signal(SIGSEGV, error_handler);
	signal(SIGABRT, error_handler);
#endif
	GC_set_warn_proc(ml_gc_warn_fn);
#ifdef ML_BACKTRACE
	BacktraceState = backtrace_create_state(ExecName, 0, NULL, NULL);
#endif
#ifdef ML_THREADS
	//GC_add_roots(&CurrentScheduler, &CurrentScheduler + 1);
	GC_add_roots(MLArgCache, MLArgCache + ML_ARG_CACHE_SIZE);
#endif
	ml_config_register("DEBUGGER", ml_config_debugger);
#include "ml_runtime_init.c"
}

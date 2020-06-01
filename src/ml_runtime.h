#ifndef ML_RUNTIME_H
#define ML_RUNTIME_H

#include "ml_types.h"
#include <limits.h>
#include <stdarg.h>

#ifdef	__cplusplus
extern "C" {
#endif

/****************************** Runtime ******************************/

struct ml_context_t {
	ml_context_t *Parent;
	int Size;
	void *Values[];
};

extern ml_context_t MLRootContext;

ml_context_t *ml_context_new(ml_context_t *Parent);

int ml_context_index_new();

#define ml_context_get(CONTEXT, INDEX) ((CONTEXT)->Size <= (INDEX) ? NULL : (CONTEXT)->Values[(INDEX)])

void ml_context_set(ml_context_t *Context, int Index, void *Value);

typedef void (*ml_state_fn)(ml_state_t *, ml_value_t *);

struct ml_state_t {
	const ml_type_t *Type;
	ml_state_t *Caller;
	ml_state_fn run;
	ml_context_t *Context;
};

extern ml_type_t MLStateT[];

ml_value_t *ml_call(ml_value_t *Value, int Count, ml_value_t **Args);

#define ml_inline(VALUE, COUNT, ARGS ...) ({ \
	void *Args ## __LINE__[COUNT] = {ARGS}; \
	ml_call(VALUE, COUNT, (ml_value_t **)(Args ## __LINE__)); \
})

void ml_default_state_run(ml_state_t *State, ml_value_t *Value);

typedef struct {
	ml_state_t Base;
	ml_value_t *Value;
} ml_value_state_t;

void ml_eval_state_run(ml_value_state_t *State, ml_value_t *Value);

#define ML_EVAL_STATE_INIT {{MLStateT, NULL, (ml_state_fn)ml_eval_state_run, &MLRootContext}, MLNil}

#define ML_WRAP_EVAL(FUNCTION, ARGS...) ({ \
	ml_value_state_t State[1] = {ML_EVAL_STATE_INIT}; \
	FUNCTION((ml_state_t *)State, ARGS); \
	State->Value; \
})

void ml_call_state_run(ml_value_state_t *State, ml_value_t *Value);

#define ML_CALL_STATE_INIT {{MLStateT, NULL, (ml_state_fn)ml_call_state_run, &MLRootContext}, MLNil}

#define ML_WRAP_CALL(FUNCTION, ARGS...) ({ \
	ml_value_state_t State[1] = {ML_CALL_STATE_INIT}; \
	FUNCTION((ml_state_t *)State, ARGS); \
	State->Value; \
})

/****************************** Functions ******************************/

typedef struct ml_function_t ml_function_t;
typedef struct ml_functionx_t ml_functionx_t;

struct ml_function_t {
	const ml_type_t *Type;
	ml_callback_t Callback;
	void *Data;
};

struct ml_functionx_t {
	const ml_type_t *Type;
	ml_callbackx_t Callback;
	void *Data;
};

extern ml_type_t MLFunctionT[];
extern ml_type_t MLFunctionXT[];
extern ml_type_t MLPartialFunctionT[];

ml_value_t *ml_function(void *Data, ml_callback_t Function);
ml_value_t *ml_functionx(void *Data, ml_callbackx_t Function);

ml_value_t *ml_return_nil(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_identity(void *Data, int Count, ml_value_t **Args);

ml_value_t *ml_partial_function_new(ml_value_t *Function, int Count);
ml_value_t *ml_partial_function_set(ml_value_t *Partial, size_t Index, ml_value_t *Value);

#define ML_FUNCTION2(NAME, FUNCTION) static ml_value_t *FUNCTION(void *Data, int Count, ml_value_t **Args); \
\
ml_function_t NAME[1] = {{MLFunctionT, FUNCTION, NULL}}; \
\
static ml_value_t *FUNCTION(void *Data, int Count, ml_value_t **Args)

#define ML_FUNCTION(NAME) ML_FUNCTION2(NAME, CONCAT3(ml_function_, __LINE__, __COUNTER__))

#define ML_FUNCTIONX2(NAME, FUNCTION) static void FUNCTION(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args); \
\
ml_functionx_t NAME[1] = {{MLFunctionXT, FUNCTION, NULL}}; \
\
static void FUNCTION(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args)

#define ML_FUNCTIONX(NAME, TYPES ...) ML_FUNCTIONX2(NAME, CONCAT3(ml_functionx_, __LINE__, __COUNTER__))

#define ML_CHECK_ARG_TYPE(N, TYPE) \
	if (!ml_is(Args[N], TYPE)) { \
		return ml_error("TypeError", "%s required", TYPE->Name); \
	}

#define ML_CHECK_ARG_COUNT(N) \
	if (Count < N) { \
		return ml_error("CallError", "%d arguments required", N); \
	}

#define ML_CHECKX_ARG_TYPE(N, TYPE) \
	if (!ml_is(Args[N], TYPE)) { \
		ML_CONTINUE(Caller, ml_error("TypeError", "%s required", TYPE->Name)); \
	}

#define ML_CHECKX_ARG_COUNT(N) \
	if (Count < N) { \
		ML_CONTINUE(Caller, ml_error("CallError", "%d arguments required", N)); \
	}

#define ML_CONTINUE(STATE, VALUE) { \
	ml_state_t *__State = (ml_state_t *)(STATE); \
	ml_value_t *__Value = (ml_value_t *)(VALUE); \
	return __State->run(__State, __Value); \
}

#define ML_RETURN(VALUE) return Caller->run(Caller, (ml_value_t *)(VALUE))

/****************************** References ******************************/

extern ml_type_t MLReferenceT[];
extern ml_type_t MLUninitializedT[];

typedef ml_value_t *(*ml_getter_t)(void *Data, const char *Name);
typedef ml_value_t *(*ml_setter_t)(void *Data, const char *Name, ml_value_t *Value);

typedef struct ml_reference_t ml_reference_t;

struct ml_reference_t {
	const ml_type_t *Type;
	ml_value_t **Address;
	ml_value_t *Value[];
};

typedef struct ml_source_t {
	const char *Name;
	int Line;
} ml_source_t;

ml_value_t *ml_uninitialized();
void ml_uninitialized_use(ml_value_t *Uninitialized, ml_value_t **Slot);
void ml_uninitialized_set(ml_value_t *Uninitialized, ml_value_t *Value);

/****************************** Errors ******************************/

typedef struct ml_error_t ml_error_t;

extern ml_type_t MLErrorT[];
extern ml_type_t MLErrorValueT[];

ml_value_t *ml_error(const char *Error, const char *Format, ...) __attribute__ ((format(printf, 2, 3)));
ml_value_t *ml_errorv(const char *Error, const char *Format, va_list Args);
const char *ml_error_type(ml_value_t *Value);
const char *ml_error_message(ml_value_t *Value);
int ml_error_trace(ml_value_t *Value, int Level, const char **Source, int *Line);
void ml_error_trace_add(ml_value_t *Error, ml_source_t Source);
void ml_error_print(ml_value_t *Error);

ml_value_t *ml_reference(ml_value_t **Address);

ml_value_t *ml_string_fn(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_list_fn(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_tuple_fn(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_map_fn(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_stringifier_fn(void *Data, int Count, ml_value_t **Args);

/****************************** Debugging ******************************/

#define SIZE_BITS (CHAR_BIT * sizeof(size_t))

typedef struct ml_decl_t ml_decl_t;
typedef struct ml_debugger_t ml_debugger_t;

struct ml_decl_t {
	ml_decl_t *Next;
	const char *Ident;
	ml_value_t *Value;
	int Index;
};

struct ml_debugger_t {
	void (*run)(ml_debugger_t *Debugger, ml_state_t *Frame, ml_value_t *Value);
	size_t *(*breakpoints)(ml_debugger_t *Debugger, const char *Source, int LineNo);
	ml_state_t *StepOverFrame;
	ml_state_t *StepOutFrame;
	size_t Revision;
	int StepIn:1;
	int BreakOnError:1;
};

#define ML_DEBUGGER_INDEX 2

int ml_debugger_check(ml_state_t *State);
ml_source_t ml_debugger_source(ml_state_t *State);
ml_decl_t *ml_debugger_decls(ml_state_t *State);
ml_value_t *ml_debugger_local(ml_state_t *State, int Index);

void ml_runtime_init();

#ifdef	__cplusplus
}
#endif

#endif

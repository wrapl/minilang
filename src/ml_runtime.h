#ifndef ML_RUNTIME_H
#define ML_RUNTIME_H

#include "ml_types.h"
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef	__cplusplus
extern "C" {
#endif

// Runtime //

struct ml_context_t {
	ml_context_t *Parent;
	int Size;
	void *Values[];
};

extern ml_context_t MLRootContext;

ml_context_t *ml_context_new(ml_context_t *Parent) __attribute__((malloc));

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

ml_state_t *ml_state_new(ml_state_t *Caller);

void ml_default_state_run(ml_state_t *State, ml_value_t *Value);

typedef struct {
	ml_state_t Base;
	ml_value_t *Value;
} ml_value_state_t;

void ml_value_state_run(ml_value_state_t *State, ml_value_t *Value);
ml_value_state_t *ml_value_state_new() __attribute__ ((malloc));

void ml_call_state_run(ml_value_state_t *State, ml_value_t *Value);
ml_value_state_t *ml_call_state_new() __attribute__ ((malloc));

ml_value_t *ml_simple_call(ml_value_t *Value, int Count, ml_value_t **Args);

#define ml_simple_inline(VALUE, COUNT, ARGS ...) ({ \
	void *Args ## __LINE__[COUNT] = {ARGS}; \
	ml_simple_call(VALUE, COUNT, (ml_value_t **)(Args ## __LINE__)); \
})

void ml_runtime_init();

// References //

extern ml_type_t MLReferenceT[];
extern ml_type_t MLUninitializedT[];

typedef ml_value_t *(*ml_getter_t)(void *Data, const char *Name);
typedef ml_value_t *(*ml_setter_t)(void *Data, const char *Name, ml_value_t *Value);

typedef struct ml_reference_t ml_reference_t;

struct ml_reference_t {
	const ml_type_t *Type;
	ml_value_t **Address;
};

ml_value_t *ml_reference(ml_value_t **Address) __attribute__((malloc));

typedef struct ml_source_t {
	const char *Name;
	int Line;
} ml_source_t;

ml_value_t *ml_uninitialized(const char *Name) __attribute__((malloc));
void ml_uninitialized_use(ml_value_t *Uninitialized, ml_value_t **Slot);
void ml_uninitialized_set(ml_value_t *Uninitialized, ml_value_t *Value);

// Errors //

typedef struct ml_error_t ml_error_t;

extern ml_type_t MLErrorT[];
extern ml_type_t MLErrorValueT[];

static inline int ml_is_error(ml_value_t *Value) {
#ifdef USE_NANBOXING
	return (!ml_tag(Value)) && (Value->Type == MLErrorT);
#else
	return Value->Type == MLErrorT;
#endif
}

ml_value_t *ml_error(const char *Error, const char *Format, ...) __attribute__ ((malloc, format(printf, 2, 3)));
ml_value_t *ml_errorv(const char *Error, const char *Format, va_list Args) __attribute__ ((malloc));
const char *ml_error_type(const ml_value_t *Value) __attribute__ ((pure));
const char *ml_error_message(const ml_value_t *Value) __attribute__ ((pure));
int ml_error_source(const ml_value_t *Value, int Level, ml_source_t *Source);
ml_value_t *ml_error_trace_add(ml_value_t *Error, ml_source_t Source);
void ml_error_print(const ml_value_t *Error);
void ml_error_fprint(FILE *File, const ml_value_t *Error);

ml_value_t *ml_string_fn(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_list_fn(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_tuple_fn(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_map_fn(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_stringifier_fn(void *Data, int Count, ml_value_t **Args);

// Debugging //

#define SIZE_BITS (CHAR_BIT * sizeof(size_t))

typedef struct ml_decl_t ml_decl_t;
typedef struct ml_debugger_t ml_debugger_t;

struct ml_decl_t {
	ml_decl_t *Next;
	const char *Ident;
	ml_value_t *Value;
	ml_source_t Source;
	int Index, Flags;
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
#define ML_SCHEDULER_INDEX 3

int ml_debugger_check(ml_state_t *State);
ml_source_t ml_debugger_source(ml_state_t *State);
ml_decl_t *ml_debugger_decls(ml_state_t *State);
ml_value_t *ml_debugger_local(ml_state_t *State, int Index);

// Preemption //

typedef struct ml_schedule_t ml_schedule_t;

struct ml_schedule_t {
	unsigned int *Counter;
	void (*swap)(ml_state_t *State, ml_value_t *Value);
};

typedef ml_schedule_t (*ml_scheduler_t)(ml_context_t *Context);

#ifdef	__cplusplus
}
#endif

#endif

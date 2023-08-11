#ifndef ML_RUNTIME_H
#define ML_RUNTIME_H

#include "ml_types.h"
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Runtime //

#define ML_ARG_CACHE_SIZE 64

extern
#ifdef ML_THREADSAFE
__thread
#endif
ml_value_t *MLArgCache[ML_ARG_CACHE_SIZE];

#define ml_alloc_args(COUNT) (((COUNT) <= ML_ARG_CACHE_SIZE) ? MLArgCache : anew(ml_value_t *, COUNT))

//#define ml_alloc_args(COUNT) anew(ml_value_t *, COUNT)

struct ml_context_t {
	ml_context_t *Parent;
	int Size;
	void *Values[];
};

extern ml_context_t MLRootContext;

ml_context_t *ml_context(ml_context_t *Parent) __attribute__((malloc));

int ml_context_index();

#define ml_context_get(CONTEXT, INDEX) ((CONTEXT)->Size <= (INDEX) ? NULL : (CONTEXT)->Values[(INDEX)])

void ml_context_set(ml_context_t *Context, int Index, void *Value);

typedef void (*ml_state_fn)(ml_state_t *State, ml_value_t *Result);

struct ml_state_t {
	ml_type_t *Type;
	ml_state_t *Caller;
	ml_state_fn run;
	ml_context_t *Context;
};

extern ml_type_t MLStateT[];

ml_state_t *ml_state(ml_state_t *Caller) __attribute__ ((malloc));

void ml_default_state_run(ml_state_t *State, ml_value_t *Value);

typedef struct {
	ml_state_t Base;
	ml_value_t *Value;
} ml_result_state_t;

void ml_result_state_run(ml_result_state_t *State, ml_value_t *Value);
ml_result_state_t *ml_result_state(ml_context_t *Context) __attribute__ ((malloc));


typedef struct {
	ml_state_t Base;
	int Count;
	ml_value_t *Args[];
} ml_call_state_t;

ml_call_state_t *ml_call_state(ml_state_t *Caller, int Count) __attribute__ ((malloc));

ml_value_t *ml_simple_call(ml_value_t *Value, int Count, ml_value_t **Args);
ml_value_t *ml_simple_assign(ml_value_t *Value, ml_value_t *Value2);

#define ml_simple_inline(VALUE, COUNT, ARGS ...) ({ \
	ml_simple_call((ml_value_t *)VALUE, COUNT, (ml_value_t **)(void *[]){ARGS}); \
})

void ml_runtime_init();

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Values[];
} ml_iter_state_t;

// References //

extern ml_type_t MLReferenceT[];
extern ml_type_t MLUninitializedT[];

typedef ml_value_t *(*ml_getter_t)(void *Globals, const char *Name, const char *Source, int Line, int Mode);
typedef ml_value_t *(*ml_setter_t)(void *Globals, const char *Name, ml_value_t *Value);

typedef struct ml_reference_t ml_reference_t;

struct ml_reference_t {
	ml_type_t *Type;
	ml_value_t **Address;
};

ml_value_t *ml_reference(ml_value_t **Address) __attribute__((malloc));

typedef struct ml_source_t {
	const char *Name;
	int Line;
} ml_source_t;

ml_value_t *ml_uninitialized(const char *Name, ml_source_t Source) __attribute__((malloc));
void ml_uninitialized_use(ml_value_t *Uninitialized, ml_value_t **Slot);
void ml_uninitialized_set(ml_value_t *Uninitialized, ml_value_t *Value);
const char *ml_uninitialized_name(ml_value_t *Uninitialized);
ml_source_t ml_uninitialized_source(ml_value_t *Uninitialized);

// Errors //

extern ml_type_t MLErrorT[];
extern ml_type_t MLErrorValueT[];
extern ml_cfunction_t MLRaise[];

static inline int ml_is_error(ml_value_t *Value) {
#ifdef ML_NANBOXING
	return (!ml_tag(Value)) && (Value->Type == MLErrorT);
#else
	return Value->Type == MLErrorT;
#endif
}

ml_value_t *ml_error(const char *Error, const char *Format, ...) __attribute__ ((malloc, format(printf, 2, 3)));
ml_value_t *ml_errorv(const char *Error, const char *Format, va_list Args) __attribute__ ((malloc));
ml_value_t *ml_error_unwrap(const ml_value_t *Value);
const char *ml_error_type(const ml_value_t *Value) __attribute__ ((pure));
const char *ml_error_message(const ml_value_t *Value) __attribute__ ((pure));
ml_value_t *ml_error_value(const ml_value_t *Value) __attribute__ ((pure));
int ml_error_source(const ml_value_t *Value, int Level, ml_source_t *Source);
ml_value_t *ml_error_trace_add(ml_value_t *Error, ml_source_t Source);
void ml_error_print(const ml_value_t *Error);
void ml_error_fprint(FILE *File, const ml_value_t *Error);

const char *ml_error_value_type(const ml_value_t *Value) __attribute__ ((pure));
const char *ml_error_value_message(const ml_value_t *Value) __attribute__ ((pure));
int ml_error_value_source(const ml_value_t *Value, int Level, ml_source_t *Source);

// Debugging //

#define SIZE_BITS (CHAR_BIT * sizeof(size_t))

typedef struct ml_decl_t ml_decl_t;
typedef struct ml_debugger_t ml_debugger_t;

struct ml_decl_t {
	ml_decl_t *Next;
	const char *Ident;
	ml_value_t *Value;
	ml_source_t Source;
	long Hash;
	int Index, Flags;
};

#define MLC_DECL_CONSTANT 1
#define MLC_DECL_FORWARD 2
#define MLC_DECL_BACKFILL 4
#define MLC_DECL_BYREF 8
#define MLC_DECL_ASVAR 16

struct ml_debugger_t {
	size_t Revision;
	void (*run)(ml_debugger_t *Debugger, ml_state_t *Frame, ml_value_t *Value);
	size_t *(*breakpoints)(ml_debugger_t *Debugger, const char *Source, int LineNo);
	int StepIn:1;
	int BreakOnError:1;
};

#define ML_DEBUGGER_INDEX 2
#define ML_SCHEDULER_INDEX 3

int ml_debugger_check(ml_state_t *State);
void ml_debugger_step_mode(ml_state_t *State, int StepOver, int StepOut);
ml_source_t ml_debugger_source(ml_state_t *State);
ml_decl_t *ml_debugger_decls(ml_state_t *State);
ml_value_t *ml_debugger_local(ml_state_t *State, int Index);

extern ml_cfunctionx_t MLBreak[];
extern ml_cfunction_t MLDebugger[];
extern ml_cfunctionx_t MLTrace[];

// Preemption //

typedef struct {
	uint64_t Counter;
	void (*add)(ml_state_t *State, ml_value_t *Value);
} ml_schedule_t;

static inline ml_schedule_t *ml_schedule(ml_context_t *Context) {
	return (ml_schedule_t *)Context->Values[ML_SCHEDULER_INDEX];
}

static inline void ml_state_schedule(ml_state_t *State, ml_value_t *Value) {
	ml_schedule(State->Context)->add(State, Value);
}

typedef struct {
	ml_state_t *State;
	ml_value_t *Value;
} ml_queued_state_t;

void ml_default_queue_init(int Size);
ml_queued_state_t ml_default_queue_next();
int ml_default_queue_add(ml_state_t *State, ml_value_t *Value);

#ifdef ML_SCHEDULER
extern ml_cfunctionx_t MLAtomic[];
#endif

#ifdef ML_THREADS
ml_queued_state_t ml_default_queue_next_wait();
void ml_default_queue_add_signal(ml_state_t *State, ml_value_t *Value);

void ml_threads_set_max_count(int Max);
void ml_default_scheduler_block();
void ml_default_scheduler_unblock();

#else
#define ml_default_queue_next_wait ml_default_queue_next
#define ml_default_queue_add_signal ml_default_queue_add
#endif

// Locks

extern ml_type_t MLSemaphoreT[];
extern ml_type_t MLConditionT[];
extern ml_type_t MLRWLockT[];

// Channels

extern ml_type_t MLChannelT[];

#ifdef __cplusplus
}
#endif

#endif

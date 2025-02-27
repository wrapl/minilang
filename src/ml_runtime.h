#ifndef ML_RUNTIME_H
#define ML_RUNTIME_H

/// \defgroup runtime
/// @{
///

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

extern ml_context_t *MLRootContext;

ml_context_t *ml_context(ml_context_t *Parent) __attribute__((malloc));

#ifdef ML_CONTEXT_SECTION

extern __attribute__ ((section("ml_context_section"))) void *ML_METHODS_INDEX[];
extern __attribute__ ((section("ml_context_section"))) void *ML_VARIABLES_INDEX[];
extern __attribute__ ((section("ml_context_section"))) void *ML_DEBUGGER_INDEX[];
extern __attribute__ ((section("ml_context_section"))) void *ML_SCHEDULER_INDEX[];
extern __attribute__ ((section("ml_context_section"))) void *ML_COUNTER_INDEX[];
extern __attribute__ ((section("ml_context_section"))) void *ML_THREAD_INDEX[];

extern __attribute__ ((section("ml_context_section"))) void *__start_ml_context_section[];
extern __attribute__ ((section("ml_context_section"))) void *__stop_ml_context_section[];

static inline void *ml_context_get_static(ml_context_t *Context, void **Index) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
	return Context->Values[Index - __start_ml_context_section];
#pragma GCC diagnostic pop
}

static inline void ml_context_set_static(ml_context_t *Context, void **Index, void *Value) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
	Context->Values[Index - __start_ml_context_section] = Value;
#pragma GCC diagnostic pop
}

#else

enum {
	ML_METHODS_INDEX,
	ML_VARIABLES_INDEX,
	ML_DEBUGGER_INDEX,
	ML_SCHEDULER_INDEX,
	ML_COUNTER_INDEX,
	ML_THREAD_INDEX,
	ML_CONTEXT_SIZE
};

static inline void *ml_context_get_static(ml_context_t *Context, int Index) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
	return Context->Values[Index];
#pragma GCC diagnostic pop
}

static inline void ml_context_set_static(ml_context_t *Context, int Index, void *Value) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
	Context->Values[Index] = Value;
#pragma GCC diagnostic pop
}

#endif

int ml_context_index();
void ml_context_reserve(int Index);

static inline void *ml_context_get_dynamic(ml_context_t *Context, int Index) {
	return Context->Size <= Index ? NULL : Context->Values[Index];
}

static inline void ml_context_set_dynamic(ml_context_t *Context, int Index, void *Value) {
	if (Context->Size > Index) Context->Values[Index] = Value;
}

typedef int (*ml_config_fn)(ml_context_t *Context);

void ml_config_register(const char *Name, ml_config_fn Fn);
ml_config_fn ml_config_lookup(const char *Name);
const char *ml_config_name(void *Fn);

typedef void (*ml_state_fn)(ml_state_t *State, ml_value_t *Result);

/**
 * An execution state.
 */
struct ml_state_t {
	/// The corresponding Minilang type for this state. Can be ``NULL``.
	ml_type_t *Type;
	/// The calling state, normally resumed once this state has completed.
	ml_state_t *Caller;
	/// Function to call to run this state ``State->run(State, Value)``. Can be reassigned as required.
	ml_state_fn run;
	/// The current context, usually inherited from the calling state.
	ml_context_t *Context;
};

extern ml_type_t MLStateT[];

extern ml_state_t MLEndState[];

ml_state_t *ml_state(ml_state_t *Caller) __attribute__ ((malloc));

void ml_state_continue(ml_state_t *State, ml_value_t *Value);

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

typedef struct {
	ml_state_t Base;
	ml_value_t *Iter;
	ml_value_t *Values[];
} ml_iter_state_t;

void ml_sum_optimized(ml_iter_state_t *State, ml_value_t *Value);
void ml_sum_fallback(ml_iter_state_t *State, ml_value_t *Iter, ml_value_t *Total, ml_value_t *Value);

void ml_runtime_init(const char *ExecName);

// Caches //

typedef size_t (*ml_cache_usage_fn)(void *Arg);
typedef void (*ml_cache_clear_fn)(void *Arg);

void ml_cache_register(const char *Name, ml_cache_usage_fn Usage, ml_cache_clear_fn Clear, void *Arg);

typedef void (*ml_cache_usage_callback_fn)(const char *Name, size_t Usage, void *Arg);

void ml_cache_usage(ml_cache_usage_callback_fn Callback, void *Arg);
void ml_cache_clear(const char *Name);
void ml_cache_clear_all();

// Nested Comparisons //

typedef struct {
	ml_state_t Base;
	ml_value_t *A, *B;
} ml_comparison_state_t;

extern ml_type_t MLComparisonStateT[];

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

// Backtraces //

#ifdef ML_BACKTRACE

extern ml_cfunction_t MLBacktrace[];

typedef int (*ml_backtrace_fn)(void *Data, uintptr_t PC, const char *Source, int Line, const char *Function);

int ml_backtrace_full(ml_backtrace_fn Callback, void *Data);

#endif

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
void ml_error_value_set(ml_value_t *Error, ml_value_t *Value);
void ml_error_print(const ml_value_t *Error);
void ml_error_fprint(FILE *File, const ml_value_t *Error);

const char *ml_error_value_type(const ml_value_t *Value) __attribute__ ((pure));
const char *ml_error_value_message(const ml_value_t *Value) __attribute__ ((pure));
int ml_error_value_source(const ml_value_t *Value, int Level, ml_source_t *Source);
ml_value_t *ml_error_value_error(ml_value_t *Value);

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
	unsigned int StepIn:1;
	unsigned int BreakOnError:1;
};

int ml_debugger_check(ml_state_t *State);
void ml_debugger_step_mode(ml_state_t *State, int StepOver, int StepOut);
ml_source_t ml_debugger_source(ml_state_t *State);
ml_decl_t *ml_debugger_decls(ml_state_t *State);
ml_value_t *ml_debugger_local(ml_state_t *State, int Index);

extern ml_cfunctionx_t MLBreak[];
extern ml_cfunctionx_t MLDebugger[];
extern ml_cfunctionx_t MLTrace[];

// Preemption //

extern volatile int MLPreempt;

typedef struct ml_scheduler_t ml_scheduler_t;

typedef int (*ml_scheduler_add_fn)(ml_scheduler_t *Scheduler, ml_state_t *State, ml_value_t *Value);
typedef void (*ml_scheduler_run_fn)(ml_scheduler_t *Scheduler);
typedef int (*ml_scheduler_fill_fn)(ml_scheduler_t *Scheduler);

static inline ml_scheduler_t *ml_context_get_scheduler(ml_context_t *Context) {
	return (ml_scheduler_t *)ml_context_get_static(Context, ML_SCHEDULER_INDEX);
}

#ifdef ML_THREADS

typedef struct ml_scheduler_block_t ml_scheduler_block_t;

#endif

struct ml_scheduler_t {
	ml_scheduler_add_fn add;
	ml_scheduler_run_fn run;
	ml_scheduler_fill_fn fill;
#ifdef ML_THREADS
	ml_scheduler_block_t *Resume;
#endif
};

static inline void ml_state_schedule(ml_state_t *State, ml_value_t *Value) {
	ml_scheduler_t *Scheduler = (ml_scheduler_t *)ml_context_get_static(State->Context, ML_SCHEDULER_INDEX);
	Scheduler->add(Scheduler, State, Value);
}

typedef struct {
	ml_state_t *State;
	ml_value_t *Value;
} ml_queued_state_t;

typedef struct ml_scheduler_queue_t ml_scheduler_queue_t;

ml_scheduler_queue_t *ml_scheduler_queue(int Slice);
uint64_t *ml_scheduler_queue_counter(ml_scheduler_queue_t *Queue);

ml_scheduler_queue_t *ml_default_queue_init(ml_context_t *Context, int Slice);

int ml_scheduler_queue_size(ml_scheduler_queue_t *Queue);
int ml_scheduler_queue_fill(ml_scheduler_queue_t *Queue);

ml_queued_state_t ml_scheduler_queue_next(ml_scheduler_queue_t *Queue);
int ml_scheduler_queue_add(ml_scheduler_queue_t *Queue, ml_state_t *State, ml_value_t *Value);

ml_queued_state_t ml_scheduler_queue_next_wait(ml_scheduler_queue_t *Queue);
int ml_scheduler_queue_add_signal(ml_scheduler_queue_t *Queue, ml_state_t *State, ml_value_t *Value);

#ifdef ML_SCHEDULER
extern ml_cfunctionx_t MLAtomic[];
#endif

extern ml_cfunctionx_t MLFinalizer[];

#define ML_STATE_FN2(NAME, FUNCTION) \
static void FUNCTION(ml_state_t *State, ml_value_t *Value); \
\
static ml_state_t NAME[1] = {{MLStateT, NULL, FUNCTION, NULL}}; \
\
static void FUNCTION(ml_state_t *State, ml_value_t *Value)

#define ML_STATE_FN(NAME) ML_STATE_FN2(NAME, CONCAT3(ml_state_fn_, __LINE__, __COUNTER__))

#ifdef ML_THREADS

void ml_threads_set_max_count(int Max);

void ml_scheduler_split(ml_scheduler_t *Scheduler);
void ml_scheduler_join(ml_scheduler_t *Scheduler);

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

/// @}

#endif

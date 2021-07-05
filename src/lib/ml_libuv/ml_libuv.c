#include "ml_libuv.h"

uv_loop_t *Loop;
static uv_idle_t Idle[1];

static unsigned int MLUVCounter = 100;

static void ml_uv_resume(uv_idle_t *Idle) {
	ml_queued_state_t QueuedState = ml_scheduler_queue_next();
	if (QueuedState.State) {
		MLUVCounter = 100;
		QueuedState.State->run(QueuedState.State, QueuedState.Value);
	} else {
		uv_idle_stop(Idle);
	}
}

static void ml_uv_swap(ml_state_t *State, ml_value_t *Value) {
	if (ml_scheduler_queue_add(State, Value) == 1) {
		uv_idle_start(Idle, ml_uv_resume);
	}
}

static ml_schedule_t ml_uv_scheduler(ml_context_t *Context) {
	return (ml_schedule_t){&MLUVCounter, ml_uv_swap};
}

ML_FUNCTIONX(Run) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLFunctionT);
	ml_state_t *State = ml_state_new(Caller);
	ml_context_set(State->Context, ML_SCHEDULER_INDEX, ml_uv_scheduler);
	ml_value_t *Function = Args[0];
	ml_call(State, Function, 0, NULL);
	uv_run(Loop, UV_RUN_DEFAULT);
}

void *ml_calloc(size_t Count, size_t Size) {
	return GC_malloc(Count * Size);
}

void ml_free(void *Ptr) {
}

extern void ml_libuv_file_init(stringmap_t *Globals);
extern void ml_libuv_process_init(stringmap_t *Globals);

void ml_library_entry(ml_value_t *Module, ml_getter_t GlobalGet, void *Globals) {
	uv_replace_allocator(GC_malloc, GC_realloc, ml_calloc, ml_free);
	Loop = uv_default_loop();
	uv_idle_init(Loop, Idle);
#include "ml_libuv_init.c"
	ml_libuv_file_init(((ml_module_t *)Module)->Exports);
	ml_libuv_process_init(((ml_module_t *)Module)->Exports);
	ml_module_export(Module, "run", (ml_value_t *)Run);
}


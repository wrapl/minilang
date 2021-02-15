#include "minilang.h"
#include "ml_console.h"
#include "ml_compiler.h"
#include "ml_macros.h"
#include "ml_file.h"
#include "ml_object.h"
#include "ml_iterfns.h"
#include "stringmap.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <gc.h>

#ifdef ML_MATH
#include "ml_math.h"
#include "ml_array.h"
#endif

#ifdef ML_IO
#include "ml_io.h"
#endif

#ifdef ML_GIR
#include "gtk_console.h"
#include "ml_gir.h"
#endif

#ifdef ML_CBOR
#include "ml_cbor.h"
#endif

#ifdef ML_MODULES
#include "ml_module.h"
#include "ml_library.h"
#endif

#ifdef ML_TABLES
#include "ml_table.h"
#endif

#ifdef ML_QUEUES
#include "ml_queue.h"
#endif

static stringmap_t Globals[1] = {STRINGMAP_INIT};

static ml_value_t *global_get(void *Data, const char *Name) {
	return stringmap_search(Globals, Name);
}

ML_FUNCTION(MLNow) {
	return ml_integer(time(NULL));
}

ML_FUNCTION(MLClock) {
	struct timespec Time[1];
	clock_gettime(CLOCK_REALTIME, Time);
	return ml_real(Time->tv_sec + Time->tv_nsec / 1000000000.0);
}

static int ml_stringbuffer_print(FILE *File, const char *String, size_t Length) {
	fwrite(String, 1, Length, File);
	return 0;
}

ML_FUNCTION(MLPrint) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = ml_stringbuffer_append(Buffer, Args[I]);
		if (ml_is_error(Result)) return Result;
	}
	ml_stringbuffer_foreach(Buffer, stdout, (void *)ml_stringbuffer_print);
	fflush(stdout);
	return MLNil;
}

ML_FUNCTION(MLHalt) {
	if (Count > 0) {
		ML_CHECK_ARG_TYPE(0, MLIntegerT);
		exit(ml_integer_value_fast(Args[0]));
	} else {
		exit(0);
	}
}

ML_FUNCTION(MLCollect) {
	GC_gcollect();
	return MLNil;
}

ML_FUNCTIONX(MLTest) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	const char *Test = ml_string_value(Args[0]);
	if (!strcmp(Test, "methods")) {
		ml_state_t *State = ml_state_new(Caller);
		ml_methods_context_new(State->Context);
		ml_value_t *Function = Args[1];
		return ml_call(State, Function, Count - 2, Args + 2);
	}
	ML_ERROR("ValueError", "Unknown test %s", Test);
}

#ifdef ML_MODULES
static stringmap_t Modules[1] = {STRINGMAP_INIT};

ML_FUNCTIONX(Import) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	const char *FileName = realpath(ml_string_value(Args[0]), NULL);
	if (!FileName) ML_RETURN(ml_error("ModuleError", "File %s not found", ml_string_value(Args[0])));
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Modules, FileName);
	if (!Slot[0]) {
		const char *Extension = strrchr(FileName, '.');
		if (!Extension) ML_RETURN(ml_error("ModuleError", "Unknown module type: %s", FileName));
		if (!strcmp(Extension, ".so")) {
			return ml_library_load_file(Caller, FileName, (ml_getter_t)stringmap_search, Globals, Slot);
		} else if (!strcmp(Extension, ".mini")) {
			return ml_module_load_file(Caller, FileName, (ml_getter_t)stringmap_search, Globals, Slot);
		} else {
			ML_RETURN(ml_error("ModuleError", "Unknown module type: %s", FileName));
		}
	}
	ML_RETURN(Slot[0]);
}

ML_FUNCTION(Unload) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *FileName = realpath(ml_string_value(Args[0]), NULL);
	if (!FileName) return ml_error("ModuleError", "File %s not found", ml_string_value(Args[0]));
	stringmap_remove(Modules, FileName);
	return MLNil;
}

#endif

#ifdef ML_SCHEDULER

typedef struct {
	ml_state_t *State;
	ml_value_t *Value;
} ml_queued_state_t;

static ml_queued_state_t *QueuedStates;
static int QueueSize, QueueFill, QueueWrite, QueueRead;
static unsigned int SliceSize = 0, Counter;

static void simple_queue_run() {
	while (QueueFill) {
		ml_queued_state_t QueuedState = QueuedStates[QueueRead];
		QueuedStates[QueueRead].State = NULL;
		QueuedStates[QueueRead].Value = NULL;
		--QueueFill;
		QueueRead = (QueueRead + 1) % QueueSize;
		Counter = SliceSize;
		QueuedState.State->run(QueuedState.State, QueuedState.Value);
	}
}

static void simple_swap_state(ml_state_t *State, ml_value_t *Value) {
	//printf("\e[34mSwapping state\n\e[0m");
	++QueueFill;
	if (QueueFill > QueueSize) {
		int NewQueueSize = QueueSize * 2;
		ml_queued_state_t *NewQueuedStates = anew(ml_queued_state_t, NewQueueSize);
		memcpy(NewQueuedStates, QueuedStates, QueueSize * sizeof(ml_queued_state_t));
		QueueRead = 0;
		QueueWrite = QueueSize;
		QueuedStates = NewQueuedStates;
		QueueSize = NewQueueSize;
	}
	QueuedStates[QueueWrite].State = State;
	QueuedStates[QueueWrite].Value = Value;
	QueueWrite = (QueueWrite + 1) % QueueSize;
}

static ml_schedule_t simple_scheduler(ml_context_t *Context) {
	return (ml_schedule_t){&Counter, simple_swap_state};
}

#endif

int main(int Argc, const char *Argv[]) {
	static const char *Parameters[] = {"Args", NULL};
	ml_init();
	ml_types_init(Globals);
	ml_file_init(Globals);
	ml_object_init(Globals);
	ml_iterfns_init(Globals);
	stringmap_insert(Globals, "now", MLNow);
	stringmap_insert(Globals, "clock", MLClock);
	stringmap_insert(Globals, "print", MLPrint);
	stringmap_insert(Globals, "error", MLErrorValueT);
	stringmap_insert(Globals, "raise", MLRaise);
	stringmap_insert(Globals, "halt", MLHalt);
	stringmap_insert(Globals, "collect", MLCollect);
	stringmap_insert(Globals, "callcc", MLCallCC);
	stringmap_insert(Globals, "markcc", MLMarkCC);
	stringmap_insert(Globals, "calldc", MLCallDC);
	stringmap_insert(Globals, "swapcc", MLSwapCC);
	stringmap_insert(Globals, "channel", MLChannelT);
	stringmap_insert(Globals, "context", MLContextKeyT);
	stringmap_insert(Globals, "compiler", MLCompilerT);
	stringmap_insert(Globals, "global", ml_stringmap_globals(Globals));
	stringmap_insert(Globals, "test", MLTest);
#ifdef ML_CBOR
	ml_cbor_init(Globals);
#endif
#ifdef ML_MATH
	ml_math_init(Globals);
	ml_array_init(Globals);
#endif
#ifdef ML_IO
	ml_io_init(Globals);
#endif
#ifdef ML_GIR
	ml_gir_init(Globals);
	int GtkConsole = 0;
#endif
#ifdef ML_MODULES
	ml_module_init(Globals);
	ml_library_init(Globals);
	stringmap_insert(Globals, "import", Import);
	stringmap_insert(Globals, "unload", Unload);
#endif
#ifdef ML_TABLES
	ml_table_init(Globals);
#endif
#ifdef ML_QUEUES
	ml_queue_init(Globals);
#endif
	ml_value_t *Args = ml_list();
	const char *FileName = 0;
#ifdef ML_MODULES
	const char *ModuleName = 0;
#endif
	for (int I = 1; I < Argc; ++I) {
		if (Argv[I][0] == '-') {
			switch (Argv[I][1]) {
#ifdef ML_MODULES
			case 'm':
				if (++I >= Argc) {
					printf("Error: module name required\n");
					exit(-1);
				}
				ModuleName = Argv[I];
			break;
#endif
#ifdef ML_SCHEDULER
			case 's':
				if (++I >= Argc) {
					printf("Error: module name required\n");
					exit(-1);
				}
				SliceSize = atoi(Argv[I]);
			break;
#endif
			case 'z': GC_disable(); break;
#ifdef ML_GIR
			case 'G': GtkConsole = 1; break;
#endif
			}
		} else if (!FileName) {
			FileName = Argv[I];
		} else {
			ml_list_append(Args, ml_cstring(Argv[I]));
		}
	}
#ifdef ML_SCHEDULER
	if (SliceSize) {
		Counter = SliceSize;
		QueueFill = 0;
		QueueSize = 4;
		QueueRead = QueueWrite = 0;
		QueuedStates = anew(ml_queued_state_t, QueueSize);
		ml_context_set(&MLRootContext, ML_SCHEDULER_INDEX, simple_scheduler);
	}
#endif
	if (FileName) {
		ml_call_state_t *State = ml_call_state_new(MLMain, 1);
		State->Args[0] = Args;
		ml_load_file((ml_state_t *)State, global_get, NULL, FileName, Parameters);
#ifdef ML_SCHEDULER
		if (SliceSize) simple_queue_run();
#endif
#ifdef ML_MODULES
	} else if (ModuleName) {
		ml_inline(MLMain, (ml_value_t *)Import, 1, ml_string(ModuleName, -1));
#ifdef ML_SCHEDULER
		if (SliceSize) simple_queue_run();
#endif
#endif
#ifdef ML_GIR
	} else if (GtkConsole) {
		console_t *Console = console_new(&MLRootContext, (ml_getter_t)stringmap_search, Globals);
		stringmap_insert(Globals, "print", ml_cfunction(Console, (void *)console_print));
		console_show(Console, NULL);
		gtk_main();
#endif
	} else {
		ml_console(&MLRootContext, (ml_getter_t)stringmap_search, Globals, "--> ", "... ");
	}
	return 0;
}

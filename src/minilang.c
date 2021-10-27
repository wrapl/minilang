#include "minilang.h"
#include "ml_console.h"
#include "ml_compiler.h"
#include "ml_macros.h"
#include "ml_file.h"
#include "ml_object.h"
#include "stringmap.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <gc.h>
#include "ml_sequence.h"

#ifdef ML_MATH
#include "ml_math.h"
#include "ml_array.h"
#endif

#ifdef ML_IO
#include "ml_io.h"
#endif

#ifdef ML_GIR
#include "ml_gir.h"
#endif

#ifdef ML_GTK_CONSOLE
#include "gtk_console.h"
#endif

#ifdef ML_CBOR
#include "ml_cbor.h"
#endif

#ifdef ML_JSON
#include "ml_json.h"
#endif

#ifdef ML_XML
#include "ml_xml.h"
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

#ifdef ML_TIME
#include "ml_time.h"
#endif

#ifdef ML_UUID
#include "ml_uuid.h"
#endif

#ifdef ML_JSENCODE
#include "ml_jsencode.h"
#endif

#ifdef ML_SQLITE
#include "ml_sqlite.h"
#endif

#ifdef ML_RAPC
#include "ml_rapc.h"
#endif

#ifdef ML_BACKTRACE
#include <backtrace.h>
#endif

static stringmap_t Globals[1] = {STRINGMAP_INIT};

static ml_value_t *global_get(void *Data, const char *Name) {
	return stringmap_search(Globals, Name);
}

ML_FUNCTION(MLNow) {
//@now
	return ml_integer(time(NULL));
}

ML_FUNCTION(MLClock) {
//@clock
	struct timespec Time[1];
	clock_gettime(CLOCK_REALTIME, Time);
	return ml_real(Time->tv_sec + Time->tv_nsec / 1000000000.0);
}

static int ml_stringbuffer_print(FILE *File, const char *String, size_t Length) {
	fwrite(String, 1, Length, File);
	return 0;
}

ML_FUNCTION(MLPrint) {
//@print
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
//@halt
	if (Count > 0) {
		ML_CHECK_ARG_TYPE(0, MLIntegerT);
		exit(ml_integer_value_fast(Args[0]));
	} else {
		exit(0);
	}
}

ML_FUNCTION(MLCollect) {
//@collect
	GC_gcollect();
	return MLNil;
}

static int ml_globals_add(const char *Name, ml_value_t *Value, ml_value_t *Result) {
	ml_map_insert(Result, ml_cstring(Name), Value);
	return 0;
}

static ml_value_t *ml_globals(stringmap_t *Globals, int Count, ml_value_t **Args) {
	ml_value_t *Result = ml_map();
	stringmap_foreach(Globals, Result, (void *)ml_globals_add);
	return Result;
}

#ifdef ML_MODULES
static stringmap_t Modules[1] = {STRINGMAP_INIT};

ML_FUNCTIONX(Import) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	const char *FileName = realpath(ml_string_value(Args[0]), NULL);
	if (!FileName) ML_ERROR("ModuleError", "File %s not found", ml_string_value(Args[0]));
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(Modules, FileName);
	if (!Slot[0]) {
		const char *Extension = strrchr(FileName, '.');
		if (!Extension) ML_ERROR("ModuleError", "Unknown module type: %s", FileName);
		if (!strcmp(Extension, ".so")) {
			return ml_library_load_file(Caller, FileName, (ml_getter_t)stringmap_search, Globals, Slot);
		} else if (!strcmp(Extension, ".mini")) {
			return ml_module_load_file(Caller, FileName, (ml_getter_t)stringmap_search, Globals, Slot);
		} else {
			ML_ERROR("ModuleError", "Unknown module type: %s", FileName);
		}
	}
	ML_RETURN(Slot[0]);
}

static ml_value_t *LibraryPath;

#include "whereami.h"
#include <sys/stat.h>

static void add_library_path(void) {
	int ExecutablePathLength = wai_getExecutablePath(NULL, 0, NULL);
	char *ExecutablePath = snew(ExecutablePathLength + 1);
	wai_getExecutablePath(ExecutablePath, ExecutablePathLength + 1, &ExecutablePathLength);
	ExecutablePath[ExecutablePathLength] = 0;
	for (int I = ExecutablePathLength - 1; I > 0; --I) {
		if (ExecutablePath[I] == '/') {
			ExecutablePath[I] = 0;
			ExecutablePathLength = I;
			break;
		}
	}
	int LibPathLength = ExecutablePathLength + strlen("/lib/minilang");
	char *LibPath = snew(LibPathLength + 1);
	memcpy(LibPath, ExecutablePath, ExecutablePathLength);
	strcpy(LibPath + ExecutablePathLength, "/lib/minilang");
	//printf("Looking for library path at %s\n", LibPath);
	struct stat Stat[1];
	if (!lstat(LibPath, Stat) && S_ISDIR(Stat->st_mode)) {
		ml_list_put(LibraryPath, ml_string(LibPath, LibPathLength));
	}
}

ML_FUNCTIONX(Library) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	const char *Name = ml_string_value(Args[0]);
	int NameLength = ml_string_length(Args[0]);
	int MaxLength = 0;
	ML_LIST_FOREACH(LibraryPath, Iter) {
		if (!ml_is(Iter->Value, MLStringT)) ML_ERROR("TypeError", "Invalid entry in library path");
		int FullLength = ml_string_length(Iter->Value) + NameLength + 10;
		if (MaxLength < FullLength) MaxLength = FullLength;
	}
	char *FileName = snew(MaxLength);
	struct stat Stat[1];
	ML_LIST_FOREACH(LibraryPath, Iter) {
		char *End = stpcpy(FileName, ml_string_value(Iter->Value));
		*End++ = '/';
		End = stpcpy(End, Name);
		strcpy(End, ".mini");
		if (!lstat(FileName, Stat) && S_ISREG(Stat->st_mode)) {
			ml_value_t **Slot = (ml_value_t **)stringmap_slot(Modules, FileName);
			if (!Slot[0]) {
				return ml_module_load_file(Caller, FileName, (ml_getter_t)stringmap_search, Globals, Slot);
			}
			ML_RETURN(Slot[0]);
		}
		strcpy(End, ".so");
		if (!lstat(FileName, Stat) && S_ISREG(Stat->st_mode)) {
			ml_value_t **Slot = (ml_value_t **)stringmap_slot(Modules, FileName);
			if (!Slot[0]) {
				return ml_library_load_file(Caller, FileName, (ml_getter_t)stringmap_search, Globals, Slot);
			}
			ML_RETURN(Slot[0]);
		}
	}
	ML_ERROR("ModuleError", "Library %s not found", Name);
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

static unsigned int SliceSize = 0;
static uint64_t Counter;

static void simple_queue_run() {
	ml_queued_state_t QueuedState;
	for (;;) {
		QueuedState = ml_scheduler_queue_next();
		if (!QueuedState.State) break;
		Counter = SliceSize;
		QueuedState.State->run(QueuedState.State, QueuedState.Value);
	}
}

static ml_schedule_t simple_scheduler(ml_context_t *Context) {
	return (ml_schedule_t){&Counter, (void *)ml_scheduler_queue_add};
}

#endif

#ifdef ML_BACKTRACE
struct backtrace_state *BacktraceState = NULL;

static void error_handler(int Signal) {
	backtrace_print(BacktraceState, 0, stderr);
	exit(0);
}
#endif

int main(int Argc, const char *Argv[]) {
#ifdef ML_BACKTRACE
	BacktraceState = backtrace_create_state(Argv[0], 0, NULL, NULL);
	signal(SIGSEGV, error_handler);
#endif
	ml_init();
	ml_types_init(Globals);
	ml_file_init(Globals);
	ml_object_init(Globals);
	ml_sequence_init(Globals);
	stringmap_insert(Globals, "now", MLNow);
	stringmap_insert(Globals, "clock", MLClock);
	stringmap_insert(Globals, "print", MLPrint);
	stringmap_insert(Globals, "error", MLErrorValueT);
	stringmap_insert(Globals, "raise", MLRaise);
	stringmap_insert(Globals, "halt", MLHalt);
	stringmap_insert(Globals, "break", MLBreak);
	stringmap_insert(Globals, "debugger", MLDebugger);
	stringmap_insert(Globals, "collect", MLCollect);
	stringmap_insert(Globals, "callcc", MLCallCC);
	stringmap_insert(Globals, "markcc", MLMarkCC);
	stringmap_insert(Globals, "calldc", MLCallDC);
	stringmap_insert(Globals, "swapcc", MLSwapCC);
	stringmap_insert(Globals, "channel", MLChannelT);
	stringmap_insert(Globals, "semaphore", MLSemaphoreT);
	stringmap_insert(Globals, "context", MLContextKeyT);
	stringmap_insert(Globals, "parser", MLParserT);
	stringmap_insert(Globals, "compiler", MLCompilerT);
	stringmap_insert(Globals, "macro", MLMacroT);
	stringmap_insert(Globals, "global", ml_stringmap_globals(Globals));
	stringmap_insert(Globals, "globals", ml_cfunction(Globals, (void *)ml_globals));
#ifdef ML_CBOR
	ml_cbor_init(Globals);
#endif
#ifdef ML_JSON
	ml_json_init(Globals);
#endif
#ifdef ML_XML
	ml_xml_init(Globals);
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
#endif
#ifdef ML_GTK_CONSOLE
	int GtkConsole = 0;
#endif
#ifdef ML_MODULES
	ml_module_init(Globals);
	ml_library_init(Globals);
	stringmap_insert(Globals, "import", Import);
	stringmap_insert(Globals, "unload", Unload);
	stringmap_insert(Globals, "library", Library);
	LibraryPath = ml_list();
	add_library_path();
#endif
#ifdef ML_TABLES
	ml_table_init(Globals);
#endif
#ifdef ML_QUEUES
	ml_queue_init(Globals);
#endif
#ifdef ML_TIME
	ml_time_init(Globals);
#endif
#ifdef ML_UUID
	ml_uuid_init(Globals);
#endif
#ifdef ML_JSENCODE
	ml_jsencode_init(Globals);
#endif
#ifdef ML_SQLITE
	ml_sqlite_init(Globals);
#endif
#ifdef ML_RAPC
	ml_rapc_init(Globals);
#endif
	ml_value_t *Args = ml_list();
	const char *FileName = 0;
#ifdef ML_MODULES
	int LoadModule = 0;
#endif
	for (int I = 1; I < Argc; ++I) {
		if (FileName) {
			ml_list_append(Args, ml_cstring(Argv[I]));
		} else if (Argv[I][0] == '-') {
			switch (Argv[I][1]) {
#ifdef ML_MODULES
			case 'm':
				if (++I >= Argc) {
					printf("Error: module name required\n");
					exit(-1);
				}
				FileName = Argv[I];
				LoadModule = 1;
			break;
#endif
#ifdef ML_SCHEDULER
			case 's':
				if (++I >= Argc) {
					printf("Error: slice size required\n");
					exit(-1);
				}
				SliceSize = atoi(Argv[I]);
			break;
#endif
			case 'z': GC_disable(); break;
#ifdef ML_GTK_CONSOLE
			case 'G':
				GtkConsole = 1;
#ifdef ML_SCHEDULER
				if (!SliceSize) SliceSize = 1000;
#endif
				break;
#endif
			}
		} else {
			FileName = Argv[I];
		}
	}
#ifdef ML_SCHEDULER
	if (SliceSize) {
		Counter = SliceSize;
		ml_scheduler_queue_init(4);
		ml_context_set(&MLRootContext, ML_SCHEDULER_INDEX, simple_scheduler);
	}
#endif
	if (FileName) {
#ifdef ML_MODULES
		if (LoadModule) {
			ml_inline(MLMain, (ml_value_t *)Import, 1, ml_string(FileName, -1));
		} else {
#endif
		ml_call_state_t *State = ml_call_state_new(MLMain, 1);
		State->Args[0] = Args;
		ml_load_file((ml_state_t *)State, global_get, NULL, FileName, NULL);
#ifdef ML_MODULES
		}
#endif
#ifdef ML_SCHEDULER
		if (SliceSize) simple_queue_run();
#endif
#ifdef ML_GTK_CONSOLE
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

#include "minilang.h"
#include "ml_console.h"
#include "ml_compiler.h"
#include "ml_bytecode.h"
#include "ml_macros.h"
#include "ml_file.h"
#include "ml_object.h"
#include "ml_expr.h"
#include "stringmap.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <gc.h>
#include "ml_sequence.h"
#include "ml_stream.h"
#include "ml_tasks.h"

#ifdef ML_MATH
#include "ml_math.h"
#include "ml_array.h"
#include "ml_polynomial.h"
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

#ifdef ML_XE
#include "ml_xe.h"
#endif

#ifdef ML_MODULES
#include "ml_module.h"
#include "ml_library.h"
#endif

#ifdef ML_THREADS
#include "ml_thread.h"
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
//<Values..:any
//>nil
// Prints :mini:`Values` to standard output, converting to strings if necessary.
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = ml_stringbuffer_simple_append(Buffer, Args[I]);
		if (ml_is_error(Result)) return Result;
	}
	ml_stringbuffer_foreach(Buffer, stdout, (void *)ml_stringbuffer_print);
	fflush(stdout);
	return MLNil;
}

ML_FUNCTION(MLHalt) {
//@halt
//<Code?:integer
//
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
	ml_map_insert(Result, ml_string(Name, -1), Value);
	return 0;
}

static ml_value_t *ml_globals(stringmap_t *Globals, int Count, ml_value_t **Args) {
	ml_value_t *Result = ml_map();
	stringmap_foreach(Globals, Result, (void *)ml_globals_add);
	return Result;
}

#ifdef ML_SCHEDULER

static unsigned int SliceSize = 256;
static ml_value_t *MainResult = NULL;
static ml_schedule_t MainSchedule = {256, (void *)ml_scheduler_queue_add_signal};

static void simple_queue_run() {
	while (!MainResult) {
		ml_queued_state_t QueuedState = ml_scheduler_queue_next_wait();
		MainSchedule.Counter = SliceSize;
		QueuedState.State->run(QueuedState.State, QueuedState.Value);
	}
}

#endif

#ifdef ML_BACKTRACE
struct backtrace_state *BacktraceState = NULL;

static void error_handler(int Signal) {
	backtrace_print(BacktraceState, 0, stderr);
	exit(0);
}
#endif

static void ml_main_state_run(ml_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		fprintf(stderr, "%s: %s\n", ml_error_type(Value), ml_error_message(Value));
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Value, Level++, &Source)) {
			fprintf(stderr, "\t%s:%d\n", Source.Name, Source.Line);
		}
		exit(1);
	}
#ifdef ML_SCHEDULER
	MainResult = Value;
#endif
}

int main(int Argc, const char *Argv[]) {
#ifdef ML_BACKTRACE
	BacktraceState = backtrace_create_state(Argv[0], 0, NULL, NULL);
	signal(SIGSEGV, error_handler);
#endif

	ml_init(Globals);
	ml_expr_init(Globals);
	ml_file_init(Globals);
	ml_object_init(Globals);
	ml_sequence_init(Globals);
	ml_tasks_init(Globals);
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
	stringmap_insert(Globals, "variable", MLVariableT);
	stringmap_insert(Globals, "global", ml_stringmap_globals(Globals));
	stringmap_insert(Globals, "globals", ml_cfunction(Globals, (void *)ml_globals));

#ifdef ML_LIBRARY
	ml_library_init(Globals);
	ml_module_t *Sys = ml_library_internal("sys");
	ml_module_t *Std = ml_library_internal("std");
	ml_module_t *Fmt = ml_library_internal("fmt");
	ml_module_t *IO = ml_library_internal("io");
#define SYS_EXPORTS Sys->Exports
#define STD_EXPORTS Std->Exports
#define FMT_EXPORTS Fmt->Exports
#define IO_EXPORTS IO->Exports
#else
#define SYS_EXPORTS Globals
#define STD_EXPORTS Globals
#define FMT_EXPORTS Globals
#define IO_EXPORTS Globals
#endif

	ml_stream_init(IO_EXPORTS);
	stringmap_insert(IO_EXPORTS, "terminal", ml_module("terminal",
		"stdin", ml_fd_stream(STDIN_FILENO),
		"stdout", ml_fd_stream(STDOUT_FILENO),
		"stderr", ml_fd_stream(STDERR_FILENO),
	NULL));
#ifdef ML_CBOR
	ml_cbor_init(FMT_EXPORTS);
#endif
#ifdef ML_JSON
	ml_json_init(FMT_EXPORTS);
#endif
#ifdef ML_XML
	ml_xml_init(FMT_EXPORTS);
#endif
#ifdef ML_XE
	ml_xe_init(FMT_EXPORTS);
#endif
#ifdef ML_MATH
	ml_math_init(Globals);
	ml_array_init(Globals);
	ml_polynomial_init(Globals);
#endif
#ifdef ML_GIR
	ml_gir_init(Globals);
#endif
#ifdef ML_GTK_CONSOLE
	int GtkConsole = 0;
	gtk_console_init();
#endif
#ifdef ML_MODULES
	ml_module_init(Globals);
#endif
#ifdef ML_TABLES
	ml_table_init(Globals);
#endif
#ifdef ML_QUEUES
	ml_queue_init(Globals);
#endif
#ifdef ML_TIME
	ml_time_init(STD_EXPORTS);
#endif
#ifdef ML_UUID
	ml_uuid_init(STD_EXPORTS);
#endif
#ifdef ML_JSENCODE
	ml_jsencode_init(Globals);
#endif
#ifdef ML_THREADS
	ml_thread_init(SYS_EXPORTS);
#endif
	ml_value_t *Args = ml_list();
	const char *FileName = NULL;
#ifdef ML_MODULES
	int LoadModule = 0;
#endif
	const char *Command = NULL;
	for (int I = 1; I < Argc; ++I) {
		if (FileName) {
			ml_list_put(Args, ml_string(Argv[I], -1));
		} else if (Argv[I][0] == '-') {
			switch (Argv[I][1]) {
			case 'E':
				if (Argv[I][2]) {
					Command = Argv[I] + 2;
				} else if (++I < Argc) {
					Command = Argv[I];
				} else {
					fprintf(stderr, "Error: command required\n");
					exit(-1);
				}
				break;
#ifdef ML_MODULES
			case 'm':
				if (Argv[I][2]) {
					FileName = Argv[I] + 2;
					LoadModule = 1;
				} else if (++I < Argc) {
					FileName = Argv[I];
					LoadModule = 1;
				} else {
					fprintf(stderr, "Error: module name required\n");
					exit(-1);
				}
				break;
#endif
#ifdef ML_LIBRARY
			case 'L':
				if (Argv[I][2]) {
					ml_library_path_add(Argv[I] + 2);
				} else if (++I < Argc) {
					ml_library_path_add(Argv[I]);
				} else {
					fprintf(stderr, "Error: library path required\n");
					exit(-1);
				}
				break;
#endif
#ifdef ML_SCHEDULER
			case 's':
				if (Argv[I][2]) {
					SliceSize = atoi(Argv[I] + 2);
				} else if (++I < Argc) {
					SliceSize = atoi(Argv[I]);
				} else {
					fprintf(stderr, "Error: slice size required\n");
					exit(-1);
				}
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
	ml_state_t *Main = ml_state(NULL);
	Main->run = ml_main_state_run;
#ifdef ML_SCHEDULER
	if (SliceSize) {
		MainSchedule.Counter = SliceSize;
		ml_scheduler_queue_init(8);
		ml_context_set(Main->Context, ML_SCHEDULER_INDEX, &MainSchedule);
	}
#endif
#ifdef ML_GTK_CONSOLE
	if (GtkConsole) {
		gtk_console_t *Console = gtk_console(&MLRootContext, (ml_getter_t)stringmap_search, Globals);
		gtk_console_show(Console, NULL);
		if (FileName) gtk_console_load_file(Console, FileName, Args);
		if (Command) gtk_console_evaluate(Console, Command);
		gtk_main();
		return 0;
	}
#endif
	if (FileName) {
#ifdef ML_LIBRARY
		if (LoadModule) {
			ml_library_load(Main, NULL, FileName);
		} else {
#endif
		ml_call_state_t *State = ml_call_state(Main, 1);
		State->Args[0] = Args;
		ml_load_file((ml_state_t *)State, global_get, NULL, FileName, NULL);
#ifdef ML_LIBRARY
		}
#endif
#ifdef ML_SCHEDULER
		if (SliceSize) simple_queue_run();
#endif
	} else if (Command) {
		ml_parser_t *Parser = ml_parser(NULL, NULL);
		ml_compiler_t *Compiler = ml_compiler(global_get, NULL);
		ml_parser_input(Parser, Command);
		ml_command_evaluate(Main, Parser, Compiler);
	} else {
		ml_console(&MLRootContext, (ml_getter_t)stringmap_search, Globals, "--> ", "... ");
	}
	return 0;
}

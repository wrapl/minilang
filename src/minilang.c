#include "minilang.h"
#include "ml_console.h"
#include "ml_compiler.h"
#include "ml_bytecode.h"
#include "ml_macros.h"
#include "ml_file.h"
#include "ml_socket.h"
#include "ml_object.h"
#include "ml_time.h"
#include "ml_debugger.h"
#include "stringmap.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include "ml_sequence.h"
#include "ml_stream.h"
#include "ml_logging.h"
#include <sys/fcntl.h>

#ifdef ML_MATH
#include "ml_math.h"
#include "ml_array.h"
#include "ml_polynomial.h"
#endif

#ifdef ML_AST
#include "ml_ast.h"
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

#ifdef ML_SCHEDULER
#include "ml_tasks.h"
#endif

#ifdef ML_TABLES
#include "ml_table.h"
#endif

#ifdef ML_PQUEUES
#include "ml_pqueue.h"
#endif

#ifdef ML_UUID
#include "ml_uuid.h"
#endif

#ifdef ML_MINIJS
#include "ml_minijs.h"
#endif

#ifdef ML_ENCODINGS
#include "ml_base16.h"
#include "ml_base64.h"
#endif

#ifdef ML_STRUCT
#include "ml_struct.h"
#endif

#ifdef ML_MMAP
#include "ml_mmap.h"
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "minilang"

stringmap_t MLGlobals[1] = {STRINGMAP_INIT};

static ml_value_t *global_get(void *Data, const char *Name, const char *Source, int Line, int Mode) {
	return stringmap_search(MLGlobals, Name);
}

#ifdef ML_CONTEXT_SECTION

__attribute__ ((section("ml_context_section"))) void *MLContextTest[1] = {NULL};

#endif

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
//<Values...:any
//>nil
// Prints :mini:`Values` to standard output, converting to strings if necessary.
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = ml_stringbuffer_simple_append(Buffer, Args[I]);
		if (ml_is_error(Result)) return Result;
	}
	ml_stringbuffer_drain(Buffer, stdout, (void *)ml_stringbuffer_print);
	fflush(stdout);
	return MLNil;
}

ML_FUNCTION(MLLocale) {
//@locale
//>string
	return ml_string(setlocale(LC_ALL, NULL), -1);
}

ML_FUNCTION(MLHalt) {
//@halt
//<Code?:integer
// Causes the current process to exit with optional exit code :mini:`Code` or :mini:`0` if omitted.
	if (Count > 0) {
		ML_CHECK_ARG_TYPE(0, MLIntegerT);
		exit(ml_integer_value_fast(Args[0]));
	} else {
		exit(0);
	}
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


#ifdef GC_BACKTRACE

#include <gc/gc_backptr.h>

int BreakOnExit = 0;

#endif

static void ml_main_state_run(ml_state_t *State, ml_value_t *Value) {
	int ExitVal;
	if (ml_is_error(Value)) {
		fprintf(stderr, "%s: %s\n", ml_error_type(Value), ml_error_message(Value));
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Value, Level++, &Source)) {
			fprintf(stderr, "\t%s:%d\n", Source.Name, Source.Line);
		}
		ExitVal = 1;
	} else {
		ExitVal = 0;
	}
#ifdef GC_BACKTRACE
	if (BreakOnExit) GC_generate_random_backtrace();
#endif
	exit(ExitVal);
}

static void ml_main_state_module(ml_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		fprintf(stderr, "%s: %s\n", ml_error_type(Value), ml_error_message(Value));
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Value, Level++, &Source)) {
			fprintf(stderr, "\t%s:%d\n", Source.Name, Source.Line);
		}
#ifdef GC_BACKTRACE
		if (BreakOnExit) GC_generate_random_backtrace();
#endif
		exit(1);
	}

}

extern ml_cfunction_t MLMemAddress[];
extern ml_cfunction_t MLMemTrace[];
extern ml_cfunction_t MLMemSize[];
extern ml_cfunction_t MLMemCollect[];
extern ml_cfunction_t MLMemUsage[];
extern ml_cfunction_t MLMemDump[];

typedef struct {
	ml_state_t Base;
	FILE *File;
} console_state_t;

static void console_run(console_state_t *State, ml_value_t *Value) {
	ml_file_console(State->Base.Context, (ml_getter_t)ml_stringmap_global_get, MLGlobals, "--> ", "... ", State->File, State->File);
}

ML_FUNCTIONX(MLConsole) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLIntegerT);
	int Fd = ml_integer_value(Args[0]);
#ifndef Mingw
	fcntl(Fd, F_SETFL, fcntl(Fd, F_GETFL) & ~O_NONBLOCK);
#endif
	console_state_t *State = new(console_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)console_run;
	State->File = fdopen(Fd, "r+");
	ml_scheduler_t *Scheduler = ml_context_get_static(Caller->Context, ML_SCHEDULER_INDEX);
	Scheduler->add(Scheduler, (ml_state_t *)State, MLNil);
	ML_RETURN(MLNil);
}

int main(int Argc, const char *Argv[]) {
	ml_init(Argv[0], MLGlobals);
	ml_sequence_init(MLGlobals);
	ml_object_init(MLGlobals);
	ml_time_init(MLGlobals);
#ifdef ML_STRUCT
	ml_struct_init(MLGlobals);
#endif

#ifdef ML_SCHEDULER
	ml_tasks_init(MLGlobals);
#endif
#ifdef ML_AST
	ml_ast_init(MLGlobals);
#endif
#ifdef ML_BACKTRACE
	stringmap_insert(MLGlobals, "backtrace", MLBacktrace);
#endif
	stringmap_insert(MLGlobals, "now", MLNow);
	stringmap_insert(MLGlobals, "clock", MLClock);
	stringmap_insert(MLGlobals, "print", MLPrint);
	stringmap_insert(MLGlobals, "locale", MLLocale);
	stringmap_insert(MLGlobals, "halt", MLHalt);
	stringmap_insert(MLGlobals, "break", MLBreak);
	stringmap_insert(MLGlobals, "debugger", MLDebugger);
	stringmap_insert(MLGlobals, "trace", MLTrace);
	stringmap_insert(MLGlobals, "memory", ml_module("memory",
		"address", MLMemAddress,
		"trace", MLMemTrace,
		"size", MLMemSize,
		"collect", MLMemCollect,
		"usage", MLMemUsage,
		"dump", MLMemDump,
	NULL));
	stringmap_insert(MLGlobals, "console", MLConsole);
	stringmap_insert(MLGlobals, "callcc", MLCallCC);
	stringmap_insert(MLGlobals, "markcc", MLMarkCC);
	stringmap_insert(MLGlobals, "calldc", MLCallDC);
	stringmap_insert(MLGlobals, "swapcc", MLSwapCC);
	stringmap_insert(MLGlobals, "channel", MLChannelT);
	stringmap_insert(MLGlobals, "semaphore", MLSemaphoreT);
	stringmap_insert(MLGlobals, "condition", MLConditionT);
	stringmap_insert(MLGlobals, "rwlock", MLRWLockT);
#ifdef ML_SCHEDULER_
	stringmap_insert(MLGlobals, "atomic", MLAtomic);
#endif
	stringmap_insert(MLGlobals, "finalize", MLFinalizer);
	stringmap_insert(MLGlobals, "context", MLContextKeyT);
	stringmap_insert(MLGlobals, "parser", MLParserT);
	stringmap_insert(MLGlobals, "compiler", MLCompilerT);
	stringmap_insert(MLGlobals, "macro", MLMacroT);
	stringmap_insert(MLGlobals, "variable", MLVariableT);
	stringmap_insert(MLGlobals, "global", ml_stringmap_globals(MLGlobals));
	stringmap_insert(MLGlobals, "globals", ml_cfunction(MLGlobals, (void *)ml_globals));

	ml_logging_init(MLGlobals);

#ifdef ML_LIBRARY
	ml_library_init(MLGlobals);
	ml_module_t *Sys = ml_library_internal("sys");
	stringmap_insert(MLGlobals, "sys", Sys);
	ml_module_t *Std = ml_library_internal("std");
	stringmap_insert(MLGlobals, "std", Std);
	ml_module_t *Fmt = ml_library_internal("fmt");
	stringmap_insert(MLGlobals, "fmt", Fmt);
	ml_module_t *IO = ml_library_internal("io");
	stringmap_insert(MLGlobals, "io", IO);
	ml_module_t *Util = ml_library_internal("util");
	stringmap_insert(MLGlobals, "util", Util);
	ml_module_t *Enc = ml_library_internal("enc");
	stringmap_insert(MLGlobals, "enc", Enc);
#define SYS_EXPORTS Sys->Exports
#define STD_EXPORTS Std->Exports
#define FMT_EXPORTS Fmt->Exports
#define IO_EXPORTS IO->Exports
#define UTIL_EXPORTS Util->Exports
#define ENC_EXPORTS Enc->Exports
#else
#define SYS_EXPORTS MLGlobals
#define STD_EXPORTS MLGlobals
#define FMT_EXPORTS MLGlobals
#define IO_EXPORTS MLGlobals
#define UTIL_EXPORTS MLGlobals
#define ENC_EXPORTS MLGlobals
#endif

	ml_stream_init(IO_EXPORTS);
	ml_file_init(MLGlobals);
	ml_socket_init(MLGlobals);
#ifdef ML_MMAP
	ml_mmap_init(MLGlobals);
#endif
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
	ml_math_init(MLGlobals);
	ml_array_init(MLGlobals);
	ml_polynomial_init(MLGlobals);
#endif
#ifdef ML_MODULES
	ml_module_init(MLGlobals);
#endif
#ifdef ML_TABLES
	ml_table_init(MLGlobals);
#endif
#ifdef ML_PQUEUES
	ml_pqueue_init(UTIL_EXPORTS);
#endif
#ifdef ML_UUID
	ml_uuid_init(UTIL_EXPORTS);
#endif
#ifdef ML_MINIJS
	ml_minijs_init(FMT_EXPORTS);
#endif
#ifdef ML_ENCODINGS
	ml_base16_init(ENC_EXPORTS);
	ml_base64_init(ENC_EXPORTS);
#endif
#ifdef ML_THREADS
	ml_thread_init(SYS_EXPORTS);
#endif
	ml_value_t *Args = ml_list();
	const char *MainModule = NULL;
#ifdef ML_MODULES
	int LoadModule = 0;
#endif
#ifdef ML_SCHEDULER
	int SliceSize = 250;
#endif
	const char *DebugAddr = NULL;
	const char *Command = NULL;
	for (int I = 1; I < Argc; ++I) {
		if (MainModule) {
			ml_list_put(Args, ml_string(Argv[I], -1));
		} else if (Argv[I][0] == '-') {
			switch (Argv[I][1]) {
			case 'V': {
				printf("%d.%d.%d\n", MINILANG_VERSION);
				exit(0);
			}
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
					MainModule = Argv[I] + 2;
					LoadModule = 1;
				} else if (++I < Argc) {
					MainModule = Argv[I];
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
			case 'D': {
				ml_log_level_set(ML_LOG_LEVEL_DEBUG);
				break;
			}
			case 'J': {
				if (Argv[I][2]) {
					DebugAddr = Argv[I] + 2;
				} else if (++I < Argc) {
					DebugAddr = Argv[I];
				} else {
					fprintf(stderr, "Error: debug address required\n");
					exit(-1);
				}
				break;
			}
#ifdef GC_BACKTRACE
			case 'B':
				BreakOnExit = 1;
				break;
#endif
			case '-': {
				if (!strcmp(Argv[I] + 2, "gc:disable")) {
					GC_disable();
				} else if (!strcmp(Argv[I] + 2, "gc:maxheap")) {
					if (++I < Argc) {
						char *End;
						size_t Limit = strtol(Argv[I], &End, 10);
						switch (*End) {
						case 'k': Limit <<= 10; break;
						case 'M': Limit <<= 20; break;
						case 'G': Limit <<= 30; break;
						}
						GC_set_max_heap_size(Limit);
					} else {
						fprintf(stderr, "Error: slice size required\n");
						exit(-1);
					}
				}
				break;
			}
			}
		} else {
			MainModule = Argv[I];
		}
	}
	ml_state_t *Main = ml_state(NULL);
	Main->run = ml_main_state_run;
#ifdef ML_SCHEDULER
	if (SliceSize) ml_default_queue_init(Main->Context, SliceSize);
#endif
	if (DebugAddr) ml_remote_debugger_init(Main->Context, DebugAddr);
#ifdef ML_THREADS
	ml_default_thread_init(Main->Context);
#endif
#ifdef ML_LIBRARY
	stringmap_insert(Sys->Exports, "Args", Args);
#endif
	if (MainModule) {
#ifdef ML_LIBRARY
		if (LoadModule) {
			Main->run = ml_main_state_module;
			ml_library_load(Main, NULL, MainModule);
		} else {
#endif
		ml_call_state_t *State = ml_call_state(Main, 1);
		State->Args[0] = Args;
		ml_load_file((ml_state_t *)State, global_get, NULL, MainModule, NULL);
#ifdef ML_LIBRARY
		}
#endif
	} else if (Command) {
		ml_parser_t *Parser = ml_parser(NULL, NULL);
		ml_compiler_t *Compiler = ml_compiler(global_get, NULL);
		ml_parser_input(Parser, Command, 0);
		ml_command_evaluate(Main, Parser, Compiler);
	} else {
		ml_console(Main->Context, (ml_getter_t)ml_stringmap_global_get, MLGlobals, "--> ", "... ");
	}
#ifdef ML_SCHEDULER
	for (;;) {
		ml_scheduler_t *Scheduler = ml_context_get_static(Main->Context, ML_SCHEDULER_INDEX);
		Scheduler->run(Scheduler);
	}
#endif
	return 0;
}

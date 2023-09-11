#include "minilang.h"
#include "ml_console.h"
#include "ml_compiler.h"
#include "ml_bytecode.h"
#include "ml_macros.h"
#include "ml_file.h"
#include "ml_object.h"
#include "stringmap.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include "ml_sequence.h"
#include "ml_stream.h"
//#include "ml_struct.h"

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

#ifdef ML_TIME
#include "ml_time.h"
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

#undef ML_CATEGORY
#define ML_CATEGORY "minilang"

static stringmap_t Globals[1] = {STRINGMAP_INIT};

static ml_value_t *global_get(void *Data, const char *Name, const char *Source, int Line, int Mode) {
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
//<Values...:any
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

#ifdef ML_BACKTRACE
#include <backtrace.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <unistd.h>

struct backtrace_state *BacktraceState = NULL;

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

static int ml_backtrace_fn(void *Data, uintptr_t PC, const char *Filename, int Lineno, const char *Function) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Data;
	ml_stringbuffer_printf(Buffer, "%08x: %s: %s:%d\n", (unsigned int)PC, Function, Filename, Lineno);
	return 0;
}

ML_FUNCTION(MLBacktrace) {
//@backtrace
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	backtrace_full(BacktraceState, 0, ml_backtrace_fn, NULL, Buffer);
	return ml_stringbuffer_to_string(Buffer);
}

#endif

static ml_value_t *MainResult = NULL;

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

extern ml_cfunction_t MLMemTrace[];
extern ml_cfunction_t MLMemSize[];
extern ml_cfunction_t MLMemCollect[];

int main(int Argc, const char *Argv[]) {
#ifdef ML_BACKTRACE
	BacktraceState = backtrace_create_state(Argv[0], 0, NULL, NULL);
	signal(SIGSEGV, error_handler);
	stringmap_insert(Globals, "backtrace", MLBacktrace);
#endif
	ml_init(Globals);
	ml_sequence_init(Globals);
	ml_object_init(Globals);
	//ml_struct_init(Globals);

#ifdef ML_SCHEDULER
	ml_tasks_init(Globals);
#endif
#ifdef ML_AST
	ml_ast_init(Globals);
#endif
	stringmap_insert(Globals, "now", MLNow);
	stringmap_insert(Globals, "clock", MLClock);
	stringmap_insert(Globals, "print", MLPrint);
	stringmap_insert(Globals, "locale", MLLocale);
	stringmap_insert(Globals, "raise", MLRaise);
	stringmap_insert(Globals, "halt", MLHalt);
	stringmap_insert(Globals, "break", MLBreak);
	stringmap_insert(Globals, "debugger", MLDebugger);
	stringmap_insert(Globals, "trace", MLTrace);
	stringmap_insert(Globals, "memory", ml_module("memory",
		"trace", MLMemTrace,
		"size", MLMemSize,
		"collect", MLMemCollect,
	NULL));
	stringmap_insert(Globals, "callcc", MLCallCC);
	stringmap_insert(Globals, "markcc", MLMarkCC);
	stringmap_insert(Globals, "calldc", MLCallDC);
	stringmap_insert(Globals, "swapcc", MLSwapCC);
	stringmap_insert(Globals, "channel", MLChannelT);
	stringmap_insert(Globals, "semaphore", MLSemaphoreT);
	stringmap_insert(Globals, "condition", MLConditionT);
	stringmap_insert(Globals, "rwlock", MLRWLockT);
#ifdef ML_SCHEDULER_
	stringmap_insert(Globals, "atomic", MLAtomic);
#endif
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
	stringmap_insert(Globals, "sys", Sys);
	ml_module_t *Std = ml_library_internal("std");
	stringmap_insert(Globals, "std", Std);
	ml_module_t *Fmt = ml_library_internal("fmt");
	stringmap_insert(Globals, "fmt", Fmt);
	ml_module_t *IO = ml_library_internal("io");
	stringmap_insert(Globals, "io", IO);
	ml_module_t *Util = ml_library_internal("util");
	stringmap_insert(Globals, "util", Util);
#define SYS_EXPORTS Sys->Exports
#define STD_EXPORTS Std->Exports
#define FMT_EXPORTS Fmt->Exports
#define IO_EXPORTS IO->Exports
#define UTIL_EXPORTS Util->Exports
#else
#define SYS_EXPORTS Globals
#define STD_EXPORTS Globals
#define FMT_EXPORTS Globals
#define IO_EXPORTS Globals
#define UTIL_EXPORTS Globals
#endif

	ml_stream_init(IO_EXPORTS);
	ml_file_init(Globals);
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
	int UseGirLoop = 0;
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
#ifdef ML_PQUEUES
	ml_pqueue_init(UTIL_EXPORTS);
#endif
#ifdef ML_TIME
	ml_time_init(Globals);
#endif
#ifdef ML_UUID
	ml_uuid_init(UTIL_EXPORTS);
#endif
#ifdef ML_MINIJS
	ml_minijs_init(FMT_EXPORTS);
#endif
#ifdef ML_ENCODINGS
	ml_base16_init(Globals);
	ml_base64_init(Globals);
#endif
#ifdef ML_THREADS
	ml_thread_init(SYS_EXPORTS);
#endif
	ml_value_t *Args = ml_list();
	const char *FileName = NULL;
#ifdef ML_MODULES
	int LoadModule = 0;
#endif
	int BreakOnExit = 0;
#ifdef ML_SCHEDULER
	int SliceSize = 256;
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
#ifdef ML_GIR
			case 'g':
				UseGirLoop = 1;
#ifdef ML_SCHEDULER
				if (!SliceSize) SliceSize = 1000;
#endif
				break;
#endif
#ifdef ML_GTK_CONSOLE
			case 'G':
				UseGirLoop = 1;
				GtkConsole = 1;
#ifdef ML_SCHEDULER
				if (!SliceSize) SliceSize = 1000;
#endif
				break;
#endif
			case 'B':
				BreakOnExit = 1;
				break;
			}
		} else {
			FileName = Argv[I];
		}
	}
	ml_state_t *Main = ml_state(NULL);
	Main->run = ml_main_state_run;
#ifdef ML_SCHEDULER
	if (SliceSize) {
		ml_default_queue_init(Main->Context, SliceSize);
#ifdef ML_GIR
		if (UseGirLoop) ml_gir_loop_init(Main->Context);
#endif
	}
#endif
#ifdef ML_LIBRARY
	stringmap_insert(Sys->Exports, "Args", Args);
#endif
#ifdef ML_GTK_CONSOLE
	if (GtkConsole) {
		gtk_console_t *Console = gtk_console(&MLRootContext, (ml_getter_t)stringmap_global_get, Globals);
		gtk_console_show(Console, NULL);
		if (FileName) gtk_console_load_file(Console, FileName, Args);
		if (Command) gtk_console_evaluate(Console, Command);
		ml_gir_loop_run();
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
#ifdef ML_GIR
		if (UseGirLoop) {
			ml_gir_loop_run();
		} else {
#endif
		if (SliceSize) {
			while (!MainResult) {
				ml_queued_state_t Queued = ml_default_queue_next_wait();
				Queued.State->run(Queued.State, Queued.Value);
			}
		}
#ifdef ML_GIR
		}
		if (BreakOnExit) {
#ifdef GC_DEBUG
			GC_generate_random_backtrace();
#endif
		}
#endif
#endif
	} else if (Command) {
		ml_parser_t *Parser = ml_parser(NULL, NULL);
		ml_compiler_t *Compiler = ml_compiler(global_get, NULL);
		ml_parser_input(Parser, Command);
		ml_command_evaluate(Main, Parser, Compiler);
#ifdef ML_SCHEDULER
		if (SliceSize) {
			while (!MainResult) {
				ml_queued_state_t Queued = ml_default_queue_next_wait();
				Queued.State->run(Queued.State, Queued.Value);
			}
		}
#endif
	} else {
		ml_console(&MLRootContext, (ml_getter_t)stringmap_global_get, Globals, "--> ", "... ");
	}
	return 0;
}

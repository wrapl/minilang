#include "minilang.h"
#include "ml_console.h"
#include "ml_compiler.h"
#include "ml_bytecode.h"
#include "ml_macros.h"
#include "ml_file.h"
#include "ml_socket.h"
#include "ml_object.h"
#include "ml_time.h"
#include "stringmap.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include "ml_sequence.h"
#include "ml_stream.h"
#include "ml_logging.h"
#include <string.h>
#include <emscripten.h>
#include <emscripten/html5.h>


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


typedef struct {
	ml_state_t Base;
	ml_parser_t *Parser;
	ml_compiler_t *Compiler;
	ml_value_t *Result;
} ml_session_t;

#define ML_SESSION_INDEX 6

static ml_session_t *Sessions = NULL;
static int NumSessions = 0, MaxSessions = 0;

EM_JS(void, _ml_output, (int Index, const char *Text), {
	ml_output(Index, UTF8ToString(Text));
});

EM_JS(void, _ml_finish, (int Index), {
	ml_finish(Index);
});

static void ml_session_run(ml_session_t *Session, ml_value_t *Value) {
	GC_gcollect();
	if (Value == MLEndOfInput) {
		Session->Result = Value;
		return _ml_finish(Session - Sessions);
	}
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	if (ml_is_error(Value)) {
	error:
		ml_stringbuffer_clear(Buffer);
		ml_stringbuffer_printf(Buffer, "%s: %s\n", ml_error_type(Value), ml_error_message(Value));
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Value, Level++, &Source)) {
			ml_stringbuffer_printf(Buffer, "\t%s:%d\n", Source.Name, Source.Line);
		}
	} else {
		Value = ml_stringbuffer_simple_append(Buffer, Value);
		if (ml_is_error(Value)) goto error;
		ml_stringbuffer_write(Buffer, "\n", strlen("\n"));
	}
	_ml_output(Session - Sessions, ml_stringbuffer_get_string(Buffer));
	ml_command_evaluate((ml_state_t *)Session, Session->Parser, Session->Compiler);
}

ML_FUNCTIONX(MLPrint) {
	ml_session_t *Session = ml_context_get_static(Caller->Context, ML_SESSION_INDEX);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = ml_stringbuffer_simple_append(Buffer, Args[I]);
		if (ml_is_error(Result)) ML_RETURN(Result);
	}
	_ml_output(Session - Sessions, ml_stringbuffer_get_string(Buffer));
	ML_RETURN(MLNil);
}

typedef struct {
	ml_context_t *Context;
	ml_value_t *Fn;
} callback_info_t;

typedef struct {
	ml_type_t *Type;
	union {
		const EmscriptenKeyboardEvent *KeyboardEvent;
		const EmscriptenMouseEvent *MouseEvent;
		const EmscriptenWheelEvent *WheelEvent;
		const EmscriptenUiEvent *UiEvent;
		const EmscriptenFocusEvent *FocusEvent;
	};
} event_t;

ML_TYPE(EventT, (), "event");

#define CALLBACK_FN(PREFIX, EVENT_TYPE) \
\
ML_TYPE(EVENT_TYPE ## EventT, (EventT), #PREFIX "event"); \
\
static bool PREFIX ##_callback(int EventType, const Emscripten ## EVENT_TYPE ## Event * EVENT_TYPE ## Event, void *Data) { \
	callback_info_t *Info = (callback_info_t *)Data; \
	ml_result_state_t *State = ml_result_state(Info->Context); \
	event_t *Event = new(event_t); \
	Event->Type = EVENT_TYPE ## EventT; \
	Event->EVENT_TYPE ## Event = EVENT_TYPE ## Event; \
	ml_value_t **Args = ml_alloc_args(2); \
	Args[0] = (ml_value_t *)Event; \
	ml_call(State, Info->Fn, 2, Args); \
	ml_scheduler_t *Scheduler = ml_context_get_static(Info->Context, ML_SCHEDULER_INDEX); \
	while (!State->Value) { \
		GC_gcollect(); \
		Scheduler->run(Scheduler); \
	} \
	return State->Value == MLTrue; \
}

CALLBACK_FN(key, Keyboard)
CALLBACK_FN(mouse, Mouse)
CALLBACK_FN(wheel, Wheel)
CALLBACK_FN(ui, Ui)
CALLBACK_FN(focus, Focus)

static callback_info_t **Callbacks = NULL;
static int MaxCallbacks = 0, NumCallbacks = 0;

static int callback_register(callback_info_t *Info) {

}

static void callback_unregister(int Index) {

}

stringmap_t MLGlobals[1] = {STRINGMAP_INIT};

static int ml_globals_add(const char *Name, ml_value_t *Value, ml_value_t *Result) {
	ml_map_insert(Result, ml_string(Name, -1), Value);
	return 0;
}

static ml_value_t *ml_globals(stringmap_t *Globals, int Count, ml_value_t **Args) {
	ml_value_t *Result = ml_map();
	stringmap_foreach(Globals, Result, (void *)ml_globals_add);
	return Result;
}

extern ml_cfunction_t MLMemTrace[];
extern ml_cfunction_t MLMemSize[];
extern ml_cfunction_t MLMemCollect[];
extern ml_cfunction_t MLMemUsage[];
extern ml_cfunction_t MLMemDump[];

void initialize() {
	ml_context_reserve(ML_SESSION_INDEX);
	ml_init("minilang", MLGlobals);
	GC_disable();
	ml_sequence_init(MLGlobals);
	ml_object_init(MLGlobals);
	//ml_time_init(MLGlobals);
#ifdef ML_STRUCT
	ml_struct_init(MLGlobals);
#endif

#ifdef ML_SCHEDULER
	ml_tasks_init(MLGlobals);
#endif
#ifdef ML_AST
	ml_ast_init(MLGlobals);
#endif
	ml_stream_init(MLGlobals);
	//ml_file_init(MLGlobals);
	//ml_socket_init(MLGlobals);
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
	ml_pqueue_init(MLGlobals);
#endif
#ifdef ML_UUID
	ml_uuid_init(MLGlobals);
#endif
#ifdef ML_MINIJS
	ml_minijs_init(MLGlobals);
#endif
#ifdef ML_ENCODINGS
	ml_base16_init(MLGlobals);
	ml_base64_init(MLGlobals);
#endif
	stringmap_insert(MLGlobals, "print", MLPrint);
	stringmap_insert(MLGlobals, "raise", MLRaise);
	stringmap_insert(MLGlobals, "memory", ml_module("memory",
		"trace", MLMemTrace,
		"size", MLMemSize,
		"collect", MLMemCollect,
		"usage", MLMemUsage,
		"dump", MLMemDump,
	NULL));
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
}

int EMSCRIPTEN_KEEPALIVE ml_session() {
	static int Initialized = 0;
	if (!Initialized) {
		initialize();
		Initialized = 1;
	}
	if (NumSessions == MaxSessions) {
		MaxSessions += 8;
		ml_session_t *NewSessions = anew(ml_session_t, MaxSessions);
		memcpy(NewSessions, Sessions, NumSessions * sizeof(ml_session_t));
		Sessions = NewSessions;
	}
	int Index = NumSessions++;
	ml_session_t *Session = Sessions + Index;
	ml_context_t *Context = Session->Base.Context = ml_context(MLRootContext);
	ml_default_queue_init(Context, 250);
	ml_context_set_static(Context, ML_SESSION_INDEX, Session);
	Session->Base.run = (ml_state_fn)ml_session_run;
	Session->Compiler = ml_compiler((ml_getter_t)ml_stringmap_global_get, MLGlobals);
	Session->Parser = ml_parser(NULL, NULL);
	return Index;
}

void EMSCRIPTEN_KEEPALIVE ml_session_evaluate(int Index, const char *Text) {
	ml_session_t *Session = Sessions + Index;
	ml_parser_reset(Session->Parser);
	ml_parser_input(Session->Parser, Text, 1);
	Session->Result = NULL;
	ml_command_evaluate((ml_state_t *)Session, Session->Parser, Session->Compiler);
	ml_scheduler_t *Scheduler = ml_context_get_static(Session->Base.Context, ML_SCHEDULER_INDEX);
	while (!Session->Result) {
		GC_gcollect();
		Scheduler->run(Scheduler);
	}
}

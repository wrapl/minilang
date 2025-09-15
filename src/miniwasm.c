#include "minilang.h"
#include "ml_console.h"
#include "ml_compiler.h"
#include "ml_bytecode.h"
#include "ml_macros.h"
#include "ml_file.h"
#include "ml_socket.h"
#include "ml_object.h"
#include "ml_time.h"
#include "ml_uuid.h"
#include "stringmap.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include "ml_sequence.h"
#include "ml_stream.h"
#include "ml_logging.h"
#include "ml_json.h"
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

#define ML_SESSION_INDEX 7

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
	const char *Output = ml_stringbuffer_get_string(Buffer);
	emscripten_console_log(Output);
	_ml_output(Session - Sessions, Output);
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

#define CALLBACK_FN(EVENT_TYPE, PREFIX) \
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
	while (Scheduler->fill(Scheduler)) { \
		GC_gcollect(); \
		Scheduler->run(Scheduler); \
	} \
	return State->Value == (ml_value_t *)MLTrue; \
}

#define EVENT_FIELD_BOOLEAN(EVENT_TYPE, NAME, FIELD) \
\
ML_METHOD(#NAME, EVENT_TYPE ## EventT) { \
	event_t *Event = (event_t *)Args[0]; \
	return ml_boolean(Event->EVENT_TYPE ## Event->FIELD); \
}

#define EVENT_FIELD_INTEGER(EVENT_TYPE, NAME, FIELD) \
\
ML_METHOD(#NAME, EVENT_TYPE ## EventT) { \
	event_t *Event = (event_t *)Args[0]; \
	return ml_integer(Event->EVENT_TYPE ## Event->FIELD); \
}

#define EVENT_FIELD_REAL(EVENT_TYPE, NAME, FIELD) \
\
ML_METHOD(#NAME, EVENT_TYPE ## EventT) { \
	event_t *Event = (event_t *)Args[0]; \
	return ml_real(Event->EVENT_TYPE ## Event->FIELD); \
}

#define EVENT_FIELD_STRING(EVENT_TYPE, NAME, FIELD) \
\
ML_METHOD(#NAME, EVENT_TYPE ## EventT) { \
	event_t *Event = (event_t *)Args[0]; \
	return ml_string_copy(Event->EVENT_TYPE ## Event->FIELD, -1); \
}

CALLBACK_FN(Keyboard, key)
EVENT_FIELD_REAL(Keyboard, timestamp, timestamp)
EVENT_FIELD_STRING(Keyboard, key, key)
EVENT_FIELD_STRING(Keyboard, code, code)
EVENT_FIELD_INTEGER(Keyboard, location, location)
EVENT_FIELD_BOOLEAN(Keyboard, ctrl, ctrlKey)
EVENT_FIELD_BOOLEAN(Keyboard, shift, shiftKey)
EVENT_FIELD_BOOLEAN(Keyboard, alt, altKey)
EVENT_FIELD_BOOLEAN(Keyboard, meta, metaKey)
EVENT_FIELD_BOOLEAN(Keyboard, repeat, repeat)
EVENT_FIELD_STRING(Keyboard, locale, locale)

CALLBACK_FN(Mouse, mouse)
EVENT_FIELD_REAL(Mouse, timestamp, timestamp)
EVENT_FIELD_INTEGER(Mouse, screenX, screenX)
EVENT_FIELD_INTEGER(Mouse, screenY, screenY)
EVENT_FIELD_INTEGER(Mouse, clientX, clientX)
EVENT_FIELD_INTEGER(Mouse, clientY, clientY)
EVENT_FIELD_BOOLEAN(Mouse, ctrl, ctrlKey)
EVENT_FIELD_BOOLEAN(Mouse, shift, shiftKey)
EVENT_FIELD_BOOLEAN(Mouse, alt, altKey)
EVENT_FIELD_BOOLEAN(Mouse, meta, metaKey)
EVENT_FIELD_INTEGER(Mouse, button, button)
EVENT_FIELD_INTEGER(Mouse, buttons, buttons)
EVENT_FIELD_INTEGER(Mouse, movementX, movementX)
EVENT_FIELD_INTEGER(Mouse, movementY, movementY)
EVENT_FIELD_INTEGER(Mouse, targetX, targetX)
EVENT_FIELD_INTEGER(Mouse, targetY, targetY)
EVENT_FIELD_INTEGER(Mouse, canvasX, canvasX)
EVENT_FIELD_INTEGER(Mouse, canvasY, canvasY)

CALLBACK_FN(Wheel, wheel)
CALLBACK_FN(Ui, ui)
CALLBACK_FN(Focus, focus)

static callback_info_t **Callbacks = NULL;
static int MaxCallbacks = 0, NumCallbacks = 0;

static int callback_register(callback_info_t *Info) {
	if (NumCallbacks == MaxCallbacks) {
		MaxCallbacks += 32;
		callback_info_t **New = anew(callback_info_t *, MaxCallbacks);
		memcpy(New, Callbacks, NumCallbacks * sizeof(callback_info_t *));
		Callbacks = New;
	}
	callback_info_t **Slot = Callbacks;
	while (*Slot) Slot++;
	*Slot = Info;
	return Slot - Callbacks;
}

static void callback_unregister(int Index) {
	Callbacks[Index] = NULL;
}

typedef int (*event_type_t)(const char *, callback_info_t *, bool);

static stringmap_t EventTypes[1] = {STRINGMAP_INIT};

#ifdef GENERATE_INIT

#define EVENT_TYPE(NAME, TYPE) \
	INIT_CODE stringmap_insert(EventTypes, #NAME, event_type_ ## NAME);

#else

#define EVENT_TYPE(NAME, TYPE) \
\
static int event_type_ ## NAME(const char *Target, callback_info_t *Info, bool UseCapture) { \
	emscripten_set_ ## NAME ## _callback(Target, Info, UseCapture, TYPE ## _callback); \
	return callback_register(Info); \
}

#endif

EVENT_TYPE(keypress, key)
EVENT_TYPE(keydown, key)
EVENT_TYPE(keyup, key)
EVENT_TYPE(click, mouse)
EVENT_TYPE(mousedown, mouse)
EVENT_TYPE(mouseup, mouse)
EVENT_TYPE(dblclick, mouse)
EVENT_TYPE(mousemove, mouse)
EVENT_TYPE(mouseenter, mouse)
EVENT_TYPE(mouseleave, mouse)
EVENT_TYPE(wheel, wheel)
EVENT_TYPE(resize, ui)
EVENT_TYPE(scroll, ui)
EVENT_TYPE(blur, focus)
EVENT_TYPE(focus, focus)
EVENT_TYPE(focusin, focus)
EVENT_TYPE(focusout, focus)

ML_FUNCTIONX(MLEvent) {
	ML_CHECKX_ARG_COUNT(3);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	ML_CHECKX_ARG_TYPE(1, MLStringT);
	ML_CHECKX_ARG_TYPE(2, MLFunctionT);
	const char *Type = ml_string_value(Args[1]);
	event_type_t EventType = stringmap_search(EventTypes, Type);
	if (!EventType) ML_ERROR("ValueError", "Unknown event type: %s", Type);
	callback_info_t *Info = new(callback_info_t);
	Info->Context = Caller->Context;
	Info->Fn = Args[2];
	const char *Target = ml_string_value(Args[0]);
	bool UseCapture = (Count > 3) && (Args[3] == (ml_value_t *)MLTrue);
	ML_RETURN(ml_integer(EventType(Target, Info, UseCapture)));
}

static void timeout_callback(void *Data) {
	callback_info_t *Info = (callback_info_t *)Data;
	callback_info_t **Slot = Callbacks;
	while (*Slot != Info) ++Slot;
	*Slot = NULL;
	ml_result_state_t *State = ml_result_state(Info->Context);
	ml_call(State, Info->Fn, 0, NULL);
	ml_scheduler_t *Scheduler = ml_context_get_static(Info->Context, ML_SCHEDULER_INDEX);
	while (Scheduler->fill(Scheduler)) {
		GC_gcollect();
		Scheduler->run(Scheduler);
	}
}

ML_FUNCTIONX(MLAfter) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLNumberT);
	ML_CHECKX_ARG_TYPE(1, MLFunctionT);
	double MSecs = ml_real_value(Args[0]) * 1000;
	callback_info_t *Info = new(callback_info_t);
	Info->Context = Caller->Context;
	Info->Fn = Args[1];
	emscripten_set_timeout(timeout_callback, MSecs, Info);
	callback_register(Info);
	ML_RETURN(MLNil);
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

static int ml_library_wasm_test(const char *Path) {
	fprintf(stderr, "Looking for <%s>\n", Path);
	return 1;
}

typedef struct {
	const char *Name;
	char *Url;
	ml_state_t *Caller;
	ml_value_t **Slot;
} ml_library_async_t;

static void ml_library_mini_load_fn(void *Arg, void *Buffer, int Size) {
	ml_library_async_t *Async = (ml_library_async_t *)Arg;
	ml_state_t *Caller = Async->Caller;
	ml_parser_t *Parser = ml_parser(NULL, NULL);
	ml_parser_source(Parser, (ml_source_t){Async->Url, 1});
	ml_parser_input(Parser, (const char *)Buffer, 0);
	const mlc_expr_t *Expr = ml_accept_file(Parser);
	if (!Expr) ML_RETURN(ml_parser_value(Parser));
	ml_compiler_t *Compiler = ml_compiler((ml_getter_t)ml_stringmap_global_get, MLGlobals);
	ml_compiler_define(Compiler, "import", ml_library_importer(Async->Url));
	ml_module_compile(Caller, Async->Url, Expr, Compiler, Async->Slot);
	free(Async->Url);
	free(Async);
	ml_scheduler_t *Scheduler = ml_context_get_scheduler(Caller->Context);
	while (Scheduler->fill(Scheduler)) {
		GC_gcollect();
		Scheduler->run(Scheduler);
	}
}

static void ml_library_mini_error_fn(void *Arg) {
	ml_library_async_t *Async = (ml_library_async_t *)Arg;
	ml_state_t *Caller = Async->Caller;
	Caller->run(Caller, ml_error("ModuleError", "Module %s not found", Async->Name));
	free(Async->Url);
	free(Async);
	ml_scheduler_t *Scheduler = ml_context_get_scheduler(Caller->Context);
	while (Scheduler->fill(Scheduler)) {
		GC_gcollect();
		Scheduler->run(Scheduler);
	}
}

static void ml_library_wasm_load_fn(void *Arg, void *Buffer, int Size) {
	ml_library_async_t *Async = (ml_library_async_t *)Arg;

}

static void ml_library_wasm_error_fn(void *Arg) {
	ml_library_async_t *Async = (ml_library_async_t *)Arg;
	sprintf(Async->Url, "%s.mini", Async->Name);
	emscripten_async_wget_data(Async->Url, Async, ml_library_mini_load_fn, ml_library_mini_error_fn);
}

static void ml_library_wasm_load(ml_state_t *Caller, const char *Name, ml_value_t **Slot) {
	emscripten_console_logf("loading module <%s>", Name);
	ml_library_async_t *Async = malloc(sizeof(ml_library_async_t));
	Async->Name = Name;
	Async->Caller = Caller;
	Async->Slot = Slot;
	Async->Url = malloc(strlen(Name) + 10);
	sprintf(Async->Url, "%s.wasm", Name);
	emscripten_async_wget_data(Async->Url, Async, ml_library_wasm_load_fn, ml_library_wasm_error_fn);
}

static ml_value_t *ml_library_wasm_load0(const char *FileName, ml_value_t **Slot) {
	return ml_error("ImplementationError", "Wasm only supports asynchronous modules");
}

void initialize(const char *BaseUrl) {
	ml_context_reserve(ML_SESSION_INDEX);
	ml_init("minilang", MLGlobals);
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
#include "miniwasm_init.c"
	stringmap_insert(MLGlobals, "event", MLEvent);
	stringmap_insert(MLGlobals, "after", MLAfter);
	ml_library_path_add(GC_strdup(BaseUrl));
	ml_library_loader_add("", ml_library_wasm_test, ml_library_wasm_load, ml_library_wasm_load0);
}

int EMSCRIPTEN_KEEPALIVE ml_session(const char *BaseUrl) {
	static int Initialized = 0;
	if (!Initialized) {
		initialize(BaseUrl);
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
	while (Scheduler->fill(Scheduler)) {
		GC_gcollect();
		Scheduler->run(Scheduler);
	}
}

int EMSCRIPTEN_KEEPALIVE ml_session_define(int Index, const char *Name, const char *Json) {
	ml_session_t *Session = Sessions + Index;
	ml_value_t *Value = ml_json_decode(Json, strlen(Json));
	if (ml_is_error(Value)) {
		Session->Result = Value;
		return 1;
	}
	ml_compiler_define(Session->Compiler, GC_strdup(Name), Value);
	return 0;
}

const char *EMSCRIPTEN_KEEPALIVE ml_session_lookup(int Index, const char *Name) {
	ml_session_t *Session = Sessions + Index;
	ml_value_t *Value = ml_compiler_lookup(Session->Compiler, Name, "<session>", 1, 0);
	if (ml_is_error(Value)) {
		Session->Result = Value;
		return "";
	}
	Value = ml_deref(Value);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_value_t *Error = ml_json_encode(Buffer, Value);
	if (Error) {
		Session->Result = Error;
		return "";
	}
	return ml_stringbuffer_get_string(Buffer);
}

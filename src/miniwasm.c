#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include <emscripten.h>

typedef struct {
	ml_state_t Base;
	ml_parser_t *Parser;
	ml_compiler_t *Compiler;
} ml_session_t;

static ml_session_t *Sessions = NULL;
static int NumSessions = 0, MaxSessions = 0;

EM_JS(void, _ml_output, (int Index, const char *Text), {
	ml_output(Index, UTF8ToString(Text));
});

EM_JS(void, _ml_finish, (int Index), {
	ml_finish(Index);
});

static void ml_session_run(ml_session_t *Session, ml_value_t *Value) {
	if (Value == MLEndOfInput) return _ml_finish(Session - Sessions);
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

int EMSCRIPTEN_KEEPALIVE ml_session() {
	static int Initialized = 0;
	static stringmap_t Globals[1] = {STRINGMAP_INIT};
	if (!Initialized) {
		ml_init("minilang", Globals);
		GC_disable();
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
	Session->Base.Context = MLRootContext;
	Session->Base.run = (ml_state_fn)ml_session_run;
	Session->Compiler = ml_compiler((ml_getter_t)ml_stringmap_global_get, Globals);
	Session->Parser = ml_parser(NULL, NULL);
	return Index;
}

void EMSCRIPTEN_KEEPALIVE ml_session_evaluate(int Index, const char *Text) {
	GC_gcollect();
	ml_session_t *Session = Sessions + Index;
	ml_parser_reset(Session->Parser);
	ml_parser_input(Session->Parser, Text, 1);
	ml_command_evaluate((ml_state_t *)Session, Session->Parser, Session->Compiler);
}

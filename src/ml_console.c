#include "ml_console.h"
#include "ml_runtime.h"
#include "ml_debugger.h"
#include "minilang.h"
#include "ml_macros.h"
#include "ml_compiler.h"
#include "stringmap.h"
#ifndef __MINGW32__
#include "linenoise.h"
#endif
#include <gc.h>
#include <stdio.h>
#include <string.h>

typedef struct ml_console_t {
	ml_getter_t ParentGetter;
	void *ParentGlobals;
	const char *Prompt;
	const char *DefaultPrompt, *ContinuePrompt;
	ml_debugger_t *Debugger;
	stringmap_t Globals[1];
} ml_console_t;

static ml_value_t *ml_console_global_get(ml_console_t *Console, const char *Name) {
	ml_value_t *Value = stringmap_search(Console->Globals, Name);
	if (Value) return Value;
	Value = (Console->ParentGetter)(Console->ParentGlobals, Name);
	if (Value) return Value;
	Value = ml_uninitialized(Name);
	stringmap_insert(Console->Globals, Name, Value);
	return Value;
}

#ifdef __MINGW32__
static ssize_t ml_read_line(FILE *File, ssize_t Offset, char **Result) {
	char Buffer[129];
	if (fgets(Buffer, 129, File) == NULL) return -1;
	int Length = strlen(Buffer);
	if (Length == 128) {
		ssize_t Total = ml_read_line(File, Offset + 128, Result);
		memcpy(*Result + Offset, Buffer, 128);
		return Total;
	} else {
		*Result = GC_MALLOC_ATOMIC(Offset + Length + 1);
		strcpy(*Result + Offset, Buffer);
		return Offset + Length;
	}
}
#endif

static const char *ml_console_line_read(ml_console_t *Console) {
#ifdef __MINGW32__
	fputs(Console->Prompt, stdout);
	char *Line;
	if (!ml_read_line(stdin, 0, &Line)) return NULL;
#else
	const char *Line = linenoise(Console->Prompt);
	if (!Line) return NULL;
	linenoiseHistoryAdd(Line);
#endif
	int Length = strlen(Line);
	char *Buffer = snew(Length + 2);
	memcpy(Buffer, Line, Length);
	Buffer[Length] = '\n';
	Buffer[Length + 1] = 0;
	Console->Prompt = Console->ContinuePrompt;
	return Buffer;
}

typedef struct {
	ml_state_t Base;
	ml_console_t *Console;
	ml_compiler_t *Compiler;
} ml_console_repl_state_t;

static int ml_stringbuffer_print(FILE *File, const char *String, size_t Length) {
	fwrite(String, 1, Length, File);
	return 0;
}

static void ml_console_log(void *Data, ml_value_t *Value) {
	if (ml_is_error(Value)) {
	error:
		printf("%s: %s\n", ml_error_type(Value), ml_error_message(Value));
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Value, Level++, &Source)) {
			printf("\t%s:%d\n", Source.Name, Source.Line);
		}
	} else {
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		Value = ml_stringbuffer_append(Buffer, Value);
		if (ml_is_error(Value)) goto error;
		ml_stringbuffer_foreach(Buffer, stdout, (void *)ml_stringbuffer_print);
		puts("");
		fflush(stdout);
	}
}

static void ml_console_repl_run(ml_console_repl_state_t *State, ml_value_t *Result) {
	if (Result == MLEndOfInput) return;
	State->Console->Prompt = State->Console->DefaultPrompt;
	Result = ml_deref(Result);
	ml_console_log(NULL, Result);
	if (ml_is_error(Result)) ml_compiler_reset(State->Compiler);
	return ml_command_evaluate((ml_state_t *)State, State->Compiler, State->Console->Globals);
}

typedef struct {
	ml_console_t *Console;
	interactive_debugger_t *Debugger;
} ml_console_debugger_t;

static ml_value_t *ml_console_debugger_get(ml_console_debugger_t *ConsoleDebugger, const char *Name) {
	ml_value_t *Value = interactive_debugger_get(ConsoleDebugger->Debugger, Name);
	if (Value) return Value;
	return ml_console_global_get(ConsoleDebugger->Console, Name);
}

static void ml_console_debug_enter(ml_console_t *Console, interactive_debugger_t *Debugger, ml_source_t Source, int Index) {
	ml_console_debugger_t *ConsoleDebugger = new(ml_console_debugger_t);
	ConsoleDebugger->Console = Console;
	ConsoleDebugger->Debugger = Debugger;
	printf("Debug break [%d]: %s:%d\n", Index, Source.Name, Source.Line);
	ml_console((void *)ml_console_debugger_get, ConsoleDebugger, "\e[34m>>>\e[0m ", "\e[34m...\e[0m ");
	interactive_debugger_resume(Debugger);
}

static void ml_console_debug_exit(void *Data, interactive_debugger_t *Debugger, ml_state_t *Caller, int Index) {
	ML_RETURN(MLEndOfInput);
}

void ml_console(ml_getter_t GlobalGet, void *Globals, const char *DefaultPrompt, const char *ContinuePrompt) {
	ml_console_t Console[1] = {{
		GlobalGet, Globals, DefaultPrompt,
		DefaultPrompt, ContinuePrompt,
		NULL,
		{STRINGMAP_INIT}
	}};
	stringmap_insert(Console->Globals, "debug", interactive_debugger(
		(void *)ml_console_debug_enter,
		(void *)ml_console_debug_exit,
		ml_console_log,
		Console,
		(ml_getter_t)ml_console_global_get,
		Console
	));
	ml_compiler_t *Compiler = ml_compiler((void *)ml_console_line_read, Console, (ml_getter_t)ml_console_global_get, Console);
	ml_compiler_source(Compiler, (ml_source_t){"<console>", 1});
	ml_console_repl_state_t *State = new(ml_console_repl_state_t);
	State->Base.run = (void *)ml_console_repl_run;
	State->Base.Context = &MLRootContext;
	State->Console = Console;
	State->Compiler = Compiler;
	ml_command_evaluate((ml_state_t *)State, Compiler, Console->Globals);
}

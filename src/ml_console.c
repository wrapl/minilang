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
	Value = ml_uninitialized();
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
	mlc_scanner_t *Scanner;
} ml_console_repl_state_t;

ml_value_t MLConsoleBreak[1] = {{MLAnyT}};

static void ml_console_log(void *Data, ml_value_t *Value) {
	if (Value->Type == MLErrorT) {
		printf("Error: %s\n", ml_error_message(Value));
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Value, Level++, &Source)) {
			printf("\t%s:%d\n", Source.Name, Source.Line);
		}
	} else {
		ml_value_t *String = ml_string_of(Value);
		if (String->Type == MLStringT) {
			printf("%s\n", ml_string_value(String));
		} else {
			printf("<%s>\n", Value->Type->Name);
		}
	}
}

static void ml_console_repl_run(ml_console_repl_state_t *State, ml_value_t *Result) {
	if (!Result || Result == MLConsoleBreak) return;
	State->Console->Prompt = State->Console->DefaultPrompt;
	Result = Result->Type->deref(Result);
	ml_console_log(NULL, Result);
	return ml_command_evaluate((ml_state_t *)State, State->Scanner, State->Console->Globals);
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

static void ml_console_debug_enter(ml_console_t *Console, interactive_debugger_t *Debugger) {
	ml_console_debugger_t *ConsoleDebugger = new(ml_console_debugger_t);
	ConsoleDebugger->Console = Console;
	ConsoleDebugger->Debugger = Debugger;
	ml_console((void *)ml_console_debugger_get, ConsoleDebugger, "\e[34m>>>\e[0m ", "\e[34m...\e[0m ");
	interactive_debugger_resume(Debugger);
}

static void ml_console_debug_exit(ml_state_t *Caller, void *Data) {
	ML_RETURN(MLConsoleBreak);
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
	mlc_scanner_t *Scanner = ml_scanner("<console>", Console, (void *)ml_console_line_read, (ml_getter_t)ml_console_global_get, Console);
	ml_console_repl_state_t *State = new(ml_console_repl_state_t);
	State->Base.run = (void *)ml_console_repl_run;
	State->Base.Context = &MLRootContext;
	State->Console = Console;
	State->Scanner = Scanner;
	ml_command_evaluate((ml_state_t *)State, Scanner, Console->Globals);
}

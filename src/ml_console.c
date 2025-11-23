#include "ml_console.h"
#include "ml_runtime.h"
#include "ml_debugger.h"
#include "minilang.h"
#include "ml_macros.h"
#include "ml_compiler.h"
#include "stringmap.h"
#ifdef USE_LINENOISE
#include "linenoise.h"
#endif
#include <stdio.h>
#include <string.h>
#include <errno.h>

typedef struct {
	ml_state_t Base;
	ml_parser_t *Parser;
	ml_compiler_t *Compiler;
	const char *Prompt;
	const char *DefaultPrompt, *ContinuePrompt;
	ml_debugger_t *Debugger;
	FILE *Input, *Output;
	char *InputLine;
	size_t InputSize;
} ml_console_t;

#ifndef USE_LINENOISE
static ssize_t ml_read_line(FILE *File, ssize_t Offset, char **Result) {
	char Buffer[129];
	if (fgets(Buffer, 129, File) == NULL) return -1;
	int Length = strlen(Buffer);
	if (Length == 128) {
		ssize_t Total = ml_read_line(File, Offset + 128, Result);
		memcpy(*Result + Offset, Buffer, 128);
		return Total;
	} else {
		*Result = snew(Offset + Length + 1);
		strcpy(*Result + Offset, Buffer);
		return Offset + Length;
	}
}
#endif

static const char *ml_console_terminal_read(ml_console_t *Console) {
#ifdef ML_HOSTTHREADS
	ml_scheduler_t *Scheduler = ml_context_get_static(Console->Base.Context, ML_SCHEDULER_INDEX);
	if (Scheduler) ml_scheduler_split(Scheduler);
#endif
#ifdef USE_LINENOISE
	const char *Line = linenoise(Console->Prompt);
	Console->Prompt = Console->ContinuePrompt;
#else
	fputs(Console->Prompt, stdout);
	char *Line = NULL;
	if (!ml_read_line(stdin, 0, &Line)) return NULL;
#endif
#ifdef ML_HOSTTHREADS
	if (Scheduler) ml_scheduler_join(Scheduler);
#endif
	if (!Line) return NULL;
#ifdef USE_LINENOISE
	linenoiseHistoryAdd(Line);
#endif
	int Length = strlen(Line);
	char *Buffer = snew(Length + 2);
	memcpy(Buffer, Line, Length);
	Buffer[Length] = '\n';
	Buffer[Length + 1] = 0;
	return Buffer;
}

#ifdef Mingw

ssize_t getline(char **Line, size_t *Length, FILE *File) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (;;) {
		int Char = fgetc(File);
		if (Char == EOF) {
			if (!Buffer->Length) return -1;
			size_t NewLength = Buffer->Length;
			*Line = ml_stringbuffer_get_string(Buffer);
			return (*Length = NewLength);
		} else if (Char == '\n') {
			ml_stringbuffer_put(Buffer, Char);
			size_t NewLength = Buffer->Length;
			*Line = ml_stringbuffer_get_string(Buffer);
			return (*Length = NewLength);
		} else {
			ml_stringbuffer_put(Buffer, Char);
		}
	}
}

#endif

static const char *ml_console_file_read(ml_console_t *Console) {
#ifdef ML_HOSTTHREADS
	ml_scheduler_t *Scheduler = ml_context_get_static(Console->Base.Context, ML_SCHEDULER_INDEX);
	if (Scheduler) ml_scheduler_split(Scheduler);
#endif
	fputs(Console->Prompt, Console->Output);
	Console->Prompt = Console->ContinuePrompt;
	//fprintf(stderr, "Waiting for input\n");
	ssize_t Size = getline(&Console->InputLine, &Console->InputSize, Console->Input);
	if (Size < 0) fprintf(stderr, "Error reading: %s\n", strerror(errno));
	//fprintf(stderr, "Read input length %ld\n", Size);
#ifdef ML_HOSTTHREADS
	if (Scheduler) ml_scheduler_join(Scheduler);
#endif
	//fprintf(stderr, "Acquired scheduler\n");
	if (Size < 0) return NULL;
	return Console->InputLine;
}

static int ml_stringbuffer_print(FILE *File, const char *String, size_t Length) {
	fwrite(String, 1, Length, File);
	return 0;
}

static void ml_console_log(ml_console_t *Console, ml_value_t *Value) {
	if (ml_is_error(Value)) {
	error:
		fprintf(Console->Output, "%s: %s\n", ml_error_type(Value), ml_error_message(Value));
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Value, Level++, &Source)) {
			fprintf(Console->Output, "\t%s:%d\n", Source.Name, Source.Line);
		}
	} else {
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		Value = ml_stringbuffer_simple_append(Buffer, Value);
		if (ml_is_error(Value)) goto error;
		ml_stringbuffer_drain(Buffer, Console->Output, (void *)ml_stringbuffer_print);
		fputs("\n", Console->Output);
		fflush(Console->Output);
	}
}

static void ml_console_repl_run(ml_console_t *Console, ml_value_t *Result) {
	if (Result == MLEndOfInput) return;
	Console->Prompt = Console->DefaultPrompt;
	Result = ml_deref(Result);
	ml_console_log(Console, Result);
	if (ml_is_error(Result)) ml_parser_reset(Console->Parser);
	return ml_command_evaluate((ml_state_t *)Console, Console->Parser, Console->Compiler);
}

typedef struct {
	ml_console_t *Console;
	ml_interactive_debugger_t *Debugger;
} ml_console_debugger_t;

static ml_value_t *ml_console_debugger_get(ml_console_debugger_t *ConsoleDebugger, const char *Name, const char *Source, int Line, int Eval) {
	ml_value_t *Value = ml_interactive_debugger_get(ConsoleDebugger->Debugger, Name);
	if (Value) return Value;
	return ml_compiler_lookup(ConsoleDebugger->Console->Compiler, Name, Source, Line, Eval);
}

static void ml_console_debug_enter(ml_console_t *Console, ml_interactive_debugger_t *Debugger, ml_source_t Source, int Index) {
	ml_console_debugger_t *ConsoleDebugger = new(ml_console_debugger_t);
	ConsoleDebugger->Console = Console;
	ConsoleDebugger->Debugger = Debugger;
	fprintf(Console->Output, "Debug break [%d]: %s:%d\n", Index, Source.Name, Source.Line);
	ml_console(Console->Base.Context, (ml_getter_t)ml_console_debugger_get, ConsoleDebugger, "\e[34m>>>\e[0m ", "\e[34m...\e[0m ");
	ml_interactive_debugger_resume(Debugger, Index);
}

static void ml_console_debug_exit(void *Data, ml_interactive_debugger_t *Debugger, ml_state_t *Caller, int Index) {
	ML_RETURN(MLEndOfInput);
}

void ml_console(ml_context_t *Context, ml_getter_t GlobalGet, void *Globals, const char *DefaultPrompt, const char *ContinuePrompt) {
	ml_console_t *Console = new(ml_console_t);
	Console->Base.run = (void *)ml_console_repl_run;
	Console->Base.Context = Context;
	Console->Prompt = Console->DefaultPrompt = DefaultPrompt;
	Console->ContinuePrompt = ContinuePrompt;
	Console->Debugger = NULL;
	ml_parser_t *Parser = ml_parser((void *)ml_console_terminal_read, Console);
	ml_compiler_t *Compiler = ml_compiler2(GlobalGet, Globals, 1);
	ml_compiler_define(Compiler, "idebug", ml_interactive_debugger(
		(void *)ml_console_debug_enter,
		(void *)ml_console_debug_exit,
		(void *)ml_console_log,
		Console,
		GlobalGet,
		Globals
	));
	ml_parser_source(Parser, (ml_source_t){"<console>", 1});
	Console->Parser = Parser;
	Console->Compiler = Compiler;
	Console->Output = stdout;
	ml_command_evaluate((ml_state_t *)Console, Parser, Compiler);
}

void ml_file_console(ml_context_t *Context, ml_getter_t GlobalGet, void *Globals, const char *DefaultPrompt, const char *ContinuePrompt, FILE *Input, FILE *Output) {
	ml_console_t *Console = new(ml_console_t);
	Console->Base.run = (void *)ml_console_repl_run;
	Console->Base.Context = Context;
	Console->Prompt = Console->DefaultPrompt = DefaultPrompt;
	Console->ContinuePrompt = ContinuePrompt;
	Console->Debugger = NULL;
	ml_parser_t *Parser = ml_parser((void *)ml_console_file_read, Console);
	ml_compiler_t *Compiler = ml_compiler(GlobalGet, Globals);
	ml_compiler_define(Compiler, "idebug", ml_interactive_debugger(
		(void *)ml_console_debug_enter,
		(void *)ml_console_debug_exit,
		(void *)ml_console_log,
		Console,
		GlobalGet,
		Globals
	));
	ml_parser_source(Parser, (ml_source_t){"<console>", 1});
	Console->Parser = Parser;
	Console->Compiler = Compiler;
	Console->Input = Input;
	Console->Output = Output;
	ml_command_evaluate((ml_state_t *)Console, Parser, Compiler);
}

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
	stringmap_t Globals[1];
} ml_console_t;

static ml_value_t *ml_console_global_get(ml_console_t *Console, const char *Name) {
	return stringmap_search(Console->Globals, Name) ?: (Console->ParentGetter)(Console->ParentGlobals, Name);
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
		*Result = GC_malloc_atomic(Offset + Length + 1);
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
	return Buffer;
}

void ml_console(ml_getter_t GlobalGet, void *Globals) {
	ml_console_t Console[1] = {{
		GlobalGet, Globals, "--> ",
		{STRINGMAP_INIT}
	}};
	mlc_error_t Error[1];
	mlc_scanner_t *Scanner = ml_scanner("console", Console, (void *)ml_console_line_read, Error);
	ml_value_t *StringMethod = ml_method("string");
	if (setjmp(Error->Handler)) {
		printf("Error: %s\n", ml_error_message(Error->Message));
		const char *Source;
		int Line;
		for (int I = 0; ml_error_trace(Error->Message, I, &Source, &Line); ++I) printf("\t%s:%d\n", Source, Line);
		ml_scanner_reset(Scanner);
	}
	for (;;) {
		mlc_expr_t *Expr = ml_accept_command(Scanner, Console->Globals);
		if (Expr == (mlc_expr_t *)-1) return;
		ml_closure_t *Closure = ml_compile(Expr, (void *)ml_console_global_get, Console, Error);
		ml_value_t *Result = ml_closure_call((ml_value_t *)Closure, 0, NULL);
		if (Result->Type == MLErrorT) {
			printf("Error: %s\n", ml_error_message(Result));
			const char *Source;
			int Line;
			for (int I = 0; ml_error_trace(Result, I, &Source, &Line); ++I) printf("\t%s:%d\n", Source, Line);
		} else {
			ml_value_t *String = ml_call(StringMethod, 1, &Result);
			if (String->Type == MLStringT) {
				printf("%s\n", ml_string_value(String));
			} else {
				printf("<%s>\n", Result->Type->Name);
			}
		}
	}
}

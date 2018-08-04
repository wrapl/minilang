#include "minilang.h"
#include "ml_compiler.h"
#include "stringmap.h"
#include "linenoise.h"
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

static const char *ml_console_line_read(ml_console_t *Console) {
	const char *Line = linenoise(Console->Prompt);
	if (!Line) return NULL;
	linenoiseHistoryAdd(Line);
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
	mlc_scanner_t *Scanner = ml_scanner("console", Console, (void *)ml_console_line_read);
	mlc_function_t Function[1] = {{(void *)ml_console_global_get, Console, NULL,}};
	SHA256_CTX HashContext[1];
	sha256_init(HashContext);
	ml_value_t *StringMethod = ml_method("string");
	if (setjmp(Scanner->OnError)) {
		printf("Error: %s\n", ml_error_message(Scanner->Error));
		const char *Source;
		int Line;
		for (int I = 0; ml_error_trace(Scanner->Error, I, &Source, &Line); ++I) printf("\t%s:%d\n", Source, Line);
		Scanner->Token = MLT_NONE;
		Scanner->Next = "";
	}
	for (;;) {
		mlc_expr_t *Expr = ml_accept_command(Scanner, Console->Globals);
		if (Expr == (mlc_expr_t *)-1) return;
		mlc_compiled_t Compiled = ml_compile(Function, Expr, HashContext);
		mlc_connect(Compiled.Exits, NULL);
		ml_closure_t *Closure = new(ml_closure_t);
		ml_closure_info_t *Info = Closure->Info = new(ml_closure_info_t);
		Closure->Type = MLClosureT;
		Info->Entry = Compiled.Start;
		Info->FrameSize = Function->Size;
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

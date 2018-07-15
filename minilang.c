#include "minilang.h"
#include <gc/gc.h>
#include <stdio.h>

static const char *ml_file_read(void *Data) {
	FILE *File = (FILE *)Data;
	char *Line = NULL;
	size_t Length = 0;
	if (getline(&Line, &Length, File) < 0) return NULL;
	return Line;
}

ml_value_t *ml_load(ml_getter_t GlobalGet, void *Globals, const char *FileName) {
	FILE *File = fopen(FileName, "r");
	if (!File) return ml_error("LoadError", "error opening %s", FileName);
	mlc_scanner_t *Scanner = ml_scanner(FileName, File, ml_file_read);
	if (setjmp(Scanner->OnError)) return Scanner->Error;
	mlc_expr_t *Expr = ml_accept_block(Scanner);
	ml_accept(Scanner, MLT_EOI);
	fclose(File);
	mlc_function_t Function[1] = {{GlobalGet, Globals, NULL,}};
	SHA256_CTX HashContext[1];
	sha256_init(HashContext);
	mlc_compiled_t Compiled = ml_compile(Function, Expr, HashContext);
	mlc_connect(Compiled.Exits, NULL);
	ml_closure_t *Closure = new(ml_closure_t);
	ml_closure_info_t *Info = Closure->Info = new(ml_closure_info_t);
	Closure->Type = MLClosureT;
	Info->Entry = Compiled.Start;
	Info->FrameSize = Function->Size;
	sha256_final(HashContext, Info->Hash);
	return (ml_value_t *)Closure;
}

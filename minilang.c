#include "minilang.h"
#include "ml_compiler.h"
#include <gc/gc.h>
#include <stdio.h>

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

static const char *ml_file_read(void *Data) {
	FILE *File = (FILE *)Data;
	char *Line = NULL;
	size_t Length = 0;
#ifdef __MINGW32__
	Length = ml_read_line(File, 0, &Line);
	if (Length < 0) return NULL;
#else
	if (getline(&Line, &Length, File) < 0) return NULL;
#endif
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

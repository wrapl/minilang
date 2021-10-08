#include "ml_rapc.h"
#include "ml_module.h"
#include "ml_macros.h"

#undef ML_CATEGORY
#define ML_CATEGORY "rapc"

extern const char RapcSource[];

static const char *ml_binary_read(const char **Source) {
	const char *Next = Source[0];
	Source[0] = NULL;
	return Next;
}

void ml_rapc_init(stringmap_t *Globals) {
	ml_compiler_t *Compiler = ml_compiler((ml_getter_t)stringmap_search, Globals);
	const char *Source[1] = {RapcSource};
	ml_parser_t *Parser = ml_parser((void *)ml_binary_read, Source);
	ml_module_compile(MLMain, Parser, Compiler, (ml_value_t **)stringmap_slot(Globals, "rapc"));
}

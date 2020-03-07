#include "ml_ansi.h"
#include "ml_macros.h"
#include "ml_module.h"
#include <string.h>

void ml_ansi_init(stringmap_t *Globals) {
#include "ml_ansi_init.c"
	if (Globals) {
		stringmap_insert(Globals, "ansi", ml_module("ansi",
			"black", ml_string("\e[30m", -1),
			"red", ml_string("\e[31m", -1),
			"green", ml_string("\e[32m", -1),
			"orange", ml_string("\e[33m", -1),
			"blue", ml_string("\e[34m", -1),
			"purple", ml_string("\e[35m", -1),
			"cyan", ml_string("\e[36m", -1),
			"grey", ml_string("\e[37m", -1),
		NULL));
	}
}

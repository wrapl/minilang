#include <stdio.h>
#include <minilang.h>

static ml_value_t *print(void *Data, int Count, ml_value_t **Args) {
	ml_value_t *StringMethod = ml_method("string");
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = Args[I];
		if (Result->Type != MLStringT) {
			Result = ml_call(StringMethod, 1, &Result);
			if (Result->Type == MLErrorT) return Result;
			if (Result->Type != MLStringT) return ml_error("ResultError", "string method did not return string");
		}
		fwrite(ml_string_value(Result), 1, ml_string_length(Result), stdout);
	}
	fflush(stdout);
	return MLNil;
}

int main(int Argc, char **Argv) {
	ml_init();
	stringmap_t *Globals = stringmap_new();
	stringmap_insert(Globals, "print", ml_function(NULL, print));
	ml_value_t *Value = ml_load_file((ml_getter_t)stringmap_search, Globals, "example1.mini");
	if (Value->Type == MLErrorT) {
		ml_error_print(Value);
		exit(1);
	}
	Value = ml_call(Value, 0, NULL);
	if (Value->Type == MLErrorT) {
		ml_error_print(Value);
		exit(1);
	}
}
#include "minilang.h"
#include "stringmap.h"
#include "ml_compiler.h"
#include "ml_file.h"
#include "ml_macros.h"
#include <string.h>
#include "ml_object.h"
#include "ml_sequence.h"

static stringmap_t Globals[1] = {STRINGMAP_INIT};

typedef struct ml_preprocessor_input_t ml_preprocessor_input_t;
typedef struct ml_preprocessor_output_t ml_preprocessor_output_t;

typedef struct ml_preprocessor_t {
	ml_preprocessor_input_t *Input;
	ml_preprocessor_output_t *Output;
} ml_preprocessor_t;

struct ml_preprocessor_input_t {
	ml_preprocessor_input_t *Prev;
	ml_value_t *Reader;
	const char *Line;
};

struct ml_preprocessor_output_t {
	ml_preprocessor_output_t *Prev;
	ml_value_t *Writer;
};

static ml_value_t *ml_preprocessor_global_get(ml_preprocessor_t *Preprocessor, const char *Name, const char *Source, int Line) {
	return stringmap_search(Globals, Name) ?: ml_error("ParseError", "Undefined symbol %s at %s:%d", Name, Source, Line);
}

static const char *ml_preprocessor_line_read(ml_preprocessor_t *Preprocessor) {
	ml_preprocessor_input_t *Input = Preprocessor->Input;
	for (;;) {
		if (Input->Line) {
			const char *Line = Input->Line;
			Input->Line = 0;
			return Line;
		}
		ml_value_t *LineValue = ml_simple_call(Input->Reader, 0, 0);
		if (LineValue == MLNil) {
			Preprocessor->Input = Input = Input->Prev;
		} else if (ml_is(LineValue, MLStringT)) {
			return ml_string_value(LineValue);
		} else if (ml_is(LineValue, MLErrorT)) {
			printf("Error: %s\n", ml_error_message(LineValue));
			ml_source_t Source;
			int Level = 0;
			while (ml_error_source(LineValue, Level++, &Source)) {
				printf("\t%s:%d\n", Source.Name, Source.Line);
			}
			exit(0);
		} else {
			printf("Error: line read function did not return string\n");
			exit(0);
		}
	}
	return NULL;
}

static ml_value_t *ml_preprocessor_output(ml_preprocessor_t *Preprocessor, int Count, ml_value_t **Args) {
	return ml_simple_call(Preprocessor->Output->Writer, Count, Args);
}

static ml_value_t *ml_preprocessor_push(ml_preprocessor_t *Preprocessor, int Count, ml_value_t **Args) {
	ml_preprocessor_output_t *Output = new(ml_preprocessor_output_t);
	Output->Prev = Preprocessor->Output;
	Output->Writer = Args[0];
	Preprocessor->Output = Output;
	return MLNil;
}

static ml_value_t *ml_preprocessor_pop(ml_preprocessor_t *Preprocessor, int Count, ml_value_t **Args) {
	ml_preprocessor_output_t *Output = Preprocessor->Output;
	Preprocessor->Output = Output->Prev;
	return Output->Writer;
}

static ml_value_t *ml_preprocessor_input(ml_preprocessor_t *Preprocessor, int Count, ml_value_t **Args) {
	ml_preprocessor_input_t *Input = new(ml_preprocessor_input_t);
	Input->Prev = Preprocessor->Input;
	Input->Reader = Args[0];
	Input->Line = 0;
	Preprocessor->Input = Input;
	return MLNil;
}

static ml_value_t *ml_preprocessor_read(FILE *File, int Count, ml_value_t **Args) {
	char *Line = NULL;
	size_t Length = 0;
	if (getline(&Line, &Length, File) < 0) {
		fclose(File);
		return MLNil;
	}
	return ml_string(Line, Length);
}

static ml_value_t *ml_preprocessor_write(FILE *File, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = Args[I];
		if (!ml_is(Result, MLStringT)) {
			Result = ml_simple_call((ml_value_t *)MLStringT, 1, &Result);
			if (ml_is(Result, MLErrorT)) return Result;
			if (!ml_is(Result, MLStringT)) return ml_error("ResultError", "string method did not return string");
		}
		fwrite(ml_string_value(Result), 1, ml_string_length(Result), File);
	}
	fflush(File);
	return MLNil;
}

static ml_value_t *ml_preprocessor_include(ml_preprocessor_t *Preprocessor, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_TYPE(0, MLStringT);
	FILE *File = fopen(ml_string_value(Args[0]), "r");
	if (!File) return ml_error("FileError", "error opening %s", ml_string_value(Args[0]));
	ml_preprocessor_input_t *Input = new(ml_preprocessor_input_t);
	Input->Prev = Preprocessor->Input;
	Input->Reader = ml_cfunction(File, (void *)ml_preprocessor_read);
	Input->Line = 0;
	Preprocessor->Input = Input;
	return MLNil;
}

static void ml_result_run(ml_state_t *State, ml_value_t *Result) {
	if (ml_is(Result, MLErrorT)) {
		printf("Error: %s\n", ml_error_message(Result));
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(Result, Level++, &Source)) {
			printf("\t%s:%d\n", Source.Name, Source.Line);
		}
	}
}

static ml_state_t MLResultState[1] = {{
	MLStateT, NULL, ml_result_run, &MLRootContext
}};

void ml_preprocess(const char *InputName, ml_value_t *Reader, ml_value_t *Writer) {
	ml_preprocessor_input_t Input0[1] = {{0, Reader}};
	ml_preprocessor_output_t Output0[1] = {{0, Writer}};
	ml_preprocessor_t Preprocessor[1] = {{Input0, Output0}};
	stringmap_insert(Globals, "write", ml_cfunction(Preprocessor, (void *)ml_preprocessor_output));
	stringmap_insert(Globals, "push", ml_cfunction(Preprocessor, (void *)ml_preprocessor_push));
	stringmap_insert(Globals, "pop", ml_cfunction(Preprocessor, (void *)ml_preprocessor_pop));
	stringmap_insert(Globals, "input", ml_cfunction(Preprocessor, (void *)ml_preprocessor_input));
	stringmap_insert(Globals, "include", ml_cfunction(Preprocessor, (void *)ml_preprocessor_include));
	stringmap_insert(Globals, "open", MLFileOpen);
	ml_parser_t *Parser = ml_parser((void *)ml_preprocessor_line_read, Preprocessor);
	ml_compiler_t *Compiler = ml_compiler((ml_getter_t)ml_preprocessor_global_get, Preprocessor);
	ml_parser_source(Parser, (ml_source_t){InputName, 1});
	ml_value_t *Semicolon = ml_cstring(";");
	for (;;) {
		ml_preprocessor_input_t *Input = Preprocessor->Input;
		const char *Line = 0;
		while (!Line) {
			if (Input->Line) {
				Line = Input->Line;
				Input->Line = 0;
			} else {
				ml_value_t *LineValue = ml_simple_call(Preprocessor->Input->Reader, 0, 0);
				if (LineValue == MLNil) {
					Input = Preprocessor->Input = Preprocessor->Input->Prev;
					if (!Input) return;
				} else if (ml_is(LineValue, MLErrorT)) {
					printf("Error: %s\n", ml_error_message(LineValue));
					ml_source_t Source;
					int Level = 0;
					while (ml_error_source(LineValue, Level++, &Source)) {
						printf("\t%s:%d\n", Source.Name, Source.Line);
					}
					exit(0);
				} else if (ml_is(LineValue, MLStringT)) {
					Line = ml_string_value(LineValue);
				} else {
					printf("Error: line read function did not return string\n");
					exit(0);
				}
			}
		}
		const char *Escape = strchr(Line, '\\');
		if (Escape) {
			if (Line < Escape) ml_simple_inline(Preprocessor->Output->Writer, 1, ml_string(Line, Escape - Line));
			if (Escape[1] == ';') {
				Input->Line = Escape + 2;
				ml_simple_inline(Preprocessor->Output->Writer, 1, Semicolon);
			} else {
				Input->Line = Escape + 1;
				ml_command_evaluate(MLResultState, Parser, Compiler);
				Input->Line = ml_parser_clear(Parser);
			}
		} else {
			if (Line[0] && Line[0] != '\n') ml_simple_inline(Preprocessor->Output->Writer, 1, ml_string(Line, strlen(Line)));
		}
	}
}

int main(int Argc, const char **Argv) {
	ml_init(Globals);
	ml_file_init(Globals);
	ml_object_init(Globals);
	ml_sequence_init(Globals);
	const char *InputName = "stdin";
	FILE *Input = stdin;
	FILE *Output = stdout;
	for (int I = 1; I < Argc; ++I) {
		if (Argv[I][0] == '-') {
			switch (Argv[I][1]) {
			case 'h': {
				printf("Usage: %s { options } input", Argv[0]);
				puts("    -h              display this message");
				exit(0);
			}
			case 'o': {
				if (Argv[I][2]) {
					Output = fopen(Argv[I] + 2, "w");
				} else {
					Output = fopen(Argv[++I], "w");
				}
				break;
			}
			case 't': {
				GC_disable();
				break;
			}
			}
		} else {
			Input = fopen(InputName = Argv[I], "r");
		}
	}
	ml_preprocess(InputName, ml_cfunction(Input, (void *)ml_preprocessor_read), ml_cfunction(Output, (void *)ml_preprocessor_write));
}

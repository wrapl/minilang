#include "minilang.c"
#include "ml_file.h"

typedef struct ml_preprocessor_input_t ml_preprocessor_input_t;
typedef struct ml_preprocessor_output_t ml_preprocessor_output_t;

typedef struct ml_preprocessor_t {
	ml_preprocessor_input_t *Input;
	ml_preprocessor_output_t *Output;
	stringmap_t Globals[1];
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

static ml_value_t *ml_preprocessor_global_get(ml_preprocessor_t *Preprocessor, const char *Name) {
	return stringmap_search(Preprocessor->Globals, Name) ?: ml_error("ParseError", "Undefined symbol %s", Name);
}

static const char *ml_preprocessor_line_read(ml_preprocessor_t *Preprocessor) {
	ml_preprocessor_input_t *Input = Preprocessor->Input;
	for (;;) {
		if (Input->Line) {
			const char *Line = Input->Line;
			Input->Line = 0;
			return Line;
		}
		ml_value_t *LineValue = ml_call(Input->Reader, 0, 0);
		if (LineValue == MLNil) {
			Preprocessor->Input = Input = Input->Prev;
		} else if (LineValue->Type == MLStringT) {
			return ml_string_value(LineValue);
		} else if (LineValue->Type == MLErrorT) {
			printf("Error: %s\n", ml_error_message(LineValue));
			const char *Source;
			int Line;
			for (int I = 0; ml_error_trace(LineValue, I, &Source, &Line); ++I) printf("\t%s:%d\n", Source, Line);
			exit(0);
		} else {
			printf("Error: line read function did not return string\n");
			exit(0);
		}
	}
}

static ml_value_t *ml_preprocessor_output(ml_preprocessor_t *Preprocessor, int Count, ml_value_t **Args) {
	return ml_call(Preprocessor->Output->Writer, Count, Args);
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
	ml_value_t *StringMethod = ml_method("string");
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = Args[I];
		if (Result->Type != MLStringT) {
			Result = ml_call(StringMethod, 1, &Result);
			if (Result->Type == MLErrorT) return Result;
			if (Result->Type != MLStringT) return ml_error("ResultError", "string method did not return string");
		}
		fwrite(ml_string_value(Result), 1, ml_string_length(Result), File);
	}
	fflush(File);
	return MLNil;
}

static ml_value_t *ml_preprocessor_include(ml_preprocessor_t *Preprocessor, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_TYPE(0, MLStringT);
	FILE *File = fopen(ml_string_value(Args[0]), "r");
	ml_preprocessor_input_t *Input = new(ml_preprocessor_input_t);
	Input->Prev = Preprocessor->Input;
	Input->Reader = ml_function(File, (void *)ml_preprocessor_read);
	Input->Line = 0;
	Preprocessor->Input = Input;
	return MLNil;
}

void ml_preprocess(const char *InputName, ml_value_t *Reader, ml_value_t *Writer) {
	ml_preprocessor_input_t Input0[1] = {{0, Reader}};
	ml_preprocessor_output_t Output0[1] = {{0, Writer}};
	ml_preprocessor_t Preprocessor[1] = {{
		Input0, Output0,
		{STRINGMAP_INIT}
	}};
	stringmap_insert(Preprocessor->Globals, "write", ml_function(Preprocessor, (void *)ml_preprocessor_output));
	stringmap_insert(Preprocessor->Globals, "push", ml_function(Preprocessor, (void *)ml_preprocessor_push));
	stringmap_insert(Preprocessor->Globals, "pop", ml_function(Preprocessor, (void *)ml_preprocessor_pop));
	stringmap_insert(Preprocessor->Globals, "input", ml_function(Preprocessor, (void *)ml_preprocessor_input));
	stringmap_insert(Preprocessor->Globals, "include", ml_function(Preprocessor, (void *)ml_preprocessor_include));
	stringmap_insert(Preprocessor->Globals, "open", ml_function(0, ml_file_open));
	mlc_scanner_t *Scanner = ml_scanner(InputName, Preprocessor, (void *)ml_preprocessor_line_read);
	mlc_function_t Function[1] = {{(void *)ml_preprocessor_global_get, Preprocessor, NULL,}};
	SHA256_CTX HashContext[1];
	sha256_init(HashContext);
	ml_value_t *StringMethod = ml_method("string");
	if (setjmp(Scanner->OnError)) {
		printf("Error: %s\n", ml_error_message(Scanner->Error));
		const char *Source;
		int Line;
		for (int I = 0; ml_error_trace(Scanner->Error, I, &Source, &Line); ++I) printf("\t%s:%d\n", Source, Line);
		exit(0);
	}
	ml_value_t *Semicolon = ml_string(";", strlen(";"));
	for (;;) {
		ml_preprocessor_input_t *Input = Preprocessor->Input;
		const char *Line = 0;
		while (!Line) {
			if (Input->Line) {
				Line = Input->Line;
				Input->Line = 0;
			} else {
				ml_value_t *LineValue = ml_call(Preprocessor->Input->Reader, 0, 0);
				if (LineValue == MLNil) {
					Input = Preprocessor->Input = Preprocessor->Input->Prev;
					if (!Input) return;
				} else if (LineValue->Type == MLErrorT) {
					printf("Error: %s\n", ml_error_message(LineValue));
					const char *Source;
					int Line;
					for (int I = 0; ml_error_trace(LineValue, I, &Source, &Line); ++I) printf("\t%s:%d\n", Source, Line);
					exit(0);
				} else if (LineValue->Type == MLStringT) {
					Line = ml_string_value(LineValue);
				} else {
					printf("Error: line read function did not return string\n");
					exit(0);
				}
			}
		}
		const char *Escape = strchr(Line, '\\');
		if (Escape) {
			if (Line < Escape) ml_inline(Preprocessor->Output->Writer, 1, ml_string(Line, Escape - Line));
			if (Escape[1] == ';') {
				Input->Line = Escape + 2;
				ml_inline(Preprocessor->Output->Writer, 1, Semicolon);
			} else {
				Input->Line = Escape + 1;
				mlc_expr_t *Expr = ml_accept_command(Scanner, Preprocessor->Globals);
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
					exit(0);
				}
				Input->Line = Scanner->Next;
				Scanner->Next = "";
			}
		} else {
			if (Line[0] && Line[0] != '\n') ml_inline(Preprocessor->Output->Writer, 1, ml_string(Line, strlen(Line)));
		}
	}
}

int main(int Argc, const char **Argv) {
	ml_init();
	ml_file_init();
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
	ml_preprocess(InputName, ml_function(Input, (void *)ml_preprocessor_read), ml_function(Output, (void *)ml_preprocessor_write));
}

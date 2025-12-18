#include "minilang.h"
#include "stringmap.h"
#include "ml_compiler.h"
#include "ml_compiler2.h"
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
	ml_source_t Position;
};

struct ml_preprocessor_output_t {
	ml_preprocessor_output_t *Prev;
	ml_value_t *Writer;
};

static ml_value_t *ml_preprocessor_global_get(ml_preprocessor_t *Preprocessor, const char *Name, const char *Source, int Line, int Mode) {
	return stringmap_search(Globals, Name) ?: ml_error("ParseError", "Undefined symbol %s at %s:%d", Name, Source, Line);
}

static const char *ml_preprocessor_read(ml_preprocessor_t *Preprocessor) {
	ml_value_t *LineValue = ml_simple_call(Preprocessor->Input->Reader, 0, 0);
	if (LineValue == MLNil) {
		Preprocessor->Input = Preprocessor->Input->Prev;
		return NULL;
	} else if (ml_is(LineValue, MLErrorT)) {
		printf("Error: %s\n", ml_error_message(LineValue));
		ml_source_t Source;
		int Level = 0;
		while (ml_error_source(LineValue, Level++, &Source)) {
			printf("\t%s:%d\n", Source.Name, Source.Line);
		}
		exit(0);
	} else if (ml_is(LineValue, MLStringT)) {
		return ml_string_value(LineValue);
	} else {
		printf("Error: line read function did not return string\n");
		exit(0);
	}
}

static const char *ml_preprocessor_line_read(ml_preprocessor_t *Preprocessor) {
	ml_preprocessor_input_t *Input = Preprocessor->Input;
	for (;;) {
		const char *Line = NULL;
		while (!Line) {
			if (Input->Line) {
				Line = Input->Line;
				Input->Line = NULL;
			} else {
				Line = ml_preprocessor_read(Preprocessor);
			}
		}
		return Line;
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
	Input->Line = NULL;
	Preprocessor->Input = Input;
	return MLNil;
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

static ml_value_t *ml_preprocessor_file_read(FILE *File, int Count, ml_value_t **Args) {
	char *Line = NULL;
	size_t Length = 0;
	if (getline(&Line, &Length, File) < 0) {
		fclose(File);
		return MLNil;
	}
	//fprintf(stderr, "Read line = %s", Line);
	return ml_string_unchecked(Line, Length);
}

static ml_value_t *ml_preprocessor_write(FILE *File, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = Args[I];
		if (Result == MLNil) continue;
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
	Input->Reader = ml_cfunction(File, (void *)ml_preprocessor_file_read);
	Input->Line = 0;
	Preprocessor->Input = Input;
	return MLNil;
}

static void ml_result_run(ml_state_t *State, ml_value_t *Result) {


}

void ml_preprocess(const char *InputName, ml_value_t *Reader, ml_value_t *Writer) {
	ml_preprocessor_input_t Input0[1] = {{0, Reader}};
	ml_preprocessor_output_t Output0[1] = {{0, Writer}};
	ml_preprocessor_t Preprocessor[1] = {{Input0, Output0}};
	stringmap_insert(Globals, "write", ml_cfunction(Preprocessor, (void *)ml_preprocessor_output));
	stringmap_insert(Globals, "push", ml_cfunction(Preprocessor, (void *)ml_preprocessor_push));
	stringmap_insert(Globals, "pop", ml_cfunction(Preprocessor, (void *)ml_preprocessor_pop));
	stringmap_insert(Globals, "input", ml_cfunction(Preprocessor, (void *)ml_preprocessor_input));
	stringmap_insert(Globals, "include", ml_cfunction(Preprocessor, (void *)ml_preprocessor_include));
	stringmap_insert(Globals, "open", MLFileT);
	ml_parser_t *Parser = ml_parser((void *)ml_preprocessor_line_read, Preprocessor);
	stringmap_insert(Parser->Extras, "preprocessor_output", Writer);
	ml_compiler_t *Compiler = ml_compiler((ml_getter_t)ml_preprocessor_global_get, Preprocessor);
	Input0->Position = (ml_source_t){InputName, 1};
	ml_parser_source(Parser, Input0->Position);
	ml_value_t *BackSlash = ml_cstring("\\");
	for (;;) {
		if (setjmp(Parser->OnError)) {
			ml_value_t *Value = Parser->Value;
			fprintf(stderr, "%s: %s\n", ml_error_type(Value), ml_error_message(Value));
			ml_source_t Source;
			int Level = 0;
			while (ml_error_source(Value, Level++, &Source)) {
				fprintf(stderr, "\t%s:%d\n", Source.Name, Source.Line);
			}
			return;
		}
		ml_preprocessor_input_t *Input = Preprocessor->Input;
		const char *Line = NULL;
		while (!Line) {
			if (Input->Line) {
				Line = Input->Line;
				Input->Line = NULL;
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
		//fprintf(stderr, "At %d, line = %s", Input->Position.Line, Line);
		const char *Escape = strchr(Line, '\\');
		if (Escape) {
			if (Line < Escape) {
				for (const char *P = Line; P < Escape; ++P) if (*P == '\n') ++Input->Position.Line;
				ml_simple_inline(Preprocessor->Output->Writer, 1, ml_string_unchecked(Line, Escape - Line));
			}
			switch (Escape[1]) {
			case '\\':
				ml_simple_inline(Preprocessor->Output->Writer, 1, BackSlash);
				Input->Line = Escape + 2;
				break;
			case '\n':
				++Input->Position.Line;
				Input->Line = Escape + 2;
				break;
			case '{':
				Input->Line = Escape + 2;
				ml_parser_source(Parser, Input->Position);
				ml_result_state_t *State = ml_result_state(MLRootContext);
				ml_command_evaluate((ml_state_t *)State, Parser, Compiler);
				if (ml_is(State->Value, MLErrorT)) {
					printf("Error: %s\n", ml_error_message(State->Value));
					ml_source_t Source;
					int Level = 0;
					while (ml_error_source(State->Value, Level++, &Source)) {
						printf("\t%s:%d\n", Source.Name, Source.Line);
					}
					exit(0);
				}
				ml_accept(Parser, MLT_RIGHT_BRACE);
				Input->Position = ml_parser_position(Parser);
				Input->Line = ml_parser_clear(Parser);
				break;
			default:
				ml_parse_warn(Parser, "ParseError", "Unknown escape sequence %c in %s", Escape[1], Line);
				break;
			}
		} else {
			for (const char *P = Line; *P; ++P) if (*P == '\n') ++Input->Position.Line;
			if (Line[0] && Line[0] != '\n') ml_simple_inline(Preprocessor->Output->Writer, 1, ml_string_unchecked(Line, strlen(Line)));
		}
	}
}

static ml_value_t *parse_output2(ml_parser_t *Parser) {
	ml_source_t Position = ml_parser_position(Parser);
	const char *Line = ml_parser_clear(Parser);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ML_EXPR(BlockExpr, block, block);
	ml_accept_block_t Accept[1];
	Accept->ExprSlot = &BlockExpr->Child;
	Accept->VarsSlot = &BlockExpr->Vars;
	Accept->LetsSlot = &BlockExpr->Lets;
	Accept->DefsSlot = &BlockExpr->Defs;
	for (;;) {
		const char *Escape = strchr(Line, '\\');
		if (Escape) {
			if (Line < Escape) {
				for (const char *P = Line; P < Escape; ++P) if (*P == '\n') ++Position.Line;
				ml_stringbuffer_write(Buffer, Line, Escape - Line);
			}
			switch (Escape[1]) {
			case '\\':
				ml_stringbuffer_put(Buffer, '\\');
				Line = Escape + 2;
				break;
			case '\n':
				++Position.Line;
				Line = Escape + 2;
				break;
			case '{':
				if (Buffer->Length) {
					ML_EXPR(CallExpr, parent_value, const_call);
					CallExpr->Value = stringmap_search(Parser->Extras, "preprocessor_output");
					ML_EXPR(ValueExpr, value, value);
					ValueExpr->Value = ml_stringbuffer_value(Buffer);
					CallExpr->Child = ML_EXPR_END(ValueExpr);
					Accept->ExprSlot[0] = ML_EXPR_END(CallExpr);
					Accept->ExprSlot= &CallExpr->Next;
				}
				ml_parser_input(Parser, Escape + 2, 0);
				ml_parser_source(Parser, Position);
				if (!ml_accept_block_child(Parser, Accept)) {
					ml_parse_warn(Parser, "ParseError", "Error parsing expression");
					break;
				}
				ml_accept(Parser, MLT_RIGHT_BRACE);
				Position = ml_parser_position(Parser);
				Line = ml_parser_clear(Parser);
				break;
			case ';':
				if (Buffer->Length) {
					ML_EXPR(CallExpr, parent_value, const_call);
					CallExpr->Value = stringmap_search(Parser->Extras, "preprocessor_output");
					ML_EXPR(ValueExpr, value, value);
					ValueExpr->Value = ml_stringbuffer_value(Buffer);
					CallExpr->Child = ML_EXPR_END(ValueExpr);
					Accept->ExprSlot[0] = ML_EXPR_END(CallExpr);
					Accept->ExprSlot = &CallExpr->Next;
				}
				ml_parser_input(Parser, Escape + 2, 0);
				ml_parser_source(Parser, Position);
				return ml_expr_value(ML_EXPR_END(BlockExpr));
			default:
				ml_parse_warn(Parser, "ParseError", "Unknown escape sequence %c in %s", Escape[1], Line);
				break;
			}
		} else {
			const char *End = Line;
			while (*End++) if (*End == '\n') ++Position.Line;
			ml_stringbuffer_write(Buffer, Line, End - Line);
			Line = End;
		}
		if (!Line[0]) {
			Line = ml_parser_read(Parser);
			if (!Line) {
				ml_parse_warn(Parser, "ParseError", "Unexpected end of input");
				break;
			}
		}
	}
	mlc_block_expr_finish(BlockExpr);
	return ml_expr_value(ML_EXPR_END(BlockExpr));
}

static ml_value_t *parse_output(ml_parser_t *Parser) {
	ml_source_t Position = ml_parser_position(Parser);
	const char *Line = ml_parser_clear(Parser);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ML_EXPR(BlockExpr, block, block);
	mlc_expr_t **Slot = &BlockExpr->Child;
	for (;;) {
		const char *Escape = strchr(Line, '\\');
		if (Escape) {
			if (Line < Escape) {
				for (const char *P = Line; P < Escape; ++P) if (*P == '\n') ++Position.Line;
				ml_stringbuffer_write(Buffer, Line, Escape - Line);
			}
			switch (Escape[1]) {
			case '\\':
				ml_stringbuffer_put(Buffer, '\\');
				Line = Escape + 2;
				break;
			case '\n':
				++Position.Line;
				Line = Escape + 2;
				break;
			case '{':
				if (Buffer->Length) {
					ML_EXPR(CallExpr, parent_value, const_call);
					CallExpr->Value = stringmap_search(Parser->Extras, "preprocessor_output");
					ML_EXPR(ValueExpr, value, value);
					ValueExpr->Value = ml_stringbuffer_value(Buffer);
					CallExpr->Child = ML_EXPR_END(ValueExpr);
					Slot[0] = ML_EXPR_END(CallExpr);
					Slot = &CallExpr->Next;
				}
				ml_parser_input(Parser, Escape + 2, 0);
				ml_parser_source(Parser, Position);
				ML_EXPR(CallExpr, parent_value, const_call);
				CallExpr->Value = stringmap_search(Parser->Extras, "preprocessor_output");
				CallExpr->Child = ml_accept_expression(Parser, EXPR_DEFAULT);
				if (!CallExpr) return ml_parser_value(Parser);
				ml_accept(Parser, MLT_RIGHT_BRACE);
				Position = ml_parser_position(Parser);
				Line = ml_parser_clear(Parser);
				Slot[0] = ML_EXPR_END(CallExpr);
				Slot = &CallExpr->Next;
				break;
			case ';':
				if (Buffer->Length) {
					ML_EXPR(CallExpr, parent_value, const_call);
					CallExpr->Value = stringmap_search(Parser->Extras, "preprocessor_output");
					ML_EXPR(ValueExpr, value, value);
					ValueExpr->Value = ml_stringbuffer_value(Buffer);
					CallExpr->Child = ML_EXPR_END(ValueExpr);
					Slot[0] = ML_EXPR_END(CallExpr);
					Slot = &CallExpr->Next;
				}
				ml_parser_input(Parser, Escape + 2, 0);
				ml_parser_source(Parser, Position);
				return ml_expr_value(ML_EXPR_END(BlockExpr));
			default:
				ml_parse_warn(Parser, "ParseError", "Unknown escape sequence %c in %s", Escape[1], Line);
				break;
			}
		} else {
			const char *End = Line;
			for (; *End; ++End) if (*End == '\n') ++Position.Line;
			ml_stringbuffer_write(Buffer, Line, End - Line);
			Line = "";
		}
		if (!Line[0]) {
			Line = ml_parser_read(Parser);
			if (!Line) {
				ml_parse_warn(Parser, "ParseError", "Unexpected end of input");
				break;
			}
		}
	}
	return ml_expr_value(ML_EXPR_END(BlockExpr));
}

int main(int Argc, const char **Argv) {
	ml_init(Argv[0], Globals);
	ml_file_init(Globals);
	ml_parser_add_escape(NULL, "", parse_output);
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
	ml_preprocess(InputName, ml_cfunction(Input, (void *)ml_preprocessor_file_read), ml_cfunction(Output, (void *)ml_preprocessor_write));
}

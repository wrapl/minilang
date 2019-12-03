#include "ml_mpc.h"
#include "minilang.h"
#include "ml_macros.h"
#include <gc/gc.h>
#include "ml_file.h"
#include "mpc/mpc.h"

typedef struct ml_parser_t {
	const ml_type_t *Type;
	mpc_parser_t *Handle;
} ml_parser_t;

static ml_type_t *MLParserT;
static ml_type_t *MLStringParserT, *MLValueParserT;
static ml_value_t Skip[1] = {{MLAnyT}};

static mpc_val_t *ml_mpc_apply_value(mpc_val_t *Value) {
	return ml_string(Value, -1);
}

static ml_value_t *ml_mpc_value_parser(ml_parser_t *StringParser) {
	ml_parser_t *ValueParser = new(ml_parser_t);
	ValueParser->Type = MLValueParserT;
	ValueParser->Handle = mpc_apply(StringParser->Handle, ml_mpc_apply_value);
	return (ml_value_t *)ValueParser;
}

ML_METHOD("$", MLStringParserT) {
	return ml_mpc_value_parser((ml_parser_t *)Args[0]);
}

static mpc_val_t *ml_mpc_value_to(mpc_val_t *_, void *Value) {
	return Value;
}

ML_METHOD("=>", MLStringParserT, MLAnyT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_apply_to(((ml_parser_t *)Args[0])->Handle, ml_mpc_value_to, Args[1]);
	return (ml_value_t *)Parser;
}

ML_METHOD("^", MLStringParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_apply_to(((ml_parser_t *)Args[0])->Handle, ml_mpc_value_to, Skip);
	return (ml_value_t *)Parser;
}

static ml_value_t *ml_mpc_pass(void *Data, int Count, ml_value_t **Args) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLParserT;
	Parser->Handle = mpc_pass();
	return (ml_value_t *)Parser;
}

static ml_value_t *ml_mpc_fail(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Chars = ml_string_value(Args[0]);
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLParserT;
	Parser->Handle = mpc_fail(Chars);
	return (ml_value_t *)Parser;
}

static ml_value_t *ml_mpc_lift_val(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_lift_val(Args[0]);
	return (ml_value_t *)Parser;
}

ML_METHOD("expect", MLParserT, MLStringT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = Args[0]->Type;
	Parser->Handle = mpc_expect(((ml_parser_t *)Args[0])->Handle, ml_string_value(Args[1]));
	return (ml_value_t *)Parser;
}

static mpc_val_t *ml_mpc_apply_to(mpc_val_t *Value, void *Function) {
	ml_value_t *Args[1] = {(ml_value_t *)Value};
	return ml_call((ml_value_t *)Function, 1, Args);
}

ML_METHOD("apply", MLValueParserT, MLFunctionT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_apply_to(((ml_parser_t *)Args[0])->Handle, ml_mpc_apply_to, Args[1]);
	return (ml_value_t *)Parser;
}

static mpc_val_t *ml_mpc_apply_to_string(mpc_val_t *Value, void *Function) {
	ml_value_t *Args[1] = {ml_string(Value, -1)};
	return ml_call((ml_value_t *)Function, 1, Args);
}

ML_METHOD("apply", MLStringParserT, MLFunctionT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_apply_to(((ml_parser_t *)Args[0])->Handle, ml_mpc_apply_to_string, Args[1]);
	return (ml_value_t *)Parser;
}

static int ml_mpc_check_with(mpc_val_t **Slot, void *Function) {
	ml_value_t *Result = ml_call((ml_value_t *)Function, 1, (ml_value_t **)Slot);
	if (Result->Type == MLErrorT) return 0;
	Slot[0] = Result;
	return 1;
}

ML_METHOD("!", MLValueParserT, MLStringT, MLFunctionT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_check_with(((ml_parser_t *)Args[0])->Handle, mpcf_dtor_null, ml_mpc_check_with, Args[2], ml_string_value(Args[1]));
	return (ml_value_t *)Parser;
}

static int ml_mpc_check_with_string(mpc_val_t **Slot, void *Function) {
	Slot[0] = ml_string(Slot[0], -1);
	ml_value_t *Result = ml_call((ml_value_t *)Function, 1, (ml_value_t **)Slot);
	if (Result->Type == MLErrorT) return 0;
	Slot[0] = Result;
	return 1;
}

ML_METHOD("!", MLStringParserT, MLStringT, MLFunctionT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_check_with(((ml_parser_t *)Args[0])->Handle, mpcf_dtor_null, ml_mpc_check_with_string, Args[2], ml_string_value(Args[1]));
	return (ml_value_t *)Parser;
}

static mpc_val_t *ml_mpc_ctor_nil(void) {
	return MLNil;
}

ML_METHOD("¬", MLValueParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_not_lift(((ml_parser_t *)Args[0])->Handle, mpcf_dtor_null, ml_mpc_ctor_nil);
	return (ml_value_t *)Parser;
}

ML_METHOD("¬", MLStringParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_not_lift(((ml_parser_t *)Args[0])->Handle, mpcf_dtor_null, mpcf_ctor_str);
	return (ml_value_t *)Parser;
}

ML_METHOD("~", MLValueParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_maybe_lift(((ml_parser_t *)Args[0])->Handle, ml_mpc_ctor_nil);
	return (ml_value_t *)Parser;
}

ML_METHOD("~", MLStringParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_maybe(((ml_parser_t *)Args[0])->Handle);
	return (ml_value_t *)Parser;
}

static mpc_val_t *ml_mpc_fold_list(int Count, mpc_val_t **Values) {
	ml_value_t *List = ml_list();
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Value = (ml_value_t *)Values[I];
		if (Value != Skip) ml_list_append(List, Value);
	}
	return List;
}

ML_METHOD("*", MLValueParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_many(ml_mpc_fold_list, ((ml_parser_t *)Args[0])->Handle);
	return (ml_value_t *)Parser;
}

ML_METHOD("*", MLStringParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_many(mpcf_strfold, ((ml_parser_t *)Args[0])->Handle);
	return (ml_value_t *)Parser;
}

ML_METHOD("+", MLValueParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_many1(ml_mpc_fold_list, ((ml_parser_t *)Args[0])->Handle);
	return (ml_value_t *)Parser;
}

ML_METHOD("+", MLStringParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_many1(mpcf_strfold, ((ml_parser_t *)Args[0])->Handle);
	return (ml_value_t *)Parser;
}

ML_METHOD("*", MLIntegerT, MLValueParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_count(ml_integer_value(Args[0]), ml_mpc_fold_list, ((ml_parser_t *)Args[1])->Handle, mpcf_dtor_null);
	return (ml_value_t *)Parser;
}

ML_METHOD("*", MLIntegerT, MLStringParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_count(ml_integer_value(Args[0]), mpcf_strfold, ((ml_parser_t *)Args[1])->Handle, mpcf_dtor_null);
	return (ml_value_t *)Parser;
}

ML_METHOD("|", MLStringParserT, MLStringParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_or(2, ((ml_parser_t *)Args[0])->Handle, ((ml_parser_t *)Args[1])->Handle);
	return (ml_value_t *)Parser;
}

ML_METHOD("|", MLValueParserT, MLValueParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_or(2, ((ml_parser_t *)Args[0])->Handle, ((ml_parser_t *)Args[1])->Handle);
	return (ml_value_t *)Parser;
}

extern mpc_parser_t *mpc_orv(int n, mpc_parser_t **parsers);
extern mpc_parser_t *mpc_andv(int n, mpc_fold_t f, mpc_parser_t **parsers);

static ml_value_t *ml_mpc_seq(void *Data, int Count, ml_value_t **Args) {
	mpc_parser_t *Parsers[Count];
	for (int I = 0; I < Count; ++I) {
		ml_parser_t *Arg = (ml_parser_t *)Args[I];
		if (Arg->Type == MLValueParserT) {
			Parsers[I] = Arg->Handle;
		} else if (Arg->Type = MLStringParserT) {
			Parsers[I] = mpc_apply(Arg->Handle, ml_mpc_apply_value);
		} else {
			ML_CHECK_ARG_TYPE(I, MLParserT);
		}
	}
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_andv(Count, ml_mpc_fold_list, Parsers);
	return (ml_value_t *)Parser;
}

ML_METHOD(".", MLStringParserT, MLStringParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_and(2, mpcf_strfold, ((ml_parser_t *)Args[0])->Handle, ((ml_parser_t *)Args[1])->Handle, mpcf_dtor_null);
	return (ml_value_t *)Parser;
}

ML_METHOD(".", MLValueParserT, MLValueParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_and(2, ml_mpc_fold_list, ((ml_parser_t *)Args[0])->Handle, ((ml_parser_t *)Args[1])->Handle, mpcf_dtor_null);
	return (ml_value_t *)Parser;
}

ML_METHOD("predictive", MLParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = Args[0]->Type;
	Parser->Handle = mpc_predictive(((ml_parser_t *)Args[0])->Handle);
	return (ml_value_t *)Parser;
}

ML_METHOD("define", MLParserT, MLParserT) {
	mpc_define(((ml_parser_t *)Args[0])->Handle, ((ml_parser_t *)Args[1])->Handle);
	Args[0]->Type = Args[1]->Type;
	return Args[0];
}

ML_METHOD("copy", MLParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = Args[0]->Type;
	Parser->Handle = mpc_copy(((ml_parser_t *)Args[0])->Handle);
	return (ml_value_t *)Parser;
}

ML_METHOD("%", MLValueParserT, MLStringT) {
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	const char *String = ml_string_value(Args[1]);
	mpc_result_t Result[1];
	if (mpc_parse("", String, Parser->Handle, Result)) {
		return (ml_value_t *)Result->output;
	} else {
		return ml_error("ParseError", "Error parsing string");
	}
}

ML_METHOD("%", MLValueParserT, MLFileT) {
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	FILE *File = ml_file_handle(Args[2]);
	mpc_result_t Result[1];
	if (mpc_parse_file("", File, Parser->Handle, Result)) {
		return (ml_value_t *)Result->output;
	} else {
		return ml_error("ParseError", "Error parsing file");
	}
}

ML_METHOD("%", MLStringParserT, MLStringT) {
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	const char *String = ml_string_value(Args[1]);
	mpc_result_t Result[1];
	if (mpc_parse("", String, Parser->Handle, Result)) {
		return ml_string(Result->output, -1);
	} else {
		return ml_error("ParseError", "Error parsing string");
	}
}

ML_METHOD("%", MLStringParserT, MLFileT) {
	ml_parser_t *Parser = (ml_parser_t *)Args[0];
	FILE *File = ml_file_handle(Args[2]);
	mpc_result_t Result[1];
	if (mpc_parse_file("", File, Parser->Handle, Result)) {
		return ml_string(Result->output, -1);
	} else {
		return ml_error("ParseError", "Error parsing file");
	}
}

static ml_value_t *ml_mpc_new(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLParserT;
	Parser->Handle = mpc_new(ml_string_value(Args[0]));
	return (ml_value_t *)Parser;
}

static ml_value_t *ml_mpc_re(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_re(ml_string_value(Args[0]));
	return (ml_value_t *)Parser;
}

static ml_value_t *ml_mpc_any(void *Data, int Count, ml_value_t **Args) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_any();
	return (ml_value_t *)Parser;
}

static ml_value_t *ml_mpc_char(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Chars = ml_string_value(Args[0]);
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_char(Chars[0]);
	return (ml_value_t *)Parser;
}

static ml_value_t *ml_mpc_range(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	if (ml_string_length(Args[0]) < 2) {
		return ml_error("ValueError", "MPC Range requires 2 characters");
	}
	const char *Chars = ml_string_value(Args[0]);
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_range(Chars[0], Chars[1]);
	return (ml_value_t *)Parser;
}

static ml_value_t *ml_mpc_oneof(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Chars = ml_string_value(Args[0]);
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_oneof(Chars);
	return (ml_value_t *)Parser;
}

static ml_value_t *ml_mpc_noneof(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Chars = ml_string_value(Args[0]);
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_noneof(Chars);
	return (ml_value_t *)Parser;
}

static ml_value_t *ml_mpc_string(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Chars = ml_string_value(Args[0]);
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_string(Chars);
	return (ml_value_t *)Parser;
}

static ml_value_t *ml_mpc_string_parser(mpc_parser_t *Handle) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = Handle;
	return (ml_value_t *)Parser;
}

static ml_value_t *ml_parser_call(ml_state_t *State, ml_parser_t *Parser, int Count, ml_value_t **Args) {

}

void ml_mpc_init(stringmap_t *Globals) {
	MLParserT = ml_type(MLAnyT, "parser");
	MLParserT->call = (void *)ml_parser_call;
	MLStringParserT = ml_type(MLParserT, "string-parser");
	MLStringParserT->call = (void *)ml_parser_call;
	MLValueParserT = ml_type(MLParserT, "value-parser");
	MLValueParserT->call = (void *)ml_parser_call;
#include "ml_mpc_init.c"
	if (Globals) {
		ml_value_t *Module = ml_map();
		stringmap_insert(Globals, "mpc", Module);

		ml_map_insert(Module, ml_string("seq", -1), ml_function(NULL, ml_mpc_seq));
		ml_map_insert(Module, ml_string("new", -1), ml_function(NULL, ml_mpc_new));
		ml_map_insert(Module, ml_string("re", -1), ml_function(NULL, ml_mpc_re));
		ml_map_insert(Module, ml_string("any", -1), ml_function(NULL, ml_mpc_any));
		ml_map_insert(Module, ml_string("char", -1), ml_function(NULL, ml_mpc_char));
		ml_map_insert(Module, ml_string("range", -1), ml_function(NULL, ml_mpc_range));
		ml_map_insert(Module, ml_string("oneof", -1), ml_function(NULL, ml_mpc_oneof));
		ml_map_insert(Module, ml_string("noneof", -1), ml_function(NULL, ml_mpc_noneof));
		ml_map_insert(Module, ml_string("string", -1), ml_function(NULL, ml_mpc_string));

		ml_map_insert(Module, ml_string("EOI", -1), ml_mpc_string_parser(mpc_eoi()));
		ml_map_insert(Module, ml_string("SOI", -1), ml_mpc_string_parser(mpc_soi()));
		ml_map_insert(Module, ml_string("Boundary", -1), ml_mpc_string_parser(mpc_boundary()));
		ml_map_insert(Module, ml_string("BoundaryNewLine", -1), ml_mpc_string_parser(mpc_boundary_newline()));
		ml_map_insert(Module, ml_string("WhiteSpace", -1), ml_mpc_string_parser(mpc_whitespace()));
		ml_map_insert(Module, ml_string("WhiteSpaces", -1), ml_mpc_string_parser(mpc_whitespaces()));
		ml_map_insert(Module, ml_string("Blank", -1), ml_mpc_string_parser(mpc_blank()));
		ml_map_insert(Module, ml_string("NewLine", -1), ml_mpc_string_parser(mpc_newline()));
		ml_map_insert(Module, ml_string("Tab", -1), ml_mpc_string_parser(mpc_tab()));
		ml_map_insert(Module, ml_string("Escape", -1), ml_mpc_string_parser(mpc_escape()));
		ml_map_insert(Module, ml_string("Digit", -1), ml_mpc_string_parser(mpc_digit()));
		ml_map_insert(Module, ml_string("HexDigit", -1), ml_mpc_string_parser(mpc_hexdigit()));
		ml_map_insert(Module, ml_string("OctDigit", -1), ml_mpc_string_parser(mpc_octdigit()));
		ml_map_insert(Module, ml_string("Digits", -1), ml_mpc_string_parser(mpc_digits()));
		ml_map_insert(Module, ml_string("HexDigits", -1), ml_mpc_string_parser(mpc_hexdigits()));
		ml_map_insert(Module, ml_string("OctDigits", -1), ml_mpc_string_parser(mpc_octdigits()));
		ml_map_insert(Module, ml_string("Lower", -1), ml_mpc_string_parser(mpc_lower()));
		ml_map_insert(Module, ml_string("Upper", -1), ml_mpc_string_parser(mpc_upper()));
		ml_map_insert(Module, ml_string("Alpha", -1), ml_mpc_string_parser(mpc_alpha()));
		ml_map_insert(Module, ml_string("Underscore", -1), ml_mpc_string_parser(mpc_underscore()));
		ml_map_insert(Module, ml_string("AlphaNum", -1), ml_mpc_string_parser(mpc_alphanum()));
		ml_map_insert(Module, ml_string("Int", -1), ml_mpc_string_parser(mpc_int()));
		ml_map_insert(Module, ml_string("Hex", -1), ml_mpc_string_parser(mpc_hex()));
		ml_map_insert(Module, ml_string("Oct", -1), ml_mpc_string_parser(mpc_oct()));
		ml_map_insert(Module, ml_string("Number", -1), ml_mpc_string_parser(mpc_number()));
		ml_map_insert(Module, ml_string("Real", -1), ml_mpc_string_parser(mpc_real()));
		ml_map_insert(Module, ml_string("Float", -1), ml_mpc_string_parser(mpc_float()));
		ml_map_insert(Module, ml_string("CharLit", -1), ml_mpc_string_parser(mpc_char_lit()));
		ml_map_insert(Module, ml_string("StringLit", -1), ml_mpc_string_parser(mpc_string_lit()));
		ml_map_insert(Module, ml_string("RegexLit", -1), ml_mpc_string_parser(mpc_regex_lit()));
		ml_map_insert(Module, ml_string("Ident", -1), ml_mpc_string_parser(mpc_ident()));
	}
}

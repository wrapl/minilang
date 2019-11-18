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

static ml_value_t *ml_mpc_and(void *Data, int Count, ml_value_t **Args) {
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
		stringmap_insert(Globals, "mpc_and", ml_function(NULL, ml_mpc_and));
		stringmap_insert(Globals, "mpc_new", ml_function(NULL, ml_mpc_new));
		stringmap_insert(Globals, "mpc_re", ml_function(NULL, ml_mpc_re));
		stringmap_insert(Globals, "mpc_any", ml_function(NULL, ml_mpc_any));
		stringmap_insert(Globals, "mpc_char", ml_function(NULL, ml_mpc_char));
		stringmap_insert(Globals, "mpc_range", ml_function(NULL, ml_mpc_range));
		stringmap_insert(Globals, "mpc_oneof", ml_function(NULL, ml_mpc_oneof));
		stringmap_insert(Globals, "mpc_noneof", ml_function(NULL, ml_mpc_noneof));
		stringmap_insert(Globals, "mpc_string", ml_function(NULL, ml_mpc_string));

		stringmap_insert(Globals, "MPCEOI", ml_mpc_string_parser(mpc_eoi()));
		stringmap_insert(Globals, "MPCSOI", ml_mpc_string_parser(mpc_soi()));
		stringmap_insert(Globals, "MPCBoundary", ml_mpc_string_parser(mpc_boundary()));
		stringmap_insert(Globals, "MPCBoundaryNewLine", ml_mpc_string_parser(mpc_boundary_newline()));
		stringmap_insert(Globals, "MPCWhiteSpace", ml_mpc_string_parser(mpc_whitespace()));
		stringmap_insert(Globals, "MPCWhiteSpaces", ml_mpc_string_parser(mpc_whitespaces()));
		stringmap_insert(Globals, "MPCBlank", ml_mpc_string_parser(mpc_blank()));
		stringmap_insert(Globals, "MPCNewLine", ml_mpc_string_parser(mpc_newline()));
		stringmap_insert(Globals, "MPCTab", ml_mpc_string_parser(mpc_tab()));
		stringmap_insert(Globals, "MPCEscape", ml_mpc_string_parser(mpc_escape()));
		stringmap_insert(Globals, "MPCDigit", ml_mpc_string_parser(mpc_digit()));
		stringmap_insert(Globals, "MPCHexDigit", ml_mpc_string_parser(mpc_hexdigit()));
		stringmap_insert(Globals, "MPCOctDigit", ml_mpc_string_parser(mpc_octdigit()));
		stringmap_insert(Globals, "MPCDigits", ml_mpc_string_parser(mpc_digits()));
		stringmap_insert(Globals, "MPCHexDigits", ml_mpc_string_parser(mpc_hexdigits()));
		stringmap_insert(Globals, "MPCOctDigits", ml_mpc_string_parser(mpc_octdigits()));
		stringmap_insert(Globals, "MPCLower", ml_mpc_string_parser(mpc_lower()));
		stringmap_insert(Globals, "MPCUpper", ml_mpc_string_parser(mpc_upper()));
		stringmap_insert(Globals, "MPCAlpha", ml_mpc_string_parser(mpc_alpha()));
		stringmap_insert(Globals, "MPCUnderscore", ml_mpc_string_parser(mpc_underscore()));
		stringmap_insert(Globals, "MPCAlphaNum", ml_mpc_string_parser(mpc_alphanum()));
		stringmap_insert(Globals, "MPCInt", ml_mpc_string_parser(mpc_int()));
		stringmap_insert(Globals, "MPCHex", ml_mpc_string_parser(mpc_hex()));
		stringmap_insert(Globals, "MPCOct", ml_mpc_string_parser(mpc_oct()));
		stringmap_insert(Globals, "MPCNumber", ml_mpc_string_parser(mpc_number()));
		stringmap_insert(Globals, "MPCReal", ml_mpc_string_parser(mpc_real()));
		stringmap_insert(Globals, "MPCFloat", ml_mpc_string_parser(mpc_float()));
		stringmap_insert(Globals, "MPCCharLit", ml_mpc_string_parser(mpc_char_lit()));
		stringmap_insert(Globals, "MPCString_Lit", ml_mpc_string_parser(mpc_string_lit()));
		stringmap_insert(Globals, "MPCRegexLit", ml_mpc_string_parser(mpc_regex_lit()));
		stringmap_insert(Globals, "MPCIdent", ml_mpc_string_parser(mpc_ident()));
	}
}
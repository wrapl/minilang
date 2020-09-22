#include "../minilang.h"
#include "../ml_macros.h"
#include <gc/gc.h>
#include "../ml_file.h"
#include "mpc/mpc.h"

typedef struct ml_parser_t {
	const ml_type_t *Type;
	mpc_parser_t *Handle;
} ml_parser_t;

ML_TYPE(MLParserT, (), "parser");
ML_TYPE(MLStringParserT, (MLParserT), "string-parser");
ML_TYPE(MLValueParserT, (MLParserT), "value-parser");

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

static ml_value_t *ml_mpc_string_parser(mpc_parser_t *Handle) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = Handle;
	return (ml_value_t *)Parser;
}

static mpc_val_t *ml_mpc_to_string(ml_value_t *Value) {
	if (ml_is(Value, MLStringT)) return (void *)ml_string_value(Value);
	return "";
}

static mpc_parser_t *ml_mpc_to_string_parser(mpc_parser_t *ValueParser) {
	return mpc_apply(ValueParser, (void *)ml_mpc_to_string);
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
	Parser->Type = ((ml_parser_t *)Args[0])->Type;
	Parser->Handle = mpc_expect(((ml_parser_t *)Args[0])->Handle, ml_string_value(Args[1]));
	return (ml_value_t *)Parser;
}

static mpc_val_t *ml_mpc_apply_to(mpc_val_t *Value, void *Function) {
	ml_value_t *Args[1] = {(ml_value_t *)Value};
	return ml_simple_call((ml_value_t *)Function, 1, Args);
}

ML_METHOD("->", MLValueParserT, MLFunctionT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_apply_to(((ml_parser_t *)Args[0])->Handle, ml_mpc_apply_to, Args[1]);
	return (ml_value_t *)Parser;
}

static mpc_val_t *ml_mpc_apply_to_string(mpc_val_t *Value, void *Function) {
	ml_value_t *Args[1] = {ml_string(Value, -1)};
	return ml_simple_call((ml_value_t *)Function, 1, Args);
}

ML_METHOD("->", MLStringParserT, MLFunctionT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_apply_to(((ml_parser_t *)Args[0])->Handle, ml_mpc_apply_to_string, Args[1]);
	return (ml_value_t *)Parser;
}

static int ml_mpc_check_with(mpc_val_t **Slot, void *Function) {
	ml_value_t *Result = ml_simple_call((ml_value_t *)Function, 1, (ml_value_t **)Slot);
	if (Result == MLNil) return 0;
	return 1;
}

ML_METHOD("?", MLValueParserT, MLFunctionT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_check_with(((ml_parser_t *)Args[0])->Handle, mpcf_dtor_null, ml_mpc_check_with, Args[1], "failed");
	return (ml_value_t *)Parser;
}

static int ml_mpc_check_with_string(mpc_val_t **Slot, void *Function) {
	Slot[0] = ml_string(Slot[0], -1);
	ml_value_t *Result = ml_simple_call((ml_value_t *)Function, 1, (ml_value_t **)Slot);
	if (Result == MLNil) return 0;
	return 1;
}

ML_METHOD("?", MLStringParserT, MLFunctionT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_check_with(((ml_parser_t *)Args[0])->Handle, mpcf_dtor_null, ml_mpc_check_with_string, Args[1], "failed");
	return (ml_value_t *)Parser;
}

static int ml_mpc_filter_with(mpc_val_t **Slot, void *Function) {
	ml_value_t *Result = ml_simple_call((ml_value_t *)Function, 1, (ml_value_t **)Slot);
	if (Result == MLNil) return 0;
	Slot[0] = Result;
	return 1;
}

ML_METHOD("->?", MLValueParserT, MLFunctionT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_check_with(((ml_parser_t *)Args[0])->Handle, mpcf_dtor_null, ml_mpc_filter_with, Args[1], "failed");
	return (ml_value_t *)Parser;
}

static int ml_mpc_filter_with_string(mpc_val_t **Slot, void *Function) {
	Slot[0] = ml_string(Slot[0], -1);
	ml_value_t *Result = ml_simple_call((ml_value_t *)Function, 1, (ml_value_t **)Slot);
	if (Result == MLNil) return 0;
	Slot[0] = Result;
	return 1;
}

ML_METHOD("->?", MLStringParserT, MLFunctionT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_check_with(((ml_parser_t *)Args[0])->Handle, mpcf_dtor_null, ml_mpc_filter_with_string, Args[1], "failed");
	return (ml_value_t *)Parser;
}

static mpc_val_t *ml_mpc_ctor_nil(void) {
	return MLNil;
}

ML_METHOD("!", MLValueParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_not_lift(((ml_parser_t *)Args[0])->Handle, mpcf_dtor_null, ml_mpc_ctor_nil);
	return (ml_value_t *)Parser;
}

ML_METHOD("!", MLStringParserT) {
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
		} else if (Arg->Type == MLStringParserT) {
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

ML_METHOD(".", MLStringParserT, MLStringT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_and(2, mpcf_strfold, ((ml_parser_t *)Args[0])->Handle, mpc_string(ml_string_value(Args[1])), mpcf_dtor_null);
	return (ml_value_t *)Parser;
}

ML_METHOD(".", MLStringT, MLStringParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLStringParserT;
	Parser->Handle = mpc_and(2, mpcf_strfold, mpc_string(ml_string_value(Args[0])), ((ml_parser_t *)Args[1])->Handle, mpcf_dtor_null);
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
	Parser->Type = ((ml_parser_t *)Args[0])->Type;
	Parser->Handle = mpc_predictive(((ml_parser_t *)Args[0])->Handle);
	return (ml_value_t *)Parser;
}

ML_METHOD("define", MLParserT, MLParserT) {
	mpc_define(((ml_parser_t *)Args[0])->Handle, ((ml_parser_t *)Args[1])->Handle);
	((ml_parser_t *)Args[0])->Type = ((ml_parser_t *)Args[1])->Type;
	return Args[0];
}

ML_METHOD("copy", MLParserT) {
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = ((ml_parser_t *)Args[0])->Type;
	Parser->Handle = mpc_copy(((ml_parser_t *)Args[0])->Handle);
	return (ml_value_t *)Parser;
}

ML_METHOD("%", MLStringT, MLValueParserT) {
	const char *String = ml_string_value(Args[0]);
	ml_parser_t *Parser = (ml_parser_t *)Args[1];
	mpc_result_t Result[1];
	if (mpc_parse("", String, Parser->Handle, Result)) {
		return (ml_value_t *)Result->output;
	} else {
		return ml_error("ParseError", "Error parsing string");
	}
}

ML_METHOD("%", MLFileT, MLValueParserT) {
	FILE *File = ml_file_handle(Args[0]);
	ml_parser_t *Parser = (ml_parser_t *)Args[1];
	mpc_result_t Result[1];
	if (mpc_parse_file("", File, Parser->Handle, Result)) {
		return (ml_value_t *)Result->output;
	} else {
		return ml_error("ParseError", "Error parsing file");
	}
}

ML_METHOD("%", MLStringT, MLStringParserT) {
	const char *String = ml_string_value(Args[0]);
	ml_parser_t *Parser = (ml_parser_t *)Args[1];
	mpc_result_t Result[1];
	if (mpc_parse("", String, Parser->Handle, Result)) {
		return ml_string(Result->output, -1);
	} else {
		return ml_error("ParseError", "Error parsing string");
	}
}

ML_METHOD("%", MLFileT, MLStringParserT) {
	FILE *File = ml_file_handle(Args[0]);
	ml_parser_t *Parser = (ml_parser_t *)Args[1];
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
	mpc_parser_t *Parsers[Count];
	for (int I = 0; I < Count; ++I) {
		ml_parser_t *Arg = (ml_parser_t *)Args[I];
		if (Arg->Type == MLValueParserT) {
			Parsers[I] = Arg->Handle;
		} else if (Arg->Type == MLStringParserT) {
			Parsers[I] = mpc_apply(Arg->Handle, ml_mpc_apply_value);
		} else {
			ML_CHECK_ARG_TYPE(I, MLParserT);
		}
	}
	ml_parser_t *Parser = new(ml_parser_t);
	Parser->Type = MLValueParserT;
	Parser->Handle = mpc_orv(Count, Parsers);
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

void ml_library_entry(ml_value_t *Module, ml_getter_t GlobalGet, void *Globals) {
#include "ml_mpc_init.c"
	ml_module_export(Module, "seq", ml_cfunction(NULL, ml_mpc_seq));
	ml_module_export(Module, "new", ml_cfunction(NULL, ml_mpc_new));
	ml_module_export(Module, "re", ml_cfunction(NULL, ml_mpc_re));
	ml_module_export(Module, "any", ml_cfunction(NULL, ml_mpc_any));
	ml_module_export(Module, "char", ml_cfunction(NULL, ml_mpc_char));
	ml_module_export(Module, "range", ml_cfunction(NULL, ml_mpc_range));
	ml_module_export(Module, "oneof", ml_cfunction(NULL, ml_mpc_oneof));
	ml_module_export(Module, "noneof", ml_cfunction(NULL, ml_mpc_noneof));
	ml_module_export(Module, "string", ml_cfunction(NULL, ml_mpc_string));
	ml_module_export(Module, "Any", ml_mpc_string_parser(mpc_any()));
	ml_module_export(Module, "EOI", ml_mpc_string_parser(mpc_eoi()));
	ml_module_export(Module, "SOI", ml_mpc_string_parser(mpc_soi()));
	ml_module_export(Module, "Boundary", ml_mpc_string_parser(mpc_boundary()));
	ml_module_export(Module, "BoundaryNewLine", ml_mpc_string_parser(mpc_boundary_newline()));
	ml_module_export(Module, "WhiteSpace", ml_mpc_string_parser(mpc_whitespace()));
	ml_module_export(Module, "WhiteSpaces", ml_mpc_string_parser(mpc_whitespaces()));
	ml_module_export(Module, "Blank", ml_mpc_string_parser(mpc_blank()));
	ml_module_export(Module, "NewLine", ml_mpc_string_parser(mpc_newline()));
	ml_module_export(Module, "Tab", ml_mpc_string_parser(mpc_tab()));
	ml_module_export(Module, "Escape", ml_mpc_string_parser(mpc_escape()));
	ml_module_export(Module, "Digit", ml_mpc_string_parser(mpc_digit()));
	ml_module_export(Module, "HexDigit", ml_mpc_string_parser(mpc_hexdigit()));
	ml_module_export(Module, "OctDigit", ml_mpc_string_parser(mpc_octdigit()));
	ml_module_export(Module, "Digits", ml_mpc_string_parser(mpc_digits()));
	ml_module_export(Module, "HexDigits", ml_mpc_string_parser(mpc_hexdigits()));
	ml_module_export(Module, "OctDigits", ml_mpc_string_parser(mpc_octdigits()));
	ml_module_export(Module, "Lower", ml_mpc_string_parser(mpc_lower()));
	ml_module_export(Module, "Upper", ml_mpc_string_parser(mpc_upper()));
	ml_module_export(Module, "Alpha", ml_mpc_string_parser(mpc_alpha()));
	ml_module_export(Module, "Underscore", ml_mpc_string_parser(mpc_underscore()));
	ml_module_export(Module, "AlphaNum", ml_mpc_string_parser(mpc_alphanum()));
	ml_module_export(Module, "Int", ml_mpc_string_parser(mpc_int()));
	ml_module_export(Module, "Hex", ml_mpc_string_parser(mpc_hex()));
	ml_module_export(Module, "Oct", ml_mpc_string_parser(mpc_oct()));
	ml_module_export(Module, "Number", ml_mpc_string_parser(mpc_number()));
	ml_module_export(Module, "Real", ml_mpc_string_parser(mpc_real()));
	ml_module_export(Module, "Float", ml_mpc_string_parser(mpc_float()));
	ml_module_export(Module, "CharLit", ml_mpc_string_parser(mpc_char_lit()));
	ml_module_export(Module, "StringLit", ml_mpc_string_parser(mpc_string_lit()));
	ml_module_export(Module, "RegexLit", ml_mpc_string_parser(mpc_regex_lit()));
	ml_module_export(Module, "Ident", ml_mpc_string_parser(mpc_ident()));
}

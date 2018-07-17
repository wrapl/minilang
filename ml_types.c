#include "minilang.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <gc.h>
#include <gc/gc_typed.h>
#include <regex.h>
#include <pthread.h>
#include "stringmap.h"

long ml_default_hash(ml_value_t *Value) {
	long Hash = 5381;
	for (const char *P = Value->Type->Name; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

ml_value_t *ml_default_call(ml_value_t *Value, int Count, ml_value_t **Args) {
	return ml_error("TypeError", "value is not callable");
}

ml_value_t *ml_default_deref(ml_value_t *Ref) {
	return Ref;
}

ml_value_t *ml_default_assign(ml_value_t *Ref, ml_value_t *Value) {
	return ml_error("TypeError", "value is not assignable");
}

ml_value_t *ml_default_next(ml_value_t *Iter) {
	return ml_error("TypeError", "%s is not iterable", Iter->Type->Name);
}

ml_value_t *ml_default_key(ml_value_t *Iter) {
	return MLNil;
}

ml_value_t *CompareMethod;
ml_value_t *AppendMethod;

ml_type_t MLAnyT[1] = {{
	NULL, "any",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

static ml_value_t *ml_nil_to_string(void *Data, int Count, ml_value_t **Args) {
	return ml_string("nil", 3);
}

ml_type_t MLNilT[1] = {{
	MLAnyT, "nil",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

ml_value_t MLNil[1] = {{MLNilT}};

static ml_value_t *ml_some_to_string(void *Data, int Count, ml_value_t **Args) {
	return ml_string("some", 3);
}

ml_type_t MLSomeT[1] = {{
	MLAnyT, "nil",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

ml_value_t MLSome[1] = {{MLSomeT}};

long ml_hash(ml_value_t *Value) {
	Value = Value->Type->deref(Value);
	return Value->Type->hash(Value);
}

int ml_is(ml_value_t *Value, ml_type_t *Expected) {
	const ml_type_t *Type = Value->Type;
	while (Type) {
		if (Type == Expected) return 1;
		Type = Type->Parent;
	}
	return 0;
}

ml_value_t *ml_call(ml_value_t *Value, int Count, ml_value_t **Args) {
	return Value->Type->call(Value, Count, Args);
}

ml_value_t *ml_inline(ml_value_t *Value, int Count, ...) {
	ml_value_t *Args[Count];
	va_list List;
	va_start(List, Count);
	for (int I = 0; I < Count; ++I) Args[I] = va_arg(List, ml_value_t *);
	va_end(List);
	return Value->Type->call(Value, Count, Args);
}

static ml_value_t *ml_function_call(ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_function_t *Function = (ml_function_t *)Value;
	return (Function->Callback)(Function->Data, Count, Args);
}

ml_type_t MLFunctionT[1] = {{
	MLAnyT, "function",
	ml_default_hash,
	ml_function_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

ml_value_t *ml_function(void *Data, ml_callback_t Callback) {
	ml_function_t *Function = fnew(ml_function_t);
	Function->Type = MLFunctionT;
	Function->Data = Data;
	Function->Callback = Callback;
	GC_end_stubborn_change(Function);
	return (ml_value_t *)Function;
}

ml_type_t MLNumberT[1] = {{
	MLAnyT, "number",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

struct ml_integer_t {
	const ml_type_t *Type;
	long Value;
};

struct ml_real_t {
	const ml_type_t *Type;
	double Value;
};

struct ml_string_t {
	const ml_type_t *Type;
	const char *Value;
	int Length;
};

struct ml_regex_t {
	const ml_type_t *Type;
	const char *Pattern;
	regex_t Value[1];
};

static long ml_integer_hash(ml_value_t *Value) {
	ml_integer_t *Integer = (ml_integer_t *)Value;
	return Integer->Value;
}

ml_type_t MLIntegerT[1] = {{
	MLNumberT, "integer",
	ml_integer_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

ml_value_t *ml_integer(long Value) {
	ml_integer_t *Integer = fnew(ml_integer_t);
	Integer->Type = MLIntegerT;
	Integer->Value = Value;
	GC_end_stubborn_change(Integer);
	return (ml_value_t *)Integer;
}

int ml_is_integer(ml_value_t *Value) {
	return Value->Type == MLIntegerT;
}

long ml_integer_value(ml_value_t *Value) {
	return ((ml_integer_t *)Value)->Value;
}

static long ml_real_hash(ml_value_t *Value) {
	ml_real_t *Real = (ml_real_t *)Value;
	return (long)Real->Value;
}

ml_type_t MLRealT[1] = {{
	MLNumberT, "real",
	ml_real_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

ml_value_t *ml_real(double Value) {
	ml_real_t *Real = fnew(ml_real_t);
	Real->Type = MLRealT;
	Real->Value = Value;
	GC_end_stubborn_change(Real);
	return (ml_value_t *)Real;
}

int ml_is_real(ml_value_t *Value) {
	return Value->Type == MLRealT;
}

double ml_real_value(ml_value_t *Value) {
	return ((ml_real_t *)Value)->Value;
}

static long ml_string_hash(ml_value_t *Value) {
	ml_string_t *String = (ml_string_t *)Value;
	long Hash = 5381;
	for (int I = 0; I < String->Length; ++I) Hash = ((Hash << 5) + Hash) + String->Value[I];
	return Hash;
}

static ml_value_t *ml_string_index(void *Data, int Count, ml_value_t **Args) {
	ml_string_t *String = (ml_string_t *)Args[0];
	int Index = ((ml_integer_t *)Args[1])->Value;
	if (Index <= 0) Index += String->Length + 1;
	if (Index <= 0) return MLNil;
	if (Index > String->Length) return MLNil;
	char *Chars = snew(2);
	Chars[0] = String->Value[Index - 1];
	Chars[1] = 0;
	return ml_string(Chars, 1);
}

static ml_value_t *ml_string_slice(void *Data, int Count, ml_value_t **Args) {
	ml_string_t *String = (ml_string_t *)Args[0];
	int Lo = ((ml_integer_t *)Args[1])->Value;
	int Hi = ((ml_integer_t *)Args[2])->Value;
	if (Lo <= 0) Lo += String->Length + 1;
	if (Hi <= 0) Hi += String->Length + 1;
	if (Lo <= 0) return MLNil;
	if (Hi > String->Length + 1) return MLNil;
	if (Hi < Lo) return MLNil;
	int Length = Hi - Lo;
	char *Chars = snew(Length + 1);
	memcpy(Chars, String->Value + Lo - 1, Length);
	Chars[Length] = 0;
	return ml_string(Chars, Length);
}

const char *ml_string_value(ml_value_t *Value) {
	return ((ml_string_t *)Value)->Value;
}

int ml_string_length(ml_value_t *Value) {
	return ((ml_string_t *)Value)->Length;
}

static ml_value_t *ml_string_trim(void *Data, int Count, ml_value_t **Args) {
	const char *Start = ml_string_value(Args[0]);
	const char *End = Start + ml_string_length(Args[0]);
	while (Start < End && Start[0] <= ' ') ++Start;
	while (Start < End && End[-1] <= ' ') --End;
	int Length = End - Start;
	char *Chars = snew(Length + 1);
	memcpy(Chars, Start, Length);
	Chars[Length] = 0;
	return ml_string(Chars, Length);
}

static ml_value_t *ml_string_length_value(void *Data, int Count, ml_value_t **Args) {
	return ml_integer(((ml_string_t *)Args[0])->Length);
}

ml_value_t *ml_string_string_split(void *Data, int Count, ml_value_t **Args) {
	ml_value_t *Results = ml_list();
	const char *Subject = ml_string_value(Args[0]);
	const char *Pattern = ml_string_value(Args[1]);
	size_t Length = strlen(Pattern);
	for (;;) {
		const char *Next = strstr(Subject, Pattern);
		while (Next == Subject) {
			Subject += Length;
			Next = strstr(Subject, Pattern);
		}
		if (!Subject[0]) return Results;
		if (Next) {
			size_t MatchLength = Next - Subject;
			char *Match = snew(MatchLength + 1);
			memcpy(Match, Subject, MatchLength);
			Match[MatchLength] = 0;
			ml_list_append(Results, ml_string(Match, MatchLength));
			Subject = Next + Length;
		} else {
			ml_list_append(Results, ml_string(Subject, strlen(Subject)));
			break;
		}
	}
	return Results;
}

ml_value_t *ml_string_regex_split(void *Data, int Count, ml_value_t **Args) {
	ml_value_t *Results = ml_list();
	const char *Subject = ml_string_value(Args[0]);
	const char *SubjectEnd = Subject + ml_string_length(Args[0]);
	ml_regex_t *Pattern = (ml_regex_t *)Args[1];
	int Index = Pattern->Value->re_nsub ? 1 : 0;
	regmatch_t Matches[2];
	for (;;) {
		switch (regexec(Pattern->Value, Subject, Index + 1, Matches, 0)) {
		case REG_NOMATCH: {
			if (SubjectEnd > Subject) ml_list_append(Results, ml_string(Subject, SubjectEnd - Subject));
			return Results;
		}
		case REG_ESPACE: {
			size_t ErrorSize = regerror(REG_ESPACE, Pattern->Value, NULL, 0);
			char *ErrorMessage = snew(ErrorSize + 1);
			regerror(REG_ESPACE, Pattern->Value, ErrorMessage, ErrorSize);
			return ml_error("RegexError", ErrorMessage);
		}
		default: {
			regoff_t Start = Matches[Index].rm_so;
			if (Start > 0) ml_list_append(Results, ml_string(Subject, Start));
			Subject += Matches[Index].rm_eo;
		}
		}
	}
	return Results;
}

static ml_value_t *ml_string_find(void *Data, int Count, ml_value_t **Args) {
	const char *Haystack = ml_string_value(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	const char *Match = strstr(Haystack, Needle);
	if (Match) {
		return ml_integer(1 + Match - Haystack);
	} else {
		return MLNil;
	}
}

ml_value_t *ml_string_match(void *Data, int Count, ml_value_t **Args) {
	const char *Subject = ml_string_value(Args[0]);
	const char *Pattern = ml_string_value(Args[1]);
	regex_t Regex[1];
	int Error = regcomp(Regex, Pattern, REG_EXTENDED);
	if (Error) {
		size_t ErrorSize = regerror(Error, Regex, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(Error, Regex, ErrorMessage, ErrorSize);
		return ml_error("RegexError", ErrorMessage);
	}
	regmatch_t Matches[Regex->re_nsub];
	switch (regexec(Regex, Subject, Regex->re_nsub, Matches, 0)) {
	case REG_NOMATCH:
		regfree(Regex);
		return MLNil;
	case REG_ESPACE: {
		regfree(Regex);
		size_t ErrorSize = regerror(REG_ESPACE, Regex, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(Error, Regex, ErrorMessage, ErrorSize);
		return ml_error("RegexError", ErrorMessage);
	}
	default: {
		ml_value_t *Results = ml_list();
		for (int I = 0; I < Regex->re_nsub; ++I) {
			regoff_t Start = Matches[I].rm_so;
			if (Start >= 0) {
				size_t Length = Matches[I].rm_eo - Start;
				char *Chars = snew(Length + 1);
				memcpy(Chars, Subject + Start, Length);
				Chars[Length] = 0;
				ml_list_append(Results, ml_string(Chars, Length));
			} else {
				ml_list_append(Results, MLNil);
			}
		}
		regfree(Regex);
		return Results;
	}
	}
}

ml_value_t *ml_string_string_replace(void *Data, int Count, ml_value_t **Args) {
	const char *Subject = ml_string_value(Args[0]);
	const char *SubjectEnd = Subject + ml_string_length(Args[0]);
	const char *Pattern = ml_string_value(Args[1]);
	int PatternLength = ml_string_length(Args[1]);
	const char *Replace = ml_string_value(Args[2]);
	int ReplaceLength = ml_string_length(Args[2]);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Find = strstr(Subject, Pattern);
	while (Find) {
		if (Find > Subject) ml_stringbuffer_add(Buffer, Subject, Find - Subject);
		ml_stringbuffer_add(Buffer, Replace, ReplaceLength);
		Subject = Find + PatternLength;
		Find = strstr(Subject, Pattern);
	}
	if (SubjectEnd > Subject) {
		ml_stringbuffer_add(Buffer, Subject, SubjectEnd - Subject);
	}
	ml_string_t *String = fnew(ml_string_t);
	String->Type = MLStringT;
	String->Length = Buffer->Length;
	String->Value = ml_stringbuffer_get(Buffer);
	GC_end_stubborn_change(String);
	return (ml_value_t *)String;
}

ml_value_t *ml_string_regex_string_replace(void *Data, int Count, ml_value_t **Args) {
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	ml_regex_t *Pattern = (ml_regex_t *)Args[1];
	const char *Replace = ml_string_value(Args[2]);
	int ReplaceLength = ml_string_length(Args[2]);
	regmatch_t Matches[1];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (;;) {
		switch (regexec(Pattern->Value, Subject, 1, Matches, 0)) {
		case REG_NOMATCH:
			if (SubjectLength) ml_stringbuffer_add(Buffer, Subject, SubjectLength);
			ml_string_t *String = fnew(ml_string_t);
			String->Type = MLStringT;
			String->Length = Buffer->Length;
			String->Value = ml_stringbuffer_get(Buffer);
			GC_end_stubborn_change(String);
			return (ml_value_t *)String;
		case REG_ESPACE: {
			size_t ErrorSize = regerror(REG_ESPACE, Pattern->Value, NULL, 0);
			char *ErrorMessage = snew(ErrorSize + 1);
			regerror(REG_ESPACE, Pattern->Value, ErrorMessage, ErrorSize);
			return ml_error("RegexError", ErrorMessage);
		}
		default: {
			regoff_t Start = Matches[0].rm_so;
			if (Start > 0) ml_stringbuffer_add(Buffer, Subject, Start);
			ml_stringbuffer_add(Buffer, Replace, ReplaceLength);
			Subject += Matches[0].rm_eo;
			SubjectLength -= Matches[0].rm_eo;
		}
		}
	}
	return 0;
}

ml_value_t *ml_string_regex_function_replace(void *Data, int Count, ml_value_t **Args) {
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	ml_regex_t *Pattern = (ml_regex_t *)Args[1];
	ml_value_t *Replacer = Args[2];
	int NumSub = Pattern->Value->re_nsub + 1;
	regmatch_t Matches[NumSub];
	ml_value_t *SubArgs[NumSub];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (;;) {
		switch (regexec(Pattern->Value, Subject, NumSub, Matches, 0)) {
		case REG_NOMATCH:
			if (SubjectLength) ml_stringbuffer_add(Buffer, Subject, SubjectLength);
			ml_string_t *String = fnew(ml_string_t);
			String->Type = MLStringT;
			String->Length = Buffer->Length;
			String->Value = ml_stringbuffer_get(Buffer);
			GC_end_stubborn_change(String);
			return (ml_value_t *)String;
		case REG_ESPACE: {
			size_t ErrorSize = regerror(REG_ESPACE, Pattern->Value, NULL, 0);
			char *ErrorMessage = snew(ErrorSize + 1);
			regerror(REG_ESPACE, Pattern->Value, ErrorMessage, ErrorSize);
			return ml_error("RegexError", ErrorMessage);
		}
		default: {
			regoff_t Start = Matches[0].rm_so;
			if (Start > 0) ml_stringbuffer_add(Buffer, Subject, Start);
			for (int I = 0; I < NumSub; ++I) {
				SubArgs[I] = ml_string(Subject + Matches[I].rm_so, Matches[I].rm_eo - Matches[I].rm_so);
			}
			ml_value_t *Replace = ml_call(Replacer, NumSub, SubArgs);
			if (Replace->Type == MLErrorT) return Replace;
			if (Replace->Type != MLStringT) return ml_error("TypeError", "expected string, not %s", Replace->Type->Name);
			ml_stringbuffer_add(Buffer, ml_string_value(Replace), ml_string_length(Replace));
			Subject += Matches[0].rm_eo;
			SubjectLength -= Matches[0].rm_eo;
		}
		}
	}
	return 0;
}

ml_type_t MLStringT[1] = {{
	MLAnyT, "string",
	ml_string_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

ml_value_t *ml_string(const char *Value, int Length) {
	ml_string_t *String = fnew(ml_string_t);
	String->Type = MLStringT;
	String->Value = Value;
	String->Length = Length >= 0 ? Length : strlen(Value);
	GC_end_stubborn_change(String);
	return (ml_value_t *)String;
}

int ml_is_string(ml_value_t *Value) {
	return Value->Type == MLStringT;
}

ml_value_t *ml_string_new(void *Data, int Count, ml_value_t **Args) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (int I = 0; I < Count; ++I) ml_inline(AppendMethod, 2, (ml_value_t *)Buffer, Args[I]);
	ml_string_t *String = fnew(ml_string_t);
	String->Type = MLStringT;
	String->Length = Buffer->Length;
	String->Value = ml_stringbuffer_get(Buffer);
	GC_end_stubborn_change(String);
	return (ml_value_t *)String;
}

static long ml_regex_hash(ml_value_t *Value) {
	ml_regex_t *Regex = (ml_regex_t *)Value;
	long Hash = 5381;
	const char *Pattern = Regex->Pattern;
	while (*Pattern) Hash = ((Hash << 5) + Hash) + *(Pattern++);
	return Hash;
}

ml_type_t MLRegexT[1] = {{
	MLAnyT, "regex",
	ml_regex_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

ml_value_t *ml_regex(const char *Pattern) {
	ml_regex_t *Regex = fnew(ml_regex_t);
	Regex->Type = MLRegexT;
	Regex->Pattern = Pattern;
	int Error = regcomp(Regex->Value, Pattern, REG_EXTENDED);
	if (Error) {
		size_t ErrorSize = regerror(Error, Regex->Value, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(Error, Regex->Value, ErrorMessage, ErrorSize);
		return ml_error("RegexError", ErrorMessage);
	}
	return (ml_value_t *)Regex;
}

typedef struct ml_method_node_t ml_method_node_t;

struct ml_method_node_t {
	ml_method_node_t *Child;
	ml_method_node_t *Next;
	const ml_type_t *Type;
	void *Data;
	ml_callback_t Callback;
};

struct ml_method_t {
	const ml_type_t *Type;
	const char *Name;
	ml_method_node_t Root[1];
};

static ml_method_node_t *ml_method_find(ml_method_node_t *Node, int Count, ml_value_t **Args) {
	if (Count == 0) return Node;
	for (const ml_type_t *Type = Args[0]->Type; Type; Type = Type->Parent) {
		for (ml_method_node_t *Test = Node->Child; Test; Test = Test->Next) {
			if (Test->Type == Type) {
				ml_method_node_t *Result = ml_method_find(Test, Count - 1, Args + 1);
				if (Result && Result->Callback) return Result;
			}
		}
	}
	return Node;
}

static long ml_method_hash(ml_value_t *Value) {
	ml_method_t *Method = (ml_method_t *)Value;
	long Hash = 5381;
	for (const char *P = Method->Name;P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

ml_value_t *ml_method_call(ml_value_t *Value, int Count, ml_value_t **Args) {
	ml_method_t *Method = (ml_method_t *)Value;
	ml_method_node_t *Node = ml_method_find(Method->Root, Count, Args);
	if (Node->Callback) {
		return (Node->Callback)(Node->Data, Count, Args);
	} else {
		int Length = 4;
		for (int I = 0; I < Count; ++I) Length += strlen(Args[I]->Type->Name) + 2;
		char *Types = snew(Length);
		char *P = Types;
		for (int I = 0; I < Count; ++I) P = stpcpy(stpcpy(P, Args[I]->Type->Name), ", ");
		P[-2] = 0;
		return ml_error("MethodError", "no matching method found for %s(%s)", Method->Name, Types);
	}
}

ml_type_t MLMethodT[1] = {{
	MLFunctionT, "method",
	ml_method_hash,
	ml_method_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

static int NumMethods = 0;
static int MaxMethods = 2;
static ml_method_t **Methods;

ml_value_t *ml_method(const char *Name) {
	if (!Name) {
		ml_method_t *Method = new(ml_method_t);
		Method->Type = MLMethodT;
		Method->Name = Name;
		return (ml_value_t *)Method;
	}
	int Lo = 0, Hi = NumMethods - 1;
	while (Lo <= Hi) {
		int Mid = (Lo + Hi) / 2;
		int Cmp = strcmp(Name, Methods[Mid]->Name);
		if (Cmp < 0) {
			Hi = Mid - 1;
		} else if (Cmp > 0) {
			Lo = Mid + 1;
		} else {
			return (ml_value_t *)Methods[Mid];
		}
	}
	ml_method_t *Method = new(ml_method_t);
	Method->Type = MLMethodT;
	Method->Name = Name;
	ml_method_t **SourceMethods = Methods;
	ml_method_t **TargetMethods = Methods;
	if (++NumMethods > MaxMethods) {
		MaxMethods += 32;
		Methods = TargetMethods = anew(ml_method_t *, MaxMethods);
		for (int I = Lo; --I >= 0;) TargetMethods[I] = SourceMethods[I];
	}
	for (int I = NumMethods; I > Lo; --I) TargetMethods[I] = SourceMethods[I - 1];
	TargetMethods[Lo] = Method;
	return (ml_value_t *)Method;
}

void ml_method_by_name(const char *Name, void *Data, ml_callback_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)ml_method(Name);
	ml_method_node_t *Node = Method->Root;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) {
		ml_method_node_t **Slot = &Node->Child;
		while (Slot[0] && Slot[0]->Type != Type) Slot = &Slot[0]->Next;
		if (Slot[0]) {
			Node = Slot[0];
		} else {
			Node = Slot[0] = new(ml_method_node_t);
			Node->Type = Type;
		}

	}
	va_end(Args);
	Node->Data = Data;
	Node->Callback = Callback;
}

void ml_method_by_value(ml_value_t *Value, void *Data, ml_callback_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)Value;
	ml_method_node_t *Node = Method->Root;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) {
		ml_method_node_t **Slot = &Node->Child;
		while (Slot[0] && Slot[0]->Type != Type) Slot = &Slot[0]->Next;
		if (Slot[0]) {
			Node = Slot[0];
		} else {
			Node = Slot[0] = new(ml_method_node_t);
			Node->Type = Type;
		}

	}
	va_end(Args);
	Node->Data = Data;
	Node->Callback = Callback;
}

static ml_value_t *ml_reference_deref(ml_value_t *Ref) {
	ml_reference_t *Reference = (ml_reference_t *)Ref;
	return Reference->Address[0];
}

static ml_value_t *ml_reference_assign(ml_value_t *Ref, ml_value_t *Value) {
	ml_reference_t *Reference = (ml_reference_t *)Ref;
	return Reference->Address[0] = Value;
}

ml_type_t MLReferenceT[1] = {{
	MLAnyT, "reference",
	ml_default_hash,
	ml_default_call,
	ml_reference_deref,
	ml_reference_assign,
	ml_default_next,
	ml_default_key
}};

ml_value_t *ml_reference(ml_value_t **Address) {
	ml_reference_t *Reference;
	if (Address == 0) {
		Reference = xnew(ml_reference_t, 1, ml_value_t *);
		Reference->Address = Reference->Value;
		Reference->Value[0] = MLNil;
	} else {
		Reference = new(ml_reference_t);
		Reference->Address = Address;
	}
	Reference->Type = MLReferenceT;
	return (ml_value_t *)Reference;
}

int ml_list_length(ml_value_t *Value) {
	return ((ml_list_t *)Value)->Length;
}

void ml_list_to_array(ml_value_t *Value, ml_value_t **Array) {
	ml_list_t *List = (ml_list_t *)Value;
	for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) *Array++ = Node->Value;
}

static ml_value_t *ml_list_length_value(void *Data, int Count, ml_value_t **Args) {
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_integer(List->Length);
}

static ml_value_t *ml_list_index(void *Data, int Count, ml_value_t **Args) {
	ml_list_t *List = (ml_list_t *)Args[0];
	long Index = ((ml_integer_t *)Args[1])->Value;
	if (Index > 0) {
		for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) {
			if (--Index == 0) return ml_reference(&Node->Value);
		}
		return MLNil;
	} else {
		Index = -Index;
		for (ml_list_node_t *Node = List->Tail; Node; Node = Node->Prev) {
			if (--Index == 0) return ml_reference(&Node->Value);
		}
		return MLNil;
	}
}

static ml_value_t *ml_list_slice(void *Data, int Count, ml_value_t **Args) {
	ml_list_t *List = (ml_list_t *)Args[0];
	long Index = ((ml_integer_t *)Args[1])->Value;
	long End = ((ml_integer_t *)Args[2])->Value;
	long Start = Index;
	if (Start <= 0) Start += List->Length + 1;
	if (End <= 0) End += List->Length + 1;
	if (Start <= 0 || End < Start || End > List->Length + 1) return MLNil;
	long Length = End - Start;
	ml_list_node_t *Source = 0;
	if (Index > 0) {
		for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) {
			if (--Index == 0) {
				Source = Node;
				break;
			}
		}
	} else {
		Index = -Index;
		for (ml_list_node_t *Node = List->Tail; Node; Node = Node->Prev) {
			if (--Index == 0) {
				Source = Node;
				break;
			}
		}
	}
	ml_list_t *Slice = (ml_list_t *)ml_list();
	Slice->Type = MLListT;
	Slice->Length = Length;
	ml_list_node_t **Slot = &Slice->Head, *Prev = 0, *Node = 0;
	while (--Length >= 0) {
		Node = Slot[0] = new(ml_list_node_t);
		Node->Prev = Prev;
		Node->Value = Source->Value;
		Slot = &Node->Next;
		Source = Source->Next;
		Prev = Node;
	}
	Slice->Tail = Node;
	return (ml_value_t *)Slice;
}

ml_type_t MLListT[1] = {{
	MLAnyT, "list",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

ml_value_t *ml_list() {
	ml_list_t *List = new(ml_list_t);
	List->Type = MLListT;
	return (ml_value_t *)List;
}

int ml_is_list(ml_value_t *Value) {
	return Value->Type == MLListT;
}

void ml_list_append(ml_value_t *List0, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = new(ml_list_node_t);
	Node->Value = Value;
	Node->Prev = List->Tail;
	if (List->Tail) {
		List->Tail->Next = Node;
	} else {
		List->Head = Node;
	}
	List->Tail = Node;
	List->Length += 1;
}

int ml_list_foreach(ml_value_t *Value, void *Data, int (*callback)(ml_value_t *, void *)) {
	ml_list_t *List = (ml_list_t *)Value;
	for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) {
		if (callback(Node->Value, Data)) return 1;
	}
	return 0;
}

ml_value_t *ml_list_new(void *Data, int Count, ml_value_t **Args) {
	ml_list_t *List = new(ml_list_t);
	List->Type = MLListT;
	ml_list_node_t **Slot = &List->Head;
	ml_list_node_t *Prev = NULL;
	for (int I = 0; I < Count; ++I) {
		ml_list_node_t *Node = Slot[0] = new(ml_list_node_t);
		Node->Value = Args[I];
		Node->Prev = Prev;
		Prev = Node;
		Slot = &Node->Next;
	}
	List->Tail = Prev;
	List->Length = Count;
	return (ml_value_t *)List;
}

struct ml_tree_t {
	const ml_type_t *Type;
	ml_tree_node_t *Root;
	int Size;
};

struct ml_tree_node_t {
	ml_tree_node_t *Left, *Right;
	ml_value_t *Key;
	ml_value_t *Value;
	long Hash;
	int Depth;
};

ml_value_t *ml_tree_search(ml_tree_t *Tree, ml_value_t *Key) {
	ml_tree_node_t *Node = Tree->Root;
	long Hash = ml_hash(Key);
	while (Node) {
		int Compare;
		if (Hash < Node->Hash) {
			Compare = -1;
		} else if (Hash > Node->Hash) {
			Compare = 1;
		} else {
			ml_value_t *Args[2] = {Key, Node->Key};
			ml_value_t *Result = ml_method_call(CompareMethod, 2, Args);
			if (Result->Type == MLIntegerT) {
				Compare = ((ml_integer_t *)Result)->Value;
			} else if (Result->Type == MLRealT) {
				Compare = ((ml_real_t *)Result)->Value;
			} else {
				return ml_error("CompareError", "comparison must return number");
			}
		}
		if (!Compare) {
			return Node->Value;
		} else {
			Node = Compare < 0 ? Node->Left : Node->Right;
		}
	}
	return MLNil;
}

static int ml_tree_balance(ml_tree_node_t *Node) {
	int Delta = 0;
	if (Node->Left) Delta = Node->Left->Depth;
	if (Node->Right) Delta -= Node->Right->Depth;
	return Delta;
}

static void ml_tree_update_depth(ml_tree_node_t *Node) {
	int Depth = 0;
	if (Node->Left) Depth = Node->Left->Depth;
	if (Node->Right && Depth < Node->Right->Depth) Depth = Node->Right->Depth;
	Node->Depth = Depth + 1;
}

static void ml_tree_rotate_left(ml_tree_node_t **Slot) {
	ml_tree_node_t *Ch = Slot[0]->Right;
	Slot[0]->Right = Slot[0]->Right->Left;
	Ch->Left = Slot[0];
	ml_tree_update_depth(Slot[0]);
	Slot[0] = Ch;
	ml_tree_update_depth(Slot[0]);
}

static void ml_tree_rotate_right(ml_tree_node_t **Slot) {
	ml_tree_node_t *Ch = Slot[0]->Left;
	Slot[0]->Left = Slot[0]->Left->Right;
	Ch->Right = Slot[0];
	ml_tree_update_depth(Slot[0]);
	Slot[0] = Ch;
	ml_tree_update_depth(Slot[0]);
}

static void ml_tree_rebalance(ml_tree_node_t **Slot) {
	int Delta = ml_tree_balance(Slot[0]);
	if (Delta == 2) {
		if (ml_tree_balance(Slot[0]->Left) < 0) ml_tree_rotate_left(&Slot[0]->Left);
		ml_tree_rotate_right(Slot);
	} else if (Delta == -2) {
		if (ml_tree_balance(Slot[0]->Right) > 0) ml_tree_rotate_right(&Slot[0]->Right);
		ml_tree_rotate_left(Slot);
	}
}

static ml_value_t *ml_tree_insert_internal(ml_tree_t *Tree, ml_tree_node_t **Slot, long Hash, ml_value_t *Key, ml_value_t *Value) {
	if (!Slot[0]) {
		++Tree->Size;
		ml_tree_node_t *Node = Slot[0] = new(ml_tree_node_t);
		Node->Depth = 1;
		Node->Hash = Hash;
		Node->Key = Key;
		Node->Value = Value;
		return NULL;
	}
	int Compare;
	if (Hash < Slot[0]->Hash) {
		Compare = -1;
	} else if (Hash > Slot[0]->Hash) {
		Compare = 1;
	} else {
		ml_value_t *Args[2] = {Key, Slot[0]->Key};
		ml_value_t *Result = ml_method_call(CompareMethod, 2, Args);
		if (Result->Type == MLIntegerT) {
			Compare = ((ml_integer_t *)Result)->Value;
		} else if (Result->Type == MLRealT) {
			Compare = ((ml_real_t *)Result)->Value;
		} else {
			return ml_error("CompareError", "comparison must return number");
		}
	}
	if (!Compare) {
		ml_value_t *Old = Slot[0]->Value;
		Slot[0]->Value = Value;
		return Old;
	} else {
		ml_value_t *Old = ml_tree_insert_internal(Tree, Compare < 0 ? &Slot[0]->Left : &Slot[0]->Right, Hash, Key, Value);
		ml_tree_rebalance(Slot);
		ml_tree_update_depth(Slot[0]);
		return Old;
	}
}

ml_value_t *ml_tree_insert(ml_tree_t *Tree, ml_value_t *Key, ml_value_t *Value) {
	return ml_tree_insert_internal(Tree, &Tree->Root, ml_hash(Key), Key, Value);
}

static void ml_tree_remove_depth_helper(ml_tree_node_t *Node) {
	if (Node) {
		ml_tree_remove_depth_helper(Node->Right);
		ml_tree_update_depth(Node);
	}
}

static ml_value_t *ml_tree_remove_internal(ml_tree_t *Tree, ml_tree_node_t **Slot, long Hash, ml_value_t *Key) {
	if (!Slot[0]) return MLNil;
	int Compare;
	if (Hash < Slot[0]->Hash) {
		Compare = -1;
	} else if (Hash > Slot[0]->Hash) {
		Compare = 1;
	} else {
		ml_value_t *Args[2] = {Key, Slot[0]->Key};
		ml_value_t *Result = ml_method_call(CompareMethod, 2, Args);
		if (Result->Type == MLIntegerT) {
			Compare = ((ml_integer_t *)Result)->Value;
		} else if (Result->Type == MLRealT) {
			Compare = ((ml_real_t *)Result)->Value;
		} else {
			return ml_error("CompareError", "comparison must return number");
		}
	}
	ml_value_t *Removed = MLNil;
	if (!Compare) {
		--Tree->Size;
		Removed = Slot[0]->Value;
		if (Slot[0]->Left && Slot[0]->Right) {
			ml_tree_node_t **Y = &Slot[0]->Left;
			while (Y[0]->Right) Y = &Y[0]->Right;
			Slot[0]->Key = Y[0]->Key;
			Slot[0]->Hash = Y[0]->Hash;
			Slot[0]->Value = Y[0]->Value;
			Y[0] = Y[0]->Left;
			ml_tree_remove_depth_helper(Slot[0]->Left);
		} else if (Slot[0]->Left) {
			Slot[0] = Slot[0]->Left;
		} else if (Slot[0]->Right) {
			Slot[0] = Slot[0]->Right;
		} else {
			Slot[0] = 0;
		}
	} else {
		Removed = ml_tree_remove_internal(Tree, Compare < 0 ? &Slot[0]->Left : &Slot[0]->Right, Hash, Key);
	}
	if (Slot[0]) {
		ml_tree_update_depth(Slot[0]);
		ml_tree_rebalance(Slot);
	}
	return Removed;
}

ml_value_t *ml_tree_remove(ml_tree_t *Tree, ml_value_t *Key) {
	return ml_tree_remove_internal(Tree, &Tree->Root, ml_hash(Key), Key);
}

static ml_value_t *ml_tree_index_get(void *Data, const char *Name) {
	ml_tree_t *Tree = (ml_tree_t *)Data;
	ml_value_t *Key = (ml_value_t *)Name;
	return ml_tree_search(Tree, Key);
}

static ml_value_t *ml_tree_index_set(void *Data, const char *Name, ml_value_t *Value) {
	ml_tree_t *Tree = (ml_tree_t *)Data;
	ml_value_t *Key = (ml_value_t *)Name;
	ml_tree_insert(Tree, Key, Value);
	return Value;
}

static ml_value_t *ml_tree_size(void *Data, int Count, ml_value_t **Args) {
	ml_tree_t *Tree = (ml_tree_t *)Args[0];
	return ml_integer(Tree->Size);
}

static ml_value_t *ml_tree_index(void *Data, int Count, ml_value_t **Args) {
	ml_tree_t *Tree = (ml_tree_t *)Args[0];
	if (Count < 1) return MLNil;
	ml_value_t *Key = Args[1];
	return ml_property(Tree, (const char *)Key, ml_tree_index_get, ml_tree_index_set, NULL, NULL);
}

static ml_value_t *ml_tree_delete(void *Data, int Count, ml_value_t **Args) {
	if (Count < 2) return MLNil;
	ml_tree_t *Tree = (ml_tree_t *)Args[0];
	ml_value_t *Key = Args[1];
	return ml_tree_remove(Tree, Key);
}

ml_type_t MLTreeT[1] = {{
	MLAnyT, "tree",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

ml_value_t *ml_tree() {
	ml_tree_t *Tree = new(ml_tree_t);
	Tree->Type = MLTreeT;
	return (ml_value_t *)Tree;
}

int ml_is_tree(ml_value_t *Value) {
	return Value->Type == MLTreeT;
}

static int ml_tree_node_foreach(ml_tree_node_t *Node, void *Data, int (*callback)(ml_value_t *, ml_value_t *, void *)) {
	if (callback(Node->Key, Node->Value, Data)) return 1;
	if (Node->Left && ml_tree_node_foreach(Node->Left, Data, callback)) return 1;
	if (Node->Right && ml_tree_node_foreach(Node->Right, Data, callback)) return 1;
	return 0;
}

int ml_tree_foreach(ml_value_t *Value, void *Data, int (*callback)(ml_value_t *, ml_value_t *, void *)) {
	ml_tree_t *Tree = (ml_tree_t *)Value;
	return Tree->Root ? ml_tree_node_foreach(Tree->Root, Data, callback) : 0;
}

ml_value_t *ml_tree_new(void *Data, int Count, ml_value_t **Args) {
	ml_tree_t *Tree = new(ml_tree_t);
	Tree->Type = MLTreeT;
	for (int I = 0; I < Count; I += 2) ml_tree_insert(Tree, Args[I], Args[I + 1]);
	return (ml_value_t *)Tree;
}

struct ml_property_t {
	const ml_type_t *Type;
	void *Data;
	const char *Name;
	ml_getter_t Get;
	ml_setter_t Set;
	ml_getter_t Next;
	ml_getter_t Key;
};

static ml_value_t *ml_property_deref(ml_value_t *Ref) {
	ml_property_t *Property = (ml_property_t *)Ref;
	return (Property->Get)(Property->Data, Property->Name);
}

static ml_value_t *ml_property_assign(ml_value_t *Ref, ml_value_t *Value) {
	ml_property_t *Property = (ml_property_t *)Ref;
	if (Property->Set) {
		return (Property->Set)(Property->Data, Property->Name, Value);
	} else {
		return ml_error("TypeError", "value is not assignable");
	}
}

static ml_value_t *ml_property_next(ml_value_t *Iter) {
	ml_property_t *Property = (ml_property_t *)Iter;
	if (Property->Next) {
		return (Property->Next)(Property->Data, "next");
	} else {
		return ml_error("TypeError", "value is not iterable");
	}
}

static ml_value_t *ml_property_key(ml_value_t *Iter) {
	ml_property_t *Property = (ml_property_t *)Iter;
	if (Property->Key) {
		return (Property->Key)(Property->Data, "next");
	} else {
		return ml_error("TypeError", "value is not iterable");
	}
}

ml_type_t MLPropertyT[1] = {{
	MLAnyT, "property",
	ml_default_hash,
	ml_default_call,
	ml_property_deref,
	ml_property_assign,
	ml_property_next,
	ml_property_key
}};

ml_value_t *ml_property(void *Data, const char *Name, ml_getter_t Get, ml_setter_t Set, ml_getter_t Next, ml_getter_t Key) {
	ml_property_t *Property = new(ml_property_t);
	Property->Type = MLPropertyT;
	Property->Data = Data;
	Property->Name = Name;
	Property->Get = Get;
	Property->Set = Set;
	Property->Next = Next;
	Property->Key = Key;
	return (ml_value_t *)Property;
}

void ml_closure_hash(ml_value_t *Value, unsigned char Hash[SHA256_BLOCK_SIZE]) {
	ml_closure_t *Closure = (ml_closure_t *)Value;
	memcpy(Hash, Closure->Info->Hash, SHA256_BLOCK_SIZE);
}

ml_type_t MLErrorT[1] = {{
	MLAnyT, "error",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

ml_type_t MLErrorValueT[1] = {{
	MLErrorT, "error_value",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

ml_value_t *ml_error(const char *Error, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Message;
	vasprintf(&Message, Format, Args);
	va_end(Args);
	ml_error_t *Value = new(ml_error_t);
	Value->Type = MLErrorT;
	Value->Error = Error;
	Value->Message = Message;
	memset(Value->Trace, 0, sizeof(Value->Trace));
	return (ml_value_t *)Value;
}

int ml_is_error(ml_value_t *Value) {
	return Value->Type == MLErrorT;
}

const char *ml_error_type(ml_value_t *Value) {
	return ((ml_error_t *)Value)->Error;
}

const char *ml_error_message(ml_value_t *Value) {
	return ((ml_error_t *)Value)->Message;
}

static ml_value_t *ml_error_type_value(void *Data, int Count, ml_value_t **Args) {
	return ml_string(((ml_error_t *)Args[0])->Error, -1);
}

static ml_value_t *ml_error_message_value(void *Data, int Count, ml_value_t **Args) {
	return ml_string(((ml_error_t *)Args[0])->Message, -1);
}

int ml_error_trace(ml_value_t *Value, int Level, const char **Source, int *Line) {
	ml_error_t *Error = (ml_error_t *)Value;
	if (Level >= MAX_TRACE) return 0;
	if (!Error->Trace[Level].Name) return 0;
	Source[0] = Error->Trace[Level].Name;
	Line[0] = Error->Trace[Level].Line;
	return 1;
}

struct ml_stringbuffer_node_t {
	ml_stringbuffer_node_t *Next;
	char Chars[ML_STRINGBUFFER_NODE_SIZE];
};

static ml_stringbuffer_node_t *Cache = NULL;
static GC_descr StringBufferDesc = 0;
static pthread_mutex_t CacheMutex[1] = {PTHREAD_MUTEX_DEFAULT};

ssize_t ml_stringbuffer_add(ml_stringbuffer_t *Buffer, const char *String, size_t Length) {
	size_t Remaining = Length;
	ml_stringbuffer_node_t **Slot = &Buffer->Nodes;
	ml_stringbuffer_node_t *Node = Buffer->Nodes;
	if (Node) {
		while (Node->Next) Node = Node->Next;
		Slot = &Node->Next;
	}
	while (Buffer->Space < Remaining) {
		memcpy(Node->Chars + ML_STRINGBUFFER_NODE_SIZE - Buffer->Space, String, Buffer->Space);
		String += Buffer->Space;
		Remaining -= Buffer->Space;
		ml_stringbuffer_node_t *Next;
		pthread_mutex_lock(CacheMutex);
		if (Cache) {
			Next = Cache;
			Cache = Cache->Next;
			Next->Next = NULL;
		} else {
			Next = (ml_stringbuffer_node_t *)GC_malloc_explicitly_typed(sizeof(ml_stringbuffer_node_t), StringBufferDesc);
			//printf("Allocating stringbuffer: %d in total\n", ++NumStringBuffers);
		}
		pthread_mutex_unlock(CacheMutex);
		Node = Slot[0] = Next;
		Slot = &Node->Next;
		Buffer->Space = ML_STRINGBUFFER_NODE_SIZE;
	}
	memcpy(Node->Chars + ML_STRINGBUFFER_NODE_SIZE - Buffer->Space, String, Remaining);
	Buffer->Space -= Remaining;
	Buffer->Length += Length;
	return Length;
}

ssize_t ml_stringbuffer_addf(ml_stringbuffer_t *Buffer, const char *Format, ...) {
	char *String;
	va_list Args;
	va_start(Args, Format);
	size_t Length = vasprintf(&String, Format, Args);
	va_end(Args);
	return ml_stringbuffer_add(Buffer, String, Length);
}

char *ml_stringbuffer_get(ml_stringbuffer_t *Buffer) {
	char *String = snew(Buffer->Length + 1);
	if (Buffer->Length == 0) {
		String[0] = 0;
	} else {
		char *P = String;
		ml_stringbuffer_node_t *Node = Buffer->Nodes;
		while (Node->Next) {
			memcpy(P, Node->Chars, ML_STRINGBUFFER_NODE_SIZE);
			P += ML_STRINGBUFFER_NODE_SIZE;
			Node = Node->Next;
		}
		memcpy(P, Node->Chars, ML_STRINGBUFFER_NODE_SIZE - Buffer->Space);
		P += ML_STRINGBUFFER_NODE_SIZE - Buffer->Space;
		*P++ = 0;
		pthread_mutex_lock(CacheMutex);
		ml_stringbuffer_node_t **Slot = &Cache;
		while (Slot[0]) Slot = &Slot[0]->Next;
		Slot[0] = Buffer->Nodes;
		pthread_mutex_unlock(CacheMutex);
		Buffer->Nodes = NULL;
		Buffer->Length = Buffer->Space = 0;
	}
	return String;
}

ml_type_t MLStringBufferT[1] = {{
	MLAnyT, "stringbuffer",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	ml_default_next,
	ml_default_key
}};

int ml_stringbuffer_foreach(ml_stringbuffer_t *Buffer, void *Data, int (*callback)(const char *, size_t, void *)) {
	ml_stringbuffer_node_t *Node = Buffer->Nodes;
	if (!Node) return 0;
	while (Node->Next) {
		if (callback(Node->Chars, ML_STRINGBUFFER_NODE_SIZE, Data)) return 1;
		Node = Node->Next;
	}
	return callback(Node->Chars, ML_STRINGBUFFER_NODE_SIZE - Buffer->Space, Data);
}

ml_value_t *stringify_nil(void *Data, int Count, ml_value_t **Args) {
	return MLNil;
}

ml_value_t *stringify_integer(void *Data, int Count, ml_value_t **Args) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_addf(Buffer, "%d", ml_integer_value(Args[1]));
	return MLSome;
}

ml_value_t *stringify_real(void *Data, int Count, ml_value_t **Args) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_addf(Buffer, "%f", ml_real_value(Args[1]));
	return MLSome;
}

ml_value_t *stringify_string(void *Data, int Count, ml_value_t **Args) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_add(Buffer, ml_string_value(Args[1]), ml_string_length(Args[1]));
	return ml_string_length(Args[1]) ? MLSome : MLNil;
}

ml_value_t *stringify_method(void *Data, int Count, ml_value_t **Args) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_method_t *Method = (ml_method_t *)Args[1];
	ml_stringbuffer_add(Buffer, Method->Name, strlen(Method->Name));
	return MLSome;
}

ml_value_t *stringify_list(void *Data, int Count, ml_value_t **Args) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_list_node_t *Node = ((ml_list_t *)Args[1])->Head;
	if (Node) {
		ml_inline(AppendMethod, 2, Buffer, Node->Value);
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, " ", 1);
			ml_inline(AppendMethod, 2, Buffer, Node->Value);
		}
		return MLSome;
	} else {
		return MLNil;
	}
}

typedef struct {ml_stringbuffer_t *Buffer; int Space;} ml_stringify_context_t;

static int stringify_tree_value(ml_value_t *Key, ml_value_t *Value, ml_stringify_context_t *Ctx) {
	if (Ctx->Space) ml_stringbuffer_add(Ctx->Buffer, " ", 1);
	ml_inline(AppendMethod, 2, Ctx->Buffer, Key);
	if (Value != MLNil) {
		ml_stringbuffer_add(Ctx->Buffer, "=", 1);
		ml_inline(AppendMethod, 2, Ctx->Buffer, Value);
	}
	Ctx->Space = 1;
	return 0;
}

ml_value_t *stringify_tree(void *Data, int Count, ml_value_t **Args) {
	ml_stringify_context_t Ctx[1] = {(ml_stringbuffer_t *)Args[0], 0};
	ml_tree_foreach(Args[1], Ctx, (void *)stringify_tree_value);
	return ((ml_tree_t *)Args[1])->Size ? MLSome : MLNil;
}

#define ml_arith_method_integer(NAME, SYMBOL) \
	static ml_value_t *ml_ ## NAME ## _integer(void *Data, int Count, ml_value_t **Args) { \
		ml_integer_t *IntegerA = (ml_integer_t *)Args[0]; \
		return ml_integer(SYMBOL(IntegerA->Value)); \
	}

#define ml_arith_method_integer_integer(NAME, SYMBOL) \
	static ml_value_t *ml_ ## NAME ## _integer_integer(void *Data, int Count, ml_value_t **Args) { \
		ml_integer_t *IntegerA = (ml_integer_t *)Args[0]; \
		ml_integer_t *IntegerB = (ml_integer_t *)Args[1]; \
		return ml_integer(IntegerA->Value SYMBOL IntegerB->Value); \
	}

#define ml_arith_method_real(NAME, SYMBOL) \
	static ml_value_t *ml_ ## NAME ## _real(void *Data, int Count, ml_value_t **Args) { \
		ml_real_t *RealA = (ml_real_t *)Args[0]; \
		return ml_real(SYMBOL(RealA->Value)); \
	}

#define ml_arith_method_real_real(NAME, SYMBOL) \
	static ml_value_t *ml_ ## NAME ## _real_real(void *Data, int Count, ml_value_t **Args) { \
		ml_real_t *RealA = (ml_real_t *)Args[0]; \
		ml_real_t *RealB = (ml_real_t *)Args[1]; \
		return ml_real(RealA->Value SYMBOL RealB->Value); \
	}

#define ml_arith_method_real_integer(NAME, SYMBOL) \
	static ml_value_t *ml_ ## NAME ## _real_integer(void *Data, int Count, ml_value_t **Args) { \
		ml_real_t *RealA = (ml_real_t *)Args[0]; \
		ml_integer_t *IntegerB = (ml_integer_t *)Args[1]; \
		return ml_real(RealA->Value SYMBOL IntegerB->Value); \
	}

#define ml_arith_method_integer_real(NAME, SYMBOL) \
	static ml_value_t *ml_ ## NAME ## _integer_real(void *Data, int Count, ml_value_t **Args) { \
		ml_integer_t *IntegerA = (ml_integer_t *)Args[0]; \
		ml_real_t *RealB = (ml_real_t *)Args[1]; \
		return ml_real(IntegerA->Value SYMBOL RealB->Value); \
	}

#define ml_arith_method_number(NAME, SYMBOL) \
	ml_arith_method_integer(NAME, SYMBOL) \
	ml_arith_method_real(NAME, SYMBOL)

#define ml_arith_method_number_number(NAME, SYMBOL) \
	ml_arith_method_integer_integer(NAME, SYMBOL) \
	ml_arith_method_real_real(NAME, SYMBOL) \
	ml_arith_method_real_integer(NAME, SYMBOL) \
	ml_arith_method_integer_real(NAME, SYMBOL)

ml_arith_method_number(neg, -)
ml_arith_method_number_number(add, +)
ml_arith_method_number_number(sub, -)
ml_arith_method_number_number(mul, *)
ml_arith_method_number_number(div, /)

ml_arith_method_integer_integer(mod, %)

#define ml_comp_method_integer_integer(NAME, SYMBOL) \
	static ml_value_t *ml_ ## NAME ## _integer_integer(void *Data, int Count, ml_value_t **Args) { \
		ml_integer_t *IntegerA = (ml_integer_t *)Args[0]; \
		ml_integer_t *IntegerB = (ml_integer_t *)Args[1]; \
		return IntegerA->Value SYMBOL IntegerB->Value ? Args[1] : MLNil; \
	}

#define ml_comp_method_real_real(NAME, SYMBOL) \
	static ml_value_t *ml_ ## NAME ## _real_real(void *Data, int Count, ml_value_t **Args) { \
		ml_real_t *RealA = (ml_real_t *)Args[0]; \
		ml_real_t *RealB = (ml_real_t *)Args[1]; \
		return RealA->Value SYMBOL RealB->Value ? Args[1] : MLNil; \
	}

#define ml_comp_method_real_integer(NAME, SYMBOL) \
	static ml_value_t *ml_ ## NAME ## _real_integer(void *Data, int Count, ml_value_t **Args) { \
		ml_real_t *RealA = (ml_real_t *)Args[0]; \
		ml_integer_t *IntegerB = (ml_integer_t *)Args[1]; \
		return RealA->Value SYMBOL IntegerB->Value ? Args[1] : MLNil; \
	}

#define ml_comp_method_integer_real(NAME, SYMBOL) \
	static ml_value_t *ml_ ## NAME ## _integer_real(void *Data, int Count, ml_value_t **Args) { \
		ml_integer_t *IntegerA = (ml_integer_t *)Args[0]; \
		ml_real_t *RealB = (ml_real_t *)Args[1]; \
		return IntegerA->Value SYMBOL RealB->Value ? Args[1] : MLNil; \
	}

#define ml_comp_method_number_number(NAME, SYMBOL) \
	ml_comp_method_integer_integer(NAME, SYMBOL) \
	ml_comp_method_real_real(NAME, SYMBOL) \
	ml_comp_method_real_integer(NAME, SYMBOL) \
	ml_comp_method_integer_real(NAME, SYMBOL)

ml_comp_method_number_number(eq, ==)
ml_comp_method_number_number(neq, !=)
ml_comp_method_number_number(les, <)
ml_comp_method_number_number(gre, >)
ml_comp_method_number_number(leq, <=)
ml_comp_method_number_number(geq, >=)

static ml_integer_t One[1] = {{MLIntegerT, 1}};
static ml_integer_t NegOne[1] = {{MLIntegerT, -1}};
static ml_integer_t Zero[1] = {{MLIntegerT, 0}};

static ml_value_t *ml_compare_integer_integer(void *Data, int Count, ml_value_t **Args) {
	ml_integer_t *IntegerA = (ml_integer_t *)Args[0];
	ml_integer_t *IntegerB = (ml_integer_t *)Args[1];
	if (IntegerA->Value < IntegerB->Value) return (ml_value_t *)NegOne;
	if (IntegerA->Value > IntegerB->Value) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

static ml_value_t *ml_compare_real_integer(void *Data, int Count, ml_value_t **Args) {
	ml_real_t *RealA = (ml_real_t *)Args[0];
	ml_integer_t *IntegerB = (ml_integer_t *)Args[1];
	if (RealA->Value < IntegerB->Value) return (ml_value_t *)NegOne;
	if (RealA->Value > IntegerB->Value) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

static ml_value_t *ml_compare_integer_real(void *Data, int Count, ml_value_t **Args) {
	ml_integer_t *IntegerA = (ml_integer_t *)Args[0];
	ml_real_t *RealB = (ml_real_t *)Args[1];
	if (IntegerA->Value < RealB->Value) return (ml_value_t *)NegOne;
	if (IntegerA->Value > RealB->Value) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

static ml_value_t *ml_compare_real_real(void *Data, int Count, ml_value_t **Args) {
	ml_real_t *RealA = (ml_real_t *)Args[0];
	ml_real_t *RealB = (ml_real_t *)Args[1];
	if (RealA->Value < RealB->Value) return (ml_value_t *)NegOne;
	if (RealA->Value > RealB->Value) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

typedef struct ml_integer_range_t {
	const ml_type_t *Type;
	ml_integer_t *Current;
	long Step, Limit;
} ml_integer_range_t;

static ml_value_t *ml_integer_range_deref(ml_value_t *Ref) {
	ml_integer_range_t *Range = (ml_integer_range_t *)Ref;
	return (ml_value_t *)Range->Current;
}

static ml_value_t *ml_integer_range_next(ml_value_t *Ref) {
	ml_integer_range_t *Range = (ml_integer_range_t *)Ref;
	if (Range->Current->Value >= Range->Limit) {
		return MLNil;
	} else {
		Range->Current = (ml_integer_t *)ml_integer(Range->Current->Value + Range->Step);
		return Ref;
	}
}

ml_type_t MLIntegerRange[1] = {{
	MLAnyT, "integer-range",
	ml_default_hash,
	ml_default_call,
	ml_integer_range_deref,
	ml_default_assign,
	ml_integer_range_next,
	ml_default_key
}};

static ml_value_t *ml_range_integer_integer(void *Data, int Count, ml_value_t **Args) {
	ml_integer_t *IntegerA = (ml_integer_t *)Args[0];
	ml_integer_t *IntegerB = (ml_integer_t *)Args[1];
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRange;
	Range->Current = IntegerA;
	Range->Limit = IntegerB->Value;
	Range->Step = 1;
	return (ml_value_t *)Range;
}

#define ml_methods_add_number_number(NAME, SYMBOL) \
	ml_method_by_name(#SYMBOL, NULL, ml_ ## NAME ## _integer_integer, MLIntegerT, MLIntegerT, NULL); \
	ml_method_by_name(#SYMBOL, NULL, ml_ ## NAME ## _real_real, MLRealT, MLRealT, NULL); \
	ml_method_by_name(#SYMBOL, NULL, ml_ ## NAME ## _real_integer, MLRealT, MLIntegerT, NULL); \
	ml_method_by_name(#SYMBOL, NULL, ml_ ## NAME ## _integer_real, MLIntegerT, MLRealT, NULL)

static ml_value_t *ml_integer_to_string(void *Data, int Count, ml_value_t **Args) {
	ml_integer_t *Integer = (ml_integer_t *)Args[0];
	ml_string_t *String = new(ml_string_t);
	String->Type = MLStringT;
	String->Length = asprintf((char **)&String->Value, "%ld", Integer->Value);
	return (ml_value_t *)String;
}

static ml_value_t *ml_real_to_string(void *Data, int Count, ml_value_t **Args) {
	ml_real_t *Real = (ml_real_t *)Args[0];
	ml_string_t *String = new(ml_string_t);
	String->Type = MLStringT;
	String->Length = asprintf((char **)&String->Value, "%f", Real->Value);
	return (ml_value_t *)String;
}

static ml_value_t *ml_identity(void *Data, int Count, ml_value_t **Args) {
	return Args[0];
}

static ml_value_t *ml_compare_string_string(void *Data, int Count, ml_value_t **Args) {
	ml_string_t *StringA = (ml_string_t *)Args[0];
	ml_string_t *StringB = (ml_string_t *)Args[1];
	int Compare = strcmp(StringA->Value, StringB->Value);
	if (Compare < 0) return (ml_value_t *)NegOne;
	if (Compare > 0) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

#define ml_comp_method_string_string(NAME, SYMBOL) \
	static ml_value_t *ml_ ## NAME ## _string_string(void *Data, int Count, ml_value_t **Args) { \
		ml_string_t *StringA = (ml_string_t *)Args[0]; \
		ml_string_t *StringB = (ml_string_t *)Args[1]; \
		return strcmp(StringA->Value, StringB->Value) SYMBOL 0 ? Args[1] : MLNil; \
	}

ml_comp_method_string_string(eq, ==)
ml_comp_method_string_string(neq, !=)
ml_comp_method_string_string(les, <)
ml_comp_method_string_string(gre, >)
ml_comp_method_string_string(leq, <=)
ml_comp_method_string_string(geq, >=)

static ml_value_t *ml_compare_any_any(void *Data, int Count, ml_value_t **Args) {
	if (Args[0] < Args[1]) return ml_integer(-1);
	if (Args[0] > Args[1]) return ml_integer(1);
	return ml_integer(0);
}

typedef struct ml_list_iter_t {
	const ml_type_t *Type;
	ml_list_node_t *Node;
	long Index;
} ml_list_iter_t;

static ml_value_t *ml_list_iter_deref(ml_value_t *Ref) {
	ml_list_iter_t *Iter = (ml_list_iter_t *)Ref;
	return Iter->Node->Value;
}

static ml_value_t *ml_list_iter_assign(ml_value_t *Ref, ml_value_t *Value) {
	ml_list_iter_t *Iter = (ml_list_iter_t *)Ref;
	return Iter->Node->Value = Value;
}

static ml_value_t *ml_list_iter_next(ml_value_t *Ref) {
	ml_list_iter_t *Iter = (ml_list_iter_t *)Ref;
	if (Iter->Node->Next) {
		++Iter->Index;
		Iter->Node = Iter->Node->Next;
		return Ref;
	} else {
		return MLNil;
	}
}

static ml_value_t *ml_list_iter_key(ml_value_t *Ref) {
	ml_list_iter_t *Iter = (ml_list_iter_t *)Ref;
	return ml_integer(Iter->Index);
}

ml_type_t MLListIter[1] = {{
	MLAnyT, "list-iterator",
	ml_default_hash,
	ml_default_call,
	ml_list_iter_deref,
	ml_list_iter_assign,
	ml_list_iter_next,
	ml_list_iter_key
}};

static ml_value_t *ml_list_values(void *Data, int Count, ml_value_t **Args) {
	ml_list_t *List = (ml_list_t *)Args[0];
	if (List->Head) {
		ml_list_iter_t *Iter = new(ml_list_iter_t);
		Iter->Type = MLListIter;
		Iter->Node = List->Head;
		Iter->Index = 1;
		return (ml_value_t *)Iter;
	} else {
		return MLNil;
	}
}

static ml_value_t *ml_list_push(void *Data, int Count, ml_value_t **Args) {
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_node_t **Slot = List->Head ? &List->Head->Prev : &List->Tail;
	ml_list_node_t *Next = List->Head;
	for (int I = Count; --I >= 1;) {
		ml_list_node_t *Node = Slot[0] = new(ml_list_node_t);
		Node->Value = Args[I];
		Node->Next = Next;
		Next = Node;
		Slot = &Node->Prev;
	}
	List->Head = Next;
	List->Length += Count - 1;
	return (ml_value_t *)List;
}

static ml_value_t *ml_list_put(void *Data, int Count, ml_value_t **Args) {
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_node_t **Slot = List->Tail ? &List->Tail->Next : &List->Head;
	ml_list_node_t *Prev = List->Tail;
	for (int I = 1; I < Count; ++I) {
		ml_list_node_t *Node = Slot[0] = new(ml_list_node_t);
		Node->Value = Args[I];
		Node->Prev = Prev;
		Prev = Node;
		Slot = &Node->Next;
	}
	List->Tail = Prev;
	List->Length += Count - 1;
	return (ml_value_t *)List;
}

static ml_value_t *ml_list_pop(void *Data, int Count, ml_value_t **Args) {
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_node_t *Node = List->Head;
	if (Node) {
		if (!(List->Head = Node->Next)) List->Tail = NULL;
		--List->Length;
		return Node->Value;
	} else {
		return MLNil;
	}
}

static ml_value_t *ml_list_pull(void *Data, int Count, ml_value_t **Args) {
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_node_t *Node = List->Tail;
	if (Node) {
		if (!(List->Tail = Node->Next)) List->Head = NULL;
		--List->Length;
		return Node->Value;
	} else {
		return MLNil;
	}
}

static ml_value_t *ml_list_add(void *Data, int Count, ml_value_t **Args) {
	ml_list_t *List1 = (ml_list_t *)Args[0];
	ml_list_t *List2 = (ml_list_t *)Args[1];
	ml_list_t *List = new(ml_list_t);
	List->Type = MLListT;
	ml_list_node_t **Slot = &List->Head;
	ml_list_node_t *Prev = NULL;
	for (ml_list_node_t *Node1 = List1->Head; Node1; Node1 = Node1->Next) {
		ml_list_node_t *Node = Slot[0] = new(ml_list_node_t);
		Node->Value = Node1->Value;
		Node->Prev = Prev;
		Prev = Node;
		Slot = &Node->Next;
	}
	for (ml_list_node_t *Node2 = List2->Head; Node2; Node2 = Node2->Next) {
		ml_list_node_t *Node = Slot[0] = new(ml_list_node_t);
		Node->Value = Node2->Value;
		Node->Prev = Prev;
		Prev = Node;
		Slot = &Node->Next;
	}
	List->Tail = Prev;
	List->Length = List1->Length + List2->Length;
	return (ml_value_t *)List;
}

static ml_value_t *ml_list_to_string(void *Data, int Count, ml_value_t **Args) {
	ml_list_t *List = (ml_list_t *)Args[0];
	if (!List->Length) return ml_string("[]", 2);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Seperator = "[";
	int SeperatorLength = 1;
	for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) {
		ml_stringbuffer_add(Buffer, Seperator, SeperatorLength);
		ml_value_t *Result = ml_inline(AppendMethod, 2, Buffer, Node->Value);
		if (Result->Type == MLErrorT) return Result;
		Seperator = ", ";
		SeperatorLength = 2;
	}
	ml_stringbuffer_add(Buffer, "]", 1);
	return ml_string(ml_stringbuffer_get(Buffer), -1);
}

#define ML_TREE_MAX_DEPTH 32

typedef struct ml_tree_iter_t {
	const ml_type_t *Type;
	ml_tree_node_t *Node;
	ml_tree_node_t *Stack[ML_TREE_MAX_DEPTH];
	int Top;
} ml_tree_iter_t;

static ml_value_t *ml_tree_iter_deref(ml_value_t *Ref) {
	ml_tree_iter_t *Iter = (ml_tree_iter_t *)Ref;
	return Iter->Node->Value;
}

static ml_value_t *ml_tree_iter_assign(ml_value_t *Ref, ml_value_t *Value) {
	ml_tree_iter_t *Iter = (ml_tree_iter_t *)Ref;
	return Iter->Node->Value = Value;
}

static ml_value_t *ml_tree_iter_next(ml_value_t *Ref) {
	ml_tree_iter_t *Iter = (ml_tree_iter_t *)Ref;
	ml_tree_node_t *Node = Iter->Node;
	if (Node->Left) {
		if (Node->Right) Iter->Stack[Iter->Top++] = Node->Right;
		Iter->Node = Node->Left;
		return Ref;
	} else if (Node->Right) {
		Iter->Node = Node->Right;
		return Ref;
	} else if (Iter->Top > 0) {
		Iter->Node = Iter->Stack[--Iter->Top];
		return Ref;
	} else {
		return MLNil;
	}
}

static ml_value_t *ml_tree_iter_key(ml_value_t *Ref) {
	ml_tree_iter_t *Iter = (ml_tree_iter_t *)Ref;
	return Iter->Node->Key;
}

ml_type_t MLTreeIter[1] = {{
	MLAnyT, "tree-iterator",
	ml_default_hash,
	ml_default_call,
	ml_tree_iter_deref,
	ml_tree_iter_assign,
	ml_tree_iter_next,
	ml_tree_iter_key
}};

static ml_value_t *ml_tree_values(void *Data, int Count, ml_value_t **Args) {
	ml_tree_t *Tree = (ml_tree_t *)Args[0];
	if (Tree->Root) {
		ml_tree_iter_t *Iter = new(ml_tree_iter_t);
		Iter->Type = MLTreeIter;
		Iter->Node = Tree->Root;
		Iter->Top = 0;
		return (ml_value_t *)Iter;
	} else {
		return MLNil;
	}
}

static int ml_tree_add_insert(ml_value_t *Key, ml_value_t *Value, ml_tree_t *Tree) {
	ml_tree_insert(Tree, Key, Value);
	return 0;
}

static ml_value_t *ml_tree_add(void *Data, int Count, ml_value_t **Args) {
	ml_tree_t *Tree = new(ml_tree_t);
	Tree->Type = MLTreeT;
	ml_tree_foreach(Args[0], Tree, (void *)ml_tree_add_insert);
	ml_tree_foreach(Args[1], Tree, (void *)ml_tree_add_insert);
	return (ml_value_t *)Tree;
}

typedef struct ml_tree_stringer_t {
	const char *Seperator;
	ml_stringbuffer_t Buffer[1];
	int SeperatorLength;
	ml_value_t *Error;
} ml_tree_stringer_t;

static int ml_tree_stringer(ml_value_t *Key, ml_value_t *Value, ml_tree_stringer_t *Stringer) {
	ml_stringbuffer_add(Stringer->Buffer, Stringer->Seperator, Stringer->SeperatorLength);
	Stringer->Error = ml_inline(AppendMethod, 2, Stringer->Buffer, Key);
	if (Stringer->Error->Type == MLErrorT) return 1;
	ml_stringbuffer_add(Stringer->Buffer, " is ", 4);
	Stringer->Error = ml_inline(AppendMethod, 2, Stringer->Buffer, Value);
	if (Stringer->Error->Type == MLErrorT) return 1;
	Stringer->Seperator = ", ";
	Stringer->SeperatorLength = 2;
	return 0;
}

static ml_value_t *ml_tree_to_string(void *Data, int Count, ml_value_t **Args) {
	ml_tree_t *Tree = (ml_tree_t *)Args[0];
	if (!Tree->Size) return ml_string("{}", 2);
	ml_tree_stringer_t Stringer[1] = {{
		"{", {ML_STRINGBUFFER_INIT}, 1
	}};
	if (ml_tree_foreach(Args[0], Stringer, (void *)ml_tree_stringer)) return Stringer->Error;
	ml_stringbuffer_add(Stringer->Buffer, "}", 1);
	return ml_string(ml_stringbuffer_get(Stringer->Buffer), -1);
}

static ml_value_t *ml_hash_any(void *Data, int Count, ml_value_t **Args) {
	ml_value_t *Value = Args[0];
	return ml_integer(Value->Type->hash(Value));
}

static ml_value_t *ml_return_nil(void *Data, int Count, ml_value_t **Args) {
	return MLNil;
}

void ml_init() {
	Methods = anew(ml_method_t *, MaxMethods);
	CompareMethod = ml_method("?");
	ml_method_by_name("#", NULL, ml_hash_any, MLAnyT, NULL);
	ml_method_by_name("?", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("?", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name("=", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("=", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name("!=", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("!=", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name("<", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("<", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name(">", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name(">", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name("<=", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("<=", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name(">=", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name(">=", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name("-", NULL, ml_neg_integer, MLIntegerT, NULL);
	ml_method_by_name("-", NULL, ml_neg_real, MLRealT, NULL);
	ml_methods_add_number_number(compare, ?);
	ml_methods_add_number_number(add, +);
	ml_methods_add_number_number(sub, -);
	ml_methods_add_number_number(mul, *);
	ml_methods_add_number_number(div, /);
	ml_methods_add_number_number(eq, =);
	ml_methods_add_number_number(neq, !=);
	ml_methods_add_number_number(les, <);
	ml_methods_add_number_number(gre, >);
	ml_methods_add_number_number(leq, <=);
	ml_methods_add_number_number(geq, >=);
	ml_method_by_name("length", NULL, ml_string_length_value, MLStringT, NULL);
	ml_method_by_name("trim", NULL, ml_string_trim, MLStringT, NULL);
	ml_method_by_name("[]", NULL, ml_string_index, MLStringT, MLIntegerT, NULL);
	ml_method_by_name("[]", NULL, ml_string_slice, MLStringT, MLIntegerT, MLIntegerT, NULL);
	ml_method_by_name("%", NULL, ml_mod_integer_integer, MLIntegerT, MLIntegerT, NULL);
	ml_method_by_name("..", NULL, ml_range_integer_integer, MLIntegerT, MLIntegerT, NULL);
	ml_method_by_name("?", NULL, ml_compare_string_string, MLStringT, MLStringT, NULL);
	ml_method_by_name("=", NULL, ml_eq_string_string, MLStringT, MLStringT, NULL);
	ml_method_by_name("!=", NULL, ml_neq_string_string, MLStringT, MLStringT, NULL);
	ml_method_by_name("<", NULL, ml_les_string_string, MLStringT, MLStringT, NULL);
	ml_method_by_name(">", NULL, ml_gre_string_string, MLStringT, MLStringT, NULL);
	ml_method_by_name("<=", NULL, ml_leq_string_string, MLStringT, MLStringT, NULL);
	ml_method_by_name(">=", NULL, ml_geq_string_string, MLStringT, MLStringT, NULL);
	ml_method_by_name("?", NULL, ml_compare_any_any, MLAnyT, MLAnyT, NULL);
	ml_method_by_name("length", NULL, ml_list_length_value, MLListT, NULL);
	ml_method_by_name("[]", NULL, ml_list_index, MLListT, MLIntegerT, NULL);
	ml_method_by_name("[]", NULL, ml_list_slice, MLListT, MLIntegerT, MLIntegerT, NULL);
	ml_method_by_name("values", NULL, ml_list_values, MLListT, NULL);
	ml_method_by_name("push", NULL, ml_list_push, MLListT, NULL);
	ml_method_by_name("put", NULL, ml_list_put, MLListT, NULL);
	ml_method_by_name("pop", NULL, ml_list_pop, MLListT, NULL);
	ml_method_by_name("pull", NULL, ml_list_pull, MLListT, NULL);
	ml_method_by_name("+", NULL, ml_list_add, MLListT, MLListT, NULL);
	ml_method_by_name("size", NULL, ml_tree_size, MLTreeT, NULL);
	ml_method_by_name("[]", NULL, ml_tree_index, MLTreeT, MLAnyT, NULL);
	ml_method_by_name("values", NULL, ml_tree_values, MLTreeT, NULL);
	ml_method_by_name("delete", NULL, ml_tree_delete, MLTreeT, NULL);
	ml_method_by_name("+", NULL, ml_tree_add, MLTreeT, MLTreeT, NULL);
	ml_method_by_name("string", NULL, ml_nil_to_string, MLNilT, NULL);
	ml_method_by_name("string", NULL, ml_some_to_string, MLSomeT, NULL);
	ml_method_by_name("string", NULL, ml_integer_to_string, MLIntegerT, NULL);
	ml_method_by_name("string", NULL, ml_real_to_string, MLRealT, NULL);
	ml_method_by_name("string", NULL, ml_identity, MLStringT, NULL);
	ml_method_by_name("string", 0, ml_list_to_string, MLListT, 0);
	ml_method_by_name("string", 0, ml_tree_to_string, MLTreeT, 0);
	ml_method_by_name("/", NULL, ml_string_string_split, MLStringT, MLStringT, NULL);
	ml_method_by_name("/", NULL, ml_string_regex_split, MLStringT, MLRegexT, NULL);
	ml_method_by_name("%", NULL, ml_string_match, MLStringT, MLStringT, NULL);
	ml_method_by_name("find", 0, ml_string_find, MLStringT, MLStringT, 0);
	ml_method_by_name("replace", NULL, ml_string_string_replace, MLStringT, MLStringT, MLStringT, NULL);
	ml_method_by_name("replace", NULL, ml_string_regex_string_replace, MLStringT, MLRegexT, MLStringT, NULL);
	ml_method_by_name("replace", NULL, ml_string_regex_function_replace, MLStringT, MLRegexT, MLFunctionT, NULL);
	ml_method_by_name("type", NULL, ml_error_type_value, MLErrorT, NULL);
	ml_method_by_name("message", NULL, ml_error_message_value, MLErrorT, NULL);

	AppendMethod = ml_method("append");
	ml_method_by_value(AppendMethod, NULL, stringify_nil, MLStringBufferT, MLNilT, NULL);
	ml_method_by_value(AppendMethod, NULL, stringify_integer, MLStringBufferT, MLIntegerT, NULL);
	ml_method_by_value(AppendMethod, NULL, stringify_real, MLStringBufferT, MLRealT, NULL);
	ml_method_by_value(AppendMethod, NULL, stringify_string, MLStringBufferT, MLStringT, NULL);
	ml_method_by_value(AppendMethod, NULL, stringify_method, MLStringBufferT, MLMethodT, NULL);
	ml_method_by_value(AppendMethod, NULL, stringify_list, MLStringBufferT, MLListT, NULL);
	ml_method_by_value(AppendMethod, NULL, stringify_tree, MLStringBufferT, MLTreeT, NULL);

	GC_word StringBufferLayout[] = {1};
	StringBufferDesc = GC_make_descriptor(StringBufferLayout, 1);
}

ml_type_t *ml_class(ml_type_t *Parent, const char *Name) {
	ml_type_t *Type = new(ml_type_t);
	Type->Parent = Parent;
	Type->Name = Name;
	Type->hash = Parent->hash;
	Type->call = Parent->call;
	Type->deref = Parent->deref;
	Type->assign = Parent->assign;
	Type->next = Parent->next;
	Type->key = Parent->key;
	return Type;
}

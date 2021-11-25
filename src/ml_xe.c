#include "ml_xe.h"
#include "ml_macros.h"
#include <string.h>
#include <expat.h>
#include <ctype.h>

#undef ML_CATEGORY
#define ML_CATEGORY "xe"

typedef struct {
	const ml_type_t *Type;
	ml_value_t *Tag, *Attributes, *Content;
	ml_source_t Source;
} xe_node_t;

extern ml_type_t XENodeT[];

ML_FUNCTIONX(XENode) {
	ML_CHECKX_ARG_COUNT(3);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	ML_CHECKX_ARG_TYPE(1, MLMapT);
	ML_CHECKX_ARG_TYPE(2, MLListT);
	xe_node_t *Node = new(xe_node_t);
	Node->Type = XENodeT;
	Node->Tag = Args[0];
	Node->Attributes = Args[1];
	Node->Content = Args[2];
	Node->Source = ml_debugger_source(Caller);
	ML_RETURN(Node);
}

ML_TYPE(XENodeT, (), "xe-node",
	.Constructor = (ml_value_t *)XENode
);

typedef struct {
	const ml_type_t *Type;
	ml_value_t *Name;
} xe_var_t;

extern ml_type_t XEVarT[];

ML_FUNCTIONX(XEVar) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLStringT);
	xe_var_t *Var = new(xe_var_t);
	Var->Type = XEVarT;
	Var->Name = Args[0];
	ML_RETURN(Var);
}

ML_TYPE(XEVarT, (), "xe-var",
	.Constructor = (ml_value_t *)XEVar
);

ML_METHOD("tag", XENodeT) {
	xe_node_t *Node = (xe_node_t *)Args[0];
	return Node->Tag;
}

ML_METHOD("attributes", XENodeT) {
	xe_node_t *Node = (xe_node_t *)Args[0];
	return Node->Attributes;
}

ML_METHOD_DECL(Index, "[]");

ML_METHODX("[]", XENodeT, MLNilT) {
	xe_node_t *Node = (xe_node_t *)Args[0];
	ML_RETURN(Node->Content);
}

ML_METHODX("[]", XENodeT, MLStringT) {
	xe_node_t *Node = (xe_node_t *)Args[0];
	Args[0] = Node->Attributes;
	return ml_call(Caller, Index, Count, Args);
}

ML_METHODX("[]", XENodeT, MLIntegerT) {
	xe_node_t *Node = (xe_node_t *)Args[0];
	Args[0] = Node->Attributes;
	return ml_call(Caller, Index, Count, Args);
}

ML_METHOD("content", XENodeT) {
	xe_node_t *Node = (xe_node_t *)Args[0];
	return Node->Content;
}

ML_METHOD("name", XEVarT) {
	xe_var_t *Var = (xe_var_t *)Args[0];
	return Var->Name;
}

static int xe_attribute_to_string(ml_value_t *Key, ml_value_t *Value, ml_stringbuffer_t *Buffer) {
	ml_stringbuffer_write(Buffer, " ", 1);
	ml_stringbuffer_append(Buffer, Key);
	ml_stringbuffer_write(Buffer, "=", 1);
	ml_stringbuffer_append(Buffer, Value);
	return 0;
}

ML_METHOD("append", MLStringBufferT, XENodeT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	xe_node_t *Node = (xe_node_t *)Args[1];
	ml_stringbuffer_write(Buffer, "<", 1);
	ml_stringbuffer_write(Buffer, ml_string_value(Node->Tag), ml_string_length(Node->Tag));
	if (ml_map_size(Node->Attributes)) {
		ml_map_foreach(Node->Attributes, Buffer, (void *)xe_attribute_to_string);
	}
	if (ml_list_length(Node->Content)) {
		ml_stringbuffer_write(Buffer, ":", 1);
		ML_LIST_FOREACH(Node->Content, Iter) {
			ml_stringbuffer_append(Buffer, Iter->Value);
		}
	}
	ml_stringbuffer_write(Buffer, ">", 1);
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, XEVarT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	xe_var_t *Var = (xe_var_t *)Args[1];
	ml_stringbuffer_write(Buffer, "<$", 2);
	ml_stringbuffer_append(Buffer, Var->Name);
	ml_stringbuffer_write(Buffer, ">", 1);
	return MLSome;
}

typedef struct xe_stream_t xe_stream_t;

struct xe_stream_t {
	const char *Next;
	const char *(*read)(xe_stream_t *);
	void *Data;
	const char *Source;
	int LineNo;
};

static ml_value_t *parse_node(xe_stream_t *Stream);

static const char *parse_escape(const char *P, ml_stringbuffer_t *Buffer) {
	switch (P[1]) {
	case '\\':
		ml_stringbuffer_write(Buffer, "\\", 1);
		break;
	case 't':
		ml_stringbuffer_write(Buffer, "\t", 1);
		break;
	case 'r':
		ml_stringbuffer_write(Buffer, "\r", 1);
		break;
	case 'n':
		ml_stringbuffer_write(Buffer, "\n", 1);
		break;
	case '\"':
		ml_stringbuffer_write(Buffer, "\"", 1);
		break;
	case '<':
		ml_stringbuffer_write(Buffer, "<", 1);
		break;
	case '>':
		ml_stringbuffer_write(Buffer, ">", 1);
		break;
	case 'x': {
		unsigned char C;
		switch (P[2]) {
		case '0' ... '9': C = P[2] - '0'; break;
		case 'a' ... 'f': C = 10 + P[2] - 'a'; break;
		case 'A' ... 'F': C = 10 + P[2] - 'A'; break;
		default: return 0;
		}
		C *= 16;
		switch (P[3]) {
		case '0' ... '9': C += P[3] - '0'; break;
		case 'a' ... 'f': C += 10 + P[3] - 'a'; break;
		case 'A' ... 'F': C += 10 + P[3] - 'A'; break;
		default: return 0;
		}
		ml_stringbuffer_write(Buffer, (char *)&C, 1);
		P += 2;
		break;
	}
	}
	return P + 2;
}

static ml_value_t *parse_string(xe_stream_t *Stream) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Next = Stream->Next;
	const char *P = Next;
	for (;;) {
		if (P[0] == 0) {
			return ml_error("ParseError", "End of input in string at line %d in %s", Stream->LineNo, Stream->Source);
		} else if (P[0] == '\\') {
			ml_stringbuffer_write(Buffer, Next, P - Next);
			P = Next = parse_escape(P, Buffer);
			if (!P) return ml_error("ParseError", "Invalid escape sequence at line %d in %s", Stream->LineNo, Stream->Source);
		} else if (P[0] == '\"') {
			ml_stringbuffer_write(Buffer, Next, P - Next);
			Stream->Next = P + 1;
			break;
		} else {
			++P;
		}
	}
	return ml_stringbuffer_get_value(Buffer);
}

#define SKIP_WHITESPACE \
	while (Next[0] <= ' ') { \
		if (Next[0] == 0) { \
			Next = Stream->read(Stream); \
			if (!Next) return ml_error("ParseError", "Unexpected end of input at line %d in %s", Stream->LineNo, Stream->Source); \
			++Stream->LineNo; \
		} else { \
			++Next; \
		} \
	}

static ml_value_t *parse_value(xe_stream_t *Stream);

static ml_value_t *parse_list(xe_stream_t *Stream) {
	ml_value_t *List = ml_list();
	const char *Next = Stream->Next;
	SKIP_WHITESPACE;
	if (*Next == ']') {
		Stream->Next = Next + 1;
		return List;
	}
	--Next;
	do {
		Stream->Next = Next + 1;
		ml_value_t *Value = parse_value(Stream);
		if (ml_is_error(Value)) return Value;
		Next = Stream->Next;
		SKIP_WHITESPACE;
		ml_list_put(List, Value);
	} while (*Next == ',');
	if (*Next == ']') {
		Stream->Next = Next + 1;
	} else {
		List = ml_error("ParseError", "Invalid value syntax at line %d in %s", Stream->LineNo, Stream->Source);
	}
	return List;
}

static ml_value_t *parse_map(xe_stream_t *Stream) {
	ml_value_t *Map = ml_map();
	const char *Next = Stream->Next;
	SKIP_WHITESPACE;
	if (*Next == '}') {
		Stream->Next = Next + 1;
		return Map;
	}
	--Next;
	do {
		Stream->Next = Next + 1;
		ml_value_t *Key = parse_value(Stream);
		if (ml_is_error(Key)) return Key;
		Next = Stream->Next;
		SKIP_WHITESPACE;
		if (*Next != '=') {
			Map = ml_error("ParseError", "Invalid value syntax at line %d in %s", Stream->LineNo, Stream->Source);
			break;
		}
		Stream->Next = Next + 1;
		ml_value_t *Value = parse_value(Stream);
		Next = Stream->Next;
		SKIP_WHITESPACE;
		ml_map_insert(Map, Key, Value);
	} while (*Next == ',');
	if (*Next == '}') {
		Stream->Next = Next + 1;
	} else {
		Map = ml_error("ParseError", "Invalid value syntax at line %d in %s", Stream->LineNo, Stream->Source);
	}
	return Map;
}

static ml_value_t *parse_value(xe_stream_t *Stream) {
	const char *Next = Stream->Next;
	SKIP_WHITESPACE;
	switch (*Next) {
	case '<':
		Stream->Next = Next + 1;
		return parse_node(Stream);
	case '\"':
		Stream->Next = Next + 1;
		return parse_string(Stream);
	case '-': case '0' ... '9': case '.': {
		char *End;
		long Value = strtol(Next, &End, 10);
		if (End[0] == '.' || End[0] == 'e' || End[0] == 'E') {
			double Value = strtod(Next, &End);
			Stream->Next = End;
			return ml_real(Value);
		} else {
			Stream->Next = End;
			return ml_integer(Value);
		}
	}
	case '$': {
		++Next;
		for (;;) {
			char Delim = *Next;
			if (Delim <= ' ') break;
			if (Delim == ':') break;
			if (Delim == '|') break;
			if (Delim == '>') break;
			++Next;
		}
		int NameLength = (Next - Stream->Next) - 1;
		char *Name = snew(NameLength + 1);
		memcpy(Name, Stream->Next + 1, NameLength);
		Name[NameLength] = 0;
		Stream->Next = Next;
		xe_var_t *Var = new(xe_var_t);
		Var->Type = XEVarT;
		if (!NameLength) {
			Var->Name = MLNil;
		} else if (isalpha(Name[0])) {
			Var->Name = ml_string(Name, NameLength);
		} else {
			Var->Name = ml_integer(atoi(Name));
		}
		return (ml_value_t *)Var;
	}
	case '[':
		Stream->Next = Next + 1;
		return parse_list(Stream);
	case '{':
		Stream->Next = Next + 1;
		return parse_map(Stream);
	default:
		if (!strncmp(Next, "true", 4)) {
			Stream->Next = Next + 4;
			return (ml_value_t *)MLTrue;
		} else if (!strncmp(Next, "false", 5)) {
			Stream->Next = Next + 5;
			return (ml_value_t *)MLFalse;
		} else if (!strncmp(Next, "nil", 3)) {
			Stream->Next = Next + 3;
			return MLNil;
		} else {
			return ml_error("ParseError", "Invalid value syntax at line %d in %s", Stream->LineNo, Stream->Source);
		}
	}
}

static ml_value_t *parse_node(xe_stream_t *Stream) {
	int LineNo = Stream->LineNo;
	const char *Next = Stream->Next;
	for (;;) {
		char Delim = *Next;
		if (Delim <= ' ') break;
		if (Delim == ':') break;
		if (Delim == '|') break;
		if (Delim == '>') break;
		++Next;
	}
	int TagLength = Next - Stream->Next;
	if (Stream->Next[0] == '$') {
		--TagLength;
		char *Name = snew(TagLength + 1);
		memcpy(Name, Stream->Next + 1, TagLength);
		Name[TagLength] = 0;
		SKIP_WHITESPACE;
		Stream->Next = Next + 1;
		xe_var_t *Var = new(xe_var_t);
		Var->Type = XEVarT;
		if (!TagLength) {
			Var->Name = MLNil;
		} else if (isalpha(Name[0])) {
			Var->Name = ml_string(Name, TagLength);
		} else {
			Var->Name = ml_integer(atoi(Name));
		}
		return (ml_value_t *)Var;
	} else {
		ml_value_t *Tag;
		ml_value_t *Attributes = ml_map();
		ml_value_t *Content = ml_list();
		int Index = 1;
		if (Stream->Next[0] == '@') {
			Tag = ml_cstring("@");
			ml_map_insert(Attributes, ml_integer(Index++), ml_string(Stream->Next + 1, TagLength - 1));
		} else {
			Tag = ml_string(Stream->Next, TagLength);
		}
		for (;;) {
			SKIP_WHITESPACE;
			if (Next[0] != ':' && Next[0] != '|' && Next[0] != '>' && Next[0] != '?') {
				const char *Start = Next;
				for (;;) {
					if (!isalpha(*Next)) break;
					++Next;
				}
				ml_value_t *Name;
				if (Next != Start) {
					Name = ml_string(Start, Next - Start);
					SKIP_WHITESPACE;
					if (Next[0] != '=') return ml_error("ParseError", "Expected = at line %d in %s", Stream->LineNo, Stream->Source);
					++Next;
					SKIP_WHITESPACE;
				} else {
					Name = ml_integer(Index++);
				}
				Stream->Next = Next;
				ml_value_t *Value = parse_value(Stream);
				if (ml_is_error(Value)) return Value;
				ml_map_insert(Attributes, Name, Value);
				Next = Stream->Next;
			} else {
				break;
			}
		}
		if (Next[0] == ':') {
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			const char *End = ++Next;
			for (;;) {
				if (!End) {
					return ml_error("ParseError", "Unexpected end of input at line %d in %s", Stream->LineNo, Stream->Source);
				} else if (End[0] == 0) {
					ml_stringbuffer_write(Buffer, Next, End - Next);
					Next = Stream->read(Stream);
					if (!Next)
					++Stream->LineNo;
					End = Next;
				} else if (End[0] == '\\') {
					ml_stringbuffer_write(Buffer, Next, End - Next);
					End = Next = parse_escape(End, Buffer);
				} else if (End[0] == '<') {
					ml_stringbuffer_write(Buffer, Next, End - Next);
					if (Buffer->Length) ml_list_put(Content, ml_stringbuffer_get_value(Buffer));
					Stream->Next = End + 1;
					ml_value_t *Node = parse_node(Stream);
					if (ml_is_error(Node)) return Node;
					ml_list_put(Content, Node);
					End = Next = Stream->Next;
				} else if (End[0] == '>') {
					ml_stringbuffer_write(Buffer, Next, End - Next);
					if (Buffer->Length) ml_list_put(Content, ml_stringbuffer_get_value(Buffer));
					break;
				} else {
					++End;
				}
			}
			Stream->Next = End + 1;
		} else if (Next[0] == '|') {
			const char *End = ++Next;
			for (;;) {
				if (!End) {
					return ml_error("ParseError", "Unexpected end of input at line %d in %s", Stream->LineNo, Stream->Source);
				} else if (End[0] == 0) {
					End = Next = Stream->read(Stream);
					++Stream->LineNo;
				} else if (End[0] == '<') {
					Stream->Next = End + 1;
					ml_value_t *Node = parse_node(Stream);
					if (ml_is_error(Node)) return Node;
					ml_list_put(Content, Node);
					End = Next = Stream->Next;
				} else if (End[0] == '>') {
					break;
				} else if (End[0] <= ' ') {
					++End;
				} else {
					return ml_error("ParseError", "Non whitespace character in | node at line %d in %s", Stream->LineNo, Stream->Source);
				}
			}
			Stream->Next = End + 1;
		} else {
			Stream->Next = Next + 1;
		}
		xe_node_t *Node = new(xe_node_t);
		Node->Type = XENodeT;
		Node->Tag = Tag;
		Node->Attributes = Attributes;
		Node->Content = Content;
		Node->Source.Name = Stream->Source;
		Node->Source.Line = LineNo;
		return (ml_value_t *)Node;
	}
}

static const char *string_read(xe_stream_t *Stream) {
	const char *Next = (const char *)Stream->Data, *End = Next;
	if (!Next) return 0;
	while (End[0] >= ' ') ++End;
	int Length = (End - Next) + 1;
	char *Line = snew(Length + 1);
	memcpy(Line, Next, Length);
	Line[Length] = 0;
	if (End[0]) {
		Stream->Data = (void *)(End + 1);
	} else {
		Stream->Data = 0;
	}
	return Line;
}

ML_FUNCTION(XEParse) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	xe_stream_t Stream[1];
	Stream->Data = (void *)ml_string_value(Args[0]);
	Stream->LineNo = 1;
	Stream->Source = "string";
	Stream->read = string_read;
	const char *Next = string_read(Stream);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *End = Next;
	ml_value_t *Content = ml_list();
	while (End) {
		if (End[0] == 0) {
			ml_stringbuffer_write(Buffer, Next, End - Next);
			End = Next = Stream->read(Stream);
			++Stream->LineNo;
		} else if (End[0] == '\\') {
			ml_stringbuffer_write(Buffer, Next, End - Next);
			End = Next = parse_escape(End, Buffer);
		} else if (End[0] == '<') {
			ml_stringbuffer_write(Buffer, Next, End - Next);
			if (Buffer->Length) ml_list_put(Content, ml_stringbuffer_get_value(Buffer));
			Stream->Next = End + 1;
			ml_value_t *Node = parse_node(Stream);
			if (ml_is_error(Node)) return Node;
			ml_list_put(Content, Node);
			End = Next = Stream->Next;
		} else {
			++End;
		}
	}
	if (Buffer->Length) ml_list_put(Content, ml_stringbuffer_get_value(Buffer));
	return Content;
}

void ml_xe_init(stringmap_t *Globals) {
#include "ml_xe_init.c"
	if (Globals) {
		stringmap_insert(Globals, "xe", ml_module("xe",
			"node", XENodeT,
			"var", XEVarT,
			"parse", XEParse,
		NULL));
	}
}

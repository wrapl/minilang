#include "ml_xml.h"
#include "ml_macros.h"
#include <string.h>
#include <expat.h>

#undef ML_CATEGORY
#define ML_CATEGORY "xml"

#define ML_XML_STACK_SIZE 10

typedef struct {
	ml_type_t *Type;
	ml_value_t *Tag;
	ml_value_t *Attributes;
	ml_value_t *Children;
} ml_xml_node_t;

ML_TYPE(MLXmlT, (), "xml");
//@xml

ML_METHOD("tag", MLXmlT) {
//<Xml
//>method
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return Node->Tag;
}

ML_METHOD("attributes", MLXmlT) {
//<Xml
//>map
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return Node->Attributes;
}

ML_METHOD("children", MLXmlT) {
//<Xml
//>list
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return Node->Children;
}

static void ml_xml_escape_string(ml_stringbuffer_t *Buffer, const char *String) {
	for (; *String; String++) {
		switch (*String) {
		case '<':
			ml_stringbuffer_write(Buffer, "&lt;", 4);
			break;
		case '>':
			ml_stringbuffer_write(Buffer, "&gt;", 4);
			break;
		case '&':
			ml_stringbuffer_write(Buffer, "&amp;", 5);
			break;
		case '\"':
			ml_stringbuffer_write(Buffer, "&quot;", 6);
			break;
		case '\'':
			ml_stringbuffer_write(Buffer, "&apos;", 6);
			break;
		default:
			ml_stringbuffer_write(Buffer, String, 1);
			break;
		}
	}
}

static ml_value_t *ml_xml_node_append(ml_stringbuffer_t *Buffer, ml_xml_node_t *Node) {
	ml_stringbuffer_printf(Buffer, "<%s", ml_method_name(Node->Tag));
	ML_MAP_FOREACH(Node->Attributes, Iter) {
		if (!ml_is(Iter->Key, MLStringT)) {
			return ml_error("XMLError", "Attribute keys must be strings");
		}
		if (!ml_is(Iter->Value, MLStringT)) {
			return ml_error("XMLError", "Attribute values must be strings");
		}
		ml_stringbuffer_printf(Buffer, " %s=\"", ml_string_value(Iter->Key));
		ml_xml_escape_string(Buffer, ml_string_value(Iter->Value));
		ml_stringbuffer_write(Buffer, "\"", 1);
	}
	if (ml_list_length(Node->Children)) {
		ml_stringbuffer_write(Buffer, ">", 1);
		ML_LIST_FOREACH(Node->Children, Iter) {
			if (ml_is(Iter->Value, MLStringT)) {
				ml_xml_escape_string(Buffer, ml_string_value(Iter->Value));
			} else if (ml_is(Iter->Value, MLXmlT)) {
				ml_value_t *Error = ml_xml_node_append(Buffer, (ml_xml_node_t *)Iter->Value);
				if (Error) return Error;
			} else {
				return ml_error("XMLError", "Children must be strings or nodes");
			}
		}
		ml_stringbuffer_printf(Buffer, "</%s>", ml_string_value(Node->Tag));
	} else {
		ml_stringbuffer_write(Buffer, "/>", 2);
	}
	return NULL;
}

ML_METHOD("append", MLStringBufferT, MLXmlT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[1];
	ml_value_t *Error = ml_xml_node_append(Buffer, Node);
	return Error ?: Args[0];
}

ML_METHOD("append", MLStringBufferT, MLXmlT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[1];
	ml_value_t *Error = ml_xml_node_append(Buffer, Node);
	return Error ?: MLSome;
}

typedef struct xml_stack_t xml_stack_t;

struct xml_stack_t {
	ml_xml_node_t *Nodes[ML_XML_STACK_SIZE];
	xml_stack_t *Prev;
	int Index;
};

typedef struct {
	void (*Callback)(void *Data, ml_value_t *Value);
	void *Data;
	ml_xml_node_t *Node;
	xml_stack_t *Stack;
	ml_stringbuffer_t Buffer[1];
	xml_stack_t Stack0;
} xml_decoder_t;

static void xml_start_element(xml_decoder_t *Decoder, const XML_Char *Name, const XML_Char **Attrs) {
	if (Decoder->Buffer->Length) {
		ml_value_t *Text = ml_stringbuffer_get_value(Decoder->Buffer);
		if (Decoder->Node) ml_list_put(Decoder->Node->Children, Text);
	}
	xml_stack_t *Stack = Decoder->Stack;
	if (Stack->Index == ML_XML_STACK_SIZE) {
		xml_stack_t *NewStack = new(xml_stack_t);
		NewStack->Prev = Stack;
		Stack = Decoder->Stack = NewStack;
	}
	Stack->Nodes[Stack->Index] = Decoder->Node;
	++Stack->Index;
	ml_xml_node_t *Node = Decoder->Node = new(ml_xml_node_t);
	Node->Type = MLXmlT;
	Node->Tag = ml_method(GC_strdup(Name));
	Node->Children = ml_list();
	Node->Attributes = ml_map();
	for (const XML_Char **Attr = Attrs; Attr[0]; Attr += 2) {
		ml_map_insert(Node->Attributes, ml_cstring(GC_strdup(Attr[0])), ml_cstring(GC_strdup(Attr[1])));
	}
	Decoder->Node = Node;
}

static void xml_end_element(xml_decoder_t *Decoder, const XML_Char *Name) {
	if (Decoder->Buffer->Length) {
		ml_list_put(Decoder->Node->Children, ml_stringbuffer_get_value(Decoder->Buffer));
	}
	xml_stack_t *Stack = Decoder->Stack;
	if (Stack->Index == 0) {
		Stack = Decoder->Stack = Stack->Prev;
	}
	ml_xml_node_t *Node = Decoder->Node;
	--Stack->Index;
	Decoder->Node = Stack->Nodes[Stack->Index];
	Stack->Nodes[Stack->Index] = NULL;
	if (Decoder->Node) {
		ml_list_put(Decoder->Node->Children, (ml_value_t *)Node);
	} else {
		Decoder->Callback(Decoder->Data, (ml_value_t *)Node);
	}
}

static void xml_character_data(xml_decoder_t *Decoder, const XML_Char *String, int Length) {
	ml_stringbuffer_write(Decoder->Buffer, String, Length);
}

static void xml_skipped_entity(xml_decoder_t *Decoder, const XML_Char *EntityName, int IsParameterEntity) {
	ml_stringbuffer_write(Decoder->Buffer, EntityName, strlen(EntityName));
}

static void xml_default(xml_decoder_t *Decoder, const XML_Char *String, int Length) {
	ml_stringbuffer_write(Decoder->Buffer, String, Length);
}

static void ml_free(void *Ptr) {
}

static void xml_decode_callback(ml_value_t **Result, ml_value_t *Value) {
	Result[0] = Value;
}

ML_METHODV(MLXmlT, MLMethodT) {
//@xml
//<Tag
//<Children...:string|xml
//<Attributes?:names|map
//>xml
	ml_xml_node_t *Node = new(ml_xml_node_t);
	Node->Type = MLXmlT;
	Node->Tag = Args[0];
	Node->Children = ml_list();
	Node->Attributes = ml_map();
	for (int I = 1; I < Count; ++I) {
		if (ml_is(Args[I], MLStringT)) {
			ml_list_put(Node->Children, Args[I]);
		} else if (ml_is(Args[I], MLXmlT)) {
			ml_list_put(Node->Children, Args[I]);
		} else if (ml_is(Args[I], MLNamesT)) {
			ML_NAMES_FOREACH(Args[I], Iter) {
				++I;
				ML_CHECK_ARG_TYPE(I, MLStringT);
				ml_map_insert(Node->Attributes, Iter->Value, Args[I]);
			}
			break;
		} else if (ml_is(Args[I], MLMapT)) {
			ML_MAP_FOREACH(Args[I], Iter) {
				if (!ml_is(Iter->Key, MLStringT)) {
					return ml_error("XMLError", "Attribute keys must be strings");
				}
				if (!ml_is(Iter->Value, MLStringT)) {
					return ml_error("XMLError", "Attribute values must be strings");
				}
				ml_map_insert(Node->Attributes, Iter->Key, Iter->Value);
			}
		} else {
			return ml_error("XMLError", "Unsupported value for xml node");
		}
	}
	return (ml_value_t *)Node;
}

ML_METHOD(MLXmlT, MLStringT) {
//@xml
//<Xml
//>xml
	ml_value_t *Result = NULL;
	xml_decoder_t Decoder = {0,};
	Decoder.Callback = (void *)xml_decode_callback;
	Decoder.Data = &Result;
	Decoder.Stack = &Decoder.Stack0;
	XML_Memory_Handling_Suite Suite = {GC_malloc, GC_realloc, ml_free};
	XML_Parser Handle = XML_ParserCreate_MM(NULL, &Suite, NULL);
	XML_SetUserData(Handle, &Decoder);
	XML_SetElementHandler(Handle, (void *)xml_start_element, (void *)xml_end_element);
	XML_SetCharacterDataHandler(Handle, (void *)xml_character_data);
	XML_SetSkippedEntityHandler(Handle, (void *)xml_skipped_entity);
	XML_SetDefaultHandler(Handle, (void *)xml_default);
	const char *Text = ml_string_value(Args[0]);
	size_t Length = ml_string_length(Args[0]);
	if (XML_Parse(Handle, Text, Length, 1) == XML_STATUS_ERROR) {
		enum XML_Error Error = XML_GetErrorCode(Handle);
		return ml_error("XMLError", "%s", XML_ErrorString(Error));
	}
	return Result ?: ml_error("XMLError", "Incomplete XML");
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Callback;
	ml_value_t *Args[1];
	XML_Parser Handle;
	xml_decoder_t Decoder[1];
} ml_xml_decoder_t;

extern ml_type_t MLXmlDecoderT[];

static void ml_xml_decode_callback(ml_xml_decoder_t *Decoder, ml_value_t *Value) {
	Decoder->Args[0] = Value;
	ml_call((ml_state_t *)Decoder, Decoder->Callback, 1, Decoder->Args);
}

static void ml_xml_decoder_run(ml_state_t *State, ml_value_t *Value) {
}

ML_FUNCTIONX(XmlDecoder) {
//@xml::decoder
//<Callback
//>xml::decoder
	ML_CHECKX_ARG_COUNT(1);
	ml_xml_decoder_t *Decoder = new(ml_xml_decoder_t);
	Decoder->Base.Type = MLXmlDecoderT;
	Decoder->Base.Context = Caller->Context;
	Decoder->Base.run = ml_xml_decoder_run;
	Decoder->Callback = Args[0];
	Decoder->Decoder->Callback = (void *)ml_xml_decode_callback;
	Decoder->Decoder->Data = Decoder;
	Decoder->Decoder->Stack = &Decoder->Decoder->Stack0;
	XML_Memory_Handling_Suite Suite = {GC_malloc, GC_realloc, ml_free};
	Decoder->Handle = XML_ParserCreate_MM(NULL, &Suite, NULL);
	XML_SetUserData(Decoder->Handle, Decoder->Decoder);
	XML_SetElementHandler(Decoder->Handle, (void *)xml_start_element, (void *)xml_end_element);
	XML_SetCharacterDataHandler(Decoder->Handle, (void *)xml_character_data);
	XML_SetSkippedEntityHandler(Decoder->Handle, (void *)xml_skipped_entity);
	XML_SetDefaultHandler(Decoder->Handle, (void *)xml_default);
	ML_RETURN(Decoder);
}

ML_TYPE(MLXmlDecoderT, (), "xml-decoder",
//@xml::decoder
	.Constructor = (ml_value_t *)XmlDecoder
);

ML_METHOD("decode", MLXmlDecoderT, MLAddressT) {
//<Decoder
//<Xml
//>Decoder
	ml_xml_decoder_t *Decoder = (ml_xml_decoder_t *)Args[0];
	const char *Text = ml_address_value(Args[1]);
	size_t Length = ml_address_length(Args[1]);
	if (XML_Parse(Decoder->Handle, Text, Length, 0) == XML_STATUS_ERROR) {
		enum XML_Error Error = XML_GetErrorCode(Decoder->Handle);
		return ml_error("XMLError", "%s", XML_ErrorString(Error));
	}
	return Args[0];
}

ML_METHOD("decode", MLXmlDecoderT, MLAddressT, MLIntegerT) {
//<Decoder
//<Xml
//<Size
//>Decoder
	ml_xml_decoder_t *Decoder = (ml_xml_decoder_t *)Args[0];
	const char *Text = ml_address_value(Args[1]);
	size_t Length = ml_integer_value(Args[2]);
	if (Length > ml_address_length(Args[1])) {
		return ml_error("ValueError", "Length larger than buffer");
	}
	if (XML_Parse(Decoder->Handle, Text, Length, 0) == XML_STATUS_ERROR) {
		enum XML_Error Error = XML_GetErrorCode(Decoder->Handle);
		return ml_error("XMLError", "%s", XML_ErrorString(Error));
	}
	return Args[0];
}

ML_METHOD("finish", MLXmlDecoderT) {
//<Decoder
//>Decoder
	ml_xml_decoder_t *Decoder = (ml_xml_decoder_t *)Args[0];
	if (XML_Parse(Decoder->Handle, "", 0, 0) == XML_STATUS_ERROR) {
		enum XML_Error Error = XML_GetErrorCode(Decoder->Handle);
		return ml_error("XMLError", "%s", XML_ErrorString(Error));
	}
	return Args[0];
}

void ml_xml_init(stringmap_t *Globals) {
#include "ml_xml_init.c"
	stringmap_insert(MLXmlT->Exports, "decoder", MLXmlDecoderT);
	if (Globals) {
		stringmap_insert(Globals, "xml", MLXmlT);
	}
}

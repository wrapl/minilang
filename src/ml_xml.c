#include "ml_xml.h"
#include "ml_macros.h"
#include "ml_stream.h"
#include <string.h>
#include <expat.h>
#ifdef ML_TRE
#include <tre/regex.h>
#else
#include <regex.h>
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "xml"

#define ML_XML_STACK_SIZE 32

struct ml_xml_node_t {
	ml_string_t Base;
	ml_xml_element_t *Parent;
	ml_xml_node_t *Next, *Prev;
	size_t Index;
};

ML_TYPE(MLXmlT, (), "xml");
// An XML node.

ml_value_t *ml_xml_node_parent(ml_value_t *Value) {
	return (ml_value_t *)((ml_xml_node_t *)Value)->Parent;
}

ml_value_t *ml_xml_node_next(ml_value_t *Value) {
	return (ml_value_t *)((ml_xml_node_t *)Value)->Next;
}

ml_value_t *ml_xml_node_prev(ml_value_t *Value) {
	return (ml_value_t *)((ml_xml_node_t *)Value)->Prev;
}

ML_METHOD("parent", MLXmlT) {
//<Xml
//>xml|nil
// Returnst the parent of :mini:`Xml` or :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return (ml_value_t *)Node->Parent ?: MLNil;
}

ML_METHOD("prev", MLXmlT) {
//<Xml
//>xml|nil
// Returnst the previous sibling of :mini:`Xml` or :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return (ml_value_t *)Node->Prev ?: MLNil;
}

ML_METHOD("next", MLXmlT) {
//<Xml
//>xml|nil
// Returns the next sibling of :mini:`Xml` or :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return (ml_value_t *)Node->Next ?: MLNil;
}

static void ML_TYPED_FN(ml_iter_next, MLXmlT, ml_state_t *Caller, ml_xml_node_t *Node) {
	ML_RETURN((ml_value_t *)Node->Next ?: MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLXmlT, ml_state_t *Caller, ml_xml_node_t *Node) {
	ML_RETURN(ml_integer(Node->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLXmlT, ml_state_t *Caller, ml_xml_node_t *Node) {
	ML_RETURN(Node);
}

ML_FUNCTION(MLXmlEscape) {
//@xml::escape
//<String:string
//>string
// Escapes characters in :mini:`String`.
//$- import: xml("fmt/xml")
//$= xml::escape("\'1 + 2 > 3 & 2 < 4\'")
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *S = ml_string_value(Args[0]);
	for (int I = ml_string_length(Args[0]); --I >= 0; ++S) {
		switch (*S) {
		case '&':
			ml_stringbuffer_write(Buffer, "&amp;", strlen("&amp;"));
			break;
		case '<':
			ml_stringbuffer_write(Buffer, "&lt;", strlen("&lt;"));
			break;
		case '>':
			ml_stringbuffer_write(Buffer, "&gt;", strlen("&gt;"));
			break;
		default:
			ml_stringbuffer_write(Buffer, S, 1);
			break;
		}
	}
	return ml_stringbuffer_get_value(Buffer);
}

ML_TYPE(MLXmlTextT, (MLXmlT, MLStringT), "xml::text");
// A XML text node.

ml_xml_node_t *ml_xml_text(const char *Content, int Length) {
	ml_xml_node_t *Text = new(ml_xml_node_t);
	Text->Base.Type = MLXmlTextT;
	Text->Base.Length = Length < 0 ? strlen(Content) : Length;
	Text->Base.Value = Content;
	return Text;
}

ml_xml_node_t *ml_xml_text_copy(const char *Content, int Length) {
	if (Length < 0) Length = strlen(Content);
	ml_xml_node_t *Text = new(ml_xml_node_t);
	Text->Base.Type = MLXmlTextT;
	Text->Base.Length = Length;
	char *Copy = snew(Length + 1);
	memcpy(Copy, Content, Length);
	Copy[Length] = 0;
	Text->Base.Value = Copy;
	return Text;
}

ML_METHOD("text", MLXmlTextT) {
//<Xml
//>string
// Returns the text content of :mini:`Xml`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return ml_string(Node->Base.Value, Node->Base.Length);
}

static stringmap_t MLXmlTags[1] = {STRINGMAP_INIT};

struct ml_xml_element_t {
	ml_xml_node_t Base;
	ml_value_t *Attributes;
	ml_xml_node_t *Head, *Tail;
};

ML_TYPE(MLXmlElementT, (MLXmlT, MLSequenceT), "xml::element");
// An XML element node.

ml_xml_element_t *ml_xml_element(const char *Tag) {
	ml_xml_element_t *Element = new(ml_xml_element_t);
	Element->Base.Base.Type = MLXmlElementT;
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(MLXmlTags, Tag);
	if (!Slot[0]) Slot[0] = ml_string(GC_strdup(Tag), -1);
	Element->Base.Base.Value = (const char *)Slot[0];
	Element->Attributes = ml_map();
	return Element;
}

ml_value_t *ml_xml_element_tag(ml_value_t *Value) {
	return (ml_value_t *)((ml_xml_element_t *)Value)->Base.Base.Value;
}

ml_value_t *ml_xml_element_attributes(ml_value_t *Value) {
	return ((ml_xml_element_t *)Value)->Attributes;
}

size_t ml_xml_element_length(ml_value_t *Value) {
	return ((ml_xml_element_t *)Value)->Base.Base.Length;
}

ml_value_t *ml_xml_element_head(ml_value_t *Value) {
	return (ml_value_t *)((ml_xml_element_t *)Value)->Head;
}

void ml_xml_element_put(ml_xml_element_t *Parent, ml_xml_node_t *Child) {
	if (Child->Parent) {
		ml_xml_element_t *OldParent = Child->Parent;
		if (Child->Prev) {
			Child->Prev->Next = Child->Next;
		} else {
			OldParent->Head = Child->Next;
		}
		if (Child->Next) {
			Child->Next->Prev = Child->Prev;
		} else {
			OldParent->Tail = Child->Prev;
		}
		size_t Index = Child->Index;
		for (ml_xml_node_t *Node = Child->Next; Node; Node = Node->Next) {
			Node->Index = Index++;
		}
		--OldParent->Base.Base.Length;
	}
	Child->Index = ++Parent->Base.Base.Length;
	Child->Parent = Parent;
	if (Parent->Tail) {
		Parent->Tail->Next = Child;
	} else {
		Parent->Head = Child;
	}
	Child->Prev = Parent->Tail;
	Child->Next = NULL;
	Parent->Tail = Child;
}

static ml_value_t *ml_xml_element_put_general(ml_xml_element_t *Element, ml_value_t *Value, ml_stringbuffer_t *Buffer) {
	if (Value == MLNil) {
		return NULL;
	} else if (ml_is(Value, MLStringT)) {
		ml_stringbuffer_write(Buffer, ml_string_value(Value), ml_string_length(Value));
		return NULL;
	} else if (ml_is(Value, MLXmlT)) {
		if (Buffer->Length) {
			ml_xml_node_t *Text = new(ml_xml_node_t);
			Text->Base.Type = MLXmlTextT;
			Text->Base.Length = Buffer->Length;
			Text->Base.Value = ml_stringbuffer_get_string(Buffer);
			ml_xml_element_put(Element, Text);
		}
		ml_xml_element_put(Element, (ml_xml_node_t *)Value);
		return NULL;
	} else if (ml_is(Value, MLListT)) {
		ML_LIST_FOREACH(Value, Iter) {
			ml_value_t *Error = ml_xml_element_put_general(Element, Iter->Value, Buffer);
			if (Error) return Error;
		}
		return NULL;
	} else if (ml_is(Value, MLMapT)) {
		ML_MAP_FOREACH(Value, Iter) {
			if (!ml_is(Iter->Key, MLStringT)) {
				return ml_error("XMLError", "Attribute keys must be strings");
			}
			if (!ml_is(Iter->Value, MLStringT)) {
				return ml_error("XMLError", "Attribute values must be strings");
			}
			ml_map_insert(Element->Attributes, Iter->Key, Iter->Value);
		}
		return NULL;
	} else {
		return ml_error("XMLError", "Unsupported type %s for XML element", ml_typeof(Value)->Name);
	}
}

ML_METHODV(MLXmlElementT, MLStringT) {
//@xml::element
//<Tag
//<Arg/1,...,Arg/n
//>xml::element
// Returns a new XML node with tag :mini:`Tag` and optional children and attributes depending on the types of each :mini:`Arg/i`:
//
// * :mini:`string`: added as child text node. Consecutive strings are added a single node.
// * :mini:`xml`: added as a child node.
// * :mini:`list`: each value must be a :mini:`string` or :mini:`xml` and is added as above.
// * :mini:`map`: keys and values must be strings, set as attributes.
// * :mini:`name is value`: values must be strings, set as attributes.
//$- import: xml("fmt/xml")
//$= xml::element("test", "Text", type is "example")
	ml_xml_element_t *Element = new(ml_xml_element_t);
	Element->Base.Base.Type = MLXmlElementT;
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(MLXmlTags, ml_string_value(Args[0]));
	if (!Slot[0]) Slot[0] = Args[0];
	Element->Base.Base.Value = (const char *)Slot[0];
	Element->Attributes = ml_map();
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (int I = 1; I < Count; ++I) {
		ml_value_t *Value = Args[I];
		if (ml_is(Value, MLNamesT)) {
			ML_NAMES_CHECK_ARG_COUNT(I);
			ML_NAMES_FOREACH(Value, Iter) {
				++I;
				ML_CHECK_ARG_TYPE(I, MLStringT);
				ml_map_insert(Element->Attributes, Iter->Value, Value);
			}
			break;
		} else {
			ml_value_t *Error = ml_xml_element_put_general(Element, Value, Buffer);
			if (Error) return Error;
		}
	}
	if (Buffer->Length) {
		ml_xml_node_t *Text = new(ml_xml_node_t);
		Text->Base.Type = MLXmlTextT;
		Text->Base.Length = Buffer->Length;
		Text->Base.Value = ml_stringbuffer_get_string(Buffer);
		ml_xml_element_put(Element, Text);
	}
	return (ml_value_t *)Element;
}

ML_METHOD("tag", MLXmlElementT) {
//<Xml
//>string
// Returns the tag of :mini:`Xml`.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	return (ml_value_t *)Element->Base.Base.Value;
}

ML_METHOD("attributes", MLXmlElementT) {
//<Xml
//>map
// Returns the attributes of :mini:`Xml`.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	return Element->Attributes;
}

static void ml_xml_element_text(ml_xml_element_t *Element, ml_stringbuffer_t *Buffer, const char *Sep, int SepLen) {
	for (ml_xml_node_t *Node = Element->Head; Node; Node = Node->Next) {
		if (Node->Base.Type == MLXmlTextT) {
			if (Buffer->Length && SepLen) ml_stringbuffer_write(Buffer, Sep, SepLen);
			ml_stringbuffer_write(Buffer, Node->Base.Value, Node->Base.Length);
		} else if (Node->Base.Type == MLXmlElementT) {
			ml_xml_element_text((ml_xml_element_t *)Node, Buffer, Sep, SepLen);
		}
	}
}

ML_METHOD("text", MLXmlElementT) {
//<Xml
//>string
// Returns the (recursive) text content of :mini:`Xml`.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_xml_element_text(Element, Buffer, "", 0);
	return ml_stringbuffer_get_value(Buffer);
}

ML_METHOD("text", MLXmlElementT, MLStringT) {
//<Xml
//<Sep
//>string
// Returns the (recursive) text content of :mini:`Xml`, adding :mini:`Sep` between the contents of adjacent nodes.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_xml_element_text(Element, Buffer, ml_string_value(Args[1]), ml_string_length(Args[1]));
	return ml_stringbuffer_get_value(Buffer);
}

ML_METHOD("set", MLXmlElementT, MLStringT, MLStringT) {
//<Xml
//<Attribute
//<Value
//>xml
// Sets the value of attribute :mini:`Attribute` in :mini:`Xml` to :mini:`Value` and returns :mini:`Xml`.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_map_insert(Element->Attributes, Args[1], Args[2]);
	return (ml_value_t *)Element;
}

ML_METHOD_DECL(PutMethod, "put");

ML_METHODVX("put", MLXmlElementT, MLStringT) {
//<Parent
//<String
//>xml
// Adds a new text node containing :mini:`String` to :mini:`Parent`.
	ml_xml_element_t *Parent = (ml_xml_element_t *)Args[0];
	ml_xml_node_t *Tail = Parent->Tail;
	if (Tail && Tail->Base.Type == MLXmlTextT) {
		ml_xml_node_t *Text = new(ml_xml_node_t);
		Text->Base.Type = MLXmlTextT;
		size_t Length1 = ml_string_length(Args[1]);
		size_t Length = Text->Base.Length = Tail->Base.Length + Length1;
		char *Value = snew(Length + 1);
		memcpy(Value, Tail->Base.Value, Tail->Base.Length);
		memcpy(Value + Tail->Base.Length, ml_string_value(Args[1]), Length1);
		Value[Length] = 0;
		Text->Base.Value = Value;
		Text->Index = Tail->Index;
		Text->Prev = Tail->Prev;
		Text->Parent = Tail->Parent;
		if (Text->Prev) {
			Text->Prev->Next = Text;
		} else {
			Parent->Head = Text;
		}
		Parent->Tail = Text;
	} else {
		ml_xml_node_t *Text = new(ml_xml_node_t);
		Text->Base.Type = MLXmlTextT;
		Text->Base.Length = ml_string_length(Args[1]);
		Text->Base.Value = ml_string_value(Args[1]);
		ml_xml_element_put(Parent, Text);
	}
	if (Count > 2) {
		Args[1] = (ml_value_t *)Parent;
		return ml_call(Caller, PutMethod, Count - 1, Args + 1);
	}
	ML_RETURN(Parent);
}

ML_METHODVX("put", MLXmlElementT, MLXmlElementT) {
//<Parent
//<Child
//>xml
// Adds :mini:`Child` to :mini:`Parent`.
	ml_xml_element_t *Parent = (ml_xml_element_t *)Args[0];
	ml_xml_node_t *Child = (ml_xml_node_t *)Args[1];
	ml_xml_element_put(Parent, Child);
	if (Count > 2) {
		Args[1] = (ml_value_t *)Parent;
		return ml_call(Caller, PutMethod, Count - 1, Args + 1);
	}
	ML_RETURN(Parent);
}

typedef struct {
	ml_state_t Base;
	ml_xml_element_t *Element;
	ml_value_t *Iter;
	ml_stringbuffer_t Buffer[1];
} ml_xml_grow_state_t;

static void ml_xml_grow_state_next(ml_xml_grow_state_t *State, ml_value_t *Value);

static void ml_xml_grow_state_value(ml_xml_grow_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_stringbuffer_t *Buffer = State->Buffer;
	ml_xml_element_t *Element = State->Element;
	if (ml_is(Value, MLStringT)) {
		ml_stringbuffer_write(Buffer, ml_string_value(Value), ml_string_length(Value));
	} else if (ml_is(Value, MLXmlElementT)) {
		if (Buffer->Length) {
			ml_xml_node_t *Text = new(ml_xml_node_t);
			Text->Base.Type = MLXmlTextT;
			Text->Base.Length = Buffer->Length;
			Text->Base.Value = ml_stringbuffer_get_string(Buffer);
			ml_xml_element_put(Element, Text);
		}
		ml_xml_element_put(Element, (ml_xml_node_t *)Value);
	} else if (ml_is(Value, MLMapT)) {
		ML_MAP_FOREACH(Value, Iter) {
			if (!ml_is(Iter->Key, MLStringT)) {
				ML_ERROR("XMLError", "Attribute keys must be strings");
			}
			if (!ml_is(Iter->Value, MLStringT)) {
				ML_ERROR("XMLError", "Attribute values must be strings");
			}
			ml_map_insert(Element->Attributes, Iter->Key, Iter->Value);
		}
	} else {
		ML_ERROR("XMLError", "Unsupported value for XML element");
	}
	State->Base.run = (ml_state_fn)ml_xml_grow_state_next;
	ml_iter_next((ml_state_t *)State, State->Iter);
}

static void ml_xml_grow_state_next(ml_xml_grow_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (Value == MLNil) {
		ml_stringbuffer_t *Buffer = State->Buffer;
		ml_xml_element_t *Element = State->Element;
		if (Buffer->Length) {
			ml_xml_node_t *Text = new(ml_xml_node_t);
			Text->Base.Type = MLXmlTextT;
			Text->Base.Length = Buffer->Length;
			Text->Base.Value = ml_stringbuffer_get_string(Buffer);
			ml_xml_element_put(Element, Text);
		}
		ML_RETURN(Element);
	}
	State->Base.run = (ml_state_fn)ml_xml_grow_state_value;
	ml_iter_value((ml_state_t *)State, State->Iter = Value);
}

ML_METHODVX("grow", MLXmlElementT, MLSequenceT) {
//<Parent
//<Children
//>xml
// Adds each node generated by :mini:`Children` to :mini:`Parent` and returns :mini:`Parent`.
	ml_xml_grow_state_t *State = new(ml_xml_grow_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_xml_grow_state_next;
	State->Element = (ml_xml_element_t *)Args[0];
	State->Buffer[0] = ML_STRINGBUFFER_INIT;
	return ml_iterate((ml_state_t *)State, ml_chained(Count - 1, Args + 1));
}

static void ML_TYPED_FN(ml_iterate, MLXmlElementT, ml_state_t *Caller, ml_xml_element_t *Node) {
	ML_RETURN((ml_value_t *)Node->Head ?: MLNil);
}

extern ml_value_t *IndexMethod;

ML_METHOD("[]", MLXmlElementT, MLIntegerT) {
//<Parent
//<Index
//>xml|nil
// Returns the :mini:`Index`-th child of :mini:`Parent` or :mini:`nil`.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index <= 0) Index += Element->Base.Base.Length + 1;
	--Index;
	if (Index < 0 || Index >= Element->Base.Base.Length) return MLNil;
	ml_xml_node_t *Child = Element->Head;
	while (--Index >= 0) Child = Child->Next;
	return (ml_value_t *)Child;
}

ML_METHODX("[]", MLXmlElementT, MLStringT) {
//<Parent
//<Attribute
//>string|nil
// Returns the value of the :mini:`Attribute` attribute of :mini:`Parent`.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	Args[0] = Element->Attributes;
	return ml_call(Caller, IndexMethod, 2, Args);
}

ML_METHODX("::", MLXmlElementT, MLStringT) {
//<Parent
//<Attribute
//>string|nil
// Returns the value of the :mini:`Attribute` attribute of :mini:`Parent`.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	Args[0] = Element->Attributes;
	return ml_call(Caller, IndexMethod, 2, Args);
}

extern ml_type_t MLDoubledT[];
extern ml_type_t MLChainedT[];

#ifdef ML_GENERICS

ML_GENERIC_TYPE(MLXmlSequenceT, MLSequenceT, MLIntegerT, MLXmlT);
ML_GENERIC_TYPE(MLXmlDoubledT, MLDoubledT, MLIntegerT, MLXmlT);
ML_GENERIC_TYPE(MLXmlChainedT, MLChainedT, MLIntegerT, MLXmlT);

#else

#define MLXmlSequenceT MLSequenceT
#define MLXmlDoubledT MLDoubledT
#define MLXmlChainedT MLChainedT

#endif

typedef struct {
	ml_type_t *Type;
	const char *Tag;
	ml_value_t *Attributes;
} ml_xml_filter_t;

static void ml_xml_filter_call(ml_state_t *Caller, ml_xml_filter_t *Filter, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLXmlElementT);
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	if (Filter->Tag && Filter->Tag != Element->Base.Base.Value) ML_RETURN(MLNil);
	if (Filter->Attributes) ML_MAP_FOREACH(Filter->Attributes, Iter) {
		ml_value_t *Value = ml_map_search(Element->Attributes, Iter->Key);
		if (Iter->Value == MLNil) {
			if (Value != MLNil) ML_RETURN(MLNil);
		} else {
			if (Value == MLNil) ML_RETURN(MLNil);
			if (strcmp(ml_string_value(Iter->Value), ml_string_value(Value))) ML_RETURN(MLNil);
		}
	}
	ML_RETURN(Element);
}

ML_TYPE(MLXmlFilterT, (MLFunctionT), "xml::filter",
// An XML filter.
	.call = (void *)ml_xml_filter_call
);

ML_METHODV(MLXmlFilterT, MLNamesT) {
//@xml::filter
//<Attr,Value
//>xml::filter
// Returns an XML filter that checks if a node has attributes :mini:`Attr/i = Value/i`.
	ML_NAMES_CHECK_ARG_COUNT(0);
	ml_xml_filter_t *Filter = new(ml_xml_filter_t);
	Filter->Type = MLXmlFilterT;
	ml_value_t *Attributes = Filter->Attributes = ml_map();
	int I = 1;
	ML_NAMES_FOREACH(Args[0], Iter) {
		if (Args[I] != MLNil) ML_CHECK_ARG_TYPE(I, MLStringT);
		ml_map_insert(Attributes, Iter->Value, Args[I++]);
	}
	return (ml_value_t *)Filter;
}

ML_METHODV(MLXmlFilterT, MLStringT, MLNamesT) {
//@xml::filter
//<Tag
//<Attr,Value
//>xml::filter
// Returns an XML filter that checks if a node has tag :mini:`Tag` and attributes :mini:`Attr/i = Value/i`.
	ML_NAMES_CHECK_ARG_COUNT(1);
	ml_xml_filter_t *Filter = new(ml_xml_filter_t);
	Filter->Type = MLXmlFilterT;
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(MLXmlTags, ml_string_value(Args[0]));
	if (!Slot[0]) Slot[0] = Args[0];
	Filter->Tag = (const char *)Slot[0];
	ml_value_t *Attributes = Filter->Attributes = ml_map();
	int I = 2;
	ML_NAMES_FOREACH(Args[1], Iter) {
		if (Args[I] != MLNil) ML_CHECK_ARG_TYPE(I, MLStringT);
		ml_map_insert(Attributes, Iter->Value, Args[I++]);
	}
	return (ml_value_t *)Filter;
}

static ML_METHOD_DECL(FilterSoloMethod, "->?");

typedef struct {
	ml_type_t *Type;
	ml_xml_node_t *Node;
	const char *Tag;
} ml_xml_iterator_t;

ML_TYPE(MLXmlForwardT, (MLXmlSequenceT), "xml::forward");
//!internal

static void ML_TYPED_FN(ml_iterate, MLXmlForwardT, ml_state_t *Caller, ml_xml_iterator_t *Iterator) {
	const char *Tag = Iterator->Tag;
	for (ml_xml_node_t *Node = Iterator->Node; Node; Node = Node->Next) {
		if (Node->Base.Type == MLXmlElementT) {
			if ((Tag == NULL) || (Tag == Node->Base.Value)) {
				Iterator->Node = Node;
				ML_RETURN(Iterator);
			}
		}
	}
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_next, MLXmlForwardT, ml_state_t *Caller, ml_xml_iterator_t *Iterator) {
	const char *Tag = Iterator->Tag;
	for (ml_xml_node_t *Node = Iterator->Node->Next; Node; Node = Node->Next) {
		if (Node->Base.Type == MLXmlElementT) {
			if ((Tag == NULL) || (Tag == Node->Base.Value)) {
				Iterator->Node = Node;
				ML_RETURN(Iterator);
			}
		}
	}
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLXmlForwardT, ml_state_t *Caller, ml_xml_iterator_t *Iterator) {
	ML_RETURN(ml_integer(Iterator->Node->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLXmlForwardT, ml_state_t *Caller, ml_xml_iterator_t *Iterator) {
	ML_RETURN(Iterator->Node);
}

ML_TYPE(MLXmlReverseT, (MLXmlSequenceT), "xml::reverse");
//!internal

static void ML_TYPED_FN(ml_iterate, MLXmlReverseT, ml_state_t *Caller, ml_xml_iterator_t *Iterator) {
	const char *Tag = Iterator->Tag;
	for (ml_xml_node_t *Node = Iterator->Node; Node; Node = Node->Prev) {
		if (Node->Base.Type == MLXmlElementT) {
			if ((Tag == NULL) || (Tag == Node->Base.Value)) {
				Iterator->Node = Node;
				ML_RETURN(Iterator);
			}
		}
	}
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_next, MLXmlReverseT, ml_state_t *Caller, ml_xml_iterator_t *Iterator) {
	const char *Tag = Iterator->Tag;
	for (ml_xml_node_t *Node = Iterator->Node->Next; Node; Node = Node->Prev) {
		if (Node->Base.Type == MLXmlElementT) {
			if ((Tag == NULL) || (Tag == Node->Base.Value)) {
				Iterator->Node = Node;
				ML_RETURN(Iterator);
			}
		}
	}
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLXmlReverseT, ml_state_t *Caller, ml_xml_iterator_t *Iterator) {
	ML_RETURN(ml_integer(Iterator->Node->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLXmlReverseT, ml_state_t *Caller, ml_xml_iterator_t *Iterator) {
	ML_RETURN(Iterator->Node);
}

#define ML_XML_ITERATOR(NAME, TYPE, FIELD, DOC) \
ML_METHOD(NAME, MLXmlElementT) { \
/*<Xml
//>sequence
// Returns a sequence of the DOC of :mini:`Xml`.
*/ \
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0]; \
	ml_xml_iterator_t *Iterator = new(ml_xml_iterator_t); \
	Iterator->Type = MLXml ## TYPE ## T; \
	Iterator->Node = Element->FIELD; \
	return (ml_value_t *)Iterator; \
} \
\
ML_METHOD(NAME, MLXmlT, MLStringT) { \
/*<Xml
//<Tag
//>sequence
// Returns a sequence of the DOC of :mini:`Xml` with tag :mini:`Tag`.
*/ \
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0]; \
	ml_xml_iterator_t *Iterator = new(ml_xml_iterator_t); \
	Iterator->Type = MLXml ## TYPE ## T; \
	Iterator->Node = Element->FIELD; \
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(MLXmlTags, ml_string_value(Args[1])); \
	if (!Slot[0]) Slot[0] = Args[1]; \
	Iterator->Tag = (const char *)Slot[0]; \
	return (ml_value_t *)Iterator; \
} \
\
ML_METHODV(NAME, MLXmlT, MLNamesT) { \
/*<Xml
//<Attribute
//>sequence
// Returns a sequence of the DOC of :mini:`Xml` with :mini:`Attribute/1 = Value/1`, etc.
*/ \
	ML_NAMES_CHECK_ARG_COUNT(1); \
	ml_xml_filter_t *Filter = new(ml_xml_filter_t); \
	Filter->Type = MLXmlFilterT; \
	ml_value_t *Attributes = Filter->Attributes = ml_map(); \
	int I = 2; \
	ML_NAMES_FOREACH(Args[1], Iter) { \
		ML_CHECK_ARG_TYPE(I, MLStringT); \
		ml_map_insert(Attributes, Iter->Value, Args[I++]); \
	} \
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0]; \
	ml_xml_iterator_t *Iterator = new(ml_xml_iterator_t); \
	Iterator->Type = MLXml ## TYPE ## T; \
	Iterator->Node = Element->FIELD; \
	ml_value_t *Chained = ml_chainedv(3, Iterator, FilterSoloMethod, Filter); \
	Chained->Type = (ml_type_t *)MLXmlChainedT; \
	return Chained; \
} \
\
ML_METHODV(NAME, MLXmlT, MLStringT, MLNamesT) { \
/*<Xml
//<Tag
//<Attribute
//>sequence
// Returns a sequence of the DOC of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute/1 = Value/1`, etc.
*/ \
	ML_NAMES_CHECK_ARG_COUNT(2); \
	ml_xml_filter_t *Filter = new(ml_xml_filter_t); \
	Filter->Type = MLXmlFilterT; \
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(MLXmlTags, ml_string_value(Args[1])); \
	if (!Slot[0]) Slot[0] = Args[1]; \
	Filter->Tag = (const char *)Slot[0]; \
	ml_value_t *Attributes = Filter->Attributes = ml_map(); \
	int I = 3; \
	ML_NAMES_FOREACH(Args[2], Iter) { \
		if (Args[I] != MLNil) ML_CHECK_ARG_TYPE(I, MLStringT); \
		ml_map_insert(Attributes, Iter->Value, Args[I++]); \
	} \
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0]; \
	ml_xml_iterator_t *Iterator = new(ml_xml_iterator_t); \
	Iterator->Type = MLXml ## TYPE ## T; \
	Iterator->Node = Element->FIELD; \
	ml_value_t *Chained = ml_chainedv(3, Iterator, FilterSoloMethod, Filter); \
	Chained->Type = (ml_type_t *)MLXmlChainedT; \
	return Chained; \
} \
\
ML_METHOD(NAME, MLXmlT, MLFunctionT) { \
/*<Xml
//<Fn
//>sequence
// Returns a sequence of the DOC of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.
*/ \
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0]; \
	ml_xml_iterator_t *Iterator = new(ml_xml_iterator_t); \
	Iterator->Type = MLXml ## TYPE ## T; \
	Iterator->Node = Element->FIELD; \
	ml_value_t *Chained = ml_chainedv(3, Iterator, FilterSoloMethod, Args[1]); \
	Chained->Type = (ml_type_t *)MLXmlChainedT; \
	return Chained; \
}

ML_XML_ITERATOR("/", Forward, Head, children);
ML_XML_ITERATOR(">>", Forward, Base.Next, next siblings);
ML_XML_ITERATOR("<<", Reverse, Base.Prev, previous siblings);

ML_METHOD("parent", MLXmlT, MLStringT) {
//<Xml
//<Tag
//>xml|nil
// Returns the ancestor of :mini:`Xml` with tag :mini:`Tag` if one exists, otherwise :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	const char *Tag = stringmap_search(MLXmlTags, ml_string_value(Args[1]));
	while ((Node = (ml_xml_node_t *)Node->Parent)) {
		if (Node->Base.Type != MLXmlElementT) continue;
		if (Node->Base.Value != Tag) continue;
		return (ml_value_t *)Node;
	}
	return MLNil;
}

ML_METHOD("parent", MLXmlT, MLIntegerT) {
//<Xml
//<N
//>xml|nil
// Returns the :mini:`N`-th parent of :mini:`Xml` or :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	int Steps = ml_integer_value(Args[1]);
	while (Steps > 0) {
		Node = (ml_xml_node_t *)Node->Parent;
		if (!Node) return MLNil;
		--Steps;
	}
	return (ml_value_t *)Node;
}

ML_METHOD("next", MLXmlT, MLIntegerT) {
//<Xml
//<N
//>xml|nil
// Returns the :mini:`N`-th next sibling of :mini:`Xml` or :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	int Steps = ml_integer_value(Args[1]);
	while (Steps > 0) {
		Node = Node->Next;
		if (!Node) return MLNil;
		if (Node->Base.Type == MLXmlElementT) --Steps;
	}
	return (ml_value_t *)Node;
}

ML_METHOD("prev", MLXmlT, MLIntegerT) {
//<Xml
//<N
//>xml|nil
// Returns the :mini:`N`-th previous sibling of :mini:`Xml` or :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	int Steps = ml_integer_value(Args[1]);
	while (Steps > 0) {
		Node = Node->Prev;
		if (!Node) return MLNil;
		if (Node->Base.Type == MLXmlElementT) --Steps;
	}
	return (ml_value_t *)Node;
}

typedef struct {
	ml_type_t *Type;
	ml_xml_element_t *Element, *Root;
	const char *Tag;
} ml_xml_recursive_t;

ML_TYPE(MLXmlRecursiveT, (MLXmlSequenceT), "xml::recursive");
//!internal

static ml_xml_element_t *ml_xml_recursive_find(ml_xml_element_t *Element, const char *Tag) {
	for (ml_xml_node_t *Node = Element->Head; Node; Node = Node->Next) {
		if (Node->Base.Type == MLXmlElementT) {
			ml_xml_element_t *Child = (ml_xml_element_t *)Node;
			if ((Tag == NULL) || (Tag == Node->Base.Value)) return Child;
			Child = ml_xml_recursive_find(Child, Tag);
			if (Child) return Child;
		}
	}
	return NULL;
}

static void ML_TYPED_FN(ml_iterate, MLXmlRecursiveT, ml_state_t *Caller, ml_xml_recursive_t *Recursive) {
	ml_xml_element_t *Element = Recursive->Element;
	const char *Tag = Recursive->Tag;
	if ((Tag == NULL) || (Tag == Element->Base.Base.Value)) {
		ML_RETURN(Recursive);
	}
	Element = ml_xml_recursive_find(Element, Tag);
	if (!Element) ML_RETURN(MLNil);
	Recursive->Element = Element;
	ML_RETURN(Recursive);
}

static void ML_TYPED_FN(ml_iter_next, MLXmlRecursiveT, ml_state_t *Caller, ml_xml_recursive_t *Recursive) {
	ml_xml_element_t *Element = Recursive->Element;
	const char *Tag = Recursive->Tag;
	ml_xml_element_t *Child = ml_xml_recursive_find(Element, Tag);
	if (Child) {
		Recursive->Element = Child;
		ML_RETURN(Recursive);
	}
	ml_xml_element_t *Root = Recursive->Root;
	while (Element != Root) {
		for (ml_xml_node_t *Node = Element->Base.Next; Node; Node = Node->Next) {
			if (Node->Base.Type == MLXmlElementT) {
				ml_xml_element_t *Child = (ml_xml_element_t *)Node;
				if ((Tag == NULL) || (Tag == Node->Base.Value)) {
					Recursive->Element = Child;
					ML_RETURN(Recursive);
				}
				Child = ml_xml_recursive_find(Child, Tag);
				if (Child) {
					Recursive->Element = Child;
					ML_RETURN(Recursive);
				}
			}
		}
		Element = (ml_xml_element_t *)Element->Base.Parent;
	}
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLXmlRecursiveT, ml_state_t *Caller, ml_xml_recursive_t *Recursive) {
	ML_RETURN(ml_integer(Recursive->Element->Base.Index));
}

static void ML_TYPED_FN(ml_iter_value, MLXmlRecursiveT, ml_state_t *Caller, ml_xml_recursive_t *Recursive) {
	ML_RETURN(Recursive->Element);
}

ML_METHOD("//", MLXmlElementT) {
//<Xml
//>sequence
// Returns a sequence of the recursive children of :mini:`Xml`, including :mini:`Xml`.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_xml_recursive_t *Recursive = new(ml_xml_recursive_t);
	Recursive->Type = MLXmlRecursiveT;
	Recursive->Element = Recursive->Root = Element;
	return (ml_value_t *)Recursive;
}

ML_METHOD("//", MLXmlT, MLStringT) {
//<Xml
//<Tag
//>sequence
// Returns a sequence of the recursive children of :mini:`Xml` with tag :mini:`Tag`.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_xml_recursive_t *Recursive = new(ml_xml_recursive_t);
	Recursive->Type = MLXmlRecursiveT;
	Recursive->Element = Recursive->Root = Element;
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(MLXmlTags, ml_string_value(Args[1]));
	if (!Slot[0]) Slot[0] = Args[1];
	Recursive->Tag = (const char *)Slot[0];
	return (ml_value_t *)Recursive;
}

ML_METHODV("//", MLXmlT, MLNamesT) {
//<Xml
//<Attribute
//>sequence
// Returns a sequence of the recursive children of :mini:`Xml` with :mini:`Attribute/1 = Value/1`, etc.
	ML_NAMES_CHECK_ARG_COUNT(1);
	ml_xml_filter_t *Filter = new(ml_xml_filter_t);
	Filter->Type = MLXmlFilterT;
	ml_value_t *Attributes = Filter->Attributes = ml_map();
	int I = 2;
	ML_NAMES_FOREACH(Args[1], Iter) {
		if (Args[I] != MLNil) ML_CHECK_ARG_TYPE(I, MLStringT);
		ml_map_insert(Attributes, Iter->Value, Args[I++]);
	}
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_xml_recursive_t *Recursive = new(ml_xml_recursive_t);
	Recursive->Type = MLXmlRecursiveT;
	Recursive->Element = Recursive->Root = Element;
	ml_value_t *Chained = ml_chainedv(3, Recursive, FilterSoloMethod, Filter);
#ifdef ML_GENERICS
	Chained->Type = (ml_type_t *)MLXmlChainedT;
#endif
	return Chained;
}

ML_METHODV("//", MLXmlT, MLStringT, MLNamesT) {
//<Xml
//<Tag
//<Attribute
//>sequence
// Returns a sequence of the recursive children of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute/1 = Value/1`, etc.
	ML_NAMES_CHECK_ARG_COUNT(2);
	ml_xml_filter_t *Filter = new(ml_xml_filter_t);
	Filter->Type = MLXmlFilterT;
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(MLXmlTags, ml_string_value(Args[1]));
	if (!Slot[0]) Slot[0] = Args[1];
	Filter->Tag = (const char *)Slot[0];
	ml_value_t *Attributes = Filter->Attributes = ml_map();
	int I = 3;
	ML_NAMES_FOREACH(Args[2], Iter) {
		if (Args[I] != MLNil) ML_CHECK_ARG_TYPE(I, MLStringT);
		ml_map_insert(Attributes, Iter->Value, Args[I++]);
	}
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_xml_recursive_t *Recursive = new(ml_xml_recursive_t);
	Recursive->Type = MLXmlRecursiveT;
	Recursive->Element = Recursive->Root = Element;
	ml_value_t *Chained = ml_chainedv(3, Recursive, FilterSoloMethod, Filter);
#ifdef ML_GENERICS
	Chained->Type = (ml_type_t *)MLXmlChainedT;
#endif
	return Chained;
}

ML_METHOD("//", MLXmlT, MLFunctionT) {
//<Xml
//<Fn
//>sequence
// Returns a sequence of the recursive children of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_xml_recursive_t *Recursive = new(ml_xml_recursive_t);
	Recursive->Type = MLXmlRecursiveT;
	Recursive->Element = Recursive->Root = Element;
	ml_value_t *Chained = ml_chainedv(3, Recursive, FilterSoloMethod, Args[1]);
#ifdef ML_GENERICS
	Chained->Type = (ml_type_t *)MLXmlChainedT;
#endif
	return Chained;
}

#ifdef ML_GENERICS

ML_METHOD_DECL(ChildrenMethod, "/");

ML_METHODV("/", MLXmlSequenceT) {
//<Sequence
//<Args
//>sequence
// Generates the sequence :mini:`Node/i / Args` where :mini:`Node/i` are the nodes generated by :mini:`Sequence`.
	ml_value_t *Partial = ml_partial_function(ChildrenMethod, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Doubled = ml_doubled(Args[0], Partial);
	Doubled->Type = (ml_type_t *)MLXmlDoubledT;
	return Doubled;
}

ML_METHOD_DECL(RecursiveMethod, "//");

ML_METHODV("//", MLXmlSequenceT) {
//<Sequence
//<Args
//>sequence
// Generates the sequence :mini:`Node/i // Args` where :mini:`Node/i` are the nodes generated by :mini:`Sequence`.
	ml_value_t *Partial = ml_partial_function(RecursiveMethod, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Doubled = ml_doubled(Args[0], Partial);
	Doubled->Type = (ml_type_t *)MLXmlDoubledT;
	return Doubled;
}

ML_METHOD_DECL(NextSiblingsMethod, ">>");

ML_METHODV(">>", MLXmlSequenceT) {
//<Sequence
//<Args
//>sequence
// Generates the sequence :mini:`Node/i >> Args` where :mini:`Node/i` are the nodes generated by :mini:`Sequence`.
	ml_value_t *Partial = ml_partial_function(NextSiblingsMethod, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Doubled = ml_doubled(Args[0], Partial);
	Doubled->Type = (ml_type_t *)MLXmlDoubledT;
	return Doubled;
}

ML_METHOD_DECL(PrevSiblingsMethod, "<<");

ML_METHODV("<<", MLXmlSequenceT) {
//<Sequence
//<Args
//>sequence
// Generates the sequence :mini:`Node/i << Args` where :mini:`Node/i` are the nodes generated by :mini:`Sequence`.
	ml_value_t *Partial = ml_partial_function(PrevSiblingsMethod, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Doubled = ml_doubled(Args[0], Partial);
	Doubled->Type = (ml_type_t *)MLXmlDoubledT;
	return Doubled;
}

ML_METHOD_DECL(ParentMethod, "parent");

ML_METHODV("parent", MLXmlSequenceT) {
//<Sequence
//<Args
//>sequence
// Generates the sequence :mini:`Node/i ^ Args` where :mini:`Node/i` are the nodes generated by :mini:`Sequence`.
	ml_value_t *Partial = ml_partial_function(ParentMethod, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Chained = ml_chainedv(4, Args[0], Partial, FilterSoloMethod, ml_integer(1));
	Chained->Type = (ml_type_t *)MLXmlChainedT;
	return Chained;
}

ML_METHOD_DECL(NextSiblingMethod, "next");

ML_METHODV("next", MLXmlSequenceT) {
//<Sequence
//<Args
//>sequence
// Generates the sequence :mini:`Node/i + Args` where :mini:`Node/i` are the nodes generated by :mini:`Sequence`.
	ml_value_t *Partial = ml_partial_function(NextSiblingMethod, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Chained = ml_chainedv(4, Args[0], Partial, FilterSoloMethod, ml_integer(1));
	Chained->Type = (ml_type_t *)MLXmlChainedT;
	return Chained;
}

ML_METHOD_DECL(PrevSiblingMethod, "prev");

ML_METHODV("prev", MLXmlSequenceT) {
//<Sequence
//<Args
//>sequence
// Generates the sequence :mini:`Node/i - Args` where :mini:`Node/i` are the nodes generated by :mini:`Sequence`.
	ml_value_t *Partial = ml_partial_function(PrevSiblingMethod, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Chained = ml_chainedv(4, Args[0], Partial, FilterSoloMethod, ml_integer(1));
	Chained->Type = (ml_type_t *)MLXmlChainedT;
	return Chained;
}

#endif

static void ml_xml_contains_text(ml_state_t *Caller, const char *Text, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLXmlT);
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	if (Node->Base.Type == MLXmlTextT) {
		if (strstr(Node->Base.Value, Text)) ML_RETURN(Node);
	} else if (Node->Base.Type == MLXmlElementT) {
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_xml_element_text((ml_xml_element_t *)Node, Buffer, "", 0);
		const char *Value = ml_stringbuffer_get_string(Buffer);
		if (strstr(Value, Text)) ML_RETURN(Node);
	}
	ML_RETURN(MLNil);
}

ML_METHOD("contains", MLXmlSequenceT, MLStringT) {
//<Sequence
//<String
//>sequence
// Equivalent to :mini:`Sequence ->? fun(X) X:text:find(String)`.
	ml_value_t *Contains = ml_cfunctionx((void *)ml_string_value(Args[1]), (ml_callbackx_t)ml_xml_contains_text);
	ml_value_t *Chained = ml_chainedv(3, Args[0], FilterSoloMethod, Contains);
#ifdef ML_GENERICS
	Chained->Type = (ml_type_t *)MLXmlChainedT;
#endif
	return Chained;
}

static int regex_test(const char *Subject, regex_t *Regex) {
	regmatch_t Matches[Regex->re_nsub + 1];
#ifdef ML_TRE
	int Length = strlen(Subject);
	return !regnexec(Regex, Subject, Length, Regex->re_nsub + 1, Matches, 0);

#else
	return !regexec(Regex, Subject, Regex->re_nsub + 1, Matches, 0);
#endif
}

static void ml_xml_contains_regex(ml_state_t *Caller, regex_t *Regex, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ML_CHECKX_ARG_TYPE(0, MLXmlT);
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	if (Node->Base.Type == MLXmlTextT) {
		if (regex_test(Node->Base.Value, Regex)) ML_RETURN(Node);
	} else if (Node->Base.Type == MLXmlElementT) {
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_xml_element_text((ml_xml_element_t *)Node, Buffer, "", 0);
		const char *Value = ml_stringbuffer_get_string(Buffer);
		if (regex_test(Value, Regex)) ML_RETURN(Node);
	}
	ML_RETURN(MLNil);
}

extern regex_t *ml_regex_value(const ml_value_t *Value);

ML_METHOD("contains", MLXmlSequenceT, MLRegexT) {
//<Sequence
//<Regex
//>sequence
// Equivalent to :mini:`Sequence ->? fun(X) X:text:find(Regex)`.
	ml_value_t *Contains = ml_cfunctionx(ml_regex_value(Args[1]), (ml_callbackx_t)ml_xml_contains_regex);
	ml_value_t *Chained = ml_chainedv(3, Args[0], FilterSoloMethod, Contains);
#ifdef ML_GENERICS
	Chained->Type = (ml_type_t *)MLXmlChainedT;
#endif
	return Chained;
}

ML_METHOD("has", MLXmlSequenceT, MLFunctionT) {
//<Sequence
//<Fn
//>sequence
// Equivalent to :mini:`Sequence ->? fun(X) some(Fn(X))`.
	ml_value_t *Filter = ml_chainedv(2, Args[1], MLSome);
	ml_value_t *Chained = ml_chainedv(3, Args[0], FilterSoloMethod, Filter);
#ifdef ML_GENERICS
	Chained->Type = (ml_type_t *)MLXmlChainedT;
#endif
	return Chained;
}

static void ml_xml_escape_string(ml_stringbuffer_t *Buffer, const char *String, int Count) {
	while (--Count >= 0) {
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
			ml_stringbuffer_put(Buffer, *String);
			break;
		}
		++String;
	}
}

static ml_value_t *ml_xml_node_append(ml_stringbuffer_t *Buffer, ml_xml_element_t *Node) {
	const char *Tag = ml_string_value((ml_value_t *)Node->Base.Base.Value);
	ml_stringbuffer_printf(Buffer, "<%s", Tag);
	ML_MAP_FOREACH(Node->Attributes, Iter) {
		if (!ml_is(Iter->Key, MLStringT)) {
			return ml_error("XMLError", "Attribute keys must be strings");
		}
		if (!ml_is(Iter->Value, MLStringT)) {
			return ml_error("XMLError", "Attribute values must be strings");
		}
		ml_stringbuffer_printf(Buffer, " %s=\"", ml_string_value(Iter->Key));
		ml_xml_escape_string(Buffer, ml_string_value(Iter->Value), ml_string_length(Iter->Value));
		ml_stringbuffer_put(Buffer, '\"');
	}
	ml_xml_node_t *Child = Node->Head;
	if (Child) {
		ml_stringbuffer_put(Buffer, '>');
		do {
			if (Child->Base.Type == MLXmlTextT) {
				ml_xml_escape_string(Buffer, Child->Base.Value, Child->Base.Length);
			} else if (Child->Base.Type == MLXmlElementT) {
				ml_value_t *Error = ml_xml_node_append(Buffer, (ml_xml_element_t *)Child);
				if (Error) return Error;
			}
		} while ((Child = Child->Next));
		ml_stringbuffer_printf(Buffer, "</%s>", Tag);
	} else {
		ml_stringbuffer_write(Buffer, "/>", 2);
	}
	return NULL;
}

ML_METHOD("append", MLStringBufferT, MLXmlElementT) {
//<Buffer
//<Xml
// Appends a string representation of :mini:`Xml` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_xml_element_t *Node = (ml_xml_element_t *)Args[1];
	ml_value_t *Error = ml_xml_node_append(Buffer, Node);
	return Error ?: MLSome;
}

typedef struct xml_stack_t xml_stack_t;

struct xml_stack_t {
	ml_xml_element_t *Nodes[ML_XML_STACK_SIZE];
	xml_stack_t *Prev;
	int Index;
};

typedef struct {
	void (*Callback)(void *Data, ml_value_t *Value);
	void *Data;
	ml_xml_element_t *Element;
	xml_stack_t *Stack;
	ml_stringbuffer_t Buffer[1];
	xml_stack_t Stack0;
} xml_decoder_t;

static void xml_start_element(xml_decoder_t *Decoder, const XML_Char *Name, const XML_Char **Attrs) {
	if (Decoder->Buffer->Length) {
		ml_xml_node_t *Text = new(ml_xml_node_t);
		Text->Base.Type = MLXmlTextT;
		Text->Base.Length = Decoder->Buffer->Length;
		Text->Base.Value = ml_stringbuffer_get_string(Decoder->Buffer);
		if (Decoder->Element) ml_xml_element_put(Decoder->Element, Text);
	}
	xml_stack_t *Stack = Decoder->Stack;
	if (Stack->Index == ML_XML_STACK_SIZE) {
		xml_stack_t *NewStack = new(xml_stack_t);
		NewStack->Prev = Stack;
		Stack = Decoder->Stack = NewStack;
	}
	Stack->Nodes[Stack->Index] = Decoder->Element;
	++Stack->Index;
	ml_xml_element_t *Element = Decoder->Element = new(ml_xml_element_t);
	Element->Base.Base.Type = MLXmlElementT;
	ml_value_t *Tag = (ml_value_t *)stringmap_search(MLXmlTags, Name);
	if (!Tag) {
		Name = GC_strdup(Name);
		Tag = ml_string(Name, -1);
		stringmap_insert(MLXmlTags, Name, Tag);
	}
	Element->Base.Base.Value = (const char *)Tag;
	Element->Attributes = ml_map();
	for (const XML_Char **Attr = Attrs; Attr[0]; Attr += 2) {
		ml_map_insert(Element->Attributes, ml_string(GC_strdup(Attr[0]), -1), ml_string(GC_strdup(Attr[1]), -1));
	}
	Decoder->Element = Element;
}

static void xml_end_element(xml_decoder_t *Decoder, const XML_Char *Name) {
	if (Decoder->Buffer->Length) {
		ml_xml_node_t *Text = new(ml_xml_node_t);
		Text->Base.Type = MLXmlTextT;
		Text->Base.Length = Decoder->Buffer->Length;
		Text->Base.Value = ml_stringbuffer_get_string(Decoder->Buffer);
		ml_xml_element_put(Decoder->Element, Text);
	}
	xml_stack_t *Stack = Decoder->Stack;
	if (Stack->Index == 0) {
		Stack = Decoder->Stack = Stack->Prev;
	}
	ml_xml_node_t *Element = (ml_xml_node_t *)Decoder->Element;
	--Stack->Index;
	ml_xml_element_t *Parent = Decoder->Element = Stack->Nodes[Stack->Index];
	Stack->Nodes[Stack->Index] = NULL;
	if (Parent) {
		ml_xml_element_put(Parent, Element);
	} else {
		Decoder->Callback(Decoder->Data, (ml_value_t *)Element);
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

ML_METHOD(MLXmlT, MLStringT) {
//<String
//>xml
// Returns :mini:`String` parsed into an XML node.
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
	ml_value_t *Stream;
	typeof(ml_stream_read) *read;
	XML_Parser Handle;
	ml_value_t *Result;
	xml_decoder_t Decoder;
	char Text[ML_STRINGBUFFER_NODE_SIZE];
} ml_xml_stream_state_t;

static void ml_xml_stream_state_run(ml_xml_stream_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	size_t Length = ml_integer_value(Result);
	if (XML_Parse(State->Handle, State->Text, Length, 0) == XML_STATUS_ERROR) {
		enum XML_Error Error = XML_GetErrorCode(State->Handle);
		ML_ERROR("XMLError", "%s", XML_ErrorString(Error));
	}
	if (!Length) ML_RETURN(State->Result ?: ml_error("XMLError", "Incomplete XML"));
	return State->read((ml_state_t *)State, State->Stream, State->Text, ML_STRINGBUFFER_NODE_SIZE);
}

ML_METHODX(MLXmlT, MLStreamT) {
//<Stream
//>xml
// Returns the contents of :mini:`Stream` parsed into an XML node.
	ml_xml_stream_state_t *State = new(ml_xml_stream_state_t);
	State->Decoder.Callback = (void *)xml_decode_callback;
	State->Decoder.Data = &State->Result;
	State->Decoder.Stack = &State->Decoder.Stack0;
	XML_Memory_Handling_Suite Suite = {GC_malloc, GC_realloc, ml_free};
	XML_Parser Handle = State->Handle = XML_ParserCreate_MM(NULL, &Suite, NULL);
	XML_SetUserData(Handle, &State->Decoder);
	XML_SetElementHandler(Handle, (void *)xml_start_element, (void *)xml_end_element);
	XML_SetCharacterDataHandler(Handle, (void *)xml_character_data);
	XML_SetSkippedEntityHandler(Handle, (void *)xml_skipped_entity);
	XML_SetDefaultHandler(Handle, (void *)xml_default);
	State->Stream = Args[0];
	State->read = ml_typed_fn_get(ml_typeof(Args[0]), ml_stream_read) ?: ml_stream_read_method;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_xml_stream_state_run;
	return State->read((ml_state_t *)State, State->Stream, State->Text, ML_STRINGBUFFER_NODE_SIZE);
}

ML_METHODV(MLXmlT, MLSymbolT) {
//<Tag
//>xml
// Returns a new xml element with tag :mini:`Tag`, adding attributes and children as :mini:`xml::element(...)`.
	ml_xml_element_t *Element = new(ml_xml_element_t);
	Element->Base.Base.Type = MLXmlElementT;
	const char *Tag = ml_symbol_name(Args[0]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(MLXmlTags, Tag);
	if (!Slot[0]) Slot[0] = ml_string(Tag, -1);
	Element->Base.Base.Value = (const char *)Slot[0];
	Element->Attributes = ml_map();
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (int I = 1; I < Count; ++I) {
		ml_value_t *Value = Args[I];
		if (ml_is(Value, MLNamesT)) {
			ML_NAMES_CHECK_ARG_COUNT(I);
			ML_NAMES_FOREACH(Value, Iter) {
				++I;
				ML_CHECK_ARG_TYPE(I, MLStringT);
				ml_map_insert(Element->Attributes, Iter->Value, Args[I]);
			}
			break;
		} else {
			ml_value_t *Error = ml_xml_element_put_general(Element, Value, Buffer);
			if (Error) return Error;
		}
	}
	if (Buffer->Length) {
		ml_xml_node_t *Text = new(ml_xml_node_t);
		Text->Base.Type = MLXmlTextT;
		Text->Base.Length = Buffer->Length;
		Text->Base.Value = ml_stringbuffer_get_string(Buffer);
		ml_xml_element_put(Element, Text);
	}
	return (ml_value_t *)Element;
}

ML_METHOD_ANON(MLXmlParse, "xml::parse");

ML_METHOD(MLXmlParse, MLStringT) {
//@xml::parse
//<String
//>xml
// Returns :mini:`String` parsed into an XML node.
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

ML_METHODX(MLXmlParse, MLStreamT) {
//@xml::parse
//<Stream
//>xml
// Returns the contents of :mini:`Stream` parsed into an XML node.
	ml_xml_stream_state_t *State = new(ml_xml_stream_state_t);
	State->Decoder.Callback = (void *)xml_decode_callback;
	State->Decoder.Data = &State->Result;
	State->Decoder.Stack = &State->Decoder.Stack0;
	XML_Memory_Handling_Suite Suite = {GC_malloc, GC_realloc, ml_free};
	XML_Parser Handle = State->Handle = XML_ParserCreate_MM(NULL, &Suite, NULL);
	XML_SetUserData(Handle, &State->Decoder);
	XML_SetElementHandler(Handle, (void *)xml_start_element, (void *)xml_end_element);
	XML_SetCharacterDataHandler(Handle, (void *)xml_character_data);
	XML_SetSkippedEntityHandler(Handle, (void *)xml_skipped_entity);
	XML_SetDefaultHandler(Handle, (void *)xml_default);
	State->Stream = Args[0];
	State->read = ml_typed_fn_get(ml_typeof(Args[0]), ml_stream_read) ?: ml_stream_read_method;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_xml_stream_state_run;
	return State->read((ml_state_t *)State, State->Stream, State->Text, ML_STRINGBUFFER_NODE_SIZE);
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
// Returns a new decoder that calls :mini:`Callback(Xml)` each time a complete XML document is parsed.
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

ML_TYPE(MLXmlDecoderT, (MLStreamT), "xml::decoder",
//@xml::decoder
// A callback based streaming XML decoder.
	.Constructor = (ml_value_t *)XmlDecoder
);

static void ML_TYPED_FN(ml_stream_write, MLXmlDecoderT, ml_state_t *Caller, ml_xml_decoder_t *Decoder, const void *Address, int Count) {
	if (XML_Parse(Decoder->Handle, Address, Count, 0) == XML_STATUS_ERROR) {
		enum XML_Error Error = XML_GetErrorCode(Decoder->Handle);
		ML_ERROR("XMLError", "%s", XML_ErrorString(Error));
	}
	ML_RETURN(ml_integer(Count));
}

static void ML_TYPED_FN(ml_stream_flush, MLXmlDecoderT, ml_state_t *Caller, ml_xml_decoder_t *Decoder) {
	if (XML_Parse(Decoder->Handle, "", 0, 0) == XML_STATUS_ERROR) {
		enum XML_Error Error = XML_GetErrorCode(Decoder->Handle);
		ML_ERROR("XMLError", "%s", XML_ErrorString(Error));
	}
	ML_RETURN(Decoder);
}

void ml_xml_init(stringmap_t *Globals) {
#include "ml_xml_init.c"
	stringmap_insert(MLXmlT->Exports, "parse", MLXmlParse);
	stringmap_insert(MLXmlT->Exports, "escape", MLXmlEscape);
	stringmap_insert(MLXmlT->Exports, "text", MLXmlTextT);
	stringmap_insert(MLXmlT->Exports, "element", MLXmlElementT);
	stringmap_insert(MLXmlT->Exports, "decoder", MLXmlDecoderT);
#ifdef ML_GENERICS
	stringmap_insert(MLXmlT->Exports, "sequence", MLXmlSequenceT);
#endif
	if (Globals) {
		stringmap_insert(Globals, "xml", MLXmlT);
	}
}

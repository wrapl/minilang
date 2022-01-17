#include "ml_xml.h"
#include "ml_macros.h"
#include "ml_stream.h"
#include <string.h>
#include <expat.h>

#undef ML_CATEGORY
#define ML_CATEGORY "xml"

#define ML_XML_STACK_SIZE 10

typedef struct ml_xml_node_t ml_xml_node_t;

struct ml_xml_node_t {
	ml_string_t Base;
	ml_xml_node_t *Parent, *Next, *Prev;
	size_t Index;
};

ML_TYPE(MLXmlT, (), "xml");
// An XML node.

ML_METHOD("^", MLXmlT) {
//<Xml
//>xml|nil
// Returnst the parent of :mini:`Xml` or :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return (ml_value_t *)Node->Parent ?: MLNil;
}

ML_METHOD("<", MLXmlT) {
//<Xml
//>xml|nil
// Returnst the previous sibling of :mini:`Xml` or :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return (ml_value_t *)Node->Prev ?: MLNil;
}

ML_METHOD(">", MLXmlT) {
//<Xml
//>xml|nil
// Returnst the next sibling of :mini:`Xml` or :mini:`nil`.
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

ML_TYPE(MLXmlTextT, (MLXmlT, MLStringT), "xml::text");
// A XML text node.

ML_METHOD("text", MLXmlTextT) {
//<Xml
//>string
// Returns the text content of :mini:`Xml`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return ml_string(Node->Base.Value, Node->Base.Length);
}

static stringmap_t MLXmlTags[1] = {STRINGMAP_INIT};

typedef struct {
	ml_xml_node_t Base;
	ml_value_t *Attributes;
	ml_xml_node_t *Head, *Tail;
} ml_xml_element_t;

ML_TYPE(MLXmlElementT, (MLXmlT, MLSequenceT), "xml::element");
// An XML element node.

static void ml_xml_element_put(ml_xml_element_t *Parent, ml_xml_node_t *Child) {
	Child->Index = ++Parent->Base.Base.Length;
	Child->Parent = (ml_xml_node_t *)Parent;
	if (Parent->Tail) {
		Parent->Tail->Next = Child;
	} else {
		Parent->Head = Child;
	}
	Child->Prev = Parent->Tail;
	Parent->Tail = Child;
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

static void ml_xml_element_text(ml_xml_element_t *Element, ml_stringbuffer_t *Buffer) {
	for (ml_xml_node_t *Node = Element->Head; Node; Node = Node->Next) {
		if (Node->Base.Type == MLXmlTextT) {
			ml_stringbuffer_write(Buffer, Node->Base.Value, Node->Base.Length);
		} else if (Node->Base.Type == MLXmlElementT) {
			ml_xml_element_text((ml_xml_element_t *)Node, Buffer);
		}
	}
}

ML_METHOD("text", MLXmlElementT) {
//<Xml
//>string
// Returns the (recursive) text content of :mini:`Xml`.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_xml_element_text(Element, Buffer);
	return ml_stringbuffer_get_value(Buffer);
}

ML_METHOD("put", MLXmlElementT, MLStringT) {
//<Parent
//<String
//>xml
// Adds a new text node containing :mini:`String` to :mini:`Parent`.
	ml_xml_element_t *Parent = (ml_xml_element_t *)Args[0];
	ml_xml_node_t *Text = new(ml_xml_node_t);
	Text->Base.Type = MLXmlTextT;
	Text->Base.Length = ml_string_length(Args[1]);
	Text->Base.Value = ml_string_value(Args[1]);
	ml_xml_element_put(Parent, Text);
	return (ml_value_t *)Parent;
}

ML_METHOD("put", MLXmlElementT, MLXmlT) {
//<Parent
//<Child
//>xml
// Adds :mini:`Child` to :mini:`Parent`.
	ml_xml_element_t *Parent = (ml_xml_element_t *)Args[0];
	ml_xml_node_t *Child = (ml_xml_node_t *)Args[1];
	ml_xml_element_put(Parent, Child);
	return (ml_value_t *)Parent;
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

ML_METHOD("^", MLXmlT, MLStringT) {
//<Xml
//<Tag
//>xml|nil
// Returns the parent of :mini:`Xml` if it has tag :mini:`Tag`, otherwise :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	ml_xml_node_t *Parent = Node->Parent;
	if (!Parent) return MLNil;
	if (Parent->Base.Type != MLXmlElementT) return MLNil;
	const char *Tag = stringmap_search(MLXmlTags, ml_string_value(Args[1]));
	if (Parent->Base.Value != Tag) return MLNil;
	return (ml_value_t *)Parent;
}

ML_METHOD("<", MLXmlT, MLStringT) {
//<Xml
//<Tag
//>xml|nil
// Returns the previous sibling of :mini:`Xml` with tag :mini:`Tag`, otherwise :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	ml_xml_node_t *Prev = Node->Prev;
	if (!Prev) return MLNil;
	if (Prev->Base.Type != MLXmlElementT) return MLNil;
	const char *Tag = stringmap_search(MLXmlTags, ml_string_value(Args[1]));
	if (Prev->Base.Value != Tag) return MLNil;
	return (ml_value_t *)Prev;
}

ML_METHOD(">", MLXmlT, MLStringT) {
//<Xml
//<Tag
//>xml|nil
// Returns the next sibling of :mini:`Xml` with tag :mini:`Tag`, otherwise :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	ml_xml_node_t *Next = Node->Next;
	if (!Next) return MLNil;
	if (Next->Base.Type != MLXmlElementT) return MLNil;
	const char *Tag = stringmap_search(MLXmlTags, ml_string_value(Args[1]));
	if (Next->Base.Value != Tag) return MLNil;
	return (ml_value_t *)Next;
}

#ifdef ML_GENERICS

ML_GENERIC_TYPE(MLXmlSequenceT, MLSequenceT, MLIntegerT, MLXmlT);

extern ml_type_t MLDoubledT[];
extern ml_type_t MLChainedT[];

ML_TYPE(MLXmlDoubledT, (MLXmlSequenceT, MLDoubledT), "xml::doubled");
//!internal

ML_TYPE(MLXmlChainedT, (MLXmlSequenceT, MLChainedT), "xml::chained");
//!internal

#else

#define MLXmlSequenceT MLSequenceT

#endif

typedef struct {
	ml_type_t *Type;
	ml_xml_node_t *Node;
	const char *Tag;
} ml_xml_children_t;

ML_TYPE(MLXmlChildrenT, (MLXmlSequenceT), "xml::children");
//!internal

static void ML_TYPED_FN(ml_iterate, MLXmlChildrenT, ml_state_t *Caller, ml_xml_children_t *Children) {
	const char *Tag = Children->Tag;
	for (ml_xml_node_t *Node = Children->Node->Next; Node; Node = Node->Next) {
		if (Node->Base.Type == MLXmlElementT) {
			if ((Tag == NULL) || (Tag == Node->Base.Value)) {
				Children->Node = Node;
				ML_RETURN(Children);
			}
		}
	}
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_next, MLXmlChildrenT, ml_state_t *Caller, ml_xml_children_t *Children) {
	const char *Tag = Children->Tag;
	for (ml_xml_node_t *Node = Children->Node->Next; Node; Node = Node->Next) {
		if (Node->Base.Type == MLXmlElementT) {
			if ((Tag == NULL) || (Tag == Node->Base.Value)) {
				Children->Node = Node;
				ML_RETURN(Children);
			}
		}
	}
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLXmlChildrenT, ml_state_t *Caller, ml_xml_children_t *Children) {
	ML_RETURN(ml_integer(Children->Node->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLXmlChildrenT, ml_state_t *Caller, ml_xml_children_t *Children) {
	ML_RETURN(Children->Node);
}

ML_METHOD("/", MLXmlElementT) {
//<Xml
//>sequence
// Returns a sequence of the children of :mini:`Xml`.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_xml_children_t *Children = new(ml_xml_children_t);
	Children->Type = MLXmlChildrenT;
	Children->Node = Element->Head;
	return (ml_value_t *)Children;
}

ML_METHOD("/", MLXmlT, MLStringT) {
//<Xml
//<Tag
//>sequence
// Returns a sequence of the children of :mini:`Xml` with tag :mini:`Tag`.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_xml_children_t *Children = new(ml_xml_children_t);
	Children->Type = MLXmlChildrenT;
	Children->Node = Element->Head;
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(MLXmlTags, ml_string_value(Args[1]));
	if (!Slot[0]) Slot[0] = Args[1];
	Children->Tag = (const char *)Slot[0];
	return (ml_value_t *)Children;
}

static ML_METHOD_DECL(FilterSoloMethod, "->?");

ML_METHOD("/", MLXmlT, MLFunctionT) {
//<Xml
//<Fn
//>sequence
// Returns a sequence of the children of :mini:`Xml` for which :mini:`Fn(Child)` is non-nil.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_xml_children_t *Children = new(ml_xml_children_t);
	Children->Type = MLXmlChildrenT;
	Children->Node = Element->Head;
	ml_value_t *Chained = ml_chainedv(3, Children, FilterSoloMethod, Args[1]);
#ifdef ML_GENERICS
	Chained->Type = MLXmlChainedT;
#endif
	return Chained;
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
	Chained->Type = MLXmlChainedT;
#endif
	return Chained;
}

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
		if (Value == MLNil) ML_RETURN(MLNil);
		if (strcmp(ml_string_value(Iter->Value), ml_string_value(Value))) ML_RETURN(MLNil);
	}
	ML_RETURN(Element);
}

ML_TYPE(MLXmlFilterT, (MLFunctionT), "xml::filter",
// An XML filter.
	.call = (void *)ml_xml_filter_call
);

ML_METHODV(MLXmlFilterT, MLNamesT) {
//@xml::filter
//>xml::filter
	ml_xml_filter_t *Filter = new(ml_xml_filter_t);
	Filter->Type = MLXmlFilterT;
	ml_value_t *Attributes = Filter->Attributes = ml_map();
	int I = 1;
	ML_NAMES_FOREACH(Args[0], Iter) {
		ML_CHECK_ARG_TYPE(I, MLStringT);
		ml_map_insert(Attributes, Iter->Value, Args[I++]);
	}
	return (ml_value_t *)Filter;
}

ML_METHODV(MLXmlFilterT, MLStringT, MLNamesT) {
//@xml::filter
//>xml::filter
	ml_xml_filter_t *Filter = new(ml_xml_filter_t);
	Filter->Type = MLXmlFilterT;
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(MLXmlTags, ml_string_value(Args[0]));
	if (!Slot[0]) Slot[0] = Args[0];
	Filter->Tag = (const char *)Slot[0];
	ml_value_t *Attributes = Filter->Attributes = ml_map();
	int I = 2;
	ML_NAMES_FOREACH(Args[1], Iter) {
		ML_CHECK_ARG_TYPE(I, MLStringT);
		ml_map_insert(Attributes, Iter->Value, Args[I++]);
	}
	return (ml_value_t *)Filter;
}

ML_METHODV("//", MLXmlT, MLNamesT) {
	ml_xml_filter_t *Filter = new(ml_xml_filter_t);
	Filter->Type = MLXmlFilterT;
	ml_value_t *Attributes = Filter->Attributes = ml_map();
	int I = 2;
	ML_NAMES_FOREACH(Args[1], Iter) {
		ML_CHECK_ARG_TYPE(I, MLStringT);
		ml_map_insert(Attributes, Iter->Value, Args[I++]);
	}
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_xml_recursive_t *Recursive = new(ml_xml_recursive_t);
	Recursive->Type = MLXmlRecursiveT;
	Recursive->Element = Recursive->Root = Element;
	ml_value_t *Chained = ml_chainedv(3, Recursive, FilterSoloMethod, Filter);
#ifdef ML_GENERICS
	Chained->Type = MLXmlChainedT;
#endif
	return Chained;
}

ML_METHODV("//", MLXmlT, MLStringT, MLNamesT) {
	ml_xml_filter_t *Filter = new(ml_xml_filter_t);
	Filter->Type = MLXmlFilterT;
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(MLXmlTags, ml_string_value(Args[1]));
	if (!Slot[0]) Slot[0] = Args[1];
	Filter->Tag = (const char *)Slot[0];
	ml_value_t *Attributes = Filter->Attributes = ml_map();
	int I = 3;
	ML_NAMES_FOREACH(Args[2], Iter) {
		ML_CHECK_ARG_TYPE(I, MLStringT);
		ml_map_insert(Attributes, Iter->Value, Args[I++]);
	}
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	ml_xml_recursive_t *Recursive = new(ml_xml_recursive_t);
	Recursive->Type = MLXmlRecursiveT;
	Recursive->Element = Recursive->Root = Element;
	ml_value_t *Chained = ml_chainedv(3, Recursive, FilterSoloMethod, Filter);
#ifdef ML_GENERICS
	Chained->Type = MLXmlChainedT;
#endif
	return Chained;
}

#ifdef ML_GENERICS

ML_METHOD_DECL(ChildrenMethod, "/");

ML_METHODV("/", MLXmlSequenceT) {
	ml_value_t *Partial = ml_partial_function_new(ChildrenMethod, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Doubled = ml_doubled(Args[0], Partial);
	Doubled->Type = MLXmlDoubledT;
	return Doubled;
}

ML_METHOD_DECL(RecursiveMethod, "//");

ML_METHODV("//", MLXmlSequenceT) {
	ml_value_t *Partial = ml_partial_function_new(RecursiveMethod, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Doubled = ml_doubled(Args[0], Partial);
	Doubled->Type = MLXmlDoubledT;
	return Doubled;
}

#endif

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
	ml_xml_element_t *Parent = Stack->Nodes[Stack->Index] = Decoder->Element;
	++Stack->Index;
	ml_xml_element_t *Element = Decoder->Element = new(ml_xml_element_t);
	Element->Base.Base.Type = MLXmlElementT;
	Element->Base.Parent = (ml_xml_node_t *)Parent;
	ml_value_t *Tag = (ml_value_t *)stringmap_search(MLXmlTags, Name);
	if (!Tag) {
		Name = GC_strdup(Name);
		Tag = ml_cstring(Name);
		stringmap_insert(MLXmlTags, Name, Tag);
	}
	Element->Base.Base.Value = (const char *)Tag;
	Element->Attributes = ml_map();
	for (const XML_Char **Attr = Attrs; Attr[0]; Attr += 2) {
		ml_map_insert(Element->Attributes, ml_cstring(GC_strdup(Attr[0])), ml_cstring(GC_strdup(Attr[1])));
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
	Decoder->Element = Stack->Nodes[Stack->Index];
	Stack->Nodes[Stack->Index] = NULL;
	if (Decoder->Element) {
		ml_xml_element_put(Decoder->Element, Element);
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

ML_METHODV(MLXmlT, MLStringT) {
//@xml
//<Tag
//<Children...:string|xml
//<Attributes?:names|map
//>xml
	ml_xml_element_t *Element = new(ml_xml_element_t);
	Element->Base.Base.Type = MLXmlElementT;
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(MLXmlTags, ml_string_value(Args[0]));
	if (!Slot[0]) Slot[0] = Args[0];
	Element->Base.Base.Value = (const char *)Slot[0];
	Element->Attributes = ml_map();
	for (int I = 1; I < Count; ++I) {
		if (ml_is(Args[I], MLStringT)) {
			ml_xml_node_t *Text = new(ml_xml_node_t);
			Text->Base.Type = MLXmlTextT;
			Text->Base.Length = ml_string_length(Args[I]);
			Text->Base.Value = ml_string_value(Args[I]);
			ml_xml_element_put(Element, Text);
		} else if (ml_is(Args[I], MLXmlT)) {
			ml_xml_element_put(Element, (ml_xml_node_t *)Args[I]);
		} else if (ml_is(Args[I], MLNamesT)) {
			ML_NAMES_FOREACH(Args[I], Iter) {
				++I;
				ML_CHECK_ARG_TYPE(I, MLStringT);
				ml_map_insert(Element->Attributes, Iter->Value, Args[I]);
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
				ml_map_insert(Element->Attributes, Iter->Key, Iter->Value);
			}
		} else {
			return ml_error("XMLError", "Unsupported value for xml node");
		}
	}
	return (ml_value_t *)Element;
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
	stringmap_insert(MLXmlT->Exports, "text", MLXmlTextT);
	stringmap_insert(MLXmlT->Exports, "element", MLXmlElementT);
	stringmap_insert(MLXmlT->Exports, "decoder", MLXmlDecoderT);
#ifdef ML_GENERICS
	stringmap_insert(MLXmlT->Exports, "sequence", MLXmlSequenceT);
	//ml_type_add_rule(MLXmlSequenceT, MLSequenceT, MLIntegerT, MLXmlT, NULL);
#endif
	if (Globals) {
		stringmap_insert(Globals, "xml", MLXmlT);
	}
}

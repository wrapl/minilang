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

ml_xml_element_t *ml_xml_node_parent(ml_xml_node_t *Value) {
	return Value->Parent;
}

ml_xml_node_t *ml_xml_node_next(ml_xml_node_t *Value) {
	return Value->Next;
}

ml_xml_node_t *ml_xml_node_prev(ml_xml_node_t *Value) {
	return Value->Prev;
}

ML_METHOD("parent", MLXmlT) {
//<Xml
//>xml|nil
// Returns the parent of :mini:`Xml` or :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return (ml_value_t *)Node->Parent ?: MLNil;
}

ML_METHOD("^", MLXmlT) {
//<Xml
//>xml|nil
// Returns the parent of :mini:`Xml` or :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return (ml_value_t *)Node->Parent ?: MLNil;
}

ML_METHOD("index", MLXmlT) {
//<Node
//>integer|nil
// Returns the index of :mini:`Node` in its parent or :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return Node->Parent ? ml_integer(Node->Index) : MLNil;
}

ML_METHOD("prev", MLXmlT) {
//<Xml
//>xml|nil
// Returns the previous sibling of :mini:`Xml` or :mini:`nil`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	return (ml_value_t *)Node->Prev ?: MLNil;
}

ML_METHOD("<", MLXmlT) {
//<Xml
//>xml|nil
// Returns the previous sibling of :mini:`Xml` or :mini:`nil`.
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

ML_METHOD(">", MLXmlT) {
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
	return ml_stringbuffer_to_string(Buffer);
}

ML_TYPE(MLXmlTextT, (MLXmlT, MLStringT), "xml::text");
// An XML text node.

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
	return ml_string_unchecked(Node->Base.Value, Node->Base.Length);
}

ML_METHOD("copy", MLVisitorT, MLXmlTextT) {
	ml_xml_node_t *Source = (ml_xml_node_t *)Args[1];
	ml_xml_node_t *Text = new(ml_xml_node_t);
	Text->Base = Source->Base;
	return (ml_value_t *)Text;
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

ml_value_t *ml_xml_element_tag(ml_xml_element_t *Value) {
	return (ml_value_t *)Value->Base.Base.Value;
}

ml_value_t *ml_xml_element_attributes(ml_xml_element_t *Value) {
	return Value->Attributes;
}

size_t ml_xml_element_length(ml_xml_element_t *Value) {
	return Value->Base.Base.Length;
}

ml_xml_node_t *ml_xml_element_head(ml_xml_element_t *Value) {
	return Value->Head;
}

void ml_xml_node_remove(ml_xml_node_t *Child) {
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

void ml_xml_element_put(ml_xml_element_t *Parent, ml_xml_node_t *Child) {
	if (Child->Parent) ml_xml_node_remove(Child);
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
		if (ml_stringbuffer_length(Buffer)) {
			ml_xml_node_t *Text = new(ml_xml_node_t);
			Text->Base.Type = MLXmlTextT;
			Text->Base.Length = ml_stringbuffer_length(Buffer);
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
	if (ml_stringbuffer_length(Buffer)) {
		ml_xml_node_t *Text = new(ml_xml_node_t);
		Text->Base.Type = MLXmlTextT;
		Text->Base.Length = ml_stringbuffer_length(Buffer);
		Text->Base.Value = ml_stringbuffer_get_string(Buffer);
		ml_xml_element_put(Element, Text);
	}
	return (ml_value_t *)Element;
}

ML_METHOD("index", MLXmlT, MLBooleanT) {
//<Node
//<Text
//>integer|nil
// Returns the index of :mini:`Node` in its parent including or excluding text nodes.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	if (!Node->Parent) return MLNil;
	if (Args[1] == (ml_value_t *)MLFalse) {
		int Index = 1;
		for (ml_xml_node_t *Child = Node->Parent->Head; Child != Node; Child = Child->Next) {
			if (Child->Base.Type == MLXmlElementT) ++Index;
		}
		return ml_integer(Index);
	}
	return ml_integer(Node->Index);
}

static void ml_xml_element_path(ml_stringbuffer_t *Buffer, ml_xml_node_t *Node, ml_xml_node_t *Top) {
	if (Node == Top) return;
	ml_xml_element_t *Parent = Node->Parent;
	if (Parent) ml_xml_element_path(Buffer, (ml_xml_node_t *)Parent, Top);
	ml_stringbuffer_put(Buffer, '/');
	ml_value_t *Tag = (ml_value_t *)Node->Base.Value;
	ml_stringbuffer_write(Buffer, ml_string_value(Tag), ml_string_length(Tag));
	if (Parent) {
		int Index = 0;
		for (ml_xml_node_t *Child = Parent->Head; Child != Node; Child = Child->Next) {
			if (Child->Base.Value == Node->Base.Value) ++Index;
		}
		if (Index) ml_stringbuffer_printf(Buffer, "[%d]", Index + 1);
	}
}

ML_METHOD("path", MLXmlT) {
//<Node
//>string
// Returns the path of :mini:`Node` from its root.
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_xml_element_path(Buffer, (ml_xml_node_t *)Args[0], NULL);
	return ml_stringbuffer_get_value(Buffer);
}

ML_METHOD("path", MLXmlT, MLXmlT) {
//<Parent
//<Node
//>string
// Returns the path of :mini:`Node` from :mini:`Parent`.
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_xml_element_path(Buffer, (ml_xml_node_t *)Args[1], (ml_xml_node_t *)Args[0]);
	return ml_stringbuffer_get_value(Buffer);
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
			if (ml_stringbuffer_length(Buffer) && SepLen) ml_stringbuffer_write(Buffer, Sep, SepLen);
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

typedef struct {
	ml_state_t Base;
	ml_value_t *Visitor;
	ml_xml_element_t *Dest;
	ml_xml_node_t *Node;
	ml_value_t *Args[];
} ml_xml_element_visit_t;

static void ml_xml_element_visit_run(ml_xml_element_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (!ml_is(Value, MLXmlT)) ML_ERROR("TypeError", "Expected xml node");
	if (State->Node->Next) {
		State->Node = State->Node->Next;
		State->Args[0] = (ml_value_t *)State->Node;
		return ml_call(State, (ml_value_t *)State->Visitor, 1, State->Args);
	}
	ML_RETURN(MLNil);
}

ML_METHODX("visit", MLVisitorT, MLXmlElementT) {
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_xml_element_t *Source = (ml_xml_element_t *)Args[1];
	if (!Source->Head) ML_RETURN(MLNil);
	ml_xml_element_visit_t *State = new(ml_xml_element_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_xml_element_visit_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Node = Source->Head;
	State->Args[0] = (ml_value_t *)State->Node;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

static void ml_xml_element_copy_run(ml_xml_element_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	if (!ml_is(Value, MLXmlT)) ML_ERROR("TypeError", "Expected xml node");
	ml_xml_node_t *Node = (ml_xml_node_t *)Value;
	ml_xml_element_t *Dest = State->Dest;
	if ((Node->Prev = Dest->Tail)) {
		Dest->Tail->Next = Node;
		Node->Index = Dest->Tail->Index + 1;
	} else {
		Dest->Head = Node;
		Node->Index = 1;
	}
	Dest->Tail = Node;
	if (State->Node->Next) {
		State->Node = State->Node->Next;
		State->Args[0] = (ml_value_t *)State->Node;
		return ml_call(State, (ml_value_t *)State->Visitor, 1, State->Args);
	}
	ML_RETURN(Dest);
}

ML_METHODX("copy", MLVisitorT, MLXmlElementT) {
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_xml_element_t *Source = (ml_xml_element_t *)Args[1];
	ml_xml_element_t *Target = new(ml_xml_element_t);
	Target->Base.Base = Source->Base.Base;
	Target->Attributes = ml_map();
	ML_MAP_FOREACH(Source->Attributes, Iter) ml_map_insert(Target->Attributes, Iter->Key, Iter->Value);
	inthash_insert(Visitor->Cache, (uintptr_t)Source, Target);
	if (!Source->Head) ML_RETURN(Target);
	ml_xml_element_visit_t *State = new(ml_xml_element_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_xml_element_copy_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Dest = Target;
	State->Node = Source->Head;
	State->Args[0] = (ml_value_t *)State->Node;
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

typedef struct {
	ml_xml_element_t *Parent;
	ml_xml_node_t *Head, *Tail;
	ml_stringbuffer_t Buffer[1];
} ml_xml_grower_t;

static ml_value_t *ml_xml_grow(ml_xml_grower_t *Grower, ml_value_t *Value) {
	if (!Value) {
		if (ml_stringbuffer_length(Grower->Buffer)) {
			ml_xml_node_t *Text = new(ml_xml_node_t);
			Text->Base.Type = MLXmlTextT;
			Text->Base.Length = ml_stringbuffer_length(Grower->Buffer);
			Text->Base.Value = ml_stringbuffer_get_string(Grower->Buffer);
			Text->Parent = Grower->Parent;
			Text->Prev = Grower->Tail;
			if (Grower->Tail) Grower->Tail->Next = Text; else Grower->Head = Text;
			Grower->Tail = Text;
		}
	} else if (ml_is(Value, MLXmlT)) {
		if (ml_stringbuffer_length(Grower->Buffer)) {
			ml_xml_node_t *Text = new(ml_xml_node_t);
			Text->Base.Type = MLXmlTextT;
			Text->Base.Length = ml_stringbuffer_length(Grower->Buffer);
			Text->Base.Value = ml_stringbuffer_get_string(Grower->Buffer);
			Text->Parent = Grower->Parent;
			Text->Prev = Grower->Tail;
			if (Grower->Tail) Grower->Tail->Next = Text; else Grower->Head = Text;
			Grower->Tail = Text;
		}
		ml_xml_node_t *Child = (ml_xml_node_t *)Value;
		if (Child->Parent) ml_xml_node_remove(Child);
		Child->Parent = Grower->Parent;
		Child->Prev = Grower->Tail;
		if (Grower->Tail) Grower->Tail->Next = Child; else Grower->Head = Child;
		Grower->Tail = Child;
	} else if (ml_is(Value, MLStringT)) {
		ml_stringbuffer_write(Grower->Buffer, ml_string_value(Value), ml_string_length(Value));
	} else {
		return ml_error("TypeError", "Invalid type for XML node");
	}
	return NULL;
}

ML_METHODV("put", MLXmlElementT, MLAnyT) {
//<Parent
//<Child
//>xml
// Adds :mini:`Child` to :mini:`Parent`.
	ml_xml_element_t *Parent = (ml_xml_element_t *)Args[0];
	ml_xml_grower_t Grower[1] = {{Parent, NULL, NULL, {ML_STRINGBUFFER_INIT}}};
	for (int I = 1; I < Count; ++I) {
		ml_value_t *Error = ml_xml_grow(Grower, Args[I]);
		if (Error) return Error;
	}
	ml_xml_grow(Grower, NULL);
	if (Grower->Tail) {
		Grower->Tail->Next = NULL;
		Grower->Head->Prev = Parent->Tail;
		if (Parent->Tail) Parent->Tail->Next = Grower->Head; else Parent->Head = Grower->Head;
		Parent->Tail = Grower->Tail;
		size_t Index = Parent->Base.Base.Length;
		for (ml_xml_node_t *Child = Grower->Head; Child; Child = Child->Next) Child->Index = ++Index;
		Parent->Base.Base.Length = Index;
	}
	return (ml_value_t *)Parent;
}

ML_METHOD("empty", MLXmlElementT) {
//<Parent
//>xml
// Removes the contents of :mini:`Parent`.
	ml_xml_element_t *Parent = (ml_xml_element_t *)Args[0];
	for (ml_xml_node_t *Child = Parent->Head; Child; Child = Child->Next) {
		Child->Parent = NULL;
		Child->Next = Child->Prev = NULL;
	}
	Parent->Head = Parent->Tail = NULL;
	Parent->Base.Base.Length = 0;
	return (ml_value_t *)Parent;
}

ML_METHOD("remove", MLXmlT) {
//<Node
//>xml
// Removes :mini:`Node` from its parent.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	if (Node->Parent) {
		ml_xml_node_remove(Node);
		Node->Parent = NULL;
		Node->Next = Node->Prev = NULL;
	}
	return (ml_value_t *)Node;
}

ML_METHOD("replace", MLXmlT, MLXmlT) {
//<Node/1
//<Node/2
//>xml
// Removes :mini:`Node/1` from its parent and replaces it with :mini:`Node/2`.
	ml_xml_node_t *Node1 = (ml_xml_node_t *)Args[0];
	ml_xml_node_t *Node2 = (ml_xml_node_t *)Args[1];
	if (Node1 == Node2) return (ml_value_t *)Node2;
	if (!Node1->Parent) return (ml_value_t *)Node2;
	if (Node2->Parent) ml_xml_node_remove(Node2);
	ml_xml_element_t *Parent = Node2->Parent = Node1->Parent;
	if ((Node2->Next = Node1->Next)) {
		Node2->Next->Prev = Node2;
	} else {
		Parent->Tail = Node2;
	}
	if ((Node2->Prev = Node1->Prev)) {
		Node2->Prev->Next = Node2;
	} else {
		Parent->Head = Node2;
	}
	Node1->Parent = NULL;
	Node1->Next = Node1->Prev = NULL;
	return (ml_value_t *)Node2;
}

ML_METHODV("add_next", MLXmlT, MLAnyT) {
//<Node
//<Other
//>xml
// Inserts :mini:`Other` directly after :mini:`Node`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	ml_xml_element_t *Parent = Node->Parent;
	if (!Parent) return ml_error("ValueError", "Node does not currently have a parent");
	ml_xml_grower_t Grower[1] = {{Parent, NULL, NULL, {ML_STRINGBUFFER_INIT}}};
	for (int I = 1; I < Count; ++I) {
		ml_value_t *Error = ml_xml_grow(Grower, Args[I]);
		if (Error) return Error;
	}
	ml_xml_grow(Grower, NULL);
	if (Grower->Tail) {
		ml_xml_node_t *Next = Node->Next;
		Grower->Tail->Next = Next;
		Grower->Head->Prev = Node;
		Node->Next = Grower->Head;
		if (Next) Next->Prev = Grower->Tail; else Parent->Tail = Grower->Tail;
		size_t Index = Node->Index;
		for (ml_xml_node_t *Child = Grower->Head; Child; Child = Child->Next) Child->Index = ++Index;
		Parent->Base.Base.Length = Index;
		return (ml_value_t *)Grower->Tail;
	} else {
		return (ml_value_t *)Node;
	}
}

ML_METHODV("add_prev", MLXmlT, MLAnyT) {
//<Node
//<Other
//>xml
// Inserts :mini:`Other` directly before :mini:`Node`.
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	ml_xml_element_t *Parent = Node->Parent;
	if (!Parent) return ml_error("ValueError", "Node does not currently have a parent");
	ml_xml_grower_t Grower[1] = {{Parent, NULL, NULL, {ML_STRINGBUFFER_INIT}}};
	for (int I = 1; I < Count; ++I) {
		ml_value_t *Error = ml_xml_grow(Grower, Args[I]);
		if (Error) return Error;
	}
	ml_xml_grow(Grower, NULL);
	if (Grower->Tail) {
		ml_xml_node_t *Prev = Node->Prev;
		Grower->Head->Prev = Prev;
		Grower->Tail->Next = Node;
		if (Prev) Prev->Next = Grower->Head; else Parent->Head = Grower->Head;
		Node->Prev = Grower->Tail;
		size_t Index = Node->Index - 1;
		for (ml_xml_node_t *Child = Grower->Head; Child; Child = Child->Next) Child->Index = ++Index;
		Parent->Base.Base.Length = Index;
		return (ml_value_t *)Grower->Tail;
	} else {
		return (ml_value_t *)Node;
	}
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
		if (ml_stringbuffer_length(Buffer)) {
			ml_xml_node_t *Text = new(ml_xml_node_t);
			Text->Base.Type = MLXmlTextT;
			Text->Base.Length = ml_stringbuffer_length(Buffer);
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
		if (ml_stringbuffer_length(Buffer)) {
			ml_xml_node_t *Text = new(ml_xml_node_t);
			Text->Base.Type = MLXmlTextT;
			Text->Base.Length = ml_stringbuffer_length(Buffer);
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

ML_METHOD("[]", MLXmlElementT, MLIntegerT, MLStringT) {
//<Parent
//<Index
//<Tag
//>xml|nil
// Returns the :mini:`Index`-th child of :mini:`Parent` with tag :mini:`Tag` or :mini:`nil`.
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index == 0) return MLNil;
	const char *Tag = stringmap_search(MLXmlTags, ml_string_value(Args[2]));
	if (!Tag) return MLNil;
	if (Index < 0) {
		for (ml_xml_node_t *Child = Element->Tail; Child; Child = Child->Prev) {
			if (Child->Base.Type != MLXmlElementT) continue;
			if (Child->Base.Value != Tag) continue;
			if (++Index == 0) return (ml_value_t *)Child;
		}
	} else {
		for (ml_xml_node_t *Child = Element->Head; Child; Child = Child->Next) {
			if (Child->Base.Type != MLXmlElementT) continue;
			if (Child->Base.Value != Tag) continue;
			if (--Index == 0) return (ml_value_t *)Child;
		}
	}
	return MLNil;
}

ML_METHODV("[]", MLXmlElementT, MLIntegerT, MLNamesT) {
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index == 0) return MLNil;
	ML_NAMES_CHECK_ARG_COUNT(2);
	int I = 2;
	ML_LIST_FOREACH(Args[2], Iter) {
		++I;
		if (Args[I] != MLNil) ML_CHECK_ARG_TYPE(I, MLStringT);
	}
	if (Index < 0) {
		for (ml_xml_node_t *Child = Element->Tail; Child; Child = Child->Prev) {
			if (Child->Base.Type != MLXmlElementT) continue;
			ml_value_t *Attributes = ((ml_xml_element_t *)Child)->Attributes;
			int I = 2;
			ML_LIST_FOREACH(Args[2], Iter) {
				++I;
				ml_value_t *Value = ml_map_search(Attributes, Iter->Value);
				if (Args[I] == MLNil) {
					if (Value != MLNil) goto next1;
				} else {
					if (Value == MLNil) goto next1;
					if (strcmp(ml_string_value(Args[I]), ml_string_value(Value))) goto next1;
				}
			}
			if (++Index == 0) return (ml_value_t *)Child;
		next1:
			continue;
		}
	} else {
		for (ml_xml_node_t *Child = Element->Head; Child; Child = Child->Next) {
			if (Child->Base.Type != MLXmlElementT) continue;
			ml_value_t *Attributes = ((ml_xml_element_t *)Child)->Attributes;
			int I = 2;
			ML_LIST_FOREACH(Args[2], Iter) {
				++I;
				ml_value_t *Value = ml_map_search(Attributes, Iter->Value);
				if (Args[I] == MLNil) {
					if (Value != MLNil) goto next2;
				} else {
					if (Value == MLNil) goto next2;
					if (strcmp(ml_string_value(Args[I]), ml_string_value(Value))) goto next2;
				}
			}
			if (--Index == 0) return (ml_value_t *)Child;
		next2:
			continue;
		}
	}
	return MLNil;
}

ML_METHODV("[]", MLXmlElementT, MLIntegerT, MLStringT, MLNamesT) {
	ml_xml_element_t *Element = (ml_xml_element_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	if (Index == 0) return MLNil;
	const char *Tag = stringmap_search(MLXmlTags, ml_string_value(Args[2]));
	if (!Tag) return MLNil;
	ML_NAMES_CHECK_ARG_COUNT(3);
	int I = 3;
	ML_LIST_FOREACH(Args[3], Iter) {
		++I;
		if (Args[I] != MLNil) ML_CHECK_ARG_TYPE(I, MLStringT);
	}
	if (Index < 0) {
		for (ml_xml_node_t *Child = Element->Tail; Child; Child = Child->Prev) {
			if (Child->Base.Type != MLXmlElementT) continue;
			if (Child->Base.Value != Tag) continue;
			ml_value_t *Attributes = ((ml_xml_element_t *)Child)->Attributes;
			int I = 3;
			ML_LIST_FOREACH(Args[3], Iter) {
				++I;
				ml_value_t *Value = ml_map_search(Attributes, Iter->Value);
				if (Args[I] == MLNil) {
					if (Value != MLNil) goto next1;
				} else {
					if (Value == MLNil) goto next1;
					if (strcmp(ml_string_value(Args[I]), ml_string_value(Value))) goto next1;
				}
			}
			if (++Index == 0) return (ml_value_t *)Child;
		next1:
			continue;
		}
	} else {
		for (ml_xml_node_t *Child = Element->Head; Child; Child = Child->Next) {
			if (Child->Base.Type != MLXmlElementT) continue;
			if (Child->Base.Value != Tag) continue;
			ml_value_t *Attributes = ((ml_xml_element_t *)Child)->Attributes;
			int I = 3;
			ML_LIST_FOREACH(Args[3], Iter) {
				++I;
				ml_value_t *Value = ml_map_search(Attributes, Iter->Value);
				if (Args[I] == MLNil) {
					if (Value != MLNil) goto next2;
				} else {
					if (Value == MLNil) goto next2;
					if (strcmp(ml_string_value(Args[I]), ml_string_value(Value))) goto next2;
				}
			}
			if (--Index == 0) return (ml_value_t *)Child;
		next2:
			continue;
		}
	}
	return MLNil;
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

ML_GENERIC_TYPE(MLXmlSequenceT, MLSequenceT, MLAnyT, MLXmlT);
ML_GENERIC_TYPE(MLXmlDoubledT, MLDoubledT, MLAnyT, MLXmlT);
ML_GENERIC_TYPE(MLXmlChainedT, MLChainedT, MLAnyT, MLXmlT);

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

static ml_value_t *adjacent_node_by_tag(void *Data, int Count, ml_value_t **Args) {
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	const char *Tag = stringmap_search(MLXmlTags, ml_string_value(Args[1]));
	if (!Tag) return MLNil;
	uintptr_t Offset = (uintptr_t)Data;
	while ((Node = *(ml_xml_node_t **)((void *)Node + Offset))) {
		if (Node->Base.Type != MLXmlElementT) continue;
		if (Node->Base.Value != Tag) continue;
		return (ml_value_t *)Node;
	}
	return MLNil;
}

static ml_value_t *adjacent_node_by_attrs(void *Data, int Count, ml_value_t **Args) {
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	ML_NAMES_CHECK_ARG_COUNT(1);
	int I = 1;
	ML_LIST_FOREACH(Args[1], Iter) {
		++I;
		if (Args[I] != MLNil) ML_CHECK_ARG_TYPE(I, MLStringT);
	}
	uintptr_t Offset = (uintptr_t)Data;
	while ((Node = *(ml_xml_node_t **)((void *)Node + Offset))) {
		if (Node->Base.Type != MLXmlElementT) continue;
		ml_value_t *Attributes = ((ml_xml_element_t *)Node)->Attributes;
		int I = 1;
		ML_LIST_FOREACH(Args[1], Iter) {
			++I;
			ml_value_t *Value = ml_map_search(Attributes, Iter->Value);
			if (Args[I] == MLNil) {
				if (Value != MLNil) goto next;
			} else {
				if (Value == MLNil) goto next;
				if (strcmp(ml_string_value(Args[I]), ml_string_value(Value))) goto next;
			}
		}
		return (ml_value_t *)Node;
	next:
		continue;
	}
	return MLNil;
}

static ml_value_t *adjacent_node_by_tag_and_attrs(void *Data, int Count, ml_value_t **Args) {
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	const char *Tag = stringmap_search(MLXmlTags, ml_string_value(Args[1]));
	if (!Tag) return MLNil;
	ML_NAMES_CHECK_ARG_COUNT(2);
	int I = 2;
	ML_LIST_FOREACH(Args[2], Iter) {
		++I;
		if (Args[I] != MLNil) ML_CHECK_ARG_TYPE(I, MLStringT);
	}
	uintptr_t Offset = (uintptr_t)Data;
	while ((Node = *(ml_xml_node_t **)((void *)Node + Offset))) {
		if (Node->Base.Type != MLXmlElementT) continue;
		if (Node->Base.Value != Tag) continue;
		ml_value_t *Attributes = ((ml_xml_element_t *)Node)->Attributes;
		int I = 2;
		ML_LIST_FOREACH(Args[2], Iter) {
			++I;
			ml_value_t *Value = ml_map_search(Attributes, Iter->Value);
			if (Args[I] == MLNil) {
				if (Value != MLNil) goto next;
			} else {
				if (Value == MLNil) goto next;
				if (strcmp(ml_string_value(Args[I]), ml_string_value(Value))) goto next;
			}
		}
		return (ml_value_t *)Node;
	next:
		continue;
	}
	return MLNil;
}

static ml_value_t *adjacent_node_by_count(void *Data, int Count, ml_value_t **Args) {
	ml_xml_node_t *Node = (ml_xml_node_t *)Args[0];
	int Steps = ml_integer_value(Args[1]);
	uintptr_t Offset = (uintptr_t)Data;
	while (Steps > 0) {
		Node = *(ml_xml_node_t **)((void *)Node + Offset);
		if (!Node) return MLNil;
		--Steps;
	}
	return (ml_value_t *)Node;
}

#define ADJACENT_METHODS(NAME, DOC) \
\
ML_METHOD(NAME, MLXmlT, MLIntegerT) { \
/*<Xml
//<N
//>xml|nil
// Returns the :mini:`N`-th DOC of :mini:`Xml` or :mini:`nil`.
*/ \
} \
\
ML_METHOD(NAME, MLXmlT, MLStringT) { \
/*<Xml
//<Tag
//>xml|nil
// Returns the DOC of :mini:`Xml` with tag :mini:`Tag` if one exists, otherwise :mini:`nil`.
*/ \
} \
\
ML_METHODV(NAME, MLXmlT, MLStringT, MLNamesT) { \
/*<Xml
//<Tag
//<Attribute
//>xml|nil
// Returns the DOC of :mini:`Xml` with tag :mini:`Tag` and :mini:`Attribute/1 = Value/1`, etc., if one exists, otherwise :mini:`nil`.
*/ \
} \
\
ML_METHODV(NAME, MLXmlT, MLNamesT) { \
/*<Xml
//<Attribute
//>xml|nil
// Returns the DOC of :mini:`Xml` with :mini:`Attribute/1 = Value/1`, etc., if one exists, otherwise :mini:`nil`.
*/ \
}

/*
ADJACENT_METHODS("^", parent)
ADJACENT_METHODS("parent", parent)
ADJACENT_METHODS(">", next sibling)
ADJACENT_METHODS("next", next sibling)
ADJACENT_METHODS("<", prev sibling)
ADJACENT_METHODS("prev", prev sibling)
*/

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
ML_METHOD_DECL(RecursiveMethod, "//");
ML_METHOD_DECL(NextSiblingsMethod, ">>");
ML_METHOD_DECL(PrevSiblingsMethod, "<<");

static ml_value_t *recursive_doubled(void *Data, int Count, ml_value_t **Args) {
	ml_value_t *Partial = ml_partial_function((ml_value_t *)Data, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Doubled = ml_doubled(Args[0], Partial);
	Doubled->Type = (ml_type_t *)MLXmlDoubledT;
	return Doubled;
}

ML_METHOD_DECL(ParentMethod, "^");
ML_METHOD_DECL(NextSiblingMethod, ">");
ML_METHOD_DECL(PrevSiblingMethod, "<");

static ml_value_t *recursive_adjacent(void *Data, int Count, ml_value_t **Args) {
	ml_value_t *Partial = ml_partial_function((ml_value_t *)Data, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Chained = ml_chainedv(4, Args[0], Partial, FilterSoloMethod, ml_integer(1));
	Chained->Type = (ml_type_t *)MLXmlChainedT;
	return Chained;
}

/*
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

ML_METHODV("^", MLXmlSequenceT) {
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

ML_METHODV("next", MLXmlSequenceT) {
//<Sequence
//<Args
//>sequence
// Generates the sequence :mini:`Node/i > Args` where :mini:`Node/i` are the nodes generated by :mini:`Sequence`.
	ml_value_t *Partial = ml_partial_function(NextSiblingMethod, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Chained = ml_chainedv(4, Args[0], Partial, FilterSoloMethod, ml_integer(1));
	Chained->Type = (ml_type_t *)MLXmlChainedT;
	return Chained;
}

ML_METHODV(">", MLXmlSequenceT) {
//<Sequence
//<Args
//>sequence
// Generates the sequence :mini:`Node/i > Args` where :mini:`Node/i` are the nodes generated by :mini:`Sequence`.
	ml_value_t *Partial = ml_partial_function(NextSiblingMethod, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Chained = ml_chainedv(4, Args[0], Partial, FilterSoloMethod, ml_integer(1));
	Chained->Type = (ml_type_t *)MLXmlChainedT;
	return Chained;
}

ML_METHODV("prev", MLXmlSequenceT) {
//<Sequence
//<Args
//>sequence
// Generates the sequence :mini:`Node/i < Args` where :mini:`Node/i` are the nodes generated by :mini:`Sequence`.
	ml_value_t *Partial = ml_partial_function(PrevSiblingMethod, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Chained = ml_chainedv(4, Args[0], Partial, FilterSoloMethod, ml_integer(1));
	Chained->Type = (ml_type_t *)MLXmlChainedT;
	return Chained;
}

ML_METHODV("<", MLXmlSequenceT) {
//<Sequence
//<Args
//>sequence
// Generates the sequence :mini:`Node/i < Args` where :mini:`Node/i` are the nodes generated by :mini:`Sequence`.
	ml_value_t *Partial = ml_partial_function(PrevSiblingMethod, Count);
	for (int I = 1; I < Count; ++I) ml_partial_function_set(Partial, I, Args[I]);
	ml_value_t *Chained = ml_chainedv(4, Args[0], Partial, FilterSoloMethod, ml_integer(1));
	Chained->Type = (ml_type_t *)MLXmlChainedT;
	return Chained;
}
*/

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
	const char *Head = String;
	while (--Count >= 0) {
		switch (*String) {
		case '<':
			ml_stringbuffer_write(Buffer, Head, String - Head);
			Head = ++String;
			ml_stringbuffer_write(Buffer, "&lt;", strlen("&lt;"));
			break;
		case '>':
			ml_stringbuffer_write(Buffer, Head, String - Head);
			Head = ++String;
			ml_stringbuffer_write(Buffer, "&gt;", strlen("&gt;"));
			break;
		case '&':
			ml_stringbuffer_write(Buffer, Head, String - Head);
			Head = ++String;
			ml_stringbuffer_write(Buffer, "&amp;", strlen("&amp;"));
			break;
		case '\"':
			ml_stringbuffer_write(Buffer, Head, String - Head);
			Head = ++String;
			ml_stringbuffer_write(Buffer, "&quot;", strlen("&quot;"));
			break;
		case '\'':
			ml_stringbuffer_write(Buffer, Head, String - Head);
			Head = ++String;
			ml_stringbuffer_write(Buffer, "&apos;", strlen("&apos;"));
			break;
		default:
			++String;
			break;
		}
	}
	ml_stringbuffer_write(Buffer, Head, String - Head);
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

static ml_value_t *ml_xml_node_append_to(ml_stringbuffer_t *Buffer, ml_xml_element_t *Node, ml_xml_node_t *End) {
	if ((ml_xml_node_t *)Node == End) return (ml_value_t *)MLFalse;
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
			if (Child == End) return (ml_value_t *)MLFalse;
			if (Child->Base.Type == MLXmlTextT) {
				ml_xml_escape_string(Buffer, Child->Base.Value, Child->Base.Length);
			} else if (Child->Base.Type == MLXmlElementT) {
				ml_value_t *Error = ml_xml_node_append_to(Buffer, (ml_xml_element_t *)Child, End);
				if (Error) return Error;
			}
		} while ((Child = Child->Next));
		ml_stringbuffer_printf(Buffer, "</%s>", Tag);
	} else {
		ml_stringbuffer_write(Buffer, "/>", 2);
	}
	return NULL;
}

static ml_value_t *ml_xml_node_append_from_to(ml_stringbuffer_t *Buffer, ml_xml_element_t *Node, ml_xml_node_t *Start, ml_xml_node_t *End) {
	if ((ml_xml_node_t *)Node == Start) return (ml_value_t *)MLTrue;
	ml_xml_node_t *Child = Node->Head;
	while (Child) {
		if (Child == Start) goto do_print_next;
		if (Child->Base.Type == MLXmlElementT) {
			ml_value_t *Error = ml_xml_node_append_from_to(Buffer, (ml_xml_element_t *)Child, Start, End);
			if (Error == (ml_value_t *)MLTrue) goto do_print_next;
			if (Error) return Error;
		}
		Child = Child->Next;
	}
	return NULL;
do_print_next:
	Child = Child->Next;
	while (Child) {
		if (Child->Base.Type == MLXmlTextT) {
			ml_xml_escape_string(Buffer, Child->Base.Value, Child->Base.Length);
		} else if (Child->Base.Type == MLXmlElementT) {
			ml_value_t *Error = ml_xml_node_append_to(Buffer, (ml_xml_element_t *)Child, End);
			if (Error) return Error;
		}
		Child = Child->Next;
	}
	const char *Tag = ml_string_value((ml_value_t *)Node->Base.Base.Value);
	ml_stringbuffer_printf(Buffer, "</%s>", Tag);
	return (ml_value_t *)MLTrue;
}

ML_METHOD("append", MLStringBufferT, MLXmlElementT, MLXmlT, MLXmlT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_xml_element_t *Node = (ml_xml_element_t *)Args[1];
	ml_value_t *Error = ml_xml_node_append_from_to(Buffer, Node, (ml_xml_node_t *)Args[2], (ml_xml_node_t *)Args[3]);
	return Error ?: MLSome;
}

ML_METHOD("append", MLStringBufferT, MLXmlElementT, MLNilT, MLXmlT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_xml_element_t *Node = (ml_xml_element_t *)Args[1];
	ml_value_t *Error = ml_xml_node_append_to(Buffer, Node, (ml_xml_node_t *)Args[3]);
	return Error ?: MLSome;
}

ML_METHOD("append", MLStringBufferT, MLXmlElementT, MLXmlT, MLNilT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_xml_element_t *Node = (ml_xml_element_t *)Args[1];
	ml_value_t *Error = ml_xml_node_append_from_to(Buffer, Node, (ml_xml_node_t *)Args[2], NULL);
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
} xml_parser_t;

static void xml_start_element(xml_parser_t *Parser, const XML_Char *Name, const XML_Char **Attrs) {
	if (ml_stringbuffer_length(Parser->Buffer)) {
		ml_xml_node_t *Text = new(ml_xml_node_t);
		Text->Base.Type = MLXmlTextT;
		Text->Base.Length = ml_stringbuffer_length(Parser->Buffer);
		Text->Base.Value = ml_stringbuffer_get_string(Parser->Buffer);
		if (Parser->Element) ml_xml_element_put(Parser->Element, Text);
	}
	xml_stack_t *Stack = Parser->Stack;
	if (Stack->Index == ML_XML_STACK_SIZE) {
		xml_stack_t *NewStack = new(xml_stack_t);
		NewStack->Prev = Stack;
		Stack = Parser->Stack = NewStack;
	}
	Stack->Nodes[Stack->Index] = Parser->Element;
	++Stack->Index;
	ml_xml_element_t *Element = Parser->Element = new(ml_xml_element_t);
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
	Parser->Element = Element;
}

static void xml_end_element(xml_parser_t *Parser, const XML_Char *Name) {
	if (ml_stringbuffer_length(Parser->Buffer)) {
		ml_xml_node_t *Text = new(ml_xml_node_t);
		Text->Base.Type = MLXmlTextT;
		Text->Base.Length = ml_stringbuffer_length(Parser->Buffer);
		Text->Base.Value = ml_stringbuffer_get_string(Parser->Buffer);
		ml_xml_element_put(Parser->Element, Text);
	}
	xml_stack_t *Stack = Parser->Stack;
	if (Stack->Index == 0) {
		Stack = Parser->Stack = Stack->Prev;
	}
	ml_xml_node_t *Element = (ml_xml_node_t *)Parser->Element;
	--Stack->Index;
	ml_xml_element_t *Parent = Parser->Element = Stack->Nodes[Stack->Index];
	Stack->Nodes[Stack->Index] = NULL;
	if (Parent) {
		ml_xml_element_put(Parent, Element);
	} else {
		Parser->Callback(Parser->Data, (ml_value_t *)Element);
	}
}

static void xml_character_data(xml_parser_t *Parser, const XML_Char *String, int Length) {
	ml_stringbuffer_write(Parser->Buffer, String, Length);
}

static void xml_skipped_entity(xml_parser_t *Parser, const XML_Char *EntityName, int IsParameterEntity) {
	ml_stringbuffer_write(Parser->Buffer, EntityName, strlen(EntityName));
}

static void xml_comment(xml_parser_t *Parser, const XML_Char *String) {
}

static void xml_default(xml_parser_t *Parser, const XML_Char *String, int Length) {
	ml_stringbuffer_write(Parser->Buffer, String, Length);
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
	xml_parser_t Parser = {0,};
	Parser.Callback = (void *)xml_decode_callback;
	Parser.Data = &Result;
	Parser.Stack = &Parser.Stack0;
	XML_Memory_Handling_Suite Suite = {GC_malloc, GC_realloc, ml_free};
	XML_Parser Handle = XML_ParserCreate_MM(NULL, &Suite, NULL);
	XML_SetUserData(Handle, &Parser);
	XML_SetElementHandler(Handle, (void *)xml_start_element, (void *)xml_end_element);
	XML_SetCharacterDataHandler(Handle, (void *)xml_character_data);
	XML_SetSkippedEntityHandler(Handle, (void *)xml_skipped_entity);
	XML_SetCommentHandler(Handle, (void *)xml_comment);
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
	xml_parser_t Parser;
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
	if (!Length) {
		if (XML_Parse(State->Handle, "", 0, 1) == XML_STATUS_ERROR) {
			enum XML_Error Error = XML_GetErrorCode(State->Handle);
			ML_ERROR("XMLError", "%s", XML_ErrorString(Error));
		}
		ML_RETURN(State->Result ?: ml_error("XMLError", "Incomplete XML"));
	}
	return State->read((ml_state_t *)State, State->Stream, State->Text, ML_STRINGBUFFER_NODE_SIZE);
}

ML_METHODX(MLXmlT, MLStreamT) {
//<Stream
//>xml
// Returns the contents of :mini:`Stream` parsed into an XML node.
	ml_xml_stream_state_t *State = new(ml_xml_stream_state_t);
	State->Parser.Callback = (void *)xml_decode_callback;
	State->Parser.Data = &State->Result;
	State->Parser.Stack = &State->Parser.Stack0;
	XML_Memory_Handling_Suite Suite = {GC_malloc, GC_realloc, ml_free};
	XML_Parser Handle = State->Handle = XML_ParserCreate_MM(NULL, &Suite, NULL);
	XML_SetReparseDeferralEnabled(Handle, XML_FALSE);
	XML_SetUserData(Handle, &State->Parser);
	XML_SetElementHandler(Handle, (void *)xml_start_element, (void *)xml_end_element);
	XML_SetCharacterDataHandler(Handle, (void *)xml_character_data);
	XML_SetSkippedEntityHandler(Handle, (void *)xml_skipped_entity);
	XML_SetCommentHandler(Handle, (void *)xml_comment);
	XML_SetDefaultHandler(Handle, (void *)xml_default);
	State->Stream = Args[0];
	State->read = (typeof(ml_stream_read) *)ml_typed_fn_get(ml_typeof(Args[0]), ml_stream_read) ?: ml_stream_read_method;
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
	if (ml_stringbuffer_length(Buffer)) {
		ml_xml_node_t *Text = new(ml_xml_node_t);
		Text->Base.Type = MLXmlTextT;
		Text->Base.Length = ml_stringbuffer_length(Buffer);
		Text->Base.Value = ml_stringbuffer_get_string(Buffer);
		ml_xml_element_put(Element, Text);
	}
	return (ml_value_t *)Element;
}

ML_METHOD_ANON(MLXmlParse, "xml::parse");

ML_METHOD(MLXmlParse, MLAddressT) {
//@xml::parse
//<String
//>xml
// Returns :mini:`String` parsed into an XML node.
	ml_value_t *Result = NULL;
	xml_parser_t Parser = {0,};
	Parser.Callback = (void *)xml_decode_callback;
	Parser.Data = &Result;
	Parser.Stack = &Parser.Stack0;
	XML_Memory_Handling_Suite Suite = {GC_malloc, GC_realloc, ml_free};
	XML_Parser Handle = XML_ParserCreate_MM(NULL, &Suite, NULL);
	XML_SetUserData(Handle, &Parser);
	XML_SetElementHandler(Handle, (void *)xml_start_element, (void *)xml_end_element);
	XML_SetCharacterDataHandler(Handle, (void *)xml_character_data);
	XML_SetSkippedEntityHandler(Handle, (void *)xml_skipped_entity);
	XML_SetCommentHandler(Handle, (void *)xml_comment);
	XML_SetDefaultHandler(Handle, (void *)xml_default);
	const char *Text = ml_address_value(Args[0]);
	size_t Length = ml_address_length(Args[0]);
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
	State->Parser.Callback = (void *)xml_decode_callback;
	State->Parser.Data = &State->Result;
	State->Parser.Stack = &State->Parser.Stack0;
	XML_Memory_Handling_Suite Suite = {GC_malloc, GC_realloc, ml_free};
	XML_Parser Handle = State->Handle = XML_ParserCreate_MM(NULL, &Suite, NULL);
	XML_SetReparseDeferralEnabled(Handle, XML_FALSE);
	XML_SetUserData(Handle, &State->Parser);
	XML_SetElementHandler(Handle, (void *)xml_start_element, (void *)xml_end_element);
	XML_SetCharacterDataHandler(Handle, (void *)xml_character_data);
	XML_SetSkippedEntityHandler(Handle, (void *)xml_skipped_entity);
	XML_SetCommentHandler(Handle, (void *)xml_comment);
	XML_SetDefaultHandler(Handle, (void *)xml_default);
	State->Stream = Args[0];
	State->read = (typeof(ml_stream_read) *)ml_typed_fn_get(ml_typeof(Args[0]), ml_stream_read) ?: ml_stream_read_method;
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
	xml_parser_t Parser[1];
} ml_xml_parser_t;

extern ml_type_t MLXmlParserT[];

static void ml_xml_decode_callback(ml_xml_parser_t *Parser, ml_value_t *Value) {
	Parser->Args[0] = Value;
	return ml_call((ml_state_t *)Parser, Parser->Callback, 1, Parser->Args);
}

static void ml_xml_parser_run(ml_state_t *State, ml_value_t *Value) {
}

ML_FUNCTIONX(XmlParser) {
//@xml::parser
//<Callback
//>xml::parser
// Returns a new parser that calls :mini:`Callback(Xml)` each time a complete XML document is parsed.
	ML_CHECKX_ARG_COUNT(1);
	ml_xml_parser_t *Parser = new(ml_xml_parser_t);
	Parser->Base.Type = MLXmlParserT;
	Parser->Base.Context = Caller->Context;
	Parser->Base.run = ml_xml_parser_run;
	Parser->Callback = Args[0];
	Parser->Parser->Callback = (void *)ml_xml_decode_callback;
	Parser->Parser->Data = Parser;
	Parser->Parser->Stack = &Parser->Parser->Stack0;
	XML_Memory_Handling_Suite Suite = {GC_malloc, GC_realloc, ml_free};
	Parser->Handle = XML_ParserCreate_MM(NULL, &Suite, NULL);
	XML_SetUserData(Parser->Handle, Parser->Parser);
	XML_SetElementHandler(Parser->Handle, (void *)xml_start_element, (void *)xml_end_element);
	XML_SetCharacterDataHandler(Parser->Handle, (void *)xml_character_data);
	XML_SetSkippedEntityHandler(Parser->Handle, (void *)xml_skipped_entity);
	XML_SetCommentHandler(Parser->Handle, (void *)xml_comment);
	XML_SetDefaultHandler(Parser->Handle, (void *)xml_default);
	ML_RETURN(Parser);
}

ML_TYPE(MLXmlParserT, (MLStreamT), "xml::parser",
//@xml::parser
// A callback based streaming XML parser.
	.Constructor = (ml_value_t *)XmlParser
);

static void ML_TYPED_FN(ml_stream_write, MLXmlParserT, ml_state_t *Caller, ml_xml_parser_t *Parser, const void *Address, int Count) {
	if (XML_Parse(Parser->Handle, Address, Count, 0) == XML_STATUS_ERROR) {
		enum XML_Error Error = XML_GetErrorCode(Parser->Handle);
		ML_ERROR("XMLError", "%s", XML_ErrorString(Error));
	}
	ML_RETURN(ml_integer(Count));
}

static void ML_TYPED_FN(ml_stream_flush, MLXmlParserT, ml_state_t *Caller, ml_xml_parser_t *Parser) {
	if (XML_Parse(Parser->Handle, "", 0, 0) == XML_STATUS_ERROR) {
		enum XML_Error Error = XML_GetErrorCode(Parser->Handle);
		ML_ERROR("XMLError", "%s", XML_ErrorString(Error));
	}
	ML_RETURN(Parser);
}

typedef enum {
	XML_ESCAPE_CONTENT,
	XML_ESCAPE_TAG,
	XML_ESCAPE_ATTR_NAME,
	XML_ESCAPE_ATTR_VALUE
} xml_escape_state_t;

typedef struct {
	ml_parser_t *Parser;
	const char *Next;
	ml_value_t *Constructor;
	ml_source_t Source;
} xml_escape_parser_t;

#include <ctype.h>
#include "ml_compiler2.h"

static ml_value_t *ml_parser_escape_xml_string(xml_escape_parser_t *Parser) {
	mlc_string_part_t *Parts = NULL, **Slot = &Parts;
	const char *Next = Parser->Next;
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	while (Next[0] != '\"') {
		if (Next[0] == 0) {
			Next = ml_parser_read(Parser->Parser);
			if (!Next) return ml_error("ParseError", "Incomplete xml");
		} else if (Next[0] == '&') {
			if (Next[1] == '{') {
				if (Buffer->Length) {
					mlc_string_part_t *Part = new(mlc_string_part_t);
					Part->Length = Buffer->Length;
					Part->Chars = ml_stringbuffer_get_string(Buffer);
					Part->Line = Parser->Source.Line;
					Slot[0] = Part;
					Slot = &Part->Next;
				}
				ml_parser_input(Parser->Parser, Next + 2, 0);
				ml_parser_source(Parser->Parser, Parser->Source);
				mlc_expr_t *Expr = ml_accept_expression(Parser->Parser, EXPR_DEFAULT);
				if (!Expr) return ml_parser_value(Parser->Parser);
				ml_accept(Parser->Parser, MLT_RIGHT_BRACE);
				Parser->Source = ml_parser_position(Parser->Parser);
				Next = ml_parser_clear(Parser->Parser);
				mlc_string_part_t *Part = new(mlc_string_part_t);
				Part->Length = 0;
				Part->Child = Expr;
				Part->Line = Parser->Source.Line;
				Slot[0] = Part;
				Slot = &Part->Next;
			} else if (Next[1] == 'a' && Next[2] == 'm' && Next[3] == 'p' && Next[4] == ';') {
				ml_stringbuffer_put(Buffer, '&');
				Next += 5;
			} else if (Next[1] == 'l' && Next[2] == 't' && Next[4] == ';') {
				ml_stringbuffer_put(Buffer, '<');
				Next += 4;
			} else if (Next[1] == 'g' && Next[2] == 't' && Next[4] == ';') {
				ml_stringbuffer_put(Buffer, '>');
				Next += 4;
			} else if (Next[1] == 'a' && Next[2] == 'p' && Next[3] == 'o' && Next[4] == 's' && Next[5] == ';') {
				ml_stringbuffer_put(Buffer, '\'');
				Next += 6;
			} else if (Next[1] == 'q' && Next[2] == 'u' && Next[3] == 'o' && Next[4] == 't' && Next[5] == ';') {
				ml_stringbuffer_put(Buffer, '\"');
				Next += 6;
			} else {
				return ml_error("ParseError", "Invalid XML entity");
			}
		} else if (Next[0] == '\n') {
			++Parser->Source.Line;
			ml_stringbuffer_put(Buffer, ' ');
		} else {
			ml_stringbuffer_put(Buffer, *Next++);
		}
	}
	Parser->Next = Next + 1;
	if (Parts) {
		if (Buffer->Length) {
			mlc_string_part_t *Part = new(mlc_string_part_t);
			Part->Length = Buffer->Length;
			Part->Chars = ml_stringbuffer_get_string(Buffer);
			Part->Line = Parser->Source.Line;
			Slot[0] = Part;
			Slot = &Part->Next;
		}
		mlc_string_expr_t *StringExpr = new(mlc_string_expr_t);
		StringExpr->compile = ml_string_expr_compile;
		StringExpr->Parts = Parts;
		StringExpr->Source = Parser->Source.Name;
		StringExpr->StartLine = StringExpr->EndLine = Parser->Source.Line;
		return ml_expr_value((mlc_expr_t *)StringExpr);
	} else {
		mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
		ValueExpr->compile = ml_value_expr_compile;
		ValueExpr->Value = ml_stringbuffer_get_value(Buffer);
		ValueExpr->Source = Parser->Source.Name;
		ValueExpr->StartLine = ValueExpr->EndLine = Parser->Source.Line;
		return ml_expr_value((mlc_expr_t *)ValueExpr);
	}
}

static ml_value_t *ml_parser_escape_xml_node(xml_escape_parser_t *Parser) {
	const char *Next = Parser->Next;
	while ((*Next > ' ') && (*Next != '/') && (*Next != '>')) ++Next;
	int TagLength = Next - Parser->Next;
	if (!TagLength) return ml_error("ParseError", "Invalid start tag");
	ml_value_t *Tag = ml_string_copy(Parser->Next, TagLength);
	mlc_value_expr_t *TagExpr = new(mlc_value_expr_t);
	TagExpr->compile = ml_value_expr_compile;
	TagExpr->Value = Tag;
	TagExpr->Source = Parser->Source.Name;
	TagExpr->StartLine = TagExpr->EndLine = Parser->Source.Line;
	mlc_parent_value_expr_t *ElementExpr = new(mlc_parent_value_expr_t);
	ElementExpr->compile = ml_const_call_expr_compile;
	ElementExpr->Value = Parser->Constructor;
	ElementExpr->Source = Parser->Source.Name;
	ElementExpr->StartLine = Parser->Source.Line;
	mlc_parent_expr_t *AttrsExpr = new(mlc_parent_expr_t);
	AttrsExpr->compile = ml_map_expr_compile;
	AttrsExpr->Source = Parser->Source.Name;
	AttrsExpr->StartLine = Parser->Source.Line;
	ElementExpr->Child = (mlc_expr_t *)TagExpr;
	TagExpr->Next = (mlc_expr_t *)AttrsExpr;
	mlc_expr_t **Attributes = &AttrsExpr->Child;
	mlc_parent_expr_t *ContentExpr = new(mlc_parent_expr_t);
	ContentExpr->compile = ml_list_expr_compile;
	ContentExpr->Source = Parser->Source.Name;
	ContentExpr->StartLine = Parser->Source.Line;
	AttrsExpr->Next = (mlc_expr_t *)ContentExpr;
	mlc_expr_t **Content = &ContentExpr->Child;
	for (;;) switch (Next[0]) {
		case ' ': case '\t': ++Next; break;
		case '\n': ++Next; ++Parser->Source.Line; break;
		case '\0': {
			Next = ml_parser_read(Parser->Parser);
			if (!Next) return ml_error("ParseError", "Incomplete xml");
			break;
		}
		case '/': {
			AttrsExpr->EndLine = Parser->Source.Line;
			ContentExpr->EndLine = Parser->Source.Line;
			if (Next[1] != '>') return ml_error("ParseError", "Invalid element");
			ElementExpr->EndLine = Parser->Source.Line;
			Parser->Next = Next + 2;
			return ml_expr_value((mlc_expr_t *)ElementExpr);
		}
		case '>': {
			AttrsExpr->EndLine = Parser->Source.Line;
			++Next;
			ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
			for (;;) {
				const char *End = strchr(Next, '<') ?: (Next + strlen(Next));
				while (Next < End) {
					if (Next[0] == '&') {
						if (Next[1] == '{') {
							if (Buffer->Length) {
								mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
								ValueExpr->compile = ml_value_expr_compile;
								ValueExpr->Value = ml_stringbuffer_get_value(Buffer);
								ValueExpr->Source = Parser->Source.Name;
								ValueExpr->StartLine = ValueExpr->EndLine = Parser->Source.Line;
								Content[0] = (mlc_expr_t *)ValueExpr;
								Content = &ValueExpr->Next;
							}
							ml_parser_input(Parser->Parser, Next + 2, 0);
							ml_parser_source(Parser->Parser, Parser->Source);
							mlc_expr_t *Expr = ml_accept_expression(Parser->Parser, EXPR_DEFAULT);
							if (!Expr) return ml_parser_value(Parser->Parser);
							ml_accept(Parser->Parser, MLT_RIGHT_BRACE);
							Parser->Source = ml_parser_position(Parser->Parser);
							Next = ml_parser_clear(Parser->Parser);
							Content[0] = (mlc_expr_t *)Expr;
							Content = &Expr->Next;
							End = strchr(Next, '<') ?: (Next + strlen(Next));
						} else if (Next[1] == 'a' && Next[2] == 'm' && Next[3] == 'p' && Next[4] == ';') {
							ml_stringbuffer_put(Buffer, '&');
							Next += 5;
						} else if (Next[1] == 'l' && Next[2] == 't' && Next[4] == ';') {
							ml_stringbuffer_put(Buffer, '<');
							Next += 4;
						} else if (Next[1] == 'g' && Next[2] == 't' && Next[4] == ';') {
							ml_stringbuffer_put(Buffer, '>');
							Next += 4;
						} else if (Next[1] == 'a' && Next[2] == 'p' && Next[3] == 'o' && Next[4] == 's' && Next[5] == ';') {
							ml_stringbuffer_put(Buffer, '\'');
							Next += 6;
						} else if (Next[1] == 'q' && Next[2] == 'u' && Next[3] == 'o' && Next[4] == 't' && Next[5] == ';') {
							ml_stringbuffer_put(Buffer, '\"');
							Next += 6;
						} else {
							return ml_error("ParseError", "Invalid XML entity");
						}
					} else if (Next[0] == '\n') {
						++Parser->Source.Line;
						ml_stringbuffer_put(Buffer, *Next++);
					} else {
						ml_stringbuffer_put(Buffer, *Next++);
					}
				}
				if (End[0] == '<') {
					if (Buffer->Length) {
						mlc_value_expr_t *ValueExpr = new(mlc_value_expr_t);
						ValueExpr->compile = ml_value_expr_compile;
						ValueExpr->Value = ml_stringbuffer_get_value(Buffer);
						ValueExpr->Source = Parser->Source.Name;
						ValueExpr->StartLine = ValueExpr->EndLine = Parser->Source.Line;
						Content[0] = (mlc_expr_t *)ValueExpr;
						Content = &ValueExpr->Next;
					}
					if (End[1] == '/') {
						ContentExpr->EndLine = Parser->Source.Line;
						Next = (End += 2);
						while ((*Next > ' ') && (*Next != '>')) ++Next;
						if (Next[0] != '>') return ml_error("ParseError", "Invalid end tag");
						if (strncmp(End, ml_string_value(Tag), Next - End)) {
							ml_parser_input(Parser->Parser, Next + 1, 0);
							ml_parser_source(Parser->Parser, Parser->Source);
							return ml_error("ParseError", "Mismatched start and end tag");
						}
						++Next;
						break;
					} else {
						Parser->Next = End + 1;
						ml_value_t *Child = ml_parser_escape_xml_node(Parser);
						if (ml_is_error(Child)) return Child;
						Content[0] = (mlc_expr_t *)Child;
						Content = &((mlc_expr_t *)Child)->Next;
						Next = Parser->Next;
					}
				} else {
					Next = ml_parser_read(Parser->Parser);
					if (!Next) return ml_error("ParseError", "Incomplete xml");
				}
			}
			Parser->Next = Next;
			return ml_expr_value((mlc_expr_t *)ElementExpr);
		}
		default: {
			const char *Start = Next;
			while ((*Next > ' ') && (*Next != '=')) ++Next;
			if (Next[0] != '=') return ml_error("ParseError", "Invalid attribute");
			mlc_value_expr_t *AttrExpr = new(mlc_value_expr_t);
			AttrExpr->compile = ml_value_expr_compile;
			AttrExpr->Value = ml_string_copy(Start, Next - Start);
			AttrExpr->Source = Parser->Source.Name;
			AttrExpr->StartLine = AttrExpr->EndLine = Parser->Source.Line;
			Attributes[0] = (mlc_expr_t *)AttrExpr;
			Attributes = &AttrExpr->Next;
			++Next;
			switch (Next[0]) {
			case '\"': {
				Parser->Next = Next + 1;
				ml_value_t *Child = ml_parser_escape_xml_string(Parser);
				if (ml_is_error(Child)) return Child;
				Attributes[0] = (mlc_expr_t *)Child;
				Attributes = &((mlc_expr_t *)Child)->Next;
				Next = Parser->Next;
				break;
			}
			case '{': {
				ml_parser_input(Parser->Parser, Next + 1, 0);
				ml_parser_source(Parser->Parser, Parser->Source);
				mlc_expr_t *Expr = ml_accept_expression(Parser->Parser, EXPR_DEFAULT);
				if (!Expr) return ml_parser_value(Parser->Parser);
				ml_accept(Parser->Parser, MLT_RIGHT_BRACE);
				Parser->Source = ml_parser_position(Parser->Parser);
				Next = ml_parser_clear(Parser->Parser);
				Attributes[0] = (mlc_expr_t *)Expr;
				Attributes = &Expr->Next;
				break;
			}
			default: return ml_error("ParseError", "Invalid attribute");
			}
		}
	}
	return NULL;
}

ml_value_t *ml_parser_escape_xml_like(ml_parser_t *Parser0, ml_value_t *Constructor) {
	xml_escape_parser_t Parser = {0,};
	Parser.Constructor = Constructor;
	Parser.Source = ml_parser_position(Parser0);
	const char *Next = ml_parser_clear(Parser0);
	char Quote = *Next++;
	while (Next[0] != '<') switch (Next[0]) {
		case ' ': case '\t': ++Next; break;
		case '\n': ++Next; ++Parser.Source.Line; break;
		case '\0': {
			Next = ml_parser_read(Parser0);
			if (!Next) return ml_error("ParseError", "Incomplete xml");
			break;
		}
		default: return ml_error("ParseError", "Invalid xml");
	}
	Parser.Next = Next + 1;
	Parser.Parser = Parser0;
	ml_value_t *Value = ml_parser_escape_xml_node(&Parser);
	if (ml_is_error(Value)) return Value;
	if (Parser.Next[0] != Quote) return ml_error("ParseError", "Invalid string");
	ml_parser_input(Parser0, Parser.Next + 1, 0);
	//ml_parser_input(Parser0, Parser.Next, 0);
	ml_parser_source(Parser0, Parser.Source);
	return Value;
}

static ml_value_t *ml_parser_escape_xml(ml_parser_t *Parser0) {
	return ml_parser_escape_xml_like(Parser0, (ml_value_t *)MLXmlElementT);
}

#define DEFINE_ADJACENT_METHODS(NAME, FIELD) \
	ml_method_by_name(NAME, &((ml_xml_node_t *)0)->FIELD, adjacent_node_by_tag, MLXmlT, MLStringT, NULL); \
	ml_method_by_name(NAME, &((ml_xml_node_t *)0)->FIELD, adjacent_node_by_attrs, MLXmlT, MLNamesT, NULL); \
	ml_method_by_name(NAME, &((ml_xml_node_t *)0)->FIELD, adjacent_node_by_tag_and_attrs, MLXmlT, MLStringT, MLNamesT, NULL); \
	ml_method_by_name(NAME, &((ml_xml_node_t *)0)->FIELD, adjacent_node_by_count, MLXmlT, MLIntegerT, NULL)

void ml_xml_init(stringmap_t *Globals) {
#include "ml_xml_init.c"
	DEFINE_ADJACENT_METHODS("parent", Parent);
	DEFINE_ADJACENT_METHODS("^", Parent);
	DEFINE_ADJACENT_METHODS("next", Next);
	DEFINE_ADJACENT_METHODS(">", Next);
	DEFINE_ADJACENT_METHODS("prev", Prev);
	DEFINE_ADJACENT_METHODS("<", Prev);
#ifdef ML_GENERICS
	ml_method_by_value(ChildrenMethod, ChildrenMethod, recursive_doubled, MLXmlSequenceT, NULL);
	ml_method_by_value(RecursiveMethod, RecursiveMethod, recursive_doubled, MLXmlSequenceT, NULL);
	ml_method_by_value(NextSiblingsMethod, NextSiblingsMethod, recursive_doubled, MLXmlSequenceT, NULL);
	ml_method_by_value(PrevSiblingsMethod, PrevSiblingsMethod, recursive_doubled, MLXmlSequenceT, NULL);
	ml_method_by_value(ParentMethod, ParentMethod, recursive_adjacent, MLXmlSequenceT, NULL);
	ml_method_by_name("parent", ParentMethod, recursive_adjacent, MLXmlSequenceT, NULL);
	ml_method_by_value(NextSiblingMethod, NextSiblingMethod, recursive_adjacent, MLXmlSequenceT, NULL);
	ml_method_by_name("next", NextSiblingMethod, recursive_adjacent, MLXmlSequenceT, NULL);
	ml_method_by_value(PrevSiblingMethod, PrevSiblingMethod, recursive_adjacent, MLXmlSequenceT, NULL);
	ml_method_by_name("prev", PrevSiblingMethod, recursive_adjacent, MLXmlSequenceT, NULL);
#endif
	stringmap_insert(MLXmlT->Exports, "parse", MLXmlParse);
	stringmap_insert(MLXmlT->Exports, "escape", MLXmlEscape);
	stringmap_insert(MLXmlT->Exports, "text", MLXmlTextT);
	stringmap_insert(MLXmlT->Exports, "element", MLXmlElementT);
	stringmap_insert(MLXmlT->Exports, "parser", MLXmlParserT);
#ifdef ML_GENERICS
	stringmap_insert(MLXmlT->Exports, "sequence", MLXmlSequenceT);
#endif
	ml_externals_default_add("xml::element", MLXmlElementT);
	ml_externals_default_add("xml::text", MLXmlTextT);
	if (Globals) {
		stringmap_insert(Globals, "xml", MLXmlT);
	}
	ml_parser_add_escape(NULL, "xml", ml_parser_escape_xml);
}

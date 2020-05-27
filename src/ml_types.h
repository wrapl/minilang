#ifndef ML_TYPES_H
#define ML_TYPES_H

#include <unistd.h>
#include <regex.h>
#include "stringmap.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct ml_value_t ml_value_t;
typedef struct ml_type_t ml_type_t;
typedef struct ml_context_t ml_context_t;
typedef struct ml_state_t ml_state_t;

/****************************** Macros ******************************/

#define _CONCAT3(X, Y, Z) X ## Y ## _ ## Z
#define CONCAT3(X, Y, Z) _CONCAT3(X, Y, Z)

/****************************** Values and Types ******************************/

struct ml_value_t {
	const ml_type_t *Type;
};

typedef struct ml_hash_chain_t ml_hash_chain_t;

struct ml_hash_chain_t {
	ml_hash_chain_t *Previous;
	ml_value_t *Value;
	long Index;
};

typedef struct ml_typed_fn_node_t ml_typed_fn_node_t;

struct ml_type_t {
	const ml_type_t *Type;
	const ml_type_t *Parent;
	const char *Name;
	long (*hash)(ml_value_t *, ml_hash_chain_t *);
	void (*call)(ml_state_t *, ml_value_t *, int, ml_value_t **);
	ml_value_t *(*deref)(ml_value_t *);
	ml_value_t *(*assign)(ml_value_t *, ml_value_t *);
	ml_typed_fn_node_t *TypedFns;
	size_t TypedFnsSize, TypedFnSpace;
};

extern ml_type_t MLTypeT[];

long ml_default_hash(ml_value_t *Value, ml_hash_chain_t *Chain);
void ml_default_call(ml_state_t *Frame, ml_value_t *Value, int Count, ml_value_t **Args);
ml_value_t *ml_default_deref(ml_value_t *Ref);
ml_value_t *ml_default_assign(ml_value_t *Ref, ml_value_t *Value);

#define ML_TYPE(TYPE, PARENT, NAME, ...) ml_type_t TYPE[1] = {{ \
	MLTypeT, PARENT, NAME, \
	.hash = ml_default_hash, \
	.call = ml_default_call, \
	.deref = ml_default_deref, \
	.assign = ml_default_assign, \
	__VA_ARGS__ \
}}

ml_type_t *ml_type(ml_type_t *Parent, const char *Name);
void *ml_typed_fn_get(const ml_type_t *Type, void *TypedFn);
void ml_typed_fn_set(ml_type_t *Type, void *TypedFn, void *Function);

#ifndef GENERATE_INIT

#define ML_TYPED_FN(FUNCTION, TYPE, ARGS ...) CONCAT3(FUNCTION ## _, __LINE__, __COUNTER__)(ARGS)

#else

#define ML_TYPED_FN(FUNCTION, TYPE, ARGS ...) INIT_CODE ml_typed_fn_set(TYPE, FUNCTION, CONCAT3(FUNCTION ## _, __LINE__, __COUNTER__));

#endif

extern ml_type_t MLAnyT[];
extern ml_type_t MLNilT[];

extern ml_value_t MLNil[];
extern ml_value_t MLSome[];

int ml_is(const ml_value_t *Value, const ml_type_t *Type);

long ml_hash_chain(ml_value_t *Value, ml_hash_chain_t *Chain);
long ml_hash(ml_value_t *Value);

typedef ml_value_t *(*ml_callback_t)(void *Data, int Count, ml_value_t **Args);
typedef void (*ml_callbackx_t)(ml_state_t *Frame, void *Data, int Count, ml_value_t **Args);

/****************************** Iterators ******************************/

extern ml_type_t MLIteratableT[];

void ml_iterate(ml_state_t *Caller, ml_value_t *Value);
void ml_iter_value(ml_state_t *Caller, ml_value_t *Iter);
void ml_iter_key(ml_state_t *Caller, ml_value_t *Iter);
void ml_iter_next(ml_state_t *Caller, ml_value_t *Iter);

/****************************** Tuples ******************************/

typedef struct ml_tuple_t ml_tuple_t;

extern ml_type_t MLTupleT[];

struct ml_tuple_t {
	const ml_type_t *Type;
	int Size, NoRefs;
	ml_value_t *Values[];
};

ml_value_t *ml_tuple(size_t Size);

inline int ml_tuple_size(ml_value_t *Tuple) {
	return ((ml_tuple_t *)Tuple)->Size;
}

inline ml_value_t *ml_tuple_get(ml_value_t *Tuple, int Index) {
	return ((ml_tuple_t *)Tuple)->Values[Index - 1];
}

inline ml_value_t *ml_tuple_set(ml_value_t *Tuple, int Index, ml_value_t *Value) {
	return ((ml_tuple_t *)Tuple)->Values[Index - 1] = Value;
}

/****************************** Numbers ******************************/

extern ml_type_t MLNumberT[];
extern ml_type_t MLIntegerT[];
extern ml_type_t MLRealT[];

ml_value_t *ml_integer(long Value);
ml_value_t *ml_real(double Value);
long ml_integer_value(ml_value_t *Value);
double ml_real_value(ml_value_t *Value);
ml_value_t *ml_integer_of(ml_value_t *Value);
ml_value_t *ml_real_of(ml_value_t *Value);

extern ml_value_t *MLIntegerOfMethod;
extern ml_value_t *MLRealOfMethod;

/****************************** Strings ******************************/

typedef struct ml_buffer_t ml_buffer_t;

struct ml_buffer_t {
	const ml_type_t *Type;
	char *Address;
	size_t Size;
};

extern ml_type_t MLBufferT[];
extern ml_type_t MLStringT[];
extern ml_type_t MLRegexT[];
extern ml_type_t MLStringBufferT[];

ml_value_t *ml_buffer(void *Data, int Count, ml_value_t **Args);

ml_value_t *ml_string(const char *Value, int Length);
#define ml_cstring(VALUE) ml_string(VALUE, strlen(VALUE))

ml_value_t *ml_string_format(const char *Format, ...);
const char *ml_string_value(ml_value_t *Value);
size_t ml_string_length(ml_value_t *Value);
ml_value_t *ml_string_of(ml_value_t *Value);

extern ml_value_t *MLStringOfMethod;

ml_value_t *ml_regex(const char *Value);
regex_t *ml_regex_value(ml_value_t *Value);

typedef struct ml_stringbuffer_t ml_stringbuffer_t;
typedef struct ml_stringbuffer_node_t ml_stringbuffer_node_t;

struct ml_stringbuffer_t {
	const ml_type_t *Type;
	ml_stringbuffer_node_t *Nodes;
	size_t Space, Length;
};

#define ML_STRINGBUFFER_NODE_SIZE 248
#define ML_STRINGBUFFER_INIT (ml_stringbuffer_t){MLStringBufferT, 0,}

ssize_t ml_stringbuffer_add(ml_stringbuffer_t *Buffer, const char *String, size_t Length);
ssize_t ml_stringbuffer_addf(ml_stringbuffer_t *Buffer, const char *Format, ...) __attribute__ ((format(printf, 2, 3)));
char *ml_stringbuffer_get(ml_stringbuffer_t *Buffer);
char *ml_stringbuffer_get_uncollectable(ml_stringbuffer_t *Buffer);
ml_value_t *ml_stringbuffer_get_string(ml_stringbuffer_t *Buffer);
int ml_stringbuffer_foreach(ml_stringbuffer_t *Buffer, void *Data, int (*callback)(void *, const char *, size_t));
ml_value_t *ml_stringbuffer_append(ml_stringbuffer_t *Buffer, ml_value_t *Value);

extern ml_value_t *MLStringBufferAppendMethod;

/****************************** Lists ******************************/

typedef struct ml_list_t ml_list_t;

extern ml_type_t MLListT[];
extern ml_type_t MLNamesT[];

struct ml_list_node_t {
	ml_value_t *Value;
};

struct ml_list_t {
	const ml_type_t *Type;
	ml_value_t **Nodes, **Head, **Tail;
	int Length, Space;
};

ml_value_t *ml_list();
void ml_list_push(ml_value_t *List, ml_value_t *Value);
void ml_list_put(ml_value_t *List, ml_value_t *Value);
ml_value_t *ml_list_pop(ml_value_t *List);
ml_value_t *ml_list_pull(ml_value_t *List);

ml_value_t *ml_list_get(ml_value_t *List, int Index);
ml_value_t *ml_list_set(ml_value_t *List, int Index, ml_value_t *Value);

#define ml_list_append ml_list_put

void ml_list_to_array(ml_value_t *List, ml_value_t **Array);
int ml_list_foreach(ml_value_t *List, void *Data, int (*callback)(ml_value_t *, void *));

inline int ml_list_length(ml_value_t *List) {
	return ((ml_list_t *)List)->Length;
}

typedef struct {
	ml_value_t **Node, **Last;
	ml_value_t *Value;
} ml_list_iter_t;

inline int ml_list_iter_forward(ml_value_t *List0, ml_list_iter_t *Iter) {
	ml_list_t *List = (ml_list_t *)List0;
	if (!List->Length) {
		Iter->Node = NULL;
		return 0;
	} else {
		Iter->Last = ((ml_list_t *)List)->Tail;
		Iter->Node = ((ml_list_t *)List)->Head;
		Iter->Value = Iter->Node[0];
		return 1;
	}
}

inline int ml_list_iter_next(ml_list_iter_t *Iter) {
	if (Iter->Node + 1 == Iter->Last) {
		Iter->Node = NULL;
		return 0;
	} else {
		++Iter->Node;
		Iter->Value = Iter->Node[0];
		return 1;
	}
}

inline int ml_list_iter_backward(ml_value_t *List0, ml_list_iter_t *Iter) {
	ml_list_t *List = (ml_list_t *)List0;
	if (!List->Length) {
		Iter->Node = NULL;
		return 0;
	} else {
		Iter->Last = ((ml_list_t *)List)->Head;
		Iter->Node = ((ml_list_t *)List)->Tail - 1;
		Iter->Value = Iter->Node[0];
		return 1;
	}
}

inline int ml_list_iter_prev(ml_list_iter_t *Iter) {
	if (Iter->Node == Iter->Last) {
		Iter->Node = NULL;
		return 0;
	} else {
		--Iter->Node;
		Iter->Value = Iter->Node[0];
		return 1;
	}
}

inline int ml_list_iter_valid(ml_list_iter_t *Iter) {
	return Iter->Node != NULL;
}

inline void ml_list_iter_update(ml_list_iter_t *Iter, ml_value_t *Value) {
	Iter->Value = Iter->Node[0] = Value;
}

#define ML_LIST_FOREACH(LIST, ITER) \
	for (ml_list_iter_t ITER[1] = {{((ml_list_t *)LIST)->Head, ((ml_list_t *)LIST)->Tail}}; (ITER->Node < ITER->Last) && (ITER->Value = ITER->Node[0]); ++ITER->Node)

#define ML_LIST_REVERSE(LIST, ITER) \
	for (ml_list_iter_t ITER[1] = {{((ml_list_t *)LIST)->Tail - 1, ((ml_list_t *)LIST)->Head}}; (ITER->Node >= ITER->Last) && (ITER->Value = ITER->Node[0]); --ITER->Node)

#define ML_NAMES_FOREACH(LIST, ITER) ML_LIST_FOREACH(LIST, ITER)

/****************************** Maps ******************************/

typedef struct ml_map_t ml_map_t;
typedef struct ml_map_node_t ml_map_node_t;

extern ml_type_t MLMapT[];

struct ml_map_t {
	const ml_type_t *Type;
	ml_map_node_t *Head, *Tail, *Root;
	int Size;
};

struct ml_map_node_t {
	ml_map_node_t *Next, *Prev;
	ml_value_t *Key;
	ml_map_node_t *Left, *Right;
	ml_value_t *Value;
	long Hash;
	int Depth;
};

ml_value_t *ml_map();
ml_value_t *ml_map_search(ml_value_t *Map, ml_value_t *Key);
ml_value_t *ml_map_insert(ml_value_t *Map, ml_value_t *Key, ml_value_t *Value);
ml_value_t *ml_map_delete(ml_value_t *Map, ml_value_t *Key);

inline int ml_map_size(ml_value_t *Map) {
	return ((ml_map_t *)Map)->Size;
}

int ml_map_foreach(ml_value_t *Map, void *Data, int (*callback)(ml_value_t *, ml_value_t *, void *));

typedef struct {
	ml_map_node_t *Node;
	ml_value_t *Key, *Value;
} ml_map_iter_t;

inline int ml_map_iter_forward(ml_value_t *Map0, ml_map_iter_t *Iter) {
	ml_map_t *Map = (ml_map_t *)Map0;
	ml_map_node_t *Node = Iter->Node = Map->Head;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		Iter->Value = Node->Value;
		return 1;
	}
}

inline int ml_map_iter_next(ml_map_iter_t *Iter) {
	ml_map_node_t *Node = Iter->Node = Iter->Node->Next;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		Iter->Value = Node->Value;
		return 1;
	}
}

inline int ml_map_iter_backward(ml_value_t *Map0, ml_map_iter_t *Iter) {
	ml_map_t *Map = (ml_map_t *)Map0;
	ml_map_node_t *Node = Iter->Node = Map->Tail;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		Iter->Value = Node->Value;
		return 1;
	}
}

inline int ml_map_iter_prev(ml_map_iter_t *Iter) {
	ml_map_node_t *Node = Iter->Node = Iter->Node->Prev;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		Iter->Value = Node->Value;
		return 1;
	}
}

inline int ml_map_iter_valid(ml_map_iter_t *Iter) {
	return Iter->Node != NULL;
}

inline void ml_map_iter_update(ml_map_iter_t *Iter, ml_value_t *Value) {
	Iter->Value = Iter->Node->Value = Value;
}

#define ML_MAP_FOREACH(MAP, ITER) \
	for (ml_map_iter_t ITER[1] = {{((ml_map_t *)MAP)->Head}}; ITER->Node && (ITER->Key = ITER->Node->Key) && (ITER->Value = ITER->Node->Value); ITER->Node = ITER->Node->Next)

/****************************** Methods ******************************/

extern ml_type_t MLMethodT[];

ml_value_t *ml_method(const char *Name);
const char *ml_method_name(ml_value_t *Value);

void ml_method_by_name(const char *Method, void *Data, ml_callback_t Function, ...) __attribute__ ((sentinel));
void ml_method_by_value(ml_value_t *Method, void *Data, ml_callback_t Function, ...) __attribute__ ((sentinel));

void ml_methodx_by_name(const char *Method, void *Data, ml_callbackx_t Function, ...) __attribute__ ((sentinel));
void ml_methodx_by_value(ml_value_t *Method, void *Data, ml_callbackx_t Function, ...) __attribute__ ((sentinel));

void ml_method_by_array(ml_value_t *Value, ml_value_t *Function, int Count, ml_type_t **Types);

#ifndef GENERATE_INIT

#define ML_METHOD(METHOD, TYPES ...) static ml_value_t *CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)(void *Data, int Count, ml_value_t **Args)

#define ML_METHODX(METHOD, TYPES ...) static void CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args)

#define ML_METHOD_DECL(NAME, METHOD) ml_value_t *NAME ## Method

#else

#ifndef __cplusplus

#define ML_METHOD(METHOD, TYPES ...) INIT_CODE _Generic(METHOD, char *: ml_method_by_name, ml_value_t *: ml_method_by_value)(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, NULL);

#define ML_METHODX(METHOD, TYPES ...) INIT_CODE _Generic(METHOD, char *: ml_methodx_by_name, ml_value_t *: ml_methodx_by_value)(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, NULL);

#else

#define ML_METHOD(METHOD, TYPES ...) INIT_CODE ml_method_by_auto(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, NULL);

#define ML_METHODX(METHOD, TYPES ...) INIT_CODE ml_methodx_by_auto(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, NULL);

#endif

#define ML_METHOD_DECL(NAME, METHOD) INIT_CODE NAME ## Method = ml_method(METHOD);

#endif

/****************************** Modules ******************************/

extern ml_type_t MLModuleT[];

ml_value_t *ml_module(const char *Path, ...) __attribute__ ((sentinel));
const char *ml_module_path(ml_value_t *Module);
ml_value_t *ml_module_import(ml_value_t *Module, const char *Name);
ml_value_t *ml_module_export(ml_value_t *Module, const char *Name, ml_value_t *Value);

/****************************** Init ******************************/

void ml_types_init(stringmap_t *Globals);

#ifdef	__cplusplus
}

template <typename... args> void ml_method_by_auto(const char *Method, void *Data, ml_callback_t Function, args... Args) {
	ml_method_by_name(Method, Data, Function, Args...);
}

template <typename... args> void ml_method_by_auto(ml_value_t *Method, void *Data, ml_callback_t Function, args... Args) {
	ml_method_by_value(Method, Data, Function, Args...);
}

template <typename... args> void ml_methodx_by_auto(const char *Method, void *Data, ml_callbackx_t Function, args... Args) {
	ml_methodx_by_name(Method, Data, Function, Args...);
}

template <typename... args> void ml_methodx_by_auto(ml_value_t *Method, void *Data, ml_callbackx_t Function, args... Args) {
	ml_methodx_by_value(Method, Data, Function, Args...);
}

#endif

#endif

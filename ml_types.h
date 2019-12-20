#ifndef ML_TYPES_H
#define ML_TYPES_H

#include <regex.h>

#ifdef	__cplusplus
extern "C" {
#endif

#include "sha256.h"

typedef struct ml_type_t ml_type_t;
typedef struct ml_value_t ml_value_t;

typedef ml_value_t *(*ml_callback_t)(void *Data, int Count, ml_value_t **Args);
typedef ml_value_t *(*ml_getter_t)(void *Data, const char *Name);
typedef ml_value_t *(*ml_setter_t)(void *Data, const char *Name, ml_value_t *Value);

typedef struct ml_function_t ml_function_t;
typedef struct ml_reference_t ml_reference_t;
typedef struct ml_integer_t ml_integer_t;
typedef struct ml_real_t ml_real_t;
typedef struct ml_string_t ml_string_t;
typedef struct ml_regex_t ml_regex_t;
typedef struct ml_list_t ml_list_t;
typedef struct ml_map_t ml_map_t;
typedef struct ml_tuple_t ml_tuple_t;
typedef struct ml_property_t ml_property_t;
typedef struct ml_closure_t ml_closure_t;
typedef struct ml_method_t ml_method_t;
typedef struct ml_error_t ml_error_t;

typedef struct ml_state_t ml_state_t;

struct ml_state_t {
	const ml_type_t *Type;
	ml_state_t *Caller;
	ml_value_t *(*run)(ml_state_t *State, ml_value_t *Value);
};

#define ML_CONTINUE(STATE, VALUE) { \
	ml_state_t *__State = (ml_state_t *)(STATE); \
	ml_value_t *__Value = (ml_value_t *)(VALUE); \
	return __State ? __State->run(__State, __Value) : __Value; \
}

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

typedef ml_value_t *(*ml_callbackx_t)(ml_state_t *Frame, void *Data, int Count, ml_value_t **Args);

struct ml_type_t {
	const ml_type_t *Type;
	const ml_type_t *Parent;
	const char *Name;
	long (*hash)(ml_value_t *, ml_hash_chain_t *);
	ml_value_t *(*call)(ml_state_t *, ml_value_t *, int, ml_value_t **);
	ml_value_t *(*deref)(ml_value_t *);
	ml_value_t *(*assign)(ml_value_t *, ml_value_t *);
	ml_typed_fn_node_t *TypedFns;
	size_t TypedFnsSize, TypedFnSpace;
};

void *ml_typed_fn_get(const ml_type_t *Type, void *TypedFn);
void ml_typed_fn_set(ml_type_t *Type, void *TypedFn, void *Function);

struct ml_function_t {
	const ml_type_t *Type;
	ml_callback_t Callback;
	void *Data;
};

typedef struct ml_closure_info_t ml_closure_info_t;

typedef struct ml_list_node_t ml_list_node_t;
typedef struct ml_map_node_t ml_map_node_t;

long ml_hash_chain(ml_value_t *Value, ml_hash_chain_t *Chain);
long ml_hash(ml_value_t *Value);
ml_type_t *ml_type(ml_type_t *Parent, const char *Name);

void ml_method_by_name(const char *Method, void *Data, ml_callback_t Function, ...) __attribute__ ((sentinel));
void ml_method_by_value(ml_value_t *Method, void *Data, ml_callback_t Function, ...) __attribute__ ((sentinel));

void ml_methodx_by_name(const char *Method, void *Data, ml_callbackx_t Function, ...) __attribute__ ((sentinel));
void ml_methodx_by_value(ml_value_t *Method, void *Data, ml_callbackx_t Function, ...) __attribute__ ((sentinel));

void ml_method_by_array(ml_value_t *Value, ml_value_t *Function, int Count, ml_type_t **Types);

ml_value_t *ml_string(const char *Value, int Length);
ml_value_t *ml_string_format(const char *Format, ...);
ml_value_t *ml_regex(const char *Value);
ml_value_t *ml_integer(long Value);
ml_value_t *ml_real(double Value);
ml_value_t *ml_list();
ml_value_t *ml_map();
ml_value_t *ml_function(void *Data, ml_callback_t Function);
ml_value_t *ml_functionx(void *Data, ml_callbackx_t Function);
ml_value_t *ml_property(void *Data, const char *Name, ml_getter_t Get, ml_setter_t Set, ml_getter_t Next, ml_getter_t Key);
ml_value_t *ml_error(const char *Error, const char *Format, ...) __attribute__ ((format(printf, 2, 3)));
ml_value_t *ml_reference(ml_value_t **Address);
ml_value_t *ml_method(const char *Name);

long ml_integer_value(ml_value_t *Value);
double ml_real_value(ml_value_t *Value);
const char *ml_string_value(ml_value_t *Value);
int ml_string_length(ml_value_t *Value);
regex_t *ml_regex_value(ml_value_t *Value);

const char *ml_method_name(ml_value_t *Value);

ml_value_t *ml_call(ml_value_t *Value, int Count, ml_value_t **Args);
ml_value_t *ml_inline(ml_value_t *Value, int Count, ...);

typedef struct ml_source_t {
	const char *Name;
	int Line;
} ml_source_t;

const char *ml_error_type(ml_value_t *Value);
const char *ml_error_message(ml_value_t *Value);
int ml_error_trace(ml_value_t *Value, int Level, const char **Source, int *Line);
void ml_error_trace_add(ml_value_t *Error, ml_source_t Source);
void ml_error_print(ml_value_t *Error);

void ml_closure_sha256(ml_value_t *Closure, unsigned char Hash[SHA256_BLOCK_SIZE]);

ml_value_t *ml_tuple(size_t Size);
size_t ml_tuple_size(ml_value_t *Tuple);
ml_value_t *ml_tuple_get(ml_value_t *Tuple, size_t Index);
ml_value_t *ml_tuple_set(ml_value_t *Tuple, size_t Index, ml_value_t *Value);

void ml_list_append(ml_value_t *List, ml_value_t *Value);
int ml_list_length(ml_value_t *List);
void ml_list_to_array(ml_value_t *List, ml_value_t **Array);
int ml_list_foreach(ml_value_t *List, void *Data, int (*callback)(ml_value_t *, void *));

ml_value_t *ml_map_search(ml_value_t *Map, ml_value_t *Key);
ml_value_t *ml_map_insert(ml_value_t *Map, ml_value_t *Key, ml_value_t *Value);
ml_value_t *ml_map_delete(ml_value_t *Map, ml_value_t *Key);
int ml_map_size(ml_value_t *Map);
int ml_map_foreach(ml_value_t *Map, void *Data, int (*callback)(ml_value_t *, ml_value_t *, void *));

long ml_default_hash(ml_value_t *Value, ml_hash_chain_t *Chain);
ml_value_t *ml_default_call(ml_state_t *Frame, ml_value_t *Value, int Count, ml_value_t **Args);
ml_value_t *ml_default_deref(ml_value_t *Ref);
ml_value_t *ml_default_assign(ml_value_t *Ref, ml_value_t *Value);

ml_value_t *ml_iterate(ml_state_t *Frame, ml_value_t *Value);
ml_value_t *ml_iter_value(ml_state_t *Frame, ml_value_t *Iter);
ml_value_t *ml_iter_key(ml_state_t *Frame, ml_value_t *Iter);
ml_value_t *ml_iter_next(ml_state_t *Frame, ml_value_t *Iter);

extern ml_type_t MLAnyT[];
extern ml_type_t MLTypeT[];
extern ml_type_t MLNilT[];
extern ml_type_t MLFunctionT[];
extern ml_type_t MLNumberT[];
extern ml_type_t MLIntegerT[];
extern ml_type_t MLRealT[];
extern ml_type_t MLStringT[];
extern ml_type_t MLRegexT[];
extern ml_type_t MLTupleT[];
extern ml_type_t MLMethodT[];
extern ml_type_t MLReferenceT[];
extern ml_type_t MLListT[];
extern ml_type_t MLMapT[];
extern ml_type_t MLPropertyT[];
extern ml_type_t MLClosureT[];
extern ml_type_t MLErrorT[];
extern ml_type_t MLErrorValueT[];
extern ml_type_t MLIteratableT[];

extern ml_value_t MLNil[];
extern ml_value_t MLSome[];

extern ml_type_t MLNamesT[];

int ml_is(const ml_value_t *Value, const ml_type_t *Type);

#define ML_STRINGBUFFER_NODE_SIZE 248

typedef struct ml_stringbuffer_t ml_stringbuffer_t;
typedef struct ml_stringbuffer_node_t ml_stringbuffer_node_t;

struct ml_stringbuffer_t {
	const ml_type_t *Type;
	ml_stringbuffer_node_t *Nodes;
	size_t Space, Length;
};

extern ml_type_t MLStringBufferT[1];

#define ML_STRINGBUFFER_INIT (ml_stringbuffer_t){MLStringBufferT, 0,}

ssize_t ml_stringbuffer_add(ml_stringbuffer_t *Buffer, const char *String, size_t Length);
ssize_t ml_stringbuffer_addf(ml_stringbuffer_t *Buffer, const char *Format, ...) __attribute__ ((format(printf, 2, 3)));
char *ml_stringbuffer_get(ml_stringbuffer_t *Buffer);
char *ml_stringbuffer_get_uncollectable(ml_stringbuffer_t *Buffer);
ml_value_t *ml_stringbuffer_get_string(ml_stringbuffer_t *Buffer);
int ml_stringbuffer_foreach(ml_stringbuffer_t *Buffer, void *Data, int (*callback)(const char *, size_t, void *));
ml_value_t *ml_stringbuffer_append(ml_stringbuffer_t *Buffer, ml_value_t *Value);

struct ml_tuple_t {
	const ml_type_t *Type;
	size_t Size;
	ml_value_t *Values[];
};

struct ml_list_t {
	const ml_type_t *Type;
	ml_list_node_t *Head, *Tail;
	int Length;
};

struct ml_list_node_t {
	ml_list_node_t *Next, *Prev;
	ml_value_t *Value;
};

#define ml_list_head(List) ((ml_list_t *)List)->Head
#define ml_list_tail(List) ((ml_list_t *)List)->Tail

#define ML_LIST_FOREACH(LIST, NODE) \
	for (ml_list_node_t *NODE = ml_list_head(LIST); NODE; NODE = NODE->Next)

#define ML_NAMES_FOREACH(LIST, NODE) ML_LIST_FOREACH(LIST, NODE)

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

#define ml_map_head(Map) ((ml_map_t *)Map)->Head
#define ml_map_tail(Map) ((ml_map_t *)Map)->Tail

#define ML_MAP_FOREACH(MAP, NODE) \
	for (ml_map_node_t *NODE = ml_map_head(MAP); NODE; NODE = NODE->Next)

#define ML_CHECK_ARG_TYPE(N, TYPE) \
	if (!ml_is(Args[N], TYPE)) { \
		return ml_error("TypeError", "%s required", TYPE->Name); \
	}

#define ML_CHECK_ARG_COUNT(N) \
	if (Count < N) { \
		return ml_error("CallError", "%d arguments required", N); \
	}

#define ML_CHECKX_ARG_TYPE(N, TYPE) \
	if (!ml_is(Args[N], TYPE)) { \
		ML_CONTINUE(Caller, ml_error("TypeError", "%s required", TYPE->Name)); \
	}

#define ML_CHECKX_ARG_COUNT(N) \
	if (Count < N) { \
		ML_CONTINUE(Caller, ml_error("CallError", "%d arguments required", N)); \
	}

#define CONCAT2(X, Y, Z) X ## Y ## _ ## Z
#define CONCAT(X, Y, Z) CONCAT2(X, Y, Z)

#ifndef GENERATE_INIT

#define ML_METHOD(METHOD, TYPES ...) static ml_value_t *CONCAT(ml_method_fn_, __LINE__, __COUNTER__)(void *Data, int Count, ml_value_t **Args)

#define ML_METHODX(METHOD, TYPES ...) static ml_value_t *CONCAT(ml_method_fn_, __LINE__, __COUNTER__)(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args)


#else

#define ML_METHOD(METHOD, TYPES ...) ml_method_by_name(METHOD, NULL, CONCAT(ml_method_fn_, __LINE__, __COUNTER__), TYPES, NULL);

#define ML_METHODX(METHOD, TYPES ...) ml_methodx_by_name(METHOD, NULL, CONCAT(ml_method_fn_, __LINE__, __COUNTER__), TYPES, NULL);

#endif

#ifdef	__cplusplus
}
#endif

#endif

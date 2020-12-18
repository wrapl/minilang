#ifndef ML_TYPES_H
#define ML_TYPES_H

#include <unistd.h>
#include "stringmap.h"
#include "inthash.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct ml_value_t ml_value_t;
typedef struct ml_type_t ml_type_t;
typedef struct ml_context_t ml_context_t;
typedef struct ml_state_t ml_state_t;

// Macros //

#define _CONCAT2(X, Y) X ## Y
#define CONCAT2(X, Y) _CONCAT2(X, Y)

#define _CONCAT3(X, Y, Z) X ## Y ## _ ## Z
#define CONCAT3(X, Y, Z) _CONCAT3(X, Y, Z)

// Values and Types //

struct ml_value_t {
	const ml_type_t *Type;
};

typedef struct ml_hash_chain_t ml_hash_chain_t;

struct ml_hash_chain_t {
	ml_hash_chain_t *Previous;
	ml_value_t *Value;
	long Index;
};

#ifdef USE_GENERICS
typedef struct ml_type_rule_t ml_type_rule_t;

struct ml_type_rule_t {
	ml_type_rule_t *Next;
	int NumArgs;
	uintptr_t Args[];
};
#endif

struct ml_type_t {
	const ml_type_t *Type;
	const ml_type_t **Types;
	const char *Name;
	long (*hash)(ml_value_t *, ml_hash_chain_t *);
	void (*call)(ml_state_t *, ml_value_t *, int, ml_value_t **);
	ml_value_t *(*deref)(ml_value_t *);
	ml_value_t *(*assign)(ml_value_t *, ml_value_t *);
	ml_value_t *Constructor;
#ifdef USE_GENERICS
	const ml_type_t **Args;
	ml_type_rule_t *Parents;
#else
	inthash_t Parents[1];
#endif
	inthash_t TypedFns[1];
	stringmap_t Exports[1];
	int Rank;
	int Interface:1;
	int NoInherit:1;
	int Reserved:30;
#ifdef USE_GENERICS
	int NumArgs;
#endif
};

extern ml_type_t MLTypeT[];

long ml_default_hash(ml_value_t *Value, ml_hash_chain_t *Chain);
void ml_default_call(ml_state_t *Frame, ml_value_t *Value, int Count, ml_value_t **Args);
ml_value_t *ml_default_deref(ml_value_t *Ref);
ml_value_t *ml_default_assign(ml_value_t *Ref, ml_value_t *Value);

#ifndef GENERATE_INIT

#define ML_TYPE(TYPE, PARENTS, NAME, ...) \
ml_type_t TYPE[1] = {{ \
	.Type = MLTypeT, \
	.Types = NULL, \
	.Name = NAME, \
	.hash = ml_default_hash, \
	.call = ml_default_call, \
	.deref = ml_default_deref, \
	.assign = ml_default_assign, \
	.TypedFns = {INTHASH_INIT}, \
	.Exports = {STRINGMAP_INIT}, \
	.Rank = 0, \
	.Interface = 0, \
	__VA_ARGS__ \
}}

#else

#define UNWRAP(ARGS...) , ##ARGS
#define ML_TYPE(TYPE, PARENTS, NAME, ...) INIT_CODE ml_type_init(TYPE UNWRAP PARENTS, NULL);

#endif

#define ML_INTERFACE(TYPE, PARENTS, NAME, ...) ML_TYPE(TYPE, PARENTS, NAME, .Interface = 1, __VA_ARGS__)

void ml_type_init(ml_type_t *Type, ...) __attribute__ ((sentinel));

ml_type_t *ml_type(ml_type_t *Parent, const char *Name);
void *ml_typed_fn_get(const ml_type_t *Type, void *TypedFn);
void ml_typed_fn_set(ml_type_t *Type, void *TypedFn, void *Function);

#ifdef USE_GENERICS
const ml_type_t *ml_type_generic(const ml_type_t *Base, int Count, const ml_type_t **Args);
ml_value_t *ml_type_generic_fn(void *Data, int Count, ml_value_t **Args);
void ml_type_add_parent(ml_type_t *Type, ...);
#define ML_TYPE_ARG(N) ((1L << 48) + N)

#else
#define ml_type_add_parent(TYPE, PARENT, _) inthash_insert(((ml_type_t *)TYPE)->Parents, (uintptr_t)PARENT, (void *)PARENT)
#endif

int ml_is_subtype(const ml_type_t *Type1, const ml_type_t *Type2) __attribute__ ((pure));
const ml_type_t *ml_type_max(const ml_type_t *Type1, const ml_type_t *Type2);

#ifndef GENERATE_INIT

#define ML_TYPED_FN(FUNCTION, TYPE, ARGS ...) CONCAT3(FUNCTION ## _, __LINE__, __COUNTER__)(ARGS)

#else

#define ML_TYPED_FN(FUNCTION, TYPE, ARGS ...) INIT_CODE ml_typed_fn_set(TYPE, FUNCTION, (typeof(FUNCTION)*)CONCAT3(FUNCTION ## _, __LINE__, __COUNTER__));

#endif

#define ML_VALUE(NAME, TYPE) \
ml_value_t NAME[1] = {{TYPE}}

extern ml_type_t MLAnyT[];
extern ml_type_t MLNilT[];

extern ml_value_t MLNil[];
extern ml_value_t MLSome[];

long ml_hash_chain(ml_value_t *Value, ml_hash_chain_t *Chain);
long ml_hash(ml_value_t *Value);

typedef ml_value_t *(*ml_callback_t)(void *Data, int Count, ml_value_t **Args);
typedef void (*ml_callbackx_t)(ml_state_t *Frame, void *Data, int Count, ml_value_t **Args);

// Boxing //

long ml_integer_value(const ml_value_t *Value) __attribute__ ((const));
double ml_real_value(const ml_value_t *Value) __attribute__ ((const));

#ifdef USE_NANBOXING

static inline int ml_tag(const ml_value_t *Value) {
	return (uint64_t)Value >> 48;
}

static inline int ml_is_int32(ml_value_t *Value) {
	return ml_tag(Value) == 1;
}

static inline ml_value_t *ml_int32(int32_t Integer) {
	return (void *)(((uint64_t)1 << 48) + (uint32_t)Integer);
}

static inline int ml_is_double(ml_value_t *Value) {
	return ml_tag(Value) >= 7;
}

static inline double ml_to_double(const ml_value_t *Value) {
	union { const ml_value_t *Value; uint64_t Bits; double Double; } Boxed;
	Boxed.Value = Value;
	Boxed.Bits -= 0x07000000000000;
	return Boxed.Double;
}

inline long ml_integer_value_fast(const ml_value_t *Value) {
	return ml_integer_value(Value);
}

inline double ml_real_value_fast(const ml_value_t *Value) {
	return ml_real_value(Value);
}

#else

typedef struct {
	const ml_type_t *Type;
	long Value;
} ml_integer_t;

inline long ml_integer_value_fast(const ml_value_t *Value) {
	return ((ml_integer_t *)Value)->Value;
}

typedef struct {
	const ml_type_t *Type;
	double Value;
} ml_real_t;

inline double ml_real_value_fast(const ml_value_t *Value) {
	return ((ml_real_t *)Value)->Value;
}

#endif

// Iterators //

extern ml_type_t MLIteratableT[];

void ml_iterate(ml_state_t *Caller, ml_value_t *Value);
void ml_iter_value(ml_state_t *Caller, ml_value_t *Iter);
void ml_iter_key(ml_state_t *Caller, ml_value_t *Iter);
void ml_iter_next(ml_state_t *Caller, ml_value_t *Iter);

// Functions //

extern ml_type_t MLFunctionT[];

typedef struct ml_cfunction_t ml_cfunction_t;
typedef struct ml_cfunctionx_t ml_cfunctionx_t;

struct ml_cfunction_t {
	const ml_type_t *Type;
	ml_callback_t Callback;
	void *Data;
};

struct ml_cfunctionx_t {
	const ml_type_t *Type;
	ml_callbackx_t Callback;
	void *Data;
};

extern ml_type_t MLCFunctionT[];
extern ml_type_t MLCFunctionXT[];
extern ml_type_t MLPartialFunctionT[];

extern ml_cfunctionx_t MLCallCC[];
extern ml_cfunctionx_t MLMark[];

extern ml_type_t MLContextKeyT[];
extern ml_cfunction_t MLContextKey[];

ml_value_t *ml_cfunction(void *Data, ml_callback_t Function) __attribute__((malloc));
ml_value_t *ml_cfunctionx(void *Data, ml_callbackx_t Function) __attribute__((malloc));

ml_value_t *ml_return_nil(void *Data, int Count, ml_value_t **Args);
ml_value_t *ml_identity(void *Data, int Count, ml_value_t **Args);

ml_value_t *ml_partial_function_new(ml_value_t *Function, int Count) __attribute__((malloc));
ml_value_t *ml_partial_function_set(ml_value_t *Partial, size_t Index, ml_value_t *Value);

#define ML_FUNCTION2(NAME, FUNCTION) static ml_value_t *FUNCTION(void *Data, int Count, ml_value_t **Args); \
\
ml_cfunction_t NAME[1] = {{MLCFunctionT, FUNCTION, NULL}}; \
\
static ml_value_t *FUNCTION(void *Data, int Count, ml_value_t **Args)

#define ML_FUNCTION(NAME) ML_FUNCTION2(NAME, CONCAT3(ml_cfunction_, __LINE__, __COUNTER__))

#define ML_FUNCTIONX2(NAME, FUNCTION) static void FUNCTION(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args); \
\
ml_cfunctionx_t NAME[1] = {{MLCFunctionXT, FUNCTION, NULL}}; \
\
static void FUNCTION(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args)

#define ML_FUNCTIONX(NAME, TYPES ...) ML_FUNCTIONX2(NAME, CONCAT3(ml_cfunctionx_, __LINE__, __COUNTER__))

#define ML_CHECK_ARG_TYPE(N, TYPE) \
	if (!ml_is(Args[N], TYPE)) { \
		return ml_error("TypeError", "expected %s for argument %d", TYPE->Name, N + 1); \
	}

#define ML_CHECK_ARG_COUNT(N) \
	if (Count < N) { \
		return ml_error("CallError", "%d arguments required", N); \
	}

#define ML_CHECKX_ARG_TYPE(N, TYPE) \
	if (!ml_is(Args[N], TYPE)) { \
		ML_CONTINUE(Caller, ml_error("TypeError", "expected %s required for argument %d", TYPE->Name, N + 1)); \
	}

#define ML_CHECKX_ARG_COUNT(N) \
	if (Count < N) { \
		ML_CONTINUE(Caller, ml_error("CallError", "%d arguments required", N)); \
	}

#define ML_CONTINUE(STATE, VALUE) { \
	ml_state_t *__State = (ml_state_t *)(STATE); \
	ml_value_t *__Value = (ml_value_t *)(VALUE); \
	return __State->run(__State, __Value); \
}

#define ML_RETURN(VALUE) return Caller->run(Caller, (ml_value_t *)(VALUE))
#define ML_ERROR(ARGS...) ML_RETURN(ml_error(ARGS))

// Tuples //

typedef struct ml_tuple_t ml_tuple_t;

extern ml_type_t MLTupleT[];

struct ml_tuple_t {
	const ml_type_t *Type;
	int Size, NoRefs;
	ml_value_t *Values[];
};

ml_value_t *ml_tuple(size_t Size) __attribute__((malloc));

static inline int ml_tuple_size(ml_value_t *Tuple) {
	return ((ml_tuple_t *)Tuple)->Size;
}

static inline ml_value_t *ml_tuple_get(ml_value_t *Tuple, int Index) {
	return ((ml_tuple_t *)Tuple)->Values[Index - 1];
}

static inline ml_value_t *ml_tuple_set(ml_value_t *Tuple, int Index, ml_value_t *Value) {
	return ((ml_tuple_t *)Tuple)->Values[Index - 1] = Value;
}

ml_value_t *ml_unpack(ml_value_t *Value, int Index);

// Booleans //

typedef struct ml_boolean_t {
	const ml_type_t *Type;
	const char *Name;
	int Value;
} ml_boolean_t;

extern ml_type_t MLBooleanT[];
extern ml_boolean_t MLTrue[];
extern ml_boolean_t MLFalse[];

ml_value_t *ml_boolean(int Value) __attribute__ ((const));
int ml_boolean_value(const ml_value_t *Value) __attribute__ ((const));

// Numbers //

extern ml_type_t MLNumberT[];
extern ml_type_t MLIntegerT[];
extern ml_type_t MLRealT[];

#ifdef USE_NANBOXING

extern ml_type_t MLInt32T[];
extern ml_type_t MLInt64T[];
extern ml_type_t MLDoubleT[];

#endif

ml_value_t *ml_integer(long Value) __attribute__((malloc));
ml_value_t *ml_real(double Value) __attribute__((malloc));

extern ml_value_t *MLIntegerOfMethod;
extern ml_value_t *MLRealOfMethod;

// Strings //

typedef struct ml_buffer_t ml_buffer_t;
typedef struct ml_string_t ml_string_t;

struct ml_buffer_t {
	const ml_type_t *Type;
	char *Address;
	size_t Size;
};

struct ml_string_t {
	const ml_type_t *Type;
	const char *Value;
	size_t Length;
	long Hash;
};

extern ml_type_t MLBufferT[];
extern ml_type_t MLStringT[];

extern ml_type_t MLRegexT[];
extern ml_type_t MLStringBufferT[];

ml_value_t *ml_buffer(void *Data, int Count, ml_value_t **Args);

ml_value_t *ml_string(const char *Value, int Length) __attribute__((malloc));
#define ml_cstring(VALUE) ml_string(VALUE, strlen(VALUE))

ml_value_t *ml_string_format(const char *Format, ...) __attribute__((malloc, format(printf, 1, 2)));

const char *ml_string_value(const ml_value_t *Value) __attribute__((const));
size_t ml_string_length(const ml_value_t *Value) __attribute__((pure));

extern ml_value_t *MLStringOfMethod;

ml_value_t *ml_regex(const char *Value, int Length) __attribute__((malloc));
const char *ml_regex_pattern(const ml_value_t *Value) __attribute__((pure));

int ml_regex_match(ml_value_t *Value, const char *Subject, int Length);

typedef struct ml_stringbuffer_t ml_stringbuffer_t;
typedef struct ml_stringbuffer_node_t ml_stringbuffer_node_t;

struct ml_stringbuffer_t {
	const ml_type_t *Type;
	ml_stringbuffer_node_t *Head, *Tail;
	ml_hash_chain_t *Chain;
	int Space, Length;
};

#define ML_STRINGBUFFER_NODE_SIZE 248
#define ML_STRINGBUFFER_INIT (ml_stringbuffer_t){MLStringBufferT, 0,}

ml_value_t *ml_stringbuffer();
ssize_t ml_stringbuffer_add(ml_stringbuffer_t *Buffer, const char *String, size_t Length);
ssize_t ml_stringbuffer_addf(ml_stringbuffer_t *Buffer, const char *Format, ...) __attribute__ ((format(printf, 2, 3)));
char *ml_stringbuffer_get(ml_stringbuffer_t *Buffer) __attribute__ ((malloc));
char *ml_stringbuffer_get_uncollectable(ml_stringbuffer_t *Buffer) __attribute__ ((malloc));
ml_value_t *ml_stringbuffer_value(ml_stringbuffer_t *Buffer) __attribute__ ((malloc));
int ml_stringbuffer_foreach(ml_stringbuffer_t *Buffer, void *Data, int (*callback)(void *, const char *, size_t));
ml_value_t *ml_stringbuffer_append(ml_stringbuffer_t *Buffer, ml_value_t *Value);


// Lists //

typedef struct ml_list_node_t ml_list_node_t;
typedef struct ml_list_t ml_list_t;

extern ml_type_t MLListT[];

struct ml_list_node_t {
	const ml_type_t *Type;
	ml_list_node_t *Next, *Prev;
	ml_value_t *Value;
	int Index;
};

struct ml_list_t {
	const ml_type_t *Type;
	ml_list_node_t *Head, *Tail;
	ml_list_node_t *CachedNode;
	int Length, CachedIndex;
	int ValidIndices;
};

ml_value_t *ml_list() __attribute__((malloc));
void ml_list_grow(ml_value_t *List, int Count);
void ml_list_push(ml_value_t *List, ml_value_t *Value);
void ml_list_put(ml_value_t *List, ml_value_t *Value);
ml_value_t *ml_list_pop(ml_value_t *List);
ml_value_t *ml_list_pull(ml_value_t *List);

ml_value_t *ml_list_get(ml_value_t *List, int Index);
ml_value_t *ml_list_set(ml_value_t *List, int Index, ml_value_t *Value);

#define ml_list_append ml_list_put

void ml_list_to_array(ml_value_t *List, ml_value_t **Array);
int ml_list_foreach(ml_value_t *List, void *Data, int (*callback)(ml_value_t *, void *));

static inline int ml_list_length(ml_value_t *List) {
	return ((ml_list_t *)List)->Length;
}

typedef struct {
	ml_list_node_t *Node;
	ml_value_t *Value;
} ml_list_iter_t;

static inline int ml_list_iter_forward(ml_value_t *List0, ml_list_iter_t *Iter) {
	ml_list_t *List = (ml_list_t *)List0;
	if ((Iter->Node = List->Head)) {
		Iter->Value = Iter->Node->Value;
		return 1;
	} else {
		Iter->Node = NULL;
		return 0;
	}
}

static inline int ml_list_iter_next(ml_list_iter_t *Iter) {
	if ((Iter->Node = Iter->Node->Next)) {
		Iter->Value = Iter->Node->Value;
		return 1;
	} else {
		return 0;
	}
}

static inline int ml_list_iter_backward(ml_value_t *List0, ml_list_iter_t *Iter) {
	ml_list_t *List = (ml_list_t *)List0;
	if ((Iter->Node = List->Tail)) {
		Iter->Value = Iter->Node->Value;
		return 1;
	} else {
		Iter->Node = NULL;
		return 0;
	}
}

static inline int ml_list_iter_prev(ml_list_iter_t *Iter) {
	if ((Iter->Node = Iter->Node->Prev)) {
		Iter->Value = Iter->Node->Value;
		return 1;
	} else {
		return 0;
	}
}

static inline int ml_list_iter_valid(ml_list_iter_t *Iter) {
	return Iter->Node != NULL;
}

static inline void ml_list_iter_update(ml_list_iter_t *Iter, ml_value_t *Value) {
	Iter->Value = Iter->Node->Value = Value;
}

#define ML_LIST_FOREACH(LIST, ITER) \
	for (ml_list_node_t *ITER = ((ml_list_t *)LIST)->Head; ITER; ITER = ITER->Next)

#define ML_LIST_REVERSE(LIST, ITER) \
	for (ml_list_node_t *ITER = ((ml_list_t *)LIST)->Tail; ITER; ITER = ITER->Prev)

// Maps //

typedef struct ml_map_t ml_map_t;
typedef struct ml_map_node_t ml_map_node_t;

extern ml_type_t MLMapT[];

struct ml_map_t {
	const ml_type_t *Type;
	ml_map_node_t *Head, *Tail, *Root;
	int Size;
};

struct ml_map_node_t {
	const ml_type_t *Type;
	ml_map_node_t *Next, *Prev;
	ml_value_t *Key;
	ml_map_node_t *Left, *Right;
	ml_value_t *Value;
	long Hash;
	int Depth;
};

ml_value_t *ml_map() __attribute__((malloc));
ml_value_t *ml_map_search(ml_value_t *Map, ml_value_t *Key);
ml_map_node_t *ml_map_slot(ml_value_t *Map, ml_value_t *Key);
ml_value_t *ml_map_insert(ml_value_t *Map, ml_value_t *Key, ml_value_t *Value);
ml_value_t *ml_map_delete(ml_value_t *Map, ml_value_t *Key);

static inline int ml_map_size(ml_value_t *Map) {
	return ((ml_map_t *)Map)->Size;
}

int ml_map_foreach(ml_value_t *Map, void *Data, int (*callback)(ml_value_t *, ml_value_t *, void *));

typedef struct {
	ml_map_node_t *Node;
	ml_value_t *Key, *Value;
} ml_map_iter_t;

static inline int ml_map_iter_forward(ml_value_t *Map0, ml_map_iter_t *Iter) {
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

static inline int ml_map_iter_next(ml_map_iter_t *Iter) {
	ml_map_node_t *Node = Iter->Node = Iter->Node->Next;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		Iter->Value = Node->Value;
		return 1;
	}
}

static inline int ml_map_iter_backward(ml_value_t *Map0, ml_map_iter_t *Iter) {
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

static inline int ml_map_iter_prev(ml_map_iter_t *Iter) {
	ml_map_node_t *Node = Iter->Node = Iter->Node->Prev;
	if (!Node) {
		return 0;
	} else {
		Iter->Key = Node->Key;
		Iter->Value = Node->Value;
		return 1;
	}
}

static inline int ml_map_iter_valid(ml_map_iter_t *Iter) {
	return Iter->Node != NULL;
}

static inline void ml_map_iter_update(ml_map_iter_t *Iter, ml_value_t *Value) {
	Iter->Value = Iter->Node->Value = Value;
}

#define ML_MAP_FOREACH(LIST, ITER) \
	for (ml_map_node_t *ITER = ((ml_map_t *)LIST)->Head; ITER; ITER = ITER->Next)

// Names //

extern ml_type_t MLNamesT[];

static inline ml_value_t *ml_names() {
	ml_value_t *Names = ml_list();
	Names->Type = MLNamesT;
	return Names;
}

static inline void ml_names_add(ml_value_t *Names, ml_value_t *Value) {
	ml_list_put(Names, Value);
}

#define ML_NAMES_FOREACH(LIST, ITER) ML_LIST_FOREACH(LIST, ITER)

// Methods //

typedef struct ml_method_t ml_method_t;
typedef struct ml_methods_t ml_methods_t;

#define ML_METHODS_INDEX 0

extern ml_type_t MLMethodT[];

ml_value_t *ml_method(const char *Name);
ml_value_t *ml_method_anon(const char *Name);
const char *ml_method_name(const ml_value_t *Value) __attribute__((pure));

void ml_method_by_name(const char *Method, void *Data, ml_callback_t Function, ...) __attribute__ ((sentinel));
void ml_method_by_value(ml_value_t *Method, void *Data, ml_callback_t Function, ...) __attribute__ ((sentinel));

void ml_methodx_by_name(const char *Method, void *Data, ml_callbackx_t Function, ...) __attribute__ ((sentinel));
void ml_methodx_by_value(ml_value_t *Method, void *Data, ml_callbackx_t Function, ...) __attribute__ ((sentinel));

void ml_method_define(ml_value_t *Method, ml_value_t *Function, int Variadic, ...);
void ml_method_insert(ml_methods_t *Methods, ml_method_t *Method, ml_value_t *Callback, int Count, int Variadic, ml_type_t **Types);

void ml_method_by_array(ml_value_t *Value, ml_value_t *Function, int Count, ml_type_t **Types);

#ifndef GENERATE_INIT

#define ML_METHOD(METHOD, TYPES ...) static ml_value_t *CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)(void *Data, int Count, ml_value_t **Args)

#define ML_METHODX(METHOD, TYPES ...) static void CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args)

#define ML_METHODV(METHOD, TYPES ...) static ml_value_t *CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)(void *Data, int Count, ml_value_t **Args)

#define ML_METHODVX(METHOD, TYPES ...) static void CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)(ml_state_t *Caller, void *Data, int Count, ml_value_t **Args)

static inline ml_value_t *ml_nop(ml_value_t *Value) {
	return Value;
}

#define ML_METHOD_DECL(NAME, METHOD) ml_value_t *NAME ## Method
#define ML_METHOD_ANON(NAME, METHOD) ml_value_t *NAME ## Method

#else

#ifndef __cplusplus

#define ML_METHOD(METHOD, TYPES ...) INIT_CODE ml_method_define(_Generic(METHOD, char *: ml_method, ml_value_t *: ml_nop)(METHOD), ml_cfunction(NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)), 0, ##TYPES, NULL);

#define ML_METHODX(METHOD, TYPES ...) INIT_CODE ml_method_define(_Generic(METHOD, char *: ml_method, ml_value_t *: ml_nop)(METHOD), ml_cfunctionx(NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)), 0, ##TYPES, NULL);

#define ML_METHODV(METHOD, TYPES ...) INIT_CODE ml_method_define(_Generic(METHOD, char *: ml_method, ml_value_t *: ml_nop)(METHOD), ml_cfunction(NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)), 1, ##TYPES, NULL);

#define ML_METHODVX(METHOD, TYPES ...) INIT_CODE ml_method_define(_Generic(METHOD, char *: ml_method, ml_value_t *: ml_nop)(METHOD), ml_cfunctionx(NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__)), 1, ##TYPES, NULL);

#else

#define ML_METHOD(METHOD, TYPES ...) INIT_CODE ml_method_by_auto(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, (void *)NULL);

#define ML_METHODX(METHOD, TYPES ...) INIT_CODE ml_methodx_by_auto(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, (void *)NULL);

#define ML_METHODV(METHOD, TYPES ...) INIT_CODE ml_method_by_auto(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, (void *)NULL);

#define ML_METHODVX(METHOD, TYPES ...) INIT_CODE ml_methodx_by_auto(METHOD, NULL, CONCAT3(ml_method_fn_, __LINE__, __COUNTER__), TYPES, (void *)NULL);

#endif

#define ML_METHOD_DECL(NAME, METHOD) INIT_CODE NAME ## Method = ml_method(METHOD);
#define ML_METHOD_ANON(NAME, METHOD) INIT_CODE NAME ## Method = ml_method_anon(METHOD);

#endif

void ml_methods_context_new(ml_context_t *Context);

// Modules //

extern ml_type_t MLModuleT[];

ml_value_t *ml_module(const char *Path, ...) __attribute__ ((malloc, sentinel));
const char *ml_module_path(ml_value_t *Module) __attribute__ ((pure));
ml_value_t *ml_module_import(ml_value_t *Module, const char *Name) __attribute__ ((pure));
ml_value_t *ml_module_export(ml_value_t *Module, const char *Name, ml_value_t *Value);

// Init //

#ifdef USE_NANBOXING

static inline const ml_type_t *ml_typeof(const ml_value_t *Value) {
	unsigned Tag = ml_tag(Value);
	if (__builtin_expect(Tag == 0, 1)) {
		return Value->Type;
	} else if (Tag == 1) {
		return MLInt32T;
	} else if (Tag < 7) {
		return NULL;
	} else {
		return MLDoubleT;
	}
}

static inline ml_value_t *ml_deref(ml_value_t *Value) {
	unsigned Tag = ml_tag(Value);
	if (__builtin_expect(Tag == 0, 1)) {
		return Value->Type->deref(Value);
	} else {
		return Value;
	}
}

#else

static inline const ml_type_t *ml_typeof(const ml_value_t *Value) {
	return Value->Type;
}

static inline ml_value_t *ml_deref(ml_value_t *Value) {
	return Value->Type->deref(Value);
}

#endif

static inline int ml_is(const ml_value_t *Value, const ml_type_t *Expected) {
	const ml_type_t *Type = ml_typeof(Value);
	if (Type == Expected) return 1;
#ifdef USE_GENERICS
	return ml_is_subtype(Type, Expected);
#else
	if (inthash_search(Type->Parents, (uintptr_t)Expected)) return 1;
#endif
	return 0;
}

static inline ml_value_t *ml_assign(ml_value_t *Value, ml_value_t *Value2) {
	return ml_typeof(Value)->assign(Value, Value2);
}

static inline void ml_call(void *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	return ml_typeof(Value)->call((ml_state_t *)Caller, Value, Count, Args);
}

#define ml_inline(STATE, VALUE, COUNT, ARGS ...) ({ \
	void *Args ## __LINE__[COUNT] = {ARGS}; \
	ml_call(STATE, VALUE, COUNT, (ml_value_t **)(Args ## __LINE__)); \
})

void ml_types_init(stringmap_t *Globals);

#ifdef	__cplusplus
}

template <typename... args> void ml_method_by_auto(const char *Method, void *Data, ml_callback_t Function, args... Args) {
	ml_method_define(ml_method(Method), ml_cfunction(Data, Function), 0, Args...);
}

template <typename... args> void ml_method_by_auto(ml_value_t *Method, void *Data, ml_callback_t Function, args... Args) {
	ml_method_define(Method, ml_cfunction(Data, Function), 0, Args...);
}

template <typename... args> void ml_methodx_by_auto(const char *Method, void *Data, ml_callbackx_t Function, args... Args) {
	ml_method_define(ml_method(Method), ml_cfunctionx(Data, Function), 0, Args...);
}

template <typename... args> void ml_methodx_by_auto(ml_value_t *Method, void *Data, ml_callbackx_t Function, args... Args) {
	ml_methodx_define(Method, ml_cfunctionx(Data, Function), 0, Args...);
}

#endif

#endif

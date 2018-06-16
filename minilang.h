#ifndef MINILANG_H
#define MINILANG_H

#include "sha256.h"
#include <stdlib.h>
#include <unistd.h>

typedef struct ml_type_t ml_type_t;
typedef struct ml_value_t ml_value_t;
typedef struct ml_function_t ml_function_t;

typedef ml_value_t *(*ml_callback_t)(void *Data, int Count, ml_value_t **Args);

typedef ml_value_t *(*ml_getter_t)(void *Data, const char *Name);
typedef ml_value_t *(*ml_setter_t)(void *Data, const char *Name, ml_value_t *Value);

void ml_init();

long ml_hash(ml_value_t *Value);

ml_type_t *ml_class(ml_type_t *Parent, const char *Name);

ml_value_t *ml_load(ml_getter_t GlobalGet, void *Globals, const char *FileName);
void ml_console(ml_getter_t GlobalGet, void *Globals);
ml_value_t *ml_call(ml_value_t *Value, int Count, ml_value_t **Args);

ml_value_t *ml_inline(ml_value_t *Value, int Count, ...);

void ml_method_by_name(const char *Method, void *Data, ml_callback_t Function, ...);
void ml_method_by_value(ml_value_t *Method, void *Data, ml_callback_t Function, ...);

ml_value_t *ml_string(const char *Value, int Length);
ml_value_t *ml_integer(long Value);
ml_value_t *ml_real(double Value);
ml_value_t *ml_list();
ml_value_t *ml_tree();
ml_value_t *ml_function(void *Data, ml_callback_t Function);
ml_value_t *ml_property(void *Data, const char *Name, ml_getter_t Get, ml_setter_t Set, ml_getter_t Next, ml_getter_t Key);
ml_value_t *ml_error(const char *Error, const char *Format, ...);
ml_value_t *ml_reference(ml_value_t **Address);
ml_value_t *ml_method(const char *Name);

long ml_integer_value(ml_value_t *Value);
double ml_real_value(ml_value_t *Value);
const char *ml_string_value(ml_value_t *Value);
int ml_string_length(ml_value_t *Value);

const char *ml_error_type(ml_value_t *Value);
const char *ml_error_message(ml_value_t *Value);
int ml_error_trace(ml_value_t *Value, int Level, const char **Source, int *Line);

void ml_closure_hash(ml_value_t *Closure, unsigned char Hash[SHA256_BLOCK_SIZE]);

void ml_list_append(ml_value_t *List, ml_value_t *Value);
int ml_list_length(ml_value_t *List);
void ml_list_to_array(ml_value_t *List, ml_value_t **Array);
int ml_list_foreach(ml_value_t *List, void *Data, int (*callback)(ml_value_t *, void *));
int ml_tree_foreach(ml_value_t *Tree, void *Data, int (*callback)(ml_value_t *, ml_value_t *, void *));

struct ml_type_t {
	const ml_type_t *Parent;
	const char *Name;
	long (*hash)(ml_value_t *);
	ml_value_t *(*call)(ml_value_t *, int, ml_value_t **);
	ml_value_t *(*deref)(ml_value_t *);
	ml_value_t *(*assign)(ml_value_t *, ml_value_t *);
	ml_value_t *(*next)(ml_value_t *);
	ml_value_t *(*key)(ml_value_t *);
};

long ml_default_hash(ml_value_t *Value);
ml_value_t *ml_default_call(ml_value_t *Value, int Count, ml_value_t **Args);
ml_value_t *ml_default_deref(ml_value_t *Ref);
ml_value_t *ml_default_assign(ml_value_t *Ref, ml_value_t *Value);
ml_value_t *ml_default_next(ml_value_t *Iter);
ml_value_t *ml_default_key(ml_value_t *Iter);

extern ml_type_t MLAnyT[];
extern ml_type_t MLNilT[];
extern ml_type_t MLFunctionT[];
extern ml_type_t MLNumberT[];
extern ml_type_t MLIntegerT[];
extern ml_type_t MLRealT[];
extern ml_type_t MLStringT[];
extern ml_type_t MLMethodT[];
extern ml_type_t MLReferenceT[];
extern ml_type_t MLListT[];
extern ml_type_t MLTreeT[];
extern ml_type_t MLPropertyT[];
extern ml_type_t MLClosureT[];
extern ml_type_t MLErrorT[];

struct ml_value_t {
	const ml_type_t *Type;
};

extern ml_value_t MLNil[];
extern ml_value_t MLSome[];

int ml_is(ml_value_t *Value, ml_type_t *Type);

struct ml_function_t {
	const ml_type_t *Type;
	ml_callback_t Callback;
	void *Data;
};

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
ssize_t ml_stringbuffer_addf(ml_stringbuffer_t *Buffer, const char *Format, ...);
char *ml_stringbuffer_get(ml_stringbuffer_t *Buffer);
int ml_stringbuffer_foreach(ml_stringbuffer_t *Buffer, void *Data, int (*callback)(const char *, size_t, void *));

#define new(T) ((T *)GC_MALLOC(sizeof(T)))
#define anew(T, N) ((T *)GC_MALLOC((N) * sizeof(T)))
#define snew(N) ((char *)GC_MALLOC_ATOMIC(N))
#define xnew(T, N, U) ((T *)GC_MALLOC(sizeof(T) + (N) * sizeof(U)))
#define fnew(T) ((T *)GC_MALLOC_STUBBORN(sizeof(T)))

#define PP_NARG(...) \
	PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
	PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
	_1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
	_11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
	_21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
	_31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
	_41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
	_51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
	_61,_62,_63,N,...) N
#define PP_RSEQ_N() \
	63,62,61,60,                   \
	59,58,57,56,55,54,53,52,51,50, \
	49,48,47,46,45,44,43,42,41,40, \
	39,38,37,36,35,34,33,32,31,30, \
	29,28,27,26,25,24,23,22,21,20, \
	19,18,17,16,15,14,13,12,11,10, \
	9,8,7,6,5,4,3,2,1,0


#define ML_CHECK_ARG_TYPE(N, TYPE) \
	if (Args[N]->Type != TYPE) return ml_error("TypeError", "%s required", TYPE->Name);

#define ML_CHECK_ARG_COUNT(N) \
	if (Count < N) return ml_error("CallError", "%d arguments required", N);

#endif

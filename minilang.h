#ifndef MINILANG_H
#define MINILANG_H

#include "sha256.h"
#include <stdlib.h>

typedef struct ml_type_t ml_type_t;
typedef struct ml_value_t ml_value_t;
typedef struct ml_function_t ml_function_t;

typedef ml_value_t *(*ml_callback_t)(void *Data, int Count, ml_value_t **Args);

typedef ml_value_t *(*ml_getter_t)(void *Data, const char *Name);
typedef ml_value_t *(*ml_setter_t)(void *Data, const char *Name, ml_value_t *Value);

void ml_init();

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

extern ml_type_t AnyT[];
extern ml_type_t NilT[];
extern ml_type_t FunctionT[];
extern ml_type_t IntegerT[];
extern ml_type_t RealT[];
extern ml_type_t StringT[];
extern ml_type_t MethodT[];
extern ml_type_t ReferenceT[];
extern ml_type_t ListT[];
extern ml_type_t TreeT[];
extern ml_type_t ObjectT[];
extern ml_type_t PropertyT[];
extern ml_type_t ClosureT[];
extern ml_type_t ErrorT[];

struct ml_value_t {
	const ml_type_t *Type;
};

extern ml_value_t Nil[];
extern ml_value_t Some[];

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

extern ml_type_t StringBufferT[1];

#define ML_STRINGBUFFER_INIT (ml_stringbuffer_t){StringBufferT, 0,}

ssize_t ml_stringbuffer_add(ml_stringbuffer_t *Buffer, const char *String, size_t Length);
ssize_t ml_stringbuffer_addf(ml_stringbuffer_t *Buffer, const char *Format, ...);
char *ml_stringbuffer_get(ml_stringbuffer_t *Buffer);
int ml_stringbuffer_foreach(ml_stringbuffer_t *Buffer, void *Data, int (*callback)(const char *, size_t, void *));

#define new(T) ((T *)GC_MALLOC(sizeof(T)))
#define anew(T, N) ((T *)GC_MALLOC((N) * sizeof(T)))
#define snew(N) ((char *)GC_MALLOC_ATOMIC(N))
#define xnew(T, N, U) ((T *)GC_MALLOC(sizeof(T) + (N) * sizeof(U)))
#define fnew(T) ((T *)GC_MALLOC_STUBBORN(sizeof(T)))

#endif

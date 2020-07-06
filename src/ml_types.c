#include "minilang.h"
#include "ml_macros.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <gc.h>
#include <gc/gc_typed.h>
#include <regex.h>
#include <pthread.h>
#include <limits.h>
#include <math.h>
#include <inttypes.h>
#include "ml_runtime.h"
#include "ml_bytecode.h"

ML_METHOD_DECL(Iterate, "iterate");
ML_METHOD_DECL(Value, "value");
ML_METHOD_DECL(Key, "key");
ML_METHOD_DECL(Next, "next");
ML_METHOD_DECL(Compare, "<>");
ML_METHOD_DECL(Index, "[]");
ML_METHOD_DECL(Symbol, "::");

ML_METHOD_DECL(MLStringOf, NULL);
ML_METHOD_DECL(MLStringBufferAppend, NULL);
ML_METHOD_DECL(MLBooleanOf, NULL);
ML_METHOD_DECL(MLIntegerOf, NULL);
ML_METHOD_DECL(MLRealOf, NULL);
ML_METHOD_DECL(MLMethodOf, NULL);
ML_METHOD_DECL(MLListOf, NULL);
ML_METHOD_DECL(MLMapOf, NULL);

/****************************** Types ******************************/

ML_INTERFACE(MLAnyT, (), "any");

static void ml_type_call(ml_state_t *Caller, ml_type_t *Type, int Count, ml_value_t **Args) {
	ml_value_t *Constructor = stringmap_search(Type->Exports, "of");
	if (!Constructor) ML_RETURN(ml_error("TypeError", "No constructor for <%s>", Type->Name));
	return Constructor->Type->call(Caller, Constructor, Count, Args);
}

ML_INTERFACE(MLTypeT, (), "type",
	.call = (void *)ml_type_call
);

ML_METHOD("rank", MLTypeT) {
	ml_type_t *Type = (ml_type_t *)Args[0];
	return ml_integer(Type->Rank);
}

void ml_default_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	ML_RETURN(ml_error("TypeError", "<%s> is not callable", Value->Type->Name));
}

long ml_default_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	long Hash = 5381;
	for (const char *P = Value->Type->Name; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

ml_value_t *ml_default_deref(ml_value_t *Ref) {
	return Ref;
}

ml_value_t *ml_default_assign(ml_value_t *Ref, ml_value_t *Value) {
	return ml_error("TypeError", "<%s> is not assignable", Value->Type->Name);
}

void ml_type_init(ml_type_t *Type, ...) {
	int NumParents = 0;
	int Rank = Type->Rank;
	va_list Args;
	va_start(Args, Type);
	ml_type_t *Parent;
	while ((Parent = va_arg(Args, ml_type_t *))) {
		if (Rank < Parent->Rank) Rank = Parent->Rank;
		const ml_type_t **Types = Parent->Types;
		if (!Types) {
			printf("Types initialized in wrong order %s < %s\n", Type->Name, Parent->Name);
			asm("int3");
		}
		do ++NumParents; while (*++Types != MLAnyT);
	}
	va_end(Args);
	const ml_type_t **Parents = anew(const ml_type_t *, NumParents + 3);
	Type->Types = Parents;
	Type->Rank = Rank + 1;
	*Parents++ = Type;
	va_start(Args, Type);
	while ((Parent = va_arg(Args, ml_type_t *))) {
		const ml_type_t **Types = Parent->Types;
		while (*Types != MLAnyT) *Parents++ = *Types++;
	}
	va_end(Args);
	*Parents++ = MLAnyT;
}

ml_type_t *ml_type(ml_type_t *Parent, const char *Name) {
	ml_type_t *Type = new(ml_type_t);
	Type->Type = MLTypeT;
	ml_type_init(Type, Parent, NULL);
	Type->Name = Name;
	Type->hash = Parent->hash;
	Type->call = Parent->call;
	Type->deref = Parent->deref;
	Type->assign = Parent->assign;
	return Type;
}

inline void *ml_typed_fn_get(const ml_type_t *Type, void *TypedFn) {
	for (const ml_type_t **Parents = Type->Types, *Type = Parents[0]; Type; Type = *++Parents) {
		void *Function = inthash_search(Type->TypedFns, (uintptr_t)TypedFn);
		if (Function) return Function;
	}
	return NULL;
}

void ml_typed_fn_set(ml_type_t *Type, void *TypedFn, void *Function) {
	inthash_insert(Type->TypedFns, (uintptr_t)TypedFn, Function);
}

ML_METHOD(MLStringOfMethod, MLTypeT) {
	ml_type_t *Type = (ml_type_t *)Args[0];
	return ml_string_format("<<%s>>", Type->Name);
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLTypeT, ml_type_t *Type) {
	return ml_string_format("<<%s>>", Type->Name);
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLTypeT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_type_t *Type = (ml_type_t *)Args[1];
	ml_stringbuffer_add(Buffer, Type->Name, strlen(Type->Name));
	return Args[0];
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLTypeT, ml_stringbuffer_t *Buffer, ml_type_t *Type) {
	ml_stringbuffer_add(Buffer, Type->Name, strlen(Type->Name));
	return MLSome;
}

ML_METHOD("::", MLTypeT, MLStringT) {
	ml_type_t *Type = (ml_type_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t *Value = stringmap_search(Type->Exports, Name) ?: ml_error("ModuleError", "Symbol %s not exported from type %s", Name, Type->Name);
	return Value;
}

/****************************** Values ******************************/

ML_TYPE(MLNilT, (), "nil");
ML_TYPE(MLSomeT, (), "some");
ML_TYPE(MLBlankT, (), "blank");

ml_value_t MLNil[1] = {{MLNilT}};
ml_value_t MLSome[1] = {{MLSomeT}};
ml_value_t MLBlank[1] = {{MLBlankT}};

int ml_is(const ml_value_t *Value, const ml_type_t *Expected) {
	for (const ml_type_t **Parents = Value->Type->Types, *Type = Parents[0]; Type; Type = *++Parents) {
		if (Type == Expected) return 1;
	}
	return 0;
}

ML_FUNCTION(MLTypeOf) {
	ML_CHECK_ARG_COUNT(1);
	return (ml_value_t *)Args[0]->Type;
}

ML_METHOD("?", MLAnyT) {
	return (ml_value_t *)Args[0]->Type;
}

ML_METHOD("isa", MLAnyT, MLTypeT) {
	return ml_is(Args[0], (ml_type_t *)Args[1]) ? Args[0] : MLNil;
}

long ml_hash_chain(ml_value_t *Value, ml_hash_chain_t *Chain) {
	Value = Value->Type->deref(Value);
	for (ml_hash_chain_t *Link = Chain; Link; Link = Link->Previous) {
		if (Link->Value == Value) return Link->Index;
	}
	ml_hash_chain_t NewChain[1] = {{Chain, Value, Chain ? Chain->Index + 1 : 1}};
	return Value->Type->hash(Value, NewChain);
}

long ml_hash(ml_value_t *Value) {
	return ml_hash_chain(Value, NULL);
}

typedef struct ml_integer_t {
	const ml_type_t *Type;
	long Value;
} ml_integer_t;

static ml_integer_t One[1] = {{MLIntegerT, 1}};
static ml_integer_t NegOne[1] = {{MLIntegerT, -1}};
static ml_integer_t Zero[1] = {{MLIntegerT, 0}};

ML_METHOD("<>", MLAnyT, MLAnyT) {
	if (Args[0] < Args[1]) return (ml_value_t *)NegOne;
	if (Args[0] > Args[1]) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("#", MLAnyT) {
	ml_value_t *Value = Args[0];
	return ml_integer(Value->Type->hash(Value, NULL));
}

ML_METHOD("=", MLAnyT, MLAnyT) {
	return (Args[0] == Args[1]) ? Args[1] : MLNil;
}

ML_METHOD("!=", MLAnyT, MLAnyT) {
	return (Args[0] != Args[1]) ? Args[1] : MLNil;
}

ML_METHOD(MLStringOfMethod, MLAnyT) {
	return ml_string_format("<%s>", Args[0]->Type->Name);
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLAnyT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_addf(Buffer, "<%s>", Args[1]->Type->Name);
	return MLSome;
}

/****************************** Iterators ******************************/

ML_INTERFACE(MLIteratableT, (), "iteratable");

void ml_iterate(ml_state_t *Caller, ml_value_t *Value) {
	typeof(ml_iterate) *function = ml_typed_fn_get(Value->Type, ml_iterate);
	if (!function) {
		ml_value_t **Args = anew(ml_value_t *, 1);
		Args[0] = Value;
		return IterateMethod->Type->call(Caller, IterateMethod, 1, Args);
	}
	return function(Caller, Value);
}

void ml_iter_value(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_value) *function = ml_typed_fn_get(Iter->Type, ml_iter_value);
	if (!function) {
		ml_value_t **Args = anew(ml_value_t *, 1);
		Args[0] = Iter;
		return ValueMethod->Type->call(Caller, ValueMethod, 1, Args);
	}
	return function(Caller, Iter);
}

void ml_iter_key(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_key) *function = ml_typed_fn_get(Iter->Type, ml_iter_key);
	if (!function) {
		ml_value_t **Args = anew(ml_value_t *, 1);
		Args[0] = Iter;
		return KeyMethod->Type->call(Caller, KeyMethod, 1, Args);
	}
	return function(Caller, Iter);
}

void ml_iter_next(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_next) *function = ml_typed_fn_get(Iter->Type, ml_iter_next);
	if (!function) {
		ml_value_t **Args = anew(ml_value_t *, 1);
		Args[0] = Iter;
		return NextMethod->Type->call(Caller, NextMethod, 1, Args);
	}
	return function(Caller, Iter);
}

/****************************** Functions ******************************/

ML_INTERFACE(MLFunctionT, (MLIteratableT), "function");

ML_METHODX("!", MLFunctionT, MLListT) {
	ml_list_t *List = (ml_list_t *)Args[1];
	ml_value_t **ListArgs = anew(ml_value_t *, List->Length);
	ml_value_t **Arg = ListArgs;
	ML_LIST_FOREACH(List, Node) *(Arg++) = Node->Value;
	ml_value_t *Function = Args[0];
	return Function->Type->call(Caller, Function, Arg - ListArgs, ListArgs);
}

ML_METHODX("!", MLFunctionT, MLMapT) {
	ml_map_t *Map = (ml_map_t *)Args[1];
	ml_value_t **ListArgs = anew(ml_value_t *, Map->Size + 1);
	ml_value_t *Names = ml_map();
	Names->Type = MLNamesT;
	ml_value_t **Arg = ListArgs;
	*(Arg++) = Names;
	ML_MAP_FOREACH(Map, Node) {
		ml_value_t *Name = Node->Key;
		if (Name->Type == MLMethodT) {
			ml_map_insert(Names, Name, ml_integer(Arg - ListArgs));
		} else if (Name->Type == MLStringT) {
			ml_map_insert(Names, ml_method(ml_string_value(Name)), ml_integer(Arg - ListArgs));
		} else {
			ML_RETURN(ml_error("TypeError", "Parameter names must be strings or methods"));
		}
		*(Arg++) = Node->Value;
	}
	ml_value_t *Function = Args[0];
	return Function->Type->call(Caller, Function, Arg - ListArgs, ListArgs);
}

ML_METHODX("!", MLFunctionT, MLListT, MLMapT) {
	ml_list_t *List = (ml_list_t *)Args[1];
	ml_map_t *Map = (ml_map_t *)Args[2];
	ml_value_t **ListArgs = anew(ml_value_t *, List->Length + Map->Size + 1);
	ml_value_t **Arg = ListArgs;
	ML_LIST_FOREACH(List, Node) *(Arg++) = Node->Value;
	ml_value_t *Names = ml_map();
	Names->Type = MLNamesT;
	*(Arg++) = Names;
	ML_MAP_FOREACH(Map, Node) {
		ml_value_t *Name = Node->Key;
		if (Name->Type == MLMethodT) {
			ml_map_insert(Names, Name, ml_integer(Arg - ListArgs));
		} else if (Name->Type == MLStringT) {
			ml_map_insert(Names, ml_method(ml_string_value(Name)), ml_integer(Arg - ListArgs));
		} else {
			ML_RETURN(ml_error("TypeError", "Parameter names must be strings or methods"));
		}
		*(Arg++) = Node->Value;
	}
	ml_value_t *Function = Args[0];
	return Function->Type->call(Caller, Function, Arg - ListArgs, ListArgs);
}

static void ml_cfunction_call(ml_state_t *Caller, ml_cfunction_t *Function, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I] = Args[I]->Type->deref(Args[I]);
		//if (Arg->Type == MLErrorT) ML_RETURN(Arg);
	}
	ML_RETURN((Function->Callback)(Function->Data, Count, Args));
}

ML_TYPE(MLCFunctionT, (MLFunctionT), "c-function",
	.call = (void *)ml_cfunction_call
);

ml_value_t *ml_cfunction(void *Data, ml_callback_t Callback) {
	ml_cfunction_t *Function = new(ml_cfunction_t);
	Function->Type = MLCFunctionT;
	Function->Data = Data;
	Function->Callback = Callback;
	return (ml_value_t *)Function;
}

static void ML_TYPED_FN(ml_iterate, MLCFunctionT, ml_state_t *Caller, ml_cfunction_t *Function) {
	ML_RETURN((Function->Callback)(Function->Data, 0, NULL));
}

static void ml_cfunctionx_call(ml_state_t *Caller, ml_cfunctionx_t *Function, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I] = Args[I]->Type->deref(Args[I]);
		//if (Arg->Type == MLErrorT) ML_RETURN(Arg);
	}
	return (Function->Callback)(Caller, Function->Data, Count, Args);
}

ML_TYPE(MLCFunctionXT, (MLFunctionT), "c-functionx",
	.call = (void *)ml_cfunctionx_call
);

ml_value_t *ml_cfunctionx(void *Data, ml_callbackx_t Callback) {
	ml_cfunctionx_t *Function = new(ml_cfunctionx_t);
	Function->Type = MLCFunctionXT;
	Function->Data = Data;
	Function->Callback = Callback;
	return (ml_value_t *)Function;
}

ml_value_t *ml_return_nil(void *Data, int Count, ml_value_t **Args) {
	return MLNil;
}

ml_value_t *ml_identity(void *Data, int Count, ml_value_t **Args) {
	return Args[0];
}

typedef struct ml_partial_function_t {
	const ml_type_t *Type;
	ml_value_t *Function;
	int Count, Set;
	ml_value_t *Args[];
} ml_partial_function_t;

static void ml_partial_function_call(ml_state_t *Caller, ml_partial_function_t *Partial, int Count, ml_value_t **Args) {
	int CombinedCount = Count + Partial->Set;
	ml_value_t **CombinedArgs = anew(ml_value_t *, CombinedCount);
	int J = 0;
	for (int I = 0; I < Partial->Count; ++I) {
		ml_value_t *Arg = Partial->Args[I];
		if (Arg) {
			CombinedArgs[I] = Arg;
		} else if (J < Count) {
			CombinedArgs[I] = Args[J++];
		} else {
			CombinedArgs[I] = MLNil;
		}
	}
	memcpy(CombinedArgs + Partial->Count, Args + J, (Count - J) * sizeof(ml_value_t *));
	return Partial->Function->Type->call(Caller, Partial->Function, CombinedCount, CombinedArgs);
}

ML_TYPE(MLPartialFunctionT, (MLFunctionT), "partial-function",
	.call = (void *)ml_partial_function_call
);

ml_value_t *ml_partial_function_new(ml_value_t *Function, int Count) {
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, Count, ml_value_t *);
	Partial->Type = MLPartialFunctionT;
	Partial->Function = Function;
	Partial->Count = Count;
	Partial->Set = 0;
	return (ml_value_t *)Partial;
}

ml_value_t *ml_partial_function_set(ml_value_t *Partial, size_t Index, ml_value_t *Value) {
	++((ml_partial_function_t *)Partial)->Set;
	return ((ml_partial_function_t *)Partial)->Args[Index] = Value;
}

ML_METHOD("!!", MLFunctionT, MLListT) {
	ml_list_t *ArgsList = (ml_list_t *)Args[1];
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, ArgsList->Length, ml_value_t *);
	Partial->Type = MLPartialFunctionT;
	Partial->Function = Args[0];
	Partial->Count = Partial->Set = ArgsList->Length;
	ml_value_t **Arg = Partial->Args;
	ML_LIST_FOREACH(ArgsList, Node) *Arg++ = Node->Value;
	return (ml_value_t *)Partial;
}

ML_METHOD("$", MLFunctionT, MLAnyT) {
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, 1, ml_value_t *);
	Partial->Type = MLPartialFunctionT;
	Partial->Function = Args[0];
	Partial->Count = 1;
	Partial->Args[0] = Args[1];
	return (ml_value_t *)Partial;
}

ML_METHOD("$", MLPartialFunctionT, MLAnyT) {
	ml_partial_function_t *Old = (ml_partial_function_t *)Args[0];
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, Old->Count + 1, ml_value_t *);
	Partial->Type = MLPartialFunctionT;
	Partial->Function = Old->Function;
	Partial->Count = Partial->Set = Old->Count + 1;
	memcpy(Partial->Args, Old->Args, Old->Count * sizeof(ml_value_t *));
	Partial->Args[Old->Count] = Args[1];
	return (ml_value_t *)Partial;
}

static void ML_TYPED_FN(ml_iterate, MLPartialFunctionT, ml_state_t *Caller, ml_partial_function_t *Partial) {
	return Partial->Function->Type->call(Caller, Partial->Function, Partial->Count, Partial->Args);
}

/****************************** Tuples ******************************/

static long ml_tuple_hash(ml_tuple_t *Tuple, ml_hash_chain_t *Chain) {
	long Hash = 739;
	for (int I = 0; I < Tuple->Size; ++I) Hash = ((Hash << 3) + Hash) + ml_hash(Tuple->Values[I]);
	return Hash;
}

static ml_value_t *ml_tuple_deref(ml_tuple_t *Ref) {
	if (Ref->NoRefs) return (ml_value_t *)Ref;
	for (int I = 0; I < Ref->Size; ++I) {
		ml_value_t *Old = Ref->Values[I];
		ml_value_t *New = Old->Type->deref(Old);
		if (Old != New) {
			ml_tuple_t *Deref = xnew(ml_tuple_t, Ref->Size, ml_value_t *);
			Deref->Type = MLTupleT;
			Deref->Size = Ref->Size;
			Deref->NoRefs = 1;
			for (int J = 0; J < I; ++J) Deref->Values[J] = Ref->Values[J];
			Deref->Values[I] = New;
			for (int J = I + 1; J < Ref->Size; ++J) {
				Deref->Values[J] = Ref->Values[J]->Type->deref(Ref->Values[J]);
			}
			return (ml_value_t *)Deref;
		}
	}
	Ref->NoRefs = 1;
	return (ml_value_t *)Ref;
}

static ml_value_t *ml_tuple_assign(ml_tuple_t *Ref, ml_value_t *Value) {
	if (Value->Type != MLTupleT) return ml_error("TypeError", "Can only assign a tuple to a tuple");
	ml_tuple_t *TupleValue = (ml_tuple_t *)Value;
	size_t Count = Ref->Size;
	if (TupleValue->Size < Count) Count = TupleValue->Size;
	ml_value_t **Values = TupleValue->Values;
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Result = Ref->Values[I]->Type->assign(Ref->Values[I], Values[I]);
		if (Result->Type == MLErrorT) return Result;
	}
	return Value;
}

ML_TYPE(MLTupleT, (), "tuple",
	.hash = (void *)ml_tuple_hash,
	.deref = (void *)ml_tuple_deref,
	.assign = (void *)ml_tuple_assign
);

ml_value_t *ml_tuple(size_t Size) {
	ml_tuple_t *Tuple = xnew(ml_tuple_t, Size, ml_value_t *);
	Tuple->Type = MLTupleT;
	Tuple->Size = Size;
	return (ml_value_t *)Tuple;
}

extern inline int ml_tuple_size(ml_value_t *Tuple);
extern inline ml_value_t *ml_tuple_get(ml_value_t *Tuple, int Index);
extern inline ml_value_t *ml_tuple_set(ml_value_t *Tuple, int Index, ml_value_t *Value);

ml_unpacked_t ml_unpack(ml_value_t *Value, int Count) {
	typeof(ml_unpack) *function = ml_typed_fn_get(Value->Type, ml_unpack);
	if (!function) return (ml_unpacked_t){NULL, 0};
	return function(Value, Count);
}

ML_FUNCTION(MLTuple) {
	ml_value_t *Tuple = ml_tuple(Count);
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Value = Args[I]->Type->deref(Args[I]);
		//if (Value->Type == MLErrorT) return Value;
		ml_tuple_set(Tuple, I + 1, Value);
	}
	return Tuple;
}

ML_METHOD("size", MLTupleT) {
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	return ml_integer(Tuple->Size);
}

ML_METHOD("[]", MLTupleT, MLIntegerT) {
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	long Index = ((ml_integer_t *)Args[1])->Value;
	if (--Index < 0) Index += Tuple->Size + 1;
	if (Index < 0 || Index >= Tuple->Size) return ml_error("RangeError", "Tuple index out of bounds");
	return Tuple->Values[Index];
}

ml_value_t *ml_tuple_fn(void *Data, int Count, ml_value_t **Args) {
	ml_tuple_t *Tuple = xnew(ml_tuple_t, Count, ml_value_t *);
	Tuple->Type = MLTupleT;
	Tuple->Size = Count;
	memcpy(Tuple->Values, Args, Count * sizeof(ml_value_t *));
	return (ml_value_t *)Tuple;
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLTupleT, ml_tuple_t *Tuple) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_add(Buffer, "(", 1);
	if (Tuple->Size) {
		ml_stringbuffer_append(Buffer, Tuple->Values[0]);
		for (int I = 1; I < Tuple->Size; ++I) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			ml_stringbuffer_append(Buffer, Tuple->Values[I]);
		}
	}
	ml_stringbuffer_add(Buffer, ")", 1);
	return ml_stringbuffer_get_string(Buffer);
}

ML_METHOD(MLStringOfMethod, MLTupleT) {
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_add(Buffer, "(", 1);
	if (Tuple->Size) {
		ml_stringbuffer_append(Buffer, Tuple->Values[0]);
		for (int I = 1; I < Tuple->Size; ++I) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			ml_stringbuffer_append(Buffer, Tuple->Values[I]);
		}
	}
	ml_stringbuffer_add(Buffer, ")", 1);
	return ml_stringbuffer_get_string(Buffer);
}

ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLTupleT, ml_stringbuffer_t *Buffer, ml_tuple_t *Tuple) {
	ml_stringbuffer_add(Buffer, "(", 1);
	if (Tuple->Size) {
		ml_stringbuffer_append(Buffer, Tuple->Values[0]);
		for (int I = 1; I < Tuple->Size; ++I) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			ml_stringbuffer_append(Buffer, Tuple->Values[I]);
		}
	}
	ml_stringbuffer_add(Buffer, ")", 1);
	return MLSome;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLTupleT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_tuple_t *Value = (ml_tuple_t *)Args[1];
	ml_stringbuffer_add(Buffer, "(", 1);
	if (Value->Size) {
		ml_stringbuffer_append(Buffer, Value->Values[0]);
		for (int I = 1; I < Value->Size; ++I) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			ml_stringbuffer_append(Buffer, Value->Values[I]);
		}
	}
	ml_stringbuffer_add(Buffer, ")", 1);
	return MLSome;
}

ml_unpacked_t ML_TYPED_FN(ml_unpack, MLTupleT, ml_tuple_t *Tuple, int Count) {
	return (ml_unpacked_t){Tuple->Values, Tuple->Size};
}

static ml_value_t *ml_tuple_compare(ml_tuple_t *A, ml_tuple_t *B) {
	ml_value_t *Args[2];
	ml_value_t *Result;
	int N;
	if (A->Size > B->Size) {
		N = B->Size;
		Result = (ml_value_t *)One;
	} else if (A->Size < B->Size) {
		N = A->Size;
		Result = (ml_value_t *)NegOne;
	} else {
		N = A->Size;
		Result = (ml_value_t *)Zero;
	}
	for (int I = 0; I < N; ++I) {
		Args[0] = A->Values[I];
		Args[1] = B->Values[I];
		ml_value_t *C = ml_call(CompareMethod, 2, Args);
		if (C->Type == MLErrorT) return C;
		if (C->Type != MLIntegerT) return ml_error("TypeError", "Comparison returned non integer value");
		if (((ml_integer_t *)C)->Value) return C;
	}
	return Result;
}

ML_METHOD("<>", MLTupleT, MLTupleT) {
	return ml_tuple_compare((ml_tuple_t *)Args[0], (ml_tuple_t *)Args[1]);
}

#define ml_comp_tuple_tuple(NAME, NEG, ZERO, POS) \
ML_METHOD(NAME, MLTupleT, MLTupleT) { \
	ml_value_t *Result = ml_tuple_compare((ml_tuple_t *)Args[0], (ml_tuple_t *)Args[1]); \
	if (Result == (ml_value_t *)NegOne) return NEG; \
	if (Result == (ml_value_t *)Zero) return ZERO; \
	if (Result == (ml_value_t *)One) return POS; \
	return Result; \
}

ml_comp_tuple_tuple("=", MLNil, Args[1], MLNil);
ml_comp_tuple_tuple("!=", Args[1], MLNil, Args[1]);
ml_comp_tuple_tuple("<", Args[1], MLNil, MLNil);
ml_comp_tuple_tuple("<=", Args[1], Args[1], MLNil);
ml_comp_tuple_tuple(">", MLNil, MLNil, Args[1]);
ml_comp_tuple_tuple(">=", MLNil, Args[1], Args[1]);

/****************************** Boolean ******************************/

static long ml_boolean_hash(ml_boolean_t *Boolean, ml_hash_chain_t *Chain) {
	return (long)Boolean;
}

ML_TYPE(MLBooleanT, (MLFunctionT), "boolean",
	.hash = (void *)ml_boolean_hash
);

int ml_boolean_value(ml_value_t *Value) {
	return ((ml_boolean_t *)Value)->Value;
}

ml_boolean_t MLFalse[1] = {{MLBooleanT, "false", 0}};
ml_boolean_t MLTrue[1] = {{MLBooleanT, "true", 1}};

static ml_value_t *MLBooleans[2] = {
	[0] = (ml_value_t *)MLFalse,
	[1] = (ml_value_t *)MLTrue
};

ml_value_t *ml_boolean(int Value) {
	return Value ? (ml_value_t *)MLTrue : (ml_value_t *)MLFalse;
}

ML_METHOD(MLBooleanOfMethod, MLStringT) {
	const char *Name = ml_string_value(Args[0]);
	if (!strcasecmp(Name, "true")) return (ml_value_t *)MLTrue;
	if (!strcasecmp(Name, "false")) return (ml_value_t *)MLFalse;
	return ml_error("ValueError", "Invalid boolean: %s", Name);
}

ML_METHOD("-", MLBooleanT) {
	return MLBooleans[1 - ml_boolean_value(Args[0])];
}

ML_METHOD("/\\", MLBooleanT, MLBooleanT) {
	return MLBooleans[ml_boolean_value(Args[0]) & ml_boolean_value(Args[1])];
}

ML_METHOD("\\/", MLBooleanT, MLBooleanT) {
	return MLBooleans[ml_boolean_value(Args[0]) | ml_boolean_value(Args[1])];
}

#define ml_comp_method_boolean_boolean(NAME, SYMBOL) \
	ML_METHOD(NAME, MLBooleanT, MLBooleanT) { \
		ml_boolean_t *BooleanA = (ml_boolean_t *)Args[0]; \
		ml_boolean_t *BooleanB = (ml_boolean_t *)Args[1]; \
		return BooleanA->Value SYMBOL BooleanB->Value ? Args[1] : MLNil; \
	}

ml_comp_method_boolean_boolean("=", ==);
ml_comp_method_boolean_boolean("!=", !=);
ml_comp_method_boolean_boolean("<", <);
ml_comp_method_boolean_boolean(">", >);
ml_comp_method_boolean_boolean("<=", <=);
ml_comp_method_boolean_boolean(">=", >=);

/****************************** Numbers ******************************/

ML_TYPE(MLNumberT, (MLFunctionT), "number");

typedef struct ml_real_t {
	const ml_type_t *Type;
	double Value;
} ml_real_t;

static long ml_integer_hash(ml_integer_t *Integer, ml_hash_chain_t *Chain) {
	return Integer->Value;
}

static void ml_integer_call(ml_state_t *Caller, ml_integer_t *Integer, int Count, ml_value_t **Args) {
	long Index = Integer->Value;
	if (Index <= 0) Index += Count + 1;
	if (Index <= 0) ML_RETURN(MLNil);
	if (Index > Count) ML_RETURN(MLNil);
	ML_RETURN(Args[Index - 1]);
}

ML_TYPE(MLIntegerT, (MLNumberT), "integer",
	.hash = (void *)ml_integer_hash,
	.call = (void *)ml_integer_call
);

ml_value_t *ml_integer(long Value) {
	ml_integer_t *Integer = new(ml_integer_t);
	Integer->Type = MLIntegerT;
	Integer->Value = Value;
	return (ml_value_t *)Integer;
}

long ml_integer_value(ml_value_t *Value) {
	if (Value->Type == MLIntegerT) {
		return ((ml_integer_t *)Value)->Value;
	} else if (Value->Type == MLRealT) {
		return ((ml_real_t *)Value)->Value;
	} else {
		return 0;
	}
}

ml_value_t *ml_integer_of(ml_value_t *Value) {
	typeof(ml_integer_of) *function = ml_typed_fn_get(Value->Type, ml_string_of);
	if (!function) return ml_inline(MLIntegerOfMethod, 1, Value);
	return function(Value);
}

static ml_value_t *ML_TYPED_FN(ml_integer_of, MLIntegerT, ml_value_t *Value) {
	return Value;
}

static ml_value_t *ML_TYPED_FN(ml_integer_of, MLRealT, ml_real_t *Real) {
	return ml_integer(Real->Value);
}

ML_METHOD(MLIntegerOfMethod, MLRealT) {
	return ml_integer(((ml_real_t *)Args[0])->Value);
}

static long ml_real_hash(ml_real_t *Real, ml_hash_chain_t *Chain) {
	return (long)Real->Value;
}

ML_TYPE(MLRealT, (MLNumberT), "real",
	.hash = (void *)ml_real_hash
);

ml_value_t *ml_real(double Value) {
	ml_real_t *Real = new(ml_real_t);
	Real->Type = MLRealT;
	Real->Value = Value;
	return (ml_value_t *)Real;
}

double ml_real_value(ml_value_t *Value) {
	if (Value->Type == MLIntegerT) {
		return ((ml_integer_t *)Value)->Value;
	} else if (Value->Type == MLRealT) {
		return ((ml_real_t *)Value)->Value;
	} else {
		return 0;
	}
}

ml_value_t *ml_real_of(ml_value_t *Value) {
	typeof(ml_integer_of) *function = ml_typed_fn_get(Value->Type, ml_string_of);
	if (!function) return ml_inline(MLRealOfMethod, 1, Value);
	return function(Value);
}

static ml_value_t *ML_TYPED_FN(ml_real_of, MLIntegerT, ml_integer_t *Integer) {
	return ml_real(Integer->Value);
}

ML_METHOD(MLRealOfMethod, MLIntegerT) {
	return ml_real(((ml_integer_t *)Args[0])->Value);
}

static ml_value_t *ML_TYPED_FN(ml_real_of, MLRealT, ml_value_t *Value) {
	return Value;
}

#define ml_arith_method_integer(NAME, SYMBOL) \
	ML_METHOD(NAME, MLIntegerT) { \
		ml_integer_t *IntegerA = (ml_integer_t *)Args[0]; \
		return ml_integer(SYMBOL(IntegerA->Value)); \
	}

#define ml_arith_method_integer_integer(NAME, SYMBOL) \
	ML_METHOD(NAME, MLIntegerT, MLIntegerT) { \
		ml_integer_t *IntegerA = (ml_integer_t *)Args[0]; \
		ml_integer_t *IntegerB = (ml_integer_t *)Args[1]; \
		return ml_integer(IntegerA->Value SYMBOL IntegerB->Value); \
	}

#define ml_arith_method_real(NAME, SYMBOL) \
	ML_METHOD(NAME, MLRealT) { \
		ml_real_t *RealA = (ml_real_t *)Args[0]; \
		return ml_real(SYMBOL(RealA->Value)); \
	}

#define ml_arith_method_real_real(NAME, SYMBOL) \
	ML_METHOD(NAME, MLRealT, MLRealT) { \
		ml_real_t *RealA = (ml_real_t *)Args[0]; \
		ml_real_t *RealB = (ml_real_t *)Args[1]; \
		return ml_real(RealA->Value SYMBOL RealB->Value); \
	}

#define ml_arith_method_real_integer(NAME, SYMBOL) \
	ML_METHOD(NAME, MLRealT, MLIntegerT) { \
		ml_real_t *RealA = (ml_real_t *)Args[0]; \
		ml_integer_t *IntegerB = (ml_integer_t *)Args[1]; \
		return ml_real(RealA->Value SYMBOL IntegerB->Value); \
	}

#define ml_arith_method_integer_real(NAME, SYMBOL) \
	ML_METHOD(NAME, MLIntegerT, MLRealT) { \
		ml_integer_t *IntegerA = (ml_integer_t *)Args[0]; \
		ml_real_t *RealB = (ml_real_t *)Args[1]; \
		return ml_real(IntegerA->Value SYMBOL RealB->Value); \
	}

#define ml_arith_method_number(NAME, SYMBOL) \
	ml_arith_method_integer(NAME, SYMBOL) \
	ml_arith_method_real(NAME, SYMBOL)

#define ml_arith_method_number_number(NAME, SYMBOL) \
	ml_arith_method_integer_integer(NAME, SYMBOL) \
	ml_arith_method_real_real(NAME, SYMBOL) \
	ml_arith_method_real_integer(NAME, SYMBOL) \
	ml_arith_method_integer_real(NAME, SYMBOL)

ml_arith_method_number("-", -)
ml_arith_method_number_number("+", +)
ml_arith_method_number_number("-", -)
ml_arith_method_number_number("*", *)

ML_METHOD("++", MLIntegerT) {
	return ml_integer(ml_integer_value(Args[0]) + 1);
}

ML_METHOD("--", MLIntegerT) {
	return ml_integer(ml_integer_value(Args[0]) - 1);
}

ML_METHOD("++", MLRealT) {
	return ml_real(ml_real_value(Args[0]) + 1);
}

ML_METHOD("--", MLRealT) {
	return ml_real(ml_real_value(Args[0]) - 1);
}

ml_arith_method_real_real("/", /)
ml_arith_method_real_integer("/", /)
ml_arith_method_integer_real("/", /)

ML_METHOD("/", MLIntegerT, MLIntegerT) {
	ml_integer_t *IntegerA = (ml_integer_t *)Args[0];
	ml_integer_t *IntegerB = (ml_integer_t *)Args[1];
	if (IntegerA->Value % IntegerB->Value == 0) {
		return ml_integer(IntegerA->Value / IntegerB->Value);
	} else {
		return ml_real((double)IntegerA->Value / (double)IntegerB->Value);
	}
}

ml_arith_method_integer_integer("%", %)

ML_METHOD("div", MLIntegerT, MLIntegerT) {
	ml_integer_t *IntegerA = (ml_integer_t *)Args[0];
	ml_integer_t *IntegerB = (ml_integer_t *)Args[1];
	long A = IntegerA->Value;
	long B = IntegerB->Value;
	long Q = A / B;
	if (A < 0 && B * Q != A) {
		if (B < 0) ++Q; else --Q;
	}
	return ml_integer(Q);
}

ML_METHOD("mod", MLIntegerT, MLIntegerT) {
	ml_integer_t *IntegerA = (ml_integer_t *)Args[0];
	ml_integer_t *IntegerB = (ml_integer_t *)Args[1];
	long A = IntegerA->Value;
	long B = IntegerB->Value;
	long R = A % B;
	if (R < 0) R += labs(B);
	return ml_integer(R);
}

#define ml_comp_method_integer_integer(NAME, SYMBOL) \
	ML_METHOD(NAME, MLIntegerT, MLIntegerT) { \
		ml_integer_t *IntegerA = (ml_integer_t *)Args[0]; \
		ml_integer_t *IntegerB = (ml_integer_t *)Args[1]; \
		return IntegerA->Value SYMBOL IntegerB->Value ? Args[1] : MLNil; \
	}

#define ml_comp_method_real_real(NAME, SYMBOL) \
	ML_METHOD(NAME, MLRealT, MLRealT) { \
		ml_real_t *RealA = (ml_real_t *)Args[0]; \
		ml_real_t *RealB = (ml_real_t *)Args[1]; \
		return RealA->Value SYMBOL RealB->Value ? Args[1] : MLNil; \
	}

#define ml_comp_method_real_integer(NAME, SYMBOL) \
	ML_METHOD(NAME, MLRealT, MLIntegerT) { \
		ml_real_t *RealA = (ml_real_t *)Args[0]; \
		ml_integer_t *IntegerB = (ml_integer_t *)Args[1]; \
		return RealA->Value SYMBOL IntegerB->Value ? Args[1] : MLNil; \
	}

#define ml_comp_method_integer_real(NAME, SYMBOL) \
	ML_METHOD(NAME, MLIntegerT, MLRealT) { \
		ml_integer_t *IntegerA = (ml_integer_t *)Args[0]; \
		ml_real_t *RealB = (ml_real_t *)Args[1]; \
		return IntegerA->Value SYMBOL RealB->Value ? Args[1] : MLNil; \
	}

#define ml_comp_method_number_number(NAME, SYMBOL) \
	ml_comp_method_integer_integer(NAME, SYMBOL) \
	ml_comp_method_real_real(NAME, SYMBOL) \
	ml_comp_method_real_integer(NAME, SYMBOL) \
	ml_comp_method_integer_real(NAME, SYMBOL)

ml_comp_method_number_number("=", ==)
ml_comp_method_number_number("!=", !=)
ml_comp_method_number_number("<", <)
ml_comp_method_number_number(">", >)
ml_comp_method_number_number("<=", <=)
ml_comp_method_number_number(">=", >=)

ML_METHOD("<>", MLIntegerT, MLIntegerT) {
	ml_integer_t *IntegerA = (ml_integer_t *)Args[0];
	ml_integer_t *IntegerB = (ml_integer_t *)Args[1];
	if (IntegerA->Value < IntegerB->Value) return (ml_value_t *)NegOne;
	if (IntegerA->Value > IntegerB->Value) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLRealT, MLIntegerT) {
	ml_real_t *RealA = (ml_real_t *)Args[0];
	ml_integer_t *IntegerB = (ml_integer_t *)Args[1];
	if (RealA->Value < IntegerB->Value) return (ml_value_t *)NegOne;
	if (RealA->Value > IntegerB->Value) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLIntegerT, MLRealT) {
	ml_integer_t *IntegerA = (ml_integer_t *)Args[0];
	ml_real_t *RealB = (ml_real_t *)Args[1];
	if (IntegerA->Value < RealB->Value) return (ml_value_t *)NegOne;
	if (IntegerA->Value > RealB->Value) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLRealT, MLRealT) {
	ml_real_t *RealA = (ml_real_t *)Args[0];
	ml_real_t *RealB = (ml_real_t *)Args[1];
	if (RealA->Value < RealB->Value) return (ml_value_t *)NegOne;
	if (RealA->Value > RealB->Value) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

typedef struct ml_integer_iter_t {
	const ml_type_t *Type;
	long Current, Step, Limit;
	long Index;
} ml_integer_iter_t;

static void ML_TYPED_FN(ml_iter_value, MLIntegerIterT, ml_state_t *Caller, ml_integer_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Current));
}

static void ML_TYPED_FN(ml_iter_next, MLIntegerIterT, ml_state_t *Caller, ml_integer_iter_t *Iter) {
	Iter->Current += Iter->Step;
	if (Iter->Step > 0) {
		if (Iter->Current > Iter->Limit) ML_RETURN(MLNil);
	} else if (Iter->Step < 0) {
		if (Iter->Current < Iter->Limit) ML_RETURN(MLNil);
	}
	++Iter->Index;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLIntegerIterT, ml_state_t *Caller, ml_integer_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

ML_TYPE(MLIntegerIterT, (), "integer-iter");

typedef struct ml_integer_range_t {
	const ml_type_t *Type;
	long Start, Limit, Step;
} ml_integer_range_t;

static void ML_TYPED_FN(ml_iterate, MLIntegerRangeT, ml_state_t *Caller, ml_value_t *Value) {
	ml_integer_range_t *Range = (ml_integer_range_t *)Value;
	if (Range->Step > 0 && Range->Start > Range->Limit) ML_RETURN(MLNil);
	if (Range->Step < 0 && Range->Start < Range->Limit) ML_RETURN(MLNil);
	ml_integer_iter_t *Iter = new(ml_integer_iter_t);
	Iter->Type = MLIntegerIterT;
	Iter->Index = 1;
	Iter->Current = Range->Start;
	Iter->Limit = Range->Limit;
	Iter->Step = Range->Step;
	ML_RETURN(Iter);
}

ML_TYPE(MLIntegerRangeT, (MLIteratableT), "integer-range");

ML_METHOD("..", MLIntegerT, MLIntegerT) {
	ml_integer_t *IntegerA = (ml_integer_t *)Args[0];
	ml_integer_t *IntegerB = (ml_integer_t *)Args[1];
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = IntegerA->Value;
	Range->Limit = IntegerB->Value;
	Range->Step = 1;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerT, MLIntegerT) {
	ml_integer_t *IntegerA = (ml_integer_t *)Args[0];
	ml_integer_t *IntegerB = (ml_integer_t *)Args[1];
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = IntegerA->Value;
	Range->Step = IntegerB->Value;
	Range->Limit = Range->Step > 0 ? LONG_MAX : LONG_MIN;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerRangeT, MLIntegerT) {
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = Range0->Start;
	Range->Limit = Range0->Limit;
	Range->Step = ml_integer_value(Args[1]);
	return (ml_value_t *)Range;
}

ML_METHOD("in", MLIntegerT, MLIntegerRangeT) {
	long Value = ((ml_integer_t *)Args[0])->Value;
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("in", MLRealT, MLIntegerRangeT) {
	double Value = ((ml_real_t *)Args[0])->Value;
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

typedef struct ml_real_iter_t {
	const ml_type_t *Type;
	double Current, Step, Limit;
	long Index, Remaining;
} ml_real_iter_t;

static void ML_TYPED_FN(ml_iter_value, MLRealIterT, ml_state_t *Caller, ml_real_iter_t *Iter) {
	ML_RETURN(ml_real(Iter->Current));
}

static void ML_TYPED_FN(ml_iter_next, MLRealIterT, ml_state_t *Caller, ml_real_iter_t *Iter) {
	Iter->Current += Iter->Step;
	if (--Iter->Remaining < 0) ML_RETURN(MLNil);
	++Iter->Index;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLRealIterT, ml_state_t *Caller, ml_real_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

ML_TYPE(MLRealIterT, (), "real-iter");

typedef struct ml_real_range_t {
	const ml_type_t *Type;
	double Start, Limit, Step;
	long Count;
} ml_real_range_t;

static void ML_TYPED_FN(ml_iterate, MLRealRangeT, ml_state_t *Caller, ml_value_t *Value) {
	ml_real_range_t *Range = (ml_real_range_t *)Value;
	if (Range->Step > 0 && Range->Start > Range->Limit) ML_RETURN(MLNil);
	if (Range->Step < 0 && Range->Start < Range->Limit) ML_RETURN(MLNil);
	ml_real_iter_t *Iter = new(ml_real_iter_t);
	Iter->Type = MLRealIterT;
	Iter->Index = 1;
	Iter->Current = Range->Start;
	Iter->Limit = Range->Limit;
	Iter->Step = Range->Step;
	Iter->Remaining = Range->Count;
	ML_RETURN(Iter);
}

ML_TYPE(MLRealRangeT, (MLIteratableT), "real-range");

ML_METHOD("..", MLNumberT, MLNumberT) {
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = ml_real_value(Args[0]);
	Range->Limit = ml_real_value(Args[1]);
	Range->Step = 1.0;
	Range->Count = floor(Range->Limit - Range->Start);
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLNumberT, MLNumberT) {
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = ml_real_value(Args[0]);
	Range->Step = ml_real_value(Args[1]);
	Range->Limit = Range->Step > 0.0 ? INFINITY : -INFINITY;
	Range->Count = LONG_MAX;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLRealRangeT, MLNumberT) {
	ml_real_range_t *Range0 = (ml_real_range_t *)Args[0];
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	double Start = Range->Start = Range0->Start;
	double Limit = Range->Limit = Range0->Limit;
	Range->Step = ml_real_value(Args[1]);
	double C = (Limit - Start) / Range->Step;
	if (C > LONG_MAX) C = LONG_MAX;
	Range->Count = C;
	return (ml_value_t *)Range;
}

ML_METHOD("in", MLRealRangeT, MLIntegerT) {
	ml_real_range_t *Range0 = (ml_real_range_t *)Args[0];
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = Range0->Start;
	Range->Limit = Range0->Limit;
	long C = Range->Count = ml_integer_value(Args[1]) - 1;
	Range->Step = (Range->Limit - Range->Start) / C;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerRangeT, MLRealT) {
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	double Start = Range->Start = Range0->Start;
	double Limit = Range->Limit = Range0->Limit;
	double Step = Range->Step = ml_real_value(Args[1]);
	double C = (Limit - Start) / Step;
	if (C > LONG_MAX) C = LONG_MAX;
	Range->Count = LONG_MAX;
	return (ml_value_t *)Range;
}

ML_METHOD("in", MLIntegerT, MLRealRangeT) {
	long Value = ml_integer_value(Args[0]);
	ml_real_range_t *Range = (ml_real_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("in", MLRealT, MLRealRangeT) {
	double Value = ml_real_value(Args[0]);
	ml_real_range_t *Range = (ml_real_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

/****************************** Strings ******************************/

ML_TYPE(MLBufferT, (), "buffer");

ml_value_t *ml_buffer(void *Data, int Count, ml_value_t **Args) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	long Size = ml_integer_value(Args[0]);
	if (Size < 0) return ml_error("ValueError", "Buffer size must be non-negative");
	ml_buffer_t *Buffer = new(ml_buffer_t);
	Buffer->Type = MLBufferT;
	Buffer->Size = Size;
	Buffer->Address = GC_MALLOC_ATOMIC(Size);
	return (ml_value_t *)Buffer;
}

ML_METHOD("+", MLBufferT, MLIntegerT) {
	ml_buffer_t *Buffer = (ml_buffer_t *)Args[0];
	long Offset = ml_integer_value(Args[1]);
	if (Offset >= Buffer->Size) return ml_error("ValueError", "Offset larger than buffer");
	ml_buffer_t *Buffer2 = new(ml_buffer_t);
	Buffer2->Type = MLBufferT;
	Buffer2->Address = Buffer->Address + Offset;
	Buffer2->Size = Buffer->Size - Offset;
	return (ml_value_t *)Buffer2;
}

ML_METHOD("-", MLBufferT, MLBufferT) {
	ml_buffer_t *Buffer1 = (ml_buffer_t *)Args[0];
	ml_buffer_t *Buffer2 = (ml_buffer_t *)Args[1];
	return ml_integer(Buffer1->Address - Buffer2->Address);
}

ML_METHOD(MLStringOfMethod, MLBufferT) {
	ml_buffer_t *Buffer = (ml_buffer_t *)Args[0];
	return ml_string_format("#%" PRIxPTR ":%ld", Buffer->Address, Buffer->Size);
}

typedef struct ml_string_t {
	const ml_type_t *Type;
	const char *Value;
	size_t Length;
} ml_string_t;

static long ml_string_hash(ml_string_t *String, ml_hash_chain_t *Chain) {
	long Hash = 5381;
	for (int I = 0; I < String->Length; ++I) Hash = ((Hash << 5) + Hash) + String->Value[I];
	return Hash;
}

ML_TYPE(MLStringT, (MLBufferT, MLIteratableT), "string",
	.hash = (void *)ml_string_hash
);

ml_value_t *ml_string(const char *Value, int Length) {
	ml_string_t *String = new(ml_string_t);
	String->Type = MLStringT;
	if (Length >= 0) {
		if (Value[Length]) {
			char *Copy = snew(Length + 1);
			memcpy(Copy, Value, Length);
			Copy[Length] = 0;
			Value = Copy;
		}
	} else {
		Length = Value ? strlen(Value) : 0;
	}
	String->Value = Value;
	String->Length = Length;
	return (ml_value_t *)String;
}

ML_FUNCTION(StringNew) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (int I = 0; I < Count; ++I) ml_stringbuffer_append(Buffer, Args[I]);
	return ml_stringbuffer_get_string(Buffer);
}

ml_value_t *ml_string_format(const char *Format, ...) {
	ml_string_t *String = new(ml_string_t);
	String->Type = MLStringT;
	va_list Args;
	va_start(Args, Format);
	String->Length = vasprintf((char **)&String->Value, Format, Args);
	va_end(Args);
	return (ml_value_t *)String;
}

const char *ml_string_value(ml_value_t *Value) {
	return ((ml_string_t *)Value)->Value;
}

size_t ml_string_length(ml_value_t *Value) {
	return ((ml_string_t *)Value)->Length;
}

ml_value_t *ml_string_of(ml_value_t *Value) {
	typeof(ml_string_of) *function = ml_typed_fn_get(Value->Type, ml_string_of);
	if (!function) return ml_inline(MLStringOfMethod, 1, Value);
	return function(Value);
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLStringT, ml_value_t *Value) {
	return Value;
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLNilT, ml_value_t *Value) {
	return ml_cstring("nil");
}

ML_METHOD(MLStringOfMethod, MLNilT) {
	return ml_cstring("nil");
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLSomeT, ml_value_t *Value) {
	return ml_cstring("some");
}

ML_METHOD(MLStringOfMethod, MLSomeT) {
	return ml_cstring("some");
}

ML_METHOD(MLStringOfMethod, MLBooleanT) {
	ml_boolean_t *Boolean = (ml_boolean_t *)Args[0];
	return ml_string(Boolean->Name, -1);
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLIntegerT, ml_integer_t *Integer) {
	ml_string_t *String = new(ml_string_t);
	String->Type = MLStringT;
	String->Length = asprintf((char **)&String->Value, "%ld", Integer->Value);
	return (ml_value_t *)String;
}

ML_METHOD(MLStringOfMethod, MLIntegerT) {
	ml_integer_t *Integer = (ml_integer_t *)Args[0];
	ml_string_t *String = new(ml_string_t);
	String->Type = MLStringT;
	String->Length = asprintf((char **)&String->Value, "%ld", Integer->Value);
	return (ml_value_t *)String;
}

ML_METHOD(MLStringOfMethod, MLIntegerT, MLIntegerT) {
	ml_integer_t *Integer = (ml_integer_t *)Args[0];
	long Base = ((ml_integer_t *)Args[1])->Value;
	if (Base < 2 || Base > 36) return ml_error("RangeError", "Invalid base");
	ml_string_t *String = new(ml_string_t);
	String->Type = MLStringT;
	char *P = GC_MALLOC_ATOMIC(66) + 65;
	*P = '\0';
	long Value = Integer->Value;
	long Neg = Value < 0 ? Value : -Value;
	do {
		*--P = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[-(Neg % Base)];
		Neg /= Base;
	} while (Neg);
	if (Value < 0) *--P = '-';
	String->Value = P;
	String->Length = strlen(P);
	return (ml_value_t *)String;
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLRealT, ml_real_t *Real) {
	ml_string_t *String = new(ml_string_t);
	String->Type = MLStringT;
	String->Length = asprintf((char **)&String->Value, "%f", Real->Value);
	return (ml_value_t *)String;
}

ML_METHOD(MLStringOfMethod, MLRealT) {
	ml_real_t *Real = (ml_real_t *)Args[0];
	ml_string_t *String = new(ml_string_t);
	String->Type = MLStringT;
	String->Length = asprintf((char **)&String->Value, "%f", Real->Value);
	return (ml_value_t *)String;
}

ML_METHOD(MLIntegerOfMethod, MLStringT) {
	return ml_integer(strtol(ml_string_value(Args[0]), 0, 10));
}

ML_METHOD(MLIntegerOfMethod, MLStringT, MLIntegerT) {
	return ml_integer(strtol(ml_string_value(Args[0]), 0, ml_integer_value(Args[1])));
}

ML_METHOD(MLRealOfMethod, MLStringT) {
	return ml_real(strtod(ml_string_value(Args[0]), 0));
}

typedef struct {
	const ml_type_t *Type;
	const char *Value;
	int Index, Length;
} ml_string_iterator_t;

ML_TYPE(MLStringIteratorT, (), "string-iterator");

static void ML_TYPED_FN(ml_iter_next, MLStringIteratorT, ml_state_t *Caller, ml_string_iterator_t *Iter) {
	if (++Iter->Index > Iter->Length) ML_RETURN(MLNil);
	++Iter->Value;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLStringIteratorT, ml_state_t *Caller, ml_string_iterator_t *Iter) {
	ML_RETURN(ml_string(Iter->Value, 1));
}

static void ML_TYPED_FN(ml_iter_key, MLStringIteratorT, ml_state_t *Caller, ml_string_iterator_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

static void ML_TYPED_FN(ml_iterate, MLStringT, ml_state_t *Caller, ml_string_t *String) {
	if (!String->Length) ML_RETURN(MLNil);
	ml_string_iterator_t *Iter = new(ml_string_iterator_t);
	Iter->Type = MLStringIteratorT;
	Iter->Index = 1;
	Iter->Length = String->Length;
	Iter->Value = String->Value;
	ML_RETURN(Iter);
}

typedef struct ml_regex_t ml_regex_t;

typedef struct ml_regex_t {
	const ml_type_t *Type;
	const char *Pattern;
	regex_t Value[1];
} ml_regex_t;

static long ml_regex_hash(ml_regex_t *Regex, ml_hash_chain_t *Chain) {
	long Hash = 5381;
	const char *Pattern = Regex->Pattern;
	while (*Pattern) Hash = ((Hash << 5) + Hash) + *(Pattern++);
	return Hash;
}

ML_TYPE(MLRegexT, (), "regex",
	.hash = (void *)ml_regex_hash
);

ml_value_t *ml_regex(const char *Pattern) {
	ml_regex_t *Regex = new(ml_regex_t);
	Regex->Type = MLRegexT;
	Regex->Pattern = Pattern;
	int Error = regcomp(Regex->Value, Pattern, REG_EXTENDED);
	if (Error) {
		size_t ErrorSize = regerror(Error, Regex->Value, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(Error, Regex->Value, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	return (ml_value_t *)Regex;
}

regex_t *ml_regex_value(ml_value_t *Value) {
	ml_regex_t *Regex = (ml_regex_t *)Value;
	return Regex->Value;
}

ML_FUNCTION(MLRegex) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	return ml_regex(ml_string_value(Args[0]));
}

ML_TYPE(MLStringBufferT, (), "stringbuffer");

struct ml_stringbuffer_node_t {
	ml_stringbuffer_node_t *Next;
	char Chars[ML_STRINGBUFFER_NODE_SIZE];
};

static GC_descr StringBufferDesc = 0;

ssize_t ml_stringbuffer_add(ml_stringbuffer_t *Buffer, const char *String, size_t Length) {
	size_t Remaining = Length;
	ml_stringbuffer_node_t **Slot = &Buffer->Nodes;
	ml_stringbuffer_node_t *Node = Buffer->Nodes;
	if (Node) {
		while (Node->Next) Node = Node->Next;
		Slot = &Node->Next;
	}
	while (Buffer->Space < Remaining) {
		memcpy(Node->Chars + ML_STRINGBUFFER_NODE_SIZE - Buffer->Space, String, Buffer->Space);
		String += Buffer->Space;
		Remaining -= Buffer->Space;
		ml_stringbuffer_node_t *Next = (ml_stringbuffer_node_t *)GC_MALLOC_EXPLICITLY_TYPED(sizeof(ml_stringbuffer_node_t), StringBufferDesc);
			//printf("Allocating stringbuffer: %d in total\n", ++NumStringBuffers);
		Node = Slot[0] = Next;
		Slot = &Node->Next;
		Buffer->Space = ML_STRINGBUFFER_NODE_SIZE;
	}
	memcpy(Node->Chars + ML_STRINGBUFFER_NODE_SIZE - Buffer->Space, String, Remaining);
	Buffer->Space -= Remaining;
	Buffer->Length += Length;
	return Length;
}

ssize_t ml_stringbuffer_addf(ml_stringbuffer_t *Buffer, const char *Format, ...) {
	char *String;
	va_list Args;
	va_start(Args, Format);
	size_t Length = vasprintf(&String, Format, Args);
	va_end(Args);
	return ml_stringbuffer_add(Buffer, String, Length);
}

static void ml_stringbuffer_finish(ml_stringbuffer_t *Buffer, char *String) {
	char *P = String;
	ml_stringbuffer_node_t *Node = Buffer->Nodes;
	while (Node->Next) {
		memcpy(P, Node->Chars, ML_STRINGBUFFER_NODE_SIZE);
		P += ML_STRINGBUFFER_NODE_SIZE;
		Node = Node->Next;
	}
	memcpy(P, Node->Chars, ML_STRINGBUFFER_NODE_SIZE - Buffer->Space);
	P += ML_STRINGBUFFER_NODE_SIZE - Buffer->Space;
	*P++ = 0;
	Buffer->Nodes = NULL;
	Buffer->Length = Buffer->Space = 0;
}

char *ml_stringbuffer_get(ml_stringbuffer_t *Buffer) {
	if (Buffer->Length == 0) return "";
	char *String = snew(Buffer->Length + 1);
	ml_stringbuffer_finish(Buffer, String);
	return String;
}

char *ml_stringbuffer_get_uncollectable(ml_stringbuffer_t *Buffer) {
	if (Buffer->Length == 0) return "";
	char *String = GC_MALLOC_ATOMIC_UNCOLLECTABLE(Buffer->Length + 1);
	ml_stringbuffer_finish(Buffer, String);
	return String;
}

ml_value_t *ml_stringbuffer_get_string(ml_stringbuffer_t *Buffer) {
	size_t Length = Buffer->Length;
	if (Length == 0) {
		return ml_cstring("");
	} else {
		char *Chars = snew(Length + 1);
		ml_stringbuffer_finish(Buffer, Chars);
		return ml_string(Chars, Length);
	}
}

int ml_stringbuffer_foreach(ml_stringbuffer_t *Buffer, void *Data, int (*callback)(void *, const char *, size_t)) {
	ml_stringbuffer_node_t *Node = Buffer->Nodes;
	if (!Node) return 0;
	while (Node->Next) {
		if (callback(Data, Node->Chars, ML_STRINGBUFFER_NODE_SIZE)) return 1;
		Node = Node->Next;
	}
	return callback(Data, Node->Chars, ML_STRINGBUFFER_NODE_SIZE - Buffer->Space);
}

ml_value_t *ml_stringbuffer_append(ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	ml_hash_chain_t *Chain = Buffer->Chain;
	for (ml_hash_chain_t *Link = Chain; Link; Link = Link->Previous) {
		if (Link->Value == Value) {
			ml_stringbuffer_addf(Buffer, "<%s>^%ld", Value->Type->Name, Link->Index);
			return MLSome;
		}
	}
	ml_hash_chain_t NewChain[1] = {{Chain, Value, Chain ? Chain->Index + 1 : 1}};
	Buffer->Chain = NewChain;
	typeof(ml_stringbuffer_append) *function = ml_typed_fn_get(Value->Type, ml_stringbuffer_append);
	ml_value_t *Result = function ? function(Buffer, Value) : ml_inline(MLStringBufferAppendMethod, 2, Buffer, Value);
	Buffer->Chain = Chain;
	return Result;
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLNilT, ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	return MLNil;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLNilT) {
	return MLNil;
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLSomeT, ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	return MLNil;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLSomeT) {
	return MLNil;
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLIntegerT, ml_stringbuffer_t *Buffer, ml_integer_t *Value) {
	ml_stringbuffer_addf(Buffer, "%ld", Value->Value);
	return MLSome;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLIntegerT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_addf(Buffer, "%ld", ml_integer_value(Args[1]));
	return MLSome;
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLRealT, ml_stringbuffer_t *Buffer, ml_real_t *Value) {
	ml_stringbuffer_addf(Buffer, "%f", Value->Value);
	return MLSome;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLRealT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_addf(Buffer, "%f", ml_real_value(Args[1]));
	return MLSome;
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLStringT, ml_stringbuffer_t *Buffer, ml_string_t *Value) {
	ml_stringbuffer_add(Buffer, Value->Value, Value->Length);
	return Value->Length ? MLSome : MLNil;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLStringT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_add(Buffer, ml_string_value(Args[1]), ml_string_length(Args[1]));
	return ml_string_length(Args[1]) ? MLSome : MLNil;
}

ML_METHOD("[]", MLStringT, MLIntegerT) {
	ml_string_t *String = (ml_string_t *)Args[0];
	int Index = ((ml_integer_t *)Args[1])->Value;
	if (Index <= 0) Index += String->Length + 1;
	if (Index <= 0) return MLNil;
	if (Index > String->Length) return MLNil;
	char *Chars = snew(2);
	Chars[0] = String->Value[Index - 1];
	Chars[1] = 0;
	return ml_string(Chars, 1);
}

ML_METHOD("[]", MLStringT, MLIntegerT, MLIntegerT) {
	ml_string_t *String = (ml_string_t *)Args[0];
	int Lo = ((ml_integer_t *)Args[1])->Value;
	int Hi = ((ml_integer_t *)Args[2])->Value;
	if (Lo <= 0) Lo += String->Length + 1;
	if (Hi <= 0) Hi += String->Length + 1;
	if (Lo <= 0) return MLNil;
	if (Hi > String->Length + 1) return MLNil;
	if (Hi < Lo) return MLNil;
	int Length = Hi - Lo;
	if (Hi == String->Length + 1) {
		return ml_string(String->Value + Lo - 1, Length);
	}
	char *Chars = snew(Length + 1);
	memcpy(Chars, String->Value + Lo - 1, Length);
	Chars[Length] = 0;
	return ml_string(Chars, Length);
}

ML_METHOD("+", MLStringT, MLStringT) {
	int Length1 = ml_string_length(Args[0]);
	int Length2 = ml_string_length(Args[1]);
	int Length = Length1 + Length2;
	char *Chars = GC_MALLOC_ATOMIC(Length + 1);
	memcpy(Chars, ml_string_value(Args[0]), Length1);
	memcpy(Chars + Length1, ml_string_value(Args[1]), Length2);
	Chars[Length] = 0;
	return ml_string(Chars, Length);
}

ML_METHOD("trim", MLStringT) {
	const char *Start = ml_string_value(Args[0]);
	const char *End = Start + ml_string_length(Args[0]);
	while (Start < End && Start[0] <= ' ') ++Start;
	while (Start < End && End[-1] <= ' ') --End;
	int Length = End - Start;
	char *Chars = snew(Length + 1);
	memcpy(Chars, Start, Length);
	Chars[Length] = 0;
	return ml_string(Chars, Length);
}

ML_METHOD("length", MLStringT) {
	return ml_integer(((ml_string_t *)Args[0])->Length);
}

ML_METHOD("<>", MLStringT, MLStringT) {
	ml_string_t *StringA = (ml_string_t *)Args[0];
	ml_string_t *StringB = (ml_string_t *)Args[1];
	int Compare = strcmp(StringA->Value, StringB->Value);
	if (Compare < 0) return (ml_value_t *)NegOne;
	if (Compare > 0) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

#define ml_comp_method_string_string(NAME, SYMBOL) \
	ML_METHOD(NAME, MLStringT, MLStringT) { \
		ml_string_t *StringA = (ml_string_t *)Args[0]; \
		ml_string_t *StringB = (ml_string_t *)Args[1]; \
		return strcmp(StringA->Value, StringB->Value) SYMBOL 0 ? Args[1] : MLNil; \
	}

ml_comp_method_string_string("=", ==)
ml_comp_method_string_string("!=", !=)
ml_comp_method_string_string("<", <)
ml_comp_method_string_string(">", >)
ml_comp_method_string_string("<=", <=)
ml_comp_method_string_string(">=", >=)

#define SWAP(A, B) { \
	typeof(A) Temp = A; \
	A = B; \
	B = Temp; \
}

ML_METHOD("~", MLStringT, MLStringT) {
	const char *CharsA, *CharsB;
	int LenA = ml_string_length(Args[0]);
	int LenB = ml_string_length(Args[1]);
	if (LenA < LenB) {
		SWAP(LenA, LenB);
		CharsA = ml_string_value(Args[1]);
		CharsB = ml_string_value(Args[0]);
	} else {
		CharsA = ml_string_value(Args[0]);
		CharsB = ml_string_value(Args[1]);
	}
	int *Row0 = alloca((LenB + 1) * sizeof(int));
	int *Row1 = alloca((LenB + 1) * sizeof(int));
	int *Row2 = alloca((LenB + 1) * sizeof(int));
	const int Insert = 1, Replace = 1, Swap = 1, Delete = 1;
	for (int J = 0; J <= LenB; ++J) Row1[J] = J * Insert;
	char PrevA = 0, PrevB;
	for (int I = 0; I < LenA; ++I) {
		Row2[0] = (I + 1) * Delete;
		for (int J = 0; J < LenB; ++J) {
			int Min = Row1[J] + Replace * (CharsA[I] != CharsB[J]);
			if (I > 0 && J > 0 && PrevA == CharsB[J] && CharsA[I] == PrevB && Min > Row0[J - 1] + Swap) {
				Min = Row0[J - 1] + Swap;
			}
			if (Min > Row1[J + 1] + Delete) Min = Row1[J + 1] + Delete;
			if (Min > Row2[J] + Insert) Min = Row2[J] + Insert;
			Row2[J + 1] = Min;
			PrevB = CharsB[J];
		}
		int *Dummy = Row0;
		Row0 = Row1;
		Row1 = Row2;
		Row2 = Dummy;
		PrevA = CharsA[I];
	}
	return ml_integer(Row1[LenB]);
}

ML_METHOD("~>", MLStringT, MLStringT) {
	int LenA = ml_string_length(Args[0]);
	int LenB = ml_string_length(Args[1]);
	const char *CharsA = ml_string_value(Args[0]);
	const char *CharsB = ml_string_value(Args[1]);
	int *Row0 = alloca((LenB + 1) * sizeof(int));
	int *Row1 = alloca((LenB + 1) * sizeof(int));
	int *Row2 = alloca((LenB + 1) * sizeof(int));
	int Best = LenB;
	const int Insert = 1, Replace = 1, Swap = 1, Delete = 1;
	for (int J = 0; J <= LenB; ++J) Row1[J] = J * Insert;
	char PrevA = 0, PrevB;
	for (int I = 0; I < 2 * LenB; ++I) {
		Row2[0] = (I + 1) * Delete;
		char CharA = I < LenA ? CharsA[I] : 0;
		for (int J = 0; J < LenB; ++J) {
			int Min = Row1[J] + Replace * (CharA != CharsB[J]);
			if (I > 0 && J > 0 && PrevA == CharsB[J] && CharA == PrevB && Min > Row0[J - 1] + Swap) {
				Min = Row0[J - 1] + Swap;
			}
			if (Min > Row1[J + 1] + Delete) Min = Row1[J + 1] + Delete;
			if (Min > Row2[J] + Insert) Min = Row2[J] + Insert;
			Row2[J + 1] = Min;
			PrevB = CharsB[J];
		}
		int *Dummy = Row0;
		Row0 = Row1;
		Row1 = Row2;
		Row2 = Dummy;
		PrevA = CharA;
		if (Row1[LenB] < Best) Best = Row1[LenB];
	}
	return ml_integer(Best);
}

ML_METHOD("/", MLStringT, MLStringT) {
	ml_value_t *Results = ml_list();
	const char *Subject = ml_string_value(Args[0]);
	const char *Pattern = ml_string_value(Args[1]);
	size_t Length = strlen(Pattern);
	for (;;) {
		const char *Next = strstr(Subject, Pattern);
		while (Next == Subject) {
			Subject += Length;
			Next = strstr(Subject, Pattern);
		}
		if (!Subject[0]) return Results;
		if (Next) {
			size_t MatchLength = Next - Subject;
			char *Match = snew(MatchLength + 1);
			memcpy(Match, Subject, MatchLength);
			Match[MatchLength] = 0;
			ml_list_put(Results, ml_string(Match, MatchLength));
			Subject = Next + Length;
		} else {
			ml_list_put(Results, ml_string(Subject, strlen(Subject)));
			break;
		}
	}
	return Results;
}

ML_METHOD("/", MLStringT, MLRegexT) {
	ml_value_t *Results = ml_list();
	const char *Subject = ml_string_value(Args[0]);
	const char *SubjectEnd = Subject + ml_string_length(Args[0]);
	ml_regex_t *Pattern = (ml_regex_t *)Args[1];
	int Index = Pattern->Value->re_nsub ? 1 : 0;
	regmatch_t Matches[2];
	for (;;) {
		switch (regexec(Pattern->Value, Subject, Index + 1, Matches, 0)) {
		case REG_NOMATCH: {
			if (SubjectEnd > Subject) ml_list_put(Results, ml_string(Subject, SubjectEnd - Subject));
			return Results;
		}
		case REG_ESPACE: {
			size_t ErrorSize = regerror(REG_ESPACE, Pattern->Value, NULL, 0);
			char *ErrorMessage = snew(ErrorSize + 1);
			regerror(REG_ESPACE, Pattern->Value, ErrorMessage, ErrorSize);
			return ml_error("RegexError", "regex error: %s", ErrorMessage);
		}
		default: {
			regoff_t Start = Matches[Index].rm_so;
			if (Start > 0) ml_list_put(Results, ml_string(Subject, Start));
			Subject += Matches[Index].rm_eo;
		}
		}
	}
	return Results;
}

ML_METHOD("find", MLStringT, MLStringT) {
	const char *Haystack = ml_string_value(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	const char *Match = strstr(Haystack, Needle);
	if (Match) {
		return ml_integer(1 + Match - Haystack);
	} else {
		return MLNil;
	}
}

ML_METHOD("find", MLStringT, MLStringT, MLIntegerT) {
	const char *Haystack = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	int Start = ml_integer_value(Args[2]);
	if (Start <= 0) Start += Length + 1;
	if (Start <= 0) return MLNil;
	if (Start > Length) return MLNil;
	Haystack += Start - 1;
	const char *Match = strstr(Haystack, Needle);
	if (Match) {
		return ml_integer(Start + Match - Haystack);
	} else {
		return MLNil;
	}
}

ML_METHOD("find", MLStringT, MLRegexT) {
	regex_t *Regex = ml_regex_value(Args[1]);
#ifdef __USE_GNU
	regoff_t Offset = re_search(Regex,
		ml_string_value(Args[0]), ml_string_length(Args[0]),
		0, ml_string_length(Args[0]), NULL
	);
	if (Offset >= 0) {
		return ml_integer(Offset);
	} else {
		return MLNil;
	}
#else
	regmatch_t Matches[1];
	switch (regexec(Regex, ml_string_value(Args[0]), 1, Matches, 0)) {
	case REG_NOMATCH:
		return MLNil;
	case REG_ESPACE: {
		size_t ErrorSize = regerror(REG_ESPACE, Regex, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(REG_ESPACE, Regex, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	}
	return ml_integer(Matches->rm_so);
#endif
}

ML_METHOD("%", MLStringT, MLStringT) {
	const char *Subject = ml_string_value(Args[0]);
	const char *Pattern = ml_string_value(Args[1]);
	regex_t Regex[1];
	int Error = regcomp(Regex, Pattern, REG_EXTENDED);
	if (Error) {
		size_t ErrorSize = regerror(Error, Regex, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(Error, Regex, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	regmatch_t Matches[Regex->re_nsub + 1];
	switch (regexec(Regex, Subject, Regex->re_nsub + 1, Matches, 0)) {
	case REG_NOMATCH:
		regfree(Regex);
		return MLNil;
	case REG_ESPACE: {
		regfree(Regex);
		size_t ErrorSize = regerror(REG_ESPACE, Regex, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(REG_ESPACE, Regex, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	default: {
		ml_value_t *Results = ml_tuple(Regex->re_nsub + 1);
		for (int I = 0; I < Regex->re_nsub + 1; ++I) {
			regoff_t Start = Matches[I].rm_so;
			if (Start >= 0) {
				size_t Length = Matches[I].rm_eo - Start;
				char *Chars = snew(Length + 1);
				memcpy(Chars, Subject + Start, Length);
				Chars[Length] = 0;
				ml_tuple_set(Results, I + 1, ml_string(Chars, Length));
			} else {
				ml_tuple_set(Results, I + 1, MLNil);
			}
		}
		regfree(Regex);
		return Results;
	}
	}
}

ML_METHOD("%", MLStringT, MLRegexT) {
	const char *Subject = ml_string_value(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	regmatch_t Matches[Regex->re_nsub + 1];
	switch (regexec(Regex, Subject, Regex->re_nsub + 1, Matches, 0)) {
	case REG_NOMATCH:
		return MLNil;
	case REG_ESPACE: {
		size_t ErrorSize = regerror(REG_ESPACE, Regex, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(REG_ESPACE, Regex, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	default: {
		ml_value_t *Results = ml_tuple(Regex->re_nsub + 1);
		for (int I = 0; I < Regex->re_nsub + 1; ++I) {
			regoff_t Start = Matches[I].rm_so;
			if (Start >= 0) {
				size_t Length = Matches[I].rm_eo - Start;
				char *Chars = snew(Length + 1);
				memcpy(Chars, Subject + Start, Length);
				Chars[Length] = 0;
				ml_tuple_set(Results, I + 1, ml_string(Chars, Length));
			} else {
				ml_tuple_set(Results, I + 1, MLNil);
			}
		}
		return Results;
	}
	}
}

ML_METHOD("?", MLStringT, MLRegexT) {
	const char *Subject = ml_string_value(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	regmatch_t Matches[Regex->re_nsub + 1];
	switch (regexec(Regex, Subject, Regex->re_nsub + 1, Matches, 0)) {
	case REG_NOMATCH:
		return MLNil;
	case REG_ESPACE: {
		size_t ErrorSize = regerror(REG_ESPACE, Regex, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(REG_ESPACE, Regex, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	default: {
		regoff_t Start = Matches[0].rm_so;
		if (Start >= 0) {
			size_t Length = Matches[0].rm_eo - Start;
			char *Chars = snew(Length + 1);
			memcpy(Chars, Subject + Start, Length);
			Chars[Length] = 0;
			return ml_string(Chars, Length);
		} else {
			return MLNil;
		}
	}
	}
}

ML_METHOD("replace", MLStringT, MLStringT, MLStringT) {
	const char *Subject = ml_string_value(Args[0]);
	const char *SubjectEnd = Subject + ml_string_length(Args[0]);
	const char *Pattern = ml_string_value(Args[1]);
	int PatternLength = ml_string_length(Args[1]);
	const char *Replace = ml_string_value(Args[2]);
	int ReplaceLength = ml_string_length(Args[2]);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Find = strstr(Subject, Pattern);
	while (Find) {
		if (Find > Subject) ml_stringbuffer_add(Buffer, Subject, Find - Subject);
		ml_stringbuffer_add(Buffer, Replace, ReplaceLength);
		Subject = Find + PatternLength;
		Find = strstr(Subject, Pattern);
	}
	if (SubjectEnd > Subject) {
		ml_stringbuffer_add(Buffer, Subject, SubjectEnd - Subject);
	}
	return ml_stringbuffer_get_string(Buffer);
}

ML_METHOD("replace", MLStringT, MLRegexT, MLStringT) {
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	ml_regex_t *Pattern = (ml_regex_t *)Args[1];
	const char *Replace = ml_string_value(Args[2]);
	int ReplaceLength = ml_string_length(Args[2]);
	regmatch_t Matches[1];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (;;) {
		switch (regexec(Pattern->Value, Subject, 1, Matches, 0)) {
		case REG_NOMATCH:
			if (SubjectLength) ml_stringbuffer_add(Buffer, Subject, SubjectLength);
			return ml_stringbuffer_get_string(Buffer);
		case REG_ESPACE: {
			size_t ErrorSize = regerror(REG_ESPACE, Pattern->Value, NULL, 0);
			char *ErrorMessage = snew(ErrorSize + 1);
			regerror(REG_ESPACE, Pattern->Value, ErrorMessage, ErrorSize);
			return ml_error("RegexError", "regex error: %s", ErrorMessage);
		}
		default: {
			regoff_t Start = Matches[0].rm_so;
			if (Start > 0) ml_stringbuffer_add(Buffer, Subject, Start);
			ml_stringbuffer_add(Buffer, Replace, ReplaceLength);
			Subject += Matches[0].rm_eo;
			SubjectLength -= Matches[0].rm_eo;
		}
		}
	}
	return 0;
}

ML_METHOD("replace", MLStringT, MLRegexT, MLFunctionT) {
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	ml_regex_t *Pattern = (ml_regex_t *)Args[1];
	ml_value_t *Replacer = Args[2];
	int NumSub = Pattern->Value->re_nsub + 1;
	regmatch_t Matches[NumSub];
	ml_value_t *SubArgs[NumSub];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (;;) {
		switch (regexec(Pattern->Value, Subject, NumSub, Matches, 0)) {
		case REG_NOMATCH:
			if (SubjectLength) ml_stringbuffer_add(Buffer, Subject, SubjectLength);
			return ml_stringbuffer_get_string(Buffer);
		case REG_ESPACE: {
			size_t ErrorSize = regerror(REG_ESPACE, Pattern->Value, NULL, 0);
			char *ErrorMessage = snew(ErrorSize + 1);
			regerror(REG_ESPACE, Pattern->Value, ErrorMessage, ErrorSize);
			return ml_error("RegexError", "regex error: %s", ErrorMessage);
		}
		default: {
			regoff_t Start = Matches[0].rm_so;
			if (Start > 0) ml_stringbuffer_add(Buffer, Subject, Start);
			for (int I = 0; I < NumSub; ++I) {
				SubArgs[I] = ml_string(Subject + Matches[I].rm_so, Matches[I].rm_eo - Matches[I].rm_so);
			}
			ml_value_t *Replace = ml_call(Replacer, NumSub, SubArgs);
			if (Replace->Type == MLErrorT) return Replace;
			if (Replace->Type != MLStringT) return ml_error("TypeError", "expected string, not %s", Replace->Type->Name);
			ml_stringbuffer_add(Buffer, ml_string_value(Replace), ml_string_length(Replace));
			Subject += Matches[0].rm_eo;
			SubjectLength -= Matches[0].rm_eo;
		}
		}
	}
	return 0;
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLRegexT, ml_regex_t *Regex) {
	return ml_string(Regex->Pattern, -1);
}

ML_METHOD(MLStringOfMethod, MLRegexT) {
	ml_regex_t *Regex = (ml_regex_t *)Args[0];
	return ml_string(Regex->Pattern, -1);
}

typedef struct ml_stringifier_t {
	const ml_type_t *Type;
	int Count;
	ml_value_t *Args[];
} ml_stringifier_t;

ML_TYPE(MLStringifierT, (), "stringifier");

static ml_value_t *ML_TYPED_FN(ml_string_of, MLStringifierT, ml_stringifier_t *Stringifier) {
	return ml_call(MLStringOfMethod, Stringifier->Count, Stringifier->Args);}

ML_METHODX(MLStringOfMethod, MLStringifierT) {
	ml_stringifier_t *Stringifier = (ml_stringifier_t *)Args[0];
	return MLStringOfMethod->Type->call(Caller, MLStringOfMethod, Stringifier->Count, Stringifier->Args);
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLStringifierT, ml_stringbuffer_t *Buffer, ml_stringifier_t *Stringifier) {
	ml_value_t *Result = ml_call(MLStringOfMethod, Stringifier->Count, Stringifier->Args);
	if (Result->Type == MLErrorT) return Result;
	if (Result->Type != MLStringT) return ml_error("ResultError", "string method did not return string");
	ml_stringbuffer_add(Buffer, ml_string_value(Result), ml_string_length(Result));
	return (ml_value_t *)Buffer;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLStringifierT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringifier_t *Stringifier = (ml_stringifier_t *)Args[1];
	ml_value_t *Result = ml_call(MLStringOfMethod, Stringifier->Count, Stringifier->Args);
	if (Result->Type == MLErrorT) return Result;
	if (Result->Type != MLStringT) return ml_error("ResultError", "string method did not return string");
	ml_stringbuffer_add(Buffer, ml_string_value(Result), ml_string_length(Result));
	return (ml_value_t *)Buffer;
}

ML_FUNCTION(StringifierNew) {
	ml_stringifier_t *Stringifier = xnew(ml_stringifier_t, Count, ml_value_t *);
	Stringifier->Type = MLStringifierT;
	Stringifier->Count = Count;
	for (int I = 0; I < Count; ++I) Stringifier->Args[I] = Args[I];
	return (ml_value_t *)Stringifier;
}

/****************************** Lists ******************************/

static void ml_list_call(ml_state_t *Caller, ml_list_t *List, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = Args[0]->Type->deref(Args[0]);
	if (Arg->Type == MLErrorT) ML_RETURN(Arg);
	if (Arg->Type != MLIntegerT) ML_RETURN(ml_error("TypeError", "List index must be an integer"));
	long Index = ((ml_integer_t *)Args[0])->Value;
	if (--Index < 0) Index += List->Length + 1;
	if (Index < 0 || Index >= List->Length) ML_RETURN(MLNil);
	ML_RETURN(ml_reference(&List->Head[Index]));
}

ML_TYPE(MLListT, (MLFunctionT), "list",
	.call = (void *)ml_list_call
);

#define ML_LIST_SIZE 8

ml_value_t *ml_list() {
	ml_list_t *List = new(ml_list_t);
	List->Type = MLListT;
	List->Nodes = anew(ml_value_t *, ML_LIST_SIZE * 2);
	List->Head = List->Nodes + ML_LIST_SIZE;
	List->Tail = List->Head;
	List->Space = ML_LIST_SIZE;
	List->Length = 0;
	return (ml_value_t *)List;
}

ml_value_t *ml_list_copy(ml_value_t **Nodes, int Length) {
	ml_list_t *List = new(ml_list_t);
	List->Type = MLListT;
	int Size = (Length + ML_LIST_SIZE - 1) & ~(ML_LIST_SIZE - 1);
	List->Nodes = anew(ml_value_t *, Size);
	List->Head = List->Nodes;
	List->Tail = List->Head + Length;
	memcpy(List->Head, Nodes, Length * sizeof(ml_value_t *));
	List->Space = Size - Length;
	List->Length = Length;
	return (ml_value_t *)List;
}

void ml_list_push(ml_value_t *List0, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)List0;
	int Start = List->Head - List->Nodes;
	if (!Start) {
		int Space = List->Space;
		if (Space > ML_LIST_SIZE) {
			memmove(List->Head + ML_LIST_SIZE, List->Head, List->Length * sizeof(ml_value_t *));
			List->Head += ML_LIST_SIZE;
			List->Tail += ML_LIST_SIZE;
			List->Space -= ML_LIST_SIZE;
		} else {
			int Length = List->Length, Size = Length + Space;
			ml_value_t **Nodes = anew(ml_value_t *, Size + ML_LIST_SIZE);
			ml_value_t **Head = Nodes + Start;
			memcpy(Head, List->Head, Length * sizeof(ml_value_t *));
			List->Nodes = Nodes;
			List->Head = Head;
			List->Tail = Head + Length;
		}
	}
	(--List->Head)[0] = Value;
	++List->Length;
}

void ml_list_put(ml_value_t *List0, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)List0;
	int Space = List->Space;
	if (!Space) {
		int Start = List->Head - List->Nodes;
		if (Start > ML_LIST_SIZE) {
			memmove(List->Head - ML_LIST_SIZE, List->Head, List->Length * sizeof(ml_value_t *));
			List->Head -= ML_LIST_SIZE;
			List->Tail -= ML_LIST_SIZE;
			List->Space += ML_LIST_SIZE;
		} else {
			int Length = List->Length, Size = Start + Length;
			ml_value_t **Nodes = anew(ml_value_t *, Size + ML_LIST_SIZE);
			ml_value_t **Head = Nodes + Start;
			memcpy(Head, List->Head, Length * sizeof(ml_value_t *));
			List->Nodes = Nodes;
			List->Head = Head;
			List->Tail = List->Head + Length;
			List->Space += ML_LIST_SIZE;
		}
	}
	(List->Tail++)[0] = Value;
	++List->Length;
	--List->Space;
}

ml_value_t *ml_list_pop(ml_value_t *List0) {
	ml_list_t *List = (ml_list_t *)List0;
	if (List->Length) {
		--List->Length;
		ml_value_t *Value = List->Head[0];
		List->Head[0] = NULL;
		++List->Head;
		return Value;
	} else {
		return NULL;
	}
}

ml_value_t *ml_list_pull(ml_value_t *List0) {
	ml_list_t *List = (ml_list_t *)List0;
	if (List->Length) {
		--List->Length;
		++List->Space;
		--List->Tail;
		ml_value_t *Value = List->Tail[0];
		List->Tail[0] = NULL;
		return Value;
	} else {
		return NULL;
	}
}

ml_value_t *ml_list_get(ml_value_t *List0, int Index) {
	ml_list_t *List = (ml_list_t *)List0;
	if (--Index < 0) Index += List->Length + 1;
	if (Index < 0 || Index >= List->Length) return NULL;
	return List->Head[Index];
}

ml_value_t *ml_list_set(ml_value_t *List0, int Index, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)List0;
	if (--Index < 0) Index += List->Length + 1;
	if (Index < 0 || Index >= List->Length) return NULL;
	ml_value_t *Old = List->Head[Index];
	List->Head[Index] = Value;
	return Old;
}

void ml_list_to_array(ml_value_t *Value, ml_value_t **Array) {
	ml_list_t *List = (ml_list_t *)Value;
	memcpy(Array, List->Nodes, List->Length * sizeof(ml_value_t *));
}

int ml_list_foreach(ml_value_t *Value, void *Data, int (*callback)(ml_value_t *, void *)) {
	ML_LIST_FOREACH(Value, Node) if (callback(Node->Value, Data)) return 1;
	return 0;
}

ML_METHOD("size", MLListT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_integer(List->Length);
}

ML_METHOD("length", MLListT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_integer(List->Length);
}

ML_METHOD("filter", MLListT, MLFunctionT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_value_t *Filter = Args[1];
	ml_value_t *New = ml_list();
	ML_LIST_FOREACH(List, Node) {
		ml_value_t *Result = ml_inline(Filter, 1, Node->Value);
		if (Result->Type == MLErrorT) return Result;
		if (Result != MLNil) ml_list_append(New, Node->Value);
	}
	return New;
}

ML_METHOD("map", MLListT, MLFunctionT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_value_t *Map = Args[1];
	ml_value_t *New = ml_list();
	ML_LIST_FOREACH(List, Node) {
		ml_value_t *Result = ml_inline(Map, 1, Node->Value);
		if (Result->Type == MLErrorT) return Result;
		ml_list_append(New, Result);
	}
	return New;
}

typedef struct {
	const ml_type_t *Type;
	ml_value_t *List;
	int Position;
} ml_list_index_t;

static long ml_list_index_hash(ml_list_index_t *Index, ml_hash_chain_t *Chain) {
	ml_value_t *Value = ml_list_get(Index->List, Index->Position);
	return Value->Type->hash(Value, Chain);
}

static ml_value_t *ml_list_index_deref(ml_list_index_t *Index) {
	return ml_list_get(Index->List, Index->Position);
}

static ml_value_t *ml_list_index_assign(ml_list_index_t *Index, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)Index->List;
	int Position = Index->Position;
	if (--Position < 0) Position += List->Length + 1;
	if (Position < 0 || Position >= List->Length) return ml_error("RangeError", "Index outside list");
	return (List->Head[Position] = Value);
}

ML_TYPE(MLListIndexT, (), "list-index",
	.hash = (void *)ml_list_index_hash,
	.deref = (void *)ml_list_index_deref,
	.assign = (void *)ml_list_index_assign
);

ML_METHOD("[]", MLListT, MLIntegerT) {
	ml_list_index_t *Index = new(ml_list_index_t);
	Index->Type = MLListIndexT;
	Index->List = Args[0];
	Index->Position = ml_integer_value(Args[1]);
	return (ml_value_t *)Index;
}

typedef struct {
	const ml_type_t *Type;
	ml_value_t *List;
	int Start, End;
} ml_list_slice_t;

static ml_value_t *ml_list_slice_deref(ml_list_slice_t *Slice) {
	ml_list_t *List = (ml_list_t *)Slice->List;
	int Start = Slice->Start;
	int End = Slice->End;
	if (--Start < 0) Start += List->Length + 1;
	if (--End < 0) End += List->Length + 1;
	if (Start < 0 || End < Start || End > List->Length) return MLNil;
	int Length = End - Start;
	return ml_list_copy(List->Head + Start, Length);
}

static ml_value_t *ml_list_slice_assign(ml_list_slice_t *Slice, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)Slice->List;
	int Start = Slice->Start;
	int End = Slice->End;
	if (--Start < 0) Start += List->Length + 1;
	if (--End < 0) End += List->Length + 1;
	if (Start < 0 || End < Start || End > List->Length) return ml_error("RangeError", "Index outside list");
	int Length = End - Start;
	ml_unpacked_t Unpacked = ml_unpack(Value, Length);
	if (Unpacked.Count != Length) return ml_error("ValueError", "Incorrect number of values to unpack (%d != %d)", Unpacked.Count, Length);
	memcpy(List->Head + Start, Unpacked.Values, Length * sizeof(ml_value_t *));
	return Value;
}

ML_TYPE(MLListSliceT, (), "list-slice",
	.deref = (void *)ml_list_slice_deref,
	.assign = (void *)ml_list_slice_assign
);

ML_METHOD("[]", MLListT, MLIntegerT, MLIntegerT) {
	ml_list_slice_t *Slice = new(ml_list_slice_t);
	Slice->Type = MLListSliceT;
	Slice->List = Args[0];
	Slice->Start = ml_integer_value(Args[1]);
	Slice->End = ml_integer_value(Args[2]);
	return (ml_value_t *)Slice;
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLListT, ml_stringbuffer_t *Buffer, ml_list_t *List) {
	int Length = List->Length;
	if (--Length >= 0) {
		ml_value_t **Node = List->Head;
		ml_stringbuffer_append(Buffer, Node[0]);
		while (--Length >= 0) {
			++Node;
			ml_stringbuffer_add(Buffer, " ", 1);
			ml_stringbuffer_append(Buffer, Node[0]);
		}
		return MLSome;
	} else {
		return MLNil;
	}
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLListT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_list_t *List = (ml_list_t *)Args[1];
	int Length = List->Length;
	if (--Length >= 0) {
		ml_value_t **Node = List->Head;
		ml_stringbuffer_append(Buffer, Node[0]);
		while (--Length >= 0) {
			++Node;
			ml_stringbuffer_add(Buffer, " ", 1);
			ml_stringbuffer_append(Buffer, Node[0]);
		}
		return MLSome;
	} else {
		return MLNil;
	}
}

ml_unpacked_t ML_TYPED_FN(ml_unpack, MLListT, ml_list_t *List, int Count) {
	return (ml_unpacked_t){List->Head, List->Length};
}

typedef struct ml_list_iterator_t {
	const ml_type_t *Type;
	ml_value_t **Node, **Tail;
	long Index;
} ml_list_iterator_t;

static void ML_TYPED_FN(ml_iter_value, MLListIterT, ml_state_t *Caller, ml_list_iterator_t *Iter) {
	ML_RETURN(ml_reference(Iter->Node));
}

static void ML_TYPED_FN(ml_iter_next, MLListIterT, ml_state_t *Caller, ml_list_iterator_t *Iter) {
	if (++Iter->Node < Iter->Tail) {
		++Iter->Index;
		ML_RETURN(Iter);
	} else {
		ML_RETURN(MLNil);
	}
}

static void ML_TYPED_FN(ml_iter_key, MLListIterT, ml_state_t *Caller, ml_list_iterator_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

ML_TYPE(MLListIterT, (), "list-iterator");

static void ML_TYPED_FN(ml_iterate, MLListT, ml_state_t *Caller, ml_list_t *List) {
	if (List->Length) {
		ml_list_iterator_t *Iter = new(ml_list_iterator_t);
		Iter->Type = MLListIterT;
		Iter->Node = List->Head;
		Iter->Tail = List->Tail;
		Iter->Index = 1;
		ML_RETURN(Iter);
	} else {
		ML_RETURN(MLNil);
	}
}

ML_METHODV("push", MLListT) {
	ml_value_t *List = Args[0];
	for (int I = 1; I < Count; ++I) ml_list_push(List, Args[I]);
	return Args[0];
}

ML_METHODV("put", MLListT) {
	ml_value_t *List = Args[0];
	for (int I = 1; I < Count; ++I) ml_list_put(List, Args[I]);
	return Args[0];
}

ML_METHOD("pop", MLListT) {
	return ml_list_pop(Args[0]) ?: MLNil;
}

ML_METHOD("pull", MLListT) {
	return ml_list_pull(Args[0]) ?: MLNil;
}

ML_METHOD("copy", MLListT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_list_copy(List->Head, List->Length);
}

ML_METHOD("+", MLListT, MLListT) {
	ml_list_t *List1 = (ml_list_t *)Args[0];
	ml_list_t *List2 = (ml_list_t *)Args[1];
	ml_list_t *List = new(ml_list_t);
	List->Type = MLListT;
	int Length = List->Length = List1->Length + List2->Length;
	int Size = (Length + ML_LIST_SIZE - 1) & ~(ML_LIST_SIZE - 1);
	ml_value_t **Nodes = List->Nodes = anew(ml_value_t *, Size);
	memcpy(Nodes, List1->Head, List1->Length * sizeof(ml_value_t *));
	memcpy(Nodes + List1->Length, List2->Head, List2->Length * sizeof(ml_value_t *));
	List->Head = Nodes;
	List->Tail = Nodes + Length;
	List->Space = Size - Length;
	return (ml_value_t *)List;
}

ML_METHOD(MLStringOfMethod, MLListT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	if (!List->Length) return ml_cstring("[]");
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Seperator = "[";
	int SeperatorLength = 1;
	ML_LIST_FOREACH(List, Node) {
		ml_stringbuffer_add(Buffer, Seperator, SeperatorLength);
		ml_value_t *Result = ml_stringbuffer_append(Buffer, Node->Value);
		if (Result->Type == MLErrorT) return Result;
		Seperator = ", ";
		SeperatorLength = 2;
	}
	ml_stringbuffer_add(Buffer, "]", 1);
	return ml_stringbuffer_get_string(Buffer);
}

ML_METHOD("join", MLListT, MLStringT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Seperator = ml_string_value(Args[1]);
	size_t SeperatorLength = ml_string_length(Args[1]);
	int Length = List->Length;
	if (--Length >= 0) {
		ml_value_t **Node = List->Head;
		ml_value_t *Result = ml_stringbuffer_append(Buffer, Node[0]);
		if (Result->Type == MLErrorT) return Result;
		while (--Length >= 0) {
			++Node;
			ml_stringbuffer_add(Buffer, Seperator, SeperatorLength);
			ml_value_t *Result = ml_stringbuffer_append(Buffer, Node[0]);
			if (Result->Type == MLErrorT) return Result;
		}
	}
	return ml_stringbuffer_get_string(Buffer);
}

ML_TYPE(MLNamesT, (MLListT), "names",
	.call = (void *)ml_list_call
);

/****************************** Maps ******************************/

static void ml_map_call(ml_state_t *Caller, ml_value_t *Map, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = Args[0]->Type->deref(Args[0]);
	//if (Arg->Type == MLErrorT) ML_RETURN(Arg);
	ml_value_t *Value = ml_map_search(Map, Arg);
	if (Count > 1) return Value->Type->call(Caller, Value, Count - 1, Args + 1);
	ML_RETURN(Value);
}

ML_TYPE(MLMapT, (MLFunctionT), "map",
	.call = (void *)ml_map_call
);

ml_value_t *ml_map() {
	ml_map_t *Map = new(ml_map_t);
	Map->Type = MLMapT;
	return (ml_value_t *)Map;
}

ml_value_t *ml_map_search(ml_value_t *Map0, ml_value_t *Key) {
	ml_map_t *Map = (ml_map_t *)Map0;
	ml_map_node_t *Node = Map->Root;
	long Hash = Key->Type->hash(Key, NULL);
	while (Node) {
		int Compare;
		if (Hash < Node->Hash) {
			Compare = -1;
		} else if (Hash > Node->Hash) {
			Compare = 1;
		} else {
			ml_value_t *Args[2] = {Key, Node->Key};
			ml_value_t *Result = ml_call(CompareMethod, 2, Args);
			if (Result->Type == MLErrorT) return Result;
			Compare = ml_integer_value(Result);
		}
		if (!Compare) {
			return Node->Value;
		} else {
			Node = Compare < 0 ? Node->Left : Node->Right;
		}
	}
	return MLNil;
}

static int ml_map_balance(ml_map_node_t *Node) {
	int Delta = 0;
	if (Node->Left) Delta = Node->Left->Depth;
	if (Node->Right) Delta -= Node->Right->Depth;
	return Delta;
}

static void ml_map_update_depth(ml_map_node_t *Node) {
	int Depth = 0;
	if (Node->Left) Depth = Node->Left->Depth;
	if (Node->Right && Depth < Node->Right->Depth) Depth = Node->Right->Depth;
	Node->Depth = Depth + 1;
}

static void ml_map_rotate_left(ml_map_node_t **Slot) {
	ml_map_node_t *Ch = Slot[0]->Right;
	Slot[0]->Right = Slot[0]->Right->Left;
	Ch->Left = Slot[0];
	ml_map_update_depth(Slot[0]);
	Slot[0] = Ch;
	ml_map_update_depth(Slot[0]);
}

static void ml_map_rotate_right(ml_map_node_t **Slot) {
	ml_map_node_t *Ch = Slot[0]->Left;
	Slot[0]->Left = Slot[0]->Left->Right;
	Ch->Right = Slot[0];
	ml_map_update_depth(Slot[0]);
	Slot[0] = Ch;
	ml_map_update_depth(Slot[0]);
}

static void ml_map_rebalance(ml_map_node_t **Slot) {
	int Delta = ml_map_balance(Slot[0]);
	if (Delta == 2) {
		if (ml_map_balance(Slot[0]->Left) < 0) ml_map_rotate_left(&Slot[0]->Left);
		ml_map_rotate_right(Slot);
	} else if (Delta == -2) {
		if (ml_map_balance(Slot[0]->Right) > 0) ml_map_rotate_right(&Slot[0]->Right);
		ml_map_rotate_left(Slot);
	}
}

static ml_map_node_t *ml_map_node(ml_map_t *Map, ml_map_node_t **Slot, long Hash, ml_value_t *Key) {
	if (!Slot[0]) {
		++Map->Size;
		ml_map_node_t *Node = Slot[0] = new(ml_map_node_t);
		ml_map_node_t *Prev = Map->Tail;
		if (Prev) {
			Prev->Next = Node;
			Node->Prev = Prev;
		} else {
			Map->Head = Node;
		}
		Map->Tail = Node;
		Node->Depth = 1;
		Node->Hash = Hash;
		Node->Key = Key;
		return Node;
	}
	int Compare;
	if (Hash < Slot[0]->Hash) {
		Compare = -1;
	} else if (Hash > Slot[0]->Hash) {
		Compare = 1;
	} else {
		ml_value_t *Args[2] = {Key, Slot[0]->Key};
		ml_value_t *Result = ml_call(CompareMethod, 2, Args);
		Compare = ml_integer_value(Result);
	}
	if (!Compare) {
		return Slot[0];
	} else {
		ml_map_node_t *Node = ml_map_node(Map, Compare < 0 ? &Slot[0]->Left : &Slot[0]->Right, Hash, Key);
		ml_map_rebalance(Slot);
		ml_map_update_depth(Slot[0]);
		return Node;
	}
}

ml_value_t *ml_map_insert(ml_value_t *Map0, ml_value_t *Key, ml_value_t *Value) {
	ml_map_t *Map = (ml_map_t *)Map0;
	ml_map_node_t *Node = ml_map_node(Map, &Map->Root, Key->Type->hash(Key, NULL), Key);
	ml_value_t *Old = Node->Value ?: MLNil;
	Node->Value = Value;
	return Old;
}

static void ml_map_remove_depth_helper(ml_map_node_t *Node) {
	if (Node) {
		ml_map_remove_depth_helper(Node->Right);
		ml_map_update_depth(Node);
	}
}

static ml_value_t *ml_map_remove_internal(ml_map_t *Map, ml_map_node_t **Slot, long Hash, ml_value_t *Key) {
	if (!Slot[0]) return MLNil;
	ml_map_node_t *Node = Slot[0];
	int Compare;
	if (Hash < Node->Hash) {
		Compare = -1;
	} else if (Hash > Node->Hash) {
		Compare = 1;
	} else {
		ml_value_t *Args[2] = {Key, Node->Key};
		ml_value_t *Result = ml_call(CompareMethod, 2, Args);
		Compare = ml_integer_value(Result);
	}
	ml_value_t *Removed = MLNil;
	if (!Compare) {
		--Map->Size;
		Removed = Node->Value;
		if (Node->Prev) Node->Prev->Next = Node->Next; else Map->Head = Node->Next;
		if (Node->Next) Node->Next->Prev = Node->Prev; else Map->Tail = Node->Prev;
		if (Node->Left && Node->Right) {
			ml_map_node_t **Y = &Node->Left;
			while (Y[0]->Right) Y = &Y[0]->Right;
			ml_map_node_t *Node2 = Y[0];
			Y[0] = Node2->Left;
			Node2->Left = Node->Left;
			Node2->Right = Node->Right;
			Slot[0] = Node2;
			ml_map_remove_depth_helper(Node2->Left);
		} else if (Node->Left) {
			Slot[0] = Node->Left;
		} else if (Node->Right) {
			Slot[0] = Node->Right;
		} else {
			Slot[0] = 0;
		}
	} else {
		Removed = ml_map_remove_internal(Map, Compare < 0 ? &Node->Left : &Node->Right, Hash, Key);
	}
	if (Slot[0]) {
		ml_map_update_depth(Slot[0]);
		ml_map_rebalance(Slot);
	}
	return Removed;
}

ml_value_t *ml_map_delete(ml_value_t *Map0, ml_value_t *Key) {
	ml_map_t *Map = (ml_map_t *)Map0;
	return ml_map_remove_internal(Map, &Map->Root, Key->Type->hash(Key, NULL), Key);
}

int ml_map_foreach(ml_value_t *Value, void *Data, int (*callback)(ml_value_t *, ml_value_t *, void *)) {
	ml_map_t *Map = (ml_map_t *)Value;
	for (ml_map_node_t *Node = Map->Head; Node; Node = Node->Next) {
		if (callback(Node->Key, Node->Value, Data)) return 1;
	}
	return 0;
}

ML_METHOD("size", MLMapT) {
	ml_map_t *Map = (ml_map_t *)Args[0];
	return ml_integer(Map->Size);
}

typedef struct ml_map_index_t {
	const ml_type_t *Type;
	ml_value_t *Map, *Key;
} ml_map_index_t;

static ml_value_t *ml_map_index_deref(ml_map_index_t *Index) {
	return ml_map_search(Index->Map, Index->Key);
}

static ml_value_t *ml_map_index_assign(ml_map_index_t *Index, ml_value_t *Value) {
	ml_map_insert(Index->Map, Index->Key, Value);
	return Value;
}

ML_TYPE(MLMapIndexT, (), "map-index",
	.deref = (void *)ml_map_index_deref,
	.assign = (void *)ml_map_index_assign
);

ML_METHOD("[]", MLMapT, MLAnyT) {
	ml_map_index_t *Index = new(ml_map_index_t);
	Index->Type = MLMapIndexT;
	Index->Map = Args[0];
	Index->Key = Args[1];
	return (ml_value_t *)Index;
}

typedef struct {
	ml_state_t Base;
	ml_value_t **Ref;
} ml_ref_state_t;

static void ml_node_state_run(ml_ref_state_t *State, ml_value_t *Value) {
	if (Value->Type == MLErrorT) {
		ML_CONTINUE(State->Base.Caller, Value);
	} else {
		State->Ref[0] = Value;
		ML_CONTINUE(State->Base.Caller, ml_reference(State->Ref));
	}
}

ML_METHODX("[]", MLMapT, MLAnyT, MLAnyT) {
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_map_node_t *Node = ml_map_node(Map, &Map->Root, Key->Type->hash(Key, NULL), Key);
	if (!Node->Value) {
		Node->Value = MLNil;
		ml_ref_state_t *State = new(ml_ref_state_t);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (void *)ml_node_state_run;
		State->Ref = &Node->Value;
		ml_value_t *Function = Args[2];
		return Function->Type->call((ml_state_t *)State, Function, 0, NULL);
	} else {
		ML_RETURN(ml_reference(&Node->Value));
	}
}

ML_METHOD("::", MLMapT, MLStringT) {
	ml_map_index_t *Index = new(ml_map_index_t);
	Index->Type = MLMapIndexT;
	Index->Map = Args[0];
	Index->Key = Args[1];
	return (ml_value_t *)Index;
}

ML_METHOD("insert", MLMapT, MLAnyT, MLAnyT) {
	ml_value_t *Map = (ml_value_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_value_t *Value = Args[2];
	return ml_map_insert(Map, Key, Value);
}

ML_METHOD("delete", MLMapT, MLAnyT) {
	ml_value_t *Map = (ml_value_t *)Args[0];
	ml_value_t *Key = Args[1];
	return ml_map_delete(Map, Key);
}

ml_value_t *ml_map_fn(void *Data, int Count, ml_value_t **Args) {
	ml_map_t *Map = new(ml_map_t);
	Map->Type = MLMapT;
	for (int I = 0; I < Count; I += 2) ml_map_insert((ml_value_t *)Map, Args[I], Args[I + 1]);
	return (ml_value_t *)Map;
}

ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLMapT, ml_stringbuffer_t *Buffer, ml_map_t *Map) {
	ml_map_node_t *Node = Map->Head;
	if (Node) {
		ml_stringbuffer_append(Buffer, Node->Key);
		if (Node->Value != MLNil) {
			ml_stringbuffer_add(Buffer, "=", 1);
			ml_stringbuffer_append(Buffer, Node->Value);
		}
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, " ", 1);
			ml_stringbuffer_append(Buffer, Node->Key);
			if (Node->Value != MLNil) {
				ml_stringbuffer_add(Buffer, "=", 1);
				ml_stringbuffer_append(Buffer, Node->Value);
			}
		}
		return MLSome;
	} else {
		return MLNil;
	}
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLMapT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_map_t *Map = (ml_map_t *)Args[1];
	ml_map_node_t *Node = Map->Head;
	if (Node) {
		ml_stringbuffer_append(Buffer, Node->Key);
		if (Node->Value != MLNil) {
			ml_stringbuffer_add(Buffer, "=", 1);
			ml_stringbuffer_append(Buffer, Node->Value);
		}
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, " ", 1);
			ml_stringbuffer_append(Buffer, Node->Key);
			if (Node->Value != MLNil) {
				ml_stringbuffer_add(Buffer, "=", 1);
				ml_stringbuffer_append(Buffer, Node->Value);
			}
		}
		return MLSome;
	} else {
		return MLNil;
	}
}

#define ML_TREE_MAX_DEPTH 32

typedef struct ml_map_iterator_t {
	const ml_type_t *Type;
	ml_map_node_t *Node;
	ml_map_node_t *Stack[ML_TREE_MAX_DEPTH];
	int Top;
} ml_map_iterator_t;

static void ML_TYPED_FN(ml_iter_value, MLMapIterT, ml_state_t *Caller, ml_map_iterator_t *Iter) {
	ML_RETURN(ml_reference(&Iter->Node->Value));
}

static void ML_TYPED_FN(ml_iter_next, MLMapIterT, ml_state_t *Caller, ml_map_iterator_t *Iter) {
	ml_map_node_t *Node = Iter->Node;
	if ((Iter->Node = Node->Next)) ML_RETURN(Iter);
	ML_RETURN(MLNil);
}

static void ML_TYPED_FN(ml_iter_key, MLMapIterT, ml_state_t *Caller, ml_map_iterator_t *Iter) {
	ML_RETURN(Iter->Node->Key);
}

ML_TYPE(MLMapIterT, (), "map-iterator");

static void ML_TYPED_FN(ml_iterate, MLMapT, ml_state_t *Caller, ml_value_t *Value) {
	ml_map_t *Map = (ml_map_t *)Value;
	if (Map->Root) {
		ml_map_iterator_t *Iter = new(ml_map_iterator_t);
		Iter->Type = MLMapIterT;
		Iter->Node = Map->Head;
		Iter->Top = 0;
		ML_RETURN(Iter);
	} else {
		ML_RETURN(MLNil);
	}
}

ML_METHOD("+", MLMapT, MLMapT) {
	ml_value_t *Map = ml_map();
	ML_MAP_FOREACH(Args[0], Node) ml_map_insert(Map, Node->Key, Node->Value);
	ML_MAP_FOREACH(Args[1], Node) ml_map_insert(Map, Node->Key, Node->Value);
	return Map;
}

typedef struct ml_map_stringer_t {
	const char *Seperator, *Equals;
	ml_stringbuffer_t Buffer[1];
	int SeperatorLength, EqualsLength, First;
	ml_value_t *Error;
} ml_map_stringer_t;

static int ml_map_stringer(ml_value_t *Key, ml_value_t *Value, ml_map_stringer_t *Stringer) {
	if (!Stringer->First) {
		ml_stringbuffer_add(Stringer->Buffer, Stringer->Seperator, Stringer->SeperatorLength);
	} else {
		Stringer->First = 0;
	}
	Stringer->Error = ml_stringbuffer_append(Stringer->Buffer, Key);
	if (Stringer->Error->Type == MLErrorT) return 1;
	ml_stringbuffer_add(Stringer->Buffer, Stringer->Equals, Stringer->EqualsLength);
	Stringer->Error = ml_stringbuffer_append(Stringer->Buffer, Value);
	if (Stringer->Error->Type == MLErrorT) return 1;
	return 0;
}

ML_METHOD(MLStringOfMethod, MLMapT) {
	ml_map_stringer_t Stringer[1] = {{
		", ", " is ",
		{ML_STRINGBUFFER_INIT},
		2, 4,
		1
	}};
	ml_stringbuffer_add(Stringer->Buffer, "{", 1);
	if (ml_map_foreach(Args[0], Stringer, (void *)ml_map_stringer)) {
		return Stringer->Error;
	}
	ml_stringbuffer_add(Stringer->Buffer, "}", 1);
	return ml_string(ml_stringbuffer_get(Stringer->Buffer), -1);
}

ML_METHOD("join", MLMapT, MLStringT, MLStringT) {
	ml_map_stringer_t Stringer[1] = {{
		ml_string_value(Args[1]), ml_string_value(Args[2]),
		{ML_STRINGBUFFER_INIT},
		ml_string_length(Args[1]), ml_string_length(Args[2]),
		1
	}};
	if (ml_map_foreach(Args[0], Stringer, (void *)ml_map_stringer)) return Stringer->Error;
	return ml_stringbuffer_get_string(Stringer->Buffer);
}

/****************************** Methods ******************************/

#define ML_METHODS_INDEX 0

typedef struct ml_method_t ml_method_t;
typedef struct ml_methods_t ml_methods_t;
typedef struct ml_method_cached_t ml_method_cached_t;
typedef struct ml_method_definition_t ml_method_definition_t;

struct ml_method_t {
	const ml_type_t *Type;
	const char *Name;
};

struct ml_methods_t {
	ml_methods_t *Parent;
	inthash_t Cache[1];
	inthash_t Definitions[1];
	inthash_t Methods[1];
};

static ml_methods_t MLRootMethods[1] = {{NULL, {INTHASH_INIT}, {INTHASH_INIT}}};

void ml_methods_context_new(ml_context_t *Context) {
	ml_methods_t *Methods = new(ml_methods_t);
	Methods->Parent = Context->Values[ML_METHODS_INDEX];
	ml_context_set(Context, ML_METHODS_INDEX, Methods);
}

struct ml_method_cached_t {
	ml_method_cached_t *Next, *MethodNext;
	ml_method_t *Method;
	ml_method_definition_t *Definition;
	int Count, Score;
	const ml_type_t *Types[];
};

struct ml_method_definition_t {
	ml_method_definition_t *Next;
	ml_value_t *Callback;
	int Count, Variadic;
	const ml_type_t *Types[];
};

static unsigned int ml_method_definition_score(ml_method_definition_t *Definition, int Count, const ml_type_t **Types, unsigned int Best) {
	unsigned int Score = 0;
	if (Definition->Count > Count) return 0;
	if (Definition->Count < Count) {
		if (!Definition->Variadic) return 0;
		Count = Definition->Count;
		Score = 1;
	}
	for (int I = 0; I < Count; ++I) {
		const ml_type_t *Type = Definition->Types[I];
		for (const ml_type_t **T = Types[I]->Types; *T; ++T) {
			if (*T == Type) goto found;
		}
		return 0;
	found:
		Score += 5 + Type->Rank;
	}
	return Score;
}

static uintptr_t rotl(uintptr_t X, unsigned int N) {
	const unsigned int Mask = (CHAR_BIT * sizeof(uintptr_t) - 1);
	return (X << (N & Mask)) | (X >> ((-N) & Mask ));
}

static ml_method_cached_t *ml_method_search_entry(ml_methods_t *Methods, ml_method_t *Method, int Count, const ml_type_t **Types, uint64_t Hash) {
	ml_method_cached_t *Cached = inthash_search(Methods->Cache, Hash);
	while (Cached) {
		if (Cached->Method != Method) goto next;
		if (Cached->Count != Count) goto next;
		for (int I = 0; I < Count; ++I) {
			if (Cached->Types[I] != Types[I]) goto next;
		}
		if (!Cached->Definition) break;
		return Cached;
	next:
		Cached = Cached->Next;
	}
	unsigned int BestScore = 0;
	ml_method_definition_t *BestDefinition = NULL;
	ml_method_definition_t *Definition = inthash_search(Methods->Definitions, (uintptr_t)Method);
	while (Definition) {
		unsigned int Score = ml_method_definition_score(Definition, Count, Types, BestScore);
		if (Score > BestScore) {
			BestScore = Score;
			BestDefinition = Definition;
		}
		Definition = Definition->Next;
	}
	if (Methods->Parent) {
		ml_method_cached_t *Cached2 = ml_method_search_entry(Methods->Parent, Method, Count, Types, Hash);
		if (Cached2 && Cached2->Score > BestScore) {
			BestScore = Cached2->Score;
			BestDefinition = Cached2->Definition;
		}
	}
	if (!BestDefinition) return NULL;
	if (!Cached) {
		Cached = xnew(ml_method_cached_t, Count, ml_type_t *);
		Cached->Method = Method;
		Cached->Next = inthash_insert(Methods->Cache, Hash, Cached);
		Cached->MethodNext = inthash_insert(Methods->Methods, (uintptr_t)Method, Cached);
		Cached->Count = Count;
		for (int I = 0; I < Count; ++I) Cached->Types[I] = Types[I];
	}
	Cached->Definition = BestDefinition;
	Cached->Score = BestScore;
	return Cached;
}

static ml_value_t *ml_method_search(ml_methods_t *Methods, ml_method_t *Method, int Count, ml_value_t **Args) {
	// TODO: Use generation numbers to check Methods->Parent for invalidated definitions
	const ml_type_t **Types = alloca(Count * sizeof(ml_type_t *));
	uintptr_t Hash = (uintptr_t)Method;
	for (int I = Count; --I >= 0;) {
		const ml_type_t *Type = Types[I] = Args[I]->Type;
		Hash = rotl(Hash, 1) ^ (uintptr_t)Type;
	}
	ml_method_cached_t *Cached = ml_method_search_entry(Methods, Method, Count, Types, Hash);
	if (Cached) return Cached->Definition->Callback;
	return NULL;
}

void ml_method_insert(ml_methods_t *Methods, ml_method_t *Method, ml_value_t *Callback, int Count, int Variadic, ml_type_t **Types) {
	ml_method_definition_t *Definition = xnew(ml_method_definition_t, Count, ml_type_t *);
	Definition->Callback = Callback;
	Definition->Count = Count;
	Definition->Variadic = Variadic;
	memcpy(Definition->Types, Types, Count * sizeof(ml_type_t *));
	Definition->Next = inthash_insert(Methods->Definitions, (uintptr_t)Method, Definition);
	for (ml_method_cached_t *Cached = inthash_search(Methods->Methods, (uintptr_t)Method); Cached; Cached = Cached->MethodNext) {
		Cached->Definition = NULL;
	}
}

void ml_method_define(ml_value_t *Method, ml_value_t *Function, int Variadic, ...) {
	int Count = 0;
	va_list Args;
	va_start(Args, Variadic);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) ++Count;
	va_end(Args);
	ml_type_t *Types[Count], **T = Types;
	va_start(Args, Variadic);
	while ((Type = va_arg(Args, ml_type_t *))) *T++ = Type;
	va_end(Args);
	ml_method_insert(MLRootMethods, (ml_method_t *)Method, Function, Count, Variadic, Types);
}

static stringmap_t Methods[1] = {STRINGMAP_INIT};

const char *ml_method_name(ml_value_t *Value) {
	return ((ml_method_t *)Value)->Name;
}

static long ml_method_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	ml_method_t *Method = (ml_method_t *)Value;
	long Hash = 5381;
	for (const char *P = Method->Name; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

static void ml_method_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I] = Args[I]->Type->deref(Args[I]);
		//if (Arg->Type == MLErrorT) ML_RETURN(Arg);
	}
	ml_method_t *Method = (ml_method_t *)Value;
	ml_methods_t *Methods = Caller->Context->Values[ML_METHODS_INDEX];
	ml_value_t *Callback = ml_method_search(Methods, Method, Count, Args);

	if (Callback) {
		return Callback->Type->call(Caller, Callback, Count, Args);
	} else {
		int Length = 4;
		for (int I = 0; I < Count; ++I) Length += strlen(Args[I]->Type->Name) + 2;
		char *Types = snew(Length);
		char *P = Types;
#ifdef __MINGW32__
		for (int I = 0; I < Count; ++I) {
			strcpy(P, Args[I]->Type->Path);
			P += strlen(Args[I]->Type->Path);
			strcpy(P, ", ");
			P += 2;
		}
#else
		for (int I = 0; I < Count; ++I) P = stpcpy(stpcpy(P, Args[I]->Type->Name), ", ");
#endif
		P[-2] = 0;
		ML_RETURN(ml_error("MethodError", "no matching method found for %s(%s)", Method->Name, Types));
	}
}

ML_TYPE(MLMethodT, (MLFunctionT), "method",
	.hash = ml_method_hash,
	.call = ml_method_call
);

ml_value_t *ml_method(const char *Name) {
	if (!Name) {
		ml_method_t *Method = new(ml_method_t);
		Method->Type = MLMethodT;
		asprintf((char **)&Method->Name, "<anon:0x%lx>", (uintptr_t)Method);
		return (ml_value_t *)Method;
	}
	ml_method_t **Slot = (ml_method_t **)stringmap_slot(Methods, Name);
	if (!Slot[0]) {
		ml_method_t *Method = new(ml_method_t);
		Method->Type = MLMethodT;
		Method->Name = Name;
		Slot[0] = Method;
	}
	return (ml_value_t *)Slot[0];
}

ML_METHOD(MLMethodOfMethod) {
	return ml_method(NULL);
}

ML_METHOD(MLMethodOfMethod, MLStringT) {
	return ml_method(ml_string_value(Args[0]));
}

void ml_method_by_name(const char *Name, void *Data, ml_callback_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)ml_method(Name);
	int Count = 0;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) ++Count;
	va_end(Args);
	ml_type_t *Types[Count], **T = Types;
	va_start(Args, Callback);
	while ((Type = va_arg(Args, ml_type_t *))) *T++ = Type;
	va_end(Args);
	ml_method_insert(MLRootMethods, Method, ml_cfunction(Data, Callback), Count, 1, Types);
}

void ml_method_by_value(ml_value_t *Value, void *Data, ml_callback_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)Value;
	int Count = 0;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) ++Count;
	va_end(Args);
	ml_type_t *Types[Count], **T = Types;
	va_start(Args, Callback);
	while ((Type = va_arg(Args, ml_type_t *))) *T++ = Type;
	va_end(Args);
	ml_method_insert(MLRootMethods, Method, ml_cfunction(Data, Callback), Count, 1, Types);
}

void ml_methodx_by_name(const char *Name, void *Data, ml_callbackx_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)ml_method(Name);
	int Count = 0;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) ++Count;
	va_end(Args);
	ml_type_t *Types[Count], **T = Types;
	va_start(Args, Callback);
	while ((Type = va_arg(Args, ml_type_t *))) *T++ = Type;
	va_end(Args);
	ml_method_insert(MLRootMethods, Method, ml_cfunctionx(Data, Callback), Count, 1, Types);
}

void ml_methodx_by_value(ml_value_t *Value, void *Data, ml_callbackx_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)Value;
	int Count = 0;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) ++Count;
	va_end(Args);
	ml_type_t *Types[Count], **T = Types;
	va_start(Args, Callback);
	while ((Type = va_arg(Args, ml_type_t *))) *T++ = Type;
	va_end(Args);
	ml_method_insert(MLRootMethods, Method, ml_cfunctionx(Data, Callback), Count, 1, Types);
}

void ml_method_by_array(ml_value_t *Value, ml_value_t *Function, int Count, ml_type_t **Types) {
	ml_method_t *Method = (ml_method_t *)Value;
	ml_method_insert(MLRootMethods, Method, Function, Count, 1, Types);
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLMethodT, ml_method_t *Method) {
	return ml_string_format(":%s", Method->Name);
}

ML_METHOD(MLStringOfMethod, MLMethodT) {
	ml_method_t *Method = (ml_method_t *)Args[0];
	return ml_string_format(":%s", Method->Name);
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLMethodT, ml_stringbuffer_t *Buffer, ml_method_t *Value) {
	ml_stringbuffer_add(Buffer, Value->Name, strlen(Value->Name));
	return MLSome;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLMethodT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_method_t *Method = (ml_method_t *)Args[1];
	ml_stringbuffer_add(Buffer, Method->Name, strlen(Method->Name));
	return MLSome;
}

ML_METHOD_DECL(MLRange, "..");

ML_FUNCTIONX(MLMethodSet) {
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(0, MLMethodT);
	int NumTypes = Count - 2, Variadic = 0;
	if (Count >= 3 && Args[Count - 2] == MLRangeMethod) {
		Variadic = 1;
		--NumTypes;
	}
	ml_type_t *Types[NumTypes];
	for (int I = 1; I <= NumTypes; ++I) {
		if (Args[I] == MLNil) {
			Types[I - 1] = MLNilT;
		} else {
			ML_CHECKX_ARG_TYPE(I, MLTypeT);
			Types[I - 1] = (ml_type_t *)Args[I];
		}
	}
	ML_CHECKX_ARG_TYPE(Count - 1, MLFunctionT);
	ml_method_t *Method = (ml_method_t *)Args[0];
	ml_value_t *Function = Args[Count - 1];
	ml_methods_t *Methods = Caller->Context->Values[ML_METHODS_INDEX];
	ml_method_insert(Methods, Method, Function, NumTypes, Variadic, Types);
	ML_RETURN(Function);
}

/****************************** Modules ******************************/

typedef struct ml_module_t ml_module_t;

struct ml_module_t {
	const ml_type_t *Type;
	const char *Path;
	stringmap_t Exports[1];
};

ML_TYPE(MLModuleT, (), "module");

ML_METHODX("::", MLModuleT, MLStringT) {
	ml_module_t *Module = (ml_module_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t *Value = stringmap_search(Module->Exports, Name) ?: ml_error("ModuleError", "Symbol %s not exported from module %s", Name, Module->Path);
	ML_RETURN(Value);
}

ml_value_t *ml_module(const char *Path, ...) {
	ml_module_t *Module = new(ml_module_t);
	Module->Type = MLModuleT;
	Module->Path = Path;
	va_list Args;
	va_start(Args, Path);
	const char *Export;
	while ((Export = va_arg(Args, const char *))) {
		stringmap_insert(Module->Exports, Export, va_arg(Args, ml_value_t *));
	}
	va_end(Args);
	return (ml_value_t *)Module;
}

const char *ml_module_path(ml_value_t *Module) {
	return ((ml_module_t *)Module)->Path;
}

ml_value_t *ml_module_import(ml_value_t *Module0, const char *Name) {
	ml_module_t *Module = (ml_module_t *)Module0;
	return (ml_value_t *)stringmap_search(Module->Exports, Name);
}

ml_value_t *ml_module_export(ml_value_t *Module0, const char *Name, ml_value_t *Value) {
	ml_module_t *Module = (ml_module_t *)Module0;
	stringmap_insert(Module->Exports, Name, Value);
	return Value;
}

ML_METHOD(MLStringOfMethod, MLModuleT) {
	ml_module_t *Module = (ml_module_t *)Args[0];
	return ml_string_format("module(%s)", Module->Path);
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLModuleT, ml_module_t *Module) {
	return ml_string_format("module(%s)", Module->Path);
}

/****************************** Init ******************************/

void ml_init() {
#ifdef USE_ML_JIT
	GC_set_pages_executable(1);
#endif
	GC_INIT();
	GC_word StringBufferLayout[] = {1};
	StringBufferDesc = GC_make_descriptor(StringBufferLayout, 1);
#include "ml_types_init.c"
	ml_method_by_name("<>", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("<>", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name("=", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("=", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name("!=", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("!=", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name("<", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("<", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name(">", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name(">", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name("<=", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("<=", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name(">=", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name(">=", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_value(MLIntegerOfMethod, NULL, ml_identity, MLIntegerT, NULL);
	ml_method_by_value(MLRealOfMethod, NULL, ml_identity, MLRealT, NULL);
	ml_method_by_value(MLStringOfMethod, NULL, ml_identity, MLStringT, NULL);
	ml_method_by_value(MLMethodOfMethod, NULL, ml_identity, MLMethodT, NULL);
	ml_context_set(&MLRootContext, ML_METHODS_INDEX, MLRootMethods);
	ml_runtime_init();
	ml_bytecode_init();
}

void ml_types_init(stringmap_t *Globals) {
	stringmap_insert(Globals, "any", MLAnyT);
	stringmap_insert(Globals, "type", MLTypeT);
	stringmap_insert(MLTypeT->Exports, "of", MLTypeOf);
	stringmap_insert(Globals, "function", MLFunctionT);
	stringmap_insert(Globals, "iteratable", MLIteratableT);
	stringmap_insert(Globals, "boolean", MLBooleanT);
	stringmap_insert(MLBooleanT->Exports, "of", MLBooleanOfMethod);
	stringmap_insert(Globals, "true", MLTrue);
	stringmap_insert(Globals, "false", MLFalse);
	stringmap_insert(Globals, "number", MLNumberT);
	stringmap_insert(Globals, "integer", MLIntegerT);
	stringmap_insert(MLIntegerT->Exports, "of", MLIntegerOfMethod);
	stringmap_insert(Globals, "real", MLRealT);
	stringmap_insert(MLRealT->Exports, "of", MLRealOfMethod);
	stringmap_insert(Globals, "buffer", MLBufferT);
	stringmap_insert(MLBufferT->Exports, "new", ml_cfunction(NULL, ml_buffer));
	stringmap_insert(Globals, "string", MLStringT);
	stringmap_insert(MLStringT->Exports, "of", MLStringOfMethod);
	stringmap_insert(Globals, "stringbuffer", MLStringBufferT);
	stringmap_insert(Globals, "regex", MLRegexT);
	stringmap_insert(MLRegexT->Exports, "of", MLRegex);
	stringmap_insert(Globals, "method", MLMethodT);
	stringmap_insert(MLMethodT->Exports, "of", MLMethodOfMethod);
	stringmap_insert(MLMethodT->Exports, "set", MLMethodSet);
	stringmap_insert(Globals, "list", MLListT);
	stringmap_insert(MLListT->Exports, "of", MLListOfMethod);
	stringmap_insert(Globals, "names", MLNamesT);
	stringmap_insert(Globals, "map", MLMapT);
	stringmap_insert(MLMapT->Exports, "of", MLMapOfMethod);
	stringmap_insert(Globals, "tuple", MLTupleT);
	stringmap_insert(MLTupleT->Exports, "of", MLTuple);
}

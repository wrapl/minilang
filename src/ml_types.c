#include "minilang.h"
#include "ml_macros.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <gc.h>
#include <gc/gc_typed.h>
#ifdef USE_TRE
#include <tre/regex.h>
#else
#include <regex.h>
#endif
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
ML_METHOD_DECL(MLNumberOf, NULL);
ML_METHOD_DECL(MLIntegerOf, NULL);
ML_METHOD_DECL(MLRealOf, NULL);
ML_METHOD_DECL(MLMethodOf, NULL);
ML_METHOD_DECL(MLListOf, NULL);
ML_METHOD_DECL(MLMapOf, NULL);

// Types //

ML_INTERFACE(MLAnyT, (), "any");
// Base type for all values.

static void ml_type_call(ml_state_t *Caller, ml_type_t *Type, int Count, ml_value_t **Args) {
	ml_value_t *Constructor = Type->Constructor;
	if (!Constructor) {
		Constructor = Type->Constructor = stringmap_search(Type->Exports, "of");
	}
	if (!Constructor) ML_RETURN(ml_error("TypeError", "No constructor for <%s>", Type->Name));
	return ml_call(Caller, Constructor, Count, Args);
}

ML_INTERFACE(MLIteratableT, (), "iteratable");
//!iterator
// The base type for any iteratable value.

ML_INTERFACE(MLFunctionT, (), "function");
//!function
// The base type of all functions.

ML_FUNCTION(MLTypeOf) {
//!type
//@type
//<Value
//>type
// Returns the type of :mini:`Value`.
	ML_CHECK_ARG_COUNT(1);
	return (ml_value_t *)ml_typeof(Args[0]);
}

ML_INTERFACE(MLTypeT, (MLFunctionT), "type",
//!type
// Type of all types.
// Every type contains a set of named exports, which allows them to be used as modules.
	.call = (void *)ml_type_call,
	.Constructor = (ml_value_t *)MLTypeOf
);

ML_METHOD("rank", MLTypeT) {
//!type
//<Type
//>integer
// Returns the rank of :mini:`Type`, i.e. the depth of its inheritence tree.
	ml_type_t *Type = (ml_type_t *)Args[0];
	return ml_integer(Type->Rank);
}

void ml_default_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	ML_RETURN(ml_error("TypeError", "<%s> is not callable", ml_typeof(Value)->Name));
}

long ml_default_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	long Hash = 5381;
	for (const char *P = ml_typeof(Value)->Name; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

ml_value_t *ml_default_deref(ml_value_t *Ref) {
	return Ref;
}

ml_value_t *ml_default_assign(ml_value_t *Ref, ml_value_t *Value) {
	return ml_error("TypeError", "<%s> is not assignable", ml_typeof(Ref)->Name);
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
			fprintf(stderr, "Types initialized in wrong order %s < %s\n", Type->Name, Parent->Name);
			exit(1);
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
//!type
//<Type
//>string
// Returns a string representing :mini:`Type`.
	ml_type_t *Type = (ml_type_t *)Args[0];
	return ml_string_format("<<%s>>", Type->Name);
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLTypeT, ml_type_t *Type) {
	return ml_string_format("<<%s>>", Type->Name);
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLTypeT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_type_t *Type = (ml_type_t *)Args[1];
	ml_stringbuffer_addf(Buffer, "<<%s>>", Type->Name);
	return Args[0];
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLTypeT, ml_stringbuffer_t *Buffer, ml_type_t *Type) {
	ml_stringbuffer_addf(Buffer, "<<%s>>", Type->Name);
	return (ml_value_t *)Buffer;
}

ML_METHOD("::", MLTypeT, MLStringT) {
//!type
//<Type
//<Name
//>any | error
// Returns the value of :mini:`Name` exported from :mini:`Type`.
// Returns an error if :mini:`Name` is not present.
// This allows types to behave as modules.
	ml_type_t *Type = (ml_type_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t *Value = strcmp(Name, "of") ? stringmap_search(Type->Exports, Name) : Type->Constructor;
	return Value ?: ml_error("ModuleError", "Symbol %s not exported from type %s", Name, Type->Name);
}

// Values //

ML_TYPE(MLNilT, (MLFunctionT, MLIteratableT), "nil");
//!internal

ML_TYPE(MLSomeT, (), "some");
//!internal

ML_TYPE(MLBlankT, (), "blank");
//!internal

ML_VALUE(MLNil, MLNilT);
ML_VALUE(MLSome, MLSomeT);
ML_VALUE(MLBlank, MLBlankT);

int ml_is(const ml_value_t *Value, const ml_type_t *Expected) {
	for (const ml_type_t **Parents = ml_typeof(Value)->Types, *Type = Parents[0]; Type; Type = *++Parents) {
		if (Type == Expected) return 1;
	}
	return 0;
}

static void ML_TYPED_FN(ml_iterate, MLNilT, ml_state_t *Caller, ml_value_t *Value) {
	ML_RETURN(Value);
}

ML_METHOD("?", MLAnyT) {
//<Value
//>type
// Returns the type of :mini:`Value`.
	return (ml_value_t *)ml_typeof(Args[0]);
}

ML_METHOD("isa", MLAnyT, MLTypeT) {
//<Value
//<Type
//>Value | nil
// Returns :mini:`Value` if it is an instance of :mini:`Type` or a type that inherits from :mini:`Type`.
// Returns :mini:`nil` otherwise.
	return ml_is(Args[0], (ml_type_t *)Args[1]) ? Args[0] : MLNil;
}

long ml_hash_chain(ml_value_t *Value, ml_hash_chain_t *Chain) {
	Value = ml_deref(Value);
	for (ml_hash_chain_t *Link = Chain; Link; Link = Link->Previous) {
		if (Link->Value == Value) return Link->Index;
	}
	ml_hash_chain_t NewChain[1] = {{Chain, Value, Chain ? Chain->Index + 1 : 1}};
	return ml_typeof(Value)->hash(Value, NewChain);
}

long ml_hash(ml_value_t *Value) {
	return ml_hash_chain(Value, NULL);
}

#ifdef USE_NANBOXING

typedef struct {
	const ml_type_t *Type;
	int64_t Value;
} ml_int64_t;

#define NegOne ml_int32(-1)
#define One ml_int32(1)
#define Zero ml_int32(0)

#else

typedef struct ml_integer_t {
	const ml_type_t *Type;
	long Value;
} ml_integer_t;

static ml_integer_t One[1] = {{MLIntegerT, 1}};
static ml_integer_t NegOne[1] = {{MLIntegerT, -1}};
static ml_integer_t Zero[1] = {{MLIntegerT, 0}};

#endif

ML_METHOD("<>", MLAnyT, MLAnyT) {
//<Value/1
//<Value/2
//>integer
// Compares :mini:`Value/1` and :mini:`Value/2` and returns :mini:`-1`, :mini:`0` or :mini:`1`.
// This comparison is based on the internal addresses of :mini:`Value/1` and :mini:`Value/2` and thus only has no persistent meaning.
	if (Args[0] < Args[1]) return (ml_value_t *)NegOne;
	if (Args[0] > Args[1]) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("#", MLAnyT) {
//<Value
//>integer
// Returns a hash for :mini:`Value` for use in lookup tables, etc.
	ml_value_t *Value = Args[0];
	return ml_integer(ml_typeof(Value)->hash(Value, NULL));
}

ML_METHOD("=", MLAnyT, MLAnyT) {
//<Value/1
//<Value/2
//>Value/2 | nil
// Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are exactly the same instance.
// Returns :mini:`nil` otherwise.
	return (Args[0] == Args[1]) ? Args[1] : MLNil;
}

ML_METHOD("!=", MLAnyT, MLAnyT) {
//<Value/1
//<Value/2
//>Value/2 | nil
// Returns :mini:`Value2` if :mini:`Value1` and :mini:`Value2` are not exactly the same instance.
// Returns :mini:`nil` otherwise.
	return (Args[0] != Args[1]) ? Args[1] : MLNil;
}

ML_METHOD(MLStringOfMethod, MLAnyT) {
//<Value
//>string
// Returns a general (type name only) representation of :mini:`Value` as a string.
	return ml_string_format("<%s>", ml_typeof(Args[0])->Name);
}

ML_METHODV(MLStringBufferAppendMethod, MLStringBufferT, MLAnyT) {
	ml_value_t *String = ml_simple_call(MLStringOfMethod, Count - 1, Args + 1);
	if (ml_is_error(String)) return String;
	if (!ml_is(String, MLStringT)) return ml_error("TypeError", "String expected, not %s", ml_typeof(String)->Name);
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	int Length = ml_string_length(String);
	if (Length) {
		ml_stringbuffer_add(Buffer, ml_string_value(String), Length);
	}
	return (ml_value_t *)Buffer;
}

// Iterators //

void ml_iterate(ml_state_t *Caller, ml_value_t *Value) {
	typeof(ml_iterate) *function = ml_typed_fn_get(ml_typeof(Value), ml_iterate);
	if (!function) {
		ml_value_t **Args = anew(ml_value_t *, 1);
		Args[0] = Value;
		return ml_call(Caller, IterateMethod, 1, Args);
	}
	return function(Caller, Value);
}

void ml_iter_value(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_value) *function = ml_typed_fn_get(ml_typeof(Iter), ml_iter_value);
	if (!function) {
		ml_value_t **Args = anew(ml_value_t *, 1);
		Args[0] = Iter;
		return ml_call(Caller, ValueMethod, 1, Args);
	}
	return function(Caller, Iter);
}

void ml_iter_key(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_key) *function = ml_typed_fn_get(ml_typeof(Iter), ml_iter_key);
	if (!function) {
		ml_value_t **Args = anew(ml_value_t *, 1);
		Args[0] = Iter;
		return ml_call(Caller, KeyMethod, 1, Args);
	}
	return function(Caller, Iter);
}

void ml_iter_next(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_next) *function = ml_typed_fn_get(ml_typeof(Iter), ml_iter_next);
	if (!function) {
		ml_value_t **Args = anew(ml_value_t *, 1);
		Args[0] = Iter;
		return ml_call(Caller, NextMethod, 1, Args);
	}
	return function(Caller, Iter);
}

// Functions //

ML_METHODX("!", MLFunctionT, MLTupleT) {
//!function
//<Function
//<Tuple
//>any
// Calls :mini:`Function` with the values in :mini:`Tuple` as positional arguments.
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[1];
	ml_value_t *Function = Args[0];
	return ml_call(Caller, Function, Tuple->Size, Tuple->Values);
}

ML_METHODX("!", MLFunctionT, MLListT) {
//!function
//<Function
//<List
//>any
// Calls :mini:`Function` with the values in :mini:`List` as positional arguments.
	int Count2 = ml_list_length(Args[1]);
	ml_value_t **Args2 = anew(ml_value_t *, Count2);
	ml_list_to_array(Args[1], Args2);
	ml_value_t *Function = Args[0];
	return ml_call(Caller, Function, Count2, Args2);
}

ML_METHODX("!", MLFunctionT, MLMapT) {
//!function
//<Function
//<Map
//>any
// Calls :mini:`Function` with the keys and values in :mini:`Map` as named arguments.
// Returns an error if any of the keys in :mini:`Map` is not a string or method.
	int Count2 = ml_map_size(Args[1]) + 1;
	ml_value_t **Args2 = anew(ml_value_t *, Count2);
	ml_value_t *Names = ml_names();
	ml_value_t **Arg = Args2;
	*(Arg++) = Names;
	ML_MAP_FOREACH(Args[1], Node) {
		ml_value_t *Name = Node->Key;
		if (ml_is(Name, MLMethodT)) {
			ml_names_add(Names, Name);
		} else if (ml_is(Name, MLStringT)) {
			ml_names_add(Names, ml_method(ml_string_value(Name)));
		} else {
			ML_RETURN(ml_error("TypeError", "Parameter names must be strings or methods"));
		}
		*(Arg++) = Node->Value;
	}
	ml_value_t *Function = Args[0];
	return ml_call(Caller, Function, Count2, Args2);
}

ML_METHODX("!", MLFunctionT, MLTupleT, MLMapT) {
//!function
//<Function
//<Tuple
//<Map
//>any
// Calls :mini:`Function` with the values in :mini:`Tuple` as positional arguments and the keys and values in :mini:`Map` as named arguments.
// Returns an error if any of the keys in :mini:`Map` is not a string or method.
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[1];
	int TupleCount = Tuple->Size;
	int MapCount = ml_map_size(Args[2]);
	int Count2 = TupleCount + MapCount + 1;
	ml_value_t **Args2 = anew(ml_value_t *, Count2);
	memcpy(Args2, Tuple->Values, TupleCount * sizeof(ml_value_t *));
	ml_value_t *Names = ml_names();
	ml_value_t **Arg = Args2 + TupleCount;
	*(Arg++) = Names;
	ML_MAP_FOREACH(Args[2], Node) {
		ml_value_t *Name = Node->Key;
		if (ml_is(Name, MLMethodT)) {
			ml_names_add(Names, Name);
		} else if (ml_is(Name, MLStringT)) {
			ml_names_add(Names, ml_method(ml_string_value(Name)));
		} else {
			ML_RETURN(ml_error("TypeError", "Parameter names must be strings or methods"));
		}
		*(Arg++) = Node->Value;
	}
	ml_value_t *Function = Args[0];
	return ml_call(Caller, Function, Count2, Args2);
}

ML_METHODX("!", MLFunctionT, MLListT, MLMapT) {
//!function
//<Function
//<List
//<Map
//>any
// Calls :mini:`Function` with the values in :mini:`List` as positional arguments and the keys and values in :mini:`Map` as named arguments.
// Returns an error if any of the keys in :mini:`Map` is not a string or method.
	int ListCount = ml_list_length(Args[1]);
	int MapCount = ml_map_size(Args[2]);
	int Count2 = ListCount + MapCount + 1;
	ml_value_t **Args2 = anew(ml_value_t *, Count2);
	ml_list_to_array(Args[1], Args2);
	ml_value_t *Names = ml_names();
	ml_value_t **Arg = Args2 + ListCount;
	*(Arg++) = Names;
	ML_MAP_FOREACH(Args[2], Node) {
		ml_value_t *Name = Node->Key;
		if (ml_is(Name, MLMethodT)) {
			ml_names_add(Names, Name);
		} else if (ml_is(Name, MLStringT)) {
			ml_names_add(Names, ml_method(ml_string_value(Name)));
		} else {
			ML_RETURN(ml_error("TypeError", "Parameter names must be strings or methods"));
		}
		*(Arg++) = Node->Value;
	}
	ml_value_t *Function = Args[0];
	return ml_call(Caller, Function, Count2, Args2);
}

static void ml_cfunction_call(ml_state_t *Caller, ml_cfunction_t *Function, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) {
		Args[I] = ml_deref(Args[I]);
		//if (ml_is_error(Args[I])) ML_RETURN(Args[I]);
	}
	ML_RETURN((Function->Callback)(Function->Data, Count, Args));
}

ML_TYPE(MLCFunctionT, (MLFunctionT), "c-function",
//!internal
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
		Args[I] = ml_deref(Args[I]);
		//if (ml_is_error(Args[I])) ML_RETURN(Args[I]);
	}
	return (Function->Callback)(Caller, Function->Data, Count, Args);
}

ML_TYPE(MLCFunctionXT, (MLFunctionT), "c-functionx",
//!internal
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
	return ml_call(Caller, Partial->Function, CombinedCount, CombinedArgs);
}

ML_TYPE(MLPartialFunctionT, (MLFunctionT), "partial-function",
//!function
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
//!function
//<Function
//<List
//>partialfunction
// Returns a function equivalent to :mini:`fun(Args...) Function(List..., Args...)`.
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
//!function
//<Function
//<Arg
//>partialfunction
// Returns a function equivalent to :mini:`fun(Args...) Function(Arg, Args...)`.
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, 1, ml_value_t *);
	Partial->Type = MLPartialFunctionT;
	Partial->Function = Args[0];
	Partial->Count = Partial->Set = 1;
	Partial->Args[0] = Args[1];
	return (ml_value_t *)Partial;
}

ML_METHOD("$", MLPartialFunctionT, MLAnyT) {
//!internal
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
	return ml_call(Caller, Partial->Function, Partial->Count, Partial->Args);
}

// Tuples //

static long ml_tuple_hash(ml_tuple_t *Tuple, ml_hash_chain_t *Chain) {
	long Hash = 739;
	for (int I = 0; I < Tuple->Size; ++I) Hash = ((Hash << 3) + Hash) + ml_hash(Tuple->Values[I]);
	return Hash;
}

static ml_value_t *ml_tuple_deref(ml_tuple_t *Ref) {
	if (Ref->NoRefs) return (ml_value_t *)Ref;
	for (int I = 0; I < Ref->Size; ++I) {
		ml_value_t *Old = Ref->Values[I];
		ml_value_t *New = ml_deref(Old);
		if (Old != New) {
			ml_tuple_t *Deref = xnew(ml_tuple_t, Ref->Size, ml_value_t *);
			Deref->Type = MLTupleT;
			Deref->Size = Ref->Size;
			Deref->NoRefs = 1;
			for (int J = 0; J < I; ++J) Deref->Values[J] = Ref->Values[J];
			Deref->Values[I] = New;
			for (int J = I + 1; J < Ref->Size; ++J) {
				Deref->Values[J] = ml_deref(Ref->Values[J]);
			}
			return (ml_value_t *)Deref;
		}
	}
	Ref->NoRefs = 1;
	return (ml_value_t *)Ref;
}

static ml_value_t *ml_tuple_assign(ml_tuple_t *Ref, ml_value_t *Values) {
	int Count = Ref->Size;
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Value = ml_unpack(Values, I);
		if (!Value) return ml_error("ValueError", "Not enough values to unpack (%d < %d)", I, Count);
		ml_value_t *Result = ml_typeof(Ref->Values[I])->assign(Ref->Values[I], Value);
		if (ml_is_error(Result)) return Result;
	}
	return Values;
}

ML_FUNCTION(MLTuple) {
//!tuple
//@tuple
//<Value/1
//<:...
//<Value/n
//>tuple
// Returns a tuple of values :mini:`Value/1, ..., Value/n`.
	ml_value_t *Tuple = ml_tuple(Count);
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Value = ml_deref(Args[I]);
		//if (ml_is_error(Value)) return Value;
		ml_tuple_set(Tuple, I + 1, Value);
	}
	return Tuple;
}

ML_TYPE(MLTupleT, (), "tuple",
//!tuple
// An immutable tuple of values.
	.hash = (void *)ml_tuple_hash,
	.deref = (void *)ml_tuple_deref,
	.assign = (void *)ml_tuple_assign,
	.Constructor = (ml_value_t *)MLTuple
);

ml_value_t *ml_tuple(size_t Size) {
	ml_tuple_t *Tuple = xnew(ml_tuple_t, Size, ml_value_t *);
	Tuple->Type = MLTupleT;
	Tuple->Size = Size;
	return (ml_value_t *)Tuple;
}

ml_value_t *ml_unpack(ml_value_t *Value, int Index) {
	typeof(ml_unpack) *function = ml_typed_fn_get(ml_typeof(Value), ml_unpack);
	if (!function) return NULL;
	return function(Value, Index);
}

ML_METHOD("size", MLTupleT) {
//!tuple
//<Tuple
//>integer
// Returns the number of elements in :mini:`Tuple`.
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	return ml_integer(Tuple->Size);
}

ML_METHOD("[]", MLTupleT, MLIntegerT) {
//!tuple
//<Tuple
//<Index
//>any | error
// Returns the :mini:`Index`-th element in :mini:`Tuple` or an error if :mini:`Index` is out of range.
// Indexing starts at :mini:`1`. Negative indices count from the end, with :mini:`-1` returning the last element.
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	long Index = ml_integer_value(Args[1]);
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
	return ml_stringbuffer_value(Buffer);
}

ML_METHOD(MLStringOfMethod, MLTupleT) {
//!tuple
//<Tuple
//>string
// Returns a string representation of :mini:`Tuple`.
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
	return ml_stringbuffer_value(Buffer);
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
	return (ml_value_t *)Buffer;
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
	return (ml_value_t *)Buffer;
}

ml_value_t *ML_TYPED_FN(ml_unpack, MLTupleT, ml_tuple_t *Tuple, int Index) {
	if (Index >= Tuple->Size) return NULL;
	return Tuple->Values[Index];
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
		ml_value_t *C = ml_simple_call(CompareMethod, 2, Args);
		if (ml_is_error(C)) return C;
		if (ml_integer_value(C)) return C;
	}
	return Result;
}

ML_METHOD("<>", MLTupleT, MLTupleT) {
//!tuple
//<Tuple/1
//<Tuple/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Tuple/1` is less than, equal to or greater than :mini:`Tuple/2` using lexicographical ordering.
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

#if 0
ML_METHOD("<op>", MLTupleT, MLTupleT) {
//!tuple
//<Tuple/1
//<Tuple/2
//>Tuple/2 | nil
// :mini:`<op>` is :mini:`=`, :mini:`!=`, :mini:`<`, :mini:`<=`, :mini:`>` or :mini:`>=`
// Returns :mini:`Tuple/2` if :mini:`Tuple/2 <op> Tuple/1` is true, otherwise returns :mini:`nil`.
}
#endif

// Boolean //

static long ml_boolean_hash(ml_boolean_t *Boolean, ml_hash_chain_t *Chain) {
	return (long)Boolean;
}

ML_TYPE(MLBooleanT, (MLFunctionT), "boolean",
//!boolean
	.hash = (void *)ml_boolean_hash
);

int ml_boolean_value(const ml_value_t *Value) {
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
//!boolean
//<String
//>boolean | error
// Returns :mini:`true` if :mini:`String` equals :mini:`"true"` (ignoring case).
// Returns :mini:`false` if :mini:`String` equals :mini:`"false"` (ignoring case).
// Otherwise returns an error.
	const char *Name = ml_string_value(Args[0]);
	if (!strcasecmp(Name, "true")) return (ml_value_t *)MLTrue;
	if (!strcasecmp(Name, "false")) return (ml_value_t *)MLFalse;
	return ml_error("ValueError", "Invalid boolean: %s", Name);
}

ML_METHOD("-", MLBooleanT) {
//!boolean
//<Bool
//>boolean
// Returns the logical inverse of :mini:`Bool`
	return MLBooleans[1 - ml_boolean_value(Args[0])];
}

ML_METHOD("/\\", MLBooleanT, MLBooleanT) {
//!boolean
//<Bool/1
//<Bool/2
//>boolean
// Returns the logical and of :mini:`Bool/1` and :mini:`Bool/2`.
	return MLBooleans[ml_boolean_value(Args[0]) & ml_boolean_value(Args[1])];
}

ML_METHOD("\\/", MLBooleanT, MLBooleanT) {
//!boolean
//<Bool/1
//<Bool/2
//>boolean
// Returns the logical or of :mini:`Bool/1` and :mini:`Bool/2`.
	return MLBooleans[ml_boolean_value(Args[0]) | ml_boolean_value(Args[1])];
}

ML_METHOD("<>", MLBooleanT, MLBooleanT) {
//!boolean
//<Bool/1
//<Bool/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Bool/1` is less than, equal to or greater than :mini:`Bool/2`. :mini:`true` is considered greater than :mini:`false`.
	ml_boolean_t *BooleanA = (ml_boolean_t *)Args[0];
	ml_boolean_t *BooleanB = (ml_boolean_t *)Args[1];
	return ml_integer(BooleanA->Value - BooleanB->Value);
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

#if 0
ML_METHOD("<op>", MLBooleanT, MLBooleanT) {
//!boolean
//<Bool/1
//<Bool/2
//>Bool/2 | nil
// :mini:`<op>` is :mini:`=`, :mini:`!=`, :mini:`<`, :mini:`<=`, :mini:`>` or :mini:`>=`
// Returns :mini:`Bool/2` if :mini:`Bool/2 <op> Bool/1` is true, otherwise returns :mini:`nil`.
// :mini:`true` is considered greater than :mini:`false`.
}
#endif

// Numbers //

ML_TYPE(MLNumberT, (MLFunctionT), "number");
//!number
// Base type for integers and reals.

#ifdef USE_NANBOXING

static long ml_int32_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return (uint64_t)Value & 0xFFFFFFFF;
}

static void ml_int32_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	long Index = (uint64_t)Value & 0xFFFFFFFF;
	if (Index <= 0) Index += Count + 1;
	if (Index <= 0) ML_RETURN(MLNil);
	if (Index > Count) ML_RETURN(MLNil);
	ML_RETURN(Args[Index - 1]);
}

ML_TYPE(MLIntegerT, (MLNumberT), "integer");
//!number

ML_TYPE(MLInt32T, (MLIntegerT), "int32",
//!number
	.hash = (void *)ml_int32_hash,
	.call = (void *)ml_int32_call
);

static long ml_int64_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return ((ml_int64_t *)Value)->Value;
}

ML_TYPE(MLInt64T, (MLIntegerT), "int64",
//!number
	.hash = (void *)ml_int64_hash
);

ml_value_t *ml_integer(const int64_t Integer) {
	if (Integer >= INT32_MIN && Integer <= INT32_MAX) {
		return ml_int32(Integer);
	} else {
		ml_int64_t *Value = new(ml_int64_t);
		Value->Type = MLInt64T;
		Value->Value = Integer;
		return (ml_value_t *)Value;
	}
}

long ml_integer_value(const ml_value_t *Value) {
	int Tag = ml_tag(Value);
	if (Tag == 1) return (int32_t)(intptr_t)Value;
	if (Tag >= 7) return ml_to_double(Value);
	if (Tag == 0) {
		if (Value->Type == MLInt64T) {
			return ((ml_int64_t *)Value)->Value;
		}
	}
	return 0;
}

#else

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
//!number
	.hash = (void *)ml_integer_hash,
	.call = (void *)ml_integer_call
);

ml_value_t *ml_integer(long Value) {
	ml_integer_t *Integer = new(ml_integer_t);
	Integer->Type = MLIntegerT;
	Integer->Value = Value;
	return (ml_value_t *)Integer;
}

long ml_integer_value(const ml_value_t *Value) {
	if (Value->Type == MLIntegerT) {
		return ((ml_integer_t *)Value)->Value;
	} else if (Value->Type == MLRealT) {
		return ((ml_real_t *)Value)->Value;
	} else {
		return 0;
	}
}

#endif

ml_value_t *ml_integer_of(ml_value_t *Value) {
	typeof(ml_integer_of) *function = ml_typed_fn_get(ml_typeof(Value), ml_string_of);
	if (!function) return ml_simple_inline(MLIntegerOfMethod, 1, Value);
	return function(Value);
}

static ml_value_t *ML_TYPED_FN(ml_integer_of, MLIntegerT, ml_value_t *Value) {
	return Value;
}

#ifdef USE_NANBOXING

static ml_value_t *ML_TYPED_FN(ml_integer_of, MLRealT, ml_value_t *Value) {
	return ml_integer(ml_to_double(Value));
}

ML_METHOD(MLIntegerOfMethod, MLRealT) {
//!number
	return ml_integer(ml_to_double(Args[0]));
}

ML_TYPE(MLRealT, (MLNumberT), "real");
//!number

static long ml_double_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return (long)ml_to_double(Value);
}

ML_TYPE(MLDoubleT, (MLRealT), "double",
//!number
	.hash = (void *)ml_double_hash
);

ml_value_t *ml_real(double Value) {
	union { ml_value_t *Value; uint64_t Bits; double Double; } Boxed;
	Boxed.Double = Value;
	Boxed.Bits += 0x07000000000000;
	return Boxed.Value;
}

double ml_real_value(const ml_value_t *Value) {
	int Tag = ml_tag(Value);
	if (Tag == 1) return (uint64_t)Value & 0xFFFFFFFF;
	if (Tag >= 7) return ml_to_double(Value);
	if (Tag == 0) {
		if (Value->Type == MLInt64T) {
			return ((ml_int64_t *)Value)->Value;
		}
	}
	return 0;
}


#else

static ml_value_t *ML_TYPED_FN(ml_integer_of, MLRealT, ml_real_t *Real) {
	return ml_integer(Real->Value);
}

ML_METHOD(MLIntegerOfMethod, MLRealT) {
//!number
//<Real
//>integer
// Converts :mini:`Real` to an integer (using default rounding).
	return ml_integer(((ml_real_t *)Args[0])->Value);
}

static long ml_real_hash(ml_real_t *Real, ml_hash_chain_t *Chain) {
	return (long)Real->Value;
}

ML_TYPE(MLRealT, (MLNumberT), "real",
//!number
	.hash = (void *)ml_real_hash
);

ml_value_t *ml_real(double Value) {
	ml_real_t *Real = new(ml_real_t);
	Real->Type = MLRealT;
	Real->Value = Value;
	return (ml_value_t *)Real;
}

double ml_real_value(const ml_value_t *Value) {
	if (Value->Type == MLIntegerT) {
		return ((ml_integer_t *)Value)->Value;
	} else if (Value->Type == MLRealT) {
		return ((ml_real_t *)Value)->Value;
	} else {
		return 0;
	}
}

#endif

ml_value_t *ml_real_of(ml_value_t *Value) {
	typeof(ml_integer_of) *function = ml_typed_fn_get(ml_typeof(Value), ml_string_of);
	if (!function) return ml_simple_inline(MLRealOfMethod, 1, Value);
	return function(Value);
}

#ifdef USE_NANBOXING

static ml_value_t *ML_TYPED_FN(ml_real_of, MLInt32T, ml_value_t *Value) {
	return ml_real((uint64_t)Value & 0xFFFFFFFF);
}

static ml_value_t *ML_TYPED_FN(ml_real_of, MLInt64T, ml_value_t *Value) {
	return ml_real(((ml_int64_t *)Value)->Value);
}

ML_METHOD(MLRealOfMethod, MLInt32T) {
//!number
	return ml_real((uint64_t)Args[0] & 0xFFFFFFFF);
}

ML_METHOD(MLRealOfMethod, MLInt64T) {
//!number
	return ml_real(((ml_int64_t *)Args[0])->Value);
}

#else

static ml_value_t *ML_TYPED_FN(ml_real_of, MLIntegerT, ml_integer_t *Integer) {
	return ml_real(Integer->Value);
}

ML_METHOD(MLRealOfMethod, MLIntegerT) {
//!number
	return ml_real(((ml_integer_t *)Args[0])->Value);
}

#endif

static ml_value_t *ML_TYPED_FN(ml_real_of, MLRealT, ml_value_t *Value) {
	return Value;
}

#define ml_arith_method_integer(NAME, SYMBOL) \
	ML_METHOD(NAME, MLIntegerT) { \
		int64_t IntegerA = ml_integer_value(Args[0]); \
		return ml_integer(SYMBOL(IntegerA)); \
	}

#define ml_arith_method_integer_integer(NAME, SYMBOL) \
	ML_METHOD(NAME, MLIntegerT, MLIntegerT) { \
		int64_t IntegerA = ml_integer_value(Args[0]); \
		int64_t IntegerB = ml_integer_value(Args[1]); \
		return ml_integer(IntegerA SYMBOL IntegerB); \
	}

#define ml_arith_method_real(NAME, SYMBOL) \
	ML_METHOD(NAME, MLRealT) { \
		double RealA = ml_real_value(Args[0]); \
		return ml_real(SYMBOL(RealA)); \
	}

#define ml_arith_method_real_real(NAME, SYMBOL) \
	ML_METHOD(NAME, MLRealT, MLRealT) { \
		double RealA = ml_real_value(Args[0]); \
		double RealB = ml_real_value(Args[1]); \
		return ml_real(RealA SYMBOL RealB); \
	}

#define ml_arith_method_real_integer(NAME, SYMBOL) \
	ML_METHOD(NAME, MLRealT, MLIntegerT) { \
		double RealA = ml_real_value(Args[0]); \
		int64_t IntegerB = ml_integer_value(Args[1]); \
		return ml_real(RealA SYMBOL IntegerB); \
	}

#define ml_arith_method_integer_real(NAME, SYMBOL) \
	ML_METHOD(NAME, MLIntegerT, MLRealT) { \
		int64_t IntegerA = ml_integer_value(Args[0]); \
		double RealB = ml_real_value(Args[1]); \
		return ml_real(IntegerA SYMBOL RealB); \
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
//!number
//<Int
//>integer
// Returns :mini:`Int + 1`
	return ml_integer(ml_integer_value(Args[0]) + 1);
}

ML_METHOD("--", MLIntegerT) {
//!number
//<Int
//>integer
// Returns :mini:`Int - 1`
	return ml_integer(ml_integer_value(Args[0]) - 1);
}

ML_METHOD("++", MLRealT) {
//!number
//<Real
//>real
// Returns :mini:`Real + 1`
	return ml_real(ml_real_value(Args[0]) + 1);
}

ML_METHOD("--", MLRealT) {
//!number
//<Real
//>real
// Returns :mini:`Real - 1`
	return ml_real(ml_real_value(Args[0]) - 1);
}

ml_arith_method_real_real("/", /)
ml_arith_method_real_integer("/", /)
ml_arith_method_integer_real("/", /)

ML_METHOD("/", MLIntegerT, MLIntegerT) {
//!number
//<Int/1
//<Int/2
//>integer | real
// Returns :mini:`Int/1 / Int/2` as an integer if the division is exact, otherwise as a real.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	if (IntegerA % IntegerB == 0) {
		return ml_integer(IntegerA / IntegerB);
	} else {
		return ml_real((double)IntegerA / (double)IntegerB);
	}
}

ML_METHOD("%", MLIntegerT, MLIntegerT) {
//!number
//<Int/1
//<Int/2
//>integer
// Returns the remainder of :mini:`Int/1` divided by :mini:`Int/2`.
// Note: the result is calculated by rounding towards 0. In particular, if :mini:`Int/1` is negative, the result will be negative.
// For a nonnegative remainder, use :mini:`Int/1 mod Int/2`.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	return ml_integer(IntegerA % IntegerB);
}

ML_METHOD("div", MLIntegerT, MLIntegerT) {
//!number
//<Int/1
//<Int/2
//>integer
// Returns the quotient of :mini:`Int/1` divided by :mini:`Int/2`.
// The result is calculated by rounding down in all cases.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	long A = IntegerA;
	long B = IntegerB;
	long Q = A / B;
	if (A < 0 && B * Q != A) {
		if (B < 0) ++Q; else --Q;
	}
	return ml_integer(Q);
}

ML_METHOD("mod", MLIntegerT, MLIntegerT) {
//!number
//<Int/1
//<Int/2
//>integer
// Returns the remainder of :mini:`Int/1` divided by :mini:`Int/2`.
// Note: the result is calculated by rounding down in all cases. In particular, the result is always nonnegative.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	long A = IntegerA;
	long B = IntegerB;
	long R = A % B;
	if (R < 0) R += labs(B);
	return ml_integer(R);
}

#define ml_comp_method_integer_integer(NAME, SYMBOL) \
	ML_METHOD(NAME, MLIntegerT, MLIntegerT) { \
		int64_t IntegerA = ml_integer_value(Args[0]); \
		int64_t IntegerB = ml_integer_value(Args[1]); \
		return IntegerA SYMBOL IntegerB ? Args[1] : MLNil; \
	}

#define ml_comp_method_real_real(NAME, SYMBOL) \
	ML_METHOD(NAME, MLRealT, MLRealT) { \
		double RealA = ml_real_value(Args[0]); \
		double RealB = ml_real_value(Args[1]); \
		return RealA SYMBOL RealB ? Args[1] : MLNil; \
	}

#define ml_comp_method_real_integer(NAME, SYMBOL) \
	ML_METHOD(NAME, MLRealT, MLIntegerT) { \
		double RealA = ml_real_value(Args[0]); \
		int64_t IntegerB = ml_integer_value(Args[1]); \
		return RealA SYMBOL IntegerB ? Args[1] : MLNil; \
	}

#define ml_comp_method_integer_real(NAME, SYMBOL) \
	ML_METHOD(NAME, MLIntegerT, MLRealT) { \
		int64_t IntegerA = ml_integer_value(Args[0]); \
		double RealB = ml_real_value(Args[1]); \
		return IntegerA SYMBOL RealB ? Args[1] : MLNil; \
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
//!number
//<Int/1
//<Int/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int/1` is less than, equal to or greater than :mini:`Int/2`.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	if (IntegerA < IntegerB) return (ml_value_t *)NegOne;
	if (IntegerA > IntegerB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLRealT, MLIntegerT) {
//!number
//<Real/1
//<Int/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real/1` is less than, equal to or greater than :mini:`Int/2`.
	double RealA = ml_real_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	if (RealA < IntegerB) return (ml_value_t *)NegOne;
	if (RealA > IntegerB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLIntegerT, MLRealT) {
//!number
//<Int/1
//<Real/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int/1` is less than, equal to or greater than :mini:`Real/2`.
	int64_t IntegerA = ml_integer_value(Args[0]);
	double RealB = ml_real_value(Args[1]);
	if (IntegerA < RealB) return (ml_value_t *)NegOne;
	if (IntegerA > RealB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLRealT, MLRealT) {
//!number
//<Real/1
//<Real/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real/1` is less than, equal to or greater than :mini:`Real/2`.
	double RealA = ml_real_value(Args[0]);
	double RealB = ml_real_value(Args[1]);
	if (RealA < RealB) return (ml_value_t *)NegOne;
	if (RealA > RealB) return (ml_value_t *)One;
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
//!range

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
//!range

ML_METHOD("..", MLIntegerT, MLIntegerT) {
//!range
//<Start
//<Limit
//>integerrange
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = IntegerA;
	Range->Limit = IntegerB;
	Range->Step = 1;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerT, MLIntegerT) {
//!range
//<Start
//<Step
//>integerrange
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = IntegerA;
	Range->Step = IntegerB;
	Range->Limit = Range->Step > 0 ? LONG_MAX : LONG_MIN;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerRangeT, MLIntegerT) {
//!range
//<Range
//<Step
//>integerrange
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = Range0->Start;
	Range->Limit = Range0->Limit;
	Range->Step = ml_integer_value(Args[1]);
	return (ml_value_t *)Range;
}

ML_METHOD("in", MLIntegerT, MLIntegerRangeT) {
//!range
//<X
//<Range
//>X | nil
	long Value = ml_integer_value(Args[0]);
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("in", MLRealT, MLIntegerRangeT) {
//!range
//<X
//<Range
//>X | nil
	double Value = ml_real_value(Args[0]);
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
//!range

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
//!range

ML_METHOD("..", MLNumberT, MLNumberT) {
//!range
//<Start
//<Limit
//>realrange
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = ml_real_value(Args[0]);
	Range->Limit = ml_real_value(Args[1]);
	Range->Step = 1.0;
	Range->Count = floor(Range->Limit - Range->Start);
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLNumberT, MLNumberT) {
//!range
//<Start
//<Step
//>realrange
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = ml_real_value(Args[0]);
	Range->Step = ml_real_value(Args[1]);
	Range->Limit = Range->Step > 0.0 ? INFINITY : -INFINITY;
	Range->Count = LONG_MAX;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLRealRangeT, MLNumberT) {
//!range
//<Range
//<Step
//>realrange
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
//!range
//<Range
//<Count
//>realrange
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
//!range
//<Range
//<Step
//>realrange
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
//!range
//<X
//<Range
//>X | nil
	long Value = ml_integer_value(Args[0]);
	ml_real_range_t *Range = (ml_real_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("in", MLRealT, MLRealRangeT) {
//!range
//<X
//<Range
//>X | nil
	double Value = ml_real_value(Args[0]);
	ml_real_range_t *Range = (ml_real_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

// Strings //

ML_FUNCTION(MLBuffer) {
//!buffer
//@buffer
//<Length
//>buffer
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

ML_TYPE(MLBufferT, (), "buffer",
//!buffer
	.Constructor = (ml_value_t *)MLBuffer
);

ML_METHOD("+", MLBufferT, MLIntegerT) {
//!buffer
//<Buffer
//<Offset
//>buffer
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
//!buffer
//<Buffer/1
//<Buffer/2
//>integer
	ml_buffer_t *Buffer1 = (ml_buffer_t *)Args[0];
	ml_buffer_t *Buffer2 = (ml_buffer_t *)Args[1];
	return ml_integer(Buffer1->Address - Buffer2->Address);
}

ML_METHOD(MLStringOfMethod, MLBufferT) {
//!buffer
	ml_buffer_t *Buffer = (ml_buffer_t *)Args[0];
	return ml_string_format("#%" PRIxPTR ":%ld", (uintptr_t)Buffer->Address, Buffer->Size);
}

#ifdef USE_NANBOXING

ML_TYPE(MLStringT, (MLIteratableT), "string");
//!string

static long ml_string_short_hash(ml_value_t *String, ml_hash_chain_t *Chain) {
	long Hash = 5381;
	int Length = ml_tag(String) - 2;
	char *Bytes = (char *)&String;
	for (int I = 0; I < Length; ++I) Hash = ((Hash << 5) + Hash) + Bytes[I];
	return Hash;
}

ML_TYPE(MLStringShortT, (MLStringT), "short-string",
//!string
	.hash = (void *)ml_string_short_hash
);

static long ml_string_long_hash(ml_buffer_t *String, ml_hash_chain_t *Chain) {
	long Hash = 5381;
	for (int I = 0; I < String->Size; ++I) Hash = ((Hash << 5) + Hash) + String->Address[I];
	return Hash;
}

ML_TYPE(MLStringLongT, (MLStringT, MLBufferT), "long-string",
//!string
	.hash = (void *)ml_string_long_hash
);

ml_value_t *ml_string(const char *Value, int Length) {
	if (Length < 0) Length = Value ? strlen(Value) : 0;
	if (Length <= 4) {
		ml_value_t *String = (void *)(((uint64_t)Length + 2) << 48);
		char *Bytes = (char *)&String;
		for (int I = 0; I < Length; ++I) Bytes[I] = Value[I];
		Bytes[Length] = 0;
		return String;
	} else {
		ml_buffer_t *String = new(ml_buffer_t);
		String->Type = MLStringLongT;
		if (Value[Length]) {
			char *Copy = snew(Length + 1);
			memcpy(Copy, Value, Length);
			Copy[Length] = 0;
			Value = Copy;
		}
		String->Address = (char *)Value;
		String->Size = Length;
		return (ml_value_t *)String;
	}
}

ML_METHOD("+", MLStringShortT, MLIntegerT) {
//!string
	const char *Chars  = (const char *)&Args[0];
	int Length = ml_tag(Args[0]) - 2;
	long Offset = ml_integer_value(Args[1]);
	if (Offset >= Length) return ml_error("ValueError", "Offset larger than buffer");
	Length -= Offset;
	Chars += Offset;
	ml_value_t *String = (void *)(((uint64_t)Length + 2) << 48);
	char *Bytes = (char *)&String;
	for (int I = 0; I < Length; ++I) Bytes[I] = Chars[I];
	Bytes[Length] = 0;
	return String;
}

size_t ml_string_length(const ml_value_t *Value) {
	int Tag = ml_tag(Value);
	if (Tag >= 2 && Tag <= 6) return Tag - 2;
	if (Tag == 0 && Value->Type == MLStringLongT) {
		return ((ml_buffer_t *)Value)->Size;
	}
	return 0;
}

#else

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
//!string
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

const char *ml_string_value(const ml_value_t *Value) {
	return ((ml_string_t *)Value)->Value;
}

size_t ml_string_length(const ml_value_t *Value) {
	return ((ml_string_t *)Value)->Length;
}

#endif

ml_value_t *ml_string_format(const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Value;
	int Length = vasprintf(&Value, Format, Args);
	va_end(Args);
	return ml_string(Value, Length);
}

ml_value_t *ml_string_of(ml_value_t *Value) {
	typeof(ml_string_of) *function = ml_typed_fn_get(ml_typeof(Value), ml_string_of);
	if (!function) return ml_simple_inline(MLStringOfMethod, 1, Value);
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
//!boolean
	ml_boolean_t *Boolean = (ml_boolean_t *)Args[0];
	return ml_string(Boolean->Name, -1);
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLIntegerT, ml_value_t *Integer) {
	char *Value;
	int Length = asprintf(&Value, "%ld", ml_integer_value(Integer));
	return ml_string(Value, Length);
}

ML_METHOD(MLStringOfMethod, MLIntegerT) {
//!number
	char *Value;
	int Length = asprintf(&Value, "%ld", ml_integer_value(Args[0]));
	return ml_string(Value, Length);
}

ML_METHOD(MLStringOfMethod, MLIntegerT, MLIntegerT) {
//!number
	int64_t Value = ml_integer_value(Args[0]);
	int Base = ml_integer_value(Args[1]);
	if (Base < 2 || Base > 36) return ml_error("RangeError", "Invalid base");
	int Max = 65;
	char *P = GC_MALLOC_ATOMIC(Max + 1) + Max, *Q = P;
	*P = '\0';
	int64_t Neg = Value < 0 ? Value : -Value;
	do {
		*--P = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[-(Neg % Base)];
		Neg /= Base;
	} while (Neg);
	if (Value < 0) *--P = '-';
	return ml_string(P, Q - P);
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLRealT, ml_value_t *Real) {
	char *Value;
	int Length = asprintf(&Value, "%f", ml_real_value(Real));
	return ml_string(Value, Length);
}

ML_METHOD(MLStringOfMethod, MLRealT) {
//!number
	char *Value;
	int Length = asprintf(&Value, "%f", ml_real_value(Args[0]));
	return ml_string(Value, Length);
}

ML_METHOD(MLIntegerOfMethod, MLStringT) {
//!number
	const char *Start = ml_string_value(Args[0]);
	char *End;
	long Value = strtol(ml_string_value(Args[0]), &End, 10);
	if (End - Start == ml_string_length(Args[0])) {
		return ml_integer(Value);
	} else {
		return ml_error("ValueError", "Error parsing integer");
	}
}

ML_METHOD(MLIntegerOfMethod, MLStringT, MLIntegerT) {
//!number
	const char *Start = ml_string_value(Args[0]);
	char *End;
	long Value = strtol(ml_string_value(Args[0]), &End, ml_integer_value(Args[1]));
	if (End - Start == ml_string_length(Args[0])) {
		return ml_integer(Value);
	} else {
		return ml_error("ValueError", "Error parsing integer");
	}
}

ML_METHOD(MLRealOfMethod, MLStringT) {
//!number
	const char *Start = ml_string_value(Args[0]);
	char *End;
	double Value = strtod(ml_string_value(Args[0]), &End);
	if (End - Start == ml_string_length(Args[0])) {
		return ml_real(Value);
	} else {
		return ml_error("ValueError", "Error parsing real");
	}
}

typedef struct {
	const ml_type_t *Type;
	const char *Value;
	int Index, Length;
} ml_string_iterator_t;

ML_TYPE(MLStringIteratorT, (), "string-iterator");
//!internal

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

static void ML_TYPED_FN(ml_iterate, MLStringT, ml_state_t *Caller, ml_value_t *String) {
	int Length = ml_string_length(String);
	if (!Length) ML_RETURN(MLNil);
	ml_string_iterator_t *Iter = new(ml_string_iterator_t);
	Iter->Type = MLStringIteratorT;
	Iter->Index = 1;
	Iter->Length = Length;
	Iter->Value = ml_string_value(String);
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

ML_FUNCTION(MLRegex) {
//!string
//@regex
//<String
//>regex | error
// Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Pattern = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	if (Pattern[Length]) return ml_error("ValueError", "Regex pattern must be proper string");
	return ml_regex(Pattern, Length);
}

ML_TYPE(MLRegexT, (), "regex",
//!string
	.hash = (void *)ml_regex_hash,
	.Constructor = (ml_value_t *)MLRegex
);

ml_value_t *ml_regex(const char *Pattern, int Length) {
	ml_regex_t *Regex = new(ml_regex_t);
	Regex->Type = MLRegexT;
	Regex->Pattern = Pattern;
#ifdef USE_TRE
	int Error = regncomp(Regex->Value, Pattern, Length, REG_EXTENDED);
#else
	int Error = regcomp(Regex->Value, Pattern, REG_EXTENDED);
#endif
	if (Error) {
		size_t ErrorSize = regerror(Error, Regex->Value, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(Error, Regex->Value, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	return (ml_value_t *)Regex;
}

regex_t *ml_regex_value(const ml_value_t *Value) {
	ml_regex_t *Regex = (ml_regex_t *)Value;
	return Regex->Value;
}

const char *ml_regex_pattern(const ml_value_t *Value) {
	ml_regex_t *Regex = (ml_regex_t *)Value;
	return Regex->Pattern;
}

ML_METHOD("<>", MLRegexT, MLRegexT) {
//!string
	const char *PatternA = ml_regex_pattern(Args[0]);
	const char *PatternB = ml_regex_pattern(Args[1]);
	int Compare = strcmp(PatternA, PatternB);
	if (Compare < 0) return (ml_value_t *)NegOne;
	if (Compare > 0) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

#define ml_comp_method_regex_regex(NAME, SYMBOL) \
	ML_METHOD(NAME, MLRegexT, MLRegexT) { \
		const char *PatternA = ml_regex_pattern(Args[0]); \
		const char *PatternB = ml_regex_pattern(Args[1]); \
		int Compare = strcmp(PatternA, PatternB); \
		return Compare SYMBOL 0 ? Args[1] : MLNil; \
	}

ml_comp_method_regex_regex("=", ==)
ml_comp_method_regex_regex("!=", !=)
ml_comp_method_regex_regex("<", <)
ml_comp_method_regex_regex(">", >)
ml_comp_method_regex_regex("<=", <=)
ml_comp_method_regex_regex(">=", >=)

ml_value_t *ml_stringbuffer() {
	ml_stringbuffer_t *Buffer = new(ml_stringbuffer_t);
	Buffer->Type = MLStringBufferT;
	return (ml_value_t *)Buffer;
}

ML_FUNCTION(MLStringBuffer) {
//!stringbuffer
//@stringbuffer
	return ml_stringbuffer();
}

ML_TYPE(MLStringBufferT, (), "stringbuffer",
//!stringbuffer
	.Constructor = (ml_value_t *)MLStringBuffer
);

struct ml_stringbuffer_node_t {
	ml_stringbuffer_node_t *Next;
	char Chars[ML_STRINGBUFFER_NODE_SIZE];
};

static GC_descr StringBufferDesc = 0;

ssize_t ml_stringbuffer_add(ml_stringbuffer_t *Buffer, const char *String, size_t Length) {
	size_t Remaining = Length;
	ml_stringbuffer_node_t *Node = Buffer->Tail ?: (ml_stringbuffer_node_t *)&Buffer->Head;
	while (Buffer->Space < Remaining) {
		memcpy(Node->Chars + ML_STRINGBUFFER_NODE_SIZE - Buffer->Space, String, Buffer->Space);
		String += Buffer->Space;
		Remaining -= Buffer->Space;
		ml_stringbuffer_node_t *Next = (ml_stringbuffer_node_t *)GC_MALLOC_EXPLICITLY_TYPED(sizeof(ml_stringbuffer_node_t), StringBufferDesc);
			//printf("Allocating stringbuffer: %d in total\n", ++NumStringBuffers);
		Node->Next = Next;
		Node = Next;
		Buffer->Space = ML_STRINGBUFFER_NODE_SIZE;
	}
	memcpy(Node->Chars + ML_STRINGBUFFER_NODE_SIZE - Buffer->Space, String, Remaining);
	Buffer->Space -= Remaining;
	Buffer->Length += Length;
	Buffer->Tail = Node;
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
	ml_stringbuffer_node_t *Node = Buffer->Head;
	while (Node->Next) {
		memcpy(P, Node->Chars, ML_STRINGBUFFER_NODE_SIZE);
		P += ML_STRINGBUFFER_NODE_SIZE;
		Node = Node->Next;
	}
	memcpy(P, Node->Chars, ML_STRINGBUFFER_NODE_SIZE - Buffer->Space);
	P += ML_STRINGBUFFER_NODE_SIZE - Buffer->Space;
	*P++ = 0;
	Buffer->Head = Buffer->Tail = NULL;
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

ml_value_t *ml_stringbuffer_value(ml_stringbuffer_t *Buffer) {
	size_t Length = Buffer->Length;
	if (Length == 0) {
		return ml_cstring("");
	} else {
		char *Chars = snew(Length + 1);
		ml_stringbuffer_finish(Buffer, Chars);
		return ml_string(Chars, Length);
	}
}

ML_METHOD("get", MLStringBufferT) {
//!stringbuffer
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	return ml_stringbuffer_value(Buffer);
}

int ml_stringbuffer_foreach(ml_stringbuffer_t *Buffer, void *Data, int (*callback)(void *, const char *, size_t)) {
	ml_stringbuffer_node_t *Node = Buffer->Head;
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
			ml_stringbuffer_addf(Buffer, "<%s@%ld>", ml_typeof(Value)->Name, Link->Index);
			return (ml_value_t *)Buffer;
		}
	}
	ml_hash_chain_t NewChain[1] = {{Chain, Value, Chain ? Chain->Index + 1 : 1}};
	Buffer->Chain = NewChain;
	typeof(ml_stringbuffer_append) *function = ml_typed_fn_get(ml_typeof(Value), ml_stringbuffer_append);
	ml_value_t *Result = function ? function(Buffer, Value) : ml_simple_inline(MLStringBufferAppendMethod, 2, Buffer, Value);
	Buffer->Chain = Chain;
	return Result;
}

ML_METHODV("write", MLStringBufferT, MLAnyT) {
//!stringbuffer
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_value_t *Final = MLNil;
	for (int I = 1; I < Count; ++I) {
		ml_value_t *Result = ml_stringbuffer_append(Buffer, Args[I]);
		if (ml_is_error(Result)) return Result;
		if (Result == MLSome) Final = MLSome;
	}
	return Final;
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLNilT, ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	ml_stringbuffer_add(Buffer, "nil", 3);
	return MLNil;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLNilT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_add(Buffer, "nil", 3);
	return MLNil;
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLSomeT, ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	ml_stringbuffer_add(Buffer, "some", 4);
	return MLNil;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLSomeT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_add(Buffer, "some", 4);
	return MLNil;
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLIntegerT, ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	ml_stringbuffer_addf(Buffer, "%ld", ml_integer_value(Value));
	return (ml_value_t *)Buffer;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLIntegerT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_addf(Buffer, "%ld", ml_integer_value(Args[1]));
	return (ml_value_t *)Buffer;
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLRealT, ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	ml_stringbuffer_addf(Buffer, "%f", ml_real_value(Value));
	return (ml_value_t *)Buffer;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLRealT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_addf(Buffer, "%f", ml_real_value(Args[1]));
	return (ml_value_t *)Buffer;
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLStringT, ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	int Length = ml_string_length(Value);
	if (Length) {
		ml_stringbuffer_add(Buffer, ml_string_value(Value), Length);
	}
	return (ml_value_t *)Buffer;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLStringT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_add(Buffer, ml_string_value(Args[1]), ml_string_length(Args[1]));
	return (ml_value_t *)Buffer;
}

ML_METHOD("[]", MLStringT, MLIntegerT) {
//!string
	const char *Chars = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	int Index = ml_integer_value(Args[1]);
	if (Index <= 0) Index += Length + 1;
	if (Index <= 0) return MLNil;
	if (Index > Length) return MLNil;
	return ml_string(Chars + (Index - 1), 1);
}

ML_METHOD("[]", MLStringT, MLIntegerT, MLIntegerT) {
//!string
	const char *Chars = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	int Lo = ml_integer_value(Args[1]);
	int Hi = ml_integer_value(Args[2]);
	if (Lo <= 0) Lo += Length + 1;
	if (Hi <= 0) Hi += Length + 1;
	if (Lo <= 0) return MLNil;
	if (Hi > Length + 1) return MLNil;
	if (Hi < Lo) return MLNil;
	int Length2 = Hi - Lo;
	return ml_string(Chars + Lo - 1, Length2);
}

ML_METHOD("+", MLStringT, MLStringT) {
//!string
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
//!string
	const unsigned char *Start = (const unsigned char *)ml_string_value(Args[0]);
	const unsigned char *End = Start + ml_string_length(Args[0]);
	while (Start < End && Start[0] <= ' ') ++Start;
	while (Start < End && End[-1] <= ' ') --End;
	int Length = End - Start;
	return ml_string((const char *)Start, Length);
}

ML_METHOD("trim", MLStringT, MLStringT) {
//!string
	char Trim[256] = {0,};
	const unsigned char *P = (const unsigned char *)ml_string_value(Args[1]);
	for (int Length = ml_string_length(Args[1]); --Length >= 0; ++P) Trim[*P] = 1;
	const unsigned char *Start = (const unsigned char *)ml_string_value(Args[0]);
	const unsigned char *End = Start + ml_string_length(Args[0]);
	while (Start < End && Trim[Start[0]]) ++Start;
	while (Start < End && Trim[End[-1]]) --End;
	int Length = End - Start;
	return ml_string((const char *)Start, Length);
}

ML_METHOD("ltrim", MLStringT) {
//!string
	const unsigned char *Start = (const unsigned char *)ml_string_value(Args[0]);
	const unsigned char *End = Start + ml_string_length(Args[0]);
	while (Start < End && Start[0] <= ' ') ++Start;
	int Length = End - Start;
	return ml_string((const char *)Start, Length);
}

ML_METHOD("ltrim", MLStringT, MLStringT) {
//!string
	char Trim[256] = {0,};
	const unsigned char *P = (const unsigned char *)ml_string_value(Args[1]);
	for (int Length = ml_string_length(Args[1]); --Length >= 0; ++P) Trim[*P] = 1;
	const unsigned char *Start = (const unsigned char *)ml_string_value(Args[0]);
	const unsigned char *End = Start + ml_string_length(Args[0]);
	while (Start < End && Trim[Start[0]]) ++Start;
	int Length = End - Start;
	return ml_string((const char *)Start, Length);
}

ML_METHOD("rtrim", MLStringT) {
//!string
	const unsigned char *Start = (const unsigned char *)ml_string_value(Args[0]);
	const unsigned char *End = Start + ml_string_length(Args[0]);
	while (Start < End && End[-1] <= ' ') --End;
	int Length = End - Start;
	return ml_string((const char *)Start, Length);
}

ML_METHOD("rtrim", MLStringT, MLStringT) {
//!string
	char Trim[256] = {0,};
	const unsigned char *P = (const unsigned char *)ml_string_value(Args[1]);
	for (int Length = ml_string_length(Args[1]); --Length >= 0; ++P) Trim[*P] = 1;
	const unsigned char *Start = (const unsigned char *)ml_string_value(Args[0]);
	const unsigned char *End = Start + ml_string_length(Args[0]);
	while (Start < End && Trim[End[-1]]) --End;
	int Length = End - Start;
	return ml_string((const char *)Start, Length);
}

ML_METHOD("length", MLStringT) {
//!string
	return ml_integer(ml_string_length(Args[0]));
}

ML_METHOD("<>", MLStringT, MLStringT) {
//!string
	const char *StringA = ml_string_value(Args[0]);
	const char *StringB = ml_string_value(Args[1]);
	int LengthA = ml_string_length(Args[0]);
	int LengthB = ml_string_length(Args[1]);
	if (LengthA < LengthB) {
		int Compare = memcmp(StringA, StringB, LengthA);
		if (Compare > 1) return (ml_value_t *)One;
		return (ml_value_t *)NegOne;
	} else if (LengthA > LengthB) {
		int Compare = memcmp(StringA, StringB, LengthB);
		if (Compare < 1) return (ml_value_t *)NegOne;
		return (ml_value_t *)One;
	} else {
		int Compare = memcmp(StringA, StringB, LengthA);
		if (Compare < 0) return (ml_value_t *)NegOne;
		if (Compare > 0) return (ml_value_t *)One;
		return (ml_value_t *)Zero;
	}
}

#define ml_comp_method_string_string(NAME, SYMBOL) \
	ML_METHOD(NAME, MLStringT, MLStringT) { \
		const char *StringA = ml_string_value(Args[0]); \
		const char *StringB = ml_string_value(Args[1]); \
		int LengthA = ml_string_length(Args[0]); \
		int LengthB = ml_string_length(Args[1]); \
		int Compare; \
		if (LengthA < LengthB) { \
			Compare = memcmp(StringA, StringB, LengthA) ?: -1; \
		} else if (LengthA > LengthB) { \
			Compare = memcmp(StringA, StringB, LengthB) ?: 1; \
		} else { \
			Compare = memcmp(StringA, StringB, LengthA); \
		} \
		return Compare SYMBOL 0 ? Args[1] : MLNil; \
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
//!string
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
//!string
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
//!string
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
//!string
	ml_value_t *Results = ml_list();
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	const char *SubjectEnd = Subject + SubjectLength;
	ml_regex_t *Pattern = (ml_regex_t *)Args[1];
	int Index = Pattern->Value->re_nsub ? 1 : 0;
	regmatch_t Matches[2];
	for (;;) {
#ifdef USE_TRE
		switch (regnexec(Pattern->Value, Subject, SubjectLength, Index + 1, Matches, 0)) {
#else
		switch (regexec(Pattern->Value, Subject, Index + 1, Matches, 0)) {
#endif
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
			SubjectLength -= Matches[Index].rm_eo;
		}
		}
	}
	return Results;
}

ML_METHOD("lower", MLStringT) {
//!string
	const char *Source = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	char *Target = snew(Length + 1);
	for (int I = 0; I < Length; ++I) Target[I] = tolower(Source[I]);
	return ml_string(Target, Length);
}

ML_METHOD("upper", MLStringT) {
//!string
	const char *Source = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	char *Target = snew(Length + 1);
	for (int I = 0; I < Length; ++I) Target[I] = toupper(Source[I]);
	return ml_string(Target, Length);
}

ML_METHOD("find", MLStringT, MLStringT) {
//!string
	const char *Haystack = ml_string_value(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	const char *Match = strstr(Haystack, Needle);
	if (Match) {
		return ml_integer(1 + Match - Haystack);
	} else {
		return MLNil;
	}
}

ML_METHOD("find2", MLStringT, MLStringT) {
//!string
	const char *Haystack = ml_string_value(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	const char *Match = strstr(Haystack, Needle);
	if (Match) {
		ml_value_t *Result = ml_tuple(2);
		ml_tuple_set(Result, 1, ml_integer(1 + Match - Haystack));
		ml_tuple_set(Result, 2, Args[1]);
		return Result;
	} else {
		return MLNil;
	}
}

ML_METHOD("find", MLStringT, MLStringT, MLIntegerT) {
//!string
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

ML_METHOD("find2", MLStringT, MLStringT, MLIntegerT) {
//!string
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
		ml_value_t *Result = ml_tuple(2);
		ml_tuple_set(Result, 1, ml_integer(1 + Match - Haystack));
		ml_tuple_set(Result, 2, Args[1]);
		return Result;
	} else {
		return MLNil;
	}
}

ML_METHOD("find", MLStringT, MLRegexT) {
//!string
	const char *Haystack = ml_string_value(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	regmatch_t Matches[1];
#ifdef USE_TRE
	int Length = ml_string_length(Args[0]);
	switch (regnexec(Regex, Haystack, Length, 1, Matches, 0)) {
#else
	switch (regexec(Regex, Haystack, 1, Matches, 0)) {
#endif
	case REG_NOMATCH:
		return MLNil;
	case REG_ESPACE: {
		size_t ErrorSize = regerror(REG_ESPACE, Regex, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(REG_ESPACE, Regex, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	}
	return ml_integer(1 + Matches->rm_so);
}

ML_METHOD("find2", MLStringT, MLRegexT) {
//!string
	const char *Haystack = ml_string_value(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	regmatch_t Matches[1];
#ifdef USE_TRE
	int Length = ml_string_length(Args[0]);
	switch (regnexec(Regex, Haystack, Length, 1, Matches, 0)) {
#else
	switch (regexec(Regex, Haystack, 1, Matches, 0)) {
#endif
	case REG_NOMATCH:
		return MLNil;
	case REG_ESPACE: {
		size_t ErrorSize = regerror(REG_ESPACE, Regex, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(REG_ESPACE, Regex, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	}
	ml_value_t *Result = ml_tuple(2);
	ml_tuple_set(Result, 1, ml_integer(1 + Matches->rm_so));
	ml_tuple_set(Result, 2, ml_string(Haystack + Matches->rm_so, Matches->rm_eo - Matches->rm_so));
	return Result;
}

ML_METHOD("find", MLStringT, MLRegexT, MLIntegerT) {
//!string
	const char *Haystack = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	int Start = ml_integer_value(Args[2]);
	if (Start <= 0) Start += Length + 1;
	if (Start <= 0) return MLNil;
	if (Start > Length) return MLNil;
	Haystack += Start - 1;
	Length -= (Start - 1);
	regmatch_t Matches[1];
#ifdef USE_TRE
	switch (regnexec(Regex, Haystack, Length, 1, Matches, 0)) {
#else
	switch (regexec(Regex, Haystack, 1, Matches, 0)) {
#endif
	case REG_NOMATCH:
		return MLNil;
	case REG_ESPACE: {
		size_t ErrorSize = regerror(REG_ESPACE, Regex, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(REG_ESPACE, Regex, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	}
	return ml_integer(Start + Matches->rm_so);
}

ML_METHOD("find2", MLStringT, MLRegexT, MLIntegerT) {
//!string
	const char *Haystack = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	int Start = ml_integer_value(Args[2]);
	if (Start <= 0) Start += Length + 1;
	if (Start <= 0) return MLNil;
	if (Start > Length) return MLNil;
	Haystack += Start - 1;
	Length -= (Start - 1);
	regmatch_t Matches[1];
#ifdef USE_TRE
	switch (regnexec(Regex, Haystack, Length, 1, Matches, 0)) {
#else
	switch (regexec(Regex, Haystack, 1, Matches, 0)) {
#endif
	case REG_NOMATCH:
		return MLNil;
	case REG_ESPACE: {
		size_t ErrorSize = regerror(REG_ESPACE, Regex, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(REG_ESPACE, Regex, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	}
	ml_value_t *Result = ml_tuple(2);
	ml_tuple_set(Result, 1, ml_integer(Start + Matches->rm_so));
	ml_tuple_set(Result, 2, ml_string(Haystack + Matches->rm_so, Matches->rm_eo - Matches->rm_so));
	return Result;
}

ML_METHOD("%", MLStringT, MLRegexT) {
//!string
	const char *Subject = ml_string_value(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	regmatch_t Matches[Regex->re_nsub + 1];
#ifdef USE_TRE
	int Length = ml_string_length(Args[0]);
	switch (regnexec(Regex, Subject, Length, Regex->re_nsub + 1, Matches, 0)) {

#else
	switch (regexec(Regex, Subject, Regex->re_nsub + 1, Matches, 0)) {
#endif
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
				ml_tuple_set(Results, I + 1, ml_string(Subject + Start, Length));
			} else {
				ml_tuple_set(Results, I + 1, MLNil);
			}
		}
		return Results;
	}
	}
}

int ml_regex_match(ml_value_t *Value, const char *Subject, int Length) {
	regex_t *Regex = ml_regex_value(Value);
#ifdef USE_TRE
	switch (regnexec(Regex, Subject, Length, 0, NULL, 0)) {
#else
	switch (regexec(Regex, Subject, 0, NULL, 0)) {
#endif
	case REG_NOMATCH: return 1;
	case REG_ESPACE: return -1;
	default: return 0;
	}
}

ML_METHOD("?", MLStringT, MLRegexT) {
//!string
	const char *Subject = ml_string_value(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	regmatch_t Matches[Regex->re_nsub + 1];
#ifdef USE_TRE
	int Length = ml_string_length(Args[0]);
	switch (regnexec(Regex, Subject, Length, Regex->re_nsub + 1, Matches, 0)) {

#else
	switch (regexec(Regex, Subject, Regex->re_nsub + 1, Matches, 0)) {
#endif
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
			return ml_string(Subject + Start, Length);
		} else {
			return MLNil;
		}
	}
	}
}

ML_METHOD("starts", MLStringT, MLStringT) {
//!string
	const char *Subject = ml_string_value(Args[0]);
	const char *Prefix = ml_string_value(Args[1]);
	int Length = ml_string_length(Args[1]);
	if (Length > ml_string_length(Args[0])) return MLNil;
	if (memcmp(Subject, Prefix, Length)) return MLNil;
	return Args[1];
}

ML_METHOD("starts", MLStringT, MLRegexT) {
//!string
	const char *Subject = ml_string_value(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	regmatch_t Matches[Regex->re_nsub + 1];
#ifdef USE_TRE
	int Length = ml_string_length(Args[0]);
	switch (regnexec(Regex, Subject, Length, Regex->re_nsub + 1, Matches, 0)) {

#else
	switch (regexec(Regex, Subject, Regex->re_nsub + 1, Matches, 0)) {
#endif
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
		if (Start == 0) {
			size_t Length = Matches[0].rm_eo - Start;
			return ml_string(Subject + Start, Length);
		} else {
			return MLNil;
		}
	}
	}
}

ML_METHOD("replace", MLStringT, MLStringT, MLStringT) {
//!string
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
	return ml_stringbuffer_value(Buffer);
}

ML_METHOD("replace", MLStringT, MLRegexT, MLStringT) {
//!string
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	const char *Replace = ml_string_value(Args[2]);
	int ReplaceLength = ml_string_length(Args[2]);
	regmatch_t Matches[1];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (;;) {
#ifdef USE_TRE
		switch (regnexec(Regex, Subject, SubjectLength, 1, Matches, 0)) {

#else
		switch (regexec(Regex, Subject, 1, Matches, 0)) {
#endif
		case REG_NOMATCH:
			if (SubjectLength) ml_stringbuffer_add(Buffer, Subject, SubjectLength);
			return ml_stringbuffer_value(Buffer);
		case REG_ESPACE: {
			size_t ErrorSize = regerror(REG_ESPACE, Regex, NULL, 0);
			char *ErrorMessage = snew(ErrorSize + 1);
			regerror(REG_ESPACE, Regex, ErrorMessage, ErrorSize);
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
//!string
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	ml_value_t *Replacer = Args[2];
	int NumSub = Regex->re_nsub + 1;
	regmatch_t Matches[NumSub];
	ml_value_t *SubArgs[NumSub];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (;;) {
#ifdef USE_TRE
		switch (regnexec(Regex, Subject, SubjectLength, NumSub, Matches, 0)) {

#else
		switch (regexec(Regex, Subject, NumSub, Matches, 0)) {
#endif
		case REG_NOMATCH:
			if (SubjectLength) ml_stringbuffer_add(Buffer, Subject, SubjectLength);
			return ml_stringbuffer_value(Buffer);
		case REG_ESPACE: {
			size_t ErrorSize = regerror(REG_ESPACE, Regex, NULL, 0);
			char *ErrorMessage = snew(ErrorSize + 1);
			regerror(REG_ESPACE, Regex, ErrorMessage, ErrorSize);
			return ml_error("RegexError", "regex error: %s", ErrorMessage);
		}
		default: {
			regoff_t Start = Matches[0].rm_so;
			if (Start > 0) ml_stringbuffer_add(Buffer, Subject, Start);
			for (int I = 0; I < NumSub; ++I) {
				SubArgs[I] = ml_string(Subject + Matches[I].rm_so, Matches[I].rm_eo - Matches[I].rm_so);
			}
			ml_value_t *Replace = ml_simple_call(Replacer, NumSub, SubArgs);
			if (ml_is_error(Replace)) return Replace;
			if (!ml_is(Replace, MLStringT)) return ml_error("TypeError", "expected string, not %s", ml_typeof(Replace)->Name);
			ml_stringbuffer_add(Buffer, ml_string_value(Replace), ml_string_length(Replace));
			Subject += Matches[0].rm_eo;
			SubjectLength -= Matches[0].rm_eo;
		}
		}
	}
	return 0;
}

typedef struct {
	union {
		const char *String;
		regex_t *Regex;
	} Pattern;
	union {
		const char *String;
		ml_value_t *Function;
	} Replacement;
	int PatternLength;
	int ReplacementLength;
} ml_replacement_t;

ML_METHOD("replace", MLStringT, MLMapT) {
//!string
	int NumPatterns = ml_map_size(Args[1]);
	ml_replacement_t Replacements[NumPatterns], *Last = Replacements + NumPatterns;
	int I = 0, MaxSub = 0;
	ML_MAP_FOREACH(Args[1], Iter) {
		if (ml_is(Iter->Key, MLStringT)) {
			Replacements[I].Pattern.String = ml_string_value(Iter->Key);
			Replacements[I].PatternLength = ml_string_length(Iter->Key);
		} else if (ml_is(Iter->Key, MLRegexT)) {
			regex_t *Regex = ml_regex_value(Iter->Key);
			Replacements[I].Pattern.Regex = Regex;
			Replacements[I].PatternLength = -1;
			if (MaxSub <= Regex->re_nsub) MaxSub = Regex->re_nsub + 1;
		} else {
			return ml_error("TypeError", "Unsupported pattern type: <%s>", ml_typeof(Iter->Key)->Name);
		}
		if (ml_is(Iter->Value, MLStringT)) {
			Replacements[I].Replacement.String = ml_string_value(Iter->Value);
			Replacements[I].ReplacementLength = ml_string_length(Iter->Value);
		} else if (ml_is(Iter->Value, MLFunctionT)) {
			Replacements[I].Replacement.Function = Iter->Value;
			Replacements[I].ReplacementLength = -1;
		} else {
			return ml_error("TypeError", "Unsupported replacement type: <%s>", ml_typeof(Iter->Value)->Name);
		}
		++I;
	}
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	regmatch_t Matches[MaxSub];
	ml_value_t *SubArgs[MaxSub];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (;;) {
		int MatchStart = SubjectLength, MatchEnd, SubCount;
		ml_replacement_t *Match = NULL;
		for (ml_replacement_t *Replacement = Replacements; Replacement < Last; ++Replacement) {
			if (Replacement->PatternLength < 0) {
				regex_t *Regex = Replacement->Pattern.Regex;
				int NumSub = Replacement->Pattern.Regex->re_nsub + 1;
#ifdef USE_TRE
				switch (regnexec(Regex, Subject, SubjectLength, NumSub, Matches, 0)) {

#else
				switch (regexec(Regex, Subject, NumSub, Matches, 0)) {
#endif
				case REG_NOMATCH:
					break;
				case REG_ESPACE: {
					size_t ErrorSize = regerror(REG_ESPACE, Replacement->Pattern.Regex, NULL, 0);
					char *ErrorMessage = snew(ErrorSize + 1);
					regerror(REG_ESPACE, Replacement->Pattern.Regex, ErrorMessage, ErrorSize);
					return ml_error("RegexError", "regex error: %s", ErrorMessage);
				}
				default: {
					if (Matches[0].rm_so < MatchStart) {
						MatchStart = Matches[0].rm_so;
						for (int I = 0; I < NumSub; ++I) {
							SubArgs[I] = ml_string(Subject + Matches[I].rm_so, Matches[I].rm_eo - Matches[I].rm_so);
						}
						SubCount = NumSub;
						MatchEnd = Matches[0].rm_eo;
						Match = Replacement;
					}
				}
				}
			} else {
				const char *Find = strstr(Subject, Replacement->Pattern.String);
				if (Find) {
					int Start = Find - Subject;
					if (Start < MatchStart) {
						MatchStart = Start;
						SubCount = 0;
						MatchEnd = Start + Replacement->PatternLength;
						Match = Replacement;
					}
				}
			}
		}
		if (!Match) break;
		if (MatchStart) ml_stringbuffer_add(Buffer, Subject, MatchStart);
		if (Match->ReplacementLength < 0) {
			ml_value_t *Replace = ml_simple_call(Match->Replacement.Function, SubCount, SubArgs);
			if (ml_is_error(Replace)) return Replace;
			if (!ml_is(Replace, MLStringT)) return ml_error("TypeError", "expected string, not %s", ml_typeof(Replace)->Name);
			ml_stringbuffer_add(Buffer, ml_string_value(Replace), ml_string_length(Replace));
		} else {
			ml_stringbuffer_add(Buffer, Match->Replacement.String, Match->ReplacementLength);
		}
		Subject += MatchEnd;
		SubjectLength -= MatchEnd;
	}
	if (SubjectLength) ml_stringbuffer_add(Buffer, Subject, SubjectLength);
	return ml_stringbuffer_value(Buffer);
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLRegexT, ml_value_t *Regex) {
	return ml_string_format("/%s/", ml_regex_pattern(Regex));
}

ML_METHOD(MLStringOfMethod, MLRegexT) {
//!string
	return ml_string_format("/%s/", ml_regex_pattern(Args[0]));
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLRegexT, ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	ml_stringbuffer_addf(Buffer, "/%s/", ml_regex_pattern(Value));
	return (ml_value_t *)Buffer;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLRegexT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_addf(Buffer, "/%s/", ml_regex_pattern(Args[1]));
	return (ml_value_t *)Buffer;
}

// Lists //

static ml_list_node_t *ml_list_index(ml_list_t *List, int Index) {
	int Length = List->Length;
	if (Index <= 0) Index += Length + 1;
	if (Index > Length) return NULL;
	if (Index == Length) return List->Tail;
	if (Index < 1) return NULL;
	if (Index == 1) return List->Head;
	int CachedIndex = List->CachedIndex;
	if (CachedIndex < 0) {
		CachedIndex = 0;
		List->CachedNode = List->Head;
	} else if (CachedIndex > Length) {

	}
	switch (Index - CachedIndex) {
	case -1: {
		List->CachedIndex = Index;
		return (List->CachedNode = List->CachedNode->Prev);
	}
	case 0: return List->CachedNode;
	case 1: {
		List->CachedIndex = Index;
		return (List->CachedNode = List->CachedNode->Next);
	}
	}
	List->CachedIndex = Index;
	ml_list_node_t *Node;
	if (2 * Index < CachedIndex) {
		Node = List->Head;
		int Steps = Index - 1;
		do Node = Node->Next; while (--Steps);
	} else if (Index < CachedIndex) {
		Node = List->CachedNode;
		int Steps = CachedIndex - Index;
		do Node = Node->Prev; while (--Steps);
	} else if (2 * Index < CachedIndex + Length) {
		Node = List->CachedNode;
		int Steps = Index - CachedIndex;
		do Node = Node->Next; while (--Steps);
	} else {
		Node = List->Tail;
		int Steps = Length - Index;
		do Node = Node->Prev; while (--Steps);
	}
	return (List->CachedNode = Node);
}

static void ml_list_call(ml_state_t *Caller, ml_list_t *List, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	//if (ml_is_error(Arg)) ML_RETURN(Arg);
	if (ml_typeof(Arg) != MLIntegerT) ML_RETURN(ml_error("TypeError", "List index must be an integer"));
	int Index = ml_integer_value(Args[0]);
	ml_list_node_t *Node = ml_list_index(List, Index);
	ML_RETURN(Node ? Node->Value : MLNil);
}

ML_TYPE(MLListT, (MLFunctionT, MLIteratableT), "list",
//!list
// A list of elements.
	.call = (void *)ml_list_call
);

static ml_value_t *ml_list_node_deref(ml_list_node_t *Node) {
	return Node->Value;
}

static ml_value_t *ml_list_node_assign(ml_list_node_t *Node, ml_value_t *Value) {
	return (Node->Value = Value);
}

ML_TYPE(MLListNodeT, (), "list-node",
//!list
// A node in a :mini:`list`.
// Dereferencing a :mini:`listnode` returns the corresponding value from the :mini:`list`.
// Assigning to a :mini:`listnode` updates the corresponding value in the :mini:`list`.
	.deref = (void *)ml_list_node_deref,
	.assign = (void *)ml_list_node_assign
);

ml_value_t *ml_list() {
	ml_list_t *List = new(ml_list_t);
	List->Type = MLListT;
	List->Head = List->Tail = NULL;
	List->Length = 0;
	return (ml_value_t *)List;
}

ML_METHOD(MLListOfMethod) {
	return ml_list();
}

ML_METHOD(MLListOfMethod, MLTupleT) {
	ml_value_t *List = ml_list();
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	for (int I = 0; I < Tuple->Size; ++I) ml_list_put(List, Tuple->Values[I]);
	return List;
}

ml_value_t *ml_list_from_array(ml_value_t **Values, int Length) {
	ml_value_t *List = ml_list();
	for (int I = 0; I < Length; ++I) ml_list_put(List, Values[I]);
	return List;
}

void ml_list_to_array(ml_value_t *List, ml_value_t **Values) {
	int I = 0;
	for (ml_list_node_t *Node = ((ml_list_t *)List)->Head; Node; Node = Node->Next, ++I) {
		Values[I] = Node->Value;
	}
}

void ml_list_grow(ml_value_t *List0, int Count) {
	ml_list_t *List = (ml_list_t *)List0;
	for (int I = 0; I < Count; ++I) ml_list_put(List0, MLNil);
	List->CachedIndex = 1;
	List->CachedNode = List->Head;
}

void ml_list_push(ml_value_t *List0, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = new(ml_list_node_t);
	Node->Type = MLListNodeT;
	Node->Value = Value;
	if ((Node->Next = List->Head)) {
		List->Head->Prev = Node;
	} else {
		List->Tail = Node;
	}
	List->CachedNode = List->Head = Node;
	List->CachedIndex = 1;
	++List->Length;
}

void ml_list_put(ml_value_t *List0, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = new(ml_list_node_t);
	Node->Type = MLListNodeT;
	Node->Value = Value;
	if ((Node->Prev = List->Tail)) {
		List->Tail->Next = Node;
	} else {
		List->Head = Node;
	}
	List->CachedNode = List->Tail = Node;
	List->CachedIndex = ++List->Length;
}

ml_value_t *ml_list_pop(ml_value_t *List0) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = List->Head;
	if (Node) {
		if ((List->Head = Node->Next)) {
			List->Head->Prev = NULL;
		} else {
			List->Tail = NULL;
		}
		List->CachedNode = List->Head;
		List->CachedIndex = 1;
		--List->Length;
		return Node->Value;
	} else {
		return MLNil;
	}
}

ml_value_t *ml_list_pull(ml_value_t *List0) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = List->Tail;
	if (Node) {
		if ((List->Tail = Node->Prev)) {
			List->Tail->Next = NULL;
		} else {
			List->Head = NULL;
		}
		List->CachedNode = List->Tail;
		List->CachedIndex = -List->Length;
		return Node->Value;
	} else {
		return MLNil;
	}
}

ml_value_t *ml_list_get(ml_value_t *List0, int Index) {
	ml_list_node_t *Node = ml_list_index((ml_list_t *)List0, Index);
	return Node ? Node->Value : NULL;
}

ml_value_t *ml_list_set(ml_value_t *List0, int Index, ml_value_t *Value) {
	ml_list_node_t *Node = ml_list_index((ml_list_t *)List0, Index);
	if (Node) {
		ml_value_t *Old = Node->Value;
		Node->Value = Value;
		return Old;
	} else {
		return NULL;
	}
}

int ml_list_foreach(ml_value_t *Value, void *Data, int (*callback)(ml_value_t *, void *)) {
	ML_LIST_FOREACH(Value, Node) if (callback(Node->Value, Data)) return 1;
	return 0;
}

ML_METHOD("size", MLListT) {
//!list
//<List
//>integer
// Returns the length of :mini:`List`
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_integer(List->Length);
}

ML_METHOD("length", MLListT) {
//!list
//<List
//>integer
// Returns the length of :mini:`List`
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_integer(List->Length);
}

ML_METHOD("filter", MLListT, MLFunctionT) {
//!list
//<List
//<Filter
//>list
// Removes every :mini:`Value` from :mini:`List` for which :mini:`Function(Value)` returns :mini:`nil` and returns those values in a new list.
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_t *Drop = new(ml_list_t);
	Drop->Type = MLListT;
	ml_value_t *Filter = Args[1];
	ml_list_node_t *Node = List->Head;
	ml_list_node_t **KeepSlot = &List->Head;
	ml_list_node_t *KeepTail = NULL;
	ml_list_node_t **DropSlot = &Drop->Head;
	ml_list_node_t *DropTail = NULL;
	List->Head = NULL;
	int Length = 0;
	while (Node) {
		ml_value_t *Result = ml_simple_inline(Filter, 1, Node->Value);
		if (ml_is_error(Result)) {
			List->Head = List->Tail = NULL;
			List->Length = 0;
			return Result;
		}
		if (Result == MLNil) {
			Node->Prev = DropSlot[0];
			DropSlot[0] = Node;
			DropSlot = &Node->Next;
			DropTail = Node;
		} else {
			Node->Prev = KeepSlot[0];
			KeepSlot[0] = Node;
			KeepSlot = &Node->Next;
			++Length;
			KeepTail = Node;
		}
		Node = Node->Next;
	}
	Drop->Tail = DropTail;
	if (DropTail) DropTail->Next = NULL;
	Drop->Length = List->Length - Length;
	Drop->CachedIndex = Drop->Length;
	Drop->CachedNode = DropTail;
	List->Tail = KeepTail;
	if (KeepTail) KeepTail->Next = NULL;
	List->Length = Length;
	List->CachedIndex = Length;
	List->CachedNode = KeepTail;
	return (ml_value_t *)Drop;
}

ML_METHOD("[]", MLListT, MLIntegerT) {
//!list
//<List
//<Index
//>listnode | nil
// Returns the :mini:`Index`-th node in :mini:`List` or :mini:`nil` if :mini:`Index` is outside the range of :mini:`List`.
// Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.
	ml_list_t *List = (ml_list_t *)Args[0];
	int Index = ml_integer_value(Args[1]);
	return (ml_value_t *)ml_list_index(List, Index) ?: MLNil;
}

typedef struct {
	const ml_type_t *Type;
	ml_list_node_t *Head;
	int Length;
} ml_list_slice_t;

static ml_value_t *ml_list_slice_deref(ml_list_slice_t *Slice) {
	ml_value_t *List = ml_list();
	ml_list_node_t *Node = Slice->Head;
	int Length = Slice->Length;
	while (Node && Length) {
		ml_list_put(List, Node->Value);
		Node = Node->Next;
		--Length;
	}
	return List;
}

static ml_value_t *ml_list_slice_assign(ml_list_slice_t *Slice, ml_value_t *Packed) {
	ml_list_node_t *Node = Slice->Head;
	int Length = Slice->Length;
	int Index = 0;
	while (Node && Length) {
		ml_value_t *Value = ml_unpack(Packed, Index);
		if (!Value) {
			return ml_error("ValueError", "Incorrect number of values to unpack (%d < %d)", Index, Slice->Length);
		}
		++Index;
		Node->Value = Value;
		Node = Node->Next;
		--Length;
	}
	return Packed;
}

ML_TYPE(MLListSliceT, (), "list-slice",
//!list
// A slice of a list.
	.deref = (void *)ml_list_slice_deref,
	.assign = (void *)ml_list_slice_assign
);

ML_METHOD("[]", MLListT, MLIntegerT, MLIntegerT) {
//!list
//<List
//<From
//<To
//>listslice
// Returns a slice of :mini:`List` starting at :mini:`From` (inclusive) and ending at :mini:`To` (exclusive).
// Indexing starts at :mini:`1`. Negative indices are counted from the end of the list, with :mini:`-1` returning the last node.
	ml_list_t *List = (ml_list_t *)Args[0];
	int Start = ml_integer_value(Args[1]);
	int End = ml_integer_value(Args[2]);
	if (Start <= 0) Start += List->Length + 1;
	if (End <= 0) End += List->Length + 1;
	if (Start <= 0 || End < Start || End > List->Length + 1) return MLNil;
	ml_list_slice_t *Slice = new(ml_list_slice_t);
	Slice->Type = MLListSliceT;
	Slice->Head = ml_list_index(List, Start);
	Slice->Length = End - Start;
	return (ml_value_t *)Slice;
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLListT, ml_stringbuffer_t *Buffer, ml_list_t *List) {
	ml_stringbuffer_add(Buffer, "[", 1);
	ml_list_node_t *Node = List->Head;
	if (Node) {
		ml_stringbuffer_append(Buffer, Node->Value);
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			ml_stringbuffer_append(Buffer, Node->Value);
		}
	}
	ml_stringbuffer_add(Buffer, "]", 1);
	return (ml_value_t *)Buffer;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLListT) {
//!list
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_add(Buffer, "[", 1);
	ml_list_t *List = (ml_list_t *)Args[1];
	ml_list_node_t *Node = List->Head;
	if (Node) {
		ml_stringbuffer_append(Buffer, Node->Value);
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			ml_stringbuffer_append(Buffer, Node->Value);
		}
	}
	ml_stringbuffer_add(Buffer, "]", 1);
	return (ml_value_t *)Buffer;
}

ml_value_t *ML_TYPED_FN(ml_unpack, MLListT, ml_list_t *List, int Index) {
	ml_list_node_t *Node = ml_list_index(List, Index + 1);
	return Node ? Node->Value : NULL;
}

typedef struct ml_list_iterator_t {
	const ml_type_t *Type;
	ml_list_node_t *Node;
	long Index;
} ml_list_iterator_t;

static void ML_TYPED_FN(ml_iter_value, MLListIterT, ml_state_t *Caller, ml_list_iterator_t *Iter) {
	ML_RETURN(Iter->Node);
}

static void ML_TYPED_FN(ml_iter_next, MLListIterT, ml_state_t *Caller, ml_list_iterator_t *Iter) {
	if ((Iter->Node = Iter->Node->Next)) {
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
//!internal

static void ML_TYPED_FN(ml_iterate, MLListT, ml_state_t *Caller, ml_list_t *List) {
	if (List->Length) {
		ml_list_iterator_t *Iter = new(ml_list_iterator_t);
		Iter->Type = MLListIterT;
		Iter->Node = List->Head;
		Iter->Index = 1;
		ML_RETURN(Iter);
	} else {
		ML_RETURN(MLNil);
	}
}

ML_METHODV("push", MLListT) {
//!list
//<List
//<Values...: any
//>list
// Pushes :mini:`Values` onto the start of :mini:`List` and returns :mini:`List`.
	ml_value_t *List = Args[0];
	for (int I = 1; I < Count; ++I) ml_list_push(List, Args[I]);
	return Args[0];
}

ML_METHODV("put", MLListT) {
//!list
//<List
//<Values...: MLAnyT
//>list
// Pushes :mini:`Values` onto the end of :mini:`List` and returns :mini:`List`.
	ml_value_t *List = Args[0];
	for (int I = 1; I < Count; ++I) ml_list_put(List, Args[I]);
	return Args[0];
}

ML_METHOD("pop", MLListT) {
//!list
//<List
//>any | nil
// Removes and returns the first element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.
	return ml_list_pop(Args[0]) ?: MLNil;
}

ML_METHOD("pull", MLListT) {
//!list
//<List
//>any | nil
// Removes and returns the last element of :mini:`List` or :mini:`nil` if the :mini:`List` is empty.
	return ml_list_pull(Args[0]) ?: MLNil;
}

ML_METHOD("copy", MLListT) {
//!list
//<List
//>list
// Returns a (shallow) copy of :mini:`List`.
	ml_value_t *List = ml_list();
	ML_LIST_FOREACH(Args[0], Iter) ml_list_put(List, Iter->Value);
	return List;
}

ML_METHOD("+", MLListT, MLListT) {
//!list
//<List/1
//<List/2
//>list
// Returns a new list with the elements of :mini:`List/1` followed by the elements of :mini:`List/2`.
	ml_value_t *List = ml_list();
	ML_LIST_FOREACH(Args[0], Iter) ml_list_put(List, Iter->Value);
	ML_LIST_FOREACH(Args[1], Iter) ml_list_put(List, Iter->Value);
	return List;
}

ML_METHOD(MLStringOfMethod, MLListT) {
//!list
//<List
//>string
// Returns a string containing the elements of :mini:`List` surrounded by :mini:`[`, :mini:`]` and seperated by :mini:`,`.
	ml_list_t *List = (ml_list_t *)Args[0];
	if (!List->Length) return ml_cstring("[]");
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Seperator = "[";
	int SeperatorLength = 1;
	ML_LIST_FOREACH(List, Node) {
		ml_stringbuffer_add(Buffer, Seperator, SeperatorLength);
		ml_value_t *Result = ml_stringbuffer_append(Buffer, Node->Value);
		if (ml_is_error(Result)) return Result;
		Seperator = ", ";
		SeperatorLength = 2;
	}
	ml_stringbuffer_add(Buffer, "]", 1);
	return ml_stringbuffer_value(Buffer);
}

ML_METHOD(MLStringOfMethod, MLListT, MLStringT) {
//!list
//<List
//<Seperator
//>string
// Returns a string containing the elements of :mini:`List` seperated by :mini:`Seperator`.
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Seperator = ml_string_value(Args[1]);
	size_t SeperatorLength = ml_string_length(Args[1]);
	ml_list_node_t *Node = List->Head;
	if (Node) {
		ml_value_t *Result = ml_stringbuffer_append(Buffer, Node->Value);
		if (ml_is_error(Result)) return Result;
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, Seperator, SeperatorLength);
			ml_value_t *Result = ml_stringbuffer_append(Buffer, Node->Value);
			if (ml_is_error(Result)) return Result;
		}
	}
	return ml_stringbuffer_value(Buffer);
}

static ml_value_t *ml_list_sort(ml_list_t *List, ml_value_t *Compare) {
	ml_list_node_t *Head = List->Head;
	int InSize = 1;
	for (;;) {
		ml_list_node_t *P = Head;
		ml_list_node_t *Tail = Head = 0;
		int NMerges = 0;
		while (P) {
			NMerges++;
			ml_list_node_t *Q = P;
			int PSize = 0;
			for (int I = 0; I < InSize; I++) {
				PSize++;
				Q = Q->Next;
				if (!Q) break;
			}
			int QSize = InSize;
			ml_list_node_t *E;
			while (PSize > 0 || (QSize > 0 && Q)) {
				if (PSize == 0) {
					E = Q; Q = Q->Next; QSize--;
				} else if (QSize == 0 || !Q) {
					E = P; P = P->Next; PSize--;
				} else {
					ml_value_t *Result = ml_simple_inline(Compare, 2, P->Value, Q->Value);
					if (ml_is_error(Result)) return Result;
					if (Result == MLNil) {
						E = Q; Q = Q->Next; QSize--;
					} else {
						E = P; P = P->Next; PSize--;
					}
				}
				if (Tail) {
					Tail->Next = E;
				} else {
					Head = E;
				}
				E->Prev = Tail;
				Tail = E;
			}
			P = Q;
		}
		Tail->Next = 0;
		if (NMerges <= 1) {
			List->Head = Head;
			List->Tail = Tail;
			List->CachedIndex = 1;
			List->CachedNode = Head;
			break;
		}
		InSize *= 2;
	}
	return (ml_value_t *)List;
}

static ML_METHOD_DECL(Less, "<");

ML_METHOD("sort", MLListT) {
//!list
//<List
//>List
	return ml_list_sort((ml_list_t *)Args[0], LessMethod);
}

ML_METHOD("sort", MLListT, MLFunctionT) {
//!list
//<List
//<Compare
//>List
	return ml_list_sort((ml_list_t *)Args[0], Args[1]);
}

ML_TYPE(MLNamesT, (MLListT), "names",
//!internal
	.call = (void *)ml_list_call
);

// Maps //

static void ml_map_call(ml_state_t *Caller, ml_value_t *Map, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	//if (Arg->Type == MLErrorT) ML_RETURN(Arg);
	ml_value_t *Value = ml_map_search(Map, Arg);
	if (Count > 1) return ml_call(Caller, Value, Count - 1, Args + 1);
	ML_RETURN(Value);
}

ML_TYPE(MLMapT, (MLFunctionT, MLIteratableT), "map",
//!map
// A map of key-value pairs.
// Keys can be of any type supporting hashing and comparison.
// Insert order is preserved.
	.call = (void *)ml_map_call
);

static ml_value_t *ml_map_node_deref(ml_map_node_t *Node) {
	return Node->Value;
}

static ml_value_t *ml_map_node_assign(ml_map_node_t *Node, ml_value_t *Value) {
	return (Node->Value = Value);
}

ML_TYPE(MLMapNodeT, (), "map-node",
//!map
// A node in a :mini:`map`.
// Dereferencing a :mini:`mapnode` returns the corresponding value from the :mini:`map`.
// Assigning to a :mini:`mapnode` updates the corresponding value in the :mini:`map`.
	.deref = (void *)ml_map_node_deref,
	.assign = (void *)ml_map_node_assign
);

ml_value_t *ml_map() {
	ml_map_t *Map = new(ml_map_t);
	Map->Type = MLMapT;
	return (ml_value_t *)Map;
}

ML_METHOD(MLMapOfMethod) {
	return ml_map();
}

static ml_map_node_t *ml_map_find_node(ml_map_t *Map, ml_value_t *Key) {
	ml_map_node_t *Node = Map->Root;
	long Hash = ml_typeof(Key)->hash(Key, NULL);
	while (Node) {
		int Compare;
		if (Hash < Node->Hash) {
			Compare = -1;
		} else if (Hash > Node->Hash) {
			Compare = 1;
		} else {
			ml_value_t *Args[2] = {Key, Node->Key};
			ml_value_t *Result = ml_simple_call(CompareMethod, 2, Args);
			if (ml_is_error(Result)) return NULL;
			Compare = ml_integer_value(Result);
		}
		if (!Compare) {
			return Node;
		} else {
			Node = Compare < 0 ? Node->Left : Node->Right;
		}
	}
	return NULL;
}

ml_value_t *ml_map_search(ml_value_t *Map0, ml_value_t *Key) {
	ml_map_node_t *Node = ml_map_find_node((ml_map_t *)Map0, Key);
	return Node ? Node->Value : MLNil;
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
		Node->Type = MLMapNodeT;
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
		ml_value_t *Result = ml_simple_call(CompareMethod, 2, Args);
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
	ml_map_node_t *Node = ml_map_node(Map, &Map->Root, ml_typeof(Key)->hash(Key, NULL), Key);
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
		ml_value_t *Result = ml_simple_call(CompareMethod, 2, Args);
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
	return ml_map_remove_internal(Map, &Map->Root, ml_typeof(Key)->hash(Key, NULL), Key);
}

int ml_map_foreach(ml_value_t *Value, void *Data, int (*callback)(ml_value_t *, ml_value_t *, void *)) {
	ml_map_t *Map = (ml_map_t *)Value;
	for (ml_map_node_t *Node = Map->Head; Node; Node = Node->Next) {
		if (callback(Node->Key, Node->Value, Data)) return 1;
	}
	return 0;
}

ML_METHOD("size", MLMapT) {
//!map
//<Map
//>integer
// Returns the number of entries in :mini:`Map`.
	ml_map_t *Map = (ml_map_t *)Args[0];
	return ml_integer(Map->Size);
}

static ml_value_t *ml_map_index_deref(ml_map_node_t *Index) {
	return MLNil;
}


static ml_map_node_t *ml_map_insert_node(ml_map_t *Map, ml_map_node_t **Slot, long Hash, ml_map_node_t *Index) {
	if (!Slot[0]) {
		++Map->Size;
		ml_map_node_t *Node = Slot[0] = Index;
		Node->Type = MLMapNodeT;
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
		return Node;
	}
	int Compare;
	if (Hash < Slot[0]->Hash) {
		Compare = -1;
	} else if (Hash > Slot[0]->Hash) {
		Compare = 1;
	} else {
		ml_value_t *Args[2] = {Index->Key, Slot[0]->Key};
		ml_value_t *Result = ml_simple_call(CompareMethod, 2, Args);
		Compare = ml_integer_value(Result);
	}
	if (!Compare) {
		return Slot[0];
	} else {
		ml_map_node_t *Node = ml_map_insert_node(Map, Compare < 0 ? &Slot[0]->Left : &Slot[0]->Right, Hash, Index);
		ml_map_rebalance(Slot);
		ml_map_update_depth(Slot[0]);
		return Node;
	}
}

static ml_value_t *ml_map_index_assign(ml_map_node_t *Index, ml_value_t *Value) {
	ml_map_t *Map = (ml_map_t *)Index->Value;
	ml_map_node_t *Node = ml_map_insert_node(Map, &Map->Root, ml_typeof(Index->Key)->hash(Index->Key, NULL), Index);
	ml_value_t *Old = Node->Value ?: MLNil;
	Node->Value = Value;
	return Old;
}

ML_TYPE(MLMapIndexT, (), "map-index",
//!internal
	.deref = (void *)ml_map_index_deref,
	.assign = (void *)ml_map_index_assign
);

ML_METHOD("[]", MLMapT, MLAnyT) {
//!map
//<Map
//<Key
//>mapnode
// Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then the reference withh return :mini:`nil` when dereferenced and will insert :mini:`Key` into :mini:`Map` when assigned.
	ml_map_node_t *Node = ml_map_find_node((ml_map_t *)Args[0], Args[1]);
	if (!Node) {
		Node = new(ml_map_node_t);
		Node->Type = MLMapIndexT;
		Node->Value = Args[0];
		Node->Key = Args[1];
	}
	return (ml_value_t *)Node;
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Key;
	ml_map_node_t *Node;
} ml_ref_state_t;

static void ml_node_state_run(ml_ref_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) {
		ML_CONTINUE(State->Base.Caller, Value);
	} else {
		State->Node->Value = Value;
		ML_CONTINUE(State->Base.Caller, State->Node);
	}
}

ML_METHODX("[]", MLMapT, MLAnyT, MLFunctionT) {
//!map
//<Map
//<Key
//<Default
//>mapnode
// Returns the node corresponding to :mini:`Key` in :mini:`Map`. If :mini:`Key` is not in :mini:`Map` then :mini:`Default(Key)` is called and the result inserted into :mini:`Map`.
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_map_node_t *Node = ml_map_node(Map, &Map->Root, ml_typeof(Key)->hash(Key, NULL), Key);
	if (!Node->Value) {
		Node->Value = MLNil;
		ml_ref_state_t *State = new(ml_ref_state_t);
		State->Base.Caller = Caller;
		State->Base.Context = Caller->Context;
		State->Base.run = (void *)ml_node_state_run;
		State->Key = Key;
		State->Node = Node;
		ml_value_t *Function = Args[2];
		return ml_call(State, Function, 1, &State->Key);
	} else {
		ML_RETURN(Node);
	}
}

ML_METHOD("::", MLMapT, MLStringT) {
//!map
//<Map
//<Key
//>mapnode
// Same as :mini:`Map[Key]`. This method allows maps to be used as modules.
	ml_map_node_t *Node = ml_map_find_node((ml_map_t *)Args[0], Args[1]);
	if (!Node) {
		Node = new(ml_map_node_t);
		Node->Type = MLMapIndexT;
		Node->Value = Args[0];
		Node->Key = Args[1];
	}
	return (ml_value_t *)Node;
}

ML_METHOD("insert", MLMapT, MLAnyT, MLAnyT) {
//!map
//<Map
//<Key
//<Value
//>any | nil
// Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.
// Returns the previous value associated with :mini:`Key` if any, otherwise :mini:`nil`.
	ml_value_t *Map = (ml_value_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_value_t *Value = Args[2];
	return ml_map_insert(Map, Key, Value);
}

ML_METHOD("delete", MLMapT, MLAnyT) {
//!map
//<Map
//<Key
//>any | nil
// Removes :mini:`Key` from :mini:`Map` and returns the corresponding value if any, otherwise :mini:`nil`.
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

ML_METHOD("missing", MLMapT, MLAnyT) {
//!map
//<Map
//<Key
//>any | nil
// Inserts :mini:`Key` into :mini:`Map` with corresponding value :mini:`Value`.
// Returns the previous value associated with :mini:`Key` if any, otherwise :mini:`nil`.
	ml_map_t *Map = (ml_map_t *)Args[0];
	ml_value_t *Key = Args[1];
	ml_map_node_t *Node = ml_map_node(Map, &Map->Root, ml_typeof(Key)->hash(Key, NULL), Key);
	if (!Node->Value) return Node->Value = MLSome;
	return MLNil;
}

ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLMapT, ml_stringbuffer_t *Buffer, ml_map_t *Map) {
	ml_stringbuffer_add(Buffer, "{", 1);
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
	}
	ml_stringbuffer_add(Buffer, "}", 1);
	return (ml_value_t *)Buffer;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLMapT) {
//!map
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_add(Buffer, "{", 1);
	ml_map_t *Map = (ml_map_t *)Args[1];
	ml_map_node_t *Node = Map->Head;
	if (Node) {
		ml_stringbuffer_append(Buffer, Node->Key);
		ml_stringbuffer_add(Buffer, " is ", 4);
		ml_stringbuffer_append(Buffer, Node->Value);
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			ml_stringbuffer_append(Buffer, Node->Key);
			ml_stringbuffer_add(Buffer, " is ", 4);
			ml_stringbuffer_append(Buffer, Node->Value);
		}
	}
	ml_stringbuffer_add(Buffer, "}", 1);
	return (ml_value_t *)Buffer;
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
//!internal

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
//!map
//<Map/1
//<Map/2
//>map
// Returns a new map combining the entries of :mini:`Map/1` and :mini:`Map/2`.
// If the same key is in both :mini:`Map/1` and :mini:`Map/2` then the corresponding value from :mini:`Map/2` is chosen.
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
	if (ml_is_error(Stringer->Error)) return 1;
	ml_stringbuffer_add(Stringer->Buffer, Stringer->Equals, Stringer->EqualsLength);
	Stringer->Error = ml_stringbuffer_append(Stringer->Buffer, Value);
	if (ml_is_error(Stringer->Error)) return 1;
	return 0;
}

ML_METHOD(MLStringOfMethod, MLMapT) {
//!map
//<Map
//>string
// Returns a string containing the entries of :mini:`Map` surrounded by :mini:`{`, :mini:`}` with :mini:`is` between keys and values and :mini:`,` between entries.
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

ML_METHOD(MLStringOfMethod, MLMapT, MLStringT, MLStringT) {
//!map
//<Map
//<Seperator
//<Connector
//>string
// Returns a string containing the entries of :mini:`Map` with :mini:`Connector` between keys and values and :mini:`Seperator` between entries.
	ml_map_stringer_t Stringer[1] = {{
		ml_string_value(Args[1]), ml_string_value(Args[2]),
		{ML_STRINGBUFFER_INIT},
		ml_string_length(Args[1]), ml_string_length(Args[2]),
		1
	}};
	if (ml_map_foreach(Args[0], Stringer, (void *)ml_map_stringer)) return Stringer->Error;
	return ml_stringbuffer_value(Stringer->Buffer);
}

// Methods //

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
	Context->Values[ML_METHODS_INDEX] = Methods;
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

static __attribute__ ((pure)) unsigned int ml_method_definition_score(ml_method_definition_t *Definition, int Count, const ml_type_t **Types, unsigned int Best) {
	unsigned int Score = 1;
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
		const ml_type_t *Type = Types[I] = ml_typeof(Args[I]);
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

const char *ml_method_name(const ml_value_t *Value) {
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
		Args[I] = ml_deref(Args[I]);
		//if (ml_is_error(Args[I])) ML_RETURN(Args[I]);
	}
	ml_method_t *Method = (ml_method_t *)Value;
	ml_methods_t *Methods = Caller->Context->Values[ML_METHODS_INDEX];
	ml_value_t *Callback = ml_method_search(Methods, Method, Count, Args);

	if (Callback) {
		return ml_call(Caller, Callback, Count, Args);
	} else {
		int Length = 4;
		for (int I = 0; I < Count; ++I) Length += strlen(ml_typeof(Args[I])->Name) + 2;
		char *Types = snew(Length);
		Types[0] = 0;
		char *P = Types;
#ifdef __MINGW32__
		for (int I = 0; I < Count; ++I) {
			strcpy(P, Args[I]->Type->Path);
			P += strlen(Args[I]->Type->Path);
			strcpy(P, ", ");
			P += 2;
		}
#else
		for (int I = 0; I < Count; ++I) P = stpcpy(stpcpy(P, ml_typeof(Args[I])->Name), ", ");
#endif
		P[-2] = 0;
		ML_RETURN(ml_error("MethodError", "no matching method found for %s(%s)", Method->Name, Types));
	}
}

ML_TYPE(MLMethodT, (MLFunctionT), "method",
//!method
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
//!method
//>method
	return ml_method(NULL);
}

ML_METHOD(MLMethodOfMethod, MLStringT) {
//!method
//<Name
//>method
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
//!method
//>string
	ml_method_t *Method = (ml_method_t *)Args[0];
	return ml_string_format(":%s", Method->Name);
}

static ml_value_t *ML_TYPED_FN(ml_stringbuffer_append, MLMethodT, ml_stringbuffer_t *Buffer, ml_method_t *Value) {
	ml_stringbuffer_add(Buffer, ":", 1);
	ml_stringbuffer_add(Buffer, Value->Name, strlen(Value->Name));
	return (ml_value_t *)Buffer;
}

ML_METHOD(MLStringBufferAppendMethod, MLStringBufferT, MLMethodT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_method_t *Method = (ml_method_t *)Args[1];
	ml_stringbuffer_add(Buffer, ":", 1);
	ml_stringbuffer_add(Buffer, Method->Name, strlen(Method->Name));
	return (ml_value_t *)Buffer;
}

ML_METHOD_DECL(MLRange, "..");

ML_FUNCTIONX(MLMethodSet) {
//!method
//@method::set
//<Method
//<Types...:type
//<Function:function
//>Function
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

// Modules //

typedef struct ml_module_t ml_module_t;

struct ml_module_t {
	const ml_type_t *Type;
	const char *Path;
	stringmap_t Exports[1];
};

ML_TYPE(MLModuleT, (), "module");
//!module

ML_METHODX("::", MLModuleT, MLStringT) {
//!module
//<Module
//<Name
//>MLAnyT
// Imports a symbol from a module.
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
//!module
	ml_module_t *Module = (ml_module_t *)Args[0];
	return ml_string_format("module(%s)", Module->Path);
}

static ml_value_t *ML_TYPED_FN(ml_string_of, MLModuleT, ml_module_t *Module) {
	return ml_string_format("module(%s)", Module->Path);
}

// Init //

void ml_init() {
#ifdef USE_ML_JIT
	GC_set_pages_executable(1);
#endif
	GC_INIT();
	GC_word StringBufferLayout[] = {1};
	StringBufferDesc = GC_make_descriptor(StringBufferLayout, 1);
#include "ml_types_init.c"
	MLBooleanT->Constructor = MLBooleanOfMethod;
	MLNumberT->Constructor = MLNumberOfMethod;
	MLIntegerT->Constructor = MLIntegerOfMethod;
	MLRealT->Constructor = MLRealOfMethod;
	MLStringT->Constructor = MLStringOfMethod;
	stringmap_insert(MLStringBufferT->Exports, "append", MLStringBufferAppendMethod);
	MLMethodT->Constructor = MLMethodOfMethod;
	stringmap_insert(MLMethodT->Exports, "set", MLMethodSet);
	MLListT->Constructor = MLListOfMethod;
	MLMapT->Constructor = MLMapOfMethod;
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
	ml_compiler_init();
	ml_runtime_init();
	ml_bytecode_init();
}

void ml_types_init(stringmap_t *Globals) {
	stringmap_insert(Globals, "any", MLAnyT);
	stringmap_insert(Globals, "type", MLTypeT);
	stringmap_insert(Globals, "function", MLFunctionT);
	stringmap_insert(Globals, "iteratable", MLIteratableT);
	stringmap_insert(Globals, "boolean", MLBooleanT);
	stringmap_insert(Globals, "true", MLTrue);
	stringmap_insert(Globals, "false", MLFalse);
	stringmap_insert(Globals, "number", MLNumberT);
	stringmap_insert(Globals, "integer", MLIntegerT);
	stringmap_insert(Globals, "real", MLRealT);
	stringmap_insert(Globals, "buffer", MLBufferT);
	stringmap_insert(Globals, "string", MLStringT);
	stringmap_insert(Globals, "stringbuffer", MLStringBufferT);
	stringmap_insert(Globals, "regex", MLRegexT);
	stringmap_insert(Globals, "method", MLMethodT);
	stringmap_insert(Globals, "list", MLListT);
	stringmap_insert(Globals, "names", MLNamesT);
	stringmap_insert(Globals, "map", MLMapT);
	stringmap_insert(Globals, "tuple", MLTupleT);
}

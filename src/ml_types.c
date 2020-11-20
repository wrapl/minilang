#include "minilang.h"
#include "ml_macros.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <gc/gc.h>
#include <limits.h>
#include <math.h>
#include <inttypes.h>
#include "ml_runtime.h"
#include "ml_bytecode.h"
#include "ml_string.h"
#include "ml_list.h"
#include "ml_map.h"

#ifdef USE_ML_THREADSAFE
#include <pthread.h>
#endif

ML_METHOD_DECL(Iterate, "iterate");
ML_METHOD_DECL(Value, "value");
ML_METHOD_DECL(Key, "key");
ML_METHOD_DECL(Next, "next");
ML_METHOD_DECL(Compare, "<>");
ML_METHOD_DECL(Index, "[]");
ML_METHOD_DECL(Symbol, "::");
ML_METHOD_DECL(Range, "..");
ML_METHOD_DECL(Less, "<");

ML_METHOD_DECL(MLBooleanOf, "boolean::of");
ML_METHOD_DECL(MLNumberOf, "number::of");
ML_METHOD_DECL(MLIntegerOf, "integer::of");
ML_METHOD_DECL(MLRealOf, "real::of");
ML_METHOD_DECL(MLMethodOf, "method::of");

static uintptr_t rotl(uintptr_t X, unsigned int N) {
	const unsigned int Mask = (CHAR_BIT * sizeof(uintptr_t) - 1);
	return (X << (N & Mask)) | (X >> ((-N) & Mask ));
}

// Types //

ML_INTERFACE(MLAnyT, (), "any", .Rank = 0);
// Base type for all values.

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

static long ml_type_hash(ml_type_t *Type) {
	return (intptr_t)Type;
}

static void ml_type_call(ml_state_t *Caller, ml_type_t *Type, int Count, ml_value_t **Args) {
	ml_value_t *Constructor = Type->Constructor;
	if (!Constructor) {
		Constructor = Type->Constructor = stringmap_search(Type->Exports, "of");
	}
	if (!Constructor) ML_RETURN(ml_error("TypeError", "No constructor for <%s>", Type->Name));
	return ml_call(Caller, Constructor, Count, Args);
}

ML_INTERFACE(MLTypeT, (MLFunctionT), "type",
//!type
// Type of all types.
// Every type contains a set of named exports, which allows them to be used as modules.
	.hash = (void *)ml_type_hash,
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

ML_METHOD("parents", MLTypeT) {
//!type
//<Type
//>list
	ml_type_t *Type = (ml_type_t *)Args[0];
	ml_value_t *Parents = ml_list();
	for (const ml_type_t **Parent = Type->Types; Parent[0]; ++Parent) {
		ml_list_put(Parents, (ml_value_t *)Parent[0]);
	}
	return Parents;
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

static void ml_types_sort(const ml_type_t **Types, int Lo, int Hi) {
	if (Lo >= Hi) return;
	const ml_type_t *Pivot = Types[(Hi + Lo) / 2];
	int Rank = Pivot->Rank;
	int I = Lo - 1;
	int J = Hi + 1;
	for (;;) {
		do ++I; while (Types[I]->Rank > Rank || (Types[I]->Rank == Rank && Types[I] > Pivot));
		do --J; while (Types[J]->Rank < Rank || (Types[J]->Rank == Rank && Types[J] < Pivot));
		if (I >= J) break;
		const ml_type_t *Temp = Types[I];
		Types[I] = Types[J];
		Types[J] = Temp;
	}
	if (J + 1 >= Hi) return ml_types_sort(Types, Lo, J);
	if (Lo < J) ml_types_sort(Types, Lo, J);
	return ml_types_sort(Types, J + 1, Hi);
}

void ml_type_init(ml_type_t *Type, ...) {
	int NumParents = 0;
	va_list Args;
	va_start(Args, Type);
	ml_type_t *Parent;
	while ((Parent = va_arg(Args, ml_type_t *))) {
		const ml_type_t **Types = Parent->Types;
		if (!Types) {
			fprintf(stderr, "Types initialized in wrong order %s < %s\n", Type->Name, Parent->Name);
			exit(1);
		}
		do ++NumParents; while (*++Types != MLAnyT);
	}
	va_end(Args);
	const ml_type_t **Types = Type->Types = anew(const ml_type_t *, NumParents + 3);
	const ml_type_t **Last = Types;
	*Last++ = Type;
	va_start(Args, Type);
	while ((Parent = va_arg(Args, ml_type_t *))) {
		const ml_type_t **Types = Parent->Types;
		while (*Types != MLAnyT) *Last++ = *Types++;
	}
	va_end(Args);
	int NumTypes = Last - Types;
	*Last++ = MLAnyT;
	ml_types_sort(Types, 1, NumTypes);
	Last = Types + 1;
	for (const ml_type_t **Next = Types + 1; Next[0]; ++Next) {
		if (Next[0] != Next[-1]) {
			inthash_insert(Type->Parents, (uintptr_t)Next[0], (void *)Next[0]);
			*Last++ = Next[0];
		}
	}
	Last[0] = NULL;
	if (Types[1]) Type->Rank += Types[1]->Rank + 1;
	if (Type->Constructor) stringmap_insert(Type->Exports, "of", Type->Constructor);
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
	inthash_result_t Result = inthash_search2(Type->TypedFns, (uintptr_t)TypedFn);
	if (!Result.Present) {
		for (const ml_type_t **Parents = Type->Types; Parents[0]; ++Parents) {
			Result = inthash_search2(Parents[0]->TypedFns, (uintptr_t)TypedFn);
			if (Result.Present) break;
		}
		inthash_insert(((ml_type_t *)Type)->TypedFns, (uintptr_t)TypedFn, Result.Value);
	}
	return Result.Value;
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

ML_METHOD("write", MLStringBufferT, MLTypeT) {
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
	ml_value_t *Value = stringmap_search(Type->Exports, Name);
	return Value ?: ml_error("ModuleError", "Symbol %s not exported from type %s", Name, Type->Name);
}

#ifdef USE_GENERICS
typedef struct ml_generic_t ml_generic_t;

struct ml_generic_t {
	ml_generic_t *Next;
	const ml_type_t *Type;
	const ml_type_t *Args[];
};

const ml_type_t *ml_type_generic(const ml_type_t *Base, int Count, const ml_type_t **Args) {
	inthash_t Cache[1] = {INTHASH_INIT};
	uintptr_t Hash = (uintptr_t)Base;
	for (int I = Count; --I >= 0;) Hash = rotl(Hash, 1) ^ (uintptr_t)Args[I];
	for (ml_generic_t *Generic = inthash_search(Cache, Hash); Generic; Generic = Generic->Next) {
		if (Generic->Args[0] != Base) continue;
		for (int I = 0; I < Count; ++I) {
			if (Args[I] != Generic->Args[I + 1]) goto next;
		}
		return Generic->Type;
		next: continue;
	}
	ml_type_t *Type = new(ml_type_t);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_add(Buffer, Base->Name, strlen(Base->Name));
	ml_stringbuffer_add(Buffer, "[", 1);
	for (int I = 0; I < Count; ++I) {
		if (I) ml_stringbuffer_add(Buffer, ",", 1);
		ml_stringbuffer_add(Buffer, Args[I]->Name, strlen(Args[I]->Name));
	}
	ml_stringbuffer_add(Buffer, "]", 1);
	Type = new(ml_type_t);
	Type->Type = MLTypeT;
	Type->Name = ml_stringbuffer_get(Buffer);
	Type->hash = Base->hash;
	Type->call = Base->call;
	Type->deref = Base->deref;
	Type->assign = Base->assign;
	int NumTypes = 1;
	for (const ml_type_t **Parent = Base->Types; Parent[0]; ++Parent) ++NumTypes;
	const ml_type_t **Types = Type->Types = anew(const ml_type_t *, NumTypes);
	*Types++ = Type;
	for (const ml_type_t **Parent = Base->Types; Parent[0]; ++Parent) {
		*Types++ = Parent[0];
		inthash_insert(Type->Parents, (uintptr_t)Parent[0], (void *)Parent[0]);
	}
	ml_generic_t *Generic = xnew(ml_generic_t, Count + 2, const ml_type_t *);
	Generic->Type = Type;
	Generic->Args[0] = Base;
	for (int I = 0; I < Count; ++I) Generic->Args[I + 1] = Args[I];
	Generic->Next = (ml_generic_t *)inthash_insert(Cache, Hash, Generic);
	Type->Args = Generic->Args;
	Type->NumArgs = Count;
	Type->Rank = Base->Rank + 1;
	return Type;
}

ml_value_t *ml_type_generic_fn(void *Data, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) ML_CHECK_ARG_TYPE(I, MLTypeT);
	return (ml_value_t *)ml_type_generic((const ml_type_t *)Data, Count, (const ml_type_t **)Args);
}

ML_TYPE(MLTypeRuleT, (), "type-rule");

typedef struct ml_type_rule_t ml_type_rule_t;

struct ml_type_rule_t {
	const ml_type_t *Type;
	ml_value_t *Next;
	ml_value_t **Values;
};

void ml_type_add_parent(ml_type_t *Type, ml_type_t *Parent, ...) {
	int Count = 0;
	va_list Values;
	va_start(Values, Parent);
	while (va_arg(Values, ml_value_t *)) ++Count;
	va_end(Values);
	ml_type_rule_t *Rule = xnew(ml_type_rule_t, Count + 1, ml_value_t *);
	Rule->Type = MLTypeRuleT;
	Rule->Next = inthash_insert(Type->Parents, (uintptr_t)Parent, Rule);
	ml_value_t **Value = Rule->Values;
	va_start(Values, Parent);
	while ((*Value++ = va_arg(Values, ml_value_t *)));
	va_end(Values);
}

#endif

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

__attribute__ ((pure)) int ml_is_subtype(const ml_type_t *Type, const ml_type_t *Parent) {
	if (Type == Parent) return 1;
	ml_value_t *Value = inthash_search(Type->Parents, (uintptr_t)Parent);
#ifdef USE_GENERICS
	while (Value && Value->Type == MLTypeRuleT) {
		ml_type_rule_t *Rule = (ml_type_rule_t *)Value;
		int I = 0;
		ml_value_t *Arg;
		while ((Arg = Rule->Values[I])) {

			++I;
		}
		Value = Rule->Next;
	}
	return Value != NULL;
#else
	return Value != NULL;
#endif
}

const ml_type_t *ml_type_max(const ml_type_t *Type1, const ml_type_t *Type2) {
	if (Type1 == Type2) return Type1;
	const ml_type_t **Parent1 = Type1->Types;
	const ml_type_t **Parent2 = Type2->Types;
	for (;;) {
		const ml_type_t *Type = Parent1[0];
		int Rank = Type->Rank;
		for (const ml_type_t **Parent3 = Parent2; Parent3[0]; ++Parent3) {
			if (Type == Parent3[0]) return Type;
			if (Rank > Parent3[0]->Rank) break;
		}
		++Parent1;
	}
	return MLAnyT;
}

ML_METHOD("*", MLTypeT, MLTypeT) {
	return (ml_value_t *)ml_type_max((ml_type_t *)Args[0], (ml_type_t *)Args[1]);
}

ML_METHOD("<", MLTypeT, MLTypeT) {
	ml_type_t *Type1 = (ml_type_t *)Args[0];
	ml_type_t *Type2 = (ml_type_t *)Args[1];
	if (Type1 == Type2) return MLNil;
	if (ml_is_subtype(Type1, Type2)) return Args[1];
	return MLNil;
}

ML_METHOD("<=", MLTypeT, MLTypeT) {
	ml_type_t *Type1 = (ml_type_t *)Args[0];
	ml_type_t *Type2 = (ml_type_t *)Args[1];
	if (Type1 == Type2) return Args[1];
	if (ml_is_subtype(Type1, Type2)) return Args[1];
	return MLNil;
}

ML_METHOD(">", MLTypeT, MLTypeT) {
	ml_type_t *Type1 = (ml_type_t *)Args[0];
	ml_type_t *Type2 = (ml_type_t *)Args[1];
	if (Type1 == Type2) return MLNil;
	if (ml_is_subtype(Type2, Type1)) return Args[1];
	return MLNil;
}

ML_METHOD(">=", MLTypeT, MLTypeT) {
	ml_type_t *Type1 = (ml_type_t *)Args[0];
	ml_type_t *Type2 = (ml_type_t *)Args[1];
	if (Type1 == Type2) return Args[1];
	if (ml_is_subtype(Type2, Type1)) return Args[1];
	return MLNil;
}

#ifdef USE_GENERICS
ML_METHODVX("[]", MLTypeT) {
	ml_type_t *Type = (ml_type_t *)Args[0];
	ml_value_t *Function = stringmap_search(Type->Exports, "[]");
	if (!Function) ML_ERROR("TypeError", "%s is not a generic type", Type->Name);
	return ml_call(Caller, Function, Count - 1, Args + 1);
}
#endif

static void ML_TYPED_FN(ml_iterate, MLNilT, ml_state_t *Caller, ml_value_t *Value) {
	ML_RETURN(Value);
}

ML_METHOD("in", MLAnyT, MLTypeT) {
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
	if (Partial->Count > CombinedCount) CombinedCount = Partial->Count;
	ml_value_t **CombinedArgs = anew(ml_value_t *, CombinedCount);
	int I = 0, J = 0;
	for (; I < Partial->Count; ++I) {
		CombinedArgs[I] = Partial->Args[I] ?: (J < Count) ? Args[J++] : MLNil;
	}
	for (; I < CombinedCount; ++I) {
		CombinedArgs[I] = (J < Count) ? Args[J++] : MLNil;
	}
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
	Partial->Count = 0;
	Partial->Set = 0;
	return (ml_value_t *)Partial;
}

ml_value_t *ml_partial_function_set(ml_value_t *Partial0, size_t Index, ml_value_t *Value) {
	ml_partial_function_t *Partial = (ml_partial_function_t *)Partial0;
	++Partial->Set;
	if (Partial->Count < Index + 1) Partial->Count = Index + 1;
	return Partial->Args[Index] = Value;
}

ML_METHOD("!!", MLFunctionT, MLListT) {
//!function
//<Function
//<List
//>partialfunction
// Returns a function equivalent to :mini:`fun(Args...) Function(List/1, List/2, ..., Args...)`.
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
		ml_value_t *Value = ml_unpack(Values, I + 1);
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
	if (!function) return ml_simple_inline(IndexMethod, 2, Value, ml_integer(Index));
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

ML_METHOD("write", MLStringBufferT, MLTupleT) {
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
	if (Index > Tuple->Size) return MLNil;
	return Tuple->Values[Index - 1];
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

ML_METHOD("|", MLIntegerT, MLIntegerT) {
//!number
//<Int/1
//<Int/2
//>integer
// Returns :mini:`Int/2`. if it is divisible by :mini:`Int/1` and :mini:`nil` otherwise.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	return (IntegerB % IntegerA) ? MLNil : Args[1];
}

ML_METHOD("!|", MLIntegerT, MLIntegerT) {
//!number
//<Int/1
//<Int/2
//>integer
// Returns :mini:`Int/2`. if it is not divisible by :mini:`Int/1` and :mini:`nil` otherwise.
	int64_t IntegerA = ml_integer_value(Args[0]);
	int64_t IntegerB = ml_integer_value(Args[1]);
	return (IntegerB % IntegerA) ? Args[1] : MLNil;
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
// Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).
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
// Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.
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
// Returns a range with the same limits as :mini:`Range` but with step :mini:`Step`.
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = Range0->Start;
	Range->Limit = Range0->Limit;
	Range->Step = ml_integer_value(Args[1]);
	return (ml_value_t *)Range;
}

ML_METHOD("count", MLIntegerRangeT) {
//!range
//<X
//>integer
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	int64_t Diff = Range->Limit - Range->Start;
	if (!Range->Step) {
		return (ml_value_t *)Zero;
	} else if (Range->Limit < Range->Start) {
		return (ml_value_t *)Zero;
	} else {
		return ml_integer(Diff / Range->Step + 1);
	}
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
	Range->Count = 1 + floor(Range->Limit - Range->Start);
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
	double C = (Limit - Start) / Range->Step + 1;
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
	double C = (Limit - Start) / Step + 1;
	if (C > LONG_MAX) C = LONG_MAX;
	Range->Count = LONG_MAX;
	return (ml_value_t *)Range;
}

ML_METHOD("count", MLRealRangeT) {
//!range
//<X
//>integer
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	return ml_integer(Range->Count);
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

static __attribute__ ((pure)) unsigned int ml_method_definition_score(ml_method_definition_t *Definition, int Count, const ml_type_t **Types) {
	unsigned int Score = 1;
	if (Definition->Count > Count) return 0;
	if (Definition->Count < Count) {
		if (!Definition->Variadic) return 0;
		Count = Definition->Count;
	} else if (!Definition->Variadic) {
		Score = 2;
	}
	for (int I = 0; I < Count; ++I) {
		const ml_type_t *Type = Definition->Types[I];
		if (ml_is_subtype(Types[I], Type)) goto found;
		return 0;
	found:
		Score += 5 + Type->Rank;
	}
	return Score;
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
		unsigned int Score = ml_method_definition_score(Definition, Count, Types);
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
#ifdef USE_ML_THREADSAFE
	static pthread_mutex_t Lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&Lock);
#endif
	ml_method_t **Slot = (ml_method_t **)stringmap_slot(Methods, Name);
	if (!Slot[0]) {
		ml_method_t *Method = new(ml_method_t);
		Method->Type = MLMethodT;
		Method->Name = Name;
		Slot[0] = Method;
	}
#ifdef USE_ML_THREADSAFE
	pthread_mutex_unlock(&Lock);
#endif
	return (ml_value_t *)Slot[0];
}

ml_value_t *ml_method_anon(const char *Name) {
	ml_method_t *Method = new(ml_method_t);
	Method->Type = MLMethodT;
	Method->Name = Name;
	return (ml_value_t *)Method;
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

ML_METHOD("write", MLStringBufferT, MLMethodT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_method_t *Method = (ml_method_t *)Args[1];
	ml_stringbuffer_add(Buffer, ":", 1);
	ml_stringbuffer_add(Buffer, Method->Name, strlen(Method->Name));
	return (ml_value_t *)Buffer;
}

/*ML_METHODVX("[]", MLMethodT) {
	ml_method_t *Method = (ml_method_t *)Args[0];
	for (int I = 1; I < Count; ++I) ML_CHECKX_ARG_TYPE(I, MLTypeT);
	ml_value_t *Matches = ml_list();
	const ml_type_t **Types = (const ml_type_t **)Args + 1;
	--Count;
	ml_methods_t *Methods = Caller->Context->Values[ML_METHODS_INDEX];
	do {
		ml_method_definition_t *Definition = inthash_search(Methods->Definitions, (uintptr_t)Method);
		while (Definition) {
			unsigned int Score = ml_method_definition_score(Definition, Count, Types);
			if (Score) {
				ml_value_t *Match = ml_tuple(2);
				ml_tuple_set(Match, 1, Definition->Callback);
				ml_tuple_set(Match, 2, ml_integer(Score));
				ml_list_put(Matches, Match);
			}
			Definition = Definition->Next;
		}
		Methods = Methods->Parent;
	} while (Methods);
	ML_RETURN(Matches);
}*/

ML_FUNCTIONX(MLMethodSet) {
//!method
//@method::set
//<Method
//<Types...:type
//<Function:function
//>Function
	ML_CHECKX_ARG_COUNT(2);
	if (ml_is(Args[0], MLTypeT)) Args[0] = ((ml_type_t *)Args[0])->Constructor;
	ML_CHECKX_ARG_TYPE(0, MLMethodT);
	int NumTypes = Count - 2, Variadic = 0;
	if (Count >= 3 && Args[Count - 2] == RangeMethod) {
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
#include "ml_types_init.c"
	MLBooleanT->Constructor = MLBooleanOfMethod;
	stringmap_insert(MLBooleanT->Exports, "of", MLBooleanOfMethod);
	MLNumberT->Constructor = MLNumberOfMethod;
	stringmap_insert(MLNumberT->Exports, "of", MLNumberOfMethod);
	MLIntegerT->Constructor = MLIntegerOfMethod;
	stringmap_insert(MLIntegerT->Exports, "of", MLIntegerOfMethod);
	MLRealT->Constructor = MLRealOfMethod;
	stringmap_insert(MLRealT->Exports, "of", MLRealOfMethod);
	MLMethodT->Constructor = MLMethodOfMethod;
	stringmap_insert(MLMethodT->Exports, "of", MLMethodOfMethod);
	stringmap_insert(MLMethodT->Exports, "set", MLMethodSet);
#ifdef USE_GENERICS
	stringmap_insert(MLFunctionT->Exports, "[]", ml_cfunction(MLFunctionT, ml_type_generic_fn));
	stringmap_insert(MLIteratableT->Exports, "[]", ml_cfunction(MLIteratableT, ml_type_generic_fn));
	stringmap_insert(MLListT->Exports, "[]", ml_cfunction(MLListT, ml_type_generic_fn));
	stringmap_insert(MLMapT->Exports, "[]", ml_cfunction(MLMapT, ml_type_generic_fn));
	stringmap_insert(MLTupleT->Exports, "[]", ml_cfunction(MLTupleT, ml_type_generic_fn));
#endif
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
	ml_method_by_value(MLNumberOfMethod, NULL, ml_identity, MLRealT, NULL);
	ml_method_by_value(MLNumberOfMethod, NULL, ml_identity, MLIntegerT, NULL);
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
	stringmap_insert(Globals, "method", MLMethodT);
	ml_string_init(Globals);
	ml_list_init(Globals);
	ml_map_init(Globals);
	stringmap_insert(Globals, "tuple", MLTupleT);
}

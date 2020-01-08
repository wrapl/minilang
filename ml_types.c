#include "minilang.h"
#include "ml_internal.h"
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
#include "stringmap.h"

long ml_default_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	long Hash = 5381;
	for (const char *P = Value->Type->Name; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

ml_value_t *ml_default_deref(ml_value_t *Ref) {
	return Ref;
}

ml_value_t *ml_default_assign(ml_value_t *Ref, ml_value_t *Value) {
	return ml_error("TypeError", "value is not assignable");
}

struct ml_typed_fn_node_t {
	void *TypedFn, *Function;
};

inline void *ml_typed_fn_get(const ml_type_t *Type, void *TypedFn) {
	do {
		ml_typed_fn_node_t *Nodes = Type->TypedFns;
		if (!Nodes) return NULL;
		size_t Mask = Type->TypedFnsSize - 1;
		size_t Index = ((uintptr_t)TypedFn >> 5) & Mask;
		size_t Incr = ((uintptr_t)TypedFn >> 9) | 1;
		for (;;) {
			if (Nodes[Index].TypedFn == TypedFn) return Nodes[Index].Function;
			if (Nodes[Index].TypedFn < TypedFn) break;
			Index = (Index + Incr) & Mask;
		}
		Type = Type->Parent;
	} while (Type);
	return NULL;
}

static void ml_typed_fn_nodes_sort(ml_typed_fn_node_t *A, ml_typed_fn_node_t *B) {
	ml_typed_fn_node_t *A1 = A, *B1 = B;
	ml_typed_fn_node_t Temp = *A;
	ml_typed_fn_node_t Pivot = *B;
	while (A1 < B1) {
		if (Temp.TypedFn > Pivot.TypedFn) {
			*A1 = Temp;
			++A1;
			Temp = *A1;
		} else {
			*B1 = Temp;
			--B1;
			Temp = *B1;
		}
	}
	*A1 = Pivot;
	if (A1 - A > 1) ml_typed_fn_nodes_sort(A, A1 - 1);
	if (B - B1 > 1) ml_typed_fn_nodes_sort(B1 + 1, B);
}

void ml_typed_fn_set(ml_type_t *Type, void *TypedFn, void *Function) {
	ml_typed_fn_node_t *Nodes = Type->TypedFns;
	if (!Nodes) {
		Nodes = Type->TypedFns = anew(ml_typed_fn_node_t, 4);
		Type->TypedFnsSize = 4;
		Type->TypedFnSpace = 3;
		size_t Index =  ((uintptr_t)TypedFn >> 5) & 3;
		Nodes[Index].TypedFn = TypedFn;
		Nodes[Index].Function = Function;
		return;
	}
	size_t Mask = Type->TypedFnsSize - 1;
	size_t Index = ((uintptr_t)TypedFn >> 5) & Mask;
	size_t Incr = ((uintptr_t)TypedFn >> 9) | 1;
	for (;;) {
		if (Nodes[Index].TypedFn == TypedFn) {
			Nodes[Index].Function = Function;
			return;
		}
		if (Nodes[Index].TypedFn < TypedFn) break;
		Index = (Index + Incr) & Mask;
	}
	if (--Type->TypedFnSpace > 1) {
		void *TypedFn1 = Nodes[Index].TypedFn;
		void *Function1 = Nodes[Index].Function;
		Nodes[Index].TypedFn = TypedFn;
		Nodes[Index].Function = Function;
		while (TypedFn1) {
			Incr = ((uintptr_t)TypedFn1 >> 9) | 1;
			while (Nodes[Index].TypedFn > TypedFn1) Index = (Index + Incr) & Mask;
			void *TypedFn2 = Nodes[Index].TypedFn;
			void *Function2 = Nodes[Index].Function;
			Nodes[Index].TypedFn = TypedFn1;
			Nodes[Index].Function = Function1;
			TypedFn1 = TypedFn2;
			Function1 = Function2;
		}
	} else {
		while (Nodes[Index].TypedFn) Index = (Index + 1) & Mask;
		Nodes[Index].TypedFn = TypedFn;
		Nodes[Index].Function = Function;
		ml_typed_fn_nodes_sort(Nodes, Nodes + Mask);
		size_t Size2 = 2 * Type->TypedFnsSize;
		Mask = Size2 - 1;
		ml_typed_fn_node_t *Nodes2 = anew(ml_typed_fn_node_t, Size2);
		for (ml_typed_fn_node_t *Node = Nodes; Node->TypedFn; Node++) {
			void *TypedFn2 = Node->TypedFn;
			void *Function2 = Node->Function;
			size_t Index2 = ((uintptr_t)TypedFn2 >> 5) & Mask;
			size_t Incr2 = ((uintptr_t)TypedFn2 >> 9) | 1;
			while (Nodes2[Index2].TypedFn) Index2 = (Index2 + Incr2) & Mask;
			Nodes2[Index2].TypedFn = TypedFn2;
			Nodes2[Index2].Function = Function2;
		}
		Type->TypedFns = Nodes2;
		Type->TypedFnSpace += Type->TypedFnsSize;
		Type->TypedFnsSize = Size2;
	}
}

ml_value_t *CompareMethod;
ml_value_t *AppendMethod;
ml_value_t *IndexMethod;
ml_value_t *SymbolMethod;
ml_value_t *IterateMethod, *NextMethod, *KeyMethod, *ValueMethod;

ml_value_t *ml_to_string(ml_value_t *Value) {
	typeof(ml_to_string) *function = ml_typed_fn_get(Value->Type, ml_to_string);
	if (!function) return ml_error("TypeError", "No implementation for %s(%s)", __FUNCTION__, Value->Type->Name);
	return function(Value);
}

ml_value_t *ml_stringbuffer_append(ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	typeof(ml_stringbuffer_append) *function = ml_typed_fn_get(Value->Type, ml_stringbuffer_append);
	if (!function) return ml_inline(AppendMethod, 2, Buffer, Value);
	return function(Buffer, Value);
}

ml_value_t *ml_iterate(ml_state_t *Caller, ml_value_t *Value) {
	typeof(ml_iterate) *function = ml_typed_fn_get(Value->Type, ml_iterate);
	if (!function)  {
		ml_value_t **Args = anew(ml_value_t *, 1);
		Args[0] = Value;
		return IterateMethod->Type->call(Caller, IterateMethod, 1, Args);
	}
	return function(Caller, Value);
}

ml_value_t *ml_iter_value(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_value) *function = ml_typed_fn_get(Iter->Type, ml_iter_value);
	if (!function) {
		ml_value_t **Args = anew(ml_value_t *, 1);
		Args[0] = Iter;
		return ValueMethod->Type->call(Caller, ValueMethod, 1, Args);
	}
	return function(Caller, Iter);
}

ml_value_t *ml_iter_key(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_key) *function = ml_typed_fn_get(Iter->Type, ml_iter_key);
	if (!function)  {
		ml_value_t **Args = anew(ml_value_t *, 1);
		Args[0] = Iter;
		return KeyMethod->Type->call(Caller, KeyMethod, 1, Args);
	}
	return function(Caller, Iter);
}

ml_value_t *ml_iter_next(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_next) *function = ml_typed_fn_get(Iter->Type, ml_iter_next);
	if (!function) {
		ml_value_t **Args = anew(ml_value_t *, 1);
		Args[0] = Iter;
		return NextMethod->Type->call(Caller, NextMethod, 1, Args);
	}
	return function(Caller, Iter);
}

ml_type_t MLAnyT[1] = {{
	MLTypeT,
	NULL, "any",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_type_t MLTypeT[1] = {{
	MLTypeT,
	MLAnyT, "type",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ML_METHOD("string", MLTypeT) {
	ml_type_t *Type = (ml_type_t *)Args[0];
	return ml_string_format("<<%s>>", Type->Name);
}

ML_METHOD("string", MLNilT) {
	return ml_string("nil", 3);
}

ml_type_t MLNilT[1] = {{
	MLTypeT,
	MLAnyT, "nil",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t MLNil[1] = {{MLNilT}};

ML_METHOD("string", MLSomeT) {
	return ml_string("some", 4);
}

ml_type_t MLSomeT[1] = {{
	MLTypeT,
	MLAnyT, "some",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t MLSome[1] = {{MLSomeT}};

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

int ml_is(const ml_value_t *Value, const ml_type_t *Expected) {
	const ml_type_t *Type = Value->Type;
	while (Type) {
		if (Type == Expected) return 1;
		Type = Type->Parent;
	}
	return 0;
}

static ml_value_t *ml_function_call(ml_state_t *Caller, ml_function_t *Function, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I] = Args[I]->Type->deref(Args[I]);
		if (Arg->Type == MLErrorT) ML_CONTINUE(Caller, Arg);
	}
	ML_CONTINUE(Caller, (Function->Callback)(Function->Data, Count, Args));
}

static ml_value_t *ml_function_iterate(ml_state_t *Caller, ml_function_t *Function) {
	ML_CONTINUE(Caller, (Function->Callback)(Function->Data, 0, NULL));
}

ml_type_t MLIteratableT[1] = {{
	MLTypeT,
	MLAnyT, "iterator",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_type_t MLFunctionT[1] = {{
	MLTypeT,
	MLIteratableT, "function",
	ml_default_hash,
	(void *)ml_function_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_function(void *Data, ml_callback_t Callback) {
	ml_function_t *Function = fnew(ml_function_t);
	Function->Type = MLFunctionT;
	Function->Data = Data;
	Function->Callback = Callback;
	GC_end_stubborn_change(Function);
	return (ml_value_t *)Function;
}

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
			return ml_error("TypeError", "Parameter names must be strings or methods");
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
			return ml_error("TypeError", "Parameter names must be strings or methods");
		}
		*(Arg++) = Node->Value;
	}
	ml_value_t *Function = Args[0];
	return Function->Type->call(Caller, Function, Arg - ListArgs, ListArgs);
}

typedef struct ml_partial_function_t {
	const ml_type_t *Type;
	ml_value_t *Function;
	int Count;
	ml_value_t *Args[];
} ml_partial_function_t;

static ml_value_t *ml_partial_function_call(ml_state_t *Caller, ml_partial_function_t *Partial, int Count, ml_value_t **Args) {
	int CombinedCount = Count + Partial->Count;
	ml_value_t **CombinedArgs = anew(ml_value_t *, CombinedCount);
	memcpy(CombinedArgs, Partial->Args, Partial->Count * sizeof(ml_value_t *));
	memcpy(CombinedArgs + Partial->Count, Args, Count * sizeof(ml_value_t *));
	return Partial->Function->Type->call(Caller, Partial->Function, CombinedCount, CombinedArgs);
}

static ml_value_t *ml_partial_function_iterate(ml_state_t *Caller, ml_partial_function_t *Partial) {
	return Partial->Function->Type->call(Caller, Partial->Function, Partial->Count, Partial->Args);
}

ml_type_t MLPartialFunctionT[1] = {{
	MLTypeT,
	MLFunctionT, "partial-function",
	ml_default_hash,
	(void *)ml_partial_function_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ML_METHOD("!!", MLFunctionT, MLListT) {
	ml_list_t *ArgsList = (ml_list_t *)Args[1];
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, ArgsList->Length, ml_value_t *);
	Partial->Type = MLPartialFunctionT;
	Partial->Function = Args[0];
	Partial->Count = ArgsList->Length;
	ml_value_t **Arg = Partial->Args;
	ML_LIST_FOREACH(ArgsList, Node) *Arg++ = Node->Value;
	return (ml_value_t *)Partial;
}

typedef struct ml_chained_function_t {
	const ml_type_t *Type;
	ml_value_t *Functions[];
} ml_chained_function_t;

typedef struct ml_chained_state_t {
	ml_state_t Base;
	ml_value_t **Functions;
} ml_chained_state_t;

static ml_value_t *ml_chained_state_run(ml_chained_state_t *State, ml_value_t *Value) {
	Value = Value->Type->deref(Value);
	if (Value->Type == MLErrorT) ML_CONTINUE(State->Base.Caller, Value);
	ml_value_t *Function = *State->Functions++;
	if (!Function) ML_CONTINUE(State->Base.Caller, Value);
	return Function->Type->call((ml_state_t *)State, Function, 1, &Value);
}

static ml_value_t *ml_chained_function_call(ml_state_t *Caller, ml_chained_function_t *Chained, int Count, ml_value_t **Args) {
	ml_value_t **Functions = Chained->Functions;
	ml_value_t *Function = *Functions++;
	ml_chained_state_t *Next = new(ml_chained_state_t);
	Next->Base.Caller = Caller;
	Next->Base.run = (void *)ml_chained_state_run;
	Next->Functions = Functions;
	return Function->Type->call((ml_state_t *)Next, Function, Count, Args);
}

typedef struct ml_chained_iterator_t {
	ml_chained_state_t Base;
	ml_value_t *Iterator;
	ml_value_t **Functions;
} ml_chained_iterator_t;

ml_type_t MLChainedIteratorT[1] = {{
	MLTypeT,
	MLAnyT, "chained-iterator",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

static ml_value_t *ml_chained_iterator_run(ml_chained_iterator_t *Chained, ml_value_t *Value);

static ml_value_t *ml_chained_iter_next(ml_state_t *Caller, ml_chained_iterator_t *Chained) {
	Chained->Base.Base.Caller = Caller;
	Chained->Base.Base.run = (void *)ml_chained_iterator_run;
	return ml_iter_next((ml_state_t *)Chained, Chained->Iterator);
}

static ml_value_t *ml_chained_iter_key(ml_state_t *Caller, ml_chained_iterator_t *Chained) {
	return ml_iter_key(Caller, Chained->Iterator);
}

static ml_value_t *ml_chained_iter_value(ml_state_t *Caller, ml_chained_iterator_t *Chained) {
	Chained->Base.Base.Caller = Caller;
	Chained->Base.Base.run = (void *)ml_chained_state_run;
	Chained->Base.Functions = Chained->Functions;
	return ml_iter_value((ml_state_t *)Chained, Chained->Iterator);
}

static ml_value_t *ml_chained_iterator_run(ml_chained_iterator_t *Chained, ml_value_t *Value) {
	Value = Value->Type->deref(Value);
	if (Value->Type == MLErrorT) ML_CONTINUE(Chained->Base.Base.Caller, Value);
	if (Value == MLNil) ML_CONTINUE(Chained->Base.Base.Caller, Value);
	Chained->Iterator = Value;
	ML_CONTINUE(Chained->Base.Base.Caller, Chained);
}

static ml_value_t *ml_chained_function_iterate(ml_state_t *Caller, ml_chained_function_t *Chained) {
	ml_value_t **Functions = Chained->Functions;
	ml_value_t *Function = *Functions++;
	ml_chained_iterator_t *Iterator = new(ml_chained_iterator_t);
	Iterator->Base.Base.Type =  MLChainedIteratorT;
	Iterator->Base.Base.Caller = Caller;
	Iterator->Base.Base.run = (void *)ml_chained_iterator_run;
	Iterator->Functions = Functions;
	return ml_iterate((ml_state_t *)Iterator, Function);
}

ml_type_t MLChainedFunctionT[1] = {{
	MLTypeT,
	MLFunctionT, "chained-function",
	ml_default_hash,
	(void *)ml_chained_function_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ML_METHOD(">>", MLIteratableT, MLFunctionT) {
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, 3, ml_value_t *);
	Chained->Type = MLChainedFunctionT;
	Chained->Functions[0] = Args[0];
	Chained->Functions[1] = Args[1];
	return (ml_value_t *)Chained;
}

ML_METHOD(">>", MLChainedFunctionT, MLFunctionT) {
	ml_chained_function_t *Base = (ml_chained_function_t *)Args[0];
	int N = 0;
	while (Base->Functions[N]) ++N;
	ml_chained_function_t *Chained = xnew(ml_chained_function_t, N + 2, ml_value_t *);
	Chained->Type = MLChainedFunctionT;
	for (int I = 0; I < N; ++I) Chained->Functions[I] = Base->Functions[I];
	Chained->Functions[N] = Args[1];
	return (ml_value_t *)Chained;
}

ml_type_t MLNumberT[1] = {{
	MLTypeT,
	MLFunctionT, "number",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

struct ml_integer_t {
	const ml_type_t *Type;
	long Value;
};

struct ml_real_t {
	const ml_type_t *Type;
	double Value;
};

struct ml_string_t {
	const ml_type_t *Type;
	const char *Value;
	int Length;
};

struct ml_regex_t {
	const ml_type_t *Type;
	const char *Pattern;
	regex_t Value[1];
};

static long ml_integer_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	ml_integer_t *Integer = (ml_integer_t *)Value;
	return Integer->Value;
}

static ml_value_t *ml_integer_call(ml_state_t *Caller, ml_integer_t *Value, int Count, ml_value_t **Args) {
	long Index = Value->Value;
	if (Index <= 0) Index += Count + 1;
	if (Index <= 0) ML_CONTINUE(Caller, MLNil);
	if (Index > Count) ML_CONTINUE(Caller, MLNil);
	ML_CONTINUE(Caller, Args[Index - 1]);
}

ml_type_t MLIntegerT[1] = {{
	MLTypeT,
	MLNumberT, "integer",
	ml_integer_hash,
	(void *)ml_integer_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_integer(long Value) {
	ml_integer_t *Integer = fnew(ml_integer_t);
	Integer->Type = MLIntegerT;
	Integer->Value = Value;
	GC_end_stubborn_change(Integer);
	return (ml_value_t *)Integer;
}

int ml_is_integer(ml_value_t *Value) {
	return Value->Type == MLIntegerT;
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

static long ml_real_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	ml_real_t *Real = (ml_real_t *)Value;
	return (long)Real->Value;
}

ml_type_t MLRealT[1] = {{
	MLTypeT,
	MLNumberT, "real",
	ml_real_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_real(double Value) {
	ml_real_t *Real = fnew(ml_real_t);
	Real->Type = MLRealT;
	Real->Value = Value;
	GC_end_stubborn_change(Real);
	return (ml_value_t *)Real;
}

int ml_is_real(ml_value_t *Value) {
	return Value->Type == MLRealT;
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

static long ml_string_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	ml_string_t *String = (ml_string_t *)Value;
	long Hash = 5381;
	for (int I = 0; I < String->Length; ++I) Hash = ((Hash << 5) + Hash) + String->Value[I];
	return Hash;
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
	char *Chars = snew(Length + 1);
	memcpy(Chars, String->Value + Lo - 1, Length);
	Chars[Length] = 0;
	return ml_string(Chars, Length);
}

ML_METHOD("+", MLStringT, MLStringT) {
	int Length1 = ml_string_length(Args[0]);
	int Length2 = ml_string_length(Args[1]);
	int Length =  Length1 + Length2;
	char *Chars = GC_malloc_atomic(Length + 1);
	memcpy(Chars, ml_string_value(Args[0]), Length1);
	memcpy(Chars + Length1, ml_string_value(Args[1]), Length2);
	Chars[Length] = 0;
	return ml_string(Chars, Length);
}

const char *ml_string_value(ml_value_t *Value) {
	return ((ml_string_t *)Value)->Value;
}

int ml_string_length(ml_value_t *Value) {
	return ((ml_string_t *)Value)->Length;
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
			ml_list_append(Results, ml_string(Match, MatchLength));
			Subject = Next + Length;
		} else {
			ml_list_append(Results, ml_string(Subject, strlen(Subject)));
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
			if (SubjectEnd > Subject) ml_list_append(Results, ml_string(Subject, SubjectEnd - Subject));
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
			if (Start > 0) ml_list_append(Results, ml_string(Subject, Start));
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
		ml_value_t *Results = ml_list();
		for (int I = 0; I < Regex->re_nsub + 1; ++I) {
			regoff_t Start = Matches[I].rm_so;
			if (Start >= 0) {
				size_t Length = Matches[I].rm_eo - Start;
				char *Chars = snew(Length + 1);
				memcpy(Chars, Subject + Start, Length);
				Chars[Length] = 0;
				ml_list_append(Results, ml_string(Chars, Length));
			} else {
				ml_list_append(Results, MLNil);
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
		ml_value_t *Results = ml_list();
		for (int I = 0; I < Regex->re_nsub + 1; ++I) {
			regoff_t Start = Matches[I].rm_so;
			if (Start >= 0) {
				size_t Length = Matches[I].rm_eo - Start;
				char *Chars = snew(Length + 1);
				memcpy(Chars, Subject + Start, Length);
				Chars[Length] = 0;
				ml_list_append(Results, ml_string(Chars, Length));
			} else {
				ml_list_append(Results, MLNil);
			}
		}
		return Results;
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

ml_type_t MLStringT[1] = {{
	MLTypeT,
	MLAnyT, "string",
	ml_string_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_string(const char *Value, int Length) {
	ml_string_t *String = fnew(ml_string_t);
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
	GC_end_stubborn_change(String);
	return (ml_value_t *)String;
}

ml_value_t *ml_string_format(const char *Format, ...) {
	ml_string_t *String = fnew(ml_string_t);
	String->Type = MLStringT;
	va_list Args;
	va_start(Args, Format);
	String->Length = vasprintf((char **)&String->Value, Format, Args);
	va_end(Args);
	GC_end_stubborn_change(String);
	return (ml_value_t *)String;
}

int ml_is_string(ml_value_t *Value) {
	return Value->Type == MLStringT;
}

ml_value_t *ml_string_new(void *Data, int Count, ml_value_t **Args) {
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (int I = 0; I < Count; ++I) ml_stringbuffer_append(Buffer, Args[I]);
	return ml_stringbuffer_get_string(Buffer);
}

static long ml_regex_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	ml_regex_t *Regex = (ml_regex_t *)Value;
	long Hash = 5381;
	const char *Pattern = Regex->Pattern;
	while (*Pattern) Hash = ((Hash << 5) + Hash) + *(Pattern++);
	return Hash;
}

ml_type_t MLRegexT[1] = {{
	MLTypeT,
	MLAnyT, "regex",
	ml_regex_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_regex(const char *Pattern) {
	ml_regex_t *Regex = fnew(ml_regex_t);
	Regex->Type = MLRegexT;
	Regex->Pattern = Pattern;
	int Error = regcomp(Regex->Value, Pattern, REG_EXTENDED);
	if (Error) {
		size_t ErrorSize = regerror(Error, Regex->Value, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(Error, Regex->Value, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	GC_end_stubborn_change(Regex);
	return (ml_value_t *)Regex;
}

regex_t *ml_regex_value(ml_value_t *Value) {
	ml_regex_t *Regex = (ml_regex_t *)Value;
	return Regex->Value;
}

ML_METHOD("string", MLRegexT) {
	ml_regex_t *Regex = (ml_regex_t *)Args[0];
	return ml_string(Regex->Pattern, -1);
}


typedef struct ml_method_table_t ml_method_table_t;
typedef struct ml_method_node_t ml_method_node_t;

struct ml_method_table_t {
	ml_value_t *Callback;
	const ml_type_t **Types;
	ml_method_table_t **Children;
	size_t Size, Space;
};

struct ml_method_t {
	const ml_type_t *Type;
	const char *Name;
	ml_method_table_t Table[1];
};

const char *ml_method_name(ml_value_t *Value) {
	return ((ml_method_t *)Value)->Name;
}

static long ml_method_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	ml_method_t *Method = (ml_method_t *)Value;
	long Hash = 5381;
	for (const char *P = Method->Name; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

static const ml_method_table_t *ml_method_find(const ml_method_table_t *Table, int Count, ml_value_t **Args) {
	if (Table->Size == 0) return Table;
	unsigned int Mask = Table->Size - 1;
	const ml_type_t **Types = Table->Types;
	for (const ml_type_t *Type = Args[0]->Type; Type; Type = Type->Parent) {
		unsigned int Index = ((uintptr_t)Type >> 5) & Mask;
		unsigned int Incr = ((uintptr_t)Type >> 9) | 1;
		for (;;) {
			if (Types[Index] == Type) {
				const ml_method_table_t *Result = (Count - 1)
					? ml_method_find(Table->Children[Index], Count - 1, Args + 1)
					: Table->Children[Index];
				if (Result->Callback) return Result;
				break;
			} else if (Types[Index] < Type) {
				break;
			} else {
				Index = (Index + Incr) & Mask;
			}
		}
	}
	return Table;
}

static ml_value_t *ml_method_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Arg = Args[I] = Args[I]->Type->deref(Args[I]);
		if (Arg->Type == MLErrorT) ML_CONTINUE(Caller, Arg);
	}
	ml_method_t *Method = (ml_method_t *)Value;
	const ml_method_table_t *Table = Count ? ml_method_find(Method->Table, Count, Args) : Method->Table;
	ml_value_t *Callback = Table->Callback;
	if (Callback) {
		return Callback->Type->call(Caller, Callback, Count, Args);
	} else {
		int Length = 4;
		for (int I = 0; I < Count; ++I) Length += strlen(Args[I]->Type->Name) + 2;
		char *Types = snew(Length);
		char *P = Types;
#ifdef __MINGW32__
		for (int I = 0; I < Count; ++I) {
			strcpy(P, Args[I]->Type->Name);
			P += strlen(Args[I]->Type->Name);
			strcpy(P, ", ");
			P += 2;
		}
#else
		for (int I = 0; I < Count; ++I) P = stpcpy(stpcpy(P, Args[I]->Type->Name), ", ");
#endif
		P[-2] = 0;
		ML_CONTINUE(Caller, ml_error("MethodError", "no matching method found for %s(%s)", Method->Name, Types));
	}
}

ml_type_t MLMethodT[1] = {{
	MLTypeT,
	MLFunctionT, "method",
	ml_method_hash,
	ml_method_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

typedef struct methods_t methods_t;

struct methods_t {
	methods_t *Parent;
	stringmap_t Methods[1];
};

static stringmap_t Methods[1] = {STRINGMAP_INIT};

ml_value_t *ml_method(const char *Name) {
	if (!Name) {
		ml_method_t *Method = new(ml_method_t);
		Method->Type = MLMethodT;
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

static void ml_method_nodes_sort(int A, int B, const ml_type_t **Types, ml_method_table_t **Children) {
	int A1 = A, B1 = B;
	const ml_type_t *TempType = Types[A];
	ml_method_table_t *TempChild = Children[A];
	const ml_type_t *PivotType = Types[B];
	ml_method_table_t *PivotChild = Children[B];
	while (A1 < B1) {
		if (TempType > PivotType) {
			Types[A1] = TempType;
			Children[A1] = TempChild;
			++A1;
			TempType = Types[A1];
			TempChild = Children[A1];
		} else {
			Types[B1] = TempType;
			Children[B1] = TempChild;
			--B1;
			TempType = Types[B1];
			TempChild = Children[B1];
		}
	}
	Types[A1] = PivotType;
	Children[A1] = PivotChild;
	if (A1 - A > 1) ml_method_nodes_sort(A, A1 - 1, Types, Children);
	if (B - B1 > 1) ml_method_nodes_sort(B1 + 1, B, Types, Children);
}

static ml_method_table_t *ml_method_insert(ml_method_table_t *Table, const ml_type_t *Type) {
	if (Table->Size == 0) {
		const ml_type_t **Types = anew(const ml_type_t *, 4);
		ml_method_table_t **Children = anew(ml_method_table_t *, 4);
		unsigned int Index = ((uintptr_t)Type >> 5) & 3;
		Types[Index] = Type;
		ml_method_table_t *Child = new(ml_method_table_t);
		Children[Index] = Child;
		Table->Types = Types;
		Table->Children = Children;
		Table->Size = 4;
		Table->Space = 3;
		return Child;
	}
	unsigned int Mask = Table->Size - 1;
	const ml_type_t **Types = Table->Types;
	ml_method_table_t **Children = Table->Children;
	unsigned int Index = ((uintptr_t)Type >> 5) & Mask;
	unsigned int Incr = ((uintptr_t)Type >> 9) | 1;
	for (;;) {
		if (Types[Index] == Type) {
			return Children[Index];
		} else if (Types[Index] < Type) {
			break;
		} else {
			Index = (Index + Incr) & Mask;
		}
	}
	ml_method_table_t *Child = new(ml_method_table_t);
	if (--Table->Space > 1) {
		const ml_type_t *Type1 = Types[Index];
		ml_method_table_t *Child1 = Children[Index];
		Types[Index] = Type;
		Children[Index] = Child;
		while (Type1) {
			Incr = ((uintptr_t)Type1 >> 9) | 1;
			while (Types[Index] > Type1) Index = (Index + Incr) & Mask;
			const ml_type_t *Type2 = Types[Index];
			ml_method_table_t *Child2 = Children[Index];
			Types[Index] = Type1;
			Children[Index] = Child1;
			Type1 = Type2;
			Child1 = Child2;
		}
	} else {
		while (Types[Index]) Index = (Index + 1) & Mask;
		Types[Index] = Type;
		Children[Index] = Child;
		ml_method_nodes_sort(0, Table->Size - 1, Types, Children);
		size_t Size2 = 2 * Table->Size;
		Mask = Size2 - 1;
		const ml_type_t **Types2 = anew(const ml_type_t *, Size2);
		ml_method_table_t **Children2 = anew(ml_method_table_t *, Size2);
		for (int I = 0; I < Table->Size; ++I) {
			const ml_type_t *Type2 = Types[I];
			ml_method_table_t *Child2 = Children[I];
			unsigned int Index2 = ((uintptr_t)Type2 >> 5) & Mask;
			unsigned int Incr2 = ((uintptr_t)Type2 >> 9) | 1;
			while (Types2[Index2]) Index2 = (Index2 + Incr2) & Mask;
			Types2[Index2] = Type2;
			Children2[Index2] = Child2;
		}
		Table->Types = Types2;
		Table->Children = Children2;
		Table->Space += Table->Size;
		Table->Size = Size2;
	}
	return Child;
}

void ml_method_by_name(const char *Name, void *Data, ml_callback_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)ml_method(Name);
	ml_method_table_t *Table = Method->Table;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) Table = ml_method_insert(Table, Type);
	va_end(Args);
	Table->Callback = ml_function(Data, Callback);
}

void ml_method_by_value(ml_value_t *Value, void *Data, ml_callback_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)Value;
	ml_method_table_t *Table = Method->Table;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) Table = ml_method_insert(Table, Type);
	va_end(Args);
	Table->Callback = ml_function(Data, Callback);
}

void ml_methodx_by_name(const char *Name, void *Data, ml_callbackx_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)ml_method(Name);
	ml_method_table_t *Table = Method->Table;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) Table = ml_method_insert(Table, Type);
	va_end(Args);
	Table->Callback = ml_functionx(Data, Callback);
}

void ml_methodx_by_value(ml_value_t *Value, void *Data, ml_callbackx_t Callback, ...) {
	ml_method_t *Method = (ml_method_t *)Value;
	ml_method_table_t *Table = Method->Table;
	va_list Args;
	va_start(Args, Callback);
	ml_type_t *Type;
	while ((Type = va_arg(Args, ml_type_t *))) Table = ml_method_insert(Table, Type);
	va_end(Args);
	Table->Callback = ml_functionx(Data, Callback);
}

void ml_method_by_array(ml_value_t *Value, ml_value_t *Function, int Count, ml_type_t **Types) {
	ml_method_t *Method = (ml_method_t *)Value;
	ml_method_table_t *Table = Method->Table;
	for (int I = 0; I < Count; ++I) Table = ml_method_insert(Table, Types[I]);
	Table->Callback = Function;
}

ML_METHOD("string", MLMethodT) {
	ml_method_t *Method = (ml_method_t *)Args[0];
	return ml_string_format(":%s", Method->Name);
}

int ml_list_length(ml_value_t *Value) {
	return ((ml_list_t *)Value)->Length;
}

void ml_list_to_array(ml_value_t *Value, ml_value_t **Array) {
	ml_list_t *List = (ml_list_t *)Value;
	for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) *Array++ = Node->Value;
}

ML_METHOD("length", MLListT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	return ml_integer(List->Length);
}

ML_METHOD("filter", MLListT, MLFunctionT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_value_t *Filter = Args[1];
	ml_value_t *New = ml_list();
	for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) {
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
	for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) {
		ml_value_t *Result = ml_inline(Map, 1, Node->Value);
		if (Result->Type == MLErrorT) return Result;
		ml_list_append(New, Result);
	}
	return New;
}

ML_METHOD("[]", MLListT, MLIntegerT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	long Index = ((ml_integer_t *)Args[1])->Value;
	if (Index > 0) {
		for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) {
			if (--Index == 0) return ml_reference(&Node->Value);
		}
		return MLNil;
	} else {
		Index = -Index;
		for (ml_list_node_t *Node = List->Tail; Node; Node = Node->Prev) {
			if (--Index == 0) return ml_reference(&Node->Value);
		}
		return MLNil;
	}
}

ML_METHOD("[]", MLListT, MLIntegerT, MLIntegerT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	long Index = ((ml_integer_t *)Args[1])->Value;
	long End = ((ml_integer_t *)Args[2])->Value;
	long Start = Index;
	if (Start <= 0) Start += List->Length + 1;
	if (End <= 0) End += List->Length + 1;
	if (Start <= 0 || End < Start || End > List->Length + 1) return MLNil;
	long Length = End - Start;
	ml_list_node_t *Source = 0;
	if (Index > 0) {
		for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) {
			if (--Index == 0) {
				Source = Node;
				break;
			}
		}
	} else {
		Index = -Index;
		for (ml_list_node_t *Node = List->Tail; Node; Node = Node->Prev) {
			if (--Index == 0) {
				Source = Node;
				break;
			}
		}
	}
	ml_list_t *Slice = (ml_list_t *)ml_list();
	Slice->Type = MLListT;
	Slice->Length = Length;
	ml_list_node_t **Slot = &Slice->Head, *Prev = 0, *Node = 0;
	while (--Length >= 0) {
		Node = Slot[0] = new(ml_list_node_t);
		Node->Prev = Prev;
		Node->Value = Source->Value;
		Slot = &Node->Next;
		Source = Source->Next;
		Prev = Node;
	}
	Slice->Tail = Node;
	return (ml_value_t *)Slice;
}

static ml_value_t *ml_list_call(ml_state_t *Caller, ml_list_t *List, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = Args[0]->Type->deref(Args[0]);
	if (Arg->Type == MLErrorT) ML_CONTINUE(Caller, Arg);
	if (Arg->Type != MLIntegerT) ML_CONTINUE(Caller, ml_error("TypeError", "List index must be an integer"));
	long Index = ((ml_integer_t *)Args[0])->Value;
	ml_value_t *Value = MLNil;
	if (Index > 0) {
		for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) {
			if (--Index == 0) {
				Value = Node->Value;
				break;
			}
		}
	} else {
		Index = -Index;
		for (ml_list_node_t *Node = List->Tail; Node; Node = Node->Prev) {
			if (--Index == 0) {
				Value = Node->Value;
				break;
			}
		}
	}
	ML_CONTINUE(Caller, Value);
}

ml_type_t MLListT[1] = {{
	MLTypeT,
	MLFunctionT, "list",
	ml_default_hash,
	(void *)ml_list_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_list() {
	ml_list_t *List = new(ml_list_t);
	List->Type = MLListT;
	return (ml_value_t *)List;
}

int ml_is_list(ml_value_t *Value) {
	return Value->Type == MLListT;
}

void ml_list_push(ml_value_t *List0, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = new(ml_list_node_t);
	Node->Value = Value;
	Node->Next = List->Head;
	if (List->Head) {
		List->Head->Prev = Node;
	} else {
		List->Tail = Node;
	}
	List->Head = Node;
	List->Length += 1;
}

void ml_list_put(ml_value_t *List0, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = new(ml_list_node_t);
	Node->Value = Value;
	Node->Prev = List->Tail;
	if (List->Tail) {
		List->Tail->Next = Node;
	} else {
		List->Head = Node;
	}
	List->Tail = Node;
	List->Length += 1;
}

ml_value_t *ml_list_pop(ml_value_t *List0) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = List->Head;
	if (Node) {
		List->Head = Node->Next;
		if (List->Head) {
			List->Head->Prev = 0;
		} else {
			List->Tail = 0;
		}
		--List->Length;
		return Node->Value;
	} else {
		return 0;
	}
}

ml_value_t *ml_list_pull(ml_value_t *List0) {
	ml_list_t *List = (ml_list_t *)List0;
	ml_list_node_t *Node = List->Tail;
	if (Node) {
		List->Tail = Node->Prev;
		if (List->Tail) {
			List->Tail->Next = 0;
		} else {
			List->Head = 0;
		}
		--List->Length;
		return Node->Value;
	} else {
		return 0;
	}
}

int ml_list_foreach(ml_value_t *Value, void *Data, int (*callback)(ml_value_t *, void *)) {
	ml_list_t *List = (ml_list_t *)Value;
	for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) {
		if (callback(Node->Value, Data)) return 1;
	}
	return 0;
}

ml_value_t *ml_list_new(void *Data, int Count, ml_value_t **Args) {
	ml_list_t *List = new(ml_list_t);
	List->Type = MLListT;
	ml_list_node_t **Slot = &List->Head;
	ml_list_node_t *Prev = NULL;
	for (int I = 0; I < Count; ++I) {
		ml_list_node_t *Node = Slot[0] = new(ml_list_node_t);
		Node->Value = Args[I];
		Node->Prev = Prev;
		Prev = Node;
		Slot = &Node->Next;
	}
	List->Tail = Prev;
	List->Length = Count;
	return (ml_value_t *)List;
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
			if (Result->Type == MLIntegerT) {
				Compare = ((ml_integer_t *)Result)->Value;
			} else if (Result->Type == MLRealT) {
				Compare = ((ml_real_t *)Result)->Value;
			} else {
				return ml_error("CompareError", "comparison must return number");
			}
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

static ml_value_t *ml_map_insert_internal(ml_map_t *Map, ml_map_node_t **Slot, long Hash, ml_value_t *Key, ml_value_t *Value) {
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
		Node->Value = Value;
		return NULL;
	}
	int Compare;
	if (Hash < Slot[0]->Hash) {
		Compare = -1;
	} else if (Hash > Slot[0]->Hash) {
		Compare = 1;
	} else {
		ml_value_t *Args[2] = {Key, Slot[0]->Key};
		ml_value_t *Result = ml_call(CompareMethod, 2, Args);
		if (Result->Type == MLIntegerT) {
			Compare = ((ml_integer_t *)Result)->Value;
		} else if (Result->Type == MLRealT) {
			Compare = ((ml_real_t *)Result)->Value;
		} else {
			return ml_error("CompareError", "comparison must return number");
		}
	}
	if (!Compare) {
		ml_value_t *Old = Slot[0]->Value;
		Slot[0]->Value = Value;
		return Old;
	} else {
		ml_value_t *Old = ml_map_insert_internal(Map, Compare < 0 ? &Slot[0]->Left : &Slot[0]->Right, Hash, Key, Value);
		ml_map_rebalance(Slot);
		ml_map_update_depth(Slot[0]);
		return Old;
	}
}

ml_value_t *ml_map_insert(ml_value_t *Map0, ml_value_t *Key, ml_value_t *Value) {
	ml_map_t *Map = (ml_map_t *)Map0;
	return ml_map_insert_internal(Map, &Map->Root, Key->Type->hash(Key, NULL), Key, Value);
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
		if (Result->Type == MLIntegerT) {
			Compare = ((ml_integer_t *)Result)->Value;
		} else if (Result->Type == MLRealT) {
			Compare = ((ml_real_t *)Result)->Value;
		} else {
			return ml_error("CompareError", "comparison must return number");
		}
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

static ml_value_t *ml_map_index_get(void *Data, const char *Name) {
	ml_value_t *Map = (ml_value_t *)Data;
	ml_value_t *Key = (ml_value_t *)Name;
	return ml_map_search(Map, Key);
}

static ml_value_t *ml_map_index_set(void *Data, const char *Name, ml_value_t *Value) {
	ml_value_t *Map = (ml_value_t *)Data;
	ml_value_t *Key = (ml_value_t *)Name;
	ml_map_insert(Map, Key, Value);
	return Value;
}

int ml_map_size(ml_value_t *Map0) {
	ml_map_t *Map = (ml_map_t *)Map0;
	return Map->Size;
}

ML_METHOD("size", MLMapT) {
	ml_map_t *Map = (ml_map_t *)Args[0];
	return ml_integer(Map->Size);
}

ML_METHOD("[]", MLMapT, MLAnyT) {
	ml_map_t *Map = (ml_map_t *)Args[0];
	if (Count < 1) return MLNil;
	ml_value_t *Key = Args[1];
	return ml_property(Map, (const char *)Key, ml_map_index_get, ml_map_index_set, NULL, NULL);
}

ML_METHOD("::", MLMapT, MLStringT) {
	ml_map_t *Map = (ml_map_t *)Args[0];
	if (Count < 1) return MLNil;
	ml_value_t *Key = Args[1];
	return ml_property(Map, (const char *)Key, ml_map_index_get, ml_map_index_set, NULL, NULL);
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

static ml_value_t *ml_map_call(ml_state_t *Caller, ml_value_t *Map, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = Args[0]->Type->deref(Args[0]);
	if (Arg->Type == MLErrorT) ML_CONTINUE(Caller, Arg);
	ML_CONTINUE(Caller, ml_map_search(Map, Args[0]));
}

ml_type_t MLMapT[1] = {{
	MLTypeT,
	MLFunctionT, "map",
	ml_default_hash,
	(void *)ml_map_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_map() {
	ml_map_t *Map = new(ml_map_t);
	Map->Type = MLMapT;
	return (ml_value_t *)Map;
}

int ml_is_map(ml_value_t *Value) {
	return Value->Type == MLMapT;
}

int ml_map_foreach(ml_value_t *Value, void *Data, int (*callback)(ml_value_t *, ml_value_t *, void *)) {
	ml_map_t *Map = (ml_map_t *)Value;
	for (ml_map_node_t *Node = Map->Head; Node; Node = Node->Next) {
		if (callback(Node->Key, Node->Value, Data)) return 1;
	}
	return 0;
}

ml_value_t *ml_map_new(void *Data, int Count, ml_value_t **Args) {
	ml_map_t *Map = new(ml_map_t);
	Map->Type = MLMapT;
	for (int I = 0; I < Count; I += 2) ml_map_insert((ml_value_t *)Map, Args[I], Args[I + 1]);
	return (ml_value_t *)Map;
}

static long ml_tuple_hash(ml_tuple_t *Tuple, ml_hash_chain_t *Chain) {
	long Hash = 739;
	for (int I = 0; I < Tuple->Size; ++I) Hash = ((Hash << 3) + Hash) + ml_hash(Tuple->Values[I]);
	return Hash;
}

static ml_value_t *ml_tuple_deref(ml_tuple_t *Ref) {
	for (int I = 0; I < Ref->Size; ++I) {
		ml_value_t *Old = Ref->Values[I];
		ml_value_t *New = Old->Type->deref(Old);
		if (Old != New) {
			ml_tuple_t *Deref = xnew(ml_tuple_t, Ref->Size, ml_value_t *);
			Deref->Type = MLTupleT;
			Deref->Size = Ref->Size;
			for (int J = 0; J < I; ++J) Deref->Values[J] = Ref->Values[J];
			Deref->Values[I] = New;
			for (int J = I + 1; J < Ref->Size; ++J) {
				Deref->Values[J] = Ref->Values[J]->Type->deref(Ref->Values[J]);
			}
			return (ml_value_t *)Deref;
		}
	}
	return (ml_value_t *)Ref;
}

static ml_value_t *ml_tuple_assign(ml_tuple_t *Ref, ml_value_t *Value) {
	if (Value->Type != MLTupleT) return ml_error("TypeError", "Can only assign a tuple to a tuple");
	ml_tuple_t *TupleValue = (ml_tuple_t *)Value;
	size_t Count = Ref->Size;
	if (TupleValue->Size < Count) Count = TupleValue->Size;
	ml_value_t **Values = TupleValue->Values;
	for (int I = 0; I < Count; ++I) {
		ml_value_t *Value0 = Values[I]->Type->deref(Values[I]);
		ml_value_t *Result = Ref->Values[I]->Type->assign(Ref->Values[I], Value0);
		if (Result->Type == MLErrorT) return Result;
	}
	return Value;
}

ml_type_t MLTupleT[1] = {{
	MLTypeT,
	MLAnyT, "tuple",
	(void *)ml_tuple_hash,
	ml_default_call,
	(void *)ml_tuple_deref,
	(void *)ml_tuple_assign,
	NULL, 0, 0
}};

ML_METHOD("size", MLTupleT) {
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	return ml_integer(Tuple->Size);
}

ML_METHOD("[]", MLTupleT, MLIntegerT) {
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	long Index = ((ml_integer_t *)Args[1])->Value;
	if (--Index < 0) Index += Tuple->Size + 1;
	if (Index < 0 || Index >= Tuple->Size) return ml_error("RangeError", "Tuple index out of bounds");
	return ml_reference(Tuple->Values + Index);
}

ml_value_t *ml_tuple(size_t Size) {
	ml_tuple_t *Tuple = xnew(ml_tuple_t, Size, ml_value_t *);
	Tuple->Type = MLTupleT;
	Tuple->Size = Size;
	return (ml_value_t *)Tuple;
}

size_t ml_tuple_size(ml_value_t *Tuple) {
	return ((ml_tuple_t *)Tuple)->Size;
}

ml_value_t *ml_tuple_get(ml_value_t *Tuple, size_t Index) {
	return ((ml_tuple_t *)Tuple)->Values[Index];
}

ml_value_t *ml_tuple_set(ml_value_t *Tuple, size_t Index, ml_value_t *Value) {
	return ((ml_tuple_t *)Tuple)->Values[Index] = Value;
}

ml_value_t *ml_tuple_new(void *Data, int Count, ml_value_t **Args) {
	ml_tuple_t *Tuple = xnew(ml_tuple_t, Count, ml_value_t *);
	Tuple->Type = MLTupleT;
	Tuple->Size = Count;
	memcpy(Tuple->Values, Args, Count * sizeof(ml_value_t *));
	return (ml_value_t *)Tuple;
}

ml_value_t *ml_stringbuffer_append_tuple(ml_stringbuffer_t *Buffer, ml_tuple_t *Value) {
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

struct ml_property_t {
	const ml_type_t *Type;
	void *Data;
	const char *Name;
	ml_getter_t Get;
	ml_setter_t Set;
	ml_getter_t Iterate;
	ml_getter_t Next;
	ml_getter_t Key;
};

static ml_value_t *ml_property_deref(ml_value_t *Ref) {
	ml_property_t *Property = (ml_property_t *)Ref;
	return (Property->Get)(Property->Data, Property->Name);
}

static ml_value_t *ml_property_assign(ml_value_t *Ref, ml_value_t *Value) {
	Value = Value->Type->deref(Value);
	if (Value->Type == MLErrorT) return Value;
	ml_property_t *Property = (ml_property_t *)Ref;
	if (Property->Set) {
		return (Property->Set)(Property->Data, Property->Name, Value);
	} else {
		return ml_error("TypeError", "value is not assignable");
	}
}

static ml_value_t *ml_property_iterate(ml_state_t *Caller, ml_value_t *Value) {
	ml_property_t *Property = (ml_property_t *)Value;
	if (Property->Iterate) {
		ML_CONTINUE(Caller, (Property->Iterate)(Property->Data, "next"));
	} else {
		ML_CONTINUE(Caller, ml_error("TypeError", "value is not iterable"));
	}
}

ml_type_t MLPropertyT[1] = {{
	MLTypeT,
	MLAnyT, "property",
	ml_default_hash,
	ml_default_call,
	ml_property_deref,
	ml_property_assign,
	NULL, 0, 0
}};

ml_value_t *ml_property(void *Data, const char *Name, ml_getter_t Get, ml_setter_t Set, ml_getter_t Next, ml_getter_t Key) {
	ml_property_t *Property = new(ml_property_t);
	Property->Type = MLPropertyT;
	Property->Data = Data;
	Property->Name = Name;
	Property->Get = Get;
	Property->Set = Set;
	Property->Next = Next;
	Property->Key = Key;
	return (ml_value_t *)Property;
}

#define MAX_TRACE 16

struct ml_error_t {
	const ml_type_t *Type;
	const char *Error;
	const char *Message;
	ml_source_t Trace[MAX_TRACE];
};

ml_type_t MLErrorT[1] = {{
	MLTypeT,
	MLAnyT, "error",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_type_t MLErrorValueT[1] = {{
	MLTypeT,
	MLErrorT, "error_value",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ml_value_t *ml_error(const char *Error, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Message;
	vasprintf(&Message, Format, Args);
	va_end(Args);
	ml_error_t *Value = new(ml_error_t);
	Value->Type = MLErrorT;
	Value->Error = Error;
	Value->Message = Message;
	memset(Value->Trace, 0, sizeof(Value->Trace));
	return (ml_value_t *)Value;
}

int ml_is_error(ml_value_t *Value) {
	return Value->Type == MLErrorT;
}

const char *ml_error_type(ml_value_t *Value) {
	return ((ml_error_t *)Value)->Error;
}

const char *ml_error_message(ml_value_t *Value) {
	return ((ml_error_t *)Value)->Message;
}

ML_METHOD("type", MLErrorT) {
	return ml_string(((ml_error_t *)Args[0])->Error, -1);
}

ML_METHOD("message", MLErrorT) {
	return ml_string(((ml_error_t *)Args[0])->Message, -1);
}

int ml_error_trace(ml_value_t *Value, int Level, const char **Source, int *Line) {
	ml_error_t *Error = (ml_error_t *)Value;
	if (Level >= MAX_TRACE) return 0;
	if (!Error->Trace[Level].Name) return 0;
	Source[0] = Error->Trace[Level].Name;
	Line[0] = Error->Trace[Level].Line;
	return 1;
}

void ml_error_trace_add(ml_value_t *Value, ml_source_t Source) {
	ml_error_t *Error = (ml_error_t *)Value;
	for (int I = 0; I < MAX_TRACE; ++I) if (!Error->Trace[I].Name) {
		Error->Trace[I] = Source;
		return;
	}
}

void ml_error_print(ml_value_t *Value) {
	ml_error_t *Error = (ml_error_t *)Value;
	printf("Error: %s\n", Error->Message);
	for (int I = 0; (I < MAX_TRACE) && Error->Trace[I].Name; ++I) {
		printf("\t%s:%d\n", Error->Trace[I].Name, Error->Trace[I].Line);
	}
}

struct ml_stringbuffer_node_t {
	ml_stringbuffer_node_t *Next;
	char Chars[ML_STRINGBUFFER_NODE_SIZE];
};

static ml_stringbuffer_node_t *Cache = NULL;
static GC_descr StringBufferDesc = 0;
static pthread_mutex_t CacheMutex[1] = {PTHREAD_MUTEX_DEFAULT};

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
		ml_stringbuffer_node_t *Next;
		pthread_mutex_lock(CacheMutex);
		if (Cache) {
			Next = Cache;
			Cache = Cache->Next;
			Next->Next = NULL;
		} else {
			Next = (ml_stringbuffer_node_t *)GC_malloc_explicitly_typed(sizeof(ml_stringbuffer_node_t), StringBufferDesc);
			//printf("Allocating stringbuffer: %d in total\n", ++NumStringBuffers);
		}
		pthread_mutex_unlock(CacheMutex);
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

char *ml_stringbuffer_get(ml_stringbuffer_t *Buffer) {
	char *String = snew(Buffer->Length + 1);
	if (Buffer->Length == 0) {
		String[0] = 0;
	} else {
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
		pthread_mutex_lock(CacheMutex);
		ml_stringbuffer_node_t **Slot = &Cache;
		while (Slot[0]) Slot = &Slot[0]->Next;
		Slot[0] = Buffer->Nodes;
		pthread_mutex_unlock(CacheMutex);
		Buffer->Nodes = NULL;
		Buffer->Length = Buffer->Space = 0;
	}
	return String;
}

char *ml_stringbuffer_get_uncollectable(ml_stringbuffer_t *Buffer) {
	char *String = GC_malloc_atomic_uncollectable(Buffer->Length + 1);
	if (Buffer->Length == 0) {
		String[0] = 0;
	} else {
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
		pthread_mutex_lock(CacheMutex);
		ml_stringbuffer_node_t **Slot = &Cache;
		while (Slot[0]) Slot = &Slot[0]->Next;
		Slot[0] = Buffer->Nodes;
		pthread_mutex_unlock(CacheMutex);
		Buffer->Nodes = NULL;
		Buffer->Length = Buffer->Space = 0;
	}
	return String;
}

ml_value_t *ml_stringbuffer_get_string(ml_stringbuffer_t *Buffer) {
	size_t Length = Buffer->Length;
	if (Length == 0) {
		return ml_string("", 0);
	} else {
		char *Chars = snew(Length + 1);
		char *P = Chars;
		ml_stringbuffer_node_t *Node = Buffer->Nodes;
		while (Node->Next) {
			memcpy(P, Node->Chars, ML_STRINGBUFFER_NODE_SIZE);
			P += ML_STRINGBUFFER_NODE_SIZE;
			Node = Node->Next;
		}
		memcpy(P, Node->Chars, ML_STRINGBUFFER_NODE_SIZE - Buffer->Space);
		P += ML_STRINGBUFFER_NODE_SIZE - Buffer->Space;
		*P++ = 0;
		pthread_mutex_lock(CacheMutex);
		ml_stringbuffer_node_t **Slot = &Cache;
		while (Slot[0]) Slot = &Slot[0]->Next;
		Slot[0] = Buffer->Nodes;
		pthread_mutex_unlock(CacheMutex);
		Buffer->Nodes = NULL;
		Buffer->Length = Buffer->Space = 0;
		return ml_string(Chars, Length);
	}
}

ml_type_t MLStringBufferT[1] = {{
	MLTypeT,
	MLAnyT, "stringbuffer",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

int ml_stringbuffer_foreach(ml_stringbuffer_t *Buffer, void *Data, int (*callback)(const char *, size_t, void *)) {
	ml_stringbuffer_node_t *Node = Buffer->Nodes;
	if (!Node) return 0;
	while (Node->Next) {
		if (callback(Node->Chars, ML_STRINGBUFFER_NODE_SIZE, Data)) return 1;
		Node = Node->Next;
	}
	return callback(Node->Chars, ML_STRINGBUFFER_NODE_SIZE - Buffer->Space, Data);
}

ML_METHOD("append", MLStringBufferT, MLNilT) {
	return MLNil;
}

ml_value_t *ml_stringbuffer_append_nil(ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	return MLNil;
}

ML_METHOD("append", MLStringBufferT, MLSomeT) {
	return MLNil;
}

ml_value_t *ml_stringbuffer_append_some(ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	return MLNil;
}

ML_METHOD("append", MLStringBufferT, MLIntegerT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_addf(Buffer, "%ld", ml_integer_value(Args[1]));
	return MLSome;
}

ml_value_t *ml_stringbuffer_append_integer(ml_stringbuffer_t *Buffer, ml_integer_t *Value) {
	ml_stringbuffer_addf(Buffer, "%ld", Value->Value);
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLRealT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_addf(Buffer, "%f", ml_real_value(Args[1]));
	return MLSome;
}

ml_value_t *ml_stringbuffer_append_real(ml_stringbuffer_t *Buffer, ml_real_t *Value) {
	ml_stringbuffer_addf(Buffer, "%f", Value->Value);
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLStringT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_add(Buffer, ml_string_value(Args[1]), ml_string_length(Args[1]));
	return ml_string_length(Args[1]) ? MLSome : MLNil;
}

ml_value_t *ml_stringbuffer_append_string(ml_stringbuffer_t *Buffer, ml_string_t *Value) {
	ml_stringbuffer_add(Buffer, Value->Value, Value->Length);
	return Value->Length ? MLSome : MLNil;
}

ML_METHOD("append", MLStringBufferT, MLMethodT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_method_t *Method = (ml_method_t *)Args[1];
	ml_stringbuffer_add(Buffer, Method->Name, strlen(Method->Name));
	return MLSome;
}

ml_value_t *ml_stringbuffer_append_method(ml_stringbuffer_t *Buffer, ml_method_t *Value) {
	ml_stringbuffer_add(Buffer, Value->Name, strlen(Value->Name));
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLTupleT) {
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

ML_METHOD("append", MLStringBufferT, MLListT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_list_node_t *Node = ((ml_list_t *)Args[1])->Head;
	if (Node) {
		ml_stringbuffer_append(Buffer, Node->Value);
		ml_inline(AppendMethod, 2, Buffer, Node->Value);
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, " ", 1);
			ml_inline(AppendMethod, 2, Buffer, Node->Value);
		}
		return MLSome;
	} else {
		return MLNil;
	}
}

ml_value_t *ml_stringbuffer_append_list(ml_stringbuffer_t *Buffer, ml_list_t *Value) {
	ml_list_node_t *Node = ((ml_list_t *)Value)->Head;
	if (Node) {
		ml_stringbuffer_append(Buffer, Node->Value);
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, " ", 1);
			ml_stringbuffer_append(Buffer, Node->Value);
		}
		return MLSome;
	} else {
		return MLNil;
	}
}

typedef struct {ml_stringbuffer_t *Buffer; int Space;} ml_stringify_context_t;

static int stringify_map_value(ml_value_t *Key, ml_value_t *Value, ml_stringify_context_t *Ctx) {
	if (Ctx->Space) ml_stringbuffer_add(Ctx->Buffer, " ", 1);
	ml_stringbuffer_append(Ctx->Buffer, Key);
	if (Value != MLNil) {
		ml_stringbuffer_add(Ctx->Buffer, "=", 1);
		ml_stringbuffer_append(Ctx->Buffer, Value);
	}
	Ctx->Space = 1;
	return 0;
}

ML_METHOD("append", MLStringBufferT, MLMapT) {
	ml_stringify_context_t Ctx[1] = {{(ml_stringbuffer_t *)Args[0], 0}};
	ml_map_foreach(Args[1], Ctx, (void *)stringify_map_value);
	return ((ml_map_t *)Args[1])->Size ? MLSome : MLNil;
}

ml_value_t *ml_stringbuffer_append_map(ml_stringbuffer_t *Buffer, ml_map_t *Value) {
	ml_stringify_context_t Ctx[1] = {{(ml_stringbuffer_t *)Buffer, 0}};
	ml_map_foreach((ml_value_t *)Value, Ctx, (void *)stringify_map_value);
	return Value->Size ? MLSome : MLNil;
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
ml_arith_method_integer_integer("/", /)

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

static ml_integer_t One[1] = {{MLIntegerT, 1}};
static ml_integer_t NegOne[1] = {{MLIntegerT, -1}};
static ml_integer_t Zero[1] = {{MLIntegerT, 0}};

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

static ml_value_t *ml_integer_iter_current(ml_state_t *Caller, ml_integer_iter_t *Iter) {
	ML_CONTINUE(Caller, ml_integer(Iter->Current));
}

static ml_value_t *ml_integer_iter_next(ml_state_t *Caller, ml_integer_iter_t *Iter) {
	Iter->Current += Iter->Step;
	if (Iter->Step > 0) {
		if (Iter->Current > Iter->Limit) ML_CONTINUE(Caller, MLNil);
	} else if (Iter->Step < 0) {
		if (Iter->Current < Iter->Limit) ML_CONTINUE(Caller, MLNil);
	}
	++Iter->Index;
	ML_CONTINUE(Caller, Iter);
}

static ml_value_t *ml_integer_iter_key(ml_state_t *Caller, ml_integer_iter_t *Iter) {
	ML_CONTINUE(Caller, ml_integer(Iter->Index));
}

ml_type_t MLIntegerIterT[1] = {{
	MLTypeT,
	MLAnyT, "integer-iter",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

typedef struct ml_integer_range_t {
	const ml_type_t *Type;
	long Start, Limit, Step;
} ml_integer_range_t;

static ml_value_t *ml_integer_range_iterate(ml_state_t *Caller, ml_value_t *Value) {
	ml_integer_range_t *Range = (ml_integer_range_t *)Value;
	if (Range->Step > 0 && Range->Start > Range->Limit) ML_CONTINUE(Caller, MLNil);
	if (Range->Step < 0 && Range->Start < Range->Limit) ML_CONTINUE(Caller, MLNil);
	ml_integer_iter_t *Iter = new(ml_integer_iter_t);
	Iter->Type = MLIntegerIterT;
	Iter->Index = 1;
	Iter->Current = Range->Start;
	Iter->Limit = Range->Limit;
	Iter->Step = Range->Step;
	ML_CONTINUE(Caller, Iter);
}

ml_type_t MLIntegerRangeT[1] = {{
	MLTypeT,
	MLIteratableT, "integer-range",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

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

static ml_value_t *ml_real_iter_current(ml_state_t *Caller, ml_real_iter_t *Iter) {
	ML_CONTINUE(Caller, ml_real(Iter->Current));
}

static ml_value_t *ml_real_iter_next(ml_state_t *Caller, ml_real_iter_t *Iter) {
	Iter->Current += Iter->Step;
	if (--Iter->Remaining < 0) ML_CONTINUE(Caller, MLNil);
	++Iter->Index;
	ML_CONTINUE(Caller, Iter);
}

static ml_value_t *ml_real_iter_key(ml_state_t *Caller, ml_real_iter_t *Iter) {
	ML_CONTINUE(Caller, ml_integer(Iter->Index));
}

ml_type_t MLRealIterT[1] = {{
	MLTypeT,
	MLAnyT, "real-iter",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

typedef struct ml_real_range_t {
	const ml_type_t *Type;
	double Start, Limit, Step;
	long Count;
} ml_real_range_t;

static ml_value_t *ml_real_range_iterate(ml_state_t *Caller, ml_value_t *Value) {
	ml_real_range_t *Range = (ml_real_range_t *)Value;
	if (Range->Step > 0 && Range->Start > Range->Limit) ML_CONTINUE(Caller, MLNil);
	if (Range->Step < 0 && Range->Start < Range->Limit) ML_CONTINUE(Caller, MLNil);
	ml_real_iter_t *Iter = new(ml_real_iter_t);
	Iter->Type = MLRealIterT;
	Iter->Index = 1;
	Iter->Current = Range->Start;
	Iter->Limit = Range->Limit;
	Iter->Step = Range->Step;
	Iter->Remaining = Range->Count;
	ML_CONTINUE(Caller, Iter);
}

ml_type_t MLRealRangeT[1] = {{
	MLTypeT,
	MLIteratableT, "real-range",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

ML_METHOD("..", MLNumberT, MLNumberT) {
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	if (Args[0]->Type == MLIntegerT) {
		Range->Start = ((ml_integer_t *)Args[0])->Value;
	} else if (Args[0]->Type == MLRealT) {
		Range->Start = ((ml_real_t *)Args[0])->Value;
	}
	if (Args[1]->Type == MLIntegerT) {
		Range->Limit = ((ml_integer_t *)Args[1])->Value;
	} else if (Args[1]->Type == MLRealT) {
		Range->Limit = ((ml_real_t *)Args[1])->Value;
	}
	Range->Step = 1.0;
	Range->Count = floor(Range->Limit - Range->Start);
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLNumberT, MLNumberT) {
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	if (Args[0]->Type == MLIntegerT) {
		Range->Start = ((ml_integer_t *)Args[0])->Value;
	} else if (Args[0]->Type == MLRealT) {
		Range->Start = ((ml_real_t *)Args[0])->Value;
	}
	if (Args[1]->Type == MLIntegerT) {
		Range->Step = ((ml_integer_t *)Args[1])->Value;
	} else if (Args[1]->Type == MLRealT) {
		Range->Step = ((ml_real_t *)Args[1])->Value;
	}
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
	double Step;
	if (Args[1]->Type == MLIntegerT) {
		Step = Range->Step = ((ml_integer_t *)Args[1])->Value;
	} else if (Args[1]->Type == MLRealT) {
		Step = Range->Step = ((ml_real_t *)Args[1])->Value;
	} else {
		Step = 1.0;
	}
	double C = (Limit - Start) / Step;
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
	long C = Range->Count = ((ml_integer_t *)Args[1])->Value - 1;
	Range->Step = (Range->Limit - Range->Start) / C;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerRangeT, MLRealT) {
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	double Start = Range->Start = Range0->Start;
	double Limit = Range->Limit = Range0->Limit;
	double Step = Range->Step = ((ml_real_t *)Args[1])->Value;
	double C = (Limit - Start) / Step;
	if (C > LONG_MAX) C = LONG_MAX;
	Range->Count = LONG_MAX;
	return (ml_value_t *)Range;
}

ML_METHOD("in", MLIntegerT, MLRealRangeT) {
	long Value = ((ml_integer_t *)Args[0])->Value;
	ml_real_range_t *Range = (ml_real_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("in", MLRealT, MLRealRangeT) {
	double Value = ((ml_real_t *)Args[0])->Value;
	ml_real_range_t *Range = (ml_real_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("string", MLIntegerT) {
	ml_integer_t *Integer = (ml_integer_t *)Args[0];
	ml_string_t *String = new(ml_string_t);
	String->Type = MLStringT;
	String->Length = asprintf((char **)&String->Value, "%ld", Integer->Value);
	return (ml_value_t *)String;
}

ML_METHOD("string", MLRealT) {
	ml_real_t *Real = (ml_real_t *)Args[0];
	ml_string_t *String = new(ml_string_t);
	String->Type = MLStringT;
	String->Length = asprintf((char **)&String->Value, "%f", Real->Value);
	return (ml_value_t *)String;
}

static ml_value_t *ml_identity(void *Data, int Count, ml_value_t **Args) {
	return Args[0];
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

ML_METHOD("<>", MLAnyT, MLAnyT) {
	if (Args[0] < Args[1]) return (ml_value_t *)NegOne;
	if (Args[0] > Args[1]) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("string", MLTupleT) {
	ml_tuple_t *Value = (ml_tuple_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_add(Buffer, "(", 1);
	if (Value->Size) {
		ml_stringbuffer_append(Buffer, Value->Values[0]);
		for (int I = 1; I < Value->Size; ++I) {
			ml_stringbuffer_add(Buffer, ", ", 2);
			ml_stringbuffer_append(Buffer, Value->Values[I]);
		}
	}
	ml_stringbuffer_add(Buffer, ")", 1);
	return ml_stringbuffer_get_string(Buffer);
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

typedef struct ml_list_iter_t {
	const ml_type_t *Type;
	ml_list_node_t *Node;
	long Index;
} ml_list_iter_t;

static ml_value_t *ml_list_iter_current(ml_state_t *Caller, ml_list_iter_t *Iter) {
	ML_CONTINUE(Caller, ml_reference(&Iter->Node->Value));
}

static ml_value_t *ml_list_iter_next(ml_state_t *Caller, ml_list_iter_t *Iter) {
	if (Iter->Node->Next) {
		++Iter->Index;
		Iter->Node = Iter->Node->Next;
		ML_CONTINUE(Caller, Iter);
	} else {
		ML_CONTINUE(Caller, MLNil);
	}
}

static ml_value_t *ml_list_iter_key(ml_state_t *Caller, ml_list_iter_t *Iter) {
	ML_CONTINUE(Caller, ml_integer(Iter->Index));
}

ml_type_t MLListIterT[1] = {{
	MLTypeT,
	MLAnyT, "list-iterator",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

static ml_value_t *ml_list_iterate(ml_state_t *Caller, ml_value_t *Value) {
	ml_list_t *List = (ml_list_t *)Value;
	if (List->Head) {
		ml_list_iter_t *Iter = new(ml_list_iter_t);
		Iter->Type = MLListIterT;
		Iter->Node = List->Head;
		Iter->Index = 1;
		ML_CONTINUE(Caller, Iter);
	} else {
		ML_CONTINUE(Caller, MLNil);
	}
}

ML_METHOD("push", MLListT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_node_t **Slot = List->Head ? &List->Head->Prev : &List->Tail;
	ml_list_node_t *Next = List->Head;
	for (int I = Count; --I >= 1;) {
		ml_list_node_t *Node = Slot[0] = new(ml_list_node_t);
		Node->Value = Args[I];
		Node->Next = Next;
		Next = Node;
		Slot = &Node->Prev;
	}
	List->Head = Next;
	List->Length += Count - 1;
	return (ml_value_t *)List;
}

ML_METHOD("put", MLListT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_node_t **Slot = List->Tail ? &List->Tail->Next : &List->Head;
	ml_list_node_t *Prev = List->Tail;
	for (int I = 1; I < Count; ++I) {
		ml_list_node_t *Node = Slot[0] = new(ml_list_node_t);
		Node->Value = Args[I];
		Node->Prev = Prev;
		Prev = Node;
		Slot = &Node->Next;
	}
	List->Tail = Prev;
	List->Length += Count - 1;
	return (ml_value_t *)List;
}

ML_METHOD("pop", MLListT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_node_t *Node = List->Head;
	if (Node) {
		if (!(List->Head = Node->Next)) List->Tail = NULL;
		--List->Length;
		return Node->Value;
	} else {
		return MLNil;
	}
}

ML_METHOD("pull", MLListT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	ml_list_node_t *Node = List->Tail;
	if (Node) {
		if (!(List->Tail = Node->Next)) List->Head = NULL;
		--List->Length;
		return Node->Value;
	} else {
		return MLNil;
	}
}

ML_METHOD("+", MLListT, MLListT) {
	ml_list_t *List1 = (ml_list_t *)Args[0];
	ml_list_t *List2 = (ml_list_t *)Args[1];
	ml_list_t *List = new(ml_list_t);
	List->Type = MLListT;
	ml_list_node_t **Slot = &List->Head;
	ml_list_node_t *Prev = NULL;
	for (ml_list_node_t *Node1 = List1->Head; Node1; Node1 = Node1->Next) {
		ml_list_node_t *Node = Slot[0] = new(ml_list_node_t);
		Node->Value = Node1->Value;
		Node->Prev = Prev;
		Prev = Node;
		Slot = &Node->Next;
	}
	for (ml_list_node_t *Node2 = List2->Head; Node2; Node2 = Node2->Next) {
		ml_list_node_t *Node = Slot[0] = new(ml_list_node_t);
		Node->Value = Node2->Value;
		Node->Prev = Prev;
		Prev = Node;
		Slot = &Node->Next;
	}
	List->Tail = Prev;
	List->Length = List1->Length + List2->Length;
	return (ml_value_t *)List;
}

ML_METHOD("string", MLListT) {
	ml_list_t *List = (ml_list_t *)Args[0];
	if (!List->Length) return ml_string("[]", 2);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Seperator = "[";
	int SeperatorLength = 1;
	for (ml_list_node_t *Node = List->Head; Node; Node = Node->Next) {
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
	ml_list_node_t *Node = List->Head;
	if (Node) {
		ml_value_t *Result = ml_stringbuffer_append(Buffer, Node->Value);
		if (Result->Type == MLErrorT) return Result;
		while ((Node = Node->Next)) {
			ml_stringbuffer_add(Buffer, Seperator, SeperatorLength);
			ml_value_t *Result = ml_stringbuffer_append(Buffer, Node->Value);
			if (Result->Type == MLErrorT) return Result;
		}
	}
	return ml_stringbuffer_get_string(Buffer);
}

#define ML_TREE_MAX_DEPTH 32

typedef struct ml_map_iter_t {
	const ml_type_t *Type;
	ml_map_node_t *Node;
	ml_map_node_t *Stack[ML_TREE_MAX_DEPTH];
	int Top;
} ml_map_iter_t;

static ml_value_t *ml_map_iter_current(ml_state_t *Caller, ml_map_iter_t *Iter) {
	ML_CONTINUE(Caller, ml_reference(&Iter->Node->Value));
}

static ml_value_t *ml_map_iter_next(ml_state_t *Caller, ml_map_iter_t *Iter) {
	ml_map_node_t *Node = Iter->Node;
	if ((Iter->Node = Node->Next)) ML_CONTINUE(Caller, Iter);
	ML_CONTINUE(Caller, MLNil);
}

static ml_value_t *ml_map_iter_key(ml_state_t *Caller, ml_map_iter_t *Iter) {
	ML_CONTINUE(Caller, Iter->Node->Key);
}

ml_type_t MLMapIterT[1] = {{
	MLTypeT,
	MLAnyT, "map-iterator",
	ml_default_hash,
	ml_default_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

static ml_value_t *ml_map_iterate(ml_state_t *Caller, ml_value_t *Value) {
	ml_map_t *Map = (ml_map_t *)Value;
	if (Map->Root) {
		ml_map_iter_t *Iter = new(ml_map_iter_t);
		Iter->Type = MLMapIterT;
		Iter->Node = Map->Head;
		Iter->Top = 0;
		ML_CONTINUE(Caller, Iter);
	} else {
		ML_CONTINUE(Caller, MLNil);
	}
}

static int ml_map_add_insert(ml_value_t *Key, ml_value_t *Value, ml_value_t *Map) {
	ml_map_insert(Map, Key, Value);
	return 0;
}

ML_METHOD("+", MLMapT, MLMapT) {
	ml_map_t *Map = new(ml_map_t);
	Map->Type = MLMapT;
	ml_map_foreach(Args[0], Map, (void *)ml_map_add_insert);
	ml_map_foreach(Args[1], Map, (void *)ml_map_add_insert);
	return (ml_value_t *)Map;
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

ML_METHOD("string", MLMapT){
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

ML_METHOD("#", MLAnyT) {
	ml_value_t *Value = Args[0];
	return ml_integer(Value->Type->hash(Value, NULL));
}

static ml_value_t *ml_return_nil(void *Data, int Count, ml_value_t **Args) {
	return MLNil;
}

ML_METHOD("=", MLAnyT, MLAnyT) {
	return (Args[0] == Args[1]) ? Args[1] : MLNil;
}

ML_METHOD("!=", MLAnyT, MLAnyT) {
	return (Args[0] != Args[1]) ? Args[1] : MLNil;
}

ML_METHOD("integer", MLStringT) {
	return ml_integer(strtol(ml_string_value(Args[0]), 0, 10));
}

ML_METHOD("integer", MLStringT, MLIntegerT) {
	return ml_integer(strtol(ml_string_value(Args[0]), 0, ml_integer_value(Args[1])));
}

ML_METHOD("real", MLStringT) {
	return ml_real(strtod(ml_string_value(Args[0]), 0));
}

ML_METHOD("!!", MLClosureT, MLListT) {
	ml_closure_t *Closure = (ml_closure_t *)Args[0];
	ml_list_t *ArgsList = (ml_list_t *)Args[1];
	int NumUpValues = Closure->Info->NumUpValues + Closure->PartialCount;
	ml_closure_t *Partial = xnew(ml_closure_t, NumUpValues + ArgsList->Length, ml_value_t *);
	memcpy(Partial, Closure, sizeof(ml_closure_t) + NumUpValues * sizeof(ml_value_t *));
	Partial->PartialCount += ArgsList->Length;
	ml_value_t **Arg = Partial->UpValues + NumUpValues;
	for (ml_list_node_t *Node = ArgsList->Head; Node; Node = Node->Next) *Arg++ = Node->Value;
	return (ml_value_t *)Partial;
}

ml_type_t MLNamesT[1] = {{
	MLTypeT,
	MLListT, "names",
	ml_default_hash,
	(void *)ml_map_call,
	ml_default_deref,
	ml_default_assign,
	NULL, 0, 0
}};

void ml_init() {
	CompareMethod = ml_method("<>");
	AppendMethod = ml_method("append");
	IndexMethod = ml_method("[]");
	SymbolMethod = ml_method("::");
	IterateMethod = ml_method("iterate");
	NextMethod = ml_method("next");
	KeyMethod = ml_method("key");
	ValueMethod = ml_method("value");
#include "ml_types_init.c"
	//ml_method_by_name("#", NULL, ml_hash_any, MLAnyT, NULL);
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

	ml_method_by_name("string", NULL, ml_identity, MLStringT, NULL);

	ml_typed_fn_set(MLNilT, ml_stringbuffer_append, ml_stringbuffer_append_nil);
	ml_typed_fn_set(MLSomeT, ml_stringbuffer_append, ml_stringbuffer_append_some);
	ml_typed_fn_set(MLIntegerT, ml_stringbuffer_append, ml_stringbuffer_append_integer);
	ml_typed_fn_set(MLRealT, ml_stringbuffer_append, ml_stringbuffer_append_real);
	ml_typed_fn_set(MLStringT, ml_stringbuffer_append, ml_stringbuffer_append_string);
	ml_typed_fn_set(MLMethodT, ml_stringbuffer_append, ml_stringbuffer_append_method);
	ml_typed_fn_set(MLTupleT, ml_stringbuffer_append, ml_stringbuffer_append_tuple);
	ml_typed_fn_set(MLListT, ml_stringbuffer_append, ml_stringbuffer_append_list);
	ml_typed_fn_set(MLMapT, ml_stringbuffer_append, ml_stringbuffer_append_map);

	ml_typed_fn_set(MLFunctionT, ml_iterate, ml_function_iterate);
	ml_typed_fn_set(MLPartialFunctionT, ml_iterate, ml_partial_function_iterate);

	ml_typed_fn_set(MLChainedFunctionT, ml_iterate, ml_chained_function_iterate);
	ml_typed_fn_set(MLChainedIteratorT, ml_iter_next, ml_chained_iter_next);
	ml_typed_fn_set(MLChainedIteratorT, ml_iter_key, ml_chained_iter_key);
	ml_typed_fn_set(MLChainedIteratorT, ml_iter_value, ml_chained_iter_value);

	ml_typed_fn_set(MLListT, ml_iterate, ml_list_iterate);
	ml_typed_fn_set(MLListIterT, ml_iter_value, ml_list_iter_current);
	ml_typed_fn_set(MLListIterT, ml_iter_key, ml_list_iter_key);
	ml_typed_fn_set(MLListIterT, ml_iter_next, ml_list_iter_next);
	ml_typed_fn_set(MLMapT, ml_iterate, ml_map_iterate);
	ml_typed_fn_set(MLMapIterT, ml_iter_value, ml_map_iter_current);
	ml_typed_fn_set(MLMapIterT, ml_iter_key, ml_map_iter_key);
	ml_typed_fn_set(MLMapIterT, ml_iter_next, ml_map_iter_next);
	ml_typed_fn_set(MLPropertyT, ml_iterate, ml_property_iterate);
	ml_typed_fn_set(MLIntegerRangeT, ml_iterate, ml_integer_range_iterate);
	ml_typed_fn_set(MLIntegerIterT, ml_iter_value, ml_integer_iter_current);
	ml_typed_fn_set(MLIntegerIterT, ml_iter_key, ml_integer_iter_key);
	ml_typed_fn_set(MLIntegerIterT, ml_iter_next, ml_integer_iter_next);
	ml_typed_fn_set(MLRealRangeT, ml_iterate, ml_real_range_iterate);
	ml_typed_fn_set(MLRealIterT, ml_iter_value, ml_real_iter_current);
	ml_typed_fn_set(MLRealIterT, ml_iter_key, ml_real_iter_key);
	ml_typed_fn_set(MLRealIterT, ml_iter_next, ml_real_iter_next);
	ml_runtime_init();

	GC_word StringBufferLayout[] = {1};
	StringBufferDesc = GC_make_descriptor(StringBufferLayout, 1);
}

ml_type_t *ml_type(ml_type_t *Parent, const char *Name) {
	ml_type_t *Type = new(ml_type_t);
	Type->Type = MLTypeT;
	Type->Parent = Parent;
	Type->Name = Name;
	Type->hash = Parent->hash;
	Type->call = Parent->call;
	Type->deref = Parent->deref;
	Type->assign = Parent->assign;
	return Type;
}

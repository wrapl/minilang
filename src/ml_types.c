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
#include <float.h>
#include <inttypes.h>

#include "ml_compiler2.h"
#include "ml_runtime.h"
#include "ml_string.h"
#include "ml_method.h"
#include "ml_list.h"
#include "ml_map.h"

#ifdef ML_TRE
#include <tre/regex.h>
#else
#include <regex.h>
#endif

#ifdef ML_COMPLEX
#include <complex.h>
#undef I
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "type"

ML_METHOD_DECL(IterateMethod, "iterate");
ML_METHOD_DECL(ValueMethod, "value");
ML_METHOD_DECL(KeyMethod, "key");
ML_METHOD_DECL(NextMethod, "next");
ML_METHOD_DECL(CompareMethod, "<>");
ML_METHOD_DECL(IndexMethod, "[]");
ML_METHOD_DECL(SymbolMethod, "::");
ML_METHOD_DECL(LessMethod, "<");
ML_METHOD_DECL(CallMethod, "()");

static inline uintptr_t rotl(uintptr_t X, unsigned int N) {
	const unsigned int Mask = (CHAR_BIT * sizeof(uintptr_t) - 1);
	return (X << (N & Mask)) | (X >> ((-N) & Mask ));
}

// Types //

ML_INTERFACE(MLAnyT, (), "any", .Rank = 0);
// Base type for all values.

ML_INTERFACE(MLSequenceT, (), "sequence");
//!sequence
// The base type for any sequence value.

ML_INTERFACE(MLFunctionT, (), "function");
//!function
// The base type of all functions.

int ml_function_source(ml_value_t *Value, const char **Source, int *Line) {
	typeof(ml_function_source) *function = ml_typed_fn_get(ml_typeof(Value), ml_function_source);
	if (function) return function(Value, Source, Line);
	return 0;
}

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
	return ml_call(Caller, Type->Constructor, Count, Args);
}

ML_TYPE(MLTypeT, (MLFunctionT), "type",
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

static int ml_type_exports_fn(const char *Name, void *Value, ml_value_t *Exports) {
	ml_map_insert(Exports, ml_cstring(Name), Value);
	return 0;
}

ML_METHOD("exports", MLTypeT) {
//<Type
//>map
// Returns a map of all the exports from :mini:`Type`.
	ml_type_t *Type = (ml_type_t *)Args[0];
	ml_value_t *Exports = ml_map();
	stringmap_foreach(Type->Exports, Exports, (void *)ml_type_exports_fn);
	return Exports;
}

#ifdef ML_GENERICS

ML_TYPE(MLGenericTypeT, (MLTypeT), "generic-type");
//!internal

struct ml_generic_rule_t {
	ml_generic_rule_t *Next;
	int NumArgs;
	ml_type_t *Type;
	uintptr_t Args[];
};

static void ml_generic_fill(ml_generic_rule_t *Rule, ml_type_t **Args2, int NumArgs, ml_type_t **Args) {
	Args2[0] = Rule->Type;
	for (int I = 1; I < Rule->NumArgs; ++I) {
		uintptr_t Arg = Rule->Args[I - 1];
		if (Arg >> 48) {
			unsigned int J = Arg & 0xFFFF;
			Args2[I] = (J < NumArgs) ? Args[J] : MLAnyT;
		} else {
			Args2[I] = (ml_type_t *)Arg;
		}
	}
}

static void ml_generic_parents(ml_value_t *Parents, int NumArgs, ml_type_t **Args) {
	ml_type_t *V = Args[0];
	if (Args[0] == MLTupleT) {
		for (int NumArgs2 = NumArgs; --NumArgs2 > 0;) {
			ml_type_t *Parent = ml_generic_type(NumArgs2, Args);
			ml_list_put(Parents, (ml_value_t *)Parent);
		}
	}
	for (ml_generic_rule_t *Rule = V->Rules; Rule; Rule = Rule->Next) {
		int NumArgs2 = Rule->NumArgs;
		ml_type_t *Args2[NumArgs2];
		ml_generic_fill(Rule, Args2, NumArgs, Args);
		ml_type_t *Parent = ml_generic_type(NumArgs2, Args2);
		ml_list_put(Parents, (ml_value_t *)Parent);
		ml_generic_parents(Parents, NumArgs2, Args2);
	}
}

ML_METHOD("parents", MLGenericTypeT) {
//!internal
	ml_generic_type_t *Type = (ml_generic_type_t *)Args[0];
	ml_value_t *Parents = ml_list();
	ml_generic_parents(Parents, Type->NumArgs, Type->Args);
	return Parents;
}

#endif

ML_METHOD("parents", MLTypeT) {
//!type
//<Type
//>list
	ml_type_t *Type = (ml_type_t *)Args[0];
	ml_value_t *Parents = ml_list();
#ifdef ML_GENERICS
	ml_generic_parents(Parents, 1, &Type);
#endif
	for (int I = 0; I < Type->Parents->Size; ++I) {
		ml_type_t *Parent = (ml_type_t *)Type->Parents->Keys[I];
		if (Parent) ml_list_put(Parents, (ml_value_t *)Parent);
	}
	return Parents;
}

void ml_default_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	//ML_ERROR("TypeError", "<%s> is not callable", ml_typeof(Value)->Name);
	ml_value_t **Args2 = ml_alloc_args(Count + 1);
	Args2[0] = Value;
	for (int I = 0; I < Count; ++I) Args2[I + 1] = Args[I];
	return ml_call(Caller, CallMethod, Count + 1, Args2);
}

long ml_default_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	long Hash = 5381;
	for (const char *P = ml_typeof(Value)->Name; P[0]; ++P) Hash = ((Hash << 5) + Hash) + P[0];
	return Hash;
}

/*ml_value_t *ml_default_deref(ml_value_t *Ref) {
	return Ref;
}*/

void ml_default_assign(ml_state_t *Caller, ml_value_t *Ref, ml_value_t *Value) {
	ML_ERROR("TypeError", "<%s> is not assignable", ml_typeof(Ref)->Name);
}

void ml_type_init(ml_type_t *Type, ...) {
	int Rank = 0;
	va_list Args;
	va_start(Args, Type);
	ml_type_t *Parent;
	while ((Parent = va_arg(Args, ml_type_t *))) {
		if (Parent->Rank > Rank) Rank = Parent->Rank;
		ml_type_add_parent(Type, Parent);
	}
	va_end(Args);
	if (Type != MLAnyT) Type->Rank = Rank + 1;
	stringmap_insert(Type->Exports, "of", Type->Constructor);
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
	Type->Constructor = ml_method(NULL);
	return Type;
}

const char *ml_type_name(const ml_value_t *Value) {
	return ((ml_type_t *)Value)->Name;
}

void ml_type_add_parent(ml_type_t *Type, ml_type_t *Parent) {
	inthash_insert(Type->Parents, (uintptr_t)Parent, Parent);
	for (int I = 0; I < Parent->Parents->Size; ++I) {
		ml_type_t *Parent2 = (ml_type_t *)Parent->Parents->Keys[I];
		if (Parent2) ml_type_add_parent(Type, Parent2);
	}
}

#ifdef ML_THREADSAFE

#include <stdatomic.h>

static volatile atomic_flag MLTypedFnLock[1] = {ATOMIC_FLAG_INIT};

#define ML_TYPED_FN_LOCK() while (atomic_flag_test_and_set(MLTypedFnLock))

#define ML_TYPED_FN_UNLOCK() atomic_flag_clear(MLTypedFnLock)

#else

#define ML_TYPED_FN_LOCK() {}
#define ML_TYPED_FN_UNLOCK() {}

#endif

static void *__attribute__ ((noinline)) ml_typed_fn_get_parent(ml_type_t *Type, void *TypedFn) {
	void *BestFn = NULL;
	int BestRank = 0;
	for (int I = 0; I < Type->Parents->Size; ++I) {
		ml_type_t *Parent = (ml_type_t *)Type->Parents->Keys[I];
		if (Parent && (Parent->Rank > BestRank)) {
			void *Fn = ml_typed_fn_get(Parent, TypedFn);
			if (Fn) {
				BestFn = Fn;
				BestRank = Parent->Rank;
			}
		}
	}
	ML_TYPED_FN_LOCK();
	inthash_insert(Type->TypedFns, (uintptr_t)TypedFn, BestFn);
	ML_TYPED_FN_UNLOCK();
	return BestFn;
}

inline void *ml_typed_fn_get(ml_type_t *Type, void *TypedFn) {
#ifdef ML_GENERICS
	while (Type->Type == MLGenericTypeT) Type = ml_generic_type_args(Type)[0];
#endif
	ML_TYPED_FN_LOCK();
	inthash_result_t Result = inthash_search2_inline(Type->TypedFns, (uintptr_t)TypedFn);
	ML_TYPED_FN_UNLOCK();
	if (Result.Present) return Result.Value;
	return ml_typed_fn_get_parent(Type, TypedFn);
}

void ml_typed_fn_set(ml_type_t *Type, void *TypedFn, void *Function) {
	inthash_insert(Type->TypedFns, (uintptr_t)TypedFn, Function);
}

ML_METHOD("|", MLTypeT, MLTypeT) {
//<Type/1
//<Type/2
//>type
// Returns a union interface of :mini:`Type/1` and :mini:`Type/2`.
	ml_type_t *Type1 = (ml_type_t *)Args[0];
	ml_type_t *Type2 = (ml_type_t *)Args[1];
	ml_type_t *Type = new(ml_type_t);
	Type->Type = MLTypeT;
	ml_type_init(Type, Type1, Type2, NULL);
	asprintf((char **)&Type->Name, "%s | %s", Type1->Name, Type2->Name);
	Type->hash = ml_default_hash;
	Type->call = ml_default_call;
	Type->deref = ml_default_deref;
	Type->assign = ml_default_assign;
	return (ml_value_t *)Type;
}


ML_METHOD(MLStringT, MLTypeT) {
//!type
//<Type
//>string
// Returns a string representing :mini:`Type`.
	ml_type_t *Type = (ml_type_t *)Args[0];
	return ml_string_format("<<%s>>", Type->Name);
}

ML_METHOD("append", MLStringBufferT, MLTypeT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_type_t *Type = (ml_type_t *)Args[1];
	ml_stringbuffer_addf(Buffer, "<<%s>>", Type->Name);
	return MLSome;
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

#ifdef ML_GENERICS

#ifdef ML_THREADSAFE

static volatile atomic_flag MLGenericsLock[1] = {ATOMIC_FLAG_INIT};

#define ML_GENERICS_LOCK() while (atomic_flag_test_and_set(MLGenericsLock))

#define ML_GENERICS_UNLOCK() atomic_flag_clear(MLGenericsLock)

#else

#define ML_GENERICS_LOCK() {}
#define ML_GENERICS_UNLOCK() {}

#endif

ml_type_t *ml_generic_type(int NumArgs, ml_type_t *Args[]) {
	static inthash_t GenericTypeCache[1] = {INTHASH_INIT};
	uintptr_t Hash = (uintptr_t)3541;
	for (int I = NumArgs; --I >= 0;) Hash = rotl(Hash, 1) ^ (uintptr_t)Args[I];
	ML_GENERICS_LOCK();
	ml_generic_type_t *Type = (ml_generic_type_t *)inthash_search(GenericTypeCache, Hash);
	while (Type) {
		if (Type->NumArgs != NumArgs) goto next;
		for (int I = 0; I < NumArgs; ++I) {
			if (Args[I] != Type->Args[I]) goto next;
		}
		ML_GENERICS_UNLOCK();
		return (ml_type_t *)Type;
	next:
		Type = Type->NextGeneric;
	}
	Type = xnew(ml_generic_type_t, NumArgs, ml_type_t *);
	const ml_type_t *Base = Args[0];
	const char *Name = Base->Name;
	if (NumArgs > 1) {
		ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
		ml_stringbuffer_add(Buffer, Base->Name, strlen(Base->Name));
		ml_stringbuffer_add(Buffer, "[", 1);
		ml_stringbuffer_add(Buffer, Args[1]->Name, strlen(Args[1]->Name));
		for (int I = 2; I < NumArgs; ++I) {
			ml_stringbuffer_add(Buffer, ",", 1);
			ml_stringbuffer_add(Buffer, Args[I]->Name, strlen(Args[I]->Name));
		}
		ml_stringbuffer_add(Buffer, "]", 1);
		Name = ml_stringbuffer_get(Buffer);
	}
	Type->Base.Type = MLGenericTypeT;
	Type->Base.Name = Name;
	Type->Base.hash = Base->hash;
	Type->Base.call = Base->call;
	Type->Base.deref = Base->deref;
	Type->Base.assign = Base->assign;
	Type->Base.Rank = Base->Rank + 1;
	Type->NumArgs = NumArgs;
	for (int I = 0; I < NumArgs; ++I) Type->Args[I] = Args[I];
	Type->NextGeneric = (ml_generic_type_t *)inthash_insert(GenericTypeCache, Hash, Type);
	ML_GENERICS_UNLOCK();
	return (ml_type_t *)Type;
}

void ml_type_add_rule(ml_type_t *T, ml_type_t *U, ...) {
	int Count = 0;
	va_list Args;
	va_start(Args, U);
	while (va_arg(Args, uintptr_t)) ++Count;
	va_end(Args);
	ml_generic_rule_t *Rule = xnew(ml_generic_rule_t, Count, uintptr_t);
	Rule->Type = U;
	Rule->NumArgs = Count + 1;
	uintptr_t *RuleArgs = Rule->Args;
	va_start(Args, U);
	for (;;) {
		uintptr_t Arg = va_arg(Args, uintptr_t);
		if (!Arg) break;
		*RuleArgs++ = Arg;
	}
	va_end(Args);
	ml_generic_rule_t **Slot = &T->Rules;
	int Rank = U->Rank;
	while (Slot[0] && Slot[0]->Type->Rank > Rank) Slot = &Slot[0]->Next;
	Rule->Next = Slot[0];
	Slot[0] = Rule;
}

#endif

// Values //

ML_TYPE(MLNilT, (MLFunctionT, MLSequenceT), "nil");
//!internal

ML_TYPE(MLSomeT, (), "some");
//!internal

static void ml_blank_assign(ml_state_t *Caller, ml_value_t *Blank, ml_value_t *Value) {
	ML_RETURN(Value);
}

ML_TYPE(MLBlankT, (), "blank",
//!internal
	.assign = ml_blank_assign
);

ML_VALUE(MLNil, MLNilT);
ML_VALUE(MLSome, MLSomeT);
ML_VALUE(MLBlank, MLBlankT);

#ifdef ML_GENERICS

static int ml_is_generic_subtype(int TNumArgs, ml_type_t **TArgs, int UNumArgs, ml_type_t **UArgs) {
	if (TArgs[0] == UArgs[0]) {
		if (UNumArgs == 1) return 1;
		if (UNumArgs <= TNumArgs) {
			for (int I = 0; I < UNumArgs; ++I) {
				if (!ml_is_subtype(TArgs[I], UArgs[I])) goto different;
			}
			return 1;
		}
	}
different:
	/*if (TArgs[0] == MLTupleT && TNumArgs > 1) {

		if (ml_is_generic_subtype(TNumArgs - 1, TArgs, UNumArgs, UArgs)) return 1;
	}*/
	for (ml_generic_rule_t *Rule = TArgs[0]->Rules; Rule; Rule = Rule->Next) {
		int TNumArgs2 = Rule->NumArgs;
		ml_type_t *TArgs2[TNumArgs2];
		ml_generic_fill(Rule, TArgs2, TNumArgs, TArgs);
		if (ml_is_generic_subtype(TNumArgs2, TArgs2, UNumArgs, UArgs)) return 1;
	}
	return 0;
}

#endif

int ml_is_subtype(ml_type_t *T, ml_type_t *U) {
	if (T == U) return 1;
	if (U == MLAnyT) return 1;
#ifdef ML_GENERICS
	if (T->Type == MLGenericTypeT) {
		ml_generic_type_t *GenericT = (ml_generic_type_t *)T;
		if (U->Type == MLGenericTypeT) {
			ml_generic_type_t *GenericU = (ml_generic_type_t *)U;
			return ml_is_generic_subtype(GenericT->NumArgs, GenericT->Args, GenericU->NumArgs, GenericU->Args);
		} else {
			if (GenericT->Args[0] == U) return 1;
			return ml_is_generic_subtype(GenericT->NumArgs, GenericT->Args, 1, &U);
		}
	} else {
		if (U->Type == MLGenericTypeT) {
			ml_generic_type_t *GenericU = (ml_generic_type_t *)U;
			if (ml_is_generic_subtype(1, &T, GenericU->NumArgs, GenericU->Args)) return 1;
		} else {
			if (ml_is_generic_subtype(1, &T, 1, &U)) return 1;
		}
	}
#endif
	return (uintptr_t)inthash_search(T->Parents, (uintptr_t)U);
}

#ifdef ML_GENERICS

static ml_type_t *ml_generic_type_max(ml_type_t *Max, int TNumArgs, ml_type_t **TArgs, int UNumArgs, ml_type_t **UArgs) {
	if (TArgs[0] == UArgs[0]) {
		if (TNumArgs > UNumArgs) {
			ml_type_t *Args[TNumArgs];
			Args[0] = TArgs[0];
			for (int I = 1; I < UNumArgs; ++I) Args[I] = ml_type_max(TArgs[I], UArgs[I]);
			for (int I = UNumArgs; I < TNumArgs; ++I) Args[I] = MLAnyT;
			return ml_generic_type(TNumArgs, Args);
		} else {
			ml_type_t *Args[UNumArgs];
			Args[0] = UArgs[0];
			for (int I = 1; I < TNumArgs; ++I) Args[I] = ml_type_max(TArgs[I], UArgs[I]);
			for (int I = TNumArgs; I < UNumArgs; ++I) Args[I] = MLAnyT;
			return ml_generic_type(UNumArgs, Args);
		}
	}
	if (TArgs[0] == MLTupleT && TNumArgs > 1) {
		for (ml_generic_rule_t *URule = UArgs[0]->Rules; URule; URule = URule->Next) {
			if (URule->Type->Rank <= Max->Rank) return Max;
			int UNumArgs2 = URule->NumArgs;
			ml_type_t *UArgs2[UNumArgs2];
			ml_generic_fill(URule, UArgs2, UNumArgs, UArgs);
			Max = ml_generic_type_max(Max, TNumArgs - 1, TArgs, UNumArgs2, UArgs2);
		}
	}
	for (ml_generic_rule_t *TRule = TArgs[0]->Rules; TRule; TRule = TRule->Next) {
		if (TRule->Type->Rank <= Max->Rank) return Max;
		int TNumArgs2 = TRule->NumArgs;
		ml_type_t *TArgs2[TNumArgs2];
		ml_generic_fill(TRule, TArgs2, TNumArgs, TArgs);
		for (ml_generic_rule_t *URule = UArgs[0]->Rules; URule; URule = URule->Next) {
			if (URule->Type->Rank <= Max->Rank) return Max;
			int UNumArgs2 = URule->NumArgs;
			ml_type_t *UArgs2[UNumArgs2];
			ml_generic_fill(URule, UArgs2, UNumArgs, UArgs);
			Max = ml_generic_type_max(Max, TNumArgs2, TArgs2, UNumArgs2, UArgs2);
		}
	}
	return Max;
}

#endif

ml_type_t *ml_type_max(ml_type_t *T, ml_type_t *U) {
	ml_type_t *Max = MLAnyT;
	if (T->Rank < U->Rank) {
		if (inthash_search(U->Parents, (uintptr_t)T)) return T;
	} else {
		if (inthash_search(T->Parents, (uintptr_t)U)) return U;
	}
	for (int I = 0; I < T->Parents->Size; ++I) {
		ml_type_t *Parent = (ml_type_t *)T->Parents->Keys[I];
		if (Parent && Parent->Rank > Max->Rank) {
			if (inthash_search(U->Parents, (uintptr_t)Parent)) {
				Max = Parent;
			}
		}
	}
#ifdef ML_GENERICS
	if (T->Type == MLGenericTypeT) {
		ml_generic_type_t *GenericT = (ml_generic_type_t *)T;
		if (U->Type == MLGenericTypeT) {
			ml_generic_type_t *GenericU = (ml_generic_type_t *)U;
			Max = ml_generic_type_max(Max, GenericT->NumArgs, GenericT->Args, GenericU->NumArgs, GenericU->Args);
		} else {
			if (GenericT->Args[0] == U) return U;
			Max = ml_generic_type_max(Max, GenericT->NumArgs, GenericT->Args, 1, &U);
		}
	} else {
		if (U->Type == MLGenericTypeT) {
			ml_generic_type_t *GenericU = (ml_generic_type_t *)U;
			Max = ml_generic_type_max(Max, 1, &T, GenericU->NumArgs, GenericU->Args);
		} else {
			Max = ml_generic_type_max(Max, 1, &T, 1, &U);
		}
	}
#endif
	return Max;
}

ML_METHOD("*", MLTypeT, MLTypeT) {
//<Type/1
//<Type/2
//>type
// Returns the closest common parent type of :mini:`Type/1` and :mini:`Type/2`.
	return (ml_value_t *)ml_type_max((ml_type_t *)Args[0], (ml_type_t *)Args[1]);
}

ML_METHOD("<", MLTypeT, MLTypeT) {
//<Type/1
//<Type/2
//>type or nil
// Returns :mini:`Type/2` if :mini:`Type/2` is a strict parent of :mini:`Type/1`, otherwise returns :mini:`nil`.
	ml_type_t *Type1 = (ml_type_t *)Args[0];
	ml_type_t *Type2 = (ml_type_t *)Args[1];
	if (Type1 == Type2) return MLNil;
	if (ml_is_subtype(Type1, Type2)) return Args[1];
	return MLNil;
}

ML_METHOD("<=", MLTypeT, MLTypeT) {
//<Type/1
//<Type/2
//>type or nil
// Returns :mini:`Type/2` if :mini:`Type/2` is a parent of :mini:`Type/1`, otherwise returns :mini:`nil`.
	ml_type_t *Type1 = (ml_type_t *)Args[0];
	ml_type_t *Type2 = (ml_type_t *)Args[1];
	if (Type1 == Type2) return Args[1];
	if (ml_is_subtype(Type1, Type2)) return Args[1];
	return MLNil;
}

ML_METHOD(">", MLTypeT, MLTypeT) {
//<Type/1
//<Type/2
//>type or nil
// Returns :mini:`Type/2` if :mini:`Type/2` is a strict sub-type of :mini:`Type/1`, otherwise returns :mini:`nil`.
	ml_type_t *Type1 = (ml_type_t *)Args[0];
	ml_type_t *Type2 = (ml_type_t *)Args[1];
	if (Type1 == Type2) return MLNil;
	if (ml_is_subtype(Type2, Type1)) return Args[1];
	return MLNil;
}

ML_METHOD(">=", MLTypeT, MLTypeT) {
//<Type/1
//<Type/2
//>type or nil
// Returns :mini:`Type/2` if :mini:`Type/2` is a sub-type of :mini:`Type/1`, otherwise returns :mini:`nil`.
	ml_type_t *Type1 = (ml_type_t *)Args[0];
	ml_type_t *Type2 = (ml_type_t *)Args[1];
	if (Type1 == Type2) return Args[1];
	if (ml_is_subtype(Type2, Type1)) return Args[1];
	return MLNil;
}

#ifdef ML_GENERICS
ML_METHODVX("[]", MLTypeT, MLTypeT) {
//<Base
//<Type/1,...,Type/n
//>type
// Returns the generic type :mini:`Base[Type/1, ..., Type/n]`.
	for (int I = 2; I < Count; ++I) ML_CHECKX_ARG_TYPE(I, MLTypeT);
	ML_RETURN(ml_generic_type(Count, (ml_type_t **)Args));
}
#endif

static void ML_TYPED_FN(ml_iterate, MLNilT, ml_state_t *Caller, ml_value_t *Value) {
	ML_RETURN(Value);
}

ML_METHOD("in", MLAnyT, MLTypeT) {
//<Value
//<Type
//>Value | nil
// Returns :mini:`Value` if it is an instance of :mini:`Type` or a type that inherits from :mini:`Type` and :mini:`nil` otherwise.
	return ml_is(Args[0], (ml_type_t *)Args[1]) ? Args[0] : MLNil;
}

ML_METHOD_ANON(MLCompilerSwitch, "compiler::switch");

ML_METHODVX(MLCompilerSwitch, MLFunctionT) {
//!internal
	return ml_call(Caller, Args[0], Count - 1, Args + 1);
}

ML_METHODVX(MLCompilerSwitch, MLTypeT) {
//!internal
	ml_type_t *Type = (ml_type_t *)Args[0];
	ml_value_t *Switch = (ml_value_t *)stringmap_search(Type->Exports, "switch");
	if (!Switch) ML_ERROR("SwitchError", "%s does not support switch", Type->Name);
	return ml_call(Caller, Switch, Count - 1, Args + 1);
}

typedef struct {
	ml_value_t *Index;
	ml_type_t *Type;
} ml_type_case_t;

typedef struct {
	ml_type_t *Type;
	ml_type_case_t Cases[];
} ml_type_switch_t;

static void ml_type_switch(ml_state_t *Caller, ml_type_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_type_t *Type = ml_typeof(ml_deref(Args[0]));
	for (ml_type_case_t *Case = Switch->Cases;; ++Case) {
		if (ml_is_subtype(Type, Case->Type)) ML_RETURN(Case->Index);
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLTypeSwitchT, (MLFunctionT), "type-switch",
//!internal
	.call = (void *)ml_type_switch
);

ML_FUNCTION(MLTypeSwitch) {
//!internal
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_type_switch_t *Switch = xnew(ml_type_switch_t, Total, ml_type_case_t);
	Switch->Type = MLTypeSwitchT;
	ml_type_case_t *Case = Switch->Cases;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, MLTypeT)) {
				Case->Type = (ml_type_t *)Value;
			} else {
				return ml_error("ValueError", "Unsupported value in type case");
			}
			Case->Index = ml_integer(I);
			++Case;
		}
	}
	Case->Type = MLAnyT;
	Case->Index = ml_integer(Count);
	return (ml_value_t *)Switch;
}

long ml_hash_chain(ml_value_t *Value, ml_hash_chain_t *Chain) {
	//Value = ml_deref(Value);
	for (ml_hash_chain_t *Link = Chain; Link; Link = Link->Previous) {
		if (Link->Value == Value) return Link->Index;
	}
	ml_hash_chain_t NewChain[1] = {{Chain, Value, Chain ? Chain->Index + 1 : 1}};
	return ml_typeof(Value)->hash(Value, NewChain);
}

#ifdef ML_NANBOXING

#define NegOne ml_int32(-1)
#define One ml_int32(1)
#define Zero ml_int32(0)

#else

static ml_integer_t One[1] = {{MLIntegerT, 1}};
static ml_integer_t NegOne[1] = {{MLIntegerT, -1}};
static ml_integer_t Zero[1] = {{MLIntegerT, 0}};

#endif

static ml_value_t *ml_trace(void *Ptr, inthash_t *Cache) {
	void **Base = (void **)GC_base(Ptr);
	if (Base) {
		ml_value_t *Label = inthash_search(Cache, (uintptr_t)Base);
		if (Label) return Label;
		Label = ml_string_format("V%d", Cache->Size - Cache->Space);
		inthash_insert(Cache, (uintptr_t)Base, Label);
		ml_value_t *Trace = ml_list();
		size_t Size = (GC_size(Base) + sizeof(void *) - 1) / sizeof(void *);
		ml_list_put(Trace, Label);
		ml_list_put(Trace, ml_integer(Size));
		ml_value_t *Fields = ml_map();
		ml_list_put(Trace, Fields);
		for (int I = 0; I < Size; ++I) {
			ml_value_t *Field = ml_trace(Base[I], Cache);
			if (Field) ml_map_insert(Fields, ml_integer(I), Field);
		}
		return Trace;
	} else {
		return NULL;
	}
}

ML_METHOD("trace", MLAnyT) {
	ml_value_t *Value = Args[0];
	inthash_t Cache[1] = {INTHASH_INIT};
	return ml_trace(Value, Cache) ?: MLNil;
}

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
// Returns :mini:`Value/2` if :mini:`Value/1` and :mini:`Value/2` are exactly the same instance and :mini:`nil` otherwise.
	return (Args[0] == Args[1]) ? Args[1] : MLNil;
}

ML_METHOD("!=", MLAnyT, MLAnyT) {
//<Value/1
//<Value/2
//>Value/2 | nil
// Returns :mini:`Value/2` if :mini:`Value/1` and :mini:`Value/2` are not exactly the same instance and :mini:`nil` otherwise.
	return (Args[0] != Args[1]) ? Args[1] : MLNil;
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Comparison;
	ml_value_t **Args, **End;
	ml_value_t *Values[];
} ml_compare_state_t;

static void ml_compare_state_run(ml_compare_state_t *State, ml_value_t *Result) {
	if (ml_is_error(Result)) ML_CONTINUE(State->Base.Caller, Result);
	if (Result == MLNil) ML_CONTINUE(State->Base.Caller, Result);
	State->Args[0] = Result;
	if (++State->Args == State->End) {
		return ml_call(State->Base.Caller, State->Comparison, 2, State->Args - 1);
	} else {
		return ml_call((ml_state_t *)State, State->Comparison, 2, State->Args - 1);
	}
}

#define ml_comp_any_any_any(NAME) \
ML_METHODVX(NAME, MLAnyT, MLAnyT, MLAnyT) { \
/*>any|nil
// Returns :mini:`Arg/2` if :mini:`Arg/1 SYMBOL Arg/2` and :mini:`nil` otherwise.
*/\
	ml_compare_state_t *State = xnew(ml_compare_state_t, Count - 1, ml_value_t *); \
	State->Base.Caller = Caller; \
	State->Base.Context = Caller->Context; \
	State->Base.run = (ml_state_fn)ml_compare_state_run; \
	State->Comparison = ml_method(NAME); \
	for (int I = 2; I < Count; ++I) State->Values[I - 1] = Args[I]; \
	State->Args = State->Values; \
	State->End = State->Args + (Count - 2); \
	return ml_call((ml_state_t *)State, State->Comparison, 2, Args); \
}

ml_comp_any_any_any("=");
ml_comp_any_any_any("!=");
ml_comp_any_any_any("<");
ml_comp_any_any_any("<=");
ml_comp_any_any_any(">");
ml_comp_any_any_any(">=");

ML_METHOD(MLStringT, MLAnyT) {
//<Value
//>string
// Returns a general (type name only) representation of :mini:`Value` as a string.
	return ml_string_format("<%s>", ml_typeof(Args[0])->Name);
}

void ml_value_set_name(ml_value_t *Value, const char *Name) {
	typeof(ml_value_set_name) *function = ml_typed_fn_get(ml_typeof(Value), ml_value_set_name);
	if (function) function(Value, Name);
}

// Iterators //

void ml_iterate(ml_state_t *Caller, ml_value_t *Value) {
	typeof(ml_iterate) *function = ml_typed_fn_get(ml_typeof(Value), ml_iterate);
	if (!function) {
		ml_value_t **Args = ml_alloc_args(1);
		Args[0] = Value;
		return ml_call(Caller, IterateMethod, 1, Args);
	}
	return function(Caller, Value);
}

void ml_iter_value(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_value) *function = ml_typed_fn_get(ml_typeof(Iter), ml_iter_value);
	if (!function) {
		ml_value_t **Args = ml_alloc_args(1);
		Args[0] = Iter;
		return ml_call(Caller, ValueMethod, 1, Args);
	}
	return function(Caller, Iter);
}

void ml_iter_key(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_key) *function = ml_typed_fn_get(ml_typeof(Iter), ml_iter_key);
	if (!function) {
		ml_value_t **Args = ml_alloc_args(1);
		Args[0] = Iter;
		return ml_call(Caller, KeyMethod, 1, Args);
	}
	return function(Caller, Iter);
}

void ml_iter_next(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_next) *function = ml_typed_fn_get(ml_typeof(Iter), ml_iter_next);
	if (!function) {
		ml_value_t **Args = ml_alloc_args(1);
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
	ml_value_t *Function = Args[0];
	ml_value_t *List = Args[1];
	int Count2 = ml_list_length(List);
	ml_value_t **Args2 = ml_alloc_args(Count2);
	ml_list_to_array(List, Args2);
	return ml_call(Caller, Function, Count2, Args2);
}

ML_METHODX("!", MLFunctionT, MLMapT) {
//!function
//<Function
//<Map
//>any
// Calls :mini:`Function` with the keys and values in :mini:`Map` as named arguments.
// Returns an error if any of the keys in :mini:`Map` is not a string or method.
	ml_value_t *Function = Args[0];
	ml_value_t *Map = Args[1];
	int Count2 = ml_map_size(Map) + 1;
	ml_value_t **Args2 = ml_alloc_args(Count2);
	ml_value_t *Names = ml_names();
	ml_value_t **Arg = Args2;
	*(Arg++) = Names;
	ML_MAP_FOREACH(Map, Node) {
		ml_value_t *Name = Node->Key;
		if (ml_is(Name, MLMethodT)) {
			ml_names_add(Names, Name);
		} else if (ml_is(Name, MLStringT)) {
			ml_names_add(Names, ml_method(ml_string_value(Name)));
		} else {
			ML_ERROR("TypeError", "Parameter names must be strings or methods");
		}
		*(Arg++) = Node->Value;
	}
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
	ml_value_t *Function = Args[0];
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[1];
	ml_value_t *Map = Args[2];
	int TupleCount = Tuple->Size;
	int MapCount = ml_map_size(Map);
	int Count2 = TupleCount + MapCount + 1;
	ml_value_t **Args2 = ml_alloc_args(Count2);
	memcpy(Args2, Tuple->Values, TupleCount * sizeof(ml_value_t *));
	ml_value_t *Names = ml_names();
	ml_value_t **Arg = Args2 + TupleCount;
	*(Arg++) = Names;
	ML_MAP_FOREACH(Map, Node) {
		ml_value_t *Name = Node->Key;
		if (ml_is(Name, MLMethodT)) {
			ml_names_add(Names, Name);
		} else if (ml_is(Name, MLStringT)) {
			ml_names_add(Names, ml_method(ml_string_value(Name)));
		} else {
			ML_ERROR("TypeError", "Parameter names must be strings or methods");
		}
		*(Arg++) = Node->Value;
	}
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
	ml_value_t *Function = Args[0];
	int ListCount = ml_list_length(Args[1]);
	ml_value_t *Map = Args[2];
	int MapCount = ml_map_size(Map);
	int Count2 = ListCount + MapCount + 1;
	ml_value_t **Args2 = ml_alloc_args(Count2);
	ml_list_to_array(Args[1], Args2);
	ml_value_t *Names = ml_names();
	ml_value_t **Arg = Args2 + ListCount;
	*(Arg++) = Names;
	ML_MAP_FOREACH(Map, Node) {
		ml_value_t *Name = Node->Key;
		if (ml_is(Name, MLMethodT)) {
			ml_names_add(Names, Name);
		} else if (ml_is(Name, MLStringT)) {
			ml_names_add(Names, ml_method(ml_string_value(Name)));
		} else {
			ML_ERROR("TypeError", "Parameter names must be strings or methods");
		}
		*(Arg++) = Node->Value;
	}
	return ml_call(Caller, Function, Count2, Args2);
}

static __attribute__ ((noinline)) void ml_cfunction_call_deref(ml_state_t *Caller, ml_cfunction_t *Function, int Count, ml_value_t **Args, int I) {
	Args[I] = Args[I]->Type->deref(Args[I]);
	while (--I >= 0) Args[I] = ml_deref(Args[I]);
	ML_RETURN((Function->Callback)(Function->Data, Count, Args));
}

static void ml_cfunction_call(ml_state_t *Caller, ml_cfunction_t *Function, int Count, ml_value_t **Args) {
	for (int I = Count; --I >= 0;) {
#ifdef ML_NANBOXING
		if (!ml_tag(Args[I]) && Args[I]->Type->deref != ml_default_deref) {
#else
		if (Args[I]->Type->deref != ml_default_deref) {
#endif
			return ml_cfunction_call_deref(Caller, Function, Count, Args, I);
		}
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

ml_value_t *ml_cfunction2(void *Data, ml_callback_t Callback, const char *Source, int Line) {
	ml_cfunction_t *Function = new(ml_cfunction_t);
	Function->Type = MLCFunctionT;
	Function->Data = Data;
	Function->Callback = Callback;
	Function->Source = Source;
	Function->Line = Line;
	return (ml_value_t *)Function;
}

static int ML_TYPED_FN(ml_function_source, MLCFunctionT, ml_cfunction_t *Function, const char **Source, int *Line) {
	if (Function->Source) {
		*Source = Function->Source;
		*Line = Function->Line;
		return 1;
	} else {
		return 0;
	}
}

static void ML_TYPED_FN(ml_iterate, MLCFunctionT, ml_state_t *Caller, ml_cfunction_t *Function) {
	ML_RETURN((Function->Callback)(Function->Data, 0, NULL));
}

static __attribute__ ((noinline)) void ml_cfunctionx_call_deref(ml_state_t *Caller, ml_cfunctionx_t *Function, int Count, ml_value_t **Args, int I) {
	Args[I] = Args[I]->Type->deref(Args[I]);
	while (--I >= 0) Args[I] = ml_deref(Args[I]);
	return (Function->Callback)(Caller, Function->Data, Count, Args);
}

static void ml_cfunctionx_call(ml_state_t *Caller, ml_cfunctionx_t *Function, int Count, ml_value_t **Args) {
	for (int I = Count; --I >= 0;) {
#ifdef ML_NANBOXING
		if (!ml_tag(Args[I]) && Args[I]->Type->deref != ml_default_deref) {
#else
		if (Args[I]->Type->deref != ml_default_deref) {
#endif
			return ml_cfunctionx_call_deref(Caller, Function, Count, Args, I);
		}
	}
	//for (int I = 0; I < Count; ++I) Args[I] = ml_deref(Args[I]);
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

ml_value_t *ml_cfunctionx2(void *Data, ml_callbackx_t Callback, const char *Source, int Line) {
	ml_cfunctionx_t *Function = new(ml_cfunctionx_t);
	Function->Type = MLCFunctionXT;
	Function->Data = Data;
	Function->Callback = Callback;
	Function->Source = Source;
	Function->Line = Line;
	return (ml_value_t *)Function;
}

static int ML_TYPED_FN(ml_function_source, MLCFunctionXT, ml_cfunctionx_t *Function, const char **Source, int *Line) {
	if (Function->Source) {
		*Source = Function->Source;
		*Line = Function->Line;
		return 1;
	} else {
		return 0;
	}
}

static void ml_cfunctionz_call(ml_state_t *Caller, ml_cfunctionx_t *Function, int Count, ml_value_t **Args) {
	return (Function->Callback)(Caller, Function->Data, Count, Args);
}

ML_TYPE(MLCFunctionZT, (MLFunctionT), "c-functionx",
//!internal
	.call = (void *)ml_cfunctionz_call
);

ml_value_t *ml_cfunctionz(void *Data, ml_callbackx_t Callback) {
	ml_cfunctionx_t *Function = new(ml_cfunctionx_t);
	Function->Type = MLCFunctionZT;
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

static void __attribute__ ((noinline)) ml_partial_function_copy_args(ml_partial_function_t *Partial, int CombinedCount, ml_value_t **CombinedArgs, int Count, ml_value_t **Args) {
	ml_value_t *Copy[Count];
	memcpy(Copy, Args, Count * sizeof(ml_value_t *));
	int I = 0, J = 0;
	for (; I < Partial->Count; ++I) {
		CombinedArgs[I] = Partial->Args[I] ?: (J < Count) ? Copy[J++] : MLNil;
	}
	for (; I < CombinedCount; ++I) {
		CombinedArgs[I] = (J < Count) ? Copy[J++] : MLNil;
	}
}

static void ml_partial_function_call(ml_state_t *Caller, ml_partial_function_t *Partial, int Count, ml_value_t **Args) {
	int CombinedCount = Count + Partial->Set;
	if (CombinedCount < Partial->Count) CombinedCount = Partial->Count;
	ml_value_t **CombinedArgs = ml_alloc_args(CombinedCount);
	if (CombinedArgs == Args) {
		ml_partial_function_copy_args(Partial, CombinedCount, CombinedArgs, Count, Args);
	} else {
		int I = 0, J = 0;
		for (; I < Partial->Count; ++I) {
			CombinedArgs[I] = Partial->Args[I] ?: (J < Count) ? Args[J++] : MLNil;
		}
		for (; I < CombinedCount; ++I) {
			CombinedArgs[I] = (J < Count) ? Args[J++] : MLNil;
		}
	}
	return ml_call(Caller, Partial->Function, CombinedCount, CombinedArgs);
}

ML_TYPE(MLPartialFunctionT, (MLFunctionT, MLSequenceT), "partial-function",
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

ML_METHOD("count", MLPartialFunctionT) {
//!function
	ml_partial_function_t *Partial = (ml_partial_function_t *)Args[0];
	return ml_integer(Partial->Count);
}

ML_METHOD("set", MLPartialFunctionT) {
//!function
	ml_partial_function_t *Partial = (ml_partial_function_t *)Args[0];
	return ml_integer(Partial->Set);
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

ML_METHODV("$", MLFunctionT, MLAnyT) {
//!function
//<Function
//<Values...
//>partialfunction
// Returns a function equivalent to :mini:`fun(Args...) Function(Values..., Args...)`.
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, Count - 1, ml_value_t *);
	Partial->Type = MLPartialFunctionT;
	Partial->Function = Args[0];
	Partial->Count = Partial->Set = Count - 1;
	for (int I = 1; I < Count; ++I) Partial->Args[I - 1] = Args[I];
	return (ml_value_t *)Partial;
}

static void ML_TYPED_FN(ml_iterate, MLPartialFunctionT, ml_state_t *Caller, ml_partial_function_t *Partial) {
	if (Partial->Set != Partial->Count) ML_ERROR("CallError", "Partial function used with missing arguments");
	return ml_call(Caller, Partial->Function, Partial->Count, Partial->Args);
}

// Tuples //
//!tuple

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

typedef struct {
	ml_state_t Base;
	ml_tuple_t *Ref;
	ml_value_t *Values;
	int Index;
} ml_tuple_assign_t;

static void ml_tuple_assign_run(ml_tuple_assign_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	ml_tuple_t *Ref = State->Ref;
	ml_value_t *Values = State->Values;
	int Index = State->Index;
	if (Index == Ref->Size) ML_RETURN(Values);
	State->Index = Index + 1;
	ml_value_t *Value = ml_deref(ml_unpack(Values, Index + 1));
	return ml_assign(State, Ref->Values[Index], Value);
}

static void ml_tuple_assign(ml_state_t *Caller, ml_tuple_t *Ref, ml_value_t *Values) {
	ml_tuple_assign_t *State = new(ml_tuple_assign_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_tuple_assign_run;
	State->Ref = Ref;
	State->Values = Values;
	State->Index = 1;
	ml_value_t *Value = ml_deref(ml_unpack(Values, 1));
	return ml_assign(State, Ref->Values[0], Value);
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

ML_TYPE(MLTupleT, (MLSequenceT), "tuple",
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

#ifdef ML_GENERICS

ml_value_t *ml_tuple_set(ml_value_t *Tuple0, int Index, ml_value_t *Value) {
	ml_tuple_t *Tuple = (ml_tuple_t *)Tuple0;
	Tuple->Values[Index - 1] = Value;
	if (Tuple->Type == MLTupleT) {
		for (int I = 0; I < Tuple->Size; ++I) if (!Tuple->Values[I]) return Value;
		ml_type_t *Types[Tuple->Size + 1];
		Types[0] = MLTupleT;
		for (int I = 0; I < Tuple->Size; ++I) Types[I + 1] = ml_typeof(Tuple->Values[I]);
		Tuple->Type = ml_generic_type(Tuple->Size + 1, Types);
	}
	return Value;
}

#endif

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
	long Index = ml_integer_value_fast(Args[1]);
	if (--Index < 0) Index += Tuple->Size + 1;
	if (Index < 0 || Index >= Tuple->Size) return ml_error("RangeError", "Tuple index out of bounds");
	return Tuple->Values[Index];
}

typedef struct {
	ml_type_t *Type;
	ml_value_t **Values;
	int Size, Index;
} ml_tuple_iter_t;

ML_TYPE(MLTupleIterT, (), "tuple-iter");

static void ML_TYPED_FN(ml_iter_next, MLTupleIterT, ml_state_t *Caller, ml_tuple_iter_t *Iter) {
	if (Iter->Index == Iter->Size) ML_RETURN(MLNil);
	++Iter->Index;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLTupleIterT, ml_state_t *Caller, ml_tuple_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

static void ML_TYPED_FN(ml_iter_value, MLTupleIterT, ml_state_t *Caller, ml_tuple_iter_t *Iter) {
	ML_RETURN(Iter->Values[Iter->Index - 1]);
}

static void ML_TYPED_FN(ml_iterate, MLTupleT, ml_state_t *Caller, ml_tuple_t *Tuple) {
	if (!Tuple->Size) ML_RETURN(MLNil);
	ml_tuple_iter_t *Iter = new(ml_tuple_iter_t);
	Iter->Type = MLTupleIterT;
	Iter->Size = Tuple->Size;
	Iter->Index = 1;
	Iter->Values = Tuple->Values;
	ML_RETURN(Iter);
}

ML_METHOD(MLStringT, MLTupleT) {
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

ML_METHOD("append", MLStringBufferT, MLTupleT) {
//!tuple
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
/*>tuple|nil
// Returns :mini:`Arg/2` if :mini:`Arg/1 SYMBOL Arg/2` and :mini:`nil` otherwise.
*/\
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

// Boolean //
//!boolean

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

ML_METHOD(MLBooleanT, MLStringT) {
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

ML_METHODV("/\\", MLBooleanT, MLBooleanT) {
//!boolean
//<Bool/1
//<Bool/2
//>boolean
// Returns the logical and of :mini:`Bool/1` and :mini:`Bool/2`.
	int Result = ml_boolean_value(Args[0]);
	for (int I = 1; I < Count; ++I) Result &= ml_boolean_value(Args[I]);
	return MLBooleans[Result];
}

ML_METHODV("\\/", MLBooleanT, MLBooleanT) {
//!boolean
//<Bool/1
//<Bool/2
//>boolean
// Returns the logical or of :mini:`Bool/1` and :mini:`Bool/2`.
	int Result = ml_boolean_value(Args[0]);
	for (int I = 1; I < Count; ++I) Result |= ml_boolean_value(Args[I]);
	return MLBooleans[Result];
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
/*>boolean|nil
// Returns :mini:`Arg/2` if :mini:`Arg/1 SYMBOL Arg/2` and :mini:`nil` otherwise.
*/\
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

// Numbers //
//!number

ML_TYPE(MLNumberT, (), "number");
//!number
// Base type for numbers.

ML_TYPE(MLRealT, (MLNumberT), "real");
//!number
// Base type for real numbers.

#ifdef ML_NANBOXING

static long ml_int32_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return (int32_t)(intptr_t)Value;
}

static void ml_int32_call(ml_state_t *Caller, ml_value_t *Value, int Count, ml_value_t **Args) {
	long Index = (int32_t)(intptr_t)Value;
	if (Index <= 0) Index += Count + 1;
	if (Index <= 0) ML_RETURN(MLNil);
	if (Index > Count) ML_RETURN(MLNil);
	ML_RETURN(Args[Index - 1]);
}

ML_TYPE(MLIntegerT, (MLRealT, MLFunctionT), "integer");
//!number

ML_TYPE(MLInt32T, (MLIntegerT), "int32",
//!internal
	.hash = (void *)ml_int32_hash,
	.call = (void *)ml_int32_call,
	.NoInherit = 1
);

static long ml_int64_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return ((ml_int64_t *)Value)->Value;
}

ML_TYPE(MLInt64T, (MLIntegerT), "int64",
//!internal
	.hash = (void *)ml_int64_hash,
	.NoInherit = 1
);

ml_value_t *ml_int64(int64_t Integer) {
	ml_int64_t *Value = new(ml_int64_t);
	Value->Type = MLInt64T;
	Value->Value = Integer;
	return (ml_value_t *)Value;
}

int64_t ml_integer_value(const ml_value_t *Value) {
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

ML_METHOD(MLRealT, MLInt32T) {
//!number
	return ml_real((int32_t)(intptr_t)Args[0]);
}

ML_METHOD(MLRealT, MLInt64T) {
//!number
	return ml_real(((ml_int64_t *)Args[0])->Value);
}

#else

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

ML_TYPE(MLIntegerT, (MLRealT, MLFunctionT), "integer",
//!number
	.hash = (void *)ml_integer_hash,
	.call = (void *)ml_integer_call
);

ml_value_t *ml_integer(int64_t Value) {
	ml_integer_t *Integer = new(ml_integer_t);
	Integer->Type = MLIntegerT;
	Integer->Value = Value;
	return (ml_value_t *)Integer;
}

extern int64_t ml_integer_value_fast(const ml_value_t *Value);

int64_t ml_integer_value(const ml_value_t *Value) {
	if (Value->Type == MLIntegerT) {
		return ((ml_integer_t *)Value)->Value;
	} else if (Value->Type == MLDoubleT) {
		return ((ml_double_t *)Value)->Value;
	} else if (ml_is(Value, MLIntegerT)) {
		return ((ml_integer_t *)Value)->Value;
	} else if (ml_is(Value, MLDoubleT)) {
		return ((ml_double_t *)Value)->Value;
	} else {
		return 0;
	}
}

ML_METHOD(MLRealT, MLIntegerT) {
//!number
	return ml_real(((ml_integer_t *)Args[0])->Value);
}

#endif

#ifdef ML_NANBOXING

ML_METHOD(MLIntegerT, MLDoubleT) {
//!number
	return ml_integer(ml_to_double(Args[0]));
}

static long ml_double_hash(ml_value_t *Value, ml_hash_chain_t *Chain) {
	return (long)ml_to_double(Value);
}

ML_TYPE(MLDoubleT, (MLRealT), "double",
//!internal
	.hash = (void *)ml_double_hash,
	.NoInherit = 1
);

double ml_real_value(const ml_value_t *Value) {
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

ML_METHOD(MLDoubleT, MLInt32T) {
//!number
	return ml_real((int32_t)(intptr_t)Args[0]);
}

ML_METHOD(MLDoubleT, MLInt64T) {
//!number
	return ml_real(((ml_int64_t *)Args[0])->Value);
}

#else

ML_METHOD(MLIntegerT, MLDoubleT) {
//!number
//<Real
//>integer
// Converts :mini:`Real` to an integer (using default rounding).
	return ml_integer(((ml_double_t *)Args[0])->Value);
}

static long ml_double_hash(ml_double_t *Real, ml_hash_chain_t *Chain) {
	return (long)Real->Value;
}

ML_TYPE(MLDoubleT, (MLRealT), "real",
//!number
	.hash = (void *)ml_double_hash,
	.NoInherit = 1
);

ml_value_t *ml_real(double Value) {
	ml_double_t *Real = new(ml_double_t);
	Real->Type = MLDoubleT;
	Real->Value = Value;
	return (ml_value_t *)Real;
}

extern double ml_double_value_fast(const ml_value_t *Value);

double ml_real_value(const ml_value_t *Value) {
	if (Value->Type == MLIntegerT) {
		return ((ml_integer_t *)Value)->Value;
	} else if (Value->Type == MLDoubleT) {
		return ((ml_double_t *)Value)->Value;
	} else if (ml_is(Value, MLIntegerT)) {
		return ((ml_integer_t *)Value)->Value;
	} else if (ml_is(Value, MLDoubleT)) {
		return ((ml_double_t *)Value)->Value;
	} else {
		return 0;
	}
}

ML_METHOD(MLDoubleT, MLIntegerT) {
//!number
	return ml_real(((ml_integer_t *)Args[0])->Value);
}

#endif

#define ml_arith_method_integer(NAME, SYMBOL) \
ML_METHOD(NAME, MLIntegerT) { \
	int64_t IntegerA = ml_integer_value_fast(Args[0]); \
	return ml_integer(SYMBOL(IntegerA)); \
}

#define ml_arith_method_integer_integer(NAME, SYMBOL) \
ML_METHOD(NAME, MLIntegerT, MLIntegerT) { \
	int64_t IntegerA = ml_integer_value_fast(Args[0]); \
	int64_t IntegerB = ml_integer_value_fast(Args[1]); \
	return ml_integer(IntegerA SYMBOL IntegerB); \
}

#define ml_arith_method_real(NAME, SYMBOL) \
ML_METHOD(NAME, MLDoubleT) { \
	double RealA = ml_double_value_fast(Args[0]); \
	return ml_real(SYMBOL(RealA)); \
}

#define ml_arith_method_real_real(NAME, SYMBOL) \
ML_METHOD(NAME, MLDoubleT, MLDoubleT) { \
	double RealA = ml_double_value_fast(Args[0]); \
	double RealB = ml_double_value_fast(Args[1]); \
	return ml_real(RealA SYMBOL RealB); \
}

#define ml_arith_method_real_integer(NAME, SYMBOL) \
ML_METHOD(NAME, MLDoubleT, MLIntegerT) { \
	double RealA = ml_double_value_fast(Args[0]); \
	int64_t IntegerB = ml_integer_value_fast(Args[1]); \
	return ml_real(RealA SYMBOL IntegerB); \
}

#define ml_arith_method_integer_real(NAME, SYMBOL) \
ML_METHOD(NAME, MLIntegerT, MLDoubleT) { \
	int64_t IntegerA = ml_integer_value_fast(Args[0]); \
	double RealB = ml_double_value_fast(Args[1]); \
	return ml_real(IntegerA SYMBOL RealB); \
}

#ifdef ML_COMPLEX

static long ml_complex_hash(ml_complex_t *Complex, ml_hash_chain_t *Chain) {
	return (long)creal(Complex->Value);
}

ML_TYPE(MLComplexT, (MLNumberT), "complex",
//!number
	.hash = (void *)ml_complex_hash
);

ml_value_t *ml_complex(complex double Value) {
	ml_complex_t *Complex = new(ml_complex_t);
	Complex->Type = MLComplexT;
	Complex->Value = Value;
	return (ml_value_t *)Complex;
}

ML_METHOD(MLComplexT, MLRealT) {
//!number
	return ml_complex(ml_real_value(Args[0]));
}

ML_METHOD(MLRealT, MLComplexT) {
//!number
	return ml_real(creal(ml_complex_value(Args[0])));
}

extern complex double ml_complex_value_fast(const ml_value_t *Value);

complex double ml_complex_value(const ml_value_t *Value) {
#ifdef ML_NANBOXING
	int Tag = ml_tag(Value);
	if (Tag == 1) return (int32_t)(intptr_t)Value;
	if (Tag >= 7) return ml_to_double(Value);
	if (Tag == 0) {
		if (Value->Type == MLInt64T) {
			return ((ml_int64_t *)Value)->Value;
		} else if (Value->Type == MLComplexT) {
			return ((ml_complex_t *)Value)->Value;
		}
	}
	return 0;
#else
	if (Value->Type == MLIntegerT) {
		return ((ml_integer_t *)Value)->Value;
	} else if (Value->Type == MLDoubleT) {
		return ((ml_double_t *)Value)->Value;
	} else if (ml_is(Value, MLIntegerT)) {
		return ((ml_integer_t *)Value)->Value;
	} else if (ml_is(Value, MLDoubleT)) {
		return ((ml_double_t *)Value)->Value;
	} else if (ml_is(Value, MLComplexT)) {
		return ((ml_complex_t *)Value)->Value;
	} else {
		return 0;
	}
#endif
}

#define ml_arith_method_complex(NAME, SYMBOL) \
ML_METHOD(NAME, MLComplexT) { \
	complex double ComplexA = ml_complex_value_fast(Args[0]); \
	complex double ComplexB = SYMBOL(ComplexA); \
	if (fabs(cimag(ComplexB)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexB)); \
	} else { \
		return ml_complex(ComplexB); \
	} \
}

#define ml_arith_method_complex_complex(NAME, SYMBOL) \
ML_METHOD(NAME, MLComplexT, MLComplexT) { \
	complex double ComplexA = ml_complex_value_fast(Args[0]); \
	complex double ComplexB = ml_complex_value_fast(Args[1]); \
	complex double ComplexC = ComplexA SYMBOL ComplexB; \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

#define ml_arith_method_complex_integer(NAME, SYMBOL) \
ML_METHOD(NAME, MLComplexT, MLIntegerT) { \
	complex double ComplexA = ml_complex_value_fast(Args[0]); \
	int64_t IntegerB = ml_integer_value_fast(Args[1]); \
	complex double ComplexC = ComplexA SYMBOL IntegerB; \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

#define ml_arith_method_integer_complex(NAME, SYMBOL) \
ML_METHOD(NAME, MLIntegerT, MLComplexT) { \
	int64_t IntegerA = ml_integer_value_fast(Args[0]); \
	complex double ComplexB = ml_complex_value_fast(Args[1]); \
	complex double ComplexC = IntegerA SYMBOL ComplexB; \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

#define ml_arith_method_complex_real(NAME, SYMBOL) \
ML_METHOD(NAME, MLComplexT, MLDoubleT) { \
	complex double ComplexA = ml_complex_value_fast(Args[0]); \
	double RealB = ml_double_value_fast(Args[1]); \
	complex double ComplexC = ComplexA SYMBOL RealB; \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

#define ml_arith_method_real_complex(NAME, SYMBOL) \
ML_METHOD(NAME, MLDoubleT, MLComplexT) { \
	double RealA = ml_double_value_fast(Args[0]); \
	complex double ComplexB = ml_complex_value_fast(Args[1]); \
	complex double ComplexC = RealA SYMBOL ComplexB; \
	if (fabs(cimag(ComplexC)) <= DBL_EPSILON) { \
		return ml_real(creal(ComplexC)); \
	} else { \
		return ml_complex(ComplexC); \
	} \
}

ML_METHOD("r", MLComplexT) {
//!number
//<Z
//>real
// Returns the real component of :mini:`Z`.
	return ml_real(creal(ml_complex_value_fast(Args[0])));
}

ML_METHOD("i", MLComplexT) {
//!number
//<Z
//>real
// Returns the imaginary component of :mini:`Z`.
	return ml_real(cimag(ml_complex_value_fast(Args[0])));
}

#endif

#ifdef ML_COMPLEX

#define ml_arith_method_number(NAME, SYMBOL) \
ml_arith_method_integer(NAME, SYMBOL) \
ml_arith_method_real(NAME, SYMBOL) \
ml_arith_method_complex(NAME, SYMBOL)

#define ml_arith_method_number_number(NAME, SYMBOL) \
ml_arith_method_integer_integer(NAME, SYMBOL) \
ml_arith_method_real_real(NAME, SYMBOL) \
ml_arith_method_real_integer(NAME, SYMBOL) \
ml_arith_method_integer_real(NAME, SYMBOL) \
ml_arith_method_complex_complex(NAME, SYMBOL) \
ml_arith_method_complex_real(NAME, SYMBOL) \
ml_arith_method_complex_integer(NAME, SYMBOL) \
ml_arith_method_integer_complex(NAME, SYMBOL) \
ml_arith_method_real_complex(NAME, SYMBOL) \

#else

#define ml_arith_method_number(NAME, SYMBOL) \
ml_arith_method_integer(NAME, SYMBOL) \
ml_arith_method_real(NAME, SYMBOL)

#define ml_arith_method_number_number(NAME, SYMBOL) \
ml_arith_method_integer_integer(NAME, SYMBOL) \
ml_arith_method_real_real(NAME, SYMBOL) \
ml_arith_method_real_integer(NAME, SYMBOL) \
ml_arith_method_integer_real(NAME, SYMBOL)

#endif

ml_arith_method_number("-", -)
ml_arith_method_number_number("+", +)
ml_arith_method_number_number("-", -)
ml_arith_method_number_number("*", *)
ml_arith_method_integer("~", ~);
ml_arith_method_integer_integer("&", &);
ml_arith_method_integer_integer("|", |);
ml_arith_method_integer_integer("^", ^);

ML_METHOD("<<", MLIntegerT, MLIntegerT) {
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	int64_t IntegerC;
	if (IntegerB > 0) {
		IntegerC = IntegerA << IntegerB;
	} else if (IntegerB < 0) {
		if (IntegerA < 0) {
			IntegerC = -(-IntegerA >> -IntegerB);
		} else {
			IntegerC = IntegerA >> -IntegerB;
		}
	} else {
		IntegerC = IntegerA;
	}
	return ml_integer(IntegerC);
}

ML_METHOD(">>", MLIntegerT, MLIntegerT) {
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	int64_t IntegerC;
	if (IntegerB > 0) {
		if (IntegerA < 0) {
			IntegerC = -(-IntegerA >> IntegerB);
		} else {
			IntegerC = IntegerA >> IntegerB;
		}
	} else if (IntegerB < 0) {
		IntegerC = IntegerA << -IntegerB;
	} else {
		IntegerC = IntegerA;
	}
	return ml_integer(IntegerC);
}

ML_METHOD("++", MLIntegerT) {
//!number
//<Int
//>integer
// Returns :mini:`Int + 1`
	return ml_integer(ml_integer_value_fast(Args[0]) + 1);
}

ML_METHOD("--", MLIntegerT) {
//!number
//<Int
//>integer
// Returns :mini:`Int - 1`
	return ml_integer(ml_integer_value_fast(Args[0]) - 1);
}

ML_METHOD("++", MLDoubleT) {
//!number
//<Real
//>real
// Returns :mini:`Real + 1`
	return ml_real(ml_double_value_fast(Args[0]) + 1);
}

ML_METHOD("--", MLDoubleT) {
//!number
//<Real
//>real
// Returns :mini:`Real - 1`
	return ml_real(ml_double_value_fast(Args[0]) - 1);
}

ml_arith_method_real_real("/", /)
ml_arith_method_real_integer("/", /)
ml_arith_method_integer_real("/", /)

#ifdef ML_COMPLEX

ml_arith_method_complex_complex("/", /)
ml_arith_method_complex_integer("/", /)
ml_arith_method_integer_complex("/", /)
ml_arith_method_complex_real("/", /)
ml_arith_method_real_complex("/", /)
ml_arith_method_complex("~", ~);

#endif

ML_METHOD("/", MLIntegerT, MLIntegerT) {
//!number
//<Int/1
//<Int/2
//>integer | real
// Returns :mini:`Int/1 / Int/2` as an integer if the division is exact, otherwise as a real.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
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
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	return ml_integer(IntegerA % IntegerB);
}

ML_METHOD("|", MLIntegerT, MLIntegerT) {
//!number
//<Int/1
//<Int/2
//>integer
// Returns :mini:`Int/2` if it is divisible by :mini:`Int/1` and :mini:`nil` otherwise.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (!IntegerA) return ml_error("ValueError", "Division by 0");
	return (IntegerB % IntegerA) ? MLNil : Args[1];
}

ML_METHOD("!|", MLIntegerT, MLIntegerT) {
//!number
//<Int/1
//<Int/2
//>integer
// Returns :mini:`Int/2` if it is not divisible by :mini:`Int/1` and :mini:`nil` otherwise.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (!IntegerA) return ml_error("ValueError", "Division by 0");
	return (IntegerB % IntegerA) ? Args[1] : MLNil;
}

ML_METHOD("div", MLIntegerT, MLIntegerT) {
//!number
//<Int/1
//<Int/2
//>integer
// Returns the quotient of :mini:`Int/1` divided by :mini:`Int/2`.
// The result is calculated by rounding down in all cases.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
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
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (!IntegerB) return ml_error("ValueError", "Division by 0");
	long A = IntegerA;
	long B = IntegerB;
	long R = A % B;
	if (R < 0) R += labs(B);
	return ml_integer(R);
}

#define ml_comp_method_integer_integer(NAME, SYMBOL) \
ML_METHOD(NAME, MLIntegerT, MLIntegerT) { \
	int64_t IntegerA = ml_integer_value_fast(Args[0]); \
	int64_t IntegerB = ml_integer_value_fast(Args[1]); \
	return IntegerA SYMBOL IntegerB ? Args[1] : MLNil; \
}

#define ml_comp_method_real_real(NAME, SYMBOL) \
ML_METHOD(NAME, MLDoubleT, MLDoubleT) { \
	double RealA = ml_double_value_fast(Args[0]); \
	double RealB = ml_double_value_fast(Args[1]); \
	return RealA SYMBOL RealB ? Args[1] : MLNil; \
}

#define ml_comp_method_real_integer(NAME, SYMBOL) \
ML_METHOD(NAME, MLDoubleT, MLIntegerT) { \
	double RealA = ml_double_value_fast(Args[0]); \
	int64_t IntegerB = ml_integer_value_fast(Args[1]); \
	return RealA SYMBOL IntegerB ? Args[1] : MLNil; \
}

#define ml_comp_method_integer_real(NAME, SYMBOL) \
ML_METHOD(NAME, MLIntegerT, MLDoubleT) { \
	int64_t IntegerA = ml_integer_value_fast(Args[0]); \
	double RealB = ml_double_value_fast(Args[1]); \
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
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (IntegerA < IntegerB) return (ml_value_t *)NegOne;
	if (IntegerA > IntegerB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLDoubleT, MLIntegerT) {
//!number
//<Real/1
//<Int/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real/1` is less than, equal to or greater than :mini:`Int/2`.
	double RealA = ml_double_value_fast(Args[0]);
	int64_t IntegerB = ml_integer_value_fast(Args[1]);
	if (RealA < IntegerB) return (ml_value_t *)NegOne;
	if (RealA > IntegerB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLIntegerT, MLDoubleT) {
//!number
//<Int/1
//<Real/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Int/1` is less than, equal to or greater than :mini:`Real/2`.
	int64_t IntegerA = ml_integer_value_fast(Args[0]);
	double RealB = ml_double_value_fast(Args[1]);
	if (IntegerA < RealB) return (ml_value_t *)NegOne;
	if (IntegerA > RealB) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLDoubleT, MLDoubleT) {
//!number
//<Real/1
//<Real/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Real/1` is less than, equal to or greater than :mini:`Real/2`.
	double RealA = ml_double_value_fast(Args[0]);
	double RealB = ml_double_value_fast(Args[1]);
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

ML_TYPE(MLIntegerRangeT, (MLSequenceT), "integer-range");
//!range

ML_METHOD("..", MLIntegerT, MLIntegerT) {
//!range
//<Start
//<Limit
//>integer::range
// Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = ml_integer_value_fast(Args[0]);
	Range->Limit = ml_integer_value_fast(Args[1]);
	Range->Step = 1;
	return (ml_value_t *)Range;
}

ML_METHOD("..", MLIntegerT, MLIntegerT, MLIntegerT) {
//!range
//<Start
//<Limit
//<Step
//>integer::range
// Returns a range from :mini:`Start` to :mini:`Limit` (inclusive).
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = ml_integer_value_fast(Args[0]);
	Range->Limit = ml_integer_value_fast(Args[1]);
	Range->Step = ml_integer_value_fast(Args[2]);
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerT, MLIntegerT) {
//!range
//<Start
//<Step
//>integer::range
// Returns a unlimited range from :mini:`Start` in steps of :mini:`Step`.
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = ml_integer_value_fast(Args[0]);
	Range->Step = ml_integer_value_fast(Args[1]);
	if (Range->Step < 0) {
		Range->Limit = LONG_MIN;
	} else if (Range->Step > 0) {
		Range->Limit = LONG_MAX;
	} else {
		Range->Limit = Range->Start;
	}
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerRangeT, MLIntegerT) {
//!range
//<Range
//<Step
//>integer::range
// Returns a range with the same limits as :mini:`Range` but with step :mini:`Step`.
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	ml_integer_range_t *Range = new(ml_integer_range_t);
	Range->Type = MLIntegerRangeT;
	Range->Start = Range0->Start;
	Range->Limit = Range0->Limit;
	Range->Step = ml_integer_value_fast(Args[1]);
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
	} else if (Diff < 0 && Range->Step > 0) {
		return (ml_value_t *)Zero;
	} else if (Diff > 0 && Range->Step < 0) {
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
	long Value = ml_integer_value_fast(Args[0]);
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("in", MLDoubleT, MLIntegerRangeT) {
//!range
//<X
//<Range
//>X | nil
	double Value = ml_double_value_fast(Args[0]);
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
	if (--Iter->Remaining == 0) ML_RETURN(MLNil);
	++Iter->Index;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_key, MLRealIterT, ml_state_t *Caller, ml_real_iter_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

ML_TYPE(MLRealIterT, (), "real-iter");
//!range

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

ML_TYPE(MLRealRangeT, (MLSequenceT), "real-range");
//!range

ML_METHOD("..", MLNumberT, MLNumberT) {
//!range
//<Start
//<Limit
//>real::range
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = ml_real_value(Args[0]);
	Range->Limit = ml_real_value(Args[1]);
	Range->Step = 1.0;
	Range->Count = floor(Range->Limit - Range->Start) + 1;
	return (ml_value_t *)Range;
}

ML_METHOD("..", MLNumberT, MLNumberT, MLNumberT) {
//!range
//<Start
//<Limit
//>real::range
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = ml_real_value(Args[0]);
	Range->Limit = ml_real_value(Args[1]);
	double Step = Range->Step = ml_real_value(Args[2]);
	double C = (Range->Limit - Range->Start) / Step + 1;
	if (C > LONG_MAX) C = 0;
	Range->Count = C;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLNumberT, MLNumberT) {
//!range
//<Start
//<Step
//>real::range
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = ml_real_value(Args[0]);
	Range->Step = ml_real_value(Args[1]);
	Range->Limit = Range->Step > 0.0 ? INFINITY : -INFINITY;
	Range->Count = 0;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLRealRangeT, MLNumberT) {
//!range
//<Range
//<Step
//>real::range
	ml_real_range_t *Range0 = (ml_real_range_t *)Args[0];
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	double Start = Range->Start = Range0->Start;
	double Limit = Range->Limit = Range0->Limit;
	Range->Step = ml_real_value(Args[1]);
	double C = (Limit - Start) / Range->Step + 1;
	if (C > LONG_MAX) C = 0;
	Range->Count = C;
	return (ml_value_t *)Range;
}

ML_METHOD("in", MLIntegerRangeT, MLIntegerT) {
//!range
//<Range
//<Count
//>real::range
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	long C = ml_integer_value_fast(Args[1]);
	if (C <= 0) return ml_error("RangeError", "Invalid step count");
	if ((Range0->Limit - Range0->Start) % C) {
		ml_real_range_t *Range = new(ml_real_range_t);
		Range->Type = MLRealRangeT;
		Range->Start = Range0->Start;
		Range->Limit = Range0->Limit;
		Range->Step = (Range->Limit - Range->Start) / C;
		Range->Count = C + 1;
		return (ml_value_t *)Range;
	} else {
		ml_integer_range_t *Range = new(ml_integer_range_t);
		Range->Type = MLIntegerRangeT;
		Range->Start = Range0->Start;
		Range->Limit = Range0->Limit;
		Range->Step = (Range->Limit - Range->Start) / C;
		return (ml_value_t *)Range;
	}
}

ML_METHOD("in", MLRealRangeT, MLIntegerT) {
//!range
//<Range
//<Count
//>real::range
	ml_real_range_t *Range0 = (ml_real_range_t *)Args[0];
	long C = ml_integer_value_fast(Args[1]);
	if (C <= 0) return ml_error("RangeError", "Invalid step count");
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	Range->Start = Range0->Start;
	Range->Limit = Range0->Limit;
	Range->Step = (Range->Limit - Range->Start) / C;
	Range->Count = C + 1;
	return (ml_value_t *)Range;
}

ML_METHOD("by", MLIntegerRangeT, MLDoubleT) {
//!range
//<Range
//<Step
//>real::range
	ml_integer_range_t *Range0 = (ml_integer_range_t *)Args[0];
	ml_real_range_t *Range = new(ml_real_range_t);
	Range->Type = MLRealRangeT;
	double Start = Range->Start = Range0->Start;
	double Limit = Range->Limit = Range0->Limit;
	double Step = Range->Step = ml_double_value_fast(Args[1]);
	double C = (Limit - Start) / Step + 1;
	if (C > LONG_MAX) C = 0;
	Range->Count = C;
	return (ml_value_t *)Range;
}

ML_METHOD("bin", MLIntegerRangeT, MLIntegerT) {
//!range
//<Range
//<Value
//>integer | nil
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	int64_t Value = ml_integer_value_fast(Args[1]);
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return ml_integer((Value - Range->Start) / Range->Step + 1);
}

ML_METHOD("bin", MLIntegerRangeT, MLDoubleT) {
//!range
//<Range
//<Value
//>integer | nil
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	double Value = ml_real_value(Args[1]);
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return ml_integer(floor((Value - Range->Start) / Range->Step) + 1);
}

ML_METHOD("bin", MLRealRangeT, MLIntegerT) {
//!range
//<Range
//<Value
//>integer | nil
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	int64_t Value = ml_integer_value_fast(Args[1]);
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return ml_integer((Value - Range->Start) / Range->Step + 1);
}

ML_METHOD("bin", MLRealRangeT, MLDoubleT) {
//!range
//<Range
//<Value
//>integer | nil
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	double Value = ml_real_value(Args[1]);
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return ml_integer(floor((Value - Range->Start) / Range->Step) + 1);
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
	long Value = ml_integer_value_fast(Args[0]);
	ml_real_range_t *Range = (ml_real_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

ML_METHOD("in", MLDoubleT, MLRealRangeT) {
//!range
//<X
//<Range
//>X | nil
	double Value = ml_double_value_fast(Args[0]);
	ml_real_range_t *Range = (ml_real_range_t *)Args[1];
	if (Value < Range->Start) return MLNil;
	if (Value > Range->Limit) return MLNil;
	return Args[0];
}

// Switch Functions //
//!type

typedef struct {
	ml_value_t *Index;
	int64_t Min, Max;
} ml_integer_case_t;

typedef struct {
	ml_type_t *Type;
	ml_integer_case_t Cases[];
} ml_integer_switch_t;

static void ml_integer_switch(ml_state_t *Caller, ml_integer_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (!ml_is(Arg, MLNumberT)) {
		ML_ERROR("TypeError", "expected number for argument 1");
	}
	int64_t Value = ml_integer_value(Arg);
	for (ml_integer_case_t *Case = Switch->Cases;; ++Case) {
		if (Case->Min <= Value && Value <= Case->Max) ML_RETURN(Case->Index);
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLIntegerSwitchT, (MLFunctionT), "integer-switch",
//!internal
	.call = (void *)ml_integer_switch
);

ML_FUNCTION(MLIntegerSwitch) {
//!internal
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_integer_switch_t *Switch = xnew(ml_integer_switch_t, Total, ml_integer_case_t);
	Switch->Type = MLIntegerSwitchT;
	ml_integer_case_t *Case = Switch->Cases;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, MLIntegerT)) {
				Case->Min = Case->Max = ml_integer_value(Value);
			} else if (ml_is(Value, MLDoubleT)) {
				double Real = ml_real_value(Value), Int = floor(Real);
				if (Real != Int) return ml_error("ValueError", "Non-integer value in integer case");
				Case->Min = Case->Max = Int;
			} else if (ml_is(Value, MLIntegerRangeT)) {
				ml_integer_range_t *Range = (ml_integer_range_t *)Value;
				Case->Min = Range->Start;
				Case->Max = Range->Limit;
			} else if (ml_is(Value, MLRealRangeT)) {
				ml_real_range_t *Range = (ml_real_range_t *)Value;
				Case->Min = ceil(Range->Start);
				Case->Max = floor(Range->Limit);
			} else {
				return ml_error("ValueError", "Unsupported value in integer case");
			}
			Case->Index = ml_integer(I);
			++Case;
		}
	}
	Case->Min = LONG_MIN;
	Case->Max = LONG_MAX;
	Case->Index = ml_integer(Count);
	return (ml_value_t *)Switch;
}

typedef struct {
	ml_value_t *Index;
	double Min, Max;
} ml_real_case_t;

typedef struct {
	ml_type_t *Type;
	ml_real_case_t Cases[];
} ml_real_switch_t;

static void ml_real_switch(ml_state_t *Caller, ml_real_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (!ml_is(Arg, MLNumberT)) {
		ML_ERROR("TypeError", "expected number for argument 1");
	}
	double Value = ml_real_value(Arg);
	for (ml_real_case_t *Case = Switch->Cases;; ++Case) {
		if (Case->Min <= Value && Value <= Case->Max) ML_RETURN(Case->Index);
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLRealSwitchT, (MLFunctionT), "real-switch",
//!internal
	.call = (void *)ml_real_switch
);

ML_FUNCTION(MLRealSwitch) {
//!internal
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_real_switch_t *Switch = xnew(ml_real_switch_t, Total, ml_real_case_t);
	Switch->Type = MLRealSwitchT;
	ml_real_case_t *Case = Switch->Cases;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, MLIntegerT)) {
				Case->Min = Case->Max = ml_integer_value(Value);
			} else if (ml_is(Value, MLDoubleT)) {
				Case->Min = Case->Max = ml_real_value(Value);
			} else if (ml_is(Value, MLIntegerRangeT)) {
				ml_integer_range_t *Range = (ml_integer_range_t *)Value;
				Case->Min = Range->Start;
				Case->Max = Range->Limit;
			} else if (ml_is(Value, MLRealRangeT)) {
				ml_real_range_t *Range = (ml_real_range_t *)Value;
				Case->Min = Range->Start;
				Case->Max = Range->Limit;
			} else {
				return ml_error("ValueError", "Unsupported value in real case");
			}
			Case->Index = ml_integer(I);
			++Case;
		}
	}
	Case->Min = -INFINITY;
	Case->Max = INFINITY;
	Case->Index = ml_integer(Count);
	return (ml_value_t *)Switch;
}

// Modules //
//!module

ML_FUNCTION(MLModule) {
//@module
//<Path:string
//<Lookup:function
//>module
// Returns a generic module which calls resolves :mini:`Module::Import` by calling :mini:`Lookup(Module, Import)`, caching results for future use.
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ML_CHECK_ARG_TYPE(1, MLFunctionT);
	ml_module_t *Module = new(ml_module_t);
	Module->Type = MLModuleT;
	Module->Path = ml_string_value(Args[0]);
	Module->Lookup = Args[1];
	return (ml_value_t *)Module;
}

ML_TYPE(MLModuleT, (), "module",
	.Constructor = (ml_value_t *)MLModule
);

typedef struct {
	ml_state_t Base;
	ml_module_t *Module;
	const char *Name;
} ml_module_lookup_state_t;

static void ml_module_lookup_run(ml_module_lookup_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (!ml_is_error(Value)) {
		stringmap_insert(State->Module->Exports, State->Name, Value);
	}
	ML_RETURN(Value);
}

ML_METHODX("::", MLModuleT, MLStringT) {
//<Module
//<Name
//>MLAnyT
// Imports a symbol from a module.
	ml_module_t *Module = (ml_module_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t *Value = stringmap_search(Module->Exports, Name);
	if (!Value) {
		if (Module->Lookup) {
			ml_module_lookup_state_t *State = new(ml_module_lookup_state_t);
			State->Base.Caller = Caller;
			State->Base.Context = Caller->Context;
			State->Base.run = (ml_state_fn)ml_module_lookup_run;
			State->Module = Module;
			State->Name = Name;
			return ml_call(State, Module->Lookup, 2, Args);
		} else {
			ML_ERROR("ModuleError", "Symbol %s not exported from module %s", Name, Module->Path);
		}
	}
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

ML_METHOD(MLStringT, MLModuleT) {
	ml_module_t *Module = (ml_module_t *)Args[0];
	return ml_string_format("module(%s)", Module->Path);
}

static int ml_module_exports_fn(const char *Name, void *Value, ml_value_t *Exports) {
	ml_map_insert(Exports, ml_cstring(Name), Value);
	return 0;
}

ML_METHOD("path", MLModuleT) {
	ml_module_t *Module = (ml_module_t *)Args[0];
	return ml_cstring(Module->Path);
}

ML_METHOD("exports", MLModuleT) {
	ml_module_t *Module = (ml_module_t *)Args[0];
	ml_value_t *Exports = ml_map();
	stringmap_foreach(Module->Exports, Exports, (void *)ml_module_exports_fn);
	return Exports;
}

// Init //
//!general

void ml_init() {
#ifdef ML_JIT
	GC_set_pages_executable(1);
#endif
	GC_INIT();
#include "ml_types_init.c"
#ifdef ML_GENERICS
	ml_type_add_rule(MLTupleT, MLSequenceT, MLIntegerT, MLAnyT, NULL);
#endif
	stringmap_insert(MLTypeT->Exports, "switch", MLTypeSwitch);
	stringmap_insert(MLIntegerT->Exports, "range", MLIntegerRangeT);
	stringmap_insert(MLIntegerT->Exports, "switch", MLIntegerSwitch);
	stringmap_insert(MLRealT->Exports, "range", MLRealRangeT);
	ml_method_by_value(MLIntegerT->Constructor, NULL, ml_identity, MLIntegerT, NULL);
	ml_method_by_value(MLDoubleT->Constructor, NULL, ml_identity, MLDoubleT, NULL);
	ml_method_by_value(MLRealT->Constructor, NULL, ml_identity, MLDoubleT, NULL);
	stringmap_insert(MLRealT->Exports, "infinity", ml_real(INFINITY));
	ml_method_by_value(MLNumberT->Constructor, NULL, ml_identity, MLNumberT, NULL);
#ifdef ML_COMPLEX
	stringmap_insert(MLCompilerT->Exports, "i", ml_complex(1i));
#endif
	ml_method_by_name("=", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("!=", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("<", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name(">", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("<=", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name(">=", NULL, ml_return_nil, MLNilT, MLAnyT, NULL);
	ml_method_by_name("=", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name("!=", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name("<", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name(">", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name("<=", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_method_by_name(">=", NULL, ml_return_nil, MLAnyT, MLNilT, NULL);
	ml_string_init();
	ml_method_init();
	ml_list_init();
	ml_map_init();
	ml_compiler_init();
	ml_runtime_init();
	ml_bytecode_init();
}

typedef struct {
	ml_state_t Base;
	int Index;
	ml_value_t *New;
	ml_value_t *Args[];
} ml_exchange_t;

static void ml_exchange_run(ml_exchange_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (!State->Index) ML_RETURN(State->New);
	int I = --State->Index;
	ml_value_t *New = State->New;
	State->New = ml_deref(State->Args[I]);
	return ml_assign(State, State->Args[I], New);
}

ML_FUNCTIONZ(MLExchange) {
//@exchange
	ML_CHECKX_ARG_COUNT(1);
	ml_exchange_t *State = xnew(ml_exchange_t, Count, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_exchange_run;
	memcpy(State->Args, Args, Count * sizeof(ml_value_t *));
	State->New = ml_deref(Args[0]);
	State->Index = Count;
	return ml_exchange_run(State, MLNil);
}

ML_FUNCTIONZ(MLReplace) {
//@replace
	ML_CHECKX_ARG_COUNT(2);
	ml_exchange_t *State = xnew(ml_exchange_t, Count - 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_exchange_run;
	memcpy(State->Args, Args, (Count - 1) * sizeof(ml_value_t *));
	State->New = ml_deref(Args[Count - 1]);
	State->Index = Count - 1;
	return ml_exchange_run(State, MLNil);}

void ml_types_init(stringmap_t *Globals) {
	if (Globals) {
		stringmap_insert(Globals, "any", MLAnyT);
		stringmap_insert(Globals, "type", MLTypeT);
		stringmap_insert(Globals, "function", MLFunctionT);
		stringmap_insert(Globals, "sequence", MLSequenceT);
		stringmap_insert(Globals, "boolean", MLBooleanT);
		stringmap_insert(Globals, "true", MLTrue);
		stringmap_insert(Globals, "false", MLFalse);
		stringmap_insert(Globals, "number", MLNumberT);
		stringmap_insert(Globals, "integer", MLIntegerT);
		stringmap_insert(Globals, "real", MLRealT);
		stringmap_insert(Globals, "double", MLDoubleT);
#ifdef ML_COMPLEX
		stringmap_insert(Globals, "complex", MLComplexT);
		stringmap_insert(Globals, "i", ml_complex(1i));
#endif
		stringmap_insert(Globals, "method", MLMethodT);
		stringmap_insert(Globals, "buffer", MLBufferT);
		stringmap_insert(Globals, "string", MLStringT);
		//stringmap_insert(Globals, "stringbuffer", MLStringBufferT);
		stringmap_insert(Globals, "regex", MLRegexT);
		stringmap_insert(Globals, "tuple", MLTupleT);
		stringmap_insert(Globals, "list", MLListT);
		stringmap_insert(Globals, "names", MLNamesT);
		stringmap_insert(Globals, "map", MLMapT);
		stringmap_insert(Globals, "module", MLModuleT);
		stringmap_insert(Globals, "exchange", MLExchange);
		stringmap_insert(Globals, "replace", MLReplace);
	}
}

#include "minilang.h"
#include "ml_macros.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <gc/gc.h>

#include "ml_compiler2.h"
#include "ml_runtime.h"
#include "ml_number.h"
#include "ml_string.h"
#include "ml_method.h"
#include "ml_list.h"
#include "ml_map.h"
#include "ml_set.h"

#ifdef ML_TRE
#include <tre/regex.h>
#else
#include <regex.h>
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "type"

ML_METHOD_DECL(IterateMethod, "iterate");
ML_METHOD_DECL(ValueMethod, "value");
ML_METHOD_DECL(KeyMethod, "key");
ML_METHOD_DECL(NextMethod, "next");
ML_METHOD_DECL(CompareMethod, "<>");
ML_METHOD_DECL(MinMethod, "min");
ML_METHOD_DECL(MaxMethod, "max");
ML_METHOD_DECL(IndexMethod, "[]");
ML_METHOD_DECL(SymbolMethod, "::");
ML_METHOD_DECL(CallMethod, "()");
ML_METHOD_DECL(AssignMethod, ":=");
ML_METHOD_DECL(EqualMethod, "=");
ML_METHOD_DECL(LessMethod, "<");
ML_METHOD_DECL(GreaterMethod, ">");
ML_METHOD_DECL(AddMethod, "+");
ML_METHOD_DECL(MulMethod, "*");
ML_METHOD_DECL(AndMethod, "/\\");
ML_METHOD_DECL(OrMethod, "\\/");
ML_METHOD_DECL(XorMethod, "><");

static inline uintptr_t rotl(uintptr_t X, unsigned int N) {
	const unsigned int Mask = (CHAR_BIT * sizeof(uintptr_t) - 1);
	return (X << (N & Mask)) | (X >> ((-N) & Mask ));
}

// Types //

ML_INTERFACE(MLAnyT, (), "any", .Rank = 0);
//!any
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

ML_FUNCTION(MLType) {
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
	.Constructor = (ml_value_t *)MLType
);

static int ML_TYPED_FN(ml_value_is_constant, MLTypeT, ml_value_t *Value) {
	return 1;
}

ML_METHOD("rank", MLTypeT) {
//!type
//<Type
//>integer
// Returns the rank of :mini:`Type`, i.e. the depth of its inheritence tree.
	ml_type_t *Type = (ml_type_t *)Args[0];
	return ml_integer(Type->Rank);
}

static int ml_type_exports_fn(const char *Name, void *Value, ml_value_t *Exports) {
	ml_map_insert(Exports, ml_string(Name, -1), Value);
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

ML_TYPE(MLTypeGenericT, (MLTypeT), "generic-type");
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

ML_METHOD("parents", MLTypeGenericT) {
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
// Returns a list of the parent types of :mini:`Type`.
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

void ml_type_add_parent(ml_type_t *Type, ml_type_t *Parent) {
	if (inthash_insert(Type->Parents, (uintptr_t)Parent, Parent)) return;
	//inthash_insert(Type->Parents, (uintptr_t)Parent, Parent);
	for (int I = 0; I < Parent->Parents->Size; ++I) {
		ml_type_t *Parent2 = (ml_type_t *)Parent->Parents->Keys[I];
		if (Parent2) ml_type_add_parent(Type, Parent2);
	}
	if (Type->Rank <= Parent->Rank) Type->Rank = Parent->Rank + 1;
#ifdef ML_GENERICS
	if (Parent->Type == MLTypeGenericT) ml_type_add_parent(Type, ml_generic_type_args(Parent)[0]);
#endif
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

typedef struct ml_typed_fn_entry_t ml_typed_fn_entry_t;

struct ml_typed_fn_entry_t {
	ml_typed_fn_entry_t *Next;
	ml_type_t *Type;
	void *Fn;
};

static inthash_t MLTypedFns[1] = {INTHASH_INIT};

void *ml_typed_fn_get(ml_type_t *Type, void *TypedFn) {
	ML_TYPED_FN_LOCK();
	inthash_result_t Result = inthash_search2_inline(Type->TypedFns, (uintptr_t)TypedFn);
	ML_TYPED_FN_UNLOCK();
	if (Result.Present) return Result.Value;
	int BestRank = -1;
	void *BestFn = NULL;
	for (ml_typed_fn_entry_t *Entry = inthash_search(MLTypedFns, (uintptr_t)TypedFn); Entry; Entry = Entry->Next) {
		if (Entry->Type->Rank > BestRank && ml_is_subtype(Type, Entry->Type)) {
			BestRank = Entry->Type->Rank;
			BestFn = Entry->Fn;
		}
	}
	ML_TYPED_FN_LOCK();
	inthash_insert(Type->TypedFns, (uintptr_t)TypedFn, BestFn);
	ML_TYPED_FN_UNLOCK();
	return BestFn;
}

void ml_typed_fn_set(ml_type_t *Type, void *TypedFn, void *Function) {
	ml_typed_fn_entry_t *Entry = new(ml_typed_fn_entry_t);
	Entry->Type = Type;
	Entry->Fn = Function;
	Entry->Next = inthash_insert(MLTypedFns, (uintptr_t)TypedFn, Entry);
}

typedef struct {
	ml_type_t Base;
	int NumTypes;
	ml_type_t *Types[];
} ml_union_type_t;

ML_TYPE(MLTypeUnionT, (MLTypeT), "union-type");
//!internal

ML_METHOD("|", MLTypeT, MLTypeT) {
//<Type/1
//<Type/2
//>type
// Returns a union interface of :mini:`Type/1` and :mini:`Type/2`.
	ml_type_t *Type1 = (ml_type_t *)Args[0];
	ml_type_t *Type2 = (ml_type_t *)Args[1];
	ml_union_type_t *Type = xnew(ml_union_type_t, 2, ml_type_t *);
	Type->Base.Type = MLTypeUnionT;
	asprintf((char **)&Type->Base.Name, "%s | %s", Type1->Name, Type2->Name);
	Type->Base.hash = ml_default_hash;
	Type->Base.call = ml_default_call;
	Type->Base.deref = ml_default_deref;
	Type->Base.assign = ml_default_assign;
	if (Type1->Rank > Type2->Rank) {
		Type->Base.Rank = Type1->Rank + 1;
	} else {
		Type->Base.Rank = Type2->Rank + 1;
	}
	Type->NumTypes = 2;
	Type->Types[0] = Type1;
	Type->Types[1] = Type2;
	return (ml_value_t *)Type;
}

ML_METHOD("?", MLTypeT) {
//<Type
//>type
// Returns a union interface of :mini:`Type` and :mini:`type(nil)`.
	ml_type_t *Type1 = (ml_type_t *)Args[0];
	ml_union_type_t *Type = xnew(ml_union_type_t, 2, ml_type_t *);
	Type->Base.Type = MLTypeUnionT;
	asprintf((char **)&Type->Base.Name, "%s | nil", Type1->Name);
	Type->Base.hash = ml_default_hash;
	Type->Base.call = ml_default_call;
	Type->Base.deref = ml_default_deref;
	Type->Base.assign = ml_default_assign;
	Type->Base.Rank = Type1->Rank + 1;
	Type->NumTypes = 2;
	Type->Types[0] = Type1;
	Type->Types[1] = MLNilT;
	return (ml_value_t *)Type;
}

ML_METHOD("append", MLStringBufferT, MLTypeT) {
//<Buffer
//<Value
// Appends a representation of :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_type_t *Type = (ml_type_t *)Args[1];
	ml_stringbuffer_printf(Buffer, "<<%s>>", Type->Name);
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
		size_t Length = strlen(Base->Name) + NumArgs + 1;
		for (int I = 1; I < NumArgs; ++I) Length += strlen(Args[I]->Name);
		char *Name2 = snew(Length);
		char *End = stpcpy(Name2, Base->Name);
		*End++ = '[';
		End = stpcpy(End, Args[1]->Name);
		for (int I = 2; I < NumArgs; ++I) {
			*End++ = ',';
			End = stpcpy(End, Args[I]->Name);
		}
		*End++ = ']';
		*End = 0;
		Name = Name2;
	}
	Type->Base.Type = MLTypeGenericT;
	Type->Base.Name = Name;
	Type->Base.hash = Base->hash;
	Type->Base.call = Base->call;
	Type->Base.deref = Base->deref;
	Type->Base.assign = Base->assign;
	Type->Base.Rank = Base->Rank + 1;
	Type->Base.Interface = Args[0]->Interface;
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

static int ML_TYPED_FN(ml_value_is_constant, MLNilT, ml_value_t *Value) {
	return 1;
}

ML_FUNCTION(MLSomeFn) {
//!internal
	return MLSome;
}

ML_TYPE(MLSomeT, (MLFunctionT), "some",
//!internal
	.Constructor = (ml_value_t *)MLSomeFn
);

static int ML_TYPED_FN(ml_value_is_constant, MLSomeT, ml_value_t *Value) {
	return 1;
}

static void ml_blank_assign(ml_state_t *Caller, ml_value_t *Blank, ml_value_t *Value) {
	ML_RETURN(Value);
}

ML_TYPE(MLBlankT, (), "blank",
//!internal
	.assign = ml_blank_assign
);

ML_VALUE(MLNil, MLNilT);
//!internal

ML_METHOD("append", MLStringBufferT, MLNilT) {
//!internal
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_write(Buffer, "nil", 3);
	return MLSome;
}

ML_VALUE(MLSome, MLSomeT);
//!internal

ML_METHOD("append", MLStringBufferT, MLSomeT) {
//!internal
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_write(Buffer, "some", 4);
	return MLSome;
}

ML_VALUE(MLBlank, MLBlankT);
//!internal

#ifdef ML_GENERICS

int ml_find_generic_parent0(int TNumArgs, ml_type_t **TArgs, ml_type_t *U, int Max, ml_type_t **Args) {
	if (TArgs[0] == U) {
		if (Max > TNumArgs) Max = TNumArgs;
		for (int I = 0; I < Max; ++I) Args[I] = TArgs[I];
		return Max;
	}
	for (ml_generic_rule_t *Rule = TArgs[0]->Rules; Rule; Rule = Rule->Next) {
		int TNumArgs2 = Rule->NumArgs;
		ml_type_t *TArgs2[TNumArgs2];
		ml_generic_fill(Rule, TArgs2, TNumArgs, TArgs);
		int Find = ml_find_generic_parent0(TNumArgs2, TArgs2, U, Max, Args);
		if (Find >= 0) return Find;
	}
	return -1;
}

int ml_find_generic_parent(ml_type_t *T, ml_type_t *U, int Max, ml_type_t **Args) {
	if (T->Type == MLTypeGenericT) {
		ml_generic_type_t *GenericT = (ml_generic_type_t *)T;
		return ml_find_generic_parent0(GenericT->NumArgs, GenericT->Args, U, Max, Args);
	} else {
		int Find = ml_find_generic_parent0(1, &T, U, Max, Args);
		if (Find >= 0) return Find;
		for (int I = 0; I < T->Parents->Size; ++I) {
			ml_type_t *Parent = (ml_type_t *)T->Parents->Keys[I];
			int Rank = 0;
			if (Parent && (Parent->Rank > Rank)) {
				int Find2 = ml_find_generic_parent(Parent, U, Max, Args);
				if (Find2 >= 0) {
					Rank = Parent->Rank;
					Find = Find2;
				}
			}
		}
		return Find;
	}
}

static int ml_is_generic_subtype1(int TNumArgs, ml_type_t **TArgs, ml_type_t *U) {
	if (TArgs[0] == U) return 1;
	for (ml_generic_rule_t *Rule = TArgs[0]->Rules; Rule; Rule = Rule->Next) {
		int TNumArgs2 = Rule->NumArgs;
		ml_type_t *TArgs2[TNumArgs2];
		ml_generic_fill(Rule, TArgs2, TNumArgs, TArgs);
		if (ml_is_generic_subtype1(TNumArgs2, TArgs2, U)) return 1;
	}
	return 0;
}

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
	if (U->Type == MLTypeUnionT) {
		ml_union_type_t *Union = (ml_union_type_t *)U;
		for (int I = 0; I < Union->NumTypes; ++I) {
			if (ml_is_subtype(T, Union->Types[I])) return 1;
		}
		return 0;
	}
	if (inthash_search(T->Parents, (uintptr_t)U)) return 1;
#ifdef ML_GENERICS
	if (T->Type == MLTypeGenericT) {
		ml_generic_type_t *GenericT = (ml_generic_type_t *)T;
		if (GenericT->Args[0] == U) {
			return 1;
		} else if (U->Type == MLTypeGenericT) {
			ml_generic_type_t *GenericU = (ml_generic_type_t *)U;
			return ml_is_generic_subtype(GenericT->NumArgs, GenericT->Args, GenericU->NumArgs, GenericU->Args);
		}
		return ml_is_generic_subtype1(GenericT->NumArgs, GenericT->Args, U);
	} else if (U->Type == MLTypeGenericT) {
		ml_generic_type_t *GenericU = (ml_generic_type_t *)U;
		return ml_is_generic_subtype(1, &T, GenericU->NumArgs, GenericU->Args);
	} else {
		return ml_is_generic_subtype1(1, &T, U);
	}
#endif
	return 0;
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
	if (T == U) return T;
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
	if (T->Type == MLTypeGenericT) {
		ml_generic_type_t *GenericT = (ml_generic_type_t *)T;
		if (U->Type == MLTypeGenericT) {
			ml_generic_type_t *GenericU = (ml_generic_type_t *)U;
			Max = ml_generic_type_max(Max, GenericT->NumArgs, GenericT->Args, GenericU->NumArgs, GenericU->Args);
		} else {
			if (GenericT->Args[0] == U) return U;
			Max = ml_generic_type_max(Max, GenericT->NumArgs, GenericT->Args, 1, &U);
		}
	} else {
		if (U->Type == MLTypeGenericT) {
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

// Copying //

//!general

ML_FUNCTIONX(MLVisit) {
//@visit
//<Value
//<Fn
//>any
// Returns :mini:`Fn(V, Value)` where :mini:`V` is a newly created :mini:`visitor`.
	ML_CHECKX_ARG_COUNT(2);
	ML_CHECKX_ARG_TYPE(1, MLFunctionT);
	ml_visitor_t *Visitor = new(ml_visitor_t);
	Visitor->Type = MLVisitorT;
	Visitor->Fn = Args[1];
	Visitor->Error = ml_error("CallError", "Recursive visit detected");
	Visitor->Args[0] = (ml_value_t *)Visitor;
	ml_value_t **Args2 = ml_alloc_args(2);
	Args2[0] = (ml_value_t *)Visitor;
	Args2[1] = Args[0];
	return ml_call(Caller, Visitor->Fn, 2, Args2);
}

static void ml_visitor_call(ml_state_t *Caller, ml_visitor_t *Visitor, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Value = ml_deref(Args[0]);
	if (Count > 1) {
		ml_value_t *Result = ml_deref(Args[1]);
		inthash_insert(Visitor->Cache, (uintptr_t)Value, Result);
		ML_RETURN(Result);
	} else {
#ifdef ML_NANBOXING
		if (!ml_tag(Value)) {
#endif
			ml_value_t *Result = inthash_search(Visitor->Cache, (uintptr_t)Value);
			if (Result) ML_RETURN(Result);
#ifdef ML_NANBOXING
		}
#endif
		inthash_insert(Visitor->Cache, (uintptr_t)Value, Visitor->Error);
		Visitor->Args[1] = Value;
		return ml_call(Caller, Visitor->Fn, 2, Visitor->Args);
	}
}

ML_TYPE(MLVisitorT, (MLFunctionT), "visitor",
// Used to apply a transformation recursively to values.
//
// :mini:`fun (V: visitor)(Value: any, Result: any): any`
//    Adds the pair :mini:`(Value, Result)` to :mini:`V`'s cache and returns :mini:`Result`.
//
// :mini:`fun (V: visitor)(Value: any): any`
//    Visits :mini:`Value` with :mini:`V` returning the result.
	.call = (void *)ml_visitor_call
);

ML_METHOD("visit", MLVisitorT, MLAnyT) {
//<Visitor
//<Value
//>any
// Default visitor implementation, just returns :mini:`nil`.
	return MLNil;
}

ML_METHOD_DECL(CopyMethod, "copy");

ML_FUNCTIONX(MLCopy) {
//@copy
//<Value:any
//<Fn?:function
//>any
// Returns a copy of :mini:`Value` using a new :mini:`copy` instance which applies :mini:`Fn(Copy, Value)` to each value. If omitted, :mini:`Fn` defaults to :mini:`:copy`.
	ML_CHECKX_ARG_COUNT(1);
	ml_visitor_t *Visitor = new(ml_visitor_t);
	Visitor->Type = MLVisitorT;
	Visitor->Fn = Count > 1 ? Args[1] : CopyMethod;
	Visitor->Error = ml_error("CallError", "Recursive visit detected");
	Visitor->Args[0] = (ml_value_t *)Visitor;
	ml_value_t **Args2 = ml_alloc_args(2);
	Args2[0] = (ml_value_t *)Visitor;
	Args2[1] = Args[0];
	return ml_call(Caller, Visitor->Fn, 2, Args2);
}

ML_METHOD("copy", MLVisitorT, MLAnyT) {
//<Visitor
//<Value
//>any
// Default visitor implementation, just returns :mini:`Value`.
#ifdef ML_NANBOXING
	if (!ml_tag(Args[1])) {
#endif
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	inthash_insert(Visitor->Cache, (uintptr_t)Args[1], Args[1]);
#ifdef ML_NANBOXING
	}
#endif
	return Args[1];
}

ML_METHOD("const", MLVisitorT, MLAnyT) {
//<Visitor
//<Value
//>any
// Default visitor implementation, just returns :mini:`Value`.
#ifdef ML_NANBOXING
	if (!ml_tag(Args[1])) {
#endif
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	inthash_insert(Visitor->Cache, (uintptr_t)Args[1], Args[1]);
#ifdef ML_NANBOXING
	}
#endif
	return Args[1];
}

//!any

ML_METHOD("in", MLAnyT, MLTypeT) {
//<Value
//<Type
//>Value | nil
// Returns :mini:`Value` if it is an instance of :mini:`Type` or a type that inherits from :mini:`Type` and :mini:`nil` otherwise.
	return ml_is_subtype(ml_typeof(Args[0]), (ml_type_t *)Args[1]) ? Args[0] : MLNil;
}

ML_METHOD_ANON(MLCompilerSwitch, "compiler::switch");

ML_METHOD(MLCompilerSwitch, MLFunctionT) {
//!internal
	return Args[0];
}

ML_METHOD(MLCompilerSwitch, MLTypeT) {
//!internal
	ml_type_t *Type = (ml_type_t *)Args[0];
	ml_value_t *Switch = (ml_value_t *)stringmap_search(Type->Exports, "switch");
	if (!Switch) return ml_error("SwitchError", "%s does not support switch", Type->Name);
	return Switch;
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

ML_FUNCTION_INLINE(MLTypeSwitch) {
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
			} else if (Value == MLNil) {
				Case->Type = MLNilT;
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

ML_METHODVZ("()", MLAnyT) {
//!internal
	ml_value_t *Value = ml_deref(Args[0]);
	ml_type_t *Type = ml_typeof(Value);
	if (Type->call == ml_default_call) ML_ERROR("TypeError", "<%s> is not callable", Type->Name);
	return Type->call(Caller, Value, Count - 1, Args + 1);
}

ML_METHOD("<>", MLAnyT, MLAnyT) {
//<Value/1
//<Value/2
//>integer
// Compares :mini:`Value/1` and :mini:`Value/2` and returns :mini:`-1`, :mini:`0` or :mini:`1`.
// This comparison is based on the types and internal addresses of :mini:`Value/1` and :mini:`Value/2` and thus only has no persistent meaning.
	ml_type_t *Type1 = ml_typeof(Args[0]);
	ml_type_t *Type2 = ml_typeof(Args[1]);
	if (Type1 < Type2) return (ml_value_t *)NegOne;
	if (Type1 > Type2) return (ml_value_t *)One;
	if (Args[0] < Args[1]) return (ml_value_t *)NegOne;
	if (Args[0] > Args[1]) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

ML_METHOD("<>", MLNilT, MLAnyT) {
//!internal
	return (ml_value_t *)NegOne;
}

ML_METHOD("<>", MLAnyT, MLNilT) {
//!internal
	return (ml_value_t *)One;
}

ML_METHOD("<>", MLNilT, MLNilT) {
//!internal
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

typedef struct {
	ml_state_t Base;
	ml_value_t *Args[2];
} ml_comp_state_t;

static void ml_min_state_run(ml_comp_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (Result != MLNil) ML_RETURN(State->Args[0]);
	ML_RETURN(State->Args[1]);
}

ML_METHODX("min", MLAnyT, MLAnyT) {
//<A
//<B
//>any
// Returns :mini:`A` if :mini:`A < B` and :mini:`B` otherwise.
	ml_comp_state_t *State = new(ml_comp_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_min_state_run;
	State->Args[0] = Args[0];
	State->Args[1] = Args[1];
	return ml_call(State, LessMethod, 2, State->Args);
}

static void ml_max_state_run(ml_comp_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (Result != MLNil) ML_RETURN(State->Args[0]);
	ML_RETURN(State->Args[1]);
}

ML_METHODX("max", MLAnyT, MLAnyT) {
//<A
//<B
//>any
// Returns :mini:`A` if :mini:`A > B` and :mini:`B` otherwise.
	ml_comp_state_t *State = new(ml_comp_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_max_state_run;
	State->Args[0] = Args[0];
	State->Args[1] = Args[1];
	return ml_call(State, GreaterMethod, 2, State->Args);
}

ML_METHOD("append", MLStringBufferT, MLAnyT) {
//<Buffer
//<Value
// Appends a representation of :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_printf(Buffer, "<%s>", ml_typeof(Args[1])->Name);
	return MLSome;
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Default;
	inthash_t Cases[1];
} ml_any_switch_t;

static void ml_any_switch(ml_state_t *Caller, ml_any_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	ml_value_t *Index = inthash_search(Switch->Cases, (uintptr_t)Arg);
	ML_RETURN(Index ?: Switch->Default);
}

ML_TYPE(MLAnySwitchT, (MLFunctionT), "any-switch",
//!internal
	.call = (void *)ml_any_switch
);

ML_FUNCTION_INLINE(MLAnySwitch) {
//!internal
	int Total = 1;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		Total += ml_list_length(Args[I]);
	}
	ml_any_switch_t *Switch = new(ml_any_switch_t);
	Switch->Type = MLAnySwitchT;
	for (int I = 0; I < Count; ++I) {
		ML_LIST_FOREACH(Args[I], Iter) {
			inthash_insert(Switch->Cases, (uintptr_t)Iter->Value, ml_integer(I));
		}
	}
	Switch->Default = ml_integer(Count);
	return (ml_value_t *)Switch;
}

void ml_value_set_name(ml_value_t *Value, const char *Name) {
	typeof(ml_value_set_name) *function = ml_typed_fn_get(ml_typeof(Value), ml_value_set_name);
	if (function) function(Value, Name);
}

void ml_value_find_all(ml_value_t *Value, void *Data, ml_value_find_fn RefFn) {
	typeof(ml_value_find_all) *function = ml_typed_fn_get(ml_typeof(Value), ml_value_find_all);
	if (function) return function(Value, Data, RefFn);
	RefFn(Data, Value, 0);
}

typedef struct {
	ml_value_t *Refs;
	ml_type_t *Type;
	inthash_t Done[1];
} ml_find_refs_t;

static int ml_find_all_fn(ml_find_refs_t *FindRefs, ml_value_t *Value, int HasRefs) {
	if (!inthash_insert(FindRefs->Done, (uintptr_t)Value, Value)) {
		ml_list_put(FindRefs->Refs, Value);
		return 1;
	}
	return 0;
}

static int ml_find_all_typed_fn(ml_find_refs_t *FindRefs, ml_value_t *Value, int HasRefs) {
	if (!inthash_insert(FindRefs->Done, (uintptr_t)Value, Value)) {
		if (ml_is(Value, FindRefs->Type)) ml_list_put(FindRefs->Refs, Value);
		return 1;
	}
	return 0;
}

ML_FUNCTION(MLFindAll) {
//!general
//@findall
//<Value:any
//<Filter?:boolean|type
//>list
// Returns a list of all unique values referenced by :mini:`Value` (including :mini:`Value`).
	ML_CHECK_ARG_COUNT(1);
	ml_find_refs_t FindRefs[1] = {ml_list(), MLAnyT, {INTHASH_INIT}};
	ml_value_find_fn RefFn = (ml_value_find_fn)ml_find_all_fn;
	if (Count > 1) {
		ML_CHECK_ARG_TYPE(1, MLTypeT);
		FindRefs->Type = (ml_type_t *)Args[1];
		RefFn = (ml_value_find_fn)ml_find_all_typed_fn;
	}
	ml_value_find_all(Args[0], FindRefs, (ml_value_find_fn)RefFn);
	return FindRefs->Refs;
}

int ml_value_is_constant(ml_value_t *Value) {
	typeof(ml_value_is_constant) *function = ml_typed_fn_get(ml_typeof(Value), ml_value_is_constant);
	if (function) return function(Value);
	return 0;
}

ML_FUNCTION(MLIsConstant) {
//!general
//@isconstant
//<Value:any
//>any|nil
// Returns :mini:`some` if it is a constant (i.e. directly immutable and not referencing any mutable values), otherwise returns :mini:`nil`.
//$= isconstant(1)
//$= isconstant(1.5)
//$= isconstant("Hello")
//$= isconstant(true)
//$= isconstant([1, 2, 3])
//$= isconstant((1, 2, 3))
//$= isconstant((1, [2], 3))
	ML_CHECK_ARG_COUNT(1);
	if (ml_value_is_constant(Args[0])) return Args[0];
	return MLNil;
}

// Iterators //

void ml_iterate(ml_state_t *Caller, ml_value_t *Value) {
	typeof(ml_iterate) *function = ml_typed_fn_get(ml_typeof(Value), ml_iterate);
	if (function) return function(Caller, Value);
	ml_value_t **Args = ml_alloc_args(1);
	Args[0] = Value;
	return ml_call(Caller, IterateMethod, 1, Args);
}

void ml_iter_value(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_value) *function = ml_typed_fn_get(ml_typeof(Iter), ml_iter_value);
	if (function) return function(Caller, Iter);
	ml_value_t **Args = ml_alloc_args(1);
	Args[0] = Iter;
	return ml_call(Caller, ValueMethod, 1, Args);
}

void ml_iter_key(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_key) *function = ml_typed_fn_get(ml_typeof(Iter), ml_iter_key);
	if (function) return function(Caller, Iter);
	ml_value_t **Args = ml_alloc_args(1);
	Args[0] = Iter;
	return ml_call(Caller, KeyMethod, 1, Args);
}

void ml_iter_next(ml_state_t *Caller, ml_value_t *Iter) {
	typeof(ml_iter_next) *function = ml_typed_fn_get(ml_typeof(Iter), ml_iter_next);
	if (function) return function(Caller, Iter);
	ml_value_t **Args = ml_alloc_args(1);
	Args[0] = Iter;
	return ml_call(Caller, NextMethod, 1, Args);
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
		if (ml_is(Name, MLStringT)) {
			ml_names_add(Names, Name);
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
		if (ml_is(Name, MLStringT)) {
			ml_names_add(Names, Name);
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
		if (ml_is(Name, MLStringT)) {
			ml_names_add(Names, Name);
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

ml_value_t *ml_cfunctionz2(void *Data, ml_callbackx_t Callback, const char *Source, int Line) {
	ml_cfunctionx_t *Function = new(ml_cfunctionx_t);
	Function->Type = MLCFunctionZT;
	Function->Data = Data;
	Function->Callback = Callback;
	Function->Source = Source;
	Function->Line = Line;
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

ML_TYPE(MLFunctionPartialT, (MLFunctionT, MLSequenceT), "partial-function",
//!function
	.call = (void *)ml_partial_function_call
);

ml_value_t *ml_partial_function(ml_value_t *Function, int Count) {
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, Count, ml_value_t *);
	Partial->Type = MLFunctionPartialT;
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

ML_METHOD("arity", MLFunctionPartialT) {
//!function
	ml_partial_function_t *Partial = (ml_partial_function_t *)Args[0];
	return ml_integer(Partial->Count);
}

ML_METHOD("set", MLFunctionPartialT) {
//!function
	ml_partial_function_t *Partial = (ml_partial_function_t *)Args[0];
	return ml_integer(Partial->Set);
}

ML_METHODV("[]", MLFunctionPartialT) {
//!function
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, Count, ml_value_t *);
	Partial->Type = MLFunctionPartialT;
	Partial->Function = IndexMethod;
	Partial->Count = Count;
	Partial->Set = Count - 1;
	for (int I = 1; I < Count; ++I) Partial->Args[I] = Args[I];
	return ml_chainedv(2, Args[0], Partial);
}

ML_METHOD("$!", MLFunctionT, MLListT) {
//!function
//<Function
//<List
//>function::partial
// Returns a function equivalent to :mini:`fun(Args...) Function(List/1, List/2, ..., Args...)`.
	ml_list_t *ArgsList = (ml_list_t *)Args[1];
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, ArgsList->Length, ml_value_t *);
	Partial->Type = MLFunctionPartialT;
	Partial->Function = Args[0];
	Partial->Count = Partial->Set = ArgsList->Length;
	ml_value_t **Arg = Partial->Args;
	ML_LIST_FOREACH(ArgsList, Node) *Arg++ = Node->Value;
	return (ml_value_t *)Partial;
}

ML_METHOD("!!", MLFunctionT, MLListT) {
//!function
//<Function
//<List
//>function::partial
//
// .. deprecated:: 2.7.0
//
//    Use :mini:`$!` instead.
// Returns a function equivalent to :mini:`fun(Args...) Function(List/1, List/2, ..., Args...)`.
	ml_list_t *ArgsList = (ml_list_t *)Args[1];
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, ArgsList->Length, ml_value_t *);
	Partial->Type = MLFunctionPartialT;
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
//>function::partial
// Returns a function equivalent to :mini:`fun(Args...) Function(Values..., Args...)`.
	ml_partial_function_t *Partial = xnew(ml_partial_function_t, Count - 1, ml_value_t *);
	Partial->Type = MLFunctionPartialT;
	Partial->Function = Args[0];
	Partial->Count = Partial->Set = Count - 1;
	for (int I = 1; I < Count; ++I) Partial->Args[I - 1] = Args[I];
	return (ml_value_t *)Partial;
}

static void ML_TYPED_FN(ml_iterate, MLFunctionPartialT, ml_state_t *Caller, ml_partial_function_t *Partial) {
	if (Partial->Set != Partial->Count) ML_ERROR("CallError", "Partial function used with missing arguments");
	return ml_call(Caller, Partial->Function, Partial->Count, Partial->Args);
}

typedef struct {
	ml_type_t *Type;
	ml_value_t *Function;
} ml_argless_function_t;

static void ml_argless_function_call(ml_state_t *Caller, ml_argless_function_t *Argless, int Count, ml_value_t **Args) {
	return ml_call(Caller, Argless->Function, 0, NULL);
}

ML_TYPE(MLFunctionArglessT, (MLFunctionT, MLSequenceT), "argless-function",
//!internal
	.call = (void *)ml_argless_function_call
);

ML_METHOD("/", MLFunctionT) {
//!function
//<Function
//>function
// Returns a function equivalent to :mini:`fun(Args...) Function()`.
	ml_argless_function_t *Argless = new(ml_argless_function_t);
	Argless->Type = MLFunctionArglessT;
	Argless->Function = Args[0];
	return (ml_value_t *)Argless;
}

static void ML_TYPED_FN(ml_iterate, MLFunctionArglessT, ml_state_t *Caller, ml_argless_function_t *Argless) {
	return ml_call(Caller, Argless->Function, 0, NULL);
}

// Tuples //
//!tuple

static long ml_tuple_hash(ml_tuple_t *Tuple, ml_hash_chain_t *Chain) {
	long Hash = 739;
	for (int I = 0; I < Tuple->Size; ++I) Hash = ((Hash << 3) + Hash) + ml_hash_chain(Tuple->Values[I], Chain);
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
	if (!Ref->Size) ML_RETURN(Values);
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

typedef struct {
	ml_state_t Base;
	ml_tuple_t *Functions;
	ml_value_t *Result;
	int Index, Count;
	ml_value_t *Args[];
} ml_tuple_call_t;

static void ml_tuple_call_run(ml_tuple_call_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	ml_tuple_t *Functions = State->Functions;
	int Index = State->Index;
	ml_tuple_set(State->Result, Index, Result);
	if (Index == Functions->Size) ML_RETURN(State->Result);
	State->Index = Index + 1;
	return ml_call(State, Functions->Values[Index], State->Count, State->Args);
}

static void ml_tuple_call(ml_state_t *Caller, ml_tuple_t *Functions, int Count, ml_value_t **Args) {
	if (!Functions->Size) ML_RETURN(ml_tuple(0));
	ml_tuple_call_t *State = xnew(ml_tuple_call_t, Count, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_tuple_call_run;
	State->Functions = Functions;
	State->Result = ml_tuple(Functions->Size);
	State->Index = 1;
	State->Count = Count;
	memcpy(State->Args, Args, Count * sizeof(ml_value_t *));
	return ml_call(State, Functions->Values[0], Count, Args);
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

ML_TYPE(MLTupleT, (MLFunctionT, MLSequenceT), "tuple",
//!tuple
// An immutable tuple of values.
//
// :mini:`(Tuple: tuple)(Arg/1, ..., Arg/n)`
//    Returns :mini:`(Tuple[1](Arg/1, ..., Arg/n), ..., Tuple[k](Arg/1, ..., Arg/n))`
	.hash = (void *)ml_tuple_hash,
	.deref = (void *)ml_tuple_deref,
	.assign = (void *)ml_tuple_assign,
	.call = (void *)ml_tuple_call,
	.Constructor = (ml_value_t *)MLTuple
);

static void ML_TYPED_FN(ml_value_find_all, MLTupleT, ml_tuple_t *Tuple, void *Data, ml_value_find_fn RefFn) {
	if (!RefFn(Data, (ml_value_t *)Tuple, 1)) return;
	for (int I = 0; I < Tuple->Size; ++I) ml_value_find_all(Tuple->Values[I], Data, RefFn);
}

ml_value_t *ml_tuple(size_t Size) {
	ml_tuple_t *Tuple = xnew(ml_tuple_t, Size, ml_value_t *);
	Tuple->Type = MLTupleT;
	Tuple->Size = Size;
	return (ml_value_t *)Tuple;
}

typedef struct {
	ml_state_t Base;
	ml_value_t *Visitor, *Dest;
	ml_value_t **Values;
	ml_value_t *Args[1];
	int Index, Size;
} ml_tuple_visit_t;

static void ml_tuple_visit_run(ml_tuple_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	int Index = State->Index + 1;
	if (Index > State->Size) ML_RETURN(MLNil);
	State->Index = Index;
	State->Args[0] = *++State->Values;
	return ml_call(State, State->Visitor, 1, State->Args);
}

ML_METHODX("visit", MLVisitorT, MLTupleT) {
	ml_visitor_t *Visitor = (ml_visitor_t *)Args[0];
	ml_tuple_t *Source = (ml_tuple_t *)Args[1];
	if (!Source->Size) ML_RETURN(MLNil);
	ml_tuple_visit_t *State = new(ml_tuple_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_tuple_visit_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Index = 1;
	State->Size = Source->Size;
	State->Values = Source->Values;
	State->Args[0] = Source->Values[0];
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

static void ml_tuple_copy_run(ml_tuple_visit_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_tuple_set(State->Dest, State->Index, Value);
	int Index = State->Index + 1;
	if (Index > State->Size) ML_RETURN(State->Dest);
	State->Index = Index;
	State->Args[0] = *++State->Values;
	return ml_call(State, State->Visitor, 1, State->Args);
}

static void ml_tuple_copy(ml_state_t *Caller, ml_visitor_t *Visitor, ml_tuple_t *Source) {
	ml_value_t *Dest = ml_tuple(Source->Size);
	inthash_insert(Visitor->Cache, (uintptr_t)Source, Dest);
	if (!Source->Size) ML_RETURN(Dest);
	ml_tuple_visit_t *State = new(ml_tuple_visit_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_tuple_copy_run;
	State->Visitor = (ml_value_t *)Visitor;
	State->Dest = Dest;
	State->Index = 1;
	State->Size = Source->Size;
	State->Values = Source->Values;
	State->Args[0] = Source->Values[0];
	return ml_call(State, (ml_value_t *)Visitor, 1, State->Args);
}

ML_METHODX("copy", MLVisitorT, MLTupleT) {
//<Copy
//<Tuple
//>tuple
// Returns a new tuple containing copies of the elements of :mini:`Tuple` created using :mini:`Copy`.
	return ml_tuple_copy(Caller, (ml_visitor_t *)Args[0], (ml_tuple_t *)Args[1]);
}

ML_METHODX("const", MLVisitorT, MLTupleT) {
//<Copy
//<Tuple
//>tuple
// Returns a new tuple containing copies of the elements of :mini:`Tuple` created using :mini:`Copy`.
	return ml_tuple_copy(Caller, (ml_visitor_t *)Args[0], (ml_tuple_t *)Args[1]);
}

static int ML_TYPED_FN(ml_value_is_constant, MLTupleT, ml_tuple_t *Tuple) {
	for (int I = 0; I < Tuple->Size; ++I) {
		if (!ml_value_is_constant(Tuple->Values[I])) return 0;
	}
	return 1;
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

ml_value_t *ml_tuplen(size_t Size, ml_value_t **Values) {
	ml_tuple_t *Tuple = xnew(ml_tuple_t, Size, ml_value_t *);
	Tuple->Size = Size;
	ml_type_t *Types[Size + 1];
	Types[0] = MLTupleT;
	for (int I = 0; I < Size; ++I) {
		Tuple->Values[I] = Values[I];
		Types[I + 1] = ml_typeof(Values[I]);
	}
	Tuple->Type = ml_generic_type(Size + 1, Types);
	return (ml_value_t *)Tuple;
}

ml_value_t *ml_tuplev(size_t Size, ...) {
	ml_tuple_t *Tuple = xnew(ml_tuple_t, Size, ml_value_t *);
	Tuple->Size = Size;
	ml_type_t *Types[Size + 1];
	Types[0] = MLTupleT;
	va_list Args;
	va_start(Args, Size);
	for (int I = 0; I < Size; ++I) {
		ml_value_t *Value = va_arg(Args, ml_value_t *);
		Tuple->Values[I] = Value;
		Types[I + 1] = ml_typeof(Value);
	}
	va_end(Args);
	Tuple->Type = ml_generic_type(Size + 1, Types);
	return (ml_value_t *)Tuple;
}

#else

ml_value_t *ml_tuplen(size_t Size, ml_value_t **Values) {
	ml_tuple_t *Tuple = xnew(ml_tuple_t, Size, ml_value_t *);
	Tuple->Type = MLTupleT;
	Tuple->Size = Size;
	for (int I = 0; I < Size; ++I) Tuple->Values[I] = Values[I];
	return (ml_value_t *)Tuple;
}

ml_value_t *ml_tuplev(size_t Size, ...) {
	ml_tuple_t *Tuple = xnew(ml_tuple_t, Size, ml_value_t *);
	Tuple->Type = MLTupleT;
	Tuple->Size = Size;
	va_list Args;
	va_start(Args, Size);
	for (int I = 0; I < Size; ++I) {
		Tuple->Values[I] = va_arg(Args, ml_value_t *);
	}
	va_end(Args);
	return (ml_value_t *)Tuple;
}

#endif

ml_value_t *ml_unpack(ml_value_t *Value, int Index) {
	typeof(ml_unpack) *function = ml_typed_fn_get(ml_typeof(Value), ml_unpack);
	if (!function) return ml_simple_inline(IndexMethod, 2, Value, ml_integer(Index));
	return function(Value, Index);
}

static ml_value_t *ML_TYPED_FN(ml_unpack, MLNilT, ml_value_t *Value, int Index) {
	return MLNil;
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
//!internal

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

ML_METHOD("append", MLStringBufferT, MLTupleT) {
//!tuple
//<Buffer
//<Value
// Appends a representation of :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_tuple_t *Value = (ml_tuple_t *)Args[1];
	ml_stringbuffer_put(Buffer, '(');
	if (Value->Size) {
		ml_stringbuffer_simple_append(Buffer, Value->Values[0]);
		for (int I = 1; I < Value->Size; ++I) {
			ml_stringbuffer_write(Buffer, ", ", 2);
			ml_stringbuffer_simple_append(Buffer, Value->Values[I]);
		}
	}
	ml_stringbuffer_put(Buffer, ')');
	return MLSome;
}

static ml_value_t *ML_TYPED_FN(ml_unpack, MLTupleT, ml_tuple_t *Tuple, int Index) {
	if (Index > Tuple->Size) return MLNil;
	return Tuple->Values[Index - 1];
}

static ml_value_t *ml_tuple_compare(ml_tuple_t *A, ml_tuple_t *B) {
	// TODO: Replace this with a state to remove ml_simple_call
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

typedef struct {
	ml_state_t Base;
	ml_value_t *Result, *Order, *Default;
	ml_value_t **A, **B;
	ml_value_t *Args[2];
	int Count;
} ml_tuple_compare_state_t;

static void ml_tuple_compare_equal_run(ml_tuple_compare_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (Result == MLNil) ML_RETURN(State->Result);
	if (--State->Count == 0) ML_RETURN(State->Default);
	State->Args[0] = *++State->A;
	State->Args[1] = *++State->B;
	return ml_call(State, EqualMethod, 2, State->Args);
}

ML_METHODX("=", MLTupleT, MLTupleT) {
//<A
//<B
//>B | nil
// Returns :mini:`B` if :mini:`A:size = B:size` and :mini:`A/i = B/i` for each :mini:`i`.
//$= =((1, 2, 3), (1, 2, 3))
//$= =((1, 2, 3), (1, 2))
//$= =((1, 2), (1, 2, 3))
//$= =((1, 2, 3), (1, 2, 4))
//$= =((1, 3, 2), (1, 2, 3))
	ml_tuple_t *A = (ml_tuple_t *)Args[0];
	ml_tuple_t *B = (ml_tuple_t *)Args[1];
	if (A->Size != B->Size) ML_RETURN(MLNil);
	if (!A->Size) ML_RETURN(B);
	ml_tuple_compare_state_t *State = new(ml_tuple_compare_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_tuple_compare_equal_run;
	State->Result = MLNil;
	State->Default = (ml_value_t *)B;
	State->Count = A->Size;
	State->A = A->Values;
	State->B = B->Values;
	State->Args[0] = A->Values[0];
	State->Args[1] = B->Values[0];
	return ml_call(State, EqualMethod, 2, State->Args);
}

ML_METHODX("!=", MLTupleT, MLTupleT) {
//<A
//<B
//>B | nil
// Returns :mini:`B` if :mini:`A:size != B:size` or :mini:`A/i != B/i` for some :mini:`i`.
//$= !=((1, 2, 3), (1, 2, 3))
//$= !=((1, 2, 3), (1, 2))
//$= !=((1, 2), (1, 2, 3))
//$= !=((1, 2, 3), (1, 2, 4))
//$= !=((1, 3, 2), (1, 2, 3))
	ml_tuple_t *A = (ml_tuple_t *)Args[0];
	ml_tuple_t *B = (ml_tuple_t *)Args[1];
	if (A->Size != B->Size) ML_RETURN(B);
	if (!A->Size) ML_RETURN(MLNil);
	ml_tuple_compare_state_t *State = new(ml_tuple_compare_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_tuple_compare_equal_run;
	State->Result = (ml_value_t *)B;
	State->Default = MLNil;
	State->Count = A->Size;
	State->A = A->Values;
	State->B = B->Values;
	State->Args[0] = A->Values[0];
	State->Args[1] = B->Values[0];
	return ml_call(State, EqualMethod, 2, State->Args);
}

static void ml_tuple_compare_order_run(ml_tuple_compare_state_t *State, ml_value_t *Result);

static void ml_tuple_compare_order2_run(ml_tuple_compare_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (Result == MLNil) ML_RETURN(MLNil);
	if (--State->Count == 0) ML_RETURN(State->Default);
	State->Args[0] = *++State->A;
	State->Args[1] = *++State->B;
	State->Base.run = (ml_state_fn)ml_tuple_compare_order_run;
	return ml_call(State, State->Order, 2, State->Args);
}

static void ml_tuple_compare_order_run(ml_tuple_compare_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (Result != MLNil) ML_RETURN(State->Result);
	State->Args[0] = *State->A;
	State->Args[1] = *State->B;
	State->Base.run = (ml_state_fn)ml_tuple_compare_order2_run;
	return ml_call(State, EqualMethod, 2, State->Args);
}

ML_METHODX("<", MLTupleT, MLTupleT) {
//<A
//<B
//>B | nil
// Returns :mini:`B` if :mini:`A/i = B/i` for each :mini:`i = 1 .. j-1` and :mini:`A/j < B/j`.
//$= <((1, 2, 3), (1, 2, 3))
//$= <((1, 2, 3), (1, 2))
//$= <((1, 2), (1, 2, 3))
//$= <((1, 2, 3), (1, 2, 4))
//$= <((1, 3, 2), (1, 2, 3))
	ml_tuple_t *A = (ml_tuple_t *)Args[0];
	ml_tuple_t *B = (ml_tuple_t *)Args[1];
	if (!A->Size) {
		if (!B->Size) ML_RETURN(MLNil);
		ML_RETURN(B);
	}
	if (!B->Size) ML_RETURN(MLNil);
	ml_tuple_compare_state_t *State = new(ml_tuple_compare_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_tuple_compare_order_run;
	State->Result = (ml_value_t *)B;
	State->Order = LessMethod;
	if (A->Size >= B->Size) {
		State->Default = MLNil;
		State->Count = B->Size;
	} else {
		State->Default = (ml_value_t *)B;
		State->Count = A->Size;
	}
	State->A = A->Values;
	State->B = B->Values;
	State->Args[0] = A->Values[0];
	State->Args[1] = B->Values[0];
	return ml_call(State, LessMethod, 2, State->Args);
}

ML_METHODX("<=", MLTupleT, MLTupleT) {
//<A
//<B
//>B | nil
// Returns :mini:`B` if :mini:`A/i = B/i` for each :mini:`i = 1 .. j-1` and :mini:`A/j <= B/j`.
//$= <=((1, 2, 3), (1, 2, 3))
//$= <=((1, 2, 3), (1, 2))
//$= <=((1, 2), (1, 2, 3))
//$= <=((1, 2, 3), (1, 2, 4))
//$= <=((1, 3, 2), (1, 2, 3))
	ml_tuple_t *A = (ml_tuple_t *)Args[0];
	ml_tuple_t *B = (ml_tuple_t *)Args[1];
	if (!A->Size) {
		if (!B->Size) ML_RETURN(B);
		ML_RETURN(B);
	}
	if (!B->Size) ML_RETURN(MLNil);
	ml_tuple_compare_state_t *State = new(ml_tuple_compare_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_tuple_compare_order_run;
	State->Result = (ml_value_t *)B;
	State->Order = LessMethod;
	if (A->Size > B->Size) {
		State->Default = MLNil;
		State->Count = B->Size;
	} else {
		State->Default = (ml_value_t *)B;
		State->Count = A->Size;
	}
	State->A = A->Values;
	State->B = B->Values;
	State->Args[0] = A->Values[0];
	State->Args[1] = B->Values[0];
	return ml_call(State, LessMethod, 2, State->Args);
}

ML_METHODX(">", MLTupleT, MLTupleT) {
//<A
//<B
//>B | nil
// Returns :mini:`B` if :mini:`A/i = B/i` for each :mini:`i = 1 .. j-1` and :mini:`A/j > B/j`.
//$= >((1, 2, 3), (1, 2, 3))
//$= >((1, 2, 3), (1, 2))
//$= >((1, 2), (1, 2, 3))
//$= >((1, 2, 3), (1, 2, 4))
//$= >((1, 3, 2), (1, 2, 3))
	ml_tuple_t *A = (ml_tuple_t *)Args[0];
	ml_tuple_t *B = (ml_tuple_t *)Args[1];
	if (!A->Size) {
		if (!B->Size) ML_RETURN(MLNil);
		ML_RETURN(MLNil);
	}
	if (!B->Size) ML_RETURN(A);
	ml_tuple_compare_state_t *State = new(ml_tuple_compare_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_tuple_compare_order_run;
	State->Result = (ml_value_t *)B;
	State->Order = GreaterMethod;
	if (A->Size <= B->Size) {
		State->Default = MLNil;
		State->Count = A->Size;
	} else {
		State->Default = (ml_value_t *)B;
		State->Count = B->Size;
	}
	State->A = A->Values;
	State->B = B->Values;
	State->Args[0] = A->Values[0];
	State->Args[1] = B->Values[0];
	return ml_call(State, GreaterMethod, 2, State->Args);
}

ML_METHODX(">=", MLTupleT, MLTupleT) {
//<A
//<B
//>B | nil
// Returns :mini:`B` if :mini:`A/i = B/i` for each :mini:`i = 1 .. j-1` and :mini:`A/j >= B/j`.
//$= >=((1, 2, 3), (1, 2, 3))
//$= >=((1, 2, 3), (1, 2))
//$= >=((1, 2), (1, 2, 3))
//$= >=((1, 2, 3), (1, 2, 4))
//$= >=((1, 3, 2), (1, 2, 3))
	ml_tuple_t *A = (ml_tuple_t *)Args[0];
	ml_tuple_t *B = (ml_tuple_t *)Args[1];
	if (!A->Size) {
		if (!B->Size) ML_RETURN(B);
		ML_RETURN(MLNil);
	}
	if (!B->Size) ML_RETURN(B);
	ml_tuple_compare_state_t *State = new(ml_tuple_compare_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_tuple_compare_order_run;
	State->Result = (ml_value_t *)B;
	State->Order = GreaterMethod;
	if (A->Size < B->Size) {
		State->Default = MLNil;
		State->Count = A->Size;
	} else {
		State->Default = (ml_value_t *)B;
		State->Count = B->Size;
	}
	State->A = A->Values;
	State->B = B->Values;
	State->Args[0] = A->Values[0];
	State->Args[1] = B->Values[0];
	return ml_call(State, GreaterMethod, 2, State->Args);
}

// Boolean //
//!boolean

static long ml_boolean_hash(ml_boolean_t *Boolean, ml_hash_chain_t *Chain) {
	return (long)Boolean;
}

ML_TYPE(MLBooleanT, (), "boolean",
//!boolean
// A boolean value (either :mini:`true` or :mini:`false`).
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

static int ML_TYPED_FN(ml_value_is_constant, MLBooleanT, ml_value_t *Value) {
	return 1;
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
//$= true /\ true
//$= true /\ false
//$= false /\ true
//$= false /\ false
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
//$= true \/ true
//$= true \/ false
//$= false \/ true
//$= false \/ false
	int Result = ml_boolean_value(Args[0]);
	for (int I = 1; I < Count; ++I) Result |= ml_boolean_value(Args[I]);
	return MLBooleans[Result];
}

ML_METHOD("><", MLBooleanT, MLBooleanT) {
//!boolean
//<Bool/1
//<Bool/2
//>boolean
// Returns the logical xor of :mini:`Bool/1` and :mini:`Bool/2`.
//$= true >< true
//$= true >< false
//$= false >< true
//$= false >< false
	int Result = ml_boolean_value(Args[0]) != ml_boolean_value(Args[1]);
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
ML_METHOD(#NAME, MLBooleanT, MLBooleanT) { \
/*>boolean|nil
// Returns :mini:`Arg/2` if :mini:`Arg/1 SYMBOL Arg/2` and :mini:`nil` otherwise.
//$= true NAME true
//$= true NAME false
//$= false NAME true
//$= false NAME false
*/\
	ml_boolean_t *BooleanA = (ml_boolean_t *)Args[0]; \
	ml_boolean_t *BooleanB = (ml_boolean_t *)Args[1]; \
	return BooleanA->Value SYMBOL BooleanB->Value ? Args[1] : MLNil; \
}

ml_comp_method_boolean_boolean(=, ==);
ml_comp_method_boolean_boolean(!=, !=);
ml_comp_method_boolean_boolean(<, <);
ml_comp_method_boolean_boolean(>, >);
ml_comp_method_boolean_boolean(<=, <=);
ml_comp_method_boolean_boolean(>=, >=);

ML_FUNCTION(RandomBoolean) {
//@boolean::random
//<P?:number
//>boolean
// Returns a random boolean that has probability :mini:`P` of being :mini:`true`. If omitted, :mini:`P` defaults to :mini:`0.5`.
	int Threshold;
	if (Count == 1) {
		ML_CHECK_ARG_TYPE(0, MLRealT);
		Threshold = RAND_MAX * ml_real_value(Args[0]);
	} else {
		Threshold = RAND_MAX / 2;
	}
	return (ml_value_t *)(random() > Threshold ? MLFalse : MLTrue);
}

// Modules //
//!module

ML_TYPE(MLModuleT, (), "module");

ML_METHODX("::", MLModuleT, MLStringT) {
//<Module
//<Name
//>MLAnyT
// Imports a symbol from a module.
	ml_module_t *Module = (ml_module_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t *Value = stringmap_search(Module->Exports, Name);
	if (!Value) {
		ML_ERROR("ModuleError", "Symbol %s not exported from module %s", Name, Module->Path);
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

ML_METHOD("append", MLStringBufferT, MLModuleT) {
//!module
//<Buffer
//<Value
// Appends a representation of :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_module_t *Module = (ml_module_t *)Args[1];
	ml_stringbuffer_printf(Buffer, "module(%s)", Module->Path);
	return MLSome;
}

static int ml_module_exports_fn(const char *Name, void *Value, ml_value_t *Exports) {
	ml_map_insert(Exports, ml_string(Name, -1), Value);
	return 0;
}

ML_METHOD("path", MLModuleT) {
	ml_module_t *Module = (ml_module_t *)Args[0];
	return ml_string(Module->Path, -1);
}

ML_METHOD("exports", MLModuleT) {
	ml_module_t *Module = (ml_module_t *)Args[0];
	ml_value_t *Exports = ml_map();
	stringmap_foreach(Module->Exports, Exports, (void *)ml_module_exports_fn);
	return Exports;
}

// Externals //
//!external

ml_value_t *ml_external(const char *Name) {
	ml_external_t *External = new(ml_external_t);
	External->Type = MLExternalT;
	External->Name = Name;
	External->Length = strlen(Name);
	return (ml_value_t *)External;
}

ML_FUNCTION(MLExternal) {
//@external
//<Name
//>external
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	return ml_external(ml_string_value(Args[0]));
}

ML_TYPE(MLExternalT, (), "external",
// A placeholder value that can be encoded and replaced on decoding.
	.Constructor = (ml_value_t *)MLExternal
);

ML_METHOD("::", MLExternalT, MLStringT) {
//<External
//<Import
//>external
	ml_external_t *External = (ml_external_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	ml_value_t **Slot = (ml_value_t **)stringmap_slot(External->Exports, Name);
	if (!Slot[0]) {
		char *FullName = snew(External->Length + strlen(Name) + 3);
		stpcpy(stpcpy(stpcpy(FullName, External->Name), "::"), Name);
		Slot[0] = ml_external(FullName);
	}
	return Slot[0];
}

ML_TYPE(MLExternalSetT, (), "externals");

ML_METHOD(MLExternalSetT) {
//@external::set
//>external::set
	ml_externals_t *Externals = new(ml_externals_t);
	Externals->Type = MLExternalSetT;
	Externals->Next = MLExternals;
	return (ml_value_t *)Externals;
}

ML_METHOD("add", MLExternalSetT, MLStringT, MLAnyT) {
//<Externals
//<Name
//<Value
	ml_externals_t *Externals = (ml_externals_t *)Args[0];
	const char *Name = ml_string_value(Args[1]);
	stringmap_insert(Externals->Names, Name, Args[2]);
	inthash_insert(Externals->Values, (uintptr_t)Args[1], (void *)Name);
	return MLNil;
}

ml_externals_t MLExternals[1] = {MLExternalSetT, NULL, {INTHASH_INIT}, {STRINGMAP_INIT}};

const char *ml_externals_get_name(ml_externals_t *Externals, ml_value_t *Value) {
	while (Externals) {
		const char *Name = (const char *)inthash_search(Externals->Values,  (uintptr_t)Value);
		if (Name) return Name;
		Externals = Externals->Next;
	}
	return NULL;
}

ml_value_t *ml_externals_get_value(ml_externals_t *Externals, const char *Name) {
	while (Externals) {
		ml_value_t *Value = stringmap_search(Externals->Names, Name);
		if (Value) return Value;
		Externals = Externals->Next;
	}
	return NULL;
}

void ml_externals_add(const char *Name, void *Value) {
	stringmap_insert(MLExternals->Names, Name, Value);
	inthash_insert(MLExternals->Values, (uintptr_t)Value, (void *)Name);
}

ML_FUNCTION(MLExternalGet) {
//@external::get
//<Name
//>any|error
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Name = ml_string_value(Args[0]);
	ml_value_t *Value = (ml_value_t *)stringmap_search(MLExternals->Names, Name);
	if (Value) return Value;
	return ml_error("NameError", "External %s not defined", Name);
}

ML_FUNCTION(MLExternalAdd) {
//@external::add
//<Name
//<Value
	ML_CHECK_ARG_COUNT(2);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Name = ml_string_value(Args[0]);
	stringmap_insert(MLExternals->Names, Name, Args[1]);
	inthash_insert(MLExternals->Values, (uintptr_t)Args[1], (void *)Name);
	return MLNil;
}

// Symbols //
//!symbol

static void ml_symbol_call(ml_state_t *Caller, ml_symbol_t *Symbol, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t **Args2 = ml_alloc_args(2);
	Args2[0] = Args[0];
	Args2[1] = ml_string(Symbol->Name, -1);
	return ml_call(Caller, SymbolMethod, 2, Args2);
}

ML_TYPE(MLSymbolT, (MLFunctionT), "symbol",
	.call = (void *)ml_symbol_call
);

ml_value_t *ml_symbol(const char *Name) {
	ml_symbol_t *Symbol = new(ml_symbol_t);
	Symbol->Type = MLSymbolT;
	Symbol->Name = Name;
	return (ml_value_t *)Symbol;
}

ML_TYPE(MLSymbolRangeT, (), "symbol::range");

ML_METHOD("..", MLSymbolT, MLSymbolT) {
	ml_symbol_range_t *Range = new(ml_symbol_range_t);
	Range->Type = MLSymbolRangeT;
	Range->First = ml_symbol_name(Args[0]);
	Range->Last = ml_symbol_name(Args[1]);
	return (ml_value_t *)Range;
}

// Init //
//!general

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
//<Var/1...:any
//<Var/n:any
// Assigns :mini:`Var/i := Var/i/+/1` for each :mini:`1 <= i < n` and :mini:`Var/n := Var/1`.
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
//<Var/1...:any
//<Var/n:any
//<Value:any
// Assigns :mini:`Var/i := Var/i/+/1` for each :mini:`1 <= i < n` and :mini:`Var/n := Value`. Returns the old value of :mini:`Var/1`.
	ML_CHECKX_ARG_COUNT(2);
	ml_exchange_t *State = xnew(ml_exchange_t, Count - 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_exchange_run;
	memcpy(State->Args, Args, (Count - 1) * sizeof(ml_value_t *));
	State->New = ml_deref(Args[Count - 1]);
	State->Index = Count - 1;
	return ml_exchange_run(State, MLNil);
}

ML_FUNCTION(MLDeref) {
//@deref
//<Value:any
//>any
// Returns the dereferenced value of :mini:`Value`.
	return Args[0];
}

ML_FUNCTIONZ(MLAssign) {
//@assign
//<Var:any
//<Value:any
//>any
// Functional equivalent of :mini:`Var := Value`.
	ML_CHECKX_ARG_COUNT(2);
	return ml_assign(Caller, Args[0], Args[1]);
}

ML_FUNCTIONZ(MLCall) {
//@call
//<Fn:any
//<Arg/1...:any
//<Arg/n:any
//>any
// Returns :mini:`Fn(Arg/1, ..., Arg/n)`.
	ML_CHECKX_ARG_COUNT(1);
	return ml_call(Caller, Args[0], Count - 1, Args + 1);
}

ML_FUNCTIONZ(MLCompareAndSet) {
//@cas
//<Var:any
//<Old:any
//<New:any
//>any
// If the value of :mini:`Var` is *identically* equal to :mini:`Old`, then sets :mini:`Var` to :mini:`New` and returns :mini:`New`. Otherwise leaves :mini:`Var` unchanged and returns :mini:`nil`.
//$- var X := 10
//$= cas(X, 10, 11)
//$= X
//$= cas(X, 20, 21)
//$= X
	ML_CHECKX_ARG_COUNT(3);
	ml_value_t *Var = ml_deref(Args[0]);
	ml_value_t *Old = ml_deref(Args[1]);
	ml_value_t *New = ml_deref(Args[2]);
	if (Var != Old) ML_RETURN(MLNil);
	return ml_assign(Caller, Args[0], New);
}

static ml_value_t *ml_mem_trace(void *Ptr, inthash_t *Cache) {
	void **Base = (void **)GC_base(Ptr);
	if (!Base) return NULL;
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
		ml_value_t *Field = ml_mem_trace(Base[I], Cache);
		if (Field) ml_map_insert(Fields, ml_integer(I), Field);
	}
	return Trace;
}

ML_FUNCTION(MLMemTrace) {
//!memory
//@trace
//<Value
//>list[map]
// Returns information about the blocks of memory referenced by :mini:`Value`.
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Value = Args[0];
	inthash_t Cache[1] = {INTHASH_INIT};
	return ml_mem_trace(Value, Cache) ?: MLNil;
}

static size_t ml_mem_size(void *Ptr, inthash_t *Cache) {
	void **Base = (void **)GC_base(Ptr);
	if (!Base) return 0;
	if (inthash_search(Cache, (uintptr_t)Base)) return 0;
	inthash_insert(Cache, (uintptr_t)Base, Base);
	size_t Size = GC_size(Base);
	int Count = (Size + sizeof(void *) - 1) / sizeof(void *);
	for (int I = Count; --I >= 0;) Size += ml_mem_size(*Base++, Cache);
	return Size;
}

ML_FUNCTION(MLMemSize) {
//!memory
//@size
//<Value
//>list[map]
// Returns information about the blocks of memory referenced by :mini:`Value`.
	ML_CHECK_ARG_COUNT(1);
	ml_value_t *Value = Args[0];
	inthash_t Cache[1] = {INTHASH_INIT};
	return ml_integer(ml_mem_size(Value, Cache));
}

ML_FUNCTION(MLMemCollect) {
//!memory
//@collect
// Call garbage collector.
	GC_gcollect();
	return MLNil;
}

void ml_init(stringmap_t *Globals) {
#ifdef ML_JIT
	GC_set_pages_executable(1);
#endif
	GC_INIT();
	ml_method_init();
#include "ml_types_init.c"
#ifdef ML_GENERICS
	ml_type_add_rule(MLTupleT, MLSequenceT, MLIntegerT, MLAnyT, NULL);
	ml_type_add_rule(MLTupleT, MLFunctionT, MLTupleT, NULL);
#endif
	stringmap_insert(MLTypeT->Exports, "switch", MLTypeSwitch);
	stringmap_insert(MLAnyT->Exports, "switch", MLAnySwitch);
#ifdef ML_COMPLEX
	stringmap_insert(MLCompilerT->Exports, "i", ml_complex(1i));
#endif
	stringmap_insert(MLBooleanT->Exports, "random", RandomBoolean);
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
	ml_number_init();
	ml_string_init();
	ml_list_init();
	ml_map_init();
	ml_set_init();
	ml_compiler_init();
	ml_runtime_init();
	ml_bytecode_init();
	stringmap_insert(MLExternalT->Exports, "set", MLExternalSetT);
	stringmap_insert(MLExternalT->Exports, "get", MLExternalGet);
	stringmap_insert(MLExternalT->Exports, "add", MLExternalAdd);
	ml_externals_add("type", MLTypeT);
	ml_externals_add("function", MLFunctionT);
	ml_externals_add("method", MLMethodT);
	ml_externals_add("any", MLAnyT);
	ml_externals_add("some", MLSome);
	ml_externals_add("integer", MLIntegerT);
	ml_externals_add("real", MLRealT);
	ml_externals_add("number", MLNumberT);
	ml_externals_add("string", MLStringT);
	ml_externals_add("list", MLListT);
	ml_externals_add("tuple", MLTupleT);
	ml_externals_add("map", MLMapT);
	ml_externals_add("set", MLSetT);
	ml_externals_add("boolean", MLBooleanT);
	ml_externals_add("error", MLErrorT);
	ml_externals_add("regex", MLRegexT);
#ifdef ML_COMPLEX
	ml_externals_add("complex", MLComplexT);
#endif
	ml_externals_add("method", MLMethodT);
	ml_externals_add("address", MLAddressT);
	ml_externals_add("buffer", MLBufferT);
	ml_externals_add("tuple", MLTupleT);
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
		stringmap_insert(Globals, "address", MLAddressT);
		stringmap_insert(Globals, "buffer", MLBufferT);
		stringmap_insert(Globals, "string", MLStringT);
		stringmap_insert(Globals, "regex", MLRegexT);
		stringmap_insert(Globals, "tuple", MLTupleT);
		stringmap_insert(Globals, "list", MLListT);
		stringmap_insert(Globals, "map", MLMapT);
		stringmap_insert(Globals, "set", MLSetT);
		stringmap_insert(Globals, "external", MLExternalT);
		stringmap_insert(Globals, "error", MLErrorValueT);
		stringmap_insert(Globals, "module", MLModuleT);
		stringmap_insert(Globals, "some", MLSome);
		stringmap_insert(Globals, "deref", MLDeref);
		stringmap_insert(Globals, "assign", MLAssign);
		stringmap_insert(Globals, "call", MLCall);
		stringmap_insert(Globals, "visit", MLVisit);
		stringmap_insert(Globals, "copy", MLCopy);
		stringmap_insert(Globals, "findall", MLFindAll);
		stringmap_insert(Globals, "isconstant", MLIsConstant);
		stringmap_insert(Globals, "exchange", MLExchange);
		stringmap_insert(Globals, "replace", MLReplace);
		stringmap_insert(Globals, "cas", MLCompareAndSet);
	}
}

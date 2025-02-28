#include "minilang.h"
#include "ml_macros.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

#undef ML_CATEGORY
#define ML_CATEGORY "tuple"

extern ml_value_t *EqualMethod;
extern ml_value_t *LessMethod;
extern ml_value_t *GreaterMethod;
extern ml_value_t *IndexMethod;
extern ml_value_t *CompareMethod;

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
	ml_tuple_t *Tuple;
	int Index, Count;
	ml_value_t *Args[];
} ml_tuple_call_t;

#ifdef ML_GENERICS

static __attribute__ ((noinline)) void ml_tuple_call_finish(ml_tuple_t *Tuple) {
	ml_type_t *Types[Tuple->Size + 1];
	Types[0] = MLTupleT;
	for (int I = 0; I < Tuple->Size; ++I) Types[I + 1] = ml_typeof(Tuple->Values[I]);
	Tuple->Type = ml_generic_type(Tuple->Size + 1, Types);
}

#endif

static void ml_tuple_call_run(ml_tuple_call_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	int Index = State->Index;
	ml_tuple_t *Tuple = State->Tuple;
	Tuple->Values[Index] = Result;
	ml_tuple_t *Functions = State->Functions;
	if (++Index == Functions->Size) {
#ifdef ML_GENERICS
		ml_tuple_call_finish(Tuple);
#endif
		ML_RETURN(Tuple);
	}
	State->Index = Index;
	return ml_call(State, Functions->Values[Index], State->Count, State->Args);
}

static void ml_tuple_call(ml_state_t *Caller, ml_tuple_t *Functions, int Count, ml_value_t **Args) {
	if (!Functions->Size) ML_RETURN(ml_tuple(0));
	ml_tuple_call_t *State = xnew(ml_tuple_call_t, Count, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (void *)ml_tuple_call_run;
	State->Functions = Functions;
	State->Tuple = (ml_tuple_t *)ml_tuple(Functions->Size);
	State->Index = 0;
	State->Count = Count;
	memcpy(State->Args, Args, Count * sizeof(ml_value_t *));
	return ml_call(State, Functions->Values[0], Count, Args);
}

ML_FUNCTION(MLTuple) {
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

static void ML_TYPED_FN(ml_value_sha256, MLTupleT, ml_tuple_t *Value, ml_hash_chain_t *Chain, unsigned char Hash[SHA256_BLOCK_SIZE]) {
	SHA256_CTX Ctx[1];
	sha256_init(Ctx);
	sha256_update(Ctx, (unsigned char *)"tuple", strlen("tuple"));
	for (int I = 0; I < Value->Size; ++I)  {
		unsigned char Hash[SHA256_BLOCK_SIZE];
		ml_value_sha256(Value->Values[I], Chain, Hash);
		sha256_update(Ctx, Hash, SHA256_BLOCK_SIZE);
	}
	sha256_final(Ctx, Hash);
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
	if (function) return function(Value, Index);
	Value = ml_deref(Value);
	function = ml_typed_fn_get(ml_typeof(Value), ml_unpack);
	if (function) return function(Value, Index);
	return ml_simple_inline(IndexMethod, 2, Value, ml_integer(Index));
}

static ml_value_t *ML_TYPED_FN(ml_unpack, MLNilT, ml_value_t *Value, int Index) {
	return MLNil;
}

ML_METHOD("size", MLTupleT) {
//<Tuple
//>integer
// Returns the number of elements in :mini:`Tuple`.
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	return ml_integer(Tuple->Size);
}

ML_METHOD("[]", MLTupleT, MLIntegerT) {
//<Tuple
//<Index
//>any | error
// Returns the :mini:`Index`-th element in :mini:`Tuple` or an error if :mini:`Index` is out of interval.
// Indexing starts at :mini:`1`. Negative indices count from the end, with :mini:`-1` returning the last element.
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[0];
	long Index = ml_integer_value_fast(Args[1]);
	if (--Index < 0) Index += Tuple->Size + 1;
	if (Index < 0 || Index >= Tuple->Size) return ml_error("IntervalError", "Tuple index out of bounds");
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

typedef struct {
	ml_state_t Base;
	ml_stringbuffer_t *Buffer;
	ml_value_t **Values;
	ml_value_t *Args[2];
	ml_hash_chain_t Chain[1];
	const char *Seperator;
	const char *Terminator;
	size_t SeperatorLength;
	size_t TerminatorLength;
	size_t Index, Size;
} ml_tuple_append_state_t;

extern ml_value_t *AppendMethod;

static void ml_tuple_append_state_run(ml_tuple_append_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	if (++State->Index == State->Size) {
		ml_stringbuffer_write(State->Buffer, State->Terminator, State->TerminatorLength);
		if (State->Chain->Index) ml_stringbuffer_printf(State->Buffer, "<%d", State->Chain->Index);
		State->Buffer->Chain = State->Chain->Previous;
		ML_CONTINUE(State->Base.Caller, MLSome);
	}
	ml_stringbuffer_write(State->Buffer, State->Seperator, State->SeperatorLength);
	State->Args[1] = State->Values[State->Index];
	return ml_call(State, AppendMethod, 2, State->Args);
}

ML_METHODX("append", MLStringBufferT, MLTupleT) {
//<Buffer
//<Value
// Appends a representation of :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[1];
	for (ml_hash_chain_t *Link = Buffer->Chain; Link; Link = Link->Previous) {
		if (Link->Value == (ml_value_t *)Tuple) {
			int Index = Link->Index;
			if (!Index) Index = Link->Index = ++Buffer->Index;
			ml_stringbuffer_printf(Buffer, ">%d", Index);
			ML_RETURN(Buffer);
		}
	}
	if (!Tuple->Size) {
		ml_stringbuffer_write(Buffer, "()", 2);
		ML_RETURN(MLSome);
	}
	ml_stringbuffer_put(Buffer, '(');
	ml_tuple_append_state_t *State = new(ml_tuple_append_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_tuple_append_state_run;
	State->Chain->Previous = Buffer->Chain;
	State->Chain->Value = (ml_value_t *)Tuple;
	Buffer->Chain = State->Chain;
	State->Buffer = Buffer;
	State->Values = Tuple->Values;
	State->Size = Tuple->Size;
	State->Index = 0;
	State->Seperator = ", ";
	State->SeperatorLength = strlen(", ");
	State->Terminator = ")";
	State->TerminatorLength = strlen(")");
	State->Args[0] = (ml_value_t *)Buffer;
	State->Args[1] = Tuple->Values[0];
	return ml_call(State, AppendMethod, 2, State->Args);
}

ML_METHODX("append", MLStringBufferT, MLTupleT, MLStringT) {
//<Buffer
//<Value
// Appends a representation of :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_tuple_t *Tuple = (ml_tuple_t *)Args[1];
	for (ml_hash_chain_t *Link = Buffer->Chain; Link; Link = Link->Previous) {
		if (Link->Value == (ml_value_t *)Tuple) {
			int Index = Link->Index;
			if (!Index) Index = Link->Index = ++Buffer->Index;
			ml_stringbuffer_printf(Buffer, ">%d", Index);
			ML_RETURN(Buffer);
		}
	}
	if (!Tuple->Size) ML_RETURN(MLSome);
	ml_tuple_append_state_t *State = new(ml_tuple_append_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_tuple_append_state_run;
	State->Chain->Previous = Buffer->Chain;
	State->Chain->Value = (ml_value_t *)Tuple;
	Buffer->Chain = State->Chain;
	State->Buffer = Buffer;
	State->Values = Tuple->Values;
	State->Size = Tuple->Size;
	State->Index = 0;
	State->Seperator = ml_string_value(Args[2]);
	State->SeperatorLength = ml_string_length(Args[2]);
	State->Terminator = "";
	State->TerminatorLength = 0;
	State->Args[0] = (ml_value_t *)Buffer;
	State->Args[1] = Tuple->Values[0];
	return ml_call(State, AppendMethod, 2, State->Args);
}

static ml_value_t *ML_TYPED_FN(ml_unpack, MLTupleT, ml_tuple_t *Tuple, int Index) {
	if (Index > Tuple->Size) return MLNil;
	return Tuple->Values[Index - 1];
}

#ifdef ML_NANBOXING

#define NegOne ml_integer32(-1)
#define One ml_integer32(1)
#define Zero ml_integer32(0)

#else

static ml_integer_t One[1] = {{MLIntegerT, 1}};
static ml_integer_t NegOne[1] = {{MLIntegerT, -1}};
static ml_integer_t Zero[1] = {{MLIntegerT, 0}};

#endif

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
//<Tuple/1
//<Tuple/2
//>integer
// Returns :mini:`-1`, :mini:`0` or :mini:`1` depending on whether :mini:`Tuple/1` is less than, equal to or greater than :mini:`Tuple/2` using lexicographical ordering.
	return ml_tuple_compare((ml_tuple_t *)Args[0], (ml_tuple_t *)Args[1]);
}

ML_TYPE(MLComparisonStateT, (MLStateT), "comparison_state");
//!internal

typedef struct {
	ml_comparison_state_t Base;
	ml_value_t *Result, *Order, *Default;
	ml_value_t **A, **B;
	ml_value_t *Args[2];
	int Count;
} ml_tuple_compare_state_t;

static void ml_tuple_compare_equal_run(ml_tuple_compare_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Base.Caller;
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
	for (ml_state_t *State = Caller; State && State->Type == MLComparisonStateT; State = State->Caller) {
		ml_comparison_state_t *Previous = (ml_comparison_state_t *)State;
		if (Previous->A == (ml_value_t *)A && Previous->B == (ml_value_t *)B) ML_RETURN(B);
	}
	if (A->Size != B->Size) ML_RETURN(MLNil);
	if (!A->Size) ML_RETURN(B);
	ml_tuple_compare_state_t *State = new(ml_tuple_compare_state_t);
	State->Base.Base.Type = MLComparisonStateT;
	State->Base.Base.Caller = Caller;
	State->Base.Base.Context = Caller->Context;
	State->Base.Base.run = (ml_state_fn)ml_tuple_compare_equal_run;
	State->Base.A = (ml_value_t *)A;
	State->Base.B = (ml_value_t *)B;
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
	for (ml_state_t *State = Caller; State && State->Type == MLComparisonStateT; State = State->Caller) {
		ml_comparison_state_t *Previous = (ml_comparison_state_t *)State;
		if (Previous->A == (ml_value_t *)A && Previous->B == (ml_value_t *)B) ML_RETURN(MLNil);
	}
	if (A->Size != B->Size) ML_RETURN(B);
	if (!A->Size) ML_RETURN(MLNil);
	ml_tuple_compare_state_t *State = new(ml_tuple_compare_state_t);
	State->Base.Base.Type = MLComparisonStateT;
	State->Base.Base.Caller = Caller;
	State->Base.Base.Context = Caller->Context;
	State->Base.Base.run = (ml_state_fn)ml_tuple_compare_equal_run;
	State->Base.A = (ml_value_t *)A;
	State->Base.B = (ml_value_t *)B;
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
	ml_state_t *Caller = State->Base.Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (Result == MLNil) ML_RETURN(MLNil);
	if (--State->Count == 0) ML_RETURN(State->Default);
	State->Args[0] = *++State->A;
	State->Args[1] = *++State->B;
	State->Base.Base.run = (ml_state_fn)ml_tuple_compare_order_run;
	return ml_call(State, State->Order, 2, State->Args);
}

static void ml_tuple_compare_order_run(ml_tuple_compare_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (Result != MLNil) ML_RETURN(State->Result);
	State->Args[0] = *State->A;
	State->Args[1] = *State->B;
	State->Base.Base.run = (ml_state_fn)ml_tuple_compare_order2_run;
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
	for (ml_state_t *State = Caller; State && State->Type == MLComparisonStateT; State = State->Caller) {
		ml_comparison_state_t *Previous = (ml_comparison_state_t *)State;
		if (Previous->A == (ml_value_t *)A && Previous->B == (ml_value_t *)B) ML_RETURN(MLNil);
	}
	if (!A->Size) {
		if (!B->Size) ML_RETURN(MLNil);
		ML_RETURN(B);
	}
	if (!B->Size) ML_RETURN(MLNil);
	ml_tuple_compare_state_t *State = new(ml_tuple_compare_state_t);
	State->Base.Base.Type = MLComparisonStateT;
	State->Base.Base.Caller = Caller;
	State->Base.Base.Context = Caller->Context;
	State->Base.Base.run = (ml_state_fn)ml_tuple_compare_order_run;
	State->Base.A = (ml_value_t *)A;
	State->Base.B = (ml_value_t *)B;
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
	for (ml_state_t *State = Caller; State && State->Type == MLComparisonStateT; State = State->Caller) {
		ml_comparison_state_t *Previous = (ml_comparison_state_t *)State;
		if (Previous->A == (ml_value_t *)A && Previous->B == (ml_value_t *)B) ML_RETURN(B);
	}
	if (!A->Size) {
		if (!B->Size) ML_RETURN(B);
		ML_RETURN(B);
	}
	if (!B->Size) ML_RETURN(MLNil);
	ml_tuple_compare_state_t *State = new(ml_tuple_compare_state_t);
	State->Base.Base.Type = MLComparisonStateT;
	State->Base.Base.Caller = Caller;
	State->Base.Base.Context = Caller->Context;
	State->Base.Base.run = (ml_state_fn)ml_tuple_compare_order_run;
	State->Base.A = (ml_value_t *)A;
	State->Base.B = (ml_value_t *)B;
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
	for (ml_state_t *State = Caller; State && State->Type == MLComparisonStateT; State = State->Caller) {
		ml_comparison_state_t *Previous = (ml_comparison_state_t *)State;
		if (Previous->A == (ml_value_t *)A && Previous->B == (ml_value_t *)B) ML_RETURN(MLNil);
	}
	if (!A->Size) {
		if (!B->Size) ML_RETURN(MLNil);
		ML_RETURN(MLNil);
	}
	if (!B->Size) ML_RETURN(A);
	ml_tuple_compare_state_t *State = new(ml_tuple_compare_state_t);
	State->Base.Base.Type = MLComparisonStateT;
	State->Base.Base.Caller = Caller;
	State->Base.Base.Context = Caller->Context;
	State->Base.Base.run = (ml_state_fn)ml_tuple_compare_order_run;
	State->Base.A = (ml_value_t *)A;
	State->Base.B = (ml_value_t *)B;
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
	for (ml_state_t *State = Caller; State && State->Type == MLComparisonStateT; State = State->Caller) {
		ml_comparison_state_t *Previous = (ml_comparison_state_t *)State;
		if (Previous->A == (ml_value_t *)A && Previous->B == (ml_value_t *)B) ML_RETURN(B);
	}
	if (!A->Size) {
		if (!B->Size) ML_RETURN(B);
		ML_RETURN(MLNil);
	}
	if (!B->Size) ML_RETURN(B);
	ml_tuple_compare_state_t *State = new(ml_tuple_compare_state_t);
	State->Base.Base.Type = MLComparisonStateT;
	State->Base.Base.Caller = Caller;
	State->Base.Base.Context = Caller->Context;
	State->Base.Base.run = (ml_state_fn)ml_tuple_compare_order_run;
	State->Base.A = (ml_value_t *)A;
	State->Base.B = (ml_value_t *)B;
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

void ml_tuple_init() {
#include "ml_tuple_init.c"
#ifdef ML_GENERICS
	ml_type_add_rule(MLTupleT, MLSequenceT, MLIntegerT, MLAnyT, NULL);
	ml_type_add_rule(MLTupleT, MLFunctionT, MLTupleT, NULL);
#endif
}

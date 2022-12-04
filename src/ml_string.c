#include "ml_string.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <float.h>
#include <stdatomic.h>
#include <gc/gc_typed.h>

#include "ml_sequence.h"
#ifdef ML_TRE
#include <tre/regex.h>
#else
#include <regex.h>
#endif

#ifdef ML_ICU
#include "ml_object.h"
#include <unicode/unorm2.h>
#include <unicode/ustring.h>
#endif

#ifdef ML_COMPLEX
#include <complex.h>
#undef I
#endif

#undef ML_CATEGORY
#define ML_CATEGORY "string"

// Overview
// Strings in Minilang can contain any sequence of bytes, including :mini:`0` bytes.
// Index and find methods however work on ``UTF-8`` characters, byte sequences that are not valid ``UTF-8`` are handled gracefully but the results are probably not very useful.
//
// Every :mini:`string` is also an :mini:`address` so address methods can also be used to work at the byte level if necessary.
//
// Indexing a string starts at :mini:`1`, with the last character at :mini:`String:length`. Negative indices are counted form the end, :mini:`-1` is the last character and :mini:`-String:length` is the first character.
//
// When creating a substring, the first index is inclusive and second index is exclusive. The index :mini:`0` refers to just beyond the last character and can be used to take a substring to the end of a string.

ML_TYPE(MLAddressT, (), "address");
//!address
// An address represents a read-only bounded section of memory.

ML_METHOD(MLAddressT, MLStringT) {
//!address
//<String
//>address
// Returns an address view of :mini:`String`.
//$= address("Hello world!\n")
	ml_address_t *Address = new(ml_address_t);
	Address->Type = MLAddressT;
	Address->Value = (char *)ml_string_value(Args[0]);
	Address->Length = ml_string_length(Args[0]);
	return (ml_value_t *)Address;
}

ml_value_t *ml_address(const char *Value, int Length) {
	ml_address_t *Address = new(ml_address_t);
	Address->Type = MLAddressT;
	Address->Value = (char *)Value;
	Address->Length = Length;
	return (ml_value_t *)Address;
}

static int ML_TYPED_FN(ml_value_is_constant, MLAddressT, ml_value_t *Value) {
	return 1;
}

ML_METHOD("size", MLAddressT) {
//!address
//<Address
//>integer
// Returns the number of bytes visible at :mini:`Address`.
//$= let A := address("Hello world!\n")
//$= A:size
	return ml_integer(ml_address_length(Args[0]));
}

ML_METHOD("length", MLAddressT) {
//!address
//<Address
//>integer
// Returns the number of bytes visible at :mini:`Address`.
//$= let A := address("Hello world!\n")
//$= A:length
	return ml_integer(ml_address_length(Args[0]));
}

ML_METHOD("@", MLAddressT, MLIntegerT) {
//!address
//<Address
//<Length
//>address
// Returns the same address as :mini:`Address`, limited to :mini:`Length` bytes.
//$= let A := address("Hello world!\n")
//$= A @ 5
	ml_address_t *Address = (ml_address_t *)Args[0];
	long Length = ml_integer_value_fast(Args[1]);
	if (Length > Address->Length) return ml_error("SizeError", "Size larger than buffer");
	if (Length < 0) return ml_error("ValueError", "Address size must be non-negative");
	ml_address_t *Address2 = new(ml_address_t);
	Address2->Type = MLAddressT;
	Address2->Value = Address->Value;
	Address2->Length = Length;
	return (ml_value_t *)Address2;
}

ML_METHOD("+", MLAddressT, MLIntegerT) {
//!address
//<Address
//<Offset
//>address
// Returns the address at offset :mini:`Offset` from :mini:`Address`.
//$= let A := address("Hello world!\n")
//$= A + 4
	ml_address_t *Address = (ml_address_t *)Args[0];
	long Offset = ml_integer_value_fast(Args[1]);
	if (Offset > Address->Length) return ml_error("SizeError", "Offset larger than buffer");
	ml_address_t *Address2 = new(ml_address_t);
	Address2->Type = MLAddressT;
	Address2->Value = Address->Value + Offset;
	Address2->Length = Address->Length - Offset;
	return (ml_value_t *)Address2;
}

ML_METHOD("-", MLAddressT, MLAddressT) {
//!address
//<Address/1
//<Address/2
//>integer
// Returns the offset from :mini:`Address/2` to :mini:`Address/1`, provided :mini:`Address/2` is visible to :mini:`Address/1`.
//$- let A := address("Hello world!\n")
//$- let B := A + 4
//$= B - A
//$= address("world!\n") - A
	ml_address_t *Address1 = (ml_address_t *)Args[0];
	ml_address_t *Address2 = (ml_address_t *)Args[1];
	int64_t Offset = Address1->Value - Address2->Value;
	if (Offset < 0 || Offset > Address2->Length) return ml_error("ValueError", "Addresses are not from same base");
	return ml_integer(Offset);
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

ML_METHOD("<>", MLAddressT, MLAddressT) {
//!address
//<A
//<B
//>integer
// Compares the bytes at :mini:`A` and :mini:`B` lexicographically and returns :mini:`-1`, :mini:`0` or :mini:`1` respectively.
//$= "Hello" <> "World"
//$= "World" <> "Hello"
//$= "Hello" <> "Hello"
//$= "abcd" <> "abc"
//$= "abc" <> "abcd"
	const char *AddressA = ml_address_value(Args[0]);
	const char *AddressB = ml_address_value(Args[1]);
	int LengthA = ml_address_length(Args[0]);
	int LengthB = ml_address_length(Args[1]);
	if (LengthA < LengthB) {
		int Compare = memcmp(AddressA, AddressB, LengthA);
		if (Compare > 0) return (ml_value_t *)One;
		return (ml_value_t *)NegOne;
	} else if (LengthA > LengthB) {
		int Compare = memcmp(AddressA, AddressB, LengthB);
		if (Compare < 0) return (ml_value_t *)NegOne;
		return (ml_value_t *)One;
	} else {
		int Compare = memcmp(AddressA, AddressB, LengthA);
		if (Compare < 0) return (ml_value_t *)NegOne;
		if (Compare > 0) return (ml_value_t *)One;
		return (ml_value_t *)Zero;
	}
}

#define ml_comp_method_address_address(NAME, SYMBOL) \
ML_METHOD(#NAME, MLAddressT, MLAddressT) { \
/*!address
//>address|nil
// Returns :mini:`Arg/2` if the bytes at :mini:`Arg/1` NAME the bytes at :mini:`Arg/2` and :mini:`nil` otherwise.
//$= "Hello" NAME "World"
//$= "World" NAME "Hello"
//$= "Hello" NAME "Hello"
//$= "abcd" NAME "abc"
//$= "abc" NAME "abcd"
*/\
	const char *AddressA = ml_address_value(Args[0]); \
	const char *AddressB = ml_address_value(Args[1]); \
	int LengthA = ml_address_length(Args[0]); \
	int LengthB = ml_address_length(Args[1]); \
	int Compare; \
	if (LengthA < LengthB) { \
		Compare = memcmp(AddressA, AddressB, LengthA) ?: -1; \
	} else if (LengthA > LengthB) { \
		Compare = memcmp(AddressA, AddressB, LengthB) ?: 1; \
	} else { \
		Compare = memcmp(AddressA, AddressB, LengthA); \
	} \
	return Compare SYMBOL 0 ? Args[1] : MLNil; \
}

ml_comp_method_address_address(=, ==)
ml_comp_method_address_address(!=, !=)
ml_comp_method_address_address(<, <)
ml_comp_method_address_address(>, >)
ml_comp_method_address_address(<=, <=)
ml_comp_method_address_address(>=, >=)

ML_METHOD("get8", MLAddressT) {
//!address
//<Address
//>integer
// Returns the signed 8-bit value at :mini:`Address`.
//$= let A := address("Hello world!\n")
//$= A:get8
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 1) return ml_error("SizeError", "Not enough bytes to read");
	return ml_integer(*(int8_t *)Address->Value);
}

ML_METHOD("get16", MLAddressT) {
//!address
//<Address
//>integer
// Returns the signed 16-bit value at :mini:`Address`. Currently follows the platform endiness.
//$= let A := address("Hello world!\n")
//$= A:get16
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 2) return ml_error("SizeError", "Not enough bytes to read");
	return ml_integer(*(int16_t *)Address->Value);
}

ML_METHOD("get32", MLAddressT) {
//!address
//<Address
//>integer
// Returns the signed 32-bit value at :mini:`Address`. Currently follows the platform endiness.
//$= let A := address("Hello world!\n")
//$= A:get32
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 4) return ml_error("SizeError", "Not enough bytes to read");
	return ml_integer(*(int32_t *)Address->Value);
}

ML_METHOD("get64", MLAddressT) {
//!address
//<Address
//>integer
// Returns the signed 64-bit value at :mini:`Address`. Currently follows the platform endiness.
//$= let A := address("Hello world!\n")
//$= A:get64
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 8) return ml_error("SizeError", "Not enough bytes to read");
	return ml_integer(*(int64_t *)Address->Value);
}

ML_METHOD("getu8", MLAddressT) {
//!address
//<Address
//>integer
// Returns the unsigned 8-bit value at :mini:`Address`.
//$= let A := address("Hello world!\n")
//$= A:getu8
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 1) return ml_error("SizeError", "Not enough bytes to read");
	return ml_integer(*(uint8_t *)Address->Value);
}

ML_METHOD("getu16", MLAddressT) {
//!address
//<Address
//>integer
// Returns the unsigned 16-bit value at :mini:`Address`. Currently follows the platform endiness.
//$= let A := address("Hello world!\n")
//$= A:getu16
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 2) return ml_error("SizeError", "Not enough bytes to read");
	return ml_integer(*(uint16_t *)Address->Value);
}

ML_METHOD("getu32", MLAddressT) {
//!address
//<Address
//>integer
// Returns the unsigned 32-bit value at :mini:`Address`. Currently follows the platform endiness.
//$= let A := address("Hello world!\n")
//$= A:getu32
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 4) return ml_error("SizeError", "Not enough bytes to read");
	return ml_integer(*(uint32_t *)Address->Value);
}

ML_METHOD("getu64", MLAddressT) {
//!address
//<Address
//>integer
// Returns the unsigned 64-bit value at :mini:`Address`. Currently follows the platform endiness.
// .. warning::
//
//    Minilang currently uses signed 64-bit integers so this method will produce incorrect results if the actual value is too large to fit. This may change in future implementations or if arbitrary precision integers are added to the runtime.
//$= let A := address("Hello world!\n")
//$= A:getu64
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 8) return ml_error("SizeError", "Not enough bytes to read");
	return ml_integer(*(uint64_t *)Address->Value);
}

ML_METHOD("getf32", MLAddressT) {
//!address
//<Address
//>real
// Returns the single precision floating point value at :mini:`Address`. Currently follows the platform endiness.
//$= let A := address("Hello world!\n")
//$= A:getf32
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 4) return ml_error("SizeError", "Not enough bytes to read");
	return ml_real(*(float *)Address->Value);
}

ML_METHOD("getf64", MLAddressT) {
//!address
//<Address
//>real
// Returns the double precision floating point value at :mini:`Address`. Currently follows the platform endiness.
//$= let A := address("Hello world!\n")
//$= A:getf64
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 8) return ml_error("SizeError", "Not enough bytes to read");
	return ml_real(*(double *)Address->Value);
}

ML_METHOD("gets", MLAddressT) {
//!address
//<Address
//>string
// Returns the string consisting of the bytes at :mini:`Address`.
//$= let A := address("Hello world!\n")
//$= A:gets
	ml_address_t *Address = (ml_address_t *)Args[0];
	size_t Length = ml_address_length(Args[0]);
	char *String = snew(Length + 1);
	memcpy(String, Address->Value, Length);
	String[Length] = 0;
	return ml_string(String, Length);
}

ML_METHOD("gets", MLAddressT, MLIntegerT) {
//!address
//<Address
//<Size
//>string
// Returns the string consisting of the first :mini:`Size` bytes at :mini:`Address`.
//$= let A := address("Hello world!\n")
//$= A:gets(5)
	ml_address_t *Address = (ml_address_t *)Args[0];
	size_t Length = ml_integer_value(Args[1]);
	if (Length > Address->Length) return ml_error("SizeError", "Not enough bytes to read");
	char *String = snew(Length + 1);
	memcpy(String, Address->Value, Length);
	String[Length] = 0;
	return ml_string(String, Length);
}

ML_METHOD("find", MLAddressT, MLAddressT) {
//!address
//<Haystack
//<Needle
//>integer|nil
// Returns the offset of the first occurence of the bytes of :mini:`Needle` in :mini:`Haystack` or :mini:`nil` is no occurence is found.
//$= let A := address("Hello world!\n")
//$= A:find("world")
//$= A:find("other")
	ml_address_t *Address1 = (ml_address_t *)Args[0];
	ml_address_t *Address2 = (ml_address_t *)Args[1];
	char *Find = memmem(Address1->Value, Address1->Length, Address2->Value, Address2->Length);
	if (!Find) return MLNil;
	return ml_integer(Find - Address1->Value);
}

ML_TYPE(MLBufferT, (MLAddressT), "buffer",
//!buffer
// A buffer represents a writable bounded section of memory.
);

ml_value_t *ml_buffer(char *Value, int Length) {
	ml_address_t *Buffer = new(ml_address_t);
	Buffer->Type = MLBufferT;
	Buffer->Value = Value;
	Buffer->Length = Length;
	return (ml_value_t *)Buffer;
}

ML_METHOD(MLBufferT, MLIntegerT) {
//!buffer
//@buffer
//<Length
//>buffer
// Allocates a new buffer with :mini:`Length` bytes.
//$= buffer(16)
	long Size = ml_integer_value_fast(Args[0]);
	if (Size < 0) return ml_error("ValueError", "Buffer size must be non-negative");
	ml_address_t *Buffer = new(ml_address_t);
	Buffer->Type = MLBufferT;
	Buffer->Length = Size;
	Buffer->Value = snew(Size);
	return (ml_value_t *)Buffer;
}

ML_METHOD(MLBufferT, MLAddressT) {
//!buffer
//@buffer
//<Source
//>buffer
// Allocates a new buffer with the same size and initial contents as :mini:`Source`.
//$= buffer("Hello world")
	long Size = ml_address_length(Args[0]);
	ml_address_t *Buffer = new(ml_address_t);
	Buffer->Type = MLBufferT;
	Buffer->Length = Size;
	Buffer->Value = snew(Size);
	memcpy(Buffer->Value, ml_address_value(Args[0]), Size);
	return (ml_value_t *)Buffer;
}

static int ML_TYPED_FN(ml_value_is_constant, MLBufferT, ml_value_t *Value) {
	return 0;
}

ML_METHOD("copy", MLVisitorT, MLBufferT) {
	size_t Size = ml_buffer_length(Args[1]);
	char *Value = snew(Size);
	memcpy(Value, ml_buffer_value(Args[1]), Size);
	return ml_buffer(Value, Size);
}

ML_METHOD("const", MLVisitorT, MLBufferT) {
	size_t Size = ml_buffer_length(Args[1]);
	char *Value = snew(Size);
	memcpy(Value, ml_buffer_value(Args[1]), Size);
	return ml_address(Value, Size);
}

ML_METHOD("@", MLBufferT, MLIntegerT) {
//!buffer
//<Buffer
//<Length
//>buffer
// Returns the same buffer as :mini:`Buffer`, limited to :mini:`Length` bytes.
//$= let B := buffer(16)
//$= B @ 8
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	long Length = ml_integer_value_fast(Args[1]);
	if (Length > Buffer->Length) return ml_error("SizeError", "Size larger than buffer");
	if (Length < 0) return ml_error("ValueError", "Buffer size must be non-negative");
	ml_address_t *Buffer2 = new(ml_address_t);
	Buffer2->Type = MLBufferT;
	Buffer2->Value = Buffer->Value;
	Buffer2->Length = Length;
	return (ml_value_t *)Buffer2;
}

ML_METHOD("+", MLBufferT, MLIntegerT) {
//!buffer
//<Buffer
//<Offset
//>buffer
// Returns the buffer at offset :mini:`Offset` from :mini:`Address`.
//$= let B := buffer(16)
//$= B + 8
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	long Offset = ml_integer_value_fast(Args[1]);
	if (Offset > Buffer->Length) return ml_error("SizeError", "Offset larger than buffer");
	ml_address_t *Buffer2 = new(ml_address_t);
	Buffer2->Type = MLBufferT;
	Buffer2->Value = Buffer->Value + Offset;
	Buffer2->Length = Buffer->Length - Offset;
	return (ml_value_t *)Buffer2;
}

ML_METHOD("put8", MLBufferT, MLIntegerT) {
//!buffer
//<Buffer
//<Value
//>buffer
// Puts :mini:`Value` in :mini:`Buffer` as an 8-bit signed value.
//$= buffer(1):put8(64)
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	if (Buffer->Length < 1) return ml_error("SizeError", "Not enough space");
	*(int8_t *)Buffer->Value = ml_integer_value(Args[1]);
	return Args[0];
}

ML_METHOD("put16", MLBufferT, MLIntegerT) {
//!buffer
//<Buffer
//<Value
//>buffer
// Puts :mini:`Value` in :mini:`Buffer` as an 16-bit signed value. Currently follows the platform endiness.
//$= buffer(2):put16(12345)
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	if (Buffer->Length < 2) return ml_error("SizeError", "Not enough space");
	*(int16_t *)Buffer->Value = ml_integer_value(Args[1]);
	return Args[0];
}

ML_METHOD("put32", MLBufferT, MLIntegerT) {
//!buffer
//<Buffer
//<Value
//>buffer
// Puts :mini:`Value` in :mini:`Buffer` as an 32-bit signed value. Currently follows the platform endiness.
//$= buffer(4):put32(12345678)
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	if (Buffer->Length < 4) return ml_error("SizeError", "Not enough space");
	*(int32_t *)Buffer->Value = ml_integer_value(Args[1]);
	return Args[0];
}

ML_METHOD("put64", MLBufferT, MLIntegerT) {
//!buffer
//<Buffer
//<Value
//>buffer
// Puts :mini:`Value` in :mini:`Buffer` as an 64-bit signed value. Currently follows the platform endiness.
//$= buffer(8):put64(123456789123)
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	if (Buffer->Length < 8) return ml_error("SizeError", "Not enough space");
	*(int64_t *)Buffer->Value = ml_integer_value(Args[1]);
	return Args[0];
}

ML_METHOD("putf32", MLBufferT, MLRealT) {
//!buffer
//<Buffer
//<Value
//>buffer
// Puts :mini:`Value` in :mini:`Buffer` as a single precision floating point value. Currently follows the platform endiness.
//$= buffer(4):putf32(1.23456789)
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	if (Buffer->Length < 4) return ml_error("SizeError", "Not enough space");
	*(float *)Buffer->Value = ml_real_value(Args[1]);
	return Args[0];
}

ML_METHOD("putf64", MLBufferT, MLRealT) {
//!buffer
//<Buffer
//<Value
//>buffer
// Puts :mini:`Value` in :mini:`Buffer` as a double precision floating point value. Currently follows the platform endiness.
//$= buffer(8):putf64(1.23456789)
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	if (Buffer->Length < 8) return ml_error("SizeError", "Not enough space");
	*(double *)Buffer->Value = ml_real_value(Args[1]);
	return Args[0];
}

ML_METHOD("put", MLBufferT, MLAddressT) {
//!buffer
//<Buffer
//<Value
//>buffer
// Puts the bytes of :mini:`Value` in :mini:`Buffer`.
//$= buffer(10):put("Hello\0\0\0\0\0")
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	ml_address_t *Source = (ml_address_t *)Args[1];
	if (Buffer->Length < Source->Length) return ml_error("SizeError", "Not enough space");
	memcpy(Buffer->Value, Source->Value, Source->Length);
	return Args[0];
}

static long ml_string_hash(ml_string_t *String, ml_hash_chain_t *Chain) {
	long Hash = String->Hash;
	if (!Hash) {
		Hash = 5381;
		for (int I = 0; I < String->Length; ++I) Hash = ((Hash << 5) + Hash) + String->Value[I];
		String->Hash = Hash;
	}
	return Hash;
}

ML_METHOD_DECL(AppendMethod, "append");

typedef struct {
	ml_state_t Base;
	ml_stringbuffer_t Buffer[1];
	ml_hash_chain_t Chain[1];
	ml_value_t *Args[];
} ml_string_state_t;

static void ml_string_state_run(ml_string_state_t *State, ml_value_t *Result) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Result)) ML_RETURN(Result);
	if (State->Chain->Index) ml_stringbuffer_printf(State->Buffer, "<%d", State->Chain->Index);
	ML_RETURN(ml_stringbuffer_get_value(State->Buffer));
}

ML_FUNCTIONX(MLString) {
//@string
//<Value:any
//>string
// Returns a general (type name only) representation of :mini:`Value` as a string.
//$= string(100)
//$= string(nil)
//$= string("Hello world!\n")
//$= string([1, 2, 3])
	ML_CHECKX_ARG_COUNT(1);
	if (ml_is(Args[0], MLStringT)) ML_RETURN(Args[0]);
	ml_string_state_t *State = xnew(ml_string_state_t, Count + 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_string_state_run;
	State->Chain->Value = Args[0];
	State->Buffer[0] = ML_STRINGBUFFER_INIT;
	State->Buffer->Chain = State->Chain;
	State->Args[0] = (ml_value_t *)State->Buffer;
	memcpy(State->Args + 1, Args, Count * sizeof(ml_value_t *));
	return ml_call(State, AppendMethod, Count + 1, State->Args);
}

ML_TYPE(MLStringT, (MLAddressT, MLSequenceT), "string",
// A string of characters in UTF-8 encoding.
	.hash = (void *)ml_string_hash,
	.Constructor = (ml_value_t *)MLString
);

ML_METHOD("append", MLStringBufferT, MLAddressT) {
//!address
//<Buffer
//<Value
// Appends the contents of :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_address_t *Address = (ml_address_t *)Args[1];
	//ml_stringbuffer_printf(Buffer, "#%" PRIxPTR ":%ld", (uintptr_t)Address->Value, Address->Length);
	ml_stringbuffer_write(Buffer, Address->Value, Address->Length);
	return MLSome;
}

int GC_vasprintf(char **Ptr, const char *Format, va_list Args) {
	va_list Copy;
	va_copy(Copy, Args);
	int Actual = vsnprintf(NULL, 0, Format, Args);
	char *Output = *Ptr = GC_malloc_atomic(Actual);
	vsprintf(Output, Format, Copy);
	return Actual;
}

int GC_asprintf(char **Ptr, const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	int Result = GC_vasprintf(Ptr, Format, Args);
	va_end(Args);
	return Result;
}

ml_value_t *ml_string(const char *Value, int Length) {
	Value = Value ?: "";
	if (Length >= 0) {
		if (Value[Length]) {
			char *Copy = snew(Length + 1);
			memcpy(Copy, Value, Length);
			Copy[Length] = 0;
			Value = Copy;
		}
	} else {
		Length = strlen(Value);
	}
	ml_string_t *String = new(ml_string_t);
	String->Type = MLStringT;
	String->Value = Value;
	String->Length = Length;
	return (ml_value_t *)String;
}

ml_value_t *ml_string_copy(const char *Value, int Length) {
	if (Length < 0) Length = Value ? strlen(Value) : 0;
	char *Copy = snew(Length + 1);
	memcpy(Copy, Value, Length);
	Copy[Length] = 0;
	ml_string_t *String = new(ml_string_t);
	String->Type = MLStringT;
	String->Value = Copy;
	String->Length = Length;
	return (ml_value_t *)String;
}

ml_value_t *ml_string_format(const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Value;
	int Length = GC_vasprintf(&Value, Format, Args);
	va_end(Args);
	return ml_string(Value, Length);
}

ML_METHOD("append", MLStringBufferT, MLBooleanT) {
//!boolean
//<Buffer
//<Value
// Appends :mini:`"true"` or :mini:`"false"` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_boolean_t *Boolean = (ml_boolean_t *)Args[1];
	if (Boolean->Value) {
		ml_stringbuffer_write(Buffer, "true", 4);
	} else {
		ml_stringbuffer_write(Buffer, "false", 5);
	}
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLIntegerT) {
//!number
//<Buffer
//<Value
// Appends :mini:`Value` to :mini:`Buffer` in base :mini:`10`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_printf(Buffer, "%ld", ml_integer_value_fast(Args[1]));
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLIntegerT, MLIntegerT) {
//!number
//<Buffer
//<Value
//<Base
// Appends :mini:`Value` to :mini:`Buffer` in base :mini:`Base`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	int64_t Value = ml_integer_value_fast(Args[1]);
	int Base = ml_integer_value_fast(Args[2]);
	if (Base < 2 || Base > 36) return ml_error("RangeError", "Invalid base");
	int Max = 65;
	char Temp[Max + 1], *P = Temp + Max, *Q = P;
	*P = '\0';
	int64_t Neg = Value < 0 ? Value : -Value;
	do {
		*--P = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[-(Neg % Base)];
		Neg /= Base;
	} while (Neg);
	if (Value < 0) *--P = '-';
	ml_stringbuffer_write(Buffer, P, Q - P);
	return MLSome;
}

static regex_t IntFormat[1];
static regex_t LongFormat[1];
static regex_t RealFormat[1];

ML_METHOD("append", MLStringBufferT, MLIntegerT, MLStringT) {
//!number
//<Buffer
//<Value
//<Format
// Appends :mini:`Value` to :mini:`Buffer` using :mini:`Format` as a (checked) :c:`printf` format string.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	const char *Format = ml_string_value(Args[2]);
	int64_t Value = ml_integer_value_fast(Args[1]);
	if (!regexec(IntFormat, Format, 0, NULL, 0)) {
		ml_stringbuffer_printf(Buffer, Format, (int)Value);
	} else if (!regexec(LongFormat, Format, 0, NULL, 0)) {
		ml_stringbuffer_printf(Buffer, Format, (long)Value);
	} else if (!regexec(RealFormat, Format, 0, NULL, 0)) {
		ml_stringbuffer_printf(Buffer, Format, (double)Value);
	} else {
		return ml_error("FormatError", "Invalid format string");
	}
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLIntegerRangeT) {
//!range
//<Buffer
//<Value
// Appends a representation of :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[1];
	if (Range->Step != 1) {
		ml_stringbuffer_printf(Buffer, "%ld .. %ld by %ld", Range->Start, Range->Limit, Range->Step);
	} else {
		ml_stringbuffer_printf(Buffer, "%ld .. %ld", Range->Start, Range->Limit);
	}
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLRealRangeT) {
//!range
//<Buffer
//<Value
// Appends a representation of :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_real_range_t *Range = (ml_real_range_t *)Args[1];
	ml_stringbuffer_printf(Buffer, "%g .. %g by %g", Range->Start, Range->Limit, Range->Step);
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLDoubleT) {
//!number
//<Buffer
//<Value
// Appends :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_printf(Buffer, "%g", ml_double_value_fast(Args[1]));
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLDoubleT, MLStringT) {
//!number
//<Buffer
//<Value
//<Format
// Appends :mini:`Value` to :mini:`Buffer` using :mini:`Format` as a (checked) :c:`printf` format string.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	const char *Format = ml_string_value(Args[2]);
	double Value = ml_double_value_fast(Args[1]);
	if (!regexec(IntFormat, Format, 0, NULL, 0)) {
		ml_stringbuffer_printf(Buffer, Format, (int)Value);
	} else if (!regexec(LongFormat, Format, 0, NULL, 0)) {
		ml_stringbuffer_printf(Buffer, Format, (long)Value);
	} else if (!regexec(RealFormat, Format, 0, NULL, 0)) {
		ml_stringbuffer_printf(Buffer, Format, (double)Value);
	} else {
		return ml_error("FormatError", "Invalid format string");
	}
	return MLSome;
}

#ifdef ML_COMPLEX

ML_METHOD("append", MLStringBufferT, MLComplexT) {
//!number
//<Buffer
//<Value
// Appends :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	complex double Complex = ml_complex_value(Args[1]);
	double Real = creal(Complex);
	double Imag = cimag(Complex);
	if (fabs(Real) <= DBL_EPSILON) {
		if (fabs(Imag - 1) <= DBL_EPSILON) {
			ml_stringbuffer_put(Buffer, 'i');
		} else if (fabs(Imag) <= DBL_EPSILON) {
			ml_stringbuffer_put(Buffer, '0');
		} else {
			ml_stringbuffer_printf(Buffer, "%gi", Imag);
		}
	} else if (fabs(Imag) <= DBL_EPSILON) {
		ml_stringbuffer_printf(Buffer, "%g", Real);
	} else if (Imag < 0) {
		if (fabs(Imag + 1) <= DBL_EPSILON) {
			ml_stringbuffer_printf(Buffer, "%g - i", Real);
		} else {
			ml_stringbuffer_printf(Buffer, "%g - %gi", Real, -Imag);
		}
	} else {
		if (fabs(Imag - 1) <= DBL_EPSILON) {
			ml_stringbuffer_printf(Buffer, "%g + i", Real);
		} else {
			ml_stringbuffer_printf(Buffer, "%g + %gi", Real, Imag);
		}
	}
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLComplexT, MLStringT) {
//!number
//<Buffer
//<Value
//<Format
// Appends :mini:`Value` to :mini:`Buffer` using :mini:`Format` as a (checked) :c:`printf` format string for the real and imaginary components.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	const char *Format = ml_string_value(Args[1]);
	if (regexec(RealFormat, Format, 0, NULL, 0)) {
		return ml_error("FormatError", "Invalid format string");
	}
	complex double Complex = ml_complex_value(Args[0]);
	double Real = creal(Complex);
	double Imag = cimag(Complex);
	if (Imag < 0) {
		Imag = -Imag;
		ml_stringbuffer_printf(Buffer, Format, Real);
		ml_stringbuffer_put(Buffer, '-');
		ml_stringbuffer_printf(Buffer, Format, Imag);
	} else {
		ml_stringbuffer_printf(Buffer, Format, Real);
		ml_stringbuffer_put(Buffer, '+');
		ml_stringbuffer_printf(Buffer, Format, Imag);
	}
	return MLSome;
}

#endif

ML_METHOD(MLIntegerT, MLStringT) {
//!number
//<String
//>integer|error
// Returns the base :mini:`10` integer in :mini:`String` or an error if :mini:`String` does not contain a valid integer.
//$= integer("123")
//$= integer("ABC")
	const char *Start = ml_string_value(Args[0]);
	char *End;
	long Value = strtol(Start, &End, 10);
	if (End - Start == ml_string_length(Args[0])) {
		return ml_integer(Value);
	} else {
		return ml_error("ValueError", "Error parsing integer");
	}
}

ML_METHOD(MLIntegerT, MLStringT, MLIntegerT) {
//!number
//<String
//<Base
//>integer|error
// Returns the base :mini:`Base` integer in :mini:`String` or an error if :mini:`String` does not contain a valid integer.
	const char *Start = ml_string_value(Args[0]);
	char *End;
	long Value = strtol(Start, &End, ml_integer_value_fast(Args[1]));
	if (End - Start == ml_string_length(Args[0])) {
		return ml_integer(Value);
	} else {
		return ml_error("ValueError", "Error parsing integer");
	}
}

ML_METHOD(MLDoubleT, MLStringT) {
//!internal
	const char *Start = ml_string_value(Args[0]);
	char *End;
	double Value = strtod(Start, &End);
	if (End - Start == ml_string_length(Args[0])) {
		return ml_real(Value);
	} else {
		return ml_error("ValueError", "Error parsing real");
	}
}

ML_METHOD(MLRealT, MLStringT) {
//!number
//<String
//>real|error
// Returns the real number in :mini:`String` or an error if :mini:`String` does not contain a valid real number.
	const char *Start = ml_string_value(Args[0]);
	char *End;
	double Value = strtod(Start, &End);
	if (End - Start == ml_string_length(Args[0])) {
		return ml_real(Value);
	} else {
		return ml_error("ValueError", "Error parsing real");
	}
}

#ifdef ML_COMPLEX

ML_METHOD(MLComplexT, MLStringT) {
//!number
//<String
//>complex|error
// Returns the complex number in :mini:`String` or an error if :mini:`String` does not contain a valid complex number.
	const char *Start = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	char *End = (char *)Start;
#ifdef ML_COMPLEX
	if (End[0] == 'i') {
		if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
		return ml_complex(_Complex_I);
	}
#endif
	long Integer = strtol(Start, &End, 10);
#ifdef ML_COMPLEX
	if (End[0] == 'i') {
		if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
		return ml_complex(Integer * _Complex_I);
	}
#endif
	if (End - Start == Length) return ml_complex(Integer);
	double Real = strtod(Start, &End);
#ifdef ML_COMPLEX
	if (End[0] == 'i') {
		if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
		return ml_complex(Real * _Complex_I);
	}
#endif
	if (End - Start == Length) return ml_complex(Real);
#ifdef ML_COMPLEX
	if (End[0] == ' ') ++End;
	if (End[0] == '+') {
		++End;
		if (End[0] == ' ') ++End;
		if (End[0] == 'i') {
			if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
			return ml_complex(Real + _Complex_I);
		}
		double Imag = strtod(End, &End);
		if (End[0] == 'i') {
			if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
			return ml_complex(Real + Imag * _Complex_I);
		}
	} else if (End[0] == '-') {
		++End;
		if (End[0] == ' ') ++End;
		if (End[0] == 'i') {
			if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
			return ml_complex(Real - _Complex_I);
		}
		double Imag = strtod(End, &End);
		if (End[0] == 'i') {
			if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
			return ml_complex(Real - Imag * _Complex_I);
		}
	}
#endif
	return ml_error("ValueError", "Error parsing number");
}

#endif

ML_METHOD(MLNumberT, MLStringT) {
//!number
//<String
//>integer|real|complex|error
// Returns the number in :mini:`String` or an error if :mini:`String` does not contain a valid number.
	const char *Start = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	char *End = (char *)Start;
#ifdef ML_COMPLEX
	if (End[0] == 'i') {
		if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
		return ml_complex(_Complex_I);
	}
#endif
	long Integer = strtol(Start, &End, 10);
#ifdef ML_COMPLEX
	if (End[0] == 'i') {
		if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
		return ml_complex(Integer * _Complex_I);
	}
#endif
	if (End - Start == Length) return ml_integer(Integer);
	double Real = strtod(Start, &End);
#ifdef ML_COMPLEX
	if (End[0] == 'i') {
		if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
		return ml_complex(Real * _Complex_I);
	}
#endif
	if (End - Start == Length) return ml_real(Real);
#ifdef ML_COMPLEX
	if (End[0] == ' ') ++End;
	if (End[0] == '+') {
		++End;
		if (End[0] == ' ') ++End;
		if (End[0] == 'i') {
			if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
			return ml_complex(Real + _Complex_I);
		}
		double Imag = strtod(End, &End);
		if (End[0] == 'i') {
			if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
			return ml_complex(Real + Imag * _Complex_I);
		}
	} else if (End[0] == '-') {
		++End;
		if (End[0] == ' ') ++End;
		if (End[0] == 'i') {
			if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
			return ml_complex(Real - _Complex_I);
		}
		double Imag = strtod(End, &End);
		if (End[0] == 'i') {
			if (++End - Start != Length) return ml_error("ValueError", "Error parsing number");
			return ml_complex(Real - Imag * _Complex_I);
		}
	}
#endif
	return ml_error("ValueError", "Error parsing number");
}

typedef struct {
	ml_type_t *Type;
	const char *Start, *End;
	int Index;
} ml_string_iterator_t;

ML_TYPE(MLStringIteratorT, (), "string-iterator");
//!internal

static int utf8_is_continuation(char C) {
	return (C & 0xC0) == 0x80;
}

static size_t utf8_position(const char *P, const char *Q) {
	size_t N = 0;
	while (P != Q) {
		if (!utf8_is_continuation(*P)) ++N;
		++P;
	}
	return N;
}

static size_t utf8_strlen(ml_value_t *S) {
	const char *P = ml_string_value(S);
	return utf8_position(P, P + ml_string_length(S));
}

static void ML_TYPED_FN(ml_iter_next, MLStringIteratorT, ml_state_t *Caller, ml_string_iterator_t *Iter) {
	const char *Start = Iter->End;
	if (!*Start) ML_RETURN(MLNil);
	const char *End = Start + 1;
	while (*End && utf8_is_continuation(*End)) ++End;
	Iter->Start = Start;
	Iter->End = End;
	++Iter->Index;
	ML_RETURN(Iter);
}

static void ML_TYPED_FN(ml_iter_value, MLStringIteratorT, ml_state_t *Caller, ml_string_iterator_t *Iter) {
	ML_RETURN(ml_string(Iter->Start, Iter->End - Iter->Start));
}

static void ML_TYPED_FN(ml_iter_key, MLStringIteratorT, ml_state_t *Caller, ml_string_iterator_t *Iter) {
	ML_RETURN(ml_integer(Iter->Index));
}

static void ML_TYPED_FN(ml_iterate, MLStringT, ml_state_t *Caller, ml_value_t *String) {
	const char *Start = ml_string_value(String);
	if (!*Start) ML_RETURN(MLNil);
	const char *End = Start + 1;
	while (*End && utf8_is_continuation(*End)) ++End;
	ml_string_iterator_t *Iter = new(ml_string_iterator_t);
	Iter->Type = MLStringIteratorT;
	Iter->Index = 1;
	Iter->Start = Start;
	Iter->End = End;
	ML_RETURN(Iter);
}

typedef struct ml_regex_t ml_regex_t;

typedef struct ml_regex_t {
	ml_type_t *Type;
	const char *Pattern;
	regex_t Value[1];
} ml_regex_t;

regex_t *ml_regex_value(const ml_value_t *Value) {
	ml_regex_t *Regex = (ml_regex_t *)Value;
	return Regex->Value;
}

ML_METHOD("append", MLStringBufferT, MLStringT) {
//<Buffer
//<Value
// Appends :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	int Length = ml_string_length(Args[1]);
	if (Length) {
		ml_stringbuffer_write(Buffer, ml_string_value(Args[1]), Length);
		return MLSome;
	} else {
		return MLNil;
	}
}

ML_METHOD("length", MLStringT) {
//<String
//>integer
// Returns the number of UTF-8 characters in :mini:`String`. Use :mini:`:size` to get the number of bytes.
//$= "Hello world":length
//$= "Hello world":size
//$= "λ:😀️ → 😺️":length
//$= "λ:😀️ → 😺️":size
	return ml_integer(utf8_strlen(Args[0]));
}

ML_METHOD("count", MLStringT) {
//<String
//>integer
// Returns the number of UTF-8 characters in :mini:`String`. Use :mini:`:size` to get the number of bytes.
//$= "Hello world":count
//$= "Hello world":size
//$= "λ:😀️ → 😺️":count
//$= "λ:😀️ → 😺️":size
	return ml_integer(utf8_strlen(Args[0]));
}

ML_METHOD("code", MLStringT) {
//<String
//>integer
// Returns the unicode codepoint of the first UTF-8 character of :mini:`String`.
//$= "A":code
//$= "😀️":code
	const char *S = ml_string_value(Args[0]);
	uint32_t K = S[0] ? __builtin_clz(~(S[0] << 24)) : 0;
	uint32_t Mask = (1 << (8 - K)) - 1;
	uint32_t Value = S[0] & Mask;
	for (++S, --K; K > 0 && S[0]; --K, ++S) {
		Value <<= 6;
		Value += S[0] & 0x3F;
	}
	return ml_integer(Value);
}

ML_METHOD("utf8", MLIntegerT) {
//<Codepoint
//>string
// Returns a UTF-8 string containing the character with unicode codepoint :mini:`Codepoint`.
	uint32_t Code = ml_integer_value(Args[0]);
	char Val[8];
	uint32_t LeadByteMax = 0x7F;
	int I = 0;
	while (Code > LeadByteMax) {
		Val[I++] = (Code & 0x3F) | 0x80;
		Code >>= 6;
		LeadByteMax >>= (I == 1 ? 2 : 1);
	}
	Val[I++] = (Code & LeadByteMax) | (~LeadByteMax << 1);
	char *S = snew(I + 1), *P = S;
	while (I--) *P++ = Val[I];
	*P = 0;
	return ml_string(S, I);
}

ML_METHOD("lower", MLStringT) {
//<String
//>string
// Returns :mini:`String` with each character converted to lower case.
//$= "Hello World":lower
	const char *Source = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	char *Target = snew(Length + 1);
	for (int I = 0; I < Length; ++I) Target[I] = tolower(Source[I]);
	return ml_string(Target, Length);
}

ML_METHOD("upper", MLStringT) {
//<String
//>string
// Returns :mini:`String` with each character converted to upper case.
//$= "Hello World":upper
	const char *Source = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	char *Target = snew(Length + 1);
	for (int I = 0; I < Length; ++I) Target[I] = toupper(Source[I]);
	return ml_string(Target, Length);
}

ML_METHOD("title", MLStringT) {
//<String
//>string
// Returns :mini:`String` with the first character and each character after whitespace converted to upper case and each other case converted to lower case.
//$= "hello world":title
//$= "HELLO WORLD":title
	const char *Source = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	char *Target = snew(Length + 1);
	int Upper = 1;
	for (int I = 0; I < Length; ++I) {
		Target[I] = (Upper ? toupper : tolower)(Source[I]);
		Upper = isblank(Source[I]);
	}
	return ml_string(Target, Length);
}

#ifdef ML_ICU

enum {
	ML_UNORM_NFC,
	ML_UNORM_NFD,
	ML_UNORM_NFKC,
	ML_UNORM_NFKD
};

ML_ENUM2(MLStringNormT, "string::norm",
//@string::norm
//
// * :mini:`string::norm::NFC`
// * :mini:`string::norm::NFD`
// * :mini:`string::norm::NFKC`
// * :mini:`string::norm::NFKD`
	"NFC", ML_UNORM_NFC,
	"NFD", ML_UNORM_NFD,
	"NFKC", ML_UNORM_NFKC,
	"NFKD", ML_UNORM_NFKD
);

ML_METHOD("normalize", MLStringT, MLStringNormT) {
//<String
//<Norm
//>string
// Returns a normalized copy of :mini:`String` using the normalizer specified by :mini:`Norm`.
//$= let S := "𝕥𝕖𝕩𝕥"
//$= S:normalize(string::norm::NFD)
	UErrorCode Error = U_ZERO_ERROR;
	int SrcLimit = 4 * ml_string_length(Args[0]);
	UChar Src[SrcLimit];
	int Length;
	u_strFromUTF8(Src, SrcLimit, &Length, ml_string_value(Args[0]), ml_string_length(Args[0]), &Error);
	if (U_FAILURE(Error)) return ml_error("UnicodeError", "Error decoding UTF-8");
	const UNormalizer2 *Normalizer = NULL;
	switch (ml_enum_value_value(Args[1])) {
	case ML_UNORM_NFC:
		Normalizer = unorm2_getNFCInstance(&Error);
		break;
	case ML_UNORM_NFD:
		Normalizer = unorm2_getNFDInstance(&Error);
		break;
	case ML_UNORM_NFKC:
		Normalizer = unorm2_getNFKCInstance(&Error);
		break;
	case ML_UNORM_NFKD:
		Normalizer = unorm2_getNFKDInstance(&Error);
		break;
	}
	if (U_FAILURE(Error)) return ml_error("UnicodeError", "Error getting normalizer");
	size_t Capacity = 4 * Length;
	UChar Dest[Capacity];
	int Actual = unorm2_normalize(Normalizer, Src, Length, Dest, Capacity, &Error);
	if (U_FAILURE(Error)) return ml_error("UnicodeError", "Error normalizing string");
	char *String = snew(Actual * 4);
	u_strToUTF8(String, Actual * 4, &Length, Dest, Actual, &Error);
	if (U_FAILURE(Error)) return ml_error("UnicodeError", "Error encoding UTF-8");
	return ml_string(String, Length);
}

ML_ENUM2(MLStringCTypeT, "string::ctype",
//@string::ctype
//
// * :mini:`string::ctype::Cn`: General Other Types
// * :mini:`string::ctype::Lu`: Uppercase Letter
// * :mini:`string::ctype::Ll`: Lowercase Letter
// * :mini:`string::ctype::Lt`: Titlecase Letter
// * :mini:`string::ctype::Lm`: Modifier Letter
// * :mini:`string::ctype::Lo`: Other Letter
// * :mini:`string::ctype::Mn`: Non Spacing Mark
// * :mini:`string::ctype::Me`: Enclosing Mark
// * :mini:`string::ctype::Mc`: Combining Spacing Mark
// * :mini:`string::ctype::Nd`: Decimal Digit Number
// * :mini:`string::ctype::Nl`: Letter Number
// * :mini:`string::ctype::No`: Other Number
// * :mini:`string::ctype::Zs`: Space Separator
// * :mini:`string::ctype::Zl`: Line Separator
// * :mini:`string::ctype::Zp`: Paragraph Separator
// * :mini:`string::ctype::Cc`: Control Char
// * :mini:`string::ctype::Cf`: Format Char
// * :mini:`string::ctype::Co`: Private Use Char
// * :mini:`string::ctype::Cs`: Surrogate
// * :mini:`string::ctype::Pd`: Dash Punctuation
// * :mini:`string::ctype::Ps`: Start Punctuation
// * :mini:`string::ctype::Pe`: End Punctuation
// * :mini:`string::ctype::Pc`: Connector Punctuation
// * :mini:`string::ctype::Po`: Other Punctuation
// * :mini:`string::ctype::Sm`: Math Symbol
// * :mini:`string::ctype::Sc`: Currency Symbol
// * :mini:`string::ctype::Sk`: Modifier Symbol
// * :mini:`string::ctype::So`: Other Symbol
// * :mini:`string::ctype::Pi`: Initial Punctuation
// * :mini:`string::ctype::Pf`: Final Punctuation
	"Cn", U_GENERAL_OTHER_TYPES,
	"Lu", U_UPPERCASE_LETTER,
	"Ll", U_LOWERCASE_LETTER,
	"Lt", U_TITLECASE_LETTER,
	"Lm", U_MODIFIER_LETTER,
	"Lo", U_OTHER_LETTER,
	"Mn", U_NON_SPACING_MARK,
	"Me", U_ENCLOSING_MARK,
	"Mc", U_COMBINING_SPACING_MARK,
	"Nd", U_DECIMAL_DIGIT_NUMBER,
	"Nl", U_LETTER_NUMBER,
	"No", U_OTHER_NUMBER,
	"Zs", U_SPACE_SEPARATOR,
	"Zl", U_LINE_SEPARATOR,
	"Zp", U_PARAGRAPH_SEPARATOR,
	"Cc", U_CONTROL_CHAR,
	"Cf", U_FORMAT_CHAR,
	"Co", U_PRIVATE_USE_CHAR,
	"Cs", U_SURROGATE,
	"Pd", U_DASH_PUNCTUATION,
	"Ps", U_START_PUNCTUATION,
	"Pe", U_END_PUNCTUATION,
	"Pc", U_CONNECTOR_PUNCTUATION,
	"Po", U_OTHER_PUNCTUATION,
	"Sm", U_MATH_SYMBOL,
	"Sc", U_CURRENCY_SYMBOL,
	"Sk", U_MODIFIER_SYMBOL,
	"So", U_OTHER_SYMBOL,
	"Pi", U_INITIAL_PUNCTUATION,
	"Pf", U_FINAL_PUNCTUATION
);

ML_METHOD("ctype", MLStringT) {
//<String
//>string::ctype
// Returns the unicode type of the first character of :mini:`String`.
//$= map("To €2 á\n" => (2, 2 -> :ctype))
	const char *S = ml_string_value(Args[0]);
	uint32_t K = S[0] ? __builtin_clz(~(S[0] << 24)) : 0;
	uint32_t Mask = (1 << (8 - K)) - 1;
	uint32_t Value = S[0] & Mask;
	for (++S, --K; K > 0 && S[0]; --K, ++S) {
		Value <<= 6;
		Value += S[0] & 0x3F;
	}
	return ml_enum_value(MLStringCTypeT, u_charType(Value));
}

#endif

ML_METHOD("[]", MLStringT, MLIntegerT) {
//<String
//<Index
//>string
// Returns the substring of :mini:`String` of length 1 at :mini:`Index`.
	const char *Start = ml_string_value(Args[0]);
	int Length = utf8_strlen(Args[0]);
	int N = ml_integer_value_fast(Args[1]);
	if (N <= 0) N += Length + 1;
	if (N <= 0) return MLNil;
	if (N > Length) return MLNil;
	for (;;) {
		if (!utf8_is_continuation(*Start) && (--N == 0)) break;
		++Start;
	}
	const char *End = Start + 1;
	while (*End && utf8_is_continuation(*End)) ++End;
	return ml_string(Start, End - Start);
}

ML_METHOD("[]", MLStringT, MLIntegerT, MLIntegerT) {
//<String
//<Start
//<End
//>string
// Returns the substring of :mini:`String` from :mini:`Start` to :mini:`End - 1` inclusively.
	const char *Start = ml_string_value(Args[0]);
	int Length = utf8_strlen(Args[0]);
	int Lo = ml_integer_value_fast(Args[1]);
	int Hi = ml_integer_value_fast(Args[2]);
	if (Lo <= 0) Lo += Length + 1;
	if (Hi <= 0) Hi += Length + 1;
	if (Lo <= 0) return MLNil;
	if (Hi > Length + 1) return MLNil;
	if (Hi < Lo) return MLNil;
	Hi -= Lo;
	if (Lo > 0) for (;;) {
		if (!utf8_is_continuation(*Start) && (--Lo == 0)) break;
		++Start;
	}
	const char *End = Start;
	if (++Hi > 0) while (*End) {
		if (!utf8_is_continuation(*End) && (--Hi == 0)) break;
		++End;
	}
	return ml_string(Start, End - Start);
}

ML_METHOD("[]", MLStringT, MLIntegerRangeT) {
//<String
//<Range
//>string
// Returns the substring of :mini:`String` corresponding to :mini:`Range` inclusively.
	const char *Start = ml_string_value(Args[0]);
	int Length = utf8_strlen(Args[0]);
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[1];
	int Lo = Range->Start, Hi = Range->Limit + 1, Step = Range->Step;
	if (Step != 1) return ml_error("ValueError", "Invalid step size for list slice");
	if (Lo <= 0) Lo += Length + 1;
	if (Hi <= 0) Hi += Length + 1;
	if (Lo <= 0) return MLNil;
	if (Hi > Length + 1) return MLNil;
	if (Hi < Lo) return MLNil;
	Hi -= Lo;
	if (Lo > 0) for (;;) {
		if (!utf8_is_continuation(*Start) && (--Lo == 0)) break;
		++Start;
	}
	const char *End = Start;
	if (++Hi > 0) while (*End) {
		if (!utf8_is_continuation(*End) && (--Hi == 0)) break;
		++End;
	}
	return ml_string(Start, End - Start);
}

ML_METHOD("limit", MLStringT, MLIntegerT) {
//<String
//<Length
//>string
// Returns the prefix of :mini:`String` limited to :mini:`Length`.
//$= "Hello world":limit(5)
//$= "Cake":limit(5)
	int Length = utf8_strlen(Args[0]);
	int N = ml_integer_value_fast(Args[1]);
	if (N < 0) return MLNil;
	if (N == 0) return ml_cstring("");
	if (N >= Length) return Args[0];
	++N;
	const char *Start = ml_string_value(Args[0]), *End = Start;
	for (;;) {
		if (!utf8_is_continuation(*End) && (--N == 0)) break;
		++End;
	}
	return ml_string(Start, End - Start);
}

ML_METHOD("offset", MLStringT, MLIntegerT) {
//<String
//<Index
//>integer
// Returns the byte position of the :mini:`Index`-th character of :mini:`String`.
//$- let S := "λ:😀️ → 😺️"
//$= list(1 .. S:length, S:offset(_))
	const char *Start = ml_string_value(Args[0]);
	int Length = utf8_strlen(Args[0]);
	int N = ml_integer_value_fast(Args[1]);
	if (N <= 0) N += Length + 1;
	if (N <= 0) return MLNil;
	if (N > Length) return MLNil;
	const char *P = Start;
	for (;;) {
		if (!utf8_is_continuation(*P) && (--N == 0)) break;
		++P;
	}
	return ml_integer(P - Start);
}

ML_METHOD("+", MLStringT, MLStringT) {
//<A
//<B
//>string
// Returns :mini:`A` and :mini:`B` concatentated.
//$= "Hello" + " " + "world"
	int Length1 = ml_string_length(Args[0]);
	int Length2 = ml_string_length(Args[1]);
	int Length = Length1 + Length2;
	char *Chars = snew(Length + 1);
	memcpy(Chars, ml_string_value(Args[0]), Length1);
	memcpy(Chars + Length1, ml_string_value(Args[1]), Length2);
	Chars[Length] = 0;
	return ml_string(Chars, Length);
}

ML_METHOD("*", MLIntegerT, MLStringT) {
//<N
//<String
//>string
// Returns :mini:`String` concatentated :mini:`N` times.
//$= 5 * "abc"
	int N = ml_integer_value(Args[0]);
	const char *Chars = ml_string_value(Args[1]);
	int Length = ml_string_length(Args[1]);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (int I = 0; I < N; ++I) ml_stringbuffer_write(Buffer, Chars, Length);
	return ml_stringbuffer_get_value(Buffer);
}

ML_METHOD("trim", MLStringT) {
//<String
//>string
// Returns a copy of :mini:`String` with whitespace removed from both ends.
//$= " \t Hello \n":trim
	const unsigned char *Start = (const unsigned char *)ml_string_value(Args[0]);
	const unsigned char *End = Start + ml_string_length(Args[0]);
	while (Start < End && Start[0] <= ' ') ++Start;
	while (Start < End && End[-1] <= ' ') --End;
	int Length = End - Start;
	return ml_string((const char *)Start, Length);
}

ML_METHOD("trim", MLStringT, MLStringT) {
//<String
//<Chars
//>string
// Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from both ends.
//$= " \t Hello \n":trim(" \n")
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
//<String
//>string
// Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the start.
//$= " \t Hello \n":ltrim
	const unsigned char *Start = (const unsigned char *)ml_string_value(Args[0]);
	const unsigned char *End = Start + ml_string_length(Args[0]);
	while (Start < End && Start[0] <= ' ') ++Start;
	int Length = End - Start;
	return ml_string((const char *)Start, Length);
}

ML_METHOD("ltrim", MLStringT, MLStringT) {
//<String
//<Chars
//>string
// Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the start.
//$= " \t Hello \n":trim(" \n")
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
//<String
//>string
// Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the end.
//$= " \t Hello \n":rtrim
	const unsigned char *Start = (const unsigned char *)ml_string_value(Args[0]);
	const unsigned char *End = Start + ml_string_length(Args[0]);
	while (Start < End && End[-1] <= ' ') --End;
	int Length = End - Start;
	return ml_string((const char *)Start, Length);
}

ML_METHOD("rtrim", MLStringT, MLStringT) {
//<String
//<Chars
//>string
// Returns a copy of :mini:`String` with characters in :mini:`Chars` removed from the end.
//$= " \t Hello \n":rtrim(" \n")
	char Trim[256] = {0,};
	const unsigned char *P = (const unsigned char *)ml_string_value(Args[1]);
	for (int Length = ml_string_length(Args[1]); --Length >= 0; ++P) Trim[*P] = 1;
	const unsigned char *Start = (const unsigned char *)ml_string_value(Args[0]);
	const unsigned char *End = Start + ml_string_length(Args[0]);
	while (Start < End && Trim[End[-1]]) --End;
	int Length = End - Start;
	return ml_string((const char *)Start, Length);
}

ML_METHOD("reverse", MLStringT) {
//<String
//>string
// Returns a string with the characters in :mini:`String` reversed.
//$= "Hello world":reverse
	int Length = ml_string_length(Args[0]);
	char *Reversed = snew(Length + 1), *End = Reversed + Length;
	const char *S = ml_string_value(Args[0]), *T = S, *U = S + Length;
	while (++T < U) {
		--End;
		if (!utf8_is_continuation(*T)) {
			for (char *E = End; S != T;) *E++ = *S++;
		}
	}
	memcpy(End - 1, S, T - S);
	Reversed[Length] = 0;
	return ml_string(Reversed, Length);
}

ML_METHOD("<>", MLStringT, MLStringT) {
//<A
//<B
//>integer
// Compares :mini:`A` and :mini:`B` lexicographically and returns :mini:`-1`, :mini:`0` or :mini:`1` respectively.
//$= "Hello" <> "World"
//$= "World" <> "Hello"
//$= "Hello" <> "Hello"
//$= "abcd" <> "abc"
//$= "abc" <> "abcd"
	const char *StringA = ml_string_value(Args[0]);
	const char *StringB = ml_string_value(Args[1]);
	int LengthA = ml_string_length(Args[0]);
	int LengthB = ml_string_length(Args[1]);
	if (LengthA < LengthB) {
		int Compare = memcmp(StringA, StringB, LengthA);
		if (Compare > 0) return (ml_value_t *)One;
		return (ml_value_t *)NegOne;
	} else if (LengthA > LengthB) {
		int Compare = memcmp(StringA, StringB, LengthB);
		if (Compare < 0) return (ml_value_t *)NegOne;
		return (ml_value_t *)One;
	} else {
		int Compare = memcmp(StringA, StringB, LengthA);
		if (Compare < 0) return (ml_value_t *)NegOne;
		if (Compare > 0) return (ml_value_t *)One;
		return (ml_value_t *)Zero;
	}
}

#define ml_comp_method_string_string(NAME, SYMBOL) \
ML_METHOD(#NAME, MLStringT, MLStringT) { \
/*>string|nil
// Returns :mini:`Arg/2` if :mini:`Arg/1 NAME Arg/2` and :mini:`nil` otherwise.
//$= "Hello" NAME "World"
//$= "World" NAME "Hello"
//$= "Hello" NAME "Hello"
//$= "abcd" NAME "abc"
//$= "abc" NAME "abcd"
*/\
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

ml_comp_method_string_string(=, ==)
ml_comp_method_string_string(!=, !=)
ml_comp_method_string_string(<, <)
ml_comp_method_string_string(>, >)
ml_comp_method_string_string(<=, <=)
ml_comp_method_string_string(>=, >=)

ML_METHOD("min", MLStringT, MLStringT) {
//<A
//<B
//>integer
// Returns :mini:`min(A, B)`
//$= "Hello":min("World")
//$= "World":min("Hello")
//$= "abcd":min("abc")
//$= "abc":min("abcd")
	const char *StringA = ml_string_value(Args[0]);
	const char *StringB = ml_string_value(Args[1]);
	int LengthA = ml_string_length(Args[0]);
	int LengthB = ml_string_length(Args[1]);
	if (LengthA < LengthB) {
		int Compare = memcmp(StringA, StringB, LengthA);
		if (Compare > 0) return Args[1];
		return Args[0];
	} else if (LengthA > LengthB) {
		int Compare = memcmp(StringA, StringB, LengthB);
		if (Compare < 0) return Args[0];
		return Args[1];
	} else {
		int Compare = memcmp(StringA, StringB, LengthA);
		if (Compare < 0) return Args[0];
		if (Compare > 0) return Args[1];
		return Args[1];
	}
}

ML_METHOD("max", MLStringT, MLStringT) {
//<A
//<B
//>integer
// Returns :mini:`max(A, B)`
//$= "Hello":max("World")
//$= "World":max("Hello")
//$= "abcd":max("abc")
//$= "abc":max("abcd")
	const char *StringA = ml_string_value(Args[0]);
	const char *StringB = ml_string_value(Args[1]);
	int LengthA = ml_string_length(Args[0]);
	int LengthB = ml_string_length(Args[1]);
	if (LengthA < LengthB) {
		int Compare = memcmp(StringA, StringB, LengthA);
		if (Compare > 0) return Args[0];
		return Args[1];
	} else if (LengthA > LengthB) {
		int Compare = memcmp(StringA, StringB, LengthB);
		if (Compare < 0) return Args[1];
		return Args[0];
	} else {
		int Compare = memcmp(StringA, StringB, LengthA);
		if (Compare < 0) return Args[1];
		if (Compare > 0) return Args[0];
		return Args[1];
	}
}

#define SWAP(A, B) { \
	typeof(A) Temp = A; \
	A = B; \
	B = Temp; \
}

ML_METHOD("~", MLStringT, MLStringT) {
//<A
//<B
//>integer
// Returns the edit distance between :mini:`A` and :mini:`B`.
//$= "cake" ~ "cat"
//$= "yell" ~ "hello"
//$= "say" ~ "goodbye"
//$= "goodbye" ~ "say"
	// TODO: use UTF-8 characters
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
//<A
//<B
//>integer
// Returns an asymmetric edit distance from :mini:`A` to :mini:`B`.
//$= "cake" ~> "cat"
//$= "yell" ~> "hello"
//$= "say" ~> "goodbye"
//$= "goodbye" ~> "say"
	// TODO: use UTF-8 characters
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
//<String
//<Pattern
//>list
// Returns a list of substrings from :mini:`String` by splitting around occurences of :mini:`Pattern`. Adjacent occurences of :mini:`Pattern` do not create empty strings.
//$= "The cat snored  as he slept" / " "
//$= "2022/03/08" / "/"
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
//<String
//<Pattern
//>list
// Returns a list of substrings from :mini:`String` by splitting around occurences of :mini:`Pattern`.
// If :mini:`Pattern` contains a subgroup then only the subgroup matches are removed from the output substrings.
//$= "2022/03/08" / r"[/-]"
//$= "2022-03-08" / r"[/-]"
	ml_value_t *Results = ml_list();
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	const char *SubjectEnd = Subject + SubjectLength;
	ml_regex_t *Pattern = (ml_regex_t *)Args[1];
	int Index = Pattern->Value->re_nsub ? 1 : 0;
	regmatch_t Matches[2];
	for (;;) {
#ifdef ML_TRE
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

ML_METHOD("/", MLStringT, MLRegexT, MLIntegerT) {
//<String
//<Pattern
//<Index
//>list
// Returns a list of substrings from :mini:`String` by splitting around occurences of :mini:`Pattern`.
// Only the :mini:`Index` subgroup matches are removed from the output substrings.
//$= "<A>-<B>-<C>" / (r">(-)<", 1)
	ml_value_t *Results = ml_list();
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	const char *SubjectEnd = Subject + SubjectLength;
	ml_regex_t *Pattern = (ml_regex_t *)Args[1];
	int Index = ml_integer_value(Args[2]);
	if (Index < 0 || Index > Pattern->Value->re_nsub) return ml_error("RegexError", "Invalid regex group");

	regmatch_t Matches[Index + 1];
	for (;;) {
#ifdef ML_TRE
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

ML_METHOD("/*", MLStringT, MLStringT) {
//<String
//<Pattern
//>tuple[string, string]
// Splits :mini:`String` at the first occurence of :mini:`Pattern` and returns the two substrings in a tuple.
//$= "2022/03/08" /* "/"
	const char *Subject = ml_string_value(Args[0]);
	const char *End = Subject + ml_string_length(Args[0]);
	const char *Pattern = ml_string_value(Args[1]);
	size_t Length = strlen(Pattern);
	ml_value_t *Results = ml_tuple(2);
	const char *Next = strstr(Subject, Pattern);
	if (Next) {
		ml_tuple_set(Results, 1, ml_string(Subject, Next - Subject));
		Next += Length;
		ml_tuple_set(Results, 2, ml_string(Next, End - Next));
	} else {
		ml_tuple_set(Results, 1, Args[0]);
		ml_tuple_set(Results, 2, ml_cstring(""));
	}
	return Results;
}

ML_METHOD("/*", MLStringT, MLRegexT) {
//<String
//<Pattern
//>tuple[string, string]
// Splits :mini:`String` at the first occurence of :mini:`Pattern` and returns the two substrings in a tuple.
//$= "2022/03/08" /* r"[/-]"
//$= "2022-03-08" /* r"[/-]"
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	ml_regex_t *Pattern = (ml_regex_t *)Args[1];
	ml_value_t *Results = ml_tuple(2);
	regmatch_t Matches[2];
#ifdef ML_TRE
	switch (regnexec(Pattern->Value, Subject, SubjectLength, 1, Matches, 0)) {
#else
	switch (regexec(Pattern->Value, Subject, 1, Matches, 0)) {
#endif
	case REG_NOMATCH:
		ml_tuple_set(Results, 1, Args[0]);
		ml_tuple_set(Results, 2, ml_cstring(""));
		return Results;
	case REG_ESPACE: {
		size_t ErrorSize = regerror(REG_ESPACE, Pattern->Value, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(REG_ESPACE, Pattern->Value, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	default: {
		ml_tuple_set(Results, 1, ml_string(Subject, Matches[0].rm_so));
		const char *Next = Subject + Matches[0].rm_eo;
		ml_tuple_set(Results, 2, ml_string(Next, Subject + SubjectLength - Next));
		return Results;
	}
	}
}

ML_METHOD("*/", MLStringT, MLStringT) {
//<String
//<Pattern
//>tuple[string, string]
// Splits :mini:`String` at the last occurence of :mini:`Pattern` and returns the two substrings in a tuple.
//$= "2022/03/08" */ "/"
	const char *Subject = ml_string_value(Args[0]);
	const char *End = Subject + ml_string_length(Args[0]);
	const char *Pattern = ml_string_value(Args[1]);
	size_t Length = strlen(Pattern);
	ml_value_t *Results = ml_tuple(2);
	const char *Next = End - Length;
	while (Next >= Subject) {
		if (!memcmp(Next, Pattern, Length)) {
			ml_tuple_set(Results, 1, ml_string(Subject, Next - Subject));
			Next += Length;
			ml_tuple_set(Results, 2, ml_string(Next, End - Next));
			return Results;
		}
		--Next;
	}
	ml_tuple_set(Results, 1, Args[0]);
	ml_tuple_set(Results, 2, ml_cstring(""));
	return Results;
}

ML_METHOD("*/", MLStringT, MLRegexT) {
//<String
//<Pattern
//>tuple[string, string]
// Splits :mini:`String` at the last occurence of :mini:`Pattern` and returns the two substrings in a tuple.
//$= "2022/03/08" */ r"[/-]"
//$= "2022-03-08" */ r"[/-]"
	const char *Subject = ml_string_value(Args[0]);
	const char *End = Subject + ml_string_length(Args[0]);
	ml_regex_t *Pattern = (ml_regex_t *)Args[1];
	ml_value_t *Results = ml_tuple(2);
	regmatch_t Matches[2];
	const char *Next = End - 1;
	int NextLength = 1;
	while (Next >= Subject) {
#ifdef ML_TRE
		switch (regnexec(Pattern->Value, Next, NextLength, 1, Matches, 0)) {
#else
		switch (regexec(Pattern->Value, Next, 1, Matches, 0)) {
#endif
		case REG_NOMATCH:
			--Next;
			++NextLength;
			break;
		case REG_ESPACE: {
			size_t ErrorSize = regerror(REG_ESPACE, Pattern->Value, NULL, 0);
			char *ErrorMessage = snew(ErrorSize + 1);
			regerror(REG_ESPACE, Pattern->Value, ErrorMessage, ErrorSize);
			return ml_error("RegexError", "regex error: %s", ErrorMessage);
		}
		default: {
			ml_tuple_set(Results, 1, ml_string(Subject, Next - Subject));
			Next += Matches[0].rm_eo;
			ml_tuple_set(Results, 2, ml_string(Next, End - Next));
			return Results;
		}
		}
	}
	ml_tuple_set(Results, 1, Args[0]);
	ml_tuple_set(Results, 2, ml_cstring(""));
	return Results;
}

ML_METHOD("escape", MLStringT) {
//<String
//>string
// Returns :mini:`String` with white space, quotes and backslashes replaced by escape sequences.
//$= "\t\"Text\"\r\n":escape
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Start = ml_string_value(Args[0]);
	int N = ml_string_length(Args[0]);
	const char *End = Start;
	for (; --N >= 0; ++End) {
		switch (*End) {
		case '\r':
			ml_stringbuffer_write(Buffer, Start, End - Start);
			ml_stringbuffer_write(Buffer, "\\r", 2);
			Start = End + 1;
			break;
		case '\n':
			ml_stringbuffer_write(Buffer, Start, End - Start);
			ml_stringbuffer_write(Buffer, "\\n", 2);
			Start = End + 1;
			break;
		case '\t':
			ml_stringbuffer_write(Buffer, Start, End - Start);
			ml_stringbuffer_write(Buffer, "\\t", 2);
			Start = End + 1;
			break;
		case '\e':
			ml_stringbuffer_write(Buffer, Start, End - Start);
			ml_stringbuffer_write(Buffer, "\\e", 2);
			Start = End + 1;
			break;
		case '\'':
			ml_stringbuffer_write(Buffer, Start, End - Start);
			ml_stringbuffer_write(Buffer, "\\\'", 2);
			Start = End + 1;
			break;
		case '\"':
			ml_stringbuffer_write(Buffer, Start, End - Start);
			ml_stringbuffer_write(Buffer, "\\\"", 2);
			Start = End + 1;
			break;
		case '\\':
			ml_stringbuffer_write(Buffer, Start, End - Start);
			ml_stringbuffer_write(Buffer, "\\\\", 2);
			Start = End + 1;
			break;
		case '\0':
			ml_stringbuffer_write(Buffer, Start, End - Start);
			ml_stringbuffer_write(Buffer, "\\0", 2);
			Start = End + 1;
			break;
		default:
			break;
		}
	}
	ml_stringbuffer_write(Buffer, Start, End - Start);
	return ml_stringbuffer_get_value(Buffer);
}

ML_METHOD("contains", MLStringT, MLStringT) {
//<Haystack
//<Needle
//>string|nil
// Returns the :mini:`Haystack` if it contains :mini:`Pattern` or :mini:`nil` otherwise.
//$= "The cat snored as he slept":contains("cat")
//$= "The cat snored as he slept":contains("dog")
	const char *Haystack = ml_string_value(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	return strstr(Haystack, Needle) ? Args[0] : MLNil;
}

ML_METHOD("find", MLStringT, MLStringT) {
//<Haystack
//<Needle
//>integer|nil
// Returns the index of the first occurence of :mini:`Needle` in :mini:`Haystack`, or :mini:`nil` if no occurence is found.
//$= "The cat snored as he slept":find("cat")
//$= "The cat snored as he slept":find("dog")
	const char *Haystack = ml_string_value(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	const char *Match = strstr(Haystack, Needle);
	if (Match) {
		return ml_integer(1 + utf8_position(Haystack, Match));
	} else {
		return MLNil;
	}
}

ML_METHOD("find2", MLStringT, MLStringT) {
//<Haystack
//<Needle
//>tuple[integer,string]|nil
// Returns :mini:`(Index, Needle)` where :mini:`Index` is the first occurence of :mini:`Needle` in :mini:`Haystack`, or :mini:`nil` if no occurence is found.
//$= "The cat snored as he slept":find2("cat")
//$= "The cat snored as he slept":find2("dog")
	const char *Haystack = ml_string_value(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	const char *Match = strstr(Haystack, Needle);
	if (Match) {
		return ml_tuplev(2, ml_integer(1 + utf8_position(Haystack, Match)), Args[1]);
	} else {
		return MLNil;
	}
}

static const char *utf8_skip(const char *S, int P) {
	for (;;) {
		if (!*S) break;
		if (!utf8_is_continuation(*S) && (--P == 0)) break;
		++S;
	}
	return S;
}

ML_METHOD("find", MLStringT, MLStringT, MLIntegerT) {
//<Haystack
//<Needle
//<Start
//>integer|nil
// Returns the index of the first occurence of :mini:`Needle` in :mini:`Haystack` at or after :mini:`Start`, or :mini:`nil` if no occurence is found.
//$= "The cat snored as he slept":find("s", 1)
//$= "The cat snored as he slept":find("s", 10)
//$= "The cat snored as he slept":find("s", -6)
	const char *Haystack = ml_string_value(Args[0]);
	size_t HaystackLength = utf8_strlen(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	int Start = ml_integer_value_fast(Args[2]);
	if (Start <= 0) Start += HaystackLength + 1;
	if (Start <= 0) return MLNil;
	if (Start > HaystackLength) return MLNil;
	Haystack = utf8_skip(Haystack, Start);
	const char *Match = strstr(Haystack, Needle);
	if (Match) {
		return ml_integer(Start + utf8_position(Haystack, Match));
	} else {
		return MLNil;
	}
}

ML_METHOD("find2", MLStringT, MLStringT, MLIntegerT) {
//<Haystack
//<Needle
//<Start
//>tuple[integer,string]|nil
// Returns :mini:`(Index, Needle)` where :mini:`Index` is the first occurence of :mini:`Needle` in :mini:`Haystack` at or after :mini:`Start`, or :mini:`nil` if no occurence is found.
//$= "The cat snored as he slept":find2("s", 1)
//$= "The cat snored as he slept":find2("s", 10)
//$= "The cat snored as he slept":find2("s", -6)
	const char *Haystack = ml_string_value(Args[0]);
	size_t HaystackLength = ml_string_length(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	int Start = ml_integer_value_fast(Args[2]);
	if (Start <= 0) Start += HaystackLength + 1;
	if (Start <= 0) return MLNil;
	if (Start > HaystackLength) return MLNil;
	Haystack = utf8_skip(Haystack, Start);
	const char *Match = strstr(Haystack, Needle);
	if (Match) {
		return ml_tuplev(2, ml_integer(Start + utf8_position(Haystack, Match)), Args[1]);
	} else {
		return MLNil;
	}
}

ML_METHOD("contains", MLStringT, MLRegexT) {
//<Haystack
//<Pattern
//>string|nil
// Returns the :mini:`Haystack` if it contains :mini:`Pattern` or :mini:`nil` otherwise.
//$= "The cat snored as he slept":contains(r"[a-z]{3}")
//$= "The cat snored as he slept":contains(r"[0-9]+")
	const char *Haystack = ml_string_value(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	regmatch_t Matches[1];
#ifdef ML_TRE
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
	return Args[0];
}

ML_METHOD("find", MLStringT, MLRegexT) {
//<Haystack
//<Pattern
//>integer|nil
// Returns the index of the first occurence of :mini:`Pattern` in :mini:`Haystack`, or :mini:`nil` if no occurence is found.
//$= "The cat snored as he slept":find(r"[a-z]{3}")
//$= "The cat snored as he slept":find(r"[0-9]+")
	const char *Haystack = ml_string_value(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	regmatch_t Matches[1];
#ifdef ML_TRE
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
	return ml_integer(1 + utf8_position(Haystack, Haystack + Matches->rm_so));
}

ML_METHOD("find2", MLStringT, MLRegexT) {
//<Haystack
//<Pattern
//>tuple[integer,string]|nil
// Returns :mini:`(Index, Match)` where :mini:`Index` is the first occurence of :mini:`Pattern` in :mini:`Haystack`, or :mini:`nil` if no occurence is found.
//$= "The cat snored as he slept":find2(r"[a-z]{3}")
//$= "The cat snored as he slept":find2(r"[0-9]+")
	const char *Haystack = ml_string_value(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	regmatch_t Matches[Regex->re_nsub + 1];
#ifdef ML_TRE
	int Length = ml_string_length(Args[0]);
	switch (regnexec(Regex, Haystack, Length, Regex->re_nsub + 1, Matches, 0)) {
#else
	switch (regexec(Regex, Haystack, Regex->re_nsub + 1, Matches, 0)) {
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
	ml_value_t *Result = ml_tuple(Regex->re_nsub + 2);
	ml_tuple_set(Result, 1, ml_integer(1 + utf8_position(Haystack, Haystack + Matches->rm_so)));
	for (int I = 0; I < Regex->re_nsub + 1; ++I) {
		regoff_t Start = Matches[I].rm_so;
		if (Start >= 0) {
			size_t Length = Matches[I].rm_eo - Start;
			ml_tuple_set(Result, I + 2, ml_string(Haystack + Start, Length));
		} else {
			ml_tuple_set(Result, I + 2, MLNil);
		}
	}
	return Result;
}

ML_METHOD("find", MLStringT, MLRegexT, MLIntegerT) {
//<Haystack
//<Pattern
//<Start
//>integer|nil
// Returns the index of the first occurence of :mini:`Pattern` in :mini:`Haystack` at or after :mini:`Start`, or :mini:`nil` if no occurence is found.
//$= "The cat snored as he slept":find(r"s[a-z]+", 1)
//$= "The cat snored as he slept":find(r"s[a-z]+", 10)
//$= "The cat snored as he slept":find(r"s[a-z]+", -6)
	const char *Haystack = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	int Start = ml_integer_value_fast(Args[2]);
	if (Start <= 0) Start += Length + 1;
	if (Start <= 0) return MLNil;
	if (Start > Length) return MLNil;
	const char *Haystack2 = utf8_skip(Haystack, Start);
	Length -= (Haystack2 - Haystack);
	regmatch_t Matches[1];
#ifdef ML_TRE
	switch (regnexec(Regex, Haystack2, Length, 1, Matches, 0)) {
#else
	switch (regexec(Regex, Haystack2, 1, Matches, 0)) {
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
	return ml_integer(Start + utf8_position(Haystack2, Haystack2 + Matches->rm_so));
}

ML_METHOD("find2", MLStringT, MLRegexT, MLIntegerT) {
//<Haystack
//<Pattern
//<Start
//>tuple[integer,string]|nil
// Returns :mini:`(Index, Match)` where :mini:`Index` is the first occurence of :mini:`Pattern` in :mini:`Haystack` at or after :mini:`Start`, or :mini:`nil` if no occurence is found.
//$= "The cat snored as he slept":find2(r"s[a-z]+", 1)
//$= "The cat snored as he slept":find2(r"s[a-z]+", 10)
//$= "The cat snored as he slept":find2(r"s[a-z]+", -6)
	const char *Haystack = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	int Start = ml_integer_value_fast(Args[2]);
	if (Start <= 0) Start += Length + 1;
	if (Start <= 0) return MLNil;
	if (Start > Length) return MLNil;
	const char *Haystack2 = utf8_skip(Haystack, Start);
	Length -= (Haystack2 - Haystack);
	regmatch_t Matches[Regex->re_nsub + 1];
#ifdef ML_TRE
	switch (regnexec(Regex, Haystack2, Length, Regex->re_nsub + 1, Matches, 0)) {
#else
	switch (regexec(Regex, Haystack2, Regex->re_nsub + 1, Matches, 0)) {
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
	ml_value_t *Result = ml_tuple(Regex->re_nsub + 2);
	ml_tuple_set(Result, 1, ml_integer(Start + utf8_position(Haystack2, Haystack2 + Matches->rm_so)));
	for (int I = 0; I < Regex->re_nsub + 1; ++I) {
		regoff_t Start = Matches[I].rm_so;
		if (Start >= 0) {
			size_t Length = Matches[I].rm_eo - Start;
			ml_tuple_set(Result, I + 2, ml_string(Haystack2 + Start, Length));
		} else {
			ml_tuple_set(Result, I + 2, MLNil);
		}
	}
	return Result;
}

#ifdef ML_GENERICS

ML_GENERIC_TYPE(MLTupleIntegerStringT, MLTupleT, MLIntegerT, MLStringT);

ML_METHOD("find2", MLStringT, MLStringT, MLTupleIntegerStringT) {
//<Haystack
//<Needle
//<Start
//>tuple[integer,string]|nil
// Returns :mini:`(Index, Needle)` where :mini:`Index` is the first occurence of :mini:`Needle` in :mini:`Haystack` at or after :mini:`Start`, or :mini:`nil` if no occurence is found.
//$= "The cat snored as he slept":find2("s", 1)
//$= "The cat snored as he slept":find2("s", 10)
//$= "The cat snored as he slept":find2("s", -6)
	const char *Haystack = ml_string_value(Args[0]);
	size_t HaystackLength = ml_string_length(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	int Start = ml_integer_value(ml_tuple_get(Args[2], 1)) + ml_string_length(ml_tuple_get(Args[2], 2));
	if (Start <= 0) Start += HaystackLength + 1;
	if (Start <= 0) return MLNil;
	if (Start > HaystackLength) return MLNil;
	Haystack = utf8_skip(Haystack, Start);
	const char *Match = strstr(Haystack, Needle);
	if (Match) {
		return ml_tuplev(2, ml_integer(Start + utf8_position(Haystack, Match)), Args[1]);
	} else {
		return MLNil;
	}
}

ML_METHOD("find2", MLStringT, MLRegexT, MLTupleIntegerStringT) {
//<Haystack
//<Pattern
//<Start
//>tuple[integer,string]|nil
// Returns :mini:`(Index, Match)` where :mini:`Index` is the first occurence of :mini:`Pattern` in :mini:`Haystack` at or after :mini:`Start`, or :mini:`nil` if no occurence is found.
//$= "The cat snored as he slept":find2(r"s[a-z]+", 1)
//$= "The cat snored as he slept":find2(r"s[a-z]+", 10)
//$= "The cat snored as he slept":find2(r"s[a-z]+", -6)
	const char *Haystack = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	int Start = ml_integer_value(ml_tuple_get(Args[2], 1)) + ml_string_length(ml_tuple_get(Args[2], 2));
	if (Start <= 0) Start += Length + 1;
	if (Start <= 0) return MLNil;
	if (Start > Length) return MLNil;
	const char *Haystack2 = utf8_skip(Haystack, Start);
	Length -= (Haystack2 - Haystack);
	regmatch_t Matches[Regex->re_nsub + 1];
#ifdef ML_TRE
	switch (regnexec(Regex, Haystack2, Length, Regex->re_nsub + 1, Matches, 0)) {
#else
	switch (regexec(Regex, Haystack2, Regex->re_nsub + 1, Matches, 0)) {
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
	ml_value_t *Result = ml_tuple(Regex->re_nsub + 2);
	ml_tuple_set(Result, 1, ml_integer(Start + utf8_position(Haystack2, Haystack2 + Matches->rm_so)));
	for (int I = 0; I < Regex->re_nsub + 1; ++I) {
		regoff_t Start = Matches[I].rm_so;
		if (Start >= 0) {
			size_t Length = Matches[I].rm_eo - Start;
			ml_tuple_set(Result, I + 2, ml_string(Haystack2 + Start, Length));
		} else {
			ml_tuple_set(Result, I + 2, MLNil);
		}
	}
	return Result;
}

#endif

ML_METHOD("%", MLStringT, MLRegexT) {
//<String
//<Pattern
//>tuple[string]|nil
// Matches :mini:`String` with :mini:`Pattern` returning a tuple of the matched components, or :mini:`nil` if the pattern does not match.
//$= "2022-03-08" % r"([0-9]+)[/-]([0-9]+)[/-]([0-9]+)"
//$= "Not a date" % r"([0-9]+)[/-]([0-9]+)[/-]([0-9]+)"
	const char *Subject = ml_string_value(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	regmatch_t Matches[Regex->re_nsub + 1];
#ifdef ML_TRE
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
#ifdef ML_TRE
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
//<String
//<Pattern
//>string|nil
// Returns :mini:`String` if it matches :mini:`Pattern` and :mini:`nil` otherwise.
//$= "2022-03-08" ? r"([0-9]+)[/-]([0-9]+)[/-]([0-9]+)"
//$= "Not a date" ? r"([0-9]+)[/-]([0-9]+)[/-]([0-9]+)"
	const char *Subject = ml_string_value(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	regmatch_t Matches[Regex->re_nsub + 1];
#ifdef ML_TRE
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
//<String
//<Prefix
//>string|nil
// Returns :mini:`String` if it starts with :mini:`Prefix` and :mini:`nil` otherwise.
//$= "Hello world":starts("Hello")
//$= "Hello world":starts("cake")
	const char *Subject = ml_string_value(Args[0]);
	const char *Prefix = ml_string_value(Args[1]);
	int Length = ml_string_length(Args[1]);
	if (Length > ml_string_length(Args[0])) return MLNil;
	if (memcmp(Subject, Prefix, Length)) return MLNil;
	return Args[0];
}

ML_METHOD("starts", MLStringT, MLRegexT) {
//<String
//<Pattern
//>string|nil
// Returns :mini:`String` if it starts with :mini:`Pattern` and :mini:`nil` otherwise.
//$= "Hello world":starts(r"[A-Z]")
//$= "Hello world":starts(r"[0-9]")
	const char *Subject = ml_string_value(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	regmatch_t Matches[Regex->re_nsub + 1];
#ifdef ML_TRE
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
		if (Start != 0) return MLNil;
		return Args[0];
	}
	}
}

ML_METHOD("ends", MLStringT, MLStringT) {
//<String
//<Suffix
//>string|nil
// Returns :mini:`String` if it ends with :mini:`Suffix` and :mini:`nil` otherwise.
//$= "Hello world":ends("world")
//$= "Hello world":ends("cake")
	const char *Subject = ml_string_value(Args[0]);
	const char *Suffix = ml_string_value(Args[1]);
	int Length = ml_string_length(Args[1]);
	int Length0 = ml_string_length(Args[0]);
	if (Length > Length0) return MLNil;
	if (memcmp(Subject + Length0 - Length, Suffix, Length)) return MLNil;
	return Args[0];
}

ML_METHOD("after", MLStringT, MLStringT) {
//<String
//<Delimiter
//>string|nil
// Returns the portion of :mini:`String` after the 1st occurence of :mini:`Delimiter`, or :mini:`nil` if no occurence if found.
//$= "2022/03/08":after("/")
	const char *Haystack = ml_string_value(Args[0]);
	size_t HaystackLength = ml_string_length(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	size_t NeedleLength = ml_string_length(Args[1]);
	const char *Match = strstr(Haystack, Needle);
	if (Match) {
		Match += NeedleLength;
		int Length = HaystackLength - (Match - Haystack);
		return ml_string(Match, Length);
	} else {
		return MLNil;
	}
}

ML_METHOD("after", MLStringT, MLStringT, MLIntegerT) {
//<String
//<Delimiter
//<N
//>string|nil
// Returns the portion of :mini:`String` after the :mini:`N`-th occurence of :mini:`Delimiter`, or :mini:`nil` if no :mini:`N`-th occurence if found.
// If :mini:`N < 0` then occurences are counted from the end of :mini:`String`.
//$= "2022/03/08":after("/", 2)
	const char *Haystack = ml_string_value(Args[0]);
	size_t HaystackLength = ml_string_length(Args[0]);
	const char *HaystackEnd = Haystack + HaystackLength;
	const char *Needle = ml_string_value(Args[1]);
	size_t NeedleLength = ml_string_length(Args[1]);
	int Index = ml_integer_value(Args[2]);
	if (Index > 0) {
		for (;;) {
			const char *Match = strstr(Haystack, Needle);
			if (!Match) return MLNil;
			if (--Index) {
				Haystack = Match + NeedleLength;
			} else {
				Match += NeedleLength;
				int Length = HaystackEnd - Match;
				return ml_string(Match, Length);
			}
		}
	} else if (Index < 0) {
		for (int I = HaystackLength - NeedleLength; I >= 0; --I) {
			const char *Match = Haystack + I;
			if (!memcmp(Match, Needle, NeedleLength)) {
				if (++Index) {
					I -= NeedleLength;
				} else {
					Match += NeedleLength;
					int Length = HaystackEnd - Match;
					return ml_string(Match, Length);
				}
			}
		}
		return MLNil;
	}
	return Args[0];
}

ML_METHOD("before", MLStringT, MLStringT) {
//<String
//<Delimiter
//>string|nil
// Returns the portion of :mini:`String` before the 1st occurence of :mini:`Delimiter`, or :mini:`nil` if no occurence if found.
//$= "2022/03/08":before("/")
	const char *Haystack = ml_string_value(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	const char *Match = strstr(Haystack, Needle);
	if (Match) {
		return ml_string(Haystack, Match - Haystack);
	} else {
		return MLNil;
	}
}

ML_METHOD("before", MLStringT, MLStringT, MLIntegerT) {
//<String
//<Delimiter
//<N
//>string|nil
//$= "2022/03/08":before("/", 2)
// Returns the portion of :mini:`String` before the :mini:`N`-th occurence of :mini:`Delimiter`, or :mini:`nil` if no :mini:`N`-th occurence if found.
// If :mini:`N < 0` then occurences are counted from the end of :mini:`String`.
	const char *Haystack = ml_string_value(Args[0]);
	size_t HaystackLength = ml_string_length(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	size_t NeedleLength = ml_string_length(Args[1]);
	int Index = ml_integer_value(Args[2]);
	if (Index > 0) {
		for (;;) {
			const char *Match = strstr(Haystack, Needle);
			if (!Match) return MLNil;
			if (--Index) {
				Haystack = Match + NeedleLength;
			} else {
				const char *Haystack = ml_string_value(Args[0]);
				return ml_string(Haystack, Match - Haystack);
			}
		}
	} else if (Index < 0) {
		for (int I = HaystackLength - NeedleLength; I >= 0; --I) {
			if (!memcmp(Haystack + I, Needle, NeedleLength)) {
				if (++Index) {
					I -= NeedleLength;
				} else {
					return ml_string(Haystack, I);
				}
			}
		}
		return MLNil;
	}
	return Args[0];
}

ML_METHOD("replace", MLStringT, MLStringT, MLStringT) {
//<String
//<Pattern
//<Replacement
//>string
// Returns a copy of :mini:`String` with each occurence of :mini:`Pattern` replaced by :mini:`Replacement`.
//$= "Hello world":replace("l", "bb")
	const char *Subject = ml_string_value(Args[0]);
	const char *SubjectEnd = Subject + ml_string_length(Args[0]);
	const char *Pattern = ml_string_value(Args[1]);
	int PatternLength = ml_string_length(Args[1]);
	if (!PatternLength) return ml_error("ValueError", "Empty pattern used in replace");
	const char *Replace = ml_string_value(Args[2]);
	int ReplaceLength = ml_string_length(Args[2]);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *Find = strstr(Subject, Pattern);
	while (Find) {
		if (Find > Subject) ml_stringbuffer_write(Buffer, Subject, Find - Subject);
		ml_stringbuffer_write(Buffer, Replace, ReplaceLength);
		Subject = Find + PatternLength;
		Find = strstr(Subject, Pattern);
	}
	if (SubjectEnd > Subject) {
		ml_stringbuffer_write(Buffer, Subject, SubjectEnd - Subject);
	}
	return ml_stringbuffer_get_value(Buffer);
}

ML_METHOD("replace", MLStringT, MLRegexT, MLStringT) {
//<String
//<Pattern
//<Replacement
//>string
// Returns a copy of :mini:`String` with each occurence of :mini:`Pattern` replaced by :mini:`Replacement`.
//$= "Hello world":replace(r"l+", "bb")
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	const char *Replace = ml_string_value(Args[2]);
	int ReplaceLength = ml_string_length(Args[2]);
	regmatch_t Matches[1];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (;;) {
#ifdef ML_TRE
		switch (regnexec(Regex, Subject, SubjectLength, 1, Matches, 0)) {

#else
		switch (regexec(Regex, Subject, 1, Matches, 0)) {
#endif
		case REG_NOMATCH:
			if (SubjectLength) ml_stringbuffer_write(Buffer, Subject, SubjectLength);
			return ml_stringbuffer_get_value(Buffer);
		case REG_ESPACE: {
			size_t ErrorSize = regerror(REG_ESPACE, Regex, NULL, 0);
			char *ErrorMessage = snew(ErrorSize + 1);
			regerror(REG_ESPACE, Regex, ErrorMessage, ErrorSize);
			return ml_error("RegexError", "regex error: %s", ErrorMessage);
		}
		default: {
			if (Matches[0].rm_eo == Matches[0].rm_so) return ml_error("RegexError", "empty regular expression used in replace");
			regoff_t Start = Matches[0].rm_so;
			if (Start > 0) ml_stringbuffer_write(Buffer, Subject, Start);
			ml_stringbuffer_write(Buffer, Replace, ReplaceLength);
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
} ml_str_replacement_t;

typedef struct {
	ml_state_t Base;
	ml_stringbuffer_t Buffer[1];
	const char *Subject;
	int Length, Count;
	ml_str_replacement_t Replacements[];
} ml_str_replacement_state_t;

static void ml_str_replacement_next(ml_str_replacement_state_t *State, ml_value_t *Value);

static void ml_str_replacement_func(ml_str_replacement_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)ml_str_replacement_next;
	ml_value_t **Args = ml_alloc_args(2);
	Args[0] = (ml_value_t *)State->Buffer;
	Args[1] = Value;
	ml_call(State, AppendMethod, 2, Args);
}

static void ml_str_replacement_next(ml_str_replacement_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	const char *Subject = State->Subject;
	int Length = State->Length;
	ml_stringbuffer_t *Buffer = State->Buffer;
	for (;;) {
		int MatchStart = Length, MatchEnd;
		ml_str_replacement_t *Test = State->Replacements, *Match = NULL;
		ml_value_t **SubArgs = NULL;
		int SubCount;
		for (int I = 0; I < State->Count; ++I, ++Test) {
			if (Test->PatternLength < 0) {
				regex_t *Regex = Test->Pattern.Regex;
				int NumSub = Test->Pattern.Regex->re_nsub + 1;
				regmatch_t Matches[NumSub];
#ifdef ML_TRE
				switch (regnexec(Regex, Subject, Length, NumSub, Matches, 0)) {
#else
				switch (regexec(Regex, Subject, NumSub, Matches, 0)) {
#endif
				case REG_NOMATCH:
					break;
				case REG_ESPACE: {
					size_t ErrorSize = regerror(REG_ESPACE, Test->Pattern.Regex, NULL, 0);
					char *ErrorMessage = snew(ErrorSize + 1);
					regerror(REG_ESPACE, Test->Pattern.Regex, ErrorMessage, ErrorSize);
					ML_ERROR("RegexError", "regex error: %s", ErrorMessage);
				}
				default: {
					if (Matches[0].rm_so < MatchStart) {
						MatchStart = Matches[0].rm_so;
						SubArgs = ml_alloc_args(NumSub);
						for (int I = 0; I < NumSub; ++I) {
							SubArgs[I] = ml_string(Subject + Matches[I].rm_so, Matches[I].rm_eo - Matches[I].rm_so);
						}
						SubCount = NumSub;
						MatchEnd = Matches[0].rm_eo;
						Match = Test;
					}
				}
				}
			} else {
				const char *Find = strstr(Subject, Test->Pattern.String);
				if (Find) {
					int Start = Find - Subject;
					if (Start < MatchStart) {
						MatchStart = Start;
						SubArgs = ml_alloc_args(1);
						SubArgs[0] = ml_string(Subject + Start, Test->PatternLength);
						SubCount = 1;
						MatchEnd = Start + Test->PatternLength;
						Match = Test;
					}
				}
			}
		}
		if (!Match) break;
		if (MatchStart) ml_stringbuffer_write(State->Buffer, Subject, MatchStart);
		Subject += MatchEnd;
		Length -= MatchEnd;
		if (Match->ReplacementLength < 0) {
			State->Base.run = (ml_state_fn)ml_str_replacement_func;
			State->Subject = Subject;
			State->Length = Length;
			return ml_call(State, Match->Replacement.Function, SubCount, SubArgs);
		} else {
			ml_stringbuffer_write(Buffer, Match->Replacement.String, Match->ReplacementLength);
		}
	}
	if (State->Length) ml_stringbuffer_write(Buffer, Subject, Length);
	ML_RETURN(ml_stringbuffer_get_value(Buffer));
}

ML_METHODX("replace", MLStringT, MLMapT) {
//<String
//<Replacements
//>string
// Each key in :mini:`Replacements` can be either a string or a regex. Each value in :mini:`Replacements` can be either a string or a function.
// Returns a copy of :mini:`String` with each matching string or regex from :mini:`Replacements` replaced with the corresponding value. Functions are called with the matched string or regex subpatterns.
//$- "the dog snored as he slept":replace({
//$-    r" ([a-z])" is fun(Match, A) '-{A:upper}',
//$-    "nor" is "narl"
//$= })
	int NumPatterns = ml_map_size(Args[1]);
	ml_str_replacement_state_t *State = xnew(ml_str_replacement_state_t, NumPatterns, ml_str_replacement_t);
	ml_str_replacement_t *Replacement = State->Replacements;
	ML_MAP_FOREACH(Args[1], Iter) {
		if (ml_is(Iter->Key, MLStringT)) {
			Replacement->Pattern.String = ml_string_value(Iter->Key);
			Replacement->PatternLength = ml_string_length(Iter->Key);
		} else if (ml_is(Iter->Key, MLRegexT)) {
			Replacement->Pattern.Regex = ml_regex_value(Iter->Key);
			Replacement->PatternLength = -1;
		} else {
			ML_ERROR("TypeError", "Unsupported pattern type: <%s>", ml_typeof(Iter->Key)->Name);
		}
		if (ml_is(Iter->Value, MLStringT)) {
			Replacement->Replacement.String = ml_string_value(Iter->Value);
			Replacement->ReplacementLength = ml_string_length(Iter->Value);
		} else if (ml_is(Iter->Value, MLFunctionT)) {
			Replacement->Replacement.Function = Iter->Value;
			Replacement->ReplacementLength = -1;
		} else {
			ML_ERROR("TypeError", "Unsupported replacement type: <%s>", ml_typeof(Iter->Value)->Name);
		}
		++Replacement;
	}
	State->Count = NumPatterns;
	State->Subject = ml_string_value(Args[0]);
	State->Length = ml_string_length(Args[0]);
	State->Buffer[0] = ML_STRINGBUFFER_INIT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	return ml_str_replacement_next(State, MLNil);
}

ML_METHODX("replace", MLStringT, MLRegexT, MLFunctionT) {
//<String
//<Pattern
//<Fn
//>string
// Returns a copy of :mini:`String` with each occurence of :mini:`Pattern` replaced by :mini:`Fn(Match, Sub/1, ..., Sub/n)` where :mini:`Match` is the actual matched text and :mini:`Sub/i` are the matched subpatterns.
//$= "the cat snored as he slept":replace(r" ([a-z])", fun(Match, A) '-{A:upper}')
	ml_str_replacement_state_t *State = xnew(ml_str_replacement_state_t, 1, ml_str_replacement_t);
	ml_str_replacement_t *Replacement = State->Replacements;
	Replacement->Pattern.Regex = ml_regex_value(Args[1]);
	Replacement->PatternLength = -1;
	Replacement->Replacement.Function = Args[2];
	Replacement->ReplacementLength = -1;
	State->Count = 1;
	State->Subject = ml_string_value(Args[0]);
	State->Length = ml_string_length(Args[0]);
	State->Buffer[0] = ML_STRINGBUFFER_INIT;
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	return ml_str_replacement_next(State, MLNil);
}

ML_METHOD("replace", MLStringT, MLIntegerT, MLStringT) {
//<String
//<I
//<Replacement
//>string
// Returns a copy of :mini:`String` with the :mini:`String[I]` is replaced by :mini:`Replacement`.
//$= "Hello world":replace(6, "_")
	const char *Start = ml_string_value(Args[0]);
	int Length = utf8_strlen(Args[0]);
	const char *End = Start + Length;
	int N = ml_integer_value_fast(Args[1]);
	if (N <= 0) N += Length + 1;
	if (N <= 0) return MLNil;
	if (N > Length) return MLNil;
	const char *A = Start;
	for (;;) {
		if (!utf8_is_continuation(*A) && (--N == 0)) break;
		++A;
	}
	const char *B = A + 1;
	while (*B && utf8_is_continuation(*B)) ++B;
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_write(Buffer, Start, A - Start);
	ml_stringbuffer_write(Buffer, ml_string_value(Args[2]), ml_string_length(Args[2]));
	ml_stringbuffer_write(Buffer, B, End - B);
	return ml_stringbuffer_get_value(Buffer);
}

ML_METHOD("replace", MLStringT, MLIntegerT, MLIntegerT, MLStringT) {
//<String
//<I
//<J
//<Replacement
//>string
// Returns a copy of :mini:`String` with the :mini:`String[I, J]` is replaced by :mini:`Replacement`.
//$= "Hello world":replace(1, 6, "Goodbye")
//$= "Hello world":replace(-6, 0, ", how are you?")
	const char *Start = ml_string_value(Args[0]);
	int Length = utf8_strlen(Args[0]);
	const char *End = Start + Length;
	int Lo = ml_integer_value_fast(Args[1]);
	int Hi = ml_integer_value_fast(Args[2]);
	if (Lo <= 0) Lo += Length + 1;
	if (Hi <= 0) Hi += Length + 1;
	if (Lo <= 0) return MLNil;
	if (Hi > Length + 1) return MLNil;
	if (Hi < Lo) return MLNil;
	Hi -= Lo;
	const char *A = Start;
	if (Lo > 0) for (;;) {
		if (!utf8_is_continuation(*A) && (--Lo == 0)) break;
		++A;
	}
	const char *B = A;
	if (++Hi > 0) while (*B) {
		if (!utf8_is_continuation(*B) && (--Hi == 0)) break;
		++B;
	}
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_write(Buffer, Start, A - Start);
	ml_stringbuffer_write(Buffer, ml_string_value(Args[3]), ml_string_length(Args[3]));
	ml_stringbuffer_write(Buffer, B, End - B);
	return ml_stringbuffer_get_value(Buffer);
}

typedef struct {
	ml_state_t Base;
	ml_stringbuffer_t Buffer[1];
	const char *Rest;
	int Length;
} ml_fn_replace_state_t;

static void ml_fn_replace_run(ml_fn_replace_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_stringbuffer_write(State->Buffer, State->Rest, State->Length);
	ML_RETURN(ml_stringbuffer_get_value(State->Buffer));
}

static void ml_fn_replace_func(ml_fn_replace_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	State->Base.run = (ml_state_fn)ml_fn_replace_run;
	ml_value_t **Args = ml_alloc_args(2);
	Args[0] = (ml_value_t *)State->Buffer;
	Args[1] = Value;
	return ml_call(State, AppendMethod, 2, Args);
}

ML_METHODX("replace", MLStringT, MLIntegerT, MLFunctionT) {
//<String
//<I
//<Fn
//>string
// Returns a copy of :mini:`String` with the :mini:`String[I]` is replaced by :mini:`Fn(String[I])`.
//$= "hello world":replace(1, :upper)
	const char *Start = ml_string_value(Args[0]);
	int Length = utf8_strlen(Args[0]);
	const char *End = Start + Length;
	int N = ml_integer_value_fast(Args[1]);
	if (N <= 0) N += Length + 1;
	if (N <= 0) ML_RETURN(MLNil);
	if (N > Length) ML_RETURN(MLNil);
	const char *A = Start;
	for (;;) {
		if (!utf8_is_continuation(*A) && (--N == 0)) break;
		++A;
	}
	const char *B = A + 1;
	while (*B && utf8_is_continuation(*B)) ++B;
	ml_fn_replace_state_t *State = new(ml_fn_replace_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_fn_replace_func;
	State->Buffer[0] = ML_STRINGBUFFER_INIT;
	State->Rest = B;
	State->Length = End - B;
	ml_stringbuffer_write(State->Buffer, Start, A - Start);
	ml_value_t **Args2 = ml_alloc_args(1);
	Args2[0] = ml_string(A, B - A);
	return ml_call(State, Args[2], 1, Args2);
}

ML_METHODX("replace", MLStringT, MLIntegerT, MLIntegerT, MLFunctionT) {
//<String
//<I
//<Fn
//>string
// Returns a copy of :mini:`String` with the :mini:`String[I, J]` is replaced by :mini:`Fn(String[I, J])`.
//$= "hello world":replace(1, 6, :upper)
	const char *Start = ml_string_value(Args[0]);
	int Length = utf8_strlen(Args[0]);
	const char *End = Start + Length;
	int Lo = ml_integer_value_fast(Args[1]);
	int Hi = ml_integer_value_fast(Args[2]);
	if (Lo <= 0) Lo += Length + 1;
	if (Hi <= 0) Hi += Length + 1;
	if (Lo <= 0) ML_RETURN(MLNil);
	if (Hi > Length + 1) ML_RETURN(MLNil);
	if (Hi < Lo) ML_RETURN(MLNil);
	Hi -= Lo;
	const char *A = Start;
	if (Lo > 0) for (;;) {
		if (!utf8_is_continuation(*A) && (--Lo == 0)) break;
		++A;
	}
	const char *B = A;
	if (++Hi > 0) while (*B) {
		if (!utf8_is_continuation(*B) && (--Hi == 0)) break;
		++B;
	}
	ml_fn_replace_state_t *State = new(ml_fn_replace_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_fn_replace_func;
	State->Buffer[0] = ML_STRINGBUFFER_INIT;
	State->Rest = B;
	State->Length = End - B;
	ml_stringbuffer_write(State->Buffer, Start, A - Start);
	ml_value_t **Args2 = ml_alloc_args(1);
	Args2[0] = ml_string(A, B - A);
	return ml_call(State, Args[3], 1, Args2);
}

ML_FUNCTION(MLStringEscape) {
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	const char *S = ml_string_value(Args[0]);
	for (int I = ml_string_length(Args[0]); --I >= 0; ++S) {
		switch (*S) {
		case '\0':
			ml_stringbuffer_write(Buffer, "\\0", strlen("\\0"));
			break;
		case '\a':
			ml_stringbuffer_write(Buffer, "\\a", strlen("\\a"));
			break;
		case '\b':
			ml_stringbuffer_write(Buffer, "\\b", strlen("\\b"));
			break;
		case '\t':
			ml_stringbuffer_write(Buffer, "\\t", strlen("\\t"));
			break;
		case '\r':
			ml_stringbuffer_write(Buffer, "\\r", strlen("\\r"));
			break;
		case '\n':
			ml_stringbuffer_write(Buffer, "\\n", strlen("\\n"));
			break;
		case '\\':
			ml_stringbuffer_write(Buffer, "\\\\", strlen("\\\\"));
			break;
		case '\'':
			ml_stringbuffer_write(Buffer, "\\\'", strlen("\\\'"));
			break;
		case '\"':
			ml_stringbuffer_write(Buffer, "\\\"", strlen("\\\""));
			break;
		default:
			ml_stringbuffer_write(Buffer, S, 1);
			break;
		}
	}
	return ml_stringbuffer_get_value(Buffer);
}

static long ml_regex_hash(ml_regex_t *Regex, ml_hash_chain_t *Chain) {
	long Hash = 5381;
	const char *Pattern = Regex->Pattern;
	while (*Pattern) Hash = ((Hash << 5) + Hash) + *(Pattern++);
	return Hash;
}

ML_FUNCTION(MLRegex) {
//@regex
//<String
//>regex | error
// Compiles :mini:`String` as a regular expression. Returns an error if :mini:`String` is not a valid regular expression.
//$= regex("[0-9]+")
//$= regex("[0-9")
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLStringT);
	const char *Pattern = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	if (Pattern[Length]) return ml_error("ValueError", "Regex pattern must be proper string");
	return ml_regex(Pattern, Length);
}

ML_TYPE(MLRegexT, (), "regex",
// A regular expression.
	.hash = (void *)ml_regex_hash,
	.Constructor = (ml_value_t *)MLRegex
);

ml_value_t *ml_regex(const char *Pattern, int Length) {
	ml_regex_t *Regex = new(ml_regex_t);
	Regex->Type = MLRegexT;
	Regex->Pattern = Pattern;
#ifdef ML_TRE
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

ml_value_t *ml_regexi(const char *Pattern, int Length) {
	ml_regex_t *Regex = new(ml_regex_t);
	Regex->Type = MLRegexT;
	Regex->Pattern = Pattern;
#ifdef ML_TRE
	int Error = regncomp(Regex->Value, Pattern, Length, REG_EXTENDED | REG_ICASE);
#else
	int Error = regcomp(Regex->Value, Pattern, REG_EXTENDED | REG_ICASE);
#endif
	if (Error) {
		size_t ErrorSize = regerror(Error, Regex->Value, NULL, 0);
		char *ErrorMessage = snew(ErrorSize + 1);
		regerror(Error, Regex->Value, ErrorMessage, ErrorSize);
		return ml_error("RegexError", "regex error: %s", ErrorMessage);
	}
	return (ml_value_t *)Regex;
}

const char *ml_regex_pattern(const ml_value_t *Value) {
	ml_regex_t *Regex = (ml_regex_t *)Value;
	return Regex->Pattern;
}

static int ML_TYPED_FN(ml_value_is_constant, MLRegexT, ml_value_t *Value) {
	return 1;
}

ML_METHOD("pattern", MLRegexT) {
//<Regex
//>string
// Returns the pattern used to create :mini:`Regex`.
//$= r"[0-9]+":pattern
	return ml_string(ml_regex_pattern(Args[0]), -1);
}

ML_METHOD("<>", MLRegexT, MLRegexT) {
//<A
//<B
//>integer
// Compares :mini:`A` and :mini:`B` lexicographically and returns :mini:`-1`, :mini:`0` or :mini:`1` respectively. Mainly for using regular expressions as keys in maps.
//$= r"[0-9]+" <> r"[A-Za-z0-9_]+"
//$= r"[A-Za-z0-9_]+" <> r"[0-9]+"
//$= r"[0-9]+" <> r"[0-9]+"
	const char *PatternA = ml_regex_pattern(Args[0]);
	const char *PatternB = ml_regex_pattern(Args[1]);
	int Compare = strcmp(PatternA, PatternB);
	if (Compare < 0) return (ml_value_t *)NegOne;
	if (Compare > 0) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

#define ml_comp_method_regex_regex(NAME, SYMBOL) \
ML_METHOD(#NAME, MLRegexT, MLRegexT) { \
/*>regex|nil
// Returns :mini:`Arg/2` if :mini:`Arg/1 NAME Arg/2` and :mini:`nil` otherwise.
//$= r"[0-9]+" NAME r"[A-Za-z0-9_]+"
//$= r"[A-Za-z0-9_]+" NAME r"[0-9]+"
//$= r"[0-9]+" NAME r"[0-9]+"
*/\
	const char *PatternA = ml_regex_pattern(Args[0]); \
	const char *PatternB = ml_regex_pattern(Args[1]); \
	int Compare = strcmp(PatternA, PatternB); \
	return Compare SYMBOL 0 ? Args[1] : MLNil; \
}

ml_comp_method_regex_regex(=, ==)
ml_comp_method_regex_regex(!=, !=)
ml_comp_method_regex_regex(<, <)
ml_comp_method_regex_regex(>, >)
ml_comp_method_regex_regex(<=, <=)
ml_comp_method_regex_regex(>=, >=)

ML_METHOD("append", MLStringBufferT, MLRegexT) {
//<Buffer
//<Value
// Appends a representation of :mini:`Value` to :mini:`Buffer`.
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_printf(Buffer, "/%s/", ml_regex_pattern(Args[1]));
	return MLSome;
}

typedef struct {
	ml_value_t *Index;
	ml_string_t *String;
	ml_regex_t *Regex;
} ml_string_case_t;

typedef struct {
	ml_type_t *Type;
	ml_string_case_t Cases[];
} ml_string_switch_t;

static void ml_string_switch(ml_state_t *Caller, ml_string_switch_t *Switch, int Count, ml_value_t **Args) {
	ML_CHECKX_ARG_COUNT(1);
	ml_value_t *Arg = ml_deref(Args[0]);
	if (!ml_is(Arg, MLStringT)) {
		ML_ERROR("TypeError", "expected string for argument 1");
	}
	const char *Subject = ml_string_value(Arg);
	size_t Length = ml_string_length(Arg);
	for (ml_string_case_t *Case = Switch->Cases;; ++Case) {
		if (Case->String) {
			if (Case->String->Length == Length) {
				if (!memcmp(Subject, Case->String->Value, Length)) ML_RETURN(Case->Index);
			}
		} else if (Case->Regex) {
#ifdef ML_TRE
			int Length = ml_string_length(Args[0]);
			if (!regnexec(Case->Regex->Value, Subject, Length, 0, NULL, 0)) {

#else
			if (!regexec(Case->Regex->Value, Subject, 0, NULL, 0)) {
#endif
				ML_RETURN(Case->Index);
			}
		} else {
			ML_RETURN(Case->Index);
		}
	}
	ML_RETURN(MLNil);
}

ML_TYPE(MLStringSwitchT, (MLFunctionT), "string-switch",
//!internal
	.call = (void *)ml_string_switch
);

ML_FUNCTION_INLINE(MLStringSwitch) {
//@string::switch
//<Cases...:string|regex
// Implements :mini:`switch` for string values. Case values must be strings or regular expressions.
//$- for Pet in ["cat", "dog", "mouse", "fox"] do
//$-    switch Pet: string
//$-       case "cat" do
//$-          print("Meow!\n")
//$-       case "dog" do
//$-          print("Woof!\n")
//$-       case "mouse" do
//$-          print("Squeak!\n")
//$-       else
//$-          print("???!")
//$-       end
//$= end
	int Total = 1;
	for (int I = 0; I < Count; ++I) Total += ml_list_length(Args[I]);
	ml_string_switch_t *Switch = xnew(ml_string_switch_t, Total, ml_string_case_t);
	Switch->Type = MLStringSwitchT;
	ml_string_case_t *Case = Switch->Cases;
	for (int I = 0; I < Count; ++I) {
		ML_CHECK_ARG_TYPE(I, MLListT);
		ML_LIST_FOREACH(Args[I], Iter) {
			ml_value_t *Value = Iter->Value;
			if (ml_is(Value, MLStringT)) {
				Case->String = (ml_string_t *)Value;
			} else if (ml_is(Value, MLRegexT)) {
				Case->Regex = (ml_regex_t *)Value;
			} else {
				return ml_error("ValueError", "Unsupported value in string case");
			}
			Case->Index = ml_integer(I);
			++Case;
		}
	}
	Case->Index = ml_integer(Count);
	return (ml_value_t *)Switch;
}

ml_value_t *ml_stringbuffer() {
	ml_stringbuffer_t *Buffer = new(ml_stringbuffer_t);
	Buffer->Type = MLStringBufferT;
	return (ml_value_t *)Buffer;
}

ML_FUNCTION(MLStringBuffer) {
//@string::buffer
//>string::buffer
// Returns a new :mini:`string::buffer`
	return ml_stringbuffer();
}

/*
ML_TYPE(MLStringBufferT, (MLStreamT), "stringbuffer");
// A string buffer that automatically grows and shrinks as required.
*/

ML_TYPE(MLStringBufferT, (), "stringbuffer",
//!internal
// A string buffer that automatically grows and shrinks as required.
	.Constructor = (ml_value_t *)MLStringBuffer
);

static GC_descr StringBufferDesc = 0;

#ifdef ML_THREADSAFE
static ml_stringbuffer_node_t * _Atomic StringBufferNodeCache = NULL;
#else
static ml_stringbuffer_node_t *StringBufferNodeCache = NULL;
#endif

static ml_stringbuffer_node_t *ml_stringbuffer_node() {
#ifdef ML_THREADSAFE
	ml_stringbuffer_node_t *Next = StringBufferNodeCache, *CacheNext;
	do {
		if (!Next) {
			Next = GC_MALLOC_EXPLICITLY_TYPED(sizeof(ml_stringbuffer_node_t), StringBufferDesc);
			break;
		}
		CacheNext = Next->Next;
	} while (!atomic_compare_exchange_weak(&StringBufferNodeCache, &Next, CacheNext));
#else
	ml_stringbuffer_node_t *Next = StringBufferNodeCache;
	if (!Next) {
		Next = GC_MALLOC_EXPLICITLY_TYPED(sizeof(ml_stringbuffer_node_t), StringBufferDesc);
	} else {
		StringBufferNodeCache = Next->Next;
	}
#endif
	Next->Next = NULL;
	return Next;
}

size_t ml_stringbuffer_reader(ml_stringbuffer_t *Buffer, size_t Length) {
	ml_stringbuffer_node_t *Node = Buffer->Head;
	Buffer->Length -= Length;
	Buffer->Start += Length;
	while (Node) {
		size_t Limit = ML_STRINGBUFFER_NODE_SIZE;
		if (Node == Buffer->Tail) Limit -= Buffer->Space;
		if (Buffer->Start < Limit) return Limit - Buffer->Start;
		ml_stringbuffer_node_t *Next = Node->Next;
#ifdef ML_THREADSAFE
		ml_stringbuffer_node_t *CacheNext = StringBufferNodeCache;
		do {
			Node->Next = CacheNext;
		} while (!atomic_compare_exchange_weak(&StringBufferNodeCache, &CacheNext, Node));
#else
		Node->Next = StringBufferNodeCache;
		StringBufferNodeCache = Node;
#endif
		Buffer->Start = 0;
		if (Next) {
			Node = Buffer->Head = Next;
		} else {
			Buffer->Head = Buffer->Tail = NULL;
			Buffer->Space = 0;
			break;
		}
	}
	return 0;
}

char *ml_stringbuffer_writer(ml_stringbuffer_t *Buffer, size_t Length) {
	ml_stringbuffer_node_t *Node = Buffer->Tail ?: (ml_stringbuffer_node_t *)&Buffer->Head;
	Buffer->Length += Length;
	Buffer->Space -= Length;
	if (!Buffer->Space) {
		Node = Node->Next = ml_stringbuffer_node();
		Buffer->Space = ML_STRINGBUFFER_NODE_SIZE;
		Buffer->Tail = Node;
	}
	return Node->Chars + ML_STRINGBUFFER_NODE_SIZE - Buffer->Space;
}

ssize_t ml_stringbuffer_write(ml_stringbuffer_t *Buffer, const char *String, size_t Length) {
	//fprintf(stderr, "ml_stringbuffer_add(%s, %ld)\n", String, Length);
	size_t Remaining = Length;
	ml_stringbuffer_node_t *Node = Buffer->Tail ?: (ml_stringbuffer_node_t *)&Buffer->Head;
	while (Buffer->Space < Remaining) {
		memcpy(Node->Chars + ML_STRINGBUFFER_NODE_SIZE - Buffer->Space, String, Buffer->Space);
		String += Buffer->Space;
		Remaining -= Buffer->Space;
		Node = Node->Next = ml_stringbuffer_node();
		Buffer->Space = ML_STRINGBUFFER_NODE_SIZE;
	}
	memcpy(Node->Chars + ML_STRINGBUFFER_NODE_SIZE - Buffer->Space, String, Remaining);
	Buffer->Space -= Remaining;
	Buffer->Length += Length;
	Buffer->Tail = Node;
	return Length;
}

ssize_t ml_stringbuffer_printf(ml_stringbuffer_t *Buffer, const char *Format, ...) {
	ml_stringbuffer_node_t *Node = Buffer->Tail ?: (ml_stringbuffer_node_t *)&Buffer->Head;
	int Space = Buffer->Space;
	if (!Space) {
		Node = Node->Next = ml_stringbuffer_node();
		Space = Buffer->Space = ML_STRINGBUFFER_NODE_SIZE;
		Buffer->Tail = Node;
	}
	char *Chars = Node->Chars + ML_STRINGBUFFER_NODE_SIZE - Space;
	va_list Args;
	va_start(Args, Format);
	int Length = vsnprintf(Chars, Space, Format, Args);
	va_end(Args);
	if (Length < Space) {
		Buffer->Space -= Length;
		Buffer->Length += Length;
	} else {
		char *Chars = alloca(Length + 1);
		va_list Args;
		va_start(Args, Format);
		vsprintf(Chars, Format, Args);
		va_end(Args);
		ml_stringbuffer_write(Buffer, Chars, Length);
	}
	return Length;
}

void ml_stringbuffer_put(ml_stringbuffer_t *Buffer, char Char) {
	ml_stringbuffer_node_t *Node = Buffer->Tail ?: (ml_stringbuffer_node_t *)&Buffer->Head;
	if (!Buffer->Space) {
		Node = Node->Next = ml_stringbuffer_node();
		Buffer->Space = ML_STRINGBUFFER_NODE_SIZE;
	}
	Node->Chars[ML_STRINGBUFFER_NODE_SIZE - Buffer->Space] = Char;
	Buffer->Space -= 1;
	Buffer->Length += 1;
	Buffer->Tail = Node;
}

void ml_stringbuffer_put32(ml_stringbuffer_t *Buffer, uint32_t Code) {
	char Val[8];
	uint32_t LeadByteMax = 0x7F;
	int I = 0;
	while (Code > LeadByteMax) {
		Val[I++] = (Code & 0x3F) | 0x80;
		Code >>= 6;
		LeadByteMax >>= (I == 1 ? 2 : 1);
	}
	Val[I++] = (Code & LeadByteMax) | (~LeadByteMax << 1);
	while (I--) ml_stringbuffer_put(Buffer, Val[I]);
}

static void ml_stringbuffer_finish(ml_stringbuffer_t *Buffer, char *String) {
	char *P = String;
	ml_stringbuffer_node_t *Node = Buffer->Head;
	int Start = Buffer->Start;
	while (Node->Next) {
		memcpy(P, Node->Chars + Start, ML_STRINGBUFFER_NODE_SIZE - Start);
		P += ML_STRINGBUFFER_NODE_SIZE - Start;
		Node = Node->Next;
		Start = 0;
	}
	memcpy(P, Node->Chars + Start, ML_STRINGBUFFER_NODE_SIZE - (Buffer->Space + Start));
	P += ML_STRINGBUFFER_NODE_SIZE - (Buffer->Space + Start);
	*P++ = 0;

	ml_stringbuffer_node_t *Head = Buffer->Head, *Tail = Buffer->Tail;
#ifdef ML_THREADSAFE
	ml_stringbuffer_node_t *CacheNext = StringBufferNodeCache;
	do {
		Tail->Next = CacheNext;
	} while (!atomic_compare_exchange_weak(&StringBufferNodeCache, &CacheNext, Head));
#else
	Tail->Next = StringBufferNodeCache;
	StringBufferNodeCache = Head;
#endif
	Buffer->Head = Buffer->Tail = NULL;
	Buffer->Length = Buffer->Space = Buffer->Start = 0;
}

char *ml_stringbuffer_get_string(ml_stringbuffer_t *Buffer) {
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

ml_value_t *ml_stringbuffer_get_value(ml_stringbuffer_t *Buffer) {
	size_t Length = Buffer->Length;
	if (Length == 0) {
		return ml_cstring("");
	} else {
		char *Chars = snew(Length + 1);
		ml_stringbuffer_finish(Buffer, Chars);
		return ml_string(Chars, Length);
	}
}

ML_METHOD("rest", MLStringBufferT) {
//<Buffer
//>string
// Returns the contents of :mini:`Buffer` as a string and clears :mini:`Buffer`.
//$- let B := string::buffer()
//$- B:write("Hello world")
//$= B:rest
//$= B:rest
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	return ml_stringbuffer_get_value(Buffer);
}

ML_METHOD("get", MLStringBufferT) {
//<Buffer
//>string
// Returns the contents of :mini:`Buffer` as a string and clears :mini:`Buffer`.
// .. deprecated:: 2.5.0
//
//    Use :mini:`Buffer:rest` instead.
//$- let B := string::buffer()
//$- B:write("Hello world")
//$= B:get
//$= B:get
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	return ml_stringbuffer_get_value(Buffer);
}

ML_METHOD("length", MLStringBufferT) {
//<Buffer
//>integer
// Returns the number of bytes currently available in :mini:`Buffer`.
//$- let B := string::buffer()
//$- B:write("Hello world")
//$= B:length
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	return ml_integer(Buffer->Length);
}

int ml_stringbuffer_foreach(ml_stringbuffer_t *Buffer, void *Data, int (*callback)(void *, const char *, size_t)) {
	ml_stringbuffer_node_t *Node = Buffer->Head;
	if (!Node) return 0;
	int Start = Buffer->Start;
	if (!Node->Next) {
		return callback(Data, Node->Chars + Start, (ML_STRINGBUFFER_NODE_SIZE - Buffer->Space) - Start);
	}
	if (callback(Data, Node->Chars + Start, ML_STRINGBUFFER_NODE_SIZE - Start)) return 1;
	Node = Node->Next;
	while (Node->Next) {
		if (callback(Data, Node->Chars, ML_STRINGBUFFER_NODE_SIZE)) return 1;
		Node = Node->Next;
	}
	return callback(Data, Node->Chars, ML_STRINGBUFFER_NODE_SIZE - Buffer->Space);
}

ml_value_t *ml_stringbuffer_simple_append(ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	ml_hash_chain_t *Chain = Buffer->Chain;
	for (ml_hash_chain_t *Link = Chain; Link; Link = Link->Previous) {
		if (Link->Value == Value) {
			int Index = Link->Index;
			if (!Index) Index = Link->Index = ++Buffer->Index;
			ml_stringbuffer_printf(Buffer, ">%d", Index);
			return (ml_value_t *)Buffer;
		}
	}
	ml_hash_chain_t NewChain[1] = {{Chain, Value, 0}};
	Buffer->Chain = NewChain;
	ml_value_t *Result = ml_simple_inline(AppendMethod, 2, Buffer, Value);
	if (NewChain->Index) ml_stringbuffer_printf(Buffer, "<%d", NewChain->Index);
	Buffer->Chain = Chain;
	return Result;
}

typedef struct {
	ml_state_t Base;
	ml_hash_chain_t Chain[1];
	ml_value_t *Args[2];
} ml_stringbuffer_append_state_t;

static void ml_stringbuffer_append_run(ml_stringbuffer_append_state_t *State, ml_value_t *Value) {
	ml_state_t *Caller = State->Base.Caller;
	if (ml_is_error(Value)) ML_RETURN(Value);
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)State->Args[0];
	if (State->Chain->Index) ml_stringbuffer_printf(Buffer, "<%d", State->Chain->Index);
	Buffer->Chain = State->Chain->Previous;
	ML_RETURN(Buffer);
}

void ml_stringbuffer_append(ml_state_t *Caller, ml_stringbuffer_t *Buffer, ml_value_t *Value) {
	for (ml_hash_chain_t *Link = Buffer->Chain; Link; Link = Link->Previous) {
		if (Link->Value == Value) {
			int Index = Link->Index;
			if (!Index) Index = Link->Index = ++Buffer->Index;
			ml_stringbuffer_printf(Buffer, ">%d", Index);
			ML_RETURN(Buffer);
		}
	}
	ml_stringbuffer_append_state_t *State = new(ml_stringbuffer_append_state_t);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_stringbuffer_append_run;
	State->Chain->Previous = Buffer->Chain;
	State->Chain->Value = Value;
	Buffer->Chain = State->Chain;
	State->Args[0] = (ml_value_t *)Buffer;
	State->Args[1] = Value;
	return ml_call(State, AppendMethod, 2, State->Args);
}

typedef struct {
	ml_state_t Base;
	ml_stringbuffer_t *Buffer;
	int Index, Count, Initial;
	ml_value_t *Args[];
} ml_write_state_t;

static void ml_string_buffer_write_run(ml_write_state_t *State, ml_value_t *Value) {
	if (ml_is_error(Value)) ML_CONTINUE(State->Base.Caller, Value);
	ml_stringbuffer_t *Buffer = State->Buffer;
	if (++State->Index < State->Count) return ml_stringbuffer_append((ml_state_t *)State, Buffer, State->Args[State->Index]);
	ML_CONTINUE(State->Base.Caller, ml_integer(State->Buffer->Length - State->Initial));
}

ML_METHODVX("write", MLStringBufferT, MLAnyT) {
//<Buffer
//<Value/1,...,Value/n
//>integer
// Writes each :mini:`Value/i` in turn to :mini:`Buffer`.
//$- let B := string::buffer()
//$- B:write("1 + 1 = ", 1 + 1)
//$= B:rest
	ml_write_state_t *State = xnew(ml_write_state_t, Count - 1, ml_value_t *);
	State->Base.Caller = Caller;
	State->Base.Context = Caller->Context;
	State->Base.run = (ml_state_fn)ml_string_buffer_write_run;
	State->Buffer = (ml_stringbuffer_t *)Args[0];
	State->Initial = State->Buffer->Length;
	State->Index = 0;
	State->Count = Count - 1;
	for (int I = 1; I < Count; ++I) State->Args[I - 1] = Args[I];
	return ml_stringbuffer_append((ml_state_t *)State, State->Buffer, State->Args[0]);
}

void ml_string_init() {
	GC_word StringBufferLayout[] = {1};
	StringBufferDesc = GC_make_descriptor(StringBufferLayout, 1);
	stringmap_insert(MLStringT->Exports, "buffer", MLStringBufferT);
	regcomp(IntFormat, "^\\s*%[-+ #'0]*[.0-9]*[diouxX]\\s*$", REG_NOSUB);
	regcomp(LongFormat, "^\\s*%[-+ #'0]*[.0-9]*l[diouxX]\\s*$", REG_NOSUB);
	regcomp(RealFormat, "^\\s*%[-+ #'0]*[.0-9]*[aefgAEG]\\s*$", REG_NOSUB);
	stringmap_insert(MLStringT->Exports, "switch", MLStringSwitch);
	stringmap_insert(MLStringT->Exports, "escape", MLStringEscape);
#include "ml_string_init.c"
#ifdef ML_GENERICS
	ml_type_t *TArgs[3] = {MLSequenceT, MLIntegerT, MLStringT};
	ml_type_add_parent(MLStringT, ml_generic_type(3, TArgs));
#endif
#ifdef ML_TRE
	ml_value_t *Features = ml_map();
	stringmap_insert(MLRegexT->Exports, "features", Features);
	int Result;
	if (!tre_config(TRE_CONFIG_APPROX, &Result)) {
		ml_map_insert(Features, ml_cstring("approx"), ml_boolean(Result));
	}
	if (!tre_config(TRE_CONFIG_WCHAR, &Result)) {
		ml_map_insert(Features, ml_cstring("wchar"), ml_boolean(Result));
	}
	if (!tre_config(TRE_CONFIG_MULTIBYTE, &Result)) {
		ml_map_insert(Features, ml_cstring("multibyte"), ml_boolean(Result));
	}
	const char *Version;
	if (!tre_config(TRE_CONFIG_VERSION, &Version)) {
		ml_map_insert(Features, ml_cstring("version"), ml_string(Version, -1));
	}
#endif
#ifdef ML_ICU
	stringmap_insert(MLStringT->Exports, "norm", MLStringNormT);
	stringmap_insert(MLStringT->Exports, "ctype", MLStringCTypeT);
#endif
}

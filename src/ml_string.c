#include "ml_string.h"
#include "minilang.h"
#include "ml_macros.h"
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <float.h>
#include <gc/gc_typed.h>

#include "ml_sequence.h"
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
#define ML_CATEGORY "string"

ML_TYPE(MLAddressT, (), "address");
//!address
// An address represents a read-only bounded section of memory.

ml_value_t *ml_address(const char *Value, int Length) {
	ml_address_t *Address = new(ml_address_t);
	Address->Type = MLAddressT;
	Address->Value = (char *)Value;
	Address->Length = Length;
	return (ml_value_t *)Address;
}

ML_METHOD("count", MLAddressT) {
//!address
//<Address
//>integer
// Returns the number bytes visible at :mini:`Address`.
	return ml_integer(ml_address_length(Args[0]));
}

ML_METHOD("@", MLAddressT, MLIntegerT) {
//!address
//<Address
//<Length
//>address
// Returns the same address as :mini:`Address`, limited to :mini:`Length` bytes.
	ml_address_t *Address = (ml_address_t *)Args[0];
	long Length = ml_integer_value_fast(Args[1]);
	if (Length > Address->Length) return ml_error("ValueError", "Size larger than buffer");
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
	ml_address_t *Address = (ml_address_t *)Args[0];
	long Offset = ml_integer_value_fast(Args[1]);
	if (Offset > Address->Length) return ml_error("ValueError", "Offset larger than buffer");
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
	ml_address_t *Address1 = (ml_address_t *)Args[0];
	ml_address_t *Address2 = (ml_address_t *)Args[1];
	int64_t Offset = Address1->Value - Address2->Value;
	if (Offset < 0 || Offset > Address2->Length) return ml_error("ValueError", "Addresses are not from same base");
	return ml_integer(Offset);
}

ML_METHOD("get8", MLAddressT) {
//!address
//<Address
//>integer
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 1) return ml_error("ValueError", "Buffer too small");
	return ml_integer(*(int8_t *)Address->Value);
}

ML_METHOD("get16", MLAddressT) {
//!address
//<Address
//>integer
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 2) return ml_error("ValueError", "Buffer too small");
	return ml_integer(*(int16_t *)Address->Value);
}

ML_METHOD("get32", MLAddressT) {
//!address
//<Address
//>integer
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 4) return ml_error("ValueError", "Buffer too small");
	return ml_integer(*(int32_t *)Address->Value);
}

ML_METHOD("get64", MLAddressT) {
//!address
//<Address
//>integer
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 8) return ml_error("ValueError", "Buffer too small");
	return ml_integer(*(int64_t *)Address->Value);
}

ML_METHOD("getf32", MLAddressT) {
//!address
//<Address
//>real
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 4) return ml_error("ValueError", "Buffer too small");
	return ml_real(*(float *)Address->Value);
}

ML_METHOD("getf64", MLAddressT) {
//!address
//<Address
//>real
	ml_address_t *Address = (ml_address_t *)Args[0];
	if (Address->Length < 8) return ml_error("ValueError", "Buffer too small");
	return ml_real(*(double *)Address->Value);
}

ML_METHOD("gets", MLAddressT) {
//!address
//<Address
//>string
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
	ml_address_t *Address = (ml_address_t *)Args[0];
	size_t Length = ml_integer_value(Args[1]);
	if (Length > Address->Length) return ml_error("ValueError", "Length larger than buffer");
	char *String = snew(Length + 1);
	memcpy(String, Address->Value, Length);
	String[Length] = 0;
	return ml_string(String, Length);
}

ML_FUNCTION(MLBuffer) {
//!buffer
//@buffer
//<Length
//>buffer
// Allocates a new buffer with :mini:`Length` bytes.
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLIntegerT);
	long Size = ml_integer_value_fast(Args[0]);
	if (Size < 0) return ml_error("ValueError", "Buffer size must be non-negative");
	ml_address_t *Buffer = new(ml_address_t);
	Buffer->Type = MLBufferT;
	Buffer->Length = Size;
	Buffer->Value = snew(Size);
	return (ml_value_t *)Buffer;
}

ML_TYPE(MLBufferT, (MLAddressT), "buffer",
//!buffer
// A buffer represents a writable bounded section of memory.
	.Constructor = (ml_value_t *)MLBuffer
);

ml_value_t *ml_buffer(char *Value, int Length) {
	ml_address_t *Buffer = new(ml_address_t);
	Buffer->Type = MLBufferT;
	Buffer->Value = Value;
	Buffer->Length = Length;
	return (ml_value_t *)Buffer;
}

ML_METHOD("@", MLBufferT, MLIntegerT) {
//!buffer
//<Buffer
//<Length
//>buffer
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	long Length = ml_integer_value_fast(Args[1]);
	if (Length > Buffer->Length) return ml_error("ValueError", "Size larger than buffer");
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
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	long Offset = ml_integer_value_fast(Args[1]);
	if (Offset > Buffer->Length) return ml_error("ValueError", "Offset larger than buffer");
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
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	if (Buffer->Length < 1) return ml_error("ValueError", "Buffer too small");
	*(int8_t *)Buffer->Value = ml_integer_value(Args[1]);
	return Args[0];
}

ML_METHOD("put16", MLBufferT, MLIntegerT) {
//!buffer
//<Buffer
//<Value
//>buffer
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	if (Buffer->Length < 2) return ml_error("ValueError", "Buffer too small");
	*(int16_t *)Buffer->Value = ml_integer_value(Args[1]);
	return Args[0];
}

ML_METHOD("put32", MLBufferT, MLIntegerT) {
//!buffer
//<Buffer
//<Value
//>buffer
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	if (Buffer->Length < 4) return ml_error("ValueError", "Buffer too small");
	*(int32_t *)Buffer->Value = ml_integer_value(Args[1]);
	return Args[0];
}

ML_METHOD("put64", MLBufferT, MLIntegerT) {
//!buffer
//<Buffer
//<Value
//>buffer
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	if (Buffer->Length < 8) return ml_error("ValueError", "Buffer too small");
	*(int64_t *)Buffer->Value = ml_integer_value(Args[1]);
	return Args[0];
}

ML_METHOD("putf32", MLBufferT, MLRealT) {
//!buffer
//<Buffer
//<Value
//>buffer
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	if (Buffer->Length < 4) return ml_error("ValueError", "Buffer too small");
	*(float *)Buffer->Value = ml_real_value(Args[1]);
	return Args[0];
}

ML_METHOD("putf64", MLBufferT, MLRealT) {
//!buffer
//<Buffer
//<Value
//>buffer
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	if (Buffer->Length < 8) return ml_error("ValueError", "Buffer too small");
	*(double *)Buffer->Value = ml_real_value(Args[1]);
	return Args[0];
}

ML_METHOD("put", MLBufferT, MLAddressT) {
//!buffer
//<Buffer
//<Value
//>buffer
	ml_address_t *Buffer = (ml_address_t *)Args[0];
	ml_address_t *Source = (ml_address_t *)Args[1];
	if (Buffer->Length < Source->Length) return ml_error("ValueError", "Buffer too small");
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

ML_TYPE(MLStringT, (MLAddressT, MLSequenceT), "string",
	.hash = (void *)ml_string_hash
);

ML_METHOD(MLStringT, MLAddressT) {
//!address
	ml_address_t *Address = (ml_address_t *)Args[0];
	return ml_string_format("#%" PRIxPTR ":%ld", (uintptr_t)Address->Value, Address->Length);
}

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

ml_value_t *ml_string_format(const char *Format, ...) {
	va_list Args;
	va_start(Args, Format);
	char *Value;
	int Length = vasprintf(&Value, Format, Args);
	va_end(Args);
	return ml_string(Value, Length);
}


ML_METHOD(MLStringT, MLNilT) {
	return ml_cstring("nil");
}

ML_METHOD(MLStringT, MLSomeT) {
	return ml_cstring("some");
}

ML_METHOD(MLStringT, MLBooleanT) {
//!boolean
	ml_boolean_t *Boolean = (ml_boolean_t *)Args[0];
	return ml_string(Boolean->Name, -1);
}

ML_METHOD(MLStringT, MLIntegerT) {
//!number
	char *Value;
	int Length = asprintf(&Value, "%ld", ml_integer_value_fast(Args[0]));
	return ml_string(Value, Length);
}

ML_METHOD(MLStringT, MLIntegerT, MLIntegerT) {
//!number
	int64_t Value = ml_integer_value_fast(Args[0]);
	int Base = ml_integer_value_fast(Args[1]);
	if (Base < 2 || Base > 36) return ml_error("RangeError", "Invalid base");
	int Max = 65;
	char *P = snew(Max + 1) + Max, *Q = P;
	*P = '\0';
	int64_t Neg = Value < 0 ? Value : -Value;
	do {
		*--P = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[-(Neg % Base)];
		Neg /= Base;
	} while (Neg);
	if (Value < 0) *--P = '-';
	return ml_string(P, Q - P);
}

static regex_t IntFormat[1];
static regex_t LongFormat[1];
static regex_t RealFormat[1];

ML_METHOD(MLStringT, MLIntegerT, MLStringT) {
	const char *Format = ml_string_value(Args[1]);
	int64_t Value = ml_integer_value_fast(Args[0]);
	char *String;
	int Length;
	if (!regexec(IntFormat, Format, 0, NULL, 0)) {
		Length = asprintf(&String, Format, (int)Value);
	} else if (!regexec(LongFormat, Format, 0, NULL, 0)) {
		Length = asprintf(&String, Format, (long)Value);
	} else if (!regexec(RealFormat, Format, 0, NULL, 0)) {
		Length = asprintf(&String, Format, (double)Value);
	} else {
		return ml_error("FormatError", "Invalid format string");
	}
	return ml_string(String, Length);
}

ML_METHOD(MLStringT, MLIntegerRangeT) {
	ml_integer_range_t *Range = (ml_integer_range_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_addf(Buffer, "%ld .. %ld by %ld", Range->Start, Range->Limit, Range->Step);
	return ml_stringbuffer_value(Buffer);
}

ML_METHOD(MLStringT, MLRealRangeT) {
	ml_real_range_t *Range = (ml_real_range_t *)Args[0];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	ml_stringbuffer_addf(Buffer, "%g .. %g by %g", Range->Start, Range->Limit, Range->Step);
	return ml_stringbuffer_value(Buffer);
}

ML_METHOD("ord", MLStringT) {
//<String
//>integer
// Returns the unicode codepoint of the first character of :mini:`String`.
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

ML_METHOD("chr", MLIntegerT) {
//<Codepoint
//>string
// Returns a string containing the character with unicode codepoint :mini:`Codepoint`.
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

ML_METHOD(MLStringT, MLDoubleT) {
//!number
	char *String;
	int Length = asprintf(&String, "%g", ml_double_value_fast(Args[0]));
	return ml_string(String, Length);
}

ML_METHOD(MLStringT, MLDoubleT, MLStringT) {
	const char *Format = ml_string_value(Args[1]);
	double Value = ml_double_value_fast(Args[0]);
	char *String;
	int Length;
	if (!regexec(IntFormat, Format, 0, NULL, 0)) {
		Length = asprintf(&String, Format, (int)Value);
	} else if (!regexec(LongFormat, Format, 0, NULL, 0)) {
		Length = asprintf(&String, Format, (long)Value);
	} else if (!regexec(RealFormat, Format, 0, NULL, 0)) {
		Length = asprintf(&String, Format, (double)Value);
	} else {
		return ml_error("FormatError", "Invalid format string");
	}
	return ml_string(String, Length);
}

#ifdef ML_COMPLEX

ML_METHOD(MLStringT, MLComplexT) {
//!number
	complex double Complex = ml_complex_value_fast(Args[0]);
	char *String;
	int Length;
	double Real = creal(Complex);
	double Imag = cimag(Complex);
	if (fabs(Real) <= DBL_EPSILON) {
		if (fabs(Imag - 1) <= DBL_EPSILON) {
			String = "i";
			Length = 1;
		} else if (fabs(Imag) <= DBL_EPSILON) {
			String = "0";
			Length = 1;
		} else {
			Length = asprintf(&String, "%gi", Imag);
		}
	} else if (fabs(Imag) <= DBL_EPSILON) {
		Length = asprintf(&String, "%g", Real);
	} else if (Imag < 0) {
		if (fabs(Imag + 1) <= DBL_EPSILON) {
			Length = asprintf(&String, "%g - i", Real);
		} else {
			Length = asprintf(&String, "%g - %gi", Real, -Imag);
		}
	} else {
		if (fabs(Imag - 1) <= DBL_EPSILON) {
			Length = asprintf(&String, "%g + i", Real);
		} else {
			Length = asprintf(&String, "%g + %gi", Real, Imag);
		}
	}
	return ml_string(String, Length);
}

ML_METHOD(MLStringT, MLComplexT, MLStringT) {
	const char *Format = ml_string_value(Args[1]);
	if (regexec(RealFormat, Format, 0, NULL, 0)) {
		return ml_error("FormatError", "Invalid format string");
	}
	complex double Complex = ml_complex_value_fast(Args[0]);
	double Real = creal(Complex);
	double Imag = cimag(Complex);
	char *ComplexFormat;
	if (Imag < 0) {
		Imag = -Imag;
		asprintf(&ComplexFormat, "%s - %si", Format, Format);
	} else {
		asprintf(&ComplexFormat, "%s + %si", Format, Format);
	}
	return ml_string_format(ComplexFormat, Real, Imag);
}

#endif

ML_METHOD(MLIntegerT, MLStringT) {
//!number
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
//!number
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
	ml_type_t *Type;
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

regex_t *ml_regex_value(const ml_value_t *Value) {
	ml_regex_t *Regex = (ml_regex_t *)Value;
	return Regex->Value;
}

const char *ml_regex_pattern(const ml_value_t *Value) {
	ml_regex_t *Regex = (ml_regex_t *)Value;
	return Regex->Pattern;
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

ML_METHOD("<>", MLRegexT, MLRegexT) {
	const char *PatternA = ml_regex_pattern(Args[0]);
	const char *PatternB = ml_regex_pattern(Args[1]);
	int Compare = strcmp(PatternA, PatternB);
	if (Compare < 0) return (ml_value_t *)NegOne;
	if (Compare > 0) return (ml_value_t *)One;
	return (ml_value_t *)Zero;
}

#define ml_comp_method_regex_regex(NAME, SYMBOL) \
ML_METHOD(NAME, MLRegexT, MLRegexT) { \
/*>regex|nil
// Returns :mini:`Arg/2` if :mini:`Arg/1 SYMBOL Arg/2` and :mini:`nil` otherwise.
*/\
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

ML_FUNCTION(MLStringSwitch) {
//@string::switch
//<Cases...:string|regex
// Implements :mini:`switch` for string values. Case values must be strings or regular expressions.
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
	return ml_stringbuffer();
}

ML_TYPE(MLStringBufferT, (), "stringbuffer",
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

static ML_METHOD_DECL(AppendMethod, "append");

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
	ml_value_t *Result = ml_simple_inline(AppendMethod, 2, Buffer, Value);
	Buffer->Chain = Chain;
	return Result;
}

ML_METHODV("append", MLStringBufferT, MLAnyT) {
	ml_value_t *String = ml_simple_call((ml_value_t *)MLStringT, Count - 1, Args + 1);
	if (ml_is_error(String)) return String;
	if (!ml_is(String, MLStringT)) return ml_error("TypeError", "String expected, not %s", ml_typeof(String)->Name);
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	int Length = ml_string_length(String);
	if (Length) {
		ml_stringbuffer_add(Buffer, ml_string_value(String), Length);
		return MLSome;
	} else {
		return MLNil;
	}
}

ML_METHODV("write", MLStringBufferT, MLAnyT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	for (int I = 1; I < Count; ++I) {
		ml_value_t *Result = ml_stringbuffer_append(Buffer, Args[I]);
		if (ml_is_error(Result)) return Result;
	}
	return Args[0];
}

ML_METHOD("append", MLStringBufferT, MLNilT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_add(Buffer, "nil", 3);
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLSomeT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_add(Buffer, "some", 4);
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLIntegerT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_addf(Buffer, "%ld", ml_integer_value_fast(Args[1]));
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLDoubleT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_addf(Buffer, "%g", ml_double_value_fast(Args[1]));
	return MLSome;
}

ML_METHOD("append", MLStringBufferT, MLStringT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	int Length = ml_string_length(Args[1]);
	if (Length) {
		ml_stringbuffer_add(Buffer, ml_string_value(Args[1]), Length);
		return MLSome;
	} else {
		return MLNil;
	}
}

ML_METHOD("[]", MLStringT, MLIntegerT) {
	const char *Chars = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	int Index = ml_integer_value_fast(Args[1]);
	if (Index <= 0) Index += Length + 1;
	if (Index <= 0) return MLNil;
	if (Index > Length) return MLNil;
	return ml_string(Chars + (Index - 1), 1);
}

ML_METHOD("[]", MLStringT, MLIntegerT, MLIntegerT) {
	const char *Chars = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	int Lo = ml_integer_value_fast(Args[1]);
	int Hi = ml_integer_value_fast(Args[2]);
	if (Lo <= 0) Lo += Length + 1;
	if (Hi <= 0) Hi += Length + 1;
	if (Lo <= 0) return MLNil;
	if (Hi > Length + 1) return MLNil;
	if (Hi < Lo) return MLNil;
	int Length2 = Hi - Lo;
	return ml_string(Chars + Lo - 1, Length2);
}

ML_METHOD("+", MLStringT, MLStringT) {
	int Length1 = ml_string_length(Args[0]);
	int Length2 = ml_string_length(Args[1]);
	int Length = Length1 + Length2;
	char *Chars = snew(Length + 1);
	memcpy(Chars, ml_string_value(Args[0]), Length1);
	memcpy(Chars + Length1, ml_string_value(Args[1]), Length2);
	Chars[Length] = 0;
	return ml_string(Chars, Length);
}

ML_METHOD("trim", MLStringT) {
	const unsigned char *Start = (const unsigned char *)ml_string_value(Args[0]);
	const unsigned char *End = Start + ml_string_length(Args[0]);
	while (Start < End && Start[0] <= ' ') ++Start;
	while (Start < End && End[-1] <= ' ') --End;
	int Length = End - Start;
	return ml_string((const char *)Start, Length);
}

ML_METHOD("trim", MLStringT, MLStringT) {
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
	const unsigned char *Start = (const unsigned char *)ml_string_value(Args[0]);
	const unsigned char *End = Start + ml_string_length(Args[0]);
	while (Start < End && Start[0] <= ' ') ++Start;
	int Length = End - Start;
	return ml_string((const char *)Start, Length);
}

ML_METHOD("ltrim", MLStringT, MLStringT) {
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
	const unsigned char *Start = (const unsigned char *)ml_string_value(Args[0]);
	const unsigned char *End = Start + ml_string_length(Args[0]);
	while (Start < End && End[-1] <= ' ') --End;
	int Length = End - Start;
	return ml_string((const char *)Start, Length);
}

ML_METHOD("rtrim", MLStringT, MLStringT) {
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
	return ml_integer(ml_string_length(Args[0]));
}

ML_METHOD("count", MLStringT) {
	return ml_integer(ml_string_length(Args[0]));
}

ML_METHOD("<>", MLStringT, MLStringT) {
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
/*>string|nil
// Returns :mini:`Arg/2` if :mini:`Arg/1 SYMBOL Arg/2` and :mini:`nil` otherwise.
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
	ml_value_t *Results = ml_list();
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	const char *SubjectEnd = Subject + SubjectLength;
	ml_regex_t *Pattern = (ml_regex_t *)Args[1];
	int Index = ml_integer_value(Args[2]);
	if (Index < 0 || Index >= Pattern->Value->re_nsub) return ml_error("RegexError", "Invalid regex group");

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

ML_METHOD("/*", MLStringT, MLStringT) {
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

ML_METHOD("lower", MLStringT) {
	const char *Source = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	char *Target = snew(Length + 1);
	for (int I = 0; I < Length; ++I) Target[I] = tolower(Source[I]);
	return ml_string(Target, Length);
}

ML_METHOD("upper", MLStringT) {
	const char *Source = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	char *Target = snew(Length + 1);
	for (int I = 0; I < Length; ++I) Target[I] = toupper(Source[I]);
	return ml_string(Target, Length);
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

ML_METHOD("find2", MLStringT, MLStringT) {
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
	const char *Haystack = ml_string_value(Args[0]);
	size_t HaystackLength = ml_string_length(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	int Start = ml_integer_value_fast(Args[2]);
	if (Start <= 0) Start += HaystackLength + 1;
	if (Start <= 0) return MLNil;
	if (Start > HaystackLength) return MLNil;
	Haystack += Start - 1;
	HaystackLength -= (Start - 1);
	const char *Match = strstr(Haystack, Needle);
	if (Match) {
		return ml_integer(Start + Match - Haystack);
	} else {
		return MLNil;
	}
}

ML_METHOD("find2", MLStringT, MLStringT, MLIntegerT) {
	const char *Haystack = ml_string_value(Args[0]);
	size_t HaystackLength = ml_string_length(Args[0]);
	const char *Needle = ml_string_value(Args[1]);
	int Start = ml_integer_value_fast(Args[2]);
	if (Start <= 0) Start += HaystackLength + 1;
	if (Start <= 0) return MLNil;
	if (Start > HaystackLength) return MLNil;
	Haystack += Start - 1;
	HaystackLength -= (Start - 1);
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
	return ml_integer(1 + Matches->rm_so);
}

ML_METHOD("find2", MLStringT, MLRegexT) {
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
	ml_tuple_set(Result, 1, ml_integer(1 + Matches->rm_so));
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
	const char *Haystack = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	int Start = ml_integer_value_fast(Args[2]);
	if (Start <= 0) Start += Length + 1;
	if (Start <= 0) return MLNil;
	if (Start > Length) return MLNil;
	Haystack += Start - 1;
	Length -= (Start - 1);
	regmatch_t Matches[1];
#ifdef ML_TRE
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
	const char *Haystack = ml_string_value(Args[0]);
	int Length = ml_string_length(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	int Start = ml_integer_value_fast(Args[2]);
	if (Start <= 0) Start += Length + 1;
	if (Start <= 0) return MLNil;
	if (Start > Length) return MLNil;
	Haystack += Start - 1;
	Length -= (Start - 1);
	regmatch_t Matches[Regex->re_nsub + 1];
#ifdef ML_TRE
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
	ml_tuple_set(Result, 1, ml_integer(Start + Matches->rm_so));
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

ML_METHOD("%", MLStringT, MLRegexT) {
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
	const char *Subject = ml_string_value(Args[0]);
	const char *Prefix = ml_string_value(Args[1]);
	int Length = ml_string_length(Args[1]);
	if (Length > ml_string_length(Args[0])) return MLNil;
	if (memcmp(Subject, Prefix, Length)) return MLNil;
	return Args[1];
}

ML_METHOD("starts", MLStringT, MLRegexT) {
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
		if (Start == 0) {
			size_t Length = Matches[0].rm_eo - Start;
			return ml_string(Subject + Start, Length);
		} else {
			return MLNil;
		}
	}
	}
}

ML_METHOD("ends", MLStringT, MLStringT) {
	const char *Subject = ml_string_value(Args[0]);
	const char *Suffix = ml_string_value(Args[1]);
	int Length = ml_string_length(Args[1]);
	int Length0 = ml_string_length(Args[0]);
	if (Length > Length0) return MLNil;
	if (memcmp(Subject + Length0 - Length, Suffix, Length)) return MLNil;
	return Args[1];
}

ML_METHOD("after", MLStringT, MLStringT) {
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
	const char *Subject = ml_string_value(Args[0]);
	int SubjectLength = ml_string_length(Args[0]);
	regex_t *Regex = ml_regex_value(Args[1]);
	ml_value_t *Replacer = Args[2];
	int NumSub = Regex->re_nsub + 1;
	regmatch_t Matches[NumSub];
	ml_value_t *SubArgs[NumSub];
	ml_stringbuffer_t Buffer[1] = {ML_STRINGBUFFER_INIT};
	for (;;) {
#ifdef ML_TRE
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
#ifdef ML_TRE
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

ML_METHOD(MLStringT, MLRegexT) {
	return ml_string_format("/%s/", ml_regex_pattern(Args[0]));
}

ML_METHOD("append", MLStringBufferT, MLRegexT) {
	ml_stringbuffer_t *Buffer = (ml_stringbuffer_t *)Args[0];
	ml_stringbuffer_addf(Buffer, "/%s/", ml_regex_pattern(Args[1]));
	return MLSome;
}

void ml_string_init() {
	GC_word StringBufferLayout[] = {1};
	StringBufferDesc = GC_make_descriptor(StringBufferLayout, 1);
	stringmap_insert(MLStringT->Exports, "buffer", MLStringBufferT);
	regcomp(IntFormat, "^%[-+ #'0]*[.0-9]*[dioxX]$", REG_NOSUB);
	regcomp(LongFormat, "^%[-+ #'0]*[.0-9]*l[dioxX]$", REG_NOSUB);
	regcomp(RealFormat, "^%[-+ #'0]*[.0-9]*[aefgAEG]$", REG_NOSUB);
	stringmap_insert(MLStringT->Exports, "switch", MLStringSwitch);
#include "ml_string_init.c"
	ml_method_by_value(MLStringT->Constructor, NULL, ml_identity, MLStringT, NULL);
}

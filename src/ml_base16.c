#include "minilang.h"
#include "ml_macros.h"

#undef ML_CATEGORY
#define ML_CATEGORY "base16"

ML_FUNCTION(Base16Encode) {
//@base16::encode
//<Address
//>string
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLAddressT);
	static const unsigned char HexDigits[] = "0123456789ABCDEF";
	size_t InSize = ml_address_length(Args[0]);
	size_t OutSize = 2 * InSize;
	const unsigned char *InChars = (const unsigned char *)ml_address_value(Args[0]);
	char *OutChars = snew(OutSize + 1), *Out = OutChars;
	for (int I = InSize; --I >= 0;) {
		unsigned char C = *InChars++;
		*Out++ = HexDigits[(C >> 4) & 15];
		*Out++ = HexDigits[C & 15];
	}
	*Out = 0;
	return ml_string(OutChars, OutSize);
}

ML_FUNCTION(Base16Decode) {
//@base16::decode
//<String
//>address
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLAddressT);
	size_t InSize = ml_address_length(Args[0]);
	size_t OutSize = (InSize + 1) / 2;
	const char *InChars = ml_address_value(Args[0]);
	char *OutChars = snew(OutSize + 1);
	unsigned char *Out = (unsigned char *)OutChars;
	int Odd = 1;
	for (int I = InSize; --I >= 0;) {
		unsigned char C = *InChars++;
		switch (C) {
		case 'a' ... 'f': C -= ('a' - 10); break;
		case 'A' ... 'F': C -= ('A' - 10); break;
		case '0' ... '9': C -= '0'; break;
		default: return ml_error("ValueError", "Invalid base 16 value");
		}
		if (Odd) {
			*Out = C << 4;
			Odd = 0;
		} else {
			*Out++ += C;
			Odd = 1;
		}
	}
	if (!Odd) Out++;
	*Out = 0;
	return ml_address(OutChars, OutSize);
}

void ml_base16_init(stringmap_t *Globals) {
#include "ml_base16_init.c"
	if (Globals) {
		stringmap_insert(Globals, "base16", ml_module("base16",
			"encode", Base16Encode,
			"decode", Base16Decode,
		NULL));
	}
}

#include "minilang.h"
#include "ml_macros.h"

#undef ML_CATEGORY
#define ML_CATEGORY "base64"

static const unsigned char Base64Chars[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static uint8_t Base64Value[256];

static void _encode(unsigned char *OutChars, const unsigned char *InChars, size_t InLength) {
	int NumBlocks = InLength / 3;
	int Remainder = InLength % 3;
	while (--NumBlocks >= 0) {
		unsigned char Char1 = *InChars++;
		unsigned char Char2 = *InChars++;
		unsigned char Char3 = *InChars++;
		OutChars[0] = Base64Chars[Char1 >> 2];
		OutChars[1] = Base64Chars[((Char1 << 4) & 63) + (Char2 >> 4)];
		OutChars[2] = Base64Chars[((Char2 << 2) & 63) + (Char3 >> 6)];
		OutChars[3] = Base64Chars[Char3 & 63];
		OutChars += 4;
	};
	if (Remainder == 1) {
		unsigned char Char1 = *InChars++;
		OutChars[0] = Base64Chars[Char1 >> 2];
		OutChars[1] = Base64Chars[(Char1 << 4) & 63];
		OutChars[2] = '=';
		OutChars[3] = '=';
	} else if (Remainder == 2) {
		unsigned char Char1 = *InChars++;
		unsigned char Char2 = *InChars++;
		OutChars[0] = Base64Chars[Char1 >> 2];
		OutChars[1] = Base64Chars[((Char1 << 4) & 63) + (Char2 >> 4)];
		OutChars[2] = Base64Chars[(Char2 << 2) & 63];
		OutChars[3] = '=';
	}
}

ML_FUNCTION(Base64Encode) {
//@base64::encode
//<Address
//>string
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLAddressT);
	size_t InSize = ml_address_length(Args[0]);
	size_t OutSize = 4 * ((InSize + 2) / 3);
	char *OutChars = snew(OutSize + 1);
	_encode((unsigned char *)OutChars, (const unsigned char *)ml_address_value(Args[0]), InSize);
	OutChars[OutSize] = 0;
	return ml_string(OutChars, OutSize);
}

static size_t _decode(unsigned char *OutChars, const unsigned char *InChars, size_t InLength) {
	int NumBlocks = InLength / 4;
	size_t OutLength = 0;
	while (--NumBlocks >= 0) {
		uint8_t Byte1 = Base64Value[*InChars++];
		if (Byte1 == 255) return OutLength;
		uint8_t Byte2 = Base64Value[*InChars++];
		if (Byte2 == 255) return OutLength;
		uint8_t Byte3 = Base64Value[*InChars++];
		if (Byte3 == 255) {
			OutChars[0] = (Byte1 << 2) + (Byte2 >> 4);
			return OutLength + 1;
		}
		uint8_t Byte4 = Base64Value[*InChars++];
		if (Byte4 == 255) {
			OutChars[0] = (Byte1 << 2) + (Byte2 >> 4);
			OutChars[1] = (Byte2 << 4) + (Byte3 >> 2);
			return OutLength + 2;
		}
		OutChars[0] = (Byte1 << 2) + (Byte2 >> 4);
		OutChars[1] = (Byte2 << 4) + (Byte3 >> 2);
		OutChars[2] = (Byte3 << 6) + Byte4;
		OutChars += 3;
		OutLength += 3;
	}
	return OutLength;
}

ML_FUNCTION(Base64Decode) {
//@base64::decode
//<String
//>address
	ML_CHECK_ARG_COUNT(1);
	ML_CHECK_ARG_TYPE(0, MLAddressT);
	size_t InSize = ml_address_length(Args[0]);
	size_t OutSize = (InSize / 4) * 3;
	char *OutChars = snew(OutSize + 1);
	OutSize = _decode((unsigned char *)OutChars, (const unsigned char *)ml_address_value(Args[0]), InSize);
	OutChars[OutSize] = 0;
	return ml_address(OutChars, OutSize);
}

void ml_base64_init(stringmap_t *Globals) {
#include "ml_base64_init.c"
	for (int I = 0; I < 256; ++I) Base64Value[I] = 255;
	for (int I = 0; I < 64; ++I) Base64Value[Base64Chars[I]] = I;
	if (Globals) {
		stringmap_insert(Globals, "base64", ml_module("base64",
			"encode", Base64Encode,
			"decode", Base64Decode,
		NULL));
	}
}

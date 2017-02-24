#include <cstring>

#include "conversion.hxx"

namespace replxx {

ConversionResult copyString8to32(char32_t* dst, size_t dstSize,
																				size_t& dstCount, const char* src) {
	const UTF8* sourceStart = reinterpret_cast<const UTF8*>(src);
	const UTF8* sourceEnd = sourceStart + strlen(src);
	UTF32* targetStart = reinterpret_cast<UTF32*>(dst);
	UTF32* targetEnd = targetStart + dstSize;

	ConversionResult res = ConvertUTF8toUTF32(
			&sourceStart, sourceEnd, &targetStart, targetEnd, lenientConversion);

	if (res == conversionOK) {
		dstCount = targetStart - reinterpret_cast<UTF32*>(dst);

		if (dstCount < dstSize) {
			*targetStart = 0;
		}
	}

	return res;
}

ConversionResult copyString8to32(char32_t* dst, size_t dstSize,
																				size_t& dstCount, const char8_t* src) {
	return copyString8to32(dst, dstSize, dstCount,
												 reinterpret_cast<const char*>(src));
}

size_t strlen32(const char32_t* str) {
	const char32_t* ptr = str;

	while (*ptr) {
		++ptr;
	}

	return ptr - str;
}

size_t strlen8(const char8_t* str) {
	return strlen(reinterpret_cast<const char*>(str));
}

char8_t* strdup8(const char* src) {
	return reinterpret_cast<char8_t*>(strdup(src));
}


void copyString32to16(char16_t* dst, size_t dstSize, size_t* dstCount,
														 const char32_t* src, size_t srcSize) {
	const UTF32* sourceStart = reinterpret_cast<const UTF32*>(src);
	const UTF32* sourceEnd = sourceStart + srcSize;
	char16_t* targetStart = reinterpret_cast<char16_t*>(dst);
	char16_t* targetEnd = targetStart + dstSize;

	ConversionResult res = ConvertUTF32toUTF16(
			&sourceStart, sourceEnd, &targetStart, targetEnd, lenientConversion);

	if (res == conversionOK) {
		*dstCount = targetStart - reinterpret_cast<char16_t*>(dst);

		if (*dstCount < dstSize) {
			*targetStart = 0;
		}
	}
}

void copyString32to8(char* dst, size_t dstSize, size_t* dstCount,
														const char32_t* src, size_t srcSize) {
	const UTF32* sourceStart = reinterpret_cast<const UTF32*>(src);
	const UTF32* sourceEnd = sourceStart + srcSize;
	UTF8* targetStart = reinterpret_cast<UTF8*>(dst);
	UTF8* targetEnd = targetStart + dstSize;

	ConversionResult res = ConvertUTF32toUTF8(
			&sourceStart, sourceEnd, &targetStart, targetEnd, lenientConversion);

	if (res == conversionOK) {
		*dstCount = targetStart - reinterpret_cast<UTF8*>(dst);

		if (*dstCount < dstSize) {
			*targetStart = 0;
		}
	}
}

void copyString32to8(char* dst, size_t dstLen, const char32_t* src) {
	size_t dstCount = 0;
	copyString32to8(dst, dstLen, &dstCount, src, strlen32(src));
}

void copyString32(char32_t* dst, const char32_t* src, size_t len) {
	while (0 < len && *src) {
		*dst++ = *src++;
		--len;
	}

	*dst = 0;
}

int strncmp32(const char32_t* left, const char32_t* right, size_t len) {
	while (0 < len && *left) {
		if (*left != *right) {
			return *left - *right;
		}

		++left;
		++right;
		--len;
	}

	return 0;
}

}

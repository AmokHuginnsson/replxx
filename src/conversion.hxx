#ifndef REPLXX_CONVERSION_HXX_INCLUDED
#define REPLXX_CONVERSION_HXX_INCLUDED 1

#include "ConvertUTF.h"

namespace replxx {

typedef unsigned char char8_t;

ConversionResult copyString8to32( char32_t* dst, int dstSize, int& dstCount, char const* src );
ConversionResult copyString8to32( char32_t* dst, int dstSize, int& dstCount, char8_t const* src );
void copyString32to8( char* dst, int dstSize, char32_t const* src, int srcSize, int* dstCount = nullptr );
void copyString32( char32_t* dst, char32_t const* src, size_t len );
int strncmp32( char32_t const* left, char32_t const* right, size_t len );

namespace locale {
extern bool is8BitEncoding;
}

}

#endif

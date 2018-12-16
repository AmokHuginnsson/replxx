#ifndef REPLXX_UTF8STRING_HXX_INCLUDED
#define REPLXX_UTF8STRING_HXX_INCLUDED

#include "unicodestring.hxx"

namespace replxx {

class Utf8String {
	Utf8String(const Utf8String&) = delete;
	Utf8String& operator=(const Utf8String&) = delete;

public:
	explicit Utf8String(const UnicodeString& src) {
		size_t len = src.length() * 4 + 1;
		_data = new char[len];
		copyString32to8(_data, len, src.get(), src.length());
	}

	~Utf8String() {
		delete[] _data;
	}

public:
	char* get() const {
		return _data;
	}

private:
	char* _data;
};

}

#endif


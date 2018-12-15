#ifndef REPLXX_UNICODESTRING_HXX_INCLUDED
#define REPLXX_UNICODESTRING_HXX_INCLUDED

#include <cstring>

#include "conversion.hxx"

namespace replxx {

class UnicodeString {
public:
	UnicodeString()
		: _length( 0 )
		, _data( nullptr ) {
		_data = new char32_t[1]();
	}

	explicit UnicodeString(const char* src)
		: _length( 0 )
		, _data( nullptr ) {
		size_t len = strlen(src);
		_data = new char32_t[len + 1]();
		copyString8to32(_data, len + 1, _length, src);
	}

	explicit UnicodeString(const char8_t* src)
		: _length( 0 )
		, _data( nullptr ) {
		size_t len = strlen(reinterpret_cast<const char*>(src));
		_data = new char32_t[len + 1]();
		copyString8to32(_data, len + 1, _length, src);
	}

	explicit UnicodeString(const char32_t* src)
		: _length( 0 )
		, _data( nullptr ) {
		for (_length = 0; src[_length] != 0; ++_length) {
		}
		_data = new char32_t[_length + 1]();
		memcpy(_data, src, _length * sizeof(char32_t));
	}

	explicit UnicodeString(const char32_t* src, int len)
		: _length( len )
		, _data( nullptr ) {
		_data = new char32_t[len + 1]();
		memcpy(_data, src, len * sizeof(char32_t));
	}

	explicit UnicodeString(int len)
		: _length( 0 )
		, _data( nullptr ) {
		_data = new char32_t[len]();
	}

	explicit UnicodeString(const UnicodeString& that)
		: _length( 0 )
		, _data( nullptr ) {
		_data = new char32_t[that._length + 1]();
		_length = that._length;
		memcpy(_data, that._data, sizeof(char32_t) * _length);
	}

	UnicodeString& operator=(const UnicodeString& that) {
		if (this != &that) {
			delete[] _data;
			_data = new char32_t[that._length]();
			_length = that._length;
			memcpy(_data, that._data, sizeof(char32_t) * _length);
		}

		return *this;
	}

	~UnicodeString() { delete[] _data; }

public:
	char32_t* get() const {
		return _data;
	}

	size_t length() const {
		return _length;
	}

	void initFromBuffer() {
		for (_length = 0; _data[_length] != 0; ++_length) {
		}
	}

	const char32_t& operator[](size_t pos) const {
		return _data[pos];
	}

	char32_t& operator[](size_t pos) {
		return _data[pos];
	}

 private:
	size_t _length;
	char32_t* _data;
};

}

#endif


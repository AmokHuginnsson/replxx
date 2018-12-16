#ifndef REPLXX_UNICODESTRING_HXX_INCLUDED
#define REPLXX_UNICODESTRING_HXX_INCLUDED

#include <vector>
#include <cstring>

#include "conversion.hxx"

namespace replxx {

class UnicodeString {
private:
	typedef std::vector<char32_t> data_buffer_t;
	data_buffer_t _data;
public:
	UnicodeString()
		: _data( 0 ) {
	}

	explicit UnicodeString( char const* src )
		: _data() {
		size_t byteCount( strlen( src ) );
		_data.resize( byteCount + 1 );
		int len( 0 );
		copyString8to32( _data.data(), byteCount + 1, len, src );
		_data.resize( len );
	}

	explicit UnicodeString( char8_t const* src )
		: UnicodeString( reinterpret_cast<const char*>( src ) ) {
	}

	explicit UnicodeString( char32_t const* src )
		: _data() {
		int len( 0 );
		while ( src[len] != 0 ) {
			++ len;
		}
		_data.assign( src, src + len );
	}

	explicit UnicodeString( char32_t const* src, int len )
		: _data() {
		_data.assign( src, src + len );
	}

	explicit UnicodeString( int len )
		: _data() {
		_data.resize( len );
	}

	explicit UnicodeString( UnicodeString const& ) = default;
	UnicodeString& operator = ( UnicodeString const& ) = default;

	UnicodeString& append( UnicodeString const& other ) {
		_data.insert( _data.end(), other._data.begin(), other._data.end() );
		return *this;
	}

	UnicodeString& append( char32_t const* src, int len ) {
		_data.insert( _data.end(), src, src + len );
		return *this;
	}

	char32_t const* get() const {
		return _data.data();
	}

	char32_t* get() {
		return _data.data();
	}

	int length() const {
		return static_cast<int>( _data.size() );
	}

	const char32_t& operator[]( size_t pos ) const {
		return _data[pos];
	}

	char32_t& operator[]( size_t pos ) {
		return _data[pos];
	}
};

}

#endif


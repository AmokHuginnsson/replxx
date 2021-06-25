#ifndef REPLXX_UTIL_HXX_INCLUDED
#define REPLXX_UTIL_HXX_INCLUDED 1

#include "replxx.hxx"

namespace replxx {

inline bool is_control_code(char32_t testChar) {
	return (testChar < ' ') ||											// C0 controls
				 (testChar >= 0x7F && testChar <= 0x9F);	// DEL and C1 controls
}

inline char32_t control_to_human( char32_t key ) {
	return ( key < 27 ? ( key + 0x40 ) : ( key + 0x18 ) );
}

class VisiblePromptSize {
	int _totalSize;
	int _lastLinePosition;
public:
	VisiblePromptSize( int totalSize_, int lastLinePosition_ )
		: _totalSize( totalSize_ )
		, _lastLinePosition( lastLinePosition_ ) {
	}
	int total_size( void ) const {
		return ( _totalSize );
	}
	int last_line_position( void ) const {
		return ( _lastLinePosition );
	}
};

VisiblePromptSize virtual_render( char32_t const*, int, int&, int&, int, char32_t*, int* );
void calculate_screen_position( int x, int y, int screenColumns, int charCount, int& xOut, int& yOut );
int calculate_displayed_length( char32_t const* buf32, int size );
char const* ansi_color( Replxx::Color );
std::string now_ms_str( void );

}

#endif


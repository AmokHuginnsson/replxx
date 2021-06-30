#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <wctype.h>

#include "util.hxx"
#include "terminal.hxx"

namespace replxx {

int mk_wcwidth( char32_t );

int virtual_render( char32_t const* display_, int size_, int& x_, int& y_, int screenColumns_, char32_t* rendered_, int* renderedSize_ ) {
	char32_t* out( rendered_ );
	int visibleCount( 0 );
	auto render = [&rendered_, &renderedSize_, &out, &visibleCount]( char32_t c_, bool visible_, bool renderAttributes_ = true ) {
		if ( rendered_ && renderedSize_ && renderAttributes_ ) {
			*out = c_;
			++ out;
			if ( visible_ ) {
				++ visibleCount;
			}
		}
	};
	auto advance_cursor = [&x_, &y_, &screenColumns_]( int by_ = 1 ) {
		x_ += by_;
		if ( x_ >= screenColumns_ ) {
			x_ = 0;
			++ y_;
		}
	};
	bool const renderAttributes( !!tty::out );
	int pos( 0 );
	while ( pos < size_ ) {
		char32_t c( display_[pos] );
		if ( ( c == '\n' ) || ( c == '\r' ) ) {
			render( c, true );
			x_ = 0;
			if ( c == '\n' ) {
				++ y_;
			}
			++ pos;
			continue;
		}
		if ( c == '\b' ) {
			render( c, true );
			-- x_;
			if ( x_ < 0 ) {
				x_ = screenColumns_ - 1;
				-- y_;
			}
			++ pos;
			continue;
		}
		if ( c == '\033' ) {
			render( c, false, renderAttributes );
			++ pos;
			if ( pos >= size_ ) {
				advance_cursor( 2 );
				continue;
			}
			c = display_[pos];
			if ( c != '[' ) {
				advance_cursor( 2 );
				continue;
			}
			render( c, false, renderAttributes );
			++ pos;
			if ( pos >= size_ ) {
				advance_cursor( 3 );
				continue;
			}
			int codeLen( 0 );
			while ( pos < size_ ) {
				c = display_[pos];
				if ( ( c != ';' ) && ( ( c < '0' ) || ( c > '9' ) ) ) {
					break;
				}
				render( c, false, renderAttributes );
				++ codeLen;
				++ pos;
			}
			if ( pos >= size_ ) {
				continue;
			}
			c = display_[pos];
			if ( c != 'm' ) {
				advance_cursor( 3 + codeLen );
				continue;
			}
			render( c, false, renderAttributes );
			++ pos;
			continue;
		}
		if ( is_control_code( c ) ) {
			render( c, true );
			advance_cursor( 2 );
			++ pos;
			continue;
		}
		int wcw( mk_wcwidth( c ) );
		if ( wcw < 0 ) {
			break;
		}
		render( c, true );
		advance_cursor( wcw );
		++ pos;
	}
	if ( rendered_ && renderedSize_ ) {
		*renderedSize_ = out - rendered_;
	}
	return ( visibleCount );
}

char const* ansi_color( Replxx::Color color_ ) {
	static char const reset[] = "\033[0m";
	static char const black[] = "\033[0;22;30m";
	static char const red[] = "\033[0;22;31m";
	static char const green[] = "\033[0;22;32m";
	static char const brown[] = "\033[0;22;33m";
	static char const blue[] = "\033[0;22;34m";
	static char const magenta[] = "\033[0;22;35m";
	static char const cyan[] = "\033[0;22;36m";
	static char const lightgray[] = "\033[0;22;37m";

#ifdef _WIN32
	static bool const has256colorDefault( true );
#else
	static bool const has256colorDefault( false );
#endif
	static char const* TERM( getenv( "TERM" ) );
	static bool const has256color( TERM ? ( strstr( TERM, "256" ) != nullptr ) : has256colorDefault );
	static char const* gray = has256color ? "\033[0;1;90m" : "\033[0;1;30m";
	static char const* brightred = has256color ? "\033[0;1;91m" : "\033[0;1;31m";
	static char const* brightgreen = has256color ? "\033[0;1;92m" : "\033[0;1;32m";
	static char const* yellow = has256color ? "\033[0;1;93m" : "\033[0;1;33m";
	static char const* brightblue = has256color ? "\033[0;1;94m" : "\033[0;1;34m";
	static char const* brightmagenta = has256color ? "\033[0;1;95m" : "\033[0;1;35m";
	static char const* brightcyan = has256color ? "\033[0;1;96m" : "\033[0;1;36m";
	static char const* white = has256color ? "\033[0;1;97m" : "\033[0;1;37m";
	static char const error[] = "\033[101;1;33m";

	char const* code( reset );
	switch ( color_ ) {
		case Replxx::Color::BLACK:         code = black;         break;
		case Replxx::Color::RED:           code = red;           break;
		case Replxx::Color::GREEN:         code = green;         break;
		case Replxx::Color::BROWN:         code = brown;         break;
		case Replxx::Color::BLUE:          code = blue;          break;
		case Replxx::Color::MAGENTA:       code = magenta;       break;
		case Replxx::Color::CYAN:          code = cyan;          break;
		case Replxx::Color::LIGHTGRAY:     code = lightgray;     break;
		case Replxx::Color::GRAY:          code = gray;          break;
		case Replxx::Color::BRIGHTRED:     code = brightred;     break;
		case Replxx::Color::BRIGHTGREEN:   code = brightgreen;   break;
		case Replxx::Color::YELLOW:        code = yellow;        break;
		case Replxx::Color::BRIGHTBLUE:    code = brightblue;    break;
		case Replxx::Color::BRIGHTMAGENTA: code = brightmagenta; break;
		case Replxx::Color::BRIGHTCYAN:    code = brightcyan;    break;
		case Replxx::Color::WHITE:         code = white;         break;
		case Replxx::Color::ERROR:         code = error;         break;
		case Replxx::Color::DEFAULT:       code = reset;         break;
	}
	return ( code );
}

std::string now_ms_str( void ) {
	std::chrono::milliseconds ms( std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ) );
	time_t t( ms.count() / 1000 );
	tm broken;
#ifdef _WIN32
#define localtime_r( t, b ) localtime_s( ( b ), ( t ) )
#endif
	localtime_r( &t, &broken );
#undef localtime_r
	static int const BUFF_SIZE( 32 );
	char str[BUFF_SIZE];
	strftime( str, BUFF_SIZE, "%Y-%m-%d %H:%M:%S.", &broken );
	snprintf( str + sizeof ( "YYYY-mm-dd HH:MM:SS" ), 5, "%03d", static_cast<int>( ms.count() % 1000 ) );
	return ( str );
}

}


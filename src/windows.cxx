#ifdef _WIN32

#include <iostream>

#include "windows.hxx"
#include "conversion.hxx"

namespace replxx {

WinAttributes WIN_ATTR;

size_t OutputWin(char16_t* text16, char32_t* text32, size_t len32) {
	size_t count16 = 0;

	copyString32to16(text16, len32, &count16, text32, len32);
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), text16,
								static_cast<DWORD>(count16), nullptr, nullptr);

	return count16;
}

template<typename T>
T* HandleEsc(T* p, T* end) {
	if (*p == '[') {
		int code = 0;

		int thisBackground( WIN_ATTR._defaultBackground );
		for (++p; p < end; ++p) {
			char32_t c = *p;

			if ('0' <= c && c <= '9') {
				code = code * 10 + (c - '0');
			} else if (c == 'm' || c == ';') {
				switch (code) {
					case 0:
						WIN_ATTR._consoleAttribute = WIN_ATTR._defaultAttribute;
						WIN_ATTR._consoleColor = WIN_ATTR._defaultColor | thisBackground;
						break;
					case 1:	// BOLD
					case 5:	// BLINK
						WIN_ATTR._consoleAttribute = (WIN_ATTR._defaultAttribute ^ FOREGROUND_INTENSITY) & INTENSITY;
						break;
					case 22:
						WIN_ATTR._consoleAttribute = WIN_ATTR._defaultAttribute;
						break;
					case 30:
					case 90:
						WIN_ATTR._consoleColor = BACKGROUND_WHITE;
						break;
					case 31:
					case 91:
						WIN_ATTR._consoleColor = FOREGROUND_RED | thisBackground;
						break;
					case 32:
					case 92:
						WIN_ATTR._consoleColor = FOREGROUND_GREEN | thisBackground;
						break;
					case 33:
					case 93:
						WIN_ATTR._consoleColor = FOREGROUND_RED | FOREGROUND_GREEN | thisBackground;
						break;
					case 34:
					case 94:
						WIN_ATTR._consoleColor = FOREGROUND_BLUE | thisBackground;
						break;
					case 35:
					case 95:
						WIN_ATTR._consoleColor = FOREGROUND_BLUE | FOREGROUND_RED | thisBackground;
						break;
					case 36:
					case 96:
						WIN_ATTR._consoleColor = FOREGROUND_BLUE | FOREGROUND_GREEN | thisBackground;
						break;
					case 37:
					case 97:
						WIN_ATTR._consoleColor = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE | thisBackground;
						break;
					case 101:
						thisBackground = BACKGROUND_RED;
						break;
				}

				if ( ( code >= 90 ) && ( code <= 97 ) ) {
					WIN_ATTR._consoleAttribute = (WIN_ATTR._defaultAttribute ^ FOREGROUND_INTENSITY) & INTENSITY;
				}

				code = 0;
			}

			if (*p == 'm') {
				++p;
				break;
			}
		}
	} else {
		++p;
	}

	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle,
													WIN_ATTR._consoleAttribute | WIN_ATTR._consoleColor);

	return p;
}

size_t WinWrite32(char16_t* text16, char32_t* text32, size_t len32) {
	char32_t* p = text32;
	char32_t* q = p;
	char32_t* e = text32 + len32;
	size_t count16 = 0;

	while (p < e) {
		if (*p == 27) {
			if (q < p) {
				count16 += OutputWin(text16, q, p - q);
			}

			q = p = HandleEsc(p + 1, e);
		} else {
			++p;
		}
	}

	if (q < p) {
		count16 += OutputWin(text16, q, p - q);
	}

	return count16;
}

int win_print( char const* str_, int size_ ) {
	int count( 0 );
	char const* s( str_ );
	char const* e( str_ + size_ );
	while ( str_ < e ) {
		if ( *str_ == 27 ) {
			if ( s < str_ ) {
				WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), s, static_cast<DWORD>( str_ - s ), nullptr, nullptr );
				count += ( str_ - s );
			}

			str_ = s = HandleEsc( str_ + 1, e );
		} else {
			++ str_;
		}
	}

	if ( s < str_ ) {
		WriteConsole( GetStdHandle( STD_OUTPUT_HANDLE ), s, static_cast<DWORD>( str_ - s ), nullptr, nullptr );
		count += ( str_ - s );
	}
	return ( count );
}

}

#endif


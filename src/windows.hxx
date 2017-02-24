#ifndef REPLXX_WINDOWS_HXX_INCLUDED
#define REPLXX_WINDOWS_HXX_INCLUDED 1

namespace replxx {

static const int FOREGROUND_WHITE =
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
static const int BACKGROUND_WHITE =
		BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
static const int INTENSITY = FOREGROUND_INTENSITY | BACKGROUND_INTENSITY;

class WinAttributes {
 public:
	WinAttributes() {
		CONSOLE_SCREEN_BUFFER_INFO info;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
		_defaultAttribute = info.wAttributes & INTENSITY;
		_defaultColor = info.wAttributes & FOREGROUND_WHITE;
		_defaultBackground = info.wAttributes & BACKGROUND_WHITE;

		_consoleAttribute = _defaultAttribute;
		_consoleColor = _defaultColor | _defaultBackground;
	}

 public:
	int _defaultAttribute;
	int _defaultColor;
	int _defaultBackground;

	int _consoleAttribute;
	int _consoleColor;
};

size_t OutputWin( char16_t* text16, char32_t* text32, size_t len32 );
char32_t* HandleEsc( char32_t* p, char32_t* end );
size_t WinWrite32( char16_t* text16, char32_t* text32, size_t len32 );

extern WinAttributes WIN_ATTR;

}

#endif

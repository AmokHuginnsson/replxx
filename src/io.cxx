#include <memory>
#include <cstdlib>

#ifdef _WIN32

#include <conio.h>
#include <windows.h>
#include <io.h>
#if _MSC_VER < 1900
#define snprintf _snprintf	// Microsoft headers use underscores in some names
#endif
#define strcasecmp _stricmp
#define strdup _strdup
#define isatty _isatty
#define write _write
#define STDIN_FILENO 0

#else /* _WIN32 */

#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <cctype>
#include <wctype.h>

#endif /* _WIN32 */

#include "io.hxx"
#include "conversion.hxx"

using namespace std;

namespace replxx {

int write32( int fd, char32_t* text32, int len32 ) {
#ifdef _WIN32
	if (isatty(fd)) {
		size_t len16 = 2 * len32 + 1;
		unique_ptr<char16_t[]> text16(new char16_t[len16]);
		size_t count16 = WinWrite32(text16.get(), text32, len32);

		return static_cast<int>(count16);
	} else {
		size_t len8 = 4 * len32 + 1;
		unique_ptr<char[]> text8(new char[len8]);
		size_t count8 = 0;

		copyString32to8(text8.get(), len8, &count8, text32, len32);

		return write(fd, text8.get(), static_cast<unsigned int>(count8));
	}
#else
	size_t len8 = 4 * len32 + 1;
	unique_ptr<char[]> text8(new char[len8]);
	size_t count8 = 0;

	copyString32to8(text8.get(), len8, &count8, text32, len32);

	return write(fd, text8.get(), count8);
#endif
}

int getScreenColumns(void) {
	int cols;
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO inf;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &inf);
	cols = inf.dwSize.X;
#else
	struct winsize ws;
	cols = (ioctl(1, TIOCGWINSZ, &ws) == -1) ? 80 : ws.ws_col;
#endif
	// cols is 0 in certain circumstances like inside debugger, which creates
	// further issues
	return (cols > 0) ? cols : 80;
}

int getScreenRows(void) {
	int rows;
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO inf;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &inf);
	rows = 1 + inf.srWindow.Bottom - inf.srWindow.Top;
#else
	struct winsize ws;
	rows = (ioctl(1, TIOCGWINSZ, &ws) == -1) ? 24 : ws.ws_row;
#endif
	return (rows > 0) ? rows : 24;
}

void setDisplayAttribute(bool enhancedDisplay) {
#ifdef _WIN32
	if (enhancedDisplay) {
		CONSOLE_SCREEN_BUFFER_INFO inf;
		GetConsoleScreenBufferInfo(console_out, &inf);
		oldDisplayAttribute = inf.wAttributes;
		BYTE oldLowByte = oldDisplayAttribute & 0xFF;
		BYTE newLowByte;
		switch (oldLowByte) {
			case 0x07:
				// newLowByte = FOREGROUND_BLUE | FOREGROUND_INTENSITY;	// too dim
				// newLowByte = FOREGROUND_BLUE;												 // even dimmer
				newLowByte = FOREGROUND_BLUE |
										 FOREGROUND_GREEN;	// most similar to xterm appearance
				break;
			case 0x70:
				newLowByte = BACKGROUND_BLUE | BACKGROUND_INTENSITY;
				break;
			default:
				newLowByte = oldLowByte ^ 0xFF;	// default to inverse video
				break;
		}
		inf.wAttributes = (inf.wAttributes & 0xFF00) | newLowByte;
		SetConsoleTextAttribute(console_out, inf.wAttributes);
	} else {
		SetConsoleTextAttribute(console_out, oldDisplayAttribute);
	}
#else
	if (enhancedDisplay) {
		if (write(1, "\x1b[1;34m", 7) == -1)
			return; /* bright blue (visible with both B&W bg) */
	} else {
		if (write(1, "\x1b[0m", 4) == -1) return; /* reset */
	}
#endif
}

}


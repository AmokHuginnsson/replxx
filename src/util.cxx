#include <cstdlib>
#include <cstring>
#include <wctype.h>

#include "util.hxx"
#include "keycodes.hxx"

namespace replxx {

/**
 * convert {CTRL + 'A'}, {CTRL + 'a'} and {CTRL + ctrlChar( 'A' )} into
 * ctrlChar( 'A' )
 * leave META alone
 *
 * @param c character to clean up
 * @return cleaned-up character
 */
int cleanupCtrl(int c) {
	if (c & CTRL) {
		int d = c & 0x1FF;
		if (d >= 'a' && d <= 'z') {
			c = (c + ('a' - ctrlChar('A'))) & ~CTRL;
		}
		if (d >= 'A' && d <= 'Z') {
			c = (c + ('A' - ctrlChar('A'))) & ~CTRL;
		}
		if (d >= ctrlChar('A') && d <= ctrlChar('Z')) {
			c = c & ~CTRL;
		}
	}
	return c;
}

/**
 * Recompute widths of all characters in a char32_t buffer
 * @param text					input buffer of Unicode characters
 * @param widths				output buffer of character widths
 * @param charCount		 number of characters in buffer
 */
int mk_wcwidth(char32_t ucs);

void recomputeCharacterWidths(const char32_t* text, char* widths,
																		 int charCount) {
	for (int i = 0; i < charCount; ++i) {
		widths[i] = mk_wcwidth(text[i]);
	}
}

/**
 * Calculate a new screen position given a starting position, screen width and
 * character count
 * @param x						 initial x position (zero-based)
 * @param y						 initial y position (zero-based)
 * @param screenColumns screen column count
 * @param charCount		 character positions to advance
 * @param xOut					returned x position (zero-based)
 * @param yOut					returned y position (zero-based)
 */
void calculateScreenPosition(int x, int y, int screenColumns,
																		int charCount, int& xOut, int& yOut) {
	xOut = x;
	yOut = y;
	int charsRemaining = charCount;
	while (charsRemaining > 0) {
		int charsThisRow = (x + charsRemaining < screenColumns) ? charsRemaining
																														: screenColumns - x;
		xOut = x + charsThisRow;
		yOut = y;
		charsRemaining -= charsThisRow;
		x = 0;
		++y;
	}
	if (xOut == screenColumns) {	// we have to special-case line wrap
		xOut = 0;
		++yOut;
	}
}

/**
 * Calculate a column width using mk_wcswidth()
 * @param buf32	text to calculate
 * @param len		length of text to calculate
 */
int mk_wcswidth(const char32_t* pwcs, size_t n);

int calculateColumnPosition(char32_t* buf32, int len) {
	int width = mk_wcswidth(reinterpret_cast<const char32_t*>(buf32), len);
	if (width == -1)
		return len;
	else
		return width;
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

}


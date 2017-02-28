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

char32_t* HandleEsc(char32_t* p, char32_t* end) {
	if (*p == '[') {
		int code = 0;

		for (++p; p < end; ++p) {
			char32_t c = *p;

			if ('0' <= c && c <= '9') {
				code = code * 10 + (c - '0');
			} else if (c == 'm' || c == ';') {
				switch (code) {
					case 0:
						WIN_ATTR._consoleAttribute = WIN_ATTR._defaultAttribute;
						WIN_ATTR._consoleColor =
								WIN_ATTR._defaultColor | WIN_ATTR._defaultBackground;
						break;

					case 1:	// BOLD
					case 5:	// BLINK
						WIN_ATTR._consoleAttribute =
								(WIN_ATTR._defaultAttribute ^ FOREGROUND_INTENSITY) & INTENSITY;
						break;

					case 22:
						WIN_ATTR._consoleAttribute = WIN_ATTR._defaultAttribute;
						break;

					case 30:
						WIN_ATTR._consoleColor = BACKGROUND_WHITE;
						break;

					case 31:
						WIN_ATTR._consoleColor =
								FOREGROUND_RED | WIN_ATTR._defaultBackground;
						break;

					case 32:
						WIN_ATTR._consoleColor =
								FOREGROUND_GREEN | WIN_ATTR._defaultBackground;
						break;

					case 33:
						WIN_ATTR._consoleColor =
								FOREGROUND_RED | FOREGROUND_GREEN | WIN_ATTR._defaultBackground;
						break;

					case 34:
						WIN_ATTR._consoleColor =
								FOREGROUND_BLUE | WIN_ATTR._defaultBackground;
						break;

					case 35:
						WIN_ATTR._consoleColor =
								FOREGROUND_BLUE | FOREGROUND_RED | WIN_ATTR._defaultBackground;
						break;

					case 36:
						WIN_ATTR._consoleColor = FOREGROUND_BLUE | FOREGROUND_GREEN |
																		 WIN_ATTR._defaultBackground;
						break;

					case 37:
						WIN_ATTR._consoleColor = FOREGROUND_GREEN | FOREGROUND_RED |
																		 FOREGROUND_BLUE |
																		 WIN_ATTR._defaultBackground;
						break;
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

}

#endif


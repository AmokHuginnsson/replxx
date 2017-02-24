#ifndef REPLXX_PROMPT_HXX_INCLUDED
#define REPLXX_PROMPT_HXX_INCLUDED 1

#include "util.hxx"
#include "io.hxx"

namespace replxx {

/**
 * Recompute widths of all characters in a char32_t buffer
 * @param text					input buffer of Unicode characters
 * @param widths				output buffer of character widths
 * @param charCount		 number of characters in buffer
 */
int mk_wcwidth(char32_t ucs);

inline void recomputeCharacterWidths(const char32_t* text, char* widths,
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
inline void calculateScreenPosition(int x, int y, int screenColumns,
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

inline int calculateColumnPosition(char32_t* buf32, int len) {
	int width = mk_wcswidth(reinterpret_cast<const char32_t*>(buf32), len);
	if (width == -1)
		return len;
	else
		return width;
}

struct PromptBase {						// a convenience struct for grouping prompt info
	Utf32String promptText;			// our copy of the prompt text, edited
	char* promptCharWidths;			// character widths from mk_wcwidth()
	int promptChars;						 // chars in promptText
	int promptBytes;						 // bytes in promptText
	int promptExtraLines;				// extra lines (beyond 1) occupied by prompt
	int promptIndentation;			 // column offset to end of prompt
	int promptLastLinePosition;	// index into promptText where last line begins
	int promptPreviousInputLen;	// promptChars of previous input line, for
															 // clearing
	int promptCursorRowOffset;	 // where the cursor is relative to the start of
															 // the prompt
	int promptScreenColumns;		 // width of screen in columns
	int promptPreviousLen;			 // help erasing
	int promptErrorCode;				 // error code (invalid UTF-8) or zero

	PromptBase() : promptPreviousInputLen(0) {}

	bool write() {
		if (write32(1, promptText.get(), promptBytes) == -1) return false;

		return true;
	}
};

struct PromptInfo : public PromptBase {
	PromptInfo(const char* textPtr, int columns) {
		promptExtraLines = 0;
		promptLastLinePosition = 0;
		promptPreviousLen = 0;
		promptScreenColumns = columns;
		Utf32String tempUnicode(textPtr);

		// strip control characters from the prompt -- we do allow newline
		char32_t* pIn = tempUnicode.get();
		char32_t* pOut = pIn;

		int len = 0;
		int x = 0;

		bool const strip = (isatty(1) == 0);

		while (*pIn) {
			char32_t c = *pIn;
			if ('\n' == c || !isControlChar(c)) {
				*pOut = c;
				++pOut;
				++pIn;
				++len;
				if ('\n' == c || ++x >= promptScreenColumns) {
					x = 0;
					++promptExtraLines;
					promptLastLinePosition = len;
				}
			} else if (c == '\x1b') {
				if (strip) {
					// jump over control chars
					++pIn;
					if (*pIn == '[') {
						++pIn;
						while (*pIn && ((*pIn == ';') || ((*pIn >= '0' && *pIn <= '9')))) {
							++pIn;
						}
						if (*pIn == 'm') {
							++pIn;
						}
					}
				} else {
					// copy control chars
					*pOut = *pIn;
					++pOut;
					++pIn;
					if (*pIn == '[') {
						*pOut = *pIn;
						++pOut;
						++pIn;
						while (*pIn && ((*pIn == ';') || ((*pIn >= '0' && *pIn <= '9')))) {
							*pOut = *pIn;
							++pOut;
							++pIn;
						}
						if (*pIn == 'm') {
							*pOut = *pIn;
							++pOut;
							++pIn;
						}
					}
				}
			} else {
				++pIn;
			}
		}
		*pOut = 0;
		promptChars = len;
		promptBytes = static_cast<int>(pOut - tempUnicode.get());
		promptText = tempUnicode;

		promptIndentation = len - promptLastLinePosition;
		promptCursorRowOffset = promptExtraLines;
	}
};

// Used with DynamicPrompt (history search)
//
static const Utf32String forwardSearchBasePrompt("(i-search)`");
static const Utf32String reverseSearchBasePrompt("(reverse-i-search)`");
static const Utf32String endSearchBasePrompt("': ");
static Utf32String
		previousSearchText;	// remembered across invocations of replxx_input()

// changing prompt for "(reverse-i-search)`text':" etc.
//
struct DynamicPrompt : public PromptBase {
	Utf32String searchText;	// text we are searching for
	char* searchCharWidths;	// character widths from mk_wcwidth()
	int searchTextLen;			 // chars in searchText
	int direction;					 // current search direction, 1=forward, -1=reverse

	DynamicPrompt(PromptBase& pi, int initialDirection)
			: searchTextLen(0), direction(initialDirection) {
		promptScreenColumns = pi.promptScreenColumns;
		promptCursorRowOffset = 0;
		Utf32String emptyString(1);
		searchText = emptyString;
		const Utf32String* basePrompt =
				(direction > 0) ? &forwardSearchBasePrompt : &reverseSearchBasePrompt;
		size_t promptStartLength = basePrompt->length();
		promptChars =
				static_cast<int>(promptStartLength + endSearchBasePrompt.length());
		promptBytes = promptChars;
		promptLastLinePosition = promptChars;	// TODO fix this, we are asssuming
																					 // that the history prompt won't wrap
																					 // (!)
		promptPreviousLen = promptChars;
		Utf32String tempUnicode(promptChars + 1);
		memcpy(tempUnicode.get(), basePrompt->get(),
					 sizeof(char32_t) * promptStartLength);
		memcpy(&tempUnicode[promptStartLength], endSearchBasePrompt.get(),
					 sizeof(char32_t) * (endSearchBasePrompt.length() + 1));
		tempUnicode.initFromBuffer();
		promptText = tempUnicode;
		calculateScreenPosition(0, 0, pi.promptScreenColumns, promptChars,
														promptIndentation, promptExtraLines);
	}

	void updateSearchPrompt(void) {
		const Utf32String* basePrompt =
				(direction > 0) ? &forwardSearchBasePrompt : &reverseSearchBasePrompt;
		size_t promptStartLength = basePrompt->length();
		promptChars = static_cast<int>(promptStartLength + searchTextLen +
																	 endSearchBasePrompt.length());
		promptBytes = promptChars;
		Utf32String tempUnicode(promptChars + 1);
		memcpy(tempUnicode.get(), basePrompt->get(),
					 sizeof(char32_t) * promptStartLength);
		memcpy(&tempUnicode[promptStartLength], searchText.get(),
					 sizeof(char32_t) * searchTextLen);
		size_t endIndex = promptStartLength + searchTextLen;
		memcpy(&tempUnicode[endIndex], endSearchBasePrompt.get(),
					 sizeof(char32_t) * (endSearchBasePrompt.length() + 1));
		tempUnicode.initFromBuffer();
		promptText = tempUnicode;
	}

	void updateSearchText(const char32_t* textPtr) {
		Utf32String tempUnicode(textPtr);
		searchTextLen = static_cast<int>(tempUnicode.chars());
		searchText = tempUnicode;
		updateSearchPrompt();
	}
};

}

#endif

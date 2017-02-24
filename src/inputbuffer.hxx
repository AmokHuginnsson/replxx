#ifndef REPLXX_INPUTBUFFER_HXX_INCLUDED
#define REPLXX_INPUTBUFFER_HXX_INCLUDED 1

#include <vector>

#include "prompt.hxx"

struct replxx_completions {
	std::vector<replxx::Utf32String> completionStrings;
};

namespace replxx {

struct PromptBase;

class InputBuffer {
	char32_t* buf32;	// input buffer
	char* charWidths;	// character widths from mk_wcwidth()
	int buflen;				// buffer size in characters
	int len;					// length of text in input buffer
	int pos;					// character position in buffer ( 0 <= pos <= len )

	void clearScreen(PromptBase& pi);
	int incrementalHistorySearch(PromptBase& pi, int startChar);
	int completeLine(PromptBase& pi);
	void refreshLine(PromptBase& pi);

 public:
	InputBuffer(char32_t* buffer, char* widthArray, int bufferLen)
			: buf32(buffer),
				charWidths(widthArray),
				buflen(bufferLen - 1),
				len(0),
				pos(0) {
		buf32[0] = 0;
	}
	void preloadBuffer( char const* preloadText );
	int getInputLine(PromptBase& pi);
	int length(void) const { return len; }
};

}

#endif


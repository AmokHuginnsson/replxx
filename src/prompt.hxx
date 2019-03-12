#ifndef REPLXX_PROMPT_HXX_INCLUDED
#define REPLXX_PROMPT_HXX_INCLUDED 1

#include <cstdlib>

#include "unicodestring.hxx"

namespace replxx {
struct PromptBase {						// a convenience struct for grouping prompt info
	UnicodeString promptText;			// our copy of the prompt text, edited
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

	PromptBase( int );
	void write();
};

struct PromptInfo : public PromptBase {
	PromptInfo(std::string const& textPtr, int columns);
};

extern UnicodeString previousSearchText;	// remembered across invocations of replxx_input()

// changing prompt for "(reverse-i-search)`text':" etc.
//
struct DynamicPrompt : public PromptBase {
	UnicodeString searchText; // text we are searching for
	int direction;            // current search direction, 1=forward, -1=reverse

	DynamicPrompt(PromptBase& pi, int initialDirection);
	void updateSearchPrompt(void);
};

}

#endif

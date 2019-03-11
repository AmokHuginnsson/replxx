#ifdef _WIN32

#include <conio.h>
#include <windows.h>
#include <io.h>
#if _MSC_VER < 1900 && defined (_MSC_VER)
#define snprintf _snprintf	// Microsoft headers use underscores in some names
#endif
#define strcasecmp _stricmp
#define strdup _strdup
#define write _write
#define STDIN_FILENO 0

#else /* _WIN32 */

#include <unistd.h>

#endif /* _WIN32 */

#include "prompt.hxx"
#include "util.hxx"
#include "io.hxx"

namespace replxx {


PromptBase::PromptBase( int columns_ )
	: promptExtraLines( 0 )
	, promptLastLinePosition( 0 )
	, promptPreviousInputLen( 0 )
	, promptScreenColumns( columns_ )
	, promptPreviousLen( 0 ) {
}

bool PromptBase::write() {
	if ( write32(1, promptText.get(), promptBytes) == -1 ) {
		return false;
	}
	return true;
}

PromptInfo::PromptInfo( std::string const& text_, int columns )
	: PromptBase( columns ) {
	UnicodeString tempUnicode( text_ );

	// strip control characters from the prompt -- we do allow newline
	UnicodeString::const_iterator in( tempUnicode.begin() );
	UnicodeString::iterator out( tempUnicode.begin() );

	int len = 0;
	int x = 0;

	bool const strip = !tty::out;

	while (in != tempUnicode.end()) {
		char32_t c = *in;
		if ('\n' == c || !isControlChar(c)) {
			*out = c;
			++out;
			++in;
			++len;
			if ('\n' == c || ++x >= promptScreenColumns) {
				x = 0;
				++promptExtraLines;
				promptLastLinePosition = len;
			}
		} else if (c == '\x1b') {
			if (strip) {
				// jump over control chars
				++in;
				if (*in == '[') {
					++in;
					while ( ( in != tempUnicode.end() ) && ( ( *in == ';' ) || ( ( ( *in >= '0' ) && ( *in <= '9' ) ) ) ) ) {
						++in;
					}
					if (*in == 'm') {
						++in;
					}
				}
			} else {
				// copy control chars
				*out = *in;
				++out;
				++in;
				if (*in == '[') {
					*out = *in;
					++out;
					++in;
					while ( ( in != tempUnicode.end() ) && ( ( *in == ';' ) || ( ( ( *in >= '0' ) && ( *in <= '9' ) ) ) ) ) {
						*out = *in;
						++out;
						++in;
					}
					if (*in == 'm') {
						*out = *in;
						++out;
						++in;
					}
				}
			}
		} else {
			++in;
		}
	}
	promptChars = len;
	promptBytes = static_cast<int>(out - tempUnicode.begin());
	promptText = tempUnicode;

	promptIndentation = len - promptLastLinePosition;
	promptCursorRowOffset = promptExtraLines;
}

// Used with DynamicPrompt (history search)
//
const UnicodeString forwardSearchBasePrompt("(i-search)`");
const UnicodeString reverseSearchBasePrompt("(reverse-i-search)`");
const UnicodeString endSearchBasePrompt("': ");
UnicodeString previousSearchText;	// remembered across invocations of replxx_input()

DynamicPrompt::DynamicPrompt(PromptBase& pi, int initialDirection)
	: PromptBase( pi.promptScreenColumns )
	, searchText()
	, direction( initialDirection ) {
	promptScreenColumns = pi.promptScreenColumns;
	promptCursorRowOffset = 0;
	const UnicodeString* basePrompt =
			(direction > 0) ? &forwardSearchBasePrompt : &reverseSearchBasePrompt;
	size_t promptStartLength = basePrompt->length();
	promptChars = static_cast<int>(promptStartLength + endSearchBasePrompt.length());
	promptBytes = promptChars;
	promptLastLinePosition = promptChars; // TODO fix this, we are asssuming
																				// that the history prompt won't wrap (!)
	promptPreviousLen = promptChars;
	promptText.assign( *basePrompt ).append( endSearchBasePrompt );
	calculateScreenPosition(
		0, 0, pi.promptScreenColumns, promptChars,
		promptIndentation, promptExtraLines
	);
}

void DynamicPrompt::updateSearchPrompt(void) {
	const UnicodeString* basePrompt =
			(direction > 0) ? &forwardSearchBasePrompt : &reverseSearchBasePrompt;
	size_t promptStartLength = basePrompt->length();
	promptChars = static_cast<int>(promptStartLength + searchText.length() +
																 endSearchBasePrompt.length());
	promptBytes = promptChars;
	promptText.assign( *basePrompt ).append( searchText ).append( endSearchBasePrompt );
}

}


#include <algorithm>
#include <memory>
#include <cerrno>
#include <iostream>

#ifdef _WIN32

#include <windows.h>
#include <io.h>
#if _MSC_VER < 1900
#define snprintf _snprintf // Microsoft headers use underscores in some names
#endif
#define strcasecmp _stricmp
#define write _write
#define STDIN_FILENO 0

#else /* _WIN32 */

#include <unistd.h>
#include <signal.h>

#endif /* _WIN32 */

#ifdef _WIN32
#include "windows.hxx"
#endif

#include "replxx_impl.hxx"
#include "utf8string.hxx"
#include "prompt.hxx"
#include "util.hxx"
#include "io.hxx"
#include "keycodes.hxx"
#include "history.hxx"
#include "replxx.hxx"

using namespace std;

namespace replxx {

void dynamicRefresh(Prompt& pi, char32_t* buf32, int len, int pos);

#ifndef _WIN32

bool gotResize = false;

#endif

namespace {

static int const REPLXX_MAX_HINT_ROWS( 4 );
/*
 * All whitespaces and all non-alphanumerical characters from ASCII range
 * with an exception of an underscore ('_').
 */
char const defaultBreakChars[] = " \t\v\f\a\b\r\n`~!@#$%^&*()-=+[{]}\\|;:'\",<.>/?";

#ifndef _WIN32

static void WindowSizeChanged(int) {
	// do nothing here but setting this flag
	gotResize = true;
}

#endif

static const char* unsupported_term[] = {"dumb", "cons25", "emacs", NULL};

static bool isUnsupportedTerm(void) {
	char* term = getenv("TERM");
	if (term == NULL) {
		return false;
	}
	for (int j = 0; unsupported_term[j]; ++j) {
		if (!strcasecmp(term, unsupported_term[j])) {
			return true;
		}
	}
	return false;
}

}

Replxx::ReplxxImpl::ReplxxImpl( FILE*, FILE*, FILE* )
	: _utf8Buffer()
	, _data()
	, _charWidths()
	, _display()
	, _hint()
	, _pos( 0 )
	, _prefix( 0 )
	, _hintSelection( -1 )
	, _history()
	, _killRing()
	, _maxHintRows( REPLXX_MAX_HINT_ROWS )
	, _breakChars( defaultBreakChars )
	, _completionCountCutoff( 100 )
	, _doubleTabCompletion( false )
	, _completeOnEmpty( true )
	, _beepOnAmbiguousCompletion( false )
	, _noColor( false )
	, _keyPressHandlers()
	, _prompt()
	, _completionCallback( nullptr )
	, _highlighterCallback( nullptr )
	, _hintCallback( nullptr )
	, _preloadedBuffer()
	, _errorMessage() {
	using namespace std::placeholders;
	_keyPressHandlers.insert( make_pair( ctrlChar( 'A' ),        std::bind( &ReplxxImpl::go_to_begining_of_line, this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( HOME_KEY,               std::bind( &ReplxxImpl::go_to_begining_of_line, this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( ctrlChar( 'E' ),        std::bind( &ReplxxImpl::go_to_end_of_line,      this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( END_KEY,                std::bind( &ReplxxImpl::go_to_end_of_line,      this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( ctrlChar( 'B' ),        std::bind( &ReplxxImpl::move_one_char_left,     this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( LEFT_ARROW_KEY,         std::bind( &ReplxxImpl::move_one_char_left,     this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( ctrlChar( 'F' ),        std::bind( &ReplxxImpl::move_one_char_right,    this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( RIGHT_ARROW_KEY,        std::bind( &ReplxxImpl::move_one_char_right,    this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( META + 'b',             std::bind( &ReplxxImpl::move_one_word_left,     this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( META + 'B',             std::bind( &ReplxxImpl::move_one_word_left,     this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( CTRL + LEFT_ARROW_KEY,  std::bind( &ReplxxImpl::move_one_word_left,     this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( META + LEFT_ARROW_KEY,  std::bind( &ReplxxImpl::move_one_word_left,     this, _1 ) ) ); // Emacs allows Meta, bash & readline don't
	_keyPressHandlers.insert( make_pair( META + 'f',             std::bind( &ReplxxImpl::move_one_word_right,    this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( META + 'F',             std::bind( &ReplxxImpl::move_one_word_right,    this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( CTRL + RIGHT_ARROW_KEY, std::bind( &ReplxxImpl::move_one_word_right,    this, _1 ) ) );
	_keyPressHandlers.insert( make_pair( META + RIGHT_ARROW_KEY, std::bind( &ReplxxImpl::move_one_word_right,    this, _1 ) ) ); // Emacs allows Meta, bash & readline don't
}

void Replxx::ReplxxImpl::clear( void ) {
	_pos = 0;
	_prefix = 0;
	_data.clear();
	_hintSelection = -1;
	_hint = UnicodeString();
	_display.clear();
}

Replxx::ReplxxImpl::completions_t Replxx::ReplxxImpl::call_completer( std::string const& input, int& contextLen_ ) const {
	Replxx::completions_t completionsIntermediary(
		!! _completionCallback
			? _completionCallback( input, contextLen_ )
			: Replxx::completions_t()
	);
	completions_t completions;
	completions.reserve( completionsIntermediary.size() );
	for ( std::string const& c : completionsIntermediary ) {
		completions.emplace_back( c.c_str() );
	}
	return ( completions );
}

Replxx::ReplxxImpl::hints_t Replxx::ReplxxImpl::call_hinter( std::string const& input, int& contextLen, Replxx::Color& color ) const {
	Replxx::hints_t hintsIntermediary(
		!! _hintCallback
			? _hintCallback( input, contextLen, color )
			: Replxx::hints_t()
	);
	hints_t hints;
	hints.reserve( hintsIntermediary.size() );
	for ( std::string const& h : hintsIntermediary ) {
		hints.emplace_back( h.c_str() );
	}
	return ( hints );
}

void Replxx::ReplxxImpl::set_preload_buffer( std::string const& preloadText ) {
	_preloadedBuffer = preloadText;
	// remove characters that won't display correctly
	bool controlsStripped = false;
	int whitespaceSeen( 0 );
	for ( std::string::iterator it( _preloadedBuffer.begin() ); it != _preloadedBuffer.end(); ) {
		unsigned char c = *it;
		if ( '\r' == c ) { // silently skip CR
			_preloadedBuffer.erase( it, it + 1 );
			continue;
		}
		if ( '\n' == c || '\t' == c ) { // note newline or tab
			++ whitespaceSeen;
			++ it;
			continue;
		}
		if ( whitespaceSeen > 0 ) {
			it -= whitespaceSeen;
			*it = ' ';
			_preloadedBuffer.erase( it + 1, it + whitespaceSeen - 1 );
		}
		if ( isControlChar( c ) ) { // remove other control characters, flag for message
			controlsStripped = true;
			if ( whitespaceSeen > 0 ) {
				_preloadedBuffer.erase( it, it + 1 );
				-- it;
			} else {
				*it = ' ';
			}
		}
		whitespaceSeen = 0;
		++ it;
	}
	_errorMessage.clear();
	if ( controlsStripped ) {
		_errorMessage.assign( " [Edited line: control characters were converted to spaces]\n" );
	}
}

char const* Replxx::ReplxxImpl::read_from_stdin( void ) {
	if ( _preloadedBuffer.empty() ) {
		getline( cin, _preloadedBuffer );
		if ( ! cin.good() ) {
			return nullptr;
		}
	}
	while ( ! _preloadedBuffer.empty() && ( ( _preloadedBuffer.back() == '\r' ) || ( _preloadedBuffer.back() == '\n' ) ) ) {
		_preloadedBuffer.pop_back();
	}
	_utf8Buffer.assign( _preloadedBuffer );
	_preloadedBuffer.clear();
	return _utf8Buffer.get();
}

char const* Replxx::ReplxxImpl::input( std::string const& prompt ) {
#ifndef _WIN32
	gotResize = false;
#endif
	try {
		errno = 0;
		if ( ! tty::in ) { // input not from a terminal, we should work with piped input, i.e. redirected stdin
			return ( read_from_stdin() );
		}
		if (!_errorMessage.empty()) {
			printf("%s", _errorMessage.c_str());
			fflush(stdout);
			_errorMessage.clear();
		}
		_prompt.set_text( prompt );
		if ( isUnsupportedTerm() ) {
			_prompt.write();
			fflush(stdout);
			return ( read_from_stdin() );
		}
		if (enableRawMode() == -1) {
			return nullptr;
		}
		clear();
		if (!_preloadedBuffer.empty()) {
			preloadBuffer(_preloadedBuffer.c_str());
			_preloadedBuffer.clear();
		}
		if ( getInputLine() == -1 ) {
			return ( nullptr );
		}
		disableRawMode();
		printf("\n");
		_utf8Buffer.assign( _data );
		return ( _utf8Buffer.get() );
	} catch ( std::exception const& ) {
		disableRawMode();
		return ( nullptr );
	}
}

void Replxx::ReplxxImpl::clear_screen( void ) {
	replxx::clear_screen( CLEAR_SCREEN::WHOLE );
}

int Replxx::ReplxxImpl::install_window_change_handler( void ) {
#ifndef _WIN32
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = &WindowSizeChanged;

	if (sigaction(SIGWINCH, &sa, nullptr) == -1) {
		return errno;
	}
#endif
	return 0;
}

int Replxx::ReplxxImpl::print( char const* str_, int size_ ) {
#ifdef _WIN32
	int count( win_write( str_, size_ ) );
#else
	int count( write( 1, str_, size_ ) );
#endif
	return ( count );
}

void Replxx::ReplxxImpl::preloadBuffer(const char* preloadText) {
	_data.assign( preloadText );
	_charWidths.resize( _data.length() );
	recomputeCharacterWidths( _data.get(), _charWidths.data(), _data.length() );
	_prefix = _pos = _data.length();
}

void Replxx::ReplxxImpl::setColor( Replxx::Color color_ ) {
	char const* code( ansi_color( color_ ) );
	while ( *code ) {
		_display.push_back( *code );
		++ code;
	}
}

void Replxx::ReplxxImpl::highlight( int highlightIdx, bool error_ ) {
	Replxx::colors_t colors( _data.length(), Replxx::Color::DEFAULT );
	_utf8Buffer.assign( _data );
	if ( !! _highlighterCallback ) {
		_highlighterCallback( _utf8Buffer.get(), colors );
	}
	if ( highlightIdx != -1 ) {
		colors[highlightIdx] = error_ ? Replxx::Color::ERROR : Replxx::Color::BRIGHTRED;
	}
	_display.clear();
	Replxx::Color c( Replxx::Color::DEFAULT );
	for ( int i( 0 ); i < _data.length(); ++ i ) {
		if ( colors[i] != c ) {
			c = colors[i];
			setColor( c );
		}
		_display.push_back( _data[i] );
	}
	setColor( Replxx::Color::DEFAULT );
}

int Replxx::ReplxxImpl::handle_hints( HINT_ACTION hintAction_ ) {
	if ( _noColor ) {
		return ( 0 );
	}
	if ( ! _hintCallback ) {
		return ( 0 );
	}
	if ( hintAction_ == HINT_ACTION::SKIP ) {
		return ( 0 );
	}
	if ( _pos != _data.length() ) {
		return ( 0 );
	}
	_hint = UnicodeString();
	int len( 0 );
	if ( hintAction_ == HINT_ACTION::REGENERATE ) {
		_hintSelection = -1;
	}
	Replxx::Color c( Replxx::Color::GRAY );
	_utf8Buffer.assign( _data, _pos );
	int contextLen( context_length() );
	Replxx::ReplxxImpl::hints_t hints( call_hinter( _utf8Buffer.get(), contextLen, c ) );
	int hintCount( hints.size() );
	if ( hintCount == 1 ) {
		setColor( c );
		_hint = hints.front();
		len = _hint.length();
		for ( int i( contextLen ); i < len; ++ i ) {
			_display.push_back( _hint[i] );
		}
		setColor( Replxx::Color::DEFAULT );
	} else if ( _maxHintRows > 0 ) {
		int startCol( _prompt._indentation + _pos - contextLen );
		int maxCol( _prompt.screen_columns() );
#ifdef _WIN32
		-- maxCol;
#endif
		if ( _hintSelection < -1 ) {
			_hintSelection = hintCount - 1;
		} else if ( _hintSelection >= hintCount ) {
			_hintSelection = -1;
		}
		setColor( c );
		if ( _hintSelection != -1 ) {
			_hint = hints[_hintSelection];
			len = min<int>( _hint.length(), maxCol - startCol - _data.length() );
			for ( int i( contextLen ); i < len; ++ i ) {
				_display.push_back( _hint[i] );
			}
		}
		setColor( Replxx::Color::DEFAULT );
		for ( int hintRow( 0 ); hintRow < min( hintCount, _maxHintRows ); ++ hintRow ) {
#ifdef _WIN32
			_display.push_back( '\r' );
#endif
			_display.push_back( '\n' );
			int col( 0 );
			for ( int i( 0 ); ( i < startCol ) && ( col < maxCol ); ++ i, ++ col ) {
				_display.push_back( ' ' );
			}
			setColor( c );
			for ( int i( _pos - contextLen ); ( i < _pos ) && ( col < maxCol ); ++ i, ++ col ) {
				_display.push_back( _data[i] );
			}
			int hintNo( hintRow + _hintSelection + 1 );
			if ( hintNo == hintCount ) {
				continue;
			} else if ( hintNo > hintCount ) {
				-- hintNo;
			}
			UnicodeString const& h( hints[hintNo % hintCount] );
			for ( int i( contextLen ); ( i < h.length() ) && ( col < maxCol ); ++ i, ++ col ) {
				_display.push_back( h[i] );
			}
			setColor( Replxx::Color::DEFAULT );
		}
	}
	return ( len - contextLen );
}

/**
 * Refresh the user's input line: the prompt is already onscreen and is not
 * redrawn here
 * @param pi	 Prompt struct holding information about the prompt and our
 * screen position
 */
void Replxx::ReplxxImpl::refreshLine( HINT_ACTION hintAction_ ) {
	// check for a matching brace/bracket/paren, remember its position if found
	int highlightIdx = -1;
	bool indicateError = false;
	if (_pos < _data.length()) {
		/* this scans for a brace matching _data[_pos] to highlight */
		unsigned char part1, part2;
		int scanDirection = 0;
		if (strchr("}])", _data[_pos])) {
			scanDirection = -1; /* backwards */
			if (_data[_pos] == '}') {
				part1 = '}'; part2 = '{';
			} else if (_data[_pos] == ']') {
				part1 = ']'; part2 = '[';
			} else {
				part1 = ')'; part2 = '(';
			}
		} else if (strchr("{[(", _data[_pos])) {
			scanDirection = 1; /* forwards */
			if (_data[_pos] == '{') {
				//part1 = '{'; part2 = '}';
				part1 = '}'; part2 = '{';
			} else if (_data[_pos] == '[') {
				//part1 = '['; part2 = ']';
				part1 = ']'; part2 = '[';
			} else {
				//part1 = '('; part2 = ')';
				part1 = ')'; part2 = '(';
			}
		}

		if (scanDirection) {
			int unmatched = scanDirection;
			int unmatchedOther = 0;
			for (int i = _pos + scanDirection; i >= 0 && i < _data.length(); i += scanDirection) {
				/* TODO: the right thing when inside a string */
				if (strchr("}])", _data[i])) {
					if (_data[i] == part1) {
						--unmatched;
					} else {
						--unmatchedOther;
					}
				} else if (strchr("{[(", _data[i])) {
					if (_data[i] == part2) {
						++unmatched;
					} else {
						++unmatchedOther;
					}
				}

				if (unmatched == 0) {
					highlightIdx = i;
					indicateError = (unmatchedOther != 0);
					break;
				}
			}
		}
	}

	highlight( highlightIdx, indicateError );
	int hintLen( handle_hints( hintAction_ ) );
	// calculate the position of the end of the input line
	int xEndOfInput( 0 ), yEndOfInput( 0 );
	calculateScreenPosition(
		_prompt._indentation, 0, _prompt.screen_columns(),
		calculateColumnPosition( _data.get(), _data.length() ) + hintLen,
		xEndOfInput, yEndOfInput
	);
	yEndOfInput += count( _display.begin(), _display.end(), '\n' );

	// calculate the desired position of the cursor
	int xCursorPos( 0 ), yCursorPos( 0 );
	calculateScreenPosition(
		_prompt._indentation, 0, _prompt.screen_columns(),
		calculateColumnPosition(_data.get(), _pos),
		xCursorPos,
		yCursorPos
	);

#ifdef _WIN32
	// position at the end of the prompt, clear to end of previous input
	CONSOLE_SCREEN_BUFFER_INFO inf;
	GetConsoleScreenBufferInfo(console_out, &inf);
	inf.dwCursorPosition.X = _prompt._indentation; // 0-based on Win32
	inf.dwCursorPosition.Y -= _prompt._cursorRowOffset - _prompt._extraLines;
	SetConsoleCursorPosition(console_out, inf.dwCursorPosition);
	replxx::clear_screen( CLEAR_SCREEN::TO_END );
	_prompt._previousInputLen = _data.length();

	// display the input line
	if ( !_noColor ) {
		write32( _display.data(), _display.size() );
	} else {
		write32( _data.get(), _data.length() );
	}

	// position the cursor
	GetConsoleScreenBufferInfo(console_out, &inf);
	inf.dwCursorPosition.X = xCursorPos; // 0-based on Win32
	inf.dwCursorPosition.Y -= ( yEndOfInput - yCursorPos );
	SetConsoleCursorPosition(console_out, inf.dwCursorPosition);
#else // _WIN32
	char seq[64];
	int cursorRowMovement = _prompt._cursorRowOffset - _prompt._extraLines;
	if (cursorRowMovement > 0) { // move the cursor up as required
		snprintf(seq, sizeof seq, "\x1b[%dA", cursorRowMovement);
		write8( seq, strlen(seq) );
	}
	// position at the end of the prompt, clear to end of screen
	snprintf(
		seq, sizeof seq, "\x1b[%dG\x1b[%c",
		_prompt._indentation + 1, /* 1-based on VT100 */
		'J'
	);
	write8( seq, strlen(seq) );

	if ( !_noColor ) {
		write32( _display.data(), _display.size() );
	} else { // highlightIdx the matching brace/bracket/parenthesis
		write32( _data.get(), _data.length() );
	}

	// we have to generate our own newline on line wrap
	if (xEndOfInput == 0 && yEndOfInput > 0) {
		write8( "\n", 1 );
	}

	// position the cursor
	cursorRowMovement = yEndOfInput - yCursorPos;
	if (cursorRowMovement > 0) { // move the cursor up as required
		snprintf(seq, sizeof seq, "\x1b[%dA", cursorRowMovement);
		write8( seq, strlen(seq) );
	}
	// position the cursor within the line
	snprintf(seq, sizeof seq, "\x1b[%dG", xCursorPos + 1); // 1-based on VT100
	write8( seq, strlen(seq) );
#endif

	_prompt._cursorRowOffset = _prompt._extraLines + yCursorPos; // remember row for next pass
}

int Replxx::ReplxxImpl::context_length() {
	int prefixLength = _pos;
	while ( prefixLength > 0 ) {
		if ( is_word_break_character( _data[prefixLength - 1] ) ) {
			break;
		}
		-- prefixLength;
	}
	return ( _pos - prefixLength );
}

namespace {
int longest_common_prefix( Replxx::ReplxxImpl::completions_t const& completions ) {
	int completionsCount( completions.size() );
	if ( completionsCount < 1 ) {
		return ( 0 );
	}
	int longestCommonPrefix( 0 );
	UnicodeString const& sample( completions.front() );
	while ( true ) {
		if ( longestCommonPrefix >= sample.length() ) {
			return ( longestCommonPrefix );
		}
		char32_t sc( sample[longestCommonPrefix] );
		for ( int i( 1 ); i < completionsCount; ++ i ) {
			UnicodeString const& candidate( completions[i] );
			if ( longestCommonPrefix >= candidate.length() ) {
				return ( longestCommonPrefix );
			}
			char32_t cc( candidate[longestCommonPrefix] );
			if ( cc != sc ) {
				return ( longestCommonPrefix );
			}
		}
		++ longestCommonPrefix;
	}
}
}

/**
 * Handle command completion, using a completionCallback() routine to provide
 * possible substitutions
 * This routine handles the mechanics of updating the user's input buffer with
 * possible replacement of text as the user selects a proposed completion string,
 * or cancels the completion attempt.
 * @param pi - Prompt struct holding information about the prompt and our
 * screen position
 */
int Replxx::ReplxxImpl::completeLine( void ) {
	char32_t c = 0;

	// completionCallback() expects a parsable entity, so find the previous break
	// character and
	// extract a copy to parse.	we also handle the case where tab is hit while
	// not at end-of-line.

	_utf8Buffer.assign( _data, _pos );
	// get a list of completions
	int contextLen( context_length() );
	Replxx::ReplxxImpl::completions_t completions( call_completer( _utf8Buffer.get(), contextLen ) );

	// if no completions, we are done
	if (completions.size() == 0) {
		beep();
		return 0;
	}

	// at least one completion
	int longestCommonPrefix = 0;
	int completionsCount( completions.size() );
	int selectedCompletion( 0 );
	if ( _hintSelection != -1 ) {
		selectedCompletion = _hintSelection;
		completionsCount = 1;
	}
	if ( completionsCount == 1 ) {
		longestCommonPrefix = static_cast<int>(completions[selectedCompletion].length());
	} else {
		longestCommonPrefix = longest_common_prefix( completions );
	}
	if ( _beepOnAmbiguousCompletion && ( completionsCount != 1 ) ) { // beep if ambiguous
		beep();
	}

	// if we can extend the item, extend it and return to main loop
	if ( ( longestCommonPrefix > contextLen ) || ( completionsCount == 1 ) ) {
		_data.insert( _pos, completions[selectedCompletion], contextLen, longestCommonPrefix - contextLen );
		_prefix = _pos = _pos + longestCommonPrefix - contextLen;
		refreshLine();
		return 0;
	}

	if ( _doubleTabCompletion ) {
		// we can't complete any further, wait for second tab
		do {
			c = read_char();
			c = cleanupCtrl(c);
		} while (c == static_cast<char32_t>(-1));

		// if any character other than tab, pass it to the main loop
		if (c != ctrlChar('I')) {
			return c;
		}
	}

	// we got a second tab, maybe show list of possible completions
	bool showCompletions = true;
	bool onNewLine = false;
	if ( static_cast<int>( completions.size() ) > _completionCountCutoff ) {
		int savePos = _pos; // move cursor to EOL to avoid overwriting the command line
		_pos = _data.length();
		refreshLine();
		_pos = savePos;
		printf("\nDisplay all %u possibilities? (y or n)",
					 static_cast<unsigned int>(completions.size()));
		fflush(stdout);
		onNewLine = true;
		while (c != 'y' && c != 'Y' && c != 'n' && c != 'N' && c != ctrlChar('C')) {
			do {
				c = read_char();
				c = cleanupCtrl(c);
			} while (c == static_cast<char32_t>(-1));
		}
		switch (c) {
			case 'n':
			case 'N':
				showCompletions = false;
				break;
			case ctrlChar('C'):
				showCompletions = false;
				// Display the ^C we got
				write8( "^C", 2 );
				c = 0;
				break;
		}
	}

	// if showing the list, do it the way readline does it
	bool stopList( false );
	if ( showCompletions ) {
		int longestCompletion( 0 );
		for ( size_t j( 0 ); j < completions.size(); ++ j ) {
			int itemLength( static_cast<int>( completions[j].length() ) );
			if ( itemLength > longestCompletion ) {
				longestCompletion = itemLength;
			}
		}
		longestCompletion += 2;
		int columnCount = _prompt.screen_columns() / longestCompletion;
		if ( columnCount < 1 ) {
			columnCount = 1;
		}
		if ( ! onNewLine ) {  // skip this if we showed "Display all %d possibilities?"
			int savePos = _pos; // move cursor to EOL to avoid overwriting the command line
			_pos = _data.length();
			refreshLine( HINT_ACTION::SKIP );
			_pos = savePos;
		} else {
			replxx::clear_screen( CLEAR_SCREEN::TO_END );
		}
		size_t pauseRow = getScreenRows() - 1;
		size_t rowCount = (completions.size() + columnCount - 1) / columnCount;
		for (size_t row = 0; row < rowCount; ++row) {
			if (row == pauseRow) {
				printf("\n--More--");
				fflush(stdout);
				c = 0;
				bool doBeep = false;
				while (c != ' ' && c != '\r' && c != '\n' && c != 'y' && c != 'Y' &&
							 c != 'n' && c != 'N' && c != 'q' && c != 'Q' &&
							 c != ctrlChar('C')) {
					if (doBeep) {
						beep();
					}
					doBeep = true;
					do {
						c = read_char();
						c = cleanupCtrl(c);
					} while (c == static_cast<char32_t>(-1));
				}
				switch (c) {
					case ' ':
					case 'y':
					case 'Y':
						printf("\r				\r");
						pauseRow += getScreenRows() - 1;
						break;
					case '\r':
					case '\n':
						printf("\r				\r");
						++pauseRow;
						break;
					case 'n':
					case 'N':
					case 'q':
					case 'Q':
						printf("\r				\r");
						stopList = true;
						break;
					case ctrlChar('C'):
						// Display the ^C we got
						write8( "^C", 2 );
						stopList = true;
						break;
				}
			} else {
				printf("\n");
			}
			if (stopList) {
				break;
			}
			for (int column = 0; column < columnCount; ++column) {
				size_t index = (column * rowCount) + row;
				if (index < completions.size()) {
					int itemLength = static_cast<int>(completions[index].length());
					fflush(stdout);

					static UnicodeString const col( ansi_color( Replxx::Color::BRIGHTMAGENTA ) );
					if ( !_noColor ) {
						write32( col.get(), col.length() );
					}
					write32( &_data[_pos - contextLen], longestCommonPrefix );
					static UnicodeString const res( ansi_color( Replxx::Color::DEFAULT ) );
					if ( !_noColor ) {
						write32( res.get(), res.length() );
					}

					write32( completions[index].get() + longestCommonPrefix, itemLength - longestCommonPrefix );

					if (((column + 1) * rowCount) + row < completions.size()) {
						for ( int k( itemLength ); k < longestCompletion; ++k ) {
							printf( " " );
						}
					}
				}
			}
		}
		fflush(stdout);
	}

	// display the prompt on a new line, then redisplay the input buffer
	if (!stopList || c == ctrlChar('C')) {
		write8( "\n", 1 );
	}
	_prompt.write();
#ifndef _WIN32
	// we have to generate our own newline on line wrap on Linux
	if (_prompt._indentation == 0 && _prompt._extraLines > 0) {
		write8( "\n", 1 );
	}
#endif
	_prompt._cursorRowOffset = _prompt._extraLines;
	refreshLine();
	return 0;
}

int Replxx::ReplxxImpl::getInputLine( void ) {
	// The latest history entry is always our current buffer
	if ( _data.length() > 0 ) {
		_utf8Buffer.assign( _data );
		history_add( _utf8Buffer.get() );
	} else {
		history_add( "" );
	}
	_history.reset_pos();

	// display the prompt
	_prompt.write();

#ifndef _WIN32
	// we have to generate our own newline on line wrap on Linux
	if ( ( _prompt._indentation == 0 ) && ( _prompt._extraLines > 0 ) ) {
		write8( "\n", 1 );
	}
#endif

	// the cursor starts out at the end of the prompt
	_prompt._cursorRowOffset = _prompt._extraLines;

	// kill and yank start in "other" mode
	_killRing.lastAction = KillRing::actionOther;

	// when history search returns control to us, we execute its terminating
	// keystroke
	int terminatingKeystroke = -1;

	// if there is already text in the buffer, display it first
	if (_data.length() > 0) {
		refreshLine();
	}

	// loop collecting characters, respond to line editing characters
	NEXT next( NEXT::CONTINUE );
	while ( next == NEXT::CONTINUE ) {
		int c;
		if (terminatingKeystroke == -1) {
			c = read_char(); // get a new keystroke

#ifndef _WIN32
			if (c == 0 && gotResize) {
				// caught a window resize event
				// now redraw the prompt and line
				gotResize = false;
				_prompt.update_screen_columns();
				// redraw the original prompt with current input
				dynamicRefresh( _prompt, _data.get(), _data.length(), _pos );
				continue;
			}
#endif
		} else {
			c = terminatingKeystroke;	 // use the terminating keystroke from search
			terminatingKeystroke = -1; // clear it once we've used it
		}

		c = cleanupCtrl(c); // convert CTRL + <char> into normal ctrl

		if (c == 0) {
			return _data.length();
		}

		if (c == -1) {
			refreshLine();
			continue;
		}

		if (c == -2) {
			_prompt.write();
			refreshLine();
			continue;
		}

		bool updatePrefix( true );
		switch (c) {

			case ctrlChar('C'): // ctrl-C, abort this line
				_killRing.lastAction = KillRing::actionOther;
				_history.reset_recall_most_recent();
				errno = EAGAIN;
				_history.drop_last();
				// we need one last refresh with the cursor at the end of the line
				// so we don't display the next prompt over the previous input line
				_pos = _data.length(); // pass _data.length() as _pos for EOL
				refreshLine( HINT_ACTION::SKIP );
				write8( "^C\r\n", 4 );
				next = NEXT::BAIL;
				break;

			case META + 'c': // meta-C, give word initial Cap
			case META + 'C':
				_killRing.lastAction = KillRing::actionOther;
				_history.reset_recall_most_recent();
				if (_pos < _data.length()) {
					while ( _pos < _data.length() && is_word_break_character( _data[_pos] ) ) {
						++_pos;
					}
					if (_pos < _data.length() && !is_word_break_character( _data[_pos] ) ) {
						if ( _data[_pos] >= 'a' && _data[_pos] <= 'z' ) {
							_data[_pos] += 'A' - 'a';
						}
						++_pos;
					}
					while (_pos < _data.length() && !is_word_break_character( _data[_pos] ) ) {
						if ( _data[_pos] >= 'A' && _data[_pos] <= 'Z' ) {
							_data[_pos] += 'a' - 'A';
						}
						++_pos;
					}
					refreshLine();
				}
				break;

			// ctrl-D, delete the character under the cursor
			// on an empty line, exit the shell
			case ctrlChar('D'):
				_killRing.lastAction = KillRing::actionOther;
				if ( ( _data.length() > 0 ) && ( _pos < _data.length() ) ) {
					_history.reset_recall_most_recent();
					_data.erase( _pos );
					refreshLine();
				} else if (_data.length() == 0) {
					_history.drop_last();
					next = NEXT::BAIL;
				}
				break;

			case META + 'd': // meta-D, kill word to right of cursor
			case META + 'D':
				if ( _pos < _data.length() ) {
					_history.reset_recall_most_recent();
					int endingPos = _pos;
					while ( endingPos < _data.length() && is_word_break_character( _data[endingPos] ) ) {
						++ endingPos;
					}
					while ( endingPos < _data.length() && !is_word_break_character( _data[endingPos] ) ) {
						++ endingPos;
					}
					_killRing.kill( _data.get() + _pos, endingPos - _pos, true );
					_data.erase( _pos, endingPos - _pos );
					refreshLine();
				}
				_killRing.lastAction = KillRing::actionKill;
				break;

			case ctrlChar('H'): // backspace/ctrl-H, delete char to left of cursor
				_killRing.lastAction = KillRing::actionOther;
				if ( _pos > 0 ) {
					_history.reset_recall_most_recent();
					-- _pos;
					_data.erase( _pos );
					refreshLine();
				}
				break;

			// meta-Backspace, kill word to left of cursor
			case META + ctrlChar('H'):
				if ( _pos > 0 ) {
					_history.reset_recall_most_recent();
					int startingPos = _pos;
					while ( _pos > 0 && is_word_break_character( _data[_pos - 1] ) ) {
						-- _pos;
					}
					while ( _pos > 0 && !is_word_break_character( _data[_pos - 1] ) ) {
						-- _pos;
					}
					_killRing.kill( _data.get() + _pos, startingPos - _pos, false);
					_data.erase( _pos, startingPos - _pos );
					refreshLine();
				}
				_killRing.lastAction = KillRing::actionKill;
				break;
			case ( ctrlChar('I') ): {
				if ( !! _completionCallback && ( _completeOnEmpty || ( _pos > 0 ) ) ) {
					_killRing.lastAction = KillRing::actionOther;
					_history.reset_recall_most_recent();

					// completeLine does the actual completion and replacement
					c = completeLine();

					if ( c < 0 ) {
						next = NEXT::BAIL;
					} else if ( c != 0 ) {
						terminatingKeystroke = c;
					}
				} else {
					insert_character( c );
				}
			} break;

			case ctrlChar('J'): // ctrl-J/linefeed/newline, accept line
			case ctrlChar('M'): // ctrl-M/return/enter
				_killRing.lastAction = KillRing::actionOther;
				// we need one last refresh with the cursor at the end of the line
				// so we don't display the next prompt over the previous input line
				_pos = _data.length(); // pass _data.length() as _pos for EOL
				refreshLine( HINT_ACTION::SKIP );
				_history.commit_index();
				_history.drop_last();
				next = NEXT::RETURN;
				break;

			case ctrlChar('K'): // ctrl-K, kill from cursor to end of line
				_killRing.kill( _data.get() + _pos, _data.length() - _pos, true );
				_data.erase( _pos, _data.length() - _pos );
				refreshLine();
				_killRing.lastAction = KillRing::actionKill;
				_history.reset_recall_most_recent();
				break;

			case ctrlChar('L'): // ctrl-L, clear screen and redisplay line
				clearScreen();
				break;

			case META + 'l': // meta-L, lowercase word
			case META + 'L':
				_killRing.lastAction = KillRing::actionOther;
				if (_pos < _data.length()) {
					_history.reset_recall_most_recent();
					while ( _pos < _data.length() && is_word_break_character( _data[_pos] ) ) {
						++ _pos;
					}
					while (_pos < _data.length() && !is_word_break_character( _data[_pos] ) ) {
						if ( _data[_pos] >= 'A' && _data[_pos] <= 'Z' ) {
							_data[_pos] += 'a' - 'A';
						}
						++ _pos;
					}
					refreshLine();
				}
				break;

			case ctrlChar('N'): // ctrl-N, recall next line in history
			case ctrlChar('P'): // ctrl-P, recall previous line in history
			case DOWN_ARROW_KEY:
			case UP_ARROW_KEY:
				_killRing.lastAction = KillRing::actionOther;
				// if not already recalling, add the current line to the history list so
				// we don't
				// have to special case it
				if ( _history.is_last() ) {
					_utf8Buffer.assign( _data );
					_history.update_last( _utf8Buffer.get() );
				}
				if ( ! _history.is_empty() ) {
					if (c == UP_ARROW_KEY) {
						c = ctrlChar('P');
					}
					if ( ! _history.move( c == ctrlChar('P') ) ) {
						break;
					}
					_data.assign( _history.current() );
					_pos = _data.length();
					refreshLine();
				}
				break;
			case CTRL + UP_ARROW_KEY:
				if ( ! _noColor ) {
					_killRing.lastAction = KillRing::actionOther;
					-- _hintSelection;
					refreshLine( HINT_ACTION::REPAINT );
				}
				break;
			case CTRL + DOWN_ARROW_KEY:
				if ( ! _noColor ) {
					_killRing.lastAction = KillRing::actionOther;
					++ _hintSelection;
					refreshLine( HINT_ACTION::REPAINT );
				}
				break;

			case META + 'p': // Alt-P, reverse history search for prefix
			case META + 'P': // Alt-P, reverse history search for prefix
			case META + 'n': // Alt-N, forward history search for prefix
			case META + 'N': // Alt-N, forward history search for prefix
				commonPrefixSearch( c );
				updatePrefix = false;
				break;
			case ctrlChar('R'): // ctrl-R, reverse history search
			case ctrlChar('S'): // ctrl-S, forward history search
				terminatingKeystroke = incrementalHistorySearch( c );
				break;

			case ctrlChar('T'): // ctrl-T, transpose characters
				_killRing.lastAction = KillRing::actionOther;
				if ( _pos > 0 && _data.length() > 1 ) {
					_history.reset_recall_most_recent();
					size_t leftCharPos = ( _pos == _data.length() ) ? _pos - 2 : _pos - 1;
					char32_t aux = _data[leftCharPos];
					_data[leftCharPos] = _data[leftCharPos + 1];
					_data[leftCharPos + 1] = aux;
					if ( _pos != _data.length() ) {
						++_pos;
					}
					refreshLine();
				}
				break;

			case ctrlChar('U'): // ctrl-U, kill all characters to the left of the cursor
				if (_pos > 0) {
					_history.reset_recall_most_recent();
					_killRing.kill( _data.get(), _pos, false );
					_data.erase( 0, _pos );
					_pos = 0;
					refreshLine();
				}
				_killRing.lastAction = KillRing::actionKill;
				break;

			case META + 'u': // meta-U, uppercase word
			case META + 'U':
				_killRing.lastAction = KillRing::actionOther;
				if (_pos < _data.length()) {
					_history.reset_recall_most_recent();
					while ( _pos < _data.length() && is_word_break_character( _data[_pos] ) ) {
						++ _pos;
					}
					while ( _pos < _data.length() && !is_word_break_character( _data[_pos] ) ) {
						if ( _data[_pos] >= 'a' && _data[_pos] <= 'z') {
							_data[_pos] += 'A' - 'a';
						}
						++ _pos;
					}
					refreshLine();
				}
				break;

			// ctrl-W, kill to whitespace (not word) to left of cursor
			case ctrlChar('W'):
				if ( _pos > 0 ) {
					_history.reset_recall_most_recent();
					int startingPos = _pos;
					while ( _pos > 0 && _data[_pos - 1] == ' ' ) {
						--_pos;
					}
					while ( _pos > 0 && _data[_pos - 1] != ' ' ) {
						-- _pos;
					}
					_killRing.kill( _data.get() + _pos, startingPos - _pos, false );
					_data.erase( _pos, startingPos - _pos );
					refreshLine();
				}
				_killRing.lastAction = KillRing::actionKill;
				break;

			case ctrlChar('Y'): // ctrl-Y, yank killed text
				_history.reset_recall_most_recent();
				{
					UnicodeString* restoredText = _killRing.yank();
					if (restoredText) {
						_data.insert( _pos, *restoredText, 0, restoredText->length() );
						_pos += restoredText->length();
						refreshLine();
						_killRing.lastAction = KillRing::actionYank;
						_killRing.lastYankSize = restoredText->length();
					} else {
						beep();
					}
				}
				break;

			case META + 'y': // meta-Y, "yank-pop", rotate popped text
			case META + 'Y':
				if (_killRing.lastAction == KillRing::actionYank) {
					_history.reset_recall_most_recent();
					UnicodeString* restoredText = _killRing.yankPop();
					if (restoredText) {
						_pos -= _killRing.lastYankSize;
						_data.erase( _pos, _killRing.lastYankSize );
						_data.insert( _pos, *restoredText, 0, restoredText->length() );
						_pos += restoredText->length();
						_killRing.lastYankSize = restoredText->length();
						refreshLine();
						break;
					}
				}
				beep();
				break;

#ifndef _WIN32
			case ctrlChar('Z'): // ctrl-Z, job control
				disableRawMode(); // Returning to Linux (whatever) shell, leave raw mode
				raise(SIGSTOP);   // Break out in mid-line
				enableRawMode();  // Back from Linux shell, re-enter raw mode
				// Redraw prompt
				_prompt.write();
				refreshLine();  // Refresh the line
				break;
#endif

			// DEL, delete the character under the cursor
			case 127:
			case DELETE_KEY:
				_killRing.lastAction = KillRing::actionOther;
				if (_data.length() > 0 && _pos < _data.length()) {
					_history.reset_recall_most_recent();
					_data.erase( _pos );
					refreshLine();
				}
				break;

			case META + '<':    // meta-<, beginning of history
			case PAGE_UP_KEY:   // Page Up, beginning of history
			case META + '>':    // meta->, end of history
			case PAGE_DOWN_KEY: // Page Down, end of history
				_killRing.lastAction = KillRing::actionOther;
				// if not already recalling, add the current line to the history list so
				// we don't
				// have to special case it
				if ( _history.is_last() ) {
					_utf8Buffer.assign( _data );
					_history.update_last( _utf8Buffer.get() );
				}
				if ( ! _history.is_empty() ) {
					_history.jump( (c == META + '<' || c == PAGE_UP_KEY) );
					_data.assign( _history.current().c_str() );
					_pos = _data.length();
					refreshLine();
				}
				break;

			// not one of our special characters, maybe insert it in the buffer
			default: {
				key_press_handlers_t::iterator it( _keyPressHandlers.find( c ) );
				if ( it != _keyPressHandlers.end() ) {
					next = it->second( c );
				} else {
					next = insert_character( c );
				}
				break;
			}
		}
		if ( updatePrefix ) {
			_prefix = _pos;
		}
	}
	return ( next == NEXT::RETURN ? _data.length() : -1 );
}

Replxx::ReplxxImpl::NEXT Replxx::ReplxxImpl::insert_character( int c ) {
	_killRing.lastAction = KillRing::actionOther;
	_history.reset_recall_most_recent();
	/*
	 * beep on unknown Ctrl and/or Meta keys
	 * don't insert control characters
	 */
	if ( ( c & ( META | CTRL ) ) || isControlChar( c ) ) {
		beep();
		return ( NEXT::CONTINUE );
	}
	_data.insert( _pos, c );
	++ _pos;
	int inputLen = calculateColumnPosition( _data.get(), _data.length() );
	if ( _noColor
		|| ( ! ( !! _highlighterCallback || !! _hintCallback )
			&& ( _prompt._indentation + inputLen < _prompt.screen_columns() )
		)
	) {
		/* Avoid a full assign of the line in the
		 * trivial case. */
		if (inputLen > _prompt._previousInputLen) {
			_prompt._previousInputLen = inputLen;
		}
		write32(reinterpret_cast<char32_t*>(&c), 1);
	} else {
		refreshLine();
	}
	return ( NEXT::CONTINUE );
}

// ctrl-A, HOME: move cursor to start of line
Replxx::ReplxxImpl::NEXT Replxx::ReplxxImpl::go_to_begining_of_line( int ) {
	_killRing.lastAction = KillRing::actionOther;
	_pos = 0;
	refreshLine();
	return ( NEXT::CONTINUE );
}

Replxx::ReplxxImpl::NEXT Replxx::ReplxxImpl::go_to_end_of_line( int ) {
	_killRing.lastAction = KillRing::actionOther;
	_pos = _data.length();
	refreshLine();
	return ( NEXT::CONTINUE );
}

// ctrl-B, move cursor left by one character
Replxx::ReplxxImpl::NEXT Replxx::ReplxxImpl::move_one_char_left( int ) {
	_killRing.lastAction = KillRing::actionOther;
	if (_pos > 0) {
		--_pos;
		refreshLine();
	}
	return ( NEXT::CONTINUE );
}

// ctrl-F, move cursor right by one character
Replxx::ReplxxImpl::NEXT Replxx::ReplxxImpl::move_one_char_right( int ) {
	_killRing.lastAction = KillRing::actionOther;
	if ( _pos < _data.length() ) {
		++_pos;
		refreshLine();
	}
	return ( NEXT::CONTINUE );
}

// meta-B, move cursor left by one word
Replxx::ReplxxImpl::NEXT Replxx::ReplxxImpl::move_one_word_left( int ) {
	_killRing.lastAction = KillRing::actionOther;
	if (_pos > 0) {
		while (_pos > 0 && is_word_break_character( _data[_pos - 1] ) ) {
			--_pos;
		}
		while (_pos > 0 && !is_word_break_character( _data[_pos - 1] ) ) {
			--_pos;
		}
		refreshLine();
	}
	return ( NEXT::CONTINUE );
}

// meta-F, move cursor right by one word
Replxx::ReplxxImpl::NEXT Replxx::ReplxxImpl::move_one_word_right( int ) {
	_killRing.lastAction = KillRing::actionOther;
	if ( _pos < _data.length() ) {
		while ( _pos < _data.length() && is_word_break_character( _data[_pos] ) ) {
			++_pos;
		}
		while ( _pos < _data.length() && !is_word_break_character( _data[_pos] ) ) {
			++_pos;
		}
		refreshLine();
	}
	return ( NEXT::CONTINUE );
}

void Replxx::ReplxxImpl::commonPrefixSearch( int startChar ) {
	_killRing.lastAction = KillRing::actionOther;
	_utf8Buffer.assign( _data );
	int prefixSize( calculateColumnPosition( _data.get(), _prefix ) );
	if (
		_history.common_prefix_search(
			_utf8Buffer.get(), prefixSize, ( startChar == ( META + 'p' ) ) || ( startChar == ( META + 'P' ) )
		)
	) {
		_data.assign( _history.current() );
		_pos = _data.length();
		refreshLine();
	}
}

/**
 * Incremental history search -- take over the prompt and keyboard as the user
 * types a search string, deletes characters from it, changes _direction,
 * and either accepts the found line (for execution orediting) or cancels.
 * @param startChar - the character that began the search, used to set the initial
 * _direction
 */
int Replxx::ReplxxImpl::incrementalHistorySearch( int startChar ) {

	// if not already recalling, add the current line to the history list so we
	// don't have to special case it
	if ( _history.is_last() ) {
		_utf8Buffer.assign( _data );
		_history.update_last( _utf8Buffer.get() );
	}
	int historyLinePosition( _pos );
	UnicodeString empty;
	_data.swap( empty );
	refreshLine(); // erase the old input first
	_data.swap( empty );

	DynamicPrompt dp( (startChar == ctrlChar('R')) ? -1 : 1 );

	dp._previousLen = _prompt._previousLen;
	dp._previousInputLen = _prompt._previousInputLen;
	// draw user's text with our prompt
	dynamicRefresh(dp, _data.get(), _data.length(), historyLinePosition);

	// loop until we get an exit character
	int c = 0;
	bool keepLooping = true;
	bool useSearchedLine = true;
	bool searchAgain = false;
	UnicodeString activeHistoryLine;
	while ( keepLooping ) {
		c = read_char();
		c = cleanupCtrl(c); // convert CTRL + <char> into normal ctrl

		switch (c) {
			// these characters keep the selected text but do not execute it
			case ctrlChar('A'): // ctrl-A, move cursor to start of line
			case HOME_KEY:
			case ctrlChar('B'): // ctrl-B, move cursor left by one character
			case LEFT_ARROW_KEY:
			case META + 'b': // meta-B, move cursor left by one word
			case META + 'B':
			case CTRL + LEFT_ARROW_KEY:
			case META + LEFT_ARROW_KEY: // Emacs allows Meta, bash & readline don't
			case ctrlChar('D'):
			case META + 'd': // meta-D, kill word to right of cursor
			case META + 'D':
			case ctrlChar('E'): // ctrl-E, move cursor to end of line
			case END_KEY:
			case ctrlChar('F'): // ctrl-F, move cursor right by one character
			case RIGHT_ARROW_KEY:
			case META + 'f': // meta-F, move cursor right by one word
			case META + 'F':
			case CTRL + RIGHT_ARROW_KEY:
			case META + RIGHT_ARROW_KEY: // Emacs allows Meta, bash & readline don't
			case META + ctrlChar('H'):
			case ctrlChar('J'):
			case ctrlChar('K'): // ctrl-K, kill from cursor to end of line
			case ctrlChar('M'):
			case ctrlChar('N'): // ctrl-N, recall next line in history
			case ctrlChar('P'): // ctrl-P, recall previous line in history
			case DOWN_ARROW_KEY:
			case UP_ARROW_KEY:
			case ctrlChar('T'): // ctrl-T, transpose characters
			case ctrlChar('U'): // ctrl-U, kill all characters to the left of the cursor
			case ctrlChar('W'):
			case META + 'y': // meta-Y, "yank-pop", rotate popped text
			case META + 'Y':
			case 127:
			case DELETE_KEY:
			case META + '<': // start of history
			case PAGE_UP_KEY:
			case META + '>': // end of history
			case PAGE_DOWN_KEY:
				keepLooping = false;
				break;

			// these characters revert the input line to its previous state
			case ctrlChar('C'): // ctrl-C, abort this line
			case ctrlChar('G'):
			case ctrlChar('L'): // ctrl-L, clear screen and redisplay line
				keepLooping = false;
				useSearchedLine = false;
				if (c != ctrlChar('L')) {
					c = -1; // ctrl-C and ctrl-G just abort the search and do nothing else
				}
				break;

			// these characters stay in search mode and assign the display
			case ctrlChar('S'):
			case ctrlChar('R'):
				if ( dp._searchText.length() == 0 ) { // if no current search text, recall previous text
					if ( previousSearchText.length() > 0 ) {
						dp._searchText = previousSearchText;
					}
				}
				if ((dp._direction == 1 && c == ctrlChar('R')) ||
						(dp._direction == -1 && c == ctrlChar('S'))) {
					dp._direction = 0 - dp._direction; // reverse _direction
					dp.updateSearchPrompt();         // change the prompt
				} else {
					searchAgain = true; // same _direction, search again
				}
				break;

// job control is its own thing
#ifndef _WIN32
			case ctrlChar('Z'): { // ctrl-Z, job control
				disableRawMode(); // Returning to Linux (whatever) shell, leave raw mode
				raise(SIGSTOP);   // Break out in mid-line
				enableRawMode();  // Back from Linux shell, re-enter raw mode
				dynamicRefresh(dp, activeHistoryLine.get(), activeHistoryLine.length(), historyLinePosition);
				continue;
			} break;
#endif

			// these characters assign the search string, and hence the selected input
			// line
			case ctrlChar('H'): // backspace/ctrl-H, delete char to left of cursor
				if ( dp._searchText.length() > 0 ) {
					dp._searchText.erase( dp._searchText.length() - 1 );
					dp.updateSearchPrompt();
					_history.reset_pos( dp._direction == -1 ? _history.size() - 1 : 0 );
				} else {
					beep();
				}
				break;

			case ctrlChar('Y'): // ctrl-Y, yank killed text
				break;

			default: {
				if (!isControlChar(c) && c <= 0x0010FFFF) { // not an action character
					dp._searchText.insert( dp._searchText.length(), c );
					dp.updateSearchPrompt();
				} else {
					beep();
				}
			}
		} // switch

		// if we are staying in search mode, search now
		if ( ! keepLooping ) {
			break;
		}
		activeHistoryLine.assign( _history.current() );
		if ( dp._searchText.length() > 0 ) {
			bool found = false;
			int historySearchIndex = _history.current_pos();
			int lineSearchPos = historyLinePosition;
			if ( searchAgain ) {
				lineSearchPos += dp._direction;
			}
			searchAgain = false;
			while ( true ) {
				while ( ( ( lineSearchPos + dp._searchText.length() ) <= activeHistoryLine.length() ) && ( lineSearchPos >= 0 ) ) {
					if ( std::equal( dp._searchText.begin(), dp._searchText.end(), activeHistoryLine.begin() + lineSearchPos ) ) {
						found = true;
						break;
					}
					lineSearchPos += dp._direction;
				}
				if ( found ) {
					_history.reset_pos( historySearchIndex );
					historyLinePosition = lineSearchPos;
					break;
				} else if ( ( dp._direction > 0 ) ? ( historySearchIndex < _history.size() ) : ( historySearchIndex > 0 ) ) {
					historySearchIndex += dp._direction;
					activeHistoryLine.assign( _history[historySearchIndex] );
					lineSearchPos = ( dp._direction > 0 ) ? 0 : ( activeHistoryLine.length() - dp._searchText.length() );
				} else {
					beep();
					break;
				}
			} // while
		}
		activeHistoryLine.assign( _history.current() );
		dynamicRefresh(dp, activeHistoryLine.get(), activeHistoryLine.length(), historyLinePosition); // draw user's text with our prompt
	} // while

	// leaving history search, restore previous prompt, maybe make searched line
	// current
	Prompt pb;
	pb._characterCount = _prompt._indentation;
	pb._byteCount = _prompt._byteCount;
	UnicodeString tempUnicode( &_prompt._text[_prompt._lastLinePosition], pb._byteCount - _prompt._lastLinePosition );
	pb._text = tempUnicode;
	pb._extraLines = 0;
	pb._indentation = _prompt._indentation;
	pb._lastLinePosition = 0;
	pb._previousInputLen = activeHistoryLine.length();
	pb._cursorRowOffset = dp._cursorRowOffset;
	pb.update_screen_columns();
	pb._previousLen = dp._characterCount;
	if ( useSearchedLine && ( activeHistoryLine.length() > 0 ) ) {
		_history.set_recall_most_recent();
		_data.assign( activeHistoryLine );
		_prefix = _pos = historyLinePosition;
	}
	dynamicRefresh(pb, _data.get(), _data.length(), _pos); // redraw the original prompt with current input
	_prompt._previousInputLen = _data.length();
	_prompt._cursorRowOffset = _prompt._extraLines + pb._cursorRowOffset;
	previousSearchText = dp._searchText; // save search text for possible reuse on ctrl-R ctrl-R
	return c; // pass a character or -1 back to main loop
}

void Replxx::ReplxxImpl::clearScreen( void ) {
	clear_screen();
	_prompt.write();
#ifndef _WIN32
	// we have to generate our own newline on line wrap on Linux
	if (_prompt._indentation == 0 && _prompt._extraLines > 0) {
		write8( "\n", 1 );
	}
#endif
	_prompt._cursorRowOffset = _prompt._extraLines;
	refreshLine();
}

bool Replxx::ReplxxImpl::is_word_break_character( char32_t char_ ) const {
	bool wbc( false );
	if ( char_ < 128 ) {
		wbc = strchr( _breakChars, static_cast<char>( char_ ) ) != nullptr;
	}
	return ( wbc );
}

void Replxx::ReplxxImpl::history_add( std::string const& line ) {
	_history.add( line );
}

int Replxx::ReplxxImpl::history_save( std::string const& filename ) {
	return ( _history.save( filename ) );
}

int Replxx::ReplxxImpl::history_load( std::string const& filename ) {
	return ( _history.load( filename ) );
}

int Replxx::ReplxxImpl::history_size( void ) const {
	return ( _history.size() );
}

std::string const& Replxx::ReplxxImpl::history_line( int index ) {
	return ( _history[index] );
}

void Replxx::ReplxxImpl::set_completion_callback( Replxx::completion_callback_t const& fn ) {
	_completionCallback = fn;
}

void Replxx::ReplxxImpl::set_highlighter_callback( Replxx::highlighter_callback_t const& fn ) {
	_highlighterCallback = fn;
}

void Replxx::ReplxxImpl::set_hint_callback( Replxx::hint_callback_t const& fn ) {
	_hintCallback = fn;
}

void Replxx::ReplxxImpl::set_max_history_size( int len ) {
	_history.set_max_size( len );
}

void Replxx::ReplxxImpl::set_completion_count_cutoff( int count ) {
	_completionCountCutoff = count;
}

void Replxx::ReplxxImpl::set_max_hint_rows( int count ) {
	_maxHintRows = count;
}

void Replxx::ReplxxImpl::set_word_break_characters( char const* wordBreakers ) {
	_breakChars = wordBreakers;
}

void Replxx::ReplxxImpl::set_double_tab_completion( bool val ) {
	_doubleTabCompletion = val;
}

void Replxx::ReplxxImpl::set_complete_on_empty( bool val ) {
	_completeOnEmpty = val;
}

void Replxx::ReplxxImpl::set_beep_on_ambiguous_completion( bool val ) {
	_beepOnAmbiguousCompletion = val;
}

void Replxx::ReplxxImpl::set_no_color( bool val ) {
	_noColor = val;
}

/**
 * Display the dynamic incremental search prompt and the current user input
 * line.
 * @param pi	 Prompt struct holding information about the prompt and our
 * screen position
 * @param buf32	input buffer to be displayed
 * @param len	count of characters in the buffer
 * @param pos	current cursor position within the buffer (0 <= pos <= len)
 */
void dynamicRefresh(Prompt& pi, char32_t* buf32, int len, int pos) {
	// calculate the position of the end of the prompt
	int xEndOfPrompt, yEndOfPrompt;
	calculateScreenPosition(0, 0, pi.screen_columns(), pi._characterCount,
													xEndOfPrompt, yEndOfPrompt);
	pi._indentation = xEndOfPrompt;

	// calculate the position of the end of the input line
	int xEndOfInput, yEndOfInput;
	calculateScreenPosition(xEndOfPrompt, yEndOfPrompt, pi.screen_columns(),
													calculateColumnPosition(buf32, len), xEndOfInput,
													yEndOfInput);

	// calculate the desired position of the cursor
	int xCursorPos, yCursorPos;
	calculateScreenPosition(xEndOfPrompt, yEndOfPrompt, pi.screen_columns(),
													calculateColumnPosition(buf32, pos), xCursorPos,
													yCursorPos);

#ifdef _WIN32
	// position at the start of the prompt, clear to end of previous input
	CONSOLE_SCREEN_BUFFER_INFO inf;
	GetConsoleScreenBufferInfo(console_out, &inf);
	inf.dwCursorPosition.X = 0;
	inf.dwCursorPosition.Y -= pi._cursorRowOffset /*- pi._extraLines*/;
	SetConsoleCursorPosition(console_out, inf.dwCursorPosition);
	DWORD count;
	FillConsoleOutputCharacterA(
		console_out, ' ',
		pi._previousLen + pi._previousInputLen,
		inf.dwCursorPosition, &count
	);
	pi._previousLen = pi._indentation;
	pi._previousInputLen = len;

	// display the prompt
	pi.write();

	// display the input line
	write32( buf32, len );

	// position the cursor
	GetConsoleScreenBufferInfo(console_out, &inf);
	inf.dwCursorPosition.X = xCursorPos; // 0-based on Win32
	inf.dwCursorPosition.Y -= yEndOfInput - yCursorPos;
	SetConsoleCursorPosition(console_out, inf.dwCursorPosition);
#else // _WIN32
	char seq[64];
	int cursorRowMovement = pi._cursorRowOffset - pi._extraLines;
	if (cursorRowMovement > 0) { // move the cursor up as required
		snprintf(seq, sizeof seq, "\x1b[%dA", cursorRowMovement);
		write8( seq, strlen( seq ) );
	}
	// position at the start of the prompt, clear to end of screen
	snprintf(seq, sizeof seq, "\x1b[1G\x1b[J"); // 1-based on VT100
	write8( seq, strlen( seq ) );

	// display the prompt
	pi.write();

	// display the input line
	write32( buf32, len );

	// we have to generate our own newline on line wrap
	if (xEndOfInput == 0 && yEndOfInput > 0) {
		write8( "\n", 1 );
	}

	// position the cursor
	cursorRowMovement = yEndOfInput - yCursorPos;
	if (cursorRowMovement > 0) { // move the cursor up as required
		snprintf(seq, sizeof seq, "\x1b[%dA", cursorRowMovement);
		write8( seq, strlen( seq ) );
	}
	// position the cursor within the line
	snprintf(seq, sizeof seq, "\x1b[%dG", xCursorPos + 1); // 1-based on VT100
	write8( seq, strlen( seq ) );
#endif

	pi._cursorRowOffset = pi._extraLines + yCursorPos; // remember row for next pass
}

}


#ifndef REPLXX_INPUTBUFFER_HXX_INCLUDED
#define REPLXX_INPUTBUFFER_HXX_INCLUDED 1

#include <vector>
#include <memory>

#include "replxx.h"
#include "prompt.hxx"

struct replxx_completions {
	std::vector<replxx::Utf32String> completionStrings;
};

struct replxx_hints {
	std::vector<replxx::Utf32String> hintsStrings;
};

namespace replxx {

struct PromptBase;

class InputBuffer {
public:
	typedef std::unique_ptr<char32_t[]> input_buffer_t;
	typedef std::unique_ptr<char[]> char_widths_t;
	typedef std::vector<char32_t> display_t;
	enum class HINT_ACTION {
		REGENERATE,
		REPAINT,
		SKIP
	};
private:
	input_buffer_t _buf32;      // input buffer
	char_widths_t  _charWidths; // character widths from mk_wcwidth()
	display_t       _display;
	Utf32String _hint;
	int _buflen; // buffer size in characters
	int _len;    // length of text in input buffer
	int _pos;    // character position in buffer ( 0 <= _pos <= _len )
	int _prefix; // prefix length used in common prefix search
	int _hintSelection; // Currently selected hint.

	void clearScreen(PromptBase& pi);
	int incrementalHistorySearch(PromptBase& pi, int startChar);
	void commonPrefixSearch(PromptBase& pi, int startChar);
	int completeLine(PromptBase& pi);
	void refreshLine(PromptBase& pi, HINT_ACTION = HINT_ACTION::REGENERATE);
	void highlight( int, bool );
	void handle_hints( int, HINT_ACTION );
	void setColor( replxx_color::color );
	int start_index( void );

 public:
	InputBuffer(int bufferLen)
		: _buf32(new char32_t[bufferLen])
		, _charWidths(new char[bufferLen])
		, _display()
		, _hint()
		, _buflen(bufferLen - 1)
		, _len(0)
		, _pos(0)
		, _prefix( 0 )
		, _hintSelection( -1 ) {
		_buf32[0] = 0;
	}
	void preloadBuffer( char const* preloadText );
	int getInputLine(PromptBase& pi);
	int length(void) const { return _len; }
	char32_t* buf() {
		return ( _buf32.get() );
	}
};

}

#endif


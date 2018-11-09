#ifndef REPLXX_INPUTBUFFER_HXX_INCLUDED
#define REPLXX_INPUTBUFFER_HXX_INCLUDED 1

#include <vector>
#include <memory>

#include "replxx.hxx"
#include "replxx_impl.hxx"
#include "prompt.hxx"

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
	enum class NEXT {
		CONTINUE,
		RETURN,
		BAIL
	};
	static int const REPLXX_MAX_LINE = 4096;
private:
	Replxx::ReplxxImpl& _replxx;
	int            _buflen;     // buffer size in characters
	input_buffer_t _buf32;      // input buffer
	char_widths_t  _charWidths; // character widths from mk_wcwidth()
	display_t      _display;
	Utf32String    _hint;
	int _len;    // length of text in input buffer
	int _pos;    // character position in buffer ( 0 <= _pos <= _len )
	int _prefix; // prefix length used in common prefix search
	int _hintSelection; // Currently selected hint.
	History& _history;
	KillRing& _killRing;

	void clearScreen(PromptBase& pi);
	int incrementalHistorySearch(PromptBase& pi, int startChar);
	void commonPrefixSearch(PromptBase& pi, int startChar);
	int completeLine(PromptBase& pi);
	void refreshLine(PromptBase& pi, HINT_ACTION = HINT_ACTION::REGENERATE);
	void highlight( int, bool );
	int handle_hints( PromptBase&, HINT_ACTION );
	void setColor( Replxx::Color );
	int start_index( void );
	void realloc( int );

public:
	InputBuffer( Replxx::ReplxxImpl& );
	void preloadBuffer( char const* preloadText );
	int getInputLine( PromptBase& pi );
	int length( void ) const {
		return _len;
	}
	char32_t* buf() {
		return ( _buf32.get() );
	}
};

}

#endif


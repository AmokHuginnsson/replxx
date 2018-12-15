/*
 * Copyright (c) 2017-2018, Marcin Konarski (amok at codestation.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HAVE_REPLXX_REPLXX_IMPL_HXX_INCLUDED
#define HAVE_REPLXX_REPLXX_IMPL_HXX_INCLUDED 1

#include <vector>
#include <memory>
#include <string>

#include "replxx.hxx"
#include "history.hxx"
#include "killring.hxx"
#include "unicodestring.hxx"

namespace replxx {

struct PromptBase;

class Replxx::ReplxxImpl {
public:
	typedef std::vector<UnicodeString> completions_t;
	typedef std::vector<UnicodeString> hints_t;
	typedef std::unique_ptr<char[]> utf8_buffer_t;
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
	int _maxCharacterCount;
	utf8_buffer_t _utf8Buffer;
	int            _buflen;     // buffer size in characters
	input_buffer_t _buf32;      // input buffer
	char_widths_t  _charWidths; // character widths from mk_wcwidth()
	display_t      _display;
	UnicodeString  _hint;
	int _len;    // length of text in input buffer
	int _pos;    // character position in buffer ( 0 <= _pos <= _len )
	int _prefix; // prefix length used in common prefix search
	int _hintSelection; // Currently selected hint.
	History _history;
	KillRing _killRing;
	int _maxHintRows;
	char const* _breakChars;
	int _completionCountCutoff;
	bool _doubleTabCompletion;
	bool _completeOnEmpty;
	bool _beepOnAmbiguousCompletion;
	bool _noColor;
	Replxx::completion_callback_t _completionCallback;
	Replxx::highlighter_callback_t _highlighterCallback;
	Replxx::hint_callback_t _hintCallback;
	std::string _preloadedBuffer; // used with set_preload_buffer
	std::string _errorMessage;
public:
	ReplxxImpl( FILE*, FILE*, FILE* );
	void set_completion_callback( Replxx::completion_callback_t const& fn );
	void set_highlighter_callback( Replxx::highlighter_callback_t const& fn );
	void set_hint_callback( Replxx::hint_callback_t const& fn );
	char const* input( std::string const& prompt );
	void history_add( std::string const& line );
	int history_save( std::string const& filename );
	int history_load( std::string const& filename );
	std::string const& history_line( int index );
	int history_size( void ) const;
	void set_preload_buffer(std::string const& preloadText);
	void set_word_break_characters( char const* wordBreakers );
	void set_max_hint_rows( int count );
	void set_double_tab_completion( bool val );
	void set_complete_on_empty( bool val );
	void set_beep_on_ambiguous_completion( bool val );
	void set_no_color( bool val );
	void set_max_history_size( int len );
	void set_completion_count_cutoff( int len );
	void clear_screen( void );
	int install_window_change_handler( void );
	completions_t call_completer( std::string const& input, int& ) const;
	hints_t call_hinter( std::string const& input, int&, Replxx::Color& color ) const;
	int print( char const* , int );
private:
	ReplxxImpl( ReplxxImpl const& ) = delete;
	ReplxxImpl& operator = ( ReplxxImpl const& ) = delete;
private:
	void preloadBuffer( char const* preloadText );
	int getInputLine( PromptBase& pi );
	int length( void ) const {
		return _len;
	}
	char32_t* buf() {
		return ( _buf32.get() );
	}
	NEXT insert_character( PromptBase&, int );
	void realloc_utf8_buffer( int );
	void realloc( int );
	char const* read_from_stdin( void );
	void clearScreen(PromptBase& pi);
	int incrementalHistorySearch(PromptBase& pi, int startChar);
	void commonPrefixSearch(PromptBase& pi, int startChar);
	int completeLine(PromptBase& pi);
	void refreshLine(PromptBase& pi, HINT_ACTION = HINT_ACTION::REGENERATE);
	void highlight( int, bool );
	int handle_hints( PromptBase&, HINT_ACTION );
	void setColor( Replxx::Color );
	int context_length( void );
	void clear();
	bool is_word_break_character( char32_t ) const;
};

}

#endif


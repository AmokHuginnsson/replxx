/* linenoise.c -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * Copyright (c) 2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *	 * Redistributions of source code must retain the above copyright notice,
 *		 this list of conditions and the following disclaimer.
 *	 * Redistributions in binary form must reproduce the above copyright
 *		 notice, this list of conditions and the following disclaimer in the
 *		 documentation and/or other materials provided with the distribution.
 *	 * Neither the name of Redis nor the names of its contributors may be used
 *		 to endorse or promote products derived from this software without
 *		 specific prior written permission.
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
 *
 * line editing lib needs to be 20,000 lines of C code.
 *
 * You can find the latest source code at:
 *
 *	 http://github.com/antirez/linenoise
 *
 * Does a number of crazy assumptions that happen to be true in 99.9999% of
 * the 2010 UNIX computers around.
 *
 * References:
 * - http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * - http://www.3waylabs.com/nw/WWW/products/wizcon/vt220.html
 *
 * Todo list:
 * - Switch to gets() if $TERM is something we can't support.
 * - Filter bogus Ctrl+<char> combinations.
 * - Win32 support
 *
 * Bloat:
 * - Completion?
 * - History search like Ctrl+r in readline?
 *
 * List of escape sequences used by this program, we do everything just
 * with three sequences. In order to be so cheap we may have some
 * flickering effect with some slow terminal, but the lesser sequences
 * the more compatible.
 *
 * CHA (Cursor Horizontal Absolute)
 *		Sequence: ESC [ n G
 *		Effect: moves cursor to column n (1 based)
 *
 * EL (Erase Line)
 *		Sequence: ESC [ n K
 *		Effect: if n is 0 or missing, clear from cursor to end of line
 *		Effect: if n is 1, clear from beginning of line to cursor
 *		Effect: if n is 2, clear entire line
 *
 * CUF (Cursor Forward)
 *		Sequence: ESC [ n C
 *		Effect: moves cursor forward of n chars
 *
 * The following are used to clear the screen: ESC [ H ESC [ 2 J
 * This is actually composed of two sequences:
 *
 * cursorhome
 *		Sequence: ESC [ H
 *		Effect: moves the cursor to upper left corner
 *
 * ED2 (Clear entire screen)
 *		Sequence: ESC [ 2 J
 *		Effect: clear the whole screen
 *
 */

#include <string>
#include <vector>
#include <memory>
#include <cerrno>
#include <cstdarg>

#ifdef _WIN32

#include <conio.h>
#include <windows.h>
#include <io.h>
#if _MSC_VER < 1900
#define snprintf _snprintf	// Microsoft headers use underscores in some names
#endif
#define strcasecmp _stricmp
#define strdup _strdup
#define write _write
#define STDIN_FILENO 0

#else /* _WIN32 */

#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#endif /* _WIN32 */

#include "replxx.h"
#include "conversion.hxx"

#ifdef _WIN32
#include "windows.hxx"
#endif

#include "prompt.hxx"
#include "io.hxx"
#include "util.hxx"
#include "inputbuffer.hxx"
#include "keycodes.hxx"
#include "escape.hxx"
#include "history.hxx"
#include "setup.hxx"

using std::string;
using std::vector;
using std::unique_ptr;
using namespace replxx;

namespace replxx {

#ifndef _WIN32
bool gotResize = false;
#endif
static const char* unsupported_term[] = {"dumb", "cons25", "emacs", NULL};

static bool isUnsupportedTerm(void) {
	char* term = getenv("TERM");
	if (term == NULL) return false;
	for (int j = 0; unsupported_term[j]; ++j)
		if (!strcasecmp(term, unsupported_term[j])) {
			return true;
		}
	return false;
}

/**
 * Display the dynamic incremental search prompt and the current user input
 * line.
 * @param pi	 PromptBase struct holding information about the prompt and our
 * screen position
 * @param buf32	input buffer to be displayed
 * @param len	count of characters in the buffer
 * @param pos	current cursor position within the buffer (0 <= pos <= len)
 */
void dynamicRefresh(PromptBase& pi, char32_t* buf32, int len, int pos) {
	// calculate the position of the end of the prompt
	int xEndOfPrompt, yEndOfPrompt;
	calculateScreenPosition(0, 0, pi.promptScreenColumns, pi.promptChars,
													xEndOfPrompt, yEndOfPrompt);
	pi.promptIndentation = xEndOfPrompt;

	// calculate the position of the end of the input line
	int xEndOfInput, yEndOfInput;
	calculateScreenPosition(xEndOfPrompt, yEndOfPrompt, pi.promptScreenColumns,
													calculateColumnPosition(buf32, len), xEndOfInput,
													yEndOfInput);

	// calculate the desired position of the cursor
	int xCursorPos, yCursorPos;
	calculateScreenPosition(xEndOfPrompt, yEndOfPrompt, pi.promptScreenColumns,
													calculateColumnPosition(buf32, pos), xCursorPos,
													yCursorPos);

#ifdef _WIN32
	// position at the start of the prompt, clear to end of previous input
	CONSOLE_SCREEN_BUFFER_INFO inf;
	GetConsoleScreenBufferInfo(console_out, &inf);
	inf.dwCursorPosition.X = 0;
	inf.dwCursorPosition.Y -= pi.promptCursorRowOffset /*- pi.promptExtraLines*/;
	SetConsoleCursorPosition(console_out, inf.dwCursorPosition);
	DWORD count;
	FillConsoleOutputCharacterA(console_out, ' ',
															pi.promptPreviousLen + pi.promptPreviousInputLen,
															inf.dwCursorPosition, &count);
	pi.promptPreviousLen = pi.promptIndentation;
	pi.promptPreviousInputLen = len;

	// display the prompt
	if (!pi.write()) return;

	// display the input line
	if (write32(1, buf32, len) == -1) return;

	// position the cursor
	GetConsoleScreenBufferInfo(console_out, &inf);
	inf.dwCursorPosition.X = xCursorPos;	// 0-based on Win32
	inf.dwCursorPosition.Y -= yEndOfInput - yCursorPos;
	SetConsoleCursorPosition(console_out, inf.dwCursorPosition);
#else	// _WIN32
	char seq[64];
	int cursorRowMovement = pi.promptCursorRowOffset - pi.promptExtraLines;
	if (cursorRowMovement > 0) {	// move the cursor up as required
		snprintf(seq, sizeof seq, "\x1b[%dA", cursorRowMovement);
		if (write(1, seq, strlen(seq)) == -1) return;
	}
	// position at the start of the prompt, clear to end of screen
	snprintf(seq, sizeof seq, "\x1b[1G\x1b[J");	// 1-based on VT100
	if (write(1, seq, strlen(seq)) == -1) return;

	// display the prompt
	if (!pi.write()) return;

	// display the input line
	if (write32(1, buf32, len) == -1) return;

	// we have to generate our own newline on line wrap
	if (xEndOfInput == 0 && yEndOfInput > 0)
		if (write(1, "\n", 1) == -1) return;

	// position the cursor
	cursorRowMovement = yEndOfInput - yCursorPos;
	if (cursorRowMovement > 0) {	// move the cursor up as required
		snprintf(seq, sizeof seq, "\x1b[%dA", cursorRowMovement);
		if (write(1, seq, strlen(seq)) == -1) return;
	}
	// position the cursor within the line
	snprintf(seq, sizeof seq, "\x1b[%dG", xCursorPos + 1);	// 1-based on VT100
	if (write(1, seq, strlen(seq)) == -1) return;
#endif

	pi.promptCursorRowOffset =
			pi.promptExtraLines + yCursorPos;	// remember row for next pass
}

}

/**
 * Clear the screen ONLY (no redisplay of anything)
 */
void replxx_clear_screen(void) {
#ifdef _WIN32
	COORD coord = {0, 0};
	CONSOLE_SCREEN_BUFFER_INFO inf;
	HANDLE screenHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(screenHandle, &inf);
	SetConsoleCursorPosition(screenHandle, coord);
	DWORD count;
	FillConsoleOutputCharacterA(screenHandle, ' ', inf.dwSize.X * inf.dwSize.Y,
															coord, &count);
#else
	char const clearCode[] = "\033c\033[H\033[2J\033[0m";
	if (write(1, clearCode, sizeof ( clearCode ) - 1) <= 0) return;
#endif
}

static string preloadedBufferContents;	// used with replxx_set_preload_buffer
static string preloadErrorMessage;

/**
 * replxx_set_preload_buffer provides text to be inserted into the command buffer
 *
 * the provided text will be processed to be usable and will be used to preload
 * the input buffer on the next call to replxx_input()
 *
 * @param preloadText text to begin with on the next call to replxx_input()
 */
void replxx_set_preload_buffer(const char* preloadText) {
	if (!preloadText) {
		return;
	}
	int bufferSize = static_cast<int>(strlen(preloadText) + 1);
	unique_ptr<char[]> tempBuffer(new char[bufferSize]);
	strncpy(&tempBuffer[0], preloadText, bufferSize);

	// remove characters that won't display correctly
	char* pIn = &tempBuffer[0];
	char* pOut = pIn;
	bool controlsStripped = false;
	bool whitespaceSeen = false;
	while (*pIn) {
		unsigned char c =
				*pIn++;			 // we need unsigned so chars 0x80 and above are allowed
		if ('\r' == c) {	// silently skip CR
			continue;
		}
		if ('\n' == c || '\t' == c) {	// note newline or tab
			whitespaceSeen = true;
			continue;
		}
		if (isControlChar(
						c)) {	// remove other control characters, flag for message
			controlsStripped = true;
			*pOut++ = ' ';
			continue;
		}
		if (whitespaceSeen) {	// convert whitespace to a single space
			*pOut++ = ' ';
			whitespaceSeen = false;
		}
		*pOut++ = c;
	}
	*pOut = 0;
	int processedLength = static_cast<int>(pOut - tempBuffer.get());
	bool lineTruncated = false;
	if (processedLength > (setup.maxLineLength - 1)) {
		lineTruncated = true;
		tempBuffer[setup.maxLineLength - 1] = 0;
	}
	preloadedBufferContents = tempBuffer.get();
	if (controlsStripped) {
		preloadErrorMessage +=
				" [Edited line: control characters were converted to spaces]\n";
	}
	if (lineTruncated) {
		preloadErrorMessage += " [Edited line: the line length was reduced from ";
		char buf[128];
		snprintf(buf, sizeof(buf), "%d to %d]\n", processedLength,
						 (setup.maxLineLength - 1));
		preloadErrorMessage += buf;
	}
}

/**
 * replxx_input is a readline replacement.
 *
 * call it with a prompt to display and it will return a line of input from the
 * user
 *
 * @param prompt text of prompt to display to the user
 * @return			 the returned string belongs to the caller on return and must be
 * freed to prevent
 *							 memory leaks
 */
char* replxx_input(const char* prompt) {
#ifndef _WIN32
	gotResize = false;
#endif
	if ( tty::in ) {	// input is from a terminal
		if (!preloadErrorMessage.empty()) {
			printf("%s", preloadErrorMessage.c_str());
			fflush(stdout);
			preloadErrorMessage.clear();
		}
		PromptInfo pi(prompt, getScreenColumns());
		if (isUnsupportedTerm()) {
			if (!pi.write()) return 0;
			fflush(stdout);
			if (preloadedBufferContents.empty()) {
				unique_ptr<char[]> buf8(new char[setup.maxLineLength]);
				if (fgets(buf8.get(), setup.maxLineLength, stdin) == NULL) {
					return NULL;
				}
				size_t len = strlen(buf8.get());
				while (len && (buf8[len - 1] == '\n' || buf8[len - 1] == '\r')) {
					--len;
					buf8[len] = '\0';
				}
				return strdup(buf8.get());	// caller must free buffer
			} else {
				char* buf8 = strdup(preloadedBufferContents.c_str());
				preloadedBufferContents.clear();
				return buf8;	// caller must free buffer
			}
		} else {
			if (enableRawMode() == -1) {
				return NULL;
			}
			InputBuffer ib(setup.maxLineLength);
			if (!preloadedBufferContents.empty()) {
				ib.preloadBuffer(preloadedBufferContents.c_str());
				preloadedBufferContents.clear();
			}
			int count = ib.getInputLine(pi);
			disableRawMode();
			if (count == -1) {
				return NULL;
			}
			printf("\n");
			size_t bufferSize = sizeof(char32_t) * ib.length() + 1;
			unique_ptr<char[]> buf8(new char[bufferSize]);
			copyString32to8(buf8.get(), bufferSize, ib.buf());
			return strdup(buf8.get());	// caller must free buffer
		}
	} else {	// input not from a terminal, we should work with piped input, i.e.
						// redirected stdin
		unique_ptr<char[]> buf8(new char[setup.maxLineLength]);
		if (fgets(buf8.get(), setup.maxLineLength, stdin) == NULL) {
			return NULL;
		}

		// if fgets() gave us the newline, remove it
		int count = static_cast<int>(strlen(buf8.get()));
		if (count > 0 && buf8[count - 1] == '\n') {
			--count;
			buf8[count] = '\0';
		}
		return strdup(buf8.get());	// caller must free buffer
	}
}

void replxx_free( void* mem_ ) {
	::free( mem_ );
}

int replxx_print( char const* format_, ... ) {
	::std::va_list ap;
	va_start( ap, format_ );
	int long size = vsnprintf( nullptr, 0, format_, ap );
	va_end( ap );
	va_start( ap, format_ );
	unique_ptr<char[]> buf( new char[size + 1] );
	vsnprintf( buf.get(), static_cast<size_t>( size + 1 ), format_, ap );
	va_end( ap );
#ifdef _WIN32
	int count( win_print( buf.get(), size ) );
#else
	int count( write( 1, buf.get(), size ) );
#endif
	return ( count );
}

/* Register a callback function to be called for tab-completion. */
void replxx_set_completion_callback(replxx_completion_callback_t* fn, void* userData) {
	setup.completionCallback = fn;
	setup.completionUserdata = userData;
}

void replxx_set_highlighter_callback(replxx_highlighter_callback_t* fn, void* userData) {
	setup.highlighterCallback = fn;
	setup.highlighterUserdata = userData;
}

void replxx_set_hint_callback(replxx_hint_callback_t* fn, void* userData) {
	setup.hintCallback = fn;
	setup.hintUserdata = userData;
}

void replxx_add_hint(replxx_hints* lh, const char* str) {
	lh->hintsStrings.push_back(Utf32String(str));
}

void replxx_add_completion(replxx_completions* lc, const char* str) {
	lc->completionStrings.push_back(Utf32String(str));
}

int replxx_history_add(const char* line) {
	return replxx::history_add( line );
}

int replxx_set_max_history_size(int len) {
	return replxx::set_max_history_size( len );
}

void replxx_set_max_line_size(int len) {
	setup.maxLineLength = len;
}

void replxx_set_max_hint_rows(int count) {
	setup.maxHintRows = count;
}

void replxx_set_word_break_characters( char const* breakChars_ ) {
	setup.breakChars = breakChars_;
}

void replxx_set_special_prefixes( char const* specialPrefixes_ ) {
	setup.specialPrefixes = specialPrefixes_;
}

void replxx_set_double_tab_completion(int val) {
	setup.doubleTabCompletion = val ? true : false;
}

void replxx_set_complete_on_empty(int val) {
	setup.completeOnEmpty = val ? true : false;
}

void replxx_set_no_color( int val ) {
	setup.noColor = val ? true : false;
}

void replxx_set_beep_on_ambiguous_completion(int val) {
	setup.beepOnAmbiguousCompletion = val ? true : false;
}

/* Fetch a line of the history by (zero-based) index.	If the requested
 * line does not exist, NULL is returned.	The return value is a heap-allocated
 * copy of the line, and the caller is responsible for de-allocating it. */
char* replxx_history_line(int index) {
	if (index < 0 || index >= historyLen) return NULL;

	return strdup(reinterpret_cast<char const*>(history[index]));
}

/* Save the history in the specified file. On success 0 is returned
 * otherwise -1 is returned. */
int replxx_history_save(const char* filename) {
#ifndef _WIN32
	mode_t old_umask = umask(S_IXUSR|S_IRWXG|S_IRWXO);
#endif
	FILE* fp = fopen(filename, "wt");

	if (fp == NULL) {
		return -1;
	}

#ifndef _WIN32
	umask(old_umask);
	chmod(filename,S_IRUSR|S_IWUSR);
#endif

	for (int j = 0; j < historyLen; ++j) {
		if (history[j][0] != '\0') {
			fprintf(fp, "%s\n", history[j]);
		}
	}
	fclose(fp);
	return 0;
}

/* Load the history from the specified file. If the file does not exist
 * zero is returned and no operation is performed.
 *
 * If the file exists and the operation succeeded 0 is returned, otherwise
 * on error -1 is returned. */
int replxx_history_load(const char* filename) {
	FILE* fp = fopen(filename, "rt");
	if (fp == NULL) {
		return -1;
	}

	unique_ptr<char[]> buf( new char[setup.maxLineLength] );
	while (fgets(buf.get(), setup.maxLineLength, fp) != NULL) {
		char* p = strchr(buf.get(), '\r');
		if (!p) {
			p = strchr(buf.get(), '\n');
		}
		if (p) {
			*p = '\0';
		}
		if (p != buf.get()) {
			replxx::history_add(buf.get());
		}
	}
	fclose(fp);
	return 0;
}

/* This special mode is used by replxx in order to print scan codes
 * on screen for debugging / development purposes. It is implemented
 * by the linenoise_example program using the --keycodes option. */
void replxx_debug_dump_print_codes(void) {
	char quit[4];

	printf(
			"replxx key codes debugging mode.\n"
			"Press keys to see scan codes. Type 'quit' at any time to exit.\n");
	if (enableRawMode() == -1) return;
	memset(quit, ' ', 4);
	while (1) {
		char c;
		int nread;

#if _WIN32
		nread = _read(STDIN_FILENO, &c, 1);
#else
		nread = read(STDIN_FILENO, &c, 1);
#endif
		if (nread <= 0) continue;
		memmove(quit, quit + 1, sizeof(quit) - 1); /* shift string to left. */
		quit[sizeof(quit) - 1] = c; /* Insert current char on the right. */
		if (memcmp(quit, "quit", sizeof(quit)) == 0) break;

		printf("'%c' %02x (%d) (type quit to exit)\n", isprint(c) ? c : '?', (int)c,
					 (int)c);
		printf("\r"); /* Go left edge manually, we are in raw mode. */
		fflush(stdout);
	}
	disableRawMode();
}

#ifndef _WIN32
static void WindowSizeChanged(int) {
	// do nothing here but setting this flag
	gotResize = true;
}
#endif

void replxx_history_free(void) {
	replxx::history_free();
}

int replxx_install_window_change_handler(void) {
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

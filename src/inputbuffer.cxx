#ifdef _WIN32

#include <conio.h>
#include <windows.h>
#include <io.h>
#if _MSC_VER < 1900
#define snprintf _snprintf	// Microsoft headers use underscores in some names
#endif
#define strcasecmp _stricmp
#define strdup _strdup
#define isatty _isatty
#define write _write
#define STDIN_FILENO 0

#else /* _WIN32 */

#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <cctype>
#include <wctype.h>

#endif /* _WIN32 */

#include "inputbuffer.hxx"
#include "prompt.hxx"
#include "util.hxx"
#include "io.hxx"

namespace replxx {

void InputBuffer::preloadBuffer(const char* preloadText) {
	size_t ucharCount = 0;
	copyString8to32(buf32, buflen + 1, ucharCount, preloadText);
	recomputeCharacterWidths(buf32, charWidths, static_cast<int>(ucharCount));
	len = static_cast<int>(ucharCount);
	pos = static_cast<int>(ucharCount);
}

/**
 * Refresh the user's input line: the prompt is already onscreen and is not
 * redrawn here
 * @param pi	 PromptBase struct holding information about the prompt and our
 * screen position
 */
void InputBuffer::refreshLine(PromptBase& pi) {
	// check for a matching brace/bracket/paren, remember its position if found
	int highlight = -1;
	if (pos < len) {
		/* this scans for a brace matching buf32[pos] to highlight */
		int scanDirection = 0;
		if (strchr("}])", buf32[pos]))
			scanDirection = -1; /* backwards */
		else if (strchr("{[(", buf32[pos]))
			scanDirection = 1; /* forwards */

		if (scanDirection) {
			int unmatched = scanDirection;
			for (int i = pos + scanDirection; i >= 0 && i < len; i += scanDirection) {
				/* TODO: the right thing when inside a string */
				if (strchr("}])", buf32[i]))
					--unmatched;
				else if (strchr("{[(", buf32[i]))
					++unmatched;

				if (unmatched == 0) {
					highlight = i;
					break;
				}
			}
		}
	}

	// calculate the position of the end of the input line
	int xEndOfInput, yEndOfInput;
	calculateScreenPosition(pi.promptIndentation, 0, pi.promptScreenColumns,
													calculateColumnPosition(buf32, len), xEndOfInput,
													yEndOfInput);

	// calculate the desired position of the cursor
	int xCursorPos, yCursorPos;
	calculateScreenPosition(pi.promptIndentation, 0, pi.promptScreenColumns,
													calculateColumnPosition(buf32, pos), xCursorPos,
													yCursorPos);

#ifdef _WIN32
	// position at the end of the prompt, clear to end of previous input
	CONSOLE_SCREEN_BUFFER_INFO inf;
	GetConsoleScreenBufferInfo(console_out, &inf);
	inf.dwCursorPosition.X = pi.promptIndentation;	// 0-based on Win32
	inf.dwCursorPosition.Y -= pi.promptCursorRowOffset - pi.promptExtraLines;
	SetConsoleCursorPosition(console_out, inf.dwCursorPosition);
	DWORD count;
	if (len < pi.promptPreviousInputLen)
		FillConsoleOutputCharacterA(console_out, ' ', pi.promptPreviousInputLen,
																inf.dwCursorPosition, &count);
	pi.promptPreviousInputLen = len;

	// display the input line
	if (highlight == -1) {
		if (write32(1, buf32, len) == -1) return;
	} else {
		if (write32(1, buf32, highlight) == -1) return;
		setDisplayAttribute(true); /* bright blue (visible with both B&W bg) */
		if (write32(1, &buf32[highlight], 1) == -1) return;
		setDisplayAttribute(false);
		if (write32(1, buf32 + highlight + 1, len - highlight - 1) == -1) return;
	}

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
	// position at the end of the prompt, clear to end of screen
	snprintf(seq, sizeof seq, "\x1b[%dG\x1b[J",
					 pi.promptIndentation + 1);	// 1-based on VT100
	if (write(1, seq, strlen(seq)) == -1) return;

	if (highlight == -1) {	// write unhighlighted text
		if (write32(1, buf32, len) == -1) return;
	} else {	// highlight the matching brace/bracket/parenthesis
		if (write32(1, buf32, highlight) == -1) return;
		setDisplayAttribute(true);
		if (write32(1, &buf32[highlight], 1) == -1) return;
		setDisplayAttribute(false);
		if (write32(1, buf32 + highlight + 1, len - highlight - 1) == -1) return;
	}

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


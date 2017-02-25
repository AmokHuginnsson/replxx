#include <cstdlib>
#include <cstring>

#include "history.hxx"
#include "setup.hxx"

namespace replxx {

int historyLen = 0;
int historyIndex = 0;
char8_t** history = NULL;
// used to emulate Windows command prompt on down-arrow after a recall
// we use -2 as our "not set" value because we add 1 to the previous index on
// down-arrow,
// and zero is a valid index (so -1 is a valid "previous index")
int historyPreviousIndex = -2;
bool historyRecallMostRecent = false;

void history_free(void) {
	if (history) {
		for (int j = 0; j < historyLen; ++j) free(history[j]);
		historyLen = 0;
		free(history);
		history = 0;
	}
}

int history_add(const char* line) {
	if (setup.historyMaxLen == 0) {
		return 0;
	}
	if (history == NULL) {
		history =
				reinterpret_cast<char8_t**>(malloc(sizeof(char8_t*) * setup.historyMaxLen));
		if (history == NULL) {
			return 0;
		}
		memset(history, 0, (sizeof(char*) * setup.historyMaxLen));
	}
	char8_t* linecopy = strdup8(line);
	if (!linecopy) {
		return 0;
	}

	// convert newlines in multi-line code to spaces before storing
	char8_t* p = linecopy;
	while (*p) {
		if (*p == '\n') {
			*p = ' ';
		}
		++p;
	}

	// prevent duplicate history entries
	if (historyLen > 0 && history[historyLen - 1] != nullptr &&
			strcmp(reinterpret_cast<char const*>(history[historyLen - 1]),
						 reinterpret_cast<char const*>(linecopy)) == 0) {
		free(linecopy);
		return 0;
	}

	if (historyLen == setup.historyMaxLen) {
		free(history[0]);
		memmove(history, history + 1, sizeof(char*) * (setup.historyMaxLen - 1));
		--historyLen;
		if (--historyPreviousIndex < -1) {
			historyPreviousIndex = -2;
		}
	}

	history[historyLen] = linecopy;
	++historyLen;
	return 1;
}

int set_max_history_size(int len) {
	if (len < 1) {
		return 0;
	}
	if (history) {
		int tocopy = historyLen;
		char8_t** newHistory =
				reinterpret_cast<char8_t**>(malloc(sizeof(char8_t*) * len));
		if (newHistory == NULL) {
			return 0;
		}
		if (len < tocopy) {
			tocopy = len;
		}
		memcpy(newHistory, history + setup.historyMaxLen - tocopy,
					 sizeof(char8_t*) * tocopy);
		free(history);
		history = newHistory;
	}
	setup.historyMaxLen = len;
	if (historyLen > setup.historyMaxLen) {
		historyLen = setup.historyMaxLen;
	}
	return 1;
}

}


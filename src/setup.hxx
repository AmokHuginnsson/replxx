#ifndef REPLXX_SETUP_HXX_INCLUDED
#define REPLXX_SETUP_HXX_INCLUDED 1

#include "replxx.h"

namespace replxx {

struct Setup {
	int maxLineLength;
	int historyMaxLen;
	char const* breakChars;
	char const* specialPrefixes;
	int completionCountCutoff;
	bool doubleTabCompletion;
	bool completeOnEmpty;
	bool beepOnAmbiguousCompletion;
	replxx_completion_callback_t* completionCallback;
	replxx_highlighter_callback_t* highlighterCallback;
	Setup( void );
};

extern Setup setup;

}

#endif


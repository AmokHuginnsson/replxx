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
	bool noColor;
	replxx_completion_callback_t* completionCallback;
	replxx_ud_completion_callback_t* udCompletionCallback;
	replxx_ctx_completion_callback_t* ctxCompletionCallback;
	replxx_ud_ctx_completion_callback_t* udCtxCompletionCallback;
	replxx_highlighter_callback_t* highlighterCallback;
	replxx_ud_highlighter_callback_t* udHighlighterCallback;
	replxx_hint_callback_t* hintCallback;
	replxx_ud_hint_callback_t* udHintCallback;
   void* completionUserdata;
   void* ctxCompletionUserdata;
   void* highlighterUserdata;
   void* hintUserdata;
	Setup( void );
};

extern Setup setup;

}

#endif


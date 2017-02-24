#include "setup.hxx"

namespace {

#define REPLXX_DEFAULT_HISTORY_MAX_LEN 100
char const defaultBreakChars[] = " =+-/\\*?\"'`&<>;|@{([])}";

}

namespace replxx {

Setup::Setup()
	: historyMaxLen( REPLXX_DEFAULT_HISTORY_MAX_LEN )
	, breakChars( defaultBreakChars )
	, completionCountCutoff( 100 )
	, completionCallback( nullptr ) {
}

Setup setup;

}


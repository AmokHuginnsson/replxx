#ifndef REPLXX_HISTORY_HXX_INCLUDED
#define REPLXX_HISTORY_HXX_INCLUDED 1

#include "conversion.hxx"

namespace replxx {

extern int historyLen;
extern int historyIndex;
extern char8_t** history;
// used to emulate Windows command prompt on down-arrow after a recall
// we use -2 as our "not set" value because we add 1 to the previous index on
// down-arrow,
// and zero is a valid index (so -1 is a valid "previous index")
extern int historyPreviousIndex;
extern bool historyRecallMostRecent;

void history_free(void);
int history_add(const char* line);
int set_max_history_size(int len);

}

#endif


#ifndef REPLXX_REPLXX_HXX_INCLUDED
#define REPLXX_REPLXX_HXX_INCLUDED 1

#include "conversion.hxx"

namespace replxx {

struct PromptBase;

void dynamicRefresh(PromptBase& pi, char32_t* buf32, int len, int pos);

}

#endif

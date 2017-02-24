#ifndef REPLXX_IO_HXX_INCLUDED
#define REPLXX_IO_HXX_INCLUDED 1

namespace replxx {

int write32( int fd, char32_t* text32, int len32 );
int getScreenColumns(void);
int getScreenRows(void);
void setDisplayAttribute(bool enhancedDisplay);

}

#endif


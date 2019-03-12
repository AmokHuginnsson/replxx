#ifndef REPLXX_IO_HXX_INCLUDED
#define REPLXX_IO_HXX_INCLUDED 1

#ifdef _WIN32
#include <windows.h>
#endif

namespace replxx {

void write32( char32_t const*, int );
void write8( void const*, int );
int getScreenColumns(void);
int getScreenRows(void);
int enableRawMode(void);
void disableRawMode(void);
char32_t readUnicodeCharacter(void);
void beep();
char32_t read_char(void);
enum class CLEAR_SCREEN {
	WHOLE,
	TO_END
};
void clear_screen( CLEAR_SCREEN );

namespace tty {

extern bool in;
extern bool out;

}

#ifdef _WIN32
extern HANDLE console_out;
#endif

}

#endif


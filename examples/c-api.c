#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "replxx.h"
#include "util.h"

void completionHook(char const* context, replxx_completions* lc, int* contextLen, void* ud) {
	char** examples = (char**)( ud );
	size_t i;

	int utf8ContextLen = context_len( context );
	int prefixLen = strlen( context ) - utf8ContextLen;
	*contextLen = utf8str_codepoint_len( context + prefixLen, utf8ContextLen );
	for (i = 0;	examples[i] != NULL; ++i) {
		if (strncmp(context + prefixLen, examples[i], utf8ContextLen) == 0) {
			replxx_add_completion(lc, examples[i]);
		}
	}
}

void hintHook(char const* context, replxx_hints* lc, int* contextLen, ReplxxColor* c, void* ud) {
	char** examples = (char**)( ud );
	int i;
	int utf8ContextLen = context_len( context );
	int prefixLen = strlen( context ) - utf8ContextLen;
	*contextLen = utf8str_codepoint_len( context + prefixLen, utf8ContextLen );
	if ( *contextLen > 0 ) {
		for (i = 0;	examples[i] != NULL; ++i) {
			if (strncmp(context + prefixLen, examples[i], utf8ContextLen) == 0) {
				replxx_add_hint(lc, examples[i]);
			}
		}
	}
}

void colorHook( char const* str_, ReplxxColor* colors_, int size_, void* ud ) {
	int i = 0;
	for ( ; i < size_; ++ i ) {
		if ( isdigit( str_[i] ) ) {
			colors_[i] = REPLXX_COLOR_BRIGHTMAGENTA;
		}
	}
	if ( ( size_ > 0 ) && ( str_[size_ - 1] == '(' ) ) {
		replxx_emulate_key_press( ud, ')' );
		replxx_emulate_key_press( ud, REPLXX_KEY_LEFT );
	}
}

ReplxxActionResult word_eater( int ignored, void* ud ) {
	Replxx* replxx = (Replxx*)ud;
	return ( replxx_invoke( replxx, REPLXX_ACTION_KILL_TO_BEGINING_OF_WORD, 0 ) );
}

char const* recode( char* s ) {
	char const* r = s;
	while ( *s ) {
		if ( *s == '~' ) {
			*s = '\n';
		}
		++ s;
	}
	return ( r );
}

void split( char* str_, char** data_, int size_ ) {
	int i = 0;
	char* p = str_, *o = p;
	while ( i < size_ ) {
		int last = *p == 0;
		if ( ( *p == ',' ) || last ) {
			*p = 0;
			data_[i ++] = o;
			o = p + 1;
			if ( last ) {
				break;
			}
		}
		++ p;
	}
	data_[i] = 0;
}

int main( int argc, char** argv ) {
#define MAX_EXAMPLE_COUNT 128
	char* examples[MAX_EXAMPLE_COUNT + 1] = {
		"db", "hello", "hallo", "hans", "hansekogge", "seamann", "quetzalcoatl", "quit", "power", NULL
	};
	Replxx* replxx = replxx_init();
	replxx_install_window_change_handler( replxx );

	int quiet = 0;
	char const* prompt = "\x1b[1;32mreplxx\x1b[0m> ";
	while ( argc > 1 ) {
		-- argc;
		++ argv;
#ifdef __REPLXX_DEBUG__
		if ( !strcmp( *argv, "--keycodes" ) ) {
			replxx_debug_dump_print_codes();
			exit(0);
		}
#endif
		switch ( (*argv)[0] ) {
			case 'b': replxx_set_beep_on_ambiguous_completion( replxx, (*argv)[1] - '0' ); break;
			case 'c': replxx_set_completion_count_cutoff( replxx, atoi( (*argv) + 1 ) );   break;
			case 'e': replxx_set_complete_on_empty( replxx, (*argv)[1] - '0' );            break;
			case 'd': replxx_set_double_tab_completion( replxx, (*argv)[1] - '0' );        break;
			case 'h': replxx_set_max_hint_rows( replxx, atoi( (*argv) + 1 ) );             break;
			case 's': replxx_set_max_history_size( replxx, atoi( (*argv) + 1 ) );          break;
			case 'i': replxx_set_preload_buffer( replxx, recode( (*argv) + 1 ) );          break;
			case 'w': replxx_set_word_break_characters( replxx, (*argv) + 1 );             break;
			case 'm': replxx_set_no_color( replxx, (*argv)[1] - '0' );                     break;
			case 'p': prompt = recode( (*argv) + 1 );                                      break;
			case 'q': quiet = atoi( (*argv) + 1 );                                         break;
			case 'x': split( (*argv) + 1, examples, MAX_EXAMPLE_COUNT );                   break;
		}

	}

	const char* file = "./replxx_history.txt";

	replxx_history_load( replxx, file );
	replxx_set_completion_callback( replxx, completionHook, examples );
	replxx_set_highlighter_callback( replxx, colorHook, replxx );
	replxx_set_hint_callback( replxx, hintHook, examples );
	replxx_bind_key( replxx, '.', word_eater, replxx );

	printf("starting...\n");

	while (1) {
		char const* result = NULL;
		do {
			result = replxx_input( replxx, prompt );
		} while ( ( result == NULL ) && ( errno == EAGAIN ) );

		if (result == NULL) {
			printf("\n");
			break;
		} else if (!strncmp(result, "/history", 8)) {
			/* Display the current history. */
			int index = 0;
			int size = replxx_history_size( replxx );
			for ( ; index < size; ++index) {
				char const* hist = replxx_history_line( replxx, index );
				if (hist == NULL) {
					break;
				}
				replxx_print( replxx, "%4d: %s\n", index, hist );
			}
		}
		if (*result != '\0') {
			replxx_print( replxx, quiet ? "%s\n" : "thanks for the input: %s\n", result );
			replxx_history_add( replxx, result );
		}
	}
	replxx_history_save( replxx, file );
	printf( "Exiting Replxx\n" );
	replxx_end( replxx );
}


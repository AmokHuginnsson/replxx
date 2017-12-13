#include <cctype>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "replxx.h"

void completionHook(char const* prefix, int, replxx_completions* lc, void* ud) {
	char** examples( static_cast<char**>( ud ) );
	size_t i;
	for (i = 0;	examples[i] != NULL; ++i) {
		if (strncmp(prefix, examples[i], strlen(prefix)) == 0) {
			replxx_add_completion(lc, examples[i]);
		}
	}
}

void hintHook(char const* prefix, int, replxx_hints* lc, replxx_color::color*, void* ud) {
	char** examples( static_cast<char**>( ud ) );
	size_t i;
	size_t len( strlen( prefix ) );
	for (i = 0;	examples[i] != NULL; ++i) {
		if (strncmp(prefix, examples[i], strlen(prefix)) == 0) {
			replxx_add_hint(lc, examples[i] + len);
		}
	}
}

void colorHook( char const* str_, replxx_color::color* colors_, int size_, void* ) {
	for ( int i( 0 ); i < size_; ++ i ) {
		if ( isdigit( str_[i] ) ) {
			colors_[i] = replxx_color::BRIGHTMAGENTA;
		}
	}
}

int main (int argc, char** argv) {
	const char* examples[] = {
		"db", "hello", "hallo", "hans", "hansekogge", "seamann", "quetzalcoatl", "quit", "power", NULL
	};
	replxx_install_window_change_handler();

	while(argc > 1) {
		argc--;
		argv++;
		if (!strcmp(*argv, "--keycodes")) {
			replxx_debug_dump_print_codes();
			exit(0);
		}
	}

	const char* file = "./history";

	replxx_history_load(file);
	replxx_set_completion_callback(completionHook, examples);
	replxx_set_highlighter_callback(colorHook, nullptr);
	replxx_set_hint_callback(hintHook, examples);

	printf("starting...\n");

	char const* prompt = "\x1b[1;32mreplxx\x1b[0m> ";

	while (1) {
		char* result = replxx_input(prompt);

		if (result == NULL) {
			printf("\n");
			break;
		} else if (!strncmp(result, "/history", 8)) {
			/* Display the current history. */
			for (int index = 0; ; ++index) {
				char* hist = replxx_history_line(index);
				if (hist == NULL) break;
				printf("%4d: %s\n", index, hist);
				free(hist);
			 }
		}
		if (*result == '\0') {
			free(result);
			break;
		}

		printf( "thanks for the input: %s\n", result );
		replxx_history_add(result);
		free(result);
	}
	replxx_history_save(file);
	replxx_history_free();
}

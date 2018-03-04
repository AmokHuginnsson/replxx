#include <iostream>
#include <iomanip>
#include <cstring>
#include <cctype>

#include "replxx.hxx"

using namespace std;
using namespace replxx;

Replxx::completions_t completionHook( std::string const& prefix, int, void* ud ) {
	char** examples( static_cast<char**>( ud ) );
	Replxx::completions_t completions;
	for ( int i = 0; examples[i] != NULL; ++i) {
		if ( strncmp( prefix.c_str(), examples[i], prefix.length() ) == 0 ) {
			completions.push_back( examples[i] );
		}
	}
	return ( completions );
}

Replxx::hints_t hintHook( std::string const& prefix, int, Replxx::Color&, void* ud ) {
	char** examples( static_cast<char**>( ud ) );
	Replxx::hints_t hints;
	for ( int i = 0;	examples[i] != NULL; ++i) {
		if ( strncmp( prefix.c_str(), examples[i], prefix.length() ) == 0 ) {
			hints.push_back( examples[i] + prefix.length() );
		}
	}
	return ( hints );
}

void colorHook( std::string const& str_, Replxx::colors_t& colors_, void* ) {
	for ( int i( 0 ); i < static_cast<int>( str_.length() ); ++ i ) {
		if ( isdigit( str_[i] ) ) {
			colors_[i] = Replxx::Color::BRIGHTMAGENTA;
		}
	}
}

int main ( int, char** ) {
	const char* examples[] = {
		"db", "hello", "hallo", "hans", "hansekogge", "seamann", "quetzalcoatl", "quit", "power", NULL
	};
	Replxx replxx;
	replxx.install_window_change_handler();

	const char* file = "./history";

	replxx.history_load( file );
	replxx.set_completion_callback( completionHook, examples );
	replxx.set_highlighter_callback( colorHook, nullptr );
	replxx.set_hint_callback( hintHook, examples );

	printf("starting...\n");

	char const* prompt = "\x1b[1;32mreplxx\x1b[0m> ";

	while ( true ) {
		char const* result = replxx.input( prompt );

		if (result == NULL) {
			printf("\n");
			break;
		} else if (!strncmp(result, "/history", 8)) {
			/* Display the current history. */
			for (int index = 0; ; ++index) {
				cout << setw( 4 ) << index << ": " << replxx.history_line( index ) << endl;
			}
		}
		if (*result == '\0') {
			break;
		}

		cout << "thanks for the input: " << result << endl;
		replxx.history_add( result );
	}
	replxx.history_save( file );
}


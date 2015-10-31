
all:
	gcc -std=c90 -O0 -g -Wall -Wextra -c ConvertUTF.c -o ConvertUTF.o
	gcc -std=c90 -O0 -g -Wall -Wextra -c wcwidth.c -o wcwidth.o
	g++ -std=c++11 -O0 -g -Wall -Wextra -c linenoise.cpp -o linenoise.o
	gcc -std=c90 -O0 -g -Wall -Wextra -c example.c -o example.o
	g++ -g example.o wcwidth.o ConvertUTF.o linenoise.o -o example
 

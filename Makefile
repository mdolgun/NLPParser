GCC_OPTS = -std=c++17
parser: parser.o grammar.o common.o main.o
	g++ $(GCC_OPTS) -o parser parser.o grammar.o common.o main.o -lstdc++fs
common.o: stdafx.h util.h common.h common.cpp
	g++ $(GCC_OPTS) -c common.cpp
grammar.o: stdafx.h util.h common.h grammar.h grammar.cpp
	g++ $(GCC_OPTS) -c grammar.cpp
parser.o: stdafx.h util.h common.h grammar.h parser.h parser.cpp
	g++ $(GCC_OPTS) -c parser.cpp
main.o: stdafx.h util.h common.h grammar.h parser.h main.cpp
	g++ $(GCC_OPTS) -c main.cpp
clean: 
	rm -f parser.o grammar.o common.o main.o

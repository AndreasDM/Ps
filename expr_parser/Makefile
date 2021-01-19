all: main.o Incparser.o
	g++ -std=c++20 main.o Incparser.o

Incparser.o: Incparser.cpp
	g++ -std=c++20 -c Incparser.cpp

main.o: main.cpp
	g++ -std=c++20 -c main.cpp

run: all
	@./a.out

build: main.o incparser.o
	g++ -std=c++20 main.o incparser.o

main.o: main.cpp
	g++ -c main.cpp -std=c++20

incparser.o: incparser.cpp
	g++ -c incparser.cpp -std=c++20

run: build
	@./a.out

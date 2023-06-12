all:
	g++ -Wall -pedantic -std=c++11 ims.cpp -o ims -lsimlib -lm
run:
	./ims
CC=clang++
CFLAG=-std=c++0x -Wall -O3
LIBDIR=lib
LIB=-l./$(LIBDIR)/cappuccino.so
all: sample

sample:
	$(CC) $(CFLAG) sample.cpp -o sample

clean:
	rm sample

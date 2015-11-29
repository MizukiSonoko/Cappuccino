CC=clang++
CFLAG=-std=c++0x -Wall -Ofast
LIBDIR=lib
LIB=-l./$(LIBDIR)/cappuccino.so
all: sample

sample:
	$(CC) $(CFLAG) sample.cpp -o sample

clean:
	rm sample

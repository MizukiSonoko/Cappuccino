CC=clang++
CFLAG=-std=c++0x -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -O3 -lpthread
LIBDIR=lib
LIB=-l./$(LIBDIR)/cappuccino.so
all: sample

sample:
	$(CC) $(CFLAG) sample.cpp -o sample

clean:
	rm sample

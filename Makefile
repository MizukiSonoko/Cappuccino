all: chino

CC=g++
CFLAG := -g -std=c++1y $(LIB) -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -O3 -lpthread
INCLUDES := -Ilib/json/src


chino:
	$(CC) $(CFLAG) $(INCLUDES) chino.cpp -o chino

test:
	$(CC) $(CFLAG) $(INCLUDES) -DTEST chino.cpp -o chino
	./chino
	rm chino

clean:
	rm -f chino

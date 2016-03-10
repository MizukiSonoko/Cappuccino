CC=clang++
CFLAG=-std=c++0x $(LIB) -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -O3 -lpthread
all: chino

chino:
	$(CC) $(CFLAG) chino.cpp -o chino

test:
	$(CC) $(CFLAG) -DTEST chino.cpp -o chino
	./chino
	rm chino

remake:
	rm -f chino
	$(CC) $(CFLAG) -DTEST chino.cpp -o chino

clean:
	rm -f chino

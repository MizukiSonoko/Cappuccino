CC=clang++
CFLAG=-std=c++0x $(LIB) -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -O3 -lpthread
all: sample

sample:
	$(CC) $(CFLAG) sample.cpp -o sample

test:
	$(CC) $(CFLAG) -DTEST sample.cpp -o sample
	./sample
	rm sample

remake:
	rm -f sample	
	$(CC) $(CFLAG) -DTEST sample.cpp -o sample

clean:
	rm -f sample

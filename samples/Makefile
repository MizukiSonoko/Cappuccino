all: chino

CXX ?= g++
CFLAG := -g -std=c++14 $(LIB) -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -O3 -lpthread
INCLUDES := -I../ -I../lib/json/src

chino:
	$(CXX) $(CFLAG) $(INCLUDES) chino.cpp -o chino

test:
	$(CXX) $(CFLAG) $(INCLUDES) -DTEST chino.cpp -o chino
	./chino
	rm chino

clean:
	rm -f chino

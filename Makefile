CC=g++

all: cache-simulator

cache-simulator: main.o cache.o memory.o
	$(CC) -o $@ $^

main.o: cache.h

cache.o: cache.h def.h

memory.o: memory.h

.PHONY: clean

clean:
	rm -rf cache-simulator *.o

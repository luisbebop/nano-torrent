OBJECTS = net.o util.o bencode.o
CC = gcc

all: main.c $(OBJECTS)
	$(CC) -o $@ $^
	
clean:
	rm -f all $(OBJECTS)
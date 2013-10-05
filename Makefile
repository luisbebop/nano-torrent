OBJECTS = net.o util.o bencode.o bittorrent.o sha1.o
CC = gcc

all: main.c $(OBJECTS)
	$(CC) -o $@ $^
	
clean:
	rm -f all $(OBJECTS)
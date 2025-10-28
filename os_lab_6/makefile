CFLAGS = -Wall -Wextra -std=c99 -g
OBJECTS = common.o

all: client server

client: client.c $(OBJECTS)
	$(CC) $(CFLAGS) -o client client.c $(OBJECTS) $(LDFLAGS)

server: server.c $(OBJECTS)
	$(CC) $(CFLAGS) -o server server.c $(OBJECTS) $(LDFLAGS)

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

clean:
	rm -f client server *.o

.PHONY: all clean

CC = gcc
CFLAGS = -Wall
LDFLAGS = -lpthread

all: mutex_no mutex_yes factorial deadlock

mutex_no: mutex_no.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

mutex_yes: mutex_yes.c
	$(CC) $(CFLAGS) -DUSE_MUTEX -o $@ $< $(LDFLAGS)

factorial: factorial.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

deadlock: deadlock.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f mutex_no mutex_yes factorial deadlock

.PHONY: all clean

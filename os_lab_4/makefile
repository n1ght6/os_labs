CC = gcc
CFLAGS = -std=c99 -Wall -Wextra

all: parallel_min_max process_memory zombie_demo parallel_sum

parallel_min_max: parallel_min_max.c find_min_max.c utils.c
	$(CC) $(CFLAGS) -o $@ $^

process_memory: process_memory.c
	$(CC) $(CFLAGS) -o $@ $<

zombie_demo: zombie_demo.c
	$(CC) $(CFLAGS) -o $@ $<

parallel_sum: parallel_sum.c find_min_max.c utils.c sum.c
	$(CC) $(CFLAGS) -pthread -o $@ parallel_sum.c find_min_max.c utils.c sum.c

clean:
	rm -f parallel_min_max process_memory zombie_demo parallel_sum min_* max_*

.PHONY: all clean

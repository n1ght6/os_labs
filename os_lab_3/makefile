CC = gcc
CFLAGS = -std=c99 -Wall -Wextra

all: sequential_min_max parallel_min_max exec_sequential

sequential_min_max: sequential_min_max.c find_min_max.c utils.c
	$(CC) $(CFLAGS) -o $@ $^

parallel_min_max: parallel_min_max.c find_min_max.c utils.c
	$(CC) $(CFLAGS) -o $@ $^

exec_sequential: exec_sequential.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f sequential_min_max parallel_min_max exec_sequential min_* max_*

.PHONY: all clean

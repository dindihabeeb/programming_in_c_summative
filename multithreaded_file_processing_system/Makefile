CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread

processor: processor.c
	$(CC) $(CFLAGS) -o processor processor.c

clean:
	rm -f processor *.stats

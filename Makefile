CC = gcc
CFLAGS = -Wall -Wextra -pedantic -ggdb
LDFLAGS = -lraylib -lm

main: main.c
	$(CC) $(CFLAGS) $(LDFLAGS) main.c -o main

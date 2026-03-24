all: build

CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99

build:
	$(CC) $(CFLAGS) vine.c -o ./vine

install:
	$(CC) $(CFLAGS) vine.c -o /usr/local/bin/vine

all: build

build:
	clang vine.c -o ./vine

install:
	clang vine.c -o /usr/local/bin/vine

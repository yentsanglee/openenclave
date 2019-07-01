# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CFLAGS += -Wall -g -fPIC

VALGRIND_OPTS += --tool=memcheck
VALGRIND_OPTS += --trace-children=yes
VALGRIND_OPTS += --soname-synonyms=somalloc=NONE
VALGRIND_OPTS += --leak-check=full
VALGRIND_OPTS += --track-fds=yes
VALGRIND_OPTS += --show-reachable=yes
VALGRIND_OPTS += --show-possibly-lost=yes
VALGRIND_OPTS += --track-origins=yes
VALGRIND_OPTS += --malloc-fill=0xAA
VALGRIND_OPTS += --free-fill=0xDD

all: libc libf
	gcc $(CFLAGS) -c malloc.c
	gcc $(CFLAGS) -c main.c
	gcc -o main malloc.o main.o -L$(CURDIR) $(CURDIR)/libf.so
	ldd main
	main

libf:
	gcc $(CFLAGS) -c libf.c
	gcc -shared -o libf.so -nostdlib -static-libgcc libf.o

libc:
	gcc $(CFLAGS) -c libc.c
	ar rv libc.a libc.o

clean:
	rm -f libf.so libc.a libc.o libf.o main.o malloc.o main

vg:
	valgrind $(VALGRIND_OPTS) ./main

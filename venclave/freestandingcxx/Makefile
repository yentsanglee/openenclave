# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CFLAGS += -Wall -g -fPIC -nostdinc -fno-builtin

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

all: libc libcxx libf
	g++ -o main malloc.o main.o -L$(CURDIR) $(CURDIR)/libf.so
	ldd main
	main

libf:
	g++ $(CFLAGS) -c libf.c
	g++ -shared -o libf.so -nostdlib libf.o

libc:
	g++ $(CFLAGS) -c libc.c
	ar rv libc.a libc.o

libcxx:
	g++ $(CFLAGS) -c libcxx.cpp
	ar rv libstdc++.a libcxx.o

clean:
	rm -f libf.so libc.a libc.o libf.o main.o malloc.o main

vg:
	valgrind $(VALGRIND_OPTS) ./main

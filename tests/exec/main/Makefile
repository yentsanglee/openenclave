# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CC=/opt/musl/bin/musl-gcc

SOURCES = main.c

CFLAGS += -g
CFLAGS += -pie
CFLAGS += -fPIE
CFLAGS += -static
CFLAGS += -fno-builtin
#CFLAGS += -ftls-model=initial-exec
#CFLAGS += -fvisibility=hidden

all: hello
	$(CC) $(CFLAGS) main.c -omain $(LDFLAGS)

clean:
	rm -f main

run: all
	( cd ../; ctest -V )

clang:
	clang-7 -fPIC -static main.c -omain

gcc:
	gcc -fPIC -static main.c -omain

dump:
	objdump -d main > main.dump

.PHONY: hello

hello:
	$(CC) -fPIC -static hello.c -ohello -fno-builtin

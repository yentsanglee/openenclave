# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CC=/opt/musl/bin/musl-gcc

SOURCES = main.c

CFLAGS += -g
CFLAGS += -fPIC
CFLAGS += -pie
CFLAGS += -static

LDFLAGS += -Wl,--entry=_start_c

all:
	$(CC) $(CFLAGS) main.c -omain $(LDFLAGS)

clean:
	rm -f main

run: all
	( cd ../; ctest -V )

zzz:
	$(CC) -o main -g -fPIC -pie -static --verbose main.c

r:
	(cd ../; ctest -V )

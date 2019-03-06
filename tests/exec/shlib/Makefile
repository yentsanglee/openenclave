# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CC=/opt/musl/bin/musl-gcc
#CC=/usr/bin/gcc

SOURCES = main.c

all:
	$(CC) -m64 -static -fPIC -pie -Os main.c -shared -olibmain.so $(LDFLAGS)

clean:
	rm -f libmain.so

dump:
	objdump -d libmain.so > libmain.so.dump

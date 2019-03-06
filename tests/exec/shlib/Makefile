# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CC=/opt/musl/bin/musl-gcc

SOURCES = main.c

all:
	$(CC) -static -fPIC -pie -Os main.c -shared -olibmain.so $(LDFLAGS)

clean:
	rm -f libmain.so

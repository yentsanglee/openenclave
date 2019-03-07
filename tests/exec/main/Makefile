# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CC=/opt/musl/bin/musl-gcc

SOURCES = main.c

all:
	$(CC) -static -fPIC -pie -Os main.c -omain -Wl,--entry=_start

clean:
	rm -f main

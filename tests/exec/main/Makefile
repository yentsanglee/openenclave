# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CC=/opt/musl/bin/musl-gcc

SOURCES = main.c _start.c __libc_start_main.c

LDFLAGS = -Wl,-pie -Wl,--entry=_my_start

all:
	$(CC) -static -fPIC -pie -Os $(SOURCES) -omain $(LDFLAGS)

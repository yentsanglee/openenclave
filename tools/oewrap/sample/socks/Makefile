# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CC=oe-clang

all:
	$(CC) -c srv.c
	$(CC) -o srv srv.o
	$(CC) -c cli.c
	$(CC) -o cli cli.o

clean:
	rm -rf srv srv.enc cli cli.enc srv.o cli.o

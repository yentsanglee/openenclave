# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

all:
	shadow-gcc -o main.o -c main.c
	shadow-gcc -o main main.o

run:
	oerun main.enc

clean:
	rm -rf main main.enc main.o main.enc.o

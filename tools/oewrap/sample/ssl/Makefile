# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CC=oe-clang

INCLUDES=-I/root/openssl/include
LIBRARIES=-L/root/openssl -lcrypto.enc

all:
	$(CC) -g -Wall -Werror $(INCLUDES) -c main.c
	$(CC) -g -Wall -Werror $(INCLUDES) -c load_file.c
	$(CC) -g -o main main.o load_file.o $(LIBRARIES)

run:
	oerun .main.enc $(CURDIR)/cert.pem

clean:
	rm -f main main.o load_file.o .main.enc

build:
	$(CC) -g $(INCLUDES) -o main main.c load_file.c $(LIBRARIES)

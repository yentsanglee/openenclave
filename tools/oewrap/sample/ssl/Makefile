# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

INCLUDES=-I/root/openssl/include
LIBRARIES=-L/root/openssl -lcrypto.enc

all:
	oe-gcc -g -Wall -Werror $(INCLUDES) -c main.c
	oe-gcc -g -Wall -Werror $(INCLUDES) -c load_file.c
	oe-gcc -g -o main main.o load_file.o $(LIBRARIES)

reloc:
	readelf -r main | grep R_X86_64_GLOB_DAT

run:
	oerun main $(CURDIR)/cert.pem

clean:
	rm -f main main.o load_file.o

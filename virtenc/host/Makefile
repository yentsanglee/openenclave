# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CC = clang-7
#CC = gcc

ROOT = $(CURDIR)/../..

INCLUDES += -I$(ROOT)/include

CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -g
CFLAGS += -fPIE
CFLAGS += -m64
#CFLAGS += -fsanitize=address
#CFLAGS += -fno-omit-frame-pointer

SOURCES += main.c
SOURCES += call.c
SOURCES += sendfd.c
SOURCES += malloc.c
SOURCES += hostmalloc.c
SOURCES += io.c
SOURCES += enclave.c
SOURCES += heap.c
SOURCES += err.c

LDFLAGS += -lpthread

DEFINES += -DHOST

all:
	$(CC) -o main $(CFLAGS) $(DEFINES) $(INCLUDES) $(LDFLAGS) $(SOURCES)

clean:
	rm -rf main

run:
	@ ./main ../enclave/main

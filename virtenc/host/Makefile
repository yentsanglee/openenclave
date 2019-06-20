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

SOURCES += main.c
SOURCES += msg.c
SOURCES += sendfd.c
SOURCES += malloc.c
SOURCES += ../common/lock.c

DEFINES += -DHOST

all:
	$(CC) -o main $(CFLAGS) $(DEFINES) $(INCLUDES) $(LDFLAG) $(SOURCES)

clean:
	rm -rf main

run:
	@ ./main ../enclave/main

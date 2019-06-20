# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

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

DEFINES += -DHOST

all:
	gcc -o main $(CFLAGS) $(DEFINES) $(INCLUDES) $(LDFLAG) $(SOURCES)

clean:
	rm -rf main

run:
	@ ./main ../enclave/main

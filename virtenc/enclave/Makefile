# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

ROOT = $(CURDIR)/../..

CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -g
CFLAGS += -fPIE
CFLAGS += -m64
CFLAGS += -nostdinc
CFLAGS += -nostdlib
CFLAGS += -nodefaultlibs
CFLAGS += -nostartfiles
CFLAGS += -lgcc

INCLUDES += -I.
INCLUDES += -I$(ROOT)/include

LDFLAGS += =-Wl,-pie
LDFLAGS += =-Wl,-Bstatic

SOURCES += string.c
SOURCES += init.c
SOURCES += fini.c
SOURCES += msg.c
SOURCES += syscall.c
SOURCES += ioctl.c
SOURCES += recvfd.c
SOURCES += socket.c
SOURCES += close.c
SOURCES += sleep.c
SOURCES += nanosleep.c
SOURCES += globals.c
SOURCES += clone.S
SOURCES += sbrk.c
SOURCES += start.c
SOURCES += lock.c
SOURCES += alloc.c
SOURCES += put.c
SOURCES += getpid.c
SOURCES += gettid.c
#SOURCES += clone_sample.c

all:
	gcc -o main $(CFLAGS) $(INCLUDES) $(LDFLAG) $(SOURCES)

clean:
	rm -rf main


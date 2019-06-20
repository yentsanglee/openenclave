# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

#STATIC_LINKAGE=1

ROOT = $(CURDIR)/../..

CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -g
CFLAGS += -O0
CFLAGS += -fPIE
CFLAGS += -m64
CFLAGS += -nostdinc
CFLAGS += -ffreestanding
CFLAGS += -nostartfiles
CFLAGS += -lgcc
ifdef STATIC_LINKAGE
CFLAGS += -nostdlib
CFLAGS += -nodefaultlibs
endif

INCLUDES += -I.
INCLUDES += -I$(ROOT)/include

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
SOURCES += put.c
SOURCES += getpid.c
SOURCES += gettid.c
SOURCES += malloc.c
SOURCES += abort.c
ifdef STATIC_LINKAGE
LDFLAGS += =-Wl,-pie
LDFLAGS += =-Wl,-Bstatic
LDFLAGS += =-Wl,-e_start
endif

all:
	gcc -o main $(CFLAGS) $(INCLUDES) $(LDFLAG) $(SOURCES)

clean:
	rm -rf main

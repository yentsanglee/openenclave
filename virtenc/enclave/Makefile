# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

ROOT = $(CURDIR)/../..

CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -g
CFLAGS += -O0
CFLAGS += -fPIE
CFLAGS += -m64
CFLAGS += -nostdinc
CFLAGS += -lgcc
CFLAGS += -ffreestanding

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
SOURCES += put.c
SOURCES += getpid.c
SOURCES += gettid.c
SOURCES += malloc.c
SOURCES += abort.c
SOURCES += read.c
SOURCES += write.c
SOURCES += ../common/lock.c

DYNAMIC_LDFLAGS += -nostartfiles

STATIC_LDFLAGS += $(LDFLAGS)
STATIC_LDFLAGS += -Wl,-pie
STATIC_LDFLAGS += -Wl,-Bstatic
STATIC_LDFLAGS += -Wl,-e_start
STATIC_LDFLAGS += -nostdlib
STATIC_LDFLAGS += -nodefaultlibs

all:
	gcc -o main.static $(CFLAGS) $(INCLUDES) $(STATIC_LDFLAGS) $(SOURCES)
	gcc -o main $(CFLAGS) $(INCLUDES) $(DYNAMIC_LDFLAGS) $(SOURCES)

clean:
	rm -rf main

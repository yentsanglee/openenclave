# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

CC = clang-7
#CC = gcc

#SANITIZE=1

ROOT = $(CURDIR)/../..

CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -g
#CFLAGS += -O1
CFLAGS += -O3
CFLAGS += -fPIE
CFLAGS += -m64
CFLAGS += -nostdinc
#CFLAGS += -fno-builtin
#CFLAGS += -lgcc
#CFLAGS += -ffreestanding

DYNAMIC_CFLAGS += $(CFLAGS)

ifdef SANITIZE
DYNAMIC_CFLAGS += -fsanitize=address
DYNAMIC_CFLAGS += -fno-omit-frame-pointer
DYNAMIC_CFLAGS += -fsanitize=leak
endif

INCLUDES += -I.
INCLUDES += -I$(ROOT)/include

SOURCES += string.c
SOURCES += init.c
SOURCES += fini.c
SOURCES += msg.c
SOURCES += syscall.c
SOURCES += recvfd.c
SOURCES += socket.c
SOURCES += sleep.c
SOURCES += nanosleep.c
SOURCES += globals.c
SOURCES += clone.S
SOURCES += sbrk.c
SOURCES += start.c
SOURCES += print.c
SOURCES += malloc.c
SOURCES += shm.c
SOURCES += lock.c
SOURCES += process.c
SOURCES += io.c
SOURCES += ../common/msg.c

#DYNAMIC_LDFLAGS += -nostartfiles

STATIC_LDFLAGS += $(LDFLAGS)
STATIC_LDFLAGS += -Wl,-pie
STATIC_LDFLAGS += -Wl,-Bstatic
STATIC_LDFLAGS += -Wl,-e_start
STATIC_LDFLAGS += -nostdlib
STATIC_LDFLAGS += -nodefaultlibs

all: static dynamic

static:
	$(CC) -DBUILD_STATIC -o main.static $(CFLAGS) $(INCLUDES) $(STATIC_LDFLAGS) $(SOURCES)

dynamic:
	$(CC) -o main $(DYNAMIC_CFLAGS) $(INCLUDES) $(DYNAMIC_LDFLAGS) $(SOURCES)

xxx:
	$(CC) -c $(DYNAMIC_CFLAGS) $(INCLUDES) $(DYNAMIC_LDFLAGS) -S msg.c

clean:
	rm -rf main

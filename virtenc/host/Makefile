# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

#CC = clang-7
CC = gcc

ROOT = $(CURDIR)/../..

INCLUDES += -I$(ROOT)/include

SANITIZE=1

CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -g
CFLAGS += -fPIE
CFLAGS += -m64
CFLAGS += -I.

ifdef SANITIZE
DYNAMIC_CFLAGS += -fsanitize=address
DYNAMIC_CFLAGS += -fno-omit-frame-pointer
DYNAMIC_CFLAGS += -fsanitize=leak
endif

SOURCES += main.c
SOURCES += call.c
SOURCES += sendfd.c
SOURCES += malloc.c
SOURCES += hostmalloc.c
SOURCES += io.c
SOURCES += enclave.c
SOURCES += heap.c
SOURCES += err.c
SOURCES += strings.c
SOURCES += result.c
SOURCES += safecrt.c
SOURCES += elf.c
SOURCES += host.c
SOURCES += rand.c
SOURCES += test_u_wrap.c
SOURCES += log.c

LDFLAGS += -lpthread

DEFINES += -DHOST

all: test_u.c
	$(CC) -o main $(CFLAGS) $(DEFINES) $(INCLUDES) $(SOURCES) $(LDFLAGS)

test_u.c: ../common/test.edl
	oeedger8r --untrusted ../common/test.edl

CLEAN = test_u.c test_u.h test_args.h

clean:
	rm -rf main $(CLEAN)

run:
	@ ./main ../enclave/main

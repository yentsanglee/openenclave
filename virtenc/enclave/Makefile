# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

#USE_CLANG=1

ifdef USE_CLANG
CC = clang-7
else
CC = gcc
endif

#SANITIZE=1

ROOT = $(CURDIR)/../..

CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -g
CFLAGS += -O1
#CFLAGS += -O3
#CFLAGS += -fPIE
CFLAGS += -fPIC
CFLAGS += -m64
CFLAGS += -nostdinc
CFLAGS += -lgcc
#CFLAGS += -ffreestanding
CFLAGS += -mxsave
CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -Wpointer-arith
CFLAGS += -Wconversion
CFLAGS += -Wextra
CFLAGS += -Wno-missing-field-initializers
CFLAGS += -fno-strict-aliasing
CFLAGS += -fno-stack-protector
#CFLAGS += -ftls-model=local-exec
CFLAGS += -fno-builtin-malloc
CFLAGS += -fno-builtin-calloc
CFLAGS += -fno-builtin
CFLAGS += -DBUILD_ENCLAVE

ifdef USE_CLANG
CFLAGS += -mllvm
CFLAGS += -x86-speculative-load-hardening
endif

CFLAGS += -I.
CFLAGS += -Iinclude
CFLAGS += -I$(ROOT)/include

DYNAMIC_CFLAGS += $(CFLAGS)

ifdef SANITIZE
DYNAMIC_CFLAGS += -fsanitize=address
DYNAMIC_CFLAGS += -fno-omit-frame-pointer
DYNAMIC_CFLAGS += -fsanitize=leak
endif

SOURCES += assert.c
SOURCES += string.c
SOURCES += init.c
SOURCES += fini.c
SOURCES += syscall.c
SOURCES += recvfd.c
SOURCES += socket.c
SOURCES += sleep.c
SOURCES += nanosleep.c
SOURCES += globals.c
SOURCES += clone.S
SOURCES += sbrk.c
SOURCES += main.c
SOURCES += print.c
SOURCES += malloc.c
SOURCES += shm.c
SOURCES += process.c
SOURCES += io.c
SOURCES += call.c
SOURCES += lock.c
SOURCES += settings.c
SOURCES += hexdump.c
SOURCES += register.c
SOURCES += thread.c
SOURCES += test_t.c
SOURCES += enclave.c
SOURCES += test.c
SOURCES += safecrt.c
SOURCES += -DOE_BUILD_ENCLAVE
SOURCES += log.c
SOURCES += result.c
SOURCES += memmove.c

#DYNAMIC_LDFLAGS += -nostartfiles

STATIC_LDFLAGS += $(LDFLAGS)
STATIC_LDFLAGS += -Wl,-pie
STATIC_LDFLAGS += -Wl,-Bstatic
STATIC_LDFLAGS += -Wl,-e_start
STATIC_LDFLAGS += -nostdlib
STATIC_LDFLAGS += -nodefaultlibs

all: test_t.c static dynamic

test_t.c: ../common/test.edl
	oeedger8r --trusted ../common/test.edl

static:
	$(CC) -DBUILD_STATIC -o main.static $(CFLAGS) $(STATIC_LDFLAGS) $(SOURCES)

dynamic:
	$(CC) -o main $(DYNAMIC_CFLAGS) $(DYNAMIC_LDFLAGS) $(SOURCES)

CLEAN = test_t.c test_t.h test_args.h

clean:
	rm -rf main $(CLEAN)

include ../config.mak

LDFLAGS += -nodefaultlibs
LDFLAGS += -nostdlib
LDFLAGS += -nostartfiles
LDFLAGS += -Wl,-whole-archive
LDFLAGS += -Wl,--warn-once
LDFLAGS += ../openssl/openssl-1.1.1a/libcrypto.a

CFLAGS += -Werror
CFLAGS += -Wall
CFLAGS += -nostdinc
CFLAGS += --freestanding
CFLAGS += -fno-builtin-function
CFLAGS += -fno-builtin

INCLUDES += -I../openssl/include
INCLUDES += -I../openssl/openssl-1.1.1a/include

SOURCES += $(wildcard *.c)

app:
	$(CC) $(INCLUDES) $(CFLAGS) $(INCLUDES) -o main $(SOURCES) $(LDFLAGS)

clean:
	rm -rf main

undef:
	@ ( $(MAKE) 2>&1 | grep "undefined reference" | sed "s/.*\`//g" | sed "s/'.*//g" | sort | uniq )

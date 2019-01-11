include ../config.mak

#LDFLAGS += -nodefaultlibs
LDFLAGS += -nostdlib
#LDFLAGS += -nostartfiles
#LDFLAGS += $(PREFIX)/lib/libcrypto.a
LDFLAGS += -Wl,-whole-archive
LDFLAGS += ../openssl/openssl-1.1.1a/libcrypto.a

CFLAGS = -nostdinc -Werror -Wall -Wl,--warn-once

INCLUDES += -I../openssl/include
INCLUDES += -I../openssl/openssl-1.1.1a/include

SOURCES += $(wildcard *.c)

app:
	$(CC) $(INCLUDES) $(CFLAGS) $(INCLUDES) -o main $(SOURCES) $(LDFLAGS)

clean:
	rm -rf main

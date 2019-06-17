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

LDFLAGS += =-Wl,-pie
LDFLAGS += =-Wl,-Bstatic

SOURCES += start.c
SOURCES += strings.c
SOURCES += init.c
SOURCES += fini.c

all:
	gcc -o main $(CFLAGS) $(INCLUDES) $(LDFLAG) $(SOURCES)

clean:
	rm -rf main

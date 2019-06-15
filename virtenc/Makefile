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

all:
	gcc -o main $(CFLAGS) $(INCLUDES) $(LDFLAG) start.c strings.c

clean:
	rm -rf main

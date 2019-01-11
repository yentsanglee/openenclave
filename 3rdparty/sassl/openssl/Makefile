##==============================================================================
##
## Cross-compiler:
##
##==============================================================================

include ../config.mak

CC+=-I$(CURDIR)/include
CC+=-nostdinc
CC+=-DOPENSSL_IMPLEMENTS_strncasecmp
CC+=-DOPENSSL_NO_SOCK
CC+=-DNO_SYSLOG
CC+=-DOPENSSL_NO_DEPRECATED
CC+=-DOPENSSL_NO_DGRAM
CC+=-DOPENSSL_NO_UI_CONSOLE
CC+=-DOPENSSL_NO_SOCK
CC+=-DOPENSSL_NO_HW
CC+=-Werror
CC+=$(OPENSSL_FLAGS)
CC+=$(OPENSSL_EXFLAGS)

OPENSSL=openssl-1.1.1a

URL=https://www.openssl.org/source/$(OPENSSL).tar.gz

PREFIX=/opt/openssl

##==============================================================================
##
## all:
##
##==============================================================================

all: download configure build

##==============================================================================
##
## download:
##
##==============================================================================

download: $(OPENSSL)

$(OPENSSL):
	rm -rf $(OPENSSL)
	rm -f $(OPENSSL).tar.gz
	wget $(URL)
	tar zxf $(OPENSSL).tar.gz
	rm -f $(OPENSSL).tar.gz

##==============================================================================
##
## configure:
##
##==============================================================================

configure: $(OPENSSL)/Makefile

$(OPENSSL)/Makefile:
	( cd $(OPENSSL); ./Configure $(TARGET) --prefix=$(PREFIX) --openssldir=$(PREFIX) )

##==============================================================================
##
## build:
##
##==============================================================================

build:
	( cd $(OPENSSL); $(MAKE) CC="$(CC)" build_generated libcrypto.a )

##==============================================================================
##
## clean:
##
##==============================================================================

clean:
	( cd $(OPENSSL); $(MAKE) clean )

##==============================================================================
##
## install:
##
##==============================================================================

install:
	( cd $(OPENSSL); $(MAKE) install )

##==============================================================================
##
## distclean:
##
##==============================================================================

distclean:
	rm -rf $(OPENSSL)

##==============================================================================
##
## app:
##
##==============================================================================

INCLUDES = -I$(PREFIX)/include

LDFLAGS += -L$(PREFIX)/lib
LDFLAGS += -Wl,-whole-archive
LDFLAGS += -nodefaultlibs

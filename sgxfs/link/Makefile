
LIBS += ../../build/output/lib/openenclave/enclave/libsgxfs.a
LIBS += ../../build/output/lib/openenclave/enclave/liboelibcxx.a
LIBS += ../../build/output/lib/openenclave/enclave/liboeenclave.a
LIBS += ../../build/output/lib/openenclave/enclave/libmbedx509.a
LIBS += ../../build/output/lib/openenclave/enclave/libmbedcrypto.a
LIBS += ../../build/output/lib/openenclave/enclave/liboelibc.a
LIBS += ../../build/output/lib/openenclave/enclave/liboecore.a

LDFLAGS = -nostdlib -nodefaultlibs -nostartfiles -Wl,--no-undefined -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--export-dynamic -Wl,--warn-once

INCLUDES = -I../../../openenclave/include

all:
	@ gcc $(INCLUDES) -o main stubs.c -Wl,--whole-archive $(LDFLAGS) $(LIBS)

clean:
	rm -f main

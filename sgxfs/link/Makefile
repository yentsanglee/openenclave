
LIBS += ../../build/output/lib/openenclave/enclave/libsgxfs.a
LIBS += ../../build/output/lib/openenclave/enclave/liboelibcxx.a
LIBS += ../../build/output/lib/openenclave/enclave/liboelibc.a
LIBS += ../../build/output/lib/openenclave/enclave/liboecore.a


LDFLAGS = -nostdlib -nodefaultlibs -nostartfiles -Wl,--no-undefined -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--export-dynamic

all:
	gcc -o main main.c -Wl,--whole-archive $(LDFLAGS) $(LIBS)

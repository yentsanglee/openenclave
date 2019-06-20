# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

VALGRIND_OPTS += --tool=memcheck
VALGRIND_OPTS += --trace-children=yes
VALGRIND_OPTS += --soname-synonyms=somalloc=NONE
VALGRIND_OPTS += --leak-check=full
#VALGRIND_OPTS += --verbose

all:
	$(MAKE) -C enclave
	$(MAKE) -C host

clean:
	$(MAKE) -C enclave clean
	$(MAKE) -C host clean

run:
	./host/main ./enclave/main

vg:
	valgrind $(VALGRIND_OPTS) ./host/main ./enclave/main

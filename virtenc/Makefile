# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

VALGRIND_OPTS += --tool=memcheck
VALGRIND_OPTS += --trace-children=yes
VALGRIND_OPTS += --soname-synonyms=somalloc=NONE
VALGRIND_OPTS += --leak-check=full
VALGRIND_OPTS += --track-fds=yes
VALGRIND_OPTS += --show-reachable=yes
VALGRIND_OPTS += --show-possibly-lost=yes
VALGRIND_OPTS += --track-origins=yes
VALGRIND_OPTS += --malloc-fill=0xAA
VALGRIND_OPTS += --free-fill=0xDD
#VALGRIND_OPTS += --verbose

export ASAN_OPTIONS=detect_leaks=1

all:
	$(MAKE) -C enclave
	$(MAKE) -C host

clean:
	$(MAKE) -C enclave clean
	$(MAKE) -C host clean

run:
	@ echo "********************************************"
	./host/main ./enclave/main.static
	@ echo "********************************************"
	./host/main ./enclave/main
	@ echo "********************************************"

vg:
	valgrind $(VALGRIND_OPTS) ./host/main ./enclave/main

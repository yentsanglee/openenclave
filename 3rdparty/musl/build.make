# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

all: configure build install

configure:
	( cd musl; ./configure CFLAGS=-fPIC --prefix=/opt/musl --disable-shared )

build:
	( cd musl; $(MAKE) )

install:
	( cd musl; $(MAKE) install )

#
# Copyright (C) 2011-2017 Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#   * Neither the name of Intel Corporation nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
include ../buildenv.mk

######## SGX SDK Settings ########
SGX_MODE ?= HW
SGX_ARCH ?= x64

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	$(error x86 build is not supported, only x64!!)
else
	OE_COMMON_CFLAGS := -m64 -Wall
	ifeq ($(LINUX_SGX_BUILD), 1)
		include ../../../../../buildenv.mk
		OE_ER8R := $(BUILD_DIR)/oeedger8r
		OE_SDK_INC := $(COMMON_DIR)/inc
	else
		OE_ER8R := $(OE_SDK)/bin/x64/oeedger8r
		OE_SDK_INC := $(OE_SDK)/include
	endif
endif

ifeq ($(DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

ifeq ($(DEBUG), 1)
	OE_COMMON_CFLAGS += -O0 -g
else
	OE_COMMON_CFLAGS += -O2 -D_FORTIFY_SOURCE=2
endif

OE_EDL_FILE := $(PACKAGE_INCLUDE)/oe_tssl.edl

######## App Settings ########

Oe_ussl_Include_Paths := -I. -I$(OE_SDK_INC)

Oe_ussl_C_Flags := $(OE_COMMON_CFLAGS) -fpie -fpic -fstack-protector -Wformat -Wformat-security -Wno-attributes $(Oe_ussl_Include_Paths)
Oe_ussl_Cpp_Flags := $(Oe_ussl_C_Flags) -std=c++11

Oe_ussl_Cpp_Files := $(wildcard *.cpp)
Oe_ussl_C_Files := $(wildcard *.c)

Oe_ussl_Cpp_Objects := $(addprefix $(OBJDIR)/, $(Oe_ussl_Cpp_Files:.cpp=.o))
Oe_ussl_C_Objects := $(addprefix $(OBJDIR)/, $(Oe_ussl_C_Files:.c=.o))
$(shell mkdir -p $(OBJDIR))


.PHONY: all run

all: $(UNTRUSTED_LIB) 

######## App Objects ########

# Lines below are NOT needed because currently the EDL file doesn't contain ECALL function.
#$(UNTRUSTED_DIR)/oe_tssl_u.c: $(ER8R) $(OE_EDL_FILE)
#	@mkdir -p $(UNTRUSTED_DIR) && cd $(UNTRUSTED_DIR) && $(ER8R) --header-only --untrusted $(OE_EDL_FILE) --search-path $(SGX_SDK_INC)
#	@echo "GEN  =>  $@"

oe_tssl_u.o: $(UNTRUSTED_DIR)/oe_tssl_u.c
	$(VCC) $(Oe_ussl_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

$(OBJDIR)/%.o : %.cpp
	$(VCC) $(Oe_ussl_Cpp_Flags) -c $< -o $@
	@echo "CC  <=  $<"

$(OBJDIR)/%.o : %.c
	$(VCC) $(Oe_ussl_C_Flags) -c $< -o $@
	@echo "CC  <=  $<"

$(UNTRUSTED_LIB): $(Oe_ussl_Cpp_Objects) $(Oe_ussl_C_Objects)
	ar rcs $(UNTRUSTED_LIB) $(Oe_ussl_Cpp_Objects) $(Oe_ussl_C_Objects)  
	@echo "LINK =>  $@"
	@cp $@ $(PACKAGE_LIB)/$(UNTRUSTED_LIB)

.PHONY: clean

clean:
	@rm -f liboe_ussl.* liboe_ussld.* oe_tssl_u.* $(Oe_ussl_Cpp_Objects) $(Oe_ussl_C_Objects)
	

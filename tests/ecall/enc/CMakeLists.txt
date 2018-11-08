# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(add_enclave_executable)

add_library(ecall_enc SHARED enc.cpp stubs.c)
set_property(TARGET ecall_enc APPEND PROPERTY LINK_FLAGS "-NODEFAULTLIB -entry:oe_entry -INCREMENTAL:NO")
target_link_libraries(ecall_enc oecore)

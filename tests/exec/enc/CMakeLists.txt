# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.


oeedl_file(../exec.edl enclave gen)

add_enclave(TARGET exec_enc SOURCES enc.c elf.c put.c ${gen})

target_include_directories(exec_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(exec_enc oelibc)

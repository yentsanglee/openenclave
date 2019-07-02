# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../vsample1.edl enclave gen)

add_executable(vsample1_enc enc.c ${gen})

target_include_directories(vsample1_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(vsample1_enc PRIVATE
    ${CMAKE_BINARY_DIR}/output/include/openenclave/libc)

target_link_libraries(vsample1_enc
    oevlibc
    -Wl,--whole-archive
    oevcore
    -Wl,--no-whole-archive
    oelibc_includes)

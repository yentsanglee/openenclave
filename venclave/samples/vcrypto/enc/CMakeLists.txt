# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../vcrypto.edl enclave gen)

add_executable(vcrypto_enc enc.c ${gen})

target_include_directories(vcrypto_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(vcrypto_enc
    -Wl,--warn-once
    oeenclave
    oecryptombed
    mbedx509
    mbedcrypto
    oevhostfs
    oevlibc
    oevcore
    oelibc_includes)

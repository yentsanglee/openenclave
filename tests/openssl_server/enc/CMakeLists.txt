# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../openssl_server.edl enclave gen)

add_enclave(TARGET openssl_serverenc
    SOURCES
    enc.c
    ${gen})

target_include_directories(openssl_serverenc PRIVATE -I/home/johnkord/openenclave/enclave/include ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(openssl_serverenc oelibc -L/home/johnkord/intel-sgx-ssl/Linux/package/lib/ -L/home/johnkord/intel-sgx-ssl/Linux/openenclave/liboe_tssl -loe_tssl -loe_tssl_crypto)

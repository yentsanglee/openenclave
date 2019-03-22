# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../openssl_server.edl enclave gen)

add_enclave(TARGET openssl_serverenc
    SOURCES
    enc.c
    ${gen})

add_dependencies(openssl_serverenc oe_openssl)
target_include_directories(openssl_serverenc PRIVATE ${CMAKE_CURRENT_BINARY_DIR};${CMAKE_BINARY_DIR}/3rdparty/openssl_install/include)
target_link_libraries(openssl_serverenc oelibc ${CMAKE_BINARY_DIR}/3rdparty/openssl_install/lib/libssl.a ${CMAKE_BINARY_DIR}/3rdparty/openssl_install/lib/libcrypto.a oelibc)

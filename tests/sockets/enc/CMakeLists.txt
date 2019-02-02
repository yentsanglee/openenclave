# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../socket_test.edl 
    enclave socket_test_t)

add_enclave(
    TARGET socket_enc
    SOURCES enc.c socket_enclave.c netdb_enclave.c arpa_inet_enclave.c ${socket_test_t})
  
target_include_directories(socket_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR}  ${CMAKE_CURRENT_SOURCE_DIR}/..)

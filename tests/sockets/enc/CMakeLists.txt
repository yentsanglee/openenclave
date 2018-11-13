# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)
include(add_enclave_executable)

oeedl_file(../socket_test.edl 
    enclave socket_test_t
    --edl-search-dir ../../../include
)

add_executable(socket_enc
    enc.c
    ${socket_test_t}
)

target_link_libraries(socket_enc oesocket_enc)
target_include_directories(socket_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(socket_enc oeenclave)

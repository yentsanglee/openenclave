# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)
add_subdirectory(enc)

add_test(TEST vsample
    COMMAND
    ${CMAKE_CURRENT_BINARY_DIR}/host/sample_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/sample_enclave)

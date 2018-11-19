# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)
add_subdirectory(enc)

add_enclave_test(tests/fs 
    ./host fs_host 
    ./enc fs_enc
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}")


# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)
add_subdirectory(enc)

add_test(tests/fs1 /bin/rm -rf "${CMAKE_BINARY_DIR}/tests/fs/tmp")

add_enclave_test(tests/fs2
    ./host fs_host 
    ./enc fs_enc
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}")


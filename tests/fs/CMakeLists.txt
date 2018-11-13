# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)

if (UNIX)
	add_subdirectory(enc)
endif()

add_test(
    tests/fs 
    ./host/fs_host 
    ./enc/fs_enc 
    "${PROJECT_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}")

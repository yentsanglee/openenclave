# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)

if (BUILD_ENCLAVES)
	add_subdirectory(enc)
endif()

set(PROGRAM ${CMAKE_CURRENT_SOURCE_DIR}/shlib/libmain.so)

add_enclave_test(tests/exec exec_host exec_enc ${PROGRAM})

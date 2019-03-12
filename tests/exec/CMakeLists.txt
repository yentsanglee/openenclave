# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)

if (BUILD_ENCLAVES)
	add_subdirectory(enc)
endif()

#set(PROGRAM ${CMAKE_CURRENT_SOURCE_DIR}/shlib/libmain.so)

set(PROGRAM ${CMAKE_CURRENT_SOURCE_DIR}/main/main)

add_test(tests/exec1 make -C ${CMAKE_CURRENT_SOURCE_DIR}/main)

add_enclave_test(tests/exec2 exec_host exec_enc ${PROGRAM})

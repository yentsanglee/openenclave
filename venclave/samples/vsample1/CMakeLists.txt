# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)
add_subdirectory(enc)

set(TMPDIR "${CMAKE_CURRENT_BINARY_DIR}")

add_test(vsample1
    ${CMAKE_CURRENT_BINARY_DIR}/host/vsample1_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/vsample1_enc
    ${TMPDIR})

include(../valgrind.cmake)

add_test(vsample1-valgrind valgrind ${VALGRIND_OPTS}
    ${CMAKE_CURRENT_BINARY_DIR}/host/vsample1_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/vsample1_enc
    ${TMPDIR})

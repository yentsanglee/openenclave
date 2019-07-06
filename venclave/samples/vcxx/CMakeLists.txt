# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)
add_subdirectory(enc)

set(TMPDIR "${CMAKE_CURRENT_BINARY_DIR}")

add_test(vcxx
    ${CMAKE_CURRENT_BINARY_DIR}/host/vcxx_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/vcxx_enc
    ${TMPDIR})

include(../valgrind.cmake)

add_test(vcxx-valgrind valgrind ${VALGRIND_OPTS}
    ${CMAKE_CURRENT_BINARY_DIR}/host/vcxx_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/vcxx_enc
    ${TMPDIR})

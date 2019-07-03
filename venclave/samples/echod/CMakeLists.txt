# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)

if (BUILD_ENCLAVES)
    add_subdirectory(enc)
endif()

add_test(tests/echod1
    ${CMAKE_CURRENT_BINARY_DIR}/host/echod_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/echod_enc)

set(VALGRIND_OPTS
    --tool=memcheck
    --error-exitcode=99
    --leak-check=full
    --show-leak-kinds=definite,possible
    --errors-for-leak-kinds=definite,possible
    --trace-children=yes
    --soname-synonyms=somalloc=NONE
    --track-fds=yes
    --show-reachable=yes
    --show-possibly-lost=yes
    --track-origins=yes
    --malloc-fill=0xAA
    --free-fill=0xDD)

add_test(tests/echod2 valgrind ${VALGRIND_OPTS}
    ${CMAKE_CURRENT_BINARY_DIR}/host/echod_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/echod_enc)

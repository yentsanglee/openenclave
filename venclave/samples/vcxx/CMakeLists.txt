# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)
add_subdirectory(enc)

set(TMPDIR "${CMAKE_CURRENT_BINARY_DIR}")

add_test(vcxx
    ${CMAKE_CURRENT_BINARY_DIR}/host/vcxx_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/vcxx_enc
    ${TMPDIR})

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

add_test(vcxx-valgrind valgrind ${VALGRIND_OPTS}
    ${CMAKE_CURRENT_BINARY_DIR}/host/vcxx_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/vcxx_enc
    ${TMPDIR})

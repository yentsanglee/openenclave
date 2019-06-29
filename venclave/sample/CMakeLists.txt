# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)
add_subdirectory(enc)

add_test(vsample
    ${CMAKE_CURRENT_BINARY_DIR}/host/sample_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/sample_enc)

set(VALGRIND_OPTS
    --tool=memcheck
    --leak-check=full
    #--show-leak-kinds=default
    #--errors-for-leak-kinds=default
    --error-exitcode=1
    --trace-children=yes
    --soname-synonyms=somalloc=NONE
    --track-fds=yes
    --show-reachable=yes
    --show-possibly-lost=yes
    --track-origins=yes
    --malloc-fill=0xAA
    --free-fill=0xDD)


add_test(vsample-valgrind valgrind ${VALGRIND_OPTS}
    ${CMAKE_CURRENT_BINARY_DIR}/host/sample_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/sample_enc)

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)
add_subdirectory(enc)

set(TMPDIR "${CMAKE_CURRENT_BINARY_DIR}")

add_test(vcrypto
    ${CMAKE_CURRENT_BINARY_DIR}/host/vcrypto_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/vcrypto_enc
    ${TMPDIR})

include(../valgrind.cmake)

#add_test(vcrypto-valgrind valgrind ${VALGRIND_OPTS}
#    ${CMAKE_CURRENT_BINARY_DIR}/host/vcrypto_host
#    ${CMAKE_CURRENT_BINARY_DIR}/enc/vcrypto_enc
#    ${TMPDIR})

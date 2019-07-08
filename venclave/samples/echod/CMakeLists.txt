# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(host)

if (BUILD_ENCLAVES)
    add_subdirectory(enc)
endif()

add_test(echod
    ${CMAKE_CURRENT_BINARY_DIR}/host/echod_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/echod_enc)

include(../valgrind.cmake)

add_test(echod-valgrind valgrind ${VALGRIND_OPTS}
    ${CMAKE_CURRENT_BINARY_DIR}/host/echod_host
    ${CMAKE_CURRENT_BINARY_DIR}/enc/echod_enc)

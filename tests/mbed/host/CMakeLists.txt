# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.


oeedl_file(../mbed.edl host gen)

add_executable(libmbedtest_host host.c ocalls.c ${gen})

#target_include_directories(libmbedtest_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/musl/musl/include)

target_include_directories(libmbedtest_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(libmbedtest_host oehostapp)

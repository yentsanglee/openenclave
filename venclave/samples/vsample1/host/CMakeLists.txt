# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../vsample1.edl host gen)

add_executable(vsample1_host host.c ${gen})

target_include_directories(vsample1_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(vsample1_host oevhost pthread)

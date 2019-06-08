# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../wrap.edl host gen)

add_library(oewraphost wrap.c ${gen})

target_include_directories(oewraphost
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oewraphost
    PRIVATE oe_includes ${CMAKE_CURRENT_BINARY_DIR})

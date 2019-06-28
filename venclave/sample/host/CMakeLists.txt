# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../sample.edl host gen)

add_executable(sample_host host.c sample_u_wrap.c)

target_include_directories(
    sample_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(sample_host oevhost pthread)

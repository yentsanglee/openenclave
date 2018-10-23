# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)

oeedl_file(../ext2.edl host gen)

add_executable(ext2_host host.c ${gen})

target_include_directories(ext2_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(ext2_host oehostapp)

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)

oeedl_file(../protectedfs.edl host gen)

add_executable(protectedfs_host 
    host.c 
    ${gen})

target_include_directories(protectedfs_host 
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(protectedfs_host pfshost oehostapp)

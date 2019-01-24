# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.


oeedl_file(../hostfs.edl enclave gen)

#add_enclave(TARGET hostfs_enc SOURCES enc.c ${gen})

add_executable(hostfs_enc enc.c ${gen})

target_include_directories(hostfs_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(hostfs_enc oehostfsenclave oeenclave)

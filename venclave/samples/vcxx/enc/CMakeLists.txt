# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../vcxx.edl enclave gen)

add_executable(vcxx_enc enc.cpp ${gen})

target_include_directories(vcxx_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(vcxx_enc
    oelibcxx
    oevhostfs
    oevlibc
    oevcore)

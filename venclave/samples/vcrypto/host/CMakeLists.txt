# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../vcrypto.edl host gen)

add_executable(vcrypto_host host.c ${gen})

target_include_directories(vcrypto_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(vcrypto_host oevhostapp)

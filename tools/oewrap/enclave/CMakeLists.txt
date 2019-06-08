# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../wrap.edl enclave gen)

add_library(oewrapenclave wrap.c ${gen})

maybe_build_using_clangw(oewrapenclave)

target_include_directories(oewrapenclave
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oewrapenclave
    PRIVATE oeenclave ${CMAKE_CURRENT_BINARY_DIR})

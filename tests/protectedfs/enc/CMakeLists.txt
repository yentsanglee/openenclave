# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)
include(add_enclave_executable)

oeedl_file(../protectedfs.edl enclave gen)

add_executable(protectedfs_enc enc.cpp ${gen})

target_include_directories(protectedfs_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(protectedfs_enc pfsenclave oeenclave)

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)
include(add_enclave_executable)

oeedl_file(../ext2.edl enclave gen)

add_executable(ext2_enc enc.c ${gen})

target_include_directories(ext2_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(ext2_enc oeenclave)

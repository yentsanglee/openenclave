# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)
include(add_enclave_executable)

oeedl_file(../oefs.edl enclave gen)

add_executable(oefs_enc enc.c ${gen})

target_include_directories(oefs_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oefs_enc oeenclave oelibc mbedcrypto oelibc)

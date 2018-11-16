# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)
include(add_enclave_executable)

oeedl_file(../sgxfs.edl enclave gen)

add_executable(sgxfs_enc enc.cpp ${gen})

target_include_directories(sgxfs_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(sgxfs_enc oesgxfsenclave oehostfsenclave oeenclave)

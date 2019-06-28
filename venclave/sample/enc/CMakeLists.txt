# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../sample.edl enclave gen)

add_executable(sample_enc enc.c ${gen})

target_include_directories(sample_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(sample_enc PRIVATE
    ${CMAKE_BINARY_DIR}/output/include/openenclave/libc)

target_link_libraries(sample_enc oevenclave oelibc_includes)

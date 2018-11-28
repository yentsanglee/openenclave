# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)
include(add_enclave_executable)

oeedl_file(../fs.edl enclave gen)

add_executable(fs_enc enc.cpp test_default_fs_macro.c ${gen})

target_compile_options(fs_enc PRIVATE
    -ffreestanding)

target_include_directories(fs_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(fs_enc oecpio oemuxfs oeoefsenclave oesgxfsenclave oehostfsenclave oeenclave)

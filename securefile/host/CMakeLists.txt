# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)

oeedl_file(../secure_file.edl host gen --edl-search-dir ../enc/sgx)

add_executable(secure_file_host host.c ${gen})

target_include_directories(secure_file_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR} $ENV{SGX_SDK}/include)

add_library(sgxsdk_uae_host STATIC IMPORTED)
set_target_properties(sgxsdk_uae_host PROPERTIES IMPORTED_LOCATION $ENV{SGX_SDK}/lib64/libsgx_uae_service.so)

add_library(sgxsdk_fsecure_host STATIC IMPORTED)
set_target_properties(sgxsdk_fsecure_host PROPERTIES IMPORTED_LOCATION $ENV{SGX_SDK}/lib64/libsgx_uprotected_fs.a)

target_link_libraries(secure_file_host oehostapp sgxsdk_fsecure_host sgxsdk_uae_host)

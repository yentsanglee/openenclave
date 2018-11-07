# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)
include(add_enclave_executable)

oeedl_file(sgx/sgx_tprotected_fs.edl enclave-headers gen-tprotected-fs)
oeedl_file(../secure_file.edl enclave gen-secure-file --edl-search-dir sgx)

add_executable(secure_file_enc enc.cpp ${gen-secure-file} ${gen-tprotected-fs} sgx/file_crypto.cpp sgx/file_flush.cpp sgx/file_init.cpp sgx/file_other.cpp sgx/file_read_write.cpp sgx/file_version.cpp sgx/lru_cache.cpp sgx/sgx_tprotected_fs.cpp sgx/sgx_aes_gcm.cpp  sgx/sgx_cmac128.cpp)

target_include_directories(secure_file_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR} sgx $ENV{SGX_SDK}/include)

target_link_libraries(secure_file_enc oelibcxx oeenclave )

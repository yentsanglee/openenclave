# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set(LINUX_SGX_DIR ${PROJECT_SOURCE_DIR}/3rdparty/linux-sgx)

add_library(pfshost STATIC
    handleocall.c
    sgx_unprotected.cpp)

target_include_directories(pfshost PRIVATE 
    ${LINUX_SGX_DIR}
    ${LINUX_SGX_DIR}/linux-sgx/common/inc
    ${LINUX_SGX_DIR}/linux-sgx/sdk/protected_fs/sgx_uprotected_fs)

set_property(TARGET pfshost 
    PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/host)

install(TARGETS pfshost EXPORT openenclave-targets 
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/host)

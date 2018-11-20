# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set(LINUX_SGX_DIR ${PROJECT_SOURCE_DIR}/3rdparty/linux-sgx)

add_library(oemuxfs STATIC 
    muxfs.c)

target_include_directories(oemuxfs PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(oemuxfs PUBLIC oeenclave mbedcrypto oelibc)

set_property(TARGET oemuxfs PROPERTY 
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave)

install(TARGETS oemuxfs 
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

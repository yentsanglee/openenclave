# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set(LINUX_SGX_DIR ${PROJECT_SOURCE_DIR}/3rdparty/linux-sgx)

add_library(oeconsoleenclave STATIC
    console.c
    ../../common/hostbatch.c)

target_include_directories(oeconsoleenclave PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(oeconsoleenclave PUBLIC oeenclave mbedcrypto oelibc)

set_property(TARGET oeconsoleenclave PROPERTY
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave)

install(TARGETS oeconsoleenclave
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

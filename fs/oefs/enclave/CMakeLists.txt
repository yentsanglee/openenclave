# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set(LINUX_SGX_DIR ${PROJECT_SOURCE_DIR}/3rdparty/linux-sgx)

add_library(oeoefsenclave STATIC 
    ../../common/buf.c
    ../../common/hostbatch.c
    authcryptoblkdev.c
    cacheblkdev.c
    cryptoblkdev.c
    hashlistblkdev.c
    hostblkdev.c
    list.c
    merkleblkdev.c
    merkle2blkdev.c
    oefs.c
    ramblkdev.c
    sha.c)

target_include_directories(oeoefsenclave PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(oeoefsenclave PUBLIC oeenclave mbedcrypto oelibc)

set_property(TARGET oeoefsenclave PROPERTY 
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave)

install(TARGETS oeoefsenclave 
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

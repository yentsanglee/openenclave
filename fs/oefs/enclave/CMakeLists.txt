# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set(LINUX_SGX_DIR ${PROJECT_SOURCE_DIR}/3rdparty/linux-sgx)

add_library(oeoefsenclave STATIC 
    ../../common/buf.c
    ../../common/hostbatch.c
    authcryptoblkdev.c
    cacheblkdev.c
    cryptoblkdev.c
    hostblkdev.c
    list.c
    merkleblkdev.c
    oefs.c
    ramblkdev.c
    sha256_rorx.o
    sha.c
    fast_sha256.c)

target_include_directories(oeoefsenclave PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(oeoefsenclave PUBLIC oeenclave mbedcrypto oelibc)

find_program(YASM_EXE NAMES yasm)

add_custom_command(OUTPUT sha256_rorx.o COMMAND ${YASM_EXE}
    -f x64 -f elf64 -X gnu -g dwarf2 -D LINUX 
    -o sha256_rorx.o ${CMAKE_CURRENT_SOURCE_DIR}/sha256_rorx.asm)

set_property(TARGET oeoefsenclave PROPERTY 
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave)

install(TARGETS oeoefsenclave 
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

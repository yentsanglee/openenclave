# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set(LINUX_SGX_DIR ${PROJECT_SOURCE_DIR}/3rdparty/linux-sgx)

set(USE_SHA256_RORX 1)

if(USE_SHA256_RORX)
  set(SHA256_SOURCE sha256_rorx.o)
else()
  set(SHA256_SOURCE sha256_transform.c)
endif()

add_library(oeoefsenclave STATIC 
    ../../common/buf.c
    ../../common/hostbatch.c
    cacheblkdev.c
    cryptoblkdev.c
    hostblkdev.c
    list.c
    merkleblkdev.c
    oefs.c
    ramblkdev.c
    sha256.c
    ${SHA256_SOURCE})

target_include_directories(oeoefsenclave PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(oeoefsenclave PUBLIC oeenclave mbedcrypto oelibc)

find_program(YASM_EXE NAMES yasm)

if(USE_SHA256_RORX)
add_custom_command(OUTPUT sha256_rorx.o
    COMMAND ${YASM_EXE} -f x64 -f elf64 -X gnu -g dwarf2 -D LINUX
    -o sha256_rorx.o ${CMAKE_CURRENT_SOURCE_DIR}/sha256_rorx.asm
    DEPENDS sha256_rorx.asm)
endif()

if(USE_SHA256_RORX)
set_source_files_properties(sha256.c PROPERTIES COMPILE_FLAGS -DUSE_SHA256_RORX)
endif()

set_property(TARGET oeoefsenclave PROPERTY 
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave)

install(TARGETS oeoefsenclave 
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

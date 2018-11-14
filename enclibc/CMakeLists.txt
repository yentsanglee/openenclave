# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if(UNIX)
    set(SOURCES setjmp.S longjmp.S)
elseif (WIN32)
    # Put windows setjmp.asm and longjmp.asm here
endif()


add_library(oeenclibc STATIC
    assert.c
    atexit.c
    ctype.c
    errno.c
    gmtime.c
    malloc.c
    printf.c
    pthread.c
    rand.c
    sbrk.c
    stdio.c
    strerror.c
    string.c
    strtoul.c
    time.c
    ${SOURCES})

target_link_libraries(oeenclibc oecore)

target_compile_options(oeenclibc INTERFACE -nostdinc)

target_include_directories(oeenclibc PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/openenclave/enclibc)

set_property(TARGET oeenclibc 
    PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave)

install(TARGETS oeenclibc 
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

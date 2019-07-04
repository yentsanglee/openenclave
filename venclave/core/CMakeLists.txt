# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

##==============================================================================
##
## libc:
##
##==============================================================================

add_library(c STATIC libc.c)

target_compile_options(c PRIVATE
    -fPIE
    -nostdinc)

set_property(TARGET c PROPERTY
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave/libc)

install(TARGETS c
    EXPORT openenclave-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave/libc)

##==============================================================================
##
## libcxx:
##
##==============================================================================

add_library(cxx STATIC libcxx.cpp)

target_compile_options(cxx PRIVATE
    -fPIE
    -nostdinc)

set_property(TARGET cxx PROPERTY
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave/libcxx)

set(CXX_NAME stdc++)

set_property(TARGET cxx PROPERTY
    OUTPUT_NAME ${CXX_NAME})

install(TARGETS cxx
    EXPORT openenclave-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave/libcxx)

##==============================================================================
##
## libm:
##
##==============================================================================

add_library(m STATIC libm.c)

target_compile_options(m PRIVATE
    -fPIE
    -nostdinc)

set_property(TARGET m PROPERTY
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave/libm)

install(TARGETS m
    EXPORT openenclave-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave/libm)

##==============================================================================
##
## oevend:
##
##==============================================================================

add_library(oevend SHARED libvend.c)

target_link_libraries(oevend PRIVATE -nostdlib)

set_property(TARGET oevend PROPERTY
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave)

install(TARGETS oevend
    EXPORT openenclave-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

##==============================================================================
##
## oevcore:
##
##==============================================================================

add_library(oevcore STATIC
    assert.c
    string.c
    syscall.c
    recvfd.c
    socket.c
    time.c
    globals.c
    clone.S
    sbrk.c
    main.c
    print.c
    malloc.c
    shm.c
    process.c
    io.c
    call.c
    lock.c
    hexdump.c
    register.c
    thread.c
    enclave.c
    safecrt.c
    memmove.c
    signal.c
    restore.S
    panic.c
    futex.c
    __secs_to_tm.c
    ../../enclave/core/assert.c
    ../../enclave/core/ctype.c
    ../../enclave/core/gmtime.c
    ../../enclave/core/hexdump.c
    ../../enclave/core/intstr.c
    ../../enclave/core/printf.c
    ../../enclave/core/pthread.c
    ../../enclave/core/result.c
    ../../enclave/core/stdio.c
    ../../enclave/core/strerror.c
    ../../enclave/core/string.c
    ../../enclave/core/strtok_r.c
    ../../enclave/core/strtoul.c
    ../../enclave/core/sgx/atexit.c
    ../../enclave/core/sgx/entropy.c
    ../../enclave/core/sgx/jump.c
    ../../enclave/core/sgx/keys.c
    ../../enclave/core/sgx/report.c
    ../../enclave/core/sgx/once.c
    ../../enclave/core/sgx/sched_yield.c
    ../../enclave/core/sgx/spinlock.c
    ../../enclave/core/sgx/thread.c)

target_compile_options(oevcore PUBLIC
    -fPIE
    -nostdinc)

target_compile_options(oevcore
    INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>)

target_link_libraries(
    oevcore
    PUBLIC
    -L${OE_LIBDIR}/openenclave/enclave/libc
    -L${OE_LIBDIR}/openenclave/enclave/libcxx
    -L${OE_LIBDIR}/openenclave/enclave/libm
    ${CXX_NAME}
    -l${CXX_NAME}
    -static-libgcc
    oevend
    oe_includes)

target_compile_definitions(oevcore PUBLIC OE_BUILD_ENCLAVE)

target_include_directories(oevcore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET oevcore PROPERTY
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave)

install(TARGETS oevcore EXPORT openenclave-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

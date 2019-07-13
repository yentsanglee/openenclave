# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

##==============================================================================
##
## oevcore_libc:
##
##==============================================================================

add_library(oevcore_libc STATIC libc.c)

set_property(TARGET oevcore_libc PROPERTY OUTPUT_NAME c)

set_property(TARGET oevcore_libc PROPERTY
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave/venclave)

target_link_libraries(oevcore_libc PRIVATE oecore_cflags)

install(TARGETS oevcore_libc
    EXPORT openenclave-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave/venclave)

##==============================================================================
##
## oevcore_libcxx:
##
##==============================================================================

add_library(oevcore_libcxx STATIC libcxx.cpp)

set_property(TARGET oevcore_libcxx PROPERTY OUTPUT_NAME stdc++)

set_property(TARGET oevcore_libcxx PROPERTY
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave/venclave)

target_link_libraries(oevcore_libcxx PRIVATE oecore_cflags)

install(TARGETS oevcore_libcxx
    EXPORT openenclave-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave/venclave)

##==============================================================================
##
## oevcore_libm:
##
##==============================================================================

add_library(oevcore_libm STATIC libm.c)

set_property(TARGET oevcore_libm PROPERTY OUTPUT_NAME m)

set_property(TARGET oevcore_libm PROPERTY
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave/venclave)

target_link_libraries(oevcore_libm PRIVATE oecore_cflags)

install(TARGETS oevcore_libm
    EXPORT openenclave-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave/venclave)

##==============================================================================
##
## oevend:
##
##==============================================================================

add_library(oevend SHARED libvend.c)

set_property(TARGET oevend PROPERTY
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave)

target_link_libraries(oevend PRIVATE -nostdlib oecore_cflags)

install(TARGETS oevend
    EXPORT openenclave-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

##==============================================================================
##
## oevcore:
##
##==============================================================================

add_library(oevcore STATIC
    calls.c
    clone.S
    elf.c
    enclave.c
    futex.c
    hexdump.c
    hostheap.c
    hostmalloc.c
    io.c
    lock.c
    initfini.c
    main.c
    malloc.c
    memmove.c
    msg.c
    panic.c
    print.c
    process.c
    recvfd.c
    restore.S
    sbrk.c
    __secs_to_tm.c
    shm.c
    signal.c
    socket.c
    string.c
    syscall.c
    thread.c
    time.c
    ../common/call.c
    ../../enclave/core/assert.c
    ../../enclave/core/ctype.c
    ../../enclave/core/gmtime.c
    ../../enclave/core/hexdump.c
    ../../enclave/core/intstr.c
    ../../enclave/core/printf.c
    ../../enclave/core/pthread.c
    ../../enclave/core/result.c
    ../../enclave/core/sgx/atexit.c
    ../../enclave/core/sgx/entropy.c
    ../../enclave/core/sgx/jump.c
    ../../enclave/core/sgx/keys.c
    ../../enclave/core/sgx/once.c
    ../../enclave/core/sgx/report.c
    ../../enclave/core/sgx/report.c
    ../../common/safecrt.c
    ../../enclave/core/sgx/sched_yield.c
    ../../enclave/core/sgx/spinlock.c
    ../../enclave/core/sgx/thread.c
    ../../enclave/core/stdio.c
    ../../enclave/core/strerror.c
    ../../enclave/core/string.c
    ../../enclave/core/strtok_r.c
    ../../enclave/core/strtoul.c)

set_source_files_properties(../../enclave/core/sgx/jump.c
    PROPERTIES COMPILE_FLAGS -O2)

target_compile_options(oevcore
    PUBLIC
    INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-fstack-protector-strong>
    INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>)

target_link_libraries(oevcore
    PUBLIC
    #-Wl,-eoe_main
    -static-libgcc
    -L${OE_LIBDIR}/openenclave/enclave/venclave
    oevcore_libc
    oevcore_libcxx
    oevcore_libm
    oevend
    oecore_cflags
    oe_includes)

target_compile_definitions(oevcore
    PUBLIC OE_BUILD_VENCLAVE)

target_include_directories(oevcore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR})

set_property(TARGET oevcore PROPERTY
    ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave)

install(TARGETS oevcore EXPORT openenclave-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

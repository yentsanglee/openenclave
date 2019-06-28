# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(oevenclave STATIC
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
    result.c
    memmove.c
    signal.c
    restore.S
    futex.c)

target_compile_options(oevenclave PUBLIC
    -fPIE
    -nostdinc)

target_link_libraries(oevenclave PUBLIC oe_includes)

target_compile_definitions(oevenclave PRIVATE OE_BUILD_ENCLAVE)

target_include_directories(oevenclave PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS oevenclave EXPORT openenclave-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/host)


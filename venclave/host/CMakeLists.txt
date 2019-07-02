# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(oevhost STATIC
    call.c
    enclave.c
    malloc.c
    hostmalloc.c
    io.c
    heap.c
    log.c
    err.c
    strings.c
    result.c
    safecrt.c
    elf.c
    host.c
    sendfd.c
    rand.c
    syscall.c
    ../../host/linux/hostthread.c
    ../../host/syscall_u_wrapper.c)

target_link_libraries(oevhost PUBLIC oe_includes)

target_include_directories(oevhost PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/host)

install(TARGETS oevhost EXPORT openenclave-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/host)

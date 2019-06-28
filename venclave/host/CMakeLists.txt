# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(oevhost STATIC
    call.c
    sendfd.c
    malloc.c
    hostmalloc.c
    io.c
    enclave.c
    heap.c
    err.c
    strings.c
    result.c
    safecrt.c
    elf.c
    host.c
    rand.c
    log.c)

target_link_libraries(oevhost PUBLIC oe_includes)

target_include_directories(oevhost PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS oevhost EXPORT openenclave-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/host)

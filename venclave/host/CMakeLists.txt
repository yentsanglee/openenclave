# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_custom_command(
    OUTPUT vproxyhostfp.c
    DEPENDS ${OE_BINDIR}/oevproxyhost ${OE_BINDIR}/oefingerprint
    COMMAND
        ${OE_BINDIR}/oefingerprint
        ${OE_BINDIR}/oevproxyhost
        ${CMAKE_CURRENT_BINARY_DIR}/vproxyhostfp.c
        __ve_vproxyhostfp)

add_custom_target(vproxyhostfp
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/vproxyhostfp.c)

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
    ../../host/syscall_u_wrapper.c
    ${CMAKE_CURRENT_BINARY_DIR}/vproxyhostfp.c)

add_dependencies(oevhost vproxyhostfp)

target_link_libraries(oevhost
    PRIVATE oehost
    PUBLIC oe_includes)

target_include_directories(oevhost PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/host)

install(TARGETS oevhost EXPORT openenclave-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/host)

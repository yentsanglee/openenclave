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
    calls.c
    enclave.c
    hostmalloc.c
    io.c
    heap.c
    err.c
    elf.c
    host.c
    sendfd.c
    syscall.c
    ../common/fingerprint.c
    ../common/call.c
    ../../common/result.c
    ../../common/safecrt.c
    ../../host/linux/hostthread.c
    ../../host/sgx/linux/aesm.c
    ../../host/strings.c
    ../../host/syscall_u_wrapper.c
    ../../host/sgx/quote.c
    ../../host/sgx/report.c
    ${CMAKE_CURRENT_BINARY_DIR}/vproxyhostfp.c)

add_dependencies(oevhost vproxyhostfp)

set(FULL_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")

target_compile_definitions(oevhost PRIVATE
    VPROXYHOST_BUILD="${OE_BINDIR}/oevproxyhost"
    VPROXYHOST_INSTALL="${FULL_INSTALL_BINDIR}/oevproxyhost")

target_link_libraries(oevhost
    PUBLIC crypto oe_includes)

# Prebuild the virtuaual proxy.
add_dependencies(oevhost oevproxyhost oevproxyenc)

target_include_directories(oevhost PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/host)

install(TARGETS oevhost EXPORT openenclave-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/host)

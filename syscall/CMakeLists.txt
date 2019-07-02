# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

##==============================================================================
##
## These rules generate the edge routines for the SYSCALL interface, which is
## installed by oe_register_syscall_ecall_function_table().
##
##==============================================================================

set(EDL_DIR ${CMAKE_SOURCE_DIR}/common)
set(EDL_FILE ${EDL_DIR}/syscall.edl)

add_custom_command(
    OUTPUT syscall_t.h syscall_t.c syscall_args.h
    DEPENDS ${EDL_FILE}
    COMMAND edger8r --search-path ${EDL_DIR} --trusted ${EDL_FILE})

add_custom_target(syscall_trusted_edl
    DEPENDS syscall_t.h syscall_t.c syscall_args.h)

##==============================================================================
##
## oesyscallobj target:
##
##==============================================================================

add_library(oesyscallobj OBJECT
    syscall_t_wrapper.c
    consolefs.c
    device.c
    dirent.c
    ioctl.c
    fcntl.c
    fdtable.c
    iov.c
    mount.c
    netdb.c
    poll.c
    select.c
    socket.c
    stat.c
    stdio.c
    stdlib.c
    stub.c
    syscall.c
    unistd.c
    utsname.c)

maybe_build_using_clangw(oesyscallobj)

add_dependencies(oesyscallobj syscall_trusted_edl)

target_compile_definitions(oesyscallobj PRIVATE OE_BUILD_ENCLAVE)

target_link_libraries(oesyscallobj PRIVATE oe_includes oelibc_includes)

target_include_directories(oesyscallobj PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/include/openenclave/corelibc)

set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/syscall_t.c
    PROPERTIES COMPILE_FLAGS -DOE_NEED_STDC_NAMES)

##==============================================================================
##
## oesyscall target:
##
##==============================================================================

add_library(oesyscall STATIC)

target_link_libraries(oesyscall
    PUBLIC oecore
    PRIVATE oesyscallobj)

##==============================================================================
##
## oevsyscall target:
##
##==============================================================================

add_library(oevsyscall STATIC)

target_link_libraries(oevsyscall
    PUBLIC oevenclave
    PRIVATE oesyscallobj)

##==============================================================================
##
## install targets:
##
##==============================================================================

install(TARGETS oesyscall oevsyscall oesyscallobj
    EXPORT openenclave-targets
    ARCHIVE
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

##==============================================================================
##
## devices target:
##
##==============================================================================

add_subdirectory(devices)

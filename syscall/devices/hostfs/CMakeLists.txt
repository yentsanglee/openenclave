# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

##==============================================================================
##
## oehostfsobj:
##
##==============================================================================

add_library(oehostfsobj OBJECT hostfs.c)

maybe_build_using_clangw(oehostfsobj)

add_dependencies(oehostfsobj syscall_trusted_edl)

target_include_directories(oehostfsobj PRIVATE
    ${CMAKE_BINARY_DIR}/syscall
    ${PROJECT_SOURCE_DIR}/include/openenclave/corelibc)

target_compile_definitions(oehostfsobj PRIVATE OE_BUILD_ENCLAVE)

target_compile_options(oehostfsobj PRIVATE -fPIE)

target_link_libraries(oehostfsobj PRIVATE oe_includes oelibc_includes)

##==============================================================================
##
## oehostfs:
##
##==============================================================================

add_library(oehostfs STATIC)

target_link_libraries(oehostfs
    PUBLIC oesyscall
    PRIVATE oehostfsobj)

##==============================================================================
##
## oevhostfs:
##
##==============================================================================

add_library(oevhostfs STATIC)

target_link_libraries(oevhostfs
    PUBLIC oevsyscall
    PRIVATE oehostfsobj)

##==============================================================================
##
## install:
##
##==============================================================================

install(TARGETS oehostfsobj oehostfs oevhostfs
    EXPORT openenclave-targets
    ARCHIVE
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

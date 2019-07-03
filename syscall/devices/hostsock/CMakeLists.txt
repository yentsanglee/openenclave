# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

##==============================================================================
##
## oehostsockobj:
##
##==============================================================================

add_library(oehostsockobj OBJECT hostsock.c)

maybe_build_using_clangw(oehostsockobj)

add_dependencies(oehostsockobj syscall_trusted_edl)

target_include_directories(oehostsockobj PRIVATE
    ${CMAKE_BINARY_DIR}/syscall
    ${PROJECT_SOURCE_DIR}/include/openenclave/corelibc)

target_compile_definitions(oehostsockobj PRIVATE OE_BUILD_ENCLAVE)

target_compile_options(oehostsockobj PRIVATE -fPIE)

target_link_libraries(oehostsockobj PRIVATE oe_includes oelibc_includes)

##==============================================================================
##
## oehostsock:
##
##==============================================================================

add_library(oehostsock STATIC)

target_link_libraries(oehostsock
    PUBLIC oesyscall
    PRIVATE oehostsockobj)

##==============================================================================
##
## oevhostsock:
##
##==============================================================================

add_library(oevhostsock STATIC)

target_link_libraries(oevhostsock
    PUBLIC oevsyscall
    PRIVATE oehostsockobj)

##==============================================================================
##
## install:
##
##==============================================================================

install(TARGETS oehostsockobj oehostsock oevhostsock
    EXPORT openenclave-targets
    ARCHIVE
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

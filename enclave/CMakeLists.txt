# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_subdirectory(core)
add_subdirectory(crypto)

if (OE_SGX)
    set(PLATFORM_SRC
        ../common/sgx/qeidentity.c
        ../common/sgx/quote.c
        ../common/sgx/report.c
        ../common/sgx/revocation.c
        ../common/sgx/sgxcertextensions.c
        ../common/sgx/tcbinfo.c
        ../common/sgx/tlsverifier.c
        sgx/link.c
        sgx/qeidinfo.c
        sgx/report.c
        sgx/revocationinfo.c
        sgx/start.S
    )
elseif(OE_TRUSTZONE)
    message("TODO: ADD ARM files.")
endif()

add_library(oeenclave STATIC
    ../common/datetime.c
    ../common/lockless_queue.c
    asym_keys.c
    random.c
    tls_cert.c
    ${PLATFORM_SRC})

maybe_build_using_clangw(oeenclave)

if(CMAKE_C_COMPILER_ID MATCHES GNU)
target_compile_options(oeenclave PRIVATE -Wjump-misses-init)
endif()

target_compile_options(oeenclave PRIVATE -fPIE)

target_compile_definitions(oeenclave PUBLIC OE_BUILD_ENCLAVE)

target_link_libraries(oeenclave PRIVATE oe_includes)

set_property(TARGET oeenclave PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/enclave)

install(TARGETS oeenclave EXPORT openenclave-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

##==============================================================================
##
## oeenclave_libs
##
##==============================================================================

add_library(oeenclave_libs INTERFACE)

target_link_libraries(oeenclave_libs INTERFACE
    oeenclave
    oecryptombed
    mbedx509
    mbedcrypto
    oesyscall
    oelibc
    oecore
    oe_includes)

##==============================================================================
##
## oevenclave_libs
##
##==============================================================================

add_library(oevenclave_libs INTERFACE)

target_link_libraries(oevenclave_libs INTERFACE
    oeenclave
    oecryptombed
    mbedx509
    mbedcrypto
    oesyscall
    oevlibc
    oevcore
    oe_includes)

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Copy mbedtls sources to replace config.h and build w/ own flags

if(WIN32)
    set(LIBC_DIR "${PROJECT_SOURCE_DIR}/include/openenclave/elibc")
    set(CFLAGS "-W3 -WX -GS")
    set(LIBMBEDTLS "mbedtls.lib")
    set(LIBMBEDCRYPTO "mbedcrypto.lib")
    set(LIBMBEDX509 "mbedx509.lib")
    set(LIBOELIBC "oeelibc.lib")
    set(LIBOECORE "oecore.lib")
else(UNIX)
    set(LIBC_DIR "${OE_INCDIR}/openenclave/libc")
    set(CFLAGS "-nostdinc -fPIC -fno-builtin-udivti3")
    set(LIBMBEDTLS "libmbedtls.a")
    set(LIBMBEDCRYPTO "libmbedcrypto.a")
    set(LIBMBEDX509 "libmbedx509.a")
    set(LIBOELIBC "liboelibc.a")
    set(LIBOECORE "liboecore.a")
endif()

set(MBEDTLS_WRAP_CFLAGS "-I${LIBC_DIR} ${CFLAGS} ${SPECTRE_MITIGATION_FLAGS}")

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
	configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config.h ${CMAKE_BINARY_DIR}/3rdparty/mbedtls/config_final.h COPYONLY)
	file(APPEND ${CMAKE_BINARY_DIR}/3rdparty/mbedtls/config_final.h "\#define\ MBEDTLS_CERTS_C\n\n\#define\ MBEDTLS_DEBUG_C\n")
else()
	configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config.h ${CMAKE_BINARY_DIR}/3rdparty/mbedtls/config_final.h COPYONLY)
endif()

if (UNIX)
set(MBEDTLS_DEPENDS "oelibc_includes")
endif()

include (ExternalProject)
ExternalProject_Add(mbedtls-wrap
    DEPENDS ${MBEDTLS_DEPENDS}

    DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_LIST_DIR}/mbedtls <SOURCE_DIR>
    PATCH_COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/3rdparty/mbedtls/config_final.h <SOURCE_DIR>/include/mbedtls/config.h
    # Addl args for compiler
    CMAKE_ARGS
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_C_FLAGS=${MBEDTLS_WRAP_CFLAGS}
        -DENABLE_PROGRAMS=OFF
        -DENABLE_TESTING=OFF
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

    # copy headers/libs
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory
        <SOURCE_DIR>/include/mbedtls ${OE_INCDIR}/openenclave/mbedtls
        COMMAND ${CMAKE_COMMAND} -E copy
        <BINARY_DIR>/library/${LIBMBEDTLS} ${OE_LIBDIR}/openenclave/enclave/
        <BINARY_DIR>/library/${LIBMBEDX509} ${OE_LIBDIR}/openenclave/enclave/
        <BINARY_DIR>/library/${LIBMBEDCRYPTO} ${OE_LIBDIR}/openenclave/enclave/
    )
install (DIRECTORY ${OE_INCDIR}/openenclave/mbedtls DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openenclave/3rdparty)
install (FILES ${OE_LIBDIR}/openenclave/enclave/${LIBMBEDTLS} DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)
install (FILES ${OE_LIBDIR}/openenclave/enclave/${LIBMBEDX509} DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)
install (FILES ${OE_LIBDIR}/openenclave/enclave/${LIBMBEDCRYPTO} DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

# Convenience lib for use in target_link_libraries
add_library(mbedcrypto INTERFACE)
add_dependencies(mbedcrypto mbedtls-wrap)
target_include_directories(mbedcrypto INTERFACE ${OE_INCDIR}/openenclave/)
target_link_libraries(mbedcrypto INTERFACE 
    ${OE_LIBDIR}/openenclave/enclave/${LIBMBEDTLS}
    ${OE_LIBDIR}/openenclave/enclave/${LIBMBEDX509}
    ${OE_LIBDIR}/openenclave/enclave/${LIBMBEDCRYPTO}
)
# For the libc dependency, We would just want to use "oelibc". However, we need
# an actual lib-dependency between crypto an libc, which is not expressable
# with CMake. If using the target "oelibc", CMake reorders the libs, resulting
# in undefined LIBC symbols from crypto. Using the explicit lib-name preserves
# order.
target_link_libraries(mbedcrypto INTERFACE
	${OE_LIBDIR}/openenclave/enclave/${LIBOELIBC}
	${OE_LIBDIR}/openenclave/enclave/${LIBOECORE})


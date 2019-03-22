# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if (OE_SGX)
  set(ARCH "x86_64")
else()
  set(ARCH "aarch64")
endif()

set(OPENSSL_CC ${CMAKE_C_COMPILER})
set(OPENSSL_CXX ${CMAKE_CXX_COMPILER})

set(OPENSSL_DIR ${CMAKE_CURRENT_BINARY_DIR})
get_filename_component(OPENSSL_INSTALL_DIR ${OPENSSL_DIR}/../openssl_install ABSOLUTE)
set(OPENSSL_VERSION_NAME OpenSSL_1_1_1b)

if (USE_CLANGW)
  set(OPENSSL_CFLAGS "-target x86_64-pc-linux ${MUSL_CFLAGS}")
  set(OPENSSL_CC clang)
  set(OPENSSL_CXX clang++)
endif ()

include (ExternalProject)
ExternalProject_Add(oe_openssl
  DEPENDS oecore

  DOWNLOAD_COMMAND
    tar xvzf ${CMAKE_CURRENT_LIST_DIR}/${OPENSSL_VERSION_NAME}.tar.gz -C ${OPENSSL_DIR}

  PATCH_COMMAND
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_LIST_DIR}/patches/crypto/rand/rand_lib.c
      ${OPENSSL_DIR}/openssl-${OPENSSL_VERSION_NAME}/crypto/rand/rand_lib.c
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_LIST_DIR}/patches/crypto/rand/drbg_lib.c
      ${OPENSSL_DIR}/openssl-${OPENSSL_VERSION_NAME}/crypto/rand/drbg_lib.c
  CONFIGURE_COMMAND
    ${CMAKE_COMMAND} -E chdir ${OPENSSL_DIR}/openssl-${OPENSSL_VERSION_NAME}
    ${BASH} -x ./config --with-rand-seed=none no-idea no-mdc2 no-rc5 no-rc4 no-bf no-ec2m no-camellia no-cast no-srp no-hw no-dso no-shared no-ssl3 no-md2 no-md4 no-afalgeng -D_FORTIFY_SOURCE=2 -DGETPID_IS_MEANINGLESS --prefix=${OPENSSL_INSTALL_DIR}
    CFLAGS=${OPENSSL_CFLAGS}
    CC=${OPENSSL_CC}
    CXX=${OPENSSL_CXX}

  BUILD_COMMAND
    ${CMAKE_COMMAND} -E chdir ${OPENSSL_DIR}/openssl-${OPENSSL_VERSION_NAME}
    make all install

  BUILD_BYPRODUCTS
    ${OPENSSL_INSTALL_DIR}/lib/libcrypto.a
    ${OPENSSL_INSTALL_DIR}/lib/libssl.a

  INSTALL_COMMAND "")


add_library(oe_openssl_crypto STATIC IMPORTED)
add_library(oe_openssl_ssl STATIC IMPORTED)
set_target_properties(oe_openssl_crypto PROPERTIES
  IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/lib/libcrypto.a)
set_target_properties(oe_openssl_ssl PROPERTIES
  IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/lib/libssl.a)
target_link_libraries(oe_openssl_crypto INTERFACE oecore)
target_link_libraries(oe_openssl_crypto INTERFACE oecore)
add_dependencies(oe_openssl_crypto oe_openssl)
add_dependencies(oe_openssl_ssl oe_openssl oe_openssl_crypto)
target_include_directories(oe_openssl_crypto INTERFACE ${OPENSSL_INSTALL_DIR}/include)
target_include_directories(oe_openssl_ssl INTERFACE ${OPENSSL_INSTALL_DIR}/include)



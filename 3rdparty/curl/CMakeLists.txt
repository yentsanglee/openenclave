# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if (OE_SGX)
  set(ARCH "x86_64")
else()
  set(ARCH "aarch64")
endif()

set(CURL_CC ${CMAKE_C_COMPILER})
set(CURL_CXX ${CMAKE_CXX_COMPILER})

set(CURL_DIR ${CMAKE_CURRENT_BINARY_DIR})
get_filename_component(OPENSSL_INSTALL_DIR ${CURL_DIR}/../openssl_install ABSOLUTE)

if (USE_CLANGW)
  set(CURL_CFLAGS "-target x86_64-pc-linux ${MUSL_CFLAGS}")
  set(CURL_CC clang)
  set(CURL_CXX clang++)
endif ()

include (ExternalProject)
ExternalProject_Add(oe_curl
  DOWNLOAD_COMMAND
    tar xvzf ${CMAKE_CURRENT_LIST_DIR}/curl-7.64.1.tar.gz -C ${CURL_DIR}
  PATCH_COMMAND
  CONFIGURE_COMMAND
    ${CMAKE_COMMAND} -E chdir ${CURL_DIR}/curl-7.64.1
    ./configure --enable-debug --with-ssl=${OPENSSL_INSTALL_DIR} --enable-static --disable-shared --without-ntlm --without-zlib --disable-smb --disable-ntlm-wb  --disable-pthreads --disable-threaded-resolver
    CFLAGS=${CURL_CFLAGS}
    CC=${CURL_CC}
    CXX=${CURL_CXX}
    curl_disallow_alarm=yes
    curl_disallow_signal=yes
  BUILD_COMMAND
    ${CMAKE_COMMAND} -E chdir ${CURL_DIR}/curl-7.64.1
    make
  BUILD_BYPRODUCTS
    ${CURL_DIR}
  INSTALL_COMMAND "")

add_dependencies(oe_curl oe_openssl)

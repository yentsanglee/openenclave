# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../curl_openssl_client.edl enclave gen)

add_enclave(TARGET curl_openssl_clientenc
    SOURCES
    enc.c
    ${gen})

target_include_directories(curl_openssl_clientenc PRIVATE ${CMAKE_CURRENT_BINARY_DIR};${CMAKE_BINARY_DIR}/3rdparty/openssl_install/include;${CMAKE_BINARY_DIR}/3rdparty/curl/curl-7.64.1/include)
target_link_libraries(curl_openssl_clientenc oelibc ${CMAKE_BINARY_DIR}/3rdparty/curl/curl-7.64.1/lib/.libs/libcurl.a ${CMAKE_BINARY_DIR}/3rdparty/openssl_install/lib/libssl.a ${CMAKE_BINARY_DIR}/3rdparty/openssl_install/lib/libcrypto.a)

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.


oeedl_file(../curl_openssl_client.edl host gen)

add_executable(curl_openssl_clienthost host.c ${gen})

target_include_directories(curl_openssl_clienthost PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(curl_openssl_clienthost oehostapp)

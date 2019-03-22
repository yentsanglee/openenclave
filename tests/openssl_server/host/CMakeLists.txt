# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.


oeedl_file(../openssl_server.edl host gen)

add_executable(openssl_serverhost host.c ${gen})

target_include_directories(openssl_serverhost PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(openssl_serverhost oehostapp)

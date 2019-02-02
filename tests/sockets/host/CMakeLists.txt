# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)

oeedl_file(../socket_test.edl 
    host socket_test_u
)

add_executable(socket_host
    host.c
    netdb_host.c
    socket_host.c
    arpa_inet_host.c
    ${socket_test_u}
)

target_compile_options(socket_host PRIVATE -Wno-unused-parameter)
target_compile_definitions(socket_host PRIVATE _GNU_SOURCE)
target_include_directories(socket_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(socket_host oehostapp)

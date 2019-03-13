# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.


oeedl_file(../exec.edl host gen)

add_executable(exec_host host.c ${gen})

target_include_directories(exec_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(exec_host oesgxfshost oehostapp)

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.


oeedl_file(../exec.edl host gen)

add_executable(oeexec host.c ${gen})

target_include_directories(oeexec PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oeexec oesgxfshost oehostapp)

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)

oeedl_file(../oefs.edl host gen)

add_executable(oefs_host host.c ${gen})

target_include_directories(oefs_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(oefs_host oehostapp)

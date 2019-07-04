# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../vcxx.edl host gen)

add_executable(vcxx_host host.cpp ${gen})

target_include_directories(vcxx_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(vcxx_host oevhost pthread)

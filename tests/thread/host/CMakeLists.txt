# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(../common.cmake)

oeedl_file(../thread.edl host gen)

add_executable(thread_host host.cpp rwlocks_test_host.cpp ${gen})

target_include_directories(thread_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_compile_definitions(thread_host PRIVATE -DTCS_COUNT=${TCS_COUNT})

target_link_libraries(thread_host oehosttest)

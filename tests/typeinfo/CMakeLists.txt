# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_custom_command(
    OUTPUT typeinfo_u.h
    DEPENDS oegen
    COMMAND oegen -u ${CMAKE_CURRENT_SOURCE_DIR}/typeinfo.idl)

add_custom_target(
    generate_idl
    DEPENDS typeinfo_u.h)

add_executable(typeinfo main.cpp typeinfo_u.c)

add_dependencies(typeinfo generate_idl)

target_include_directories(typeinfo
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(typeinfo oeidl oehost)

add_test(tests/typeinfo typeinfo)

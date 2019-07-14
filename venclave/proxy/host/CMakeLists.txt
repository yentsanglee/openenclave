# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../proxy.edl host gen)

add_executable(oevproxyhost main.c io.c msg.c ${gen})

target_include_directories(oevproxyhost
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(oevproxyhost oehost pthread)

set_property(TARGET oevproxyhost PROPERTY RUNTIME_OUTPUT_DIRECTORY ${OE_BINDIR})

install (TARGETS oevproxyhost DESTINATION ${CMAKE_INSTALL_BINDIR})

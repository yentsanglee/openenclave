# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../wrap.edl host gen)

add_executable(oerun run.c ${gen})

target_include_directories(oerun PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(oerun oehost)

set_property(TARGET oerun PROPERTY RUNTIME_OUTPUT_DIRECTORY ${OE_BINDIR})

install(TARGETS oerun DESTINATION ${CMAKE_INSTALL_BINDIR})

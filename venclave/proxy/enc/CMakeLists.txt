# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../proxy.edl enclave gen)

add_enclave(TARGET oevproxyenc SOURCES enc.c ${gen})

target_include_directories(oevproxyenc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

set_property(TARGET oevproxyenc PROPERTY RUNTIME_OUTPUT_DIRECTORY ${OE_BINDIR})

install (TARGETS oevproxyenc DESTINATION ${CMAKE_INSTALL_BINDIR})

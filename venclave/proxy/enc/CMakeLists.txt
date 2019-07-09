# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../proxy.edl enclave gen)

add_enclave(TARGET oevproxyenc SOURCES enc.c ${gen})

target_include_directories(oevproxyenc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

set_property(TARGET oevproxyenc PROPERTY RUNTIME_OUTPUT_DIRECTORY ${OE_BINDIR})

target_link_libraries(oevproxyenc
    oeenclave
    oecryptombed
    mbedx509
    mbedcrypto
    oelibc
    oesyscall
    oecore
    oe_includes)

install (TARGETS oevproxyenc DESTINATION ${CMAKE_INSTALL_BINDIR})

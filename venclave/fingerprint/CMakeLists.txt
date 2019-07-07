# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(oefingerprint
    main.c
    ../common/fingerprint.c)

target_include_directories(oefingerprint
    PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(oefingerprint
    PRIVATE crypto)

set_property(TARGET oefingerprint
    PROPERTY RUNTIME_OUTPUT_DIRECTORY ${OE_BINDIR})

install (TARGETS oefingerprint DESTINATION ${CMAKE_INSTALL_BINDIR})

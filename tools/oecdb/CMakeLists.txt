# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(oecdb oecdb.cpp)

target_link_libraries(oecdb crypto)

set_property(TARGET oecdb PROPERTY RUNTIME_OUTPUT_DIRECTORY ${OE_BINDIR})

install (TARGETS oecdb DESTINATION ${CMAKE_INSTALL_BINDIR})

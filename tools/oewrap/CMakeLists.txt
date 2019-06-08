# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(oewrap oewrap.cpp)

target_include_directories(oewrap PRIVATE ${CMAKE_SOURCE_DIR}/include)

set_property(TARGET oewrap PROPERTY RUNTIME_OUTPUT_DIRECTORY ${OE_BINDIR})

install (TARGETS oewrap DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory(host)
add_subdirectory(enclave)

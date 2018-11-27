# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(oeoefshost STATIC 
    oefsocall.c)

target_include_directories(oeoefshost PRIVATE 
    ${PROJECT_SOURCE_DIR}/include)

set_property(TARGET oeoefshost 
    PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/host)

install(TARGETS oeoefshost EXPORT openenclave-targets 
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/host)

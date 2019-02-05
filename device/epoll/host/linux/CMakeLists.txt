# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

include(oeedl_file)

add_library(oehostepoll_host STATIC
    host_epoll.c
    ${epoll_u_h})

if(CMAKE_C_COMPILER_ID MATCHES GNU)
target_compile_options(oehostepoll_host PRIVATE -Wjump-misses-init)
endif()

#target_link_libraries(oehostepoll_host PUBLIC
#    oelibc)

target_include_directories(oehostepoll_host PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ../../../../../include/
    )

set_property(TARGET oehostepoll_host PROPERTY ARCHIVE_OUTPUT_DIRECTORY ${OE_LIBDIR}/openenclave/host)

install(TARGETS oehostepoll_host ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/host)

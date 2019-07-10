# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(oecpio STATIC commands.c cpio.c strarr.c)

target_link_libraries(oecpio PRIVATE oecore_cflags oelibc_includes oe_includes)

maybe_build_using_clangw(oecpio)

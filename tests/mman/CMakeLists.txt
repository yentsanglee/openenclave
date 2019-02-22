# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(heap main.c mman.c)

target_link_libraries(heap oehost)

add_test(tests/heap heap)

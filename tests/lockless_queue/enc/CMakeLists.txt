# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

oeedl_file(../lockless_queue.edl enclave gen)

add_enclave(
        TARGET lockless_queue_enc
        SOURCES
        enc.c
        ../shared/lfds711_misc_internal_backoff_init.c
        ../shared/lfds711_misc_globals.c
        ../shared/lfds711_queue_unbounded_manyproducer_manyconsumer_cleanup.c
        ../shared/lfds711_queue_unbounded_manyproducer_manyconsumer_dequeue.c
        ../shared/lfds711_queue_unbounded_manyproducer_manyconsumer_enqueue.c
        ../shared/lfds711_queue_unbounded_manyproducer_manyconsumer_init.c
        ../shared/lfds711_queue_unbounded_manyproducer_manyconsumer_query.c
        ${gen})

target_include_directories(
        lockless_queue_enc
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared)

target_link_libraries(lockless_queue_enc oelibc)

if (CMAKE_C_COMPILER_ID MATCHES GNU)
   target_compile_options(
       lockless_queue_enc PRIVATE -Wno-error=unused-but-set-variable)
endif ()

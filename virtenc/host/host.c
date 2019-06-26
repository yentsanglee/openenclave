// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/host.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/utils.h>
#include <pthread.h>
#include "enclave.h"
#include "heap.h"

#define ENCLAVE_MAGIC 0x982dea60014b4c97

#define HEAP_SIZE (1024 * 1024)

struct _oe_enclave
{
    uint64_t magic;
    const oe_ocall_func_t* ocall_table;
    size_t ocall_table_size;
    ve_enclave_t* virtual_enclave;
};

extern ve_heap_t __ve_heap;

static bool _create_enclave_once_okay = false;

static void _create_enclave_once(void)
{
    /* Create the host heap to be shared with enclaves. */
    if (ve_heap_create(&__ve_heap, HEAP_SIZE) != 0)
        goto done;

    _create_enclave_once_okay = true;

done:
    return;
}

oe_result_t oe_create_enclave(
    const char* path,
    oe_enclave_type_t type,
    uint32_t flags,
    const void* config,
    uint32_t config_size,
    const oe_ocall_func_t* ocall_table,
    uint32_t ocall_table_size,
    oe_enclave_t** enclave_out)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_enclave_t* enclave = NULL;
    ve_enclave_t* virtual_enclave = NULL;
    static pthread_once_t _once = PTHREAD_ONCE_INIT;

    if (enclave_out)
        *enclave_out = NULL;

    /* Reject invalid parameters. */
    if (!path || !enclave_out || config || config_size > 0)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* If no debug mode flag. */
    if (!(flags & OE_ENCLAVE_FLAG_DEBUG))
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Simulate mode not supported by virtual enclaves. */
    if ((flags & OE_ENCLAVE_FLAG_SIMULATE))
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Call _create_enclave_once() the first time. */
    {
        if (pthread_once(&_once, _create_enclave_once) != 0)
            OE_RAISE(OE_FAILURE);

        if (!_create_enclave_once_okay)
            OE_RAISE(OE_FAILURE);
    }

    /* Create the virtual enclave. */
    if (ve_enclave_create(path, &__ve_heap, &virtual_enclave) != 0)
        OE_RAISE(OE_FAILURE);

    /* Create the OE enclave instance. */
    {
        if (!(enclave = calloc(1, sizeof(oe_enclave_t))))
            OE_RAISE(OE_OUT_OF_MEMORY);

        enclave->magic = ENCLAVE_MAGIC;
        enclave->ocall_table = ocall_table;
        enclave->ocall_table_size = ocall_table_size;
        enclave->virtual_enclave = virtual_enclave;
    }

    *enclave_out = enclave;
    enclave = NULL;

    result = OE_OK;

done:

    if (enclave)
        free(enclave);

    return result;
}

oe_result_t oe_terminate_enclave(oe_enclave_t* enclave)
{
    oe_result_t result = OE_UNEXPECTED;

    /* Reject invalid parameters. */
    if (!enclave || enclave->magic != ENCLAVE_MAGIC)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Terminate the vitual enclave. */
    if (ve_enclave_terminate(enclave->virtual_enclave) != 0)
        OE_RAISE(OE_FAILURE);

    /* Clear the contents of the enclave structure. */
    memset(enclave, 0xdd, sizeof(oe_enclave_t));

    /* Free the enclave structure */
    free(enclave);

    result = OE_OK;

done:
    return result;
}

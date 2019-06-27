// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/host.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/utils.h>
#include <pthread.h>
#include "enclave.h"
#include "heap.h"
#include "hostmalloc.h"

#define ENCLAVE_MAGIC 0x982dea60014b4c97

struct _oe_enclave
{
    uint64_t magic;
    const oe_ocall_func_t* ocall_table;
    size_t ocall_table_size;
    ve_enclave_t* venclave;
};

extern ve_heap_t __ve_heap;

static bool _create_enclave_once_okay = false;

static void _create_enclave_once(void)
{
    /* Create the host heap to be shared with enclaves. */
    if (ve_heap_create(&__ve_heap, VE_HEAP_SIZE) != 0)
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
    ve_enclave_t* venclave = NULL;
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
    if (ve_enclave_create(path, &__ve_heap, &venclave) != 0)
        OE_RAISE(OE_FAILURE);

    /* Create the OE enclave instance. */
    {
        if (!(enclave = calloc(1, sizeof(oe_enclave_t))))
            OE_RAISE(OE_OUT_OF_MEMORY);

        enclave->magic = ENCLAVE_MAGIC;
        enclave->ocall_table = ocall_table;
        enclave->ocall_table_size = ocall_table_size;
        enclave->venclave = venclave;
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
    if (ve_enclave_terminate(enclave->venclave) != 0)
        OE_RAISE(OE_FAILURE);

    /* Clear the contents of the enclave structure. */
    memset(enclave, 0xdd, sizeof(oe_enclave_t));

    /* Free the enclave structure */
    free(enclave);

    result = OE_OK;

done:
    return result;
}

oe_result_t oe_call_enclave_function_by_table_id(
    oe_enclave_t* enclave,
    uint64_t table_id,
    uint64_t function_id,
    const void* input_buffer,
    size_t input_buffer_size,
    void* output_buffer,
    size_t output_buffer_size,
    size_t* output_bytes_written)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_call_enclave_function_args_t* args = NULL;

    /* Reject invalid parameters */
    if (!enclave || enclave->magic != ENCLAVE_MAGIC)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Copy these args to the host heap. */
    if (!(args = ve_host_calloc(1, sizeof(*args))))
        OE_RAISE(OE_OUT_OF_MEMORY);

    /* Initialize the call_enclave_args structure */
    {
        args->table_id = table_id;
        args->function_id = function_id;
        args->input_buffer = input_buffer;
        args->input_buffer_size = input_buffer_size;
        args->output_buffer = output_buffer;
        args->output_buffer_size = output_buffer_size;
        args->output_bytes_written = 0;
        args->result = OE_UNEXPECTED;
    }

#if 1
    /* Test call. */
    {
        ve_enclave_t* ve = enclave->venclave;

        if (ve_enclave_run_xor_test(ve) != 0)
            OE_RAISE(OE_FAILURE);
        if (ve_enclave_run_xor_test(ve) != 0)
            OE_RAISE(OE_FAILURE);
        if (ve_enclave_run_xor_test(ve) != 0)
            OE_RAISE(OE_FAILURE);
        if (ve_enclave_run_xor_test(ve) != 0)
            OE_RAISE(OE_FAILURE);
        if (ve_enclave_run_xor_test(ve) != 0)
            OE_RAISE(OE_FAILURE);
        if (ve_enclave_run_xor_test(ve) != 0)
            OE_RAISE(OE_FAILURE);
        if (ve_enclave_run_xor_test(ve) != 0)
            OE_RAISE(OE_FAILURE);
    }
#endif

    /* Perform the ECALL */
    {
        ve_enclave_t* ve = enclave->venclave;
        uint64_t retval = 0;

        if (ve_enclave_call1(ve, VE_FUNC_ECALL, &retval, (long)args) != 0)
            OE_RAISE(OE_FAILURE);

        if (retval != 0)
            OE_CHECK(OE_FAILURE);
    }

    /* Check the result */
    OE_CHECK(args->result);

    *output_bytes_written = args->output_bytes_written;
    result = OE_OK;

done:

    if (args)
        ve_host_free(args);

    return result;
}

oe_result_t oe_call_enclave_function(
    oe_enclave_t* enclave,
    uint32_t function_id,
    const void* input_buffer,
    size_t input_buffer_size,
    void* output_buffer,
    size_t output_buffer_size,
    size_t* output_bytes_written)
{
    if (output_buffer && output_buffer_size)
        memset(output_buffer, 0, output_buffer_size);

    return oe_call_enclave_function_by_table_id(
        enclave,
        OE_UINT64_MAX,
        function_id,
        input_buffer,
        input_buffer_size,
        output_buffer,
        output_buffer_size,
        output_bytes_written);
}

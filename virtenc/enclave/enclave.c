// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/safemath.h>
#include <openenclave/edger8r/common.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/raise.h>
#include "call.h"
#include "globals.h"
#include "hexdump.h"
#include "lock.h"
#include "malloc.h"
#include "print.h"
#include "string.h"
#include "thread.h"
#include "trace.h"

extern __thread int __ve_thread_sock_tls;

extern const oe_ecall_func_t __oe_ecalls_table[];
extern const size_t __oe_ecalls_table_size;

typedef struct _ecall_table
{
    const oe_ecall_func_t* ecalls;
    size_t num_ecalls;
} ecall_table_t;

static ecall_table_t _ecall_tables[OE_MAX_ECALL_TABLES];
static ve_lock_t _ecall_tables_lock;

void* oe_host_malloc(size_t size)
{
    return ve_call_malloc(__ve_thread_sock_tls, size);
}

void* oe_host_calloc(size_t nmemb, size_t size)
{
    return ve_call_calloc(__ve_thread_sock_tls, nmemb, size);
}

void* oe_host_realloc(void* ptr, size_t size)
{
    return ve_call_realloc(__ve_thread_sock_tls, ptr, size);
}

void oe_host_free(void* ptr)
{
    ve_call_free(__ve_thread_sock_tls, ptr);
}

// Function used by oeedger8r for allocating ocall buffers.
void* oe_allocate_ocall_buffer(size_t size)
{
    return oe_host_malloc(size);
}

// Function used by oeedger8r for freeing ocall buffers.
void oe_free_ocall_buffer(void* buffer)
{
    oe_host_free(buffer);
}

char* oe_host_strndup(const char* str, size_t n)
{
    char* p;
    size_t len;

    if (!str)
        return NULL;

    len = ve_strlen(str);

    if (n < len)
        len = n;

    /* Would be an integer overflow in the next statement. */
    if (len == OE_SIZE_MAX)
        return NULL;

    if (!(p = oe_host_malloc(len + 1)))
        return NULL;

    if (ve_memcpy(p, str, len) != OE_OK)
        return NULL;

    p[len] = '\0';

    return p;
}

bool oe_is_within_enclave(const void* ptr, size_t size)
{
    const uint64_t min = (uint64_t)ptr;
    const uint64_t max = (uint64_t)ptr + size;
    const uint64_t lo = (uint64_t)__ve_shmaddr;
    const uint64_t hi = (uint64_t)__ve_shmaddr + __ve_shmsize;

    if (min >= lo && min < hi)
        return false;

    if (max >= lo && max <= hi)
        return false;

    return true;
}

bool oe_is_outside_enclave(const void* ptr, size_t size)
{
    const uint64_t min = (uint64_t)ptr;
    const uint64_t max = (uint64_t)ptr + size;
    const uint64_t lo = (uint64_t)__ve_shmaddr;
    const uint64_t hi = (uint64_t)__ve_shmaddr + __ve_shmsize;

    if (min < lo || min >= hi)
        return false;

    if (max < lo || max > hi)
        return false;

    return true;
}

oe_result_t oe_get_enclave_status(void)
{
    /* ATTN: implement! */
    return OE_OK;
}

oe_result_t oe_log(log_level_t level, const char* fmt, ...)
{
    ve_va_list ap;

    ve_va_start(ap, fmt);
    ve_print("oe_log: %u: ", level);
    ve_vprint(fmt, ap);
    ve_va_end(ap);

    return OE_OK;
}

oe_result_t oe_register_ecall_function_table(
    uint64_t table_id,
    const oe_ecall_func_t* ecalls,
    size_t num_ecalls)
{
    oe_result_t result = OE_UNEXPECTED;

    if (table_id >= OE_MAX_ECALL_TABLES || !ecalls)
        OE_RAISE(OE_INVALID_PARAMETER);

    ve_lock(&_ecall_tables_lock);
    _ecall_tables[table_id].ecalls = ecalls;
    _ecall_tables[table_id].num_ecalls = num_ecalls;
    ve_unlock(&_ecall_tables_lock);

    result = OE_OK;

done:
    return result;
}

static oe_result_t _handle_call_enclave_function(uint64_t arg_in)
{
    oe_call_enclave_function_args_t args, *args_ptr;
    oe_result_t result = OE_OK;
    oe_ecall_func_t func = NULL;
    uint8_t* buffer = NULL;
    uint8_t* input_buffer = NULL;
    uint8_t* output_buffer = NULL;
    size_t buffer_size = 0;
    size_t output_bytes_written = 0;
    ecall_table_t ecall_table;

    // Ensure that args lies outside the enclave.
    if (!oe_is_outside_enclave(
            (void*)arg_in, sizeof(oe_call_enclave_function_args_t)))
        OE_RAISE(OE_INVALID_PARAMETER);

    // Copy args to enclave memory to avoid TOCTOU issues.
    args_ptr = (oe_call_enclave_function_args_t*)arg_in;
    args = *args_ptr;

    // Ensure that input buffer is valid.
    // Input buffer must be able to hold atleast an oe_result_t.
    if (args.input_buffer == NULL ||
        args.input_buffer_size < sizeof(oe_result_t) ||
        !oe_is_outside_enclave(args.input_buffer, args.input_buffer_size))
        OE_RAISE(OE_INVALID_PARAMETER);

    // Ensure that output buffer is valid.
    // Output buffer must be able to hold atleast an oe_result_t.
    if (args.output_buffer == NULL ||
        args.output_buffer_size < sizeof(oe_result_t) ||
        !oe_is_outside_enclave(args.output_buffer, args.output_buffer_size))
        OE_RAISE(OE_INVALID_PARAMETER);

    // Validate output and input buffer sizes.
    // Buffer sizes must be correctly aligned.
    if ((args.input_buffer_size % OE_EDGER8R_BUFFER_ALIGNMENT) != 0)
        OE_RAISE(OE_INVALID_PARAMETER);

    if ((args.output_buffer_size % OE_EDGER8R_BUFFER_ALIGNMENT) != 0)
        OE_RAISE(OE_INVALID_PARAMETER);

    OE_CHECK(oe_safe_add_u64(
        args.input_buffer_size, args.output_buffer_size, &buffer_size));

    // Resolve which ecall table to use.
    if (args_ptr->table_id == OE_UINT64_MAX)
    {
        ecall_table.ecalls = __oe_ecalls_table;
        ecall_table.num_ecalls = __oe_ecalls_table_size;
    }
    else
    {
        if (args_ptr->table_id >= OE_MAX_ECALL_TABLES)
            OE_RAISE(OE_NOT_FOUND);

        ecall_table.ecalls = _ecall_tables[args_ptr->table_id].ecalls;
        ecall_table.num_ecalls = _ecall_tables[args_ptr->table_id].num_ecalls;

        if (!ecall_table.ecalls)
            OE_RAISE(OE_NOT_FOUND);
    }

    // Fetch matching function.
    if (args.function_id >= ecall_table.num_ecalls)
        OE_RAISE(OE_NOT_FOUND);

    func = ecall_table.ecalls[args.function_id];

    if (func == NULL)
        OE_RAISE(OE_NOT_FOUND);

    // Allocate buffers in enclave memory
    buffer = input_buffer = ve_malloc(buffer_size);
    if (buffer == NULL)
        OE_RAISE(OE_OUT_OF_MEMORY);

    // Copy input buffer to enclave buffer.
    ve_memcpy(input_buffer, args.input_buffer, args.input_buffer_size);

    // Clear out output buffer.
    // This ensures reproducible behavior if say the function is reading from
    // output buffer.
    output_buffer = buffer + args.input_buffer_size;
    ve_memset(output_buffer, 0, args.output_buffer_size);

    // Call the function.
    func(
        input_buffer,
        args.input_buffer_size,
        output_buffer,
        args.output_buffer_size,
        &output_bytes_written);

    // The output_buffer is expected to point to a marshaling struct,
    // whose first field is an oe_result_t. The function is expected
    // to fill this field with the status of the ecall.
    result = *(oe_result_t*)output_buffer;

    if (result == OE_OK)
    {
        // Copy outputs to host memory.
        ve_memcpy(args.output_buffer, output_buffer, output_bytes_written);

        // The ecall succeeded.
        args_ptr->output_bytes_written = output_bytes_written;
        args_ptr->result = OE_OK;
    }

done:
    if (buffer)
        ve_free(buffer);

    return result;
}

int ve_handle_call_ecall(int fd, ve_call_buf_t* buf, int* exit_status)
{
    uint16_t func = (uint16_t)buf->arg1;
    uint64_t arg_in = buf->arg2;

    OE_UNUSED(fd);
    OE_UNUSED(exit_status);

    switch (func)
    {
        case OE_ECALL_CALL_ENCLAVE_FUNCTION:
        {
            buf->retval = (uint64_t)_handle_call_enclave_function(arg_in);
            return 0;
        }
        default:
        {
            return -1;
        }
    }
}

oe_result_t oe_ocall(uint16_t func, uint64_t arg_in, uint64_t* arg_out)
{
    oe_result_t result = OE_UNEXPECTED;
    int sock = __ve_thread_sock_tls;

    if (sock < 0)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (ve_call2(sock, VE_FUNC_OCALL, arg_out, func, (uint64_t)arg_in) != 0)
        OE_RAISE(OE_FAILURE);

    result = OE_OK;

done:
    return result;
}

oe_result_t oe_call_host_function_by_table_id(
    uint64_t table_id,
    uint64_t function_id,
    const void* input_buffer,
    size_t input_buffer_size,
    void* output_buffer,
    size_t output_buffer_size,
    size_t* output_bytes_written)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_call_host_function_args_t* args = NULL;

    /* Reject invalid parameters */
    if (!input_buffer || input_buffer_size == 0)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Initialize the arguments */
    {
        if (!(args = oe_host_calloc(1, sizeof(*args))))
        {
            /* Fail if the enclave is crashing. */
            // ATTN: OE_CHECK(__oe_enclave_status);
            OE_RAISE(OE_OUT_OF_MEMORY);
        }

        args->table_id = table_id;
        args->function_id = function_id;
        args->input_buffer = input_buffer;
        args->input_buffer_size = input_buffer_size;
        args->output_buffer = output_buffer;
        args->output_buffer_size = output_buffer_size;
        args->result = OE_UNEXPECTED;
    }

    /* Call the host function with this address */
    OE_CHECK(oe_ocall(OE_OCALL_CALL_HOST_FUNCTION, (uint64_t)args, NULL));

    /* Check the result */
    OE_CHECK(args->result);

    *output_bytes_written = args->output_bytes_written;
    result = OE_OK;

done:

    oe_host_free(args);

    return result;
}

oe_result_t oe_call_host_function(
    size_t function_id,
    const void* input_buffer,
    size_t input_buffer_size,
    void* output_buffer,
    size_t output_buffer_size,
    size_t* output_bytes_written)
{
    return oe_call_host_function_by_table_id(
        OE_UINT64_MAX,
        function_id,
        input_buffer,
        input_buffer_size,
        output_buffer,
        output_buffer_size,
        output_bytes_written);
}

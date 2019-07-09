// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/properties.h>
#include <openenclave/bits/safemath.h>
#include <openenclave/corelibc/errno.h>
#include <openenclave/corelibc/stdio.h>
#include <openenclave/edger8r/common.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/globals.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/rdrand.h>
#include <openenclave/internal/sgxtypes.h>
#include <openenclave/internal/thread.h>
#include "../common/msg.h"
#include "calls.h"
#include "futex.h"
#include "globals.h"
#include "hexdump.h"
#include "hostheap.h"
#include "io.h"
#include "lock.h"
#include "malloc.h"
#include "print.h"
#include "process.h"
#include "sbrk.h"
#include "socket.h"
#include "string.h"
#include "thread.h"
#include "time.h"
#include "trace.h"

extern const oe_ecall_func_t __oe_ecalls_table[];
extern const size_t __oe_ecalls_table_size;

extern int __ve_proxy_sock;
static ve_lock_t _proxy_sock_lock;

/* Opaque enclave pointer passed by the host. */
static oe_enclave_t* _enclave;

typedef struct _ecall_table
{
    const oe_ecall_func_t* ecalls;
    size_t num_ecalls;
} ecall_table_t;

static ecall_table_t _ecall_tables[OE_MAX_ECALL_TABLES];
static ve_lock_t _ecall_tables_lock;

void oe_abort(void)
{
    ve_abort();
}

void oe_sleep_msec(uint64_t milliseconds)
{
    struct ve_timespec ts;
    const struct ve_timespec* req = &ts;
    struct ve_timespec rem = {0, 0};

    ts.tv_sec = (time_t)(milliseconds / 1000);
    ts.tv_nsec = (long)((milliseconds % 1000) * 1000000);

    while (ve_nanosleep(req, &rem) == -1)
        req = &rem;
}

uint64_t oe_get_time(void)
{
    struct ve_timespec ts;

    if (ve_clock_gettime(VE_CLOCK_REALTIME, &ts) != 0)
        return 0;

    return ((uint64_t)ts.tv_sec * 1000) + ((uint64_t)ts.tv_nsec / 1000000);
}

time_t oe_time(time_t* tloc)
{
    uint64_t msec = oe_get_time() / 1000;
    time_t time = (time_t)(msec > OE_LONG_MAX ? OE_LONG_MAX : msec);

    if (tloc)
        *tloc = time;

    return time;
}

oe_enclave_t* oe_get_enclave(void)
{
    return _enclave;
}

// Function used by oeedger8r for allocating ocall buffers.
void* oe_allocate_ocall_buffer(size_t size)
{
    void* ptr;

    if ((ptr = oe_host_malloc(size)))
        ve_memset(ptr, 0, size);

    return ptr;
}

// Function used by oeedger8r for freeing ocall buffers.
void oe_free_ocall_buffer(void* buffer)
{
    oe_host_free(buffer);
}

bool oe_is_within_enclave(const void* ptr, size_t size)
{
    const uint64_t min = (uint64_t)ptr;
    const uint64_t max = (uint64_t)ptr + size;
    const uint64_t lo = (uint64_t)ve_host_heap_get_start();
    const uint64_t hi = (uint64_t)ve_host_heap_get_end();

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
    const uint64_t lo = (uint64_t)ve_host_heap_get_start();
    const uint64_t hi = (uint64_t)ve_host_heap_get_end();

    if (min < lo || min >= hi)
        return false;

    if (max < lo || max > hi)
        return false;

    return true;
}

static oe_result_t _oe_enclave_status = OE_OK;

oe_result_t oe_get_enclave_status(void)
{
    return _oe_enclave_status;
}

static log_level_t _active_log_level = OE_LOG_LEVEL_ERROR;

oe_result_t oe_log(log_level_t level, const char* fmt, ...)
{
    if (level <= _active_log_level)
    {
        oe_va_list ap;

        oe_va_start(ap, fmt);
        ve_printf("oe_log: %u: ", level);
        ve_vprintf(fmt, ap);
        oe_va_end(ap);
    }

    return OE_OK;
}

log_level_t get_current_logging_level(void)
{
    return _active_log_level;
}

typedef struct _oe_thread_data oe_thread_data_t;
typedef struct _td td_t;

void* td_to_tcs(const td_t* td)
{
    /* ve_thread_t start on the page preceeding the td. */
    return (void*)((uint8_t*)td - OE_PAGE_SIZE);
}

oe_thread_data_t* oe_get_thread_data(void)
{
    return (oe_thread_data_t*)(ve_thread_get_extra(ve_thread_self()));
}

static void _handle_thread_wait_ocall(uint64_t arg_in)
{
    ve_thread_t thread = (ve_thread_t)arg_in;
    volatile int* value = ve_thread_get_futex_addr(thread);

    if (__sync_fetch_and_add(value, -1) == 0)
    {
        /* Wait while ignoring any spurious wakes. */
        do
        {
            ve_futex_wait(value, -1, 1);

        } while (*value == -1);
    }
}

static void _handle_thread_wake_ocall(uint64_t arg_in)
{
    ve_thread_t thread = (ve_thread_t)arg_in;
    volatile int* value = ve_thread_get_futex_addr(thread);

    if (__sync_fetch_and_add(value, 1) != 0)
    {
        ve_futex_wake(value, 1, 1);
    }
}

static void _handle_thread_wake_wait_ocall(uint64_t arg_in)
{
    oe_thread_wake_wait_args_t* args = (oe_thread_wake_wait_args_t*)arg_in;

    if (!args)
        return;

    _handle_thread_wake_ocall((uint64_t)args->waiter_tcs);
    _handle_thread_wait_ocall((uint64_t)args->self_tcs);
}

oe_result_t oe_ocall(uint16_t func, uint64_t arg_in, uint64_t* arg_out)
{
    oe_result_t result = OE_UNEXPECTED;

    OE_UNUSED(arg_out);

    switch (func)
    {
        case OE_OCALL_THREAD_WAIT:
        {
            _handle_thread_wait_ocall(arg_in);
            break;
        }
        case OE_OCALL_THREAD_WAKE:
        {
            _handle_thread_wake_ocall(arg_in);
            break;
        }
        case OE_OCALL_THREAD_WAKE_WAIT:
        {
            _handle_thread_wake_wait_ocall(arg_in);
            break;
        }
        case OE_OCALL_GET_QE_TARGET_INFO:
        case OE_OCALL_GET_QUOTE:
        {
            const int sock = ve_get_sock();
            uint64_t retval = 0;

            if (ve_call2(sock, VE_FUNC_OCALL, &retval, func, arg_in) != 0)
                OE_RAISE(OE_FAILURE);

            if (retval != 0)
                OE_RAISE(OE_FAILURE);

            break;
        }
        default:
        {
            OE_RAISE(OE_NOT_FOUND);
        }
    }

    result = OE_OK;

done:
    return result;
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
    buffer = input_buffer = ve_calloc(1, buffer_size);
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

int ve_handle_init_enclave(int fd, ve_call_buf_t* buf, int* exit_status)
{
    OE_UNUSED(fd);
    OE_UNUSED(exit_status);

    _enclave = (oe_enclave_t*)buf->arg1;
    buf->retval = 0;

    return 0;
}

int ve_handle_call_enclave_function(
    int fd,
    ve_call_buf_t* buf,
    int* exit_status)
{
    OE_UNUSED(fd);
    OE_UNUSED(exit_status);

    buf->retval = (uint64_t)_handle_call_enclave_function(buf->arg1);
    return 0;
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

    if (output_buffer && output_buffer_size)
        ve_memset(output_buffer, 0, output_buffer_size);

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

    /* Call the host function with these arguments. */
    {
        int sock = ve_get_sock();
        ve_func_t func = VE_FUNC_CALL_HOST_FUNCTION;
        uint64_t retval = 0;

        if (ve_call1(sock, func, &retval, (uint64_t)args) != 0)
            OE_RAISE(OE_FAILURE);

        if (retval != 0)
            OE_RAISE(OE_FAILURE);
    }

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

extern volatile const oe_sgx_enclave_properties_t oe_enclave_properties_sgx;

int ve_handle_get_settings(int fd, ve_call_buf_t* buf, int* exit_status)
{
    ve_enclave_settings_t* settings = (ve_enclave_settings_t*)buf->arg1;

    OE_UNUSED(fd);
    OE_UNUSED(exit_status);

    if (settings)
    {
        settings->num_heap_pages =
            oe_enclave_properties_sgx.header.size_settings.num_heap_pages;

        settings->num_stack_pages =
            oe_enclave_properties_sgx.header.size_settings.num_stack_pages;

        settings->num_tcs =
            oe_enclave_properties_sgx.header.size_settings.num_tcs;

        buf->retval = 0;
    }
    else
    {
        buf->retval = (uint64_t)-1;
    }

    return 0;
}

int oe_host_write(int device, const char* str, size_t len)
{
    int ret = -1;
    int fd;

    switch (device)
    {
        case 0:
            fd = VE_STDOUT_FILENO;
            break;
        case 1:
            fd = VE_STDERR_FILENO;
            break;
        default:
        {
            goto done;
        }
    }

    if (len == (size_t)-1)
        len = ve_strlen(str);

    if (ve_write(fd, str, len) != (ssize_t)len)
        goto done;

    ret = 0;

done:
    return ret;
}

int oe_host_vfprintf(int device, const char* fmt, oe_va_list ap_)
{
    int ret = -1;
    int n;
    char buf[256];
    char* p = buf;

    /* Try first with a fixed-length scratch buffer */
    {
        oe_va_list ap;
        oe_va_copy(ap, ap_);
        n = oe_vsnprintf(buf, sizeof(buf), fmt, ap);
        oe_va_end(ap);
    }

    /* If string was truncated, retry with correctly sized buffer */
    if (n >= (int)sizeof(buf))
    {
        if (!(p = ve_malloc((uint32_t)n + 1)))
            goto done;

        oe_va_list ap;
        oe_va_copy(ap, ap_);
        n = oe_vsnprintf(p, (size_t)n + 1, fmt, ap);
        oe_va_end(ap);
    }

    if (oe_host_write(device, p, (size_t)-1) != 0)
        goto done;

    ret = n;

done:

    if (p != buf)
        ve_free(p);

    return ret;
}

int oe_host_fprintf(int device, const char* fmt, ...)
{
    int n;

    oe_va_list ap;
    oe_va_start(ap, fmt);
    n = oe_host_vfprintf(device, fmt, ap);
    oe_va_end(ap);

    return n;
}

int oe_host_printf(const char* fmt, ...)
{
    oe_va_list ap;
    oe_va_start(ap, fmt);
    ve_vprintf(fmt, ap);
    oe_va_end(ap);

    return 0;
}

void* oe_malloc(size_t size)
{
    return ve_malloc(size);
}

void oe_free(void* ptr)
{
    return ve_free(ptr);
}

void* oe_calloc(size_t nmemb, size_t size)
{
    return ve_calloc(nmemb, size);
}

void* oe_realloc(void* ptr, size_t size)
{
    return ve_realloc(ptr, size);
}

void* oe_memalign(size_t alignment, size_t size)
{
    return ve_memalign(alignment, size);
}

int oe_posix_memalign(void** memptr, size_t alignment, size_t size)
{
    return ve_posix_memalign(memptr, alignment, size);
}

/* Non-secure replacement for RDRAND, which crashes Valgrind. */
uint64_t oe_rdrand(void)
{
    union {
        uint64_t word;
        uint8_t bytes[sizeof(uint64_t)];
    } r;

    /* Initialize all bytes with an increasing counter. */
    {
        static uint8_t _counter = 0;
        static oe_spinlock_t _lock = OE_SPINLOCK_INITIALIZER;

        oe_spin_lock(&_lock);

        for (size_t i = 0; i < sizeof(uint64_t); i++)
            r.bytes[i] = _counter++;

        oe_spin_unlock(&_lock);
    }

    /* Initialize each byte with a function of the time. */
    for (size_t i = 0; i < sizeof(uint64_t); i++)
    {
        uint8_t xor = 0;
        struct ve_timespec ts;

        if (ve_clock_gettime(VE_CLOCK_REALTIME, &ts) != 0)
            oe_abort();

        const uint8_t* p = (const uint8_t*)&ts;
        const uint8_t* end = p + sizeof(ts);

        while (p != end)
            xor ^= *p++;

        r.bytes[i] = xor;
    }

    return r.word;
}

int* __oe_errno_location(void)
{
    static __thread int _errno = 0;
    return &_errno;
}

void* oe_sbrk(intptr_t increment)
{
    return ve_sbrk(increment);
}

const void* __oe_get_enclave_base(void)
{
    return ve_get_baseaddr();
}

const void* __oe_get_enclave_elf_header(void)
{
    return ve_get_elf_header();
}

int* __h_errno_location(void)
{
    return __oe_errno_location();
}

uint64_t oe_egetkey(
    const sgx_key_request_t* sgx_key_request,
    sgx_key_t* sgx_key)
{
    uint64_t ret = SGX_EGETKEY_INVALID_ATTRIBUTE;
    const int sock = __ve_proxy_sock;
    ve_egetkey_request_t req;
    ve_egetkey_response_t rsp;
    bool locked = false;

    if (!sgx_key_request || !sgx_key)
        goto done;

    ve_memcpy(&req.request, sgx_key_request, sizeof(req.request));
    ve_memset(&rsp, 0, sizeof(rsp));

    ve_lock(&_proxy_sock_lock);
    locked = true;

    if (ve_msg_send(sock, VE_MSG_EGETKEY, &req, sizeof(req)) != 0)
        goto done;

    if (ve_msg_recv(sock, VE_MSG_EGETKEY, &rsp, sizeof(rsp)) != 0)
        goto done;

    ve_memcpy(sgx_key, &rsp.key, sizeof(sgx_key_t));
    ret = rsp.ret;

done:

    if (locked)
        ve_unlock(&_proxy_sock_lock);

    return ret;
}

oe_result_t oe_ereport(
    sgx_target_info_t* target_info,
    sgx_report_data_t* report_data,
    sgx_report_t* report)
{
    oe_result_t result = OE_UNEXPECTED;
    const int sock = __ve_proxy_sock;
    ve_ereport_request_t req;
    ve_ereport_response_t rsp;
    bool locked = false;

    if (!target_info || !report_data || !report)
        OE_RAISE(OE_INVALID_PARAMETER);

    ve_memcpy(&req.target_info, target_info, sizeof(req.target_info));
    ve_memcpy(&req.report_data, report_data, sizeof(req.report_data));
    ve_memset(&rsp, 0, sizeof(rsp));

    if (ve_msg_send(sock, VE_MSG_EREPORT, &req, sizeof(req)) != 0)
        goto done;

    if (ve_msg_recv(sock, VE_MSG_EREPORT, &rsp, sizeof(rsp)) != 0)
        goto done;

    ve_memcpy(report, &rsp.report, sizeof(sgx_report_t));

    result = (oe_result_t)rsp.result;

done:

    if (locked)
        ve_unlock(&_proxy_sock_lock);

    return result;
}

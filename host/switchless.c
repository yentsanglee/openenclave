// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "switchless.h"
#include <openenclave/host.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/utils.h>
#include "enclave.h"

#if defined(__linux__)
#include <pthread.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <Windows.h>
#else
#error "unsupported platform"
#endif

#define OE_SPIN_LOCK_ACQUIRE(lock) while (__sync_lock_test_and_set(lock, 1))

#define OE_SPIN_LOCK_RELEASE(lock) \
    do                             \
    {                              \
        __sync_lock_release(lock); \
    } while (0)

#define OE_ATOMIC_INCREMENT(value) __sync_fetch_and_add(value, 1)

#define OE_ATOMIC_DECREMENT(value) __sync_fetch_and_sub(value, 1)

#define OE_ATOMIC_COMPARE_AND_SET(var, oldvalue, newvalue) \
    __sync_bool_compare_and_swap(var, oldvalue, newvalue)

oe_result_t oe_configure_switchless_calling(
    oe_enclave_t* enclave,
    uint32_t num_enclave_workers,
    uint32_t num_host_workers)
{
    oe_result_t result = OE_UNEXPECTED;
    if (enclave == NULL)
        OE_RAISE(OE_INVALID_PARAMETER);

    OE_SPIN_LOCK_ACQUIRE(&enclave->switchless.lock);

    if (num_enclave_workers > OE_SGX_MAX_TCS)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (num_host_workers > OE_SGX_MAX_TCS)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (enclave->switchless.configured)
        OE_RAISE(OE_FAILURE);

    enclave->switchless.max_num_enclave_workers = num_enclave_workers;
    enclave->switchless.max_num_host_workers = num_host_workers;
    enclave->switchless.configured = true;
    result = OE_OK;

done:
    if (enclave)
        OE_SPIN_LOCK_RELEASE(&enclave->switchless.lock);

    return result;
}

static void* _launch_enclave_worker(void* arg)
{
    oe_enclave_t* enclave = (oe_enclave_t*)arg;

    uint32_t index =
        OE_ATOMIC_INCREMENT(&enclave->switchless.num_active_enclave_workers);
    oe_switchless_context_t* context =
        &enclave->switchless.ecall_contexts[index];

    oe_ecall(enclave, OE_ECALL_LAUNCH_ENCLAVE_WORKER, (uint64_t)context, NULL);
    OE_ATOMIC_DECREMENT(&enclave->switchless.num_active_enclave_workers);
    context->shutdown_complete = true;
    return NULL;
}

static void _terminate_worker_threads(oe_enclave_t* enclave)
{
    // Terminate any created threads.
    for (size_t i = 0; i < enclave->switchless.max_num_enclave_workers; ++i)
    {
        if (enclave->switchless.enclave_workers[i])
        {
            enclave->switchless.ecall_contexts[i].shutdown_initiated = true;
        }
    }
    for (size_t i = 0; i < enclave->switchless.max_num_enclave_workers; ++i)
    {
        if (enclave->switchless.enclave_workers[i])
        {
            pthread_join(enclave->switchless.enclave_workers[i], NULL);
        }
    }
}

oe_result_t oe_initialize_switchless(oe_enclave_t* enclave)
{
    oe_result_t result = OE_UNEXPECTED;
    bool locked = false;
    if (enclave == NULL)
        OE_RAISE(OE_INVALID_PARAMETER);

    // Double-checked locking pattern using acquire-release semantics.
    OE_ATOMIC_MEMORY_BARRIER_ACQUIRE();
    if (!enclave->switchless.initialized)
    {
        // After the test after acquire, lock and retest.
        OE_SPIN_LOCK_ACQUIRE(&enclave->switchless.lock);
        locked = true;

        if (!enclave->switchless.initialized)
        {
            // Ensure that switchless has been configured.
            if (!enclave->switchless.configured)
                OE_RAISE(OE_FAILURE);

            // Create enclave worker threads.
            for (size_t i = 0; i < enclave->switchless.max_num_enclave_workers;
                 ++i)
            {
                if (!enclave->switchless.enclave_workers[i])
                {
                    if (pthread_create(
                            &enclave->switchless.enclave_workers[i],
                            NULL,
                            _launch_enclave_worker,
                            enclave) != 0)
                    {
                        _terminate_worker_threads(enclave);
                        OE_RAISE(OE_FAILURE);
                        goto done;
                    }
                }
            }

            // The initialized field is accessed without a lock, using
            // the acquire-release lock-free programming pattern.
            OE_ATOMIC_MEMORY_BARRIER_RELEASE();
            enclave->switchless.initialized = true;
        }
    }
    result = OE_OK;

done:
    if (locked)
        OE_SPIN_LOCK_RELEASE(&enclave->switchless.lock);

    return result;
}

oe_result_t oe_terminate_switchless(oe_enclave_t* enclave)
{
    oe_result_t result = OE_UNEXPECTED;

    if (enclave == NULL)
        OE_RAISE(OE_INVALID_PARAMETER);

    // TODO: Clarify what happens when a switchless ecall is made
    // in while terminate is going on.
    OE_SPIN_LOCK_ACQUIRE(&enclave->switchless.lock);

    _terminate_worker_threads(enclave);

    OE_SPIN_LOCK_RELEASE(&enclave->switchless.lock);

    result = OE_OK;
done:

    return result;
}

oe_result_t oe_switchless_call_enclave_function(
    oe_enclave_t* enclave,
    uint32_t function_id,
    void* input_buffer,
    size_t input_buffer_size,
    void* output_buffer,
    size_t output_buffer_size,
    size_t* output_bytes_written)
{
    oe_result_t result = OE_UNEXPECTED;
    oe_call_enclave_function_args_t args;
    oe_switchless_context_t* slot = NULL;

    /* Reject invalid parameters */
    if (!enclave)
        OE_RAISE(OE_INVALID_PARAMETER);

    /* Initialize the call_enclave_args structure */
    {
        args.function_id = function_id;
        args.input_buffer = input_buffer;
        args.input_buffer_size = input_buffer_size;
        args.output_buffer = output_buffer;
        args.output_buffer_size = output_buffer_size;
        args.output_bytes_written = 0;
        args.result = __OE_RESULT_MAX;

        // Use acquire-release semantics to ensure that
        // all the values to args are written at this point
        // so that the enclave worker thread sees the fully
        // constructed args.
        OE_ATOMIC_MEMORY_BARRIER_RELEASE();
    }

    // Use lock-free test to check if initialization must be done.
    OE_ATOMIC_MEMORY_BARRIER_ACQUIRE();
    if (!enclave->switchless.initialized)
    {
        OE_CHECK(oe_initialize_switchless(enclave));
    }

    // Wait for a free slot
    while (!slot)
    {
        for (size_t i = 0; i < enclave->switchless.max_num_enclave_workers; ++i)
        {
            OE_ATOMIC_MEMORY_BARRIER_ACQUIRE();
            if (enclave->switchless.ecall_contexts[i].args == NULL)
            {
                // Try grabbing the slot atomically if it is free.
                // If contexts[i].args is NULL, then the slot is free.
                if (OE_ATOMIC_COMPARE_AND_SET(
                        &enclave->switchless.ecall_contexts[i].args,
                        NULL,
                        &args))
                {
                    // Successfully grabbed the slot.
                    slot = &enclave->switchless.ecall_contexts[i];
                    break;
                }
            }
        }
    }

    // Wait for call to complete.
    while (true)
    {
        OE_ATOMIC_MEMORY_BARRIER_ACQUIRE();
        if (args.result != __OE_RESULT_MAX)
        {
            break;
        }
    }

    /* Check the result */
    OE_CHECK(args.result);

    *output_bytes_written = args.output_bytes_written;
    result = OE_OK;

done:
    return result;
}

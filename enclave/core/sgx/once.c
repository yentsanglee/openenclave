// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/utils.h>

#define SPINLOCK_UNINITIALIZED 0
#define SPINLOCK_BUSY 1
#define SPINLOCK_USED 2


oe_result_t oe_once(oe_once_t* once, void (*func)(void))
{
    if (!once)
        return OE_INVALID_PARAMETER;

    /* Double checked locking (DCLP). */
    /* DCLP Acquire barrier. */
    OE_ATOMIC_MEMORY_BARRIER_ACQUIRE();
    if (*once != SPINLOCK_USED)
    {
        oe_once_t retval = __sync_val_compare_and_swap(once, SPINLOCK_UNINITIALIZED, SPINLOCK_BUSY);
        if (retval == SPINLOCK_UNINITIALIZED)
        {
            if (func)
                func();

            OE_ATOMIC_MEMORY_BARRIER_RELEASE();
            *once = SPINLOCK_USED;  
        }
        else if (retval == SPINLOCK_BUSY)
        {
            while (__sync_val_compare_and_swap(once, SPINLOCK_BUSY, SPINLOCK_BUSY) != SPINLOCK_BUSY)
            {
               asm volatile("pause");
            }
        }
    }

    return OE_OK;
}

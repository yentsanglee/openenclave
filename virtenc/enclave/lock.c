// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "lock.h"
#include <openenclave/bits/defs.h>
#include <openenclave/bits/types.h>

/* Set the lock value to 1 and return the old value */
static unsigned int _spin_set_locked(ve_lock_t* lock)
{
    unsigned int value = 1;

    asm volatile("lock xchg %0, %1;"
                 : "=r"(value) /* %0 */
                 : "m"(*lock), /* %1 */
                   "0"(value)  /* also %2 */
                 : "memory");

    return value;
}

int ve_lock(ve_lock_t* lock)
{
    if (!lock)
        return -1;

    while (_spin_set_locked((volatile unsigned int*)lock) != 0)
    {
        /* Spin while waiting for lock to be released (become 1) */
        while (*lock)
        {
            /* Yield to CPU */
            asm volatile("pause");
        }
    }

    return 0;
}

int ve_unlock(ve_lock_t* lock)
{
    if (!lock)
        return -1;

    asm volatile("movl %0, %1;"
                 :
                 : "r"(0), "m"(*lock) /* %1 */
                 : "memory");

    return 0;
}

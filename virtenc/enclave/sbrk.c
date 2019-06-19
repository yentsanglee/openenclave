// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "sbrk.h"
#include "lock.h"
#include "syscall.h"

void* ve_sbrk(intptr_t increment)
{
    void* ret = (void*)-1;
    static void* _brk_value;
    static ve_lock_t _lock;

    ve_lock(&_lock);

    /* Initialize the first time. */
    if (_brk_value == NULL)
    {
        long rval;

        if ((rval = ve_syscall(OE_SYS_brk, 0)) == -1)
            goto done;

        _brk_value = (void*)rval;
    }

    /* If increment is zero, then return the current break value. */
    if (increment == 0)
    {
        ret = _brk_value;
        goto done;
    }

    /* Increment the break value and return the old break value. */
    {
        long rval;
        void* old_brk_value;

        if ((rval = ve_syscall(OE_SYS_brk, _brk_value + increment)) == -1)
            goto done;

        old_brk_value = _brk_value;
        _brk_value = (void*)rval;

        ret = old_brk_value;
        goto done;
    }

done:

    ve_unlock(&_lock);

    return ret;
}

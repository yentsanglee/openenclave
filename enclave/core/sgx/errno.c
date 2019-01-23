// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

int* __errno_location(void)
{
    td_t* td = (td_t*)oe_get_thread_data();
    oe_assert(td);
    return &td->linux_errno;
}

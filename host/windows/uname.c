// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/corelibc/sys/utsname.h>
#include <string.h>
#include "../ocalls.h"

void oe_handle_uname(uint64_t arg_in, uint64_t* arg_out)
{
    struct oe_utsname* out = (struct oe_utsname*)arg_in;

    if (!out)
    {
        if (arg_out)
        {
            *arg_out = OE_EFAULT;
            return;
        }
    }

    /* ATTN: Provide a more complete implemenation for Windows. */
    memset(out, 0, sizeof(struct oe_utsname));
    strcpy(out->sysname, "Windows");
    strcpy(out->nodename, "unknown");
    strcpy(out->release, "unknown");
    strcpy(out->machine, "x86_64");
    strcpy(out->version, "unknown");

    if (arg_out)
        *arg_out = 0;
}

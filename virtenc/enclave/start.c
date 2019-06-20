// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include "clone.h"
#include "exit.h"
#include "globals.h"
#include "malloc.h"
#include "msg.h"
#include "put.h"
#include "sbrk.h"
#include "time.h"

void ve_call_init_functions(void);
void ve_call_fini_functions(void);

__attribute__((constructor)) static void constructor(void)
{
    // ve_put("constructor()\n");
}

static int _main(void)
{
    ve_put("main()\n");

    ve_malloc(1);

    /* Wait here to be initialized and to receive the main socket. */
    if (ve_handle_init() != 0)
        ve_put_err("ve_handle_init() failed");

    /* Handle messages over the main socket. */
    if (ve_handle_messages() != 0)
        ve_put_err("ve_handle_message() failed");

    return 0;
}

#if defined(VE_STATIC)
void _start(void)
{
    ve_call_init_functions();
    ve_exit(_main());
    ve_call_fini_functions();
}
#else
int main(void)
{
    ve_exit(_main());
}
#endif

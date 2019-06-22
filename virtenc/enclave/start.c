// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include <openenclave/internal/syscall/unistd.h>
#include "globals.h"
#include "io.h"
#include "malloc.h"
#include "msg.h"
#include "print.h"
#include "process.h"
#include "sbrk.h"
#include "time.h"
#include "trace.h"

void ve_call_init_functions(void);
void ve_call_fini_functions(void);

static bool _called_constructor = false;

__attribute__((constructor)) static void constructor(void)
{
    _called_constructor = true;
}

static int _main(void)
{
    if (!_called_constructor)
    {
        ve_puts("constructor not called");
        ve_exit(1);
    }

    /* Wait here to be initialized and to receive the main socket. */
    if (ve_handle_init() != 0)
    {
        ve_puts("ve_handle_init() failed");
        ve_exit(1);
    }

    /* Handle messages over the main socket. */
    if (ve_handle_calls(globals.sock) != 0)
    {
        ve_puts("enclave: ve_handle_calls() failed");
        ve_exit(1);
    }

    return 0;
}

#if defined(BUILD_STATIC)
void _start(void)
{
    ve_call_init_functions();
    int rval = _main();
    ve_call_fini_functions();
    ve_exit(rval);
}
#else
int main(void)
{
    ve_exit(_main());
}
#endif

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include <openenclave/internal/syscall/unistd.h>
#include "clone.h"
#include "close.h"
#include "exit.h"
#include "globals.h"
#include "malloc.h"
#include "msg.h"
#include "put.h"
#include "sbrk.h"
#include "time.h"

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
        ve_put_err("constructor not called");
        ve_exit(1);
    }

    /* Wait here to be initialized and to receive the main socket. */
    if (ve_handle_init() != 0)
        ve_put_err("ve_handle_init() failed");

    /* Handle messages over the main socket. */
    if (ve_handle_calls(globals.sock) != 0)
        ve_put_err("enclave: ve_handle_calls() failed");

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

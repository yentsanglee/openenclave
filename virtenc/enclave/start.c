// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include "alloc.h"
#include "clone.h"
#include "exit.h"
#include "globals.h"
#include "msg.h"
#include "put.h"
#include "sbrk.h"
#include "time.h"

void ve_call_init_functions(void);
void ve_call_fini_functions(void);

__attribute__((constructor)) void _constructor(void)
{
}

__attribute__((destructor)) void _destructor(void)
{
}

int child(void* arg)
{
    ve_put("***** child()\n");
    ve_sleep(5);
    return 99;
}

#define STACK_SIZE ((1024 * 1024) / 8)

OE_ALIGNED(4096)
uint64_t _stack[STACK_SIZE];

void _start(void)
{
    ve_call_init_functions();

    /* Wait here to be initialized and to receive the socket. */
    if (ve_handle_init() != 0)
        ve_put_err("ve_handle_init() failed");

    /* Handle messages over socket channel. */
    if (ve_handle_messages() != 0)
        ve_put_err("ve_handle_message() failed");

    ve_call_fini_functions();
    ve_exit(0);
}

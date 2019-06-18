// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include "clone.h"
#include "exit.h"
#include "globals.h"
#include "msg.h"
#include "print.h"
#include "time.h"

globals_t globals;

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
    ve_print("***** child()\n");
    ve_sleep(5);
    return 99;
}

#define STACK_SIZE ((1024 * 1024) / 8)

OE_ALIGNED(4096)
uint64_t _stack[STACK_SIZE];

#if 1
void _start()
{
    ve_call_init_functions();

    /* Wait here to be initialized and to receive the socket. */
    if (ve_handle_init() != 0)
        ve_print_err("ve_handle_init() failed");

    /* Handle messages over socket channel. */
    if (ve_handle_messages() != 0)
        ve_print_err("ve_handle_message() failed");

    // ve_exit(0);

    ve_call_fini_functions();
    ve_exit(0);
}
#else
void _start()
{
    ve_call_init_functions();

    ve_print_int_msg("xxxxx=", 12345);

    ve_print("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
    long rval = ve_clone(child, &_stack[STACK_SIZE], NULL);
    ve_print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    ve_print_int_msg("clone.rval=", rval);
    ve_sleep(3);

    ve_call_fini_functions();
    ve_exit(99);
}
#endif

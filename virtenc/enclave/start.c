#include "exit.h"
#include "globals.h"
#include "msg.h"
#include "print.h"

globals_t globals;

void ve_call_init_functions(void);
void ve_call_fini_functions(void);

__attribute__((constructor)) void _constructor(void)
{
}

__attribute__((destructor)) void _destructor(void)
{
}

void _start()
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

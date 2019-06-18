#include "exit.h"
#include "msg.h"
#include "print.h"

void ve_call_init_functions(void);
void ve_call_fini_functions(void);

__attribute__((constructor)) void _constructor(void)
{
    // ve_print_str("_constructor()\n");
}

__attribute__((destructor)) void _destructor(void)
{
    // ve_print_str("_destructor()\n");
}

void _start()
{
    ve_call_init_functions();

    // ve_print_str("_start()\n");

    if (ve_handle_messages() != 0)
    {
        ve_print_str("ve_handle_message() failed\n");
    }

    ve_call_fini_functions();
    ve_exit(99);
}

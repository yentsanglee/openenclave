#include "exit.h"
#include "msg.h"
#include "print.h"

void ve_call_init_functions(void);
void ve_call_fini_functions(void);

__attribute__((constructor)) void _constructor(void)
{
    ve_print_str("_constructor()\n");
}

__attribute__((destructor)) void _destructor(void)
{
    ve_print_str("_destructor()\n");
}

void _start()
{
    ve_call_init_functions();

    ve_print_str("_start()\n");

    ve_print_oct(0777);
    ve_print_nl();
    ve_print_uint(99);
    ve_print_nl();
    ve_print_int(-123);
    ve_print_nl();
    ve_print_hex(0xABCD);
    ve_print_nl();

    ve_msg_print("PRINT1\n");
    ve_msg_print("PRINT2\n");
    ve_msg_print("PRINT3\n");

    ve_call_fini_functions();
    ve_exit(99);
}

#include <freestanding/print.h>
#include <freestanding/exit.h>

void fs_call_init_functions(void);
void fs_call_fini_functions(void);

__attribute__((constructor)) void _constructor(void)
{
    fs_print_str("_constructor()\n");
}

void _start()
{
    fs_call_init_functions();

    fs_print_str("_start()\n");

    fs_print_oct(0777);
    fs_print_nl();
    fs_print_uint(99);
    fs_print_nl();
    fs_print_int(-123);
    fs_print_nl();
    fs_print_hex(0xABCD);
    fs_print_nl();

    fs_call_fini_functions();
    fs_exit(99);
}

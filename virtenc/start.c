#include <freestanding/print.h>
#include <freestanding/exit.h>

void _start()
{
    fs_print_str("_start()\n");
    fs_print_oct(0777);
    fs_print_nl();
    fs_print_uint(99);
    fs_print_nl();
    fs_print_int(-123);
    fs_print_nl();
    fs_print_hex(0xABCD);
    fs_print_nl();

    fs_exit(99);
}

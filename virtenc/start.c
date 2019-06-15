#include <freestanding/print.h>
#include <freestanding/exit.h>

void _start()
{
    fs_puts("_start()");
    fs_put_oct(0777);
    fs_put_dec(99);
    fs_put_hex(0xABCD);
    fs_exit(99);
}

#include <freestanding/print.h>
#include <freestanding/exit.h>

void _start()
{
    fs_put_str("_start()\n");
    fs_put_oct(0777);
    fs_put_nl();
    fs_put_uint(99);
    fs_put_nl();
    fs_put_int(-123);
    fs_put_nl();
    fs_put_hex(0xABCD);
    fs_put_nl();

    fs_exit(99);
}

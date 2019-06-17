#include "msg.h"
#include <openenclave/internal/syscall/unistd.h>
#include "print.h"
#include "string.h"
#include "syscall.h"

ssize_t ve_read(int fd, void* buf, size_t count)
{
    return ve_syscall(OE_SYS_read, fd, (long)buf, (long)count);
}

ssize_t ve_write(int fd, const void* buf, size_t count)
{
    return ve_syscall(OE_SYS_write, fd, (long)buf, (long)count);
}

int ve_msg_print(const char* data)
{
    int ret = -1;
    const int fdin = OE_STDIN_FILENO;
    const int fdout = OE_STDOUT_FILENO;
    size_t size;

    if (!data)
        goto done;

    size = ve_strlen(data);

    if (size)
    {
        ve_msg_print_out_t out;
        const ve_msg_print_in_t* in = (const ve_msg_print_in_t*)data;
        bool eof;

        if (ve_send_msg(fdout, VE_MSG_PRINT_IN, in, size) != 0)
            goto done;

        if (ve_recv_msg_by_type(
                fdin, VE_MSG_PRINT_OUT, &out, sizeof(out), &eof) != 0)
        {
            goto done;
        }

        ret = out.ret;
    }

    ret = 0;

done:
    return ret;
}

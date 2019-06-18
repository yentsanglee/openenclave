#include "msg.h"
#include <openenclave/internal/syscall/unistd.h>
#include "exit.h"
#include "globals.h"
#include "ioctl.h"
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
        const ve_msg_type_t type = VE_MSG_PRINT;
        const ve_msg_print_in_t* in = (const ve_msg_print_in_t*)data;
        bool eof;

        if (ve_send_msg(fdout, type, in, size) != 0)
            goto done;

        if (ve_recv_msg_by_type(fdin, type, &out, sizeof(out), &eof) != 0)
            goto done;

        ret = out.ret;
    }

    ret = 0;

done:
    return ret;
}

void ve_debug(const char* str)
{
    ve_print_str(str);
    ve_print_nl();
}

static int _handle_msg_terminate(size_t size, bool* eof)
{
    int ret = -1;
    ve_msg_terminate_in_t in;
    ve_msg_terminate_out_t out;
    const ve_msg_type_t type = VE_MSG_TERMINATE;

    *eof = false;

    if (size != sizeof(in))
        goto done;

    if (ve_recv_n(OE_STDIN_FILENO, &in, sizeof(in), eof) != 0)
        goto done;

    out.ret = 0;

    if (ve_send_msg(OE_STDOUT_FILENO, type, &out, sizeof(out)) != 0)
        goto done;

    ve_print_str("enclave: terminated\n");
    ve_exit(in.status);

    ret = 0;

done:
    return ret;
}

static int _handle_msg_new_thread(size_t size, bool* eof)
{
    int ret = -1;
    ve_msg_new_thread_in_t in;
    ve_msg_new_thread_out_t out;
    const ve_msg_type_t type = VE_MSG_NEW_THREAD;

    *eof = false;

    if (size != sizeof(in))
        goto done;

    if (ve_recv_n(OE_STDIN_FILENO, &in, sizeof(in), eof) != 0)
        goto done;

#if 0
    {
        int retval;
        struct ve_strrecvfd buf;

        if ((retval = ve_ioctl(OE_STDIN_FILENO, VE_I_RECVFD, &buf)) != 0)
        {
            ve_print_str("ve_ioctl failed\n");
            goto done;
        }

        ve_print_str("fd="); ve_print_int(buf.fd); ve_print_nl();
        ve_print_str("retval="); ve_print_int(retval); ve_print_nl();
    }
#endif

    out.ret = 0;

    if (ve_send_msg(OE_STDOUT_FILENO, type, &out, sizeof(out)) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}

int ve_handle_init(void)
{
    int ret = -1;
    const int sin = OE_STDIN_FILENO;
    const ve_msg_type_t type = VE_MSG_INIT;
    ve_msg_init_in_t in;
    ve_msg_init_out_t out;
    bool eof;

    if (ve_recv_msg_by_type(sin, type, &in, sizeof(in), &eof) != 0)
        goto done;

    globals.sock = in.sock;

    if (ve_send_n(globals.sock, &in.sock, sizeof(in.sock)) != 0)
        goto done;

    out.ret = 0;

    if (ve_send_msg(OE_STDOUT_FILENO, type, &out, sizeof(out)) != 0)
        goto done;

    ret = 0;

done:
    return ret;
}

int ve_handle_messages(void)
{
    int ret = -1;

    for (;;)
    {
        ve_msg_type_t type;
        size_t size;
        bool eof;

        if (ve_recv_msg(OE_STDIN_FILENO, &type, &size, &eof) != 0)
            goto done;

        switch (type)
        {
            case VE_MSG_TERMINATE:
            {
                if (_handle_msg_terminate(size, &eof) != 0)
                    goto done;
                break;
            }
            case VE_MSG_NEW_THREAD:
            {
                if (_handle_msg_new_thread(size, &eof) != 0)
                    goto done;
                break;
            }
            default:
            {
                goto done;
            }
        }
    }

    ret = 0;

done:
    return ret;
}

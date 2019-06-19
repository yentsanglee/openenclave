// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "msg.h"
#include <openenclave/internal/syscall/unistd.h>
#include "clone.h"
#include "close.h"
#include "exit.h"
#include "getpid.h"
#include "gettid.h"
#include "globals.h"
#include "ioctl.h"
#include "malloc.h"
#include "put.h"
#include "signal.h"
#include "string.h"
#include "syscall.h"
#include "time.h"

ssize_t ve_read(int fd, void* buf, size_t count)
{
    return ve_syscall(OE_SYS_read, fd, (long)buf, (long)count);
}

ssize_t ve_write(int fd, const void* buf, size_t count)
{
    return ve_syscall(OE_SYS_write, fd, (long)buf, (long)count);
}

void ve_debug(const char* str)
{
    ve_put(str);
    ve_put_nl();
}

static int _handle_terminate(size_t size, int sock)
{
    int ret = -1;
    ve_msg_terminate_in_t in;
    ve_msg_terminate_out_t out;
    const ve_msg_type_t type = VE_MSG_TERMINATE;

    if (size != sizeof(in))
        goto done;

    if (ve_recv_n(sock, &in, sizeof(in)) != 0)
        goto done;

    out.ret = 0;

    if (ve_send_msg(sock, type, &out, sizeof(out)) != 0)
        goto done;

    ve_puts("_thread: exiting");
    ve_exit(in.status);

    ret = 0;

done:
    return ret;
}

static int _thread(void* arg_)
{
    thread_arg_t* arg = (thread_arg_t*)arg_;

    ve_put_uint("_thread new: tcs: ", arg->tcs);

#if 0
    ve_put_uint("*** _thread: tcs: ", arg->tcs);
    ve_put_uint("*** _thread: tid: ", arg->tid);
    ve_put_uint("*** _thread: tid: ", ve_gettid());
    ve_put_uint("*** _thread: pid: ", ve_getpid());
#endif

    for (;;)
    {
        ve_msg_type_t type;
        size_t size;

        if (ve_recv_msg(arg->sock, &type, &size) != 0)
        {
            ve_put("_thread: failed to recv message\n");
            continue;
        }

        switch (type)
        {
            case VE_MSG_PING_THREAD:
            {
                ve_msg_ping_thread_in_t in;
                ve_msg_ping_thread_out_t out;

                if (ve_recv_n(arg->sock, &in, sizeof(in)) != 0)
                    ve_put_err("failed to recv message");

                ve_put_int("_thread: ping: tcs=", arg->tcs);
                out.ret = 0;

                if (ve_send_msg(arg->sock, type, &out, sizeof(out)) != 0)
                    ve_put_err("failed to send message");

                break;
            }
            case VE_MSG_TERMINATE:
            {
                _handle_terminate(size, arg->sock);
                break;
            }
            default:
            {
                ve_put("_thread: unknown message\n");
                break;
            }
        }
    }

    return 0;
}

static int _create_new_thread(thread_arg_t* arg)
{
    int ret = -1;
    uint8_t* stack;
    thread_arg_t* new_arg;

    if (!arg)
        goto done;

    /* Allocate and zero-fill the stack. */
    {
        const size_t STACK_ALIGNMENT = 4096;

        if (!(stack = ve_memalign(STACK_ALIGNMENT, arg->stack_size)))
            goto done;

        ve_memset(stack, 0, arg->stack_size);
    }

    /* Copy the arg into heap memory. */
    {
        if (!(new_arg = ve_calloc(1, sizeof(thread_arg_t))))
            goto done;

        *new_arg = *arg;
    }

    /* Create the new thread. */
    {
        int flags = 0;
        int rval;
        uint8_t* stack_bottom = stack + arg->stack_size;

        flags |= VE_CLONE_VM;
        flags |= VE_CLONE_FS;
        flags |= VE_CLONE_FILES;
        flags |= VE_CLONE_SIGHAND;
        flags |= VE_CLONE_THREAD;
        flags |= VE_CLONE_SYSVSEM;
        flags |= VE_CLONE_DETACHED;
        flags |= VE_SIGCHLD;
        flags |= VE_CLONE_PARENT_SETTID;
        flags |= VE_CLONE_CHILD_SETTID;

        if ((rval = ve_clone(
                 _thread,
                 stack_bottom,
                 flags,
                 new_arg,
                 &arg->tid,
                 NULL,
                 &new_arg->tid)) == -1)
        {
            goto done;
        }
    }

    ret = 0;

done:
    return ret;
}

static int _handle_add_thread(size_t size)
{
    int ret = -1;
    ve_msg_add_thread_in_t in;
    ve_msg_add_thread_out_t out;
    const ve_msg_type_t type = VE_MSG_ADD_THREAD;
    extern int ve_recv_fd(int sock);
    int sock = -1;
    const uint32_t ACK = 0xACACACAC;
    thread_arg_t arg;

    if (size != sizeof(in))
        goto done;

    if (ve_recv_n(globals.sock, &in, sizeof(in)) != 0)
        goto done;

    if (globals.num_threads == MAX_THREADS)
    {
        if (ve_send_msg(globals.sock, type, &out, sizeof(out)) != 0)
            goto done;

        out.ret = -1;
        ret = 0;
        goto done;
    }

    /* Receive the socket descriptor from the host. */
    if ((sock = ve_recv_fd(globals.sock)) < 0)
        goto done;

    /* Send an acknowledgement to the sendfd operation. */
    if (ve_send_n(sock, &ACK, sizeof(ACK)) != 0)
        goto done;

    /* Initialie the thread argument. */
    {
        arg.sock = sock;
        sock = -1;
        arg.tcs = in.tcs;
        arg.stack_size = in.stack_size;
    }

    /* Add this new thread to the global list. */
    {
        ve_lock(&globals.threads_lock);
        globals.threads[globals.num_threads] = arg;
        globals.num_threads++;
        ve_unlock(&globals.threads_lock);
    }

    /* Create the new thread. */
    if (_create_new_thread(&arg) != 0)
        goto done;

    out.ret = 0;

    if (ve_send_msg(globals.sock, type, &out, sizeof(out)) != 0)
        goto done;

    ret = 0;

done:

    if (sock != -1)
        ve_close(sock);

    return ret;
}

int ve_handle_init(void)
{
    int ret = -1;
    const int sin = OE_STDIN_FILENO;
    const ve_msg_type_t type = VE_MSG_INIT;
    ve_msg_init_in_t in;

    if (ve_recv_msg_by_type(sin, type, &in, sizeof(in)) != 0)
        goto done;

    globals.sock = in.sock;

    if (ve_send_n(globals.sock, &in.sock, sizeof(in.sock)) != 0)
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

        if (ve_recv_msg(globals.sock, &type, &size) != 0)
            goto done;

        switch (type)
        {
            case VE_MSG_TERMINATE:
            {
                if (_handle_terminate(size, globals.sock) != 0)
                    goto done;
                break;
            }
            case VE_MSG_ADD_THREAD:
            {
                if (_handle_add_thread(size) != 0)
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

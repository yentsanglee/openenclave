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
#include "recvfd.h"
#include "shm.h"
#include "signal.h"
#include "string.h"
#include "syscall.h"
#include "time.h"
#include "trace.h"
#include "waitpid.h"

void ve_debug(const char* str)
{
    ve_put(str);
    ve_put_nl();
}

static int _thread(void* arg_)
{
    thread_arg_t* arg = (thread_arg_t*)arg_;
    ve_msg_t msg;

    for (;;)
    {
        if (ve_recv_msg(arg->sock, &msg) != 0)
        {
            ve_put("_thread(): failed to recv message\n");
            continue;
        }

        switch (msg.type)
        {
            case VE_MSG_PING_THREAD:
            {
                const ve_msg_ping_thread_in_t* in;
                ve_msg_ping_thread_out_t out = {0};

                if (msg.size != sizeof(ve_msg_ping_thread_in_t))
                    ve_put_err("failed to send message");

                in = (const ve_msg_ping_thread_in_t*)msg.data;

                ve_put_int("thread: ping: tid=", ve_gettid());

                if (!in->str || ve_strcmp(in->str, "ping") != 0)
                    out.ret = -1;

                out.value = in->value;

                if (ve_send_msg(arg->sock, msg.type, &out, sizeof(out)) != 0)
                    ve_put_err("failed to send message");

                break;
            }
            case VE_MSG_TERMINATE:
            {
                ve_msg_terminate_out_t out = {0};

                ve_put_int("thread: exit: ", ve_gettid());

                if (ve_send_msg(arg->sock, msg.type, &out, sizeof(out)) != 0)
                    ve_put_err("failed to send message");

#if 0
                /* Close the standard descriptors. */
                ve_close(OE_STDIN_FILENO);
                ve_close(OE_STDOUT_FILENO);
                ve_close(OE_STDERR_FILENO);
#endif

                ve_exit(0);
                break;
            }
            default:
            {
                ve_put("thread: unknown message\n");
                break;
            }
        }

        ve_msg_free(&msg);
    }

    return 0;
}

static int _create_new_thread(thread_arg_t* arg)
{
    int ret = -1;
    uint8_t* stack;

    if (!arg)
        goto done;

    /* Allocate and zero-fill the stack. */
    {
        const size_t STACK_ALIGNMENT = 4096;

        if (!(stack = ve_memalign(STACK_ALIGNMENT, arg->stack_size)))
            goto done;

        ve_memset(stack, 0, arg->stack_size);

        arg->stack = stack;
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
        // flags |= VE_CLONE_THREAD;
        flags |= VE_CLONE_SYSVSEM;
        flags |= VE_CLONE_DETACHED;
        flags |= VE_SIGCHLD;
        flags |= VE_CLONE_PARENT_SETTID;
        flags |= VE_SIGCHLD;
        // flags |= VE_CLONE_CHILD_SETTID;

        if ((rval = ve_clone(
                 _thread, stack_bottom, flags, arg, &arg->tid, NULL, NULL)) ==
            -1)
        {
            goto done;
        }
    }

    ret = 0;

done:
    return ret;
}

static int _handle_add_thread(int fd, uint64_t arg_in)
{
    int ret = -1;
    int sock = -1;
    ve_add_thread_arg_t* arg = (ve_add_thread_arg_t*)arg_in;
    thread_arg_t* thread_arg;

    if (!arg)
        goto done;

    arg->ret = -1;

    /* If no more threads. */
    if (globals.num_threads == MAX_THREADS)
        goto done;

    /* Receive the socket descriptor from the host. */
    if ((sock = ve_recv_fd(globals.sock)) < 0)
        goto done;

    /* Add this new thread to the global list. */
    ve_lock(&globals.threads_lock);
    {
        thread_arg = &globals.threads[globals.num_threads];
        thread_arg->sock = sock;
        thread_arg->tcs = arg->tcs;
        thread_arg->stack_size = arg->stack_size;
        globals.num_threads++;
        sock = -1;
    }
    ve_unlock(&globals.threads_lock);

    /* Create the new thread. */
    if (_create_new_thread(thread_arg) != 0)
        goto done;

    arg->ret = 0;
    ret = 0;

done:

    if (sock != -1)
        ve_close(sock);

    return ret;
}

int _attach_host_heap(globals_t* globals, int shmid, const void* shmaddr)
{
    int ret = -1;
    void* rval;

    if (shmid == -1 || shmaddr == NULL || shmaddr == (void*)-1)
        goto done;

    /* Attach the host's shared memory heap. */
    if ((rval = ve_shmat(shmid, shmaddr, VE_SHM_RND)) == (void*)-1)
    {
        /* ATTN: send response on failure? */
        ve_put_uint("error: ve_shmat(1) failed: shmaddr=", (uint64_t)shmaddr);
        goto done;
    }

    if (rval != shmaddr)
    {
        /* ATTN: send response on failure? */
        ve_put_uint("error: ve_shmat(2) failed: shmaddr=", (uint64_t)shmaddr);
        goto done;
    }

    globals->shmaddr = rval;

    *((uint64_t*)rval) = VE_SHMADDR_MAGIC;

    ret = 0;

done:
    return 0;
}

int ve_handle_init(void)
{
    int ret = -1;
    ve_init_arg_t arg;
    uint64_t retval = 0;

    /* Receive request from standard input. */
    {
        if (ve_recv_n(OE_STDIN_FILENO, &arg, sizeof(arg)) != 0)
            goto done;

        if (arg.magic != VE_INIT_ARG_MAGIC)
        {
            ve_puts("ve_handle_init(): bad magic number");
            goto done;
        }
    }

    /* Handle the request. */
    {
        globals.sock = arg.sock;

        if (_attach_host_heap(&globals, arg.shmid, arg.shmaddr) != 0)
        {
            ve_put("_attach_host_heap() failed");
            retval = -1;
        }
    }

    /* Send response back on the socket. */
    if (ve_send_n(globals.sock, &retval, sizeof(retval)) != 0)
        goto done;

    ret = retval;

done:
    return ret;
}

int ve_handle_messages(void)
{
    int ret = -1;
#if 0
    /* ATTN: These both crashes with clang using -O1 and -fsanitize=address */
    //ve_msg_t msg = VE_MSG_INITIALIZER;
    ve_msg_t msg = { 0 };
#else
    /* Really no need to initialize this. */
    ve_msg_t msg;
#endif

    for (;;)
    {
        if (ve_recv_msg(globals.sock, &msg) != 0)
            goto done;

        switch (msg.type)
        {
            case VE_MSG_TERMINATE:
            {
                ve_msg_terminate_out_t out = {0};

                /* Release thread stack memory and sockets . */
                for (size_t i = 0; i < globals.num_threads; i++)
                {
                    ve_free(globals.threads[i].stack);
                    ve_close(globals.threads[i].sock);
                }

                ve_put_int("main: exit: ", ve_gettid());

                if (ve_send_msg(globals.sock, msg.type, &out, sizeof(out)) != 0)
                    ve_put_err("failed to send message");

                ve_close(globals.sock);
                ve_msg_free(&msg);
                ve_shmdt(globals.shmaddr);
                ve_exit(0);
                break;
            }
            default:
            {
                goto done;
            }
        }

        ve_msg_free(&msg);
    }

    ret = 0;

done:

    ve_msg_free(&msg);

    return ret;
}

static int _handle_terminate(int fd, uint64_t arg_in)
{
    int ret = -1;
    ve_terminate_arg_t* arg = (ve_terminate_arg_t*)arg_in;

    if (!arg)
        goto done;

    arg->ret = -1;

    /* Release thread stack memory and sockets . */
    for (size_t i = 0; i < globals.num_threads; i++)
    {
        ve_free(globals.threads[i].stack);
        ve_close(globals.threads[i].sock);
    }

    ve_close(globals.sock);
    ve_shmdt(globals.shmaddr);

    /* Wait on the exit status of each thread. */
    for (size_t i = 0; i < globals.num_threads; i++)
    {
        int pid;
        int status;

        if ((pid = ve_waitpid(-1, &status, 0)) < 0)
        {
            ve_puts("enclave: error: failed to wait for thread");
            goto done;
        }
    }

    /* Close the standard descriptors. */
    ve_close(OE_STDIN_FILENO);
    ve_close(OE_STDOUT_FILENO);
    ve_close(OE_STDERR_FILENO);

    arg->ret = 0;
    ret = 0;

    /* No response is expected. */
    ve_exit(0);

done:
    return ret;
}

static int _handle_ping(int fd, uint64_t arg_in, uint64_t* arg_out)
{
    ve_puts("enclave pinged");

    (void)fd;

    *arg_out = arg_in;

    return 0;
}

static int _handle_call(
    int fd,
    uint64_t func,
    uint64_t arg_in,
    uint64_t* arg_out)
{
    switch (func)
    {
        case VE_FUNC_PING:
        {
            return _handle_ping(fd, arg_in, arg_out);
        }
        case VE_FUNC_ADD_THREAD:
        {
            return _handle_add_thread(fd, arg_in);
        }
        case VE_FUNC_TERMINATE:
        {
            return _handle_terminate(fd, arg_in);
        }
        default:
        {
            return -1;
        }
    }
}

int ve_call(int fd, uint64_t func, uint64_t arg_in, uint64_t* arg_out)
{
    int ret = -1;

    if (arg_out)
        *arg_out = 0;

    if (fd < 0)
        goto done;

    /* Send request. */
    {
        ve_call_msg_t msg = {func, arg_in};

        if (ve_send_n(fd, &msg, sizeof(msg)) != 0)
            goto done;
    }

    /* Receive response. */
    for (;;)
    {
        ve_call_msg_t msg;

        if (ve_recv_n(fd, &msg, sizeof(msg)) != 0)
            goto done;

        switch (msg.func)
        {
            case VE_FUNC_RET:
            {
                if (arg_out)
                    *arg_out = msg.arg;

                ret = 0;
                goto done;
            }
            case VE_FUNC_ERR:
            {
                goto done;
            }
            default:
            {
                uint64_t arg = 0;

                if (_handle_call(fd, msg.func, msg.arg, &arg) == 0)
                {
                    msg.func = VE_FUNC_RET;
                    msg.arg = arg;
                }
                else
                {
                    msg.func = VE_FUNC_ERR;
                    msg.arg = 0;
                }

                if (ve_send_n(fd, &msg, sizeof(msg)) != 0)
                    goto done;

                /* Go back to waiting for return from original call. */
                continue;
            }
        }
    }

done:
    return ret;
}

int ve_handle_calls(int fd)
{
    int ret = -1;

    if (fd < 0)
        goto done;

    for (;;)
    {
        ve_call_msg_t msg_in;
        ve_call_msg_t msg_out;
        uint64_t arg = 0;

        if (ve_recv_n(fd, &msg_in, sizeof(msg_in)) != 0)
            goto done;

        ve_put_str("ENCLAVE:", ve_func_name(msg_in.func));

        if (_handle_call(fd, msg_in.func, msg_in.arg, &arg) == 0)
        {
            msg_out.func = VE_FUNC_RET;
            msg_out.arg = arg;
        }
        else
        {
            msg_out.func = VE_FUNC_ERR;
            msg_out.arg = 0;
        }

        if (ve_send_n(fd, &msg_out, sizeof(msg_out)) != 0)
            goto done;
    }

    ret = 0;

done:
    return ret;
}

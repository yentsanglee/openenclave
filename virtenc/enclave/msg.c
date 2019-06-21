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

void ve_debug(const char* str)
{
    ve_put(str);
    ve_put_nl();
}

static int _thread(void* arg_)
{
    thread_arg_t* arg = (thread_arg_t*)arg_;
    ve_msg_t msg;

    ve_put_uint("=== _thread: ", arg->tid);
#if 0
    ve_put_uint("thread.tcs: ", arg->tcs);
    ve_put_uint("thread.tid: ", arg->tid);
    ve_put_uint("thread.tid: ", ve_gettid());
    ve_put_uint("thread.pid: ", ve_getpid());
#endif

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

                ve_put_int("thread: ping: tid=", arg->tid);
                ve_put_int("thread: ping: value=", in->value);
                ve_put_str("thread: ping: str=", in->str);

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

                /* Close the standard descriptors. */
                ve_close(OE_STDIN_FILENO);
                ve_close(OE_STDOUT_FILENO);
                ve_close(OE_STDERR_FILENO);

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
        flags |= VE_CLONE_THREAD;
        flags |= VE_CLONE_SYSVSEM;
        flags |= VE_CLONE_DETACHED;
        flags |= VE_SIGCHLD;
        flags |= VE_CLONE_PARENT_SETTID;
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

static int _handle_add_thread(const ve_msg_add_thread_in_t* in)
{
    int ret = -1;
    const ve_msg_type_t type = VE_MSG_ADD_THREAD;
    int sock = -1;
    thread_arg_t* arg;

    /* Fail if out of threads. */
    /* TODO: host should pass num_tcs parameter. */
    if (globals.num_threads == MAX_THREADS)
    {
        const ve_msg_add_thread_out_t out = {.ret = -1};

        if (ve_send_msg(globals.sock, type, &out, sizeof(out)) != 0)
            goto done;

        ret = 0;
        goto done;
    }

    /* Receive the socket descriptor from the host. */
    if ((sock = ve_recv_fd(globals.sock)) < 0)
        goto done;

    /* Add this new thread to the global list. */
    ve_lock(&globals.threads_lock);
    {
        arg = &globals.threads[globals.num_threads];
        arg->sock = sock;
        arg->tcs = in->tcs;
        arg->stack_size = in->stack_size;
        globals.num_threads++;
        sock = -1;
    }
    ve_unlock(&globals.threads_lock);

    /* Create the new thread. */
    if (_create_new_thread(arg) != 0)
        goto done;

    /* Send the response. */
    {
        ve_msg_add_thread_out_t out = {.ret = 0};

        if (ve_send_msg(globals.sock, type, &out, sizeof(out)) != 0)
            goto done;
    }

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
    const ve_msg_type_t type = VE_MSG_INIT;
    ve_msg_init_in_t in;
    ve_msg_terminate_out_t out = {-1};

    /* Receive request from standard-input. */
    if (ve_recv_msg_by_type(OE_STDIN_FILENO, type, &in, sizeof(in)) != 0)
        goto done;

    globals.sock = in.sock;

    if (_attach_host_heap(&globals, in.shmid, in.shmaddr) != 0)
        ve_put("_attach_host_heap() failed");
    else
        out.ret = 0;

    /* Send response on the socket. */
    if (ve_send_msg(globals.sock, type, &out, sizeof(out)) != 0)
        goto done;

    ret = out.ret;

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
            case VE_MSG_ADD_THREAD:
            {
                const ve_msg_add_thread_in_t* in;

                if (msg.size != sizeof(ve_msg_add_thread_in_t))
                    goto done;

                in = (const ve_msg_add_thread_in_t*)msg.data;

                if (_handle_add_thread(in) != 0)
                    goto done;

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

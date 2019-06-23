// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "msg.h"
#include <openenclave/internal/syscall/unistd.h>
#include "globals.h"
#include "io.h"
#include "malloc.h"
#include "print.h"
#include "process.h"
#include "recvfd.h"
#include "shm.h"
#include "signal.h"
#include "string.h"
#include "syscall.h"
#include "time.h"
#include "trace.h"

static int _thread(void* arg_)
{
    thread_t* arg = (thread_t*)arg_;

    if (ve_handle_calls(arg->sock) != 0)
    {
        ve_put("_thread(): ve_handle_calls() failed\n");
        ve_exit(1);
    }

    return 0;
}

static int _create_new_thread(thread_t* arg)
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
        uint8_t* child_stack = stack + arg->stack_size;

        flags |= VE_CLONE_VM;
        flags |= VE_CLONE_FS;
        flags |= VE_CLONE_FILES;
        flags |= VE_CLONE_SIGHAND;
        flags |= VE_CLONE_SYSVSEM;
        flags |= VE_CLONE_DETACHED;
        flags |= VE_SIGCHLD;
        flags |= VE_CLONE_PARENT_SETTID;
        flags |= VE_SIGCHLD;
        // flags |= VE_CLONE_CHILD_SETTID;

        rval = ve_clone(_thread, child_stack, flags, arg, &arg->tid);

        if (rval == -1)
            goto done;

        ve_print("encl: new thread: rval=%d\n", rval);
    }

    ret = 0;

done:
    return ret;
}

static void _handle_add_thread(uint64_t arg_in)
{
    int sock = -1;
    ve_add_thread_arg_t* arg = (ve_add_thread_arg_t*)arg_in;
    thread_t* thread;

    arg->ret = -1;

    /* If no more threads. */
    if (globals.threads.size == MAX_THREADS)
        goto done;

    /* Receive the socket descriptor from the host. */
    if ((sock = ve_recv_fd(globals.sock)) < 0)
        goto done;

    /* Add this new thread to the global list. */
    ve_lock(&globals.threads.lock);
    {
        thread = &globals.threads.data[globals.threads.size];
        thread->sock = sock;
        thread->tcs = arg->tcs;
        thread->stack_size = arg->stack_size;
        globals.threads.size++;
        sock = -1;
    }
    ve_unlock(&globals.threads.lock);

    /* Create the new thread. */
    if (_create_new_thread(thread) != 0)
        goto done;

    arg->ret = 0;

done:

    if (sock != -1)
        ve_close(sock);
}

static int _attach_host_heap(globals_t* globals, int shmid, const void* shmaddr)
{
    int ret = -1;
    void* rval;

    if (shmid == -1 || shmaddr == NULL || shmaddr == (void*)-1)
        goto done;

    /* Attach the host's shared memory heap. */
    if ((rval = ve_shmat(shmid, shmaddr, VE_SHM_RND)) == (void*)-1)
    {
        ve_print("error: ve_shmat(1) failed: shmaddr=%p", shmaddr);
        goto done;
    }

    if (rval != shmaddr)
    {
        ve_print("error: ve_shmat(2) failed: shmaddr=%p", shmaddr);
        goto done;
    }

    /* Save so it can be released on process exit. */
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
        if (ve_readn(OE_STDIN_FILENO, &arg, sizeof(arg)) != 0)
            goto done;

        if (arg.magic != VE_INIT_ARG_MAGIC)
        {
            ve_put("ve_handle_init(): bad magic number\n");
            goto done;
        }
    }

    /* Handle the request. */
    {
        globals.sock = arg.sock;

        if (_attach_host_heap(&globals, arg.shmid, arg.shmaddr) != 0)
        {
            ve_put("_attach_host_heap() failed\n");
            retval = -1;
        }
    }

    /* Send response back on the socket. */
    if (ve_writen(globals.sock, &retval, sizeof(retval)) != 0)
        goto done;

    ret = retval;

done:
    return ret;
}

static void _handle_terminate(void)
{
    /* Wait on the exit status of each thread. */
    for (size_t i = 0; i < globals.threads.size; i++)
    {
        int pid;
        int status;

        if ((pid = ve_waitpid(-1, &status, 0)) < 0)
        {
            ve_put("encl: error: failed to wait for thread\n");
            ve_exit(1);
        }
    }

    /* Release resources held by threads. */
    for (size_t i = 0; i < globals.threads.size; i++)
    {
        ve_free(globals.threads.data[i].stack);
        ve_close(globals.threads.data[i].sock);
    }

    /* Release resources held by the main thread. */
    ve_close(globals.sock);
    ve_shmdt(globals.shmaddr);

    /* Close the standard descriptors. */
    ve_close(OE_STDIN_FILENO);
    ve_close(OE_STDOUT_FILENO);
    ve_close(OE_STDERR_FILENO);

    /* No response is expected. */
    ve_exit(0);
}

static void _handle_ping(int fd, uint64_t arg_in, uint64_t* arg_out)
{
    uint64_t arg;

    ve_print("encl: ping: tid=%d\n", ve_gettid());

    if (ve_call(fd, VE_FUNC_PING, arg_in, &arg) != 0)
        ve_put("encl: ve_call() failed\n");

    if (arg_out)
        *arg_out = arg;
}

static void _handle_terminate_thread(void)
{
    ve_print("encl: thread exit: tid=%d\n", ve_gettid());
    ve_exit(0);
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
            _handle_ping(fd, arg_in, arg_out);
            return 0;
        }
        case VE_FUNC_ADD_THREAD:
        {
            if (!arg_in)
                return -1;

            _handle_add_thread(arg_in);
            return 0;
        }
        case VE_FUNC_TERMINATE:
        {
            /* does not return. */
            _handle_terminate();
            return 0;
        }
        case VE_FUNC_TERMINATE_THREAD:
        {
            /* does not return. */
            _handle_terminate_thread();
            return 0;
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

        if (ve_writen(fd, &msg, sizeof(msg)) != 0)
            goto done;
    }

    /* Receive response. */
    for (;;)
    {
        ve_call_msg_t msg;

        if (ve_readn(fd, &msg, sizeof(msg)) != 0)
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

                if (ve_writen(fd, &msg, sizeof(msg)) != 0)
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

        if (ve_readn(fd, &msg_in, sizeof(msg_in)) != 0)
            goto done;

#if defined(TRACE_CALLS)
        ve_put_s("ENCLAVE:", ve_func_name(msg_in.func));
#endif

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

        if (ve_writen(fd, &msg_out, sizeof(msg_out)) != 0)
            goto done;
    }

    ret = 0;

done:
    return ret;
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include <openenclave/internal/syscall/unistd.h>
#include "call.h"
#include "globals.h"
#include "io.h"
#include "malloc.h"
#include "print.h"
#include "process.h"
#include "recvfd.h"
#include "sbrk.h"
#include "shm.h"
#include "signal.h"
#include "string.h"
#include "time.h"
#include "trace.h"

void ve_call_init_functions(void);

void ve_call_fini_functions(void);

int ve_handle_calls(int fd);

static bool _called_constructor = false;

__attribute__((constructor)) static void constructor(void)
{
    _called_constructor = true;
}

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

void ve_handle_call_add_thread(uint64_t arg_in)
{
    int sock = -1;
    ve_add_thread_arg_t* arg = (ve_add_thread_arg_t*)arg_in;
    thread_t* thread;

    arg->retval = -1;

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

    arg->retval = 0;

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
    return ret;
}

int ve_handle_init(void)
{
    int ret = -1;
    ve_init_arg_t arg;
    int retval = 0;

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

void ve_handle_call_terminate(void)
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

void ve_handle_call_ping(int fd, uint64_t arg1, uint64_t* retval)
{
    uint64_t tmp_retval;

    ve_print("encl: ping: tid=%d\n", ve_gettid());

    if (ve_call1(fd, VE_FUNC_PING, &tmp_retval, arg1) != 0)
        ve_put("encl: ve_call() failed\n");

    if (retval)
        *retval = tmp_retval;
}

void ve_handle_call_terminate_thread(void)
{
    ve_print("encl: thread exit: tid=%d\n", ve_gettid());
    ve_exit(0);
}

static int _main(void)
{
    if (!_called_constructor)
    {
        ve_puts("constructor not called");
        ve_exit(1);
    }

    /* Wait here to be initialized and to receive the main socket. */
    if (ve_handle_init() != 0)
    {
        ve_puts("ve_handle_init() failed");
        ve_exit(1);
    }

    /* Handle messages over the main socket. */
    if (ve_handle_calls(globals.sock) != 0)
    {
        ve_puts("enclave: ve_handle_calls() failed");
        ve_exit(1);
    }

    return 0;
}

#if defined(BUILD_STATIC)
void _start(void)
{
    ve_call_init_functions();
    int rval = _main();
    ve_call_fini_functions();
    ve_exit(rval);
}
#else
int main(void)
{
    ve_exit(_main());
    return 0;
}
#endif

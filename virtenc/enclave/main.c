// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include <openenclave/internal/syscall/unistd.h>
#include <openenclave/internal/utils.h>
#include "assert.h"
#include "call.h"
#include "globals.h"
#include "hexdump.h"
#include "io.h"
#include "malloc.h"
#include "print.h"
#include "process.h"
#include "recvfd.h"
#include "register.h"
#include "sbrk.h"
#include "settings.h"
#include "shm.h"
#include "signal.h"
#include "string.h"
#include "time.h"
#include "trace.h"

#define VE_PAGE_SIZE 4096

#define THREAD_VALUE_INITIALIZER 0xaabbccddeeff1122

__thread uint64_t __thread_value = THREAD_VALUE_INITIALIZER;

__thread int __ve_thread_pid;

void ve_call_init_functions(void);

void ve_call_fini_functions(void);

int ve_handle_calls(int fd);

static bool _called_constructor = false;

__attribute__((constructor)) static void constructor(void)
{
    _called_constructor = true;
}

typedef struct _thread_arg
{
    int sock;
} thread_arg_t;

static void _thread_destructor(void* arg_)
{
    thread_arg_t* arg = (thread_arg_t*)arg_;
    // ve_close(arg->sock);
    ve_free(arg);
}

static int _thread_func(void* arg_)
{
    thread_arg_t* arg = (thread_arg_t*)arg_;

    __ve_thread_pid = ve_getpid();

    if (ve_thread_set_destructor(_thread_destructor, arg) != 0)
        ve_panic("ve_thread_set_destructor() failed");

    if (__thread_value != THREAD_VALUE_INITIALIZER)
        ve_panic("__thread_value != THREAD_VALUE_INITIALIZER");

    __thread_value = 0;

    if (ve_handle_calls(arg->sock) != 0)
        ve_panic("_thread(): ve_handle_calls() failed\n");

    return 0;
}

void ve_handle_call_add_thread(int fd, ve_call_buf_t* buf)
{
    int sock = -1;

    OE_UNUSED(fd);

    buf->retval = (uint64_t)-1;

    /* Receive the socket descriptor from the host. */
    if ((sock = ve_recv_fd(g_sock)) < 0)
        goto done;

    /* Create the new thread. */
    {
        size_t stack_size = (size_t)buf->arg2;
        thread_arg_t* arg;
        ve_thread_t thread;

        if (!(arg = ve_calloc(1, sizeof(thread_arg_t))))
            goto done;

        arg->sock = sock;

        if (ve_thread_create(&thread, _thread_func, arg, stack_size) != 0)
        {
            ve_free(arg);
            goto done;
        }
    }

    sock = -1;
    buf->retval = 0;

done:

    if (sock != -1)
        ve_close(sock);
}

static int _attach_host_heap(globals_t* globals, int shmid, const void* shmaddr)
{
    int ret = -1;
    void* rval;
    const int shmflg = VE_SHM_RND | VE_SHM_REMAP;

    if (shmid == -1 || shmaddr == NULL || shmaddr == (void*)-1)
        goto done;

    /* Attach the host's shared memory heap. */
    if ((rval = ve_shmat(shmid, shmaddr, shmflg)) == (void*)-1)
    {
        ve_print(
            "error: ve_shmat(1) failed: rval=%p shmid=%d shmaddr=%p\n",
            rval,
            shmid,
            shmaddr);
        goto done;
    }

    if (rval != shmaddr)
    {
        ve_print(
            "error: ve_shmat(2) failed: rval=%p shmid=%d shmaddr=%p\n",
            rval,
            shmid,
            shmaddr);
        goto done;
    }

    /* Save so it can be released on process exit. */
    globals->shmaddr = rval;

    ret = 0;

done:
    return ret;
}

int ve_handle_init(void)
{
    int ret = -1;
    ve_init_arg_t arg;
    int retval = 0;

    __ve_thread_pid = ve_getpid();

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

    /* Save the TLS information. */
    g_tdata_rva = arg.tdata_rva;
    g_tdata_size = arg.tdata_size;
    g_tdata_align = arg.tdata_align;
    g_tbss_rva = arg.tbss_rva;
    g_tbss_size = arg.tbss_size;
    g_tbss_align = arg.tbss_align;
    __ve_self = arg.self_rva;

    /* Handle the request. */
    {
        g_sock = arg.sock;

        if (_attach_host_heap(&globals, arg.shmid, arg.shmaddr) != 0)
        {
            ve_put("_attach_host_heap() failed\n");
            retval = -1;
        }
    }

    /* Send response back on the socket. */
    if (ve_writen(g_sock, &retval, sizeof(retval)) != 0)
        goto done;

    ret = retval;

done:
    return ret;
}

void ve_handle_call_terminate(int fd, ve_call_buf_t* buf)
{
    OE_UNUSED(fd);
    OE_UNUSED(buf);

    /* Wait for all threads to gracefuly shutdown. */
    ve_thread_join_all();

    /* Release resources held by the main thread. */
    ve_close(g_sock);
    ve_shmdt(globals.shmaddr);

    /* Close the standard descriptors. */
    ve_close(OE_STDIN_FILENO);
    ve_close(OE_STDOUT_FILENO);
    ve_close(OE_STDERR_FILENO);

    /* No response is expected. */
    ve_exit(0);
}

void test_malloc(int fd)
{
    char* p;
    size_t n = 32;

    if (!(p = ve_call_calloc(fd, 1, n)))
    {
        ve_puts("ve_call_calloc() failed\n");
        ve_exit(1);
    }

    ve_strlcpy(p, "hello world!", n);

    if (ve_strcmp(p, "hello world!") != 0)
    {
        ve_puts("ve_call_calloc() failed\n");
        ve_exit(1);
    }

    if (ve_call_free(fd, p) != 0)
    {
        ve_puts("ve_call_free() failed\n");
        ve_exit(1);
    }
}

void ve_handle_call_ping(int fd, ve_call_buf_t* buf)
{
    uint64_t retval = 0;

    ve_print("encl: ping: value=%lx [%u]\n", buf->arg1, ve_getpid());

    ve_assert(__ve_thread_pid == ve_getpid());

    test_malloc(fd);

    if (ve_call1(fd, VE_FUNC_PING, &retval, buf->arg1) != 0)
        ve_put("encl: ve_call() failed\n");

    buf->retval = retval;
}

void ve_handle_get_settings(int fd, ve_call_buf_t* buf)
{
    ve_enclave_settings_t* settings = (ve_enclave_settings_t*)buf->arg1;

    OE_UNUSED(fd);

    if (settings)
    {
        /* ATTN: get this from the enclave information struct. */
        *settings = __ve_enclave_settings;
        buf->retval = 0;
    }
    else
    {
        buf->retval = (uint64_t)-1;
    }
}

void ve_handle_call_terminate_thread(int fd, ve_call_buf_t* buf)
{
    OE_UNUSED(fd);
    OE_UNUSED(buf);

    ve_close(fd);

    ve_print("encl: thread exit: tid=%d\n", ve_gettid());
    ve_exit(0);
}

int __ve_pid;

static int _main(void)
{
    if (!_called_constructor)
    {
        ve_puts("constructor not called");
        ve_exit(1);
    }

    /* Save the process id into a global. */
    if ((__ve_pid = ve_getpid()) < 0)
    {
        ve_puts("ve_getpid() failed");
        ve_exit(1);
    }

    ve_print("encl: pid=%d\n", __ve_pid);

    /* Wait here to be initialized and to receive the main socket. */
    if (ve_handle_init() != 0)
    {
        ve_puts("ve_handle_init() failed");
        ve_exit(1);
    }

    /* Handle messages over the main socket. */
    if (ve_handle_calls(g_sock) != 0)
    {
        ve_puts("encl: ve_handle_calls() failed");
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

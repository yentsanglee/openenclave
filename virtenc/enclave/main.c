// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "assert.h"
#include "call.h"
#include "common.h"
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
#include "syscall.h"
#include "thread.h"
#include "time.h"
#include "trace.h"

#define MAX_THREADS 1024

#define VE_PAGE_SIZE 4096

#define THREAD_VALUE_INITIALIZER 0xaabbccddeeff1122

__thread uint64_t __thread_value = THREAD_VALUE_INITIALIZER;

static __thread int _pid_tls;

__thread int __ve_thread_sock;

int ve_handle_calls(int fd);

static bool _called_constructor = false;

static ve_thread_t _threads[MAX_THREADS];
static size_t _nthreads;

__attribute__((constructor)) static void constructor(void)
{
    _called_constructor = true;
}

static int _thread_func(void* arg)
{
    int sock = (int)(int64_t)arg;

    __ve_thread_sock = sock;

    _pid_tls = ve_getpid();

    if (__thread_value != THREAD_VALUE_INITIALIZER)
        ve_panic("__thread_value != THREAD_VALUE_INITIALIZER");

    __thread_value = 0;

    if (ve_handle_calls(sock) != 0)
        ve_panic("_thread(): ve_handle_calls() failed\n");

    return 0;
}

int ve_handle_call_add_thread(int fd, ve_call_buf_t* buf)
{
    int sock = -1;

    OE_UNUSED(fd);

    buf->retval = (uint64_t)-1;

    /* Receive the socket descriptor from the host. */
    if ((sock = ve_recv_fd(__ve_sock)) < 0)
        goto done;

    /* Create the new thread. */
    {
        size_t stack_size = (size_t)buf->arg2;
        ve_thread_t thread;
        void* arg = (void*)(int64_t)sock;

        /* Create a new thread. */
        if (ve_thread_create(&thread, _thread_func, arg, stack_size) != 0)
        {
            ve_free(arg);
            goto done;
        }

        /* Add new thread to the list. */
        {
            if (_nthreads == MAX_THREADS)
                ve_panic("too many threads");

            _threads[_nthreads++] = thread;
        }
    }

    sock = -1;
    buf->retval = 0;

done:

    if (sock != -1)
        ve_close(sock);

    return 0;
}

static int _attach_host_heap(int shmid, const void* shmaddr, size_t shmsize)
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
    __ve_shmaddr = shmaddr;
    __ve_shmsize = shmsize;

    ret = 0;

done:
    return ret;
}

int ve_handle_init(void)
{
    int ret = -1;
    ve_init_arg_t arg;
    int retval = 0;

    _pid_tls = ve_getpid();

    /* Receive request from standard input. */
    {
        if (ve_readn(VE_STDIN_FILENO, &arg, sizeof(arg)) != 0)
            goto done;

        if (arg.magic != VE_INIT_ARG_MAGIC)
        {
            ve_put("ve_handle_init(): bad magic number\n");
            goto done;
        }
    }

    /* Save the TLS information. */
    __ve_tdata_rva = arg.tdata_rva;
    __ve_tdata_size = arg.tdata_size;
    __ve_tdata_align = arg.tdata_align;
    __ve_tbss_rva = arg.tbss_rva;
    __ve_tbss_size = arg.tbss_size;
    __ve_tbss_align = arg.tbss_align;
    __ve_self = arg.self_rva;

    /* Handle the request. */
    {
        __ve_sock = arg.sock;

        if (_attach_host_heap(arg.shmid, arg.shmaddr, arg.shmsize) != 0)
        {
            ve_put("_attach_host_heap() failed\n");
            retval = -1;
        }
    }

    /* Send response back on the socket. */
    if (ve_writen(__ve_sock, &retval, sizeof(retval)) != 0)
        goto done;

    ret = retval;

done:
    return ret;
}

int ve_handle_call_terminate(int fd, ve_call_buf_t* buf)
{
    OE_UNUSED(fd);
    OE_UNUSED(buf);

    /* Wait for all threads to gracefuly shutdown. */
    for (size_t i = 0; i < _nthreads; i++)
    {
        int retval;

        if (ve_thread_join(_threads[i], &retval) != 0)
            ve_panic("failed to join threads");

        ve_print("encl: join: retval=%d\n", retval);
    }

    /* Release resources held by the main thread. */
    ve_close(__ve_sock);
    ve_shmdt(__ve_shmaddr);

    /* Close the standard descriptors. */
    ve_close(VE_STDIN_FILENO);
    ve_close(VE_STDOUT_FILENO);
    ve_close(VE_STDERR_FILENO);

    /* No response is expected. */
    ve_exit(0);

    return 0;
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

int ve_handle_call_ping(int fd, ve_call_buf_t* buf)
{
    uint64_t retval = 0;

    ve_print("encl: ping: value=%lx [%u]\n", buf->arg1, ve_getpid());

    ve_assert(_pid_tls == ve_getpid());

    test_malloc(fd);

    if (ve_call1(fd, VE_FUNC_PING, &retval, buf->arg1) != 0)
        ve_put("encl: ve_call() failed\n");

    buf->retval = retval;

    return 0;
}

int ve_handle_get_settings(int fd, ve_call_buf_t* buf)
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

    return 0;
}

int ve_handle_call_terminate_thread(int fd, ve_call_buf_t* buf)
{
    OE_UNUSED(fd);
    OE_UNUSED(buf);

    ve_close(fd);

    ve_print("encl: thread exit: tid=%d\n", ve_gettid());
    ve_exit(99);

    return 0;
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
    if (ve_handle_calls(__ve_sock) != 0)
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

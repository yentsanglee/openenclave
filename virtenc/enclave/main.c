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

__thread int __ve_thread_pid;

__thread uint64_t __thread_value = 0xbaadf00dbaadf00d;

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

    __ve_thread_pid = ve_getpid();

    if (ve_handle_calls(arg->sock) != 0)
    {
        ve_put("_thread(): ve_handle_calls() failed\n");
        ve_exit(1);
    }

    return 0;
}

static int ve_get_tls(thread_t* thread, const void** tls, size_t* tls_size)
{
    int ret = -1;
    const uint8_t* p;
    size_t align = 0;

    if (tls)
        *tls = NULL;

    if (tls_size)
        *tls_size = 0;

    if (!thread || !tls || !tls_size)
        goto done;

    if (g_tdata_size == 0 && g_tbss_size == 0)
        goto done;

    /* align = max(g_tdata_align, g_tbss_align) */
    if (g_tdata_align > g_tbss_align)
        align = g_tdata_align;
    else
        align = g_tbss_align;

    /* Check that align is a power of two. */
    if ((align & (align - 1)))
        goto done;

    p = (const uint8_t*)thread;
    p -= oe_round_up_to_multiple(g_tdata_size, align);
    p -= oe_round_up_to_multiple(g_tbss_size, align);

    *tls = p;
    *tls_size = (size_t)((const uint8_t*)thread - p);

    ret = 0;

done:
    return ret;
}

static int _create_new_thread(int sock, uint64_t tcs, size_t stack_size)
{
    int ret = -1;
    uint8_t* stack = NULL;
    thread_t* main_thread;
    thread_t* thread;
    const void* main_tls;
    size_t main_tls_size;
    void* tls = NULL;
    size_t tls_size;

    /* Get the main thread. */
    if (!(main_thread = ve_thread_self()))
        goto done;

    /* Get the TLS data from the main thread. */
    if (ve_get_tls(main_thread, &main_tls, &main_tls_size) != 0)
        goto done;

    /* Calculate the TLS size the new thread (rounded to the page size). */
    tls_size = oe_round_up_to_multiple(main_tls_size, VE_PAGE_SIZE);

    /* Allocate TLS followed by the thread. */
    {
        const size_t alloc_size = tls_size + sizeof(thread_t);

        if (!(tls = ve_memalign(VE_PAGE_SIZE, alloc_size)))
            goto done;

        ve_memset(tls, 0, alloc_size);
    }

    /* Set thread pointer into the middle of the allocation. */
    thread = (thread_t*)((uint8_t*)tls + tls_size);

    /* Allocate and zero-fill the stack. */
    {
        if (!(stack = ve_memalign(VE_PAGE_SIZE, stack_size)))
            goto done;

        ve_memset(stack, 0, stack_size);
    }

    /* Copy the base fields from the parent thread. */
    ve_memcpy(&thread->base, main_thread, sizeof(thread_base_t));

    /* Initilize the thread. */
    {
        thread->base.self = &thread->base;
        thread->next = NULL;
        thread->tls = tls;
        thread->tls_size = tls_size;
        thread->tcs = tcs;
        thread->stack = stack;
        thread->stack_size = stack_size;
        thread->sock = sock;
    }

    /* Create the new thread. */
    {
        int flags = 0;
        int rval;
        uint8_t* child_stack = stack + thread->stack_size;
        void* child_tls = thread;

        flags |= VE_CLONE_VM;
        flags |= VE_CLONE_FS;
        flags |= VE_CLONE_FILES;
        flags |= VE_CLONE_SIGHAND;
        flags |= VE_CLONE_SYSVSEM;
        flags |= VE_CLONE_DETACHED;
        flags |= VE_SIGCHLD;
        flags |= VE_CLONE_PARENT_SETTID;
        flags |= VE_SIGCHLD;
        flags |= VE_CLONE_CHILD_SETTID;
        flags |= VE_CLONE_SETTLS;

        rval = ve_clone(
            _thread,
            child_stack,
            flags,
            thread,
            &thread->ptid,
            child_tls,
            &thread->ctid);

        if (rval == -1)
            goto done;

        ve_print("encl: new thread: rval=%d\n", rval);
    }

    /* Prepend the thread to the global linked list. */
    {
        ve_lock(&globals.lock);
        thread->next = globals.threads;
        globals.threads = thread;
        ve_unlock(&globals.lock);
    }

    stack = NULL;
    tls = NULL;
    ret = 0;

done:

    if (stack)
        ve_free(stack);

    if (tls)
        ve_free(tls);

    return ret;
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
        uint64_t tcs = buf->arg1;
        size_t stack_size = (size_t)buf->arg2;

        if (_create_new_thread(sock, tcs, stack_size) != 0)
            goto done;
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
    g_tdata_size = arg.tdata_size;
    g_tdata_align = arg.tdata_align;
    g_tbss_size = arg.tbss_size;
    g_tbss_align = arg.tbss_align;

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
    thread_t* p;

    OE_UNUSED(fd);
    OE_UNUSED(buf);

    /* Wait on the exit status of each thread. */
    for (p = globals.threads; p; p = p->next)
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
    for (p = globals.threads; p;)
    {
        thread_t* next = p->next;
        ve_close(p->sock);
        ve_free(p->stack);
        ve_free(p->tls);
        p = next;
    }

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

#if 0
    const uint8_t arr[8] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

    for (size_t i = 0; i < OE_COUNTOF(arr); i++)
        __arr[i] = arr[i];
#endif

#if 0
    ve_dump_thread(ve_thread_self());
#endif

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

#if 1
    {
        const void* tls;
        size_t tls_size;
        thread_t* thread = ve_thread_self();

        if (ve_get_tls(thread, &tls, &tls_size) != 0)
        {
            ve_puts("ve_get_tls() failed");
            ve_exit(1);
        }

        ve_hexdump(tls, tls_size + 64);
    }
#endif

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

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "thread.h"
#include <openenclave/internal/utils.h>
#include "globals.h"
#include "hexdump.h"
#include "malloc.h"
#include "print.h"
#include "process.h"
#include "signal.h"
#include "string.h"
#include "syscall.h"

#define VE_MAX_THREADS 1024
#define VE_PAGE_SIZE 4096
#define VE_TLS_SIZE (1 * VE_PAGE_SIZE)

/* The thread ABI for Linux x86-64: do not change! */
typedef struct _thread_base
{
    struct _thread_base* self;
    uint64_t dtv;
    uint64_t unused1;
    uint64_t unused2;
    uint64_t sysinfo;
    uint64_t canary;
    uint64_t canary2;
} thread_base_t;

/* Represents a thread (the FS register points to instances of this) */
typedef struct _thread
{
    thread_base_t base;

    /* Internal implementation. */
    struct _thread* next;
    void* tls;
    size_t tls_size;
    void* stack;
    size_t stack_size;
    void* __thread_arg;
    void (*destructor)(void*);
    void* destructor_arg;
    int ptid;
    int ctid;
} thread_t;

/* Global array of threads. */
static thread_t* _threads;
static ve_lock_t _lock;

static thread_t* _thread_self(void)
{
    thread_t* thread = NULL;
    const long ARCH_GET_FS = 0x1003;
    ve_syscall2(OE_SYS_arch_prctl, ARCH_GET_FS, (long)&thread);
    return thread;
}

ve_thread_t ve_thread_self(void)
{
    ve_thread_t thread;
    thread.__impl = _thread_self();
    return thread;
}

static int _get_tls(const thread_t* thread, const void** tls, size_t* tls_size)
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

typedef struct _thread_arg
{
    thread_t* thread;
    int (*func)(void* arg);
    void* arg;
} thread_arg_t;

static int _thread_func(void* arg_)
{
    thread_arg_t* arg = (thread_arg_t*)arg_;
    int rval;

    /* Prepend the thread to the global linked list. */
    {
        ve_lock(&_lock);
        arg->thread->next = _threads;
        _threads = arg->thread;
        ve_unlock(&_lock);
    }

    /* Invoke the caller's thread routine. */
    rval = arg->func(arg->arg);

    return rval;
}

int ve_thread_create(
    ve_thread_t* thread_out,
    int (*func)(void*),
    void* arg,
    size_t stack_size)
{
    int ret = -1;
    uint8_t* stack = NULL;
    thread_t* main_thread;
    thread_t* thread;
    const void* main_tls;
    size_t main_tls_size;
    void* tls = NULL;

    if (thread_out)
        thread_out->__impl = NULL;

    if (!thread_out || !func || !stack_size)
        goto done;

    /* Get the main thread. */
    if (!(main_thread = _thread_self()))
        goto done;

    /* Get the TLS data from the main thread. */
    if (_get_tls(main_thread, &main_tls, &main_tls_size) != 0)
        goto done;

    /* Fail if the tdata section will not fit into the TLS. */
    if (main_tls_size < g_tdata_size)
        goto done;

    /* Allocate TLS followed by the thread. */
    {
        /* ATTN: Vaoid "unaddressable byte(s)" error in Valgrind. */
        const size_t EXTRA = 64;

        if (!(tls = ve_calloc(1, main_tls_size + sizeof(thread_t) + EXTRA)))
            goto done;
    }

    /* Set thread pointer into the middle of the allocation. */
    thread = (thread_t*)((uint8_t*)tls + main_tls_size);

    /* Copy tdata section onto the new_tls. */
    {
        void* tdata = (uint8_t*)ve_get_baseaddr() + g_tdata_rva;
        ve_memcpy(tls, tdata, g_tdata_size);
    }

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
        thread->tls_size = main_tls_size;
        thread->stack = stack;
        thread->stack_size = stack_size;
    }

    /* Create the new thread. */
    {
        int flags = 0;
        int rval;
        uint8_t* child_stack = stack + thread->stack_size;
        void* child_tls = (uint8_t*)tls + main_tls_size;
        thread_arg_t* thread_arg = NULL;

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

        /* Create the thread argument structure. */
        {
            if (!(thread_arg = ve_calloc(1, sizeof(thread_arg_t))))
                goto done;

            thread_arg->thread = thread;
            thread_arg->func = func;
            thread_arg->arg = arg;
            thread->__thread_arg = thread_arg;
        }

        if ((rval = ve_clone(
                 _thread_func,
                 child_stack,
                 flags,
                 thread_arg,
                 &thread->ptid,
                 child_tls,
                 &thread->ctid)) == -1)
        {
            ve_free(thread_arg);
            goto done;
        }

        ve_print("encl: new thread: rval=%d\n", rval);
    }

    thread_out->__impl = thread;

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

int ve_thread_set_destructor(void (*destructor)(void*), void* arg)
{
    int ret = -1;
    thread_t* thread;

    if (!destructor)
        goto done;

    if (!(thread = _thread_self()))
        goto done;

    thread->destructor = destructor;
    thread->destructor_arg = arg;

    ret = 0;

done:
    return ret;
}

int ve_thread_join_all(void)
{
    int ret = -1;
    thread_t* p;

    /* Wait on the exit status of each thread. */
    for (p = _threads; p; p = p->next)
    {
        int pid;
        int status;

        if ((pid = ve_waitpid(-1, &status, 0)) < 0)
            ve_panic("ve_waitpid() failed");
    }

    /* Release resources held by threads. */
    for (p = _threads; p;)
    {
        thread_t* next = p->next;

        if (p->destructor)
            (*p->destructor)(p->destructor_arg);

        ve_free(p->stack);

        ve_free(p->__thread_arg);

        /* This frees the tread_t struct. */
        ve_free(p->tls);

        p = next;
    }

    ret = 0;

    return ret;
}

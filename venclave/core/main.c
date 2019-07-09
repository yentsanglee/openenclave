// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/elf.h>
#include "../common/msg.h"
#include "calls.h"
#include "common.h"
#include "globals.h"
#include "hexdump.h"
#include "io.h"
#include "malloc.h"
#include "panic.h"
#include "print.h"
#include "process.h"
#include "recvfd.h"
#include "sbrk.h"
#include "shm.h"
#include "signal.h"
#include "socket.h"
#include "string.h"
#include "syscall.h"
#include "thread.h"
#include "time.h"
#include "trace.h"

#define MAX_THREADS 1024

#define THREAD_VALUE 0xaabbccddeeff1122

#if 1
#define USE_PROXY
#endif

__thread uint64_t _thread_value_tls = THREAD_VALUE;

static __thread int _pid_tls;

static bool _called_constructor = false;
static bool _called_destructor = false;

static ve_thread_t _threads[MAX_THREADS];
static size_t _nthreads;

/* pid and socket for the child proxy process. */
int __ve_proxy_pid;
int __ve_proxy_sock;

__attribute__((constructor)) static void constructor(void)
{
    VE_T(ve_printf("encl: constructor()\n");)
    _called_constructor = true;
}

__attribute__((destructor)) static void destructor(void)
{
    VE_T(ve_printf("encl: destructor()\n");)
    _called_destructor = true;
}

static int _thread_func(void* arg)
{
    int sock = (int)(int64_t)arg;
    int exit_status;

    __ve_thread_sock_tls = sock;

    _pid_tls = ve_getpid();

    if (_thread_value_tls != THREAD_VALUE)
        ve_panic("_thread_value_tls != THREAD_VALUE");

    _thread_value_tls = 0;

    if ((exit_status = ve_handle_calls(sock)) == -1)
        ve_panic("_thread(): ve_handle_calls() failed\n");

    return exit_status;
}

int ve_handle_call_add_thread(int fd, ve_call_buf_t* buf, int* exit_status)
{
    int sock = -1;

    OE_UNUSED(fd);
    OE_UNUSED(exit_status);

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
        ve_printf(
            "error: ve_shmat(1) failed: rval=%p shmid=%d shmaddr=%p\n",
            rval,
            shmid,
            shmaddr);
        goto done;
    }

    if (rval != shmaddr)
    {
        ve_printf(
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

static int _check_elf_header()
{
    int ret = -1;
    const uint8_t e_ident[] = {0x7f, 'E', 'L', 'F'};
    const elf64_ehdr_t* ehdr;

    if (!(ehdr = (const elf64_ehdr_t*)ve_get_elf_header()))
        goto done;

    /* Check the ELF magic number. */
    if (ve_memcmp(ehdr->e_ident, e_ident, sizeof(e_ident)) != 0)
        goto done;

    /* Check that this is an ELF-64-bit header. */
    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
        goto done;

    /* Check that the data encoding is 2's complement. */
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
        goto done;

    /* Check that this is an x86-64 machine. */
    if (ehdr->e_machine != EM_X86_64)
        goto done;

    /* Check the size of the ELF header. */
    if (ehdr->e_ehsize != sizeof(elf64_ehdr_t))
        goto done;

    /* Check the size of the program header structures. */
    if (ehdr->e_phentsize != sizeof(elf64_phdr_t))
        goto done;

    ret = 0;

done:
    return ret;
}

/* Create a proxy enclave and return a socket for communicating with it. */
static int _create_proxy(const char* path, int* sock)
{
    int ret = -1;
    int pid;
    int socks[2] = {-1, -1};

    if (sock)
        *sock = -1;

    if (!path || !sock)
        goto done;

    if (ve_socketpair(VE_AF_LOCAL, VE_SOCK_STREAM, 0, socks) == -1)
        goto done;

    if ((pid = ve_fork()) < 0)
        goto done;

    /* If inside the child process. */
    if (pid == 0)
    {
        char* argv[3];
        ve_dstr_buf buf;

        argv[0] = (char*)path;
        argv[1] = (char*)ve_dstr(&buf, socks[1], NULL);
        argv[2] = NULL;

        /* Close all non-standard file descriptors except socks[1]. */
        {
            const int max_fd = ve_getdtablesize() - 1;

            for (int i = VE_STDERR_FILENO + 1; i <= max_fd; i++)
            {
                if (i != socks[1])
                    ve_close(i);
            }
        }

        /* Execute the enclave proxy. */
        ve_execv(path, argv);
        ve_panic("ve_execv() failed");
    }

    *sock = socks[0];
    ret = pid;

done:

    if (socks[1] != -1)
        ve_close(socks[1]);

    return ret;
}

static int _handle_init(void)
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
    __ve_base_rva = arg.base_rva;

    /* Verify the ELF headfer (pointed to by the base address). */
    if (_check_elf_header() != 0)
        ve_panic("elf header check failed");

    /* Handle the request. */
    {
        __ve_sock = arg.sock;

        /* ATTN: use of this variable is tricky. */
        __ve_thread_sock_tls = __ve_sock;

        if (_attach_host_heap(arg.shmid, arg.shmaddr, arg.shmsize) != 0)
        {
            ve_put("_attach_host_heap() failed\n");
            retval = -1;
        }
    }

#if defined(USE_PROXY)
    {
        const char* path = arg.vproxyhost_path;

        /* Create the proxy process to handle SGX requests. */
        if ((__ve_proxy_pid = _create_proxy(path, &__ve_proxy_sock)) == -1)
            ve_panic("_create_proxy_enclave() failed");
    }
#endif /* defined(USE_PROXY) */

    /* Send response back on the socket. */
    if (ve_writen(__ve_sock, &retval, sizeof(retval)) != 0)
        goto done;

    ret = retval;

done:
    return ret;
}

int ve_handle_post_init(int fd, ve_call_buf_t* buf, int* exit_status)
{
    extern void __libc_csu_init(void);

    OE_UNUSED(fd);
    OE_UNUSED(buf);
    OE_UNUSED(exit_status);

    /* Invoke constructors, which may perform ocalls. */
    __libc_csu_init();

    return 0;
}

static void _terminate_proxy(int pid, int sock)
{
    int status = 0;
    int r;

    /* Tell the proxy to terminate. */
    {
        ve_msg_type_t type;
        void* data;
        size_t size;

        if (ve_msg_send(sock, VE_MSG_TERMINATE, NULL, 0) != 0)
            ve_panic("failed to send terminate message to proxy");

        if (ve_msg_recv_any(sock, &type, &data, &size) != 0)
            ve_panic("failed to receive terminate message to proxy");

        ve_close(sock);
    }

    /* Wait for the child to exit. */
    while ((r = ve_waitpid(pid, &status, 0)) < 0)
        ;

    if (r != pid)
        ve_panic("waitpid failed on proxy");

    if (VE_WEXITSTATUS(status) != 0)
        ve_panic("bad proxy exit status");
}

int ve_handle_call_terminate(int fd, ve_call_buf_t* buf, int* exit_status)
{
    OE_UNUSED(fd);
    OE_UNUSED(buf);

    /* Wait for all threads to gracefuly shutdown. */
    for (size_t i = 0; i < _nthreads; i++)
    {
        int retval;

        if (ve_thread_join(_threads[i], &retval) != 0)
            ve_panic("failed to join threads");

        VE_T(ve_printf("encl: join: retval=%d\n", retval);)
    }

    /* Call the atexit handlers. */
    {
        extern void oe_call_atexit_functions(void);
        oe_call_atexit_functions();
    }

    /* Call any destructors. */
    {
        extern void __libc_csu_fini(void);
        __libc_csu_fini();

        /* Self-test for destructors. */
        if (!_called_destructor)
            ve_panic("_destructor() not called");
    }

#if defined(USE_PROXY)

    /* Terminate the proxy. */
    _terminate_proxy(__ve_proxy_pid, __ve_proxy_sock);

#endif /* defined(USE_PROXY) */

    /* Terminate. */
    *exit_status = 0;
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

int ve_handle_call_terminate_thread(
    int fd,
    ve_call_buf_t* buf,
    int* exit_status)
{
    OE_UNUSED(fd);
    OE_UNUSED(buf);

    VE_T(ve_printf("encl: thread exit: tid=%d\n", ve_gettid());)

    *exit_status = 99;

#if 0
    uint32_t sec = (uint32_t)(2 + (ve_gettid() - __ve_main_pid) * 2);
    ve_printf("sec=%u\n", sec);
    ve_sleep(sec);
#endif

    return 0;
}

void _main_sig_handler(int arg)
{
    (void)arg;
    ve_write(VE_STDERR_FILENO, "sighandler\n", 11);
}

void test_signals(void)
{
    if (ve_signal(VE_SIGUSR1, _main_sig_handler) == VE_SIG_ERR)
        ve_panic("ve_signal() failed");

    if (ve_kill(__ve_main_pid, VE_SIGUSR1) != 0)
        ve_panic("ve_kill() failed");
}

static bool _called_sigusr1;

static void _sigusr1(int sig)
{
    (void)sig;
    _called_sigusr1 = true;
}

int main(void)
{
    int exit_status;

    __ve_main_pid = ve_getpid();

    /* Self-test for signals. */
    {
        ve_signal(VE_SIGUSR1, _sigusr1);
        ve_kill(ve_getpid(), VE_SIGUSR1);

        if (!_called_sigusr1)
            ve_panic("_sigusr1() not called");

        ve_signal(VE_SIGUSR1, VE_SIG_DFL);
    }

#if 0
    test_signals();
#endif

    /* Wait here to be initialized and to receive the main socket. */
    if (_handle_init() != 0)
        ve_panic("ve_handle_init() failed");

    /* Handle messages over the main socket. */
    if ((exit_status = ve_handle_calls(__ve_sock)) == -1)
        ve_panic("ve_handle_calls() failed");

    /* Close the standard descriptors. */
    ve_close(VE_STDIN_FILENO);
    ve_close(VE_STDOUT_FILENO);
    ve_close(VE_STDERR_FILENO);

    return exit_status;
}

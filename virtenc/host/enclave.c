// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "enclave.h"
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "call.h"
#include "elf.h"
#include "heap.h"
#include "hostmalloc.h"
#include "io.h"

#define MAX_THREADS 1024

#define VE_PAGE_SIZE 4096

typedef struct _thread
{
    int sock;
    int child_sock;
    uint64_t tcs;
} thread_t;

struct _ve_enclave
{
    int pid;
    int sock;
    int child_sock;

    thread_t threads[MAX_THREADS];
    size_t nthreads;
    pthread_spinlock_t lock;
};

extern const char* __ve_arg0;

static int _init_child(
    ve_enclave_t* enclave,
    int child_fd,
    int child_sock,
    ve_elf_info_t* elf_info)
{
    int ret = -1;
    ve_init_arg_t arg;
    int retval;

    if (!enclave || child_fd < 0 || child_sock < 0)
        goto done;

    arg.magic = VE_INIT_ARG_MAGIC;
    arg.sock = child_sock;
    arg.shmid = __ve_heap.shmid;
    arg.shmaddr = __ve_heap.shmaddr;
    arg.tdata_rva = elf_info->tdata_rva;
    arg.tdata_size = elf_info->tdata_size;
    arg.tdata_align = elf_info->tdata_align;
    arg.tbss_rva = elf_info->tbss_rva;
    arg.tbss_size = elf_info->tbss_size;
    arg.tbss_align = elf_info->tbss_align;
    arg.self_rva = elf_info->self_rva;

    /* Send the message to the child's standard input. */
    if (ve_writen(child_fd, &arg, sizeof(arg)) != 0)
        goto done;

    /* Receive the message on the socket. */
    if (ve_readn(enclave->sock, &retval, sizeof(retval)) != 0)
        goto done;

    if (retval == -1)
        goto done;

    ret = 0;

done:

    return ret;
}

static pid_t _fork_exec_enclave(
    const char* path,
    ve_enclave_t* enclave,
    ve_elf_info_t* elf_info)
{
    pid_t ret = -1;
    pid_t pid;
    int fds[2] = {-1, -1};
    int socks[2] = {-1, -1};

    if (!path || access(path, X_OK) != 0 || !enclave)
        goto done;

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, socks) == -1)
        goto done;

    if (pipe(fds) == -1)
        goto done;

    if ((pid = fork()) < 0)
        goto done;

    /* If child. */
    if (pid == 0)
    {
        char* argv[2] = {(char*)path, NULL};
        dup2(fds[0], STDIN_FILENO);

        /* Close all non-standard file descriptors except socks[1]. */
        {
            const int max_fd = getdtablesize() - 1;

            for (int i = STDERR_FILENO + 1; i <= max_fd; i++)
            {
                if (i != socks[1])
                    close(i);
            }
        }

        execv(path, argv);

        fprintf(stderr, "%s: execv() failed\n", __ve_arg0);
        abort();
    }

    enclave->sock = socks[0];
    enclave->child_sock = socks[1];

    if (_init_child(enclave, fds[1], socks[1], elf_info) != 0)
        goto done;

    ret = pid;

done:

    if (fds[0] != -1)
        close(fds[0]);

    if (fds[1] != -1)
        close(fds[1]);

    if (socks[1] != -1)
        close(socks[1]);

    if (ret == -1 && socks[0] != -1)
        close(socks[0]);

    return ret;
}

static int _terminate_enclave_process(ve_enclave_t* enclave)
{
    int ret = -1;

    if (!enclave)
        goto done;

    /* Terminate the enclave threads. */
    for (size_t i = 0; i < enclave->nthreads; i++)
    {
        const int sock = enclave->threads[i].sock;
        const int child_sock = enclave->threads[i].child_sock;

        if (ve_call_send0(sock, VE_FUNC_TERMINATE_THREAD) != 0)
            goto done;

        close(sock);
        close(child_sock);
    }

    /* Terminate the main enclave thread. */
    {
        const int sock = enclave->sock;
        const int child_sock = enclave->child_sock;

        if (ve_call_send0(sock, VE_FUNC_TERMINATE) != 0)
            goto done;

        close(sock);
        close(child_sock);
    }

    ret = 0;

done:
    return ret;
}

static int _add_enclave_thread(
    ve_enclave_t* enclave,
    uint64_t tcs,
    size_t stack_size)
{
    int ret = -1;
    int socks[2] = {-1, -1};
    extern int send_fd(int sock, int fd);

    /* Fail if no more threads. */
    if (enclave->nthreads == MAX_THREADS)
        goto done;

    /* Create the socket pair. */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socks) == -1)
        goto done;

    /* Send the request. */
    if (ve_call_send2(enclave->sock, VE_FUNC_ADD_THREAD, tcs, stack_size) != 0)
        goto done;

    /* Send the fd to the enclave after send but before receive. */
    if (send_fd(enclave->sock, socks[1]) != 0)
        goto done;

    /* Receive the response. */
    {
        uint64_t retval;

        if (ve_call_recv(enclave->sock, &retval) != 0 || retval != 0)
            goto done;
    }

    pthread_spin_lock(&enclave->lock);
    {
        enclave->threads[enclave->nthreads].sock = socks[0];
        enclave->threads[enclave->nthreads].child_sock = socks[1];
        enclave->threads[enclave->nthreads].tcs = tcs;
        enclave->nthreads++;
    }
    pthread_spin_unlock(&enclave->lock);

    socks[0] = -1;
    socks[1] = -1;

    ret = 0;

done:

    if (socks[0] != -1)
        close(socks[0]);

    if (socks[1] != -1)
        close(socks[1]);

    return ret;
}

static int _lookup_thread_sock(ve_enclave_t* enclave, uint64_t tcs)
{
    int sock = -1;

    if (!enclave)
        goto done;

    pthread_spin_lock(&enclave->lock);
    {
        for (size_t i = 0; i < enclave->nthreads; i++)
        {
            if (enclave->threads[i].tcs == tcs)
            {
                sock = enclave->threads[i].sock;
                break;
            }
        }
    }
    pthread_spin_unlock(&enclave->lock);

done:
    return sock;
}

static int _get_child_exit_status(int pid, int* status_out)
{
    int ret = -1;
    int r;
    int status;

    *status_out = 0;

    while ((r = waitpid(pid, &status, 0)) == -1 && errno == EINTR)
        ;

    if (r != pid)
        goto done;

    *status_out = WEXITSTATUS(status);

    ret = 0;

done:
    return ret;
}

extern int __ve_pid;

void ve_handle_call_ping(ve_call_buf_t* buf)
{
    extern int gettid(void);

    printf("host: ping: value=%lx\n", buf->arg1);

    buf->retval = buf->arg1;
}

int ve_enclave_create(const char* path, ve_enclave_t** enclave_out)
{
    int ret = -1;
    ve_enclave_t* enclave = NULL;
    size_t stack_size;
    ve_elf_info_t elf_info;

    if (enclave_out)
        *enclave_out = NULL;

    if (!path)
        goto done;

    if (ve_get_elf_info(path, &elf_info) != 0)
        goto done;

    if (!(enclave = calloc(1, sizeof(ve_enclave_t))))
        goto done;

    pthread_spin_init(&enclave->lock, PTHREAD_PROCESS_PRIVATE);

    if ((enclave->pid = _fork_exec_enclave(path, enclave, &elf_info)) == -1)
        goto done;

    ve_enclave_settings_t settings;

    /* Get the enclave settings. */
    if (ve_enclave_get_settings(enclave, &settings) != 0)
        goto done;

    stack_size = settings.num_stack_pages * VE_PAGE_SIZE;

    /* Add threads to the child process. */
    for (uint64_t i = 0; i < settings.num_tcs; i++)
    {
        if (_add_enclave_thread(enclave, i, stack_size) != 0)
            goto done;
    }

    *enclave_out = enclave;
    enclave = 0;

    ret = 0;

done:

    if (enclave)
    {
        /* ATTN: cleanup enclave stuff */
    }

    return ret;
}

int ve_enclave_terminate(ve_enclave_t* enclave)
{
    int ret = -1;
    int status;

    if (!enclave)
        goto done;

    /* Terminate the enclave process. */
    if (_terminate_enclave_process(enclave) != 0)
        goto done;

    /* Wait for child to exit. */
    if (_get_child_exit_status(enclave->pid, &status) != 0)
        goto done;

    printf("host: child exit status: %d\n", status);

    pthread_spin_destroy(&enclave->lock);

    free(enclave);

    ret = 0;

done:
    return ret;
}

int ve_enclave_ping(ve_enclave_t* enclave, uint64_t tcs, uint64_t ping_value)
{
    int ret = -1;
    uint64_t retval;
    int sock;

    if (!enclave)
        goto done;

    if (tcs == (uint64_t)-1)
    {
        sock = enclave->sock;
    }
    else if ((sock = _lookup_thread_sock(enclave, tcs)) == -1)
    {
        goto done;
    }

    if (ve_call1(sock, VE_FUNC_PING, &retval, ping_value) != 0)
        goto done;

    if (retval != ping_value)
        goto done;

    ret = 0;

done:
    return ret;
}

int ve_enclave_get_settings(ve_enclave_t* enclave, ve_enclave_settings_t* buf)
{
    int ret = -1;
    uint64_t retval;
    int sock;
    ve_enclave_settings_t* settings = NULL;

    if (buf)
        memset(buf, 0, sizeof(ve_enclave_settings_t));

    if (!enclave || !buf)
        goto done;

    if (!(settings = ve_host_calloc(1, sizeof(ve_enclave_settings_t))))
        goto done;

    sock = enclave->sock;

    if (ve_call1(sock, VE_FUNC_GET_SETTINGS, &retval, (uint64_t)settings) != 0)
        goto done;

    if (retval != 0)
        goto done;

    *buf = *settings;

    ret = 0;

done:

    if (settings)
        ve_host_free(settings);

    return ret;
}

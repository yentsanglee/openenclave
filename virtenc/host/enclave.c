// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "enclave.h"
#include <errno.h>
#include <fcntl.h>
#include <openenclave/bits/defs.h>
#include <openenclave/internal/syscall/unistd.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "call.h"
#include "heap.h"
#include "hostmalloc.h"
#include "io.h"
#include "trace.h"

extern const char* arg0;

static int _init_child(ve_enclave_t* enclave, int child_fd, int child_sock)
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

static pid_t _exec(const char* path, ve_enclave_t* enclave)
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
        dup2(fds[0], STDIN_FILENO);
        close(fds[0]);
        close(fds[1]);
        close(socks[0]);

        char* argv[2] = {(char*)path, NULL};
        execv(path, argv);

        fprintf(stderr, "%s: execv() failed\n", arg0);
        abort();
    }

    enclave->sock = socks[0];
    enclave->child_sock = socks[1];

    if (_init_child(enclave, fds[1], socks[1]) != 0)
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

/* Should be called when there is only one surviving thread in the system. */
static int _terminate_enclave_process(ve_enclave_t* enclave)
{
    int ret = -1;

    if (!enclave)
        goto done;

    /* Terminate the enclave threads. */
    for (size_t i = 0; i < enclave->threads_size; i++)
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

int _add_enclave_thread(ve_enclave_t* enclave, int tcs, size_t stack_size)
{
    int ret = -1;
    int socks[2] = {-1, -1};
    extern int send_fd(int sock, int fd);
    ve_add_thread_arg_t* arg = NULL;

    /* Fail if no more threads. */
    if (enclave->threads_size == MAX_THREADS)
        goto done;

    /* Create the socket pair. */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socks) == -1)
        goto done;

    /* Create the call argument. */
    {
        if (!(arg = ve_host_calloc(1, sizeof(ve_add_thread_arg_t))))
            goto done;

        arg->tcs = tcs;
        arg->stack_size = stack_size;
    }

    /* Send the request. */
    if (ve_call_send1(enclave->sock, VE_FUNC_ADD_THREAD, (uint64_t)arg) != 0)
        goto done;

    /* Send the fd to the enclave after send but before receive. */
    if (send_fd(enclave->sock, socks[1]) != 0)
        goto done;

    /* Receive the response. */
    if (ve_call_recv(enclave->sock, NULL) != 0)
        goto done;

    if (arg->retval != 0)
        goto done;

    pthread_spin_lock(&enclave->threads_lock);
    {
        enclave->threads[enclave->threads_size].sock = socks[0];
        enclave->threads[enclave->threads_size].child_sock = socks[1];
        enclave->threads[enclave->threads_size].tcs = tcs;
        enclave->threads_size++;
    }
    pthread_spin_unlock(&enclave->threads_lock);

    socks[0] = -1;
    socks[1] = -1;

    ret = 0;

done:

    if (socks[0] != -1)
        close(socks[0]);

    if (socks[1] != -1)
        close(socks[1]);

    if (arg)
        ve_host_free(arg);

    return ret;
}

static int _lookup_thread_sock(ve_enclave_t* enclave, uint64_t tcs)
{
    int sock = -1;

    if (!enclave)
        goto done;

    pthread_spin_lock(&enclave->threads_lock);
    {
        for (size_t i = 0; i < enclave->threads_size; i++)
        {
            if (enclave->threads[i].tcs == tcs)
            {
                sock = enclave->threads[i].sock;
                break;
            }
        }
    }
    pthread_spin_unlock(&enclave->threads_lock);

done:
    return sock;
}

static int _call_ping(int sock)
{
    int ret = -1;
    const uint64_t VALUE = 12345;
    uint64_t retval;

    if (ve_call1(sock, VE_FUNC_PING, &retval, VALUE) != 0)
        goto done;

    if (retval != VALUE)
        goto done;

    ret = 0;

done:
    return ret;
}

int _ping_thread(ve_enclave_t* enclave, int tcs)
{
    int ret = -1;
    int sock;

    if (!enclave)
        goto done;

    /* If a thread with this TCS was not found. */
    if ((sock = _lookup_thread_sock(enclave, tcs)) == -1)
        goto done;

    /* Ping the enclave thread. */
    if (_call_ping(sock) != 0)
        goto done;

    ret = 0;

done:

    return ret;
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

void ve_handle_call_ping(ve_call_buf_t* buf)
{
    extern int gettid(void);

    printf("host: ping: pid=%d value=%lu\n", getpid(), buf->arg1);

    buf->retval = buf->arg1;
}

int ve_create_enclave(const char* path, ve_enclave_t** enclave_out)
{
    int ret = -1;
    ve_enclave_t* enclave = NULL;
    const size_t STACK_SIZE = 4096 * 256;

    if (!path)
        goto done;

    if (enclave_out)
        *enclave_out = NULL;

    if (!(enclave = calloc(1, sizeof(ve_enclave_t))))
        goto done;

    pthread_spin_init(&enclave->threads_lock, PTHREAD_PROCESS_PRIVATE);

    if ((enclave->pid = _exec(path, enclave)) == -1)
        goto done;

    /* Add threads to the child process. */
    if (_add_enclave_thread(enclave, 0, STACK_SIZE) != 0)
        goto done;

    if (_add_enclave_thread(enclave, 1, STACK_SIZE) != 0)
        goto done;

    if (_add_enclave_thread(enclave, 2, STACK_SIZE) != 0)
        goto done;

    /* Ping each of the threads. */
    {
        if (_ping_thread(enclave, 0) != 0)
            goto done;

        if (_ping_thread(enclave, 1) != 0)
            goto done;

        if (_ping_thread(enclave, 2) != 0)
            goto done;
    }

    /* Ping the main thread. */
    if (_call_ping(enclave->sock) != 0)
        goto done;

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

int ve_terminate_enclave(ve_enclave_t* enclave)
{
    int ret = -1;
    int status;

    if (!enclave)
        goto done;

    pthread_spin_destroy(&enclave->threads_lock);

    /* Terminate the enclave process. */
    if (_terminate_enclave_process(enclave) != 0)
        goto done;

    /* Wait for child to exit. */
    if (_get_child_exit_status(enclave->pid, &status) != 0)
        goto done;

    printf("host: child exit status: %d\n", status);

    free(enclave);

    ret = 0;

done:
    return ret;
}

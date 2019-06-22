// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include <openenclave/internal/syscall/unistd.h>
#include "../common/msg.h"

#include <errno.h>
#include <fcntl.h>
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
#include "globals.h"
#include "hostmalloc.h"
#include "trace.h"

const char* arg0;

globals_t globals;

OE_PRINTF_FORMAT(1, 2)
void err(const char* fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: error: ", arg0);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

OE_PRINTF_FORMAT(1, 2)
void puterr(const char* fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: error: ", arg0);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

/* Create a shared-memory heap for making ecalls and ocalls. */
static int _create_host_heap(globals_t* globals, size_t heap_size)
{
    int ret = -1;
    const int PERM = (S_IRUSR | S_IWUSR);
    int shmid = -1;
    void* shmaddr = NULL;

    if ((shmid = shmget(IPC_PRIVATE, heap_size, PERM)) == -1)
        goto done;

    if ((shmaddr = shmat(shmid, NULL, 0)) == (void*)-1)
        goto done;

    globals->shmid = shmid;
    globals->shmaddr = shmaddr;
    globals->shmsize = heap_size;
    shmid = -1;
    shmaddr = NULL;

    ret = 0;

done:

    if (shmaddr)
        shmdt(shmaddr);

    if (shmid != -1)
        shmctl(shmid, IPC_RMID, NULL);

    return ret;
}

static int _init_child(int child_fd, int child_sock)
{
    int ret = -1;
    ve_init_arg_t arg;
    uint64_t retval;

    arg.magic = VE_INIT_ARG_MAGIC;
    arg.sock = child_sock;
    arg.shmid = globals.shmid;
    arg.shmaddr = globals.shmaddr;

    *((uint64_t*)globals.shmaddr) = 0xffffffffffffffff;

    /* Send the message to the child's standard input. */
    if (ve_send_n(child_fd, &arg, sizeof(arg)) != 0)
        goto done;

    /* Receive the message on the socket. */
    if (ve_recv_n(globals.sock, &retval, sizeof(retval)) != 0)
        goto done;

    if (retval != 0)
        goto done;

    /* Check that child was able to write to shared memory. */
    if (*((uint64_t*)globals.shmaddr) != VE_SHMADDR_MAGIC)
    {
        puterr("shared memory crosscheck failed");
        goto done;
    }

    ret = 0;

done:

    return ret;
}

static pid_t _exec(const char* path)
{
    pid_t ret = -1;
    pid_t pid;
    int fds[2] = {-1, -1};
    int socks[2] = {-1, -1};

    if (!path || access(path, X_OK) != 0)
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

    globals.sock = socks[0];
    globals.child_sock = socks[1];

    if (_init_child(fds[1], socks[1]) != 0)
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
static int _terminate_enclave_process(void)
{
    int ret = -1;

    /* Terminate the enclave threads. */
    for (size_t i = 0; i < globals.num_threads; i++)
    {
        const int sock = globals.threads[i].sock;
        const int child_sock = globals.threads[i].child_sock;

        if (ve_call_send(sock, VE_FUNC_TERMINATE_THREAD, 0) != 0)
        {
            puterr("ve_call_send() failed: VE_FUNC_TERMINATE_THREAD");
            goto done;
        }

        close(sock);
        close(child_sock);
    }

    /* Terminate the main enclave thread. */
    {
        const int sock = globals.sock;
        const int child_sock = globals.child_sock;

        if (ve_call_send(sock, VE_FUNC_TERMINATE, 0) != 0)
        {
            puterr("_call_terminate() failed");
            goto done;
        }

        close(sock);
        close(child_sock);
    }

    ret = 0;

done:
    return ret;
}

int _add_enclave_thread(int tcs, size_t stack_size)
{
    int ret = -1;
    int socks[2] = {-1, -1};
    extern int send_fd(int sock, int fd);
    ve_add_thread_arg_t* arg = NULL;

    /* Fail if no more threads. */
    if (globals.num_threads == MAX_THREADS)
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

    /* Send ther request. */
    if (ve_call_send(globals.sock, VE_FUNC_ADD_THREAD, (uint64_t)arg) != 0)
        goto done;

    /* Send the fd to the enclave after send but before receive. */
    if (send_fd(globals.sock, socks[1]) != 0)
        goto done;

    /* Receive the response. */
    if (ve_call_recv(globals.sock, NULL) != 0)
        goto done;

    if (arg->ret != 0)
        goto done;

    ve_lock(&globals.threads_lock);
    {
        globals.threads[globals.num_threads].sock = socks[0];
        globals.threads[globals.num_threads].child_sock = socks[1];
        globals.threads[globals.num_threads].tcs = tcs;
        globals.num_threads++;
    }
    ve_unlock(&globals.threads_lock);

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

static int _lookup_thread_sock(uint64_t tcs)
{
    int sock = -1;

    ve_lock(&globals.threads_lock);
    {
        for (size_t i = 0; i < globals.num_threads; i++)
        {
            if (globals.threads[i].tcs == tcs)
            {
                sock = globals.threads[i].sock;
                break;
            }
        }
    }
    ve_unlock(&globals.threads_lock);

    return sock;
}

static int _call_ping(int sock)
{
    int ret = -1;
    const uint64_t VALUE = 12345;
    uint64_t value;

    if (ve_call(sock, VE_FUNC_PING, VALUE, &value) != 0)
        goto done;

    if (value != VALUE)
        goto done;

    ret = 0;

done:
    return ret;
}

int _ping_thread(int tcs)
{
    int ret = -1;
    int sock;

    /* If a thread with this TCS was not found. */
    if ((sock = _lookup_thread_sock(tcs)) == -1)
        goto done;

    /* Ping the thread. */
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

int main(int argc, const char* argv[])
{
    arg0 = argv[0];
    pid_t pid;
    int status;
    const size_t STACK_SIZE = 4096 * 256;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s program\n", argv[0]);
        exit(1);
    }

    /* Create shared memory before fork-exec. */
    if (_create_host_heap(&globals, HOST_HEAP_SIZE) != 0)
        err("failed to allocate shared memory");

    /* Create the child process. */
    if ((pid = _exec(argv[1])) == -1)
        err("failed to execute %s", argv[1]);

    /* Add threads to the child process. */
    if (_add_enclave_thread(0, STACK_SIZE) != 0)
        err("failed to add child thread");

    if (_add_enclave_thread(1, STACK_SIZE) != 0)
        err("failed to add child thread");

    if (_add_enclave_thread(2, STACK_SIZE) != 0)
        err("failed to add child thread");

    /* Ping each of the threads. */
    {
        if (_ping_thread(0) != 0)
            err("host: _ping_thread() failed");

        if (_ping_thread(1) != 0)
            err("host: _ping_thread() failed");

        if (_ping_thread(2) != 0)
            err("host: _ping_thread() failed");
    }

    /* Ping the main thread. */
    if (_call_ping(globals.sock) != 0)
        err("host: failed to ping main thread");

    /* Terminate the enclave process. */
    if (_terminate_enclave_process() != 0)
        err("failed to terminate child");

    /* Wait for child to exit. */
    if (_get_child_exit_status(pid, &status) != 0)
        err("failed to get child exit status");

    printf("host: child exit status: %d\n", status);

    close(OE_STDIN_FILENO);
    close(OE_STDOUT_FILENO);
    close(OE_STDERR_FILENO);

    /* ATTN: close sockets and release shared memory. */

    return 0;
}

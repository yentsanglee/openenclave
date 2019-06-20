// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include <openenclave/internal/syscall/unistd.h>
#include "../common/msg.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "globals.h"

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

static int _init_child(int child_fd, int child_sock)
{
    int ret = -1;
    ve_msg_init_in_t in;
    const ve_msg_type_t type = VE_MSG_INIT;

    in.sock = child_sock;

    if (ve_send_msg(child_fd, type, &in, sizeof(in)) != 0)
        goto done;

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
static int _terminate_child(void)
{
    int ret = -1;
    const ve_msg_type_t type = VE_MSG_TERMINATE;

    /* Terminate child threads. */
    for (size_t i = 0; i < globals.num_threads; i++)
    {
        const int sock = globals.threads[i].sock;
        const int child_sock = globals.threads[i].child_sock;
        ve_msg_terminate_out_t out;

        if (ve_send_msg(sock, type, NULL, 0) != 0)
            goto done;

        if (ve_recv_msg_by_type(sock, type, &out, sizeof(out)) != 0)
            goto done;

        if (out.ret != 0)
            goto done;

        close(sock);
        close(child_sock);
    }

    /* Terminate the main thread. */
    {
        const int sock = globals.sock;
        const int child_sock = globals.child_sock;
        ve_msg_terminate_out_t out;

        if (ve_send_msg(sock, type, NULL, 0) != 0)
            goto done;

        if (ve_recv_msg_by_type(sock, type, &out, sizeof(out)) != 0)
            goto done;

        if (out.ret != 0)
            goto done;

        close(sock);
        close(child_sock);
    }

    ret = 0;

done:
    return ret;
}

int _add_child_thread(int tcs, size_t stack_size)
{
    int ret = -1;
    const ve_msg_type_t type = VE_MSG_ADD_THREAD;
    int socks[2] = {-1, -1};
    extern int send_fd(int sock, int fd);

    /* Fail if no more threads. */
    if (globals.num_threads == MAX_THREADS)
        goto done;

    /* Send the request. */
    {
        ve_msg_add_thread_in_t in;

        memset(&in, 0, sizeof(in));
        in.tcs = tcs;
        in.stack_size = stack_size;

        if (ve_send_msg(globals.sock, type, &in, sizeof(in)) != 0)
            goto done;
    }

    /* Create socket pair and send one side to the child. */
    {
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, socks) == -1)
            goto done;

        /* Send the fd to the enclave */
        if (send_fd(globals.sock, socks[1]) != 0)
            err("send_fd() failed");
    }

    /* Receive the response. */
    {
        ve_msg_add_thread_out_t out;

        if (ve_recv_msg_by_type(globals.sock, type, &out, sizeof(out)) != 0)
            goto done;

        if (out.ret != 0)
            goto done;
    }

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

    return ret;
}

int _ping_thread(int tcs)
{
    int ret = -1;
    ve_msg_ping_thread_out_t out;
    const ve_msg_type_t type = VE_MSG_PING_THREAD;
    int sock = -1;

    /* Select the thread to ping. */
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

    /* If the the thread not found. */
    if (sock == -1)
        goto done;

    /* Send the message */
    if (ve_send_msg(sock, type, NULL, 0) != 0)
        goto done;

    /* Receive the response. */
    if (ve_recv_msg_by_type(sock, type, &out, sizeof(out)) != 0)
        goto done;

    if (out.ret != 0)
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

    /* Create the child process. */
    if ((pid = _exec(argv[1])) == -1)
        err("failed to execute %s", argv[1]);

    /* Add threads to the child process. */
    _add_child_thread(0, STACK_SIZE);
    _add_child_thread(1, STACK_SIZE);
    _add_child_thread(2, STACK_SIZE);

    /* Ping each of the threads. */
    _ping_thread(0);
    _ping_thread(1);
    _ping_thread(2);

    /* Terminate the child process. */
    _terminate_child();

    /* Wait for child to exit. */
    if (_get_child_exit_status(pid, &status) != 0)
        err("failed to get child exit status");

    printf("child exit status: %d\n", status);

    close(OE_STDIN_FILENO);
    close(OE_STDOUT_FILENO);
    close(OE_STDERR_FILENO);

    return 0;
}

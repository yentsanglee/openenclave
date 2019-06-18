// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/defs.h>
#include <openenclave/internal/syscall/unistd.h>
#include "../common/msg.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "globals.h"

const char* arg0;

globals_t globals;

OE_PRINTF_FORMAT(1, 2)
static void err(const char* fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: error: ", arg0);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

int ve_init_child(int child_fd)
{
    int ret = -1;
    ve_msg_init_in_t in;
    const ve_msg_type_t type = VE_MSG_INIT;

    in.sock = globals.child_sock;

    if (ve_send_msg(child_fd, type, &in, sizeof(in)) != 0)
        goto done;

    /* Test the socket connection between child and parent. */
    {
        int sock = -1;
        bool eof;

        if (ve_recv_n(globals.sock, &sock, sizeof(sock), &eof) != 0)
            err("init failed: read of sock failed");

        if (sock != globals.child_sock)
            err("init failed: sock confirm failed");

        printf("sock=%d\n", sock);
    }

    ret = 0;

done:
    return ret;
}

pid_t exec(const char* path)
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

    if (ve_init_child(fds[1]) != 0)
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

int ve_terminate_child(void)
{
    int ret = -1;
    ve_msg_terminate_in_t in;
    ve_msg_terminate_out_t out;
    const ve_msg_type_t type = VE_MSG_TERMINATE;
    bool eof;

    in.status = 0;

    if (ve_send_msg(globals.sock, type, &in, sizeof(in)) != 0)
        goto done;

    if (ve_recv_msg_by_type(globals.sock, type, &out, sizeof(out), &eof) != 0)
        goto done;

    printf("terminate response: ret=%d\n", out.ret);

    ret = 0;

done:
    return ret;
}

int ve_msg_new_thread(int tcs)
{
    int ret = -1;
    ve_msg_new_thread_in_t in;
    ve_msg_new_thread_out_t out;
    const ve_msg_type_t type = VE_MSG_NEW_THREAD;
    bool eof;

    in.tcs = tcs;

    if (ve_send_msg(globals.sock, type, &in, sizeof(in)) != 0)
        goto done;

#if 0
    int socks[2];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socks) == -1)
        goto done;

printf("socks[0]=%d\n", socks[0]);
printf("socks[1]=%d\n", socks[1]);

    /* Send the fd to the enclave */
    {
        int retval;

        if ((retval = ioctl(fds[1], I_SENDFD, socks[1])) == -1)
        {
            printf("*** ioctl(): retval=%d errno=%d\n", retval, errno);
            goto done;
        }
    }
#endif

    if (ve_recv_msg_by_type(globals.sock, type, &out, sizeof(out), &eof) != 0)
        goto done;

    printf("new_thread response: ret=%d\n", out.ret);

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

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s program\n", argv[0]);
        exit(1);
    }

    if ((pid = exec(argv[1])) == -1)
        err("failed to execute %s", argv[1]);

    ve_msg_new_thread(0);

    /* Terminate the child process. */
    ve_terminate_child();

    /* Wait for child to exit. */
    if (_get_child_exit_status(pid, &status) != 0)
        err("failed to get child exit status");

    printf("child exit status: %d\n", status);

    return 0;
}

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

pid_t exec(const char* path, int* child_fd)
{
    pid_t ret = -1;
    pid_t pid;
    int stdin_pipe[2]; /* child's stdin pipe */
    int socks[2];

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, socks) == -1)
        goto done;

    if (!path)
        goto done;

    if (access(path, X_OK) != 0)
        goto done;

    if (pipe(stdin_pipe) == -1)
        goto done;

    pid = fork();

    if (pid < 0)
        goto done;

    /* If child. */
    if (pid == 0)
    {
        dup2(stdin_pipe[0], STDIN_FILENO);
        close(stdin_pipe[1]);

        char* argv[2] = {(char*)path, NULL};

        execv(path, argv);
        abort();
    }

    close(socks[1]);
    close(stdin_pipe[0]);

    *child_fd = stdin_pipe[1];

    globals.sock = socks[0];
    globals.child_sock = socks[1];

    ret = pid;

done:
    return ret;
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

int main(int argc, const char* argv[])
{
    arg0 = argv[0];
    pid_t pid;
    int child_fd;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s program\n", argv[0]);
        exit(1);
    }

    if ((pid = exec(argv[1], &child_fd)) == -1)
    {
        fprintf(stderr, "%s: failed to execute %s\n", argv[0], argv[1]);
        exit(1);
    }

    /* Initialize the child process. */
    ve_init_child(child_fd);

    ve_msg_new_thread(0);

    /* Terminate the child process. */
    ve_terminate_child();

    return 0;
}

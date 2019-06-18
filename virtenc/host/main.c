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

pid_t exec(const char* path, int fds[2])
{
    pid_t ret = -1;
    pid_t pid;
    int stdout_pipe[2]; /* child's stdout pipe */
    int stdin_pipe[2];  /* child's stdin pipe */
    int socks[2];

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, socks) == -1)
        goto done;

    if (!path)
        goto done;

    if (access(path, X_OK) != 0)
        goto done;

    if (pipe(stdout_pipe) == -1)
        goto done;

    if (pipe(stdin_pipe) == -1)
        goto done;

    pid = fork();

    if (pid < 0)
        goto done;

    /* If child. */
    if (pid == 0)
    {
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdout_pipe[0]);
        close(socks[0]);

        dup2(stdin_pipe[0], STDIN_FILENO);
        close(stdin_pipe[1]);

        char* argv[2] = {(char*)path, NULL};

        execv(path, argv);
        abort();
    }

    close(socks[1]);
    close(stdout_pipe[1]);
    close(stdin_pipe[0]);

    fds[0] = stdout_pipe[0];
    fds[1] = stdin_pipe[1];

    globals.sock = socks[0];
    globals.child_sock = socks[1];

    ret = pid;

done:
    return ret;
}

int handle_print(int fds[2], size_t size, bool* eof)
{
    int ret = -1;
    ve_msg_print_in_t* in = NULL;
    ve_msg_print_out_t out;
    const ve_msg_type_t type = VE_MSG_PRINT;

    if (eof)
        *eof = true;

    if (!eof)
        goto done;

    if (!(in = malloc(size)))
        goto done;

    if (ve_recv_n(fds[0], in, size, eof) != 0)
        goto done;

    out.ret = (write(OE_STDOUT_FILENO, in->data, size) == -1) ? -1 : 0;

    if (ve_send_msg(fds[1], type, &out, sizeof(out)) != 0)
        goto done;

    ret = 0;

done:

    if (in)
        free(in);

    return ret;
}

void handle_messages(int fds[2])
{
    for (;;)
    {
        ve_msg_type_t type;
        size_t size;
        bool eof;

        if (ve_recv_msg(fds[0], &type, &size, &eof) != 0)
        {
            if (eof)
            {
                printf("enclave exited (1)\n");
                return;
            }

            err("ve_recv_msg() failed");
        }

        switch (type)
        {
            case VE_MSG_PRINT:
            {
                if (handle_print(fds, size, &eof) != 0)
                {
                    if (eof)
                    {
                        printf("enclave exited (2)\n");
                        return;
                    }

                    err("handle_print() failed");
                }
                break;
            }
            default:
            {
                err("unhandled: %u", type);
            }
        }
    }
}

int ve_init_child(int fds[2])
{
    int ret = -1;
    ve_msg_init_in_t in;
    ve_msg_init_out_t out;
    const ve_msg_type_t type = VE_MSG_INIT;
    bool eof;

    in.sock = globals.child_sock;

    if (ve_send_msg(fds[1], type, &in, sizeof(in)) != 0)
        goto done;

    if (ve_recv_msg_by_type(fds[0], type, &out, sizeof(out), &eof) != 0)
        goto done;

    /* Test the socket connection between child and parent. */
    {
        int sock = -1;
        bool eof;

        if (ve_recv_n(globals.sock, &sock, sizeof(sock), &eof) != 0)
            err("init failed: read of sock failed");

        if (sock != globals.child_sock)
            err("init failed: sock confirm failed");
    }

    printf("init response: ret=%d\n", out.ret);

    ret = 0;

done:
    return ret;
}

int ve_terminate_child(int fds[2])
{
    int ret = -1;
    ve_msg_terminate_in_t in;
    ve_msg_terminate_out_t out;
    const ve_msg_type_t type = VE_MSG_TERMINATE;
    bool eof;

    in.status = 0;

    if (ve_send_msg(fds[1], type, &in, sizeof(in)) != 0)
        goto done;

    if (ve_recv_msg_by_type(fds[0], type, &out, sizeof(out), &eof) != 0)
        goto done;

    printf("terminate response: ret=%d\n", out.ret);

    ret = 0;

done:
    return ret;
}

int ve_msg_new_thread(int fds[2], int tcs)
{
    int ret = -1;
    ve_msg_new_thread_in_t in;
    ve_msg_new_thread_out_t out;
    const ve_msg_type_t type = VE_MSG_NEW_THREAD;
    bool eof;

    in.tcs = tcs;

    if (ve_send_msg(fds[1], type, &in, sizeof(in)) != 0)
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

    if (ve_recv_msg_by_type(fds[0], type, &out, sizeof(out), &eof) != 0)
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
    int fds[2];

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s program\n", argv[0]);
        exit(1);
    }

    if ((pid = exec(argv[1], fds)) == -1)
    {
        fprintf(stderr, "%s: failed to execute %s\n", argv[0], argv[1]);
        exit(1);
    }

    /* Initialize the child process. */
    ve_init_child(fds);

    ve_msg_new_thread(fds, 0);

    /* Terminate the child process. */
    ve_terminate_child(fds);

    return 0;
}

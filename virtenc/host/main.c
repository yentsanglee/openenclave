#include <openenclave/bits/defs.h>
#include <openenclave/internal/syscall/unistd.h>
#include "../common/msg.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const char* arg0;

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

    if (!path)
        goto done;

    if (access(path, X_OK) != 0)
        goto done;

    if (pipe(fds) == -1)
        goto done;

    pid = fork();

    if (pid < 0)
        goto done;

    /* If child. */
    if (pid == 0)
    {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        dup2(fds[0], STDIN_FILENO);
        dup2(fds[1], STDOUT_FILENO);

        char* argv[2] = {(char*)path, NULL};

        execv(path, argv);
        abort();
    }

    ret = pid;

done:
    return ret;
}

int handle_print_in(int fds[2], size_t size)
{
    int ret = -1;
    ve_msg_print_in_t* in = NULL;
    ve_msg_print_out_t out;

    if (!(in = malloc(size)))
        goto done;

    if (ve_recv_n(fds[0], in, size) != 0)
        goto done;

    printf("{%.*s}\n", (int)size, in->data);
    out.ret = (write(OE_STDOUT_FILENO, in->data, size) == -1) ? -1 : 0;

    if (ve_send_msg(fds[1], VE_MSG_PRINT_OUT, &out, sizeof(out)) != 0)
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

        printf("<<<<<<<<<<<<<\n");
        if (ve_recv_msg(fds[0], &type, &size) != 0)
            err("ve_recv_msg() failed");
        printf(">>>>>>>>>>>>>\n");

        switch (type)
        {
            case VE_MSG_PRINT_IN:
            {
                printf("case VE_MSG_PRINT_IN\n");
                fflush(stdout);
                if (handle_print_in(fds, size) != 0)
                    err("handle_print_in() failed");
                break;
            }
            default:
            {
                err("unhandled: %u", type);
            }
        }
    }
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

    handle_messages(fds);

    return 0;
}

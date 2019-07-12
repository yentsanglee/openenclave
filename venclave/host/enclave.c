// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "enclave.h"
#include <assert.h>
#include <errno.h>
#include <openenclave/internal/raise.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "../common/io.h"
#include "calls.h"
#include "elf.h"
#include "err.h"
#include "heap.h"
#include "hostmalloc.h"

#define VE_PAGE_SIZE 4096

#define MAGIC 0xd77aba04

extern const char* __ve_arg0;
extern const char* __ve_vproxyhost_path;

typedef struct _thread
{
    int sock;
    int child_sock;
    uint64_t tcs;
    bool busy;
} thread_t;

struct _ve_enclave
{
    uint32_t magic;
    int pid;
    int sock;
    int child_sock;

    ve_heap_t* heap;

    thread_t threads[MAX_THREADS];
    size_t nthreads;
    pthread_spinlock_t lock;

    void* data;
};

void ve_enclave_set_data(ve_enclave_t* enclave, void* data)
{
    if (enclave)
        enclave->data = data;
}

void* ve_enclave_get_data(ve_enclave_t* enclave)
{
    return enclave ? enclave->data : NULL;
}

ve_enclave_t* ve_enclave_cast(void* ptr)
{
    ve_enclave_t* enclave = (ve_enclave_t*)ptr;
    return (enclave && enclave->magic == MAGIC) ? enclave : NULL;
}

static int _init_child(
    ve_enclave_t* enclave,
    int child_fd,
    int child_sock,
    ve_elf_info_t* elf_info)
{
    int ret = -1;
    ve_init_arg_t arg;
    int retval;

    if (!enclave || child_fd < 0 || child_sock < 0 || !elf_info)
        goto done;

    if (!__ve_vproxyhost_path || strlen(__ve_vproxyhost_path) >= OE_PATH_MAX)
        goto done;

    memset(&arg, 0, sizeof(arg));
    arg.magic = VE_INIT_ARG_MAGIC;
    arg.sock = child_sock;
    arg.shmid = enclave->heap->shmid;
    arg.shmaddr = enclave->heap->shmaddr;
    arg.shmsize = enclave->heap->shmsize;
    arg.elf_info = *elf_info;
    strcpy(arg.vproxyhost_path, __ve_vproxyhost_path);

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

        fprintf(
            stderr,
            "%s(%u): %s(): execv() failed\n",
            __FILE__,
            __LINE__,
            __FUNCTION__);
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
        uint64_t retval;

        if (ve_call(
                sock,
                VE_FUNC_TERMINATE_THREAD,
                &retval,
                0,
                0,
                0,
                0,
                0,
                0,
                ve_call_handler,
                enclave) != 0)
        {
            goto done;
        }

        close(sock);
        close(child_sock);
    }

    /* Terminate the main enclave thread. */
    {
        const int sock = enclave->sock;
        const int child_sock = enclave->child_sock;
        uint64_t retval;

        if (ve_call(
                sock,
                VE_FUNC_TERMINATE,
                &retval,
                0,
                0,
                0,
                0,
                0,
                0,
                ve_call_handler,
                enclave) != 0)
        {
            goto done;
        }

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
    if (ve_call_send(
            enclave->sock, VE_FUNC_ADD_THREAD, tcs, stack_size, 0, 0, 0, 0) !=
        0)
    {
        goto done;
    }

    /* Send the fd to the enclave after send but before receive. */
    if (send_fd(enclave->sock, socks[1]) != 0)
        goto done;

    /* Receive the response. */
    {
        uint64_t retval;

        if (ve_call_recv(enclave->sock, &retval, ve_call_handler, enclave) != 0)
        {
            goto done;
        }

        if (retval != 0)
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

static int _call_post_init(ve_enclave_t* enclave)
{
    int ret = -1;
    uint64_t retval;
    int sock;

    sock = enclave->sock;

    if (ve_call(
            enclave->sock,
            VE_FUNC_POST_INIT,
            &retval,
            0,
            0,
            0,
            0,
            0,
            0,
            ve_call_handler,
            enclave) != 0)
    {
        goto done;
    }

    if (retval != 0)
        goto done;

    ret = 0;

done:

    return ret;
}

int ve_enclave_create(
    const char* path,
    ve_heap_t* heap,
    ve_enclave_t** enclave_out)
{
    int ret = -1;
    ve_enclave_t* enclave = NULL;
    size_t stack_size;
    ve_elf_info_t elf_info;
    ve_enclave_settings_t settings;

    if (enclave_out)
        *enclave_out = NULL;

    if (!path)
        goto done;

    if (ve_get_elf_info(path, &elf_info) != 0)
        goto done;

    if (!(enclave = calloc(1, sizeof(ve_enclave_t))))
        goto done;

    enclave->magic = MAGIC;

    enclave->heap = heap;

    pthread_spin_init(&enclave->lock, PTHREAD_PROCESS_PRIVATE);

    if ((enclave->pid = _fork_exec_enclave(path, enclave, &elf_info)) == -1)
        goto done;

    /* Perform post-initialization (to invoke constructors). */
    if (_call_post_init(enclave) != 0)
        goto done;

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
    enclave = NULL;

    ret = 0;

done:

    if (enclave)
    {
        enclave->magic = 0;
        ;
        free(enclave);

        /* ATTN: terminate the enclave on error! */
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

    if (ve_call(
            sock,
            VE_FUNC_GET_SETTINGS,
            &retval,
            (uint64_t)settings,
            0,
            0,
            0,
            0,
            0,
            ve_call_handler,
            enclave) != 0)
    {
        goto done;
    }

    if (retval != 0)
        goto done;

    *buf = *settings;

    ret = 0;

done:

    if (settings)
        ve_host_free(settings);

    return ret;
}

static thread_t* _assign_thread(ve_enclave_t* enclave)
{
    thread_t* thread = NULL;

    pthread_spin_lock(&enclave->lock);
    {
        for (size_t i = 0; i < enclave->nthreads; i++)
        {
            if (!enclave->threads[i].busy)
            {
                thread = &enclave->threads[i];
                thread->busy = true;
                break;
            }
        }
    }
    pthread_spin_unlock(&enclave->lock);

    return thread;
}

static void _release_thread(ve_enclave_t* enclave, thread_t* thread)
{
    if (thread)
    {
        pthread_spin_lock(&enclave->lock);
        thread->busy = false;
        pthread_spin_unlock(&enclave->lock);
    }
}

oe_result_t ve_enclave_call(
    ve_enclave_t* enclave,
    ve_func_t func,
    uint64_t* retval,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    uint64_t arg6)
{
    oe_result_t result = OE_UNEXPECTED;
    int sock = -1;
    thread_t* thread = NULL;

    if (!enclave || func == VE_FUNC_RET || func == VE_FUNC_ERR)
        OE_RAISE(OE_INVALID_PARAMETER);

    if (!(thread = _assign_thread(enclave)))
        OE_RAISE(OE_OUT_OF_THREADS);

    if ((sock = thread->sock) < 0)
        OE_RAISE(OE_FAILURE);

    if (ve_call(
            sock,
            func,
            retval,
            arg1,
            arg2,
            arg3,
            arg4,
            arg5,
            arg6,
            ve_call_handler,
            enclave) != 0)
    {
        OE_RAISE(OE_FAILURE);
    }

    result = OE_OK;

done:

    _release_thread(enclave, thread);

    return result;
}

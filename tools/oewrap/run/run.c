// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <limits.h>
#include <openenclave/host.h>
#include <stdio.h>
#include <unistd.h>
#include "wrap_u.h"

static int _serialize_argv(
    int argc,
    const char* argv[],
    void** argv_buf,
    size_t* argv_buf_size)
{
    int ret = -1;
    char** new_argv = NULL;
    size_t array_size;
    size_t alloc_size;

    if (argv_buf)
        *argv_buf = NULL;

    if (argv_buf_size)
        *argv_buf_size = 0;

    if (argc < 0 || !argv || !argv_buf || !argv_buf_size)
        goto done;

    /* Calculate the total allocation size. */
    {
        /* Calculate the size of the null-terminated array of pointers. */
        array_size = ((size_t)argc + 1) * sizeof(char*);

        alloc_size = array_size;

        /* Calculate the sizeo of the individual zero-terminated strings. */
        for (int i = 0; i < argc; i++)
            alloc_size += strlen(argv[i]) + 1;
    }

    /* Allocate space for the array followed by strings. */
    if (!(new_argv = (char**)malloc(alloc_size)))
        goto done;

    /* Copy each string. */
    {
        char* p = (char*)&new_argv[argc + 1];
        size_t offset = array_size;

        for (int i = 0; i < argc; i++)
        {
            size_t size = strlen(argv[i]) + 1;

            memcpy(p, argv[i], size);
            new_argv[i] = (char*)offset;
            p += size;
            offset += size;
        }

        new_argv[argc] = NULL;
    }

    *argv_buf = new_argv;
    *argv_buf_size = alloc_size;
    new_argv = NULL;

    ret = 0;

done:

    if (new_argv)
        free(new_argv);

    return ret;
}

int run_enclave(const char* path, int argc, const char* argv[])
{
    oe_result_t r;
    oe_enclave_t* enc = NULL;
    const uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;
    const oe_enclave_type_t type = OE_ENCLAVE_TYPE_SGX;
    int retval;
    void* argv_buf = NULL;
    size_t argv_buf_size = 0;

    /* Create the enclave. */
    {
        r = oe_create_wrap_enclave(path, type, flags, NULL, 0, &enc);

        if (r != OE_OK)
        {
            fprintf(stderr, "%s: oe_create_wrap_enclave()\n", path);
            exit(1);
        }
    }

    /* Serialize the arguments. */
    if (_serialize_argv(argc, argv, &argv_buf, &argv_buf_size) != 0)
    {
        fprintf(stderr, "%s: _serialize_argv()\n", argv[0]);
        exit(1);
    }

    /* Initialize the enclave. */
    {
        char cwd[1024];

        if (!getcwd(cwd, sizeof(cwd)))
        {
            fprintf(stderr, "%s: oe_wrap_main_ecall()\n", argv[0]);
            exit(1);
        }

        r = oe_wrap_init(enc, &retval, path, cwd);

        if (r != OE_OK || retval != 0)
        {
            fprintf(stderr, "%s: oe_wrap_init()\n", argv[0]);
            exit(1);
        }
    }

    /* Run the enclave. */
    {
        r = oe_wrap_main_ecall(enc, &retval, argc, argv_buf, argv_buf_size);

        if (r != OE_OK)
        {
            fprintf(stderr, "%s: oe_wrap_main_ecall()\n", argv[0]);
            exit(1);
        }
    }

    /* Terminate the enclave. */
    {
        r = oe_terminate_enclave(enc);

        if (r != OE_OK)
        {
            fprintf(stderr, "%s: oe_terminate_enclave()\n", argv[0]);
            exit(1);
        }
    }

    free(argv_buf);

    return retval;
}

static const char* _basename(const char* path)
{
    char* slash = strrchr(path, '/');
    return slash ? (slash + 1) : path;
}

int main(int argc, const char* argv[])
{
    char path[PATH_MAX];

    const char* bn = _basename(argv[0]);

    if (strcmp(bn, "oerun") == 0)
    {
        if (argc < 2)
        {
            fprintf(stderr, "Usage: %s enclave args...\n", argv[0]);
            exit(1);
        }

        snprintf(path, sizeof(path), "%s", argv[1]);

        argc--;
        argv++;
    }
    else
    {
        snprintf(path, sizeof(path), "%s.enc", argv[0]);
    }

    return run_enclave(path, argc, argv);
}

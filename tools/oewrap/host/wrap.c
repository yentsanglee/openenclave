// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <limits.h>
#include <openenclave/host.h>
#include <stdio.h>
#include "wrap_u.h"

extern uint8_t __enc_data[];
extern size_t __enc_size;

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

int main(int argc, const char* argv[])
{
    oe_result_t r;
    oe_enclave_t* enc = NULL;
    const uint32_t flags = OE_ENCLAVE_FLAG_DEBUG;
    const oe_enclave_type_t type = OE_ENCLAVE_TYPE_SGX;
    int retval;
    void* argv_buf = NULL;
    size_t argv_buf_size = 0;
    const char* path = NULL;
    char buf[L_tmpnam];
    FILE* os = NULL;

    /* Create a temporary enclave file. */
    {
        size_t n = __enc_size;

        if (!(path = tmpnam(buf)))
        {
            fprintf(stderr, "%s: tmpnam():\n", argv[0]);
            exit(1);
        }

        if (!(os = fopen(path, "w")))
        {
            fprintf(stderr, "%s: fopen():\n", argv[0]);
            exit(1);
        }

        if (fwrite(__enc_data, 1, n, os) != n)
        {
            fprintf(stderr, "%s: fwrite():\n", argv[0]);
            exit(1);
        }

        fclose(os);
    }

    /* Create the enclave. */
    {
        r = oe_create_wrap_enclave(path, type, flags, NULL, 0, &enc);

        if (r != OE_OK)
        {
            fprintf(stderr, "%s: oe_create_test_hostfs_enclave()\n", argv[0]);
            exit(1);
        }
    }

    /* Serialize the arguments. */
    if (_serialize_argv(argc, argv, &argv_buf, &argv_buf_size) != 0)
    {
        fprintf(stderr, "%s: _serialize_argv()\n", argv[0]);
        exit(1);
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

    /* Remove the file. */
    remove(path);

    free(argv_buf);

    return retval;
}

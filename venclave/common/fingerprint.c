// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/internal/fingerprint.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int oe_compute_fingerprint(const char* path, oe_fingerprint_t* fingerprint)
{
    int ret = 1;
    FILE* is = NULL;

    if (fingerprint)
        memset(fingerprint, 0, sizeof(oe_fingerprint_t));

    if (!path || !fingerprint)
        goto done;

    /* Get the size of the file. */
    {
        struct stat buf;

        if (stat(path, &buf) != 0 || buf.st_size < 0)
            goto done;

        fingerprint->size = (size_t)buf.st_size;
    }

    /* Open the input file */
    if (!(is = fopen(path, "r")))
        goto done;

    /* Compute SHA-256 of the input file. */
    {
        char buf[4096];
        size_t n;
        SHA256_CTX ctx;

        if (!SHA256_Init(&ctx))
            goto done;

        while ((n = fread(buf, 1, sizeof(buf), is)) > 0)
        {
            if (!SHA256_Update(&ctx, buf, n))
                goto done;
        }

        if (!SHA256_Final(fingerprint->hash, &ctx))
            goto done;
    }

    ret = 0;

done:

    if (is)
        fclose(is);

    return ret;
}

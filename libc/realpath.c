// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char* realpath(const char* path, char resolved_path[PATH_MAX])
{
    char buf[PATH_MAX];
    const char* in[PATH_MAX];
    size_t nin = 0;
    const char* out[PATH_MAX];
    size_t nout = 0;
    char resolved[PATH_MAX];

    if (!path)
    {
        errno = EINVAL;
        return NULL;
    }

    if (path[0] == '/')
    {
        if (strlcpy(buf, path, sizeof(buf)) >= sizeof(buf))
        {
            errno = ENAMETOOLONG;
            return NULL;
        }
    }
    else
    {
        char cwd[PATH_MAX];

        if (!getcwd(cwd, sizeof(cwd)))
            return NULL;

        if (strlcpy(buf, cwd, sizeof(buf)) >= sizeof(buf))
        {
            errno = ENAMETOOLONG;
            return NULL;
        }

        if (strlcat(buf, "/", sizeof(buf)) >= sizeof(buf))
        {
            errno = ENAMETOOLONG;
            return NULL;
        }

        if (strlcat(buf, path, sizeof(buf)) >= sizeof(buf))
        {
            errno = ENAMETOOLONG;
            return NULL;
        }
    }

    /* Split the path into elements. */
    {
        char* p;
        char* save;

        in[nin++] = "/";

        for (p = strtok_r(buf, "/", &save); p; p = strtok_r(NULL, "/", &save))
            in[nin++] = p;
    }

    /* Normalize the path. */
    for (size_t i = 0; i < nin; i++)
    {
        /* Skip "." elements. */
        if (strcmp(in[i], ".") == 0)
            continue;

        /* If "..", remove previous element. */
        if (strcmp(in[i], "..") == 0)
        {
            if (nout)
                nout--;
            continue;
        }

        out[nout++] = in[i];
    }

    /* Build the resolved path. */
    {
        *resolved = '\0';

        for (size_t i = 0; i < nout; i++)
        {
            if (strlcat(resolved, out[i], PATH_MAX) >= PATH_MAX)
            {
                errno = ENAMETOOLONG;
                return NULL;
            }

            if (i != 0 && i + 1 != nout)
            {
                if (strlcat(resolved, "/", PATH_MAX) >= PATH_MAX)
                {
                    errno = ENAMETOOLONG;
                    return NULL;
                }
            }
        }
    }

    if (resolved_path)
    {
        if (strlcpy(resolved_path, resolved, PATH_MAX) >= PATH_MAX)
        {
            errno = ENAMETOOLONG;
            return NULL;
        }

        return resolved_path;
    }
    else
    {
        char* p = strdup(resolved);

        if (!p)
            errno = ENOMEM;

        return p;
    }

    return NULL;
}

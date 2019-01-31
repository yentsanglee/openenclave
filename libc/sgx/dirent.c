// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <dirent.h>
#include <errno.h>
#include <openenclave/internal/fs.h>
#include <stdio.h>
#include <string.h>

DIR* opendir(const char* name)
{
    DIR* dirp;

    if (!(dirp = (DIR*)oe_opendir(name)))
        errno = oe_errno;

    return dirp;
}

struct dirent* readdir(DIR* dirp)
{
    struct dirent* entry;

    /* Verify that 'struct dirent' and 'struct oe_dirent' are identical. */
    typedef struct dirent X;
    typedef struct oe_dirent Y;
    struct dirent* x = NULL;
    struct oe_dirent* y = NULL;
    OE_STATIC_ASSERT(sizeof(x->d_ino) == sizeof(y->d_ino));
    OE_STATIC_ASSERT(sizeof(x->d_off) == sizeof(y->d_off));
    OE_STATIC_ASSERT(sizeof(x->d_reclen) == sizeof(y->d_reclen));
    OE_STATIC_ASSERT(sizeof(x->d_type) == sizeof(y->d_type));
    OE_STATIC_ASSERT(sizeof(x->d_name) == sizeof(y->d_name));
    OE_STATIC_ASSERT(sizeof(struct dirent) == sizeof(struct oe_dirent));
    OE_STATIC_ASSERT(OE_OFFSETOF(X, d_ino) == OE_OFFSETOF(Y, d_ino));
    OE_STATIC_ASSERT(OE_OFFSETOF(X, d_off) == OE_OFFSETOF(Y, d_off));
    OE_STATIC_ASSERT(OE_OFFSETOF(X, d_reclen) == OE_OFFSETOF(Y, d_reclen));
    OE_STATIC_ASSERT(OE_OFFSETOF(X, d_type) == OE_OFFSETOF(Y, d_type));
    OE_STATIC_ASSERT(OE_OFFSETOF(X, d_name) == OE_OFFSETOF(Y, d_name));

    if (!(entry = (struct dirent*)oe_readdir((oe_device_t*)dirp)))
        errno = oe_errno;

    return entry;
}

int closedir(DIR* dirp)
{
    int ret;

    if ((ret = oe_closedir((oe_device_t*)dirp)) != 0)
        errno = oe_errno;

    return ret;
}

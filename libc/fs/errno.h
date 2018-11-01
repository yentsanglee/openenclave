// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _fs_errno_h
#define _fs_errno_h

/* Subset of supported errno's. */
typedef enum _fs_errno {
    FS_EOK = 0,           /* Success */
    FS_EPERM = 1,         /* Operation not permitted */
    FS_ENOENT = 2,        /* No such file or directory */
    FS_EINTR = 4,         /* Interrupted system call */
    FS_EIO = 5,           /* I/O error */
    FS_EBADF = 9,         /* Bad file number */
    FS_ENOMEM = 12,       /* Out of memory */
    FS_EACCES = 13,       /* Permission denied */
    FS_EBUSY = 16,        /* Device or resource busy */
    FS_EEXIST = 17,       /* File exists */
    FS_ENOTDIR = 20,      /* Not a directory */
    FS_EISDIR = 21,       /* Is a directory */
    FS_EINVAL = 22,       /* Invalid argument */
    FS_ENFILE = 23,       /* File table overflow */
    FS_EMFILE = 24,       /* Too many open files */
    FS_EFBIG = 27,        /* File too large */
    FS_ENOSPC = 28,       /* No space left on device */
    FS_ESPIPE = 29,       /* Illegal seek */
    FS_EROFS = 30,        /* Readonly file system */
    FS_EMLINK = 31,       /* Too many links */
    FS_ERANGE = 34,       /* Numerical result of out range */
    FS_ENAMETOOLONG = 36, /* File name too long */
    FS_ENOSYS = 38,       /* Function not implemented */
    FS_ENOTEMPTY = 39,    /* Directory not empty */
    FS_ENOLINK = 67,      /* Link has been severed */
    FS_EOVERFLOW = 75,    /* Value too large for defined data type */
    FS_EBADFD = 77,       /* File descriptor in bad state */
    FS_ENOTSOCK = 88,     /* Socket operation on nonsocket */
} fs_errno_t;

#endif /* _fs_errno_h */

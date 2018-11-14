// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <openenclave/internal/enclavelibc.h>
#include <string.h>

struct pair
{
    int num;
    const char* str;
};

static struct pair _pairs[] = {
    {ENCLIBC_EPERM, "Operation not permitted"},
    {ENCLIBC_ENOENT, "No such file or directory"},
    {ENCLIBC_ESRCH, "No such process"},
    {ENCLIBC_EINTR, "Interrupted system call"},
    {ENCLIBC_EIO, "Input/output error"},
    {ENCLIBC_ENXIO, "No such device or address"},
    {ENCLIBC_E2BIG, "Argument list too long"},
    {ENCLIBC_ENOEXEC, "Exec format error"},
    {ENCLIBC_EBADF, "Bad file descriptor"},
    {ENCLIBC_ECHILD, "No child processes"},
    {ENCLIBC_EAGAIN, "Resource temporarily unavailable"},
    {ENCLIBC_ENOMEM, "Cannot allocate memory"},
    {ENCLIBC_EACCES, "Permission denied"},
    {ENCLIBC_EFAULT, "Bad address"},
    {ENCLIBC_ENOTBLK, "Block device required"},
    {ENCLIBC_EBUSY, "Device or resource busy"},
    {ENCLIBC_EEXIST, "File exists"},
    {ENCLIBC_EXDEV, "Invalid cross-device link"},
    {ENCLIBC_ENODEV, "No such device"},
    {ENCLIBC_ENOTDIR, "Not a directory"},
    {ENCLIBC_EISDIR, "Is a directory"},
    {ENCLIBC_EINVAL, "Invalid argument"},
    {ENCLIBC_ENFILE, "Too many open files in system"},
    {ENCLIBC_EMFILE, "Too many open files"},
    {ENCLIBC_ENOTTY, "Inappropriate ioctl for device"},
    {ENCLIBC_ETXTBSY, "Text file busy"},
    {ENCLIBC_EFBIG, "File too large"},
    {ENCLIBC_ENOSPC, "No space left on device"},
    {ENCLIBC_ESPIPE, "Illegal seek"},
    {ENCLIBC_EROFS, "Read-only file system"},
    {ENCLIBC_EMLINK, "Too many links"},
    {ENCLIBC_EPIPE, "Broken pipe"},
    {ENCLIBC_EDOM, "Numerical argument out of domain"},
    {ENCLIBC_ERANGE, "Numerical result out of range"},
    {ENCLIBC_EDEADLK, "Resource deadlock avoided"},
    {ENCLIBC_ENAMETOOLONG, "File name too long"},
    {ENCLIBC_ENOLCK, "No locks available"},
    {ENCLIBC_ENOSYS, "Function not implemented"},
    {ENCLIBC_ENOTEMPTY, "Directory not empty"},
    {ENCLIBC_ELOOP, "Too many levels of symbolic links"},
    {ENCLIBC_EWOULDBLOCK, "Resource temporarily unavailable"},
    {ENCLIBC_ENOMSG, "No message of desired type"},
    {ENCLIBC_EIDRM, "Identifier removed"},
    {ENCLIBC_ECHRNG, "Channel number out of range"},
    {ENCLIBC_EL2NSYNC, "Level 2 not synchronized"},
    {ENCLIBC_EL3HLT, "Level 3 halted"},
    {ENCLIBC_EL3RST, "Level 3 reset"},
    {ENCLIBC_ELNRNG, "Link number out of range"},
    {ENCLIBC_EUNATCH, "Protocol driver not attached"},
    {ENCLIBC_ENOCSI, "No CSI structure available"},
};

static const int _npairs = sizeof(_pairs) / sizeof(_pairs[0]);

static const char _unknown[] = "Unknown error";

char* enclibc_strerror(int errnum)
{
    for (size_t i = 0; i < _npairs; i++)
    {
        if (_pairs[i].num == errnum)
            return (char*)_pairs[i].str;
    }

    return (char*)_unknown;
}

int enclibc_strerror_r(int errnum, char* buf, size_t buflen)
{
    const char* str = NULL;

    for (size_t i = 0; i < _npairs; i++)
    {
        if (_pairs[i].num == errnum)
        {
            str = _pairs[i].str;
            break;
        }
    }

    if (!str)
        str = _unknown;

    return strlcpy(buf, str, buflen) >= buflen ? ENCLIBC_ERANGE : 0;
}

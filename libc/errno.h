#ifndef _oe_errno_h
#define _oe_errno_h

/* Subset of supported errno's. */
typedef enum _oe_errno
{
    OE_EOK           = 0, /* Success */
    OE_EPERM         = 1, /* Operation not permitted */
    OE_ENOENT        = 2, /* No such file or directory */
    OE_EINTR         = 4, /* Interrupted system call */
    OE_EIO           = 5, /* I/O error */
    OE_EBADF         = 9, /* Bad file number */
    OE_ENOMEM       = 12, /* Out of memory */
    OE_EACCES       = 13, /* Permission denied */
    OE_EBUSY        = 16, /* Device or resource busy */
    OE_EEXIST       = 17, /* File exists */
    OE_ENOTDIR      = 20, /* Not a directory */
    OE_EISDIR       = 21, /* Is a directory */
    OE_EINVAL       = 22, /* Invalid argument */
    OE_ENFILE       = 23, /* File table overflow */
    OE_EMFILE       = 24, /* Too many open files */
    OE_EFBIG        = 27, /* File too large */
    OE_ENOSPC       = 28, /* No space left on device */
    OE_ESPIPE       = 29, /* Illegal seek */
    OE_EROFS        = 30, /* Readonly file system */
    OE_EMLINK       = 31, /* Too many links */
    OE_ENAMETOOLONG = 36, /* File name too long */
    OE_ENOSYS       = 38, /* Function not implemented */
    OE_ENOTEMPTY    = 39, /* Directory not empty */
    OE_ENOLINK      = 67, /* Link has been severed */
    OE_EOVERFLOW    = 75, /* Value too large for defined data type */
    OE_EBADFD       = 77, /* File descriptor in bad state */
    OE_ENOTSOCK     = 88, /* Socket operation on nonsocket */
}
oe_errno_t;

#endif /* _oe_errno_h */

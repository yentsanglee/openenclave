#ifndef _FS_SYSCALL_H
#define _FS_SYSCALL_H

#include "errno.h"

int fs_handle_syscall(
    long num,
    long arg1,
    long arg2,
    long arg3,
    long arg4,
    long arg5,
    long arg6,
    long* ret_out,
    fs_errno_t* err_out);

#endif /* _FS_SYSCALL_H */

#ifndef _OE_HOSTSYSCALL_H
#define _OE_HOSTSYSCALL_H

typedef struct _oe_host_syscall_args
{
    long ret;
    long num;
    long arg1;
    long arg2;
    long arg3;
    long arg4;
    long arg5;
    long arg6;
} oe_host_syscall_args_t;

#endif /* _OE_HOSTSYSCALL_H */

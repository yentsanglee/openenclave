// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _VE_ENCLAVE_PROCESS_H
#define _VE_ENCLAVE_PROCESS_H

#include "common.h"

#define VE_CLONE_VM 0x00000100
#define VE_CLONE_FS 0x00000200
#define VE_CLONE_FILES 0x00000400
#define VE_CLONE_SIGHAND 0x00000800
#define VE_CLONE_PTRACE 0x00002000
#define VE_CLONE_VFORK 0x00004000
#define VE_CLONE_PARENT 0x00008000
#define VE_CLONE_THREAD 0x00010000
#define VE_CLONE_NEWNS 0x00020000
#define VE_CLONE_SYSVSEM 0x00040000
#define VE_CLONE_SETTLS 0x00080000
#define VE_CLONE_PARENT_SETTID 0x00100000
#define VE_CLONE_CHILD_CLEARTID 0x00200000
#define VE_CLONE_DETACHED 0x00400000
#define VE_CLONE_UNTRACED 0x00800000
#define VE_CLONE_CHILD_SETTID 0x01000000
#define VE_CLONE_NEWCGROUP 0x02000000
#define VE_CLONE_NEWUTS 0x04000000
#define VE_CLONE_NEWIPC 0x08000000
#define VE_CLONE_NEWUSER 0x10000000
#define VE_CLONE_NEWPID 0x20000000
#define VE_CLONE_NEWNET 0x40000000
#define VE_CLONE_IO 0x80000000

#define VE_WEXITSTATUS(status) (((status)&0xff00) >> 8)

OE_NO_RETURN void ve_exit(int status);

OE_NO_RETURN void ve_abort(void);

int ve_gettid(void);

int ve_getpid(void);

int ve_waitpid(int pid, int* status, int options);

/* Get the real base address of this process. */
void* ve_get_baseaddr(void);

void* ve_get_elf_header(void);

/* Create a new process (used to implement thread creation). */
int ve_clone(
    int (*fn)(void*),
    void* child_stack,
    int flags,
    void* arg,
    int* ptid,
    void* tls,
    volatile int* ctid);

int ve_fork(void);

int ve_execv(const char* path, char* const argv[]);

#endif /* _VE_ENCLAVE_PROCESS_H */

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _ENCLIBC_SETJMP_H
#define _ENCLIBC_SETJMP_H

#include "bits/common.h"

ENCLIBC_EXTERNC_BEGIN

typedef struct _oe_jmp_buf
{
    // These are the registers that are preserved across function calls
    // according to the 'System V AMD64 ABI' calling conventions:
    // RBX, RSP, RBP, R12, R13, R14, R15. In addition, enclibc_setjmp() saves
    // the RIP register (instruction pointer) to know where to jump back to).
    uint64_t rsp;
    uint64_t rbp;
    uint64_t rip;
    uint64_t rbx;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
} enclibc_jmp_buf[1];

int enclibc_setjmp(enclibc_jmp_buf env);

void enclibc_longjmp(enclibc_jmp_buf env, int val);

#if defined(ENCLIBC_NEED_STDC_NAMES)

typedef enclibc_jmp_buf jmp_buf;

ENCLIBC_INLINE
int setjmp(jmp_buf env)
{
    return enclibc_setjmp(env);
}

ENCLIBC_INLINE
void longjmp(jmp_buf env, int val)
{
    return enclibc_longjmp(env, val);
}

#endif /* defined(ENCLIBC_NEED_STDC_NAMES) */

ENCLIBC_EXTERNC_END

#endif /* _ENCLIBC_SETJMP_H */

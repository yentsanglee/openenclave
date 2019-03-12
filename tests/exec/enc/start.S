// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

.globl oe_call_start
.type oe_call_start, @function
oe_call_start:
.cfi_startproc

    mov %rdx, %rcx /* RCX <= address of callback. */
    mov %rsi, %rdx /* RDX <= stack_end */
    mov %rdi, %rsp /* RSP <= stack */
    jmpq *%rcx
    hlt

.cfi_endproc

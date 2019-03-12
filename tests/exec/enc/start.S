// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

/*
.globl my_start
.type my_start, @function
my_start:
.cfi_startproc

    // rdi: main
    // rsi: argc
    // rdx: argv
    // rcx: init
    // r8: fini
    // r9: stack_end

    xor    %ebp,%ebp
    mov    %rdx,%r9
    pop    %rsi
    mov    %rsp,%rdx
    and    $0xfffffffffffffff0,%rsp
    push   %rax
    push   %rsp
    mov    $0xFFFF, %r8
    mov    $0x1111, %rcx
    mov    $0xAAAA, %rdi
    callq  __oe_libc_start_main
    hlt
    nopw   0x0(%rax,%rax,1)

.cfi_endproc
*/

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

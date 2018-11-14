.text
        .globl  _rdrand
_rdrand:
 pushq   %rbp
 movq    %rsp, %rbp
111:
        rdrand %rax
        jnc 111
leave
ret


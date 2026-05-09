.section .text
.global waks_save_state
.global waks_load_state

waks_save_state:
    # Save Return Address (PC)
    # AT&T syntax: mov (src), dest
    movq (%rsp), %rax
    movq %rax, global_panic_env@GOTPCREL(%rip)
    movq global_panic_env@GOTPCREL(%rip), %rdx
    movq %rax, 8(%rdx)

    # Save Stack Pointer (SP) as it was before the call
    leaq 8(%rsp), %rax
    movq %rax, 0(%rdx)

    # Save Callee-Saved Registers
    movq %rbx, 16(%rdx)
    movq %rbp, 24(%rdx)
    movq %r12, 32(%rdx)
    movq %r13, 40(%rdx)
    movq %r14, 48(%rdx)
    movq %r15, 56(%rdx)

    xorq %rax, %rax
    ret

waks_load_state:
    movq global_panic_env@GOTPCREL(%rip), %rdx
    # Restore RSP and RBP
    movq 0(%rdx), %rsp
    movq 24(%rdx), %rbp

    #  Restore other Callee-Saved Registers
    movq 16(%rdx), %rbx
    movq 32(%rdx), %r12
    movq 40(%rdx), %r13
    movq 48(%rdx), %r14
    movq 56(%rdx), %r15

    # Restore error code to EAX (return value)
    movl 72(%rdx), %eax

    # Jump back to the saved PC
    jmp *8(%rdx)

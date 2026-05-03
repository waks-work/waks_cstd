section .text 
global waks_save_state
global waks_load_state
extern global_panic_env

waks_save_state:
    ;; at function call the pc is at rsp 
    mov rax, [rsp]
    mov [rel global_panic_env + 8], rax ;; the offset 8 to .ip

    ;; save the rsp as it was before the  call (rsp+8)
    lea rax, [rsp + 8]
    mov [rel global_panic_env + 0], rax ;; offset 0 is at .sp
    
    ;; save the rbp and other callee-saved registers
    mov [rel global_panic_env + 16], rbx
    mov [rel global_panic_env + 24], rbp
    mov [rel global_panic_env + 32], r12
    mov [rel global_panic_env + 40], r13
    mov [rel global_panic_env + 48], r14
    mov [rel global_panic_env + 56], r15

    xor rax, rax  ; return 0

    ret

waks_load_state:
   mov rsp, [rel global_panic_env] ; restore the rsp
   mov rbp, [rel global_panic_env + 24]

    ;; restore the rbp and other callee-saved registers
    mov rbx, [rel global_panic_env + 16]
    mov r12, [rel global_panic_env + 32]
    mov r13, [rel global_panic_env + 40]
    mov r14, [rel global_panic_env + 48]
    mov r15, [rel global_panic_env + 56] 

   ;; restore the error code to eax
   mov eax, [rel global_panic_env + 72] ; offset is at 16
   jmp [rel global_panic_env + 8]


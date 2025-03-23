DEFAULT REL
SECTION .text

global ASM_PFX(Shellcode)
ASM_PFX(Shellcode):
    push rdi
    push rax
    mov rdi, 0x555555555555555
    mov rax, 0x555555555555555
    call rax                
    mov rdi, 0x555555555555555
    call rax

    pop rax
    pop rdi

    ret

